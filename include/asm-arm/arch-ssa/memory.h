/*
 *  linux/include/asm-arm/arch-ssa/memory.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MEMORY_H_
#define __ASM_ARCH_MEMORY_H_

#define TASK_SIZE	    (0x80000000UL)
#define TASK_SIZE_26	(0x04000000UL)

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE (TASK_SIZE / 2)

/*
 * Page offset: 3GB
 */
#define PAGE_OFFSET	    (0xC0000000UL)
#define PHYS_OFFSET	    (0xC0000000UL)

/*
 * SDRAM is contiguous
 */
#define __virt_to_phys__is_a_macro
#define __virt_to_phys(vpage) ((vpage) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt__is_a_macro
#define __phys_to_virt(ppage) ((ppage) + PAGE_OFFSET - PHYS_OFFSET)


/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations to an address that the kernel can use.
 
 THESE MAY BE WRONG !!! (BUT AS WE DON'T HAVE ANY DMA DEVICES, MAYBE ITS OK ???
 
 */
#define __virt_to_bus__is_a_macro
#define __virt_to_bus(x)	(x)
#define __bus_to_virt__is_a_macro
#define __bus_to_virt(x)	(x)


/*
   NID == Node ID (where nodes are individual sections of physical memory if physical memory is discontiguous).
   (If physical memory is contiguous, PHYS_TO_NID should be defined as (0)).
*/

#define PHYS_TO_NID(addr)	(0)

#endif
