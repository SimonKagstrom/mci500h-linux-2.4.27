/*
 * linux/arch/arm/mach-ssa/w3_gpi.c
 *
 * Copyright (C) 2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/config.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/errno.h>

#include <asm/arch/ssa_gpi.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tsc.h>

#ifdef CONFIG_SSA_W3_PCCARD
#error "CONFIG_SSA_W3_PCCARD and CONFIG_SSA_W3_GPI should not both be defined"
#endif

#ifdef CONFIG_SSA_W3_SPI
#error "CONFIG_SSA_W3_SPI and CONFIG_SSA_W3_GPI should not both be defined"
#endif

#define DEBUG_LEVEL             (1)

#define CPLD_DC                 (IO_ADDRESS_EPLD_BASE + 0x000)
#define CPLD_CC                 (IO_ADDRESS_EPLD_BASE + 0x200)
#define CPLD_BU                 (IO_ADDRESS_EPLD_BASE + 0x400)


#if 0
#define DISABLE_INTERRUPTS()            disable_irq (INT_GPI)
#define RESTORE_INTERRUPTS()            enable_irq (INT_GPI)
#else
#define DISABLE_INTERRUPTS()            { unsigned long flags; local_irq_save (flags)
#define RESTORE_INTERRUPTS()            local_irq_restore (flags); }
#endif

#define GUCIRDY_ASSERTED()              (gpio_get_value (GPIO_GPI_GUCIRDY) == 0)
#define GDFCIRDY_ASSERTED()             (gpio_get_value (GPIO_GPI_GDFCIRDY) == 0)

#define POLL_FOR_GUCIRDY_ASSERTION()    while (! GUCIRDY_ASSERTED())  { /* delay */ }
#define POLL_FOR_GDFCIRDY_ASSERTION()   while (! GDFCIRDY_ASSERTED()) { /* delay */ }


/*****************************************************************************
*****************************************************************************/

void ssa_gpi_reset_assert (void)
{
   gpio_set_as_output (GPIO_GPI_RESET, 0);	/* drive low to assert GPI reset */
}

void ssa_gpi_reset_deassert (void)
{
   gpio_set_as_output (GPIO_GPI_RESET, 1);	/* drive high to de-assert GPI reset */
}

/*
   debug function: return state of GUCIRDY input
*/
int ssa_gpi_read_gucirdy (void)
{
   return gpio_get_value (GPIO_GPI_GUCIRDY);
}

/*
   debug function: return state of GDFCIRDY input
*/
int ssa_gpi_read_gdfcirdy (void)
{
   return gpio_get_value (GPIO_GPI_GDFCIRDY);
}

/*
   return the interrupt number which the driver should use when
   registering an interrupt handler for the SA5252 device.
*/
int ssa_gpi_get_irq (void)
{
   return INT_GPI;
}

/*
   HW init etc. called during kernel startup.
   May also be called by the SA5252 device driver during its initialisation.
*/
int ssa_gpi_init (void)
{
   gpio_set_as_output (GPIO_CPLD_RESET, 0);     /* drive low to force CPLD reset */
   udelay (1);
   gpio_set_as_output (GPIO_CPLD_RESET, 1);

   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform a single low level 16bit read from the control address
   This function should not be called directly by the SA5252 device driver.
*/
static unsigned int ssa_gpi_control_read_single16 (void)
{
   unsigned int temp;
   
   DISABLE_INTERRUPTS();
   readb (CPLD_CC);
   temp = readw (CPLD_BU);
   RESTORE_INTERRUPTS();
   if (DEBUG_LEVEL > 3)
      printk ("%s : 0x%04x\n", __FUNCTION__, temp);
   return temp;
}

/*
   Perform a single low level 16bit write to the control address
   This function should not be called directly by the SA5252 device driver.
*/
static unsigned int ssa_gpi_control_write_single16 (unsigned short value)
{
   if (DEBUG_LEVEL > 3)
      printk ("%s: 0x%04x\n", __FUNCTION__, value);
   DISABLE_INTERRUPTS();
   writew (value, CPLD_CC);
   readb (CPLD_BU);             /* dummy read from buffer to ensure 4 ebi clocks between write and possible next read */
   RESTORE_INTERRUPTS();
   return 0;
}

/*
   Perform a single low level 16bit read from the data address
   This function should not be called directly by the SA5252 device driver.
*/
static unsigned int ssa_gpi_data_read_single16 (void)
{
   unsigned int temp;
   
   DISABLE_INTERRUPTS();
   readb (CPLD_DC);
   temp = readw (CPLD_BU);
   RESTORE_INTERRUPTS();
   if (DEBUG_LEVEL > 3)
      printk ("%s : 0x%04x\n", __FUNCTION__, temp);
   return temp;
}

/*
   Perform a single low level 16bit write to the data address
   This function should not be called directly by the SA5252 device driver.
*/
static unsigned int ssa_gpi_data_write_single16 (unsigned short value)
{
   if (DEBUG_LEVEL > 3)
      printk ("%s: 0x%04x\n", __FUNCTION__, value);
   DISABLE_INTERRUPTS();
   writew (value, CPLD_DC);
   readb (CPLD_BU);             /* dummy read from buffer to ensure 4 ebi clocks between write and possible next read */
   RESTORE_INTERRUPTS();
   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform a low level 32bit read from the control address

   This function should be called by the SA5252 device driver in order to
   fetch the value in the Frame register at the start of a device initiated
   read or write transfer.
*/
unsigned int ssa_gpi_control_read_single (void)
{
   unsigned int temp, ls, ms;
   
   DISABLE_INTERRUPTS();
   ls = ssa_gpi_control_read_single16();                        /* ls 16bits first */
   ms = ssa_gpi_control_read_single16();                        /* ms 16bits second */
   RESTORE_INTERRUPTS();
   temp = (ms << 16) | ls;
   if (DEBUG_LEVEL > 2)
      printk ("%s : 0x%08x\n", __FUNCTION__, temp);
   return temp;
}

/*
   Perform a low level 32bit write to the control address
   This function should not be called directly by the SA5252 device driver (?).
*/
unsigned int ssa_gpi_control_write_single (unsigned int value)
{
   if (DEBUG_LEVEL > 2)
      printk ("%s: 0x%08x\n", __FUNCTION__, value);
   DISABLE_INTERRUPTS();
   ssa_gpi_control_write_single16 ((value >>  0) & 0xFFFF);     /* ls 16bits first */
   ssa_gpi_control_write_single16 ((value >> 16) & 0xFFFF);     /* ms 16bits second */
   RESTORE_INTERRUPTS();
   return 0;
}

/*
   Perform a low level 32bit read from the data address
   This function should not be called directly by the SA5252 device driver (?).
*/
unsigned int ssa_gpi_data_read_single (void)
{
   unsigned int temp, ls, ms;
   
   DISABLE_INTERRUPTS();
   ls = ssa_gpi_data_read_single16();                           /* ls 16bits first */
   ms = ssa_gpi_data_read_single16();                           /* ms 16bits second */
   RESTORE_INTERRUPTS();
   temp = (ms << 16) | ls;
   if (DEBUG_LEVEL > 2)
      printk ("%s : 0x%08x\n", __FUNCTION__, temp);
   return temp;
}

/*
   Perform a low level 32bit write to the data address
   This function should not be called directly by the SA5252 device driver (?).
*/
unsigned int ssa_gpi_data_write_single (unsigned int value)
{
   if (DEBUG_LEVEL > 2)
      printk ("%s: 0x%08x\n", __FUNCTION__, value);
   DISABLE_INTERRUPTS();
   ssa_gpi_data_write_single16 ((value >>  0) & 0xFFFF);        /* ls 16bits first */
   ssa_gpi_data_write_single16 ((value >> 16) & 0xFFFF);        /* ms 16bits second */
   RESTORE_INTERRUPTS();
   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform host initiated read transfer from the device.
*/
unsigned int ssa_gpi_read_hi (unsigned int frame, unsigned char *buffer, unsigned int bytecount)
{
   unsigned int temp;

   if (DEBUG_LEVEL > 1)
      printk ("%s : frame = 0x%08x, bytecount = %4d\n", __FUNCTION__, frame, bytecount);

   if (unlikely (bytecount & 0x03)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : rounding up bytecount\n", __FUNCTION__);
      bytecount = (bytecount + 3) & ~0x03;
   }

   if (unlikely (GUCIRDY_ASSERTED())) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : GUCIRDY asserted on entry\n", __FUNCTION__);
   }

   DISABLE_INTERRUPTS();

   ssa_gpi_control_write_single (frame);

   if ((((unsigned long) buffer) & 0x03) == 0)
   {
      unsigned int *buf = (unsigned int *) buffer;

      while (bytecount) {
         bytecount -= 4;
         POLL_FOR_GUCIRDY_ASSERTION();
         temp = ssa_gpi_data_read_single();
         *buf++ = temp;
      }
   }
   else
   {
      /*
         Hopefully this doesn't happen too often...
      */
      
      if (DEBUG_LEVEL > 1)
         printk ("%s : dst buffer is not aligned !!\n", __FUNCTION__);
   
      while (bytecount) {
         bytecount -= 4;
         POLL_FOR_GUCIRDY_ASSERTION();
         temp = ssa_gpi_data_read_single();
         *buffer++ = temp;
         *buffer++ = temp >> 8;
         *buffer++ = temp >> 16;
         *buffer++ = temp >> 24;
      }
   }

   RESTORE_INTERRUPTS();
   
   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform host initiated write transfer to the device.
*/
unsigned int ssa_gpi_write_hi (unsigned int frame, unsigned char *buffer, unsigned int bytecount)
{
   unsigned int temp;

   if (DEBUG_LEVEL > 1)
      printk ("%s: frame = 0x%08x, bytecount = %4d\n", __FUNCTION__, frame, bytecount);

   if (unlikely (bytecount & 0x03)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: rounding up bytecount\n", __FUNCTION__);
      bytecount = (bytecount + 3) & ~0x03;
   }

   if (unlikely (GUCIRDY_ASSERTED())) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: GUCIRDY asserted on entry\n", __FUNCTION__);
   }

   DISABLE_INTERRUPTS();

   ssa_gpi_control_write_single (frame);

   if ((((unsigned long) buffer) & 0x03) == 0)
   {
      unsigned int *buf = (unsigned int *) buffer;

      while (bytecount) {
         bytecount -= 4;
         temp = *buf++;
         POLL_FOR_GUCIRDY_ASSERTION();
         ssa_gpi_data_write_single (temp);
      }
   }
   else
   {
      /*
         Hopefully this doesn't happen too often...
      */
      
      if (DEBUG_LEVEL > 1)
         printk ("%s: src buffer is not aligned !!\n", __FUNCTION__);
   
      while (bytecount) {
         bytecount -= 4;
         temp = *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         POLL_FOR_GUCIRDY_ASSERTION();
         ssa_gpi_data_write_single (temp);
      }
   }

   RESTORE_INTERRUPTS();
   
   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform the second phase of a device initiated read transfer from the device.
   This function will be called within an interrupt handler.
   
   The first phase of the transfer (ie reading the frame register to determine
   the type and length of the transfer) should have already been performed
   before this function is called !!
*/
unsigned int ssa_gpi_read_di (unsigned char *buffer, unsigned int bytecount)
{
   unsigned int temp;

   if (DEBUG_LEVEL > 1)
      printk ("%s : bytecount = %4d\n", __FUNCTION__, bytecount);

   if (unlikely (bytecount & 0x03)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : rounding up bytecount\n", __FUNCTION__);
      bytecount = (bytecount + 3) & ~0x03;
   }

   DISABLE_INTERRUPTS();        /* we are already in an interrupt handler, but this should do no harm... */

   if ((((unsigned long) buffer) & 0x03) == 0)
   {
      unsigned int *buf = (unsigned int *) buffer;

      while (bytecount) {
         bytecount -= 4;
         POLL_FOR_GDFCIRDY_ASSERTION();
         temp = ssa_gpi_data_read_single();
         *buf++ = temp;
      }
   }
   else
   {
      if (DEBUG_LEVEL > 1)
         printk ("%s : dst buffer is not aligned !!\n", __FUNCTION__);
   
      while (bytecount) {
         bytecount -= 4;
         POLL_FOR_GDFCIRDY_ASSERTION();
         temp = ssa_gpi_data_read_single();
         *buffer++ = temp;
         *buffer++ = temp >> 8;
         *buffer++ = temp >> 16;
         *buffer++ = temp >> 24;
      }
   }

   RESTORE_INTERRUPTS();
   
   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform the second phase of a device initiated write transfer to the device.
   This function will be called within an interrupt handler.
   
   The first phase of the transfer (ie reading the frame register to determine
   the type and length of the transfer) should have already been performed
   before this function is called !!
*/
unsigned int ssa_gpi_write_di (unsigned char *buffer, unsigned int bytecount)
{
   unsigned int temp;

   if (DEBUG_LEVEL > 1)
      printk ("%s: bytecount = %4d\n", __FUNCTION__, bytecount);

   if (unlikely (bytecount & 0x03)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: rounding up bytecount\n", __FUNCTION__);
      bytecount = (bytecount + 3) & ~0x03;
   }

   DISABLE_INTERRUPTS();        /* we are already in an interrupt handler, but this should do no harm... */

   if ((((unsigned long) buffer) & 0x03) == 0)
   {
      unsigned int *buf = (unsigned int *) buffer;

      while (bytecount) {
         bytecount -= 4;
         temp = *buf++;
         POLL_FOR_GDFCIRDY_ASSERTION();
         ssa_gpi_data_write_single (temp);
      }
   }
   else
   {
      if (DEBUG_LEVEL > 1)
         printk ("%s: src buffer is not aligned !!\n", __FUNCTION__);
   
      while (bytecount) {
         bytecount -= 4;
         temp = *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         temp |= *buffer++;
         temp = (temp << 24) | (temp >> 8);
         POLL_FOR_GDFCIRDY_ASSERTION();
         ssa_gpi_data_write_single (temp);
      }
   }

   RESTORE_INTERRUPTS();
   
   return 0;
}


/*******************************************************************************
*******************************************************************************/

static int __init w3_gpi_init (void)
{
   printk ("Initialising W3 GPI interface\n");
   return ssa_gpi_init();
}

__initcall (w3_gpi_init);

/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(ssa_gpi_reset_assert);
EXPORT_SYMBOL(ssa_gpi_reset_deassert);
EXPORT_SYMBOL(ssa_gpi_read_gucirdy);
EXPORT_SYMBOL(ssa_gpi_read_gdfcirdy);
EXPORT_SYMBOL(ssa_gpi_get_irq);
EXPORT_SYMBOL(ssa_gpi_init);
EXPORT_SYMBOL(ssa_gpi_control_read_single);
EXPORT_SYMBOL(ssa_gpi_control_write_single);
EXPORT_SYMBOL(ssa_gpi_data_read_single);
EXPORT_SYMBOL(ssa_gpi_data_write_single);
EXPORT_SYMBOL(ssa_gpi_read_hi);
EXPORT_SYMBOL(ssa_gpi_write_hi);
EXPORT_SYMBOL(ssa_gpi_read_di);
EXPORT_SYMBOL(ssa_gpi_write_di);

