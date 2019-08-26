/*
 *  linux/arch/arm/mach-stmp37xx/generic.c
 *
 *  Copyright (C) 2005-2006 Sigmatel Inc
 *  Copyright (C) 2008 MIZI Research, Inc.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/pm.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/system.h>
#include <asm/pgtable.h>

#include <asm/mach/map.h>
/*
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/arch/ocram-malloc.h>
*/

#include <asm/arch/system.h>

#include "generic.h"


/*
 * ToDo:
 *   gpio handler
 */

/*
 * The registers are all very closely mapped, so we might as well map them all
 * with a single mapping
 * 
 * Logical      Physical
 * f0000000	80000000	On-chip registers
 * f1000000	00000000	256k on-chip SRAM
 */

static struct map_desc standard_io_desc[] __initdata = {
	/* registers */
  	{
		.virtual	= IO_ADDRESS(STMP37XX_REGS_BASE),
		.pfn		= __phys_to_pfn(STMP37XX_REGS_BASE),
		.length		= SZ_1M,
		.type		= MT_DEVICE
	},
	/* ocram */
	{
		.virtual	= STMP37XX_OCRAM_BASE_VIRT,
		.pfn		= __phys_to_pfn(STMP37XX_OCRAM_BASE),
		.length		= STMP37XX_OCRAM_SIZE,
		.type		= MT_DEVICE_CACHED
	}
};

void __init stmp37xx_map_io(void)
{
	iotable_init(standard_io_desc, ARRAY_SIZE(standard_io_desc));

	//stmp37xx_init_clock();
}
