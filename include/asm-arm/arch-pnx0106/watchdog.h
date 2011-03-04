/*
 *  linux/include/asm-arm/arch-pnx0106/watchdog.h
 *
 *  Copyright (C) 2005-2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_WATCHDOG_H_
#define __ASM_ARCH_WATCHDOG_H_

extern unsigned int reset_was_via_watchdog (void);
extern void watchdog_register_pre_reset_callback (void (*handler) (void));
extern void watchdog_force_reset (int add_extra_delay);

#endif

