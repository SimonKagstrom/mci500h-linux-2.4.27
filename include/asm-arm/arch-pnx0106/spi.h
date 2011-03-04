/*
 *  linux/include/asm-arm/arch-pnx0106/spi.h
 *
 *  Copyright (C) 2004-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SPI_H
#define __ASM_ARCH_SPI_H

/*
   This header defines a common API for accessing an SPI host interface
   and should be included by all platform specific drivers supporting
   such an interface.
*/

extern void         ssa_spi_reset_drive_low (void);
extern void         ssa_spi_reset_drive_high (void);
extern int          ssa_spi_get_irq (void);
extern int          ssa_spi_init (void);
extern unsigned int ssa_spi_read_single8 (void);
extern unsigned int ssa_spi_write_single8 (unsigned char value);
extern unsigned int ssa_spi_read_single16 (void);
extern unsigned int ssa_spi_write_single16 (unsigned short value);
extern unsigned int ssa_spi_read_multi (unsigned char *dst, unsigned int count);
extern unsigned int ssa_spi_write_multi (unsigned char *src, unsigned int count);

#endif

