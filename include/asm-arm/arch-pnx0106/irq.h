/*
 *  linux/include/asm-arm/arch-pnx0106/irq.h
 *
 *  Copyright (C) 2002-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_IRQ_H_
#define __ASM_ARCH_IRQ_H_

/*
   Architecture specific IRQ initialisation...
*/
static __inline__ void irq_init_irq (void)
{
}

#define fixup_irq(i) (i)

#endif

