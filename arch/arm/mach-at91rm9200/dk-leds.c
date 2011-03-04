/*
 * LED driver for the Atmel AT91RM9200 Development Kit.
 *
 * (c) SAN People (Pty) Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
*/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/mach-types.h>
#include <asm/leds.h>
#include <asm/arch/hardware.h>
#include <asm/arch/pio.h>


static inline void at91_led_on(void)
{
	AT91_SYS->PIOB_CODR = AT91C_PIO_PB2;
}

static inline void at91_led_off(void)
{
	AT91_SYS->PIOB_SODR = AT91C_PIO_PB2;
}

static inline void at91_led_toggle(void)
{
	unsigned long curr = AT91_SYS->PIOB_ODSR;
	if (curr & AT91C_PIO_PB2)
		AT91_SYS->PIOB_CODR = AT91C_PIO_PB2;
	else
		AT91_SYS->PIOB_SODR = AT91C_PIO_PB2;
}


/*
 * Handle LED events.
 */
static void at91rm9200dk_leds_event(led_event_t evt)
{
	unsigned long flags;

	local_irq_save(flags);
	
	switch(evt) {
	case led_start:		/* System startup */
		at91_led_on();
		break;

	case led_stop:		/* System stop / suspend */
		at91_led_off();
		break;

#ifdef CONFIG_LEDS_TIMER
	case led_timer:		/* Every 50 timer ticks */
		at91_led_toggle();
		break;
#endif

#ifdef CONFIG_LEDS_CPU
	case led_idle_start:	/* Entering idle state */
		at91_led_off();
		break;

	case led_idle_end:	/* Exit idle state */
		at91_led_on();
		break;
#endif

	default:
		break;
	}
	
	local_irq_restore(flags);
}


static int __init leds_init(void)
{
	/* Enable PIO to access the LEDs */
	AT91_SYS->PIOB_PER = AT91C_PIO_PB2;
	AT91_SYS->PIOB_OER = AT91C_PIO_PB2;

	leds_event = at91rm9200dk_leds_event;

	leds_event(led_start);
	return 0;
}

__initcall(leds_init);
