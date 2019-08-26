/*
 *  linux/drivers/serial/stmp37xx_dbg.c
 *
 *  Driver for AMBA serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *  Modifications for STMP37XX Debug Serial (c) 2005 Sigmatel Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is a generic driver for ARM AMBA-type serial ports.  They
 * have a lot of 16550-like features, but are not register compatible.
 * Note that although they do have CTS, DCD and DSR inputs, they do
 * not have an RI input, nor do they have DTR or RTS outputs.  If
 * required, these have to be supplied via some other means (eg, GPIO)
 * and hooked into this driver.
 */
#include <linux/autoconf.h>

#if defined(CONFIG_SERIAL_STMP37XX_DBG_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>

#define UART_NR		1

#define SERIAL_AMBA_MAJOR	204
#define SERIAL_AMBA_MINOR	16
#define SERIAL_AMBA_NR		UART_NR

#define AMBA_ISR_PASS_LIMIT	256

/*
 * Access macros for the AMBA UARTs
 */
#define UART_GET_INT_STATUS(p)	readb((p)->membase + UART010_IIR)
#define UART_PUT_ICR(p, c)	writel((c), (p)->membase + UART010_ICR)
#define UART_GET_FR(p)		readb((p)->membase + UART01x_FR)
#define UART_GET_CHAR(p)	readb((p)->membase + UART01x_DR)
#define UART_PUT_CHAR(p, c)	writel((c), (p)->membase + UART01x_DR)
#define UART_GET_RSR(p)		readb((p)->membase + UART01x_RSR)
#define UART_GET_CR(p)		readb((p)->membase + UART010_CR)
#define UART_PUT_CR(p,c)	writel((c), (p)->membase + UART010_CR)
#define UART_GET_LCRL(p)	readb((p)->membase + UART010_LCRL)
#define UART_PUT_LCRL(p,c)	writel((c), (p)->membase + UART010_LCRL)
#define UART_GET_LCRM(p)	readb((p)->membase + UART010_LCRM)
#define UART_PUT_LCRM(p,c)	writel((c), (p)->membase + UART010_LCRM)
#define UART_GET_LCRH(p)	readb((p)->membase + UART010_LCRH)
#define UART_PUT_LCRH(p,c)	writel((c), (p)->membase + UART010_LCRH)
#define UART_RX_DATA(s)		(((s) & UART01x_FR_RXFE) == 0)
#define UART_TX_READY(s)	(((s) & UART01x_FR_TXFF) == 0)
#define UART_TX_EMPTY(p)	((UART_GET_FR(p) & UART01x_FR_TMSK) == 0)

#define UART_DUMMY_RSR_RX	/*256*/0
#define UART_PORT_SIZE		64

/* Hardware access */
#define sreadw(x)		(*((volatile unsigned int*)(x)))
#define swritew(d,x)		*((volatile unsigned int*)(x))=(d)

/*
 * We wrap our port structure around the generic uart_port.
 */
struct uart_amba_port {
	struct uart_port	port;
	struct clk		*clk;
	unsigned int		im;	/* interrupt mask */
	unsigned int		old_status;
};


static void stmp37xx_dbg_stop_tx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im &= ~UART011_TXIM;
	swritew(uap->im, uap->port.membase + UART011_IMSC);
}

static void stmp37xx_dbg_start_tx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im |= UART011_TXIM;
	swritew(uap->im, uap->port.membase + UART011_IMSC);
}

static void stmp37xx_dbg_stop_rx(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im &= ~(UART011_RXIM|UART011_RTIM|UART011_FEIM|
		     UART011_PEIM|UART011_BEIM|UART011_OEIM);
	swritew(uap->im, uap->port.membase + UART011_IMSC);
}

static void stmp37xx_dbg_enable_ms(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;

	uap->im |= UART011_RIMIM|UART011_CTSMIM|UART011_DCDMIM|UART011_DSRMIM;
	swritew(uap->im, uap->port.membase + UART011_IMSC);
}

static void
stmp37xx_dbg_rx_chars(struct uart_amba_port *uap)
{
	struct tty_struct *tty = uap->port.info->tty;
	unsigned int status, ch, flag, rsr, max_count = 256;

	status = sreadw(uap->port.membase + UART01x_FR);
	while ((status & UART01x_FR_RXFE) == 0 && max_count--) {
#if 0
		// TODO: fathom out why this code stopped compiling in 2.6.16.1 and marge in other
		// changes from the standard amba serial driver.
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			if (tty->low_latency)
				tty_flip_buffer_push(tty);
			/*
			 * If this failed then we will throw away the
			 * bytes but must do so to clear interrupts
			 */
		}
#endif

		ch = sreadw(uap->port.membase + UART01x_DR);
		flag = TTY_NORMAL;
		uap->port.icount.rx++;

		/*
		 * Note that the error handling code is
		 * out of the main execution path
		 */
		rsr = sreadw(uap->port.membase + UART01x_RSR) | UART_DUMMY_RSR_RX;
		if (unlikely(rsr & UART01x_RSR_ANY)) {
			if (rsr & UART01x_RSR_BE) {
				rsr &= ~(UART01x_RSR_FE | UART01x_RSR_PE);
				uap->port.icount.brk++;
				if (uart_handle_break(&uap->port))
					goto ignore_char;
			} else if (rsr & UART01x_RSR_PE)
				uap->port.icount.parity++;
			else if (rsr & UART01x_RSR_FE)
				uap->port.icount.frame++;
			if (rsr & UART01x_RSR_OE)
				uap->port.icount.overrun++;

			rsr &= uap->port.read_status_mask;

			if (rsr & UART01x_RSR_BE)
				flag = TTY_BREAK;
			else if (rsr & UART01x_RSR_PE)
				flag = TTY_PARITY;
			else if (rsr & UART01x_RSR_FE)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(&uap->port, ch))
			goto ignore_char;

		uart_insert_char(&uap->port, rsr, UART01x_RSR_OE, ch, flag);

	ignore_char:
		status = sreadw(uap->port.membase + UART01x_FR);
	}
	tty_flip_buffer_push(tty);
	return;
}

static void stmp37xx_dbg_tx_chars(struct uart_amba_port *uap)
{
	struct circ_buf *xmit = &uap->port.info->xmit;
	int count;

	if (uap->port.x_char) {
		swritew(uap->port.x_char, uap->port.membase + UART01x_DR);
		uap->port.icount.tx++;
		uap->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&uap->port)) {
		stmp37xx_dbg_stop_tx(&uap->port);
		return;
	}

	count = uap->port.fifosize >> 1;
	do {
		swritew(xmit->buf[xmit->tail], uap->port.membase + UART01x_DR);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uap->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&uap->port);

	if (uart_circ_empty(xmit))
		stmp37xx_dbg_stop_tx(&uap->port);
}

static void stmp37xx_dbg_modem_status(struct uart_amba_port *uap)
{
	unsigned int status, delta;

	status = sreadw(uap->port.membase + UART01x_FR) & UART01x_FR_MODEM_ANY;

	delta = status ^ uap->old_status;
	uap->old_status = status;

	if (!delta)
		return;

	if (delta & UART01x_FR_DCD)
		uart_handle_dcd_change(&uap->port, status & UART01x_FR_DCD);

	if (delta & UART01x_FR_DSR)
		uap->port.icount.dsr++;

	if (delta & UART01x_FR_CTS)
		uart_handle_cts_change(&uap->port, status & UART01x_FR_CTS);

	wake_up_interruptible(&uap->port.info->delta_msr_wait);
}

static irqreturn_t stmp37xx_dbg_int(int irq, void *dev_id)
{
	struct uart_amba_port *uap = dev_id;
	unsigned int status, pass_counter = AMBA_ISR_PASS_LIMIT;
	int handled = 0;

	spin_lock(&uap->port.lock);

	status = sreadw(uap->port.membase + UART011_MIS);
	if (status) {
		do {
			swritew(status & ~(UART011_TXIS|UART011_RTIS|
					  UART011_RXIS),
			       uap->port.membase + UART011_ICR);

			if (status & (UART011_RTIS|UART011_RXIS))
				stmp37xx_dbg_rx_chars(uap);
			if (status & (UART011_DSRMIS|UART011_DCDMIS|
				      UART011_CTSMIS|UART011_RIMIS))
				stmp37xx_dbg_modem_status(uap);
			if (status & UART011_TXIS)
				stmp37xx_dbg_tx_chars(uap);

			if (pass_counter-- == 0)
				break;

			status = sreadw(uap->port.membase + UART011_MIS);
		} while (status != 0);
		handled = 1;
	}

	spin_unlock(&uap->port.lock);

	return IRQ_RETVAL(handled);
}

static unsigned int stmp37xx_dbg_tx_empty(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int status = sreadw(uap->port.membase + UART01x_FR);
	return status & (UART01x_FR_BUSY|UART01x_FR_TXFF) ? 0 : TIOCSER_TEMT;
}

static unsigned int stmp37xx_dbg_get_mctrl(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int result = 0;
	unsigned int status = sreadw(uap->port.membase + UART01x_FR);

#define tmp_BIT(uartbit, tiocmbit)		\
	if (status & uartbit)		\
		result |= tiocmbit

	tmp_BIT(UART01x_FR_DCD, TIOCM_CAR);
	tmp_BIT(UART01x_FR_DSR, TIOCM_DSR);
	tmp_BIT(UART01x_FR_CTS, TIOCM_CTS);
	tmp_BIT(UART011_FR_RI, TIOCM_RNG);
#undef tmp_BIT
	return result;
}

static void stmp37xx_dbg_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int cr;

	cr = sreadw(uap->port.membase + UART011_CR);

#define	tmp_BIT(tiocmbit, uartbit)		\
	if (mctrl & tiocmbit)		\
		cr |= uartbit;		\
	else				\
		cr &= ~uartbit

	tmp_BIT(TIOCM_RTS, UART011_CR_RTS);
	tmp_BIT(TIOCM_DTR, UART011_CR_DTR);
	tmp_BIT(TIOCM_OUT1, UART011_CR_OUT1);
	tmp_BIT(TIOCM_OUT2, UART011_CR_OUT2);
	tmp_BIT(TIOCM_LOOP, UART011_CR_LBE);
#undef tmp_BIT

	swritew(cr, uap->port.membase + UART011_CR);
}

static void stmp37xx_dbg_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned long flags;
	unsigned int lcr_h;

	spin_lock_irqsave(&uap->port.lock, flags);
	lcr_h = sreadw(uap->port.membase + UART011_LCRH);
	if (break_state == -1)
		lcr_h |= UART01x_LCRH_BRK;
	else
		lcr_h &= ~UART01x_LCRH_BRK;
	swritew(lcr_h, uap->port.membase + UART011_LCRH);
	spin_unlock_irqrestore(&uap->port.lock, flags);
}

static int stmp37xx_dbg_startup(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned int cr;
	int retval;

	uap->port.uartclk = 24000000;

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(uap->port.irq, stmp37xx_dbg_int, 0, "uart-pl011", uap);
	if (retval)
		return retval;

	swritew(UART011_IFLS_RX4_8|UART011_IFLS_TX4_8,
	       uap->port.membase + UART011_IFLS);

	/*
	 * Provoke TX FIFO interrupt into asserting.
	 */
#if 0	 
	cr = UART01x_CR_UARTEN | UART011_CR_TXE | UART011_CR_LBE;
	swritew(cr, uap->port.membase + UART011_CR);
	swritew(0, uap->port.membase + UART011_FBRD);
	swritew(1, uap->port.membase + UART011_IBRD);
	swritew(0, uap->port.membase + UART011_LCRH);
	swritew(0, uap->port.membase + UART01x_DR);
	while (sreadw(uap->port.membase + UART01x_FR) & UART01x_FR_BUSY)
		barrier();
#endif
	cr = UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE;
	swritew(cr, uap->port.membase + UART011_CR);

	/*
	 * initialise the old status of the modem signals
	 */
	uap->old_status = sreadw(uap->port.membase + UART01x_FR) & UART01x_FR_MODEM_ANY;

	/*
	 * Finally, enable interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	uap->im = UART011_RXIM | UART011_RTIM;
	swritew(uap->im, uap->port.membase + UART011_IMSC);
	spin_unlock_irq(&uap->port.lock);

	return 0;
}

static void stmp37xx_dbg_shutdown(struct uart_port *port)
{
	struct uart_amba_port *uap = (struct uart_amba_port *)port;
	unsigned long val;

	/*
	 * disable all interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	uap->im = 0;
	swritew(uap->im, uap->port.membase + UART011_IMSC);
	swritew(0xffff, uap->port.membase + UART011_ICR);
	spin_unlock_irq(&uap->port.lock);

	/*
	 * Free the interrupt
	 */
	free_irq(uap->port.irq, uap);

	/*
	 * disable the port
	 */
	swritew(UART01x_CR_UARTEN | UART011_CR_TXE, uap->port.membase + UART011_CR);

	/*
	 * disable break condition and fifos
	 */
	val = sreadw(uap->port.membase + UART011_LCRH);
	val &= ~(UART01x_LCRH_BRK | UART01x_LCRH_FEN);
	swritew(val, uap->port.membase + UART011_LCRH);
}

static void
stmp37xx_dbg_set_termios(struct uart_port *port, struct ktermios *termios,
		     struct ktermios *old)
{
	unsigned int lcr_h, old_cr;
	unsigned long flags;
	unsigned int baud, quot;

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
	quot = port->uartclk * 4 / baud;

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr_h = UART01x_LCRH_WLEN_5;
		break;
	case CS6:
		lcr_h = UART01x_LCRH_WLEN_6;
		break;
	case CS7:
		lcr_h = UART01x_LCRH_WLEN_7;
		break;
	default: // CS8
		lcr_h = UART01x_LCRH_WLEN_8;
		break;
	}
	if (termios->c_cflag & CSTOPB)
		lcr_h |= UART01x_LCRH_STP2;
	if (termios->c_cflag & PARENB) {
		lcr_h |= UART01x_LCRH_PEN;
		if (!(termios->c_cflag & PARODD))
			lcr_h |= UART01x_LCRH_EPS;
	}
	if (port->fifosize > 1)
		lcr_h |= UART01x_LCRH_FEN;

	spin_lock_irqsave(&port->lock, flags);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask = UART01x_RSR_OE;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART01x_RSR_FE | UART01x_RSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UART01x_RSR_BE;

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART01x_RSR_FE | UART01x_RSR_PE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= UART01x_RSR_BE;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= UART01x_RSR_OE;
	}

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_DUMMY_RSR_RX;

	if (UART_ENABLE_MS(port, termios->c_cflag))
		stmp37xx_dbg_enable_ms(port);
		
	/* first, disable everything */
	old_cr = sreadw(port->membase + UART011_CR);
	swritew(0, port->membase + UART011_CR);

	/* Set baud rate */
	swritew(quot & 0x3f, port->membase + UART011_FBRD);
	swritew(quot >> 6, port->membase + UART011_IBRD);

	/*
	 * ----------v----------v----------v----------v-----
	 * NOTE: MUST BE WRITTEN AFTER UARTLCR_M & UARTLCR_L
	 * ----------^----------^----------^----------^-----
	 */
	swritew(lcr_h, port->membase + UART011_LCRH);
	swritew(old_cr, port->membase + UART011_CR);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *stmp37xx_dbg_type(struct uart_port *port)
{
	return port->type == PORT_AMBA ? "AMBA/PL011" : NULL;
}


/*
 * Release the memory region(s) being used by 'port'
 */
static void stmp37xx_dbg_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, UART_PORT_SIZE);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int stmp37xx_dbg_request_port(struct uart_port *port)
{
	return request_mem_region(port->mapbase, UART_PORT_SIZE, "uart-pl010")
			!= NULL ? 0 : -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void stmp37xx_dbg_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_AMBA;
		stmp37xx_dbg_request_port(port);
	}
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int stmp37xx_dbg_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_AMBA)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops amba_stmp37xx_dbg_pops = {
	.tx_empty	= stmp37xx_dbg_tx_empty,
	.set_mctrl	= stmp37xx_dbg_set_mctrl,
	.get_mctrl	= stmp37xx_dbg_get_mctrl,
	.stop_tx	= stmp37xx_dbg_stop_tx,
	.start_tx	= stmp37xx_dbg_start_tx,
	.stop_rx	= stmp37xx_dbg_stop_rx,
	.enable_ms	= stmp37xx_dbg_enable_ms,
	.break_ctl	= stmp37xx_dbg_break_ctl,
	.startup	= stmp37xx_dbg_startup,
	.shutdown	= stmp37xx_dbg_shutdown,
	.set_termios	= stmp37xx_dbg_set_termios,
	.type		= stmp37xx_dbg_type,
	.release_port	= stmp37xx_dbg_release_port,
	.request_port	= stmp37xx_dbg_request_port,
	.config_port	= stmp37xx_dbg_config_port,
	.verify_port	= stmp37xx_dbg_verify_port,
};

static struct uart_amba_port stmp37xx_dbg_ports[UART_NR] = {
	{
		.port	= {
			.membase	= (void *)HW_UARTDBGDR_ADDR,		/* This *is* the virtual address */
			.mapbase	= HW_UARTDBGDR_ADDR,
			.iotype		= UPIO_MEM,
			.irq		= IRQ_DEBUG_UART,
			.uartclk	= 24000000,
			.fifosize	= 16,
			.ops		= &amba_stmp37xx_dbg_pops,
			.flags		= UPF_IOREMAP | UPF_BOOT_AUTOCONF,
			.line		= 0,
		},
	}
};

#ifdef CONFIG_SERIAL_STMP37XX_DBG_CONSOLE

static void
stmp37xx_dbg_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &stmp37xx_dbg_ports[co->index].port;
	unsigned int status, old_cr;
	int i;

	/*
	 *	First save the CR then disable the interrupts
	 */
	old_cr = UART_GET_CR(port);
	UART_PUT_CR(port, UART01x_CR_UARTEN);

	/*
	 *	Now, do each character
	 */
	for (i = 0; i < count; i++) {
		do {
			status = UART_GET_FR(port);
		} while (!UART_TX_READY(status));
		UART_PUT_CHAR(port, s[i]);
		if (s[i] == '\n') {
			do {
				status = UART_GET_FR(port);
			} while (!UART_TX_READY(status));
			UART_PUT_CHAR(port, '\r');
		}
	}

	/*
	 *	Finally, wait for transmitter to become empty
	 *	and restore the TCR
	 */
	do {
		status = UART_GET_FR(port);
	} while (status & UART01x_FR_BUSY);
	UART_PUT_CR(port, old_cr);
}

static void __init
stmp37xx_dbg_console_get_options(struct uart_port *port, int *baud,
			     int *parity, int *bits)
{
	if (UART_GET_CR(port) & UART01x_CR_UARTEN) {
		unsigned int lcr_h, quot;
		lcr_h = UART_GET_LCRH(port);

		*parity = 'n';
		if (lcr_h & UART01x_LCRH_PEN) {
			if (lcr_h & UART01x_LCRH_EPS)
				*parity = 'e';
			else
				*parity = 'o';
		}

		if ((lcr_h & 0x60) == UART01x_LCRH_WLEN_7)
			*bits = 7;
		else
			*bits = 8;

		quot = UART_GET_LCRL(port) | UART_GET_LCRM(port) << 8;
		*baud = port->uartclk / (16 * (quot + 1));
	}
}

static int __init stmp37xx_dbg_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index >= UART_NR)
		co->index = 0;
	port = &stmp37xx_dbg_ports[co->index].port;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		stmp37xx_dbg_console_get_options(port, &baud, &parity, &bits);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver stmp37xx_dbg_reg;
static struct console stmp37xx_dbg_console = {
	.name		= "ttySAM",
	.write		= stmp37xx_dbg_console_write,
	.device		= uart_console_device,
	.setup		= stmp37xx_dbg_console_setup,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &stmp37xx_dbg_reg,
};

static int __init stmp37xx_dbg_console_init(void)
{
	/*
	 * All port initializations are done statically
	 */
	register_console(&stmp37xx_dbg_console);
	return 0;
}
console_initcall(stmp37xx_dbg_console_init);

static int __init stmp37xx_dbg_late_console_init(void)
{
	if (!(stmp37xx_dbg_console.flags & CON_ENABLED))
		register_console(&stmp37xx_dbg_console);
	return 0;
}
late_initcall(stmp37xx_dbg_late_console_init);

#define STMP37XX_DBG_CONSOLE	&stmp37xx_dbg_console
#else
#define STMP37XX_DBG_CONSOLE	NULL
#endif

static struct uart_driver stmp37xx_dbg_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "ttySAM",
	.dev_name		= "ttySAM",
	.major			= SERIAL_AMBA_MAJOR,
	.minor			= SERIAL_AMBA_MINOR,
	.nr			= UART_NR,
	.cons			= STMP37XX_DBG_CONSOLE,
};

static int __init stmp37xx_dbg_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: STMP37XX DBG driver\n");

	ret = uart_register_driver(&stmp37xx_dbg_reg);	
	uart_add_one_port(&stmp37xx_dbg_reg, &stmp37xx_dbg_ports[0].port);

	return ret;
}

static void __exit stmp37xx_dbg_exit(void)
{
	uart_unregister_driver(&stmp37xx_dbg_reg);
}

module_init(stmp37xx_dbg_init);
module_exit(stmp37xx_dbg_exit);

MODULE_AUTHOR("ARM Ltd/Deep Blue Solutions Ltd/Sigmatel Inc");
MODULE_DESCRIPTION("ARM AMBA serial port driver $Revision: 1.1.2.2 $");
MODULE_LICENSE("GPL");
