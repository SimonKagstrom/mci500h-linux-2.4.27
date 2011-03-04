/*
 *  linux/arch/arm/mach-pnx0106/leds.c
 *
 *  Copyright (C) 2002-2005 Andre McCurdy, Philips Semiconductors.
 *  Copyright (C) 2006-2007 Andre McCurdy, NXP Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/leds.h>
#include <asm/system.h>
#include <asm/arch/gpio.h>

#if (LED_ON == 0)
#define LED_TURN_ON(pin)	gpio_set_low (pin)
#define LED_TURN_OFF(pin)	gpio_set_high (pin)
#else
#define LED_TURN_ON(pin)	gpio_set_high (pin)
#define LED_TURN_OFF(pin)	gpio_set_low (pin)
#endif

static void pnx0106_leds_event (led_event_t ledevt)
{
	if (ledevt == led_idle_end)
		LED_TURN_ON(GPIO_LED_ACTIVITY);
	else if (ledevt == led_idle_start)
		LED_TURN_OFF(GPIO_LED_ACTIVITY);
	else if (ledevt == led_timer)
		gpio_toggle_output (GPIO_LED_HEARTBEAT);
}

static int __init leds_init (void)
{
	gpio_set_as_output (GPIO_LED_ACTIVITY, LED_ON); 	/* cpu is initially busy */
	gpio_set_as_output (GPIO_LED_HEARTBEAT, LED_ON);	/* timer tick LED startup state is arbitrary */

	leds_event = pnx0106_leds_event;
	return 0;
}

__initcall(leds_init);

