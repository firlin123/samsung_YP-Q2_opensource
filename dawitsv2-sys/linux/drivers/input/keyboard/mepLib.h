/**
 * Version: $Id: mepLib.h,v 1.12 2004/09/23 15:20:27 abowman Exp $
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

#ifndef MEPLIB_H
#define MEPLIB_H

/*****************************************************************
 * Define the error return codes that may be generated by calls
 * to the MEP data layer API routines (mep_rx, mep_tx, etc.).
 */
typedef enum {
    MEP_NOERR=0,        /* The 'no error' condition */
    MEP_BADCMD,         /* Bad command */
    MEP_TXQFULL,        /* The transmit queue is full */
    MEP_RXQEMPTY,       /* The receive queue is empty */
    MEP_TIMEOUT,        /* The MEP machine timed out during a transfer */
    MEP_PARITY         /* We observed a parity error during a packet receive operation */
} mep_err_t;

/*****************************************************************
 * Define the return values for the MEP machine.
 */
typedef enum {
    MEP_ENABLE_INTS=0,  /* The MEP machine wants MEP interrupts enabled */
    MEP_DISABLE_INTS,   /* The MEP machine wants MEP interrupts disabled */
    MEP_CLK_TIMEOUT    /* The MEP machine timed out waiting on the CLK signal */
} mep_machine_t;


/*****************************************************************
 * Define the callback ID's for use with the mep_setCallback()
 * API.
 */
typedef enum {
    MEP_RX_CALLBACK,    /* Called when the MEP receive queue goes non-empty */
    MEP_TX_CALLBACK,    /* Called when the MEP transmit queue goes empty */
    MEP_ERR_CALLBACK   /* Called when we generate an error that the host should know about */
} mep_callbackId_t;


/*****************************************************************
 * This is the datatype used for the call-back functions.
 */
typedef void (*mep_callbackFuncPtr_t)(mep_err_t error);



/*****************************************************************
 * Define the MEP packet data type as used by the Transport API
 * and below.  
 *
 * If desired, host application code can define their own packet
 * type and then typecast to this datatype as they call the
 * functions in the MEP data layer API.
 */
typedef struct {
    unsigned char rawPkt[8];
} mep_packet_t;


/*****************************************************************
 * Define macros to manipulate fields within the MEP packet header.
 */
#define MEP_GET_HEADER(packetPtr)            ((packetPtr)->rawPkt[0])
#define MEP_GET_GUESTWARD_PKT_LEN(packetPtr) ((MEP_GET_HEADER(packetPtr) & 0x08) ? \
                                             1 : ((MEP_GET_HEADER(packetPtr) & 0x07)+1))
#define MEP_GET_HOSTWARD_PKT_LEN(packetPtr)  ((MEP_GET_HEADER(packetPtr) & 0x07)+1)
#define MEP_GET_ADDR(packetPtr)              ((MEP_GET_HEADER(packetPtr)>>5) & 0x07)
#define MEP_GET_CTRL(packetPtr)              ((MEP_GET_HEADER(packetPtr)>>3) & 0x03)
#define MEP_GET_SHORTCMD(packetPtr)          ((MEP_GET_HEADER(packetPtr)>>0) & 0x07)
#define MEP_GET_LONGCMD(packetPtr)           ((packetPtr)->rawPkt[1])


#define MEP_SET_ADDR(packetPtr, addr) \
        do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0xE0) | (((addr)&0x7)<<5);} while (0)

#define MEP_SET_GLOBALCMD(packetPtr, global) \
        do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0x10) | (((global)&0x1)<<4);} while (0)

#define MEP_SET_FORMATCTL(packetPtr, formatctl) \
    do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0x08) | (((formatctl)&0x1)<<3);} while (0)

#define MEP_SET_GUESTWARD_LENGTH(packetPtr, shortcmd) \
        do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0x07) | (((shortcmd)&0x7)<<0);} while (0)

#define MEP_SET_CTRL(packetPtr, ctrl) \
        do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0x18) | (((ctrl)&0x3)<<3);} while (0)

#define MEP_SET_SHORTCMD(packetPtr, shortcmd) \
        do {MEP_GET_HEADER(packetPtr) = (MEP_GET_HEADER(packetPtr) & ~0x07) | (((shortcmd)&0x7)<<0);} while (0)

#define MEP_SET_LONGCMD(packetPtr, longcmd) \
        do {(packetPtr)->rawPkt[1] = longcmd;} while (0)



/*****************************************************************
 * The prototypes represent the Data Layer API used by a host
 * application to interact with the MEP library.
 */
void      mep_init(void);

mep_err_t mep_tx(mep_packet_t *userTxBuffer);
mep_err_t mep_rx(mep_packet_t *userRxBuffer);

mep_err_t mep_txBlock(mep_packet_t *userTxBuffer);
mep_err_t mep_rxBlock(mep_packet_t *userRxBuffer);

mep_err_t mep_setCallback(mep_callbackId_t id,
                          mep_callbackFuncPtr_t  new_callbackFuncPtr,
                          mep_callbackFuncPtr_t *old_callbackFuncPtr
                          );





/*****************************************************************
 * These prototypes represent the Physical Layer functions that
 * are required to implement the MepLib Hardware Abstraction API.
 *
 * Each of these routines needs to be supplied by the host
 * software codebase.  In short, a MepLib port is accomplished by
 * instantiating these routines according to the specific host
 * hardware available for use by MEP.
 *
 * See the MepLib Porting Guide for more complete information
 * regarding the porting process.
 */

/*-----------------------------------------------------------------
 * Perform the host-specific operations required to initialize
 * the MEP physical layer hardware:
 *
 *  - init the ACK IO as an push-pull OUTPUT; initial value: 1 (HIGH).
 *  - init the DATA IO as an open-drain OUTPUT; initial value: 1 (HIGH).
 *  - init the CLK IO as an INPUT
 */
void mep_pl_init(void);

/*-----------------------------------------------------------------
 * These routines are required to return the current logic-level
 * value of specified MEP link signal as a C boolean type
 * (0==LOW, non-zero==HIGH).
 */
unsigned char mep_pl_getDATA(void);
unsigned char mep_pl_getCLK(void);

/*-----------------------------------------------------------------
 * These routines perform all host-specific operations required
 * to set the specified MEP link signal to the desired logic
 * level.  The levels will specified as C booleans:
 * 0==LOW, non-zero==HIGH).
 */
void mep_pl_setDATA(unsigned char logicLevel);
void mep_pl_setACK(unsigned char logicLevel);



/*----------------------------------------------------------------
 * The MEP physical layer support routines only need to be
 * implemented for interrupt-driven and hybrid configurations
 * ONLY!  Polled configurations do not require them to exist.
 *
 * These routines provide the various control mechanisms over the
 * host interrupt input that has been assigned to the MEP link
 * CLK signal.
 *
 * See the MepLib porting guide for a more lengthy discussion on
 * interrupt hardware requirements.
 */
void mep_pl_intEnable(void);      /* Enable MEP interrupts */
void mep_pl_intDisable(void);     /* Mask (disable) MEP interrupts */


#ifdef MEP_DEBUG
/*----------------------------------------------------------------
 * Required for debug builds only: This routine provides a simple
 * interface to some host-supplied output stream.
 * 
 * Hosts which have no spare output stream but do support some
 * form of internal visibility like a debugger can accomplish
 * useful things by implementing mep_putc() as a write to a
 * memory-resident circular buffer.  A debugger can be used to
 * dump the buffer contents to see how transfers progress.
 *
 * MepLib debug output terminates lines with '\n' only.  If
 * <CRLF> is required, the mep_putc() routine must add a '\r'
 * for every '\n' that it processes.
 */
void mep_debug_putc(unsigned char c);

/*****************************************************************
 * Defined for debug builds only: Print a string until a NULL
 * char is observed.  MepLib implements this routine by
 * repeatedly calling the host-supplied mep_putc().
 */
void mep_debug_puts(unsigned char *msg);
#endif  /* MEP_DEBUG */



#if !defined MEP_CONFIG_POLLED_MODE
/*****************************************************************
 * The MEP library provides this function.  For interrupt-driven
 * configurations, this routine must be called by host code
 * whenever an unmasked low-going CLK interrupt is processed by
 * the host CPU.
 * 
 * The function returns an error or an indication of how MepLib
 * would like the MEP interrupts to be enabled/disabled.  This is
 * required to perform the control dance associated with
 * interruptable ISRs.
 *
 * Polled configurations do NOT need to call this routine.  It
 * will automatically get invoked as required without explicit
 * host intervention.
 */
mep_machine_t mep_machine(void);
#endif /* !defined MEP_CONFIG_POLLED_MODE */


/*****************************************************************
 * Provide a default macro if the host configuration has not
 * supplied a better one.  This default handler has no timeout
 * support.  It will wait forever for a CLK event!  If this is
 * not acceptable, the host <mep_config.h> file should define a
 * version of this macro which performs the same CLK-waiting
 * task, but also implements a host-specific timeout mechanism
 * while it waits.
 */
#if !defined MEP_WAIT_FOR_CLK
#  define MEP_USE_DEFAULT_CLK_WATCHER
#  define MEP_WAIT_FOR_CLK(CLK_desiredLevel) mep_waitForClkForever(CLK_desiredLevel)
#endif



#endif /* MEPLIB_H */
