/**
 * Version: $Id: mepLib.c,v 1.13 2004/09/23 15:20:27 abowman Exp $
 * 
 * Copyright (c) 1993, 1994 Synaptics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 *
 *
 **/

/*****************************************************************
 * The include file <mep_config.h> (included below) must be
 * provided by the local host implementation.  The file is used
 * to define the MepLib configuration options.  See the "MepLib
 * Porting Guide" for more info.
 */
#include <linux/kernel.h>
#include "mep_config.h"
#include "mepLib.h"
//disable dhsong, #include "SysDebug.h"


/*****************************************************************
 * The host configuration file <mep_config.h> must define exactly
 * one of the following syms:
 *
 *    MEP_CONFIG_INTERRUPT_MODE
 *    MEP_CONFIG_POLLED_MODE
 *    MEP_CONFIG_HYBRID_MODE
 *
 * The following #if statement will generate an error if this is
 * not the case.
 */
#if defined MEP_CONFIG_INTERRUPT_MODE
#  if defined MEP_CONFIG_POLLED_MODE || defined MEP_CONFIG_HYBRID_MODE
#    error More than one MEP configuration mode was defined!
#  endif
#elif defined MEP_CONFIG_POLLED_MODE
#  if defined MEP_CONFIG_INTERUPT_MODE || defined MEP_CONFIG_HYBRID_MODE
#    error More than one MEP configuration mode was defined!
#  else
#    define MEP_POLLED_STATE_MACHINE
#  endif
#elif defined MEP_CONFIG_HYBRID_MODE
#  if defined MEP_CONFIG_INTERUPT_MODE || defined MEP_CONFIG_POLLED_MODE
#    error More than one MEP configuration mode was defined!
#  else
#    define MEP_POLLED_STATE_MACHINE
#  endif
#else
#  error MEP configuration file must specify a configuration mode!
#endif


#if defined MEP_WAIT_FOR_CLK_PROTO
/* If the host configuration defined a replacement routine for
 * MEP_WAIT_FOR_CLK, it must also declare a prototype for the
 * routine which needs to appear here. */
MEP_WAIT_FOR_CLK_PROTO;
#endif

/*****************************************************************
 * Forward definitions:
 */
static void mepState_arbitrate(void);
static void setFlusherState(void);


#if defined NULL
#  define MEP_NULL NULL
#else
#  define MEP_NULL (0)
#endif

#if defined FALSE
#  define MEP_FALSE FALSE
#else
#  define MEP_FALSE (0)
#endif

#if defined TRUE
#  define MEP_TRUE TRUE
#else
#  define MEP_TRUE (!(MEP_FALSE))
#endif


/* MEP machine states are manipulated as function pointers. */
typedef void (*mepState_t)(void);

/*****************************************************************
 * Allow a host implementations to override this definition of
 * MEP_NEXT_STATE by defining their own in the configuration file
 * <mep_config.h>.  The only reason to do this would be for local
 * debug purposes.  Obviously, any local definition must always
 * include the statement:
 *
 *   mep_nextState = (state);
 */
#if !defined MEP_NEXT_STATE
#  if defined MEP_DEBUG
     /*-----------------------------------------------------------------*/
#    define MEP_NEXT_STATE(state, id) \
        do { \
            mep_nextState = (state); \
            mep_puts(id); \
        } while (0)
#  else /* !defined MEP_DEBUG */
     /*-----------------------------------------------------------------*/
#    define MEP_NEXT_STATE(state, id) \
        do { \
            mep_nextState = (state); \
        } while (0)
#  endif 
#endif /* !defined MEP_NEXT_STATE */


/* The length of a link flush operation (in bit-transfer operations) */
#define MEP_FLUSH_BITCOUNT 72

/* The initial state gets set by mep_init() */
static mepState_t     mep_nextState;

/* We track the current ACK signal state here. */
static unsigned char  mep_ackLevel;

/* These globals are used to maintain state data between
 * consecutive interrupt state machine invocations. */
static unsigned char  mep_bitMask;
static unsigned char  mep_parity;
static unsigned char  mep_pktLen;
static unsigned char *mep_pktPtr;
static unsigned char  mep_flushCount;
static mep_packet_t   mep_rxBuffer;

/* The transmit and receive 'queues' of length 1.  All actual
 * queuing should be done in the context of the host's RTOS or
 * application software since it is very likely to be
 * application-specific.  
 *
 * Note: the 'volatile' qualifier as used in this declaration
 * specifies that the pointer value itself is volatile, not the
 * object that it points to. */
static mep_packet_t * volatile mep_rxQ;
static mep_packet_t * volatile mep_txQ;

/* The callback function pointers are stored here.  Host code
 * must use the mepSetCallback() API to change these values. */
static void (*mep_rxCallback)(mep_err_t error);
static void (*mep_txCallback)(mep_err_t error);
static void (*mep_errCallback)(mep_err_t error);


#if defined MEP_DEBUG
  /*****************************************************************
   * Display the Least Significant nybble of the 'value' param as a
   * hex digit using the host-supplied mep_debug_putc() method.
   */
  static void dbg_toHex4(unsigned char value)
  {
      value &= 0x0f;
      value += '0';
      if (value > '9')
          value += 7;
//      mep_debug_putc(value);
  }

  /*****************************************************************
   * Display the 'value' param byte as two hex digits using the
   * host-supplied mep_debug_putc() method.
   */
  static void dbg_toHex8(unsigned char value)
  {
      dbg_toHex4(value>>4);
      dbg_toHex4(value);
  }

  /*****************************************************************
   * Display a string using the host-suplied debug mep_debug_putc()
   * method.  
   */
  void mep_puts(unsigned char *msg)
  {
  		/*
      if (msg) {
          while (*msg) {
              mep_debug_putc(*msg);
              msg++;
          }
      }
      */
  }

#endif  /* defined MEP_DEBUG */



#if defined MEP_USE_DEFAULT_CLK_WATCHER
#define TIMEOUT_CNT	5000
/*****************************************************************
 * This is the default CLK watcher code.  It will only be used if
 * the host configuration does not define anything else.  The
 * reason for the host to define a host-specific version would be
 * to implement something that implements timeouts in a
 * host-specific fashion.  Remember: If this routine is replaced
 * in a host configuration, a timeout error must return the value
 * MEP_TIMEOUT.
 */
static mep_err_t mep_waitForClkForever(unsigned char CLK_desiredLevel)
{
	int i = 0;
    do {
        /* Spinwait with NO timeout until the actual CLK level
         * matches the desired CLK level. */
		if(i++ == TIMEOUT_CNT)
		{
			return MEP_TIMEOUT;	/* 100ms */
		}
    } while ((mep_pl_getCLK()!=0) != ((CLK_desiredLevel)!=0));

    return(MEP_NOERR);
}
#endif /* MEP_USE_DEFAULT_CLK_WATCHER */



/*****************************************************************
 * For the benefit of mep_pl_getACK (see below), this routine is
 * used to set the ACK signal internally to MepLib.  
 *
 * WARNING: Except for the invocation inside this routine, mepLib
 * should never invoke mep_pl_setACK() directly!  Otherwise, the
 * mep_ackLevel variable might get out of sync with the actual
 * ACK state.
 */
static void _mep_pl_setACK(unsigned char logicLevel)
{
    mep_ackLevel = logicLevel;
    mep_pl_setACK(logicLevel);
}


static unsigned char mep_pl_getACK(void)
{
    return(mep_ackLevel);
}



/*****************************************************************
 * Returns a boolean MEP_TRUE if the MEP link is in phase P1.
 */
static unsigned char isLinkInP1(void)
{
    /* A P1 cycle is CLK=0, ACK=1 */
    return(!mep_pl_getCLK() && mep_pl_getACK());
}


/*****************************************************************
 * Returns a boolean MEP_TRUE if the MEP link is in phase P3.
 */
static unsigned char isLinkInP3(void)
{
    /* A P3 cycle is CLK=1, ACK=0 */
    return(mep_pl_getCLK() && !mep_pl_getACK());
}


/*****************************************************************
 * The MEP Machine state-handlers.                               *
 *                                                               *
 * These functions are invoked by the mep_machine() in response  *
 * to events on the CLK pin.                                     *
 *                                                               *
 * To move the state machine to a new state for the *next* CLK   *
 * event, use the MEP_NEXT_STATE() macro.                        *
 *****************************************************************/


/*****************************************************************
 * As a 'flushee', the host simply generates handshake cycles
 * until it observes DATA=='1' during a P3 cycle.  
 */
static void mepState_flushee(void)
{
    if (mep_pl_getDATA() && isLinkInP3()) {
        /* The upcoming handshake put the link into P0 (IDLE).
         * The next CLK event would therefore be the start of an
         * arbitrate state. */
        MEP_NEXT_STATE(mepState_arbitrate, "\n");
    }
}


/*****************************************************************
 * A FLUSH operation is used to initialize a MEP link.  In the
 * host 'flusher' state, the host is responsible for transferring
 * a set number of '0' bits across the link.  The number of zero
 * bits is chosen to be slightly longer than the longest possible
 * packet.  See the MEP spec for a complete explanation.
 */
static void mepState_flusher(void)
{
    if (mep_flushCount>0) {
        mep_flushCount -= 1;
    }
    else {
        /* The 'flusher' terminates its flush operation in P1 by
         * attempting to drive DATA to '1'. */
        if (isLinkInP1()) {
            mep_pl_setDATA(1);

            /* The 'flushee' state exits back to 'idle' when the
             * link actually goes idle.  This will occur on the
             * next interrupt, UNLESS the module is flushing us
             * for some reason.  In either case, the 'flushee'
             * state will do the right thing. */
            MEP_NEXT_STATE(mepState_flushee, "Fe");
        }
    }
}


/*****************************************************************
 * The third of three stop cycles in a TX packet.
 *
 * When we execute this state, the DATA line better be '1', or
 * else it means that the module is flushing us.  This also means
 * that the transfer was unsuccessful.  Unsuccessful transfers do
 * not remove the packet from the transmit queue, so it will be
 * retransmitted as soon as the flush completes.
 */
static void mepState_tx_stop2(void)
{
    if (mep_pl_getDATA() == 0) {
        /* We are being flushed: abort the transfer. */
        MEP_NEXT_STATE(mepState_flushee, "Fe");
    }
    else {
        /* The packet was transferred without errors. */
        MEP_NEXT_STATE(mepState_arbitrate, "\n");

        /* Remove the transmitted packet from the txQueue.  This
         * signals the foreground that the packet has been
         * sent. */
        mep_txQ = MEP_NULL;

        /* If a callback function is defined, invoke it now */
        if (mep_txCallback) (*mep_txCallback)(MEP_NOERR);
    }
}


/*****************************************************************
 * The second of three stop cycles in a TX packet.
 *
 * Now that the parity bit has been driven, we must drive DATA
 * back to its IDLE state ('1') for the remainder of the packet.
 */
static void mepState_tx_stop1(void)
{
    mep_pl_setDATA(1);
    MEP_NEXT_STATE(mepState_tx_stop2, "S2");
}


/*****************************************************************
 * This state transfers out the parity bit based on the parity
 * calculation we performed during the mepState_tx_data states.
 *
 * MEP uses 'even' parity: the parity bit needs to be a '1' if
 * there were an odd number of 1's transmitted to this point,
 * from the start of the header to the last bit of TX data.  The
 * parity calculation does NOT include the parity bit.
 */
static void mepState_tx_parity(void)
{
    /* drive the parity bit */
    mep_pl_setDATA(mep_parity);
    MEP_NEXT_STATE(mepState_tx_stop1, "S1");
}


/*****************************************************************
 * Transmit the next bit of the packet pointed at by
 * <mep_pktPtr>.  When all the bits have been transmitted, we
 * transition to the stop cycle states.  In debug mode, each byte
 * is printed just before we begin to transmit it.
 */
static void mepState_tx_data(void)
{
    unsigned char mep_nxtBit;

#if defined MEP_DEBUG
    if (mep_bitMask == 0x01) {
        /* We are about to TX new byte: display it */
        dbg_toHex8(*mep_pktPtr);
    }
#endif

    /* Calculate the value of the next bit we want to send
     * as the value 0x00 or 0x01 */
    mep_nxtBit = (*mep_pktPtr & mep_bitMask) != 0;

    /* Drive DATA with the next data bit to send */
    mep_pl_setDATA(mep_nxtBit);

    /* Include the new bit in the parity calculation */
    mep_parity ^= mep_nxtBit;

    mep_bitMask <<= 1;
    if (!mep_bitMask) {
        /* We finished transferring that byte.  Are there more? */
        if (--mep_pktLen) {
            /* Yes: initialize the next byte transfer. */
            mep_bitMask = 0x01;
            mep_pktPtr++;
        }
        else {
            /* No: drop into the initial stop cycle state after
             * this last bit transfer is completed. */
            MEP_NEXT_STATE(mepState_tx_parity, "Tp");
        }
    }
}


/*****************************************************************
 * The second of two stop cycles for an RX operation.
 * If DATA=='1' in this state, the receive operation is complete.
 * If DATA=='0' in this state, we are being flushed and the
 * transfer will be aborted.
 */
static void mepState_rx_stop1(void)
{
    if (mep_pl_getDATA() == 0) {
        /* We are being flushed: abort the transfer by forcing a flush. */
        setFlusherState();
    }
    else {
        /* The packet is completely received!  Notify the
         * foreground by saving a pointer to the new packet into
         * the receive queue. */
        mep_rxQ = &mep_rxBuffer;

        /* If a callback function is defined, invoke it now */
        if (mep_rxCallback) (*mep_rxCallback)(MEP_NOERR);

        MEP_NEXT_STATE(mepState_arbitrate, "\n");
    }
}


/*****************************************************************
 * The first bit after the last RX data bit is a parity bit.  We
 * check this parity bit against the parity that we calculated
 * during the RX state.
 *
 * MEP defines that the result of calculating parity on all of
 * the RX bits INCLUDING the parity bit should be 0.
 */
static void mepState_rx_parity(void)
{
    unsigned char mep_nxtBit;

    /* Read the parity bit */
    mep_nxtBit = mep_pl_getDATA();
    
    mep_parity ^= mep_nxtBit;

    if (mep_parity) {
        /* Parity Error detected!  The result should have been 0.
         * We always perform a guest-ward flush when we see a
         * parity error. */
        setFlusherState();

        /* If a callback function is defined, invoke it now */
        if (mep_errCallback) (*mep_errCallback)(MEP_PARITY);
    }
    else {
        /* if parity was OK, we proceed to the stop state */
        MEP_NEXT_STATE(mepState_rx_stop1, "S1");
    }
}


/*****************************************************************
 * This state receives all the data bits in the packet being
 * transferred.  In debug mode, each byte is printed as soon as
 * it is completely received.
 */
static void mepState_rx_data(void)
{
    unsigned char mep_nxtBit;

    /* Read the next data bit */
    mep_nxtBit = mep_pl_getDATA();

    if (mep_nxtBit) {
        *mep_pktPtr |= mep_bitMask;
    }

    /* Include the bit we received into the parity calculation */
    mep_parity ^= mep_nxtBit;

    mep_bitMask <<= 1;

    /* The mep_bitMask will have a value of 0x00 when we have
     * received an entire byte. */
    if (!mep_bitMask) {
#if defined MEP_DEBUG
        dbg_toHex8(*mep_pktPtr);       /* debug: display the byte we just received */
#endif
        /* Was this the module header byte (the first byte)?  */
        if ((void*)mep_pktPtr == (void*)&mep_rxBuffer) {
            /* Yes: extract the number of bytes left to receive */
            mep_pktLen = *mep_pktPtr & 0x07;
        }
        else {
            mep_pktLen -= 1;
        }

        if (mep_pktLen == 0) {
            /* We have received all the bytes in this
             * packet. Time to get the parity bit. */
            MEP_NEXT_STATE(mepState_rx_parity, "Rp");
        }
        else {
            mep_pktPtr++;
            *mep_pktPtr = 0;           /* new byte must be zeroed */
            mep_bitMask = 0x01;        /* Reset the bit mask for the next byte */
        }
    }
}



/*****************************************************************
 * This state checks for the module Request To Send (RTS).  If
 * RTS is present, we prepare to receive a packet.  If RTS is not
 * present, the link goes IDLE again.
 *
 * The 'arbitrate' state ensures that we never get into this
 * state unless the RXQ has room for a packet.
 */
static void mepState_rx_RTS(void)
{
    if (mep_pl_getDATA() != 0) {
        /* The module decided not to transmit for some reason: we
         * will go back to IDLE. */
        MEP_NEXT_STATE(mepState_arbitrate, "\n");
    }
    else {
        /* The module is asserting a valid RTS: we will start to
         * receive data on the next CLK edge.  Initialize
         * everything required to receive the packet. */
        mep_pktPtr = (unsigned char *)&mep_rxBuffer;
        *mep_pktPtr = 0;
        mep_bitMask = 0x01;
        mep_parity = 0;
        MEP_NEXT_STATE(mepState_rx_data, "Rx");
    }
}


/*****************************************************************
 * When a 'deferred arbtrate' state is executed, it is in fact
 * EXACTLY the same as an arbitration state.  See mep_machine()
 * for more info.
 */
static void mepState_deferredArbitrate(void)
{
    mepState_arbitrate();
}


/*****************************************************************
 * We enter this state when CLK is driven low on a previously
 * idle link.
 *
 * It may be that:
 *
 * A) the module wants to transmit
 * B) the host wants to transmit
 * C) the host and the module both want to transmit
 *
 * The ARBITRATE state resolves this situation and determines the
 * direction of the upcoming tranfer operation.
 *
 * This driver resolves contention in an extremely simple
 * fashion: the host always gets its way.  Therefore, if the host
 * TX queue is not empty, the host will transfer a packet to the
 * module.  If the host TX queue is empty, we will let the module
 * transfer a packet to the host.
 */
static void mepState_arbitrate(void)
{
#if defined MEP_DEBUG
    /* When displaying debug IO, we don't print that we are in an
     * arbitration state until it becomes active.  It makes the
     * output a bit cleaner. */
    mep_puts("Ar");
#endif
    
    /* For a host, transmitting is always allowed to take
     * priority over receiving.  Note that the MEP spec does not
     * require this, but it makes life simpler.  Therefore, if
     * the txBuf is non-empty, we will transmit a packet to the
     * module.
     */
    if (mep_txQ) {
#ifdef MEP_T0_EVENT
        /* Performance test hook: if the configuration has
         * defined a T0 macro, invoke it now to mark the start of
         * the packet transmission. */
        MEP_T0_EVENT;
#endif
        mep_pl_setDATA(0);           /* Assert host-RTS */
        
        /* Init everything required for entering the tx_data state */
        mep_pktLen = MEP_GET_GUESTWARD_PKT_LEN(mep_txQ);
        mep_pktPtr = (unsigned char *)mep_txQ;
        mep_bitMask = 0x01;
        mep_parity = 0;
        MEP_NEXT_STATE(mepState_tx_data, "Tx");
    }
    else {
        /* The module wants us to receive a packet */
        if (mep_rxQ) {
            /* Unfortunately, our receive queue is full.  We
             * stall the MEP link by moving into the 'deferred
             * arbitrate' state */
            MEP_NEXT_STATE(mepState_deferredArbitrate, "Da");
        }
        else {
#ifdef MEP_T0_EVENT
            /* Performance test hook: if the configuration has
             * defined a T0 macro, invoke it now to mark the start of
             * the packet reception. */
            MEP_T0_EVENT;
#endif

            /* RXQ has space in it: prepare to receive. */
            MEP_NEXT_STATE(mepState_rx_RTS, "Mr");
        }
    }
}


/*****************************************************************
 * A MEP handshake is defined to be the act of driving ACK so
 * that it matches the current state of CLK.
 */
static void doHandshake(void)
{
    _mep_pl_setACK(mep_pl_getCLK());
}


/*****************************************************************
 * Initialize the state machine to enter the FLUSHER state on the
 * next CLK event.  Used to initialize the state machine, or when
 * we have to abort in the middle of a transfer.
 */
static void setFlusherState(void)
{
    mep_flushCount = MEP_FLUSH_BITCOUNT;
    MEP_NEXT_STATE(mepState_flusher, "Fr");

    /* Drive DATA to '0' for the duration of the flush. */
     mep_pl_setDATA(0);
}



/*****************************************************************
 * The state machine that drives the MEP data transfers.
 *
 * In the case of POLLED or HYBRID configurations, this machine
 * will execute states until it gets back to the ARBITRATE state.
 * Disregarding FLUSH events, this means that a POLLED/HYBRID
 * state machine runs until a packet is transferred.  In the case
 * of INTERRUPT configurations, the machine just executes a pair
 * of MEP states: one for the low-going CLK event, and one for
 * the high-going CLK event, and depends on repeated interrupts
 * to transfer a packet.
 */
#if defined MEP_POLLED_STATE_MACHINE
#  define REPEAT_CONDITION (mep_nextState != mepState_arbitrate)
#else
#  define REPEAT_CONDITION (0)
#endif

#if defined MEP_CONFIG_POLLED_MODE
  static
#endif
mep_machine_t mep_machine(void)
{
	int arb_cnt = 0;

    if (mep_pl_getCLK()) {
        /* If this was a POLLED system: CLK==HIGH means there
         * is no need to start up the state machine.  In an
         * INTERRUPT system: CLK==HIGH means that there must
         * have been a spurious noise event on CLK which caused
         * us to get here.  In either case, we don't need to
         * anything. */
        return(MEP_ENABLE_INTS);
    }

    /* If we get here, we are in P1, and we have something to do: */

    do {
#if defined MEP_POLLED_STATE_MACHINE
        /* Once inside the main stateMachine loop, POLLED
         * configurations need to spin-wait here until they
         * observe the module drive CLK low. */
        if (MEP_WAIT_FOR_CLK(0) == MEP_TIMEOUT) {
            return(MEP_CLK_TIMEOUT);
        }
#endif
        (*mep_nextState)();         /* Invoke the P1 state handler */
        
        if (mep_nextState == mepState_deferredArbitrate) {
            /* The 'deferredArbitrate' state is used when the
             * module wants us to RX, but our RXQ is full and we
             * have nothing to TX.  We disable MEP interrupts and
             * leave WITHOUT HANDSHAKING the CLK=0 event that got
             * us here.  Remember that interrupts will be
             * reenabled when either the TXQ goes non-empty or
             * when the RXQ becomes empty.  In either case, the
             * resulting interrupt will take us right back to the
             * arbitrate state again.
             */
            return(MEP_DISABLE_INTS);
        }
        
        doHandshake();
        
        /* We have entered P2 now:        
         * In P2, we wait until the module drives CLK high (the
         * MEP spec says that this will take perhaps 10-20 uSec. */
        if (MEP_WAIT_FOR_CLK(1) == MEP_TIMEOUT) {
            /* We abort this transfer by going into the FLUSHER
             * state, and returning a timout failure. */
            setFlusherState();
            return(MEP_CLK_TIMEOUT);
        }
        
        /* We have entered P3 now: */
        (*mep_nextState)();     /* Invoke the P3 state handler */
        doHandshake();          /* Handshake when the handler completes */
        
        /* We have entered P0 now:
         * If mep_nextState is 'arbitrate', it indicates that the
         * MEP link became idle as we entered P0.  If we have
         * something in the TXQ ready to TX, assert RTS. */
        if ((mep_nextState == mepState_arbitrate) && mep_txQ) {
            mep_pl_setDATA(0);
        }

		if(mep_nextState == mepState_flushee)
		{
			arb_cnt++;
			if(arb_cnt > 1000)
			{
				arb_cnt = 0;
				return(MEP_CLK_TIMEOUT);
			}
		}

    } while (REPEAT_CONDITION);

    return(MEP_ENABLE_INTS);
}    
    
/*****************************************************************
 * This transmit function performs a non-blocking write.  
 *
 * Note: for polled & hybrid configurations, calling this routine
 * will actually perform a blocking write.  This is required to
 * repeatedly trigger the MEP machine to transmit the packet.
 *
 */
mep_err_t mep_tx(mep_packet_t *newPacketPtr)
{
    //mep_pl_init(); //dhsong
    //mep_pl_setDATA(0); //dhsong
    /****** Start of critical code section ******/
#if !defined MEP_CONFIG_POLLED_MODE
    mep_pl_intDisable();
#endif
//printk("%d, mep_txQ = %d\n\n\n", __LINE__, mep_txQ); //dhsong
    if (mep_txQ != MEP_NULL) {
        /* Error: We are in the midst of transmitting a packet */
#if !defined MEP_CONFIG_POLLED_MODE
        mep_pl_intEnable();
#endif
        return(MEP_TXQFULL);
    }
    
    /* Point the TX interrupt state machine at the packet to be transmitted */
    mep_txQ = newPacketPtr;
//printk("%d, mep_txQ = %d\n\n\n", __LINE__, mep_txQ); //dhsong

    /* If the next state is 'arbitrate', it indicates that the
     * MEP link is idle.  If the next state is
     * 'deferredArbitrate', it means that the link is
     * "effectively" idle: a module wants to send to us, but our
     * RXQ is full.  Even though we can't accept the RX packet,
     * we can still transmit!
     *
     * In either case, we bootstrap the state machine into
     * transmitting our packet by asserting a host RTS on the MEP
     * link.  When the module responds to our RTS by driving CLK
     * to 0, the state machine will take over from there.  If the
     * MEP link is not idle, the state machine will detect the
     * non-empty TX queue when it finishes the current
     * transfer. */
    if ((mep_nextState == mepState_arbitrate) ||
        (mep_nextState == mepState_deferredArbitrate)) {
        /* The Host RTS is performed by driving DATA=0 */
        mep_pl_setDATA(0);
    }
    
    /****** End of critical code section ******
     *
     * We always want to reenable ints here: if the state machine
     * was stalled in 'deferredArbitrate' due to the RXQ being
     * full, we are still able to TX.  */
#if defined MEP_HYBRID
    /* Hybrid configurations need to invoke the callback before
       unmasking interrupts again */
#else
#  if !defined MEP_CONFIG_POLLED_MODE
    mep_pl_intEnable();
#  endif
#endif

#if defined MEP_CONFIG_POLLED_MODE
    do {
		mep_machine_t result;
        /* Polled configurations (including HYBRID) must block
         *  anyway or else the packet will never get sent. */
		result = mep_machine();
        if (result == MEP_CLK_TIMEOUT || result == MEP_ENABLE_INTS) {
            return(MEP_TIMEOUT);
        }
    } while (mep_txQ);
#endif
    
#if defined MEP_HYBRID_ISR
    /* Now that the packet is transmitted, we reenable ints so
     * that we can receive the ACK. */
    mep_pl_intEnable();
#endif
    return(MEP_NOERR);
}



/*****************************************************************
 * This is a blocking transmit function.  When called, it will
 * block until the packet is transmitted.
 */
mep_err_t mep_txBlock(mep_packet_t *newPacketPtr)
{
    mep_err_t rVal;
    
    do {
        /* Poll the non-blocking transmit routine until it
         * accepts the new TX packet... */
        rVal = mep_tx(newPacketPtr);
    } while (rVal == MEP_TXQFULL);
    
    do {
        /* ...then block until the packet disappears. */
    } while (mep_txQ);
    
    return(MEP_NOERR);
}



/*****************************************************************
 * Call this routine to receive the oldest packet in the receive
 * queue.  This is a non-blocking receive: if the receive queue
 * is empty, we immediately return with a MEP_RXQEMPTY error.
 * This allows polled systems to implement receive timeouts in a
 * simple fashion.
 */
mep_err_t mep_rx(mep_packet_t *hostRxBuffer)
{

#if defined MEP_CONFIG_POLLED_MODE
    /* Non-interrupt implementations must trigger the state
     * machine manually.  If a module is trying to send a packet,
     * calling the stateMachine will receive it into the
     * rxQueue. */
    mep_machine();
#endif
    if (!mep_rxQ) {
        /* No packet in the queue */
        return(MEP_RXQEMPTY);
    }
    
    /* The MepLib RX buffer is not empty: copy the packet to the
       host's buffer */
    *hostRxBuffer = *mep_rxQ;

    /* Mark the RX queue as being 'empty' to signal the receive
     * state that it can reuse it. */
    mep_rxQ = MEP_NULL;

    /* Reenable interrupts in case the state machine had disabled
     * them due to the MEP RX buffer being full. */
#if !defined MEP_CONFIG_POLLED_MODE
    mep_pl_intEnable();
#endif
    
    return(MEP_NOERR);
}


/*****************************************************************
 * Call this routine to receive the oldest packet in the receive
 * queue.  This is a blocking receive: it does not return until
 * it has a packet, or an error other than RXQ empty.
 */
mep_err_t mep_rxBlock(mep_packet_t *hostRxBuffer)
{
    mep_err_t rVal;
    
    do {
        rVal = mep_rx(hostRxBuffer);
    } while (rVal == MEP_RXQEMPTY);
    
    return(rVal);
}



/*****************************************************************
 * Initialize the MEP link.
 *
 * Inits the host hardware signals.
 * Inits the MEP state machine.
 */
void mep_init(void)
{
    /* Init the MEP IO and Interrupt hardware in a fashion
     * specific to the physical layer. */
    mep_pl_init();

    /* Make sure that our ACK state variable gets initialized to
     * the proper level */
    _mep_pl_setACK(1);

    /* Flush the MEP packet queues. */
    mep_txQ = MEP_NULL;
    mep_rxQ = MEP_NULL;

    /* Flush the callback function pointers. */
    mep_rxCallback = MEP_NULL;
    mep_txCallback = MEP_NULL;
    mep_errCallback = MEP_NULL;
    
    /* Init the MEP state machine by forcing it to perform a
     * FLUSH operation on the MEP link the next time it tries to
     * do a transfer. */
    setFlusherState();
    
#if !defined MEP_CONFIG_POLLED_MODE
    /* Activate interrupts on the MEP link. */
    mep_pl_intEnable();
#endif
}


/*****************************************************************
 * Associate a host's callback function pointer with a specific
 * MEP callback.  
 *
 * Params:
 *
 * - If the function pointer to the new callback is NULL, the
 * callback is disabled.
 *
 * - If the old callback pointer is non-NULL, then the value of
 * the old function pointer will be stored at that pointer
 * location.  If the pointer value is NULL, the old value will be
 * discarded.
 *
 * All callbacks are disabled after calling mepInit().
 *
 * MEP interrupts are disabled while we update the function
 * pointer just in case the update of the pointer is not an
 * atomic operation.  This would likely be the case with a
 * processor having an 8-bit bus width needing to perform a pair
 * of writes to update the 16-bit pointer value.
 *
 * MEP interrupts are always enabled after calling this routine.
 *
 * Returns MEP_BADCMD if the callback ID is bad.
 */
mep_err_t mep_setCallback(mep_callbackId_t id,
                          mep_callbackFuncPtr_t  new_callbackFuncPtr,
                          mep_callbackFuncPtr_t *old_callbackFuncPtr
                          )
{
    mep_err_t rVal = MEP_NOERR;
    mep_callbackFuncPtr_t old=MEP_NULL;
    
#if !defined MEP_CONFIG_POLLED_MODE
    mep_pl_intDisable();
#endif

    switch (id) {
    case MEP_RX_CALLBACK:
        old = mep_rxCallback;
        mep_rxCallback = new_callbackFuncPtr;
        break;

    case MEP_TX_CALLBACK:
        old = mep_txCallback;
        mep_txCallback = new_callbackFuncPtr;
        break;

    case MEP_ERR_CALLBACK:
        old = mep_errCallback;
        mep_errCallback = new_callbackFuncPtr;
        break;

    default: rVal = MEP_BADCMD;
    }

    /* If the user requests, return the previous func pointer */
    if (old_callbackFuncPtr) {
        *old_callbackFuncPtr = old;
    }

#if !defined MEP_CONFIG_POLLED_MODE
    mep_pl_intEnable();
#endif

    return(rVal);
}
