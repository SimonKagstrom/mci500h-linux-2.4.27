#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#include <asm/hardware.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/syscreg.h>

#include <linux/usb.h>

#include "host/ehci-amba.h"
#include "host/pnx0106_usb2ip.h"

#undef PNX0106_USB_DEBUG

#ifdef PNX0106_USB_DEBUG
# define DBG_PRINTK(...) printk(__VA_ARGS__)
#else
# define DBG_PRINTK(...) do {} while(0)
#endif

//************************ Platform specific *****************
//CGU
#define CGU_CONFIG_REGS_LENGTH	   (1024)  	/* clock generator registers */
#define CGU_CONFIG_USB_AHB_RST_N_SOFT		(0x0DCul)
#define CGU_CONFIG_USB_OTG_AHB_RST_N_SOFT	(0x0E0ul)

//EVENT ROUTER
#define EVENT_ROUTER_PEND0_REG       		(0xc00ul)
// usb events belong to slice 5
#define ER_NUM_EVENTS                           (170)
#define ER_EVENTS_PER_SLICE                     (32)
#define ER_NUM_SLICES                           ((ER_NUM_EVENTS + (ER_EVENTS_PER_SLICE - 1)) / ER_EVENTS_PER_SLICE)     /* should equal 6... ;-) */
#define ER_EVENT_SLICE(eid)                     ((eid) / ER_EVENTS_PER_SLICE)
#define ER_EVENT_MASK(eid)                      (1 << ((eid) % ER_EVENTS_PER_SLICE))
#define EVENT_ID_USB_UC_GOSUSP                  161
#define EVENT_ID_USB_UC_WKUPCS                  162
#define EVENT_ID_USB_UC_PWROFF                  163
#define EVENT_ID_USB_VBUS                       164
#define EVENT_ID_USB_BUS_RESET                  165
#define EVENT_ID_USB_GOOD_LINK                  166
#define EVENT_ID_USB_OTG_AHB_NEEDCLK            167
#define EVENT_ID_USB_ATX_PLL_LOCK               168
#define EVENT_ID_USB_OTG_VBUS_PWR_EN            169


extern void hcd_irq (int irq, void *__hcd, struct pt_regs * r);
extern struct usb_operations hcd_operations;

/****************************************************************************/

//static int host = 1;	/* for host and OTG */

static unsigned int *gcgu_config_regs;
static pusb_otg_regs gpusb_otg_regs;
static struct proc_dir_entry *proc_entry;

/****************************************************************************/

void usb_set_vbus_on (void);
void usb_set_vbus_off (void);

#if defined (CONFIG_USB_EHCI_PNX0106) && defined (GPIO_USB_POWER_FAULT)

#define KEEP_LOW_TIME_MS				(5000)
#define USB_POWER_FAULT_POLLING_INTERVAL_MS		(500)
#define USB_POWER_FAULT_POLLING_INTERVAL_JIFFIES	((USB_POWER_FAULT_POLLING_INTERVAL_MS * HZ) / 1000)
#define CHECK_USB_POWER_COUNT				(KEEP_LOW_TIME_MS / USB_POWER_FAULT_POLLING_INTERVAL_MS)

static struct timer_list timer_check_usb_power_fault;
static unsigned int usb_power_fault_count;

static void check_usb_power_fault_func (unsigned long unused)
{
	if (gpio_get_value (GPIO_USB_POWER_FAULT) == 0) {
		if (usb_power_fault_count == 0) {
			printk ("USB power fault detected (%d msec)\n", (usb_power_fault_count * USB_POWER_FAULT_POLLING_INTERVAL_MS));
		}
		if (usb_power_fault_count == CHECK_USB_POWER_COUNT) {
			printk ("USB power fault detected (%d msec)\n", (usb_power_fault_count * USB_POWER_FAULT_POLLING_INTERVAL_MS));
			usb_set_vbus_off();
			return; 		/* don't start the timer again... */
		}
		usb_power_fault_count++;
	}
	else
		usb_power_fault_count = 0;

	mod_timer (&timer_check_usb_power_fault, jiffies + USB_POWER_FAULT_POLLING_INTERVAL_JIFFIES);
}

#endif

/****************************************************************************/

void usb_set_vbus_on (void)
{
#if defined (GPIO_USB_BOOST)
	gpio_set_as_output (GPIO_USB_BOOST, USB_BOOST_ON);
#endif
	gpio_set_as_output (GPIO_USB_POWER, USB_POWER_ON);

#if defined (CONFIG_USB_EHCI_PNX0106) && defined (GPIO_USB_POWER_FAULT)
	usb_power_fault_count = 0;
	mod_timer (&timer_check_usb_power_fault, jiffies + USB_POWER_FAULT_POLLING_INTERVAL_JIFFIES);
#endif
}

void usb_set_vbus_off (void)
{
	gpio_set_as_output (GPIO_USB_POWER, USB_POWER_OFF);
#if defined (GPIO_USB_BOOST)
    /* keep bost walways on */
//	gpio_set_as_output (GPIO_USB_BOOST, USB_BOOST_OFF);
#endif

#if defined (CONFIG_USB_EHCI_PNX0106) && defined (GPIO_USB_POWER_FAULT)
	del_timer_sync (&timer_check_usb_power_fault);
#endif
}

/****************************************************************************/

static int proc_vbus_read (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len = 0;
	int vbus_status_raw = gpio_get_value (GPIO_USB_POWER);
	char *status = "OFF";

	if (((vbus_status_raw == 0) && (USB_POWER_ON == 0)) ||
	    ((vbus_status_raw != 0) && (USB_POWER_ON != 0)))
	{
		status = "ON";
	}
	
	len += sprintf (buf + len, "USB VBUS status\n"
				   "Current state: %s\n", status);

	*eof = 1;

	return len;
}

ssize_t proc_vbus_write (struct file *filp, const char __user *buff, unsigned long len, void *data)
{
	char val = 0;

	if (copy_from_user (&val, buff, 1))
		return -EFAULT;

	if (val == '1')
		usb_set_vbus_on();
	else if (val == '2')
		usb_set_vbus_off();

	return len;
}

static int proc_vbus_init (void)
{
	proc_entry = create_proc_entry ("vbus", 0644, NULL);

	if (proc_entry == NULL)
		return -ENOMEM;

	proc_entry->read_proc = proc_vbus_read;
	proc_entry->write_proc = proc_vbus_write;
	return 0;
}

static void vbus_cleanup (void)
{
	remove_proc_entry ("vbus", NULL);
}

/****************************************************************************/

#ifdef PNX0106_USB_DEBUG
static void print_status(void)
{
	if (gpusb_otg_regs != 0){
		printk( "OTG_ID_REG (0x%08X)\t 0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_ID_REG, (unsigned int)gpusb_otg_regs->id);
		printk( "OTG_HWGENERAL_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HWGENERAL_REG, (unsigned int)gpusb_otg_regs->hwgeneral);
		printk( "OTG_HWHOST_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HWHOST_REG, (unsigned int)gpusb_otg_regs->hwhost);
		printk( "OTG_HWDEVICE_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HWDEVICE_REG, (unsigned int)gpusb_otg_regs->hwdevice);
		printk( "OTG_IP_INFO_REG_REG (0x%08X) \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_IP_INFO_REG_REG, (unsigned int)gpusb_otg_regs->ip_info_reg);

		printk( "OTG_CAPLENGTH_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_CAPLENGTH_REG, (unsigned int)gpusb_otg_regs->caplength);
		printk( "OTG_HCIVERSION_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HCIVERSION_REG, (unsigned int)gpusb_otg_regs->hciversion);
		printk( "OTG_HCSPARAMS_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HCSPARAMS_REG, (unsigned int)gpusb_otg_regs->hcsparams);
		printk( "OTG_HCCPARAMS_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_HCCPARAMS_REG, (unsigned int)gpusb_otg_regs->hccparams);
		printk( "OTG_DCIVERSION_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_DCIVERSION_REG, (unsigned int)gpusb_otg_regs->dciversion);
		printk( "OTG_DCCPARAMS_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_DCCPARAMS_REG, (unsigned int)gpusb_otg_regs->dccparams);

		printk( "OTG_USBCMD_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_USBCMD_REG, (unsigned int)gpusb_otg_regs->usbcmd);
		printk( "OTG_USBSTS_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_USBSTS_REG, (unsigned int)gpusb_otg_regs->usbsts);
		printk( "OTG_USBINTR_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_USBINTR_REG, (unsigned int)gpusb_otg_regs->usbintr);
		printk( "OTG_FRINDEX_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_FRINDEX_REG, (unsigned int)gpusb_otg_regs->frindex);
		printk( "OTG_PERIODICLISTBASE__DEVICEADDR_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_PERIODICLISTBASE__DEVICEADDR_REG, (unsigned int)gpusb_otg_regs->periodiclistbase__deviceaddr);
		printk( "OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG (0x%08X)\t0x%08X.\n",
				(unsigned int)USB_OTG_SLV_BASE+OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG, (unsigned int)gpusb_otg_regs->asynclistaddr__endpointlistaddr);
		printk( "OTG_CONFIGFLAG_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_CONFIGFLAG_REG, (unsigned int)gpusb_otg_regs->configflag);
		printk( "OTG_PORTSC1_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_PORTSC1_REG, (unsigned int)gpusb_otg_regs->portsc1);
		printk( "OTG_OTGSC_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_OTGSC_REG, (unsigned int)gpusb_otg_regs->otgsc);
		printk( "OTG_USBMODE_REG (0x%08X)\t \t0x%08X.\n",(unsigned int)USB_OTG_SLV_BASE+OTG_USBMODE_REG, (unsigned int)gpusb_otg_regs->usbmode);
		printk( "OTG_ENDPTSETUPSTAT_REG (0x%08X)\t0x%08X.\n",
				(unsigned int)USB_OTG_SLV_BASE+OTG_ENDPTSETUPSTAT_REG, (unsigned int)gpusb_otg_regs->endptsetupstat);
	}
}
#endif /* PNX0106_USB_DEBUG */

static int usb2ip_enable (void)
{
	u32 value=0;

	/*Memory for CGU registers */
	gcgu_config_regs = (unsigned int *) ioremap_nocache(CGU_CONFIG_REGS_BASE, CGU_CONFIG_REGS_LENGTH) ;
	if ( gcgu_config_regs == 0){
		printk( "USB2: Unable to map the CGU_BASE configuration memory region.\n" ) ;
		return -ENOMEM;
	}
	DBG_PRINTK( "USB2: gpcgu_config_regs: phys: 0X%08x  virt: 0X%08x.\n",CGU_CONFIG_REGS_BASE, gcgu_config_regs);

	DBG_PRINTK( "USB2: gpsyscreg_regs: phys:0X%08x virt:0X%08x.\n",SYSCREG_REGS_BASE, IO_ADDRESS_SYSCREG_BASE);

	/*Memory for USB OTG */
	gpusb_otg_regs = (pusb_otg_regs) ioremap_nocache(USB_OTG_SLV_BASE, USB_OTG_SLV_LENGTH);
	if ( gpusb_otg_regs == 0 ){
		printk( "USB2: Unable to map the USB_OTG_SLV_BASE configuration memory region.\n" ) ;
		return -ENOMEM;
	}
	DBG_PRINTK( "USB2: gpusb_otg_regs: phys:0X%08x virt:0X%08x.\n",USB_OTG_SLV_BASE, (unsigned int)gpusb_otg_regs);

	/*
	 * Init sequence
	 */
#ifdef PNX0106_USB_DEBUG
	DBG_PRINTK("usb_otg_ahb_rst_n_soft(0X%08x) = 0x%08x\n",
			(unsigned int)(gcgu_config_regs +CGU_CONFIG_USB_AHB_RST_N_SOFT),
			readl(gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT));
#endif
	writel(0, gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT);

	/*1- set PLL SYSCREG_USB_ATX_PLL_PD_REG On */
	value = readl(VA_SYSCREG_USB_ATX_PLL_PD_REG);
	DBG_PRINTK( "USB2: SYSCREG_USB_ATX_PLL_PD_REG(0X%08x) = 0X%08x\n",(unsigned int)VA_SYSCREG_USB_ATX_PLL_PD_REG , value);
	writel(0, VA_SYSCREG_USB_ATX_PLL_PD_REG);
	mdelay (2) ;
	/*value = readl(gpsyscreg_regs + SYSCREG_USB_ATX_PLL_PD_REG); */
	value = readl(VA_SYSCREG_USB_ATX_PLL_PD_REG);
	DBG_PRINTK( "USB2: SYSCREG_USB_ATX_PLL_PD_REG(0X%08x) = 0X%08x\n",(unsigned int)VA_SYSCREG_USB_ATX_PLL_PD_REG , value);

	/*Wait PLL Lock */
	DBG_PRINTK("Wait PLL...\n");
	for(;;){
		udelay (100);
		value = readl(IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C00 + ( 4 * ( ER_EVENT_SLICE(EVENT_ID_USB_ATX_PLL_LOCK) ) ) );
		if( !(value & (ER_EVENT_MASK(EVENT_ID_USB_ATX_PLL_LOCK))) )
			break;
		DBG_PRINTK("EVENT PENDING Register value: 0X%08x\n", value);
	}
	DBG_PRINTK("\nUSB PLL locked\n");
	DBG_PRINTK("usb_atx_pll_pd_reg(0X%08x) = 0x%08x\n",(unsigned int) (unsigned int)VA_SYSCREG_USB_ATX_PLL_PD_REG, readl(VA_SYSCREG_USB_ATX_PLL_PD_REG) );
	DBG_PRINTK("Wait 25 secs\n");
	mdelay (25);

	/*Set AHB clock ON */
	DBG_PRINTK("usb_otg_ahb_rst_n_soft(0X%08x) = 0x%08x\n",(unsigned int)(gcgu_config_regs +CGU_CONFIG_USB_AHB_RST_N_SOFT),readl(gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT));
	writel(0, gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT);
	mdelay (50) ;
	writel(1, gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT);
	mdelay (10) ;
	DBG_PRINTK("usb_otg_ahb_rst_n_soft(0X%08x) = 0x%08x\n",(unsigned int)(gcgu_config_regs +CGU_CONFIG_USB_AHB_RST_N_SOFT),readl(gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT));

	/*Select OTG CORE */
	DBG_PRINTK("usb_otg_cfg(0X%08x) = 0x%08x\n",(unsigned int)VA_SYSCREG_USB_OTG_CFG, readl(VA_SYSCREG_USB_OTG_CFG));
	/*gpsyscreg_regs->usb_otg_cfg |= ~0x1; */
	writel(0x5, VA_SYSCREG_USB_OTG_CFG);
	mdelay (10) ;
	DBG_PRINTK("usb_otg_cfg(0X%08x) = 0x%08x\n",(unsigned int)VA_SYSCREG_USB_OTG_CFG, readl(VA_SYSCREG_USB_OTG_CFG));
	if ((gpusb_otg_regs->usbsts) & (1<<4))
		DBG_PRINTK("Error usb_otg_cfg(0X%08x) = 0x%08x\n",(unsigned int)&(gpusb_otg_regs->usbsts),(unsigned int)gpusb_otg_regs->usbsts);

	/* we may have to set host mode and USB power enable gpio bits */
	/* USB power is initially off (unless bootloader has detected service mode UART on pce da2007 platforms...) */
	/* USB power is switched on from userspace via /proc/vbus during application startup */

#if defined (CONFIG_ARCH_PNX0106)
#if defined (CONFIG_SERIAL_PNX010X_CONSOLE)
#if defined (CONFIG_PNX0106_HASLI7) || \
	defined (CONFIG_PNX0106_HACLI7) || \
	defined (CONFIG_PNX0106_MCIH) || \
	defined (CONFIG_PNX0106_MCI)

	extern int pnx010x_console_in_use;

	if (pnx010x_console_in_use)
	{
		printk ("%s"
			"##\n"
			"## Console is on internal UART0... switching on USB vbus.\n"
			"##\n"
			"## THIS MESSAGE SHOULD ONLY BE SEEN WHEN SERVICE UART USED !!\n"
			"##\n"
			"%s",
			"##################################################################\n",
			"##################################################################\n");

		usb_set_vbus_on();
	}
	else
#endif
#endif
#endif
	{
		usb_set_vbus_off();
	}

#if defined (GPIO_USB_POWER_FAULT)
	gpio_set_as_input (GPIO_USB_POWER_FAULT);	// set as input for USB power fault detection (fixme... bootloader should have done this already)
#endif

	DBG_PRINTK( "NEW SYSCREG_USB_ATX_PLL_PD_REG (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_ATX_PLL_PD_REG, readl(VA_SYSCREG_USB_ATX_PLL_PD_REG) ) ;
	DBG_PRINTK( "SYSCREG_USB_OTG_CFG (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_OTG_CFG, readl(VA_SYSCREG_USB_OTG_CFG)) ;
	DBG_PRINTK( "SYSCREG_USB_OTG_PORT_IND_CTL (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_OTG_PORT_IND_CTL, readl(VA_SYSCREG_USB_OTG_PORT_IND_CTL) );
	DBG_PRINTK( "SYSCREG_USB_TPR_DYN (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_SYS_USB_TPR_DYN, readl(VA_SYSCREG_SYS_USB_TPR_DYN ));
	DBG_PRINTK( "SYSCREG_USB_PLL_NDEC (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_NDEC , readl(VA_SYSCREG_USB_PLL_NDEC)) ;
	DBG_PRINTK( "SYSCREG_USB_PLL_MDEC  (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_MDEC , readl(VA_SYSCREG_USB_PLL_MDEC)) ;
	DBG_PRINTK( "SYSCREG_USB_PLL_PDEC  (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_PDEC , readl(VA_SYSCREG_USB_PLL_PDEC)) ;
	DBG_PRINTK( "SYSCREG_USB_PLL_SELR (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_SELR, readl(VA_SYSCREG_USB_PLL_SELR)) ;
	DBG_PRINTK( "SYSCREG_USB_PLL_SELI (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_SELI, readl(VA_SYSCREG_USB_PLL_SELI)) ;
	DBG_PRINTK( "SYSCREG_USB_PLL_SELP (0x%08X) set to 0x%08X.\n",(unsigned int)VA_SYSCREG_USB_PLL_SELP, readl(VA_SYSCREG_USB_PLL_SELP)) ;

	/* Cleanup */
	iounmap((void*) gcgu_config_regs );
	iounmap((void*) gpusb_otg_regs );

	return 0;
}

static int usb2ip_disable (void)
{
	gcgu_config_regs = (unsigned int *) ioremap_nocache(CGU_CONFIG_REGS_BASE, CGU_CONFIG_REGS_LENGTH) ;
	if ( gcgu_config_regs == 0){
		printk( "USB2: Unable to map the CGU_BASE configuration memory region.\n" ) ;
		return -ENOMEM;
	}
	DBG_PRINTK( "USB2: gcgu_config_regs: phys: 0X%08x  virt: 0X%08x.\n",CGU_CONFIG_REGS_BASE, gcgu_config_regs);
	writel(0, gcgu_config_regs+CGU_CONFIG_USB_AHB_RST_N_SOFT);
	writel(1, VA_SYSCREG_USB_ATX_PLL_PD_REG);

	iounmap((void*) gcgu_config_regs );
	return 0;
}

int usb2ip_init (struct amba_device *d)
{
	int retval = 0;

	// set driver private data
	d->res.start	= USB_OTG_SLV_BASE;
	d->res.end	= USB_OTG_SLV_BASE + USB_OTG_SLV_LENGTH - 1;
	d->res.flags	= IORESOURCE_MEM;
	d->irq[0]	= IRQ_USB_OTG_IRQ;
	d->irq[1]	= NO_IRQ;
	d->periphid	= 0x00141077; /* TODO: what needs to be in here? (made up: conf=0, rev=1, manuf=0x41?=arm, part=0x077) */
	//strcpy(d->name, "ehci-amba ");

	// initialize core
	if ((retval = usb2ip_enable()) < 0)  {
		DBG_PRINTK("Failed to enable usb core\n");
		return retval;
	}

	return 0;
}

int usb2ip_exit (struct amba_device *dev)
{
	return usb2ip_disable();
}

int usb_hcd_amba_remove (void *vid)
{
	struct amba_device *dev = (struct amba_id *)vid;
	struct usb_hcd *hcd = (struct usb_hcd*)(&dev->dev);

	if (!hcd) {
		printk(KERN_ERR"usb_hcd_amba_remove: dev has no drvdata!\n");
		return -ENODEV;
	}

	if (in_interrupt ())
		BUG ();

	if (HCD_IS_RUNNING (hcd->state))
		hcd->state = USB_STATE_QUIESCING;

	if (hcd->driver->stop)
		hcd->driver->stop(hcd);

	hcd->state = USB_STATE_HALT;

	free_irq(dev->irq[0], hcd);
	if (hcd->driver->flags & HCD_MEMORY) {
		iounmap (hcd->regs);
	}

	usb_set_vbus_off();
	vbus_cleanup();

	return 0;
}
EXPORT_SYMBOL (usb_hcd_amba_remove);

int usb_hcd_amba_probe (struct amba_device *dev, void *vid)
{
	int			retval = 0;
	char			buf [8], *bufp = buf;
	struct usb_bus		*bus = NULL;
	struct usb_hcd 		*hcd = NULL;
	void 			*device_base = NULL;
	struct amba_id *id 	= (struct amba_id *)vid;
	struct hc_driver	*driver = (struct hc_driver *)id->data;

#if defined (CONFIG_USB_EHCI_PNX0106) && defined (GPIO_USB_POWER_FAULT)
	init_timer (&timer_check_usb_power_fault);
	timer_check_usb_power_fault.function = check_usb_power_fault_func;
	timer_check_usb_power_fault.data = 0;
#endif
	proc_vbus_init();

	usb2ip_init (dev);

	if (driver->flags & HCD_MEMORY) {	// EHCI, OHCI
		/* map the address (already requested by amba_device) */
		device_base = ioremap_nocache( dev->res.start, dev->res.end - dev->res.start + 1 ) ;
		if ( !device_base ) {
			printk( KERN_ERR "%s: failed to remap the amba registers\n", __FUNCTION__) ;
			return -1 ;
		}
	} else {
		printk (KERN_ERR "%s: tried to allocate i/o memory for UHCI\n", __FUNCTION__);
	}


	hcd = driver->hcd_alloc ();
	if (hcd == NULL) {
		printk(KERN_DEBUG "%s: hcd alloc fail\n", __FUNCTION__);
		retval = -ENOMEM;
clean_1:
		if (driver->flags & HCD_MEMORY) {
			iounmap (device_base);
			device_base = 0;
		}
		printk(KERN_DEBUG "hcd-amba::init usb_amba fail, %d\n", retval);
		goto done;
	}
	// hcd zeroed everything
	hcd->regs = device_base;

	//amba_set_drvdata (dev, hcd);
	hcd->pdev = dev;
	hcd->driver = driver;
	hcd->description = driver->description;

	if (hcd->product_desc == NULL)
		hcd->product_desc = "USB EHCI 1.0 Host Controller (amba)";

	hcd->state = USB_STATE_HALT;

#ifndef __sparc__
	sprintf (buf, "%d", dev->irq[0]);
#else
	bufp = __irq_itoa(dev->irq[0]);
#endif

	/* request the irq (there should be one) */
	retval = request_irq (dev->irq[0], hcd_irq, 0/*SA_SHIRQ*/, "usb_ehci", hcd);
	if (retval) {
		printk(KERN_ERR	"hcd-amba.c :: request interrupt %s failed\n", bufp);
		retval = -EBUSY;
		goto clean_1;
	}
	hcd->irq = dev->irq[0];

	bus = usb_alloc_bus (&hcd_operations);
	if (bus == NULL) {
		printk(KERN_ERR "usb_alloc_bus fail\n");
		retval = -ENOMEM;
		free_irq (dev->irq, hcd);
		goto clean_1;
	}

	hcd->bus = bus;
	strcpy(dev->slot_name, "1");
	bus->bus_name = dev->slot_name;
	bus->hcpriv = (void *) hcd;

	INIT_LIST_HEAD (&hcd->dev_list);
	INIT_LIST_HEAD (&hcd->hcd_list);

	down (&hcd_list_lock);
	list_add (&hcd->hcd_list, &hcd_list);
	up (&hcd_list_lock);

	usb_register_bus (bus);

	if ((retval = driver->start (hcd)) < 0)
		usb_hcd_amba_remove (dev);

done:
	return retval;
}

EXPORT_SYMBOL (usb_hcd_amba_probe);

#ifdef CONFIG_PM
int usb_hcd_amba_suspend (struct amba_device *dev, u32 state)
{
	printk (KERN_WARNING"usb_hcd_pci_suspend: not yet implemented\n");
	return 0;
}

int usb_hcd_amba_resume (struct amba_device *dev)
{
	printk (KERN_WARNING"usb_hcd_amba_resume: not yet implemented\n");
	return 0;
}
#endif
