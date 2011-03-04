/*
 *  linux/arch/arm/mach-pnx0106/mm.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/sizes.h>

#include <asm/mach/map.h>

static struct map_desc pnx0106_io_desc[] __initdata =
{
//  virtual, physical, length, domain, prot_read, prot_write, cacheable, bufferable

    { IO_ADDRESS_INTC_BASE,          INTC_REGS_BASE,               SZ_4K,     DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_E7B_BASE,           E7B_MEM_BASE,                 E7B_MEM_SIZE, DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_FLASH_BASE,         MPMC_NCS0_BASE,               SZ_4M,     DOMAIN_IO, 0, 1, 0, 0 },  /* 4MB is limit of MPMC address bus (device may be bigger or smaller) */
    { IO_ADDRESS_ISRAM_BASE,         INTERNAL_SRAM_BASE,           SZ_128K,   DOMAIN_IO, 0, 1, 0, 0 },  /* NB: internal SRAM size is actually 96k */

    { IO_ADDRESS_VPB0_BASE,          VPB0_PHYS_BASE,               VPB0_SIZE, DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_VPB1_BASE,          VPB1_PHYS_BASE,               VPB1_SIZE, DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_VPB2_BASE,          VPB2_PHYS_BASE,               VPB2_SIZE, DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_VPB3_BASE,          VPB3_PHYS_BASE,               VPB3_SIZE, DOMAIN_IO, 0, 1, 0, 0 },  /* NB: VPB3 range is only 2k */

    { IO_ADDRESS_PCCARD_BASE,        PCCARD_REGS_BASE,             SZ_4K,     DOMAIN_IO, 0, 1, 0, 0 },
#if defined (CONFIG_PNX0106_PCCARD)
    { IO_ADDRESS_PCCARD_COMMON_BASE, PCCARD_COMMON_PAGEFRAME_BASE, SZ_16M,    DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_PCCARD_ATTRIB_BASE, PCCARD_ATTRIB_PAGEFRAME_BASE, SZ_1M,     DOMAIN_IO, 0, 1, 0, 0 },
    { IO_ADDRESS_PCCARD_IO_BASE,     PCCARD_IO_PAGEFRAME_BASE,     SZ_16M,    DOMAIN_IO, 0, 1, 0, 0 },
#endif
#if defined (CONFIG_PNX0106_PCCARD_CPLD)
    { IO_ADDRESS_CPLD_BASE,          CPLD_REGS_BASE,               SZ_1M,     DOMAIN_IO, 0, 1, 0, 0 },
#endif

    LAST_DESC
};

void __init pnx0106_map_io (void)
{
    iotable_init (pnx0106_io_desc);
}

