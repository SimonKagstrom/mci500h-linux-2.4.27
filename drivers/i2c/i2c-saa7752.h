/*
 * i2c Support for Philips SAA7752 I2C Controller
 * Copyright (c) 2003 Philips Semiconductor
 *
 * Author: Ranjit Deshpande <Ranjit@Kenati.com>
 * 
 * $Id: i2c-saa7752.h,v 1.5 2003/10/17 20:40:57 ranjitd Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _I2C_SAA7752_H
#define _I2C_SAA7752_H

typedef enum {
	mstatus_tdi	= 0x00000001,
	mstatus_afi	= 0x00000002,
	mstatus_nai	= 0x00000004,
	mstatus_drmi	= 0x00000008,
	mstatus_active	= 0x00000020,
	mstatus_scl	= 0x00000040,
	mstatus_sda	= 0x00000080,
	mstatus_rff	= 0x00000100,
	mstatus_rfe	= 0x00000200,
	mstatus_tff	= 0x00000400,
	mstatus_tfe	= 0x00000800,
} mstatus_bits_t;

typedef enum {
	mcntrl_tdie	= 0x00000001,
	mcntrl_afie	= 0x00000002,
	mcntrl_naie	= 0x00000004,
	mcntrl_drmie	= 0x00000008,
	mcntrl_rffie	= 0x00000020,
	mcntrl_daie	= 0x00000040,
	mcntrl_tffie	= 0x00000080,
	mcntrl_reset	= 0x00000100,
	mcntrl_cdbmode	= 0x00000400,
} mcntrl_bits_t;

enum {
	sstatus_drsi	= 0x00000010,
	sstatus_active	= 0x00000020,
	sstatus_scl	= 0x00000040,
	sstatus_sda	= 0x00000080,
	sstatus_rff	= 0x00000100,
	sstatus_rfe	= 0x00000200,
	sstatus_tff	= 0x00000400,
	sstatus_tfe	= 0x00000800,
} sstatus_bits_t;

typedef enum {
	scntrl_drsie	= 0x00000010,
	scntrl_rffie	= 0x00000020,
	scntrl_daie	= 0x00000040,
	scntrl_tffie	= 0x00000080,
	scntrl_reset	= 0x00000100,
} scntrl_bits_t;

typedef enum {
	rw_bit		= 0x00000001,
	start_bit	= 0x00000100,
	stop_bit	= 0x00000200,
} i2c_saa7752_prot_bits_t;

typedef enum {
	xmit,
	rcv,
	nodata,
} i2c_saa7752_mode_t;

//#if defined (I2C_SAA7752_DEBUG) || defined (I2C_SAA7752_SLAVE_DEBUG)
char *i2c_saa7752_modestr[] = {
	"Transmit",
	"Receive",
	"No Data",
};
//#endif

typedef struct {
	u32	fifo;		/* Offset 0x00 */
	u32	status;		/* Offset 0x04 */
	u32	cntrl;		/* Offset 0x08 */
	u32	clockhi;	/* Offset 0x0C */
	u32	clocklo;	/* Offset 0x10 */
	u32	reserved;	/* Offset 0x14 */
	u32	rx_level;	/* Offset 0x18 */
	u32	tx_level;	/* Offset 0x1C */
	u32	rx_byte;	/* Offset 0x20 */
	u32	tx_byte;	/* Offset 0x24 */
} i2c_saa7752_master_t;

typedef struct {
	u32	fifo;
	u32	status;
	u32	cntrl;
	u32	clock;
	u32	address;
	u32	rx_level;
	u32	tx_level;
	u32	rx_byte;
	u32	tx_byte;
} i2c_saa7752_slave_t;

typedef struct {
	int 			ret; 	/* Return value */
	i2c_saa7752_mode_t	mode;	/* Interface mode */
	struct semaphore	sem;	/* Mutex for this interface */
	struct completion 	complete;	/* I/O completion */
	struct timer_list	timer;	/* Timeout */
	char *			buf;	/* Data buffer */
	int			len;	/* Length of data buffer */
	int			stop;	/* Issue a stop after the next xfer */
} i2c_saa7752_mif_t;

typedef struct {
		char *		buf;
		int		head;
		int 		tail;
} i2c_saa7752_sif_desc_t;

typedef struct {
	int 			len;
	char 			*buf;
} i2c_saa7752_no_data_t;


typedef struct {
	int 			ret; 	/* Return value */
	i2c_saa7752_mode_t	mode;	/* Interface mode */
	struct semaphore	sem;	/* Mutex for this interface */
	struct completion	tx_complete;	/* Transmit completion */
	struct completion	rx_complete;	/* Receive completion */
	struct timer_list	timer;	/* Timeout */
	int			txlen;	/* Length for reads */
	int			rxlen;	/* Length for reads */
	i2c_saa7752_sif_desc_t	txd;	/* Transmit descriptor */
	i2c_saa7752_sif_desc_t	rxd;	/* Receive descriptor */
	i2c_saa7752_no_data_t	nodata; /* Response to send when no data is available */
	int			ndi;	/* Index into nodata */
} i2c_saa7752_sif_t;

#define master			(*((volatile i2c_saa7752_master_t *) \
					IO_ADDRESS_IICMASTER_BASE))
#define slave			(*((volatile i2c_saa7752_slave_t *) \
					IO_ADDRESS_IICSLAVE_BASE))

#define TIMEOUT			(2 * HZ)	/* 2 seconds seems excessive... but apparently 1 second was not enough for a HAC play command ?? */
#define I2C_BLOCK_SIZE		0x100
#define SLAVE_ADDR		0x7f		/* Slave address */
#define CHIP_NAME		"SAA7752 I2C"
#define ADAPTER_NAME		CHIP_NAME" adapter"
#define I2C_SAA7752_SPEED_KHZ	100
#define CDB_ENGINE_CMD_IF 	0x120
#define SLAVE_BUFFER_SIZE	1024

#endif /* _I2C_SAA7752_H_ */
