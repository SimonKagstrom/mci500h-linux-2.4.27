/*
 *  linux/include/asm-arm/arch-ssa/ssa_gpi.h
 *
 *  Copyright (C) 2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SSA_GPI_H
#define __ASM_ARCH_SSA_GPI_H

/*
   This header defines a common API for accessing a GPI host interface
   and should be included by all platform specific drivers supporting
   such an interface.
   
   Initially the only platform with a GPI host interface is the W3
   module (which provides a GPI compatible interface via a CPLD).
   In the future, there will be other platforms which provide a glueless
   GPI interface but hopefully they can use the same basic host API...
   
   This is the ONLY platform specific header file which should be
   needed by the SA5252 device driver (if you think there is a platform
   specific function or definition which can't be found from this API
   alone, please email andre.mccurdy@philips.com)
*/

extern void         ssa_gpi_reset_assert (void);
extern void         ssa_gpi_reset_deassert (void);
extern int          ssa_gpi_read_gucirdy (void);
extern int          ssa_gpi_read_gdfcirdy (void);
extern int          ssa_gpi_get_irq (void);
extern int          ssa_gpi_init (void);
extern unsigned int ssa_gpi_control_read_single (void);
extern unsigned int ssa_gpi_control_write_single (unsigned int value);
extern unsigned int ssa_gpi_data_read_single (void);
extern unsigned int ssa_gpi_data_write_single (unsigned int value);
extern unsigned int ssa_gpi_read_hi (unsigned int frame, unsigned char *buffer, unsigned int bytecount);
extern unsigned int ssa_gpi_write_hi (unsigned int frame, unsigned char *buffer, unsigned int bytecount);
extern unsigned int ssa_gpi_read_di (unsigned char *buffer, unsigned int bytecount);
extern unsigned int ssa_gpi_write_di (unsigned char *buffer, unsigned int bytecount);

#endif

