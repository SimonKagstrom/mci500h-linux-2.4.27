/*
 *  linux/include/asm-arm/arch-ssa/tsc.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_TSC_H_
#define __ASM_ARCH_TSC_H_

typedef struct
{
   unsigned int ticks_initial;
   unsigned int ticks_required;
}
ssa_timer_t;

extern unsigned int       ReadTSC           (void);
extern unsigned long long ReadTSC64         (void);
extern unsigned int       ReadTSCMHz        (void);
extern void               ssa_timer_set     (ssa_timer_t *timer, unsigned int usec);
extern int                ssa_timer_expired (ssa_timer_t *timer);
extern unsigned int       ssa_timer_ticks   (ssa_timer_t *timer);
extern void               ssa_udelay        (unsigned int usec);

#endif

