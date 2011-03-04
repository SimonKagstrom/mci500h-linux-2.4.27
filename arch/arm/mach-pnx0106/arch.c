/*
 *  linux/arch/arm/mach-pnx0106/arch.c
 *
 *  Copyright (C) 2002-2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <asm/hardware.h>

extern void pnx0106_fixup(void);
extern void pnx0106_map_io(void);
extern void pnx0106_init_irq(void);

/*
   Build the struct machine_desc structure.

   BOOT_MEM(pram,pio,vio)
   'pram' specifies the physical start address of RAM.  Must always
   be present, and should be the same as PHYS_OFFSET (see arch/arm/boot/Makefile).

   `pio' is the physical address of an 8MB region containing IO for
   use with the debugging macros in arch/arm/kernel/debug-armv.S.

   `vio' is the virtual address of the 8MB debugging region.

   It is expected that the debugging region will be re-initialised
   by the architecture specific code later in the code (via the
   MAPIO function).

   BOOT_PARAMS
   Same as, and see PARAMS_PHYS.

   FIXUP(func)
   Machine specific fixups, run before memory subsystems have been initialised.

   MAPIO(func)
   Machine specific function to map IO areas (including the debug region above).

   INITIRQ(func)
   Machine specific function to initialise interrupts.

   See also linux/include/asm-arm/mach/arch.h for more details.
*/

MACHINE_START(PNX0106, BOARD_NAMESTRING)
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(SDRAM_BANK0_BASE, 0x88000000, 0xF8800000)
	BOOT_PARAMS((SDRAM_BANK0_BASE + 0x100))
//	FIXUP(pnx0106_fixup)
	MAPIO(pnx0106_map_io)
	INITIRQ(pnx0106_init_irq)
MACHINE_END

