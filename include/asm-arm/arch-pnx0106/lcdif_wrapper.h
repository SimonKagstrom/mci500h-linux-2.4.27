/*
 *  linux/include/asm-arm/arch-pnx0106/lcdif_wrapper.h
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_LCDIF_WRAPPER_H
#define __ASM_ARCH_LCDIF_WRAPPER_H

#define LCD_HW_IF_STATUS	0x00
#define LCD_HW_IF_CONTROL	0x04
#define LCD_HW_IF_INT_RAW	0x08
#define LCD_HW_IF_INT_CLEAR	0x0C 
#define LCD_HW_IF_INT_MASK	0x10
#define LCD_HW_IF_READ_CMD	0x14 
#define LCD_HW_IF_INST_BYTE	0x20
#define LCD_HW_IF_DATA_BYTE	0x30
#define LCD_HW_IF_INST_WORD	0x40
#define LCD_HW_IF_DATA_WORD	0x80

#define LCD_HW_IF_RANGE 	0x80


extern unsigned int lcd_hw_if_read (unsigned int offset);
extern void lcd_hw_if_write (unsigned int offset, unsigned int value);

#endif

