/*
 *  linux/include/asm-arm/arch-pnx0106/spisd.h
 *
 *  Copyright (C) 2004-2006 Andre McCurdy, NXP Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SPISD_H
#define __ASM_ARCH_SPISD_H

/*
   This header defines a common API for accessing an SPI host interface
   and should be included by all platform specific drivers supporting
   such an interface.
*/

#define NCS_LEAVE_LOW		(0)
#define NCS_LEAVE_HIGH		(1)
#define NCS_LEAVE_UNCHANGED	(2)

extern int          spisd_init (void);
extern int          spisd_get_irq (void);
extern void         spisd_ncs_drive_high (void);
extern void         spisd_ncs_drive_low (void);
extern void         spisd_reset_drive_high (void);
extern void         spisd_reset_drive_low (void);
extern unsigned int spisd_read_multi (unsigned char *dst, unsigned int count);
extern unsigned int spisd_read_multi_setncs (unsigned char *dst, unsigned int count, int ncs);
extern unsigned int spisd_read_single16_be (void);
extern unsigned int spisd_read_single16_le (void);
extern unsigned int spisd_read_single8 (void);
extern unsigned int spisd_read_single8_setncs (int ncs);
extern unsigned int spisd_write_multi (unsigned char *src, unsigned int count);
extern unsigned int spisd_write_multi_setncs (unsigned char *src, unsigned int count, int ncs);
extern unsigned int spisd_write_single16_be (unsigned short value);
extern unsigned int spisd_write_single16_le (unsigned short value);
extern unsigned int spisd_write_single8 (unsigned char value);
extern unsigned int spisd_write_single8_setncs (unsigned char value, int ncs);

#endif

