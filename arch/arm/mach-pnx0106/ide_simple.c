/*
 *  linux/arch/arm/mach-pnx0106/ide_simple.c
 *
 *  Copyright (C) 2003-2006 Andre McCurdy, Philips Semiconductors.
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


/***************************************************************************/

typedef struct
{
    volatile unsigned int TransferModeMaster;                   /*  0x000  Read  Only */
    volatile unsigned int TransferModeSlave;                    /*  0x004  Read  Only */
    volatile unsigned int SetMultipleMaster;                    /*  0x008  Read  Only */
    volatile unsigned int SetMultipleSlave;                     /*  0x00C  Read  Only */
    volatile unsigned int Interrupt;                            /*  0x010             */
    volatile unsigned int InterruptEnable;                      /*  0x014             */
    volatile unsigned int Automation;                           /*  0x018             */
    volatile unsigned int Validation;                           /*  0x01C  Read  Only */
    volatile unsigned int MasterSlaveConfig;                    /*  0x020             */
    volatile unsigned int GeneralConfig;                        /*  0x024             */

    volatile unsigned int _d0[(0x040 - 0x028) / 4];

    volatile unsigned int IDE_Data;                             /*  0x040             */
    volatile unsigned int IDE_Feature;                          /*  0x044  Write Only */
    #define               IDE_Error IDE_Feature                 /*  0x044  Read  Only */
    volatile unsigned int IDE_SectorCount;                      /*  0x048             */
    volatile unsigned int IDE_SectorNumber;                     /*  0x04C             */
    volatile unsigned int IDE_CylinderLow;                      /*  0x050             */
    volatile unsigned int IDE_CylinderHigh;                     /*  0x054             */
    volatile unsigned int IDE_DriveSelect;                      /*  0x058  Write Only */
    #define               IDE_DeviceHead IDE_DriveSelect        /*  0x058  Read  Only */
    volatile unsigned int IDE_Command;                          /*  0x05C  Write Only */
    #define               IDE_Status IDE_Command                /*  0x05C  Read  Only */
    volatile unsigned int IDE_DeviceControl;                    /*  0x060  Write Only */
    #define               IDE_AltStatus IDE_DeviceControl       /*  0x060  Read  Only */

    volatile unsigned int _d1[(0x070 - 0x064) / 4];

    volatile unsigned int GPIO_0;                               /*  0x070             */
    volatile unsigned int GPIO_1;                               /*  0x074             */
    volatile unsigned int GPIO_2;                               /*  0x078             */
    volatile unsigned int GPIO_3;                               /*  0x07C             */

    volatile unsigned int SlotA_Control_0;                      /*  0x080             */
    volatile unsigned int SlotA_Control_1;                      /*  0x084             */
    volatile unsigned int SlotA_Hotplug;                        /*  0x088             */
    volatile unsigned int SlotA_Status;                         /*  0x08C             */
    volatile unsigned int SlotA_IntPending;                     /*  0x090             */
    volatile unsigned int SlotA_IntEnable;                      /*  0x094             */

    volatile unsigned int _d2[(0x0A0 - 0x098) / 4];

    volatile unsigned int SlotB_Control_0;                      /*  0x0A0             */
    volatile unsigned int SlotB_Control_1;                      /*  0x0A4             */
    volatile unsigned int SlotB_Hotplug;                        /*  0x0A8             */
    volatile unsigned int SlotB_Status;                         /*  0x0AC             */
    volatile unsigned int SlotB_IntPending;                     /*  0x0B0             */
    volatile unsigned int SlotB_IntEnable;                      /*  0x0B4             */

    volatile unsigned int _d3[(0x0C0 - 0x0B8) / 4];

    volatile unsigned int CardStartAddress;                     /*  0x0C0             */
    volatile unsigned int CardByteCount;                        /*  0x0C4             */
    volatile unsigned int CardAutoControl;                      /*  0x0C8             */
    volatile unsigned int CardDetectDebounce;                   /*  0x0CC             */

    volatile unsigned int _d4[(0xC00 - 0x0D0) / 4];

    volatile unsigned int SoftReset;                            /*  0xC00             */
    volatile unsigned int BM_Command;                           /*  0xC04             */
    volatile unsigned int BM_Status;                            /*  0xC08             */
    volatile unsigned int BM_Interrupt;                         /*  0xC0C             */
    volatile unsigned int BM_InterruptEnable;                   /*  0xC10             */
    volatile unsigned int BM_DescriptorTablePointer;            /*  0xC14             */
    volatile unsigned int BM_PacketFifo[3];                     /*  0xC18  Write Only */
    volatile unsigned int BM_PacketFifoReset;                   /*  0xC24  Write Only */
    volatile unsigned int BM_CurrentTransferBaseAddress;        /*  0xC28  Read  Only */
    volatile unsigned int BM_CurrentTransferLength;             /*  0xC2C  Read  Only */
    volatile unsigned int BM_Endian;                            /*  0xC30             */
    volatile unsigned int BM_TimeOut;                           /*  0xC34             */

    volatile unsigned int _d5[(0xFF8 - 0xC38) / 4];

    volatile unsigned int Revision;                             /*  0xFF8  Read  Only */
    volatile unsigned int ID;                                   /*  0xFFC  Read  Only */
}
PCCARD_REGS_t;

#define PCCARD ((PCCARD_REGS_t *) IO_ADDRESS_PCCARD_BASE)


/***************************************************************************/

static u8 pnx0106_inb (unsigned long port)
{
	unsigned int value;

	value = inb (port);

	/*
	   Reading taskfile status register clears the interrupt request
	   from the drive, but the request has also been latched in the
	   host interface - so we need to clear it there as well...
	*/
	if (port == (ide_ioreg_t) &PCCARD->IDE_Status)
		writel ((1 << 1), &PCCARD->Interrupt);

	return value;
}

void __init ide_init_default_hwifs (void)
{
	hw_regs_t hw;
	ide_hwif_t *hwif;

	printk ("PNX0106 Simple IDE, (c) Andre McCurdy, Philips Semiconductors\n");

	/*
	    Note that apart from ensuring automation is off, we skip
	    all setup of the ATA host interface and rely on the bootloader
	    to have left everything in a sensible state...
	*/

	PCCARD->Automation = 0; 		/* 0 == automation off */
	PCCARD->Interrupt = (1 << 1);		/* bit1 : intrq from drive */
	PCCARD->InterruptEnable = (1 << 1);	/* bit1 : intrq from drive */

	memset (&hw, 0, sizeof(hw_regs_t));

	hw.io_ports[IDE_DATA_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_Data;
	hw.io_ports[IDE_ERROR_OFFSET  ] = (ide_ioreg_t) &PCCARD->IDE_Error;		/* Feature reg when written */
	hw.io_ports[IDE_NSECTOR_OFFSET] = (ide_ioreg_t) &PCCARD->IDE_SectorCount;
	hw.io_ports[IDE_SECTOR_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_SectorNumber;
	hw.io_ports[IDE_LCYL_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_CylinderLow;
	hw.io_ports[IDE_HCYL_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_CylinderHigh;
	hw.io_ports[IDE_SELECT_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_DriveSelect;	/* DeviceHead reg when read */
	hw.io_ports[IDE_STATUS_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_Status;		/* Command reg when written */
	hw.io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) &PCCARD->IDE_DeviceControl;	/* AltStatus reg when read */

	hw.irq = IRQ_PCCARD;

	ide_register_hw (&hw, &hwif);
	if (!hwif) {
		printk ("PNX0106 IDE: ide_register_hw() failed\n");
		return;
	}

	hwif->INB = pnx0106_inb;
}

