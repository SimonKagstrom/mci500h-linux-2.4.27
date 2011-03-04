/*
 *  linux/include/asm-arm/arch-pnx0106/adc.h
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_ADC_H
#define __ASM_ARCH_ADC_H

#define NR_ADC_CHANNELS		(4)	/* PNX0106 has 4 channels (0..3) */

extern unsigned int adc_read (unsigned int channel);

/* PDCC API */
extern unsigned int adc_get_value (unsigned int channel);
extern void adcdrv_set_mode (int mode);
extern void adcdrv_disable_adc (void);

#endif

