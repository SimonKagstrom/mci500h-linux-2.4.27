/*
 *  linux/include/asm-arm/arch-ssa/system.h
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
#include <asm/arch/gpio.h>
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
	/*
	 * Shutdown CPU and jump to start of
	 * bootloader in external Flash (first make
	 * sure the correct Flash bank is selected...)
	 */

#if defined (CONFIG_SSA_HAS7752)
	gpio_set_as_output (GPIO_FLASH_BANK_SELECT, 0);
#endif

	cpu_reset (SSA_NCS0_BASE);
}

#endif
