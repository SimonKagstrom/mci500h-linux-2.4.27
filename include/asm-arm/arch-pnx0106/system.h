/*
 *  linux/include/asm-arm/arch-pnx0106/system.h
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/config.h>

#include <asm/hardware.h>
#include <asm/arch/watchdog.h>
#include <asm/io.h>

static void arch_idle (void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset (char mode)
{
	watchdog_force_reset (1);	/* this never comes back... */
}

#endif
