/*
 *  linux/arch/arm/mach-stmp37xx/irq.c
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

/*
 * Sat Feb 10, SeonKon Choi <bushi@mizi.com>
 *  - initial
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sysdev.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>

#include <asm/arch/stmp37xx.h>

#include "generic.h"

#define VU32P(x) *(volatile unsigned int*)(x)

static void stmp37xx_ack_irq(unsigned int irq)
{
	/* do tnothing */

	/* Barrier */
	(void) HW_ICOLL_STAT_RD();                
}

static void stmp37xx_mask_irq(unsigned int irq)
{
	/* IRQ disable */
	HW_ICOLL_PRIORITYn_CLR(irq / 4, 0x04 << ((irq % 4) * 8));
}

static void stmp37xx_unmask_irq(unsigned int irq)
{
	/* IRQ enable */
	HW_ICOLL_PRIORITYn_SET(irq / 4, 0x04 << ((irq % 4) * 8));
}

static struct irq_chip stmp37xx_chip = {
	.name	= "core",
	.ack	= stmp37xx_ack_irq,
	.mask	= stmp37xx_mask_irq,
	.unmask = stmp37xx_unmask_irq,
};

static void stmp37xx_ack_gpio_irq(unsigned int irq)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);

	VU32P(HW_PINCTRL_IRQSTAT0_CLR_ADDR + bank * 0x10) = (1 << pin);
}

static void stmp37xx_mask_gpio_irq(unsigned int irq)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);

	VU32P(HW_PINCTRL_PIN2IRQ0_CLR_ADDR + bank * 0x10) = (1 << pin);
}

static void stmp37xx_unmask_gpio_irq(unsigned int irq)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);

	VU32P(HW_PINCTRL_PIN2IRQ0_SET_ADDR + bank * 0x10) = (1 << pin);
}

void stmp37xx_enable_gpio_irq(unsigned int irq)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);

	stmp37xx_ack_gpio_irq(irq);
	stmp37xx_unmask_gpio_irq(irq);

	VU32P(HW_PINCTRL_IRQEN0_SET_ADDR + bank * 0x10) = (1 << pin);

#if 0
	// needless.
	stmp37xx_ack_irq(IRQ_GPIO_BANK0 + bank);
	stmp37xx_unmask_irq(IRQ_GPIO_BANK0 + bank);
#endif
}
EXPORT_SYMBOL(stmp37xx_enable_gpio_irq);

static void stmp37xx_disable_gpio_irq(unsigned int irq)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);

	/* FIXME */
	stmp37xx_mask_gpio_irq(irq);

	VU32P(HW_PINCTRL_IRQEN0_CLR_ADDR + bank * 0x10) = (1 << pin);
}

static int stmp37xx_gpio_irq_type(unsigned int irq, unsigned int type)
{
	unsigned int bank = __IRQ2BANK(irq);
	unsigned int pin = __IRQ2PINO(irq);
	unsigned int offset = bank * 0x10;
	unsigned int mask = (1 << pin);

	/* both edges not supported */

	switch (type) {
	case IRQ_TYPE_EDGE_FALLING:
		VU32P(HW_PINCTRL_IRQLEVEL0_CLR_ADDR + offset) = mask;
		VU32P(HW_PINCTRL_IRQPOL0_CLR_ADDR + offset) = mask;
		break;
	case IRQ_TYPE_EDGE_RISING:
		VU32P(HW_PINCTRL_IRQLEVEL0_CLR_ADDR + offset) = mask;
		VU32P(HW_PINCTRL_IRQPOL0_SET_ADDR + offset) = mask;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		VU32P(HW_PINCTRL_IRQLEVEL0_SET_ADDR + offset) = mask;
		VU32P(HW_PINCTRL_IRQPOL0_CLR_ADDR + offset) = mask;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		VU32P(HW_PINCTRL_IRQLEVEL0_SET_ADDR + offset) = mask;
		VU32P(HW_PINCTRL_IRQPOL0_SET_ADDR + offset) = mask;
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

static struct irq_chip stmp37xx_ext_gpio = {
	.name	= "gpio",
	.ack	= stmp37xx_ack_gpio_irq,
	.mask	= stmp37xx_mask_gpio_irq,
	.unmask = stmp37xx_unmask_gpio_irq,
	.enable = stmp37xx_enable_gpio_irq,
	.disable = stmp37xx_disable_gpio_irq,
	.set_type = stmp37xx_gpio_irq_type,
};

static inline void __handle_ext_gpio_irq(unsigned int irq,
		struct irq_desc *desc, unsigned int pending, unsigned int bank)
{
	unsigned int idx;

	while ((idx = ffs(pending))) {
		idx--;
		pending &= ~(1<<idx);
		irq = IRQ_START_OF_EXT_GPIO + (bank * 32 + idx);
		desc = irq_desc + irq;
		desc_handle_irq(irq, desc);
	}
}

static void handle_ext_gpio_b0_irq(unsigned int irq, struct irq_desc *desc)
{
	unsigned int val;
	val = VU32P(HW_PINCTRL_IRQSTAT0_ADDR);
	__handle_ext_gpio_irq(irq, desc, val, 0);
}

static void handle_ext_gpio_b1_irq(unsigned int irq, struct irq_desc *desc)
{
	unsigned int val;
	val = VU32P(HW_PINCTRL_IRQSTAT1_ADDR);
	__handle_ext_gpio_irq(irq, desc, val, 1);
}

static void handle_ext_gpio_b2_irq(unsigned int irq, struct irq_desc *desc)
{
	unsigned int val;
	val = VU32P(HW_PINCTRL_IRQSTAT2_ADDR);
	__handle_ext_gpio_irq(irq, desc, val, 2);
}

void __init stmp37xx_init_irq(void)
{
	unsigned int i;
	unsigned int irq;

	HW_PINCTRL_IRQEN0_WR(0);
	HW_PINCTRL_IRQEN1_WR(0);
	HW_PINCTRL_IRQEN2_WR(0);
	HW_PINCTRL_PIN2IRQ0_WR(0);
	HW_PINCTRL_PIN2IRQ1_WR(0);
	HW_PINCTRL_PIN2IRQ2_WR(0);
	HW_PINCTRL_IRQLEVEL0_WR(0);
	HW_PINCTRL_IRQLEVEL1_WR(0);
	HW_PINCTRL_IRQLEVEL2_WR(0);
	HW_PINCTRL_IRQPOL0_WR(0);
	HW_PINCTRL_IRQPOL1_WR(0);
	HW_PINCTRL_IRQPOL2_WR(0);

	/* Reset the interrupt controller */
	HW_ICOLL_CTRL_CLR(BM_ICOLL_CTRL_CLKGATE);
	udelay(10);
	HW_ICOLL_CTRL_CLR(BM_ICOLL_CTRL_SFTRST);
	udelay(10);
	HW_ICOLL_CTRL_SET(BM_ICOLL_CTRL_SFTRST);
	while(!(HW_ICOLL_CTRL_RD() & BM_ICOLL_CTRL_CLKGATE))
		;
	HW_ICOLL_CTRL_CLR(BM_ICOLL_CTRL_SFTRST | BM_ICOLL_CTRL_CLKGATE);
	
	/* Disable all interrupts initially */
	for (i=0; i < NR_CORE_IRQS;i++) {
		irq = i;
		stmp37xx_mask_irq(irq);
		set_irq_chip(irq, &stmp37xx_chip);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}
	
	for (i = 0; i < NR_EXT_GPIO_IRQS; i++) {
		irq = i + IRQ_START_OF_EXT_GPIO;
		stmp37xx_mask_gpio_irq(irq);
		set_irq_chip(irq, &stmp37xx_ext_gpio);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID | IRQF_NOAUTOEN);
	}

	set_irq_chained_handler(IRQ_GPIO_BANK0, handle_ext_gpio_b0_irq);	
	set_irq_chained_handler(IRQ_GPIO_BANK1, handle_ext_gpio_b1_irq);	
	set_irq_chained_handler(IRQ_GPIO_BANK2, handle_ext_gpio_b2_irq);	
	stmp37xx_unmask_irq(IRQ_GPIO_BANK0);
	stmp37xx_unmask_irq(IRQ_GPIO_BANK1);
	stmp37xx_unmask_irq(IRQ_GPIO_BANK2);
	set_irq_flags(IRQ_GPIO_BANK0, 0);
	set_irq_flags(IRQ_GPIO_BANK1, 0);
	set_irq_flags(IRQ_GPIO_BANK2, 0);

	/* Ensure vector is cleared */
	HW_ICOLL_LEVELACK_WR(1);	
	HW_ICOLL_LEVELACK_WR(2);	
	HW_ICOLL_LEVELACK_WR(4);	
	HW_ICOLL_LEVELACK_WR(8);
	
	HW_ICOLL_VECTOR_WR(0);
	/* Barrier */
	(void) HW_ICOLL_STAT_RD();	
}

#ifdef CONFIG_PM
#error Not implemented
#else
#define irq_suspend NULL
#define irq_resume NULL
#endif

static struct sysdev_class irq_class = {
	set_kset_name("irq"),
	.suspend	= irq_suspend,
	.resume		= irq_resume,
};

static struct sys_device irq_device = {
	.id	= 0,
	.cls	= &irq_class,
};

static int __init irq_init_sysfs(void)
{
	int ret = sysdev_class_register(&irq_class);
	if (ret == 0)
		ret = sysdev_register(&irq_device);
	return ret;
}

device_initcall(irq_init_sysfs);

