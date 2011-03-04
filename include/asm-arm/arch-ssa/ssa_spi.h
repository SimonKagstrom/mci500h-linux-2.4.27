/*
 *  linux/include/asm-arm/arch-ssa/ssa_spi.h
 *
 *  Copyright (C) 2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SSA_SPI_H
#define __ASM_ARCH_SSA_SPI_H

/*
   This header defines a common API for accessing an SPI host interface
   and should be included by all platform specific drivers supporting
   such an interface.
*/

extern void         ssa_spi_reset_drive_low   (void);
extern void         ssa_spi_reset_drive_high  (void);
extern int          ssa_spi_get_irq           (void);
extern int          ssa_spi_init              (void);
extern unsigned int ssa_spi_read_single8      (void);
extern unsigned int ssa_spi_write_single8     (unsigned char value);
extern unsigned int ssa_spi_read_single16_be  (void);
extern unsigned int ssa_spi_read_single16_le  (void);
extern unsigned int ssa_spi_write_single16_be (unsigned short value);
extern unsigned int ssa_spi_write_single16_le (unsigned short value);
extern unsigned int ssa_spi_read_multi        (unsigned char *dst, int count);
extern unsigned int ssa_spi_write_multi       (unsigned char *src, int count);

/* implicit little endian versions... eventually, these should go away */

extern unsigned int ssa_spi_read_single16     (void);
extern unsigned int ssa_spi_write_single16    (unsigned short value);

#endif

