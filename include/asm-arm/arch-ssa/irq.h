/*
 *  linux/include/asm-arm/arch-ssa/irq.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#define fixup_irq(i)    (i)

/*
   Architecture specific IRQ initialisation...
*/
static __inline__ void irq_init_irq (void)
{
}

