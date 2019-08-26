/*
 * linux/arch/arm/mach-stmp37xx/leds-arma37.c
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

#include <asm/hardware.h>
#include <asm/leds.h>
#include <asm/system.h>

#include "leds.h"

#define LED_STATE_ENABLED	1
#define LED_STATE_CLAIMED	2
static unsigned int led_state;

#define _LED_TIMER (1<<0)
#define _LED_CPU (1<<1)
static unsigned int _hw_led = 0;

void arma37_leds_event(led_event_t evt)
{
	unsigned long flags;

	local_irq_save(flags);

	switch (evt) {
	case led_start:
		_hw_led = 0;
		led_state = LED_STATE_ENABLED;
		break;
	case led_stop:
		led_state &= ~LED_STATE_ENABLED;
		break;
	case led_claim:
		led_state |= LED_STATE_CLAIMED;
		break;
	case led_release:
		break;
#ifdef CONFIG_LEDS_TIMER
	case led_timer:
		led_state &= ~LED_STATE_CLAIMED;
		_hw_led ^= _LED_TIMER;
		break;
#endif
#ifdef CONFIG_LEDS_CPU
	case led_idle_start:
		_hw_led &= ~_LED_CPU;
		break;

	case led_idle_end:
		_hw_led |= _LED_CPU;
		break;
#endif
	case led_halted:
		break;
	default:
		break;
	}
	
	if (led_state & LED_STATE_ENABLED) {
	__stmp37xx_gpio_set_level(ARMA_GPIO_LED0, _hw_led & _LED_TIMER ? 1:0);
	}

	local_irq_restore(flags);
}
