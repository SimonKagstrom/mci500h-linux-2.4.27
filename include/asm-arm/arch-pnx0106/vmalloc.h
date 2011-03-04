/*
 *  linux/include/asm-arm/arch-ssa/vmalloc.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

/*
   Keep all vmalloc addresses in a fixed range rather than starting from
   (just past the..) end of SDRAM. Note that this breaks if we ever have
   more than 128M Bytes physical memory...
*/

#define VMALLOC_START     (PAGE_OFFSET + 0x08000000)
#define VMALLOC_END       (PAGE_OFFSET + 0x10000000)
#define VMALLOC_VMADDR(x) ((unsigned long)(x))

