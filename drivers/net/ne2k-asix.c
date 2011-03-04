/* linux/drivers/net/ne2k-asix.c
 *
 *
 * based on: ne2k-pci.c: A NE2000 clone on PCI bus driver for Linux. */

/*
	A Linux device driver for ASIX onboard ASIX ethernet

	Authors and other copyright holders:
	2003      by Ben Dooks, Modifies PCI NE2000 support for ASIX onboard
	          ASIX fast-ethernet controller

	1992-2000 by Donald Becker, NE2000 core and various modifications.
	1995-1998 by Paul Gortmaker, core modifications and PCI support.
	Copyright 1993 assigned to the United States Government as represented
	by the Director, National Security Agency.

	This software may be used and distributed according to the terms of
	the GNU General Public License (GPL), incorporated herein by reference.
	Drivers based on or derived from this code fall under the GPL and must
	retain the authorship, copyright and license notice.  This file is not
	a complete program and may only be used when the entire operating
	system is licensed under the GPL.

	The author may be reached as becker@scyld.com, or C/O
	Scyld Computing Corporation
	410 Severn Ave., Suite 210
	Annapolis MD 21403

	Issues remaining:
	People are making any ne2000 clones! Oh the horror, the horror...
	Limited full-duplex support.
*/

#define DRV_NAME	"has7752-asix"
#define DRV_VERSION	"1.02"
#define DRV_RELDATE	"21/05/2003"

#include "8390-asix.h"

/* The user-configurable values.
   These may be modified when a driver module is loaded.*/

static int debug = 1;			/* 1 normal messages, 0 quiet .. 7 verbose. */

#define MAX_UNITS 1				/* More are supported, limit only on options */
/* Used to pass the full-duplex flag, etc. */
#if 0
static int full_duplex[MAX_UNITS];
static int options[MAX_UNITS];
#endif

/* Force a non std. amount of memory.  Units are 256 byte pages. */
/* #define PACKETBUF_MEMSIZE	0x40 */


#if 1
/* define the outsl/insl as the faster multi-word variants, as this
 * card supports this IO format.
*/

#define _outsl __raw_writesl
#define _insl __raw_readsl
#else
#define _outsl outsl
#define _insl insl
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ethtool.h>
#include <linux/mii.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>

#include <asm/arch/tsc.h>               /* only used if we need to fake a MAC address */

#define COMPILE_ASIX

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "8390.h"

/* These identify the driver base version and may not be removed. */
static char version[] __devinitdata =
KERN_INFO DRV_NAME ".c v" DRV_VERSION " " DRV_RELDATE " D. Becker/P. Gortmaker/Ben Dooks\n"

//KERN_INFO "  http://www.simtec.co.uk/products/EB2410ITX/\n";


#define PFX DRV_NAME ": "

MODULE_AUTHOR("Donald Becker / Paul Gortmaker / Ben Dooks");
MODULE_DESCRIPTION("ASIX NE2000 (ASIX Onboard ethernet)");
MODULE_LICENSE("GPL");

MODULE_PARM(debug, "i");
MODULE_PARM(options, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM(full_duplex, "1-" __MODULE_STRING(MAX_UNITS) "i");
MODULE_PARM_DESC(debug, "debug level (1-2)");
MODULE_PARM_DESC(options, "Bit 5: full duplex");
MODULE_PARM_DESC(full_duplex, "full duplex setting(s) (1)");

/* Some defines that people can play with if so inclined. */

/* Use 32 bit data-movement operations instead of 16 bit. */
#define USE_LONGIO

/* Do we implement the read before write bugfix ? */
/* #define NE_RW_BUGFIX */

/* Flags.  We rename an existing ei_status field to store flags! */
/* Thus only the low 8 bits are usable for non-init-time flags. */
#define ne2k_flags reg0
enum {
	ONLY_16BIT_IO=8, ONLY_32BIT_IO=4,	/* Chip can do only 16/32-bit xfers. */
	FORCE_FDX=0x20,						/* User override. */
	STOP_PG_0x60=0x100,
};

/* ---- No user-serviceable parts below ---- */

#define NE_BASE	        (dev->base_addr)
#define NE_CMD	 	EI_SHIFT(0x00)
#define NE_DATAPORT     EI_SHIFT(0x10)
#define NE_BTCR 	EI_SHIFT(0x15)	/* AX88796 Rev B devices only... */
#define NE_RESET	EI_SHIFT(0x1f)	/* Issue a read to reset, a write to clear. */
#define NE_IO_EXTENT	EI_SHIFT(0x20)
#define EN0_MEMR	EI_SHIFT(0x14)	/* MII management */

#define NESM_START_PG	0x40	/* First page of TX buffer */
#define NESM_STOP_PG	0x80	/* Last page +1 of RX ring */

#define MDO		0x08
#define MDI		0x04
#define MDIR		0x02
#define MDC		0x01

#define BTCR_PME_POL	(1 << 0)	/* 0: (default) PME output is active low, 1: PME output is active high (ignored if PME_TYPE is open drain) */
#define BTCR_PME_TYPE	(1 << 1)	/* 0: (default) PME output is open drain, 1: PME output is push-pull */
#define BTCR_PME_IND	(1 << 2)	/* 0: (default) PME generates static active signal, 1: PME generates a single 60 msec active pulse */
#define BTCR_IRQ_POL	(1 << 4)	/* 0: (default) IRQ output is active low, 1: IRQ output is active high */
#define BTCR_IRQ_TYPE	(1 << 5)	/* 0: (default) IRQ output is open drain, 1: IRQ output is push-pull */
#define BTCR_PME_IRQ_EN (1 << 6)	/* 0: (default) PME interrup disabled, 1: PME interrupt enabled */


extern unsigned char eth0mac[6];	/* value may be set from a boot tag passed by the bootloader... */
/* Only one interface supported currently */
static struct mii_if_info mii;
static struct timer_list link_timer;

static int net_asix_open(struct net_device *dev);
static int net_asix_close(struct net_device *dev);

static void net_asix_reset_8390(struct net_device *dev);
static void net_asix_get_8390_hdr(struct net_device *dev, struct e8390_pkt_hdr *hdr,
			  int ring_page);
static void net_asix_block_input(struct net_device *dev, int count,
			  struct sk_buff *skb, int ring_offset);
static void net_asix_block_output(struct net_device *dev, const int count,
		const unsigned char *buf, const int start_page);
static int netdev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);



/* There is no room in the standard 8390 structure for extra info we need,
   so we build a meta/outer-wrapper structure.. */
struct net_asix_card {
	struct net_device *dev;
};

/* io routines to deal with the register spacing on the ASIX onboard
 * controller... these have an real advantage for other operations
 */

static inline void
ax_outb(unsigned int val, unsigned long iobase, unsigned int reg)
{
#if defined(CONFIG_NET_NE2KASIX_REGDEBUG)
	if (reg != 0 && reg < 32)
		printk("ax_outb(%02x,%08x,%04x)\n", val, iobase, reg);
#endif
	__raw_writew((u16)val, iobase + (reg));
}

static inline unsigned char
ax_inb(unsigned long iobase, unsigned int reg)
{
#if defined(CONFIG_NET_NE2KASIX_REGDEBUG)
	if (reg != 0 && reg < 32)
		printk("ax_inb(%08x,%04x)\n", iobase, reg);
#endif
	return (unsigned char)__raw_readw(iobase + (reg));
}

/*
  NEx000-clone boards have a Station Address (SA) PROM (SAPROM) in the packet
  buffer memory space.  By-the-spec NE2000 clones have 0x57,0x57 in bytes
  0x0e,0x0f of the SAPROM, while other supposed NE2000 clones must be
  detected by their SA prefix.

  Reading the SAPROM from a word-wide card with the 8390 set in byte-wide
  mode results in doubled values, which can be detected and compensated for.

  The probe is also responsible for initializing the card and filling
  in the 'dev' and 'ei_status' structures.
*/

static int initialised;

#ifdef CONFIG_SSA_HAS7752
#include <asm/hardware.h>

static void get_has_ax88796_addr_shift(void)
{
	int platform_id = RELEASE_ID_PLATFORM(release_id);

	switch (platform_id) {
	case PLATFORM_SDB_REVA:
//		HAS_AX88796_ADDRESS_SHIFT = 1;
		panic ("%s: Ancient Rev A board requires address shift of 1 "
                       "(please edit 8390.h and recompile)", __FUNCTION__);
		break;

	case PLATFORM_SDB_REVC:
	case PLATFORM_HAS_LAB1:
	case PLATFORM_HAS_LAB2:
//		HAS_AX88796_ADDRESS_SHIFT = 6;
		break;

	default:
		panic("%s: Platform ID %d is unsupported or invalid\n", 
					__FUNCTION__, platform_id);
	}
}
#endif

#define mdio_delay()	udelay(1)

static inline void mdio_send(unsigned long ioaddr, int val, int len)
{
	int mask = 1 << (len-1);
	unsigned int mdo, temp; 

	temp = ax_inb(ioaddr, EN0_MEMR) & 0xf0;
	while (len--) {
		mdo = (val & mask) ? MDO : 0;
		mdo |= temp;
		ax_outb(mdo, ioaddr, EN0_MEMR);
		mdio_delay();
		ax_outb(mdo | MDC, ioaddr, EN0_MEMR);
		mdio_delay();
		mask >>= 1;
	}
	/* Put PHY in read mode */
	ax_outb(MDC | MDIR, ioaddr, EN0_MEMR);
}

static inline int mdio_recv(unsigned long ioaddr, int len)
{
	int ret = 0;
	unsigned int temp;

	temp = ax_inb(ioaddr, EN0_MEMR) & 0xf0;
	while (len--) {
		ax_outb(temp | MDIR, ioaddr, EN0_MEMR);
		mdio_delay();
		ax_outb(temp | MDIR | MDC, ioaddr, EN0_MEMR);
		mdio_delay();
		ret = (ret << 1) | ((ax_inb(ioaddr, EN0_MEMR) & MDI) >> 2);
	}
	return ret;
}

static void mdio_sync(unsigned long ioaddr, int bits)
{
	/* Establish sync by sending at least 32 logic ones. */
	mdio_send(ioaddr, 0xffffffff, bits % 33);
}

static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	unsigned long ioaddr = dev->base_addr;
	unsigned long flags;
	int ret;

	local_irq_save(flags);

	/* Send preamble */
	mdio_sync(ioaddr, 32);

	/* Send start pattern */
	mdio_send(ioaddr, 0x01, 2);

	/* Send read op */
	mdio_send(ioaddr, 0x02, 2);

	/* Send PHY address */
	mdio_send(ioaddr, phy_id & 0x1f, 5);

	/* Send register address */
	mdio_send(ioaddr, location & 0x1f, 5);

	/* Just transition the clock for the first bit period */
	mdio_recv(ioaddr, 1);
	/* Send a 1, since there seems to be a bug in the PHY the next line
	 * is commented out.
	 */
	/*mdio_send(ioaddr, 0x01, 1);*/

	/* Now read data */
	ret = mdio_recv(ioaddr, 16);
	ret &= 0xffff;

	local_irq_restore(flags);
	return ret;
}

static void mdio_write(struct net_device *dev, int phy_id, int location, int value)
{
	unsigned long ioaddr = dev->base_addr;
	unsigned long flags;

	local_irq_save(flags);

	/* Send preamble */
	mdio_sync(ioaddr, 32);

	/* Send start pattern */
	mdio_send(ioaddr, 0x01, 2);

	/* Send write op */
	mdio_send(ioaddr, 0x01, 2);

	/* Send PHY address */
	mdio_send(ioaddr, phy_id & 0x1f, 5);

	/* Send register address */
	mdio_send(ioaddr, location & 0x1f, 5);

	/* Send Turnaround */
	mdio_send(ioaddr, 0x02, 2);

	/* Now write data */
	mdio_send(ioaddr, value & 0xffff, 16);

	local_irq_restore(flags);
}

static void check_link_status(unsigned long data)
{
	mii_check_media(&mii, 1, 0);
	link_timer.expires = jiffies + HZ;
	add_timer(&link_timer);
}

int __devinit ne_asix_probe (struct net_device *dev)
{
	int i;
	unsigned char SA_prom[32];
	int start_page, stop_page;
	int irq, reg0;
	unsigned long ioaddr;

/* when built into the kernel, we only print version if device is found */
#ifndef MODULE
	static int printed_version;
	if (!printed_version++)
		printk(version);
#endif

	printk("ASIX Onboard NE2000 driver, (c) 2003 Simtec Electronics\n");

#ifdef CONFIG_SSA_HAS7752
	get_has_ax88796_addr_shift();
#endif

	if (initialised) {
	  return -ENODEV;
	}
	initialised = 1;

	/* configure ioaddr, and make sure it is the fast version */

#if defined (CONFIG_ARCH_SSA)
	ioaddr = (unsigned long)ioremap_nocache(SSA_NCS1_BASE, 0x100000);       /* Hmmm... */
	irq = INT_ETHERNET;
#elif defined (CONFIG_ARCH_PNX0106)
	ioaddr = (unsigned long) ioremap_nocache (AX88796_REGS_BASE, (8 * 1024));
	irq = IRQ_ETHERNET;
#else
#error "Unknown platform..."
#endif

	printk("%s: io=0x%08lx, irq=%d\n", __FUNCTION__, ioaddr, irq);

	if (request_region (ioaddr, NE_IO_EXTENT, DRV_NAME) == NULL) {
		printk (KERN_ERR PFX "I/O resource 0x%x @ 0x%lx busy\n",
			NE_IO_EXTENT, ioaddr);
		return -EBUSY;
	}

	reg0 = ax_inb(ioaddr, 0x00);
	if (reg0 == 0xFF)
		goto err_out_free_res;

	printk("%s: prelimary test...\n", __FUNCTION__);
	/* Do a preliminary verification that we have a 8390. */
	{
		int regd;
		ax_outb(E8390_NODMA+E8390_PAGE1+E8390_STOP, ioaddr, E8390_CMD);
		regd = ax_inb(ioaddr, EI_SHIFT(0x0d));
		ax_outb(0xff, ioaddr, EI_SHIFT(0x0d));
		ax_outb(E8390_NODMA+E8390_PAGE0, ioaddr, E8390_CMD);
		ax_inb(ioaddr, EN0_COUNTER0); /* Clear the counter by reading. */
		if (ax_inb(ioaddr, EN0_COUNTER0) != 0) {
		        printk("%s: EN0_COUNTER0 did not clear (%02x)\n", __FUNCTION__, ax_inb(ioaddr, EN0_COUNTER0));
			ax_outb(reg0, ioaddr, EI_SHIFT(0));
			ax_outb(regd, ioaddr, EI_SHIFT(0x0d));	/* Restore the old values. */
			goto err_out_free_res;
		}
	}

	SET_MODULE_OWNER(dev);

	printk("%s: applying reset\n", __FUNCTION__);

	/* Reset card. Who knows what dain-bramaged state it was left in. */
	{
		unsigned long reset_start_time = jiffies;

		ax_outb(ax_inb(ioaddr, NE_RESET), ioaddr, NE_RESET);

		/* This looks like a horrible timing loop, but it should never take
		   more than a few cycles.
		*/
		while ((ax_inb(ioaddr, EN0_ISR) & ENISR_RESET) == 0)
			/* Limit wait: '2' avoids jiffy roll-over. */
			if (jiffies - reset_start_time > 4) {
				printk(KERN_ERR PFX "Card failure (no reset ack).\n");
				goto err_out_free_netdev;
			}

		ax_outb(0xff, ioaddr, EN0_ISR);		/* Ack all intr. */
	}

	printk("%s: configuring...\n", __FUNCTION__);

	/* Read the 16 bytes of station address PROM.
	   We must first initialize registers, similar to NS8390_init(eifdev, 0).
	   We can't reliably read the SAPROM address without this.
	   (I learned the hard way!). */
	{
		struct {
		  unsigned char value;
		  unsigned int offset; } program_seq[] = {
			{E8390_NODMA+E8390_PAGE0+E8390_STOP, E8390_CMD}, /* Select page 0*/
			{0x49,	EN0_DCFG},	/* Set word-wide access. */
			{0x00,	EN0_RCNTLO},	/* Clear the count regs. */
			{0x00,	EN0_RCNTHI},
			{0x00,	EN0_IMR},	/* Mask completion irq. */
			{0xFF,	EN0_ISR},
			{E8390_RXOFF|0x40, EN0_RXCR},	/* 0x20  Set to monitor  and irq high */
			{E8390_TXOFF, EN0_TXCR},	/* 0x02  and loopback mode. */
			{32,	EN0_RCNTLO},
			{0x00,	EN0_RCNTHI},
			{0x00,	EN0_RSARLO},	/* DMA starting at 0x0000. */
			{0x00,	EN0_RSARHI},
			{E8390_RREAD+E8390_START, E8390_CMD},
		};
		for (i = 0; i < sizeof(program_seq)/sizeof(program_seq[0]); i++)
			ax_outb(program_seq[i].value, ioaddr, program_seq[i].offset);

	}

#if defined (CONFIG_NE2K_ASIX_REVB)
	{
		unsigned char btcr_old = ax_inb (ioaddr, NE_BTCR);
		unsigned char btcr_new = btcr_old | (BTCR_IRQ_POL | BTCR_IRQ_TYPE);
		printk ("%s: NE_BTCR: 0x%02x -> 0x%02x\n", __FUNCTION__, btcr_old, btcr_new);
		ax_outb (btcr_new, ioaddr, NE_BTCR);
	}
#endif

	printk("%s: reading sa-prom:\n", __FUNCTION__);

	for(i = 0; i < 32 /*sizeof(SA_prom)*/; i++) 
	  SA_prom[i] = ax_inb(ioaddr, NE_DATAPORT);

	/* We always set the 8390 registers for word mode. */
	ax_outb(0x49, ioaddr, EN0_DCFG);
	start_page = NESM_START_PG;
	stop_page = NESM_STOP_PG;

	printk("%s: dev=%p, irq=%d, base=0x%08lx\n", __FUNCTION__,
	       dev, irq, ioaddr);

	/* Set up the rest of the parameters. */
	dev->irq = irq;
	dev->base_addr = ioaddr;

	printk("%s: calling asix_ethdev_init\n", __FUNCTION__);

	/* Allocate dev->priv and fill in 8390 specific dev fields. */
	if (asix_ethdev_init(dev)) {
	     printk (KERN_ERR "ne2k-asix: no memory for dev->priv.\n");
		goto err_out_free_netdev;
	}

	printk("%s: setting ei_status bits\n", __FUNCTION__);

	ei_status.name = "asix";
	ei_status.tx_start_page = start_page;
	ei_status.stop_page = stop_page;
	ei_status.word16 = 1;

#if 0
	if (fnd_cnt < MAX_UNITS) {
		if (full_duplex[fnd_cnt] > 0  ||  (options[fnd_cnt] & FORCE_FDX))
			ei_status.net_flags |= FORCE_FDX;
	}
#endif

	ei_status.rx_start_page = start_page + TX_PAGES;
#ifdef PACKETBUF_MEMSIZE
	/* Allow the packet buffer size to be overridden by know-it-alls. */
	ei_status.stop_page = ei_status.tx_start_page + PACKETBUF_MEMSIZE;
#endif

	printk("%s: setting up ei functions:\n", __FUNCTION__);

	ei_status.reset_8390 = &net_asix_reset_8390;
	ei_status.block_input = &net_asix_block_input;
	ei_status.block_output = &net_asix_block_output;
	ei_status.get_8390_hdr = &net_asix_get_8390_hdr;
	dev->open = &net_asix_open;
	dev->stop = &net_asix_close;
	dev->do_ioctl = &netdev_ioctl;
	mii.phy_id = 0x10;
	mii.phy_id_mask = 0x1f;
	mii.reg_num_mask = 0x1f;
	mii.dev = dev;
	mii.mdio_read = mdio_read;
	mii.mdio_write = mdio_write;
	link_timer.expires = jiffies + HZ;
	link_timer.function = check_link_status;
	link_timer.data = 0;
	add_timer(&link_timer);
	asix_NS8390_init(dev, 0);

#if 0
	i = register_netdev(dev);
	if (i) {
	        printk(KERN_ERR PFX "cannot register network device\n");
		goto err_out_free_8390;
	}
#endif

	if (((dev->dev_addr[0] = eth0mac[0]) |
	     (dev->dev_addr[1] = eth0mac[1]) |
	     (dev->dev_addr[2] = eth0mac[2]) |
	     (dev->dev_addr[3] = eth0mac[3]) |
	     (dev->dev_addr[4] = eth0mac[4]) |
	     (dev->dev_addr[5] = eth0mac[5])) == 0)
	{
                printk ("%s: eth0mac boot tag was empty. Faking MAC address\n", dev->name);

		dev->dev_addr[0] = 0x00;
		dev->dev_addr[1] = 0x12;
		dev->dev_addr[2] = 0x34;
		dev->dev_addr[3] = 0x56;
		dev->dev_addr[4] = 0x78;
		dev->dev_addr[5] = ReadTSC();	/* attempt at a random last byte... */
	}

	printk("%s: asix-ne2k found at %#lx, IRQ %d, ", 
		   dev->name, ioaddr, dev->irq);

	for (i = 0; i < 6; i++)
		printk("%2.2X%s", dev->dev_addr[i], i == 5 ? "\n" : ":");

	return 0;

#if 0
err_out_free_8390:
	kfree(dev->priv);
	dev->priv = 0;
#endif
        
err_out_free_netdev:
err_out_free_res:
	release_region (ioaddr, NE_IO_EXTENT);
	printk("%s: exiting with -ENODEV\n", __FUNCTION__);
	return -ENODEV;

}

static int net_asix_open(struct net_device *dev)
{
	int ret = request_irq(dev->irq, ei_interrupt,
			      SA_SHIRQ, dev->name, dev);
	if (ret)
		return ret;

	/* Set full duplex for the chips that we know about. */

	ei_open(dev);
	mii.advertising = mdio_read(dev, mii.phy_id & 0x1f, MII_ADVERTISE);
	mii_check_media(&mii, 1, 1);
	return 0;
}

static int net_asix_close(struct net_device *dev)
{
	ei_close(dev);
	free_irq(dev->irq, dev);
	return 0;
}

/* Hard reset the card.  This used to pause for the same period that a
   8390 reset command required, but that shouldn't be necessary. */
static void net_asix_reset_8390(struct net_device *dev)
{
	unsigned long reset_start_time = jiffies;

	if (debug > 1) printk("%s: Resetting the 8390 t=%ld...", 
						  dev->name, jiffies);

	ax_outb(ax_inb(NE_BASE, NE_RESET), NE_BASE, NE_RESET);

	ei_status.txing = 0;
	ei_status.dmaing = 0;

	/* This check _should_not_ be necessary, omit eventually. */
	while ((ax_inb(NE_BASE,EN0_ISR) & ENISR_RESET) == 0)
		if (jiffies - reset_start_time > 2) {
			printk("%s: net_asix_reset_8390() did not complete.\n", dev->name);
			break;
		}
	ax_outb(ENISR_RESET, NE_BASE, EN0_ISR);	/* Ack intr. */
}

/* Grab the 8390 specific header. Similar to the block_input routine, but
   we don't need to be concerned with ring wrap as the header will be at
   the start of a page, so we optimize accordingly. */

static void net_asix_get_8390_hdr(struct net_device *dev, struct e8390_pkt_hdr *hdr, int ring_page)
{

	long nic_base = dev->base_addr;

	/* This *shouldn't* happen. If it does, it's the last thing you'll see */
	if (ei_status.dmaing) {
		printk("%s: DMAing conflict in net_asix_get_8390_hdr "
			   "[DMAstat:%d][irqlock:%d].\n", 
			   dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}

	ei_status.dmaing |= 0x01;
	ax_outb(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base, NE_CMD);
	ax_outb(sizeof(struct e8390_pkt_hdr), nic_base, EN0_RCNTLO);
	ax_outb(0, nic_base, EN0_RCNTHI);
	ax_outb(0, nic_base, EN0_RSARLO);		/* On page boundary */
	ax_outb(ring_page, nic_base, EN0_RSARHI);
	ax_outb(E8390_RREAD+E8390_START, nic_base, NE_CMD);

	insw(NE_BASE + NE_DATAPORT, hdr, sizeof(struct e8390_pkt_hdr)>>1);

	ax_outb(ENISR_RDC, nic_base, EN0_ISR);	/* Ack intr. */
	ei_status.dmaing &= ~0x01;
}

/* Block input and output, similar to the Crynwr packet driver.  If you
   are porting to a new ethercard, look at the packet driver source for hints.
   The NEx000 doesn't share the on-board packet memory -- you have to put
   the packet out through the "remote DMA" dataport using ax_outb. */

static void net_asix_block_input(struct net_device *dev, int count,
				 struct sk_buff *skb, int ring_offset)
{
	long nic_base = dev->base_addr;
	unsigned short *buf = (unsigned short *)skb->data;

	/* This *shouldn't* happen. If it does, it's the last thing you'll see */
	if (ei_status.dmaing) {
		printk("%s: DMAing conflict in net_asix_block_input "
			   "[DMAstat:%d][irqlock:%d].\n", 
			   dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}
	ei_status.dmaing |= 0x01;
	ax_outb(E8390_NODMA+E8390_PAGE0+E8390_START, nic_base, NE_CMD);
	ax_outb(count & 0xff, nic_base, EN0_RCNTLO);
	ax_outb(count >> 8, nic_base, EN0_RCNTHI);
	ax_outb(ring_offset & 0xff, nic_base, EN0_RSARLO);
	ax_outb(ring_offset >> 8, nic_base, EN0_RSARHI);
	ax_outb(E8390_RREAD+E8390_START, nic_base, NE_CMD);

#if 0
	{
		unsigned long tmp;
	
		/* ensure that we have an aligned buffer... */
		if ((((unsigned int)buf) & 3) == 2) {
		  *buf = __raw_readw(NE_BASE + NE_DATAPORT);
		  count -= 2;
		  buf += 2;
		}
	
		_insl(NE_BASE + NE_DATAPORT,buf,count>>2);
	
		/* mop up any remaining bytes */
		switch (count & 0x3) {
		case 0x03:
		  tmp = __raw_readl(NE_BASE + NE_DATAPORT);
		  buf[count-3] = tmp;
		  buf[count-2] = tmp>>8;
		  buf[count-1] = tmp>>16;
		  break;
	
		case 0x02:
		  tmp = __raw_readw(NE_BASE + NE_DATAPORT);
		  buf[count-2] = tmp;
		  buf[count-1] = tmp>>8;
		  break;
	
		case 0x01:
		  buf[count-1] = __raw_readw(NE_BASE + NE_DATAPORT);
		  break;
		}
	}
#else
	insw(NE_BASE + NE_DATAPORT, buf, (count+1)>>1);
#endif

	ax_outb(ENISR_RDC, nic_base, EN0_ISR);	/* Ack intr. */
	ei_status.dmaing &= ~0x01;
}

static void net_asix_block_output(struct net_device *dev, int count,
				  const unsigned char *buf, const int start_page)
{
	long nic_base = NE_BASE;
	unsigned short *bufptr = (unsigned short *)buf;
	unsigned long dma_start;
	int rcount;

	/* On little-endian it's always safe to round the count up for
	   word writes. */

	rcount = ((count + 3) & ~0x03);

	/* This *shouldn't* happen. If it does, it's the last thing you'll see */
	if (ei_status.dmaing) {
		printk("%s: DMAing conflict in net_asix_block_output."
			   "[DMAstat:%d][irqlock:%d]\n", 
			   dev->name, ei_status.dmaing, ei_status.irqlock);
		return;
	}
	ei_status.dmaing |= 0x01;
	/* We should already be in page 0, but to be safe... */
	ax_outb(E8390_PAGE0+E8390_START+E8390_NODMA, nic_base, NE_CMD);

#ifdef NE8390_RW_BUGFIX
	/* Handle the read-before-write bug the same way as the
	   Crynwr packet driver -- the NatSemi method doesn't work.
	   Actually this doesn't always work either, but if you have
	   problems with your NEx000 this is better than nothing! */
	ax_outb(0x42, nic_base, EN0_RCNTLO);
	ax_outb(0x00, nic_base, EN0_RCNTHI);
	ax_outb(0x42, nic_base, EN0_RSARLO);
	ax_outb(0x00, nic_base, EN0_RSARHI);
	ax_outb(E8390_RREAD+E8390_START, nic_base, NE_CMD);
#endif
	ax_outb(ENISR_RDC, nic_base, EN0_ISR);

   /* Now the normal output. */
	ax_outb(count & 0xff, nic_base, EN0_RCNTLO);
	ax_outb(count >> 8,   nic_base, EN0_RCNTHI);
	ax_outb(0x00, nic_base, EN0_RSARLO);
	ax_outb(start_page, nic_base, EN0_RSARHI);
	ax_outb(E8390_RWRITE+E8390_START, nic_base, NE_CMD);

	if (((unsigned int)bufptr & 3) == 2) {
	  __raw_writew(*bufptr, NE_BASE + NE_DATAPORT);
	  bufptr++;
	}

	//_outsl(NE_BASE + NE_DATAPORT, bufptr, rcount>>2);
	//for (i = 0; i < rcount >> 1; i++)
	// ax_outb(bufptr[i], nic_base, NE_DATAPORT);
	outsw(NE_BASE + NE_DATAPORT, bufptr, rcount>>1);
	
	dma_start = jiffies;

	while ((ax_inb(nic_base, EN0_ISR) & ENISR_RDC) == 0)
		if (jiffies - dma_start > 2) {			/* Avoid clock roll-over. */
			printk(KERN_WARNING "%s: timeout waiting for Tx RDC.\n", dev->name);
			net_asix_reset_8390(dev);
			asix_NS8390_init(dev,1);
			break;
		}

	ax_outb(ENISR_RDC, nic_base, EN0_ISR);	/* Ack intr. */
	ei_status.dmaing &= ~0x01;
	return;
}

static int netdev_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
//	struct ei_device *ei = dev->priv;
	u32 ethcmd;
	unsigned long flags;

	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		return -EFAULT;

        switch (ethcmd) {
        case ETHTOOL_GDRVINFO: {
		struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};
		strcpy(info.driver, DRV_NAME);
		strcpy(info.version, DRV_VERSION);
		strcpy(info.bus_info, "asix");
		if (copy_to_user(useraddr, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	/* get settings */
	case ETHTOOL_GSET: {
		struct ethtool_cmd ecmd = { ETHTOOL_GSET };
		local_irq_save(flags);
		mii_ethtool_gset(&mii, &ecmd);
		local_irq_restore(flags);
		if (copy_to_user(useraddr, &ecmd, sizeof(ecmd)))
			return -EFAULT;
		return 0;
	}
	/* set settings */
	case ETHTOOL_SSET: {
		int r;
		struct ethtool_cmd ecmd;
		if (copy_from_user(&ecmd, useraddr, sizeof(ecmd)))
			return -EFAULT;
		local_irq_save(flags);
		r = mii_ethtool_sset(&mii, &ecmd);
		local_irq_restore(flags);
		return r;
	}
	/* restart autonegotiation */
	case ETHTOOL_NWAY_RST: {
		return mii_nway_restart(&mii);
	}
	/* get link status */
	case ETHTOOL_GLINK: {
		struct ethtool_value edata = {ETHTOOL_GLINK};
		edata.data = mii_link_ok(&mii);
		if (copy_to_user(useraddr, &edata, sizeof(edata)))
			return -EFAULT;
		return 0;
	}
	/* get message-level */
	case ETHTOOL_GMSGLVL: {
		struct ethtool_value edata = {ETHTOOL_GMSGLVL};
		edata.data = debug;
		if (copy_to_user(useraddr, &edata, sizeof(edata)))
			return -EFAULT;
		return 0;
	}
	/* set message-level */
	case ETHTOOL_SMSGLVL: {
		struct ethtool_value edata;
		if (copy_from_user(&edata, useraddr, sizeof(edata)))
			return -EFAULT;
		debug = edata.data;
		return 0;
	}

        }

	return -EOPNOTSUPP;
}

static int netdev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	switch(cmd) {
	case SIOCETHTOOL:
		return netdev_ethtool_ioctl(dev, (void *) rq->ifr_data);
	default:
		return -EOPNOTSUPP;
	}
}

static int __init net_asix_modinit(void)
{
        struct net_device *dev;
/* when a module, this is printed whether or not devices are found in probe */
#if defined(MODULE)
	printk(version);
#endif

	dev = (struct net_device *)kmalloc(sizeof(*dev), 0);
	printk("net_asix_modinit: new device at 0x%p\n", dev);

	if (dev != NULL) {
		memset(dev, 0, sizeof(*dev));

		dev->init = ne_asix_probe;
		if (register_netdev(dev) != 0) {
			printk("failed to initialise driver\n");
			return -ENXIO;
		}
	}

	return 0;
}


static void __exit net_asix_modcleanup(void)
{
  /* todo: cleanup */
}

module_init(net_asix_modinit);
module_exit(net_asix_modcleanup);
