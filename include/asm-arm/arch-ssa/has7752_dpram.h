/*
 *  linux/include/asm-arm/arch-ssa/has7752_dpram.h
 *
 *  Copyright (C) 2004 Thomas J. Merritt, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_HAS7752_DPRAM_H
#define __ASM_ARCH_HAS7752_DPRAM_H

#include <linux/ioctl.h>

#define DPRAM_MINOR		210

#define HAS7752_DPRAM_GET_SIZE			_IOW ('D', 1, int)
#define HAS7752_DPRAM_CLR_WR			_IO  ('D', 2)
#define HAS7752_DPRAM_CLR_RD			_IO  ('D', 3)
#define HAS7752_DPRAM_CLR_WRAP_CFG		_IO  ('D', 4)

#endif
