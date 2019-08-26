/*
 * arch/arm/mach-stmp37xx/leds.c
 *
 */
#include <linux/compiler.h>
#include <linux/init.h>

#include <asm/leds.h>
#include <asm/mach-types.h>

#include "leds.h"

static int __init stmp37xx_leds_init(void)
{
	if (machine_is_arma37())
		leds_event = arma37_leds_event;

	leds_event(led_start);
	return 0;
}

core_initcall(stmp37xx_leds_init);
