/*
 * linux/arch/arm/mach-ssa/gpio.c
 *
 * Copyright (C) 2003-2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/gpio.h>


#ifdef CONFIG_SSA_HAS7750
#error "Current GPIO routines are correct for SAA7752 only"
#endif


#define VA_GPIO_VALUE           (IO_ADDRESS_GPIO_BASE + 0x00)
#define VA_GPIO_OUTPUT          (IO_ADDRESS_GPIO_BASE + 0x04)
#define VA_GPIO_DIRECTION       (IO_ADDRESS_GPIO_BASE + 0x08)
#define VA_GPIO_INT_ENABLE      (IO_ADDRESS_GPIO_BASE + 0x20)
#define VA_GPIO_INT_RAWSTATUS   (IO_ADDRESS_GPIO_BASE + 0x24)
#define VA_GPIO_INT_STATUS      (IO_ADDRESS_GPIO_BASE + 0x28)
#define VA_GPIO_INT_POLARITY    (IO_ADDRESS_GPIO_BASE + 0x2C)
#define VA_GPIO_INT_TYPE        (IO_ADDRESS_GPIO_BASE + 0x30)

#define DEBUG

#ifdef DEBUG
void ssa_dump_gpio_regs (void)
{
   printk ("VA_GPIO_VALUE        : 0x%08x %s\n", readl (VA_GPIO_VALUE        ), "(1 == pin is high)");
   printk ("VA_GPIO_OUTPUT       : 0x%08x %s\n", readl (VA_GPIO_OUTPUT       ), "(1 == pin is driving high (if an output))");
   printk ("VA_GPIO_DIRECTION    : 0x%08x %s\n", readl (VA_GPIO_DIRECTION    ), "(1 == pin is an output)");
   printk ("VA_GPIO_INT_ENABLE   : 0x%08x %s\n", readl (VA_GPIO_INT_ENABLE   ), "(1 == source will be unmasked)");
   printk ("VA_GPIO_INT_RAWSTATUS: 0x%08x %s\n", readl (VA_GPIO_INT_RAWSTATUS), "(1 == int request is active)");
   printk ("VA_GPIO_INT_STATUS   : 0x%08x %s\n", readl (VA_GPIO_INT_STATUS   ), "(1 == int request is active and enabled)");
   printk ("VA_GPIO_INT_POLAR    : 0x%08x %s\n", readl (VA_GPIO_INT_POLARITY ), "(1 == active high / rising edge)");
   printk ("VA_GPIO_INT_TYPE     : 0x%08x %s\n", readl (VA_GPIO_INT_TYPE     ), "(1 == edge triggered)");
}
#endif


void gpio_set_as_input (unsigned int pin)
{
   unsigned int direction;
   unsigned long flags;

   local_irq_save (flags);
   direction = readl (VA_GPIO_DIRECTION);
   writel ((direction & ~pin), VA_GPIO_DIRECTION);
   local_irq_restore (flags);
}

void gpio_set_as_output (unsigned int pin, int initial_value)
{
   unsigned int output;
   unsigned int direction;
   unsigned long flags;

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   if (initial_value != 0)
      output |= pin;
   else
      output &= ~pin;
   writel (output, VA_GPIO_OUTPUT);
   direction = readl (VA_GPIO_DIRECTION);
   writel ((direction | pin), VA_GPIO_DIRECTION);
   local_irq_restore (flags);
}

int gpio_get_value (unsigned int pin)
{
   return (readl (VA_GPIO_VALUE) & pin) ? 1 : 0;
}

void gpio_set_high (unsigned int pin)
{
   unsigned int output;
   unsigned long flags;

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   writel ((output | pin), VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

void gpio_set_low (unsigned int pin)
{
   unsigned int output;
   unsigned long flags;

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   writel ((output & ~pin), VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

void gpio_set_output (unsigned int pin, int value)
{
   unsigned int output;
   unsigned long flags;

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   if (value)
      output |= pin;
   else
      output &= ~pin;
   writel (output, VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

int gpio_toggle_output (unsigned int pin)
{
   unsigned int previous;
   unsigned long flags;

   local_irq_save (flags);
   previous = readl (VA_GPIO_OUTPUT);
   writel ((previous ^ pin), VA_GPIO_OUTPUT);
   local_irq_restore (flags);

   return ((previous & pin) ? 1 : 0);
}


/*****************************************************************************
*****************************************************************************/

void gpio_int_enable (unsigned int pin)
{
   unsigned int inten;
   unsigned long flags;

   if (pin >= (1 << 16)) pin <<= 7;             /* Pins 16..24 map to 23..31 in GPIO int controller... */
   local_irq_save (flags);
   inten = readl (VA_GPIO_INT_ENABLE);
   writel ((inten | pin), VA_GPIO_INT_ENABLE);
   local_irq_restore (flags);
}

void gpio_int_disable (unsigned int pin)
{
   unsigned int inten;
   unsigned long flags;

   if (pin >= (1 << 16)) pin <<= 7;             /* Pins 16..24 map to 23..31 in GPIO int controller... */
   local_irq_save (flags);
   inten = readl (VA_GPIO_INT_ENABLE);
   writel ((inten & ~pin), VA_GPIO_INT_ENABLE);
   local_irq_restore (flags);
}

/*
   Note: Changing the polarity of edge triggered interrupts may cause
   the HW to detect an 'edge' even if the value on the pin does not change.
   Therefore the recommended sequence for setting interrupt type is:

   disable -> set type -> clear latch -> enable.
*/
void gpio_int_set_type (unsigned int pin, gpio_int_t int_type)
{
   unsigned int polarity;
   unsigned int type;
   unsigned long flags;

   local_irq_save (flags);

   writel (readl (VA_GPIO_DIRECTION) & ~pin, VA_GPIO_DIRECTION );   /* ensure that this pin is an input */

   if (pin >= (1 << 16)) pin <<= 7;                                 /* Pins 16..24 map to 23..31 in GPIO int controller... */
   writel (readl (VA_GPIO_INT_ENABLE) & ~pin, VA_GPIO_INT_ENABLE);  /* disable ints from this pin */

   polarity = readl (VA_GPIO_INT_POLARITY);
   type = readl (VA_GPIO_INT_TYPE);

   if ((int_type == GPIO_LEVEL_TIGGERED_ACTIVE_LOW) ||
       (int_type == GPIO_LEVEL_TIGGERED_ACTIVE_HIGH))
   {
      type &= ~pin;                             /* level triggered */
   }
   else
   {
      type |= pin;                              /* edge triggered */
   }

   if ((int_type == GPIO_LEVEL_TIGGERED_ACTIVE_LOW) ||
       (int_type == GPIO_FALLING_EDGE_TRIGGERED))
   {
      polarity &= ~pin;                         /* falling edge / low level triggered */
   }
   else
   {
      polarity |= pin;                          /* rising edge / high level triggered */
   }

   writel (polarity, VA_GPIO_INT_POLARITY);
   writel (type, VA_GPIO_INT_TYPE);
   writel (pin, VA_GPIO_INT_RAWSTATUS);         /* clear any spurious latched int requests */

   local_irq_restore (flags);
}

unsigned int gpio_int_get_status (void)
{
   unsigned int status;

   status = readl (VA_GPIO_INT_STATUS);
   return ((status & 0x0000FFFF) | ((status & 0xFF800000) >> 7));
}

unsigned int gpio_int_get_raw_status (void)
{
   unsigned int rawstatus;

   rawstatus = readl (VA_GPIO_INT_RAWSTATUS);
   return ((rawstatus & 0x0000FFFF) | ((rawstatus & 0xFF800000) >> 7));
}

void gpio_int_clear_latch (unsigned int pin)
{
   if (pin >= (1 << 16)) pin <<= 7;             /* Pins 16..24 map to 23..31 in GPIO int controller... */
   writel (pin, VA_GPIO_INT_RAWSTATUS);         /* no read-modify-write required... */
}


EXPORT_SYMBOL(gpio_int_get_raw_status);
EXPORT_SYMBOL(gpio_int_clear_latch);
EXPORT_SYMBOL(gpio_set_high);
EXPORT_SYMBOL(gpio_set_as_output);
EXPORT_SYMBOL(gpio_set_low);
EXPORT_SYMBOL(gpio_int_set_type);
EXPORT_SYMBOL(gpio_get_value);


/*****************************************************************************
*****************************************************************************/

void __init gpio_int_init (void)        /* called by the irq init code */
{
   /*
      This should probably set all GPIOs to a known state
      But for now we rely on the bootloader to set everything up correctly...
   */

   printk ("GPIO init complete\n");
   return 0;
}

/*****************************************************************************
*****************************************************************************/

