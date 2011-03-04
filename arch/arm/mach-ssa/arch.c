/*
 *  linux/arch/arm/mach-ssa/arch.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
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

extern void ssa_map_io(void);
extern void ssa_init_irq(void);
extern void ssa_fixup(void);

/*
   Build the struct machine_desc strutcure.

   BOOT_MEM(pram,pio,vio)
   'pram' specifies the physical start address of RAM.  Must always
   be present, and should be the same as PHYS_OFFSET.

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

#if defined (CONFIG_SSA_SIIG)

MACHINE_START(SSA, "Philips SAA7752 SIIG Connectivity Demo System")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#elif defined (CONFIG_SSA_WESSLI)

MACHINE_START(SSA, "Philips SAA7752 WessLi Module")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#elif defined (CONFIG_SSA_W3)

MACHINE_START(SSA, "Philips SAA7752 WessLi-3 (HacLi) Module")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#elif defined (CONFIG_SSA_TAHOE)

MACHINE_START(SSA, "Philips SAA7752 Tahoe Development Board")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#elif defined (CONFIG_SSA_HAS7750)

MACHINE_START(SSA, "Philips Home Audio Server (SAA7750 cpu)")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#elif defined (CONFIG_SSA_HAS7752)

MACHINE_START(SSA, "Philips Home Audio Server (SAA7752 cpu)")
	MAINTAINER("Andre McCurdy")
	BOOT_MEM(0xC0000000, 0x88000000, 0xF8800000)
	BOOT_PARAMS(0xC0000100)
//	FIXUP(ssa_fixup)
	MAPIO(ssa_map_io)
	INITIRQ(ssa_init_irq)
MACHINE_END

#else
#error "SSA subarchitecture is undefined"
#endif

