/* Register and bit definitions for the ISP1581 USB 2.0 device controller.
 * (C) 2004 Philips Semiconductor Inc.
 * Author: Ranjit Deshpande
 */

#ifndef __ASM_ARCH_HAS7752_USBDEV_H
#define __ASM_ARCH_HAS7752_USBDEV_H

enum epld_isp1581_access_regs {
	intr_pending	= 0x00,		/* Interrupt cause register */
	intr_mask	= 0x02,		/* Interrupt mask register */
	cmd_status	= 0x04,		/* Command status */
	fifo_count	= 0x06,		/* FIFO count */
	fifo_reset	= 0x08,		/* FIFO reset control */
	fifo_port	= 0x0a,		/* FIFO data port (debug) */
	epld_error	= 0x10,		/* Error/Feature register */
	epld_nsector	= 0x12,		/* Number of sectors */
	epld_sector	= 0x14,		/* Sector number/LBA[7:0] */
	epld_lcyl	= 0x16,		/* LBA[15:8]/cylinder low */
	epld_hcyl	= 0x18,		/* LBA[23:16]/cylinder high */
	epld_select	= 0x1a,		/* Drive select */
	epld_cmd_status	= 0x1c,		/* Status/command */
	epld_control	= 0x1e,		/* Alt status/command OR control */
	regs		= 0x7fe0,	/* Register access page frame */
	micro_speed	= 0xbfe0,	/* Acess timings for ISP1581 */
	micro_wait	= 0xbfe2,	/* Wait states for ISP1581 accesses */
	micro_valid	= 0xbfe4,	/* Transaction status */
	data		= 0xbfe6,	/* Data register for reads */
};

enum epld_isp1581_intr_bits {
	isp1581_eoc	= 0x01,		/* End of ATA command */
	isp1581_intr	= 0x02		/* ISP1581 INTRQ */
};

enum epld_isp1581_cmd_status_bits {
	isp1581_active	= 0x01		/* Command is in progress */
};

enum epld_isp1581_micro_valid_bits {
	isp1581_micro_if_active	= 0x01	/* Microprocessor interface is active */
};

union isp1581_mode_reg {
	struct {
		unsigned softct		: 1; 	/* Soft connect */
		unsigned reserved	: 1;	/* Reserved */
		unsigned wkupcs		: 1;	/* Wake on Chip-Select */
		unsigned glintena	: 1;	/* Global interrupt enable */
		unsigned sfreset	: 1;	/* Soft reset */
		unsigned gosusp		: 1;	/* Go suspend */
		unsigned sndrsu		: 1;	/* Send resume */
		unsigned clkaon		: 1;	/* Clock always on */
	} bits;
	u8 val;
};

enum isp1581_int_debug_mode_bits {
	int_on_ack_nak	= 0x00,
	int_on_ack	= 0x01,
	int_on_ack_fnak	= 0x02
};

enum isp1581_intlvl_bits {
	intlvl_level			= 0,
	intlvl_pulsed			= 1
};

enum isp1581_intpol_bits {
	intpol_active_low		= 0,
	intpol_active_high		= 1
};

union isp1581_int_config_reg {
	struct {
		unsigned intpol		: 1;	/* Interrupt polarity */
		unsigned intlvl		: 1;	/* Interrupt Level */
		unsigned ddbgmodout	: 2;	/* Data Debug Mode Out */
		unsigned ddbgmodin	: 2;	/* Data Debug Mode In */
		unsigned cdbgmod	: 2;	/* Control 0 Debug Mode */
	} bits;
	u8 val;
};

union isp1581_int_enable_reg {
	struct {
		unsigned iebrst		: 1;	/* Bus Reset */
		unsigned iesof		: 1;	/* Start of Frame */
		unsigned iepsof		: 1;	/* Pseudo Start of Frame */
		unsigned iesusp		: 1;	/* Suspend */
		unsigned ieresm		: 1;	/* Resume */
		unsigned iehs_sta	: 1;	/* High speed status change */
		unsigned iedma		: 1;	/* DMA status change */
		unsigned reserved1	: 1;	/* Reserved */
		unsigned iep0setup	: 1;	/* Setup data received on EP0 */
		unsigned reserved2	: 1;	/* Reserved */
		unsigned iep0rx		: 1;	/* Control OUT EP0 */
		unsigned iep0tx		: 1;	/* Control IN EP0 */
		unsigned iep1rx		: 1;	/* EP1 OUT */
		unsigned iep1tx		: 1;	/* EP1 IN */
		unsigned iep2rx		: 1;	/* EP2 OUT */
		unsigned iep2tx		: 1;	/* EP2 IN */
		unsigned iep3rx		: 1;	/* EP3 OUT */
		unsigned iep3tx		: 1;	/* EP3 IN */
		unsigned iep4rx		: 1;	/* EP4 OUT */
		unsigned iep4tx		: 1;	/* EP4 IN */
		unsigned iep5rx		: 1;	/* EP5 OUT */
		unsigned iep5tx		: 1;	/* EP5 IN */
		unsigned iep6rx		: 1;	/* EP6 OUT */
		unsigned iep6tx		: 1;	/* EP6 IN */
		unsigned iep7rx		: 1;	/* EP7 OUT */
		unsigned iep7tx		: 1;	/* EP7 IN */
	} bits;
	u32 val;
};

enum isp1581_dma_config_width_bits {
	dcw_8_bit	= 0,
	dcw_16_bit	= 1
};

enum isp1581_dma_config_mode_bits {
	dcm_dior_diow	= 0x00,
	dcm_dior_dack	= 0x01,
	dcm_dack	= 0x02,
	dcm_reserved	= 0x03
};

enum isp1581_dma_config_burst_bits {
	dcb_complete	= 0x00,
	dcb_each	= 0x01,
	dcb_2		= 0x02,
	dcb_4		= 0x03,
	dcb_8		= 0x04,
	dcb_12		= 0x05,
	dcb_16		= 0x06,
	dcb_32		= 0x07
};

enum isp1581_dma_config_pio_mode_bits {
	dcpm_pio_0	= 0x00,
	dcpm_pio_1	= 0x01,
	dcpm_pio_2	= 0x02,
	dcpm_pio_3	= 0x03,
	dcpm_pio_4	= 0x04,
	dcpm_res1	= 0x05,
	dcpm_res2	= 0x06,
	dcpm_res3	= 0x07
};

enum isp1581_dma_config_dma_mode_bits {
	dcdm_umdma0	= 0x00,
	dcdm_umdma1	= 0x01,
	dcdm_umdma2	= 0x02,
	dcdm_mdma3	= 0x03
};

enum isp1581_dma_config_ata_mode_bits {
	dcam_non_ata	= 0x00,
	dcam_ata_mdma	= 0x01
};

union isp1581_dma_config_reg {
	struct {
		unsigned width		: 1;	/* DMA bus width */
		unsigned reserved1	: 1;	/* Reserved */
		unsigned mode		: 2;	/* Handshake signals */
		unsigned burst		: 3;	/* Burst length & DREQ time */
		unsigned dis_xfer_cnt	: 1;	/* Disable transfer count */
		unsigned pio_mode	: 3;	/* PIO modes */
		unsigned dma_mode	: 2;	/* DMA modes */
		unsigned ata_mode	: 1;	/* ATA mode */
		unsigned ignore_iordy	: 1;	/* Ignore IORDY in UDMA mode */
	} bits;
	u16 val;
};

union isp1581_dma_hw_reg {
	struct {
		unsigned read_pol	: 1;	/* DIOR strobe polarity */
		unsigned write_pol	: 1;	/* DIOW strobe polarity */
		unsigned dreq_pol	: 1;	/* DMA request polarity */
		unsigned ack_pol	: 1;	/* DMA ACK polarity */
		unsigned master		: 1;	/* MDMA Master/GDMA slave */
		unsigned eot_pol	: 1;	/* End of Transfer polarity */
		unsigned endian		: 2;	/* Endian mode */
	} bits;
	u8 val;
};

union isp1581_dma_int_enable_reg {
	struct {
		unsigned reserved	: 1;	/* Reserved */
		unsigned cmd_intrq_ok	: 1;	/* DMA count == 0 and INTRQ */
		unsigned tf_rd_done	: 1;	/* Read taskfile completed */
		unsigned bsy_done	: 1;	/* BSY bit == 0 */
		unsigned read_1f0	: 1;	/* 1F0 FIFO contains data */
		unsigned _1f0_rf_e	: 1;	/* 1F0 read FIFO empty */
		unsigned _1f0_wf_f	: 1;	/* 1F0 write FIFO full */
		unsigned _1f0_wf_e	: 1;	/* 1F0 write FIFO empty */
		unsigned dma_xfer_ok	: 1;	/* GDMA/MDMA xfer done */
		unsigned intrq_pending	: 1;	/* Pending interrupt on INTRQ */
		unsigned int_eot	: 1;	/* Internal EOT */
		unsigned ext_eot	: 1;	/* External EOT */
		unsigned odd_ind	: 1;	/* Odd byte packet xfered */
	} bits;
	u16 val;
};

enum isp1581_control_status_bits {
	cs_send_nak		= 0x00,
	cs_send_empty_pkt	= 0x01
};

union isp1581_control_reg {
	struct {
		unsigned stall		: 1;	/* Stall endpoint */
		unsigned status		: 1;	/* Status acknowledge */
		unsigned reserved1	: 1;	/* Reserved */
		unsigned vendp		: 1;	/* Validate endpoint */
		unsigned clbuf		: 1;	/* Clear buffer; */
	} bits;
	u8 val;
};

enum isp1581_ep_index_dir_bits {
	epdir_out		= 0x00,
	epdir_in		= 0x01
};

union isp1581_endpoint_index_reg {
	struct {
		unsigned dir		: 1;	/* Direction bit */
		unsigned endpidx	: 4;	/* Endpoint index */
		unsigned ep0setup	: 1;	/* Select setup buf for EP0 */
	} bits;
	u8 val;
};

enum isp1581_ep_mps_ntrans_bits {
	epmps_ntrans_1pkt	= 0x00,
	epmps_ntrans_2pkt	= 0x01,
	epmps_ntrans_3pkt	= 0x02
};

union isp1581_endpoint_mps_reg {
	struct {
		unsigned ffosz		: 11;	/* Sets FIFO size of EP */
		unsigned ntrans		: 2;	/* Number of transaction */
	} bits;
	u16 val;
};

enum isp1581_ep_type_bits {
	eptype_iso		= 0x01,
	eptype_bulk		= 0x02,
	eptype_intr		= 0x03
};

union isp1581_endpoint_type_reg {
	struct {
		unsigned endptyp	: 2;	/* Endpoint type */
		unsigned dblbuf		: 1;	/* Double buffered ? */
		unsigned enable		: 1;	/* Endpoint enable */
		unsigned noempkt	: 1;	/* No empty packet */
	} bits;
	u16 val;
};

union isp1581_dma_ep_reg {
	struct {
		unsigned dmadir		: 1;	/* Direction of DMA xfer */
		unsigned epidx		: 3;	/* Endpoint index */
	} bits;
	u8 val;
};

union isp1581_address_reg {
	struct {
		unsigned devaddr	: 7;	/* USB device address */
		unsigned deven		: 1;	/* Enable device */
	} bits;
	u8 val;
};

enum isp1581_dma_cmd_defs {
	dcmd_gdma_read		= 0x00,
	dcmd_gdma_write		= 0x01,
	dcmd_udma_read		= 0x02,
	dcmd_udma_write		= 0x03,
	dcmd_pio_read		= 0x04,
	dcmd_pio_write		= 0x05,
	dcmd_mdma_read		= 0x06,
	dcmd_mdma_write		= 0x07,
	dcmd_read_1f0		= 0x0a,
	dcmd_poll_bsy		= 0x0b,
	dcmd_read_tf		= 0x0c,
	dcmd_validate_buf	= 0x0e,
	dcmd_clear_buf		= 0x0f,
	dcmd_restart		= 0x10,
	dcmd_reset_dma		= 0x11,
	dcmd_mdma_stop		= 0x12
};

enum isp1581_reg_offsets {
	/* Initialization registers */
	address 	= 0x00,		/* USB device address and enable */
	mode		= 0x0c,		/* Mode bits */
	intr_config	= 0x10,		/* Interrupt configuration */
	intr_enable	= 0x14,		/* Interrupt source enable */
	dma_config	= 0x38,		/* DMA configuration */
	dma_hw		= 0x3c,		/* DMA hardware configuration */

	/* Data flow registers */
	ep_index	= 0x2c,		/* Endpoint index */
	control		= 0x28,		/* Endpoint buffer management */
	data_port	= 0x20,		/* Data access to endpoint FIFO */
	buffer_len	= 0x1c,		/* Packet size counter */
	ep_max_ps	= 0x04,		/* Maximum packet size */
	ep_type		= 0x08,		/* Endpoint type */
	short_packet	= 0x24,		/* Short packet received on OUT ep */

	/* DMA registers */
	dma_cmd		= 0x30,		/* DMA command */
	dma_count	= 0x34,		/* Byte count for DMA transfer */

	/* Taskfile block */
	tf_data		= 0x40,		/* Data register */
	tf_error	= 0x48,		/* Error/Feature register */
	tf_nsector	= 0x49,		/* Number of sectors */
	tf_sector	= 0x4a,		/* Sector number/LBA[7:0] */
	tf_lcyl		= 0x4b,		/* LBA[15:8]/cylinder low */
	tf_hcyl		= 0x4c,		/* LBA[23:16]/cylinder high */
	tf_select	= 0x4d,		/* Drive select */
	tf_cmd_status	= 0x44,		/* Status/command */
	tf_control	= 0x4e,		/* Alt status/command OR control */
	tf_drive_addr	= 0x4f,		/* Drive address, reserved */
	dma_intr_cause	= 0x50,		/* DMA interrupt cause */
	dma_intr_enable	= 0x54,		/* DMA interrupt mask */
	dma_ep		= 0x58,		/* Select FIFO and direction of xfer */
	dma_timings	= 0x60,		/* DMA timings for MDMA/UDMA */

	/* General registers */
	intr_cr		= 0x18,		/* Interrupt cause bitfield */
	chip_id		= 0x70,		/* Product ID and version */
	frame_num	= 0x74,		/* Last successfull SOF */
	scratch		= 0x78,		/* Saved state during suspend */
	unlock		= 0x7c,		/* Undocumented !!! */
	test_mode	= 0x84		/* Test mode settings */
};

enum isp1581_speeds {
	full_speed	= 0x00,		/* USB 1.x */
	high_speed	= 0x01		/* USB 2.0 */
};

enum isp1581_state_bits {
	st_default	= 0x00,
	st_configured	= 0x01,
	st_addressed	= 0x02,
	st_stalled	= 0x03
};

union isp1581_state {
	struct {
		unsigned speed		: 1;	/* USB 1.x or 2.0 speed */
		unsigned state		: 2;	/* Address or configured */
		unsigned halt		: 1;	/* Endpoint halted ? */
		unsigned iface		: 1;	/* Interface number */
	} bits;
	u32 val;
};


/* USB 2.0 Descriptors */
struct usb_device_qualifier_descriptor {
	u8	bLength;
	u8	bDescriptorType;
	u16	wVersion;
	u8	bDeviceClass;
	u8	bDeviceSubClass;
	u8	bProtocol;
	u8	bMaxPktSize;
	u8	bOtherConfig;
	u8	bReserved;
} __attribute__ ((packed));

struct isp1581_cbw {
	u32	dCBWSignature;
	u32	dCBWTag;
	u32	dCBWDataTransferLen;
	u8 	bmCBWFlags;
	u8	bCBWLun;
	u8	bCBWCBLength;
	u8	bCBWCB[16];
} __attribute__ ((packed));

struct isp1581_csw {
	u32	dCSWSignature;
	u32	dCSWTag;
	u32	dCSWDataResidue;
	u8	bCSWStatus;
} __attribute__ ((packed));

union isp1581_inquiry_data_vers {
	struct {
		unsigned ansi	: 3;		/* Ansi version */
		unsigned ecma	: 3;		/* Reserved */
		unsigned iso	: 2;		/* Logical Unit Number */
	} bits;
	u8 val;
};

struct isp1581_inquiry_data {
	u8	pdt;				/* Peripheral Device Type */
	u8	rmb;				/* Removable Media */
	u8	vers;				/* Versions */
	u8	rdf;				/* Response data format */
	u8	add_len;			/* Additional Length */
	u8	reserved[3];			/* Reserved */
	u8	vendor[8];			/* Vendor information */
	u8	product[16];			/* Product ID */
	u8	version[4];			/* Product version */
} __attribute__ ((packed));

struct isp1581_capacity_list_header {
	u8	reserved[3];
	u8	len;		/* Capacity list length */
} __attribute__ ((packed));

struct isp1581_capacity_desc {
	u32	blocks;				/* Number of blocks */
	u8	desc_code;			/* Descriptor code */
	u8	blen_high;			/* Block length [23:16] */
	u16	blen_low;			/* Block length [15:0] */
} __attribute__ ((packed));

struct isp1581_read_capacity_data {
	u32	last_lba;			/* Last valid LBA */
	u32	blen;				/* Block length */
} __attribute__ ((packed));

union isp1581_request_sense_errcode {
	struct {
		unsigned code	: 7;	/* Error code */
		unsigned valid	: 1;	/* Info field valid ? */
	} bits;
	u8 val;
};

struct isp1581_request_sense_data {
	u8	errcode;
	u8	reserved;
	u8	sense_key;			/* Sense key */
	u8	info1;				/* Info[31:24] */
	u8	info2;				/* Info[23:16] */
	u8	info3;				/* Info[15:08] */
	u8	info4;				/* Info[07:00] */
	u8	add_len;			/* Additional sense length */
	u8	reserved1[4];
	u8	asc;				/* Additional sense code */
	u8	ascq;				/* Additional sense code 
						 * qualifier.
						 */
	u8	reserved2[4];
} __attribute__ ((packed));

union isp1581_mode_param_hdr_wp {
	struct {
		unsigned res1	: 4;
		unsigned dpofua	: 1;
		unsigned res2	: 2;
		unsigned wp	: 1;
	} bits;
	u8 val;
};

struct isp1581_mode_param_hdr {
	u16	mdl;				/* Mode data length */
	u8	mtc;				/* Medium type code */
	u8	wp;
	u8 reserved[4];
} __attribute__ ((packed));

struct isp1581_timer_protect_page {
	u8	page_code;			/* Page code */
	u8	page_len;			/* Page length */
	u8	res1;				/* Reserved */
	u8	timer;				/* Inactivity timer */
	u8	disp_swpp;			/* DISP and SWPP */
	u8	res2[3];
} __attribute__ ((packed));


#define epld_isp1581_regs(base, offs)	\
		((volatile u16 *)EPLD_ISP1581_INT_REGS_BASE)[(base+offs)>>1]
#define TIMEOUT			1000

/* These definitions are not in linux/usb.h */
#define USB_DT_CONFIG1		(USB_DT_CONFIG | 0x100)
#define USB_DT_DEV_QUALIFIER	6
#define USB_DT_OTHER_SPEED	7
#define USB_DT_IFACE_POWER	8

#define HAS7752_USBDEV_IFACES		1
#define HAS7752_USBDEV_ENDPOINTS	2

#define CONFIG_DESC_LENGTH		9
#define ENDPOINT_DESC_LENGTH		7
#define IFACE_DESC_LENGTH		9

#define HAS7752_MANF_INDEX	1
#define HAS7752_PROD_INDEX	2
#define HAS7752_SERIAL_INDEX	3
#define HAS7752_CONFIG1_INDEX	4
#define HAS7752_CONFIG2_INDEX	5
		
#define MANF_ID_STRING		"Philips"
#define PROD_ID_STRING		"Audio Hub"
#define SERIAL_ID_STRING	"12345678"
#define CONFIG1_ID_STRING_HS	"USB 2.0"
#define CONFIG1_ID_STRING_FS	"USB 1.1"
#define CONFIG2_ID_STRING	CONFIG1_ID_STRING_FS
#define PROD_VERS_STRING	"1.0"

#define CBW_SIG			0x43425355
#define CSW_SIG			0x53425355

#define READ_FORMAT_CAPACITIES	0x23		/* Not defined in scsi/scsi.h */
#define SECTOR_SIZE		512

static u16 isp1581_read_reg(u8 reg);
static void isp1581_write_reg(u16 val, u8 reg);
		
static inline u8 isp1581_inb(u8 reg)
{
	return (u8)isp1581_read_reg(reg);
}

static inline u16 isp1581_inw(u8 reg)
{
	return isp1581_read_reg(reg) | 
		(isp1581_read_reg(reg+1) << 8);
}

static inline u32 isp1581_inl(u8 reg)
{
	return (u32)(isp1581_read_reg(reg) | 
		(isp1581_read_reg(reg+1) << 8) |
		(isp1581_read_reg(reg+2) << 16) |
		(isp1581_read_reg(reg+3) << 24));
}

static inline void isp1581_outb(u8 val, u8 reg)
{
	isp1581_write_reg((u16)val, reg);
}

static inline void isp1581_outw(u16 val, u8 reg)
{
	isp1581_write_reg(val & 0xff, reg);
	isp1581_write_reg(((val >> 8) & 0xff), reg + 1);
}

static inline void isp1581_outl(u32 val, u8 reg)
{
	isp1581_write_reg(val & 0xff, reg);
	isp1581_write_reg(((val >> 8) & 0xff), reg + 1);
	isp1581_write_reg(((val >> 16) & 0xff), reg + 2);
	isp1581_write_reg(((val >> 24) & 0xff), reg + 3);
}

static inline void isp1581_send_data(u8 *data, int size)
{
	while (size--)
		isp1581_outb(*data++, data_port);
}

static inline void isp1581_recv_data(u8 *data, int size)
{
	while (size--)
		*data++ = isp1581_inb(data_port);
}

#endif /* __ASM_ARCH_HAS7752_USBDEV_H */
