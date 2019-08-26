/*
 *  linux/arch/arm/mach-stmp37xx/gpio.c
 *
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
 * Wed Feb 6, SeonKon Choi <bushi@mizi.com>
 *  - initial
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>

void stmp37xx_config_pin(unsigned int pin)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_config_pin(pin);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_config_pin);

void stmp37xx_config_pins(unsigned int *pin, int count)
{
	unsigned long flags;
	local_irq_save(flags);
	for (; count > 0; count--)
		__stmp37xx_config_pin(*pin);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_config_pins);

void stmp37xx_gpio_set_af(unsigned int pin, int function)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_af(pin, function);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_af);

int stmp37xx_gpio_get_af(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_af(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_af);

void stmp37xx_gpio_set_dir(unsigned int pin, int direction)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_dir(pin, direction);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_dir);

int stmp37xx_gpio_get_dir(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_dir(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_dir);

void stmp37xx_gpio_set_level(unsigned int pin, int level)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_level(pin, level);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_level);

int stmp37xx_gpio_get_level(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_level(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_level);

void stmp37xx_gpio_set_drv(unsigned int pin, int drive)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_drv(pin, drive);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_drv);

int stmp37xx_gpio_get_drv(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_drv(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_drv);

void stmp37xx_gpio_set_volt(unsigned int pin, int voltage)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_volt(pin, voltage);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_volt);

int stmp37xx_gpio_get_volt(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_volt(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_volt);

void stmp37xx_gpio_set_pullup(unsigned int pin, int pullup)
{
	unsigned long flags;
	local_irq_save(flags);
	__stmp37xx_gpio_set_pullup(pin, pullup);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(stmp37xx_gpio_set_pullup);

int stmp37xx_gpio_get_pullup(unsigned int pin)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = __stmp37xx_gpio_get_pullup(pin);
	local_irq_restore(flags);
	return ret;
}
EXPORT_SYMBOL(stmp37xx_gpio_get_pullup);
