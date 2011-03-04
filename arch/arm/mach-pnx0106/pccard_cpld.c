/*
 * linux/arch/arm/mach-pnx0106/pccard_cpld.c
 *
 * Copyright (C) 2004-2006 Andre McCurdy, Philips Semiconductors.
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
#include <linux/interrupt.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/errno.h>

#include <asm/arch/gpio.h>
#include <asm/arch/tsc.h>
#include <asm/arch/irq.h>

#include <asm/arch/pccard.h>


/*******************************************************************************
*******************************************************************************/

#ifdef CONFIG_PNX0106_PCCARD
#error "CONFIG_PNX0106_PCCARD and CONFIG_PNX0106_PCCARD_CPLD should not be defined together"
#endif

#define DEBUG_LEVEL             (1)

#define IO_SPACE                (0 << 7)    /* This may in fact be common memory space (depending on CPLD...) */
#define ATTR_SPACE              (1 << 7)

#if 1
#if 0
#define DISABLE_INTERRUPTS()    disable_irq (IRQ_PCMCIA)
#define RESTORE_INTERRUPTS()    enable_irq (IRQ_PCMCIA)
#else
#define DISABLE_INTERRUPTS()    do { if (! in_irq()) disable_irq (IRQ_PCMCIA); } while (0)
#define RESTORE_INTERRUPTS()    do { if (! in_irq()) enable_irq (IRQ_PCMCIA); } while (0)
#endif
#else
#define DISABLE_INTERRUPTS()    { unsigned long flags; local_irq_save (flags)
#define RESTORE_INTERRUPTS()    local_irq_restore (flags); }
#endif

static unsigned int dbg_count = 100;


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
   /*
      Waitstates are hardcoded in CPLD logic and so can't be changed at runtime...
   */
   if (DEBUG_LEVEL > 1)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);
}

void has7752_pccard_set_io_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws)
{
   /*
      Waitstates are hardcoded in CPLD logic and so can't be changed at runtime...
   */
   if (DEBUG_LEVEL > 1)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);
}

int has7752_pccard_word_swap_enable (void)
{
   if (DEBUG_LEVEL > 0)
      printk ("%s: word swapping not supported within this pccard driver\n", __FUNCTION__);

    return 0;
}

int has7752_pccard_word_swap_disable (void)
{
   if (DEBUG_LEVEL > 0)
      printk ("%s: word swapping not supported within this pccard driver\n", __FUNCTION__);

    return 0;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read the single 8bit value from PCMCIA attribute memory space at offset 'addr'.
   This is only valid for pccard versions of the glue logic CPLD.
*/
unsigned int has7752_pccard_attribute_read (unsigned int addr)
{
   unsigned int temp;

   if (unlikely (addr & ~0xFFFE)) {     /* pccard CPLD supports address lines [15..1] */
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writeb ((ATTR_SPACE | (addr >> 10)), IO_ADDRESS_CPLD_BASE + 0x400);      /* setup MEM space and bank address */
   temp = readb (IO_ADDRESS_CPLD_BASE + (addr & 0x3FF));                    /* dummy read to begin sequence */

   while ((readb (IO_ADDRESS_CPLD_BASE + 0x401) /* & 0x01 */) != 0)
   {
      /* poll for completion */
   }

   temp = readw (IO_ADDRESS_CPLD_BASE + 0x402);                             /* read 16bit real data */

   RESTORE_INTERRUPTS();

   if (DEBUG_LEVEL > 2)
      printk ("has7752_pccard_at_read:  addr = 0x%04x, value = 0x%04x\n", addr, temp);

   return temp;
}


/*
   Write the single 8bit value 'value' to PCMCIA attribute memory space at offset 'addr'.
   This is only valid for pccard versions of the glue logic CPLD.
*/
unsigned int has7752_pccard_attribute_write (unsigned int addr, unsigned char value)
{
   if (DEBUG_LEVEL > 2)
      printk ("has7752_pccard_at_write: addr = 0x%04x, value = 0x%04x\n", addr, value);

   if (unlikely (addr & ~0xFFFE)) {     /* pccard CPLD supports address lines [15..1] */
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writeb ((ATTR_SPACE | (addr >> 10)), IO_ADDRESS_CPLD_BASE + 0x400);      /* setup Memory space and bank address */
   writew (value, IO_ADDRESS_CPLD_BASE + (addr & 0x3FF));                   /* write 16bit value to pageframe */

   while ((readb (IO_ADDRESS_CPLD_BASE + 0x401) /* & 0x01 */) != 0)
   {
      /* poll for completion */
   }

   RESTORE_INTERRUPTS();

   return 0;
}


/*
   Read a single 16bit value from PCMCIA IO space at offset 'addr'.
   This is valid for both pccard and NDC versions of the glue CPLD, however
   the NDC version supports a greater address range.
*/
unsigned int has7752_pccard_mem_read (unsigned int addr)
{
   unsigned int temp;

   if (unlikely (addr & ~0x3FFFE)) {    /* check for limit based on NDC driver. ie address lines [17..1] */
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writeb ((IO_SPACE | (addr >> 10)), IO_ADDRESS_CPLD_BASE + 0x400);        /* setup IO space and bank address */
   (void) readb (IO_ADDRESS_CPLD_BASE + (addr & 0x3FF));                    /* dummy read to begin sequence */

   while ((readb (IO_ADDRESS_CPLD_BASE + 0x401) /* & 0x01 */) != 0)
   {
      /* poll for completion */
   }

   temp = readw (IO_ADDRESS_CPLD_BASE + 0x402);                             /* read 16bit real data */

   RESTORE_INTERRUPTS();

   if ((DEBUG_LEVEL > 2) && dbg_count) {
      dbg_count--;
      printk ("%s:  addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, temp);
   }

   return temp;
}


/*
   Write the single 16bit value 'value' to PCMCIA IO space at offset 'addr'.
   This is valid for both pccard and NDC versions of the glue CPLD, however
   the NDC version supports a greater address range.
*/
unsigned int has7752_pccard_mem_write (unsigned int addr, unsigned short value)
{
   if ((DEBUG_LEVEL > 2) && dbg_count) {
      dbg_count--;
      printk ("%s: addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, value);
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writeb ((IO_SPACE | (addr >> 10)), IO_ADDRESS_CPLD_BASE + 0x400);        /* setup IO space and bank address */
   writew (value, IO_ADDRESS_CPLD_BASE + (addr & 0x3FF));                   /* write 16bit value to pageframe */

   while ((readb (IO_ADDRESS_CPLD_BASE + 0x401) /* & 0x01 */) != 0)
   {
      /* poll for completion */
   }

   RESTORE_INTERRUPTS();

   return 0;
}


/*
   Reads 'count' bytes from PCMCIA IO space at offset 'addr' into 'buffer'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned long ioaddr_ctrl;
   unsigned long ioaddr_data;

   if (DEBUG_LEVEL > 1)
      printk ("%s  : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (((unsigned long) buffer) & 0x01)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : dst buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   ioaddr_ctrl = IO_ADDRESS_CPLD_BASE + 0x400;
   ioaddr_data = IO_ADDRESS_CPLD_BASE + (addr & 0x3FF);

   DISABLE_INTERRUPTS();

   writeb ((IO_SPACE | (addr >> 10)), ioaddr_ctrl);                 /* setup IO space and bank address */

   while (count >= 2) {
      (void) readb (ioaddr_data);                                   /* dummy read to begin sequence */
      count -= 2;
      while ((readb (ioaddr_ctrl + 1) /* & 0x01 */) != 0) {
   	 /* poll for completion */
      }
      *buffer++ = readw (ioaddr_ctrl + 2);                          /* read 16bit real data */
   }

   if (count != 0)
      *((unsigned char *) buffer) = has7752_pccard_mem_read (addr);

   RESTORE_INTERRUPTS();

   return 0;
}


/*
   Write 'count' bytes from 'buffer' into PCMCIA IO space at offset 'addr'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   /*
      Unaligned source buffer support is not required for ZyDAS driver.
      Fixme... what about Agere ?? or Broadcom ??
   */

   if ((((unsigned long) buffer) & 0x01) == 0)
   {
      unsigned long ioaddr_ctrl = IO_ADDRESS_CPLD_BASE + 0x400;
      unsigned long ioaddr_data = IO_ADDRESS_CPLD_BASE + (addr & 0x3FF);

      DISABLE_INTERRUPTS();

      writeb ((IO_SPACE | (addr >> 10)), ioaddr_ctrl);              /* setup IO space and bank address */
      while (count > 0) {
   	 writew (*buffer++, ioaddr_data);                           /* write 16bit value to pageframe */
   	 count -= 2;
   	 while ((readb (ioaddr_ctrl + 1) /* & 0x01 */) != 0) {
   	    /* poll for completion */
   	 }
      }

      RESTORE_INTERRUPTS();
   }
   else
   {
      unsigned char *byteptr = (unsigned char *) buffer;

      static int been_here;

      if (!been_here) {
         been_here = 1;
         printk ("%s: unaligned source buffer\n", __FUNCTION__);
      }

      while (count > 0) {
   	 unsigned int lsb = *byteptr++; 	    /* fixme: this assumes a little endian system... */
   	 unsigned int msb = *byteptr++;
   	 unsigned int data = lsb | (msb << 8);
   	 has7752_pccard_mem_write (addr, data);
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
unsigned int has7752_pccard_mem_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s  : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

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
   Writes 'count' bytes from buffer into PCMCIA IO space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
*/
unsigned int has7752_pccard_mem_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   /* NDC driver requires misaligned src buffer support... */

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

      static int been_here;

      if (!been_here) {
         been_here = 1;
         printk ("%s: unaligned source buffer\n", __FUNCTION__);
      }

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

/*
    Note: selection between common memory and IO modes is hardcoded in CPLD design... !!
    Note: selection between common memory and IO modes is hardcoded in CPLD design... !!
    Note: selection between common memory and IO modes is hardcoded in CPLD design... !!
*/

unsigned int has7752_pccard_io_read (unsigned int addr)
{
    return has7752_pccard_mem_read (addr);
}

unsigned int has7752_pccard_io_write (unsigned int addr, unsigned short value)
{
    return has7752_pccard_mem_write (addr, value);
}

unsigned int has7752_pccard_io_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
    return has7752_pccard_mem_read_multi (addr, count, buffer);
}

unsigned int has7752_pccard_io_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
    return has7752_pccard_mem_write_multi (addr, count, buffer);
}

unsigned int has7752_pccard_io_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
    return has7752_pccard_mem_read_multi_direct (addr, count, buffer);
}

unsigned int has7752_pccard_io_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
    return has7752_pccard_mem_write_multi_direct (addr, count, buffer);
}


/*******************************************************************************
*******************************************************************************/

static int __init pccard_init (void)
{
   printk ("Initialising Memory bus + CPLD pccard interface\n");
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
EXPORT_SYMBOL(has7752_pccard_word_swap_enable);
EXPORT_SYMBOL(has7752_pccard_word_swap_disable);
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

