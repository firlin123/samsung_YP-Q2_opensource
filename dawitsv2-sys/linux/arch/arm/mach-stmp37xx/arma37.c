/*
 *  linux/arch/arm/mach-stmp37xx/arma37.c
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
#include <asm/mach-types.h>

#include <asm/mach/arch.h>

#include <asm/arch/system.h>

#include "generic.h"

/* FIXME */
#ifdef CONFIG_CPU_STMP3700
#include "stmp3700.h"
#endif

static struct platform_device adc_button_device = {
	.name		= "A.Button",
	.id		= -1,
	.num_resources	= 0,
};

static struct platform_device touchpad_device = {
	.name		= "stmp37xx_touchpad",
	.id		= -1,
	.num_resources	= 0,
};

static struct platform_device touchpad_melfas_device = {
	.name		= "stmp37xx_touchpad_melfas",
	.id		= -1,
	.num_resources	= 0,
};

static struct platform_device touchpad_synaptics_OT_device = {
	.name		= "stmp37xx_touchpad_synaptics_OT",
	.id		= -1,
	.num_resources	= 0,
};

static struct platform_device stmp37xx_reset_device = {
	.name		= "stmp37xx_reset",
	.id		= -1,
	.num_resources	= 0,
};

static struct platform_device stmp37xx_commevt_device = {
	.name		= "stmp37xx_commevt",
	.id		= -1,
	.num_resources	= 0,
};

static struct resource udc_resources[] = {
	{              
		.start          = HW_USBCTRL_ID_ADDR,
		.end            = HW_USBCTRL_ID_ADDR + 4096,
		.flags          = IORESOURCE_MEM,
	}
};

static u64 udc_dmamask = (u32)0xffffffffU;

static struct platform_device udc_device = {
	.name           = "stmp37xx_udc",
	.id             = -1,
	.dev = {
		/*              .release                = usb_release,*/
		.dma_mask               = &udc_dmamask,
		.coherent_dma_mask      = 0xffffffffU,
	},
	.num_resources  = ARRAY_SIZE(udc_resources),
	.resource       = udc_resources,
};



static void __init arma37_board_init(void)
{
#ifdef CONFIG_CPU_STMP3700
	stmp3700_init();	
#else
# error "unknown cpu"
#endif

#ifdef CONFIG_LEDS
	stmp37xx_config_pin(ARMA_GPIO_LED0);
#endif

	/*
 	 * ToDo
 	 *   GPIO, iodesc, ...
 	 */
	platform_device_register(&adc_button_device);
	platform_device_register(&udc_device);
	platform_device_register(&touchpad_device);
	platform_device_register(&touchpad_melfas_device);
	platform_device_register(&touchpad_synaptics_OT_device);
	platform_device_register(&stmp37xx_reset_device);
	platform_device_register(&stmp37xx_commevt_device);
}

MACHINE_START(ARMA37, "Armadillo STMP3700")
	/* Maintainer: Sigmatel Inc */
	.phys_io	= 0x80000000,
	.io_pg_offst	= ((0xf0000000) >> 18) & 0xfffc,
	.boot_params	= 0x40000100,
	.map_io		= stmp37xx_map_io,
	.init_irq	= stmp37xx_init_irq,
	.timer		= &stmp37xx_timer,
	.init_machine	= arma37_board_init,
MACHINE_END
