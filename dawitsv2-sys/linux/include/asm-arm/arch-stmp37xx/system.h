/*
 *  linux/include/asm-arm/arch-stmp36xx/system.h
 *
 *  Copyright (C) 2005 Sigmatel Inc
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
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/proc-fns.h>
#include <asm/arch/stmp37xx.h>

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	 
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	#if 1
	// Set BATTCHRG to default value
        HW_POWER_CHARGE_WR(0x00010000);
	
	// Set MINPWR to default value
        HW_POWER_MINPWR_WR(0);

	// Reset digital side of chip (but not power or RTC)
	//HW_CLKCTRL_RESET_WR(BM_CLKCTRL_RESET_DIG);
#if 1 // add dhsong
	HW_POWER_RESET_WR(0x3e77 << 16);
 
        //--------------------------------------------------------------------------
        // Set the PWD bit to shut off the power.
        //--------------------------------------------------------------------------
        HW_POWER_RESET_WR((0x3e77 << 16) | 0x00000001);
#endif
	// Should not return
	#else
	HW_POWER_RESET_WR(0x3e77 << 16);
	BF_SET(CLKCTRL_RESET,CHIP);
	#endif
}

#endif
