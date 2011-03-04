/*
 *  linux/arch/arm/mach-pnx0106/eco_standby.c
 *
 *  Copyright (C) 2006 Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/version.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <asm/arch/hardware.h>
#include <asm/arch/adc.h>
#include <asm/arch/clocks.h>
#include <asm/arch/gpio.h>
#include <asm/arch/platform_info.h>
#include <asm/arch/sdma.h>
#include <asm/arch/watchdog.h>

#if defined (CONFIG_SERIAL_PNX010X_CONSOLE)
#if defined (CONFIG_PNX0106_HASLI7) || defined (CONFIG_PNX0106_HACLI7) || (CONFIG_PNX0106_MCIH) || defined (CONFIG_PNX0106_MCI)
extern int pnx010x_console_in_use;
#endif
#endif

extern void usb_set_vbus_off (void);


static inline void eco_standby_led_off (void)
{
	gpio_set_as_output (GPIO_LED_ECOSTANDBY, 0);
}

static inline void eco_standby_led_green (void)
{
	gpio_set_as_output (GPIO_LED_ECOSTANDBY, 1);
}

static inline void eco_standby_led_red (void)
{
	gpio_set_as_input (GPIO_LED_ECOSTANDBY);
}

static void eco_standby_enter (int shutdown_main_power_supply, int autoreboot)
{
	unsigned long flags;
	unsigned long ioaddr_asix;
	unsigned int key_releases_seen;
	unsigned int adc;
	int channel = platform_info_get_eco_standby_channel();
	int i;

	printk ("Entering ECO-Standby mode...%s\n", shutdown_main_power_supply ? "" : " (but leaving main power supply on !!)");

#if defined (CONFIG_USB)
	usb_set_vbus_off();
#endif

	local_irq_save (flags);

#if defined (IRQ_RC6RX)
	disable_irq (IRQ_RC6RX);		/* this on could be an FIQ... */
#endif

#if defined (CONFIG_PNX0106_W6)
	/*
	   Power down ASIX ethernet... (Rev A)
	*/
	ioaddr_asix = (unsigned long) ioremap_nocache (AX88796_REGS_BASE, (8 * 1024));
	if (ioaddr_asix) {
		printk ("ECO-Standby: ASIX Rev A...\n");
		writew (0x0040, (ioaddr_asix + 0xB80));
	}
#elif defined (CONFIG_PNX0106_HASLI7) || \
	  defined (CONFIG_PNX0106_MCIH) || \
	  defined (CONFIG_PNX0106_MCI) || \
      defined (CONFIG_PNX0106_HACLI7)
	/*
	   Power down ASIX ethernet... (Rev B)
	*/
	ioaddr_asix = (unsigned long) ioremap_nocache (AX88796_REGS_BASE, (8 * 1024));
	if (ioaddr_asix) {
		printk ("ECO-Standby: ASIX Rev B...\n");
		writew (0x00E1, (ioaddr_asix + 0x1000));
		writew (0x0012, (ioaddr_asix + 0x1580));
	}
#else
#error "Unexpected platform..."
#endif

#if defined (CONFIG_PNX0106_W6)
	{
		/*
		   Mute the audio... (fixme.. should this be done by the application before begining eco standby ?!?)
		   Drive GPIO_MUTE high to mute (Production boards)
		   Drive GPIO_MUTE_OLD high to mute (TR boards and before)
		*/

		unsigned int previous_state;

		previous_state = gpio_get_value (GPIO_MUTE);
		gpio_set_as_output (GPIO_MUTE, 1);
		printk ("ECO-Standby: Mute Audio (GPIO_MUTE)... %d -> 1\n", previous_state);

		previous_state = gpio_get_value (GPIO_MUTE_OLD);
		gpio_set_as_output (GPIO_MUTE_OLD, 1);
		printk ("ECO-Standby: Mute Audio (GPIO_MUTE_OLD)... %d -> 1\n", previous_state);
	}
#endif

	/*
	   Set all ATA interface signals to low (only if shutting down main power supply !!)
	*/
	if (shutdown_main_power_supply)
	{
#if defined (CONFIG_IDE)
		printk ("ECO-Standby: ATA...\n");

		gpio_set_as_output (GPIO_ATA_RESET, 0);
		gpio_set_as_output (GPIO_PCCARDA_NCS0, 0);
		gpio_set_as_output (GPIO_PCCARDA_NCS1, 0);
		gpio_set_as_output (GPIO_PCCARDA_INTRQ, 0);
		gpio_set_as_output (GPIO_PCCARDA_NDMACK, 0);
		gpio_set_as_output (GPIO_PCCARDA_DMARQ, 0);
		gpio_set_as_output (GPIO_PCCARDA_IORDY, 0);
		gpio_set_as_output (GPIO_PCCARD_NDIOW, 0);
		gpio_set_as_output (GPIO_PCCARD_NDIOR, 0);
		gpio_set_as_output (GPIO_PCCARD_DD15, 0);
		gpio_set_as_output (GPIO_PCCARD_DD14, 0);
		gpio_set_as_output (GPIO_PCCARD_DD13, 0);
		gpio_set_as_output (GPIO_PCCARD_DD12, 0);
		gpio_set_as_output (GPIO_PCCARD_DD11, 0);
		gpio_set_as_output (GPIO_PCCARD_DD10, 0);
		gpio_set_as_output (GPIO_PCCARD_DD9, 0);
		gpio_set_as_output (GPIO_PCCARD_DD8, 0);
		gpio_set_as_output (GPIO_PCCARD_DD7, 0);
		gpio_set_as_output (GPIO_PCCARD_DD6, 0);
		gpio_set_as_output (GPIO_PCCARD_DD5, 0);
		gpio_set_as_output (GPIO_PCCARD_DD4, 0);
		gpio_set_as_output (GPIO_PCCARD_DD3, 0);
		gpio_set_as_output (GPIO_PCCARD_DD2, 0);
		gpio_set_as_output (GPIO_PCCARD_DD1, 0);
		gpio_set_as_output (GPIO_PCCARD_DD0, 0);
		gpio_set_as_output (GPIO_PCCARD_A2, 0);
		gpio_set_as_output (GPIO_PCCARD_A1, 0);
		gpio_set_as_output (GPIO_PCCARD_A0, 0);
#if defined (CONFIG_PNX0106_HACLI7)
#error "Huh ?!?"
#endif
#endif

#if defined (CONFIG_PNX0106_HACLI7)

#if defined (CONFIG_SERIAL_PNX010X_CONSOLE)
#if defined (CONFIG_PNX0106_HASLI7) || defined (CONFIG_PNX0106_HACLI7) || defined (CONFIG_PNX0106_MCIH) || defined (CONFIG_PNX0106_MCI)
		if (pnx010x_console_in_use)
		{
			printk ("ECO-Standby: NOT touching GPIO_UART0_TXD and GPIO_UART0_RXD (console in use)\n");
		}
		else
#endif
#endif
		{
			gpio_set_as_output (GPIO_UART0_TXD, 0);
			gpio_set_as_output (GPIO_UART0_RXD, 0);
		}

		printk ("ECO-Standby: BT module, etc...\n");

		gpio_set_as_output (GPIO_IEEE1394_NRESET, 0);		/* fixme: GPIO_BT_RESET    ?? : add definition is hardware.h !! */

		gpio_set_as_output (GPIO_DAO_WS, 0);
		gpio_set_as_output (GPIO_DAO_BCK, 0);
		gpio_set_as_output (GPIO_DAO_DATA, 0);
		gpio_set_as_output (GPIO_DAO_CLK, 0);

		gpio_set_as_output (GPIO_SPI1_SCK, 0);
		gpio_set_as_output (GPIO_SPI1_MISO, 0);
		gpio_set_as_output (GPIO_SPI1_MOSI, 0);
		gpio_set_as_output (GPIO_SPI1_EXT, 0);
		gpio_set_as_output (GPIO_SPI1_S_N, 0);

		gpio_set_as_output (GPIO_BT_POWER, 0);			/* GPIO_BT_POWER : drive low : disable power to BT module */

		/* misc... fixme: add comments !! */

		gpio_set_as_input (GPIO_IEEE1394_LPS);			/* fixme: GPIO_WLAN_ACTIVE ?? : add definition is hardware.h !! */

		gpio_set_as_output (GPIO_IEEE1394_CTL1, 0);
		gpio_set_as_output (GPIO_IEEE1394_D0, 0);
		gpio_set_as_output (GPIO_GPIO_17, 0);
		gpio_set_as_output (GPIO_UART0_NRTS, 0);
		
		gpio_set_as_output (GPIO_SPDIF_OUT, 1);
#endif
	}

#if 0
	/*
	   Power down WiFi card...
	*/
	printk ("ECO-Standby: WiFi...\n");
	gpio_set_as_output (GPIO_WLAN_POWERDOWN, 0);
#endif

	/*
	   Stop any SDMA bus master activity...
	*/
	printk ("ECO-Standby: SDMA...\n");
	sdma_force_shutdown_all_channels();

	/*
	   Turn off LCD backlight...
	*/
#if defined (CONFIG_PNX0106_ECOSTANDBY_LCD_BACKLIGHT_CONTROL)
#if defined (CONFIG_PNX0106_ECOSTANDBY_LCD_BACKLIGHT_OFF_HIGH)
	printk ("ECO-Standby: LCD Backlight... driving high\n");
	gpio_set_as_output (GPIO_LCD_BACKLIGHT, 1);
#else
	printk ("ECO-Standby: LCD Backlight... driving low\n");
	gpio_set_as_output (GPIO_LCD_BACKLIGHT, 0);
#endif
	/*
	   Put LCD controller into reset (toggle required to prevent line across display...) ??
	*/
	gpio_set_as_output (GPIO_LCD_RESET, 0);
	udelay (1001);
	gpio_set_as_output (GPIO_LCD_RESET, 1);
#endif

	/*
	   Kill the EPICs... Unfortunately, there's no API to do that yet,
	   so just delay a while and hope it runs out of things to do and
	   goes idle on it's own...
	*/
	mdelay (500);	/* clocks not twiddled yet - this delay is accurate */

	/*
	   Twiddle clocks + SDRAM refresh...
	*/
	printk ("ECO-Standby: Clocks...\n");
	clocks_enter_low_power_state();

	mdelay (2);	/* clocks have been twiddled... this will take longer than expected... */

	/*
	   Assert eco-standby signal to power supply...
	   This should be the final step - if power consumption has not been reduced
	   below the required level at this point, we will be rebooted... !!
	*/
	if (shutdown_main_power_supply) {
		gpio_set_as_output (GPIO_ECO_STBY, 0);
#if defined (CONFIG_PNX0106_HACLI7)
		/*
		   Note: GPIO_ECO_CORE has different functions on different platforms...
		         On w6, it controlled power to WiFi card. On hacli7 it
			 controls voltage level of main 3v3 regulator (drive low to
			 reduce 3v3 power supply to 3v1). It's Function on hasli7 is
			 a mystery... (maybe a mixture of the two controlled by
			 stuffing options ?!?).
		*/
		gpio_set_as_output (GPIO_ECO_CORE, 0);
#endif
	}

	mdelay (2);	/* clocks have been twiddled... this will take longer than expected... */

	eco_standby_led_red();

	gpio_dump_state_all();

	printk ("ECO-Standby: Polling for key presses (channel %d)...\n", channel);

	/*
	   We enter and leave Eco standby by pressing the power button.
	   Therefore to avoid instant exiting if the button is held for
	   too long on entry, ensure we have seen at least some period
	   of 'no keys pressed' before exiting eco standby.
	*/
	key_releases_seen = 0;

	/* For real eco-standby, required behaviour is LED steady red */

	if (shutdown_main_power_supply) {
		while (1) {
			mdelay (2);					/* clocks have been twiddled... this will take longer than expected... */
			adc = adc_get_value (channel);
			if (adc < 0x3FC)
				printk ("ECO-Standby: adc=0x%04x\n", adc);
			else {
				if (++key_releases_seen >= 1024) {	/* saturate (safe from theoretical overflow...) */
					key_releases_seen = 1024;
					if (autoreboot)
						goto force_reset;
				}
			}
			if ((adc < 32) && (key_releases_seen > 3))
				goto force_reset;
		}
	}

	/* If faking eco-standby, flash the LED instead of steady red */

	while (1) {
		eco_standby_led_red();

		for (i = 0; i < 20; i++) {
			mdelay (8);					/* clocks have been twiddled... this will take longer than expected... */
			adc = adc_get_value (channel);
			if (adc < 0x3FC)
				printk ("ECO-Standby: adc=0x%04x\n", adc);
			else {
				if (++key_releases_seen >= 1024) {	/* saturate (safe from theoretical overflow...) */
					key_releases_seen = 1024;
					if (autoreboot)
						goto force_reset;
				}
			}
			if ((adc < 32) && (key_releases_seen > 3))
				goto force_reset;
		}

		eco_standby_led_off();

		for (i = 0; i < 18; i++) {
			mdelay (8);					/* clocks have been twiddled... this will take longer than expected... */
			adc = adc_get_value (channel);
			if (adc < 0x3FC)
				printk ("ECO-Standby: adc=0x%04x\n", adc);
			else {
				if (++key_releases_seen >= 1024) {	/* saturate (safe from theoretical overflow...) */
					key_releases_seen = 1024;
					if (autoreboot)
						goto force_reset;
				}
			}
			if ((adc < 32) && (key_releases_seen > 3))
				goto force_reset;
		}
	}

force_reset:

	gpio_set_high (GPIO_ECO_STBY);		/* begin startup of main power supply (takes approx 1 second...) */
	mdelay (10);				/* clocks have been twiddled... this will take longer than expected... */

	watchdog_force_reset (0);

	/* code here is never reached... */

	local_irq_restore (flags);
}

static int eco_standby_read_proc (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;

	p += sprintf (p, "To enter ECO standby mode: echo \"enter_eco_standby\" > /proc/eco_standby\n");

	*eof = 1;
	return (p - page);
}

static int eco_standby_write_proc (struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	char sbuf[64];

	if (count > 0) {
		int len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
		memset (sbuf, 0, sizeof(sbuf));
		if (copy_from_user (sbuf, buffer, len))
			return -EFAULT;
		if (strcmp (sbuf, "enter_eco_standby\n") == 0)
			eco_standby_enter (1, 0);				/* never returns... */
		if (strcmp (sbuf, "fake_eco_standby\n") == 0)
			eco_standby_enter (0, 0);				/* never returns... */
		if (strcmp (sbuf, "enter_eco_standby_autoreboot\n") == 0)
			eco_standby_enter (1, 1);				/* never returns... */
		if (strcmp (sbuf, "fake_eco_standby_autoreboot\n") == 0)
			eco_standby_enter (0, 1);				/* never returns... */

//		printk ("'%s' len=%d\n", sbuf, len);
	}
	return count;
}

static int __init eco_standby_init (void)
{
	struct proc_dir_entry *res;

	eco_standby_led_green();

	if ((res = create_proc_entry ("eco_standby", (S_IWUSR | S_IRUGO), NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = eco_standby_read_proc;
	res->write_proc = eco_standby_write_proc;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__initcall (eco_standby_init);			/* 2.4 */
#else
device_initcall (eco_standby_init);		/* 2.6 */
#endif

