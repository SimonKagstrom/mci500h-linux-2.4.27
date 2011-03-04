/*
 * linux/arch/arm/mach-ssa/has7752_gpio.c
 *
 * Copyright (C) 2003-2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/gpio.h>

#define VA_GPIO_VALUE           (IO_ADDRESS_GPIO_BASE + 0x00)
#define VA_GPIO_OUTPUT          (IO_ADDRESS_GPIO_BASE + 0x04)
#define VA_GPIO_DIRECTION       (IO_ADDRESS_GPIO_BASE + 0x08)
#define VA_GPIO_INT_ENABLE      (IO_ADDRESS_GPIO_BASE + 0x20)
#define VA_GPIO_INT_RAWSTATUS   (IO_ADDRESS_GPIO_BASE + 0x24)
#define VA_GPIO_INT_STATUS      (IO_ADDRESS_GPIO_BASE + 0x28)
#define VA_GPIO_INT_POLARITY    (IO_ADDRESS_GPIO_BASE + 0x2C)
#define VA_GPIO_INT_TYPE        (IO_ADDRESS_GPIO_BASE + 0x30)

#if 0
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


static const unsigned int gpio_io_lut_SDB_RevA[NR_HAS7752_GPIOS] =
{
   /*  0 : GPIO_FLASH_BANK_SELECT : */  (1 <<  0),
   /*  1 : GPIO_LED_ACTIVITY      : */  (1 <<  2),
   /*  2 : GPIO_LED_HEARTBEAT     : */  (1 <<  3),
   /*  3 : GPIO_7750_RESET        : */  (1 <<  4),
   /*  4 : GPIO_CMD_TO_52         : */  (1 <<  5),
   /*  5 : GPIO_CMD_TO_50         : */  (1 <<  6),
   /*  6 : GPIO_SUSPEND_1581      : */  (1 <<  7),
   /*  7 : GPIO_WAKEUP_1581       : */  (1 <<  8),
   /*  8 : GPIO_INTRQ_FPM         : */  (1 <<  9),
   /*  9 : GPIO_USB_RESET         : */  (1 << 11),
   /* 10 : GPIO_PCMCIA_RESET      : */  (1 << 12),
   /* 11 : GPIO_PCMCIA_INTRQ      : */  (1 << 13),
   /* 12 : GPIO_ETHERNET_INTRQ    : */  (1 << 14),
   /* 13 : GPIO_ETHERNET_RESET    : */  (1 << 16),
   /* 14 : GPIO_ATA_RESET         : */  (1 << 17),
   /* 15 : GPIO_SWITCH_1          : */  (1 << 18),
   /* 16 : GPIO_SWITCH_2          : */  (1 << 19),
   /* 17 : GPIO_EPLD_INTRQ        : */  (1 << 20),
   /* 18 : GPIO_CPLD_TDI          : */  (1 << 21),
   /* 19 : GPIO_CPLD_TDO          : */  (1 << 22),
   /* 20 : GPIO_CPLD_TMS          : */  (1 << 23),
   /* 21 : GPIO_CPLD_TCK          : */  (1 << 24),
   /* 22 : GPIO_USB_SENSE         : */  (      0),
   /* 23 : GPIO_USBH_NSELECT      : */  (      0),
   /* 24 : GPIO_USBH_SUSPEND      : */  (      0),
   /* 25 : GPIO_FLASH_NSELECT     : */  (      0),
   /* 26 : GPIO_USBH_RESET        : */  (      0),
   /* 27 : GPIO_USBH_WAKE         : */  (      0),
   /* 28 : GPIO_USBH_INTRQ        : */  (      0),
};

static const unsigned int gpio_io_lut_SDB_RevC[NR_HAS7752_GPIOS] =
{
   /*  0 : GPIO_FLASH_BANK_SELECT : */  (1 <<  0),
   /*  1 : GPIO_LED_ACTIVITY      : */  (1 <<  2),
   /*  2 : GPIO_LED_HEARTBEAT     : */  (1 <<  3),
   /*  3 : GPIO_7750_RESET        : */  (1 <<  4),
   /*  4 : GPIO_CMD_TO_52         : */  (      0),
   /*  5 : GPIO_CMD_TO_50         : */  (      0),
   /*  6 : GPIO_SUSPEND_1581      : */  (1 <<  7),
   /*  7 : GPIO_WAKEUP_1581       : */  (1 <<  8),
   /*  8 : GPIO_INTRQ_FPM         : */  (1 <<  9),
   /*  9 : GPIO_USB_RESET         : */  (1 << 11),
   /* 10 : GPIO_PCMCIA_RESET      : */  (1 << 12),
   /* 11 : GPIO_PCMCIA_INTRQ      : */  (1 << 22),
   /* 12 : GPIO_ETHERNET_INTRQ    : */  (1 << 14),
   /* 13 : GPIO_ETHERNET_RESET    : */  (1 << 16),
   /* 14 : GPIO_ATA_RESET         : */  (1 << 17),
   /* 15 : GPIO_SWITCH_1          : */  (1 << 18),
   /* 16 : GPIO_SWITCH_2          : */  (1 << 19),
   /* 17 : GPIO_EPLD_INTRQ        : */  (1 << 20),
   /* 18 : GPIO_CPLD_TDI          : */  (1 <<  5),
   /* 19 : GPIO_CPLD_TDO          : */  (1 << 13),
   /* 20 : GPIO_CPLD_TMS          : */  (1 << 23),
   /* 21 : GPIO_CPLD_TCK          : */  (1 << 24),
   /* 22 : GPIO_USB_SENSE         : */  (1 << 21),
   /* 23 : GPIO_USBH_NSELECT      : */  (      0),
   /* 24 : GPIO_USBH_SUSPEND      : */  (      0),
   /* 25 : GPIO_FLASH_NSELECT     : */  (      0),
   /* 26 : GPIO_USBH_RESET        : */  (      0),
   /* 27 : GPIO_USBH_WAKE         : */  (      0),
   /* 28 : GPIO_USBH_INTRQ        : */  (      0),
};

static const unsigned int gpio_io_lut_HAS_Lab1[NR_HAS7752_GPIOS] =
{
   /*  0 : GPIO_FLASH_BANK_SELECT : */  (1 <<  0),
   /*  1 : GPIO_LED_ACTIVITY      : */  (1 <<  2),
   /*  2 : GPIO_LED_HEARTBEAT     : */  (1 <<  3),
   /*  3 : GPIO_7750_RESET        : */  (1 <<  4),
   /*  4 : GPIO_CMD_TO_52         : */  (      0),
   /*  5 : GPIO_CMD_TO_50         : */  (      0),
   /*  6 : GPIO_SUSPEND_1581      : */  (      0),
   /*  7 : GPIO_WAKEUP_1581       : */  (      0),
   /*  8 : GPIO_INTRQ_FPM         : */  (      0),      /* Lab1 has INTRQ_FPM is connected to CDB nCRST */
   /*  9 : GPIO_USB_RESET         : */  (      0),
   /* 10 : GPIO_PCMCIA_RESET      : */  (1 << 12),
   /* 11 : GPIO_PCMCIA_INTRQ      : */  (1 << 22),
   /* 12 : GPIO_ETHERNET_INTRQ    : */  (1 << 14),
   /* 13 : GPIO_ETHERNET_RESET    : */  (1 << 16),
   /* 14 : GPIO_ATA_RESET         : */  (1 << 17),
   /* 15 : GPIO_SWITCH_1          : */  (1 << 18),
   /* 16 : GPIO_SWITCH_2          : */  (      0),
   /* 17 : GPIO_EPLD_INTRQ        : */  (1 << 20),
   /* 18 : GPIO_CPLD_TDI          : */  (1 <<  5),
   /* 19 : GPIO_CPLD_TDO          : */  (1 << 13),
   /* 20 : GPIO_CPLD_TMS          : */  (1 << 23),
   /* 21 : GPIO_CPLD_TCK          : */  (1 << 24),
   /* 22 : GPIO_USB_SENSE         : */  (1 << 21),
   /* 23 : GPIO_USBH_NSELECT      : */  (1 <<  1),
   /* 24 : GPIO_USBH_SUSPEND      : */  (1 <<  6),
   /* 25 : GPIO_FLASH_NSELECT     : */  (1 <<  9),
   /* 26 : GPIO_USBH_RESET        : */  (1 << 10),
   /* 27 : GPIO_USBH_WAKE         : */  (1 << 15),
   /* 28 : GPIO_USBH_INTRQ        : */  (1 << 21),
};


/*****************************************************************************
*****************************************************************************/

static unsigned int gpio_pin_to_io_mask (unsigned int pin)
{
   unsigned int result;
   unsigned int platform_id;
   unsigned int *lutptr;

   if (pin >= NR_HAS7752_GPIOS) {
      printk ("%s: Requested GPIO pin %d is out of range\n", __FUNCTION__, pin);
      return 0;
   }

   platform_id = RELEASE_ID_PLATFORM(release_id);

   switch (platform_id)
   {
      case PLATFORM_SDB_REVA:
         lutptr = (unsigned int *) gpio_io_lut_SDB_RevA;
         break;

      case PLATFORM_SDB_REVC:
         lutptr = (unsigned int *) gpio_io_lut_SDB_RevC;
         break;

      case PLATFORM_HAS_LAB1:
      case PLATFORM_HAS_LAB2:
         lutptr = (unsigned int *) gpio_io_lut_HAS_Lab1;
         break;

      default:
         panic ("%s: Platform ID %d is unsupported or invalid\n", __FUNCTION__, platform_id);
         return 0;
   }

   if ((result = lutptr[pin]) == 0)
      printk ("%s: pin %d is not supported on platform ID %d\n", __FUNCTION__, pin, platform_id);

   return result;
}


/*****************************************************************************
*****************************************************************************/

void gpio_set_as_input (unsigned int pin)
{
   unsigned int direction;
   unsigned long flags;

   pin = gpio_pin_to_io_mask (pin);

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

   pin = gpio_pin_to_io_mask (pin);

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
   pin = gpio_pin_to_io_mask (pin);

   return (readl (VA_GPIO_VALUE) & pin) ? 1 : 0;
}

void gpio_set_high (unsigned int pin)
{
   unsigned int output;
   unsigned long flags;

   pin = gpio_pin_to_io_mask (pin);

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   writel ((output | pin), VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

void gpio_set_low (unsigned int pin)
{
   unsigned int output;
   unsigned long flags;

   pin = gpio_pin_to_io_mask (pin);

   local_irq_save (flags);
   output = readl (VA_GPIO_OUTPUT);
   writel ((output & ~pin), VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

void gpio_set_output (unsigned int pin, int value)
{
   unsigned int output;
   unsigned long flags;

   pin = gpio_pin_to_io_mask (pin);

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

   pin = gpio_pin_to_io_mask (pin);

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

// printk ("%s: raw pin 0x%08x\n", __FUNCTION__, pin);

   pin = gpio_pin_to_io_mask (pin);

// printk ("%s: cooked pin 0x%08x\n", __FUNCTION__, pin);

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

   pin = gpio_pin_to_io_mask (pin);

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

   pin = gpio_pin_to_io_mask (pin);

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

void gpio_dump_state (unsigned int pin)
{
   printk ("%s: raw pin %d\n", __FUNCTION__, pin);

   pin = gpio_pin_to_io_mask (pin);

   printk ("%s: cooked pin 0x%08x\n", __FUNCTION__, pin);

   printk ("%s: pin is being held %s\n", __FUNCTION__, (readl (VA_GPIO_VALUE) & pin) ? "high" : "low");
   printk ("%s: pin output is driving: %s\n", __FUNCTION__, (readl (VA_GPIO_OUTPUT) & pin) ? "high" : "low");
   printk ("%s: pin direction is: %s\n", __FUNCTION__, (readl (VA_GPIO_DIRECTION) & pin) ? "output" : "input");

   if (pin >= (1 << 16)) pin <<= 7;             /* Pins 16..24 map to 23..31 in GPIO int controller... */

   printk ("%s: pin int is: %s\n", __FUNCTION__, (readl (VA_GPIO_INT_ENABLE) & pin) ? "enabled" : "disabled");
   printk ("%s: pin int raw status is: %s\n", __FUNCTION__, (readl (VA_GPIO_INT_RAWSTATUS) & pin) ? "active" : "inactive");
   printk ("%s: pin int status is: %s\n", __FUNCTION__, (readl (VA_GPIO_INT_STATUS) & pin) ? "active" : "inactive");
   printk ("%s: pin int polarity is: %s\n", __FUNCTION__, (readl (VA_GPIO_INT_POLARITY) & pin) ? "active high / rising edge" : "active low / falling edge");
   printk ("%s: pin int type is: %s\n", __FUNCTION__, (readl (VA_GPIO_INT_TYPE) & pin) ? "edge triggered" : "level triggered");
}


#if 0
unsigned int gpio_int_get_status (void)
{
   unsigned int status;

   status = readl (VA_GPIO_INT_STATUS);
   return ((status & 0x0000FFFF) | ((status & 0xFF800000) >> 7));
}
#endif

#if 0
unsigned int gpio_int_get_raw_status (void)
{
   unsigned int rawstatus;

   rawstatus = readl (VA_GPIO_INT_RAWSTATUS);
   return ((rawstatus & 0x0000FFFF) | ((rawstatus & 0xFF800000) >> 7));
}
#endif

#if 0
void gpio_int_clear_latch (unsigned int pin)
{
   pin = gpio_pin_to_io_mask (pin);

   if (pin >= (1 << 16)) pin <<= 7;             /* Pins 16..24 map to 23..31 in GPIO int controller... */
   writel (pin, VA_GPIO_INT_RAWSTATUS);         /* no read-modify-write required... */
}
#endif


EXPORT_SYMBOL(gpio_set_as_input);
EXPORT_SYMBOL(gpio_get_value);
EXPORT_SYMBOL(gpio_set_as_output);
EXPORT_SYMBOL(gpio_set_high);
EXPORT_SYMBOL(gpio_set_low);
EXPORT_SYMBOL(gpio_int_set_type);


/*****************************************************************************
*****************************************************************************/

void __init gpio_int_init (void)        /* called by the irq init code */
{
   /*
      This should probably set all GPIOs to a known state
      But for now we rely on the bootloader to set everything up correctly...
   */

   printk ("GPIO init complete\n");
}

/*****************************************************************************
*****************************************************************************/
