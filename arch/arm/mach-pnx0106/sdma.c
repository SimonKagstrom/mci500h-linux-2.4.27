/*
 * linux/arch/arm/mach-pnx0106/sdma.c
 *
 * Copyright (C) 2006 QM Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/sdma.h>


#define DEVICE_NAME		"sdma"

#define MAX_CHANNEL		8

#define SDMA_IRQ_STAT		(IO_ADDRESS_SDMA_BASE + 0x404)
#define SDMA_IRQ_MASK		(IO_ADDRESS_SDMA_BASE + 0x408)
#define SDMA_SOFT_IRQ		(IO_ADDRESS_SDMA_BASE + 0x410)

#define SRC_OFFS		0
#define DST_OFFS		0x04
#define LEN_OFFS		0x08
#define CONF_OFFS		0x0C
#define ENABLE_OFFS		0x10
#define CNT_OFFS		0x1C


/****************************************************************************/

static int irqsaint = 1;

static int __init irqsaint_setup (char *str)
{
	irqsaint = 0;
	return 1;
}

__setup("sdmairqsaint=", irqsaint_setup);


/****************************************************************************/

struct dma_chaninfo
{
	int used;
	unsigned int src;
	unsigned int dst;
	unsigned int len;
	unsigned int conf;
	unsigned int enable;
	unsigned int count;
	void (*handler) (unsigned int irq_stat);
	unsigned int irq_mask;
};

struct dma_chaninfo dma_channel[MAX_CHANNEL];


static void pnx0106_sdma_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	int i;
	unsigned int irqStat;

	irqStat = readl (SDMA_IRQ_STAT);
	writel (irqStat, SDMA_IRQ_STAT);	/* clear all interrupts */

	for (i = 0; i < MAX_CHANNEL; i++) {
		if ((irqStat & dma_channel[i].irq_mask) != 0)
			if (dma_channel[i].used && dma_channel[i].handler)
				dma_channel[i].handler ((irqStat & dma_channel[i].irq_mask) >> (i * 2));
	}
}

int sdma_allocate_channel (void)
{
	int i;

	for (i = 0; i < MAX_CHANNEL; i++) {
		if (dma_channel[i].used == 0) {
			dma_channel[i].used = 1;
			dma_channel[i].handler = NULL;
			dma_channel[i].irq_mask = 0;
			return i;
		}
	}

	printk ("SetupDma: no free sdma channel\n");
	return -1;
}

void sdma_free_channel (int chan)
{
	if (dma_channel[chan].used == 0)
		printk ("free_sdma_channel: something wrong!\n");

	writel (0, dma_channel[chan].enable);
	dma_channel[chan].used = 0;
	dma_channel[chan].handler = 0;
	dma_channel[chan].irq_mask = 0;
}

void sdma_setup_channel (int chan, struct sdma_config *sdma,
			 void (*handler)(unsigned int))
{
	unsigned int val;

	if (dma_channel[chan].used) {
		writel (sdma->src_addr, dma_channel[chan].src);
		writel (sdma->dst_addr, dma_channel[chan].dst);
		writel ((sdma->len - 1), dma_channel[chan].len);
		val = ((sdma->dst_name << 0) | (sdma->src_name << 5) |
		       (sdma->size << 10) | (sdma->endian << 12) |
		       (sdma->circ_buf << 18));
		writel (val, dma_channel[chan].conf);
		dma_channel[chan].handler = handler;
	}
}

void sdma_config_irq (int chan, unsigned int mask)
{
	if (dma_channel[chan].used) {
		dma_channel[chan].irq_mask = (mask & 0x3) << (chan * 2);
		writel ((readl (SDMA_IRQ_MASK) | (0x3 << (chan * 2))) & ~dma_channel[chan].irq_mask, SDMA_IRQ_MASK);
	}
}

void sdma_clear_irq (int chan)
{
	writel ((0x3 << (chan * 2)), SDMA_IRQ_STAT);
}

void sdma_start_channel (int chan)
{
	if (dma_channel[chan].used && dma_channel[chan].handler) //just in case
		writel (1, dma_channel[chan].enable);
}

void sdma_stop_channel (int chan)
{
	writel (0, dma_channel[chan].enable);
}

unsigned int sdma_get_count (int chan)
{
	return readl (dma_channel[chan].count);
}

void sdma_reset_count (int chan)
{
	writel (0, dma_channel[chan].count);
}

/*
   Only to be used in special cases...
   e.g before enterring eco-standby mode when we need to ensure that
   the SDMA bus master is not active before we disable the SDMA clock.
*/
void sdma_force_shutdown_all_channels (void)
{
	int i;

	for (i = 0; i < MAX_CHANNEL; i++)
		writel (0, dma_channel[i].enable);

//	udelay (10);
}
void sdma_set_len (int chan, unsigned int len )
{

	if (dma_channel[chan].used)
		writel (len, dma_channel[chan].len);

}
static int __init sdma_init (void)
{
	int i;
	int result;
	unsigned int saint;

	for (i = 0; i < MAX_CHANNEL; i++) {
		dma_channel[i].used = 0;
		dma_channel[i].src = IO_ADDRESS_SDMA_BASE + (0x20 * i);
		dma_channel[i].dst = dma_channel[i].src + DST_OFFS;
		dma_channel[i].len = dma_channel[i].src + LEN_OFFS;
		dma_channel[i].conf = dma_channel[i].src + CONF_OFFS;
		dma_channel[i].enable = dma_channel[i].src + ENABLE_OFFS;
		dma_channel[i].count = dma_channel[i].src + CNT_OFFS;
		dma_channel[i].handler = NULL;
		dma_channel[i].irq_mask = 0;
	}

	saint = 0;
	if (irqsaint) {
		printk ("Requesting SDMA irq with SA_INTERRUPT flag\n");
		saint = SA_INTERRUPT;
	}

	result = request_irq (IRQ_SDMA, pnx0106_sdma_isr, saint, DEVICE_NAME, NULL);
	if (result != 0)
		printk ("%s: could not register interrupt %d res=%d\n",
			DEVICE_NAME, IRQ_SDMA, result);

	return result;
}

__initcall (sdma_init);

#if 1
EXPORT_SYMBOL(sdma_allocate_channel);
EXPORT_SYMBOL(sdma_free_channel);
EXPORT_SYMBOL(sdma_setup_channel);
EXPORT_SYMBOL(sdma_config_irq);
EXPORT_SYMBOL(sdma_clear_irq);
EXPORT_SYMBOL(sdma_start_channel);
EXPORT_SYMBOL(sdma_stop_channel);
EXPORT_SYMBOL(sdma_get_count);
EXPORT_SYMBOL(sdma_reset_count);
EXPORT_SYMBOL(sdma_set_len);
#endif

