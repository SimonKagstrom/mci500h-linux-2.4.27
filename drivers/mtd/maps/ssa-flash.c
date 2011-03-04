/*
 *  linux/drivers/mtd/maps/ssa-flash.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *  Copyright (C) 2004 Ranjit Deshpande
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/sizes.h>

#define SSA_FLASH_SECTOR_SIZE (64 * 1024)


#if defined (CONFIG_SSA_HAS7750) || \
    defined (CONFIG_SSA_HAS7752) || \
    defined (CONFIG_SSA_TAHOE)   || \
    defined (CONFIG_SSA_WESSLI)  || \
    defined (CONFIG_SSA_W3)

#if defined (CONFIG_SSA_HAS7750) || \
    defined (CONFIG_SSA_HAS7752) || \
    defined (CONFIG_SSA_TAHOE)

#define FLASH_SZ_MAX    SZ_4M

static int ssa_last_bank;

static inline unsigned long ssa_toggle_flash_bank (unsigned long addr)
{
   int bank = addr < SZ_2M ? 0 : 1;

   if (bank != ssa_last_bank) {
      ssa_last_bank = bank;
      gpio_set_as_output (GPIO_FLASH_BANK_SELECT, bank);
   }

   return addr & (SZ_2M - 1);
}

#else

#define FLASH_SZ_MAX    SZ_2M

static inline unsigned long ssa_toggle_flash_bank (unsigned long addr)
{
   return addr;
}

#endif


/* We need to provide these functions to allow access to the 2 flash
 * banks by toggling the GPIO bit.
 */

static u16 ssa_read16 (struct map_info *map, unsigned long ofs)
{
	ofs = ssa_toggle_flash_bank (ofs);
	return readw (IO_ADDRESS_FLASH_BASE + ofs);
}

static void ssa_copy_from (struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	from = ssa_toggle_flash_bank (from);
	memcpy (to, (void *)(IO_ADDRESS_FLASH_BASE + from), len);
}

static void ssa_write16 (struct map_info *map, __u16 d, unsigned long adr)
{
	adr = ssa_toggle_flash_bank (adr);
	writew (d, (IO_ADDRESS_FLASH_BASE + adr));
}

static void ssa_copy_to (struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	to = ssa_toggle_flash_bank (to);
	memcpy ((void *)(IO_ADDRESS_FLASH_BASE + to), from, len);
}

#else

    /* fallthrough for original siig development boards... */

#define FLASH_SZ_MAX    SZ_2M

#endif



static struct map_info ssa_flash_map =
{
   name:      "EBI Flash",
   size:      FLASH_SZ_MAX, 	/* max possible size (sets limits for probing) */
   buswidth:  2,

#if defined (CONFIG_SSA_HAS7750) || \
    defined (CONFIG_SSA_HAS7752) || \
    defined (CONFIG_SSA_TAHOE)   || \
    defined (CONFIG_SSA_WESSLI)  || \
    defined (CONFIG_SSA_W3)

   read16:    ssa_read16,
   write16:   ssa_write16,
   copy_from: ssa_copy_from,
   copy_to:   ssa_copy_to,

#endif

   virt:      IO_ADDRESS_FLASH_BASE,
   phys:      NO_XIP,
};


#if (defined (CONFIG_SSA_HAS7750) || defined (CONFIG_SSA_HAS7752)) && 1

static struct mtd_partition ssa_partitions_2mb[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "zdfw",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for zd1201 firmware image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "jbc",
      size:   (SSA_FLASH_SECTOR_SIZE * 3),      /* 192k for EPLD config (jbc format) */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "kernel",
      size:   (SSA_FLASH_SECTOR_SIZE * 10),     /* 640k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "cramfs",
      size:   (SSA_FLASH_SECTOR_SIZE * 16),     /* 1024k for compressed initrd (or other OS) image */
      offset: MTDPART_OFS_APPEND
   }
};

static struct mtd_partition ssa_partitions_4mb[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "zdfw1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for zd1201 firmware image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "jbc1",
      size:   (SSA_FLASH_SECTOR_SIZE * 3),      /* 192k for EPLD config (jbc format) */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "kernel1",
      size:   (SSA_FLASH_SECTOR_SIZE * 10),     /* 640k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "cramfs1",
      size:   (SSA_FLASH_SECTOR_SIZE * 16),     /* 1024k for compressed initrd (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "dummy",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "zdfw2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for zd1201 firmware image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "jbc2",
      size:   (SSA_FLASH_SECTOR_SIZE * 3),      /* 192k for EPLD config (jbc format) */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "kernel2",
      size:   (SSA_FLASH_SECTOR_SIZE * 10),     /* 640k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "cramfs2",
      size:   (SSA_FLASH_SECTOR_SIZE * 16),     /* 1024k for compressed initrd (or other OS) image */
      offset: MTDPART_OFS_APPEND
   }
};

#if 1

/* fixme... hardcoded hack will break old boards... Sorry... */

static struct mtd_partition ssa_partitions_4mb_has_lab2[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "blob1",
      size:   (SSA_FLASH_SECTOR_SIZE * 30),     /* 1920k for combined image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "ridsel",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for release ID selection vector */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "blob2",
      size:   (SSA_FLASH_SECTOR_SIZE * 30),     /* 1920k for combined image */
      offset: MTDPART_OFS_APPEND
   }
};

#else

static struct mtd_partition ssa_partitions_4mb_has_lab2[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "rbf1",
      size:   (SSA_FLASH_SECTOR_SIZE * 2),      /* 128k for EPLD config (rbf format) */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "slaveapp1",
      size:   (SSA_FLASH_SECTOR_SIZE * 10),     /* 640k for slave cpu application image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "zImage1",
      size:   (SSA_FLASH_SECTOR_SIZE * 11),     /* 704k for kernel zImage */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "initfs1",
      size:   (SSA_FLASH_SECTOR_SIZE * 7),      /* 448k for cramfs / initrd image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "slaveloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for slave cpu bootloader */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "rbf2",
      size:   (SSA_FLASH_SECTOR_SIZE * 2),      /* 128k for EPLD config (rbf format) */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "slaveapp2",
      size:   (SSA_FLASH_SECTOR_SIZE * 10),     /* 640k for slave cpu application image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "zImage2",
      size:   (SSA_FLASH_SECTOR_SIZE * 11),     /* 704k for kernel zImage */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "initfs2",
      size:   (SSA_FLASH_SECTOR_SIZE * 7),      /* 448k for cramfs / initrd image */
      offset: MTDPART_OFS_APPEND
   }
};

#endif

#elif defined (CONFIG_SSA_WESSLI) || defined (CONFIG_SSA_W3)

static struct mtd_partition ssa_partitions_2mb_w3[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "jbc",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for CPLD config */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "app1",
      size:   (SSA_FLASH_SECTOR_SIZE * 14),     /* 896k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "app2",
      size:   (SSA_FLASH_SECTOR_SIZE * 14),     /* 896k for cramfs (or other OS) image */
      offset: MTDPART_OFS_APPEND
   }
};

#elif defined (CONFIG_SSA_TAHOE)

static struct mtd_partition ssa_partitions_4mb_tahoe[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env2",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "jbc",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for CPLD config */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "app1",
      size:   (SSA_FLASH_SECTOR_SIZE * 14),     /* 896k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "app2",
      size:   (SSA_FLASH_SECTOR_SIZE * 14),     /* 896k for cramfs (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },

   {
      name:   "blob1",
      size:   (SSA_FLASH_SECTOR_SIZE * 31),     /* 1984k for combined image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "ridsel",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for release ID selection vector */
      offset: MTDPART_OFS_APPEND
   }
};

#else

static struct mtd_partition ssa_partitions[] =
{
   {
      name:   "bloader",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for bootloader */
      offset: 0x00000000
   },
   {
      name:   "zdfw",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for zd1201 firmware image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "env1",
      size:   (SSA_FLASH_SECTOR_SIZE * 1),      /* 64k for system environment settings, config, etc */
      offset: MTDPART_OFS_APPEND
   },
#if 1
   {
      name:   "appboth",
      size:   (SSA_FLASH_SECTOR_SIZE * 13),     /* 832k for compressed kernel (or other OS) image (all remaining sectors combined) */
      offset: MTDPART_OFS_APPEND
   },
#else
   {
      name:   "app1",
      size:   (SSA_FLASH_SECTOR_SIZE * 9),      /* 576k for compressed kernel (or other OS) image */
      offset: MTDPART_OFS_APPEND
   },
   {
      name:   "app2",
      size:   (SSA_FLASH_SECTOR_SIZE * 4),      /* 256k for compressed initrd (or other OS) image */
      offset: MTDPART_OFS_APPEND
   }

#endif
};

#endif


static int __init ssa_mtd_init (void)
{
   struct mtd_info *flash_mtd;

#if (defined (CONFIG_SSA_HAS7750) || defined (CONFIG_SSA_HAS7752)) 

   int platform_id = RELEASE_ID_PLATFORM(release_id);

   gpio_set_as_output (GPIO_FLASH_BANK_SELECT, 0);

   flash_mtd = do_map_probe ("cfi_probe", &ssa_flash_map);
   if (!flash_mtd)
      return -ENXIO;

   switch (platform_id)
   {
      case PLATFORM_SDB_REVC:
      case PLATFORM_HAS_LAB1:
         add_mtd_partitions (flash_mtd, ssa_partitions_4mb, ARRAY_SIZE(ssa_partitions_4mb));
         break;

      case PLATFORM_HAS_LAB2:
         add_mtd_partitions (flash_mtd, ssa_partitions_4mb_has_lab2, ARRAY_SIZE(ssa_partitions_4mb_has_lab2));
         break;

      case PLATFORM_SDB_REVA:
      default:
         add_mtd_partitions (flash_mtd, ssa_partitions_2mb, ARRAY_SIZE(ssa_partitions_2mb));
   }

#elif defined (CONFIG_SSA_WESSLI)

   flash_mtd = do_map_probe ("cfi_probe", &ssa_flash_map);
   if (!flash_mtd)
      return -ENXIO;

   add_mtd_partitions (flash_mtd, ssa_partitions_2mb_w3, ARRAY_SIZE(ssa_partitions_2mb_w3));

#elif defined (CONFIG_SSA_W3)

   flash_mtd = do_map_probe ("cfi_probe", &ssa_flash_map);
   if (!flash_mtd)
      return -ENXIO;

   add_mtd_partitions (flash_mtd, ssa_partitions_2mb_w3, ARRAY_SIZE(ssa_partitions_2mb_w3));

#elif defined (CONFIG_SSA_TAHOE)

   flash_mtd = do_map_probe ("cfi_probe", &ssa_flash_map);
   if (!flash_mtd)
      return -ENXIO;

   add_mtd_partitions (flash_mtd, ssa_partitions_4mb_tahoe, ARRAY_SIZE(ssa_partitions_4mb_tahoe));

#else

   simple_map_init(&ssa_flash_map);

   flash_mtd = do_map_probe ("jedec_probe", &ssa_flash_map);
   if (!flash_mtd)
      return -ENXIO;

   add_mtd_partitions (flash_mtd, ssa_partitions, ARRAY_SIZE(ssa_partitions));

#endif

   return 0;
}

static void __exit ssa_mtd_cleanup (void)
{
   /* unloading not supported if compiled as a module... */
}

module_init(ssa_mtd_init);
module_exit(ssa_mtd_cleanup);

MODULE_DESCRIPTION("MTD map driver for Philips Semiconductors SAA7752 SIIG demo boards");
MODULE_AUTHOR("Andre McCurdy");
MODULE_LICENSE("GPL");

