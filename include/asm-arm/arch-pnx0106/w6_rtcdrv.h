/*
   rtcdrv.h - header file for rtcdrv.c

   Copyright (C) 2005-2006 PDCC, Philips CE.
   Revision History :
   ----------------
   Version               DATE                           NAME     NOTES
   Initial version        2006-05-19 	       relena         new ceated


   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 */

#ifndef RTC_DRV_H
#define RTC_DRV_H

#include <linux/ioctl.h>

/* The major device number */
#define MAJOR_NUM 				160
#define DEVICE_NAME 	  		"rtcdev"
#define DEVICE_FILE_NAME   		"rtcdev"

#define SYSCREG_REGION_LEN   	0x400
#define RTCREG_REGION_LEN     	0x400

#define SYSCREG_SSA1_RTC_CFG	(*((unsigned int *)(SYSCREGS_VIRT_BASE + 0x00c)))

#define RTCREG_INT_LOC			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x000)))
#define RTCREG_INT_CLEAR   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x000)))
#define RTCREG_PRESCALE   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x004)))
#define RTCREG_CLK   			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x008)))
#define RTCREG_INC_INT  			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x00c)))
#define RTCREG_ALARM_MASK   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x010)))
#define RTCREG_CTIME0   			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x014)))
#define RTCREG_CTIME1   			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x018)))
#define RTCREG_CTIME2   			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x01c)))

#define RTCREG_SECONDS   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x020)))
#define RTCREG_MINUTES   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x024)))
#define RTCREG_HOURS  			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x028)))
#define RTCREG_DAYOFMONTH   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x02c)))
#define RTCREG_DAYOFWEEK   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x030)))
#define RTCREG_DAYOFYEAR   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x034)))
#define RTCREG_MONTHS   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x038)))
#define RTCREG_YEARS   			(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x03c)))

#define RTCREG_SEC_ALARM   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x060)))
#define RTCREG_MIN_ALARM   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x064)))
#define RTCREG_HRS_ALARM   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x068)))
#define RTCREG_DOM_ALARM   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x06c)))
#define RTCREG_DOW_ALARM   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x070)))
#define RTCREG_DOY_ALARM   		(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x074)))
#define RTCREG_MONTHS_ALARM   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x078)))
#define RTCREG_YEARS_ALARM   	(*((unsigned int *)(RTCREGS_VIRT_BASE + 0x07c)))

//marco for ioctrl number
#define IOCTRL_SET_CURRENT_TIME		1
#define IOCTRL_GET_CURRENT_TIME		2
#define IOCTRL_SET_START_FLAG		3
#define IOCTRL_GET_START_FLAG		4
#endif

