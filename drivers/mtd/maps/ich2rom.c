/*
 * ich2rom.c
 *
 * Normal mappings of chips in physical memory
 * $Id: ich2rom.c,v 1.8 2003/10/23 23:10:59 thayne Exp $
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>

#define xstr(s) str(s)
#define str(s) #s
#define MOD_NAME xstr(KBUILD_BASENAME)

#define MTD_DEV_NAME_LENGTH 16

#define RESERVE_MEM_REGION 0

#define ICH2_FWH_REGION_START	0xFF000000UL
#define ICH2_FWH_REGION_SIZE	0x01000000UL
#define BIOS_CNTL	0x4e
#define FWH_DEC_EN1	0xE3
#define FWH_DEC_EN2	0xF0
#define FWH_SEL1	0xE8
#define FWH_SEL2	0xEE

struct ich2rom_map_info {
	struct map_info map;
	struct mtd_info *mtd;
	unsigned long window_addr;
	struct pci_dev *pdev;
	struct resource window_rsrc;
	struct resource rom_rsrc;
	char mtd_name[MTD_DEV_NAME_LENGTH];
};

static inline unsigned long addr(struct map_info *map, unsigned long ofs)
{
	unsigned long offset;
	offset = ((8*1024*1024) - map->size) + ofs;
	if (offset >= (4*1024*1024)) {
		offset += 0x400000;
	}
	return map->map_priv_1 + 0x400000 + offset;
}

static inline unsigned long dbg_addr(struct map_info *map, unsigned long addr)
{
	return addr - map->map_priv_1 + ICH2_FWH_REGION_START;
}
	
static __u8 ich2rom_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(addr(map, ofs));
}

static __u16 ich2rom_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(addr(map, ofs));
}

static __u32 ich2rom_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(addr(map, ofs));
}

static void ich2rom_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, addr(map, from), len);
}

static void ich2rom_write8(struct map_info *map, __u8 d, unsigned long ofs)
{
	__raw_writeb(d, addr(map,ofs));
	mb();
}

static void ich2rom_write16(struct map_info *map, __u16 d, unsigned long ofs)
{
	__raw_writew(d, addr(map, ofs));
	mb();
}

static void ich2rom_write32(struct map_info *map, __u32 d, unsigned long ofs)
{
	__raw_writel(d, addr(map, ofs));
	mb();
}

static void ich2rom_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(addr(map, to), from, len);
}

static struct ich2rom_map_info ich2rom_map = {
	.map = {
		.name = MOD_NAME,
		.phys = NO_XIP,
		.size = 0,
		.buswidth = 1,
		.read8 = ich2rom_read8,
		.read16 = ich2rom_read16,
		.read32 = ich2rom_read32,
		.copy_from = ich2rom_copy_from,
		.write8 = ich2rom_write8,
		.write16 = ich2rom_write16,
		.write32 = ich2rom_write32,
		.copy_to = ich2rom_copy_to,
		/* Firmware hubs only use vpp when being programmed
		 * in a factory setting.  So in-place programming
		 * needs to use a different method.
		 */
	},
	/* remaining fields of structure are initialized to 0 */
};

enum fwh_lock_state {
	FWH_DENY_WRITE = 1,
	FWH_IMMUTABLE  = 2,
	FWH_DENY_READ  = 4,
};

static void ich2rom_cleanup(struct ich2rom_map_info *info)
{
	u16 word;

	/* Disable writes through the rom window */
	pci_read_config_word(info->pdev, BIOS_CNTL, &word);
	pci_write_config_word(info->pdev, BIOS_CNTL, word & ~1);

	if (info->mtd) {
		del_mtd_device(info->mtd);
		map_destroy(info->mtd);
		info->mtd = NULL;
		info->map.virt = 0;
	}
	if (info->rom_rsrc.parent)
		release_resource(&info->rom_rsrc);
	if (info->window_rsrc.parent)
		release_resource(&info->window_rsrc);

	if (info->window_addr) {
		iounmap((void *)(info->window_addr));
		info->window_addr = 0;
	}
}


static int ich2rom_set_lock_state(struct mtd_info *mtd, loff_t ofs, size_t len,
	enum fwh_lock_state state)
{
	struct map_info *map = mtd->priv;
	unsigned long start = ofs;
	unsigned long end = start + len -1;

	/* FIXME do I need to guard against concurrency here? */
	/* round down to 64K boundaries */
	start = start & ~0xFFFF;
	end = end & ~0xFFFF;
	while (start <= end) {
		unsigned long ctrl_addr;
		ctrl_addr = addr(map, start) - 0x400000 + 2;
		writeb(state, ctrl_addr);
		start = start + 0x10000;
	}
	return 0;
}

static int ich2rom_lock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return ich2rom_set_lock_state(mtd, ofs, len, FWH_DENY_WRITE);
}

static int ich2rom_unlock(struct mtd_info *mtd, loff_t ofs, size_t len)
{
	return ich2rom_set_lock_state(mtd, ofs, len, 0);
}

static int __devinit ich2rom_init_one (struct pci_dev *pdev,
	const struct pci_device_id *ent)
{
	u16 word;
	struct ich2rom_map_info *info = &ich2rom_map;
	unsigned long map_size;

	/* For now I just handle the ich2 and I assume there
	 * are not a lot of resources up at the top of the address
	 * space.  It is possible to handle other devices in the
	 * top 16MB but it is very painful.  Also since
	 * you can only really attach a FWH to an ICH2 there
	 * a number of simplifications you can make.
	 *
	 * Also you can page firmware hubs if an 8MB window isn't enough 
	 * but don't currently handle that case either.
	 */

	info->pdev = pdev;

	/*
	 * Try to reserve the window mem region.  If this fails then
	 * it is likely due to the window being "reseved" by the BIOS.
	 */
	info->window_rsrc.name = MOD_NAME;
	info->window_rsrc.start = ICH2_FWH_REGION_START;
	info->window_rsrc.end = ICH2_FWH_REGION_START + ICH2_FWH_REGION_SIZE - 1;
	info->window_rsrc.flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	if (request_resource(&iomem_resource, &info->window_rsrc)) {
		info->window_rsrc.parent = NULL;
		printk(KERN_ERR MOD_NAME
		       " %s(): Unable to register resource"
		       " 0x%.08lx-0x%.08lx - kernel bug?\n",
		       __func__,
		       info->window_rsrc.start, info->window_rsrc.end);
	}
	
	/* Enable writes through the rom window */
	pci_read_config_word(pdev, BIOS_CNTL, &word);
	if (!(word & 1)  && (word & (1<<1))) {
		/* The BIOS will generate an error if I enable
		 * this device, so don't even try.
		 */
		printk(KERN_ERR MOD_NAME ": firmware access control, I can't enable writes\n");
		goto failed;
	}
	pci_write_config_word(pdev, BIOS_CNTL, word | 1);


	/* Map the firmware hub into my address space. */
	/* Does this use too much virtual address space? */
	info->window_addr = (unsigned long)ioremap(
		ICH2_FWH_REGION_START, ICH2_FWH_REGION_SIZE);
	if (!info->window_addr) {
		printk(KERN_ERR "Failed to ioremap\n");
		goto failed;
	}

	/* For now assume the firmware has setup all relevant firmware
	 * windows.  We don't have enough information to handle this case
	 * intelligently.
	 */

	/* FIXME select the firmware hub and enable a window to it. */

	info->mtd = 0;
	info->map.map_priv_1 = info->window_addr;

	map_size = ICH2_FWH_REGION_SIZE;
	while(!info->mtd && (map_size > 0)) {
		info->map.size = map_size;
		info->mtd = do_map_probe("jedec_probe", &ich2rom_map.map);
		map_size -= 512*1024;
	}
	if (!info->mtd) {
		goto failed;
	}
	/* I know I can only be a firmware hub here so put
	 * in the special lock and unlock routines.
	 */
	info->mtd->lock = ich2rom_lock;
	info->mtd->unlock = ich2rom_unlock;
		
	info->mtd->owner = THIS_MODULE;
	add_mtd_device(info->mtd);

	if (info->window_rsrc.parent) {
		/*
		 * Registering the MTD device in iomem may not be possible
		 * if there is a BIOS "reserved" and BUSY range.  If this
		 * fails then continue anyway.
		 */
		snprintf(info->mtd_name, MTD_DEV_NAME_LENGTH,
			 "mtd%d", info->mtd->index);

		info->rom_rsrc.name = info->mtd_name;
		info->rom_rsrc.start = ICH2_FWH_REGION_START
			+ ICH2_FWH_REGION_SIZE - map_size;
		info->rom_rsrc.end = ICH2_FWH_REGION_START
			+ ICH2_FWH_REGION_SIZE;
		info->rom_rsrc.flags = IORESOURCE_MEM | IORESOURCE_BUSY;
		if (request_resource(&info->window_rsrc, &info->rom_rsrc)) {
			printk(KERN_ERR MOD_NAME
			       ": cannot reserve MTD resource\n");
			info->rom_rsrc.parent = NULL;
		}
	}

	return 0;

 failed:
	ich2rom_cleanup(info);
	return -ENODEV;
}


static void __devexit ich2rom_remove_one (struct pci_dev *pdev)
{
	struct ich2rom_map_info *info = &ich2rom_map;
	u16 word;

	del_mtd_device(info->mtd);
	map_destroy(info->mtd);
	info->mtd = 0;
	info->map.map_priv_1 = 0;

	iounmap((void *)(info->window_addr));
	info->window_addr = 0;

	/* Disable writes through the rom window */
	pci_read_config_word(pdev, BIOS_CNTL, &word);
	pci_write_config_word(pdev, BIOS_CNTL, word & ~1);

#if RESERVE_MEM_REGION	
	release_mem_region(ICH2_FWH_REGION_START, ICH2_FWH_REGION_SIZE);
#endif
}

static struct pci_device_id ich2rom_pci_tbl[] __devinitdata = {
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801BA_0, 
	  PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801CA_0, 
	  PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801DB_0, 
	  PCI_ANY_ID, PCI_ANY_ID, },
	{ 0, },
};

MODULE_DEVICE_TABLE(pci, ich2rom_pci_tbl);

#if 0
static struct pci_driver ich2rom_driver = {
	.name =		MOD_NAME,
	.id_table =	ich2rom_pci_tbl,
	.probe =	ich2rom_init_one,
	.remove =	ich2rom_remove_one,
};
#endif

static struct pci_dev *mydev;
int __init init_ich2rom(void)
{
	struct pci_dev *pdev;
	struct pci_device_id *id;
	pdev = 0;
	for(id = ich2rom_pci_tbl; id->vendor; id++) {
		pdev = pci_find_device(id->vendor, id->device, 0);
		if (pdev) {
			break;
		}
	}
	if (pdev) {
		mydev = pdev;
		return ich2rom_init_one(pdev, &ich2rom_pci_tbl[0]);
	}
	return -ENXIO;
#if 0
	return pci_module_init(&ich2rom_driver);
#endif
}

static void __exit cleanup_ich2rom(void)
{
	ich2rom_remove_one(mydev);
}

module_init(init_ich2rom);
module_exit(cleanup_ich2rom);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eric Biederman <ebiederman@lnxi.com>");
MODULE_DESCRIPTION("MTD map driver for BIOS chips on the ICH2 southbridge");
