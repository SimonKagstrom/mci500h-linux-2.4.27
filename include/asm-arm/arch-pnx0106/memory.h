/*
 *  linux/include/asm-arm/arch-pnx0106/memory.h
 *
 *  Copyright (C) 2002-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MEMORY_H_
#define __ASM_ARCH_MEMORY_H_

#include <asm/arch/hardware.h>

#define TASK_SIZE	(0x80000000UL)
#define TASK_SIZE_26	(0x04000000UL)

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE (TASK_SIZE / 2)

/*
 * Page offset: 3GB
 */
#define PAGE_OFFSET	(0xC0000000UL)
#define PHYS_OFFSET	(SDRAM_BANK0_BASE)

/*
 * SDRAM is contiguous
 */
#define __virt_to_phys__is_a_macro
#define __virt_to_phys(vpage) ((vpage) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt__is_a_macro
#define __phys_to_virt(ppage) ((ppage) + PAGE_OFFSET - PHYS_OFFSET)

/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an
 *              address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations
 *              to an address that the kernel can use.
 */
#define __virt_to_bus__is_a_macro
#define __virt_to_bus(x) __virt_to_phys(x)
#define __bus_to_virt__is_a_macro
#define __bus_to_virt(x) __phys_to_virt(x)

/*
 * NID == Node ID
 * Physical memory is contiguous, therefore PHYS_TO_NID should be defined as (0)
 */

#define PHYS_TO_NID(addr)	(0)

#endif
