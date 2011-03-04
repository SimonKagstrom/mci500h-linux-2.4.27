/*
 * linux/arch/arm/mach-ssa/w3_spi.c
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

#include <asm/arch/ssa_spi.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tsc.h>

#ifdef CONFIG_SSA_W3_PCCARD
#error "CONFIG_SSA_W3_PCCARD and CONFIG_SSA_W3_SPI should not both be defined"
#endif

#ifdef CONFIG_SSA_W3_GPI
#error "CONFIG_SSA_W3_GPI and CONFIG_SSA_W3_SPI should not both be defined"
#endif

#define DEBUG_LEVEL             (1)


/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_SSA_W3_SPI)

#define WLAN_RESET_LINE     GPIO_SPI_RESET
#define CPLD_RESET_LINE     GPIO_CPLD_RESET
#define SPI_IRQ             INT_SPI

#elif defined (CONFIG_SSA_TAHOE)

#define WLAN_RESET_LINE     GPIO_WLAN_AND_CPLD_RESET
#undef  CPLD_RESET_LINE     
#define SPI_IRQ             INT_WLAN

#else
#error "Please define a target platform"
#endif


/*****************************************************************************
*****************************************************************************/

void ssa_spi_reset_drive_low (void)
{
   gpio_set_as_output (WLAN_RESET_LINE, 0);
}

void ssa_spi_reset_drive_high (void)
{
   gpio_set_as_output (WLAN_RESET_LINE, 1);
}

/*
   Return the interrupt number which the driver should use when
   registering an interrupt handler for the SPI target device.
*/
int ssa_spi_get_irq (void)
{
   return SPI_IRQ;
}

/*
   Perform HW init etc.
   Called during kernel startup.
   May also be called by users of the SPI interface for last resort disaster recovery etc...
*/
int ssa_spi_init (void)
{
#if defined (CPLD_RESET_LINE)
   gpio_set_as_output (CPLD_RESET_LINE, 0);     /* drive low to force CPLD reset */
   udelay (1);
   gpio_set_as_output (CPLD_RESET_LINE, 1);
#endif

   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Perform a single low level 8bit SPI read
*/
unsigned int ssa_spi_read_single8 (void)
{
   unsigned int temp;
   
   temp = readb (IO_ADDRESS_EPLD_BASE);

   if (DEBUG_LEVEL > 3)
      printk ("%s : 0x%02x\n", __FUNCTION__, temp);

   return temp;
}

/*
   Perform a single low level 8bit SPI write
*/
unsigned int ssa_spi_write_single8 (unsigned char value)
{
   if (DEBUG_LEVEL > 3)
      printk ("%s: 0x%02x\n", __FUNCTION__, value);

   writeb (value, IO_ADDRESS_EPLD_BASE);

   return 0;
}

/*
   Perform a 16bit SPI read.

   Note that this routine returns data in host endian format (e.g. on the
   little endian SAA7752 ARM platform, first byte read from SPI will be
   returned as the LSByte of the result, second byte read will be the MSByte).
   
   The device driver using this SPI interface is responsible for converting
   from host endian format to the endian format used by the target device !!
*/
unsigned int ssa_spi_read_single16 (void)
{
   unsigned int temp;
   
   temp = readw (IO_ADDRESS_EPLD_BASE);

   if (DEBUG_LEVEL > 3)
      printk ("%s : 0x%04x\n", __FUNCTION__, temp);

   return temp;
}

/*
   Perform a 16bit SPI write.

   Note that this routine sends data in host endian format (e.g. on the little
   endian on SAA7752 ARM platform, first byte sent on SPI will be the LSByte of
   the input value, second byte sent will be the MSByte of the input value).
   
   The device driver using this SPI interface is responsible for converting
   the output value from the endian format used by the target device into host
   endian format before calling this function !!
*/
unsigned int ssa_spi_write_single16 (unsigned short value)
{
   if (DEBUG_LEVEL > 3)
      printk ("%s: 0x%04x\n", __FUNCTION__, value);

   writew (value, IO_ADDRESS_EPLD_BASE);

   return 0;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read 'count' bytes from SPI into 'dst' buffer.
*/
unsigned int ssa_spi_read_multi (unsigned char *dst, int count)
{
   unsigned int quadlet_count;
   unsigned char *dst_end = &dst[count];

   if (DEBUG_LEVEL > 1)
      printk ("%s : dst:0x%08lx, count:%4d\n", __FUNCTION__, (unsigned long) dst, count);

   while ((((unsigned long) dst) & 0x03) && (dst < dst_end))
      *dst++ = readb (IO_ADDRESS_EPLD_BASE);

   quadlet_count = ((dst_end - dst) / 4);
   insl (IO_ADDRESS_EPLD_BASE, dst, quadlet_count);
   dst += (quadlet_count * 4);

   while (dst < dst_end)
      *dst++ = readb (IO_ADDRESS_EPLD_BASE);

   return 0;
}

/*
   Write 'count' bytes from 'src' buffer to SPI.
*/
unsigned int ssa_spi_write_multi (unsigned char *src, int count)
{
   unsigned int quadlet_count;
   unsigned char *src_end = &src[count];

   if (DEBUG_LEVEL > 1)
      printk ("%s: src:0x%08lx, count:%4d\n", __FUNCTION__, (unsigned long) src, count);

   while ((((unsigned long) src) & 0x03) && (src < src_end))
      writeb (*src++, IO_ADDRESS_EPLD_BASE);

   quadlet_count = ((src_end - src) / 4);
   outsl (IO_ADDRESS_EPLD_BASE, src, quadlet_count);
   src += (quadlet_count * 4);

   while (src < src_end)
      writeb (*src++, IO_ADDRESS_EPLD_BASE);

   return 0;
}


/*******************************************************************************
*******************************************************************************/

static int __init w3_spi_init (void)
{
   printk ("Initialising W3 SPI interface\n");
   return ssa_spi_init();
}

__initcall (w3_spi_init);

/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(ssa_spi_reset_drive_low);
EXPORT_SYMBOL(ssa_spi_reset_drive_high);
EXPORT_SYMBOL(ssa_spi_get_irq);
EXPORT_SYMBOL(ssa_spi_init);
EXPORT_SYMBOL(ssa_spi_read_single8);
EXPORT_SYMBOL(ssa_spi_write_single8);
EXPORT_SYMBOL(ssa_spi_read_single16);
EXPORT_SYMBOL(ssa_spi_write_single16);
EXPORT_SYMBOL(ssa_spi_read_multi);
EXPORT_SYMBOL(ssa_spi_write_multi);

