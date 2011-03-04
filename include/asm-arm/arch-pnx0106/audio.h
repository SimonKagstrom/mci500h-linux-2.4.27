/*
 *  linux/include/asm-arm/arch-pnx0106/audio.h
 *
 *  Copyright (C) 2006 TJ Merritt, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_AUDIO_H
#define __ASM_ARCH_AUDIO_H

#include <linux/ioctl.h>

typedef struct pnx0106_audio_stats
{
    int norm_fill;              /* normal DMA fill */
    int norm_underrun_empty;    /* Underrun w/no data */
    int norm_underrun_partial;  /* Underrun w/data */
    int norm_stalled;    	/* Underrun after underrun */
    int sync_late_norm;         /* normal late synchronization */
    int sync_late_nodata;       /* late sync w/no data */
    int sync_late_underrun;     /* late sync w/partial data */
    int sync_wait;              /* normal sync */
    int sync_underrun;          /* sync w/no data */
    int sync_fill;              /* sync w/full data */
}
pnx0106_audio_stats_t;

#define PNX0106_AUDIO_STATS		_IOW ('A', 1, struct pnx0106_audio_stats)

#endif
