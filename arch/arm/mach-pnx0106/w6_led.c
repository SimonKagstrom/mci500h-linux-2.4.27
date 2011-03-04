/*
   led.c - power device driver implementation code,for Philips PNX0106
   Copyright (C) 2005-2006 PDCC, Philips CE.
   Revision History :
   ----------------
   Version               DATE                           NAME     NOTES
   Initial version        12-Mar-2006		Alex Xu		Initial Version
                           2006-07-03 3:24PM        wxl                     add irq process for rds

   This source code is free software ; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 */
/*system header files*/
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/semaphore.h>
#include <linux/tqueue.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/errno.h> 	/* for -EBUSY */
#include <linux/ioport.h>	/* for verify_area */
#include <linux/init.h>		/* for module_init */
#include <asm/uaccess.h>  	/* for get_user and put_user */
#include <asm/arch/hardware.h>
#include <asm/arch/adc.h>
#include <asm/arch/gpio.h>
#include <linux/delay.h>
#include <asm/mach/irq.h>
#include <asm/arch/event_router.h>

#define MAJOR_NUM 56
#define MINOR_NUM 0			//how about the same major number, different minor number
#define DEVICE_NAME "led"

#define ECOLED_SET_VALUE		0
#define ECOLED_GET_VALUE		1
#define IOCTL_FAN_CTL			2
#define IOCTL_MISC_COUNTRY_DETECT	3
#define GPIO_SET_AS_INPUT		0x100
#define GPIO_SET_AS_OUTPUT		0x101
#define GPIO_SET_AS_FUNCTION		0x102
#define GPIO_GET_VALUE			0x103
#define GPIO_SET_HIGH			0x104
#define GPIO_SET_LOW			0x105
#define IOCTL_MISC_GET_NTC		0x106
#define IOCTL_MISC_WAKEUP_THREAD	0x110

#define IOCTRL_INIT_RDS_TIMER		0x306
#define IOCTRL_READ_RDS_MSG_BLOCK	0x307

#define ADC_CHANNEL_2			2
#define ADC_CHANNEL_3			3

/* function declarations*/
static void check_rds_davnpin(unsigned long unused);

//global state variable
static DECLARE_WAIT_QUEUE_HEAD(read_rds_msg_wq) ;
struct timer_list      timer_check_rds; //check rds davn pin timer

int ecoStandbyValue=0;	//current eco-standby status
int fan_ctl_mode=0;

#define RECHECK_RDS_INTERVAL	(jiffies + 1)

static void check_rds_davnpin(unsigned long unused)
{
	gpio_set_as_input (GPIO_RDS_INTRQ);

	if (gpio_get_value (GPIO_RDS_INTRQ) == 0)	// check rds data available status pin
		wake_up_interruptible (&read_rds_msg_wq);

	timer_check_rds.expires = RECHECK_RDS_INTERVAL;
	add_timer (&timer_check_rds);
}

static void init_rds_check_timer(void)
{
	static rds_check_timer_init_flag = 0;

	if (rds_check_timer_init_flag) {
		printk ("\n rds check timer already inited");
		return;
	}
	rds_check_timer_init_flag = 1;

	init_timer (&timer_check_rds);
	timer_check_rds.function = check_rds_davnpin;
	timer_check_rds.data = 0;
	timer_check_rds.expires = RECHECK_RDS_INTERVAL;
	add_timer (&timer_check_rds);
	printk ("\n rds check timer inited.");

	return 0;
}

#if 0
#define DBG_GPIO_FUNCTION(pin, value, action)	\
	printk ("gpio: %s, %s%s\n",		\
		gpio_pin_to_padname ((pin)),	\
		action, 			\
		((value) == -1) ? "" :		\
		((value) ==  0) ? ": 0" :	\
		((value) ==  1) ? ": 1" : ": X")
#else
#define DBG_GPIO_FUNCTION(pin, value, action) do { } while (0)
#endif

static int leddev_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int cmd_buf[10];
	switch (cmd)
	{
		case GPIO_GET_VALUE:
			if (copy_from_user (cmd_buf, (void *) arg, 4))
				return -EFAULT;
			cmd_buf[1] = gpio_get_value (cmd_buf[0]);
			DBG_GPIO_FUNCTION (cmd_buf[0], cmd_buf[1], "gpio_get_value");
			if (copy_to_user ((unsigned *) arg, &cmd_buf[1], 4))
				return -EFAULT;
			break;

		case GPIO_SET_HIGH:
			if (copy_from_user (cmd_buf, (void *) arg, 4))
				return -EFAULT;
			DBG_GPIO_FUNCTION (cmd_buf[0], -1, "gpio_set_high");
			gpio_set_high (cmd_buf[0]);
			break;

		case GPIO_SET_LOW:
			if (copy_from_user (cmd_buf, (void *) arg, 4))
				return -EFAULT;
			DBG_GPIO_FUNCTION (cmd_buf[0], -1, "gpio_set_low");
			gpio_set_low (cmd_buf[0]);
			break;

		case GPIO_SET_AS_INPUT:
			if (copy_from_user (cmd_buf, (void *) arg, 4))
				return -EFAULT;
			DBG_GPIO_FUNCTION (cmd_buf[0], -1, "gpio_set_as_input");
			gpio_set_as_input (cmd_buf[0]);
			break;

		case GPIO_SET_AS_OUTPUT:
			if (copy_from_user (cmd_buf, (void *) arg, 4*2))
				return -EFAULT;
			DBG_GPIO_FUNCTION (cmd_buf[0], cmd_buf[1], "gpio_set_as_output");
			gpio_set_as_output (cmd_buf[0], cmd_buf[1]);
			break;

		case GPIO_SET_AS_FUNCTION:
			if (copy_from_user (cmd_buf, (void *) arg, 4))
				return -EFAULT;
			DBG_GPIO_FUNCTION (cmd_buf[0], -1, "gpio_set_as_function");
			gpio_set_as_function (cmd_buf[0]);
			break;

		case ECOLED_SET_VALUE:
			if (copy_from_user (&ecoStandbyValue, (void *) arg, sizeof(ecoStandbyValue)))
				return -EFAULT;
			gpio_set_as_output (GPIO_SPI1_SCK, ecoStandbyValue);
			break;

		case ECOLED_GET_VALUE:
			if (copy_to_user ((unsigned *) arg, &ecoStandbyValue, sizeof(ecoStandbyValue)))
				return -EFAULT;
			break;

		case IOCTL_FAN_CTL:
#if defined (CONFIG_PNX0106_W6)
			if (copy_from_user (&fan_ctl_mode, (void *) arg, sizeof(fan_ctl_mode)))
				return -EFAULT;
			gpio_set_as_output (GPIO_IEEE1394_D4, (fan_ctl_mode == 0) ? 0 : 1);	// 0 == fan off
#else
			/* GPIO_IEEE1394_D4 is not the correct GPIO on non-w6 platforms... */
			printk ("IOCTL_FAN_CTL: hardcoded gpio pin definition supports W6 platforms only...\n");
			return -ENODEV;
#endif
			break;

		case IOCTL_MISC_COUNTRY_DETECT:
		{
			unsigned int w_country_code;
			unsigned int w_ad_value;

			adcdrv_set_mode (1);					// may be initialized twice, anyway should be ok
			gpio_set_as_input (GPIO_V2);				// B8--Version_2
			gpio_set_as_input (GPIO_V3);				// L16--Version_3
			w_country_code  = (gpio_get_value (GPIO_V2)) << 1;
			w_country_code |= (gpio_get_value (GPIO_V3)) << 0;
			w_ad_value = adc_read (ADC_CHANNEL_3);
			w_country_code |= (w_ad_value << 16);
			if (copy_to_user ((unsigned *) arg, &w_country_code, sizeof(w_country_code)))
				return -EFAULT;
			printk ("IOCTL_MISC_COUNTRY_DETECT called\n");
			break;
		}

		case IOCTRL_INIT_RDS_TIMER:
			init_rds_check_timer();
			break;

		case  IOCTRL_READ_RDS_MSG_BLOCK:
			//printk("\n rds msg read blocked ");
			interruptible_sleep_on (&read_rds_msg_wq);
			break;

		case IOCTL_MISC_GET_NTC:
		{
			int ntc_val;

			ntc_val = adc_get_value (ADC_CHANNEL_2);	// ADC10B_GPA2, pin U4
			if (copy_to_user ((int *) arg, &ntc_val, 4))
				return -EFAULT;
			break;
		}

		case IOCTL_MISC_WAKEUP_THREAD:
		{
			int pid;
			struct task_struct *p;
			if (copy_from_user (&pid, (void *) arg, 4))
				return -EFAULT;
			printk ("Try to wake up pid:%d\n", pid);
			if ((pid > 0) && (p = find_task_by_pid (pid)))
				wake_up_process (p);
			break;
		}

		default:
			return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_PROC_FS
static int led_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	return sprintf (buf, "ecoStandbyValue status=%d\n", ecoStandbyValue);
}
#endif

struct file_operations Fops =
{
	.ioctl	= leddev_ioctl,
};

/* Initialize the module - Register the character device */
static int __init led_module_init( void )
{
	int ret_val;

	adcdrv_set_mode (1);	// may be initialized twice, anyway should be ok

	/* Register the character device (atleast try) */
	ret_val = register_chrdev (MAJOR_NUM, DEVICE_NAME, &Fops);

	if (ret_val < 0)
	{
		printk ("%s failed with %d\n", "Sorry, registering the character device ", ret_val);
		return ret_val;
	}

#ifdef CONFIG_PROC_FS
	if (! create_proc_read_entry ("led", 0, NULL, led_proc_read, NULL))
		return -ENOMEM;
#endif
	printk ("led driver(v0.51) init done\n");
	return 0;
}

__initcall (led_module_init);

