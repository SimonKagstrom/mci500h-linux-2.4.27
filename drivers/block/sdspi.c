/*
 * linux/drivers/block/sdspi.c
 *
 * Copyright (c) 2006 Andre McCurdy, NXP Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkpg.h>
#include <linux/hdreg.h>
#include <linux/major.h>

#include <asm/arch/spisd.h>
#include <asm/io.h>
#include <asm/uaccess.h>

/*
   Macros to be declared before including blk.h
*/
#define MAJOR_NR		241
#define DEVICE_NAME		"sd"
#define DEVICE_NR(device)	(MINOR(device))
#define DEVICE_NO_RANDOM
#define DEVICE_REQUEST		sdspi_request

#include <linux/blk.h>


/***************************************************************************/

static int hd_sizes[1<<6];
static int hd_maxsect[1<<6];
static struct hd_struct hd[1<<6];

static struct block_device_operations sdspi_bdops;

static struct gendisk hd_gendisk =
{
	.major		= MAJOR_NR,
	.major_name	= DEVICE_NAME,
	.minor_shift	= 6,
	.max_p		= (1 << 6),
	.part		= hd,
	.sizes		= hd_sizes,
	.fops		= &sdspi_bdops,
};


/*****************************************************************************
*****************************************************************************/

#define NCS		(0)	/* number of stuffing bytes added by host between nCS assertion and start of command */
#define NCR_MAX 	(8)	/* number of stuffing bytes added by card between cmd and response */

#define CMD0		( 0)	/* GO_IDLE_STATE            : software reset */
#define CMD1		( 1)	/* SEND_OP_COND             : initiate initialisation process */
#define CMD9		( 9)	/* SEND_CSD                 : read CSD register */
#define CMD10		(10)	/* SEND_CID	            : read CID register */
#define CMD12		(12)	/* STOP_TRANSMISSION        : Stop reading data */
#define CMD13		(13)	/* SEND_STATUS              : */
#define CMD16		(16)	/* SET_BLOCKLEN             : */
#define CMD17		(17)	/* READ_SINGLE_BLOCK        : */
#define CMD18		(18)	/* READ_MULTIPLE_BLOCK      : */
#define CMD24		(24)	/* WRITE_BLOCK              : */
#define CMD25		(25)	/* WRITE_MULTIPLE_BLOCK     : */
#define CMD27		(27)	/* PROGRAM_CSD              : */
#define CMD28		(28)	/* SET_WRITE_PROT           : */
#define CMD29		(29)	/* CLR_WRITE_PROT           : */
#define CMD30		(30)	/* SEND_WRITE_PROT          : */
#define CMD32		(32)	/* ERASE_WR_BLK_START_ADDR  : */
#define CMD33		(33)	/* ERASE_WR_BLK_END_ADDR    : */
#define CMD38		(38)	/* ERASE                    : erase all previously selected write blocks */
#define CMD42		(42)	/* */
#define CMD55		(55)	/* APP_CMD                  : */
#define CMD56		(56)	/* GEN_CMD                  : */
#define CMD58		(58)	/* READ_OCR                 : */
#define CMD59		(59)	/* CRC_ON_OFF               : turns CRC option on or off (least sig bit of arg) */


#define CMD_R1B(cmd)	(((cmd) == CMD12) || ((cmd) == CMD28) || ((cmd) == CMD29) || ((cmd) == CMD38))
#define CMD_R2(cmd)	((cmd) == CMD13)


#define DATABLK_START	0xFE


/***************************************************************************/

static unsigned int readbits_be (unsigned char *ptr, unsigned int bit_offset, unsigned int bit_count)
{
	unsigned int result = 0;

	ptr += (bit_offset / 8);
	bit_offset -= ((bit_offset / 8) * 8);

	if (bit_count >= 32)		/* should never happen... */
		bit_count = 32;

	while (bit_count--) {
		unsigned int carry = (*ptr >> (7 - bit_offset)) & 0x01;
		result = result + result + carry;
		if (++bit_offset >= 8) {
			bit_offset = 0;
			ptr++;
		}
	}

	return result;
}


/***************************************************************************/

static unsigned int sd_cmd_setup (unsigned int cmd, unsigned int arg)
{
	int i;
	unsigned int response;
	unsigned char buf[NCS + 6];

	if (NCS != 0)					/* Note: most cards seem to be OK with (NCS == 0)... */
		memset (buf, 0xFF, NCS);

	buf[NCS + 0] = 0x40 | cmd;
	buf[NCS + 1] = arg >> 24;
	buf[NCS + 2] = arg >> 16;
	buf[NCS + 3] = arg >> 8;
	buf[NCS + 4] = arg >> 0;
	buf[NCS + 5] = 0x95;				/* CRC correct for CMD0 only... (but other commands don't use CRC) */

	spisd_ncs_drive_low();				/* Note: may already be low (eg if this is a command to stop transmission at the end of a multi block transfer) */
	spisd_write_multi (buf, sizeof(buf));

#if 0
	for (i = 0; i < sizeof(buf); i++)
		printk ("0x%02x ", buf[i]);
	bl_newline();
#endif

	for (i = 0; i < NCR_MAX; i++) {			/* collect upto NCR_MAX (ie 8) bytes of 0xFF before "Card Response" data is sent... */
		response = spisd_read_single8();
//		printk ("0x%02x\n", response);
		if (response != 0xFF)
			break;
	}

	if (response == 0xFF) {
		response = (unsigned int) -1;
		goto done;
	}

	if (CMD_R1B(cmd)) {				/* Command generates R1b response */
		i = 0;
		while (spisd_read_single8() == 0x00) {	/* wait for idle state on DataOut before continuing */
			i++;				/* Fixme: should timeout here !! eg if card is removed while we are polling, we could poll forever !! */
		}
		if (i >= 8)				/* filter out simple cases such as end of multi block read (when idle state seems to happen on first polling attempt) */
			printk ("CMD_R1B command complete after %d attempts\n", i + 1);
	}
	else if (CMD_R2(cmd))						/* Command generates R2 response */
		response = (response << 8) | spisd_read_single8();	/* collect second byte of response */

done:
//	printk ("sd_cmd_setup: cmd 0x%02x arg 0x%x, resp: 0x%x (%d usec)\n", cmd, arg, response, (ReadTSC() - start) / ReadTSCMHz());
	return response;
}

static void sd_cmd_complete (void)
{
	spisd_ncs_drive_high();
	spisd_write_single8 (0xFF);			/* clock extra byte to allow card to set data out to Hi-Z (fixme... do we even want to do that ?!?) */
}

static unsigned int sd_cmd (unsigned int cmd, unsigned int arg)
{
	unsigned int response = sd_cmd_setup (cmd, arg);
	sd_cmd_complete();
	return response;
}

static unsigned int sd_cmd_read (unsigned int cmd, unsigned int arg, unsigned char *buf, unsigned int blksize, unsigned int blkcount)
{
	int i;
	unsigned int crc;
	unsigned int response;

	response = sd_cmd_setup (cmd, arg);		/* NCS + Read command + NCR + card response */
	if (response == (unsigned int) -1) {		/* no response... most likely reason is no card */
		printk ("%s: no response\n", "sd_cmd_read");
		goto done;
	}

	while (blkcount--) {
		for (i = 0; i < 8192; i++) { 		/* Fixme: limit should be NAC_MAX for all commands except SEND_CSR (which uses NCR_MAX instead) */
			response = spisd_read_single8();
//			printk ("0x%02x\n", response);
			if (response == DATABLK_START) {
				if (i > 1024)
					printk ("DATABLK_START after %d attempts\n", i + 1);
				break;
			}
		}

		if (response != DATABLK_START) {	/* no data block start... most likely reason is... ?!? (maybe incorrect NAC_MAX ?!?) */
			printk ("%s: no data block start (resp = 0x%02x)\n", "sd_cmd_read", response);
			response = (unsigned int) -1;
			goto done;
		}

		spisd_read_multi (buf, blksize);		/* Read data... */
		crc = spisd_read_single8();		/* read CRC msbyte... */
		crc = (crc << 8) + spisd_read_single8();	/* read CRC lsbyte... */

//		bl_hexdump (buf, blksize);
//		printk ("0x%04x (CRC)\n", crc);

		buf += blksize;
	}

done:
	if (cmd == CMD18)				/* CMD18 == READ_MULTIPLE_BLOCK */
		response = sd_cmd (CMD12, 0);		/* CMD12 == STOP_TRANSMISSION (will call sd_cmd_complete() as part of sending CMD12) */
	else
		sd_cmd_complete();

	return response;
}


/***************************************************************************/

#define END_REQUEST_STATUS_IO_ERROR	0
#define END_REQUEST_STATUS_SUCCESS	1

static void sdspi_request (request_queue_t *q)
{
	while (1)
	{
		unsigned int lba;
		unsigned int nr_sectors;
		unsigned char *buffer_address;
		int status = END_REQUEST_STATUS_SUCCESS;
		int cmd;

		INIT_REQUEST;

		lba = (CURRENT->sector + hd[MINOR(CURRENT->rq_dev)].start_sect);
		nr_sectors = CURRENT->current_nr_sectors;
		buffer_address = CURRENT->buffer;
		cmd = CURRENT->cmd;
		
		if ((lba + nr_sectors) > hd[0].nr_sects) {
			status = END_REQUEST_STATUS_IO_ERROR;
		}

		else if (cmd == READ) {
			spin_unlock_irq (&io_request_lock);
//			printk ("sdrd: %3d from %6d\n", nr_sectors, lba);
			if (sd_cmd_read (CMD18, (lba * 512), buffer_address, 512, nr_sectors) == (unsigned int) -1)
				status = END_REQUEST_STATUS_IO_ERROR;
			spin_lock_irq(&io_request_lock);
		}

		else if (cmd == WRITE) {
			spin_unlock_irq (&io_request_lock);
//			printk ("sdwr: %3d to   %6d\n", nr_sectors, lba);
			while (nr_sectors--) {
				int i;
				unsigned int response;
				unsigned int offset = (lba * 512);

				response = sd_cmd_setup (CMD24, offset);	/* NCS + Read command + NCR + card response */
				if (response == (unsigned int) -1) {		/* no response... most likely reason is no card */
					printk ("%s: no response\n", "sd_write");
					status = END_REQUEST_STATUS_IO_ERROR;
					sd_cmd_complete();
					goto abort_write;
				}

				spisd_write_single8 (DATABLK_START);
				spisd_write_multi (buffer_address, 512);	/* Data */
				spisd_write_single16_be (0x0000);		/* Fake CRC */

				response = spisd_read_single8();		/* Data response */
				response &= 0x1F;				/* meaning of upper 3 bits is not specified ?!? */

				if (response == 0x05) {
//					printk ("%s: Data accepted\n", "sd_write");
				}
				else if (response == 0x0B) {
					printk ("%s: Data rejected (CRC error)\n", "sd_write");
					status = END_REQUEST_STATUS_IO_ERROR;
					sd_cmd_complete();
					goto abort_write;
				}
				else if (response == 0x0D) {
					printk ("%s: Data rejected (write error)\n", "sd_write");
					status = END_REQUEST_STATUS_IO_ERROR;
					sd_cmd_complete();
					goto abort_write;
				}

				i = 0;
				while (spisd_read_single8() == 0x00) {		/* wait for idle state on DataOut before continuing */
					i++;					/* Fixme: should timeout here !! eg if card is removed while we are polling, we could poll forever !! */
				}
				if (i >= (128 * 1024))
					printk ("%s: idle after %d attempts\n", "sd_write", i + 1);

				sd_cmd_complete();

				buffer_address += 512;
				lba += 1;
			}
abort_write:
			spin_lock_irq (&io_request_lock);
		}
		else
		{
			status = END_REQUEST_STATUS_IO_ERROR;
		}

		end_request (status);
	}
}

static int sdspi_open (struct inode *inode, struct file *filp)
{
//	if (sdspi_media_detect == 0)
//		return -ENODEV;
	return 0;
}

static int sdspi_release (struct inode *inode, struct file *filp)
{
	fsync_dev (inode->i_rdev);
	invalidate_buffers (inode->i_rdev);
	return 0;
}

static int sdspi_revalidate (kdev_t dev)
{
	int target;
	int max_p;
	int start;
	int i;

//	if (sdspi_media_detect == 0)
//		return -ENODEV;

	target = DEVICE_NR(dev);

	max_p = hd_gendisk.max_p;
	start = target << 6;
	for (i = max_p - 1; i >= 0; i--) {
		int minor = start + i;
		invalidate_device (MKDEV(MAJOR_NR, minor), 1);
		hd_gendisk.part[minor].start_sect = 0;
		hd_gendisk.part[minor].nr_sects = 0;
	}

	grok_partitions (&hd_gendisk, target, (1 << 6), hd_sizes[0] * 2);
	return 0;
}

static int sdspi_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (!inode || !inode->i_rdev)
		return -EINVAL;

	switch (cmd)
	{
#if 0
		case BLKGETSIZE:
			printk ("SD: BLKGETSIZE: %d\n", hd[MINOR(inode->i_rdev)].nr_sects);
			return put_user (hd[MINOR(inode->i_rdev)].nr_sects, (unsigned long *) arg);

		case BLKGETSIZE64:
			printk ("SD: BLKGETSIZE64: %d\n", hd[MINOR(inode->i_rdev)].nr_sects);
			return put_user ((u64) hd[MINOR(inode->i_rdev)].nr_sects, (u64 *) arg);
#endif

		case BLKRRPART:
			printk ("SD: BLKRRPART\n");
			if (!capable(CAP_SYS_ADMIN))
				return -EACCES;
			return sdspi_revalidate (inode->i_rdev);

		case HDIO_GETGEO:
		{
			struct hd_geometry *loc, g;
			loc = (struct hd_geometry *) arg;
			if (!loc)
				return -EINVAL;

			printk ("SD: HDIO_GETGEO\n");
			g.heads = 4;
			g.sectors = 16;
			g.cylinders = hd[0].nr_sects / (4 * 16);
			g.start = hd[MINOR(inode->i_rdev)].start_sect;
			return copy_to_user (loc, &g, sizeof(g)) ? -EFAULT : 0;
		}

		default:
			printk ("SD: IOCTL 0x%04x\n", cmd);
			return blk_ioctl (inode->i_rdev, cmd, arg);
	}
}

static struct block_device_operations sdspi_bdops =
{
	.open		= sdspi_open,
	.release	= sdspi_release,
	.ioctl		= sdspi_ioctl,
};


/***************************************************************************/

int sdspi_probe (void)
{
	int i;
	unsigned char csd[16];				/* csd == Card Specific Data (16 bytes) */
	unsigned int response;
	unsigned int block_len;
	unsigned int c_size;
	unsigned int c_size_mult;
	unsigned int block_nr;
	unsigned int retries = 0;

retry:
	if (retries++ == 4)
		return -ENODEV;

	spisd_init();					/* Note: assume that nCS is left high after spisd_init()... */
	for (i = 0; i < 10; i++)
		spisd_write_single8 (0xFF);		/* (more than) 74 SCLK pulses with data and nCS high */

	response = sd_cmd (CMD0, 0);
	if (response == (unsigned int) -1) {
		printk ("sd_init: GO_IDLE_STATE error\n");
		goto retry;
	}

	for (i = 0; i < 1000; i++) {
		response = sd_cmd (CMD1, 0);		/* 0x00 when card enters idle state */
		if (response == 0x00)
			break;
		udelay (100);
	}

	if (response != 0x00) {
		printk ("sd_init: timeout waiting for card to enter idle state\n");
		goto retry;
	}

	response = sd_cmd_read (CMD9, 0, csd, sizeof(csd), 1);	/* */
	if (response == (unsigned int) -1) {
		printk ("sd_init: SEND_CSD error\n");
		goto retry;
	}

	block_len = 1 << readbits_be (csd, (127 - 83), 4);	/* expect 512, 1024 or 2048 bytes */
	c_size = readbits_be (csd, (127 - 73), 12);		/* expect 0..4095 */
	c_size_mult = readbits_be (csd, (127 - 49), 3); 	/* expect 0..7 */

	block_nr = (c_size + 1) * (1 << (c_size_mult + 2));	/* max is (4096 * 512) == 2M blocks */

	printk ("SD: CSD:");
	for (i = 0; i < sizeof(csd); i++)
		printk (" %02x", csd[i]);
	printk ("\n");

	printk ("SD: block_len: %d, block_nr %d\n", block_len, block_nr);

	if (block_len < 512)					/* should never happen... corrupt CSD ?!? */
		goto retry;

	if (block_len > 512) {
		block_nr *= (block_len / 512);			/* scale block_nr to be in terms of 512 byte blocks... */
		printk ("SD: setting block length to 512\n");
		response = sd_cmd (CMD16, 512); 		/* CMD16 == SET_BLOCKLEN */
		if (response == (unsigned int) -1)
			goto retry;
	}

	memset (hd_sizes, 0, sizeof(hd_sizes));
	hd_sizes[0] = (block_nr / 2);				/* used by kernel to check if accesses are beyond the bounds of a device. Units of 1k */

	blk_size[MAJOR_NR] = hd_sizes;
	blksize_size[MAJOR_NR] = NULL;				/* setting to NULL means we will use the default (1 kbytes) */
	hardsect_size[MAJOR_NR] = NULL; 			/* setting to NULL means we will use the default (512 bytes) */

	for (i = 0; i < (1 << 6); i++)
		hd_maxsect[i] = 256;				/* max hard sectors per request */

	max_sectors[MAJOR_NR] = hd_maxsect;

	memset (hd, 0, sizeof(hd));
	hd[0].nr_sects = block_nr;				/* 512 byte sectors... */

	hd_gendisk.nr_real = 1;
	register_disk (&hd_gendisk, MKDEV(MAJOR_NR, 0), (1 << 6), &sdspi_bdops, hd_sizes[0] * 2);

	return 0;
}


/*******************************************************************************
*******************************************************************************/

static int __init sdspi_init (void)
{
	int result;

	if ((result = register_blkdev (MAJOR_NR, DEVICE_NAME, &sdspi_bdops)) < 0) {
		printk ("%s: Unable to register major device %d\n", DEVICE_NAME, MAJOR_NR);
		return result;
	}

	blk_init_queue (BLK_DEFAULT_QUEUE(MAJOR_NR), sdspi_request);
	read_ahead[MAJOR_NR] = 8;
	add_gendisk (&hd_gendisk);

	sdspi_probe();

	return 0;
}

__initcall (sdspi_init);


/*******************************************************************************
*******************************************************************************/

