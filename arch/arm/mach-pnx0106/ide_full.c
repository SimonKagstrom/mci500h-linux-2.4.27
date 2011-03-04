/*
 * linux/arch/arm/mach-pnx0106/ide_full.c
 *
 * Copyright (c) 2006 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <asm/arch/hardware.h>
#include <asm/arch/syscreg.h>
#include <asm/arch/tsc.h>
#include <asm/arch/gpio.h>              /* to allow control of ata reset line for debugging */
#include <asm/io.h>
#include <asm/uaccess.h>

#define END_REQUEST_STATUS_IO_ERROR	0
#define END_REQUEST_STATUS_SUCCESS	1

#if 0
#define DEBUG
#endif

unsigned int wibble_debug_level;
unsigned int wibble_nr_sectors;

#ifdef DEBUG
static unsigned int debug_level = 2;
void pnx0106_ide_force_debug_level (int new)
{
	debug_level = new;
}
#if 1
#define DBG(l,x...)	do { if (debug_level > l) { printk(x); } } while (0)
#else
static unsigned int previous_tsc;
#define DBG(l,x...)							\
	do {								\
		if (debug_level > l) {					\
			unsigned int zznoW = ReadTSC(); 		\
			printk ("%7d: ", (zznoW - previous_tsc));	\
			printk (x);					\
			previous_tsc = zznoW;				\
		}							\
	}								\
	while (0)
#endif
#else
#define DBG(l,x...)     do { /* nothing */ } while (0)
#endif


static ide_startstop_t pnx0106_ide_dma_isr (ide_drive_t *drive);


/*******************************************************************************
*******************************************************************************/

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


#define PNX0106_BM_COMMAND_START_READ	(0x00000001 | 0x00000000)
#define PNX0106_BM_COMMAND_START_WRITE	(0x00000001 | 0x00000008)

#define BM_INT_MASK			((1 << 0) |	/* short tables   BM int */	\
					 (1 << 1) |	/* short transfer BM int */	\
					 (1 << 2) |	/* normal end     BM int */	\
					 (1 << 4) |	/* bus error      BM int */	\
					 (1 << 5))	/* timeout        BM int */


/*******************************************************************************
*******************************************************************************/

#define PNX0106_PRD_ENTRIES_MAX_READ	(1)		/* 1 entry == 4k limit (due to HW bug in PNX0106 N1A, N1B and N1C) */
#define PNX0106_PRD_ENTRIES_MAX_WRITE	(64)		/* 64 PRD entries covers 256 sector ATA transfer if all requests are 2k bytes */

#define PNX0106_PRD_BYTES		(8)
#define PNX0106_PRD_ENTRIES		((PNX0106_PRD_ENTRIES_MAX_READ > PNX0106_PRD_ENTRIES_MAX_WRITE) ? PNX0106_PRD_ENTRIES_MAX_READ : PNX0106_PRD_ENTRIES_MAX_WRITE)
#define PNX0106_PRD_TABLE_SIZE		(PNX0106_PRD_ENTRIES * PNX0106_PRD_BYTES)

#define MAX_SG_ENTRIES			(256)		/* better safe than sorry... */

#define SW_CMD_TIMEOUT_MSEC  		(16 * 1024)

#if 0
#define BM_TIMEOUT_SECONDS      	(0)		/* 0 disables internal HW timer (rely on kernel IDE layer timeouts instead) */
#else
#define BM_TIMEOUT_SECONDS      	(20)		/* if the shit really hits the fan... */
#endif


/*******************************************************************************
*******************************************************************************/

struct pnx0106_ide_state
{
	unsigned int stf_feature_stack;
	unsigned int stf_sector_count_stack;
	unsigned int stf_sector_number_stack;
	unsigned int stf_cyl_low_stack;
	unsigned int stf_cyl_high_stack;
	unsigned int stf_drive_select_stack;
	unsigned int stf_device_control_stack;
	unsigned short stf_data_stack[8];
	unsigned int stf_data_index;

	unsigned long long last_command_complete_tsc;

	unsigned int SlotA_Control_0_saved;

	unsigned int start_sector;

	struct scatterlist *sg_table;
	unsigned int sg_partial_offset;
	unsigned int sg_nents_remaining;
	unsigned int sector_count;

#if defined (RESTART_SHORT_XFERS_AFTER_FIRST_PRD)
	struct scatterlist *sg_table_first_prd;
	unsigned int sg_partial_offset_first_prd;
	unsigned int sg_nents_remaining_first_prd;
	unsigned int sector_count_first_prd;
#endif
};

static struct pnx0106_ide_state gstate;


/*******************************************************************************
*******************************************************************************/

#define XFER_PIO_5		0x0d
#define XFER_UDMA_SLOW		0x4f

#define XFER_MODE		0xf0
#define XFER_UDMA_133		0x48
#define XFER_UDMA_100		0x44
#define XFER_UDMA_66		0x42
#define XFER_UDMA		0x40
#define XFER_MWDMA		0x20
#define XFER_SWDMA		0x10
#define XFER_EPIO		0x01
#define XFER_PIO		0x00

static int ide_find_best_mode (ide_drive_t *drive, int map)
{
	struct hd_driveid *id = drive->id;
	int best = 0;

	if (!id)
		return XFER_PIO_SLOW;

	if ((map & XFER_UDMA) && (id->field_valid & 4)) {	/* Want UDMA and UDMA bitmap valid */

		if ((map & XFER_UDMA_133) == XFER_UDMA_133)
			if ((best = (id->dma_ultra & 0x0040) ? XFER_UDMA_6 : 0)) return best;

		if ((map & XFER_UDMA_100) == XFER_UDMA_100)
			if ((best = (id->dma_ultra & 0x0020) ? XFER_UDMA_5 : 0)) return best;

		if ((map & XFER_UDMA_66) == XFER_UDMA_66)
			if ((best = (id->dma_ultra & 0x0010) ? XFER_UDMA_4 :
                	    	    (id->dma_ultra & 0x0008) ? XFER_UDMA_3 : 0)) return best;

                if ((best = (id->dma_ultra & 0x0004) ? XFER_UDMA_2 :
                	    (id->dma_ultra & 0x0002) ? XFER_UDMA_1 :
                	    (id->dma_ultra & 0x0001) ? XFER_UDMA_0 : 0)) return best;
	}

	if ((map & XFER_MWDMA) && (id->field_valid & 2)) {	/* Want MWDMA and drive has EIDE fields */

		if ((best = (id->dma_mword & 0x0004) ? XFER_MW_DMA_2 :
                	    (id->dma_mword & 0x0002) ? XFER_MW_DMA_1 :
                	    (id->dma_mword & 0x0001) ? XFER_MW_DMA_0 : 0)) return best;
	}

	if (map & XFER_SWDMA) {					/* Want SWDMA */

 		if (id->field_valid & 2) {			/* EIDE SWDMA */

			if ((best = (id->dma_1word & 0x0004) ? XFER_SW_DMA_2 :
      				    (id->dma_1word & 0x0002) ? XFER_SW_DMA_1 :
				    (id->dma_1word & 0x0001) ? XFER_SW_DMA_0 : 0)) return best;
		}

		if (id->capability & 1) {			/* Pre-EIDE style SWDMA */

			if ((best = (id->tDMA == 2) ? XFER_SW_DMA_2 :
				    (id->tDMA == 1) ? XFER_SW_DMA_1 :
				    (id->tDMA == 0) ? XFER_SW_DMA_0 : 0)) return best;
		}
	}


	if ((map & XFER_EPIO) && (id->field_valid & 2)) {	/* EIDE PIO modes */

		if ((best = (drive->id->eide_pio_modes & 4) ? XFER_PIO_5 :
			    (drive->id->eide_pio_modes & 2) ? XFER_PIO_4 :
			    (drive->id->eide_pio_modes & 1) ? XFER_PIO_3 : 0)) return best;
	}

	return  (drive->id->tPIO == 2) ? XFER_PIO_2 :
		(drive->id->tPIO == 1) ? XFER_PIO_1 :
		(drive->id->tPIO == 0) ? XFER_PIO_0 : XFER_PIO_SLOW;
}


/*******************************************************************************
*******************************************************************************/

#if defined (DEBUG) && 0

static void hexdump (unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			printk ("%08x: %02x ", i, buf[i]);
		else
			printk ("%02x ", buf[i]);
		if (i != 0 && i % 16 == 0)
			printk ("\n");
	}
	printk ("\n");
}

static void dump_regs (void)
{
    printk ("TransferModeMaster            : 0x%08x\n", readl (&PCCARD->TransferModeMaster));
    printk ("TransferModeSlave             : 0x%08x\n", readl (&PCCARD->TransferModeSlave));
    printk ("SetMultipleMaster             : 0x%08x\n", readl (&PCCARD->SetMultipleMaster));
    printk ("SetMultipleSlave              : 0x%08x\n", readl (&PCCARD->SetMultipleSlave));
    printk ("Interrupt                     : 0x%08x\n", readl (&PCCARD->Interrupt));
    printk ("InterruptEnable               : 0x%08x\n", readl (&PCCARD->InterruptEnable));
    printk ("Automation                    : 0x%08x\n", readl (&PCCARD->Automation));
    printk ("Validation                    : 0x%08x\n", readl (&PCCARD->Validation));
    printk ("MasterSlaveConfig             : 0x%08x\n", readl (&PCCARD->MasterSlaveConfig));
    printk ("GeneralConfig                 : 0x%08x\n", readl (&PCCARD->GeneralConfig));

    printk ("IDE_Data                      : 0x%08x\n", readl (&PCCARD->IDE_Data));
    printk ("IDE_Error                     : 0x%08x\n", readl (&PCCARD->IDE_Error));
    printk ("IDE_SectorCount               : 0x%08x\n", readl (&PCCARD->IDE_SectorCount));
    printk ("IDE_SectorNumber              : 0x%08x\n", readl (&PCCARD->IDE_SectorNumber));
    printk ("IDE_CylinderLow               : 0x%08x\n", readl (&PCCARD->IDE_CylinderLow));
    printk ("IDE_CylinderHigh              : 0x%08x\n", readl (&PCCARD->IDE_CylinderHigh));
    printk ("IDE_DeviceHead                : 0x%08x\n", readl (&PCCARD->IDE_DeviceHead));
    printk ("IDE_Status                    : 0x%08x\n", readl (&PCCARD->IDE_Status));
    printk ("IDE_AltStatus                 : 0x%08x\n", readl (&PCCARD->IDE_AltStatus));

    printk ("GPIO_0                        : 0x%08x\n", readl (&PCCARD->GPIO_0));
    printk ("GPIO_1                        : 0x%08x\n", readl (&PCCARD->GPIO_1));
    printk ("GPIO_2                        : 0x%08x\n", readl (&PCCARD->GPIO_2));
    printk ("GPIO_3                        : 0x%08x\n", readl (&PCCARD->GPIO_3));

    printk ("SlotA_Control_0               : 0x%08x\n", readl (&PCCARD->SlotA_Control_0));
    printk ("SlotA_Control_1               : 0x%08x\n", readl (&PCCARD->SlotA_Control_1));
    printk ("SlotA_Hotplug                 : 0x%08x\n", readl (&PCCARD->SlotA_Hotplug));
    printk ("SlotA_Status                  : 0x%08x\n", readl (&PCCARD->SlotA_Status));
    printk ("SlotA_IntPending              : 0x%08x\n", readl (&PCCARD->SlotA_IntPending));
    printk ("SlotA_IntEnable               : 0x%08x\n", readl (&PCCARD->SlotA_IntEnable));
    printk ("SlotB_Control_0               : 0x%08x\n", readl (&PCCARD->SlotB_Control_0));
    printk ("SlotB_Control_1               : 0x%08x\n", readl (&PCCARD->SlotB_Control_1));
    printk ("SlotB_Hotplug                 : 0x%08x\n", readl (&PCCARD->SlotB_Hotplug));
    printk ("SlotB_Status                  : 0x%08x\n", readl (&PCCARD->SlotB_Status));
    printk ("SlotB_IntPending              : 0x%08x\n", readl (&PCCARD->SlotB_IntPending));
    printk ("SlotB_IntEnable               : 0x%08x\n", readl (&PCCARD->SlotB_IntEnable));
    printk ("CardStartAddress              : 0x%08x\n", readl (&PCCARD->CardStartAddress));
    printk ("CardByteCount                 : 0x%08x\n", readl (&PCCARD->CardByteCount));
    printk ("CardAutoControl               : 0x%08x\n", readl (&PCCARD->CardAutoControl));
    printk ("CardDetectDebounce            : 0x%08x\n", readl (&PCCARD->CardDetectDebounce));
    printk ("SoftReset                     : 0x%08x\n", readl (&PCCARD->SoftReset));
    printk ("BM_Command                    : 0x%08x\n", readl (&PCCARD->BM_Command));
    printk ("BM_Status                     : 0x%08x\n", readl (&PCCARD->BM_Status));
    printk ("BM_Interrupt                  : 0x%08x\n", readl (&PCCARD->BM_Interrupt));
    printk ("BM_InterruptEnable            : 0x%08x\n", readl (&PCCARD->BM_InterruptEnable));
    printk ("BM_DescriptorTablePointer     : 0x%08x\n", readl (&PCCARD->BM_DescriptorTablePointer));
    printk ("BM_PacketFifo[0]              : 0x%08x\n", readl (&PCCARD->BM_PacketFifo[0]));
    printk ("BM_PacketFifo[1]              : 0x%08x\n", readl (&PCCARD->BM_PacketFifo[1]));
    printk ("BM_PacketFifo[2]              : 0x%08x\n", readl (&PCCARD->BM_PacketFifo[2]));
    printk ("BM_PacketFifoReset            : 0x%08x\n", readl (&PCCARD->BM_PacketFifoReset));
    printk ("BM_CurrentTransferBaseAddress : 0x%08x\n", readl (&PCCARD->BM_CurrentTransferBaseAddress));
    printk ("BM_CurrentTransferLength      : 0x%08x\n", readl (&PCCARD->BM_CurrentTransferLength));
    printk ("BM_Endian                     : 0x%08x\n", readl (&PCCARD->BM_Endian));
    printk ("BM_TimeOut                    : 0x%08x\n", readl (&PCCARD->BM_TimeOut));
    printk ("Revision                      : 0x%08x\n", readl (&PCCARD->Revision));
    printk ("ID                            : 0x%08x\n", readl (&PCCARD->ID));

    printk ("-----\n");
}

#endif

static char *port_to_reg_string_read (unsigned long port)
{
	switch (port)	/* register names when reading */
	{
		case (unsigned long) &PCCARD->IDE_Data:          return "DATA   ";
		case (unsigned long) &PCCARD->IDE_Error:         return "ERROR  ";
		case (unsigned long) &PCCARD->IDE_SectorCount:   return "NSECTOR";
		case (unsigned long) &PCCARD->IDE_SectorNumber:  return "SECTOR ";
		case (unsigned long) &PCCARD->IDE_CylinderLow:   return "LCYL   ";
		case (unsigned long) &PCCARD->IDE_CylinderHigh:  return "HCYL   ";
		case (unsigned long) &PCCARD->IDE_DriveSelect:   return "SELECT ";
		case (unsigned long) &PCCARD->IDE_Status:        return "STATUS ";
		case (unsigned long) &PCCARD->IDE_AltStatus:     return "ALTSTAT";
        }

	return "UNKNOWN";
}

static char *port_to_reg_string_write (unsigned long port)
{
	switch (port)	/* register names when writing */
	{
		case (unsigned long) &PCCARD->IDE_Data:          return "DATA   ";
		case (unsigned long) &PCCARD->IDE_Error:         return "FEATURE";
		case (unsigned long) &PCCARD->IDE_SectorCount:   return "NSECTOR";
		case (unsigned long) &PCCARD->IDE_SectorNumber:  return "SECTOR ";
		case (unsigned long) &PCCARD->IDE_CylinderLow:   return "LCYL   ";
		case (unsigned long) &PCCARD->IDE_CylinderHigh:  return "HCYL   ";
		case (unsigned long) &PCCARD->IDE_DriveSelect:   return "SELECT ";
		case (unsigned long) &PCCARD->IDE_Command:       return "COMMAND";
		case (unsigned long) &PCCARD->IDE_DeviceControl: return "CONTROL";
        }

	return "UNKNOWN";
}

static char *decode_command (unsigned int cmd_byte)
{
	switch (cmd_byte)
	{
		case 0x00: return "WIN_NOP		     ";
		case 0x03: return "CFA_REQ_EXT_ERROR_CODE    "; /* CFA Request Extended Error Code */
		case 0x08: return "WIN_DEVICE_RESET          ";
		case 0x10: return "WIN_RECAL / WIN_RESTORE   ";
		case 0x20: return "WIN_READ		     "; /* 28-Bit */
		case 0x21: return "WIN_READ_ONCE	     "; /* 28-Bit without retries */
		case 0x22: return "WIN_READ_LONG	     "; /* 28-Bit */
		case 0x23: return "WIN_READ_LONG_ONCE	     "; /* 28-Bit without retries */
		case 0x24: return "WIN_READ_EXT 	     "; /* 48-Bit */
		case 0x25: return "WIN_READDMA_EXT	     "; /* 48-Bit */
		case 0x26: return "WIN_READDMA_QUEUED_EXT    "; /* 48-Bit */
		case 0x27: return "WIN_READ_NATIVE_MAX_EXT   "; /* 48-Bit */
		case 0x29: return "WIN_MULTREAD_EXT	     "; /* 48-Bit */
		case 0x30: return "WIN_WRITE		     "; /* 28-Bit */
		case 0x31: return "WIN_WRITE_ONCE	     "; /* 28-Bit without retries */
		case 0x32: return "WIN_WRITE_LONG	     "; /* 28-Bit */
		case 0x33: return "WIN_WRITE_LONG_ONCE       "; /* 28-Bit without retries */
		case 0x34: return "WIN_WRITE_EXT	     "; /* 48-Bit */
		case 0x35: return "WIN_WRITEDMA_EXT	     "; /* 48-Bit */
		case 0x36: return "WIN_WRITEDMA_QUEUED_EXT   "; /* 48-Bit */
		case 0x37: return "WIN_SET_MAX_EXT	     "; /* 48-Bit */
		case 0x38: return "CFA_WRITE_SECT_WO_ERASE   "; /* CFA Write Sectors without erase */
		case 0x39: return "WIN_MULTWRITE_EXT	     "; /* 48-Bit */
		case 0x3C: return "WIN_WRITE_VERIFY	     "; /* 28-Bit */
		case 0x40: return "WIN_VERIFY		     "; /* 28-Bit - Read Verify Sectors */
		case 0x41: return "WIN_VERIFY_ONCE	     "; /* 28-Bit - without retries */
		case 0x42: return "WIN_VERIFY_EXT	     "; /* 48-Bit */
		case 0x50: return "WIN_FORMAT		     ";
		case 0x60: return "WIN_INIT		     ";
		case 0x70: return "WIN_SEEK		     "; /* 0x70-0x7F Reserved */
		case 0x87: return "CFA_TRANSLATE_SECTOR      "; /* CFA Translate Sector */
		case 0x90: return "WIN_DIAGNOSE 	     ";
		case 0x91: return "WIN_SPECIFY  	     "; /* set drive geometry translation */
		case 0x92: return "WIN_DOWNLOAD_MICROCODE    ";
		case 0x94: return "WIN_STANDBYNOW2	     ";
		case 0x96: return "WIN_STANDBY2 	     ";
		case 0x97: return "WIN_SETIDLE2 	     ";
		case 0x98: return "WIN_CHECKPOWERMODE2       ";
		case 0x99: return "WIN_SLEEPNOW2	     ";
		case 0xA0: return "WIN_PACKETCMD	     "; /* Send a packet command. */
		case 0xA1: return "WIN_PIDENTIFY	     "; /* identify ATAPI device   */
		case 0xA2: return "WIN_QUEUED_SERVICE	     ";
		case 0xB0: return "WIN_SMART		     "; /* self-monitoring and reporting */
		case 0xC0: return "CFA_ERASE_SECTORS	     ";
		case 0xC4: return "WIN_MULTREAD 	     "; /* read sectors using multiple mode*/
		case 0xC5: return "WIN_MULTWRITE	     "; /* write sectors using multiple mode */
		case 0xC6: return "WIN_SETMULT  	     "; /* enable/disable multiple mode */
		case 0xC7: return "WIN_READDMA_QUEUED	     "; /* read sectors using Queued DMA transfers */
		case 0xC8: return "WIN_READDMA  	     "; /* read sectors using DMA transfers */
		case 0xC9: return "WIN_READDMA_ONCE	     "; /* 28-Bit - without retries */
		case 0xCA: return "WIN_WRITEDMA 	     "; /* write sectors using DMA transfers */
		case 0xCB: return "WIN_WRITEDMA_ONCE	     "; /* 28-Bit - without retries */
		case 0xCC: return "WIN_WRITEDMA_QUEUED       "; /* write sectors using Queued DMA transfers */
		case 0xCD: return "CFA_WRITE_MULTI_WO_ERASE  "; /* CFA Write multiple without erase */
		case 0xDA: return "WIN_GETMEDIASTATUS	     ";
		case 0xDB: return "WIN_ACKMEDIACHANGE	     "; /* ATA-1, ATA-2 vendor */
		case 0xDC: return "WIN_POSTBOOT 	     ";
		case 0xDD: return "WIN_PREBOOT  	     ";
		case 0xDE: return "WIN_DOORLOCK 	     "; /* lock door on removable drives */
		case 0xDF: return "WIN_DOORUNLOCK	     "; /* unlock door on removable drives */
		case 0xE0: return "WIN_STANDBYNOW1	     ";
		case 0xE1: return "WIN_IDLEIMMEDIATE	     "; /* force drive to become "ready" */
		case 0xE2: return "WIN_STANDBY  	     "; /* Set device in Standby Mode */
		case 0xE3: return "WIN_SETIDLE1 	     ";
		case 0xE4: return "WIN_READ_BUFFER	     "; /* force read only 1 sector */
		case 0xE5: return "WIN_CHECKPOWERMODE1       ";
		case 0xE6: return "WIN_SLEEPNOW1	     ";
		case 0xE7: return "WIN_FLUSH_CACHE	     ";
		case 0xE8: return "WIN_WRITE_BUFFER	     "; /* force write only 1 sector */
		case 0xE9: return "WIN_WRITE_SAME	     "; /* read ata-2 to use */
		case 0xEA: return "WIN_FLUSH_CACHE_EXT       "; /* 48-Bit */
		case 0xEC: return "WIN_IDENTIFY 	     "; /* ask drive to identify itself    */
		case 0xED: return "WIN_MEDIAEJECT	     ";
		case 0xEE: return "WIN_IDENTIFY_DMA	     "; /* same as WIN_IDENTIFY, but DMA */
		case 0xEF: return "WIN_SETFEATURES	     "; /* set special drive features */
		case 0xF0: return "EXABYTE_ENABLE_NEST       ";
		case 0xF1: return "WIN_SECURITY_SET_PASS     ";
		case 0xF2: return "WIN_SECURITY_UNLOCK       ";
		case 0xF3: return "WIN_SECURITY_ERASE_PREPARE";
		case 0xF4: return "WIN_SECURITY_ERASE_UNIT   ";
		case 0xF5: return "WIN_SECURITY_FREEZE_LOCK  ";
		case 0xF6: return "WIN_SECURITY_DISABLE      ";
		case 0xF8: return "WIN_READ_NATIVE_MAX       "; /* return the native maximum address */
		case 0xF9: return "WIN_SET_MAX  	     ";
		case 0xFB: return "DISABLE_SEAGATE	     ";
	}

	return "UNKNOWN";
}


/*******************************************************************************
*******************************************************************************/

void pnx0106_ide_full_soft_reset (void)
{
	writel ((1 << 0), &PCCARD->SoftReset); 		/* Bit0: Soft Reset : complete reset of ATA and bus master blocks... */
}


/*******************************************************************************
*******************************************************************************/

static void pnx0106_setspeed_pio (ide_drive_t *drive, u8 speed)
{
	DBG (3, "%s (0x%02x)\n", __FUNCTION__, speed);

	if (speed == 255)
		speed = ide_find_best_mode (drive, (XFER_PIO | XFER_EPIO));

#if 0
	if (ide_config_drive_speed (drive, speed) != 0)
		printk ("ide%d: Drive %d didn't accept speed setting !!\n",
				drive->dn >> 1, drive->dn & 1);

	if (!drive->init_speed)
		drive->init_speed = speed;

	drive->current_speed = speed;
#endif

}

static int pnx0106_setspeed_dma (ide_drive_t *drive, u8 speed)
{
	DBG (3, "%s (0x%02x)\n", __FUNCTION__, speed);

	printk ("%s: %s selected\n", drive->name, ide_xfer_verbose (speed));

	if (ide_config_drive_speed (drive, speed) != 0)
		printk ("ide%d: Drive %d didn't accept speed setting !!\n",
				drive->dn >> 1, drive->dn & 1);

	if (!drive->init_speed)
		drive->init_speed = speed;

	drive->current_speed = speed;

	return 0;
}


/*******************************************************************************
*******************************************************************************/

static void push_stf_byte (unsigned int *stack, u8 data)
{
	unsigned int temp;

	temp = *stack;
	*stack = (temp << 8) | data;
}

static u8 pop_stf_byte (unsigned int *stack)
{
	unsigned int temp;

	temp = *stack;
	*stack = temp >> 8;
	return temp & 0xFF;
}

static u8 extract_stf_byte (unsigned int *stack, int index)
{
	unsigned int temp;

	temp = *stack;
	return (temp >> (index * 8)) & 0xFF;
}

static void playback_taskfile_outb (u8 addr, unsigned long port)
{
	DBG (5, "%s : %s = 0x%02X\n", __FUNCTION__, port_to_reg_string_write (port), addr);
	outb (addr, port);
}

static void playback_taskfile (ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;
	unsigned int drive_select;
	unsigned int lba;

	/*
	   Snoop the LBA start address from previous writes to the taskfile.

	   We also setup taskfile registers that are static thoughout this request.
	   The potentially dynamic ones - ie registers such as LBA address and sector
	   count which we need to cook in order to split up one request into multiple
	   ATA commands - are setup in continue_xfer().
	*/

	drive_select = extract_stf_byte (&state->stf_drive_select_stack, 0);
	playback_taskfile_outb (drive_select, (unsigned long) &PCCARD->IDE_DriveSelect);
	playback_taskfile_outb (extract_stf_byte (&state->stf_feature_stack, 0), (unsigned long) &PCCARD->IDE_Feature);
	playback_taskfile_outb (extract_stf_byte (&state->stf_device_control_stack, 0), (unsigned long) &PCCARD->IDE_DeviceControl);

	if (drive->addressing == 1)
		lba = extract_stf_byte (&state->stf_sector_number_stack, 1);
	else
		lba = (drive_select & 0x0F);

	lba = (lba << 8) | extract_stf_byte (&state->stf_cyl_high_stack, 0);
	lba = (lba << 8) | extract_stf_byte (&state->stf_cyl_low_stack, 0);
	lba = (lba << 8) | extract_stf_byte (&state->stf_sector_number_stack, 0);

	DBG (5, "%s: %s LBA 0x%08x\n", __FUNCTION__, (drive->addressing == 1) ? "48bit" : "28bit", lba);

	state->start_sector = lba;			/* LBA address to be transferred next */
}

static void automation_on (ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;

	DBG (3, "%s\n", __FUNCTION__);

	writel (0, &PCCARD->InterruptEnable);		/* clear bit1 : ie ignore intrq from drive */
//	writel (1, &PCCARD->Interrupt); 		/* reset bit0 : clear previous intrq from automation HW (debug) */
	writel (1, &PCCARD->BM_PacketFifoReset);	/* */
	writel (1, &PCCARD->Automation);		/* 1 == automation on */

	/*
	   Playback previous writes to the taskfile registers it give host
	   interface a chance to snoop transfer parameters...
	*/

	playback_taskfile (drive);

	/* slowest PIO timing for non data register accesses... */
	writel ((state->SlotA_Control_0_saved | 0x0000FFFF), &PCCARD->SlotA_Control_0);
}

static void automation_off (ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;

//	DBG (3, "%s (ATA int 0x%02x)\n", __FUNCTION__, PCCARD->Interrupt);

	writel (0, &PCCARD->Automation);		/* 0 == automation off */

	writel ((1 << 1), &PCCARD->Interrupt);

	writel (2, &PCCARD->InterruptEnable);		/* set bit1 : ie re-enable intrqs from drive */

	DBG (3, "%s (ATA int 0x%02x)\n", __FUNCTION__, PCCARD->Interrupt);

	/* restore PIO timing for non data register accesses... */
	writel (state->SlotA_Control_0_saved, &PCCARD->SlotA_Control_0);
}

static int automation_is_on (ide_drive_t *drive)
{
	return readl (&PCCARD->Automation) & 0x01;
}


/*******************************************************************************
*******************************************************************************/

static u8 pnx0106_inb (unsigned long port)
{
	unsigned int value;

	value = inb (port);
	DBG (4, "%s  : %s = 0x%02X\n", __FUNCTION__, port_to_reg_string_read (port), value);

	/*
	   Reading taskfile status register clears the interrupt request
	   from the drive, but the request has also been latched in the
	   host interface - so we need to clear it there as well...
	*/
	if (port == (ide_ioreg_t) &PCCARD->IDE_Status)
		writel ((1 << 1), &PCCARD->Interrupt);

	return value;
}

static u16 pnx0106_inw (unsigned long port)
{
	unsigned int value;

	value = inw (port);
	DBG (3, "%s  : %s = 0x%04X\n", __FUNCTION__, port_to_reg_string_read (port), value);
	return value;
}

static void pnx0106_insw (unsigned long port, void *addr, u32 count)
{
	DBG (3, "%s : %s (count = %d)\n", __FUNCTION__, port_to_reg_string_read (port), count);
	insw (port, addr, count);
}

static u32 pnx0106_inl (unsigned long port)
{
	unsigned int value;

	value = inl (port);
	DBG (3, "%s  : %s = 0x%08X\n", __FUNCTION__, port_to_reg_string_read (port), value);
	return value;
}

static void pnx0106_insl (unsigned long port, void *addr, u32 count)
{
	DBG (3, "%s : %s (count = %d)\n", __FUNCTION__, port_to_reg_string_read (port), count);
	insl (port, addr, count);
}

static void pnx0106_outb (u8 addr, unsigned long port)
{
	struct pnx0106_ide_state *state = &gstate;
	unsigned int *sp = NULL;

	if (port == (unsigned long) &PCCARD->IDE_Status)
		DBG (3, "%s : %s = 0x%02X : %s (%d)\n", __FUNCTION__, port_to_reg_string_write (port), addr, decode_command (addr), wibble_nr_sectors);
	else
		DBG (4, "%s : %s = 0x%02X\n", __FUNCTION__, port_to_reg_string_write (port), addr);

	switch (port)
	{
		case (unsigned long) &PCCARD->IDE_Feature:
			sp = &state->stf_feature_stack;
			break;
		case (unsigned long) &PCCARD->IDE_SectorCount:
			sp = &state->stf_sector_count_stack;
			break;
		case (unsigned long) &PCCARD->IDE_SectorNumber:
			sp = &state->stf_sector_number_stack;
			break;
		case (unsigned long) &PCCARD->IDE_CylinderLow:
			sp = &state->stf_cyl_low_stack;
			break;
		case (unsigned long) &PCCARD->IDE_CylinderHigh:
			sp = &state->stf_cyl_high_stack;
			break;
		case (unsigned long) &PCCARD->IDE_DriveSelect:
			sp = &state->stf_drive_select_stack;
			break;
		case (unsigned long) &PCCARD->IDE_DeviceControl:
			sp = &state->stf_device_control_stack;
			break;
		default:
			break;
	}

	if (sp)
		push_stf_byte (sp, addr);

	outb (addr, port);
}

static void pnx0106_outbsync (ide_drive_t *drive, u8 addr, unsigned long port)
{
#if 1
	pnx0106_outb (addr, port);
#else
	struct pnx0106_ide_state *state = &gstate;
	unsigned int *sp = NULL;

	if (port == (unsigned long) &PCCARD->IDE_Status)
		DBG (3, "%s: %s = 0x%02X : %s\n", __FUNCTION__, port_to_reg_string_write (port), addr, decode_command (addr));
	else
		DBG (4, "%s: %s = 0x%02X\n", __FUNCTION__, port_to_reg_string_write (port), addr);

	switch (port)
	{
		case (unsigned long) &PCCARD->IDE_Feature:
			sp = &state->stf_feature_stack;
			break;
		case (unsigned long) &PCCARD->IDE_SectorCount:
			sp = &state->stf_sector_count_stack;
			break;
		case (unsigned long) &PCCARD->IDE_SectorNumber:
			sp = &state->stf_sector_number_stack;
			break;
		case (unsigned long) &PCCARD->IDE_CylinderLow:
			sp = &state->stf_cyl_low_stack;
			break;
		case (unsigned long) &PCCARD->IDE_CylinderHigh:
			sp = &state->stf_cyl_high_stack;
			break;
		case (unsigned long) &PCCARD->IDE_DriveSelect:
			sp = &state->stf_drive_select_stack;
			break;
		case (unsigned long) &PCCARD->IDE_DeviceControl:
			sp = &state->stf_device_control_stack;
			break;
		default:
			break;
	}

	if (sp)
		push_stf_byte (sp, addr);

	outb (addr, port);
#endif
}

static void pnx0106_outw (u16 addr, unsigned long port)
{
	DBG (3, "%s : %s = 0x%04X\n", __FUNCTION__, port_to_reg_string_write (port), addr);
	outw (addr, port);
}

static void pnx0106_outsw (unsigned long port, void *addr, u32 count)
{
	DBG (3, "%s: %s (count = %d)\n", __FUNCTION__, port_to_reg_string_write (port), count);
	outsw (port, addr, count);
}

static void pnx0106_outl (u32 addr, unsigned long port)
{
	DBG (3, "%s : %s = 0x%08X\n", __FUNCTION__, port_to_reg_string_write (port), addr);
	outl (addr, port);
}

static void pnx0106_outsl (unsigned long port, void *addr, u32 count)
{
	DBG (3, "%s: %s (count = %d)\n", __FUNCTION__, port_to_reg_string_write (port), count);
	outsl (port, addr, count);
}


/*******************************************************************************
*******************************************************************************/

/*
   Walk a request cluster and build a corresponding scatterlist table.
   This is't really PNX0106 specific, so maybe there's already a generic
   implementation in the kernel somewhere ??
*/
static int pnx0106_ide_sglist_build (struct scatterlist *sg, struct request *rq)
{
	struct buffer_head *bh = rq->bh;
	unsigned long newdatastart;
	unsigned long lastdataend = ~0UL;
	int nents = 0;

	do {
		newdatastart = (bh->b_page != NULL) ? bh_phys (bh) : (unsigned long) bh->b_data;
		if (newdatastart == lastdataend) {
			sg[nents - 1].length += bh->b_size;
			lastdataend += bh->b_size;
			continue;
		}
		if (unlikely (nents >= MAX_SG_ENTRIES)) {
			DBG (1, "%s: MAX_SG_ENTRIES too small for request\n", __FUNCTION__);
			return 0;
		}
		memset (&sg[nents], 0, sizeof(*sg));
		if (bh->b_page != NULL) {
			sg[nents].page = bh->b_page;
			sg[nents].offset = bh_offset (bh);
		}
		else
			sg[nents].address = bh->b_data;
		sg[nents].length = bh->b_size;
		lastdataend = newdatastart + bh->b_size;
		nents++;
	}
	while ((bh = bh->b_reqnext) != NULL);

	return nents;
}

static int pnx0106_ide_sglist_setup (ide_drive_t *drive, int ddir)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;
	struct request *rq = hwif->hwgroup->rq;
	int nents;

	if (unlikely (rq->cmd == IDE_DRIVE_TASKFILE))
		return 1;

	nents = pnx0106_ide_sglist_build (hwif->sg_table, rq);
	if (unlikely (nents == 0))
		return 1;

	nents = pci_map_sg (NULL, hwif->sg_table, nents, ddir);

	hwif->sg_nents = nents; 			/* remember total for when we can eventually call pci_unmap_sg() */
	hwif->sg_dma_direction = ddir;

	state->sg_table = hwif->sg_table;
	state->sg_partial_offset = 0;
	state->sg_nents_remaining = nents;

	return 0;
}

/*
   Take mappings from the sg list and construct a corresponding prd table.
   Stop when either the sg list is exhausted or the table entry limit is reached.
*/
static int pnx0106_ide_sglist_consume (ide_hwif_t *hwif)
{
	struct pnx0106_ide_state *state = hwif->hwif_data;
	unsigned int *table = hwif->dmatable_cpu;
	unsigned int table_entries_count;
	unsigned int table_entries_limit;
	struct scatterlist *sg;
	unsigned int nents_remaining;
	dma_addr_t addr_cur;
	unsigned int length_remaining_total;
	unsigned int length_remaining_cur;
	unsigned int sector_count;

	table_entries_count = 0;
	if (hwif->sg_dma_direction == PCI_DMA_FROMDEVICE)
		table_entries_limit = PNX0106_PRD_ENTRIES_MAX_READ;
	else
		table_entries_limit = PNX0106_PRD_ENTRIES_MAX_WRITE;

	sg = state->sg_table;
	addr_cur = sg_dma_address (sg) + state->sg_partial_offset;
	length_remaining_total = sg_dma_len (sg);
	length_remaining_cur = length_remaining_total - state->sg_partial_offset;
	nents_remaining = state->sg_nents_remaining;
	sector_count = 0;

//	DBG (5, "sg = 0x%08lx, sg_dma_address = 0x%08lx, sg_dma_len = %5d, hwif->sg_nents = %d\n", (unsigned long) sg, (unsigned long) sg_dma_address (sg), sg_dma_len (sg), hwif->sg_nents);
//	DBG (5, "sg_partial_offset = %5d, nents_remaining = %d, length_remaining_total = %5d\n", state->sg_partial_offset, nents_remaining, length_remaining_total);

	while (1) {
		unsigned int entry_len = (length_remaining_cur < (4 * 1024)) ? length_remaining_cur : (4 * 1024);
		*table++ = cpu_to_le32 (addr_cur);
		*table++ = cpu_to_le32 ((entry_len >> 2) - 1);

//		DBG (5, "table[%2d]: addr = 0x%08x, len_remaining = %5d, entry_len = %5d\n", table_entries_count, addr_cur, length_remaining_cur, entry_len);

		addr_cur += entry_len;
		length_remaining_cur -= entry_len;
		sector_count += (entry_len / 512);

		if (length_remaining_cur == 0) {
			if (--nents_remaining == 0) {
				sg = NULL;
				break;
			}
			sg++;
			addr_cur = sg_dma_address (sg);
			length_remaining_total = sg_dma_len (sg);
			length_remaining_cur = length_remaining_total;
		}

		table_entries_count++;
#if defined (RESTART_SHORT_XFERS_AFTER_FIRST_PRD)
		if (table_entries_count == 1) {
			state->sg_table_first_prd = sg;
			state->sg_partial_offset_first_prd = (length_remaining_total - length_remaining_cur);
			state->sg_nents_remaining_first_prd = nents_remaining;
			state->sector_count_first_prd = sector_count;
		}
#endif
		if (table_entries_count == table_entries_limit)
			break;
	}

	table[-1] |= cpu_to_le32 (0x80000000);

#if 0
	{
		int fff = 0;

		while (1) {
			printk ("%08x: %04x\n", hwif->dmatable_cpu[(2*fff) + 0], (hwif->dmatable_cpu[(2*fff) + 1] & 0x0FFF) | (hwif->dmatable_cpu[(2*fff) + 1] >> 16));
			if (hwif->dmatable_cpu[(2*fff) + 1] & 0x80000000)
				break;
			fff++;
		}
	}
#endif

//	DBG (5, "sg_partial_offset new = %5d, sector_count = %5d\n", (length_remaining_total - length_remaining_cur), sector_count);

	state->sg_table = sg;
	state->sg_partial_offset = (length_remaining_total - length_remaining_cur);
	state->sg_nents_remaining = nents_remaining;
	state->sector_count = sector_count;

	hwif->dmatable_dma = pci_map_single (NULL, hwif->dmatable_cpu, PNX0106_PRD_TABLE_SIZE, PCI_DMA_TODEVICE);
	writel (cpu_to_le32 (hwif->dmatable_dma), &PCCARD->BM_DescriptorTablePointer);

	return 0;
}

static void pnx0106_ide_sglist_unmap (ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct scatterlist *sg = hwif->sg_table;
	int nents = hwif->sg_nents;
	int ddir = hwif->sg_dma_direction;

	pci_unmap_sg (NULL, sg, nents, ddir);
}

static void continue_xfer_outb (u8 addr, unsigned long port)
{
	DBG (4, "%s : %s = 0x%02X\n", __FUNCTION__, port_to_reg_string_write (port), addr);
	outb (addr, port);
}

static int continue_xfer (ide_drive_t *drive, int init_timeouts)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;
	unsigned int timeout_ticks;
	unsigned int start_sector;
	unsigned int sector_count;
	task_ioreg_t command;
	int ddir;
	int lba48;
	unsigned long flags;

	pnx0106_ide_sglist_consume (hwif);

	ddir = hwif->sg_dma_direction;
	lba48 = (drive->addressing == 1) ? 1 : 0;
	if (ddir == PCI_DMA_FROMDEVICE) {
		writel (PNX0106_BM_COMMAND_START_READ, &PCCARD->BM_Command);
		command = (lba48) ? WIN_READDMA_EXT : WIN_READDMA;
	}
	else {
		writel (PNX0106_BM_COMMAND_START_WRITE, &PCCARD->BM_Command);
		command = (lba48) ? WIN_WRITEDMA_EXT : WIN_WRITEDMA;
	}

	start_sector = state->start_sector;
	sector_count = state->sector_count;

	DBG (2, "%s: %s %d (%d) from 0x%08x\n",
		drive->name, (ddir == PCI_DMA_FROMDEVICE) ? "read" : "write",
		sector_count, wibble_nr_sectors, start_sector);

	if (lba48) {
		continue_xfer_outb (((sector_count >> 8) & 0xFF), (unsigned long) &PCCARD->IDE_SectorCount);
		continue_xfer_outb (((sector_count >> 0) & 0xFF), (unsigned long) &PCCARD->IDE_SectorCount);
		continue_xfer_outb (0, (unsigned long) &PCCARD->IDE_CylinderHigh);
		continue_xfer_outb (0, (unsigned long) &PCCARD->IDE_CylinderLow);
		continue_xfer_outb (((start_sector >> 24) & 0xFF), (unsigned long) &PCCARD->IDE_SectorNumber);
		continue_xfer_outb (((start_sector >> 16) & 0xFF), (unsigned long) &PCCARD->IDE_CylinderHigh);
		continue_xfer_outb (((start_sector >> 8) & 0xFF), (unsigned long) &PCCARD->IDE_CylinderLow);
		continue_xfer_outb (((start_sector >> 0) & 0xFF), (unsigned long) &PCCARD->IDE_SectorNumber);
	}
	else {
		unsigned int temp;
		continue_xfer_outb (sector_count, (unsigned long) &PCCARD->IDE_SectorCount);
		continue_xfer_outb (((start_sector >> 0) & 0xFF), (unsigned long) &PCCARD->IDE_SectorNumber);
		continue_xfer_outb (((start_sector >> 8) & 0xFF), (unsigned long) &PCCARD->IDE_CylinderLow);
		continue_xfer_outb (((start_sector >> 16) & 0xFF), (unsigned long) &PCCARD->IDE_CylinderHigh);
		temp = inb (&PCCARD->IDE_DriveSelect);
		temp = ((temp & ~0x0F) | ((start_sector >> 24) & 0x0F));
		continue_xfer_outb (temp, (unsigned long) &PCCARD->IDE_DriveSelect);
	}

	spin_lock_irqsave (&io_request_lock, flags);

	hwif->hwgroup->handler = &pnx0106_ide_dma_isr;
	hwif->hwgroup->expiry = NULL;			/* fixme... this needs to be set... otherwise timer setup below is pointless... */

	if (init_timeouts) {
		if (hwif->hwgroup->expiry != NULL) {
			hwif->hwgroup->timer.expires = (jiffies + (((SW_CMD_TIMEOUT_MSEC * HZ) + 999) / 1000));
			add_timer (&hwif->hwgroup->timer);
		}
	}

#if (BM_TIMEOUT_SECONDS != 0)
	writel ((BM_TIMEOUT_SECONDS * 64 * 1000000), &PCCARD->BM_TimeOut);
#endif
	outb (command, &PCCARD->IDE_Command);

	spin_unlock_irqrestore (&io_request_lock, flags);

	return 0;
}

static ide_startstop_t pnx0106_ide_dma_isr (ide_drive_t *drive)
{
	int i;
	ide_hwif_t *hwif = drive->hwif;
	struct pnx0106_ide_state *state = hwif->hwif_data;
	struct request *rq = hwif->hwgroup->rq;
	unsigned int bmdma_status;
	unsigned int ata_status;

#if (BM_TIMEOUT_SECONDS != 0)
	writel (0, &PCCARD->BM_TimeOut);			/* stop HW timeout timer... */
#endif
	bmdma_status = readl (&PCCARD->BM_Interrupt);
	writel (bmdma_status, &PCCARD->BM_Interrupt);		/* clear the BM interrupts that we now know about */

	DBG (3, "%s: bmdma_status 0x%02x, ata_int 0x%02x\n", __FUNCTION__, bmdma_status, readl (&PCCARD->Interrupt));

	pci_unmap_single (NULL, hwif->dmatable_dma, PNX0106_PRD_TABLE_SIZE, PCI_DMA_TODEVICE);

	if (bmdma_status & 0x04) {				/* BM controller says transfer is complete */
		ata_status = readl (&PCCARD->IDE_Status);

		DBG (4, "%s: ata_status 0x%02x\n", __FUNCTION__, ata_status);

//		if (OK_STAT (ata_status, DRIVE_READY, (BAD_R_STAT | BAD_W_STAT))) {
		if (OK_STAT (ata_status, DRIVE_READY, drive->bad_wstat | DRQ_STAT)) {
			if (state->sg_table != NULL) {
				state->start_sector += state->sector_count;	/* setup next transfer */
				continue_xfer (drive, 0);
				return ide_started;
			}

#if 0
			while (drive->driver->end_request (drive, END_REQUEST_STATUS_SUCCESS) != 0)
				;

#else
			for (i = rq->nr_sectors; i > 0;) {
				i -= rq->current_nr_sectors;
				if (drive->driver->end_request (drive, END_REQUEST_STATUS_SUCCESS) == 0) {
					if (i != 0)
						DBG (1, "pnx0106_ide_dma_isr: end_request complete, i = %d\n", i);
					break;
				}
			}
#endif

			automation_off (drive);
			state->last_command_complete_tsc = ReadTSC64();
			drive->waiting_for_dma = 0;
			pnx0106_ide_sglist_unmap (drive);
			return ide_stopped;
		}

		printk ("%s: error\n", __FUNCTION__);

		state->last_command_complete_tsc = ReadTSC64();
		drive->waiting_for_dma = 0;

		return DRIVER(drive)->error (drive, __FUNCTION__, ata_status);
	}
	else {
		unsigned int validation = readl (&PCCARD->Validation);

		printk ("%s: %s: bmdma_status 0x%02x, ata_int 0x%02x, ata_validation 0x%02x\n",
			drive->name, (hwif->sg_dma_direction == PCI_DMA_FROMDEVICE) ? "Read" : "Write",
			bmdma_status, readl (&PCCARD->Interrupt), validation);

		if (bmdma_status & 0x20) {
			printk ("%s: Interrupt: BM timer (ATA Validation = 0x%02x)\n", drive->name, validation);

			if ((validation && (1 << 2)) == 0) {		/* ATA block is no longer active */
				/*
				   This situation is most likely the result of a HW bug in which the Bus
				   Master never completes writing data to SDRAM if it is blocked for too
				   long while writing the final bytes of the transfer (e.g. if AHB bus
				   is saturated by other traffic). Note that this can happen even with
				   single PRD read transfers... so when restarting the command, we
				   have to begin right from the beginning. As the Bus Master is stuck,
				   we need a soft abort of the ATA block (which will turn off automation).

				   Strategy is to duplicate the basic setup from pnx0106_ide_sglist_setup()
				   (but _without_ re-creating sg list...) and then call automation_on()
				   and continue_xfer() to re-do all the initial taskfile setup etc and
				   re-issue the ATA command.

				   Fixme... should also verify that data transfer direction is read
				   (HW bug shouldn't be seen for writes...)
				*/

//				dump_regs();
				writel ((1 << 1), &PCCARD->SoftReset); 		/* Bit1: Abort : minimal reset of ATA and bus master blocks... */
//				dump_regs();

				/*
				   Some Bus Master interrupts seem to get set during the abort sequence ?!?.
				   Clear them again here.
				*/
				writel (BM_INT_MASK, &PCCARD->BM_Interrupt);
				writel (BM_INT_MASK, &PCCARD->BM_InterruptEnable);

				state->sg_table = hwif->sg_table;
				state->sg_partial_offset = 0;
				state->sg_nents_remaining = hwif->sg_nents;
				automation_on (drive);				/* will setup state->start_sector with a call to playback_taskfile() */
				continue_xfer (drive, 0);
				return ide_started;
			}
		}

		if (bmdma_status & 0x02) {
#if defined (RESTART_SHORT_XFERS_AFTER_FIRST_PRD)
			printk ("%s: Interrupt: BM short xfer (continue from 2nd PRD)\n", drive->name);
			state->sg_table = state->sg_table_first_prd;
			state->sg_partial_offset = state->sg_partial_offset_first_prd;
			state->sg_nents_remaining = state->sg_nents_remaining_first_prd;
			state->sector_count = state->sector_count_first_prd;
			state->start_sector += state->sector_count;		/* setup next transfer */
			continue_xfer (drive, 0);
			return ide_started;
#else
			printk ("%s: Interrupt: BM short xfer (handle with soft reset)\n", drive->name);
			writel ((1 << 1), &PCCARD->SoftReset); 			/* Bit1: Abort : minimal reset of ATA and bus master blocks... */
			writel (BM_INT_MASK, &PCCARD->BM_Interrupt);
			writel (BM_INT_MASK, &PCCARD->BM_InterruptEnable);
			state->sg_table = hwif->sg_table;
			state->sg_partial_offset = 0;
			state->sg_nents_remaining = hwif->sg_nents;
			automation_on (drive);					/* will setup state->start_sector with a call to playback_taskfile() */
			continue_xfer (drive, 0);
			return ide_started;
#endif
		}

		if (bmdma_status & 0x01)
			panic ("%s: Interrupt: BM short tables\n", drive->name);
		if (bmdma_status & 0x10)
			panic ("%s: Interrupt: BM bus error\n", drive->name);
	}

	/* should never get here... */

	return ide_stopped;
}


/*******************************************************************************
*******************************************************************************/

static int pnx0106_ide_dma_host_on (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_host_off (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	automation_off (drive);
	return 0;
}

/*
   Returning non-zero means abort dma and fall back to PIO instead.
*/
static int pnx0106_ide_dma_xfer (ide_drive_t *drive, int ddir)
{
	if (drive->media != ide_disk)
		return 1;

	if (unlikely (pnx0106_ide_sglist_setup (drive, ddir) != 0))
		return 1;

	automation_on (drive);
	drive->waiting_for_dma = 1;

	return continue_xfer (drive, 1);
}

static int pnx0106_ide_dma_read (ide_drive_t *drive)
{
	return pnx0106_ide_dma_xfer (drive, PCI_DMA_FROMDEVICE);
}

static int pnx0106_ide_dma_write (ide_drive_t *drive)
{
	return pnx0106_ide_dma_xfer (drive, PCI_DMA_TODEVICE);
}

static int pnx0106_ide_dma_begin (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_end (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	drive->waiting_for_dma = 0;
	return 0;
}

static int pnx0106_ide_dma_on (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	drive->using_dma = 1;
	return pnx0106_ide_dma_host_on (drive);
}

static int pnx0106_ide_dma_off_quietly (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	drive->using_dma = 0;
	return pnx0106_ide_dma_host_off (drive);
}

static int pnx0106_ide_dma_off (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	DBG (3, "%s: DMA disabled\n", drive->name);
	return pnx0106_ide_dma_off_quietly (drive);
}

static int pnx0106_ide_dma_check (ide_drive_t *drive)
{
//	struct hd_driveid *id = drive->id;
	ide_hwif_t *hwif = HWIF(drive);
	int speed;
	int map;

	DBG (3, "%s\n", __FUNCTION__);

	map = (XFER_PIO | XFER_EPIO | XFER_MWDMA | XFER_UDMA);
	if (hwif->udma_four)
		map |= (XFER_UDMA_66 | XFER_UDMA_100);

	/* fixme... should this be a call to ide_dma_speed() instead ?? */

	speed = ide_find_best_mode (drive, map);

	return 0;

#if 0
	if ((speed & XFER_MODE) != XFER_PIO) {
		pnx0106_ide_dma_on (drive);
		return pnx0106_setspeed_dma (drive, speed);
	}

	return pnx0106_ide_dma_off (drive);
#endif

#if 0

	struct hd_driveid *id = drive->id;
	ide_hwif_t *hwif = HWIF(drive);
	int xfer_mode = XFER_PIO_2;
	int on;

	if (!id || !(id->capability & 1) || !hwif->autodma)
		goto out;

	/*
	 * Consult the list of known "bad" drives
	 */
	if (in_drive_list(id, drive_blacklist)) {
		printk("%s: Disabling DMA for %s (blacklisted)\n",
			drive->name, id->model);
		goto out;
	}

	/*
	 * Enable DMA on any drive that has multiword DMA
	 */
	if (id->field_valid & 2) {
		if (id->dma_mword & 4) {
			xfer_mode = XFER_MW_DMA_2;
		} else if (id->dma_mword & 2) {
			xfer_mode = XFER_MW_DMA_1;
		} else if (id->dma_mword & 1) {
			xfer_mode = XFER_MW_DMA_0;
		}
		goto out;
	}

	/*
	 * Consult the list of known "good" drives
	 */
	if (in_drive_list(id, drive_whitelist)) {
		if (id->eide_dma_time > 150)
			goto out;
		xfer_mode = XFER_MW_DMA_1;
	}

out:
	on = icside_set_speed(drive, xfer_mode);

	if (on)
		return icside_dma_on(drive);
	else
		return icside_dma_off(drive);

#endif

}

static int pnx0106_ide_dma_test_irq (ide_drive_t *drive)
{
#if 1
	return 1;	/* irq isn't shared... so it's always for us */
#else
	unsigned int bmdma_status = readl (&PCCARD->BM_Interrupt);
	DBG (3, "%s: bmdma_status 0x%02x\n", __FUNCTION__, bmdma_status);
	return ((bmdma_status & BM_INT_MASK) != 0) ? 1 : 0;
#endif
}

static int pnx0106_ide_dma_bad_drive (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_good_drive (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 1;
}

static int pnx0106_ide_dma_count (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_verbose (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
//	DBG (3, ", PIO-4");
	return 0;
}

static int pnx0106_ide_dma_retune (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_lostirq (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}

static int pnx0106_ide_dma_timeout (ide_drive_t *drive)
{
	DBG (3, "%s\n", __FUNCTION__);
	return 0;
}


/*******************************************************************************
*******************************************************************************/

#ifdef CONFIG_PROC_FS

static int proc_ata_idle_seconds_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	char *p = page;
	unsigned long long now;
	unsigned int idle_seconds;
	struct pnx0106_ide_state *state = &gstate;

	now = ReadTSC64();
	idle_seconds = (now - state->last_command_complete_tsc) / (ReadTSCMHz() * 1000000);
	p += sprintf (p, "%u\n", idle_seconds);
	if ((len = (p - page) - off) < 0)
		len = 0;
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

#if defined (DEBUG)

static int proc_ata_debug_level_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	char *p = page;

	p += sprintf (p, "%u\n", debug_level);
	if ((len = (p - page) - off) < 0)
		len = 0;
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}

static int proc_ata_debug_level_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
	char sbuf[16 + 1];
	int value;
	int len;

	if (count > 0) {
		len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
		memset (sbuf, 0, sizeof(sbuf));
		if (copy_from_user (sbuf, buffer, len))
			return -EFAULT;
		if (sscanf (sbuf, "%u", &value) == 1)
			wibble_debug_level = debug_level = value;
	}
	return count;
}

#endif

static int __init proc_init (void)
{
	struct proc_dir_entry *res;

	if ((res = create_proc_entry ("ata_idle_seconds", S_IRUGO, NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = proc_ata_idle_seconds_read;
#if defined (DEBUG)
	if ((res = create_proc_entry ("ata_debug_level", (S_IWUSR | S_IRUGO), NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = proc_ata_debug_level_read;
	res->write_proc = proc_ata_debug_level_write;
#endif
	return 0;
}

#else

static int __init proc_init (void)
{
	return 0;
}

#endif


/*******************************************************************************
*******************************************************************************/

void __init ide_init_default_hwifs (void)
{
	int unit;
	hw_regs_t hw;
	ide_hwif_t *hwif = NULL;
	struct pnx0106_ide_state *state = &gstate;

	printk ("PNX0106 IDE: (C) 2005, Andre McCurdy, Philips Semiconductors\n");

	memset (&hw, 0, sizeof(hw_regs_t));

	hw.io_ports[IDE_DATA_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_Data;
	hw.io_ports[IDE_ERROR_OFFSET  ] = (ide_ioreg_t) &PCCARD->IDE_Error;		/* Feature reg when written */
	hw.io_ports[IDE_NSECTOR_OFFSET] = (ide_ioreg_t) &PCCARD->IDE_SectorCount;
	hw.io_ports[IDE_SECTOR_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_SectorNumber;
	hw.io_ports[IDE_LCYL_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_CylinderLow;
	hw.io_ports[IDE_HCYL_OFFSET   ] = (ide_ioreg_t) &PCCARD->IDE_CylinderHigh;
	hw.io_ports[IDE_SELECT_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_DriveSelect;	/* DeviceHead reg when read */
	hw.io_ports[IDE_STATUS_OFFSET ] = (ide_ioreg_t) &PCCARD->IDE_Status;		/* Command reg when written */
	hw.io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) &PCCARD->IDE_DeviceControl;	/* AltStatus reg when read  */

	hw.irq = IRQ_PCCARD;
	hw.chipset = ide_unknown;

	ide_register_hw (&hw, &hwif);

	if (!hwif) {
		printk ("PNX0106 IDE: ide_register_hw() failed\n");
		return;
	}

	memset (state, 0, sizeof(struct pnx0106_ide_state));

	hwif->dmatable_cpu = kmalloc (PNX0106_PRD_TABLE_SIZE, (GFP_KERNEL | GFP_DMA));
	if (!hwif->dmatable_cpu) {
		printk ("PNX0106 IDE: alloc dmatable_cpu failed\n");
		goto failed;
	}

	hwif->sg_table = kmalloc (sizeof(struct scatterlist) * MAX_SG_ENTRIES, GFP_KERNEL);
	if (!hwif->sg_table) {
		printk ("PNX0106 IDE: alloc sg_table failed\n");
		goto failed;
	}

	/*
	   If we want to _disable_ 48bit LBA addressing for this interface (which
	   we don't as the interface handles it fine...) we would set
	   hwif->addressing to 1 here.

	   Note that (hwif->addressing == 1) indicates that the interface does not
	   support 48bit LBA, whereas (drive->addressing == 1) indicates that the
	   drive _does_ support 48bit LBA (and will only be true if hwif->addressing
	   is 0) !!.
	*/

//	hwif->addressing = 1;		/* set to 1 to disable 48bit LBA addressing for this interface (ie master and slave) */
	hwif->rqsize = 256;		/* max sectors per request. Defaults to 128 (ie half ATA limit ?!?) if we don't set it here */

	/* Init ATA PIO functions */
//	hwif->ata_input_data  = pnx0106_ata_input_data;
//	hwif->ata_output_data = pnx0106_ata_output_data;

	hwif->tuneproc = pnx0106_setspeed_pio;
	hwif->speedproc = pnx0106_setspeed_dma;

	/* Init I/O functions */
	hwif->OUTB = pnx0106_outb;
	hwif->OUTBSYNC = pnx0106_outbsync;
	hwif->OUTW = pnx0106_outw;
	hwif->OUTL = pnx0106_outl;
	hwif->OUTSW = pnx0106_outsw;
	hwif->OUTSL = pnx0106_outsl;
	hwif->INB = pnx0106_inb;
	hwif->INW = pnx0106_inw;
	hwif->INL = pnx0106_inl;
	hwif->INSW = pnx0106_insw;
	hwif->INSL = pnx0106_insl;

	/* Init DMA operations */
	hwif->ide_dma_read = pnx0106_ide_dma_read;
	hwif->ide_dma_write = pnx0106_ide_dma_write;
	hwif->ide_dma_begin = pnx0106_ide_dma_begin;
	hwif->ide_dma_end = pnx0106_ide_dma_end;
	hwif->ide_dma_check = pnx0106_ide_dma_check;
	hwif->ide_dma_on = pnx0106_ide_dma_on;
	hwif->ide_dma_off = pnx0106_ide_dma_off;
	hwif->ide_dma_off_quietly = pnx0106_ide_dma_off_quietly;
	hwif->ide_dma_test_irq = pnx0106_ide_dma_test_irq;
	hwif->ide_dma_host_on = pnx0106_ide_dma_host_on;
	hwif->ide_dma_host_off = pnx0106_ide_dma_host_off;
	hwif->ide_dma_bad_drive = pnx0106_ide_dma_bad_drive;
	hwif->ide_dma_good_drive = pnx0106_ide_dma_good_drive;
	hwif->ide_dma_count = pnx0106_ide_dma_count;
	hwif->ide_dma_verbose = pnx0106_ide_dma_verbose;
	hwif->ide_dma_retune = pnx0106_ide_dma_retune;
	hwif->ide_dma_lostirq = pnx0106_ide_dma_lostirq;
	hwif->ide_dma_timeout = pnx0106_ide_dma_timeout;

	hwif->hwif_data = state;

	hwif->autodma = 1;
	hwif->atapi_dma = 1;

#if 0
	hwif->ultra_mask = 0x3f;	/* 0x3f -> all modes upto and including UDMA mode 5 (ie UDMA-100) */
#else
	hwif->ultra_mask = 0x07;	/* 0x07 -> all modes upto and including UDMA mode 2 (ie UDMA-33) */
#endif

	hwif->mwdma_mask = 0x00;	/* 0x00 -> disable all MW DMA modes... */
	hwif->swdma_mask = 0x00;	/* 0x00 -> disable all SW DMA modes... */

	hwif->udma_four = 1;		/* Assume cable is safe for UDMA-66 and above... */

	state->SlotA_Control_0_saved = readl (&PCCARD->SlotA_Control_0);
	printk ("PCCARD->SlotA_Control_0 = 0x%08x\n", state->SlotA_Control_0_saved);

	for (unit = 0; unit < MAX_DRIVES; unit++) {
		ide_drive_t *drive = &hwif->drives[unit];
		pnx0106_ide_dma_host_off (drive);

		drive->unmask = 1;
	}

	writel (0, &PCCARD->BM_TimeOut);
	writel (BM_INT_MASK, &PCCARD->BM_Interrupt);
	writel (BM_INT_MASK, &PCCARD->BM_InterruptEnable);

#if 0
	/* Force DMA on */
	for (unit = 0; unit < MAX_DRIVES; unit++) {
		ide_drive_t *drive = &hwif->drives[unit];

		drive->using_dma = 1;
		drive->special.b.set_multmode = 0;
		drive->special.b.set_geometry = 0;
		drive->special.b.recalibrate = 0;
		drive->mult_req = 0;
		drive->mult_count = 0;
	}
#endif

#if 0
	/*
	   Idea here is to cripple to USB bus master(s) to try to ensure that
	   ATA always has enough AHB bandwidth to avoid triggering the HW bugs
	   associated with slow Bus Master Fifo Flush at the end of ATA read
	   operations (which seem to trigger quite easily when USB is being
	   written to...) however it doesn't seem to help.
	*/
	{
		unsigned int abc_old = readl (VA_SYSCREG_ABC_CFG);
		unsigned int abc_new = abc_old;
		abc_new = abc_new | (1 << 12);
		abc_new = abc_new | (1 << 15);
		writel (abc_new, VA_SYSCREG_ABC_CFG);
		printk ("VA_SYSCREG_ABC_CFG: 0x%08x -> 0x%08x\n", abc_old, abc_new);
	}
#endif

	proc_init();
	return;

failed:
	/* fixme... free any allocated buffers here */
	return;
}

