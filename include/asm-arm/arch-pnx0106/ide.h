/*
 *  linux/include/asm-arm/arch-pnx0106/ide.h
 *
 *  Copyright (C) 2002-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_IDE_H_
#define __ASM_ARCH_IDE_H_

#include <asm/irq.h>
#include <asm/arch/hardware.h>

/*
   PNX0106 only has one ata interface.
   Note that we need to tweak the arm specific header file which includes this
   one to ensure that this value does not get ignored...
*/

#define MAX_HWIFS   1


extern void ide_init_default_hwifs (void);

static __inline__ void ide_init_hwif_ports (hw_regs_t *hw, int data_port, int ctrl_port, int *irq)
{
   /*
      ARM architecture code doesn't seem to have any default ide
      port addresses... so this code is redundent ??
   */
}

#endif

