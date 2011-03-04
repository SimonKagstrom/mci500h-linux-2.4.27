/*
 * Header file for i2c support for PNX0106.
 *
 * Author: Srinivasa Mylarappa <srinivasa.mylarappa@philips.com>
 *
 * 2006 (c) Philips Semiconductors, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _I2C_PNX_H
#define _I2C_PNX_H
/*#ifndef CONFIG_I2C_PNX_SLAVE_SUPPORT*/
	#define CONFIG_I2C_PNX_SLAVE_SUPPORT
/*#endif*/

/* --------------------------------------------------------------------------
*  The I2C device description
*  -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
*  Register offsets from base address
*  -------------------------------------------------------------------------- */

#define I2C_RX_FIFO_REG     0x00
#define I2C_TX_FIFO_REG     0x00
#define I2C_STS_REG         0x04
#define I2C_CTL_REG         0x08
#define I2C_CKL_REG         0x0C
#define I2C_CKH_REG         0x10
#define I2C_ADR_REG         0x14
#define I2C_RFL_REG         0x18
#define I2C_TFL_REG         0x1C
#define I2C_RXB_REG         0x20
#define I2C_TXB_REG         0x24
#define I2C_TXS_REG         0x28
#define I2C_STFL_REG        0x2C

#define I2C_REGS_NUMBER         12

/* --------------------------------------------------------------------------
*  Device description
*  -------------------------------------------------------------------------- */

typedef volatile struct {
	/* LSB */
	union {
		const u32 rx; /* Receive FIFO Register - Read Only */
		u32 tx; /* Transmit FIFO Register - Write Only */
	} fifo;
	u32 sts;  /* Status Register - Read Only */
	u32 ctl;  /* Control Register - Read/Write */
	u32 ckl;  /* Clock divider low  - Read/Write */
	u32 ckh;  /* Clock divider high - Read/Write */
	u32 adr;  /* I2C address (optional) - Read/Write */
	const u32 rfl;  /* Rx FIFO level (optional) - Read Only */
	const u32 _unused;     //  const UInt32 tfl;  /* Tx FIFO level (optional) - Read Only */
	const u32 rxb;  /* Number of bytes received (opt.) - Read Only */
	const u32 txb;  /* Number of bytes transmitted (opt.) - Read Only */
	u32 txs;  /* Tx Slave FIFO register - Write Only */
	const u32 stfl; /* Tx Slave FIFO level - Read Only */
        /* MSB */
} i2c_regs, *pi2c_regs;

/* --------------------------------------------------------------------------
*  I2C Register descriptions
*  -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
*  Receive Fifo (RX) register (0x00) - Read Only
*  -------------------------------------------------------------------------- */

/* Overall register mask */
#define I2C_RX_FIFO_REG_MSK      0x000000FF
/* Read/Write bits */
#define I2C_RX_FIFO_REG_RW_MSK   0x00000000
/* Power-on/Reset mask & value */
#define I2C_RX_FIFO_REG_POR_MSK  0x00000000
#define I2C_RX_FIFO_REG_POR_VAL  0x00000000

/* Data */
#define I2C_RX_FIFO_REG_DATA_POS          0
#define I2C_RX_FIFO_REG_DATA_LEN          8
#define I2C_RX_FIFO_REG_DATA_MSK 0x000000FF

/* --------------------------------------------------------------------------
*  Transmit Fifo (TX) register (0x00) - Write only
*  -------------------------------------------------------------------------- */

/* TX fifo register mask */
#define I2C_TX_FIFO_REG_MSK       0x000003FF
/* Read/Write bits */
#define I2C_TX_FIFO_REG_RW_MSK    0x00000000
/* Power-on/Reset mask & value */
#define I2C_TX_FIFO_REG_POR_MSK   0x00000000
#define I2C_TX_FIFO_REG_POR_VAL   0x00000000

/* Data */
#define I2C_TX_FIFO_REG_DATA_POS           0
#define I2C_TX_FIFO_REG_DATA_LEN           8
#define I2C_TX_FIFO_REG_DATA_MSK  0x000000FF
/* Start bit */
#define I2C_TX_FIFO_REG_START_POS          8
#define I2C_TX_FIFO_REG_START_LEN          1
#define I2C_TX_FIFO_REG_START_MSK 0x00000100
/* Stop bit */
#define I2C_TX_FIFO_REG_STOP_POS           9
#define I2C_TX_FIFO_REG_STOP_LEN           1
#define I2C_TX_FIFO_REG_STOP_MSK  0x00000200

/* --------------------------------------------------------------------------
*  Status Register (0x04) - Read Only
*  -------------------------------------------------------------------------- */

/* Status register mask */
#define I2C_STS_REG_MSK          0x00003FFF
/* Read/Write bits */
#define I2C_STS_REG_RW_MSK       0x00000003
/* Power-on/Reset mask & value */
#define I2C_STS_REG_POR_MSK      0x00003F3F
#define I2C_STS_REG_POR_VAL      0x00002A00

/* Transmitter Done */
#define I2C_STS_REG_TDI_POS               0
#define I2C_STS_REG_TDI_LEN               1
#define I2C_STS_REG_TDI_MSK      0x00000001
/* Transmitter Arbitration Failure */
#define I2C_STS_REG_AFI_POS               1
#define I2C_STS_REG_AFI_LEN               1
#define I2C_STS_REG_AFI_MSK      0x00000002
/* Transmitter No Acknowledge */
#define I2C_STS_REG_NAI_POS               2
#define I2C_STS_REG_NAI_LEN               1
#define I2C_STS_REG_NAI_MSK      0x00000004
/* Data Request Master */
#define I2C_STS_REG_DRM_POS               3
#define I2C_STS_REG_DRM_LEN               1
#define I2C_STS_REG_DRM_MSK      0x00000008
/* Data Request Slave */
#define I2C_STS_REG_DRS_POS               4
#define I2C_STS_REG_DRS_LEN               1
#define I2C_STS_REG_DRS_MSK      0x00000010
/* I2C Active */
#define I2C_STS_REG_ACTIVE_POS            5
#define I2C_STS_REG_ACTIVE_LEN            1
#define I2C_STS_REG_ACTIVE_MSK   0x00000020
/* Value of SCL signal */
#define I2C_STS_REG_SCL_POS               6
#define I2C_STS_REG_SCL_LEN               1
#define I2C_STS_REG_SCL_MSK      0x00000040
/* Value of SDA signal */
#define I2C_STS_REG_SDA_POS               7
#define I2C_STS_REG_SDA_LEN               1
#define I2C_STS_REG_SDA_MSK      0x00000080
/* Receive FIFO Full */
#define I2C_STS_REG_RFF_POS               8
#define I2C_STS_REG_RFF_LEN               1
#define I2C_STS_REG_RFF_MSK      0x00000100
/* Receive FIFO Empty */
#define I2C_STS_REG_RFE_POS               9
#define I2C_STS_REG_RFE_LEN               1
#define I2C_STS_REG_RFE_MSK      0x00000200
/* Transmit FIFO Full */
#define I2C_STS_REG_TFF_POS              10
#define I2C_STS_REG_TFF_LEN               1
#define I2C_STS_REG_TFF_MSK      0x00000400
/* Transmit FIFO Empty */
#define I2C_STS_REG_TFE_POS              11
#define I2C_STS_REG_TFE_LEN               1
#define I2C_STS_REG_TFE_MSK      0x00000800
/* Slave Transmit FIFO Full */
#define I2C_STS_REG_STFF_POS             12
#define I2C_STS_REG_STFF_LEN              1
#define I2C_STS_REG_STFF_MSK     0x00001000
/* Slave Transmit FIFO Empty */
#define I2C_STS_REG_STFE_POS             13
#define I2C_STS_REG_STFE_LEN              1
#define I2C_STS_REG_STFE_MSK     0x00002000

/* Valid values */
#define I2C_STS_REG_ALLINT       0x00003F1F


/* --------------------------------------------------------------------------
*  Control (CTL) Register (0x08) - Read/Write
*  -------------------------------------------------------------------------- */

/* Control register mask */
/*      Bit 9 is not implemented (Seven-bit Slave Addres). */
#define I2C_CTL_REG_MSK         0x000005FF
/* Read/Write bits */
#define I2C_CTL_REG_RW_MSK      0x000004FF
/* Power-on/Reset mask & value */
#define I2C_CTL_REG_POR_MSK     0x000005FF
#define I2C_CTL_REG_POR_VAL     0x00000000

/* Transmitter Done Interrupt Enable */
#define I2C_CTL_REG_TDIE_POS             0
#define I2C_CTL_REG_TDIE_LEN             1
#define I2C_CTL_REG_TDIE_MSK    0x00000001
/* Transmitter Arbitration Failure Interrupt Enable */
#define I2C_CTL_REG_TAFIE_POS            1
#define I2C_CTL_REG_TAFIE_LEN            1
#define I2C_CTL_REG_TAFIE_MSK   0x00000002
/* Transmitter No Acknowledge Interrupt Enable */
#define I2C_CTL_REG_TNAIE_POS            2
#define I2C_CTL_REG_TNAIE_LEN            1
#define I2C_CTL_REG_TNAIE_MSK   0x00000004
/* Data Request Master Interrupt Enable */
#define I2C_CTL_REG_DRMIE_POS            3
#define I2C_CTL_REG_DRMIE_LEN            1
#define I2C_CTL_REG_DRMIE_MSK   0x00000008
/* Data Request Slave Interrupt Enable */
#define I2C_CTL_REG_DRSIE_POS            4
#define I2C_CTL_REG_DRSIE_LEN            1
#define I2C_CTL_REG_DRSIE_MSK   0x00000010
/* Receive FIFO Full Interrupt Enable */
#define I2C_CTL_REG_RFFIE_POS            5
#define I2C_CTL_REG_RFFIE_LEN            1
#define I2C_CTL_REG_RFFIE_MSK   0x00000020
/* Receive Data Available Interrupt Enable  */
#define I2C_CTL_REG_RFDAIE_POS           6
#define I2C_CTL_REG_RFDAIE_LEN           1
#define I2C_CTL_REG_RFDAIE_MSK  0x00000040
/* Transmit FIFO Not Full Interrupt Enable */
#define I2C_CTL_REG_TFNFIE_POS           7
#define I2C_CTL_REG_TFNFIE_LEN           1
#define I2C_CTL_REG_TFNFIE_MSK  0x00000080
/* Soft Reset */
#define I2C_CTL_REG_SRESET_POS           8
#define I2C_CTL_REG_SRESET_LEN           1
#define I2C_CTL_REG_SRESET_MSK  0x00000100
/* Slave Transmit FIFO Not Full Interrupt Enable */
#define I2C_CTL_REG_STFNFIE_POS         10
#define I2C_CTL_REG_STFNFIE_LEN          1
#define I2C_CTL_REG_STFNFIE_MSK 0x00000400

/* Valid values */
#define I2C_CTL_REG_ALLINT      0x000004FF

/* --------------------------------------------------------------------------
*  Clock (CKL) divisor low register (0x0C) - Read/Write
*  -------------------------------------------------------------------------- */

/* Clock register mask */
/*     The Clock registers width is optimized by the HDLI template. */
/*     It is fixed at 15 bits wide. */

#define I2C_CKL_REG_MSK         0x00007FFF
/* Read/Write bits */
#define I2C_CKL_REG_RW_MSK      0x00007FFF
/* Power-on/Reset mask & value */
#define I2C_CKL_REG_POR_MSK     0x00007FFF
/* Reset value is not usable. */
#define I2C_CKL_REG_POR_VAL     0x0000752e
/* Data */
#define I2C_CKL_REG_DATA_POS             0
#define I2C_CKL_REG_DATA_LEN            15
#define I2C_CKL_REG_DATA_MSK    0x00007FFF

/* --------------------------------------------------------------------------
*  Clock (CKH) divisor high register (0x10) - Read/Write
*  -------------------------------------------------------------------------- */

#define I2C_CKH_REG_MSK         0x00007FFF
/* Read/Write bits */
#define I2C_CKH_REG_RW_MSK      0x00007FFF
/* Power-on/Reset mask & value */
#define I2C_CKH_REG_POR_MSK     0x00007FFF
/* Reset value is not usable. */
#define I2C_CKH_REG_POR_VAL     0x0000752e
/* Data */
#define I2C_CKH_REG_DATA_POS             0
#define I2C_CKH_REG_DATA_LEN            15
#define I2C_CKH_REG_DATA_MSK    0x00007FFF


/* --------------------------------------------------------------------------
*  Address (ADR) Register (0x14) - Read/Write
*  -------------------------------------------------------------------------- */

/* Address register mask */
/*     Only the lower 7 bits are used, i.e. [6:0] */
#define I2C_ADR_REG_MSK         0x0000007F
/* Read/Write bits */
#define I2C_ADR_REG_RW_MSK      0x0000007F
/* Power-on/Reset mask & value */
#define I2C_ADR_REG_POR_MSK     0x0000007F
#define I2C_ADR_REG_POR_VAL     0x1d

/* Data */
#define I2C_ADR_REG_DATA_POS             0
#define I2C_ADR_REG_DATA_LEN             7
#define I2C_ADR_REG_DATA_MSK    0x0000007F


/* --------------------------------------------------------------------------
*  RX Fifo Level (RFL) Register (0x18) - Read only
*  -------------------------------------------------------------------------- */

/* RX FIFO Level mask */
/*      Eventhough the fifo levels are defined as U32, they only contain */
/*      U16 data multiplexed in the upper and lower 16 bits. */
#define I2C_RFL_REG_MSK         0x0000FFFF
/* Read/Write bits */
#define I2C_RFL_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_RFL_REG_POR_MSK     0x0000FFFF
#define I2C_RFL_REG_POR_VAL     0x00000000


/* --------------------------------------------------------------------------
*  TX Fifo Level (TFL) Register (0x1C) - Read only
*  -------------------------------------------------------------------------- */

/* TX FIFO Level mask */
/*      Eventhough the fifo levels are defined as U32, they only contain */
/*      U16 data multiplexed in the upper and lower 16 bits. */
#define I2C_TFL_REG_MSK         0x0000FFFF
/* Read/Write bits */
#define I2C_TFL_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_TFL_REG_POR_MSK     0x0000FFFF
#define I2C_TFL_REG_POR_VAL     0x00000000

/* --------------------------------------------------------------------------
*  Nr. of Bytes Received (RXB) Register (0x20) - Read only
*  -------------------------------------------------------------------------- */

/* RXB mask */
/*      Eventhough the registers are defined as U32, they only contain */
/*      U16 data multiplexed in the upper and lower 16 bits. */
#define I2C_RXB_REG_MSK         0x0000FFFF
/* Read/Write bits */
#define I2C_RXB_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_RXB_REG_POR_MSK     0x0000FFFF
#define I2C_RXB_REG_POR_VAL     0x00000000

/* --------------------------------------------------------------------------
*  Nr. of Bytes Transmitted (TXB) Register (0x20) - Read only
*  -------------------------------------------------------------------------- */

/* TXB mask */
/*      Eventhough the registers are defined as U32, they only contain */
/*      U16 data multiplexed in the upper and lower 16 bits. */
#define I2C_TXB_REG_MSK         0x0000FFFF
/* Read/Write bits */
#define I2C_TXB_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_TXB_REG_POR_MSK     0x0000FFFF
#define I2C_TXB_REG_POR_VAL     0x00000000

/* --------------------------------------------------------------------------
*  Slave Transmit Fifo (TXS) register (0x28) - Write only
*  -------------------------------------------------------------------------- */

/* TXS mask */
#define I2C_TXS_REG_MSK         0x000000FF
/* Read/Write bits */
#define I2C_TXS_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_TXS_REG_POR_MSK     0x00000000
#define I2C_TXS_REG_POR_VAL     0x00000000

/* Data */
#define I2C_TXS_REG_DATA_POS             0
#define I2C_TXS_REG_DATA_LEN             8
#define I2C_TXS_REG_DATA_MSK    0x000000FF

/* --------------------------------------------------------------------------
*  Slave Transmit Fifo level (STFL) register (0x2C) - Read only
*  -------------------------------------------------------------------------- */

/* TXS mask */
#define I2C_STFL_REG_MSK         0x000000FF
/* Read/Write bits */
#define I2C_STFL_REG_RW_MSK      0x00000000
/* Power-on/Reset mask & value */
#define I2C_STFL_REG_POR_MSK     0x00000000
#define I2C_STFL_REG_POR_VAL     0x00000000

/* Data */
#define I2C_STFL_REG_DATA_POS             0
#define I2C_STFL_REG_DATA_LEN             8
#define I2C_STFL_REG_DATA_MSK    0x000000FF

/* --------------------------------------------------------------------------
*  Generic Type
*  -------------------------------------------------------------------------- */

/* Generic type for read/write registers */
#define DEFINE_vhI2cRWregsTable      \
const UInt32 vhI2cRWregsTable [] = { \
	     I2C_RX_FIFO_REG_RW_MSK, \
	     I2C_STS_REG_RW_MSK,     \
	     I2C_CTL_REG_RW_MSK,     \
	     I2C_CKL_REG_RW_MSK,     \
	     I2C_CKH_REG_RW_MSK,     \
	     I2C_ADR_REG_RW_MSK,     \
	     I2C_RFL_REG_RW_MSK,     \
	     I2C_TFL_REG_RW_MSK,     \
	     I2C_RXB_REG_RW_MSK,     \
	     I2C_TXB_REG_RW_MSK,     \
	     I2C_TXS_REG_RW_MSK,     \
	     I2C_STFL_REG_RW_MSK     \
         };

typedef enum {
	mstatus_tdi = 1 << I2C_STS_REG_TDI_POS,
	mstatus_afi = 1 << I2C_STS_REG_AFI_POS,
	mstatus_nai = 1 << I2C_STS_REG_NAI_POS,
	mstatus_drmi = 1 << I2C_STS_REG_DRM_POS,
	mstatus_active = 1 << I2C_STS_REG_ACTIVE_POS,
	mstatus_scl = 1 << I2C_STS_REG_SCL_POS,
	mstatus_sda = 1 << I2C_STS_REG_SDA_POS,
	mstatus_rff = 1 << I2C_STS_REG_RFF_POS,
	mstatus_rfe = 1 << I2C_STS_REG_RFE_POS,
	mstatus_tff = 1 << I2C_STS_REG_TFF_POS,
	mstatus_tfe = 1 << I2C_STS_REG_TFE_POS,
} mstatus_bits_t;

typedef enum {
	mcntrl_tdie = 1 << I2C_CTL_REG_TDIE_POS,
	mcntrl_afie = 1 << I2C_CTL_REG_TAFIE_POS,
	mcntrl_naie = 1 << I2C_CTL_REG_TNAIE_POS,
	mcntrl_drmie = 1 << I2C_CTL_REG_DRMIE_POS,
	/* drsie is not used here */
	mcntrl_rffie = 1 << I2C_CTL_REG_RFFIE_POS,
	mcntrl_daie = 1 << I2C_CTL_REG_RFDAIE_POS,
	mcntrl_tffie = 1 << I2C_CTL_REG_TFNFIE_POS,
	mcntrl_reset = 1 << I2C_CTL_REG_SRESET_POS,
	mcntrl_cdbmode = 1 << I2C_CTL_REG_STFNFIE_POS,
} mcntrl_bits_t;

typedef enum {
	sstatus_afi = 1 << I2C_STS_REG_AFI_POS,
	sstatus_nai = 1 << I2C_STS_REG_NAI_POS,
	sstatus_drsi = 1 << I2C_STS_REG_DRS_POS,
	sstatus_active = 1 << I2C_STS_REG_ACTIVE_POS,
	sstatus_scl = 1 << I2C_STS_REG_SCL_POS,
	sstatus_sda = 1 << I2C_STS_REG_SDA_POS,
	sstatus_rff = 1 << I2C_STS_REG_RFF_POS,
	sstatus_rfe = 1 << I2C_STS_REG_RFE_POS,
	sstatus_tffs = 1 << I2C_STS_REG_TFF_POS,
	sstatus_tfes = 1 << I2C_STS_REG_TFE_POS,
	sstatus_stffs = 1 << I2C_STS_REG_STFF_POS,
	sstatus_stfes = 1 << I2C_STS_REG_STFE_POS,
	
} sstatus_bits_t;

typedef enum {
	scntrl_afie = 1 << I2C_CTL_REG_TAFIE_POS,
	scntrl_naie = 1 << I2C_CTL_REG_TNAIE_POS,
	scntrl_drsie = 1 << I2C_CTL_REG_DRSIE_POS,
	scntrl_rffie = 1 << I2C_CTL_REG_RFFIE_POS,
	scntrl_daie = 1 << I2C_CTL_REG_RFDAIE_POS,
	scntrl_reset = 1 << I2C_CTL_REG_SRESET_POS,
	scntrl_tffsie = 1 << I2C_CTL_REG_STFNFIE_POS,
} scntrl_bits_t;

typedef enum {
	rw_bit = 1 << I2C_TX_FIFO_REG_DATA_POS,
	start_bit = 1 << I2C_TX_FIFO_REG_START_POS,
	stop_bit = 1 << I2C_TX_FIFO_REG_STOP_POS,
} i2c_pnx_prot_bits_t;

typedef enum {
	xmit,
	rcv,
	nodata,
} i2c_pnx_mode_t;

typedef enum {
	master,
	slave,
} i2c_pnx_adap_mode_t;

#define I2C_BUFFER_SIZE   0x20

typedef struct {
	int ret;			/* Return value */
	i2c_pnx_mode_t mode;	/* Interface mode */
	struct semaphore sem;		/* Mutex for this interface */
	struct completion complete;	/* I/O completion */
	struct timer_list timer;	/* Timeout */
	char *buf;			/* Data buffer */
	int len;			/* Length of data buffer */
} i2c_pnx_mif_t;

typedef struct {
	u32 base;
	int irq;
	i2c_pnx_adap_mode_t mode;
	int slave_addr;
	void (*slave_recv_cb) (void *);
	void (*slave_send_cb) (void *);
	i2c_regs *master;
	i2c_regs *slave;
	char buffer[I2C_BUFFER_SIZE];	/* contains data received from I2C bus */
	int buf_index;		/* index within I2C buffer (see above) */
	i2c_pnx_mif_t mif;

} i2c_pnx_algo_data_t;

#define HCLK_MHZ		40	/* TODO: Make dynamic. */
#define TIMEOUT			(2*(HZ))
#define I2C_PNX0106_SPEED_KHZ	400

#define I2C_BLOCK_SIZE		0x100

#if defined (CONFIG_I2C_PNX_SLAVE_SUPPORT)
extern int i2c_slave_set_recv_cb(void (*i2c_slave_recv_cb) (void *),
				 unsigned int adap);
extern int i2c_slave_set_send_cb(void (*i2c_slave_send_cb) (void *),
				 unsigned int adap);
extern int i2c_slave_send_data(char *buf, int len, void *data);
extern int i2c_slave_recv_data(char *buf, int len, void *data);
#endif

#endif

