/**
 * Version: $Id: mep_hostHw.c,v 1.1 2006/02/22 23:29:03 tknoh Exp $
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
 * This file implements all of the MepLib functions required to
 * port MepLib to an Atmel AVR AT90S8535 processor.  
 *
 *    - Changes to support other processors from the AVR family
 *      should be minimal.
 *    - This particular port is designed to be compiled using the
 *      WINAVR port of gcc (The Gnu C Compiler) for the AVR family.
 *
 */

//#include <stdio.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <avr/signal.h>

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/fs.h>				//for register_chrdev()
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/types.h>
#include <asm/arch/irqs.h>
#include <asm/arch/digctl.h>
#include <asm/arch/37xx/regs.h>
#include <asm/arch/37xx/regsdigctl.h>
#include <asm/arch/37xx/regslradc.h>
#include <asm/arch/37xx/regspinctrl.h>


//#include <asm/arch/mepLib.h>
//#include <asm/arch/mep_config.h>
#include "mepLib.h"
#include "mep_config.h"

/*****************************************************************
 * Make the pin assignments symbolic within this file so that
 * they are easy to reassign, if required.
 */
//#if !defined P_ACK
/* The ACK signal will be a push-pull OUTPUT signal connected to PC5 */
/*
#define DDR_ACK         DDRC
#define PORT_ACK        PORTC
#define PIN_ACK         PINC
#define P_ACK           PC5
*/
//#endif /* !defined P_ACK */

//#if !defined P_DATA
/* The DATA signal is an open-drain INPUT/OUPUT signal connected to PC4 */
/*
#define DDR_DATA        DDRC
#define PORT_DATA       PORTC
#define PIN_DATA        PINC
#define P_DATA          PC4
*/
//#endif /* !defined P_DATA */

//#if !defined P_CLK
/* The CLK signal is an INPUT signal connected to PD3/INT1 */
/*
#define DDR_CLK         DDRD
#define PORT_CLK        PORTD
#define PIN_CLK         PIND
#define P_CLK           PD3
*/
//#endif /* !defined P_CLK */

#if defined MEP_CONFIG_INTERRUPT_MODE || defined MEP_CONFIG_HYBRID_MODE
/*****************************************************************
 * The MEP interrupt control functions only need to be defined
 * for INTERRUPT or HYBRID configurations.  
 * 
 * The test hardware wires CLK to the AVR INT1 interrupt pin.
 */
void mep_pl_intEnable(void)
{
	//printk("mep_pl_intEnable(void)-------------------------------------------------\n");
	/* Enable GPIO irq */
	HW_PINCTRL_IRQEN0_SET(0x1 << 18);		
	


}

void mep_pl_intDisable(void)
{
	//printk("mep_pl_intDISable(void)-------------------------------------------------\n");
	/* Disable GPIO irq */
	HW_PINCTRL_IRQEN0_CLR(0x1 << 18);			
}

#endif /* defined MEP_CONFIG_INTERRUPT_MODE || defined MEP_CONFIG_HYBRID_MODE */
//-------------------------------------------------insert
void mep_pl_intDisable(void)
{
	//printk("mep_pl_intDISable(void)-------------------------------------------------\n");
	/* Disable GPIO irq */
	HW_PINCTRL_IRQEN0_CLR(0x1 << 18);			
}



/*****************************************************************
 * Perform the host-specific operations required to initialize
 * the MEP physical layer hardware:
 *
 *  - init the DATA interrupt input to be active LOW and DISABLED.
 *
 *  - init the ACK IO as an push-pull OUTPUT; initial value: 1 (HIGH).
 *  - init the DATA IO as an open-drain OUTPUT; initial value: 1 (HIGH).
 *  - init the CLK IO as an INPUT
 *
 * A pullup is activated on CLK to protect against a floating
 * input in case no touchpad has been connected.
 */
void mep_pl_init(void)
{
	//printk("_2_void mep_pl_init(void)\n");

	//HW_PINCTRL_MUXSEL1_SET(0x003c0000);	//GPIO0:26(SSP_CMD), GPIO0:25(SSP_DETECT)
	//HW_PINCTRL_MUXSEL3_SET(0x00000c00);	//GPIO1:21(LCD_BUSY)
	//HW_PINCTRL_MUXSEL6_SET(0xc0000000);	//GPIO3:15(ROTARY_A)	
	HW_PINCTRL_MUXSEL0_SET(0x3 << 16);	//bank0:8, ack
	HW_PINCTRL_MUXSEL0_SET(0x3 << 18);	//bank0:9, data
	HW_PINCTRL_MUXSEL1_SET(0x3 << 4);	//bank0:18, clock	
	
	/* 1. Configure the CLK GPIO as an INPUT */
	/* Set bank0:18 to input Enable for using "CLK" signal */
	HW_PINCTRL_DOE0_CLR(0x1 << 18);
	
	/* 2. Configure the ACK GPIO as an OUTPUT(push-pull:HW) HIGH */	
	/* Set bank0:8 to Output High Enable for using "ACK" signal */
	HW_PINCTRL_DOE0_SET(0x1 << 8); //output
	HW_PINCTRL_DOUT0_SET(0x1 << 8); //high		

	/* 3. Configure the DATA GPIO as an OUTPUT(open-drain:HW) HIGH */
	/* Set bank0:9 to Output High Enable for using "DATA" signal */
	HW_PINCTRL_DOE0_SET(0x1 << 9);			
	HW_PINCTRL_DOUT0_SET(0x1 << 9);		

	mep_pl_setDATA(1);
#if 0
	printk("HW_PINCTRL_MUXSEL0=0x%08x\n", HW_PINCTRL_MUXSEL0);
	printk("HW_PINCTRL_MUXSEL1=0x%08x\n", HW_PINCTRL_MUXSEL1);
	printk("HW_PINCTRL_DOE0=0x%08x\n", HW_PINCTRL_DOE0);
	printk("HW_PINCTRL_DOUT0=0x%08x\n", HW_PINCTRL_DOUT0);
	printk("HW_PINCTRL_DIN0=0x%08x\n", HW_PINCTRL_DIN0);
#endif
}
/*****************************************************************
 * Returns current state of DATA: [0=LOW, non-zero=HIGH]
 */
unsigned char mep_pl_getDATA(void)
{
	unsigned char get_data;
	u32 temp;
	
	/* Configue the DATA GPIO as an INPUT */
	HW_PINCTRL_DOE0_CLR(0x1 << 9);	

	//udelay(1000);

	/* Read from the DATA GPIO */
	temp = HW_PINCTRL_DIN0_RD();
	temp &= 0x00000200;
	get_data = (unsigned char) (temp >> 9);
	
    return get_data;
}


/*****************************************************************
 * Returns current state of CLK: [0=LOW, non-zero=HIGH]
 */
unsigned char mep_pl_getCLK(void)
{
	unsigned char get_clk;
	u32 temp;
	
	/* Configue the CLK GPIO as an INPUT */
	//HW_PINCTRL_DOE0_CLR(0x1 << 25);	//already done in mep_pl_init()

	//udelay(1000);

	/* Read from the CLK GPIO */
	temp = HW_PINCTRL_DIN0_RD();
//printk("HW_PINCTRL_DIN0_RD = 0x%08x\n\n", HW_PINCTRL_DIN0_RD() ); //dhsong	
//printk("HW_PINCTRL_DIN0 = 0x%08x\n\n", HW_PINCTRL_DIN0 ); //dhsong	
	//temp &= 0x02000000;
	temp &= 0x00040000;
//printk("temp = 0x%08x\n\n", temp ); //dhsong	
	get_clk = (unsigned char) (temp >> 18);
//printk("get_clk = 0x%08x\n\n", get_clk); //dhsong	
//printk("get_clk = %d\n\n", get_clk); //dhsong	
    return get_clk;
}


/*****************************************************************
 * The MEP DATA signal is defined to be a bidirectional
 * open-drain signal shared by host and module.  The module
 * provides a pullup resistor on this line, but for the purposes
 * of our test setup, we configure our own pullup resistor just
 * in case no module is present.
 *
 * To drive a '0' on DATA, the I/O pin is configured as an output
 * and then driven to '0'.  
 *
 * TO drive a '1' on DATA, the I/O pin is reconfigured as an
 * input pin.  This effectively puts it in a high-Z state, which
 * lets the pullup resistor on the input port do its job.
 */
void mep_pl_setDATA(unsigned char sigLevel)
{
    if (sigLevel == 0) {
        HW_PINCTRL_DOE0_SET(0x1 << 9);		/* Program as an OUTPUT pin */
		HW_PINCTRL_DOUT0_CLR(0x1 << 9);	/* Drive DATA to '0' */		        
    }
    else {
        HW_PINCTRL_DOE0_CLR(0x1 << 9);		/* Program as an OUTPUT pin */
    }
}


/*****************************************************************
 * The MEP ACK signal can be driven using a push-pull output or
 * an open-drain output, depending on the specific MEP module.
 * We will drive it as push-pull according to the input parameter
 * value of 0 (LOW) or non-zero (HIGH).
 */
void mep_pl_setACK(unsigned char sigLevel)
{
    if (sigLevel == 0) {
		HW_PINCTRL_DOUT0_CLR(0x1 << 8);		/* Drive ACK to '0' */
    }
    else {
		HW_PINCTRL_DOUT0_SET(0x1 << 8);		/* Drive ACK to '1' */
    }
}


/*****************************************************************
 * This next routine implements a system-specific Interrupt
 * Service Routine (ISR) which is invoked when the CLK line goes
 * low.  The test hardware for this port wires CLK to the INT1
 * interrupt input pin on the AVR processor.
 *
 * The main responsibility of the ISR is to call mep_machine, and
 * then adjust the MEP interrupts according to the return value.
 * 
 * If the symbol MEP_INTERRUPTABLE_ISR is defined in the AVR
 * mep_config.h file, this ISR will be interruptable to allow
 * service of non-MEP interrupts.  This is almost certainly
 * required if MEP_DEBUG is defined by the configuration file
 * <mep_config.h>.  With MEP_DEBUG defined, MepLib will produce
 * RS232 output via the MepLib API routine mep_putc().  If the
 * MEP interrupt were to encounter an RX232 TXQ-full situation,
 * it would wait forever since the RS232 ISR would not be able to
 * run to empty the TXQ.  Making the MEP ISR interruptable solves
 * this problem.
 *
 * Note: the avr-gcc SIGNAL macro declares an interrupt handler
 * function which is invoked with AVR global interrupts DISABLED.
 * Global interrupts are automatically reenabled when this
 * function exits.
 */
#if !defined MEP_CONFIG_POLLED_MODE
SIGNAL(SIG_INTERRUPT1)
{
//=================================================================================
//printk("SIGNAL(SIG_INTERRUPT1)\n");
//=================================================================================
	
    mep_machine_t result;

    /* Let the logic analyzer know we are in the MEP ISR */
    //setPC1(0);

#if defined MEP_INTERRUPTABLE_ISR
    /* Disable further MEP interrupts so that we may reenable the
     * global interrupts to service other, potentially more
     * important events */
    mep_pl_intDisable();                  /* Disable further MEP ints... */
    //sei();                                /* ...before we reenable global ints */
    sti();                                /* ...before we reenable global ints */
#endif

    result = mep_machine();               /* Invoke the MEP machine */

#if defined MEP_INTERRUPTABLE_ISR
    cli();                                /* Disable global interrupts */
#endif

    /* Set the MEP interrupts the way that the state machine
     * wants them. */
    if (result == MEP_DISABLE_INTS) {
        mep_pl_intDisable();
    }
    else {
        /*Must have returned MEP_ENABLE_INTS or MEP_CLK_TIMEOUT. */
        if (result == MEP_CLK_TIMEOUT) {
            /* We might want to reset the module, if possible. */
        }
        
        /* In either case, we want to reenable MEP interrupts */
        mep_pl_intEnable();
    }

    /* Let the logic analyzer know we are leaving the MEP ISR */
    //setPC1(1);
}
#endif /* !defined MEP_CONFIG_POLLED_MODE */


/*****************************************************************
 * Spinwait until the CLK signal is observed to be at the
 * desiredClkLevel.
 *
 * If the timeoutIterations parameter is passed in with the value
 * 0, then timeouts are disabled: the routine will wait as long
 * as required to observe the desired CLK level of [0,1].
 *
 * IFF the timeoutIterations parameter is > 0, we decrement the
 * iteration count each time we test the current CLK value.  If
 * the iteration count hits zero, we have "timed out" and we
 * return an error code.
 *
 * Obviously, the iteration count approach to timing is quite
 * hardware specific, but at least it allows a simple timeout
 * mechanism without having to complicate MEP with an abstract to
 * hardware-specific mapping of system timer resources.
 *
 * See the Porting Guide for more information.
 */
mep_err_t mep_waitForCLK(unsigned char desiredClkLevel, unsigned int timeoutIterations)
{
//printk("mep_waitForCLK(unsigned char desiredClkLevel, unsigned int timeoutIterations)\n");
    while ((mep_pl_getCLK()!=0) != desiredClkLevel) {
        if (timeoutIterations > 0) {
            if (--timeoutIterations == 0) {
                /* Trouble: we timed out waiting for CLK. */
                return(MEP_TIMEOUT);
            }
        }
    }
    return(MEP_NOERR);
}

