/*
 *  linux/include/asm-arm/arch-ssa/txtimeout_hack.h
 *
 *  Copyright (C) 2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_TXTIMEOUT_HACK_H_
#define __ASM_ARCH_TXTIMEOUT_HACK_H_

extern void txtimeout_hack_init (unsigned int value);
extern void txtimeout_hack_inc  (void);

#endif

