/*
 *  linux/arch/arm/mach-pnx0106/platform_info.c
 *
 *  Copyright (C) 2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/version.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/string.h>
#include <asm/system.h>
#include <asm/setup.h>
#include <asm/uaccess.h>

#include <asm/arch/platform_info.h>


#define PLATFORM_NAME_LEN		(32)		/* ie 31 chars maximum + 1 nul */

static char platform_name[PLATFORM_NAME_LEN];
static unsigned int adc_reading;
static unsigned int devid;
static unsigned int bootmode;


unsigned int platform_info_get_deviceid (void)
{
	return devid;
}

char *platform_info_get_deviceid_string (void)
{
	int i;
	static const struct
	{
		unsigned int type;
		char *string;
	}
	devidstring_table[] =
	{
		{ PLATFORM_INFO_DEVICEID_N1A_00, "N1A00" },
		{ PLATFORM_INFO_DEVICEID_N1B_00, "N1B00" },
		{ PLATFORM_INFO_DEVICEID_N1C_02, "N1C02" },
		{ PLATFORM_INFO_DEVICEID_N1C_03, "N1C03" },
		{ PLATFORM_INFO_DEVICEID_N1C_06, "N1C06" },
		{ PLATFORM_INFO_DEVICEID_N1C_07, "N1C07" },
		{ PLATFORM_INFO_DEVICEID_N1C_08, "N1C08" }
	};

	for (i = 0; i < (sizeof(devidstring_table) / sizeof(devidstring_table[0])); i++)
		if (devid == devidstring_table[i].type)
			return devidstring_table[i].string;

	return "UNKNOWN";
}

unsigned int platform_info_get_bootmode (void)
{
	return bootmode;
}

char *platform_info_get_name (void)
{
	return platform_name;
}

unsigned int platform_info_get_config_adc_value (void)
{
	return adc_reading;
}


/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_PNX0106_W6)

static const struct
{
    unsigned short adc_min;
    unsigned short adc_max;
    unsigned char eco_standby_channel;
    unsigned char default_audio_attenuation_db;
    char *name;
}
ptable[] =
{
    { 0x000, 0x03E, 1, 0, "WAC5000" },  /* */
    { 0x3CD, 0x3FF, 0, 3, "WAS5000" },  /* */
    { 0x0A9, 0x0C9, 0, 3, "WAS5000" },  /* */
    { 0x1E6, 0x21A, 0, 3, "WAS5700" },  /* Note: 5700, not 5000 */
    { 0x29D, 0x2E4, 1, 0, "WAC5000" },  /* */
    { 0x12F, 0x14F, 0, 3, "WAS5000" },  /* */

    { 0x248, 0x285, 0, 0, "WAC7000" },  /* */
    { 0x185, 0x1AE, 1, 0, "WAS7000" },  /* */
    { 0x337, 0x356, 1, 0, "WAS7000" },  /* */
};

unsigned int platform_info_get_eco_standby_channel (void)
{
	int i;
	unsigned int adc = platform_info_get_config_adc_value();

	for (i = 0; i < (sizeof(ptable) / sizeof(ptable[0])); i++) {
		if ((adc >= ptable[i].adc_min) && (adc <= ptable[i].adc_max)) {
			return ptable[i].eco_standby_channel;
		}
	}

	printk ("Warning: Unknown platform (ADC: 0x%03x) assuming eco power connected to ADC[0]\n", adc);
	return 0;
}

unsigned int platform_info_default_audio_attenuation_db (void)
{
	int i;
	unsigned int adc = platform_info_get_config_adc_value();

	for (i = 0; i < (sizeof(ptable) / sizeof(ptable[0])); i++) {
		if ((adc >= ptable[i].adc_min) && (adc <= ptable[i].adc_max)) {
			return ptable[i].default_audio_attenuation_db;
		}
	}

	printk ("Warning: Unknown platform (ADC: 0x%03x) assuming default_audio_attenuation_db 0\n", adc);
	return 0;
}

#elif defined (CONFIG_PNX0106_HASLI7) || defined (CONFIG_PNX0106_MCIH) || defined (CONFIG_PNX0106_MCI)

unsigned int platform_info_get_eco_standby_channel (void)
{
	return 1;	/* fixme... */
}

unsigned int platform_info_default_audio_attenuation_db (void)
{
	return 0;
}

#elif defined (CONFIG_PNX0106_HACLI7)

unsigned int platform_info_get_eco_standby_channel (void)
{
	return 1;	/* fixme... */
}

unsigned int platform_info_default_audio_attenuation_db (void)
{
	return 0;
}

#else

unsigned int platform_info_default_audio_attenuation_db (void)
{
	return 0;
}

#endif


/****************************************************************************
****************************************************************************/

#ifdef CONFIG_PROC_FS

static int proc_deviceid_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	char *p = page;

	p += sprintf (p, "%s\n", platform_info_get_deviceid_string());
	if ((len = (p - page) - off) < 0)
		len = 0;
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}

static int proc_platform_name_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	char *p = page;

	p += sprintf (p, "%s\n", platform_name);
	if ((len = (p - page) - off) < 0)
		len = 0;
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}

static int proc_platform_name_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[PLATFORM_NAME_LEN];
	int value;
	int len;

	if (count > 0) {
		len = (count > (sizeof(buf) - 1)) ? (sizeof(buf) - 1) : count;
		memset (buf, 0, sizeof(buf));
		if (copy_from_user (buf, buffer, len))
			return -EFAULT;
		while (len >= 0) {
			if ((buf[len - 1] == '\r') || (buf[len - 1] == '\n')) {
				buf[len - 1] = 0;
				len--;
			}
			else
				break;
		}

		memcpy (platform_name, buf, PLATFORM_NAME_LEN);
	}

	return count;
}

static int __init platform_info_init (void)
{
	struct proc_dir_entry *res;

	if ((res = create_proc_entry ("deviceid", S_IRUGO, NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = proc_deviceid_read;

	if ((res = create_proc_entry ("platform_name", (S_IWUSR | S_IRUGO), NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = proc_platform_name_read;
	res->write_proc = proc_platform_name_write;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__initcall (platform_info_init);	/* 2.4 */
#else
device_initcall (platform_info_init);	/* 2.6 */
#endif

#endif


/****************************************************************************
****************************************************************************/

static int __init parse_tag_platform_info (const struct tag *tag)
{
	memcpy (platform_name, tag->u.platform_info.platform_name, sizeof(platform_name));
	adc_reading = tag->u.platform_info.adc_reading;
	devid = tag->u.platform_info.devid;
	bootmode = tag->u.platform_info.bootmode;

	printk ("PI: %s, %s, ADC: 0x%03x, bootmode 0x%02x\n", platform_name, platform_info_get_deviceid_string(), platform_info_get_config_adc_value(), bootmode);
	return 0;
}

__tagtable(ATAG_PLATFORM_INFO, parse_tag_platform_info);


/****************************************************************************
****************************************************************************/

