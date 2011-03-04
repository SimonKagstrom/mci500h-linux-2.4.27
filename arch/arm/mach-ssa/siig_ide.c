/*
 *  linux/arch/arm/mach-ssa/siig_ide.c
 *
 *  Copyright (C) 2003-2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/ide.h>
#include <linux/config.h>

#include <asm/arch/hardware.h>
#include <asm/io.h>


static u8 SAA7752_ide_inb (unsigned long port)
{
	return (u8) inw(port);      /* use 16bit read, even for 8bit access */
}

static void SAA7752_ide_outb (u8 addr, unsigned long port)
{
	outw (addr, port);          /* use 16bit write, even for 8bit access */
}

static void SAA7752_ide_outbsync (ide_drive_t *drive, u8 addr, unsigned long port)
{
	outw (addr, port);          /* use 16bit write, even for 8bit access */
}


#define ATA_REGS_SIZE   (PAGE_SIZE)     /* Fixme: not really this big... */

/*
   Register the standard port for this architecture with the IDE driver.
*/
void __init ide_init_default_hwifs (void)
{
	hw_regs_t hw;
	ide_hwif_t *hwif;
	unsigned long ata_regs_virt;

	if (ata_regs_phys == 0)
	{
		printk ("Aborting ATA init (ata_regs_phys not been passed by bootloader)\n");
		return;
	}

	memset (&hw, 0, sizeof(hw_regs_t));

	if ((check_region (ata_regs_phys, ATA_REGS_SIZE) != 0) ||
	    (!request_region (ata_regs_phys, ATA_REGS_SIZE, "ATA")))
        {
		printk ("ATA registers physical memory (0x%08x) already claimed\n", ata_regs_phys);
		printk ("Try disabling the cirrus CS8900 ethernet driver ?\n");
		return;
	}

	ata_regs_virt = (unsigned long) ioremap_nocache (ata_regs_phys, ATA_REGS_SIZE);
	if (ata_regs_virt == 0) {
		printk ("Failed to ioremap ATA registers at phys 0x%08x\n", ata_regs_phys);
		printk ("Try disabling the cirrus CS8900 ethernet driver ?\n");
		return;
	}

	hw.io_ports[IDE_DATA_OFFSET   ] = (ide_ioreg_t) (ata_regs_virt + 0x200);
	hw.io_ports[IDE_ERROR_OFFSET  ] = (ide_ioreg_t) (ata_regs_virt + 0x220);    /* Feature reg when written */
	hw.io_ports[IDE_NSECTOR_OFFSET] = (ide_ioreg_t) (ata_regs_virt + 0x240);
	hw.io_ports[IDE_SECTOR_OFFSET ] = (ide_ioreg_t) (ata_regs_virt + 0x260);
	hw.io_ports[IDE_LCYL_OFFSET   ] = (ide_ioreg_t) (ata_regs_virt + 0x280);
	hw.io_ports[IDE_HCYL_OFFSET   ] = (ide_ioreg_t) (ata_regs_virt + 0x2A0);
	hw.io_ports[IDE_SELECT_OFFSET ] = (ide_ioreg_t) (ata_regs_virt + 0x2C0);    /* DeviceHead reg when read */
	hw.io_ports[IDE_STATUS_OFFSET ] = (ide_ioreg_t) (ata_regs_virt + 0x2E0);    /* Command reg when written */
	hw.io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) (ata_regs_virt + 0x1C0);    /* AltStatus reg when read  */

	hw.irq = INT_ATA;

	ide_register_hw (&hw, &hwif);

	hwif->OUTB     = SAA7752_ide_outb;
	hwif->OUTBSYNC = SAA7752_ide_outbsync;
//	hwif->OUTW     = ide_outw;
//	hwif->OUTL     = ide_outl;
//	hwif->OUTSW    = ide_outsw;
//	hwif->OUTSL    = ide_outsl;
	hwif->INB      = SAA7752_ide_inb;
//	hwif->INW      = ide_inw;
//	hwif->INL      = ide_inl;
//	hwif->INSW     = ide_insw;
//	hwif->INSL     = ide_insl;
}

