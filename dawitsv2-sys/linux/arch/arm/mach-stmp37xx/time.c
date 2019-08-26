/*
 *  linux/arch/arm/mach-stmp37xx/time.c
 *
 *  Copyright (C) 2005 Sigmatel Inc
 *  Copyright (C) 2008 MIZI Research, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/sched.h>

#if 0
#include <asm/div64.h>
#include <asm/cnt32_to_63.h>
#endif
#include <asm/mach/irq.h>
#include <asm/mach/time.h>

#include <asm/arch/smp.h>

#include "generic.h"

/*
 * ToDo:
 * 	timer pm
 */

static unsigned long last_tick;

/*
 * Returns number of ms since last clock interrupt.  Note that interrupts
 * will have been disabled by do_gettimeoffset()
 */
unsigned long stmp37xx_gettimeoffset(void)
{
    	/* Return number of ms since last interrupt */
    	return HW_DIGCTL_MICROSECONDS_RD() - last_tick;
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t stmp37xx_timer_interrupt(int irq, void *dev_id)
{
	write_seqlock(&xtime_lock);

	/* Grab us value at this tick */
	last_tick = HW_DIGCTL_MICROSECONDS_RD();
	
	/*
	 * clear the interrupt
	 */
	HW_TIMROT_TIMCTRLn_CLR(0, BM_TIMROT_TIMCTRLn_IRQ); 
	
#if 0
	/*
	 * the clock tick routines are only processed on the
	 * primary CPU
	 */
	if (hard_smp_processor_id() == 0) {
		timer_tick();
#ifdef CONFIG_SMP
		smp_send_timer();
#endif
	}

#ifdef CONFIG_SMP
	/*
	 * this is the ARM equivalent of the APIC timer interrupt
	 */
	update_process_times(user_mode(regs));
#endif /* CONFIG_SMP */

#else
	timer_tick();
#endif

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

static struct irqaction stmp37xx_timer_irq = {
	.name		= "stmp37xx Tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= stmp37xx_timer_interrupt,
};

/*
 * Set up timer interrupt, and return the current time in seconds.
 */
void __init stmp37xx_time_init(unsigned long reload, unsigned int ctrl)
{
	HW_TIMROT_ROTCTRL_CLR(BM_TIMROT_ROTCTRL_SFTRST |
			      BM_TIMROT_ROTCTRL_CLKGATE);
	HW_TIMROT_TIMCOUNTn_WR(0, 0);

	HW_TIMROT_TIMCTRLn_WR(0, 
			      (BF_TIMROT_TIMCTRLn_SELECT(11) | // Xtal>>15 == 1kHz
			       BF_TIMROT_TIMCTRLn_PRESCALE(0) |
			       BF_TIMROT_TIMCTRLn_RELOAD(0x1) | 
			       BF_TIMROT_TIMCTRLn_UPDATE(0x1) |
			       BF_TIMROT_TIMCTRLn_POLARITY(0x0) | 
			       BF_TIMROT_TIMCTRLn_IRQ_EN(0x1)));
			       
	HW_TIMROT_TIMCOUNTn_WR(0, reload);			       

	// Reset the microsecond counter. Yes, this can be written!
	// If we don't do this, then time jumps by a huge amount at start up,
	// causing the first nanosleep() to return immediately.
	HW_DIGCTL_MICROSECONDS_WR(0);	
	
	/*
	 * Make irqs happen for the system timer
	 */
	setup_irq(IRQ_TIMER0, &stmp37xx_timer_irq);
}

static void __init stmp37xx_init_timer(void)
{
	stmp37xx_time_init(1000 / HZ - 1, 0);
}

struct sys_timer stmp37xx_timer = {
	.init		= stmp37xx_init_timer,
	.offset		= stmp37xx_gettimeoffset,
	.suspend	= NULL,
	.resume		= NULL,
};
