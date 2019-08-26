/*
 * Copyright (C) 2007 SigmaTel, Inc., Ioannis Kappas <ikappas@sigmatel.com>
 * 
 * Based on the Tiny TTY skeleton driver.
 * Copyright (C) 2002-2004 Greg Kroah-Hartman (greg@kroah.com)
 *
 * This  program is  free  software; you  can  redistribute it  and/or
 * modify  it under the  terms of  the GNU  General Public  License as
 * published by the Free Software  Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 *
 * You should have  received a copy of the  GNU General Public License
 * along  with  this program;  if  not,  write  to the  Free  Software
 * Foundation,  Inc.,  51 Franklin  Street,  Fifth  Floor, Boston,  MA
 * 02110-1301, USA.
 */

/* @todo list:
 *
 * a.   Figure  out  whether  and  how to  implemented  software  flow
 * control.  More specificaly,  how does  the core  indicate  when the
 * driver should send a stop or start character?
 *
 * b.  Figure   out  if  any   other  tty  interface   functions  need
 * implementing (particularly, throttle and unthrottle).
 *
 * c. Whether to implement a break handler. I have written one that is
 * commented out because I'm not sure this is how it should work.
 *
 * d. Convert all BUGS() to proper error handling conditions.
 *
 * e.  Decide whether  the other  two ioctl  of the  original tiny_tty
 * source code are worth anything.
 */

#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>


#include <linux/dma-mapping.h>
#include <asm/arch/dma.h>

#define DRIVER_VERSION "v0.5"
#define DRIVER_AUTHOR "Ioannis Kappas <ikappas@sigmatel.com>"
#define DRIVER_DESC "stmp37xx APP UART driver"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");


#define APPUART_TTY_MAJOR		242	/* experimental range */
#define APPUART_TTY_MINORS		1	/* only have so many devices */

#define UART_CLK                24000000

struct appuart_serial {
	struct	tty_struct   *tty;		/* pointer to the tty for this device */
	int		open_count;	/* number of times this port has been opened */
	struct	semaphore    sem;		/* locks this structure */

	/* for tiocmget and tiocmset functions */
	int         msr;		/* MSR shadow */
	int         mcr;		/* MCR shadow */

	/* for ioctl fun */
	struct serial_struct    serial;
	wait_queue_head_t   wait;
	wait_queue_head_t   tx_dma_done;
	struct async_icount icount;

	/* The index inside the next dma buffer that data can be written on */
	unsigned tx_buffer_index;
};

static struct appuart_serial *appuart_table[APPUART_TTY_MINORS];	/* initially all NULL */

static struct tty_driver *appuart_tty_driver;

static struct platform_device app_uart_device = {
	.name   = "stmp37xx_app_uart",
	.id     = 0,
	.dev    = {
		.coherent_dma_mask = ISA_DMA_THRESHOLD,
	}
};

static stmp37xx_dma_user_t dma_user =
{
    .name = "stmp37xx app UART"
};

/* The RX DMA chain specification.
 * There exits two implementation controled by the RX_PARIS macro.
 * 
 * The  proper method  (RX_PAIRS  is 1):  The  idea is  that we  chain
 * command pairs; The first command is the regular DMA RX command, the
 * second  the command  that will  copy  the the  status register  (rx
 * count, error  flags) to a padded  position in the  RX buffer.  This
 * means that the  rx buffer is padded with four  extra bytes to store
 * the status flag.
 *
 * Unfortunately this doesn't  work due to what it  appears a hardware
 * bug; When a timeout happen and  the receive count is not a multiple
 * of 16 bytes, then the copy of the status register is not the actual
 * status register,  but incoming data  that were not copied  when the
 * timeout occured.
 *
 * The  alternative method  (RX_PAIRS is  0): There  are  two DMA-recv
 * commands in a chain and when one is complete, the DMA on completion
 * interrupt  occurs, the  status  register is  copied  and the  other
 * command is set to run  (by increasing the semaphore).  This has the
 * apparent  problem  that between  reading  the  status register  and
 * increasing the  semaphore for the  next command to run,  data might
 * have already  arrived and  missed (the effect  is more  apparent at
 * high speeds).  The other peculiarity  with this approach is that it
 * only works when the receiving buffer size is set to 10 bytes.
 */
#define RX_PAIRS 0

static circular_dma_chain_t rx_dma_chain =
{
	.id = DMA_UART_RX,
/* bus channel should be deleted */
	.bus = STMP37XX_BUS_APBX,
    .channel = 6
};
#if RX_PAIRS
#  define RX_CHAIN_PAIRS 2
#  define RX_CHAIN_LEN 50 * RX_CHAIN_PAIRS
#  define RX_BUFFER_SIZE 1024
/* Where exactly in the rx buffer the status register is kept */
#  define GET_RX_STATUS( buffer ) ( (hw_uartapp_stat_t*) \
                                                       (((char*)buffer) + RX_BUFFER_SIZE))
#  define GET_RX_BUFFER( buffer ) buffer
#else
#  define RX_CHAIN_LEN 2
#  define RX_BUFFER_SIZE 10
#endif /* RX_PAIRS */

static dma_addr_t rx_dma_buffers[RX_CHAIN_LEN];
static stmp37xx_dma_descriptor_t rx_dma_descrs[RX_CHAIN_LEN];

/* The TX DMA chain specification */
static circular_dma_chain_t tx_dma_chain =
{
	.id = DMA_UART_TX,
/* Todo: bus channel should be deleted */
    .bus = STMP37XX_BUS_APBX,
    .channel = 7
};
#define TX_CHAIN_LEN 2
#define TX_BUFFER_SIZE 1024
static dma_addr_t tx_dma_buffers[TX_CHAIN_LEN];
static stmp37xx_dma_descriptor_t tx_dma_descrs[TX_CHAIN_LEN];


#if 0
/* Print out app UART register */
static void app_regs(char* label)
{
    printk( "%s:\n"
	    "APBX_CTRL1: 0x%x\n"
	    "RCV CTRL0: 0x%x, TRM CTRL1: 0x%x\n"
	    "CTR REG  : 0x%x\n"
	    "LIN CTRL0: 0x%x, LIN CTRL1: 0x%x\n"
	    "INT REG  : 0x%x, DAT REG  : 0x%x\n"
	    "STAT REG : 0x%x, DGB REG  : 0x%x\n"
	    , label,
	    HW_APBX_CTRL1_RD(),
	    HW_UARTAPP_CTRL0_RD(), HW_UARTAPP_CTRL1_RD(),
	    HW_UARTAPP_CTRL2_RD(),
	    HW_UARTAPP_LINECTRL_RD(), HW_UARTAPP_LINECTRL2_RD(),
	    HW_UARTAPP_INTR_RD(), HW_UARTAPP_DATA_RD(),
	    HW_UARTAPP_STAT_RD(), HW_UARTAPP_DEBUG_RD()
	    );

}
#endif 

/* Update TX command with new size */
static void update_tx_command( stmp37xx_dma_descriptor_t* descr, unsigned new_size )
{
    hw_uartapp_ctrl1_t tx_cmd;
    
    descr->command->cmd &= ~(BF_APBX_CHn_CMD_XFER_COUNT(0xFFFF));
    descr->command->cmd |= BF_APBX_CHn_CMD_XFER_COUNT(new_size);
    
    tx_cmd.U = HW_UARTAPP_CTRL1_RD();
    tx_cmd.B.XFER_COUNT = new_size;
    descr->command->pio_words[0] = tx_cmd.U;
}

static irqreturn_t uart_internal_int(int irq, void *dev_id)
{
	hw_uartapp_intr_t status;
	status.U = HW_UARTAPP_INTR_RD();

	if ( irq != IRQ_UARTAPP_INTERNAL) BUG();

	if ( status.B.RXIS ) BF_CLR( UARTAPP_INTR, RXIS );
	if ( status.B.TXIS ) BF_CLR( UARTAPP_INTR, TXIS );
	if ( status.B.RTIS ) {
		BF_CLR( UARTAPP_INTR, RTIS );
	}
	else if ( status.B.OEIS ) {
		BF_CLR( UARTAPP_INTR, OEIS );
	}
	else if ( status.B.BEIS ) {
		BF_CLR( UARTAPP_INTR, BEIS );
	}
	else if ( status.B.PEIS ) {
		BF_CLR( UARTAPP_INTR, PEIS );   
	}
	else if ( status.B.FEIS ) {
		BF_CLR( UARTAPP_INTR, FEIS );   
	}
	return IRQ_HANDLED;        
}

/* The interrupt handler for tx DMA */
static void tx_int(int id, unsigned int dma_status,  void* dev_id)
{
	struct appuart_serial *appuart = dev_id;

	if (tx_dma_chain.active_count == 0)
		BUG();

	appuart->tx_buffer_index = 0;
	wake_up_interruptible(&appuart->tx_dma_done);
}

/* @todo:
 *
 *  Prove that using  the rx_dma_chain without a spin  lock is safe in
 *  future code update  (currently it is, since is  is only touched by
 *  the  uart when  it is  being  turned off,  when interrupts  cannot
 *  happen).
 */
static void rx_int (int id, unsigned int dma_status,  void* dev_id)
{
    volatile hw_uartapp_stat_t* status;

	if ( rx_dma_chain.active_count == 0 ) BUG();

/* It oculd be that in one dma complete, we served two commands (since we are slow)
    if ( circ_advance_cooked( &rx_dma_chain ) == 0 ) BUG();
*/
#if RX_PAIRS
    status = (volatile hw_uartapp_stat_t *) HW_UARTAPP_STAT_ADDR;
    circ_advance_cooked(&rx_dma_chain);
#else
    status = (volatile hw_uartapp_stat_t *) HW_UARTAPP_STAT_ADDR;
    circ_advance_cooked(&rx_dma_chain);
    //circ_advance_active(&rx_dma_chain, 1);		
#endif /* RX_PAIRS */    

#if RX_PAIRS
    while ( rx_dma_chain.cooked_count > 1)
#else
    while ( rx_dma_chain.cooked_count )
#endif /* RX_PAIRS */	
    {
	unsigned i;
	struct tty_struct* tty = appuart_table[0]->tty;
	char* buffer;
	unsigned flag = TTY_NORMAL;
	unsigned data_size;
	stmp37xx_dma_descriptor_t* descr = circ_get_cooked_head( &rx_dma_chain );

#if RX_PAIRS
	status = GET_RX_STATUS( descr->virtual_buf_ptr );
	buffer = GET_RX_BUFFER( descr->virtual_buf_ptr );
#else
	buffer = (char*) descr->virtual_buf_ptr;
#endif /* RX_PAIRS */
	  
	data_size = status->B.RXCOUNT;
	if ( status->B.FERR ) 	   flag = TTY_FRAME;
	else if ( status->B.PERR ) flag = TTY_PARITY;
	else if ( status->B.BERR ) flag = TTY_BREAK;
	else if ( status->B.OERR ) flag = TTY_OVERRUN;		
	//if (data_size > RX_BUFFER_SIZE) BUG();

	for (i = 0; i < data_size - 1; ++i)
	{
/* 	    printk("%2x ", buffer[i]); */
	    tty_insert_flip_char(tty, buffer[i], TTY_NORMAL);
	}
	/* Report any error condition on last character */
	if ( flag ) printk("Rx error: 0x%x\n", flag);
	tty_insert_flip_char(tty, buffer[i], flag);	
	tty_flip_buffer_push(tty);

#if RX_PAIRS
	circ_advance_free( &rx_dma_chain, 2);
	circ_advance_active( &rx_dma_chain, 2);
#else
	circ_advance_free( &rx_dma_chain, 1);
	circ_advance_active(&rx_dma_chain, 1);		
#endif /* RX_PAIRS */
    }
}


/* Power on APP UART and configure */
static void uart_on (void)
{
	HW_PINCTRL_MUXSEL1_CLR( 0xFF << 20 );
	{ 
		// Reset the block.
		HW_UARTAPP_CTRL0_SET ( BM_UARTAPP_CTRL0_SFTRST );

		// Release the block from reset and start the clocks.
		HW_UARTAPP_CTRL0_CLR( BM_UARTAPP_CTRL0_SFTRST | BM_UARTAPP_CTRL0_CLKGATE );

		HW_UARTAPP_CTRL2_SET( BM_UARTAPP_CTRL2_UARTEN | BM_UARTAPP_CTRL2_RXE | BM_UARTAPP_CTRL2_TXE );      

		// Enable the Application UART DMA bits.
		HW_UARTAPP_CTRL2_SET( BM_UARTAPP_CTRL2_TXDMAE | BM_UARTAPP_CTRL2_RXDMAE );

		HW_UARTAPP_INTR_WR( 0 );

		/* Note there are three interrupts available:
		 *
		 * DMA Rx, DMA Tx and uart internal.
		 *
		 * The  first two will  assert on  DMA completion.   The third
		 * will  assert when  any  of the  _IEN  flags is  set in  the
		 * UARTAPP  hardware register.  These are  indepented  of each
		 * other.   So,  if  RXIEN  is  set, then  you  will  get  one
		 * interrupt  when data  arrive and  another when  the  DMA Rx
		 * command   is  completed   (on  their   dedicated  interrupt
		 * handlers).  Note that Receive Timeout (RTIEN) does not have
		 * to be set for the DMA Rx command to break on a timeout.
		 */

/*   	HW_UARTAPP_INTR_SET( BM_UARTAPP_INTR_TXIEN |  */
/*   			     BM_UARTAPP_INTR_RXIEN );  */

		/* Interrupt on error */ 
/*  	HW_UARTAPP_INTR_SET( BM_UARTAPP_INTR_OEIEN | */
/* 			     BM_UARTAPP_INTR_BEIEN | */
/* 			     BM_UARTAPP_INTR_PEIEN | */
/* 			     BM_UARTAPP_INTR_RTIEN | */
/* 			     BM_UARTAPP_INTR_FEIEN ); */

	}

	/* Enable fifo so all four bytes of a DMA word are written to
	 * output (otherwise, only the LSB is written, ie. 1 in 4 bytes)
	 */
	BF_SET(UARTAPP_LINECTRL, FEN);
	/* Don't see any use for them at the moment */
//    BF_SET(UARTAPP_CTRL2, DMAONERR);
//    BF_WR( UARTAPP_CTRL2, RXIFLSEL, 0);
}

/* Power APP UART OFF */
static void uart_off(void)
{
	/* Reset sets  the clock gate  automatically (at least for  DMA, I
	 * guess it is the same for every other?) */
	HW_UARTAPP_CTRL0_SET( BM_UARTAPP_CTRL0_SFTRST | BM_UARTAPP_CTRL0_CLKGATE );

	free_irq(IRQ_UARTAPP_INTERNAL, NULL);
	stmp37xx_free_dma(tx_dma_chain.id);
	stmp37xx_free_dma(rx_dma_chain.id);
}


/* Set new line speed
 */
static void set_speed(unsigned baud)
{
	unsigned div;

	div = (UART_CLK * 32) / baud;
	BF_WR( UARTAPP_LINECTRL, BAUD_DIVFRAC, div & 0x3F );
	BF_WR( UARTAPP_LINECTRL, BAUD_DIVINT, div >> 6); 
}

/* Set word length on app UART */
static void __set_word_length( unsigned length )
{
	switch ( length ) {
	case 5:
		BF_WR( UARTAPP_LINECTRL, WLEN, 0);
		break;
	case  6:
		BF_WR( UARTAPP_LINECTRL, WLEN, 1);
		break;
	case 7:
		BF_WR( UARTAPP_LINECTRL, WLEN, 2);
		break;
	case 8:
		BF_WR( UARTAPP_LINECTRL, WLEN, 3);
		break;
	default:
		printk(KERN_ERR "Unsupported word length %u\n", length);
		return;
	}
}

/* Set APP UART speed according to the CS symbol */
static void set_word_length( unsigned word_length )
{
    switch (word_length)
    {
    case CS5:
	__set_word_length( 5 );
	break;
    case CS6:
	__set_word_length( 6 );
	break;
    case CS7:
	__set_word_length( 7 );
	break;
    case CS8:
	__set_word_length( 8 );
	break;
    default:
	BUG();
    }
}

/* Enable parity according to parity type */
static void parity_enable( unsigned odd_parity )
{
	BF_SET(UARTAPP_LINECTRL, PEN);
	BF_SET(UARTAPP_LINECTRL, SPS);    
	if ( odd_parity ) {
		BF_CLR(UARTAPP_LINECTRL, EPS);  
	}
	else {
		BF_SET(UARTAPP_LINECTRL, EPS);  
	}
}

/* Disable parity checking and generation */
static void parity_disable(void)
{
	BF_CLR(UARTAPP_LINECTRL, SPS);    
	BF_CLR(UARTAPP_LINECTRL, PEN);
}

/* Enable or disable hardware flow control */
static void hardware_flowcontrol( int enable )
{
	if ( enable ) {
		BF_SET( UARTAPP_CTRL2, CTSEN);
		BF_SET( UARTAPP_CTRL2, RTSEN);
	}
	else {
		BF_CLR( UARTAPP_CTRL2, CTSEN);
		BF_CLR( UARTAPP_CTRL2, RTSEN);
	}

}

static void loopback( int enable )
{
	if (enable)
		BF_SET(UARTAPP_CTRL2, LBE);
	else
		BF_CLR(UARTAPP_CTRL2, LBE); 
}

static void __open_uart (struct tty_struct *tty)
{
	struct appuart_serial *appuart = tty->driver_data;
	int retval = -EINVAL;

	uart_on(); 

	set_speed( tty_get_baud_rate(tty) );
	set_word_length( C_CSIZE( tty ) );
	hardware_flowcontrol( C_CRTSCTS( tty ) );

	retval = request_irq(IRQ_UARTAPP_INTERNAL, uart_internal_int, 0, "stmp37xx app uart internal", NULL);
	retval = stmp37xx_request_dma(tx_dma_chain.id, "stmp37xx app uart dma tx", tx_int, appuart);
	retval = stmp37xx_request_dma(rx_dma_chain.id, "stmp37xx app uart dma rx", rx_int, appuart);

	init_waitqueue_head(&appuart->tx_dma_done);

	if (retval)
		BUG();

	stmp37xx_dma_reset_channel(tx_dma_chain.id);
	stmp37xx_dma_reset_channel(rx_dma_chain.id);

	circ_clear_chain( &tx_dma_chain );
	stmp37xx_dma_go(tx_dma_chain.id, &tx_dma_descrs[0], 0);         

	circ_clear_chain( &rx_dma_chain );
	stmp37xx_dma_go(rx_dma_chain.id, &rx_dma_descrs[0], 0);
#if RX_PAIRS
	circ_advance_active( &rx_dma_chain, RX_CHAIN_LEN );
#else    
//	circ_advance_active( &rx_dma_chain, 1 );
	circ_advance_active( &rx_dma_chain,  RX_CHAIN_LEN);
#endif /* RX_PAIRS */    
}


/* Open APP UART device and configure if necessary */
static int appuart_open(struct tty_struct *tty, struct file *file)
{
	struct appuart_serial *appuart;
	int index;

	/* initialize the pointer in case something fails */
	tty->driver_data = NULL;

	/* get the serial object associated with this tty pointer */
	index = tty->index;
	appuart = appuart_table[index];
	if (appuart == NULL) {
		/* first time accessing this device, let's create it */
		appuart = kmalloc(sizeof(*appuart), GFP_KERNEL);
		if (!appuart)
			return -ENOMEM;

		init_MUTEX(&appuart->sem);
		appuart->open_count = 0;
		appuart->tx_buffer_index = 0;

		appuart_table[index] = appuart;
	}

	down(&appuart->sem);

	/* save our structure within the tty structure */
	tty->driver_data = appuart;
	appuart->tty = tty;

	++appuart->open_count;
	if (appuart->open_count == 1) {
		/* this is the first time this port is opened */
		/* do any hardware initialization needed here */
		__open_uart( tty );
	}

	up(&appuart->sem);
	return 0;
}

/* Wait  (block) until  all data  scheduled for  transmiting  has been
 * transmitted */
static void __wait_until_sent (struct appuart_serial *appuart)
{
	if (appuart->tx_buffer_index > 0)
		wait_event_interruptible(appuart->tx_dma_done, appuart->tx_buffer_index == 0);
}

static void __close_uart(struct appuart_serial *appuart)
{
	__wait_until_sent(appuart);
	uart_off();
}

/* Close  app UART, ensuring  that all  data have  been send  and then
 * power down device */
static void do_close(struct appuart_serial *appuart)
{
	down(&appuart->sem);

	if (!appuart->open_count) {
		/* port was never opened */
		goto exit;
	}

	if (--appuart->open_count <= 0) {
		/* The port is being closed by the last user. */
		/* Do any hardware specific stuff here */
		__close_uart(appuart);
	}
 exit:
	up(&appuart->sem);
}

/* Close app UART device */
static void appuart_close(struct tty_struct *tty, struct file *file)
{
	struct appuart_serial *appuart = tty->driver_data;

	if (appuart)
		do_close(appuart);
}   

/* Write data to device */
static int appuart_write (struct tty_struct *tty, const unsigned char *buffer, int count)
{
	struct appuart_serial *appuart = tty->driver_data;
	stmp37xx_dma_descriptor_t* free_descr;
	int retval = -EINVAL;

	if (!appuart)
		return -ENODEV;

	down(&appuart->sem);

	if (!appuart->open_count)
		/* port was not opened */
		goto exit;

	__wait_until_sent(appuart);

	circ_advance_free(&tx_dma_chain, circ_advance_cooked(&tx_dma_chain));

	if (tx_dma_chain.free_count == 0)
		BUG();

	if (count > TX_BUFFER_SIZE)
		count = TX_BUFFER_SIZE;

	free_descr = circ_get_free_head( &tx_dma_chain );
	memcpy( free_descr->virtual_buf_ptr, buffer, count );
	appuart->tx_buffer_index = count;

	update_tx_command(free_descr, appuart->tx_buffer_index);

	circ_advance_active(&tx_dma_chain, 1);

	retval = count;

 exit:
	up(&appuart->sem);
	return retval;
}

/* How much room is there available in the transmit buffer */
static int appuart_write_room(struct tty_struct *tty) 
{
	struct appuart_serial *appuart = tty->driver_data;
	int room = -EINVAL;

	if (!appuart)
		return -ENODEV;

	down(&appuart->sem);

	if (!appuart->open_count) {
		/* port was not opened */
		goto exit;
	}

	/* calculate how much room is left in the device */
	circ_advance_free( &tx_dma_chain, circ_advance_cooked( &tx_dma_chain ) );
	room = tx_dma_chain.free_count * TX_BUFFER_SIZE;
	room -= appuart->tx_buffer_index;
	if ( room == 0 ) printk(KERN_ERR "Run out of buffer space\n");

	exit:
	up(&appuart->sem);

	return room;
}

/* Send all data scheduled to be transmitted and then return */
static void appuart_flush_buffer(struct tty_struct *tty)
{
    struct appuart_serial *appuart = tty->driver_data;
    
    if (!appuart)
		return;

	down(&appuart->sem);
    if (appuart->tx_buffer_index > 0) {
		stmp37xx_dma_descriptor_t* free_descr = circ_get_free_head(&tx_dma_chain);
		update_tx_command(free_descr, appuart->tx_buffer_index);
		circ_advance_active( &tx_dma_chain, 1 );
		appuart->tx_buffer_index = 0;
    }
    up(&appuart->sem);
    
    return;
}

/* Set break line accordingly */
static void appuart_break_ctl(struct tty_struct *tty, int state)
{
	if ( state == -1 )
		BF_SET( UARTAPP_LINECTRL, BRK );
	else /* state == 0 */
		BF_CLR( UARTAPP_LINECTRL, BRK );
}

#define RELEVANT_IFLAG(iflag) ((iflag) & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))
/* Configure device */
static void appuart_set_termios(struct tty_struct *tty, struct ktermios *old_termios)
{
	unsigned int cflag;

	cflag = tty->termios->c_cflag;

	/* check that they really want us to change something */
	if (old_termios) {
		if ((cflag == old_termios->c_cflag) &&
			(RELEVANT_IFLAG(tty->termios->c_iflag) == 
			 RELEVANT_IFLAG(old_termios->c_iflag))) {
			printk(KERN_DEBUG " - nothing to change...\n");
			return;
		}
	}

	/* get the byte size */
	set_word_length( C_CSIZE(tty) );

	/* determine the parity */
	if (cflag & PARENB)
		parity_enable( cflag & PARODD );
	else
		parity_disable();

	/* figure out the stop bits requested */
	if (cflag & CSTOPB)
		BF_SET(UARTAPP_LINECTRL, STP2);
	else
		BF_CLR(UARTAPP_LINECTRL, STP2);     

	/* figure out the hardware flow control settings */
	if (cflag & CRTSCTS)
		hardware_flowcontrol( 1 );
	else
		hardware_flowcontrol( 0 );

	/* determine software flow control */
	/* if we are implementing XON/XOFF, set the start and 
	 * stop character in the device */
/* 	if (I_IXOFF(tty) || I_IXON(tty)) { */
/* 		unsigned char stop_char  = STOP_CHAR(tty); */
/* 		unsigned char start_char = START_CHAR(tty); */

/* 		/\* if we are implementing INBOUND XON/XOFF *\/ */
/* 		if (I_IXOFF(tty)) */
/* 		{ */
/* 			printk(KERN_DEBUG " - INBOUND XON/XOFF is enabled, " */
/* 				"XON = %2x, XOFF = %2x", start_char, stop_char); */
/* 		} */
/* 		else */
/* 			printk(KERN_DEBUG" - INBOUND XON/XOFF is disabled"); */

/* 		/\* if we are implementing OUTBOUND XON/XOFF *\/ */
/* 		if (I_IXON(tty)) */
/* 			printk(KERN_DEBUG" - OUTBOUND XON/XOFF is enabled, " */
/* 				"XON = %2x, XOFF = %2x", start_char, stop_char); */
/* 		else */
/* 			printk(KERN_DEBUG" - OUTBOUND XON/XOFF is disabled"); */
/* 	} */

	/* get the baud rate wanted */
	set_speed( tty_get_baud_rate(tty) );    
}

/* Our fake UART values */
#define MCR_DTR		0x01
#define MCR_RTS		0x02
#define MCR_LOOP	0x04
#define MSR_CTS		0x08
#define MSR_CD		0x10
#define MSR_RI		0x20
#define MSR_DSR		0x40

/* Set modem  control state. Since only  a very small  subset of modem
 * control is  implemented, the settings are  just kept in  a bit flag
 * (this is following the suggestion by the Linux Device Drivers book,
 * 3rd edition)
 */
static int appuart_tiocmget(struct tty_struct *tty, struct file *file)
{
	struct appuart_serial *appuart = tty->driver_data;

	unsigned int result = 0;
	unsigned int msr = appuart->msr;
	unsigned int mcr = appuart->mcr;

	/* We only support CTS line read; hardware does not support
	 * anything else
	 */
	msr &= ~MSR_CTS;
	if ( BF_RD( UARTAPP_STAT, CTS ) ) msr &= MSR_CTS;

	result = ((mcr & MCR_DTR)  ? TIOCM_DTR  : 0) |	/* DTR is set */
			 ((mcr & MCR_RTS)  ? TIOCM_RTS  : 0) |	/* RTS is set */
			 ((mcr & MCR_LOOP) ? TIOCM_LOOP : 0) |	/* LOOP is set */
			 ((msr & MSR_CTS)  ? TIOCM_CTS  : 0) |	/* CTS is set */
			 ((msr & MSR_CD)   ? TIOCM_CAR  : 0) |	/* Carrier detect is set*/
			 ((msr & MSR_RI)   ? TIOCM_RI   : 0) |	/* Ring Indicator is set */
			 ((msr & MSR_DSR)  ? TIOCM_DSR  : 0);	/* DSR is set */

	return result;
}

static int appuart_tiocmset(struct tty_struct *tty, struct file *file,
							unsigned int set, unsigned int clear)
{
	struct appuart_serial *appuart = tty->driver_data;
	unsigned int mcr = appuart->mcr;

	if (set & TIOCM_RTS)
		mcr |= MCR_RTS;
	if (set & TIOCM_DTR)
		mcr |= MCR_RTS;

	if (clear & TIOCM_RTS)
		mcr &= ~MCR_RTS;
	if (clear & TIOCM_DTR)
		mcr &= ~MCR_RTS;

	/* set the new MCR value in the device */
	appuart->mcr = mcr;
	return 0;
}

/* Display app UART registers values on proc tty driver read */
static int appuart_read_proc(char *page, char **start, off_t off, int count,
							 int *eof, void *data)
{
	struct appuart_serial *appuart;
	off_t begin = 0;
	int length = 0;
	int i;

	length += sprintf(page+length, "appuartserinfo:1.0 driver:%s\n", DRIVER_VERSION);
	length += sprintf(page+length, 
					  "RCV CTRL0: 0x%x, TRM CTRL1: 0x%x\n"
					  "CTR REG  : 0x%x\n"
					  "LIN CTRL0: 0x%x, LIN CTRL1: 0x%x\n"
					  "INT REG  : 0x%x, DAT REG  : 0x%x\n"
					  "STAT REG : 0x%x, DGB REG  : 0x%x\n",
					  HW_UARTAPP_CTRL0_RD(), HW_UARTAPP_CTRL1_RD(),
					  HW_UARTAPP_CTRL2_RD(),
					  HW_UARTAPP_LINECTRL_RD(), HW_UARTAPP_LINECTRL2_RD(),
					  HW_UARTAPP_INTR_RD(), HW_UARTAPP_DATA_RD(),
					  HW_UARTAPP_STAT_RD(), HW_UARTAPP_DEBUG_RD()
					 );
	for (i = 0; i < APPUART_TTY_MINORS && length < PAGE_SIZE; ++i) {
		appuart = appuart_table[i];
		if (appuart == NULL)
			continue;

		length += sprintf(page+length, "%d\n", i);
		if ((length + begin) > (off + count))
			goto done;
		if ((length + begin) < off) {
			begin += length;
			length = 0;
		}
	}
	*eof = 1;
	done:
	if (off >= (length + begin))
		return 0;
	*start = page + (off-begin);
	return(count < begin+length-off) ? count : begin + length-off;
}

static int appuart_ioctl_tiocgserial (struct tty_struct *tty, struct file *file,
						 unsigned int cmd, unsigned long arg)
{
	struct appuart_serial *appuart = tty->driver_data;
	if (cmd == TIOCGSERIAL) {
		struct serial_struct tmp;

		if (!arg)
			return -EFAULT;

		memset(&tmp, 0, sizeof(tmp));

		tmp.type        = appuart->serial.type;
		tmp.line        = appuart->serial.line;
		tmp.port        = appuart->serial.port;
		tmp.irq         = appuart->serial.irq;
		tmp.flags       = ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ;
		tmp.xmit_fifo_size  = appuart->serial.xmit_fifo_size;
		tmp.baud_base       = appuart->serial.baud_base;
		tmp.close_delay     = 5*HZ;
		tmp.closing_wait    = 30*HZ;
		tmp.custom_divisor  = appuart->serial.custom_divisor;
		tmp.hub6        = appuart->serial.hub6;
		tmp.io_type     = appuart->serial.io_type;

		if (copy_to_user((void __user *)arg, &tmp, sizeof(struct serial_struct)))
			return -EFAULT;
		return 0;
	}

	return -ENOIOCTLCMD;
}

static int appuart_ioctl_tiocmiwait(struct tty_struct *tty, struct file *file,
						 unsigned int cmd, unsigned long arg)
{
	struct appuart_serial *appuart = tty->driver_data;

	if (cmd == TIOCMIWAIT) {
		DECLARE_WAITQUEUE(wait, current);
		struct async_icount cnow;
		struct async_icount cprev;

		cprev = appuart->icount;
		while (1) {
			add_wait_queue(&appuart->wait, &wait);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			remove_wait_queue(&appuart->wait, &wait);

			/* see if a signal woke us up */
			if (signal_pending(current))
				return -ERESTARTSYS;

			cnow = appuart->icount;
			if (cnow.rng == cprev.rng && cnow.dsr == cprev.dsr &&
				cnow.dcd == cprev.dcd && cnow.cts == cprev.cts)
				return -EIO; /* no change => error */
			if (((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
				((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
				((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
				((arg & TIOCM_CTS) && (cnow.cts != cprev.cts)) ) {
				return 0;
			}
			cprev = cnow;
		}

	}
	return -ENOIOCTLCMD;
}

static int appuart_ioctl_tiocgicount(struct tty_struct *tty, struct file *file,
						 unsigned int cmd, unsigned long arg)
{
	struct appuart_serial *appuart = tty->driver_data;

	if (cmd == TIOCGICOUNT) {
		struct async_icount cnow = appuart->icount;
		struct serial_icounter_struct icount;

		icount.cts  = cnow.cts;
		icount.dsr  = cnow.dsr;
		icount.rng  = cnow.rng;
		icount.dcd  = cnow.dcd;
		icount.rx   = cnow.rx;
		icount.tx   = cnow.tx;
		icount.frame    = cnow.frame;
		icount.overrun  = cnow.overrun;
		icount.parity   = cnow.parity;
		icount.brk  = cnow.brk;
		icount.buf_overrun = cnow.buf_overrun;

		if (copy_to_user((void __user *)arg, &icount, sizeof(icount)))
			return -EFAULT;
		return 0;
	}
	return -ENOIOCTLCMD;
}

/* the  real appuart_ioctl  function.  The  above is  done to  get the
 * small functions in the book */
static int appuart_ioctl(struct tty_struct *tty, struct file *file,
						 unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case TIOCGSERIAL:
		return appuart_ioctl_tiocgserial(tty, file, cmd, arg);
	case TIOCMIWAIT:
		return appuart_ioctl_tiocmiwait(tty, file, cmd, arg);
	case TIOCGICOUNT:
		return appuart_ioctl_tiocgicount(tty, file, cmd, arg);
	case TIOCLOOPBACK:
		loopback( arg );
		return 0;
	}

	return -ENOIOCTLCMD;
}

/* Prepare the rx DMA chain */
static void __init prepare_rx_chain(void)
{
	hw_uartapp_ctrl0_t rx_cmd;    
	void* virtual_addr;
#if RX_PAIRS
	int pair_i;
#else
	int i;
#endif /* RX_PAIRS */

	if (!make_circular_dma_chain( &dma_user, &rx_dma_chain, rx_dma_descrs, RX_CHAIN_LEN) )
		BUG();

#if !RX_PAIRS
	for (i = 0; i < RX_CHAIN_LEN; i++) {
		stmp37xx_dma_descriptor_t* descr = &rx_dma_descrs[i];
		descr->command->cmd =
		BF_APBX_CHn_CMD_XFER_COUNT(RX_BUFFER_SIZE) |
		BF_APBX_CHn_CMD_CMDWORDS(1) |
		BF_APBX_CHn_CMD_WAIT4ENDCMD(1) |
		BF_APBX_CHn_CMD_SEMAPHORE(1) |
		BF_APBX_CHn_CMD_IRQONCMPLT(1) |
		BF_APBX_CHn_CMD_CHAIN(1) |
		BV_FLD(APBH_CHn_CMD,COMMAND, DMA_WRITE);

		virtual_addr = dma_alloc_coherent( &app_uart_device.dev,
										   RX_BUFFER_SIZE,
										   &rx_dma_buffers[i],
										   GFP_DMA);
		memset( virtual_addr, 0x4C, RX_BUFFER_SIZE);
		descr->command->buf_ptr = rx_dma_buffers[i];
		descr->virtual_buf_ptr = virtual_addr;

		rx_cmd.U = 0;
		rx_cmd.B.RUN = 1;
		rx_cmd.B.RX_SOURCE = 0;
		rx_cmd.B.RXTO_ENABLE = 1;
		rx_cmd.B.RXTIMEOUT = 0x03;
		rx_cmd.B.XFER_COUNT = RX_BUFFER_SIZE;
		descr->command->pio_words[0] = rx_cmd.U;
	};
#else
	/* RX CHAIN
	 * Note that for each DMA buffer, two dma commands are required:
	 * The first to copy the data from the RX channel, the second to
	 * copy the receive count from the status register.
	 */

	/* The idea is there are pairs of commands; The first reads the
	 * data from the dma channels as usual.  The second, when first is
	 * completed, reads the status register (rx size, any errors) to
	 * a position in the RX buffer. Extra padding is created for this
	 * purpose.
	 */
	for (pair_i = 0; pair_i < RX_CHAIN_LEN; pair_i+=2) {
		stmp37xx_dma_descriptor_t* descr = &rx_dma_descrs[pair_i];

		const unsigned status_size = sizeof(hw_uartapp_stat_t);
		const unsigned ex_buffer_size = RX_BUFFER_SIZE + status_size;
		const unsigned buffer_index = pair_i/2;

		descr->command->cmd =
		BF_APBX_CHn_CMD_XFER_COUNT(RX_BUFFER_SIZE) |
		BF_APBX_CHn_CMD_CMDWORDS(1) |
		BF_APBX_CHn_CMD_WAIT4ENDCMD(1) |
		BF_APBX_CHn_CMD_SEMAPHORE(1) |
		BF_APBX_CHn_CMD_IRQONCMPLT(0) |
		BF_APBX_CHn_CMD_CHAIN(1) |
		BV_FLD(APBX_CHn_CMD,COMMAND, DMA_WRITE);

		virtual_addr = dma_alloc_coherent( &app_uart_device.dev,
										   ex_buffer_size,
										   &rx_dma_buffers[buffer_index],
										   GFP_DMA);
		memset( virtual_addr, 0x4C + pair_i, ex_buffer_size);
		descr->command->buf_ptr = rx_dma_buffers[buffer_index];
		descr->virtual_buf_ptr  = virtual_addr;	/* Going to access STATUS from that pointer too */

		rx_cmd.U = 0;
		rx_cmd.B.RUN = 1;
		rx_cmd.B.RX_SOURCE = 0;
		rx_cmd.B.RXTO_ENABLE = 1;  
		rx_cmd.B.RXTIMEOUT = 0x03;  
		rx_cmd.B.XFER_COUNT = RX_BUFFER_SIZE; 

		descr->command->pio_words[0] = rx_cmd.U;

		/* Copy status register at the beginning of buffer
		 */
		descr = &rx_dma_descrs[pair_i+1];
		descr->command->cmd =
		BF_APBX_CHn_CMD_XFER_COUNT(status_size) |
		BF_APBX_CHn_CMD_CMDWORDS(1) |
		BF_APBX_CHn_CMD_WAIT4ENDCMD(0) |
		BF_APBX_CHn_CMD_SEMAPHORE(1) |
		BF_APBX_CHn_CMD_IRQONCMPLT(1) |
		BF_APBX_CHn_CMD_CHAIN(1) |
		BV_FLD(APBX_CHn_CMD,COMMAND, DMA_WRITE);
		descr->command->buf_ptr = rx_dma_buffers[buffer_index] + RX_BUFFER_SIZE;
		descr->virtual_buf_ptr = virtual_addr + RX_BUFFER_SIZE;

		rx_cmd.B.RX_SOURCE = 1;
		rx_cmd.B.XFER_COUNT = status_size;
		descr->command->pio_words[0] = rx_cmd.U;


	};
#endif /* !RX_PAIRS */    
}

/* Allocate and initialise rx and tx DMA chains */
static void __init dma_init(void)
{
	hw_uartapp_ctrl1_t tx_cmd;
	void* virtual_addr;
	int i;

	stmp37xx_dma_user_init(&app_uart_device.dev, &dma_user);

	/* TX chain */
	if (!make_circular_dma_chain( &dma_user, &tx_dma_chain, tx_dma_descrs, TX_CHAIN_LEN)) {
		BUG();
	}

	for (i = 0; i < TX_CHAIN_LEN; i++) {
		stmp37xx_dma_descriptor_t* descr = &tx_dma_descrs[i];
		descr->command->cmd =
		BF_APBX_CHn_CMD_XFER_COUNT(TX_BUFFER_SIZE) |
		BF_APBX_CHn_CMD_CMDWORDS(1) |
		BF_APBX_CHn_CMD_WAIT4ENDCMD(1) |
		BF_APBX_CHn_CMD_SEMAPHORE(1) |
		BF_APBX_CHn_CMD_IRQONCMPLT(1) |
		BF_APBX_CHn_CMD_CHAIN(1) |
		BV_FLD(APBX_CHn_CMD,COMMAND, DMA_READ);

		virtual_addr = dma_alloc_coherent( &app_uart_device.dev,
										   TX_BUFFER_SIZE,
										   &tx_dma_buffers[i],
										   GFP_DMA);
		memset( virtual_addr, 0x4B, TX_BUFFER_SIZE );
		descr->command->buf_ptr = tx_dma_buffers[i];
		descr->virtual_buf_ptr = virtual_addr;

		tx_cmd.U = 0;
		tx_cmd.B.RUN = 1;
		tx_cmd.B.XFER_COUNT = TX_BUFFER_SIZE;
		descr->command->pio_words[0] = tx_cmd.U;
	};

	prepare_rx_chain();

	/* Just to make certain that the buffers are clean what's so over */
	arm926_flush_kern_cache_all();
}


static struct tty_operations serial_ops = {
	.open = appuart_open,
	.close = appuart_close,
	.write = appuart_write,
	.write_room = appuart_write_room,
	.flush_buffer = appuart_flush_buffer,
/* 	.wait_until_sent= appuart_wait_until_sent, */
	.set_termios = appuart_set_termios,
	.break_ctl = appuart_break_ctl,
};

static int __init appuart_init(void)
{
	int retval;
	int i;

	/* allocate the tty driver */
	appuart_tty_driver = alloc_tty_driver(APPUART_TTY_MINORS);
	if (!appuart_tty_driver)
		return -ENOMEM;

	/* initialize the tty driver */
	appuart_tty_driver->owner = THIS_MODULE;
	appuart_tty_driver->driver_name = "ttySA";
	appuart_tty_driver->name = "ttySA";	// Sigmatel APP UART
	appuart_tty_driver->major = APPUART_TTY_MAJOR,
	appuart_tty_driver->type = TTY_DRIVER_TYPE_SERIAL,
	appuart_tty_driver->subtype = SERIAL_TYPE_NORMAL,
	appuart_tty_driver->flags = TTY_DRIVER_REAL_RAW	| TTY_DRIVER_DYNAMIC_DEV;
	appuart_tty_driver->init_termios = tty_std_termios;
	appuart_tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	tty_set_operations(appuart_tty_driver, &serial_ops);

	/* hack to make the book purty, yet still use these functions in the
	 * real driver.  They really should be set up in the serial_ops
	 * structure above... */
	appuart_tty_driver->read_proc = appuart_read_proc;
	appuart_tty_driver->tiocmget = appuart_tiocmget;
	appuart_tty_driver->tiocmset = appuart_tiocmset;
	appuart_tty_driver->ioctl = appuart_ioctl;

	/* register the tty driver */
	retval = tty_register_driver(appuart_tty_driver);
	if (retval) {
		put_tty_driver(appuart_tty_driver);
		return retval;
	}

	retval = platform_device_register(&app_uart_device); 

	if ( retval ) {
		platform_device_unregister(&app_uart_device);
		/* @todo: Provide an proper handling in case of failure */
		printk( "Error: Can't register %s device\n", app_uart_device.name);
		BUG();
	}

	for (i = 0; i < APPUART_TTY_MINORS; ++i)
		tty_register_device(appuart_tty_driver, i, &app_uart_device.dev);

	/* dma chain init */
	dma_init();

	printk(KERN_INFO DRIVER_DESC " " DRIVER_VERSION);
	return retval;
}

static void __exit appuart_exit(void)
{
	struct appuart_serial *appuart;
	int i;

	for (i = 0; i < APPUART_TTY_MINORS; ++i)
		tty_unregister_device(appuart_tty_driver, i);
	tty_unregister_driver(appuart_tty_driver);

	/* shut down all of the timers and free the memory */
	for (i = 0; i < APPUART_TTY_MINORS; ++i) {
		appuart = appuart_table[i];
		if (appuart) {
			/* close the port */
			while (appuart->open_count)
				do_close(appuart);

			kfree(appuart);
			appuart_table[i] = NULL;

		}
	}

	for (i = 0; i < TX_CHAIN_LEN; i++) {
		dma_free_coherent( &app_uart_device.dev,
						   TX_BUFFER_SIZE,
						   tx_dma_descrs[i].virtual_buf_ptr,
						   tx_dma_buffers[i] );
	}

	for (i = 0; i < RX_CHAIN_LEN; i++) {
		dma_free_coherent( &app_uart_device.dev,
						   RX_BUFFER_SIZE,
						   rx_dma_descrs[i].virtual_buf_ptr,
						   rx_dma_buffers[i] );

	}    

	stmp37xx_dma_user_destroy( &dma_user );     
}

module_init(appuart_init);
module_exit(appuart_exit);

