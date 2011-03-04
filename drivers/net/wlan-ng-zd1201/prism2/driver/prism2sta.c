/* src/prism2/driver/prism2sta.c
*
* Implements the station functionality for prism2
*
* Copyright (C) 1999 AbsoluteValue Systems, Inc.  All Rights Reserved.
* --------------------------------------------------------------------
*
* linux-wlan
*
*   The contents of this file are subject to the Mozilla Public
*   License Version 1.1 (the "License"); you may not use this file
*   except in compliance with the License. You may obtain a copy of
*   the License at http://www.mozilla.org/MPL/
*
*   Software distributed under the License is distributed on an "AS
*   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*   implied. See the License for the specific language governing
*   rights and limitations under the License.
*
*   Alternatively, the contents of this file may be used under the
*   terms of the GNU Public License version 2 (the "GPL"), in which
*   case the provisions of the GPL are applicable instead of the
*   above.  If you wish to allow the use of your version of this file
*   only under the terms of the GPL and not to allow others to use
*   your version of this file under the MPL, indicate your decision
*   by deleting the provisions above and replace them with the notice
*   and other provisions required by the GPL.  If you do not delete
*   the provisions above, a recipient may use your version of this
*   file under either the MPL or the GPL.
*
* --------------------------------------------------------------------
*
* Inquiries regarding the linux-wlan Open Source project can be
* made directly to:
*
* AbsoluteValue Systems Inc.
* info@linux-wlan.com
* http://www.linux-wlan.com
*
* --------------------------------------------------------------------
*
* Portions of the development of this software were funded by 
* Intersil Corporation as part of PRISM(R) chipset product development.
*
* --------------------------------------------------------------------
*
* This file implements the module and linux pcmcia routines for the
* prism2 driver.
*
* --------------------------------------------------------------------
*/

/*================================================================*/
/* System Includes */

#include <linux/config.h>
#define WLAN_DBVAR	prism2_debug
#include <linux/version.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/wireless.h>
#include <linux/netdevice.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
#include <linux/tqueue.h>
#else
#include <linux/workqueue.h>
#endif

#include <asm/io.h>
#include <linux/delay.h>
#include <asm/byteorder.h>
#include <linux/if_arp.h>

#include <wlan/wlan_compat.h>

#if (WLAN_HOSTIF == WLAN_PCMCIA)
#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>
#include <pcmcia/cisreg.h>
#endif

#if ((WLAN_HOSTIF == WLAN_PLX) || (WLAN_HOSTIF == WLAN_PCI))
#include <linux/ioport.h>
#include <linux/pci.h>
#endif

#if (WLAN_HOSTIF == WLAN_USB)
#include <linux/usb.h>
#endif

#if (WLAN_HOSTIF == WLAN_MMIO)          /* more SAA7752 specific than generic MMIO... */
#include <linux/ioport.h>
#include <asm/arch/hardware.h>
#define ZD1201_REGS_SIZE        (1024)  /* Fixme: check real size... */
#endif


/*================================================================*/
/* Project Includes */

#include <wlan/version.h>
#include <wlan/p80211types.h>
#include <wlan/p80211hdr.h>
#include <wlan/p80211mgmt.h>
#include <wlan/p80211conv.h>
#include <wlan/p80211msg.h>
#include <wlan/p80211netdev.h>
#include <wlan/p80211req.h>
#include <wlan/p80211metadef.h>
#include <wlan/p80211metastruct.h>
#include <prism2/hfa384x.h>
#include <prism2/prism2mgmt.h>

/*================================================================*/
/* Local Constants */

#if (WLAN_HOSTIF == WLAN_PLX)
#define PLX_ATTR_SIZE	0x1000	/* Attribute memory size - 4K bytes */
#define COR_OFFSET	0x3e0	/* COR attribute offset of Prism2 PC card */
#define COR_VALUE	0x41	/* Enable PC card with irq in level trigger */
#define PLX_INTCSR	0x4c	/* Interrupt Control and Status Register */
#define PLX_INTCSR_INTEN (1<<6) /* Interrupt Enable bit */
#define PLX_MIN_ATTR_LEN 512    /* at least 2 x 256 is needed for CIS */

/* 3Com 3CRW777A (PLX) board ID */
#define PCIVENDOR_3COM       0x10B7
#define PCIDEVICE_AIRCONNECT 0x7770

/* Eumitcom PCI WL11000 PCI Adapter (PLX) board device+vendor ID */
#define PCIVENDOR_EUMITCOM	0x1638UL
#define PCIDEVICE_WL11000	0x1100UL

/* Global Sun Tech GL24110P PCI Adapter (PLX) board device+vendor ID */
#define PCIVENDOR_GLOBALSUN	0x16abUL
#define PCIDEVICE_GL24110P	0x1101UL
#define PCIDEVICE_GL24110P_ALT	0x1102UL

/* Netgear MA301 PCI Adapter (PLX) board device+vendor ID */
#define PCIVENDOR_NETGEAR	0x1385UL
#define PCIDEVICE_MA301		0x4100UL

/* US Robotics USR2410 PCI Adapter (PLX) board device+vendor ID */
#define	PCIVENDOR_USROBOTICS    0x16ecUL
#define PCIDEVICE_USR2410       0x3685UL

/* Linksys WPC11 card with the WDT11 adapter (PLX) board device+vendor ID */
#define	PCIVENDOR_Linksys       0x16abUL
#define PCIDEVICE_Wpc11Wdt11    0x1102UL

/* National Datacomm Corp SOHOware Netblaster II PCI */
#define PCIVENDOR_NDC	        0x15e8UL
#define PCIDEVICE_NCP130_PLX 	0x0130UL
#define PCIDEVICE_NCP130_ASIC	0x0131UL

/* NDC NCP130_PLX is also sold by Corega. Their name is CGWLPCIA11 */
#define PCIVENDOR_COREGA       PCIVENDOR_NDC
#define PCIDEVICE_CGWLPCIA11   PCIDEVICE_NCP130_PLX

/* PCI Class & Sub-Class code, Network-'Other controller' */
#define PCI_CLASS_NETWORK_OTHERS 0x280
#endif /* WLAN_PLX */

#if (WLAN_HOSTIF == WLAN_PCI)
#define PCI_SIZE		0x1000		/* Memory size - 4K bytes */

/* ISL3874A 11Mb/s WLAN controller */
#define PCIVENDOR_INTERSIL	0x1260UL
#define PCIDEVICE_ISL3874	0x3873UL /* [MSM] yeah I know...the ID says 
					    3873. Trust me, it's a 3874. */

/* Samsung SWL-2210P 11Mb/s WLAN controller (uses ISL3874A) */
#define PCIVENDOR_SAMSUNG      0x167dUL
#define PCIDEVICE_SWL_2210P    0xa000UL

/* PCI Class & Sub-Class code, Network-'Other controller' */
#define PCI_CLASS_NETWORK_OTHERS 0x280

#endif /* WLAN_PCI */


/*================================================================*/
/* Local Macros */

/*================================================================*/
/* Local Types */

/*================================================================*/
/* Local Static Definitions */

#if (WLAN_HOSTIF == WLAN_PCMCIA)
#define DRIVER_SUFFIX	"_cs"
#elif (WLAN_HOSTIF == WLAN_PLX)
#define DRIVER_SUFFIX	"_plx"
typedef char* dev_info_t;
#elif (WLAN_HOSTIF == WLAN_PCI)
#define DRIVER_SUFFIX	"_pci"
typedef char* dev_info_t;
#elif (WLAN_HOSTIF == WLAN_USB)
#define DRIVER_SUFFIX	"_usb"
typedef char* dev_info_t;
#elif (WLAN_HOSTIF == WLAN_MMIO)
#define DRIVER_SUFFIX	"_mmio"
typedef char* dev_info_t;
#else
#error "HOSTIF unsupported or undefined!"
#endif

#ifdef ZD1201_CHIPSET
  #define DEVNAME "zd1201"
#else
  #define DEVNAME "prism2"
#endif

static char		*version = DEVNAME DRIVER_SUFFIX ".o: " WLAN_RELEASE;
static dev_info_t	dev_info = DEVNAME DRIVER_SUFFIX;

#if (WLAN_HOSTIF == WLAN_PCMCIA)

static dev_link_t	*dev_list = NULL;	/* head of instance list */

#endif

/*-----------------------------------------------------------------*/ 
#if (WLAN_HOSTIF == WLAN_PLX || WLAN_HOSTIF == WLAN_PCI)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
/* NOTE: The pci code in this driver is written to the
 * 2.4.x (or some 2.3.x and newer) pci support.  The declarations
 * inside this #if block are to provide backward compatibility to 2.2.x.
 * NOTE2: If you want to modify the pci support, please make sure you do 
 * it in a 2.4.x compliant way.
 */

struct pci_driver_mapping {
	struct pci_dev *dev;
	struct pci_driver *drv;
	void *driver_data;
};

struct pci_device_id
{
	unsigned int vendor, device;
        unsigned int subvendor, subdevice;
        unsigned int class, class_mask;
        unsigned long driver_data;
};

struct pci_driver
{
        struct {int a;} dummy;
        char *name;
        const struct pci_device_id *id_table;   /* NULL if wants all devices */
        int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);
        void (*remove) (struct pci_dev *dev);
        int  (*save_state) (struct pci_dev *dev, u32 state);
        int  (*suspend)(struct pci_dev *dev, u32 state);
        int  (*resume) (struct pci_dev *dev);
        int  (*enable_wake) (struct pci_dev *dev, u32 state, int enable);
};

#define PCI_MAX_MAPPINGS 16
static struct pci_driver_mapping drvmap [PCI_MAX_MAPPINGS] = { { NULL, } , };

#define PCI_ANY_ID	0xffff

static int pci_register_driver(struct pci_driver *drv);
static void pci_unregister_driver(struct pci_driver *drv);
static int pci_populate_drvmap (struct pci_dev *dev, struct pci_driver *drv);
static void *pci_get_drvdata (struct pci_dev *dev);
static void pci_set_drvdata (struct pci_dev *dev, void *driver_data);

/* no-ops */
#define pci_enable_device(a) 0
#define pci_disable_device(a) 0
#define request_mem_region(x,y,z)       (1)
#define release_mem_region(x,y)         do {} while (0)

#ifndef pci_resource_start
static unsigned long pci_resource_len (struct pci_dev *dev, int n_base)
{
        u32 l, sz;
        int reg = PCI_BASE_ADDRESS_0 + (n_base << 2);

        /* XXX temporarily disable I/O and memory decoding for this device? */

        pci_read_config_dword (dev, reg, &l);
        if (l == 0xffffffff)
                return 0;

        pci_write_config_dword (dev, reg, ~0);
        pci_read_config_dword (dev, reg, &sz);
        pci_write_config_dword (dev, reg, l);

        if (!sz || sz == 0xffffffff)
                return 0;
        if ((l & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY) {
                sz = ~(sz & PCI_BASE_ADDRESS_MEM_MASK);
        } else {
                sz = ~(sz & PCI_BASE_ADDRESS_IO_MASK) & 0xffff;
        }
        
        return sz + 1;
}
#define pci_resource_start(dev, i) \
        (((dev)->base_address[i] & PCI_BASE_ADDRESS_SPACE) ? \
         ((dev)->base_address[i] & PCI_BASE_ADDRESS_IO_MASK) : \
         ((dev)->base_address[i] & PCI_BASE_ADDRESS_MEM_MASK))
#define pci_resource_end(dev,bar) \
        (pci_resource_len((dev),(bar)) == 0 ? \
        pci_resource_start(dev,bar) : \
        (pci_resource_start(dev,bar) + pci_resource_len((dev),(bar)) - 1)
#define pci_resource_flags(dev, i) \
        (dev->base_address[i] & IORESOURCE_IO)
#endif

#endif /* (WLAN_HOSTIF == WLAN_PLX || WLAN_HOSTIF == WLAN_PCI) */
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)) */
/*-----------------------------------------------------------------*/ 

#if (WLAN_HOSTIF == WLAN_PLX || WLAN_HOSTIF == WLAN_PCI)
#ifdef CONFIG_PM
static int prism2sta_suspend_pci(struct pci_dev *pdev, u32 state);
static int prism2sta_resume_pci(struct pci_dev *pdev);
#endif
#endif

#if (WLAN_HOSTIF == WLAN_PLX)
static struct pci_device_id plx_id_tbl[] = {
	{
		PCIVENDOR_EUMITCOM, PCIDEVICE_WL11000,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0,
		/* Driver data, we just put the name here */
		(unsigned long)"Eumitcom WL11000 PCI(PLX) card"	
	},
	{
		PCIVENDOR_GLOBALSUN, PCIDEVICE_GL24110P,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"Global Sun Tech GL24110P PCI(PLX) card"
	},
	{
		PCIVENDOR_GLOBALSUN, PCIDEVICE_GL24110P_ALT,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"Global Sun Tech GL24110P PCI(PLX) card"
	},
	{
		PCIVENDOR_NETGEAR, PCIDEVICE_MA301,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"Global Sun Tech GL24110P PCI(PLX) card"
	},
	{
		PCIVENDOR_USROBOTICS, PCIDEVICE_USR2410,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0,
		/* Driver data, we just put the name here */
		(unsigned long)"US Robotics USR2410 PCI(PLX) card"	
	},
	{
		PCIVENDOR_Linksys, PCIDEVICE_Wpc11Wdt11,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0,
		/* Driver data, we just put the name here */
		(unsigned long)"Linksys WPC11 with WDT11 PCI(PLX) adapter"	
	},
	{
	        PCIVENDOR_NDC, PCIDEVICE_NCP130_PLX,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"NDC Netblaster II PCI(PLX)"
	},
	{
	        PCIVENDOR_NDC, PCIDEVICE_NCP130_ASIC,
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"NDC Netblaster II PCI(TMC7160)"
	},
	{
		PCIVENDOR_3COM, PCIDEVICE_AIRCONNECT,	
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"3Com AirConnect PCI 802.11b 11Mb/s WLAN Controller"
	},
	{
		0, 0, 0, 0, 0, 0, 0
	}
};

MODULE_DEVICE_TABLE(pci, plx_id_tbl);

/* Function declared here because of ptr reference below */
static int __devinit prism2sta_probe_plx(struct pci_dev *pdev, 
					 const struct pci_device_id *);
static void __devexit prism2sta_remove_plx(struct pci_dev *pdev);

struct pci_driver prism2_plx_drv_id = {
        .name = "prism2_plx",
        .id_table = plx_id_tbl,
        .probe = prism2sta_probe_plx,
        .remove = prism2sta_remove_plx,
#ifdef CONFIG_PM
        .suspend = prism2sta_suspend_pci,
        .resume = prism2sta_resume_pci,
#endif
};

#endif /* WLAN_PLX */

#if (WLAN_HOSTIF == WLAN_PCI)

static struct pci_device_id pci_id_tbl[] = {
	{
		PCIVENDOR_INTERSIL, PCIDEVICE_ISL3874,	
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"Intersil Prism2.5 ISL3874 11Mb/s WLAN Controller"
	},
	{
		PCIVENDOR_INTERSIL, 0x3872,	
		PCI_ANY_ID, PCI_ANY_ID,
		0, 0, 
		/* Driver data, we just put the name here */
		(unsigned long)"Intersil Prism2.5 ISL3872 11Mb/s WLAN Controller"
	},
        { 
               PCIVENDOR_SAMSUNG, PCIDEVICE_SWL_2210P,
               PCI_ANY_ID, PCI_ANY_ID,
               0, 0,
               /* Driver data, we just put the name here */
               (unsigned long)"Samsung MagicLAN SWL-2210P 11Mb/s WLAN Controller"
	},
	{
		0, 0, 0, 0, 0, 0, 0
	}
};

MODULE_DEVICE_TABLE(pci, pci_id_tbl);

/* Function declared here because of ptr reference below */
static int  __devinit prism2sta_probe_pci(struct pci_dev *pdev, 
				const struct pci_device_id *id);
static void  __devexit prism2sta_remove_pci(struct pci_dev *pdev);

struct pci_driver prism2_pci_drv_id = {
        .name = "prism2_pci",
        .id_table = pci_id_tbl,
        .probe = prism2sta_probe_pci,
        .remove = prism2sta_remove_pci,
#ifdef CONFIG_PM
        .suspend = prism2sta_suspend_pci,
        .resume = prism2sta_resume_pci,
#endif
};

#endif /* WLAN_PCI */

#if (WLAN_HOSTIF == WLAN_USB)

#define PRISM_USB_DEVICE(vid, pid, name) \
           USB_DEVICE(vid, pid),  \
           driver_info: (unsigned long) name

static struct usb_device_id usb_prism_tbl[] = {
	{PRISM_USB_DEVICE(0x04bb, 0x0922, "IOData AirPort WN-B11/USBS")},
	{PRISM_USB_DEVICE(0x07aa, 0x0012, "Corega Wireless LAN USB Stick-11")},
	{PRISM_USB_DEVICE(0x09aa, 0x3642, "Prism2.x 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x1668, 0x0408, "Actiontec Prism2.5 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x1668, 0x0421, "Actiontec Prism2.5 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x066b, 0x2212, "Linksys WUSB11v2.5 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x066b, 0x2213, "Linksys WUSB12v1.1 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x067c, 0x1022, "Siemens SpeedStream 1022 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x049f, 0x0033, "Compaq/Intel W100 PRO/Wireless 11Mbps multiport WLAN Adapter")},
	{PRISM_USB_DEVICE(0x0411, 0x0016, "Melco WLI-USB-S11 11Mbps WLAN Adapter")},
	{PRISM_USB_DEVICE(0x08de, 0x7a01, "PRISM25 IEEE 802.11 Mini USB Adapter")},
	{PRISM_USB_DEVICE(0x8086, 0x1111, "Intel PRO/Wireless 2011B LAN USB Adapter")},
	{PRISM_USB_DEVICE(0x0d8e, 0x7a01, "PRISM25 IEEE 802.11 Mini USB Adapter")},
	{PRISM_USB_DEVICE(0x045e, 0x006e, "Microsoft MN510 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x0967, 0x0204, "Acer Warplink USB Adapter")},
	{PRISM_USB_DEVICE(0x0cde, 0x0002, "Z-Com 725/726 Prism2.5 USB/USB Integrated")},
	{PRISM_USB_DEVICE(0x413c, 0x8100, "Dell TrueMobile 1180 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x0b3b, 0x1601, "ALLNET 0193 11Mbps WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x0b3b, 0x1602, "ZyXEL ZyAIR B200 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x0baf, 0x00eb, "USRobotics USR1120 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x0411, 0x0027, "Melco WLI-USB-KS11G 11Mbps WLAN Adapter")},
        {PRISM_USB_DEVICE(0x04f1, 0x3009, "JVC MP-XP7250 Builtin USB WLAN Adapter")},
	{PRISM_USB_DEVICE(0x0846, 0x4110, "NetGear MA111")},
        {PRISM_USB_DEVICE(0x03f3, 0x0020, "Adaptec AWN-8020 USB WLAN Adapter")},
//	{PRISM_USB_DEVICE(0x0ace, 0x1201, "ZyDAS ZD1201 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x2821, 0x3300, "ASUS-WL140 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x2001, 0x3700, "DWL-122 Wireless USB Adapter")},
	{PRISM_USB_DEVICE(0x50c2, 0x4013, "Averatec USB WLAN Adapter")},
	{PRISM_USB_DEVICE(0x2c02, 0x14ea, "Planex GW-US11H WLAN USB Adapter")},
	{PRISM_USB_DEVICE(0x124a, 0x168b, "Airvast PRISM3 WLAN USB Adapter")},
	{ /* terminator */ }
};

MODULE_DEVICE_TABLE(usb, usb_prism_tbl);

/* Functions declared here because of ptr references below */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
static void __devinit *prism2sta_probe_usb(
	struct usb_device *dev, 
	unsigned int ifnum,
	const struct usb_device_id *id);
static void __devexit prism2sta_disconnect_usb(struct usb_device *dev, void *ptr);
#else
static int prism2sta_probe_usb(
	struct usb_interface *interface,
	const struct usb_device_id *id);
static void prism2sta_disconnect_usb(struct usb_interface *interface);
#endif

struct usb_driver prism2_usb_driver = {
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,4,19))
	.owner = THIS_MODULE,
#endif
	.name = "prism2_usb",
	.probe = prism2sta_probe_usb,
	.disconnect = prism2sta_disconnect_usb,
	.id_table = usb_prism_tbl,
	/* fops, minor? */
};

#endif /* WLAN_USB */


#if (WLAN_HOSTIF == WLAN_MMIO)

static wlandevice_t *mmio_wlandev;      /* fixme: this is only enough for a single device... */

#endif


static wlandevice_t *create_wlan(void);

/*----------------------------------------------------------------*/
/* --Module Parameters */

int      prism2_reset_holdtime=30;	/* Reset hold time in ms */
int	 prism2_reset_settletime=100;	/* Reset settle time in ms */

#if (WLAN_HOSTIF == WLAN_USB)
static int	prism2_doreset=0;		/* Do a reset at init? */
#else
static int      prism2_doreset=1;		/* Do a reset at init? */
int             prism2_bap_timeout=1000;        /* BAP timeout */
int		prism2_irq_evread_max=20;	/* Maximum number of 
						 * ev_reads (loops)
						 * in irq handler 
						 */
#endif

#ifdef WLAN_INCLUDE_DEBUG

#if 1
int		prism2_debug=5;			/* Debug output level, */
#else
int		prism2_debug=0;			/* Debug output level, */
#endif

MODULE_PARM( prism2_debug, "i");
MODULE_PARM_DESC(prism2_debug, "prism2 debugging");	
#endif

MODULE_PARM( prism2_doreset, "i");
MODULE_PARM_DESC(prism2_doreset, "Issue a reset on initialization?");

MODULE_PARM( prism2_reset_holdtime, "i");
MODULE_PARM_DESC( prism2_reset_holdtime, "reset hold time in ms");
MODULE_PARM( prism2_reset_settletime, "i");
MODULE_PARM_DESC( prism2_reset_settletime, "reset settle time in ms");

#if (WLAN_HOSTIF != WLAN_USB)
MODULE_PARM( prism2_bap_timeout, "i");
MODULE_PARM_DESC(prism2_bap_timeout, "BufferAccessPath Timeout in 10*n us");
MODULE_PARM( prism2_irq_evread_max, "i");
MODULE_PARM_DESC( prism2_irq_evread_max, "Maximim number of event reads in interrupt handler");
#endif

#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual MPL/GPL");
#endif

#if (WLAN_HOSTIF == WLAN_PCMCIA)
static u_int	irq_mask = 0xdeb8;		/* Interrupt mask */
static int	irq_list[4] = { -1 };		/* Interrupt list */
static u_int	prism2_ignorevcc=0;		/* Boolean, if set, we
						 * ignore what the Vcc
						 * is set to and what the CIS
						 * says.
						 */
MODULE_PARM( irq_mask, "i");
MODULE_PARM( irq_list, "1-4i");
MODULE_PARM( prism2_ignorevcc, "i");
#endif /* WLAN_PCMCIA */


/*================================================================*/
/* Local Function Declarations */

#if (WLAN_HOSTIF == WLAN_PCMCIA)
dev_link_t	*prism2sta_attach(void);
static void	prism2sta_detach(dev_link_t *link);
static void	prism2sta_config(dev_link_t *link);
static void	prism2sta_release(UINT32 arg);
static int 	prism2sta_event (event_t event, int priority, event_callback_args_t *args);
#endif

static int	prism2sta_open(wlandevice_t *wlandev);
static int	prism2sta_close(wlandevice_t *wlandev);
static void	prism2sta_reset(wlandevice_t *wlandev );
static int      prism2sta_txframe(wlandevice_t *wlandev, struct sk_buff *skb, p80211_hdr_t *p80211_hdr, p80211_metawep_t *p80211_wep);
static int	prism2sta_mlmerequest(wlandevice_t *wlandev, p80211msg_t *msg);
static int	prism2sta_getcardinfo(wlandevice_t *wlandev);
static int	prism2sta_globalsetup(wlandevice_t *wlandev);
static int      prism2sta_setmulticast(wlandevice_t *wlandev, 
				       netdevice_t *dev);

static void	prism2sta_inf_handover(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_tallies(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void     prism2sta_inf_hostscanresults(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_scanresults(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_chinforesults(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_linkstatus(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_assocstatus(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_authreq(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_authreq_defer(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);
static void	prism2sta_inf_psusercnt(
			wlandevice_t *wlandev, hfa384x_InfFrame_t *inf);

/*================================================================*/
/* Function Definitions */

/*----------------------------------------------------------------
* dmpmem
*
* Debug utility function to dump memory to the kernel debug log.
*
* Arguments:
*	buf	ptr data we want dumped
*	len	length of data
*
* Returns: 
*	nothing
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
inline void dmpmem(void *buf, int n)
{
	int c;
	for ( c= 0; c < n; c++) {
		if ( (c % 16) == 0 ) printk(KERN_DEBUG"dmp[%d]: ", c);
		printk("%02x ", ((UINT8*)buf)[c]);
		if ( (c % 16) == 15 ) printk("\n");
	}
	if ( (c % 16) != 0 ) printk("\n");
}

#if (WLAN_HOSTIF == WLAN_PCMCIA)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,68))
/*----------------------------------------------------------------
* cs_error
*
* Utility function to print card services error messages.
*
* Arguments:
*	handle	client handle identifying this CS client
*	func	CS function number that generated the error
*	ret	CS function return code
*
* Returns: 
*	nothing
* Side effects:
*
* Call context:
*	process thread
*	interrupt
----------------------------------------------------------------*/
static void cs_error(client_handle_t handle, int func, int ret)
{
#if CS_RELEASE_CODE < 0x2911
	CardServices(ReportError, dev_info, (void *)func, (void *)ret);
#else
	error_info_t err = { func, ret };
	CardServices(ReportError, handle, &err);
#endif
}
#else // kernel_version
static struct pcmcia_driver prism2_cs_driver = {
	.drv = { 
		.name = "prism2_cs",
	},
	.attach = prism2sta_attach,
	.detach = prism2sta_detach,
	.owner = THIS_MODULE,
};
#endif /* kernel_version */
#endif /* PCMCIA */

/*-----------------------------------------------------------------*/ 
#if (WLAN_HOSTIF == WLAN_PLX || WLAN_HOSTIF == WLAN_PCI)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
/* NOTE: See the note above about 2.4.x and pci support */

/*----------------------------------------------------------------
* pci_register_driver
* pci_unregister_driver
*
* 2.4.x PCI support function emulation for 2.2.x kernels.
*
* Arguments:
*	drv	2.4.x type driver description block
*
* Returns: 
*	0	success
*	~0	error
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
static int pci_register_driver(struct pci_driver *drv)
{
	int			nfound = 0;
	struct pci_dev		*pdev;
	const struct pci_device_id	*id_tbl=drv->id_table;

	DBFENTER;

	/* Scan the bus for matching devices */
	if (pcibios_present()) {
		static int	pci_index = 0;
		UINT8		pci_bus;
		UINT8		pci_device_fn;

		for(;pci_index < 0xff; pci_index++) {
			u16	vendor;
			u16	device;
			int	idx;

			if (pcibios_find_class(PCI_CLASS_NETWORK_OTHER<<8,pci_index, 
				&pci_bus, &pci_device_fn) != PCIBIOS_SUCCESSFUL) 
				break;
			pcibios_read_config_word(pci_bus, pci_device_fn, 
				PCI_VENDOR_ID, &vendor);
			pcibios_read_config_word(pci_bus, pci_device_fn,
				PCI_DEVICE_ID, &device);
			for( idx = 0; id_tbl[idx].vendor; idx++) {
				if (	id_tbl[idx].vendor == vendor &&
					id_tbl[idx].device == device )
					break; /* found one! */
			} 
			if (id_tbl[idx].vendor == 0) continue;

			nfound++;

			/* Probably an invalid assumption...but we'll assume the
			 * card is alive for now.  TODO: need to add power management
			 * stuff here.
			 */

			/* Collect the pci_device structure */
			pdev = pci_find_slot(pci_bus, pci_device_fn);

			/* Set the driver for this device in the drvmap */
			if (pci_populate_drvmap(pdev, drv) == 0) {
				/* Call the driver probe function */
				(*(drv->probe))(pdev, &(id_tbl[idx]));
			}
		}
	}
	DBFEXIT;
	return nfound;
}

static int pci_populate_drvmap (struct pci_dev *dev, struct pci_driver *drv)
{
	int i;
	int vacant = -1;

	for (i = 0; i < PCI_MAX_MAPPINGS; i++){
		if (drvmap[i].dev == dev) {
			drvmap[i].drv = drv;
			return 0;
		} else if(vacant < 0 && drvmap[i].dev == NULL) {
			vacant = i;
		}
	}
	
	if(vacant >= 0){
		drvmap[vacant].dev = dev;
		drvmap[vacant].drv = drv;
		return 0;
	} else {
		return -1;
	}
}


static void * pci_get_drvdata (struct pci_dev *dev)
{
	int i;
        
	for (i = 0; i < PCI_MAX_MAPPINGS; i++)
		if (drvmap[i].dev == dev)
			return drvmap[i].driver_data;

	return NULL;
}

static void pci_set_drvdata (struct pci_dev *dev, void *driver_data)
{
	int i;
        
	for (i = 0; i < PCI_MAX_MAPPINGS; i++)
		if (drvmap[i].dev == dev) {
			drvmap[i].driver_data = driver_data;
			return;
		}
}
static void pci_unregister_driver(struct pci_driver *drv)
{
        struct pci_dev *dev;
        int i;

        for(dev = pci_devices; dev; dev = dev->next) {
                for (i = 0; i < PCI_MAX_MAPPINGS; i++)
                        if (drvmap[i].dev == dev &&
                            drvmap[i].drv == drv)
                                break;
                if (PCI_MAX_MAPPINGS == i)
                        continue;
                if (drv->remove)
                        drv->remove(dev);
                drvmap[i].dev = NULL;
                drvmap[i].drv = NULL;
        }
}

#endif
#endif

/*----------------------------------------------------------------
* prism2sta_open
*
* WLAN device open method.  Called from p80211netdev when kernel 
* device open (start) method is called in response to the 
* SIOCSIIFFLAGS ioctl changing the flags bit IFF_UP 
* from clear to set.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	0	success
*	>0	f/w reported error
*	<0	driver reported error
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_open(wlandevice_t *wlandev)
{
	DBFENTER;

#ifdef ANCIENT_MODULE_CODE
	MOD_INC_USE_COUNT;
#endif

	/* We don't currently have to do anything else. 
	 * The setup of the MAC should be subsequently completed via
	 * the mlme commands.
	 * Higher layers know we're ready from dev->start==1 and 
	 * dev->tbusy==0.  Our rx path knows to pass up received/
	 * frames because of dev->flags&IFF_UP is true. 
	 */

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2sta_close
*
* WLAN device close method.  Called from p80211netdev when kernel 
* device close method is called in response to the 
* SIOCSIIFFLAGS ioctl changing the flags bit IFF_UP 
* from set to clear.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	0	success
*	>0	f/w reported error
*	<0	driver reported error
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_close(wlandevice_t *wlandev)
{
	DBFENTER;

#ifdef ANCIENT_MODULE_CODE
	MOD_DEC_USE_COUNT;
#endif

	/* We don't currently have to do anything else.
	 * Higher layers know we're not ready from dev->start==0 and
	 * dev->tbusy==1.  Our rx path knows to not pass up received
	 * frames because of dev->flags&IFF_UP is false.
	 */

	DBFEXIT;
	return 0;
}


/*----------------------------------------------------------------
* prism2sta_reset
*
* Not currently implented.
*
* Arguments:
*	wlandev		wlan device structure
*	none
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
void prism2sta_reset(wlandevice_t *wlandev )
{
	DBFENTER;
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_txframe
*
* Takes a frame from p80211 and queues it for transmission.
*
* Arguments:
*	wlandev		wlan device structure
*	pb		packet buffer struct.  Contains an 802.11
*			data frame.
*       p80211_hdr      points to the 802.11 header for the packet.
* Returns: 
*	0		Success and more buffs available
*	1		Success but no more buffs
*	2		Allocation failure
*	4		Buffer full or queue busy
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_txframe(wlandevice_t *wlandev, struct sk_buff *skb,
		      p80211_hdr_t *p80211_hdr, p80211_metawep_t *p80211_wep)
{
	hfa384x_t		*hw = (hfa384x_t *)wlandev->priv;
	int			result;
	DBFENTER;

	/* If necessary, set the 802.11 WEP bit */
	if ((wlandev->hostwep & (HOSTWEP_PRIVACYINVOKED | HOSTWEP_ENCRYPT)) == HOSTWEP_PRIVACYINVOKED) {
		p80211_hdr->a3.fc |= host2ieee16(WLAN_SET_FC_ISWEP(1));
	}

	result = hfa384x_drvr_txframe(hw, skb, p80211_hdr, p80211_wep);

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_mlmerequest
*
* wlan command message handler.  All we do here is pass the message
* over to the prism2sta_mgmt_handler.
*
* Arguments:
*	wlandev		wlan device structure
*	msg		wlan command message
* Returns: 
*	0		success
*	<0		successful acceptance of message, but we're
*			waiting for an async process to finish before
*			we're done with the msg.  When the asynch
*			process is done, we'll call the p80211 
*			function p80211req_confirm() .
*	>0		An error occurred while we were handling
*			the message.
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_mlmerequest(wlandevice_t *wlandev, p80211msg_t *msg)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;

	int result = 0;
	DBFENTER;

	switch( msg->msgcode )
	{
	case DIDmsg_dot11req_mibget :
		WLAN_LOG_DEBUG(2,"Received mibget request\n");
		result = prism2mgmt_mibset_mibget(wlandev, msg);
		break;
	case DIDmsg_dot11req_mibset :
		WLAN_LOG_DEBUG(2,"Received mibset request\n");
		result = prism2mgmt_mibset_mibget(wlandev, msg);
		break;
	case DIDmsg_dot11req_powermgmt :
		WLAN_LOG_DEBUG(2,"Received powermgmt request\n");
		result = prism2mgmt_powermgmt(wlandev, msg);
		break;
	case DIDmsg_dot11req_scan :
		WLAN_LOG_DEBUG(2,"Received scan request\n");
		result = prism2mgmt_scan(wlandev, msg);
		break;
	case DIDmsg_dot11req_scan_results :
		WLAN_LOG_DEBUG(2,"Received scan_results request\n");
		result = prism2mgmt_scan_results(wlandev, msg);
		break;
	case DIDmsg_dot11req_join :
		WLAN_LOG_DEBUG(2,"Received join request\n");
		result = prism2mgmt_join(wlandev, msg);
		break;
	case DIDmsg_dot11req_authenticate :
		WLAN_LOG_DEBUG(2,"Received authenticate request\n");
		result = prism2mgmt_authenticate(wlandev, msg);
		break;
	case DIDmsg_dot11req_deauthenticate :
		WLAN_LOG_DEBUG(2,"Received mlme deauthenticate request\n");
		result = prism2mgmt_deauthenticate(wlandev, msg);
		break;
	case DIDmsg_dot11req_associate :
		WLAN_LOG_DEBUG(2,"Received mlme associate request\n");
		result = prism2mgmt_associate(wlandev, msg);
		break;
	case DIDmsg_dot11req_reassociate :
		WLAN_LOG_DEBUG(2,"Received mlme reassociate request\n");
		result = prism2mgmt_reassociate(wlandev, msg);
		break;
	case DIDmsg_dot11req_disassociate :
		WLAN_LOG_DEBUG(2,"Received mlme disassociate request\n");
		result = prism2mgmt_disassociate(wlandev, msg);
		break;
	case DIDmsg_dot11req_reset :
		WLAN_LOG_DEBUG(2,"Received mlme reset request\n");
		result = prism2mgmt_reset(wlandev, msg);
		break;
	case DIDmsg_dot11req_start :
		WLAN_LOG_DEBUG(2,"Received mlme start request\n");
		result = prism2mgmt_start(wlandev, msg);
		break;
	/*
	 * Prism2 specific messages
	 */
	case DIDmsg_p2req_join :
		WLAN_LOG_DEBUG(2,"Received p2 join request\n");
		result = prism2mgmt_p2_join(wlandev, msg);
		break;
       	case DIDmsg_p2req_readpda :
		WLAN_LOG_DEBUG(2,"Received mlme readpda request\n");
		result = prism2mgmt_readpda(wlandev, msg);
		break;
	case DIDmsg_p2req_readcis :
		WLAN_LOG_DEBUG(2,"Received mlme readcis request\n");
		result = prism2mgmt_readcis(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_state :
		WLAN_LOG_DEBUG(2,"Received mlme auxport_state request\n");
		result = prism2mgmt_auxport_state(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_read :
		WLAN_LOG_DEBUG(2,"Received mlme auxport_read request\n");
		result = prism2mgmt_auxport_read(wlandev, msg);
		break;
	case DIDmsg_p2req_auxport_write :
		WLAN_LOG_DEBUG(2,"Received mlme auxport_write request\n");
		result = prism2mgmt_auxport_write(wlandev, msg);
		break;
	case DIDmsg_p2req_low_level :
		WLAN_LOG_DEBUG(2,"Received mlme low_level request\n");
		result = prism2mgmt_low_level(wlandev, msg);
		break;
        case DIDmsg_p2req_test_command :
                WLAN_LOG_DEBUG(2,"Received mlme test_command request\n");
                result = prism2mgmt_test_command(wlandev, msg);
                break;
        case DIDmsg_p2req_mmi_read :
                WLAN_LOG_DEBUG(2,"Received mlme mmi_read request\n");
                result = prism2mgmt_mmi_read(wlandev, msg);
                break;
        case DIDmsg_p2req_mmi_write :
                WLAN_LOG_DEBUG(2,"Received mlme mmi_write request\n");
                result = prism2mgmt_mmi_write(wlandev, msg);
                break;
	case DIDmsg_p2req_ramdl_state :
		WLAN_LOG_DEBUG(2,"Received mlme ramdl_state request\n");
		result = prism2mgmt_ramdl_state(wlandev, msg);
		break;
	case DIDmsg_p2req_ramdl_write :
		WLAN_LOG_DEBUG(2,"Received mlme ramdl_write request\n");
		result = prism2mgmt_ramdl_write(wlandev, msg);
		break;
	case DIDmsg_p2req_flashdl_state :
		WLAN_LOG_DEBUG(2,"Received mlme flashdl_state request\n");
		result = prism2mgmt_flashdl_state(wlandev, msg);
		break;
	case DIDmsg_p2req_flashdl_write :
		WLAN_LOG_DEBUG(2,"Received mlme flashdl_write request\n");
		result = prism2mgmt_flashdl_write(wlandev, msg);
		break;
	case DIDmsg_p2req_dump_state :
		WLAN_LOG_DEBUG(2,"Received mlme dump_state request\n");
		result = prism2mgmt_dump_state(wlandev, msg);
		break;
	case DIDmsg_p2req_channel_info :
		WLAN_LOG_DEBUG(2,"Received mlme channel_info request\n");
		result = prism2mgmt_channel_info(wlandev, msg);
		break;
	case DIDmsg_p2req_channel_info_results :
		WLAN_LOG_DEBUG(2,"Received mlme channel_info_results request\n");
		result = prism2mgmt_channel_info_results(wlandev, msg);
		break;
#ifdef ZD1201_CHIPSET
	case DIDmsg_p2req_zd1201_state :
		WLAN_LOG_DEBUG(2,"Received mlme zd1201_state request\n");
		result = prism2mgmt_zd1201_state(wlandev, msg);
		break;
	case DIDmsg_p2req_zd1201_fwwrite :
		WLAN_LOG_DEBUG(2,"Received mlme zd1201_fwwrite request\n");
		result = prism2mgmt_zd1201_fwwrite(wlandev, msg);
		break;
#endif        
	/*
	 * Linux specific messages
	 */
	case DIDmsg_lnxreq_hostwep : 
		break;   // ignore me.
        case DIDmsg_lnxreq_ifstate :
		{
		p80211msg_lnxreq_ifstate_t	*ifstatemsg;
                WLAN_LOG_DEBUG(2,"Received mlme ifstate request\n");
		ifstatemsg = (p80211msg_lnxreq_ifstate_t*)msg;
                result = prism2sta_ifstate(wlandev, ifstatemsg->ifstate.data);
		ifstatemsg->resultcode.status = 
			P80211ENUM_msgitem_status_data_ok;
		ifstatemsg->resultcode.data = result;
		result = 0;
		}
                break;
        case DIDmsg_lnxreq_wlansniff :
                WLAN_LOG_DEBUG(2,"Received mlme wlansniff request\n");
                result = prism2mgmt_wlansniff(wlandev, msg);
                break;
	case DIDmsg_lnxreq_autojoin :
		WLAN_LOG_DEBUG(2,"Received mlme autojoin request\n");
		result = prism2mgmt_autojoin(wlandev, msg);
		break;
	case DIDmsg_p2req_enable :
		WLAN_LOG_DEBUG(2,"Received mlme enable request\n");
		result = prism2mgmt_enable(wlandev, msg);
		break;
	case DIDmsg_lnxreq_commsquality: {
		p80211msg_lnxreq_commsquality_t *qualmsg;
		hfa384x_commsquality_t qual;

		WLAN_LOG_DEBUG(2,"Received commsquality request\n");

		if (hw->ap)
			break;

		qualmsg = (p80211msg_lnxreq_commsquality_t*) msg;

		result = hfa384x_drvr_getconfig(hw, 
						HFA384x_RID_COMMSQUALITY, 
						&qual, 
						HFA384x_RID_COMMSQUALITY_LEN);
		if (result != 0) {
			WLAN_LOG_ERROR("Failed to read %s statistics: error=%d\n",
					wlandev->name, result);
			break;
		}

		qualmsg->link.status = P80211ENUM_msgitem_status_data_ok;
		qualmsg->level.status = P80211ENUM_msgitem_status_data_ok;
		qualmsg->noise.status = P80211ENUM_msgitem_status_data_ok;

		if (qualmsg->dbm.data == P80211ENUM_truth_true) {
			qualmsg->link.data = hfa384x2host_16(qual.CQ_currBSS);
			qualmsg->level.data = hfa384x2host_16(HFA384x_LEVEL_TO_dBm(qual.ASL_currBSS));
			qualmsg->noise.data = hfa384x2host_16(HFA384x_LEVEL_TO_dBm(qual.ANL_currFC));
		} else {
			qualmsg->link.data = hfa384x2host_16(qual.CQ_currBSS);
			qualmsg->level.data = hfa384x2host_16(qual.ASL_currBSS);
			qualmsg->noise.data = hfa384x2host_16(qual.ANL_currFC);
		}

		break;
	}
	default:
		WLAN_LOG_WARNING("Unknown mgmt request message 0x%08lx", msg->msgcode);
		break;
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_ifstate
*
* Interface state.  This is the primary WLAN interface enable/disable
* handler.  Following the driver/load/deviceprobe sequence, this
* function must be called with a state of "enable" before any other
* commands will be accepted.
*
* Arguments:
*	wlandev		wlan device structure
*	msgp		ptr to msg buffer
*
* Returns: 
* 	A p80211 message resultcode value.
*
* Side effects:
*
* Call context:
*	process thread  (usually)
*	interrupt
----------------------------------------------------------------*/
UINT32 prism2sta_ifstate(wlandevice_t *wlandev, UINT32 ifstate)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	UINT32 			result;
	DBFENTER;

	result = P80211ENUM_resultcode_implementation_failure;

	switch (ifstate)
	{
	case P80211ENUM_ifstate_fwload:
		switch (wlandev->msdstate) {
		case WLAN_MSD_HWPRESENT:
			wlandev->msdstate = WLAN_MSD_FWLOAD_PENDING;
			/*
			 * Initialize the device+driver sufficiently
			 * for firmware loading.
			 */
#ifdef ZD1201_CHIPSET
            /*
             * If the firmware is not loaded yet, don't try to
             * execute cmd_initialize.  It will surely fail.
             * This function should be followed by a message to
             * halt the ZD1201.
             */
            if (!hw->zd1201_fwactive) {
            }
            else {
#endif            
#if (WLAN_HOSTIF != WLAN_USB)
			if ((result=hfa384x_cmd_initialize(hw))) {
				WLAN_LOG_ERROR(
					"hfa384x_cmd_initialize() failed,"
					"result=%d\n", (int)result);
				result = 
				P80211ENUM_resultcode_implementation_failure;
				wlandev->msdstate = WLAN_MSD_HWPRESENT;
				break;
			}
#else
			if ((result=hfa384x_drvr_start(hw))) {
				WLAN_LOG_ERROR(
					"hfa384x_drvr_start() failed,"
					"result=%d\n", (int)result);
				result = 
				P80211ENUM_resultcode_implementation_failure;
				wlandev->msdstate = WLAN_MSD_HWPRESENT;
				break;
			}
#endif
#ifdef ZD1201_CHIPSET
            }
#endif            
			wlandev->msdstate = WLAN_MSD_FWLOAD;
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_FWLOAD:
			/* Do nothing, we're already in this state.*/
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_RUNNING:
			WLAN_LOG_WARNING(
				"Cannot enter fwload state from enable state,"
				"you must disable first.\n");
			result = P80211ENUM_resultcode_invalid_parameters;
			break;
		case WLAN_MSD_HWFAIL:
		default:
			/* probe() had a problem or the msdstate contains
			 * an unrecognized value, there's nothing we can do.
			 */
			result = P80211ENUM_resultcode_implementation_failure;
			break;
		}
		break;
	case P80211ENUM_ifstate_enable:
		switch (wlandev->msdstate) {
		case WLAN_MSD_HWPRESENT:
		case WLAN_MSD_FWLOAD:
			wlandev->msdstate = WLAN_MSD_RUNNING_PENDING;
            
			/* Initialize the device+driver for full
			 * operation. Note that this might me an FWLOAD to
			 * to RUNNING transition so we must not do a chip
			 * or board level reset.  Note that on failure,
			 * the MSD state is set to HWPRESENT because we 
			 * can't make any assumptions about the state 
			 * of the hardware or a previous firmware load.
			 */
			if ((result=hfa384x_drvr_start(hw))) {
				WLAN_LOG_ERROR(
					"hfa384x_drvr_start() failed,"
					"result=%d\n", (int)result);
				result = 
				P80211ENUM_resultcode_implementation_failure;
				wlandev->msdstate = WLAN_MSD_HWPRESENT;
				break;
			}

			if ((result=prism2sta_getcardinfo(wlandev))) {
				WLAN_LOG_ERROR(
					"prism2sta_getcardinfo() failed,"
					"result=%d\n", (int)result);
				result = 
				P80211ENUM_resultcode_implementation_failure;
				hfa384x_drvr_stop(hw);
				wlandev->msdstate = WLAN_MSD_HWPRESENT;
				break;
			}
			if ((result=prism2sta_globalsetup(wlandev))) {
				WLAN_LOG_ERROR(
					"prism2sta_globalsetup() failed,"
					"result=%d\n", (int)result);
				result = 
				P80211ENUM_resultcode_implementation_failure;
				hfa384x_drvr_stop(hw);
				wlandev->msdstate = WLAN_MSD_HWPRESENT;
				break;
			}
			wlandev->msdstate = WLAN_MSD_RUNNING;
			hw->join_ap = 0;
			hw->join_retries = 60;
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_RUNNING:
			/* Do nothing, we're already in this state.*/
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_HWFAIL:
		default:
			/* probe() had a problem or the msdstate contains
			 * an unrecognized value, there's nothing we can do.
			 */
			result = P80211ENUM_resultcode_implementation_failure;
			break;
		}
		break;
	case P80211ENUM_ifstate_disable:
		switch (wlandev->msdstate) {
		case WLAN_MSD_HWPRESENT:
			/* Do nothing, we're already in this state.*/
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_FWLOAD:
		case WLAN_MSD_RUNNING:
			wlandev->msdstate = WLAN_MSD_HWPRESENT_PENDING;
			/*
			 * TODO: Shut down the MAC completely. Here a chip
			 * or board level reset is probably called for.
			 * After a "disable" _all_ results are lost, even
			 * those from a fwload. Note the memset of priv,
			 * if priv ever gets any pointers in it, we'll
			 * need to do a deep copy.
			 */
			if (wlandev->netdev)
				netif_stop_queue(wlandev->netdev);
			hfa384x_drvr_stop(hw);

			// XXX possibly  Clean out hw->stuff.

			wlandev->macmode = WLAN_MACMODE_NONE;
			wlandev->msdstate = WLAN_MSD_HWPRESENT;
			result = P80211ENUM_resultcode_success;
			break;
		case WLAN_MSD_HWFAIL:
		default:
			/* probe() had a problem or the msdstate contains
			 * an unrecognized value, there's nothing we can do.
			 */
			result = P80211ENUM_resultcode_implementation_failure;
			break;
		}
		break;
	default:
		result = P80211ENUM_resultcode_invalid_parameters;
		break;
	}

	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_getcardinfo
*
* Collect the NICID, firmware version and any other identifiers
* we'd like to have in host-side data structures.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	0	success
*	>0	f/w reported error
*	<0	driver reported error
*
* Side effects:
*
* Call context:
*	Either.
----------------------------------------------------------------*/
int prism2sta_getcardinfo(wlandevice_t *wlandev)
{
	int 			result = 0;
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	UINT16                  temp;
	UINT8			snum[HFA384x_RID_NICSERIALNUMBER_LEN];
	char			pstr[(HFA384x_RID_NICSERIALNUMBER_LEN * 4) + 1];

	DBFENTER;

	/* Collect version and compatibility info */
	/*  Some are critical, some are not */
	/* NIC identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_NICIDENTITY,
			&hw->ident_nic, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve NICIDENTITY\n");
		goto failed;
	}

	/* get all the nic id fields in host byte order */
	hw->ident_nic.id = hfa384x2host_16(hw->ident_nic.id);
	hw->ident_nic.variant = hfa384x2host_16(hw->ident_nic.variant);
	hw->ident_nic.major = hfa384x2host_16(hw->ident_nic.major);
	hw->ident_nic.minor = hfa384x2host_16(hw->ident_nic.minor);

	WLAN_LOG_INFO( "ident: nic h/w: id=0x%02x %d.%d.%d\n",
			hw->ident_nic.id, hw->ident_nic.major,
			hw->ident_nic.minor, hw->ident_nic.variant);

	/* Primary f/w identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRIIDENTITY,
			&hw->ident_pri_fw, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve PRIIDENTITY\n");
		goto failed;
	}

	/* get all the private fw id fields in host byte order */
	hw->ident_pri_fw.id = hfa384x2host_16(hw->ident_pri_fw.id);
	hw->ident_pri_fw.variant = hfa384x2host_16(hw->ident_pri_fw.variant);
	hw->ident_pri_fw.major = hfa384x2host_16(hw->ident_pri_fw.major);
	hw->ident_pri_fw.minor = hfa384x2host_16(hw->ident_pri_fw.minor);

	WLAN_LOG_INFO( "ident: pri f/w: id=0x%02x %d.%d.%d\n",
			hw->ident_pri_fw.id, hw->ident_pri_fw.major,
			hw->ident_pri_fw.minor, hw->ident_pri_fw.variant);

	/* Station (Secondary?) f/w identity */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STAIDENTITY,
			&hw->ident_sta_fw, sizeof(hfa384x_compident_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve STAIDENTITY\n");
		goto failed;
	}

#ifndef ZD1201_CHIPSET
	if (hw->ident_nic.id < 0x8000) {
		WLAN_LOG_ERROR("FATAL: Card is not an Intersil Prism2/2.5/3\n");
		result = -1;
		goto failed;
	}
#endif

	/* get all the station fw id fields in host byte order */
	hw->ident_sta_fw.id = hfa384x2host_16(hw->ident_sta_fw.id);
	hw->ident_sta_fw.variant = hfa384x2host_16(hw->ident_sta_fw.variant);
	hw->ident_sta_fw.major = hfa384x2host_16(hw->ident_sta_fw.major);
	hw->ident_sta_fw.minor = hfa384x2host_16(hw->ident_sta_fw.minor);

	/* strip out the 'special' variant bits */
	hw->mm_mods = hw->ident_sta_fw.variant & (BIT14 | BIT15);
	hw->ident_sta_fw.variant &= ~((UINT16)(BIT14 | BIT15));

#ifndef ZD1201_CHIPSET
	if  ( hw->ident_sta_fw.id == 0x1f ) {
#else
	/* Force the card into station mode for ZD1201 */
	if  ( 1 )  {
#endif
		hw->ap = 0;
		WLAN_LOG_INFO( 
			"ident: sta f/w: id=0x%02x %d.%d.%d\n",
			hw->ident_sta_fw.id, hw->ident_sta_fw.major,
			hw->ident_sta_fw.minor, hw->ident_sta_fw.variant);
	} else {
		hw->ap = 1;
		WLAN_LOG_INFO(
			"ident:  ap f/w: id=0x%02x %d.%d.%d\n",
			hw->ident_sta_fw.id, hw->ident_sta_fw.major,
			hw->ident_sta_fw.minor, hw->ident_sta_fw.variant);
	}

	/* Compatibility range, Modem supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_MFISUPRANGE,
			&hw->cap_sup_mfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve MFISUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, modem interface supplier
	fields in byte order */
	hw->cap_sup_mfi.role = hfa384x2host_16(hw->cap_sup_mfi.role);
	hw->cap_sup_mfi.id = hfa384x2host_16(hw->cap_sup_mfi.id);
	hw->cap_sup_mfi.variant = hfa384x2host_16(hw->cap_sup_mfi.variant);
	hw->cap_sup_mfi.bottom = hfa384x2host_16(hw->cap_sup_mfi.bottom);
	hw->cap_sup_mfi.top = hfa384x2host_16(hw->cap_sup_mfi.top);

	WLAN_LOG_INFO(
		"MFI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_sup_mfi.role, hw->cap_sup_mfi.id,
		hw->cap_sup_mfi.variant, hw->cap_sup_mfi.bottom,
		hw->cap_sup_mfi.top);

	/* Compatibility range, Controller supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CFISUPRANGE,
			&hw->cap_sup_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve CFISUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, controller interface supplier
	fields in byte order */
	hw->cap_sup_cfi.role = hfa384x2host_16(hw->cap_sup_cfi.role);
	hw->cap_sup_cfi.id = hfa384x2host_16(hw->cap_sup_cfi.id);
	hw->cap_sup_cfi.variant = hfa384x2host_16(hw->cap_sup_cfi.variant);
	hw->cap_sup_cfi.bottom = hfa384x2host_16(hw->cap_sup_cfi.bottom);
	hw->cap_sup_cfi.top = hfa384x2host_16(hw->cap_sup_cfi.top);

	WLAN_LOG_INFO( 
		"CFI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_sup_cfi.role, hw->cap_sup_cfi.id,
		hw->cap_sup_cfi.variant, hw->cap_sup_cfi.bottom,
		hw->cap_sup_cfi.top);

	/* Compatibility range, Primary f/w supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRISUPRANGE,
			&hw->cap_sup_pri, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve PRISUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, primary firmware supplier
	fields in byte order */
	hw->cap_sup_pri.role = hfa384x2host_16(hw->cap_sup_pri.role);
	hw->cap_sup_pri.id = hfa384x2host_16(hw->cap_sup_pri.id);
	hw->cap_sup_pri.variant = hfa384x2host_16(hw->cap_sup_pri.variant);
	hw->cap_sup_pri.bottom = hfa384x2host_16(hw->cap_sup_pri.bottom);
	hw->cap_sup_pri.top = hfa384x2host_16(hw->cap_sup_pri.top);

	WLAN_LOG_INFO(
		"PRI:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_sup_pri.role, hw->cap_sup_pri.id,
		hw->cap_sup_pri.variant, hw->cap_sup_pri.bottom,
		hw->cap_sup_pri.top);
	
	/* Compatibility range, Station f/w supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STASUPRANGE,
			&hw->cap_sup_sta, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve STASUPRANGE\n");
		goto failed;
	}

	/* get all the Compatibility range, station firmware supplier
	fields in byte order */
	hw->cap_sup_sta.role = hfa384x2host_16(hw->cap_sup_sta.role);
	hw->cap_sup_sta.id = hfa384x2host_16(hw->cap_sup_sta.id);
	hw->cap_sup_sta.variant = hfa384x2host_16(hw->cap_sup_sta.variant);
	hw->cap_sup_sta.bottom = hfa384x2host_16(hw->cap_sup_sta.bottom);
	hw->cap_sup_sta.top = hfa384x2host_16(hw->cap_sup_sta.top);

	if ( hw->cap_sup_sta.id == 0x04 ) {
		WLAN_LOG_INFO(
		"STA:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_sup_sta.role, hw->cap_sup_sta.id,
		hw->cap_sup_sta.variant, hw->cap_sup_sta.bottom,
		hw->cap_sup_sta.top);
	} else {
		WLAN_LOG_INFO(
		"AP:SUP:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_sup_sta.role, hw->cap_sup_sta.id,
		hw->cap_sup_sta.variant, hw->cap_sup_sta.bottom,
		hw->cap_sup_sta.top);
	}

	/* Compatibility range, primary f/w actor, CFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_PRI_CFIACTRANGES,
			&hw->cap_act_pri_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve PRI_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, primary f/w actor, CFI supplier
	fields in byte order */
	hw->cap_act_pri_cfi.role = hfa384x2host_16(hw->cap_act_pri_cfi.role);
	hw->cap_act_pri_cfi.id = hfa384x2host_16(hw->cap_act_pri_cfi.id);
	hw->cap_act_pri_cfi.variant = hfa384x2host_16(hw->cap_act_pri_cfi.variant);
	hw->cap_act_pri_cfi.bottom = hfa384x2host_16(hw->cap_act_pri_cfi.bottom);
	hw->cap_act_pri_cfi.top = hfa384x2host_16(hw->cap_act_pri_cfi.top);

	WLAN_LOG_INFO(
		"PRI-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_act_pri_cfi.role, hw->cap_act_pri_cfi.id,
		hw->cap_act_pri_cfi.variant, hw->cap_act_pri_cfi.bottom,
		hw->cap_act_pri_cfi.top);
	
	/* Compatibility range, sta f/w actor, CFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_CFIACTRANGES,
			&hw->cap_act_sta_cfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve STA_CFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, station f/w actor, CFI supplier
	fields in byte order */
	hw->cap_act_sta_cfi.role = hfa384x2host_16(hw->cap_act_sta_cfi.role);
	hw->cap_act_sta_cfi.id = hfa384x2host_16(hw->cap_act_sta_cfi.id);
	hw->cap_act_sta_cfi.variant = hfa384x2host_16(hw->cap_act_sta_cfi.variant);
	hw->cap_act_sta_cfi.bottom = hfa384x2host_16(hw->cap_act_sta_cfi.bottom);
	hw->cap_act_sta_cfi.top = hfa384x2host_16(hw->cap_act_sta_cfi.top);

	WLAN_LOG_INFO(
		"STA-CFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_act_sta_cfi.role, hw->cap_act_sta_cfi.id,
		hw->cap_act_sta_cfi.variant, hw->cap_act_sta_cfi.bottom,
		hw->cap_act_sta_cfi.top);

	/* Compatibility range, sta f/w actor, MFI supplier */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_STA_MFIACTRANGES,
			&hw->cap_act_sta_mfi, sizeof(hfa384x_caplevel_t));
	if ( result ) {
		WLAN_LOG_ERROR("Failed to retrieve STA_MFIACTRANGES\n");
		goto failed;
	}

	/* get all the Compatibility range, station f/w actor, MFI supplier
	fields in byte order */
	hw->cap_act_sta_mfi.role = hfa384x2host_16(hw->cap_act_sta_mfi.role);
	hw->cap_act_sta_mfi.id = hfa384x2host_16(hw->cap_act_sta_mfi.id);
	hw->cap_act_sta_mfi.variant = hfa384x2host_16(hw->cap_act_sta_mfi.variant);
	hw->cap_act_sta_mfi.bottom = hfa384x2host_16(hw->cap_act_sta_mfi.bottom);
	hw->cap_act_sta_mfi.top = hfa384x2host_16(hw->cap_act_sta_mfi.top);

	WLAN_LOG_INFO(
		"STA-MFI:ACT:role=0x%02x:id=0x%02x:var=0x%02x:b/t=%d/%d\n",
		hw->cap_act_sta_mfi.role, hw->cap_act_sta_mfi.id,
		hw->cap_act_sta_mfi.variant, hw->cap_act_sta_mfi.bottom,
		hw->cap_act_sta_mfi.top);

	/* Serial Number */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_NICSERIALNUMBER,
			snum, HFA384x_RID_NICSERIALNUMBER_LEN);
	if ( !result ) {
		wlan_mkprintstr(snum, HFA384x_RID_NICSERIALNUMBER_LEN,
				pstr, sizeof(pstr));
		WLAN_LOG_INFO("Prism2 card SN: %s\n", pstr);
	} else {
		WLAN_LOG_ERROR("Failed to retrieve Prism2 Card SN\n");
		goto failed;
	}

	/* Collect the MAC address */
	result = hfa384x_drvr_getconfig(hw, HFA384x_RID_CNFOWNMACADDR, 
		wlandev->netdev->dev_addr, WLAN_ADDR_LEN);
	if ( result != 0 ) {
		WLAN_LOG_ERROR("Failed to retrieve mac address\n");
		goto failed;
	}

	/* short preamble is always implemented */
	wlandev->nsdcaps |= P80211_NSDCAP_SHORT_PREAMBLE;

	/* find out if hardware wep is implemented */
	hfa384x_drvr_getconfig16(hw, HFA384x_RID_PRIVACYOPTIMP, &temp);
	if (temp)
		wlandev->nsdcaps |= P80211_NSDCAP_HARDWAREWEP;

	/* Only enable scan by default on newer firmware */
        if (HFA384x_FIRMWARE_VERSION(hw->ident_sta_fw.major,
                                     hw->ident_sta_fw.minor,
                                     hw->ident_sta_fw.variant) <
	    HFA384x_FIRMWARE_VERSION(1,5,5)) {
		wlandev->nsdcaps |= P80211_NSDCAP_NOSCAN;
	}

	/* TODO: Set any internally managed config items */

	goto done;
failed:
	WLAN_LOG_ERROR("Failed, result=%d\n", result);
done:
	DBFEXIT;
	return result;
}


/*----------------------------------------------------------------
* prism2sta_globalsetup
*
* Set any global RIDs that we want to set at device activation.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	0	success
*	>0	f/w reported error
*	<0	driver reported error
*
* Side effects:
*
* Call context:
*	process thread
----------------------------------------------------------------*/
int prism2sta_globalsetup(wlandevice_t *wlandev)
{
	hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;

	/* Set the maximum frame size */
	return hfa384x_drvr_setconfig16(hw, HFA384x_RID_CNFMAXDATALEN,
	                                    WLAN_DATA_MAXLEN);
}

int prism2sta_setmulticast(wlandevice_t *wlandev, netdevice_t *dev)
{
	int result = 0, i = 0;
	hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	UINT16  promisc;
	hfa384x_GroupAddresses_t grp;
	struct dev_mc_list *element = dev->mc_list;

	DBFENTER;

	/* If we're not ready, what's the point? */
	if ( hw->state != HFA384x_STATE_RUNNING )
		goto exit;

	/* If we're an AP, do nothing here */
	if (hw->ap)
		goto exit;

	if ( (dev->flags & (IFF_PROMISC | IFF_ALLMULTI)) != 0 )
		promisc = P80211ENUM_truth_true;
	else
		promisc = P80211ENUM_truth_false;

	result = hfa384x_drvr_setconfig16_async(
	                              hw, HFA384x_RID_PROMISCMODE, promisc);
	if (result)
		goto exit;
	
	/* Setup Multicast list */
	while (element != NULL && 
			i < sizeof(grp.MACAddress)/sizeof(grp.MACAddress[0])) {
		memcpy(grp.MACAddress[i], element->dmi_addr, WLAN_ADDR_LEN);
		element = element->next;
		i++;
	}

	if (i) {
		memcpy(hw->dot11_grp_addr, &grp.MACAddress, 
				sizeof(grp.MACAddress[0]) * i);
		result = hfa384x_drvr_setconfig(hw, HFA384x_RID_GROUPADDR, 
					&grp, sizeof(grp.MACAddress[0]) * i);
	}
 exit:
	DBFEXIT;
	return result;
}

/*----------------------------------------------------------------
* prism2sta_inf_handover
*
* Handles the receipt of a Handover info frame. Should only be present
* in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_handover(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	WLAN_LOG_DEBUG(2,"received infoframe:HANDOVER (unhandled)\n");
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_inf_tallies
*
* Handles the receipt of a CommTallies info frame. 
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_tallies(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	UINT16			*src16;
	UINT32			*dst;
	UINT32			*src32;
	int			i;
	int			cnt;

	DBFENTER;

	/*
	** Determine if these are 16-bit or 32-bit tallies, based on the
	** record length of the info record.
	*/

	cnt = sizeof(hfa384x_CommTallies32_t) / sizeof(UINT32);
	if (inf->framelen > 22) {
		dst   = (UINT32 *) &hw->tallies;
		src32 = (UINT32 *) &inf->info.commtallies32;
		for (i = 0; i < cnt; i++, dst++, src32++)
			*dst += hfa384x2host_32(*src32);
	} else {
		dst   = (UINT32 *) &hw->tallies;
		src16 = (UINT16 *) &inf->info.commtallies16;
		for (i = 0; i < cnt; i++, dst++, src16++)
			*dst += hfa384x2host_16(*src16);
	}

	DBFEXIT;

	return;
}

/*----------------------------------------------------------------
* prism2sta_inf_scanresults
*
* Handles the receipt of a Scan Results info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_scanresults(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{

        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	int			nbss;
	hfa384x_ScanResult_t	*sr = &(inf->info.scanresult);
	int			i;
	hfa384x_JoinRequest_data_t	joinreq;
	int			result;
	DBFENTER;

	/* Get the number of results, first in bytes, then in results */
	nbss = (inf->framelen * sizeof(UINT16)) - 
		sizeof(inf->infotype) -
		sizeof(inf->info.scanresult.scanreason);
	nbss /= sizeof(hfa384x_ScanResultSub_t);

	/* Print em */
	WLAN_LOG_DEBUG(1,"rx scanresults, reason=%d, nbss=%d:\n",
		inf->info.scanresult.scanreason, nbss);
	for ( i = 0; i < nbss; i++) {
		WLAN_LOG_DEBUG(1, "chid=%d anl=%d sl=%d bcnint=%d\n",
			sr->result[i].chid,
			sr->result[i].anl,
			sr->result[i].sl,
			sr->result[i].bcnint);
		WLAN_LOG_DEBUG(1, "  capinfo=0x%04x proberesp_rate=%d\n",
			sr->result[i].capinfo,
			sr->result[i].proberesp_rate);
	}
	/* issue a join request */
	joinreq.channel = sr->result[0].chid;
	memcpy( joinreq.bssid, sr->result[0].bssid, WLAN_BSSID_LEN);
	result = hfa384x_drvr_setconfig( hw, 
			HFA384x_RID_JOINREQUEST,
			&joinreq, HFA384x_RID_JOINREQUEST_LEN);
	if (result) {
		WLAN_LOG_ERROR("setconfig(joinreq) failed, result=%d\n", result);
	}
	
	DBFEXIT;
	return;
}

/*----------------------------------------------------------------
* prism2sta_inf_hostscanresults
*
* Handles the receipt of a Scan Results info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_hostscanresults(wlandevice_t *wlandev, 
				   hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	int			nbss;
	DBFENTER;

	nbss = (inf->framelen - 3) / 32;
	WLAN_LOG_DEBUG(2, "Received %d hostscan results\n", nbss);

	if (hw->scanresults)
		kfree(hw->scanresults);

	hw->scanresults = kmalloc(sizeof(hfa384x_InfFrame_t), GFP_ATOMIC);
	memcpy(hw->scanresults, inf, sizeof(hfa384x_InfFrame_t));

	if (nbss == 0)
		nbss = -1;

        /* Notify/wake the sleeping caller. */
        hw->scanflag = nbss;
        wake_up_interruptible(&hw->cmdq);

	DBFEXIT;
};

/*----------------------------------------------------------------
* prism2sta_inf_chinforesults
*
* Handles the receipt of a Channel Info Results info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_chinforesults(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	unsigned int		i, n;

	DBFENTER;
	hw->channel_info.results.scanchannels = 
		hfa384x2host_16(inf->info.chinforesult.scanchannels);
#if 0
	memcpy(&inf->info.chinforesult, &hw->channel_info.results, sizeof(hfa384x_ChInfoResult_t));
#endif

	for (i=0, n=0; i<HFA384x_CHINFORESULT_MAX; i++) {
		if (hw->channel_info.results.scanchannels & (1<<i)) {
			int 	channel=hfa384x2host_16(inf->info.chinforesult.result[n].chid)-1;
			hfa384x_ChInfoResultSub_t *chinforesult=&hw->channel_info.results.result[channel];
			chinforesult->chid   = channel;
			chinforesult->anl    = hfa384x2host_16(inf->info.chinforesult.result[n].anl);
			chinforesult->pnl    = hfa384x2host_16(inf->info.chinforesult.result[n].pnl);
			chinforesult->active = hfa384x2host_16(inf->info.chinforesult.result[n].active);
			WLAN_LOG_DEBUG(2, "chinfo: channel %d, %s level (avg/peak)=%d/%d dB, pcf %d\n",
					channel+1, 
					chinforesult->active & 
					HFA384x_CHINFORESULT_BSSACTIVE ? "signal" : "noise", 
					chinforesult->anl, chinforesult->pnl, 
					chinforesult->active & HFA384x_CHINFORESULT_PCFACTIVE ? 1 : 0
			);
			n++;
		}
	}
	atomic_set(&hw->channel_info.done, 2);

	hw->channel_info.count = n;
	DBFEXIT;
	return;
}
#ifdef DECLARE_TASKLET
void prism2sta_processing_defer(unsigned long data)
#else
void prism2sta_processing_defer(void *data)
#endif
{
	hfa384x_t		*hw = (hfa384x_t *) data;
	wlandevice_t            *wlandev = hw->wlandev;
	hfa384x_bytestr32_t ssid;

	DBFENTER;
	/* First let's process the auth frames */
	{
		struct sk_buff          *skb;
		hfa384x_InfFrame_t *inf;
		
		while ( (skb = skb_dequeue(&hw->authq)) ) {
			inf = (hfa384x_InfFrame_t *) skb->data;
			prism2sta_inf_authreq_defer(wlandev, inf);
		}

	}

	/* Now let's handle the linkstatus stuff */
	if (hw->link_status == hw->link_status_new)
		goto failed;

	hw->link_status = hw->link_status_new;

	switch(hw->link_status) {
	case HFA384x_LINK_NOTCONNECTED:
		/* I'm currently assuming that this is the initial link 
		 * state.  It should only be possible immediately
		 * following an Enable command.
		 * Response:
		 * Block Transmits, Ignore receives of data frames
		 */
		WLAN_LOG_INFO("linkstatus=NOTCONNECTED (unhandled)\n");
		break;

	case HFA384x_LINK_CONNECTED:
		/* This one indicates a successful scan/join/auth/assoc.
		 * When we have the full MLME complement, this event will
		 * signify successful completion of both mlme_authenticate
		 * and mlme_associate.  State management will get a little
		 * ugly here.
		 * Response:
		 * Indicate authentication and/or association
		 * Enable Transmits, Receives and pass up data frames
		 */

		/* If we are joining a specific AP, set our state and reset retries */
		if(hw->join_ap == 1)
			hw->join_ap = 2;
		hw->join_retries = 60;

		/* Don't call this in monitor mode */
		if ( wlandev->netdev->type == ARPHRD_ETHER ) {
		  UINT16			portstatus;
		  int			result;

		  WLAN_LOG_INFO("linkstatus=CONNECTED\n");

		  /* For non-usb devices, we can use the sync versions */
		  /* Collect the BSSID, and set state to allow tx */
		  
		  result = hfa384x_drvr_getconfig(hw, 
				HFA384x_RID_CURRENTBSSID,
				wlandev->bssid, WLAN_BSSID_LEN);
		  if ( result ) {
			WLAN_LOG_DEBUG(1,
				"getconfig(0x%02x) failed, result = %d\n",
				HFA384x_RID_CURRENTBSSID, result);
			goto failed;
		  }

		  result = hfa384x_drvr_getconfig(hw, 
						  HFA384x_RID_CURRENTSSID,
						  &ssid, sizeof(ssid));
		  if ( result ) {
			  WLAN_LOG_DEBUG(1,
					 "getconfig(0x%02x) failed, result = %d\n",
					 HFA384x_RID_CURRENTSSID, result);
			  goto failed;
		  }
		  prism2mgmt_bytestr2pstr((hfa384x_bytestr_t *)&ssid, 
					  (p80211pstrd_t *) &wlandev->ssid);

		  /* Collect the port status */
		  result = hfa384x_drvr_getconfig16(hw, 
				HFA384x_RID_PORTSTATUS, &portstatus);
		  if ( result ) {
			WLAN_LOG_DEBUG(1,
				"getconfig(0x%02x) failed, result = %d\n",
				HFA384x_RID_PORTSTATUS, result);
			goto failed;
		  }
		  portstatus = hfa384x2host_16(portstatus);
		  wlandev->macmode = 
			(portstatus == HFA384x_PSTATUS_CONN_IBSS) ?
			WLAN_MACMODE_IBSS_STA : WLAN_MACMODE_ESS_STA;
		}
		break;

	case HFA384x_LINK_DISCONNECTED:
		/* This one indicates that our association is gone.  We've
		 * lost connection with the AP and/or been disassociated.  
		 * This indicates that the MAC has completely cleared it's
		 * associated state.  We * should send a deauth indication 
		 * (implying disassoc) up * to the MLME.
		 * Response:
		 * Indicate Deauthentication
		 * Block Transmits, Ignore receives of data frames
		 */
		if(hw->join_ap == 2)
		{
			hfa384x_JoinRequest_data_t      joinreq;
			joinreq = hw->joinreq;
			/* Send the join request */
			hfa384x_drvr_setconfig( hw, 
				HFA384x_RID_JOINREQUEST,
				&joinreq, HFA384x_RID_JOINREQUEST_LEN);
			WLAN_LOG_INFO("linkstatus=DISCONNECTED (re-submitting join)\n");
		}
		else
		{
		  if (wlandev->netdev->type == ARPHRD_ETHER)
		    WLAN_LOG_INFO("linkstatus=DISCONNECTED (unhandled)\n");
		}

		break;

	case HFA384x_LINK_AP_CHANGE:
		/* This one indicates that the MAC has decided to and 
		 * successfully completed a change to another AP.  We
		 * should probably implement a reassociation indication 
		 * in response to this one.  I'm thinking that the the 
		 * p80211 layer needs to be notified in case of 
		 * buffering/queueing issues.  User mode also needs to be
		 * notified so that any BSS dependent elements can be 
		 * updated.
		 * associated state.  We * should send a deauth indication 
		 * (implying disassoc) up * to the MLME.
		 * Response:
		 * Indicate Reassociation
		 * Enable Transmits, Receives and pass up data frames
		 */
		WLAN_LOG_INFO("linkstatus=AP_CHANGE (unhandled)\n");
		break;

	case HFA384x_LINK_AP_OUTOFRANGE:
		/* This one indicates that the MAC has decided that the
		 * AP is out of range, but hasn't found a better candidate
		 * so the MAC maintains its "associated" state in case
		 * we get back in range.  We should block transmits and 
		 * receives in this state.  Do we need an indication here?
		 * Probably not since a polling user-mode element would 
		 * get this status from from p2PortStatus(FD40). What about
		 * p80211?
		 * Response:
		 * Block Transmits, Ignore receives of data frames
		 */
		WLAN_LOG_INFO("linkstatus=AP_OUTOFRANGE (unhandled)\n");
		break;

	case HFA384x_LINK_AP_INRANGE:
		/* This one indicates that the MAC has decided that the
		 * AP is back in range.  We continue working with our 
		 * existing association.
		 * Response:
		 * Enable Transmits, Receives and pass up data frames
		 */
		WLAN_LOG_INFO("linkstatus=AP_INRANGE (unhandled)\n");
		break;

	case HFA384x_LINK_ASSOCFAIL:
		/* This one is actually a peer to CONNECTED.  We've 
		 * requested a join for a given SSID and optionally BSSID.
		 * We can use this one to indicate authentication and 
		 * association failures.  The trick is going to be 
		 * 1) identifying the failure, and 2) state management.
		 * Response:
		 * Disable Transmits, Ignore receives of data frames
		 */
		if(hw->join_ap && --hw->join_retries > 0)
		{
			hfa384x_JoinRequest_data_t      joinreq;
			joinreq = hw->joinreq;
			/* Send the join request */
			hfa384x_drvr_setconfig( hw, 
				HFA384x_RID_JOINREQUEST,
				&joinreq, HFA384x_RID_JOINREQUEST_LEN);
			WLAN_LOG_INFO("linkstatus=ASSOCFAIL (re-submitting join)\n");
		}
		else
		{
			WLAN_LOG_INFO("linkstatus=ASSOCFAIL (unhandled)\n");
		}
		break;

	default:
		/* This is bad, IO port problems? */
		WLAN_LOG_WARNING( 
			"unknown linkstatus=0x%02x\n", hw->link_status);
		goto failed;
		break;
	}

 failed:
	DBFEXIT;
}

/*----------------------------------------------------------------
* prism2sta_inf_linkstatus
*
* Handles the receipt of a Link Status info frame.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_linkstatus(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;

	DBFENTER;

	hw->link_status_new = hfa384x2host_16(inf->info.linkstatus.linkstatus);

	tasklet_schedule(&hw->link_bh);

	DBFEXIT;
	return;
}

/*----------------------------------------------------------------
* prism2sta_inf_assocstatus
*
* Handles the receipt of an Association Status info frame. Should 
* be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_assocstatus(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	hfa384x_AssocStatus_t	rec;
	int			i;

	DBFENTER;

	memcpy(&rec, &inf->info.assocstatus, sizeof(rec));
	rec.assocstatus = hfa384x2host_16(rec.assocstatus);
	rec.reason      = hfa384x2host_16(rec.reason);

	/*
	** Find the address in the list of authenticated stations.  If it wasn't
	** found, then this address has not been previously authenticated and
	** something weird has happened if this is anything other than an
	** "authentication failed" message.  If the address was found, then
	** set the "associated" flag for that station, based on whether the
	** station is associating or losing its association.  Something weird
	** has also happened if we find the address in the list of authenticated
	** stations but we are getting an "authentication failed" message.
	*/

	for (i = 0; i < hw->authlist.cnt; i++)
		if (memcmp(rec.sta_addr, hw->authlist.addr[i], WLAN_ADDR_LEN) == 0)
			break;

	if (i >= hw->authlist.cnt) {
		if (rec.assocstatus != HFA384x_ASSOCSTATUS_AUTHFAIL)
			WLAN_LOG_WARNING("assocstatus info frame received for non-authenticated station.\n");
	} else {
		hw->authlist.assoc[i] =
			(rec.assocstatus == HFA384x_ASSOCSTATUS_STAASSOC ||
			 rec.assocstatus == HFA384x_ASSOCSTATUS_REASSOC);

		if (rec.assocstatus == HFA384x_ASSOCSTATUS_AUTHFAIL)
			WLAN_LOG_WARNING("authfail assocstatus info frame received for authenticated station.\n");
	}

	DBFEXIT;

	return;
}

/*----------------------------------------------------------------
* prism2sta_inf_authreq
*
* Handles the receipt of an Authentication Request info frame. Should 
* be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
*
----------------------------------------------------------------*/
void prism2sta_inf_authreq( wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	struct sk_buff *skb;
	
	DBFENTER;

	skb = dev_alloc_skb(sizeof(*inf));
	if (skb) {
		skb_put(skb, sizeof(*inf));
		memcpy(skb->data, inf, sizeof(*inf));
		skb_queue_tail(&hw->authq, skb);
		tasklet_schedule(&hw->link_bh);
	}		

	DBFEXIT;
}

void prism2sta_inf_authreq_defer(wlandevice_t *wlandev, 
				 hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
	hfa384x_authenticateStation_data_t  rec;

	int    i, added, result, cnt;
	UINT8  *addr;

	DBFENTER;

	/*
	** Build the AuthenticateStation record.  Initialize it for denying
	** authentication.
	*/

	memcpy(rec.address, inf->info.authreq.sta_addr, WLAN_ADDR_LEN);
	rec.status = P80211ENUM_status_unspec_failure;

	/*
	** Authenticate based on the access mode.
	*/

	switch (hw->accessmode) {
		case WLAN_ACCESS_NONE:

			/*
			** Deny all new authentications.  However, if a station
			** is ALREADY authenticated, then accept it.
			*/

			for (i = 0; i < hw->authlist.cnt; i++)
				if (memcmp(rec.address, hw->authlist.addr[i],
						WLAN_ADDR_LEN) == 0) {
					rec.status = P80211ENUM_status_successful;
					break;
				}

			break;

		case WLAN_ACCESS_ALL:

			/*
			** Allow all authentications.
			*/

			rec.status = P80211ENUM_status_successful;
			break;

		case WLAN_ACCESS_ALLOW:

			/*
			** Only allow the authentication if the MAC address
			** is in the list of allowed addresses.
			**
			** Since this is the interrupt handler, we may be here
			** while the access list is in the middle of being
			** updated.  Choose the list which is currently okay.
			** See "prism2mib_priv_accessallow()" for details.
			*/

			if (hw->allow.modify == 0) {
				cnt  = hw->allow.cnt;
				addr = hw->allow.addr[0];
			} else {
				cnt  = hw->allow.cnt1;
				addr = hw->allow.addr1[0];
			}

			for (i = 0; i < cnt; i++, addr += WLAN_ADDR_LEN)
				if (memcmp(rec.address, addr, WLAN_ADDR_LEN) == 0) {
					rec.status = P80211ENUM_status_successful;
					break;
				}

			break;

		case WLAN_ACCESS_DENY:

			/*
			** Allow the authentication UNLESS the MAC address is
			** in the list of denied addresses.
			**
			** Since this is the interrupt handler, we may be here
			** while the access list is in the middle of being
			** updated.  Choose the list which is currently okay.
			** See "prism2mib_priv_accessdeny()" for details.
			*/

			if (hw->deny.modify == 0) {
				cnt  = hw->deny.cnt;
				addr = hw->deny.addr[0];
			} else {
				cnt  = hw->deny.cnt1;
				addr = hw->deny.addr1[0];
			}

			rec.status = P80211ENUM_status_successful;

			for (i = 0; i < cnt; i++, addr += WLAN_ADDR_LEN)
				if (memcmp(rec.address, addr, WLAN_ADDR_LEN) == 0) {
					rec.status = P80211ENUM_status_unspec_failure;
					break;
				}

			break;
	}

	/*
	** If the authentication is okay, then add the MAC address to the list
	** of authenticated stations.  Don't add the address if it is already in
	** the list.  (802.11b does not seem to disallow a station from issuing
	** an authentication request when the station is already authenticated.
	** Does this sort of thing ever happen?  We might as well do the check
	** just in case.)
	*/

	added = 0;

	if (rec.status == P80211ENUM_status_successful) {
		for (i = 0; i < hw->authlist.cnt; i++)
			if (memcmp(rec.address, hw->authlist.addr[i], WLAN_ADDR_LEN) == 0)
				break;

		if (i >= hw->authlist.cnt) {
			if (hw->authlist.cnt >= WLAN_AUTH_MAX) {
				rec.status = P80211ENUM_status_ap_full;
			} else {
				memcpy(hw->authlist.addr[hw->authlist.cnt],
					rec.address, WLAN_ADDR_LEN);
				hw->authlist.cnt++;
				added = 1;
			}
		}
	}

	/*
	** Send back the results of the authentication.  If this doesn't work,
	** then make sure to remove the address from the authenticated list if
	** it was added.
	*/

	rec.status = host2hfa384x_16(rec.status);
	rec.algorithm = inf->info.authreq.algorithm;

	result = hfa384x_drvr_setconfig(hw, HFA384x_RID_AUTHENTICATESTA,
							&rec, sizeof(rec));
	if (result) {
		if (added) hw->authlist.cnt--;
		WLAN_LOG_ERROR("setconfig(authenticatestation) failed, result=%d\n", result);
	}

	DBFEXIT;

	return;
}


/*----------------------------------------------------------------
* prism2sta_inf_psusercnt
*
* Handles the receipt of a PowerSaveUserCount info frame. Should 
* be present in APs only.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to info frame (contents in hfa384x order)
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_inf_psusercnt( wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;

	DBFENTER;

	hw->psusercount = hfa384x2host_16(inf->info.psusercnt.usercnt);

	DBFEXIT;

	return;
}

/*----------------------------------------------------------------
* prism2sta_ev_dtim
*
* Handles the DTIM early warning event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_dtim(wlandevice_t *wlandev)
{
#if 0
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG(3, "DTIM event, currently unhandled.\n");
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_ev_infdrop
*
* Handles the InfDrop event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_infdrop(wlandevice_t *wlandev)
{
#if 0
        hfa384x_t               *hw = (hfa384x_t *)wlandev->priv;
#endif
	DBFENTER;
	WLAN_LOG_DEBUG(3, "Info frame dropped due to card mem low.\n");
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_ev_info
*
* Handles the Info event.
*
* Arguments:
*	wlandev		wlan device structure
*	inf		ptr to a generic info frame
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_info(wlandevice_t *wlandev, hfa384x_InfFrame_t *inf)
{
	DBFENTER;
	inf->infotype = hfa384x2host_16(inf->infotype);
	/* Dispatch */
	switch ( inf->infotype ) {
		case HFA384x_IT_HANDOVERADDR:
			prism2sta_inf_handover(wlandev, inf);
			break;
		case HFA384x_IT_COMMTALLIES:
			prism2sta_inf_tallies(wlandev, inf);
			break;
               case HFA384x_IT_HOSTSCANRESULTS:
                        prism2sta_inf_hostscanresults(wlandev, inf);
                        break;
		case HFA384x_IT_SCANRESULTS:
			prism2sta_inf_scanresults(wlandev, inf);
			break;
		case HFA384x_IT_CHINFORESULTS:
			prism2sta_inf_chinforesults(wlandev, inf);
			break;
		case HFA384x_IT_LINKSTATUS:
			prism2sta_inf_linkstatus(wlandev, inf);
			break;
		case HFA384x_IT_ASSOCSTATUS:
			prism2sta_inf_assocstatus(wlandev, inf);
			break;
		case HFA384x_IT_AUTHREQ:
			prism2sta_inf_authreq(wlandev, inf);
			break;
		case HFA384x_IT_PSUSERCNT:
			prism2sta_inf_psusercnt(wlandev, inf);
			break;
	        case HFA384x_IT_KEYIDCHANGED:
			WLAN_LOG_WARNING("Unhandled IT_KEYIDCHANGED\n");
			break;
	        case HFA384x_IT_ASSOCREQ:
			WLAN_LOG_WARNING("Unhandled IT_ASSOCREQ\n");
			break;
	        case HFA384x_IT_MICFAILURE:
			WLAN_LOG_WARNING("Unhandled IT_MICFAILURE\n");
			break;
		default:
			WLAN_LOG_WARNING(
				"Unknown info type=0x%02x\n", inf->infotype);
			break;
	}
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_ev_txexc
*
* Handles the TxExc event.  A Transmit Exception event indicates
* that the MAC's TX process was unsuccessful - so the packet did
* not get transmitted.
*
* Arguments:
*	wlandev		wlan device structure
*	status		tx frame status word
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_txexc(wlandevice_t *wlandev, UINT16 status)
{
	DBFENTER;

	WLAN_LOG_DEBUG(3, "TxExc status=0x%x.\n", status);

	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_ev_tx
*
* Handles the Tx event.
*
* Arguments:
*	wlandev		wlan device structure
*	status		tx frame status word
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_tx(wlandevice_t *wlandev, UINT16 status)
{
	DBFENTER;
	WLAN_LOG_DEBUG(4, "Tx Complete, status=0x%04x\n", status);
	/* update linux network stats */
	wlandev->linux_stats.tx_packets++;
	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_ev_rx
*
* Handles the Rx event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_rx(wlandevice_t *wlandev, struct sk_buff *skb)
{
	DBFENTER;

	p80211netdev_rx(wlandev, skb);

	DBFEXIT;
	return;
}

/*----------------------------------------------------------------
* prism2sta_ev_alloc
*
* Handles the Alloc event.
*
* Arguments:
*	wlandev		wlan device structure
*
* Returns: 
*	nothing
*
* Side effects:
*
* Call context:
*	interrupt
----------------------------------------------------------------*/
void prism2sta_ev_alloc(wlandevice_t *wlandev)
{
	DBFENTER;

	p80211netdev_wake_queue(wlandev);

	DBFEXIT;
	return;
}

#if (WLAN_HOSTIF == WLAN_PCMCIA)
/*----------------------------------------------------------------
* prism2sta_attach
*
* Half of the attach/detach pair.  Creates and registers a device
* instance with Card Services.  In this case, it also creates the
* wlandev structure and device private structure.  These are 
* linked to the device instance via its priv member.
*
* Arguments:
*	none
*
* Returns: 
*	A valid ptr to dev_link_t on success, NULL otherwise
*
* Side effects:
*	
*
* Call context:
*	process thread (insmod/init_module/register_pccard_driver)
----------------------------------------------------------------*/
dev_link_t *prism2sta_attach(void)
{
	client_reg_t		client_reg;
	int			result;
	dev_link_t		*link = NULL;
	wlandevice_t		*wlandev = NULL;
	hfa384x_t		*hw = NULL;

	DBFENTER;

	/* Alloc our structures */
	link =		kmalloc(sizeof(struct dev_link_t), GFP_KERNEL);

	if (!link || ((wlandev = create_wlan()) == NULL)) {
		WLAN_LOG_ERROR("%s: Memory allocation failure.\n", dev_info);
		result = -EIO;
		goto failed;
	}
	hw = wlandev->priv;

	/* Clear all the structs */
	memset(link, 0, sizeof(struct dev_link_t));

	if ( wlan_setup(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: wlan_setup() failed.\n", dev_info);
		result = -EIO;
		goto failed;
	}

	/* Initialize the device private data stucture. */
	hw->cs_link = link;

	/* Initialize the PC card device object. */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
	init_timer(&link->release);
	link->release.function = &prism2sta_release;
	link->release.data = (u_long)link;
#endif
	link->conf.IntType = INT_MEMORY_AND_IO;
	link->priv = wlandev;
#if CS_RELEASE_CODE > 0x2911
	link->irq.Instance = wlandev;
#endif

	/* Link in to the list of devices managed by this driver */
	link->next = dev_list;
	dev_list = link;	

	/* Register with Card Services */
	client_reg.dev_info = &dev_info;
	client_reg.Attributes = INFO_IO_CLIENT | INFO_CARD_SHARE;
	client_reg.EventMask =
		CS_EVENT_CARD_INSERTION | CS_EVENT_CARD_REMOVAL |
		CS_EVENT_RESET_REQUEST |
		CS_EVENT_RESET_PHYSICAL | CS_EVENT_CARD_RESET |
		CS_EVENT_PM_SUSPEND | CS_EVENT_PM_RESUME;
	client_reg.event_handler = &prism2sta_event;
	client_reg.Version = 0x0210;
	client_reg.event_callback_args.client_data = link;

	result = CardServices(RegisterClient, &link->handle, &client_reg);
	if (result != 0) {
		cs_error(link->handle, RegisterClient, result);
		prism2sta_detach(link);
		return NULL;
	}

	goto done;

 failed:
	if (link)	kfree(link);
	if (wlandev)	kfree(wlandev);
	if (hw)		kfree(hw);
	link = NULL;

 done:
	DBFEXIT;
	return link;
}


/*----------------------------------------------------------------
* prism2sta_detach
*
* Remove one of the device instances managed by this driver.
*   Search the list for the given instance, 
*   check our flags for a waiting timer'd release call
*   call release
*   Deregister the instance with Card Services
*   (netdevice) unregister the network device.
*   unlink the instance from the list
*   free the link, priv, and priv->priv memory
* Note: the dev_list variable is a driver scoped static used to
*	maintain a list of device instances managed by this
*	driver.
*
* Arguments:
*	link	ptr to the instance to detach
*
* Returns: 
*	nothing
*
* Side effects:
*	the link structure is gone, the netdevice is gone
*
* Call context:
*	Might be interrupt, don't block.
----------------------------------------------------------------*/
void prism2sta_detach(dev_link_t *link)
{
	dev_link_t		**linkp;
	wlandevice_t		*wlandev;
	hfa384x_t		*hw;

	DBFENTER;

	/* Locate prev device structure */
	for (linkp = &dev_list; *linkp; linkp = &(*linkp)->next) {
		if (*linkp == link) break;
	}

	if (*linkp != NULL) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
		unsigned long	flags;
		/* Get rid of any timer'd release call */
		save_flags(flags);
		cli();
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		if (link->state & DEV_RELEASE_PENDING) {
			del_timer_sync(&link->release);
			link->state &= ~DEV_RELEASE_PENDING;
		}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0))
		restore_flags(flags);
#endif

		/* If link says we're still config'd, call release */
		if (link->state & DEV_CONFIG) {
			prism2sta_release((u_long)link);
			if (link->state & DEV_STALE_CONFIG) {
				link->state |= DEV_STALE_LINK;
				return;
			}
		}
		
		/* Tell Card Services we're not around any more */
		if (link->handle) {
			CardServices(DeregisterClient, link->handle);
		}	

		/* Unlink device structure, free bits */
		*linkp = link->next;
		if ( link->priv != NULL ) {
			wlandev = (wlandevice_t*)link->priv;
			p80211netdev_hwremoved(wlandev);
			if (link->dev != NULL) {
				unregister_wlandev(wlandev);
			}
			wlan_unsetup(wlandev);
			if (wlandev->priv) {
				hw = wlandev->priv;
				wlandev->priv = NULL;
				hfa384x_destroy(hw);
				kfree(hw);
			}
			link->priv = NULL;
			kfree(wlandev);
		}
		kfree(link);
	}

	DBFEXIT;
	return;
}


/*----------------------------------------------------------------
* prism2sta_config
*
* Half of the config/release pair.  Usually called in response to
* a card insertion event.  At this point, we _know_ there's some
* physical device present.  That means we can start poking around
* at the CIS and at any device specific config data we want.
*
* Note the gotos and the macros.  I recoded this once without
* them, and it got incredibly ugly.  It's actually simpler with
* them.
*
* Arguments:
*	link	the dev_link_t structure created in attach that 
*		represents this device instance.
*
* Returns: 
*	nothing
*
* Side effects:
*	Resources (irq, io, mem) are allocated
*	The pcmcia dev_link->node->name is set
*	(For netcards) The device structure is finished and,
*	  most importantly, registered.  This means that there
*	  is now a _named_ device that can be configured from
*	  userland.
*
* Call context:
*	May be called from a timer.  Don't block!
----------------------------------------------------------------*/
#define CS_CHECK(fn, args...) \
while ((last_ret=CardServices(last_fn=(fn), args))!=0) goto cs_failed;

#if defined(WLAN_INCLUDE_DEBUG)
#define CFG_CHECK(fn, args...) \
if ((last_ret=CardServices(last_fn=(fn), args))!=0) {  \
	WLAN_LOG_DEBUG(1,"CFG_CHECK failed\n"); \
	cs_error(link->handle, last_fn, last_ret); \
	goto next_entry; \
}
#else
#define CFG_CHECK(fn, args...) if (CardServices(fn, args)!=0) goto next_entry;
#endif 

void prism2sta_config(dev_link_t *link)
{
	client_handle_t		handle;
	wlandevice_t		*wlandev;
	hfa384x_t               *hw;
	int			last_fn;
	int			last_ret;
	tuple_t			tuple;
	cisparse_t		parse;
	config_info_t		socketconf;
	UINT8			buf[64];
	int			i;
	int			minVcc = 0;
	int			maxVcc = 0;
	cistpl_cftable_entry_t	dflt = { 0 };

	DBFENTER;

	handle = link->handle;
	wlandev = (wlandevice_t*)link->priv;
	hw = wlandev->priv;

	/* Collect the config register info */
	tuple.DesiredTuple = CISTPL_CONFIG;
	tuple.Attributes = 0;
	tuple.TupleData = buf;
	tuple.TupleDataMax = sizeof(buf);
	tuple.TupleOffset = 0;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	CS_CHECK(GetTupleData, handle, &tuple);
	CS_CHECK(ParseTuple, handle, &tuple, &parse);
	link->conf.ConfigBase = parse.config.base;
	link->conf.Present = parse.config.rmask[0];
	
	/* Configure card */
	link->state |= DEV_CONFIG;

	/* Acquire the current socket config (need Vcc setting) */
	CS_CHECK(GetConfigurationInfo, handle, &socketconf);

	/* Loop through the config table entries until we find one that works */
	/* Assumes a complete and valid CIS */
	tuple.DesiredTuple = CISTPL_CFTABLE_ENTRY;
	CS_CHECK(GetFirstTuple, handle, &tuple);
	while (1) {
		cistpl_cftable_entry_t *cfg = &(parse.cftable_entry);
		CFG_CHECK(GetTupleData, handle, &tuple);
		CFG_CHECK(ParseTuple, handle, &tuple, &parse);

		if (cfg->index == 0) goto next_entry;
		link->conf.ConfigIndex = cfg->index;

		/* Lets print out the Vcc that the controller+pcmcia-cs set
		 * for us, cause that's what we're going to use.
		 */
		WLAN_LOG_DEBUG(1,"Initial Vcc=%d/10v\n", socketconf.Vcc);
		if (prism2_ignorevcc) {
			link->conf.Vcc = socketconf.Vcc;
			goto skipvcc;
		}

		/* Use power settings for Vcc and Vpp if present */
		/* Note that the CIS values need to be rescaled */
		if (cfg->vcc.present & (1<<CISTPL_POWER_VNOM)) {
			WLAN_LOG_DEBUG(1, "Vcc obtained from curtupl.VNOM\n");
			minVcc = maxVcc = 
				cfg->vcc.param[CISTPL_POWER_VNOM]/10000;
		} else if (dflt.vcc.present & (1<<CISTPL_POWER_VNOM)) {
			WLAN_LOG_DEBUG(1, "Vcc set from dflt.VNOM\n");
			minVcc = maxVcc = 
				dflt.vcc.param[CISTPL_POWER_VNOM]/10000;
		} else if ((cfg->vcc.present & (1<<CISTPL_POWER_VMAX)) &&
			   (cfg->vcc.present & (1<<CISTPL_POWER_VMIN)) ) {
			WLAN_LOG_DEBUG(1, "Vcc set from curtupl(VMIN,VMAX)\n");			minVcc = cfg->vcc.param[CISTPL_POWER_VMIN]/10000;
			maxVcc = cfg->vcc.param[CISTPL_POWER_VMAX]/10000;
		} else if ((dflt.vcc.present & (1<<CISTPL_POWER_VMAX)) &&
			   (dflt.vcc.present & (1<<CISTPL_POWER_VMIN)) ) {
			WLAN_LOG_DEBUG(1, "Vcc set from dflt(VMIN,VMAX)\n");
			minVcc = dflt.vcc.param[CISTPL_POWER_VMIN]/10000;
			maxVcc = dflt.vcc.param[CISTPL_POWER_VMAX]/10000;
		}

		if ( socketconf.Vcc >= minVcc && socketconf.Vcc <= maxVcc) {
			link->conf.Vcc = socketconf.Vcc;
		} else {
			/* [MSM]: Note that I've given up trying to change 
			 * the Vcc if a change is indicated.  It seems the
			 * system&socketcontroller&card vendors can't seem
			 * to get it right, so I'm tired of trying to hack
			 * my way around it.  pcmcia-cs does its best using 
			 * the voltage sense pins but sometimes the controller
			 * lies.  Then, even if we have a good read on the VS 
			 * pins, some system designs will silently ignore our
			 * requests to set the voltage.  Additionally, some 
			 * vendors have 3.3v indicated on their sense pins, 
			 * but 5v specified in the CIS or vice-versa.  I've
			 * had it.  My only recommendation is "let the buyer
			 * beware".  Your system might supply 5v to a 3v card
			 * (possibly causing damage) or a 3v capable system 
			 * might supply 5v to a 3v capable card (wasting 
			 * precious battery life).
			 * My only recommendation (if you care) is to get 
			 * yourself an extender card (I don't know where, I 
			 * have only one myself) and a meter and test it for
			 * yourself.
			 */
			goto next_entry;
		}
skipvcc:			
		WLAN_LOG_DEBUG(1, "link->conf.Vcc=%d\n", link->conf.Vcc);

		/* Do we need to allocate an interrupt? */
		/* HACK: due to a bad CIS....we ALWAYS need an interrupt */
		/* if (cfg->irq.IRQInfo1 || dflt.irq.IRQInfo1) */
			link->conf.Attributes |= CONF_ENABLE_IRQ;

		/* IO window settings */
		link->io.NumPorts1 = link->io.NumPorts2 = 0;
		if ((cfg->io.nwin > 0) || (dflt.io.nwin > 0)) {
			cistpl_io_t *io = (cfg->io.nwin) ? &cfg->io : &dflt.io;
			link->io.Attributes1 = IO_DATA_PATH_WIDTH_AUTO;
			if (!(io->flags & CISTPL_IO_8BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_16;
			if (!(io->flags & CISTPL_IO_16BIT))
				link->io.Attributes1 = IO_DATA_PATH_WIDTH_8;
			link->io.BasePort1 = io->win[0].base;
			if  ( link->io.BasePort1 != 0 ) {
				WLAN_LOG_WARNING(
				"Brain damaged CIS: hard coded iobase="
				"0x%x, try letting pcmcia_cs decide...\n",
				link->io.BasePort1 );
				link->io.BasePort1 = 0;
			}
			link->io.NumPorts1 = io->win[0].len;
			if (io->nwin > 1) {
				link->io.Attributes2 = link->io.Attributes1;
				link->io.BasePort2 = io->win[1].base;
				link->io.NumPorts2 = io->win[1].len;
			}
		}

		/* This reserves IO space but doesn't actually enable it */
		CFG_CHECK(RequestIO, link->handle, &link->io);

		/* If we got this far, we're cool! */
		break;

next_entry:
		if (cfg->flags & CISTPL_CFTABLE_DEFAULT)
			dflt = *cfg;
		CS_CHECK(GetNextTuple, handle, &tuple);
	}

	/* Allocate an interrupt line.  Note that this does not assign a */
	/* handler to the interrupt, unless the 'Handler' member of the */
	/* irq structure is initialized. */
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
	{
		link->irq.Attributes = IRQ_TYPE_EXCLUSIVE | IRQ_HANDLE_PRESENT;
		link->irq.IRQInfo1 = IRQ_INFO2_VALID | IRQ_LEVEL_ID;
		if (irq_list[0] == -1)
			link->irq.IRQInfo2 = irq_mask;
		else
			for (i=0; i<4; i++)
				link->irq.IRQInfo2 |= 1 << irq_list[i];
		link->irq.Handler = hfa384x_interrupt;
		link->irq.Instance = wlandev;
		CS_CHECK(RequestIRQ, link->handle, &link->irq);
	}

	/* This actually configures the PCMCIA socket -- setting up */
	/* the I/O windows and the interrupt mapping, and putting the */
	/* card and host interface into "Memory and IO" mode. */
	CS_CHECK(RequestConfiguration, link->handle, &link->conf);

	/* Fill the netdevice with this info */
	wlandev->netdev->irq = link->irq.AssignedIRQ;
	wlandev->netdev->base_addr = link->io.BasePort1;

	/* Report what we've done */
	WLAN_LOG_INFO("%s: index 0x%02x: Vcc %d.%d", 
		dev_info, link->conf.ConfigIndex, 
		link->conf.Vcc/10, link->conf.Vcc%10);
	if (link->conf.Vpp1)
		printk(", Vpp %d.%d", link->conf.Vpp1/10, link->conf.Vpp1%10);
	if (link->conf.Attributes & CONF_ENABLE_IRQ)
		printk(", irq %d", link->irq.AssignedIRQ);
	if (link->io.NumPorts1)
		printk(", io 0x%04x-0x%04x", link->io.BasePort1, link->io.BasePort1+link->io.NumPorts1-1);
	if (link->io.NumPorts2)
		printk(" & 0x%04x-0x%04x", link->io.BasePort2, link->io.BasePort2+link->io.NumPorts2-1);
	printk("\n");

	link->state &= ~DEV_CONFIG_PENDING;

	/* Let pcmcia know the device name */
	link->dev = &hw->node;

	/* Register the network device and get assigned a name */
	SET_MODULE_OWNER(wlandev->netdev);
	if (register_wlandev(wlandev) != 0) {
		WLAN_LOG_NOTICE("prism2sta_cs: register_wlandev() failed.\n");
		goto failed;
	}

	strcpy(hw->node.dev_name, wlandev->name);

	/* Any device custom config/query stuff should be done here */
	/* For a netdevice, we should at least grab the mac address */

	return;
cs_failed:
	cs_error(link->handle, last_fn, last_ret);
	WLAN_LOG_ERROR("NextTuple failure? It's probably a Vcc mismatch.\n");

failed:
	prism2sta_release((UINT32)link);
	return;
}



/*----------------------------------------------------------------
* prism2sta_release
*
* Half of the config/release pair.  Usually called in response to 
* a card ejection event.  Checks to make sure no higher layers
* are still (or think they are) using the card via the link->open
* field.  
*
* NOTE: Don't forget to increment the link->open variable in the 
*  device_open method, and decrement it in the device_close 
*  method.
*
* Arguments:
*	arg	a generic 32 bit variable.  It's the value that
*		we assigned to link->release.data in sta_attach().
*
* Returns: 
*	nothing
*
* Side effects:
*	All resources should be released after this function
*	executes and finds the device !open.
*
* Call context:
*	Possibly in a timer context.  Don't do anything that'll
*	block.
----------------------------------------------------------------*/
void prism2sta_release(UINT32 arg)
{
        dev_link_t	*link = (dev_link_t *)arg;

	DBFENTER;

	/* First thing we should do is get the MSD back to the
	 * HWPRESENT state.  I.e. everything quiescent.
	 */
	prism2sta_ifstate(link->priv, P80211ENUM_ifstate_disable);

        if (link->open) {
		/* TODO: I don't think we're even using this bit of code
		 * and I don't think it's hurting us at the moment.
		 */
                WLAN_LOG_DEBUG(1, 
			"prism2sta_cs: release postponed, '%s' still open\n",
			link->dev->dev_name);
                link->state |= DEV_STALE_CONFIG;
                return;
        }

        CardServices(ReleaseConfiguration, link->handle);
        CardServices(ReleaseIO, link->handle, &link->io);
        CardServices(ReleaseIRQ, link->handle, &link->irq);

        link->state &= ~(DEV_CONFIG | DEV_RELEASE_PENDING);

	DBFEXIT;
}



/*----------------------------------------------------------------
* prism2sta_event
*
* Handler for card services events.
*
* Arguments:
*	event		The event code
*	priority	hi/low - REMOVAL is the only hi
*	args		ptr to card services struct containing info about
*			pcmcia status
*
* Returns: 
*	Zero on success, non-zero otherwise
*
* Side effects:
*	
*
* Call context:
*	Both interrupt and process thread, depends on the event.
----------------------------------------------------------------*/
static int 
prism2sta_event (
	event_t event, 
	int priority, 
	event_callback_args_t *args)
{
	int			result = 0;
	dev_link_t		*link = (dev_link_t *) args->client_data;
	wlandevice_t		*wlandev = (wlandevice_t*)link->priv;
	hfa384x_t		*hw = NULL;
	dev_node_t	node;

	DBFENTER;

	if (wlandev) hw = wlandev->priv;

	switch (event)
	{
	case CS_EVENT_CARD_INSERTION:
		WLAN_LOG_DEBUG(5,"event is INSERTION\n");
		link->state |= DEV_PRESENT | DEV_CONFIG_PENDING;
		prism2sta_config(link);
		if (!(link->state & DEV_CONFIG)) {
			wlandev->netdev->irq = 0;
			WLAN_LOG_ERROR(
				"%s: Initialization failed!\n", dev_info);
			wlandev->msdstate = WLAN_MSD_HWFAIL;
			break;
		}

		memcpy(&node, &hw->node, sizeof(hw->node));
		hfa384x_create(hw, 
			wlandev->netdev->irq,
			wlandev->netdev->base_addr, 
			(UINT8*) link);
		hw->wlandev = wlandev;
		hw->cs_link = link;
		memcpy(&hw->node, &node, sizeof(hw->node));

		if (prism2_doreset) {
			result = hfa384x_corereset(hw, 
					prism2_reset_holdtime, 
					prism2_reset_settletime, 0);
			if ( result ) {
				WLAN_LOG_ERROR(
					"corereset() failed, result=%d.\n", 
					result);
				wlandev->msdstate = WLAN_MSD_HWFAIL;
				break;
			}
		}

#if 0
		/*
		 * TODO: test_hostif() not implemented yet.
		 */
		result = hfa384x_test_hostif(hw);
		if (result) {
			WLAN_LOG_ERROR(
			"test_hostif() failed, result=%d.\n", result);
			wlandev->msdstate = WLAN_MSD_HWFAIL;
			break;
		}
#endif

#ifdef ZD1201_CHIPSET
		/* Since the card was inserted, the firmware cannot
		 * be expected to be active.
		 */
		hw->zd1201_fwactive = 0;

#if 0        
		result = zydas_cmd_fw_download (hw);
		if (result) {
			WLAN_LOG_ERROR(
			"zydas_cmd_fw_download failed, result=%d.\n", result);
			wlandev->msdstate = WLAN_MSD_HWFAIL;
			break;
		}
#endif        
#endif      

		wlandev->msdstate = WLAN_MSD_HWPRESENT;
		break;

	case CS_EVENT_CARD_REMOVAL:
		WLAN_LOG_DEBUG(5,"event is REMOVAL\n");
		link->state &= ~DEV_PRESENT;

		if (wlandev) {
			p80211netdev_hwremoved(wlandev);
		}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		if (link->state & DEV_CONFIG)
		{
			link->release.expires = jiffies + (HZ/20);
			add_timer(&link->release);
		}
#endif
		break;
	case CS_EVENT_RESET_REQUEST:
		WLAN_LOG_DEBUG(5,"event is RESET_REQUEST\n");
		WLAN_LOG_NOTICE(
			"prism2 card reset not supported "
			"due to post-reset user mode configuration "
			"requirements.\n");
		WLAN_LOG_NOTICE(
			"  From user mode, use "
			"'cardctl suspend;cardctl resume' "
			"instead.\n");
		break;
	case CS_EVENT_RESET_PHYSICAL:
	case CS_EVENT_CARD_RESET:
		WLAN_LOG_WARNING("Rx'd CS_EVENT_RESET_xxx, should not "
			"be possible since RESET_REQUEST was denied.\n");
		break;

	case CS_EVENT_PM_SUSPEND:
		WLAN_LOG_DEBUG(5,"event is SUSPEND\n");
		link->state |= DEV_SUSPEND;
		if (link->state & DEV_CONFIG)
		{
			prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);
			CardServices(ReleaseConfiguration, link->handle);
		}
		break;

	case CS_EVENT_PM_RESUME:
		WLAN_LOG_DEBUG(5,"event is RESUME\n");
		link->state &= ~DEV_SUSPEND;
		if (link->state & DEV_CONFIG)
		{
			CardServices(
				RequestConfiguration, 
				link->handle, &link->conf);
		}
		break;
	}

	DBFEXIT;
	return 0;  /* noone else does anthing with the return value */
}
#endif /* WLAN_PCMCIA */

#if (WLAN_HOSTIF == WLAN_PCI)
/*----------------------------------------------------------------
* prism2sta_probe_pci
*
* Probe routine called when a PCI device w/ matching ID is found. 
* The ISL3874 implementation uses the following map:
*   BAR0: Prism2.x registers memory mapped, size=4k
* Here's the sequence:
*   - Allocate the PCI resources.  
*   - Read the PCMCIA attribute memory to make sure we have a WLAN card
*   - Reset the MAC 
*   - Initialize the netdev and wlan data
*   - Initialize the MAC
*
* Arguments:
*	pdev		ptr to pci device structure containing info about 
*			pci configuration.
*	id		ptr to the device id entry that matched this device.
*
* Returns: 
*	zero		- success
*	negative	- failed
*
* Side effects:
*	
*
* Call context:
*	process thread
*	
----------------------------------------------------------------*/
static int __devinit
prism2sta_probe_pci(
	struct pci_dev *pdev, 
	const struct pci_device_id *id)
{
	int		result;
	phys_t		phymem = 0;
	void		*mem = NULL;
        wlandevice_t    *wlandev = NULL;
	hfa384x_t	*hw = NULL;

	DBFENTER;

	/* Enable the pci device */
	if (pci_enable_device(pdev)) {
		WLAN_LOG_ERROR("%s: pci_enable_device() failed.\n", dev_info);
		result = -EIO;
		goto fail;
	}

	/* Figure out our resources */
	phymem = pci_resource_start(pdev, 0);

        if (!request_mem_region(phymem, pci_resource_len(pdev, 0), "Prism2")) {
		printk(KERN_ERR "prism2: Cannot reserve PCI memory region\n");
		result = -EIO;
		goto fail;
        }

	mem = ioremap(phymem, PCI_SIZE);
	if ( mem == 0 ) {
		WLAN_LOG_ERROR("%s: ioremap() failed.\n", dev_info);
		result = -EIO;
		goto fail;
	}

	/* Log the device */
        WLAN_LOG_INFO("A Prism2.5 PCI device found, "
		"phymem:0x%llx, irq:%d, mem:0x%p\n", 
		(unsigned long long)phymem, pdev->irq, mem);
	
	if ((wlandev = create_wlan()) == NULL) {
		WLAN_LOG_ERROR("%s: Memory allocation failure.\n", dev_info);
		result = -EIO;
		goto fail;
	}
	hw = wlandev->priv;
	
	if ( wlan_setup(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: wlan_setup() failed.\n", dev_info);
		result = -EIO;
		goto fail;
	}

	/* Setup netdevice's ability to report resources 
	 * Note: the netdevice was allocated by wlan_setup()
	 */
        wlandev->netdev->irq = pdev->irq;
        wlandev->netdev->mem_start = (unsigned long) mem;
        wlandev->netdev->mem_end = wlandev->netdev->mem_start + 
		pci_resource_len(pdev, 0);

	/* Register the wlandev, this gets us a name and registers the
	 * linux netdevice.
	 */
	SET_MODULE_OWNER(wlandev->netdev);
        if ( register_wlandev(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: register_wlandev() failed.\n", dev_info);
		result = -EIO;
		goto fail;
        }

#if 0
	/* TODO: Move this and an irq test into an hfa384x_testif() routine.
	 */
	outw(PRISM2STA_MAGIC, HFA384x_SWSUPPORT(wlandev->netdev->base_addr));
	reg=inw( HFA384x_SWSUPPORT(wlandev->netdev->base_addr));
	if ( reg != PRISM2STA_MAGIC ) {
		WLAN_LOG_ERROR("MAC register access test failed!\n");
		result = -EIO;
		goto fail;
	}		
#endif

	/* Initialize the hw data */
        hfa384x_create(hw, wlandev->netdev->irq, 0, mem);
	hw->wlandev = wlandev;

	/* Do a chip-level reset on the MAC */
	if (prism2_doreset) {
		result = hfa384x_corereset(hw, 
				prism2_reset_holdtime, 
				prism2_reset_settletime, 0);
		if (result != 0) {
			WLAN_LOG_ERROR(
				"%s: hfa384x_corereset() failed.\n", 
				dev_info);
			unregister_wlandev(wlandev);
			hfa384x_destroy(hw);
			result = -EIO;
			goto fail;
		}
	}

        pci_set_drvdata(pdev, wlandev);

	/* Shouldn't actually hook up the IRQ until we 
	 * _know_ things are alright.  A test routine would help.
	 */
       	request_irq(wlandev->netdev->irq, hfa384x_interrupt, 
		SA_SHIRQ, wlandev->name, wlandev);

	wlandev->msdstate = WLAN_MSD_HWPRESENT;

	result = 0;
	goto done;

 fail:
	if (wlandev)	kfree(wlandev);
	if (hw)		kfree(hw);
        if (mem)        iounmap((void *) mem);

 done:
	DBFEXIT;
	return result;
}

static void __devexit prism2sta_remove_pci(struct pci_dev *pdev)
{
       	wlandevice_t		*wlandev;
	hfa384x_t	*hw;

	wlandev = (wlandevice_t *) pci_get_drvdata(pdev);
	hw = wlandev->priv;

	p80211netdev_hwremoved(wlandev);

	/* reset hardware */
	prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);

        if (pdev->irq)
		free_irq(pdev->irq, wlandev);

	unregister_wlandev(wlandev);

	/* free local stuff */
	if (hw) {
		iounmap(hw->membase);
		hfa384x_destroy(hw);
		kfree(hw);
	}

	wlan_unsetup(wlandev);

        release_mem_region(pci_resource_start(pdev, 0),
                           pci_resource_len(pdev, 0));

        pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);

	kfree(wlandev);
}
#endif /* WLAN_PCI */

#if (WLAN_HOSTIF == WLAN_PLX || WLAN_HOSTIF == WLAN_PCI)
#ifdef CONFIG_PM
static int prism2sta_suspend_pci(struct pci_dev *pdev, u32 state)
{
       	wlandevice_t		*wlandev;

	wlandev = (wlandevice_t *) pci_get_drvdata(pdev);

	/* reset hardware */
	if (wlandev) {
		prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);
		p80211_suspend(wlandev);
	}

	// call a netif_device_detach(wlandev->netdev) ?

	return 0;
}

static int prism2sta_resume_pci (struct pci_dev *pdev)
{
       	wlandevice_t		*wlandev;

	wlandev = (wlandevice_t *) pci_get_drvdata(pdev);

	if (wlandev) {
		prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);
		p80211_resume(wlandev);
	}
		
        return 0;
}
#endif
#endif

#if (WLAN_HOSTIF == WLAN_PLX)
/*----------------------------------------------------------------
* prism2sta_probe_plx
*
* Probe routine called when a PCI device w/ matching ID is found. 
* This PLX implementation uses the following map:
*   BAR0: Unused
*   BAR1: ????
*   BAR2: PCMCIA attribute memory
*   BAR3: PCMCIA i/o space 
* Here's the sequence:
*   - Allocate the PCI resources.  
*   - Read the PCMCIA attribute memory to make sure we have a WLAN card
*   - Reset the MAC using the PCMCIA COR
*   - Initialize the netdev and wlan data
*   - Initialize the MAC
*
* Arguments:
*	pdev		ptr to pci device structure containing info about 
*			pci configuration.
*	id		ptr to the device id entry that matched this device.
*
* Returns: 
*	zero		- success
*	negative	- failed
*
* Side effects:
*	
*
* Call context:
*	process thread
*	
----------------------------------------------------------------*/
static int __devinit
prism2sta_probe_plx(
	struct pci_dev			*pdev, 
	const struct pci_device_id	*id)
{
	int		result;
        phys_t	pccard_ioaddr;
	phys_t  pccard_attr_mem;
        unsigned int    pccard_attr_len;
	UINT8 *attr_mem = NULL;
	UINT32		plx_addr; 
        wlandevice_t    *wlandev = NULL;
	hfa384x_t	*hw = NULL;
	int		reg;
        u32		regic;

	if (pci_enable_device(pdev))
		return -EIO;

	/* TMC7160 boards are special */
	if ((pdev->vendor == PCIVENDOR_NDC) && 
	    (pdev->device == PCIDEVICE_NCP130_ASIC)) {
		unsigned long delay;

		pccard_attr_mem = 0;
		pccard_ioaddr = pci_resource_start(pdev, 1);

		outb(0x45, pccard_ioaddr);
		delay = jiffies + 1*HZ;
		while (time_before(jiffies, delay));

		if (inb(pccard_ioaddr) != 0x45) {
			WLAN_LOG_ERROR("Initialize the TMC7160 failed. (0x%x)\n", inb(pccard_ioaddr));
			return -EIO;
		}
		
		pccard_ioaddr = pci_resource_start(pdev, 2);
		prism2_doreset = 0;

		WLAN_LOG_INFO("NDC NCP130 with TMC716(ASIC) PCI interface device found at io:0x%lx, irq:%d\n", pccard_ioaddr, pdev->irq);
		goto init;
	}

	/* Collect the resource requirements */
	pccard_attr_mem = pci_resource_start(pdev, 2);
	pccard_attr_len = pci_resource_len(pdev, 2);
        if (pccard_attr_len < PLX_MIN_ATTR_LEN)
		return -EIO;

	pccard_ioaddr = pci_resource_start(pdev, 3);

	/* bjoern: We need to tell the card to enable interrupts, in
	 * case the serial eprom didn't do this already. See the
	 * PLX9052 data book, p8-1 and 8-24 for reference.
	 * [MSM]: This bit of code came from the orinoco_cs driver.
	 */
	plx_addr = pci_resource_start(pdev, 1);

	regic = 0;
	regic = inl(plx_addr+PLX_INTCSR);
	if(regic & PLX_INTCSR_INTEN) {
		WLAN_LOG_DEBUG(1,
			"%s: Local Interrupt already enabled\n", dev_info);
	} else {
		regic |= PLX_INTCSR_INTEN;
		outl(regic, plx_addr+PLX_INTCSR);
		regic = inl(plx_addr+PLX_INTCSR);
		if(!(regic & PLX_INTCSR_INTEN)) {
			WLAN_LOG_ERROR(
				"%s: Couldn't enable Local Interrupts\n",
				dev_info);
			return -EIO;
		}
	}

	/* These assignments are here in case of future mappings for
	 * io space and irq that might be similar to ioremap 
	 */
        if (!request_mem_region(pccard_attr_mem, pci_resource_len(pdev, 2), "Prism2")) {
		WLAN_LOG_ERROR("%s: Couldn't reserve PCI memory region\n", dev_info);
		return -EIO;
        }

	attr_mem = ioremap(pccard_attr_mem, pccard_attr_len);

	WLAN_LOG_INFO("A PLX PCI/PCMCIA interface device found, "
		"phymem:0x%llx, phyio=0x%lx, irq:%d, "
		"mem: 0x%lx\n", 
		(unsigned long long)pccard_attr_mem, pccard_ioaddr, pdev->irq,
		(long)attr_mem);

	/* Verify whether PC card is present. 
	 * [MSM] This needs improvement, the right thing to do is
	 * probably to walk the CIS looking for the vendor and product
	 * IDs.  It would be nice if this could be tied in with the
	 * etc/pcmcia/wlan-ng.conf file.  Any volunteers?  ;-)
	 */
	if (
	attr_mem[0] != 0x01 || attr_mem[2] != 0x03 ||
	attr_mem[4] != 0x00 || attr_mem[6] != 0x00 ||
	attr_mem[8] != 0xFF || attr_mem[10] != 0x17 ||
	attr_mem[12] != 0x04 || attr_mem[14] != 0x67) {
		WLAN_LOG_ERROR("Prism2 PC card CIS is invalid.\n");
		return -EIO;
        }
        WLAN_LOG_INFO("A PCMCIA WLAN adapter was found.\n");

        /* Write COR to enable PC card */
	attr_mem[COR_OFFSET] = COR_VALUE; reg = attr_mem[COR_OFFSET];
	
 init:

	/*
	 * Now do everything the same as a PCI device
	 * [MSM] TODO: We could probably factor this out of pcmcia/pci/plx
	 * and perhaps usb.  Perhaps a task for another day.......
	 */

	if ((wlandev = create_wlan()) == NULL) {
		WLAN_LOG_ERROR("%s: Memory allocation failure.\n", dev_info);
		result = -EIO;
		goto failed;
	}

	hw = wlandev->priv;

	if ( wlan_setup(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: wlan_setup() failed.\n", dev_info);
		result = -EIO;
		goto failed;
	}

	/* Setup netdevice's ability to report resources 
	 * Note: the netdevice was allocated by wlan_setup()
	 */
        wlandev->netdev->irq = pdev->irq;
        wlandev->netdev->base_addr = pccard_ioaddr;
        wlandev->netdev->mem_start = (unsigned long)attr_mem;
        wlandev->netdev->mem_end = (unsigned long)attr_mem + pci_resource_len(pdev, 0);
	/* Register the wlandev, this gets us a name and registers the
	 * linux netdevice.
	 */
	SET_MODULE_OWNER(wlandev->netdev);
        if ( register_wlandev(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: register_wlandev() failed.\n", dev_info);
		result = -EIO;
		goto failed;
        }

#if 0
	/* TODO: Move this and an irq test into an hfa384x_testif() routine.
	 */
	outw(PRISM2STA_MAGIC, HFA384x_SWSUPPORT(wlandev->netdev->base_addr));
	reg=inw( HFA384x_SWSUPPORT(wlandev->netdev->base_addr));
	if ( reg != PRISM2STA_MAGIC ) {
		WLAN_LOG_ERROR("MAC register access test failed!\n");
 		result = -EIO;
		goto failed;
	}		
#endif

	/* Initialize the hw data */
        hfa384x_create(hw, wlandev->netdev->irq, pccard_ioaddr, attr_mem);
	hw->wlandev = wlandev;

	/* Do a chip-level reset on the MAC */
	if (prism2_doreset) {
		result = hfa384x_corereset(hw, 
				prism2_reset_holdtime, 
				prism2_reset_settletime, 0);
		if (result != 0) {
			unregister_wlandev(wlandev);
			hfa384x_destroy(hw);
			WLAN_LOG_ERROR(
				"%s: hfa384x_corereset() failed.\n", 
				dev_info);
			result = -EIO;
			goto failed;
		}
	}

	pci_set_drvdata(pdev, wlandev);

	/* Shouldn't actually hook up the IRQ until we 
	 * _know_ things are alright.  A test routine would help.
	 */
       	request_irq(wlandev->netdev->irq, hfa384x_interrupt, 
		SA_SHIRQ, wlandev->name, wlandev);

	wlandev->msdstate = WLAN_MSD_HWPRESENT;

	result = 0;

	goto done;
	
 failed:
	if (wlandev)	kfree(wlandev);
	if (hw)		kfree(hw);
        if (attr_mem)        iounmap((void *) attr_mem);
 done:
	DBFEXIT;
        return result;
}

static void __devexit prism2sta_remove_plx(struct pci_dev *pdev)
{
       	wlandevice_t		*wlandev;
	hfa384x_t               *hw;

	wlandev = (wlandevice_t *) pci_get_drvdata(pdev);
	hw = wlandev->priv;

	p80211netdev_hwremoved(wlandev);

	/* reset hardware */
	prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);

        if (pdev->irq)
		free_irq(pdev->irq, wlandev);

	unregister_wlandev(wlandev);

	/* free local stuff */
	if (hw) {
		iounmap(hw->membase);
		hfa384x_destroy(hw);
		kfree(hw);
	}
	wlan_unsetup(wlandev);

        release_mem_region(pci_resource_start(pdev, 2),
                           pci_resource_len(pdev, 2));

        pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);

	kfree(wlandev);
}

#endif /* WLAN_PLX */

#if (WLAN_HOSTIF == WLAN_USB)

/*----------------------------------------------------------------
* prism2sta_probe_usb
*
* Probe routine called by the USB subsystem.
*
* Arguments:
*	dev		ptr to the usb_device struct
*	ifnum		interface number being offered
*
* Returns: 
*	NULL		- we're not claiming the device+interface
*	non-NULL	- we are claiming the device+interface and
*			  this is a ptr to the data we want back
*			  when disconnect is called.
*
* Side effects:
*	
* Call context:
*	I'm not sure, assume it's interrupt.
*	
----------------------------------------------------------------*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
void __devinit *prism2sta_probe_usb(
	struct usb_device *dev, 
	unsigned int ifnum,
	const struct usb_device_id *id)
#else
int prism2sta_probe_usb(
	struct usb_interface *interface,
	const struct usb_device_id *id)
#endif
{

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	struct usb_interface *interface;
#else
	struct usb_device *dev;
#endif

	wlandevice_t	*wlandev = NULL;
	hfa384x_t	*hw = NULL;
	int              result = 0;

	DBFENTER;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	interface = &dev->actconfig->interface[ifnum];
#else
	dev = interface_to_usbdev(interface);
#endif
	

	if ((wlandev = create_wlan()) == NULL) {
		WLAN_LOG_ERROR("%s: Memory allocation failure.\n", dev_info);
		result = -EIO;
		goto failed;
	}
	hw = wlandev->priv;

	if ( wlan_setup(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: wlan_setup() failed.\n", dev_info);
		result = -EIO;
		goto failed;
	}	

	/* Register the wlandev, this gets us a name and registers the
	 * linux netdevice.
	 */
	SET_MODULE_OWNER(wlandev->netdev);
        if ( register_wlandev(wlandev) != 0 ) {
		WLAN_LOG_ERROR("%s: register_wlandev() failed.\n", dev_info);
		result = -EIO;
		goto failed;
        }

	/* Initialize the hw data */
        hfa384x_create(hw, dev);
	hw->wlandev = wlandev;

	/* set up the endpoints */
	hw->endp_in = 1;
	hw->endp_out = 2;

	/* Do a chip-level reset on the MAC */
	if (prism2_doreset) {
		result = hfa384x_corereset(hw, 
				prism2_reset_holdtime, 
				prism2_reset_settletime, 0);
		if (result != 0) {
			unregister_wlandev(wlandev);
			hfa384x_destroy(hw);
			result = -EIO;
			WLAN_LOG_ERROR(
				"%s: hfa384x_corereset() failed.\n", 
				dev_info);
			goto failed;
		}
	}

#ifndef NEW_MODULE_CODE
	usb_inc_dev_use(dev);
#endif
	wlandev->msdstate = WLAN_MSD_HWPRESENT;

	goto done;

 failed:
	if (wlandev)	kfree(wlandev);
	if (hw)		kfree(hw);
	wlandev = NULL;

 done:
	DBFEXIT;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	return wlandev;
#else
	usb_set_intfdata(interface, wlandev);
	return result;
#endif
}


/*----------------------------------------------------------------
* prism2sta_disconnect_usb
*
* Called when a device previously claimed by probe is removed 
* from the USB. 
*
* Arguments:
*	dev		ptr to the usb_device struct
*	ptr		ptr returned by probe() when the device
*                       was claimed.
*
* Returns: 
*	Nothing
*
* Side effects:
*	
* Call context:
*	process
----------------------------------------------------------------*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
void __devexit
prism2sta_disconnect_usb(struct usb_device *dev, void *ptr)
#else
void
prism2sta_disconnect_usb(struct usb_interface *interface)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	wlandevice_t		*wlandev;
#else
	wlandevice_t		*wlandev = (wlandevice_t*)ptr;
#endif

        DBFENTER;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	wlandev = (wlandevice_t *) usb_get_intfdata(interface);
#endif

	if ( wlandev != NULL ) {
		LIST_HEAD(cleanlist);
		struct list_head	*entry;
		struct list_head	*temp;

		hfa384x_t		*hw = wlandev->priv;

		if (!hw)
			goto exit;

		disable_irq(hw->irq);
		hw->usb_removed = 1;
		list_splice_init(&hw->ctlxq.finished, &cleanlist);
		list_splice_init(&hw->ctlxq.pending, &cleanlist);
		list_splice_init(&hw->ctlxq.active, &cleanlist);
		enable_irq(hw->irq);

		p80211netdev_hwremoved(wlandev);

		/* There's no hardware to shutdown, but the driver
		 * might have some tasks or tasklets that must be
		 * stopped before we can tear everything down.
		 */
		prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);

		/* Unlink the tx/rx URBs. This "removes the wheels"
		 * from the entire CTLX handling mechanism.
		 */
		usb_unlink_urb(&hw->rx_urb);
		usb_unlink_urb(&hw->tx_urb);

		flush_scheduled_work();

		/* Now we complete any outstanding commands
		 * and tell everyone who is waiting for their
		 * responses that we have shut down.
		 */
		list_for_each_safe(entry, temp, &cleanlist) {
			hfa384x_usbctlx_t	*ctlx;

			ctlx = list_entry(entry, hfa384x_usbctlx_t, list);

			/* kill the timers */
			del_timer_sync(&ctlx->resptimer);
			del_timer_sync(&ctlx->reqtimer);

			/* Unlink the CTLX OUT URB */
			usb_unlink_urb(&ctlx->outurb);

			if (ctlx->is_async) {
				/* We've stopped the BULK-IN pipe,
				 * the timers and the OUT URB. The
				 * only thing left to disable is
				 * the async completion handler and
				 * then we can destroy this CTLX.
				 */
				STOP_DEFERRED_TASK(ctlx->async_bh);
				complete(&ctlx->done);
				kfree(ctlx);
			}
			else {
				ctlx->state = CTLX_REQ_FAILED;
				ctlx->wanna_wakeup = 1;
				wake_up_interruptible(&hw->cmdq);

				/* We don't own this CTLX, so wait
				 * for its owner to destroy it.
				 */
				wait_for_completion(&ctlx->done);
			}
		}

		/* Unhook the wlandev */
		unregister_wlandev(wlandev);
		wlan_unsetup(wlandev);

#ifndef NEW_MODULE_CODE
		usb_dec_dev_use(hw->usb);
#endif

		hfa384x_destroy(hw);
		kfree(hw);

		kfree(wlandev);
	}

 exit:

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	usb_set_intfdata(interface, NULL);
#endif
	DBFEXIT;
}

#endif /* WLAN_USB */


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#if (WLAN_HOSTIF == WLAN_MMIO)

static int __devinit mmio_register (void)
{
   wlandevice_t	*wlandev = NULL;
   hfa384x_t *hw = NULL;
   unsigned long zd1201_regs_virt;
#ifdef CONFIG_ARCH_SSA
   struct file *file = NULL;
   int ofs = 0, ret = 1;
#endif

   DBFENTER;

#ifdef CONFIG_ARCH_SSA
#ifdef CONFIG_SSA_HAS7752

   zd1201_regs_virt = (unsigned long) IO_ADDRESS_PCCARD_REGS_BASE;

#else

   if ((check_region (SSA_NCS2_BASE, ZD1201_REGS_SIZE) != 0) ||
       (!request_region (SSA_NCS2_BASE, ZD1201_REGS_SIZE, "zd1201_wlan")))
   {
      printk ("ZD1201 registers physical memory (0x%08x) already claimed\n", SSA_NCS2_BASE);
      return -ENODEV;
   }

   zd1201_regs_virt = (unsigned long) ioremap_nocache (SSA_NCS2_BASE, ZD1201_REGS_SIZE);

#endif
#endif

   if (zd1201_regs_virt == 0) {
      printk ("Failed to ioremap zd1201 registers at phys 0x%08x\n", SSA_NCS2_BASE);
      return -ENODEV;
   }

   /* Fixme: mmio_wlandev should probably be locked while we initialise it... */

   if (mmio_wlandev != NULL) {
      WLAN_LOG_ERROR ("%s: fatal: mmio_wlandev is already in use\n", dev_info);
      goto failed;
   }

   if ((wlandev = create_wlan()) == NULL) {                                 /* allocate wlan_dev and hw struct memory and some basic wlan_dev init */
      WLAN_LOG_ERROR ("%s: create_wlan() failed\n", dev_info);
      goto failed;
   }
   hw = wlandev->priv;

   if (wlan_setup (wlandev) != 0) {                                         /* allocates and inits a kernel netdevice_t struct */
      WLAN_LOG_ERROR ("%s: wlan_setup() failed\n", dev_info);
      goto failed;
   }

   SET_MODULE_OWNER(wlandev->netdev);

   if (register_wlandev (wlandev) != 0) {                                   /* assign wlandev->name and register netdevice with kernel */
      WLAN_LOG_ERROR ("%s: register_wlandev() failed\n", dev_info);
      goto failed;
   }

   hfa384x_create (hw, INT_PCMCIA, 0, (UINT8 *) zd1201_regs_virt);          /* zero then init the hw struct */
   hw->wlandev = wlandev;

#ifdef CONFIG_ARCH_SSA
   /* Reset the PCMCIA card */
   if (zd1201_pcmcia_reset(hw))
	   goto failed0;
   
   /* Create a firmware buffer and read in firmware */
   hw->zd1201_fwbuf = (void *)__get_free_pages(GFP_KERNEL, 
		   			get_order(ZD1201_FIRMWARE_BUF_SZ));
   if (hw->zd1201_fwbuf == NULL) {
      WLAN_LOG_ERROR("%s: Failed to allocate firmware buffer\n", dev_info);
      goto failed0;
   }

   /* Read in the firmware */
   file = filp_open(ZD1201_FIRMWARE_MTD_DEV, O_RDONLY, 0);
   if(IS_ERR(file)) {
      WLAN_LOG_ERROR("%s: Could not open MTD partition containing firmware. "
		      "Return Code: %ld\n", dev_info, PTR_ERR(file));
      goto failed0;
   }

   while (ofs < ZD1201_FIRMWARE_BUF_SZ && ret != 0) {
      if ((ret = kernel_read(file, ofs, hw->zd1201_fwbuf + ofs, 
				PAGE_SIZE)) < 0) {
         WLAN_LOG_ERROR("%s: Could not read MTD partition containing "
			      "firmware (%d).\n", dev_info, ret);
         filp_close(file, NULL);
         goto failed0;
      }
      ofs += ret;
   }

#if 0
   {
      /* dump first 64 bytes of ZyDAS firmware */
      int i;
      for (i = 0; i < 64; i++) {
         if (i % 16 == 0) printk ("\n0x%04x: ", i);
         if (i % 16 == 8) printk ("- ");
         printk ("%02x ", ((char *) hw->zd1201_fwbuf)[i]);
      }
      printk ("\n");
   }
#endif    

   filp_close(file, NULL);

   /* Write firmware to card */
   if (hfa384x_zd1201_reset_helper(hw)) {
      WLAN_LOG_ERROR("Unable to reset card.\n");
      goto failed0;
   }

#endif
   if (prism2_doreset) {
      if (hfa384x_corereset (hw, prism2_reset_holdtime, prism2_reset_settletime, 0) != 0) {
         WLAN_LOG_ERROR ("%s: hfa384x_corereset() failed\n", dev_info);
         goto failed0;
      }
   }

   request_irq (INT_PCMCIA, hfa384x_interrupt, SA_INTERRUPT, wlandev->name, wlandev);

   wlandev->msdstate = WLAN_MSD_HWPRESENT;

   mmio_wlandev = wlandev;

   DBFEXIT;

   return 0;

failed0:
   if (hw->zd1201_fwbuf)
      free_pages((unsigned long)hw->zd1201_fwbuf, 
		      get_order(ZD1201_FIRMWARE_BUF_SZ));
   unregister_wlandev (wlandev);
   hfa384x_destroy (hw);

failed:
#ifndef CONFIG_SSA_HAS7752
   iounmap ((void *) zd1201_regs_virt);
   release_region (SSA_NCS2_BASE, ZD1201_REGS_SIZE);
#endif

   if (wlandev)
      kfree(wlandev);
   if (hw)
      kfree(hw);

   DBFEXIT;

   return -EIO;
}

static void mmio_unregister (void)
{
   wlandevice_t	*wlandev = mmio_wlandev;
   hfa384x_t *hw = wlandev->priv;
#ifndef CONFIG_SSA_HAS7752
   void *zd1201_regs_virt = (void *) hw->membase;
#endif
   struct list_head *pos, *n;
   hfa384x_saved_rec_t *rec;

   DBFENTER;

   unregister_wlandev (wlandev);
   hfa384x_destroy (hw);

   mmio_wlandev = NULL;

   if (hw->zd1201_fwbuf)
      free_pages((unsigned long)hw->zd1201_fwbuf, 
		      get_order(ZD1201_FIRMWARE_BUF_SZ));
   list_for_each_safe(pos, n, &hw->record_list) {
      rec = list_entry(pos, hfa384x_saved_rec_t, list);
      if (rec->buf)
	 kfree(rec->buf);
      list_del(&rec->list);
      kfree(rec);
   }

#ifndef CONFIG_SSA_HAS7752
   iounmap (zd1201_regs_virt);
   release_region (SSA_NCS2_BASE, ZD1201_REGS_SIZE);
#endif
   free_irq(INT_PCMCIA, wlandev);

   DBFEXIT;
}

#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


/*----------------------------------------------------------------
* init_module
*
* Module initialization routine, called once at module load time.
* This one simulates some of the pcmcia calls.
*
* Arguments:
*	none
*
* Returns: 
*	0	- success 
*	~0	- failure, module is unloaded.
*
* Side effects:
* 	alot
*
* Call context:
*	process thread (insmod or modprobe)
----------------------------------------------------------------*/
static int __init prism2_init(void)
{
#if (WLAN_HOSTIF == WLAN_PCMCIA)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,68))
	servinfo_t	serv;
#endif
#endif

        DBFENTER;

        WLAN_LOG_NOTICE("%s Loaded\n", version);
        WLAN_LOG_NOTICE("dev_info is: %s\n", dev_info);

#if (WLAN_HOSTIF == WLAN_PCMCIA)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,68))
	CardServices(GetCardServicesInfo, &serv);
	if ( serv.Revision != CS_RELEASE_CODE )
	{
		printk(KERN_NOTICE"%s: CardServices release does not match!\n", dev_info);
		return -1;
	}

	/* This call will result in a call to prism2sta_attach */
	/*   and eventually prism2sta_detach */
	register_pccard_driver( &dev_info, &prism2sta_attach, &prism2sta_detach);
#else
	pcmcia_register_driver(&prism2_cs_driver);
#endif

#elif (WLAN_HOSTIF == WLAN_PLX)
	/* This call will result in a call to prism2sta_probe_plx 
	 * if there is a matched PCI card present (ie., which 
	 * vendor+device id are matched) 
	 */
	if (pci_register_driver(&prism2_plx_drv_id) <= 0) {
		WLAN_LOG_NOTICE("prism2_plx: No devices found, driver not installed.\n");
		pci_unregister_driver(&prism2_plx_drv_id);
		return -ENODEV;
	}

#elif (WLAN_HOSTIF == WLAN_PCI)
	/* This call will result in a call to prism2sta_probe_pci 
	 * if there is a matched PCI card present (ie., which 
	 * vendor+device id are matched) 
	 */
	if (pci_register_driver(&prism2_pci_drv_id) <= 0) {
		WLAN_LOG_NOTICE("prism2_pci: No devices found, driver not installed.\n");
		pci_unregister_driver(&prism2_pci_drv_id);
		return -ENODEV;
	}

#elif (WLAN_HOSTIF == WLAN_USB)
	/* This call will result in calls to prism2sta_probe_usb. */
	return usb_register(&prism2_usb_driver);

#elif (WLAN_HOSTIF == WLAN_MMIO)

	return mmio_register();

#endif 

        DBFEXIT;
        return 0;
}


/*----------------------------------------------------------------
* cleanup_module
*
* Called at module unload time.  This is our last chance to
* clean up after ourselves.
*
* Arguments:
*	none
*
* Returns: 
*	nothing
*
* Side effects:
* 	alot
*
* Call context:
*	process thread
*
----------------------------------------------------------------*/
static void __exit prism2_cleanup(void)
{
#if (WLAN_HOSTIF == WLAN_PCMCIA)
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,68))
        dev_link_t *link = dev_list;
        dev_link_t *nlink;
        DBFENTER;

	for (link=dev_list; link != NULL; link = nlink) {
		nlink = link->next;
		if ( link->state & DEV_CONFIG ) {
			prism2sta_release((u_long)link);
		}
		prism2sta_detach(link); /* remember detach() frees link */
	}
	unregister_pccard_driver( &dev_info);
#else
	pcmcia_unregister_driver(&prism2_cs_driver);
#endif
#elif (WLAN_HOSTIF == WLAN_PLX)

        DBFENTER;
	pci_unregister_driver(&prism2_plx_drv_id);

#elif (WLAN_HOSTIF == WLAN_PCI)

        DBFENTER;
	pci_unregister_driver(&prism2_pci_drv_id);

#elif (WLAN_HOSTIF == WLAN_USB)
	/* The nice folks who did the USB code made this easy....
	 * usb_deregister() calls the driver disconnect function
	 * for each configured device.  That should clean things up nicely.
	 */
        DBFENTER;
	usb_deregister(&prism2_usb_driver);

#elif (WLAN_HOSTIF == WLAN_MMIO)

        DBFENTER;
        mmio_unregister();

#endif

        printk(KERN_NOTICE "%s Unloaded\n", version);

        DBFEXIT;
        return;
}

module_init(prism2_init);
module_exit(prism2_cleanup);

/*----------------------------------------------------------------
* create_wlan
*
* Called at module init time.  This creates the wlandevice_t structure
* and initializes it with relevant bits.
*
* Arguments:
*	none
*
* Returns: 
*	the created wlandevice_t structure.
*
* Side effects:
* 	also allocates the priv/hw structures.
*
* Call context:
*	process thread
*
----------------------------------------------------------------*/
static wlandevice_t *create_wlan(void)
{
        wlandevice_t    *wlandev = NULL;
	hfa384x_t	*hw = NULL;

      	/* Alloc our structures */
	wlandev =	kmalloc(sizeof(wlandevice_t), GFP_KERNEL);
	hw =		kmalloc(sizeof(hfa384x_t), GFP_KERNEL);

	if (!wlandev || !hw) {
		WLAN_LOG_ERROR("%s: Memory allocation failure.\n", dev_info);
		if (wlandev)	kfree(wlandev);
		if (hw)		kfree(hw);
		return NULL;
	}

	/* Clear all the structs */
	memset(wlandev, 0, sizeof(wlandevice_t));
	memset(hw, 0, sizeof(hfa384x_t));

	/* Initialize the network device object. */
	wlandev->nsdname = dev_info;
	wlandev->msdstate = WLAN_MSD_HWPRESENT_PENDING;
	wlandev->priv = hw;
	wlandev->open = prism2sta_open;
	wlandev->close = prism2sta_close;
	wlandev->reset = prism2sta_reset;
	wlandev->txframe = prism2sta_txframe;
	wlandev->mlmerequest = prism2sta_mlmerequest;
	wlandev->set_multicast_list = prism2sta_setmulticast;
	wlandev->tx_timeout = hfa384x_tx_timeout;

	wlandev->nsdcaps = P80211_NSDCAP_HWFRAGMENT | 
	                   P80211_NSDCAP_AUTOJOIN;

	/* Initialize the device private data stucture. */
        hw->dot11_desired_bss_type = 1;

	return wlandev;
}

