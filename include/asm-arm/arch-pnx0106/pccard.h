/*
 *  linux/include/asm-arm/arch-pnx0106/pccard.h
 *
 *  Copyright (C) 2004-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_PCCARD_H
#define __ASM_ARCH_PCCARD_H

#define MAX_ADDRESS_SETUP_WAITSTATES    (15)
#define MAX_DATA_STROBE_WAITSTATES      (31)
#define MAX_ADDRESS_HOLD_WAITSTATES     ( 7)

extern int          has7752_pccard_init (void);
extern void         has7752_pccard_reset_assert (void);
extern void         has7752_pccard_reset_deassert (void);
extern int          has7752_pccard_read_ready (void);
extern int          has7752_pccard_get_irq (void);
extern void         has7752_pccard_set_mem_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws);
extern void         has7752_pccard_set_io_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws);
extern int          has7752_pccard_word_swap_enable (void);
extern int          has7752_pccard_word_swap_disable (void);

extern unsigned int has7752_pccard_attribute_read (unsigned int addr);
extern unsigned int has7752_pccard_attribute_write (unsigned int addr, unsigned char value);

extern unsigned int has7752_pccard_io_read (unsigned int addr);
extern unsigned int has7752_pccard_io_write (unsigned int addr, unsigned short value);
extern unsigned int has7752_pccard_io_read_multi (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_io_write_multi (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_io_read_multi_direct (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_io_write_multi_direct (unsigned int addr, int count, unsigned short *buffer);

extern unsigned int has7752_pccard_mem_read (unsigned int addr);
extern unsigned int has7752_pccard_mem_write (unsigned int addr, unsigned short value);
extern unsigned int has7752_pccard_mem_read_multi (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_mem_write_multi (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_mem_read_multi_direct (unsigned int addr, int count, unsigned short *buffer);
extern unsigned int has7752_pccard_mem_write_multi_direct (unsigned int addr, int count, unsigned short *buffer);

#endif
