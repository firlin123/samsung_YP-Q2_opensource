/*
 *  linux/include/asm-arm/arch-stmp36xx/hardware.h
 *
 *  This file contains the hardware definitions of the STMP36XX.
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
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>
#include <asm/arch/platform.h>
#include <asm/arch/stmp37xx.h>
#include <asm-arm/memory.h>

/*
 * Where in virtual memory the IO devices (timers, system controllers
 * and so on)
 */
#define IO_BASE			0xF0000000                 // VA of IO 
#define IO_SIZE			0x00100000                 // How much?
#define IO_START		0x80000000                 // PA of IO

/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x) (((x) & 0x000fffff) + IO_BASE) 

static inline unsigned long stmp_virt_to_phys (void *x)
{
        unsigned long vaddr = (unsigned long)x;
        if (vaddr >= STMP37XX_OCRAM_BASE_VIRT && vaddr < (STMP37XX_OCRAM_BASE_VIRT+STMP37XX_OCRAM_SIZE)) {
                return (vaddr - STMP37XX_OCRAM_BASE_VIRT);
        }
        return virt_to_phys(x);
}
 
static inline void *stmp_phys_to_virt (unsigned long x)
{
        if (x >= STMP37XX_OCRAM_BASE && x < (STMP37XX_OCRAM_BASE+STMP37XX_OCRAM_SIZE)) {
                return (void*)(x + STMP37XX_OCRAM_BASE_VIRT);
        }
        return phys_to_virt(x);
}
 
#ifdef CONFIG_MACH_ARMA37
#include "arma37.h"
#endif

#endif
