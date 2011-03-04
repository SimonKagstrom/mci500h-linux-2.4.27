/*
 *  linux/include/asm-arm/arch-pnx0106/platform_info.h
 *
 *  Copyright (C) 2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_PLATFORM_INFO_H
#define __ASM_ARCH_PLATFORM_INFO_H

#define PLATFORM_INFO_DEVICEID_UNKNOWN		0
#define PLATFORM_INFO_DEVICEID_N1A_00		1
#define PLATFORM_INFO_DEVICEID_N1B_00		2
#define PLATFORM_INFO_DEVICEID_N1C_02		3
#define PLATFORM_INFO_DEVICEID_N1C_03		4
#define PLATFORM_INFO_DEVICEID_N1C_06		5
#define PLATFORM_INFO_DEVICEID_N1C_07		6
#define PLATFORM_INFO_DEVICEID_N1C_08		7

extern unsigned int platform_info_get_deviceid (void);
extern char *platform_info_get_deviceid_string (void);

#define PLATFORM_INFO_BOOTMODE_UNKNOWN			0
#define PLATFORM_INFO_BOOTMODE_SELFREFRESH_SDRAM	1
#define PLATFORM_INFO_BOOTMODE_NOR			2
#define PLATFORM_INFO_BOOTMODE_SPINOR 			3
#define PLATFORM_INFO_BOOTMODE_NAND			4
#define PLATFORM_INFO_BOOTMODE_UART			5
#define PLATFORM_INFO_BOOTMODE_ATA			6
#define PLATFORM_INFO_BOOTMODE_DFU			7

extern unsigned int platform_info_get_bootmode (void);

extern char *platform_info_get_name (void);
extern unsigned int platform_info_get_config_adc_value (void);
extern unsigned int platform_info_get_eco_standby_channel (void);
extern unsigned int platform_info_default_audio_attenuation_db (void);

#endif
