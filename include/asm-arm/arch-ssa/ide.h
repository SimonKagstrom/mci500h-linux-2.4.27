/*
 *  linux/include/asm-arm/arch-ssa/ide.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
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
   SSA only has one ata interface.
   Note that we need to tweak the arm specific header file which includes this
   one to ensure that this value does not get ignored...
*/

#define MAX_HWIFS   1

/*
   The SSA ATA interface requires that all accesses are 16bit wide, therefore
   we must provide custom access functions for accessing the byte wide ATA
   taskfile registers.
   
   In 2.4.19, this was done with the following macros:
   
      #define OUT_BYTE(b,p)          writew((b),(p))
      #define OUT_WORD(w,p)          writew((w),(p))

      #define IN_BYTE(p)             (byte) readw(p)
      #define IN_WORD(p)             (short)readw(p)

      #define HAVE_ARCH_OUT_BYTE
      #define HAVE_ARCH_IN_BYTE
   
   For 2.4.21 however, the ide code has been changed and now requires us to
   provide our own access functions. We must write pointers to these access
   functions into the interface 'ide_hwif' struct, using the pointer passed
   back by ide_register_hw().

   Or at least we would do if we knew what a 'ide_hwif_t' was at this point
   Currently header file include order means we don't.
   Instead we hack drivers/ide/ide-iops.c and override the defaults with our
   own custom requirements...
*/

extern void ide_init_default_hwifs (void);

static __inline__ void ide_init_hwif_ports (hw_regs_t *hw, int data_port, int ctrl_port, int *irq)
{
   /*
      ARM architecture code doesn't seem to have any default ide
      port addresses... so this code is redundent ??
   */
}

#endif

