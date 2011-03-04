/*
 * linux/arch/arm/mach-pnx0106/lcdif_wrapper.c
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/module.h>

#include <asm/hardware.h>
#include <asm/io.h>

#include <asm/arch/lcdif_wrapper.h>

unsigned int lcd_hw_if_read (unsigned int offset)
{
	unsigned int value;

	if (offset > LCD_HW_IF_RANGE) {
		printk ("\n lcd_hw_if offset exceed range,pls check!");
		return 0;
	}

	value = readl (IO_ADDRESS_LCDIF_BASE + offset);
//	printk ((value > 0xFF) ? "lcdr %02x %08x\n" : "lcdr %02x       %02x\n", offset, value);
	return value;
}

void lcd_hw_if_write (unsigned int offset, unsigned int value)
{
	if (offset > LCD_HW_IF_RANGE) {
		printk ("\n lcd_hw_if offset exceed range,pls check!");
		return;
	}

//	printk ((value > 0xFF) ? "lcdw %02x %08x\n" : "lcdw %02x       %02x\n", offset, value);
	writel (value, (IO_ADDRESS_LCDIF_BASE + offset));
}


/*****************************************************************************
*****************************************************************************/

#if 1
EXPORT_SYMBOL(lcd_hw_if_read);
EXPORT_SYMBOL(lcd_hw_if_write);
#endif

