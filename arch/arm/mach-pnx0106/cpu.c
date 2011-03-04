/*
 *  linux/arch/arm/mach-pnx0106/cpu.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
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
unsigned int ffasthz = FFAST_HZ;
unsigned int cpuclkhz;
unsigned int hclkhz;
unsigned int uartclkhz;
unsigned int bootstart_tsc;

unsigned char eth0mac[6];               /* used by the cs8900 (cirrus.c) and ax88796 ethernet drivers */

static int __init parse_tag_ffasthz (const struct tag *tag)
{
	ffasthz = tag->u.ffasthz.hz;
	printk ("FFAST: %d.%03d MHz\n",
		(ffasthz / 1000000),
		(((ffasthz + 500) / 1000) % 1000));
	return 0;
}

__tagtable(ATAG_FFASTHZ, parse_tag_ffasthz);

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


#if 0
static int __init parse_tag_cpuclkmhz (const struct tag *tag)
{
	cpuclkmhz = tag->u.cpuclkmhz.mhz;
	return 0;
}

__tagtable(ATAG_CPUCLKMHZ, parse_tag_cpuclkmhz);
#endif

#if 0
static int __init parse_tag_hclkmhz (const struct tag *tag)
{
	hclkmhz = tag->u.hclkmhz.mhz;
	return 0;
}

__tagtable(ATAG_HCLKMHZ, parse_tag_hclkmhz);
#endif

#if 0
static int __init parse_tag_release_id (const struct tag *tag)
{
	return 0;
}

__tagtable(ATAG_RELEASE_ID, parse_tag_release_id);
#endif

static int __init parse_tag_eth0mac (const struct tag *tag)
{
	memcpy (eth0mac, tag->u.eth0mac.macaddr, 6);
	return 0;
}

__tagtable(ATAG_ETH0MAC, parse_tag_eth0mac);

static int __init parse_tag_bootstart_tsc (const struct tag *tag)
{
	bootstart_tsc = tag->u.bootstart_tsc.value;
	return 0;
}

__tagtable(ATAG_BOOTSTART_TSC, parse_tag_bootstart_tsc);


#if 0

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

static int __init cpu_init (void)
{
#if 0
   pm_power_off = has7752_power_off;                /* register HW power down function (if we have one...) */
#endif

   return 0;
}

__initcall (cpu_init);

#endif


EXPORT_SYMBOL(release_id);
EXPORT_SYMBOL(ffasthz);
EXPORT_SYMBOL(cpuclkhz);
EXPORT_SYMBOL(hclkhz);
EXPORT_SYMBOL(uartclkhz);
EXPORT_SYMBOL(bootstart_tsc);


