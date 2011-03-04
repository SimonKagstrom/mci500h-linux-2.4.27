/*
 *  linux/arch/arm/mach-ssa/leds.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
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

static void ssa_leds_event (led_event_t ledevt)
{
   if ((ledevt == led_idle_end) || (ledevt == led_idle_start))
      gpio_toggle_output (GPIO_LED_ACTIVITY);
   else if (ledevt == led_timer)
      gpio_toggle_output (GPIO_LED_HEARTBEAT);
}

static int __init leds_init (void)
{
   gpio_set_as_output (GPIO_LED_ACTIVITY, LED_ON);      /* cpu is initially busy */
   gpio_set_as_output (GPIO_LED_HEARTBEAT, LED_ON);     /* timer tick LED startup state is fairly arbitrary */
   leds_event = ssa_leds_event;
   return 0;
}

__initcall(leds_init);

