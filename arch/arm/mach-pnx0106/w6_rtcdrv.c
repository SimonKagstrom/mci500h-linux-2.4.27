/*
   rtcdrv.c - RTC device driver implementation code,WAC7K,for Philips PNX0106
   Copyright (C) 2005-2006 PDCC, Philips CE.
   Revision History :
   ----------------
   Version               DATE                        NAME     NOTES
   Initial version   2006-05-19                 Relena   new ceated
   modify             2006-05-22                 Relena

   This source code is free software ; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 */

/*system header files*/
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/wrapper.h>

#include <asm/arch/gpio.h>

/*user header files*/
#include  <asm/arch/w6_rtcdrv.h>


void  *syscreg_virt_base = NULL;//the virtual address mapped to the physical address of SYSCREG
void  *rtcreg_virt_base = NULL;//the virtual address mapped to the physical address of RTC registers region


#define SYSCREGS_VIRT_BASE		syscreg_virt_base
#define RTCREGS_VIRT_BASE		rtcreg_virt_base

/* function declarations*/
static void		InitRtcHardware(void);
static int 			rtcdev_ioctl(struct inode *,struct file *,unsigned int ,unsigned long ) ;
static int __init 	rtc_module_init( void ) ;
static void __exit 	rtc_module_cleanup(void) ;

struct file_operations Fopsrtc = {
	.ioctl		= rtcdev_ioctl,
	} ;
/*
	This function is called whenever a process tries to
  	do an ioctl on our device file.
*/
static int rtcdev_ioctl ( struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	unsigned int rtc_current_time;
	unsigned int rtc_start_flag;

	/* Switch according to the ioctl called number */
	switch (ioctl_num)
	{
		case  IOCTRL_SET_CURRENT_TIME:
			/*
				bit define:
		  		31-24:day of week; 23-16:hours; 15-8:minutes; 7-0:seconds
		  	*/
		  	copy_from_user((char *)&rtc_current_time,(char *)ioctl_param,4);
			RTCREG_SECONDS = rtc_current_time & 0xff;
			RTCREG_MINUTES = (rtc_current_time>>8) & 0xff;
			RTCREG_HOURS = (rtc_current_time>>16) & 0xff;
			RTCREG_DAYOFMONTH = 1;
			RTCREG_DAYOFWEEK = 0;
			RTCREG_DAYOFYEAR = 1;
			RTCREG_MONTHS =1;
			RTCREG_YEARS = 0;
			break;

		case  IOCTRL_GET_CURRENT_TIME:
			/*
				bit define:
				31-24:day of week; 23-16:hours; 15-8:minutes; 7-0:seconds
			*/
			rtc_current_time = RTCREG_CTIME0;
			copy_to_user((char *)ioctl_param,(char *)&rtc_current_time,4);
			break;

		case  IOCTRL_SET_START_FLAG:
		    copy_from_user((char *)&rtc_start_flag,(char *)ioctl_param,4);
		    RTCREG_SEC_ALARM = rtc_start_flag;
			break;

		case  IOCTRL_GET_START_FLAG:
			rtc_start_flag = RTCREG_SEC_ALARM;
			copy_to_user((char *)ioctl_param,(char *)&rtc_start_flag,4);
			break;

		default:
			break ;
   	}
	return 0 ;
}


static void  InitRtcHardware(void)
{
	//init configuration register
	SYSCREG_SSA1_RTC_CFG = 0x1;
	RTCREG_CLK = 0x1;
	RTCREG_INT_CLEAR = 0x3;

	//mask interrupt register
	RTCREG_INC_INT = 0x0;
	RTCREG_ALARM_MASK = 0xffff;
}

/* Initialize the module - Register the character device */
static int __init rtc_module_init( void )
{
	int ret_val ;

	printk("\n init rtc...");

	/* Prepare our memory-maps */
	if ( !request_mem_region( SYSCREG_REGS_BASE, SYSCREG_REGION_LEN, "PNX0106 SYSCREG registers") )
	{
		printk("\nFATAL ERROR !  failed to request the memory region for SYSCREG registers ! \n");
		return 1;
	}
	/* Map the physical address to a virtual address */
	syscreg_virt_base = (void *)ioremap_nocache( SYSCREG_REGS_BASE, SYSCREG_REGION_LEN);
	if ( !syscreg_virt_base )
	{
		printk( "FATAL ERROR! PNX0106: Unable to map the memory region for SYSCREG.\n" ) ;
        	return 1;
	}

	if ( !request_mem_region( RTC_REGS_BASE, RTCREG_REGION_LEN, "PNX0106 RTC registers") )
	{
		printk("\nFATAL EERROR !  failed to request the memory region for RTC registers ! \n" );
		return 1;
	}

	rtcreg_virt_base = (void *)ioremap_nocache( RTC_REGS_BASE,RTCREG_REGION_LEN);
	if ( !rtcreg_virt_base )
	{
		printk( "FATAL ERROR! PNX0106: Unable to map the memory region for RTCREG.\n" ) ;
        	return 1;
	}

	InitRtcHardware() ;

	/* Register the character device (atleast try) */
	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fopsrtc) ;

 	/* Negative values signify an error */
 	if ( ret_val < 0 )
 	{
 		printk ("FATAL ERROR! register_chrdev failed,errval is %d\n",ret_val) ;
 		return 1;
 	}
	printk("success\n");
	return 0 ;
}

/* Cleanup - unregister the RTC device module */
static void __exit rtc_module_cleanup(void)
{
	int ret;

	/* Unregister the device  */
	ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
	/* If there's an error, report it */
	if (ret < 0)
	{
		printk("FATAL ERROR! unregister_chrdev: %d\n", ret);
	}
	//ummap the sysc registers region mapping
	iounmap(syscreg_virt_base);
	//release the sysc registers region
	release_mem_region(SYSCREG_REGS_BASE, SYSCREG_REGION_LEN) ;

	//ummap the rtc registers region mapping
	iounmap(rtcreg_virt_base);
	//release the rtc registers region
	release_mem_region( RTC_REGS_BASE, RTCREG_REGION_LEN) ;

	printk("\n Success remove RTC module.\n");
  	return;

}

module_init(rtc_module_init) ;
module_exit(rtc_module_cleanup) ;

MODULE_LICENSE("GPL") ;
MODULE_AUTHOR("relena") ;
MODULE_DESCRIPTION("RTC char driver in PNX0106") ;

