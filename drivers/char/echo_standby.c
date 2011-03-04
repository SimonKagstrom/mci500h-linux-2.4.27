// Echo standby mode / power save mode driver

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>

#ifndef ECHO_STANDBY_DEBUG 
#define ECHO_STANDBY_DEBUG 1
#endif

#define NORMAL_MODE 0
#define ECHO_STANDBY_MODE 1

#define ECHO_STANDBY_TURN_ON	1
#define ECHO_STANDBY_TURN_OFF	0

static struct timer_list echo_standby_timer;
#define ECHO_STANDBY_KEY_SCAN_PERIOD	1	// 1 sec

static int echo_standby_current_status = NORMAL_MODE;
static struct proc_dir_entry *proc_entry;

static void echo_standby_turn_off_eth(void)
{
	unsigned long ioaddr = 0;

	/* turn of wired ethernet phy power */
	ioaddr = (unsigned long) ioremap_nocache (AX88796_REGS_BASE, (8 * 1024));
	if( ioaddr == 0) {
		printk(KERN_NOTICE "Access to ethernet PHY registers failed\n");
		return;
	}
	writew(0x0040, (ioaddr + 0xB80) );	
	iounmap(ioaddr);
}

static void echo_standby_turn_on_eth(void)
{
	unsigned long ioaddr = 0;

	/* turn of wired ethernet phy power */
	ioaddr = (unsigned long) ioremap_nocache (AX88796_REGS_BASE, (8 * 1024));
	if( ioaddr == 0) {
		printk(KERN_NOTICE "Access to ethernet PHY registers failed\n");
		return;
	}
	writew(0x0000, (ioaddr + 0xB80) );	
	iounmap(ioaddr);
}

static void echo_standby_turn_off_ata(void)
{
	gpio_set_low(GPIO_ATA_RESET);
	gpio_set_low(GPIO_PCCARDA_NCS0);
	gpio_set_low(GPIO_PCCARDA_NCS1);
	gpio_set_low(GPIO_PCCARDA_INTRQ);
	gpio_set_low(GPIO_PCCARDA_NDMACK);
	gpio_set_low(GPIO_PCCARDA_DMARQ);
	gpio_set_low(GPIO_PCCARDA_IORDY);
	gpio_set_low(GPIO_PCCARD_NDIOW);
	gpio_set_low(GPIO_PCCARD_NDIOR);
	gpio_set_low(GPIO_PCCARD_DD15);
	gpio_set_low(GPIO_PCCARD_DD14);
	gpio_set_low(GPIO_PCCARD_DD13);
	gpio_set_low(GPIO_PCCARD_DD12);
	gpio_set_low(GPIO_PCCARD_DD11);
	gpio_set_low(GPIO_PCCARD_DD10);
	gpio_set_low(GPIO_PCCARD_DD9);
	gpio_set_low(GPIO_PCCARD_DD8);
	gpio_set_low(GPIO_PCCARD_DD7);
	gpio_set_low(GPIO_PCCARD_DD6);
	gpio_set_low(GPIO_PCCARD_DD5);
	gpio_set_low(GPIO_PCCARD_DD4);
	gpio_set_low(GPIO_PCCARD_DD3);
	gpio_set_low(GPIO_PCCARD_DD2);
	gpio_set_low(GPIO_PCCARD_DD1);
	gpio_set_low(GPIO_PCCARD_DD0);
	gpio_set_low(GPIO_PCCARD_A2);
	gpio_set_low(GPIO_PCCARD_A1);
	gpio_set_low(GPIO_PCCARD_A0);
}

static void echo_standby_turn_on_ata(void)
{

}

static void set_echo_standby_mode()
{
	echo_standby_turn_off_ata();
	echo_standby_turn_off_eth();

	/* turn_off_wlan */
	gpio_set_as_output(GPIO_WLAN_PD, 0);

	/* turn off LCD backlight */
	gpio_set_low(GPIO_LCD_BACKLIGHT);

	/* set echo stand by led to red */
	gpio_set_as_input(GPIO_LED_ECHOSTDBY);

	/* assert eco-standby signal to power supply to cut power to ATA and ethernet (should be last thing...) */
	gpio_set_low(GPIO_SPI1_SCK);
}

static void clear_echo_standby_mode()
{
	echo_standby_turn_on_ata();
	echo_standby_turn_on_eth();

	/* turn on wlan power */
	gpio_set_high(GPIO_WLAN_PD);

	/* turn of LCD backlight */
	gpio_set_high(GPIO_LCD_BACKLIGHT);

	/* set echo stand by led to green */
	gpio_set_as_output(GPIO_LED_ECHOSTDBY, 1);

}

static void echo_standby_timer_func(void)
{
	int val0, val1;

	val0 = adc_get_value(0);
	//printk(KERN_NOTICE "echstdby: echo standby key val0 : %d\n", val0);

	val1 = adc_get_value(1);
	//printk(KERN_NOTICE "echstdby: echo standby key val1 : %d\n", val1);

	if(( val0 < 32) || (val1 < 32 )) {
		clear_echo_standby_mode();
		printk(KERN_NOTICE "echstdby: rebooting now....");
		machine_restart();
	} else {
		/* start scanning again*/
		mod_timer(&echo_standby_timer, jiffies + HZ);
	}
}

static int echo_standby_read_proc(char *buf, char **start, off_t offset,
		       int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(buf + len, "Echo Stand by driver for Wacs5k/7k (c) 2006\n\n");
	len += sprintf(buf + len, "Current Mode : %s \n", ((echo_standby_current_status == NORMAL_MODE) ? "NORMAL_MODE" : "ECHO_STANDBY_MODE"));

	*eof = 1;

	return len;
}

ssize_t echo_standby_write_proc( struct file *filp, const char __user *buff,
                        unsigned long len, void *data )
{
	printk(KERN_NOTICE "echstdby: Truning on echo standby \n");
	if( echo_standby_current_status  == ECHO_STANDBY_MODE)
		return 1;

	set_echo_standby_mode();

	/* now start polling for echo stand by key press */
	/* start timer for periodic scanning */
	echo_standby_current_status = ECHO_STANDBY_MODE;
	
	echo_standby_timer.expires = jiffies + HZ;
	add_timer(&echo_standby_timer);


	return 1;
}


int __init echo_standby_init(void)
{
	int ret;

	printk(KERN_NOTICE "Initializing echo stand by driver ...\n");

	proc_entry = create_proc_entry( "echostandby", 0644, NULL );

    	if (proc_entry == NULL) {

      		ret = -ENOMEM;
      		printk(KERN_INFO "echostandby: Couldn't create proc entry\n");

    	} else {
      		proc_entry->read_proc = echo_standby_read_proc;
      		proc_entry->write_proc = echo_standby_write_proc;
      		//proc_entry->owner = THIS_MODULE;
      		printk(KERN_INFO "echostandby: driver loaded.\n");

    	}
	/* initialize echo standby key scanner */
	init_timer(&echo_standby_timer);
	echo_standby_timer.function = echo_standby_timer_func;
	echo_standby_timer.data = 0;

  	gpio_set_as_output(GPIO_LED_ECHOSTDBY, 1);
	echo_standby_current_status = NORMAL_MODE;

	return ret;
}
 
void __exit echo_standby_cleanup(void)
{
#if  ECHO_STANDBY_DEBUG
	printk(KERN_NOTICE "echostandby: cleanup\n");
#endif

	remove_proc_entry( "echostandby", NULL);
	gpio_set_as_output(GPIO_LED_ACTIVITY, 0);
 
}

__initcall(echo_standby_init);
