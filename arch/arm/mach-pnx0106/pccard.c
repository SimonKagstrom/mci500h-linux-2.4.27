/*
 * linux/arch/arm/mach-pnx0106/pccard.c
 *
 * Copyright (C) 2004-2005 Andre McCurdy, Philips Semiconductors.
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

#include <asm/arch/pccard.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tsc.h>

#define DEBUG_LEVEL             (1)

#ifdef CONFIG_PNX0106_PCCARD_CPLD
#error "CONFIG_PNX0106_PCCARD_CPLD and CONFIG_PNX0106_PCCARD should not be defined together"
#endif


/*******************************************************************************
*******************************************************************************/

void has7752_pccard_reset_assert (void)
{
   gpio_set_as_output (GPIO_PCMCIA_RESET, 1);	/* assert PCMCIA reset */
}

void has7752_pccard_reset_deassert (void)
{
   gpio_set_as_output (GPIO_PCMCIA_RESET, 0);	/* de-assert PCMCIA reset */
}

int has7752_pccard_read_ready (void)
{
   return gpio_get_value (GPIO_PCMCIA_INTRQ);
}

int has7752_pccard_get_irq (void)
{
   return IRQ_PCMCIA;
}

int has7752_pccard_init (void)
{
   gpio_set_as_input (GPIO_PCMCIA_INTRQ);

   return 0;
}


void has7752_pccard_set_mem_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws)
{
   if (DEBUG_LEVEL > 0)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);
}


void has7752_pccard_set_io_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws)
{
   if (DEBUG_LEVEL > 0)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);
}


/*******************************************************************************
*******************************************************************************/

/*
   Read the single 8bit value from PCMCIA attribute memory space at offset 'addr'.
*/
unsigned int has7752_pccard_attribute_read (unsigned int addr)
{
    unsigned int temp;

    temp = *((volatile unsigned char *) (IO_ADDRESS_PCCARD_ATTRIB_BASE + addr));

    if (DEBUG_LEVEL > 2)
        printk ("has7752_pccard_at_read:  addr = 0x%04x, value = 0x%04x\n", addr, temp);

    return temp;
}

/*
   Write the single 8bit value 'value' to PCMCIA attribute memory space at offset 'addr'.
*/
unsigned int has7752_pccard_attribute_write (unsigned int addr, unsigned char value)
{
    if (DEBUG_LEVEL > 2)
        printk ("has7752_pccard_at_write: addr = 0x%04x, value = 0x%04x\n", addr, value);

    *((volatile unsigned char *) (IO_ADDRESS_PCCARD_ATTRIB_BASE + addr)) = value;

    return 0;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read a single 16bit value from PCMCIA IO space at offset 'addr'.
*/
unsigned int has7752_pccard_io_read (unsigned int addr)
{
    unsigned int temp;

    temp = *((volatile unsigned short *) (IO_ADDRESS_PCCARD_IO_BASE + addr));

    if (DEBUG_LEVEL > 2)
        printk ("%s:  addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, temp);

    return temp;
}

/*
   Write the single 16bit value 'value' to PCMCIA IO space at offset 'addr'.
*/
unsigned int has7752_pccard_io_write (unsigned int addr, unsigned short value)
{
    if (DEBUG_LEVEL > 2)
        printk ("%s: addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, value);

    *((volatile unsigned short *) (IO_ADDRESS_PCCARD_IO_BASE + addr)) = value;

    return 0;
}

/*
   Reads 'count' bytes from PCMCIA IO space at offset 'addr' into 'buffer'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_io_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
    while (count >= 2) {
        *buffer++ = has7752_pccard_io_read (addr);
        count -= 2;
    }

    if (count != 0)
        *((unsigned char *) buffer) = has7752_pccard_io_read (addr);

   return 0;
}

/*
   Write 'count' bytes from 'buffer' into PCMCIA IO space at offset 'addr'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_io_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
   /* unaligned source buffer support is not required for ZyDAS driver, but may be for Agere ?? */

   if ((((unsigned long) buffer) & 0x01) == 0)
   {
      while (count > 0) {
         has7752_pccard_io_write (addr, *buffer++);
         count -= 2;
      }
   }
   else
   {
      unsigned char *byteptr = (unsigned char *) buffer;

      while (count > 0) {
         unsigned int lsb = *byteptr++;             /* fixme: this assumes a little endian system... */
         unsigned int msb = *byteptr++;
         unsigned int data = lsb | (msb << 8);
         has7752_pccard_io_write (addr, data);
         count -= 2;
      }
   }

   return 0;
}

/*
   Reads 'count' bytes from PCMCIA IO space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
   Data is written into 'buffer'.
*/
unsigned int has7752_pccard_io_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s  : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (((unsigned long) buffer) & 0x01)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : dst buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   while (count >= 2) {
      *buffer++ = has7752_pccard_io_read (addr);
      addr += 2;
      count -= 2;
   }

   if (count != 0)
      *((unsigned char *) buffer) = (has7752_pccard_io_read (addr) & 0xFF);

   return 0;
}

/*
   Writes 'count' bytes from buffer into PCMCIA IO space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
*/
unsigned int has7752_pccard_io_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   /* NDC driver requires misaligned src buffer support... */

   if ((((unsigned long) buffer) & 0x01) == 0)
   {
      while (count > 0) {
         has7752_pccard_io_write (addr, *buffer++);
         addr += 2;
         count -= 2;
      }
   }
   else
   {
      unsigned char *byteptr = (unsigned char *) buffer;

      while (count > 0) {
         unsigned int lsb = *byteptr++;             /* fixme: this assumes a little endian system... */
         unsigned int msb = *byteptr++;
         unsigned int data = lsb | (msb << 8);
         has7752_pccard_io_write (addr, data);
         addr += 2;
         count -= 2;
      }
   }

   return 0;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read a single 16bit value from PCMCIA memory space at offset 'addr'.
*/
unsigned int has7752_pccard_mem_read (unsigned int addr)
{
    unsigned int temp;

    temp = *((volatile unsigned short *) (IO_ADDRESS_PCCARD_COMMON_BASE + addr));

    if (DEBUG_LEVEL > 2)
        printk ("%s:  addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, temp);

    return temp;
}

/*
   Write the single 16bit value 'value' to PCMCIA memory space at offset 'addr'.
*/
unsigned int has7752_pccard_mem_write (unsigned int addr, unsigned short value)
{
    if (DEBUG_LEVEL > 2)
        printk ("%s: addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, value);

    *((volatile unsigned short *) (IO_ADDRESS_PCCARD_COMMON_BASE + addr)) = value;

    return 0;
}

/*
   Reads 'count' bytes from PCMCIA memory space at offset 'addr' into 'buffer'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
    while (count >= 2) {
        *buffer++ = has7752_pccard_mem_read (addr);
        count -= 2;
    }

    if (count != 0)
        *((unsigned char *) buffer) = has7752_pccard_mem_read (addr);

   return 0;
}

/*
   Write 'count' bytes from 'buffer' into PCMCIA memory space at offset 'addr'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
   if ((((unsigned long) buffer) & 0x01) == 0)
   {
      while (count > 0) {
         has7752_pccard_mem_write (addr, *buffer++);
         count -= 2;
      }
   }
   else
   {
      unsigned char *byteptr = (unsigned char *) buffer;

      while (count > 0) {
         unsigned int lsb = *byteptr++;             /* fixme: this assumes a little endian system... */
         unsigned int msb = *byteptr++;
         unsigned int data = lsb | (msb << 8);
         has7752_pccard_mem_write (addr, data);
         count -= 2;
      }
   }

   return 0;
}

/*
   Reads 'count' bytes from PCMCIA memory space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
   Data is written into 'buffer'.
*/
unsigned int has7752_pccard_mem_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s  : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (((unsigned long) buffer) & 0x01)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : dst buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   while (count >= 2) {
      *buffer++ = has7752_pccard_mem_read (addr);
      addr += 2;
      count -= 2;
   }

   if (count != 0)
      *((unsigned char *) buffer) = (has7752_pccard_mem_read (addr) & 0xFF);

   return 0;
}

/*
   Writes 'count' bytes from buffer into PCMCIA memory space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
*/
unsigned int has7752_pccard_mem_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if ((((unsigned long) buffer) & 0x01) == 0)
   {
      while (count > 0) {
         has7752_pccard_mem_write (addr, *buffer++);
         addr += 2;
         count -= 2;
      }
   }
   else
   {
      unsigned char *byteptr = (unsigned char *) buffer;

      while (count > 0) {
         unsigned int lsb = *byteptr++;             /* fixme: this assumes a little endian system... */
         unsigned int msb = *byteptr++;
         unsigned int data = lsb | (msb << 8);
         has7752_pccard_mem_write (addr, data);
         addr += 2;
         count -= 2;
      }
   }

   return 0;
}


/*******************************************************************************
*******************************************************************************/

static int __init pccard_init (void)
{
   printk ("Initialising pccard interface\n");
   return has7752_pccard_init();
}

__initcall (pccard_init);

/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(has7752_pccard_init);
EXPORT_SYMBOL(has7752_pccard_reset_assert);
EXPORT_SYMBOL(has7752_pccard_reset_deassert);
EXPORT_SYMBOL(has7752_pccard_read_ready);
EXPORT_SYMBOL(has7752_pccard_get_irq);
EXPORT_SYMBOL(has7752_pccard_set_mem_waitstates);
EXPORT_SYMBOL(has7752_pccard_set_io_waitstates);
//EXPORT_SYMBOL(has7752_pccard_word_swap_enable);
//EXPORT_SYMBOL(has7752_pccard_word_swap_disable);
EXPORT_SYMBOL(has7752_pccard_attribute_read);
EXPORT_SYMBOL(has7752_pccard_attribute_write);
EXPORT_SYMBOL(has7752_pccard_io_read);
EXPORT_SYMBOL(has7752_pccard_io_write);
EXPORT_SYMBOL(has7752_pccard_io_read_multi);
EXPORT_SYMBOL(has7752_pccard_io_write_multi);
EXPORT_SYMBOL(has7752_pccard_io_read_multi_direct);
EXPORT_SYMBOL(has7752_pccard_io_write_multi_direct);
EXPORT_SYMBOL(has7752_pccard_mem_read);
EXPORT_SYMBOL(has7752_pccard_mem_write);
EXPORT_SYMBOL(has7752_pccard_mem_read_multi);
EXPORT_SYMBOL(has7752_pccard_mem_write_multi);
EXPORT_SYMBOL(has7752_pccard_mem_read_multi_direct);
EXPORT_SYMBOL(has7752_pccard_mem_write_multi_direct);

