/*
 * linux/arch/arm/mach-pnx0106/adc.c
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/errno.h>

#include <asm/arch/adc.h>


#define VA_ADC_RESULT(channel)	(IO_ADDRESS_ADC_BASE + 0x00 + (4 * (channel)))
#define VA_ADC_CON		(IO_ADDRESS_ADC_BASE + 0x20)
#define VA_ADC_CSEL_RES		(IO_ADDRESS_ADC_BASE + 0x24)
#define VA_ADC_INT_ENABLE	(IO_ADDRESS_ADC_BASE + 0x28)
#define VA_ADC_INT_STATUS	(IO_ADDRESS_ADC_BASE + 0x2C)
#define VA_ADC_INT_CLEAR	(IO_ADDRESS_ADC_BASE + 0x30)

#define CON_SELVREF_0		(1 << 0)
#define CON_SELVREF_1		(0 << 0)
#define CON_ENABLE		(1 << 1)
#define CON_CSCAN		(1 << 2)
#define CON_START		(1 << 3)
#define CON_STATUS		(1 << 4)


unsigned int adc_read (unsigned int channel)
{
	unsigned int result;
	unsigned int vref_sel;
	unsigned int resolution;
	unsigned long flags;

	if (channel >= NR_ADC_CHANNELS)
		return (unsigned int) -1;

	resolution = 10;			/* resolution must be between 2 and 10 bits */
	vref_sel = CON_SELVREF_0;		/* CON_SELVREF_0 or CON_SELVREF_1 */

	local_irq_save (flags);

	writel (vref_sel, VA_ADC_CON);
	writel ((resolution << (channel * 4)), VA_ADC_CSEL_RES);
	writel ((vref_sel | CON_ENABLE), VA_ADC_CON);
	writel ((vref_sel | CON_ENABLE | CON_START), VA_ADC_CON);
	writel ((vref_sel | CON_ENABLE), VA_ADC_CON);

	while ((readl (VA_ADC_CON) & CON_STATUS) != 0)
		;

	result = readl (VA_ADC_RESULT(channel));
	writel (vref_sel, VA_ADC_CON);

	local_irq_restore (flags);

	return result;
}


unsigned int adc_get_value (unsigned int channel)
{
	if (channel >= NR_ADC_CHANNELS) {
		printk ("adc_get_value: channel number too large\n");
		return 0;
	}
	return adc_read (channel);
}

/*****************************************************************************
* adcdrv_set_mode: set the operation mode of AD
   0--single mode, stops after one single conversion
   1--continous mode, run forever
   This function also enable the ADC at the mode
***************************************************************************/
void adcdrv_set_mode (int mode)
{
	/* ignore... currently adc is used in single single mode only */
}

void adcdrv_disable_adc (void)
{
	/* ignore... adc is disabled automatically after every read in single mode */
}


/*******************************************************************************
*******************************************************************************/

#if defined (CONFIG_PROC_FS)

static int proc_adc_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	unsigned int channel = (int) data;

	if (channel >= NR_ADC_CHANNELS)
		p += sprintf (p, "Channel out of range\n");
	else
		p += sprintf (p, "0x%03x\n", adc_read (channel));

	*eof = 1;
	return (p - page);
}

static int __init er_init_proc_entry (void)
{
	int i;
	char name[16];

	for (i = 0; i < NR_ADC_CHANNELS; i++) {
		sprintf (name, "adc%d", i);
		create_proc_read_entry (name, 0, NULL, proc_adc_read, (void *) i);
	}

	return 0;
}

#else
#define er_init_proc_entry	do { /* nothing */ } while (0)
#endif

static int __init adc_init (void)
{
	printk ("Initialising 10bit ADC\n");
	er_init_proc_entry();
	return 0;
}

__initcall (adc_init);


/*****************************************************************************
*****************************************************************************/

#if 1
EXPORT_SYMBOL(adc_read);
EXPORT_SYMBOL(adc_get_value);
EXPORT_SYMBOL(adcdrv_set_mode);
EXPORT_SYMBOL(adcdrv_disable_adc);
#endif

