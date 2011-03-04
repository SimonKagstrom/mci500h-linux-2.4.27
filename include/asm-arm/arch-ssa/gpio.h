/*
 *  linux/include/asm-arm/arch-ssa/gpio.h
 *
 *  Copyright (C) 2003 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <linux/config.h>

typedef enum
{
   GPIO_LEVEL_TIGGERED_ACTIVE_LOW  = 0,
   GPIO_LEVEL_TIGGERED_ACTIVE_HIGH = 1,
   GPIO_FALLING_EDGE_TRIGGERED     = 2,
   GPIO_RISING_EDGE_TRIGGERED      = 3
}
gpio_int_t;

extern void gpio_int_init (void);

extern void gpio_set_as_input (unsigned int pin);
extern void gpio_set_as_output (unsigned int pin, int initial_value);
extern int  gpio_get_value (unsigned int pin);
extern void gpio_set_high (unsigned int pin);
extern void gpio_set_low (unsigned int pin);
extern void gpio_set_output (unsigned int pin, int value);
extern int  gpio_toggle_output (unsigned int pin);

extern void gpio_int_enable (unsigned int pin);
extern void gpio_int_disable (unsigned int pin);
extern void gpio_int_set_type (unsigned int pin, gpio_int_t int_type);

extern void gpio_dump_state (unsigned int pin);

/*
   Direct access to GPIO interrupt status does _not_ work for
   the HAS7752 platform (and shouldn't be needed anyway...)
*/

#ifndef CONFIG_SSA_HAS7752
extern unsigned int gpio_int_get_status (void);
extern unsigned int gpio_int_get_raw_status (void);
extern void gpio_int_clear_latch (unsigned int pin);
#endif

#endif
