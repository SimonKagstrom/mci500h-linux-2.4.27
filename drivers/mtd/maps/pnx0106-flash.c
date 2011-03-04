/*
 *  linux/drivers/mtd/maps/pnx0106-flash.c
 *
 *  Copyright (C) 2002-2007 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/arch/watchdog.h>

#if defined (CONFIG_PNX0106_HACLI7) || \
	defined (CONFIG_PNX0106_MCI)

#define FLASH_SZ_MAX    SZ_16M                  /* 4 banks of 4 MBytes... */

#elif defined (CONFIG_PNX0106_W6)       || \
      defined (CONFIG_PNX0106_LPAS1)    || \
      defined (CONFIG_PNX0106_LDMA1)    || \
      defined (CONFIG_PNX0106_RSS1)     || \
      defined (CONFIG_PNX0106_RSC1)

#define FLASH_SZ_MAX    SZ_8M                   /* 2 banks of 4 MBytes... */

#elif defined (CONFIG_PNX0106_WMA100)   || \
      defined (CONFIG_PNX0106_WSA)      || \
      defined (CONFIG_PNX0106_GH526)

#define FLASH_SZ_MAX    SZ_4M                   /* 1 banks of 4 MBytes... */

#elif defined (CONFIG_PNX0106_LAGUNA_REVA) || \
      defined (CONFIG_PNX0106_HASLI7) || \
      defined (CONFIG_PNX0106_MCIH)

#define FLASH_SZ_MAX    SZ_2M                   /* 1/2 of a 4 MByte bank... */

#else
#error "Platform is not supported"
#endif


/*****************************************************************************
*****************************************************************************/

#if (FLASH_SZ_MAX == SZ_16M)

#ifndef SZ_12M
#define SZ_12M (SZ_8M + SZ_4M)
#endif

static unsigned long pnx0106_select_flash_bank (unsigned long addr)
{
	static int bank_previous = -1;
	int bank = 0;

	while (1) {
		if (addr < SZ_4M)
			break;
		addr -= SZ_4M;
		bank++;
	}
	
	if (bank != bank_previous) {
		switch (bank)
		{
			case 0:
				gpio_set_low (GPIO_FLASH_A21);
				gpio_set_low (GPIO_FLASH_A22);
				break;
			case 1:
				gpio_set_high (GPIO_FLASH_A21);
				gpio_set_low (GPIO_FLASH_A22);
				break;
			case 2:
				gpio_set_low (GPIO_FLASH_A21);
				gpio_set_high (GPIO_FLASH_A22);
				break;
			case 3:
				gpio_set_high (GPIO_FLASH_A21);
				gpio_set_high (GPIO_FLASH_A22);
				break;

			default:
				panic ("%s: unexpected bank (%d)\n", __FUNCTION__, bank);
				break;
		}

		bank_previous = bank;
	}

	return addr;
}

static u16 pnx0106_read16 (struct map_info *map, unsigned long ofs)
{
	ofs = pnx0106_select_flash_bank (ofs);
	return readw (IO_ADDRESS_FLASH_BASE + ofs);
}

static void pnx0106_write16 (struct map_info *map, __u16 d, unsigned long adr)
{
//	if ((adr & 0x0000FFFF) == 0)
//		printk ("Flash w16: 0x%08x=0x%04x\n", adr, d);

	adr = pnx0106_select_flash_bank (adr);
	writew (d, (IO_ADDRESS_FLASH_BASE + adr));
}

static void pnx0106_copy_from (struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	unsigned long len_partial;

	if ((from < SZ_4M) && (len != 0)) {
		pnx0106_select_flash_bank (0);
		len_partial = (len < (SZ_4M - from)) ? len : (SZ_4M - from);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + (from - 0)), len_partial);
		len -= len_partial;
		from += len_partial;
		to = (void *) (((unsigned long) to) + len_partial);
	}

	if ((from < SZ_8M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_4M);
		len_partial = (len < (SZ_8M - from)) ? len : (SZ_8M - from);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + (from - SZ_4M)), len_partial);
		len -= len_partial;
		from += len_partial;
		to = (void *) (((unsigned long) to) + len_partial);
	}

	if ((from < SZ_12M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_8M);
		len_partial = (len < (SZ_12M - from)) ? len : (SZ_12M - from);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + (from - SZ_8M)), len_partial);
		len -= len_partial;
		from += len_partial;
		to = (void *) (((unsigned long) to) + len_partial);
	}

	if ((from < SZ_16M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_12M);
		len_partial = (len < (SZ_16M - from)) ? len : (SZ_16M - from);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + (from - SZ_12M)), len_partial);
//		len -= len_partial;
//		from += len_partial;
//		to = (void *) (((unsigned long) to) + len_partial);
	}
}

static void pnx0106_copy_to (struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	unsigned long len_partial;

	printk ("memcopying to Flash... ?!?\n");

	if ((to < SZ_4M) && (len != 0)) {
		pnx0106_select_flash_bank (0);
		len_partial = (len < (SZ_4M - to)) ? len : (SZ_4M - to);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + to), (from - 0), len_partial);
		len -= len_partial;
		from = (void *) (((unsigned long) from) + len_partial);
		to += len_partial;
	}

	if ((to < SZ_8M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_4M);
		len_partial = (len < (SZ_8M - to)) ? len : (SZ_8M - to);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + to), (from - SZ_4M), len_partial);
		len -= len_partial;
		from = (void *) (((unsigned long) from) + len_partial);
		to += len_partial;
	}

	if ((to < SZ_12M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_8M);
		len_partial = (len < (SZ_12M - to)) ? len : (SZ_12M - to);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + to), (from - SZ_8M), len_partial);
		len -= len_partial;
		from = (void *) (((unsigned long) from) + len_partial);
		to += len_partial;
	}

	if ((to < SZ_16M) && (len != 0)) {
		pnx0106_select_flash_bank (SZ_12M);
		len_partial = (len < (SZ_16M - to)) ? len : (SZ_16M - to);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + to), (from - SZ_12M), len_partial);
//		len -= len_partial;
//		from = (void *) (((unsigned long) from) + len_partial);
//		to += len_partial;
	}
}

static void pre_reset_callback (void)
{
	int i;
	pnx0106_select_flash_bank (0);
	for (i = 0; i < 8; i += 2)
		printk ("0x%08x = 0x%04x\n", (MPMC_FLASH_BASE + i), readw (IO_ADDRESS_FLASH_BASE + i));
}

#elif (FLASH_SZ_MAX == SZ_8M)

static inline unsigned long pnx0106_select_flash_bank (unsigned long addr)
{
	if (addr < SZ_4M) {
		gpio_set_low (GPIO_FLASH_A21);
		return addr;
	}
	gpio_set_high (GPIO_FLASH_A21);
	return (addr - SZ_4M);
}

static u16 pnx0106_read16 (struct map_info *map, unsigned long ofs)
{
	ofs = pnx0106_select_flash_bank (ofs);
	return readw (IO_ADDRESS_FLASH_BASE + ofs);
}

static void pnx0106_write16 (struct map_info *map, __u16 d, unsigned long adr)
{
//	if ((adr & 0x0000FFFF) == 0)
//		printk ("Flash w16: 0x%08x=0x%04x\n", adr, d);

	adr = pnx0106_select_flash_bank (adr);
	writew (d, (IO_ADDRESS_FLASH_BASE + adr));
}

static void pnx0106_copy_from (struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	unsigned long len_partial;

	if (from < SZ_4M) {
		pnx0106_select_flash_bank (0);
		len_partial = (len < (SZ_4M - from)) ? len : (SZ_4M - from);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + from), len_partial);
		len -= len_partial;
		from += len_partial;
		to = (void *) (((unsigned long) to) + len_partial);
	}

	if (from >= SZ_4M) {
		pnx0106_select_flash_bank (SZ_4M);
		memcpy_fromio (to, (void *)(IO_ADDRESS_FLASH_BASE + (from - SZ_4M)), len);
	}
}

static void pnx0106_copy_to (struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	unsigned long len_partial;

	printk ("memcopying to Flash... ?!?\n");

	if (to < SZ_4M) {
		pnx0106_select_flash_bank (0);
		len_partial = (len < (SZ_4M - to)) ? len : (SZ_4M - to);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + to), from, len_partial);
		len -= len_partial;
		from = (void *) (((unsigned long) from) + len_partial);
		to += len_partial;
	}

	if (to >= SZ_4M) {
		pnx0106_select_flash_bank (SZ_4M);
		memcpy_toio ((void *)(IO_ADDRESS_FLASH_BASE + (to - SZ_4M)), from, len);
	}
}

static void pre_reset_callback (void)
{
	int i;
	pnx0106_select_flash_bank (0);
	for (i = 0; i < 8; i += 2)
		printk ("0x%08x = 0x%04x\n", (MPMC_FLASH_BASE + i), readw (IO_ADDRESS_FLASH_BASE + i));
}

#endif


/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_PNX0106_MCI)  || \
	defined (CONFIG_PNX0106_HACLI7)

static struct mtd_partition pnx0106_partitions_16mb[] =
{
    {
        name:   "bl1",
        size:   (SZ_64K * 2),                   /* 128k for first bootloader */
        offset: 0x00000000
    },
    {
        name:   "bl2",
        size:   (SZ_64K * 2),                   /* 128k for backup bootloader */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob1",
        size:   (SZ_64K * 184),                 /* 11776k for combined image 1 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob2",
        size:   (SZ_64K * 64),                  /* 4096k for combined image 2 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env1",
        size:   (SZ_64K * 2),                   /* 128k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env2",
        size:   (SZ_64K * 2),                   /* 128k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    }
};

#endif

#if defined (CONFIG_PNX0106_W6)         || \
    defined (CONFIG_PNX0106_LPAS1)      || \
    defined (CONFIG_PNX0106_LDMA1)      || \
    defined (CONFIG_PNX0106_RSS1)       || \
    defined (CONFIG_PNX0106_RSC1)       || \
    defined (CONFIG_PNX0106_MCI)       || \
    defined (CONFIG_PNX0106_HACLI7)

static struct mtd_partition pnx0106_partitions_8mb[] =
{
    {
        name:   "bl1",
        size:   (SZ_64K * 1),                   /* 64k for first bootloader */
        offset: 0x00000000
    },
    {
        name:   "bl2",
        size:   (SZ_64K * 1),                   /* 64k for backup bootloader */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob1",
        size:   (SZ_64K * 82),                  /* 5248k for combined image 1 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob2",
        size:   (SZ_64K * 42),                  /* 2688k for combined image 2 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env1",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env2",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    }
};

static struct mtd_partition pnx0106_partitions_2mb[] =
{
    {
        name:   "bl1",
        size:   (SZ_64K * 1),                   /* 64k for first bootloader */
        offset: 0x00000000
    },
    {
        name:   "bl2",
        size:   (SZ_64K * 1),                   /* 64k for backup bootloader */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob1",
        size:   (SZ_64K * 28),                  /* 1792k for combined image 1 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env1",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env2",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    }
};

#elif defined (CONFIG_PNX0106_WMA100)   || \
      defined (CONFIG_PNX0106_GH526)

//    defined (CONFIG_PNX0106_WSA)      || \

static struct mtd_partition pnx0106_partitions[] =
{
    {
        name:   "bl1",
        size:   (SZ_64K * 1),                   /* 64k for first bootloader */
        offset: 0x00000000
    },
    {
        name:   "bl2",
        size:   (SZ_64K * 1),                   /* 64k for backup bootloader */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob1",
        size:   (SZ_64K * 30),                  /* 1920k for combined image 1 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env1",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env2",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob2",
        size:   (SZ_64K * 30),                  /* 1920k for combined image 2 */
        offset: MTDPART_OFS_APPEND
    }
};

#elif defined (CONFIG_PNX0106_LAGUNA_REVA)  || \
      defined (CONFIG_PNX0106_HASLI7)       || \
      defined (CONFIG_PNX0106_MCIH)       || \
      defined (CONFIG_PNX0106_WSA)

static struct mtd_partition pnx0106_partitions[] =
{
    {
        name:   "bl1",
        size:   (SZ_64K * 1),                   /* 64k for first bootloader */
        offset: 0x00000000
    },
    {
        name:   "bl2",
        size:   (SZ_64K * 1),                   /* 64k for backup bootloader */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "blob1",
        size:   (SZ_64K * 28),                  /* 1792k for combined image 1 */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env1",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    },
    {
        name:   "env2",
        size:   (SZ_64K * 1),                   /* 64k for system environment settings, config, etc */
        offset: MTDPART_OFS_APPEND
    }
};

#else
#error "Platform is not supported"
#endif


/*****************************************************************************
*****************************************************************************/

static struct map_info pnx0106_flash_map =
{
    name:      "MPMC nCS0 Flash",
    size:      FLASH_SZ_MAX,                    /* max possible size (sets limits for probing) */
    buswidth:  2,

    virt:      IO_ADDRESS_FLASH_BASE,
    phys:      NO_XIP,
};


static int __init pnx0106_mtd_init (void)
{
    struct mtd_info *flash_mtd;

    simple_map_init (&pnx0106_flash_map);

#if (FLASH_SZ_MAX > SZ_4M)
    /*
       Over-ride default access functions setup by simple_map_init()...
       Note: In theory we should over-ride all access functions, however by
       constraining the possible Flash data bus width, we ensure that the
       other ones will not be called by MTD core...
    */
    pnx0106_flash_map.read16 = pnx0106_read16;
    pnx0106_flash_map.write16 = pnx0106_write16;
    pnx0106_flash_map.copy_from = pnx0106_copy_from;
    pnx0106_flash_map.copy_to = pnx0106_copy_to;
#endif

    flash_mtd = do_map_probe ("cfi_probe", &pnx0106_flash_map);
    if (!flash_mtd)
        return -ENXIO;

//  printk ("pnx0106_mtd_init: size == %d\n", flash_mtd->size);

#if (FLASH_SZ_MAX > SZ_4M)
    if (flash_mtd->size > SZ_4M)
        watchdog_register_pre_reset_callback (pre_reset_callback);
    else {
        pnx0106_select_flash_bank (0);
        simple_map_init (&pnx0106_flash_map);
    }
#endif

#if defined (CONFIG_PNX0106_W6)     || \
    defined (CONFIG_PNX0106_LPAS1)  || \
    defined (CONFIG_PNX0106_LDMA1)  || \
    defined (CONFIG_PNX0106_RSS1)   || \
    defined (CONFIG_PNX0106_RSC1)   || \
    defined (CONFIG_PNX0106_HACLI7) || \
    defined (CONFIG_PNX0106_MCI)

#if (FLASH_SZ_MAX == SZ_16M)
    if (flash_mtd->size == SZ_16M)
        add_mtd_partitions (flash_mtd, pnx0106_partitions_16mb, ARRAY_SIZE(pnx0106_partitions_16mb));
    else
#endif
    if (flash_mtd->size == SZ_8M)
        add_mtd_partitions (flash_mtd, pnx0106_partitions_8mb, ARRAY_SIZE(pnx0106_partitions_8mb));
    else
        add_mtd_partitions (flash_mtd, pnx0106_partitions_2mb, ARRAY_SIZE(pnx0106_partitions_2mb));
#else
    add_mtd_partitions (flash_mtd, pnx0106_partitions, ARRAY_SIZE(pnx0106_partitions));
#endif

    return 0;
}

static void __exit pnx0106_mtd_cleanup (void)
{
    /* unloading not supported if compiled as a module... */
}

module_init(pnx0106_mtd_init);
module_exit(pnx0106_mtd_cleanup);

MODULE_DESCRIPTION("MTD map driver for Philips PNX0106 development boards");
MODULE_AUTHOR("Andre McCurdy");
MODULE_LICENSE("GPL");

