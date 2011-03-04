/*
 * linux/arch/arm/mach-ssa/cpu.c
 *
 * Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/string.h>
#include <asm/system.h>
#include <asm/setup.h>

unsigned int release_id;
unsigned int cpuclkhz;
unsigned int hclkhz;
unsigned int uartclkhz;

unsigned int cpuclkmhz;
unsigned int hclkmhz;

unsigned char eth0mac[6];               /* used by the cs8900 (cirrus.c) and ax88796 ethernet drivers */
unsigned long ata_regs_phys;


static int __init parse_tag_release_id (const struct tag *tag)
{
#ifdef CONFIG_SSA_HAS7752

	char *platform_string;

	release_id = tag->u.release_id.value;

	switch (RELEASE_ID_PLATFORM(release_id))
	{
		case PLATFORM_SDB_REVA:
			platform_string = "SIIG Demo Board, Rev A";
			break;
		case PLATFORM_SDB_REVC:
			platform_string = "SIIG Demo Board, Rev C";
			break;
		case PLATFORM_HAS_LAB1:
			platform_string = "HasLi Lab1";
			break;
		case PLATFORM_HAS_LAB2:
			platform_string = "HasLi Lab2";
			break;
		default:
			platform_string = "UNKNOWN PLATFORM";
			break;
	}

	printk ("Bootloader platformID: %s v%d.%d.%d-%d\n",
		platform_string,
		RELEASE_ID_MAJOR(release_id),
		RELEASE_ID_MINOR(release_id),
		RELEASE_ID_POINT(release_id), 
		RELEASE_ID_BUILD(release_id));

#elif defined (CONFIG_SSA_WESSLI)

	char *platform_string;

	release_id = tag->u.release_id.value;

	switch (RELEASE_ID_PLATFORM(release_id))
	{
		default:
			platform_string = "WessLi";
			break;
	}

	printk ("Bootloader platformID: %s v%d.%d.%d-%d\n",
		platform_string,
		RELEASE_ID_MAJOR(release_id),
		RELEASE_ID_MINOR(release_id),
		RELEASE_ID_POINT(release_id), 
		RELEASE_ID_BUILD(release_id));

#elif defined (CONFIG_SSA_W3)

	char *platform_string;

	release_id = tag->u.release_id.value;

	switch (RELEASE_ID_PLATFORM(release_id))
	{
		default:
			platform_string = "WessLi-3 (HacLi) Lab1/Lab2";
			break;
	}

	printk ("Bootloader platformID: %s v%d.%d.%d-%d\n",
		platform_string,
		RELEASE_ID_MAJOR(release_id),
		RELEASE_ID_MINOR(release_id),
		RELEASE_ID_POINT(release_id), 
		RELEASE_ID_BUILD(release_id));

#elif defined (CONFIG_SSA_TAHOE)

	char *platform_string;

	release_id = tag->u.release_id.value;

	switch (RELEASE_ID_PLATFORM(release_id))
	{
		default:
			platform_string = "Tahoe, Rev A";
			break;
	}

	printk ("Bootloader platformID: %s v%d.%d.%d-%d\n",
		platform_string,
		RELEASE_ID_MAJOR(release_id),
		RELEASE_ID_MINOR(release_id),
		RELEASE_ID_POINT(release_id), 
		RELEASE_ID_BUILD(release_id));

#endif
	return 0;
}

__tagtable(ATAG_RELEASE_ID, parse_tag_release_id);


static int __init parse_tag_cpuclkhz (const struct tag *tag)
{
	cpuclkhz = tag->u.cpuclkhz.hz;
	printk ("cpuclk: %d.%03d MHz\n",
        	(cpuclkhz / 1000000),
        	(((cpuclkhz + 500) / 1000) % 1000));
	return 0;
}

__tagtable(ATAG_CPUCLKHZ, parse_tag_cpuclkhz);


static int __init parse_tag_hclkhz (const struct tag *tag)
{
	hclkhz = tag->u.hclkhz.hz;
	printk ("hclk: %d.%03d MHz\n",
		(hclkhz / 1000000),
		(((hclkhz + 500) / 1000) % 1000));
	return 0;
}

__tagtable(ATAG_HCLKHZ, parse_tag_hclkhz);


static int __init parse_tag_uartclkhz (const struct tag *tag)
{
	uartclkhz = tag->u.uartclkhz.hz;
	printk ("uartclk: %d.%03d MHz\n",
		(uartclkhz / 1000000),
		(((uartclkhz + 500) / 1000) % 1000));
	return 0;
}

__tagtable(ATAG_UARTCLKHZ, parse_tag_uartclkhz);


static int __init parse_tag_cpuclkmhz (const struct tag *tag)
{
	cpuclkmhz = tag->u.cpuclkmhz.mhz;
	return 0;
}

__tagtable(ATAG_CPUCLKMHZ, parse_tag_cpuclkmhz);


static int __init parse_tag_hclkmhz (const struct tag *tag)
{
	hclkmhz = tag->u.hclkmhz.mhz;
	return 0;
}

__tagtable(ATAG_HCLKMHZ, parse_tag_hclkmhz);


static int __init parse_tag_ataregs (const struct tag *tag)
{
	ata_regs_phys = tag->u.ataregs.start;
	return 0;
}

__tagtable(ATAG_ATAREGS, parse_tag_ataregs);


static int __init parse_tag_eth0mac (const struct tag *tag)
{
	memcpy (eth0mac, tag->u.eth0mac.macaddr, 6);
	return 0;
}

__tagtable(ATAG_ETH0MAC, parse_tag_eth0mac);


#ifdef CONFIG_SSA_HAS7752
static void has7752_power_off (void)
{
   /*
      Note SW powerdown will only work on boards with an ATX power supply
      (e.g. SIIG SDB RevA and RevC boards).
   */

   writel (EPLD_SYSCONTROL_POWER_DOWN, IO_ADDRESS_SYSCONTROL_BASE);
   mdelay (500);
   printk ("Power down failed -- System halted\n");
   while (1);
}
#endif

static int __init cpu_init (void)
{
#if 0
   printk ("De-asserting ATA nHRST...\n");
   gpio_set_as_output (GPIO_ATA_NHRST, 1);          /* set nHRST gpio pin high to de-assert ATA reset line */
#endif

#ifdef CONFIG_SSA_HAS7752
   pm_power_off = has7752_power_off;                /* register HW power down function (if we have one...) */
#endif

   return 0;
}

__initcall (cpu_init);


EXPORT_SYMBOL(release_id);
EXPORT_SYMBOL(cpuclkhz);
EXPORT_SYMBOL(hclkhz);
EXPORT_SYMBOL(uartclkhz);

EXPORT_SYMBOL(cpuclkmhz);
EXPORT_SYMBOL(hclkmhz);

