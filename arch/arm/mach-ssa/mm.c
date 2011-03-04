/*
 *  linux/arch/arm/mach-ssa/mm.c
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

static struct map_desc ssa_io_desc[] __initdata =
{
// virtual, physical, length, domain, prot_read, prot_write, cacheable, bufferable

   { IO_ADDRESS_INTC_BASE,          SSA_INTC_BASE,          SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_CT0_BASE,           SSA_CT0_BASE,           SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_CT1_BASE,           SSA_CT1_BASE,           SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_UART0_BASE,         SSA_UART0_BASE,         SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_GPIO_BASE,          SSA_GPIO_BASE,          SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_SMC_BASE,           SSA_SMC_BASE,           SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_FLASH_BASE,         SSA_NCS0_BASE,          SZ_2M,  DOMAIN_IO, 0, 1, 0, 0 },   /* 2MB is limit of EBI address bus. Device may be smaller */
   { IO_ADDRESS_ATU_BASE,           SSA_ATU_BASE,           SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_ATU_FIFO_BASE,      ATU_CH1_LEFT_FIFO_BASE, SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_CDB_BASE,           SSA_CDB_BASE,           SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_IICMASTER_BASE,     SSA_IICMASTER_BASE,     SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_IICSLAVE_BASE,      SSA_IICSLAVE_BASE,      SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_CLOCK_CTRL_BASE,    SSA_CLOCK_CTRL_BASE,    SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_SDAC_BASE,          SSA_SDAC_BASE,          SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
   { IO_ADDRESS_ISRAM_BASE,         SSA_ISRAM_BASE,         SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },

#if defined (CONFIG_SSA_HAS7750) || \
    defined (CONFIG_SSA_HAS7752) || \
    defined (CONFIG_SSA_W3)      || \
    defined (CONFIG_SSA_TAHOE)
   { IO_ADDRESS_EPLD_BASE,          SSA_NCS2_BASE,          SZ_1M,  DOMAIN_IO, 0, 1, 0, 0 },
#endif

   LAST_DESC
};

void __init ssa_map_io (void)
{
   iotable_init (ssa_io_desc);
}

