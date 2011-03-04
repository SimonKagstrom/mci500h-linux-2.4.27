/*
 * This is header file Philips(PNX0106), AMBA bus based USB OTG-EHCI-1.0 host controller
 * IP core ( 9028).
 * (C) Copyright ???? Philips Electronics. All rights reserved.
 *
 * You can redistribute and/or modify this software under the terms of
 * version 2 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _PNX0106_USB2IP_H_
#define	_PNX0106_USB2IP_H_

/*!
 * The USB2IP9028 device description
 * ---------------------------------
 */

/* These should be part of include/asm-arm/mach-pnx0106/hardware.h */

#define USB_DMA_REGS_BASE 	0x80000000

#define USB_OTG_SLV_BASE 	0x80002000
#define USB_OTG_SLV_LENGTH	0x1000
 

/*!
 * Register offsets from base address
 * ----------------------------------
 */

#define USB_DMA_CH0_STS_REG    0x0000     /* Channel 0 status */
#define USB_DMA_CH0_CTRL_REG   0x0004     /* Channel 0 control */
#define USB_DMA_CH0_SRC_REG    0x0008     /* Channel 0 source */
#define USB_DMA_CH0_DST_REG    0x000C     /* Channel 0 destination */
#define USB_DMA_CH0_THR_REG    0x0010     /* Channel 0 throttle */
#define USB_DMA_CH0_CNT_REG    0x0014     /* Channel 0 count */

#define USB_DMA_CH1_STS_REG    0x0040     /* Channel 1 status */
#define USB_DMA_CH1_CTRL_REG   0x0044     /* Channel 1 control */
#define USB_DMA_CH1_SRC_REG    0x0048     /* Channel 1 source */
#define USB_DMA_CH1_DST_REG    0x004C     /* Channel 1 destination */
#define USB_DMA_CH1_THR_REG    0x0050     /* Channel 1 throttle */
#define USB_DMA_CH1_CNT_REG    0x0054     /* Channel 1 count */

#define USB_DMA_CTRL_REG       0x0400     /* DMA control */
#define USB_DMA_RST_REG        0x0404     /* DMA soft reset */
#define USB_DMA_STS_REG        0x0408     /* DMA status */

#define USB_DMA_ISTS_REG       0x0410     /* DMA INT status */

#define USB_DMA_IEN_REG        0x0418     /* DMA INT enable */

#define USB_DMA_IDIS_REG       0x0420     /* DMA INT disable */

#define USB_DMA_ISET_REG       0x0428     /* DMA INT set */

#define USB_DMA_ICLR_REG       0x0430     /* DMA INT clear */

#define USB_DMA_FCP0_REG       0x0500     /* DMA flow-control 0 */
#define USB_DMA_FCP1_REG       0x0504     /* DMA flow-control 1 */
#define USB_DMA_FCP2_REG       0x0508     /* DMA flow-control 2 */
#define USB_DMA_FCP3_REG       0x050C     /* DMA flow-control 3 */

#define USB_DMA_MODID_REG      0x0800     /* DMA module ID */

#define USB_DEVADR_REG         0x1000     /* device address */
#define USB_EMAXSIZE_REG       0x1004     /* endpoint max packet size */
#define USB_ETYPE_REG          0x1008     /* endpoint type */
#define USB_MODE_REG           0x100C     /* mode */
#define USB_INT_CFG_REG        0x1010     /* interrupt config */

#define USB_DCNT_REG           0x101C     /* data count */
#define USB_DATA_REG           0x1020     /* data port to fifo */
#define USB_SHORT_REG          0x1024     /* short packet flag */
#define USB_ECTRL_REG          0x1028     /* endpoint control */
#define USB_EIDX_REG           0x102C     /* endpoint index */

#define USB_MODID_REG          0x1070     /* module ID */
#define USB_FNB_REG            0x1074     /* frame number */
#define USB_SCRATCH_REG        0x1078     /* scratch information register */
#define USB_LOCK_REG           0x107C     /* lock register */

#define USB_TST_REG            0x1084     /* test mode */

#define USB_SIE_REG            0x108C     /* system interrupt enable */
#define USB_EIE_REG            0x1090     /* endpoint interrupt enable */
#define USB_SIS_REG            0x1094     /* system interrupt status */
#define USB_EIS_REG            0x1098     /* endpoint interrupt status */

#define USB_EIC_REG            0x10A0     /* endpoint interrupt clear */
#define USB_EISET_REG          0x10A4     /* endpoint interrupt set */
#define USB_EIL_REG            0x10A8     /* endpoint interrupt line */
#define USB_SIC_REG            0x10AC     /* system interrupt clear */
#define USB_SISET_REG          0x10B0     /* system interrupt set */
#define USB_SIL_REG            0x10B4     /* system interrupt line */

/*======================================================================= */

/* USB OTG */
#define OTG_ID_REG            0x000 /*Identification register */
#define OTG_HWGENERAL_REG     0x004 /*General hardware parameters */
#define OTG_HWHOST_REG        0x008 /*Host hardware parameters */
#define OTG_HWDEVICE_REG      0x00C /*Device hardware parameters */
#define OTG_HWTXBUF_REG       0x010 /*TX Buffer hardware parameters */
#define OTG_HWRXBUF_REG       0x014 /*RX Buffer hardware parameters */
#define OTG_HWTTTXBUF_REG     0x018 /*TT-TX Buffer hardware parameters */
#define OTG_HWTTRXBUF_REG     0x01C /*TT-RX Buffer hardware parameters */
#define OTG_IP_INFO_REG_REG   0x020 /*IP number and version number (this register is not present in the original ARC core 	  0x000    it is added for IP_9028) */

/*Capability registers */
#define OTG_CAPLENGTH_REG     0x100 /*Capability register length */
#define OTG_HCIVERSION_REG    0x102 /*Host interface version number */
#define OTG_HCSPARAMS_REG     0x104 /*Host controller structural parameters */
#define OTG_HCCPARAMS_REG     0x108 /*Host controller capability parameters */
#define OTG_DCIVERSION_REG    0x120 /*Device interface version number */
#define OTG_DCCPARAMS_REG     0x124 /*Device controller capability parameters */

/*Operational registers */
#define OTG_USBCMD_REG        0x140 /*USB command */
#define OTG_USBSTS_REG        0x144 /*USB status */
#define OTG_USBINTR_REG       0x148 /*USB interrupt enable */
#define OTG_FRINDEX_REG       0x14C /*USB frame index */
#define OTG_PERIODICLISTBASE__DEVICEADDR_REG    0x154 /*Frame list base address__USB device address */
#define OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG 0x158 /*Next asynchronous list address__Address of endpoint list in memory */
#define OTG_TTCTRL_REG	      0x15C /*Asynchronous buffer status for embedded TT */
#define OTG_BURSTSIZE_REG     0x160 /*Programmable burst size */
#define OTG_TXFILLTUNING_REG  0x164 /*Host transmit pre-buffer packet tuning */
#define OTG_TXTTFILLTUNING_REG 0x168 /*Host TT transmit pre-buffer packet tuning */
#define OTG_ENDPTNAK_REG       0x178 /*Endpoint NAK */
#define OTG_ENDPTNAKEN_REG     0x17C /*Endpoint NAK Enable */
#define OTG_CONFIGFLAG_REG     0x180 /*Configured flag register */
#define OTG_PORTSC1_REG	       0x184 /*Port status/control 1 */
/*#define OTG_PORTSC2_REG        0x188   Port status/control 2 */
/*#define OTG_PORTSC3_REG        0x18C   Port status/control 3 */
#define OTG_OTGSC_REG          0x1A4 /*OTG status and control */
#define OTG_USBMODE_REG	       0x1A8 /*USB device mode */
#define OTG_ENDPTSETUPSTAT_REG 0x1AC /*Endpoint setup status */
#define OTG_ENDPTPRIME_REG     0x1B0 /*Endpoint initialization */
#define OTG_ENDPTFLUSH_REG     0x1B4 /*Endpoint de-initialization */
#define OTG_ENDPTSTATUS_REG    0x1B8 /*Endpoint status */
#define OTG_ENDPTCOMPLETE_REG  0x1BC /*Endpoint complete */
#define OTG_ENDPTCTRL0_REG     0x1C0 /*Endpoint control 0 */
#define OTG_ENDPTCTRL1_REG     0x1C4 /*ENDPTCTRL1 Endpoint control 1 */
#define OTG_ENDPTCTRL2_REG     0x1C8 /*ENDPTCTRL1 Endpoint control 2 */
#define OTG_ENDPTCTRL3_REG     0x1CC /*ENDPTCTRL1 Endpoint control 3 */

/*!
 * Device description
 * ------------------
 */

#ifndef __ASSEMBLY__

typedef volatile struct {

  /* USB DMA */
  u32 dma_ch0_sts;
  u32 dma_ch0_ctrl;
  u32 dma_ch0_src;
  u32 dma_ch0_dst;
  u32 dma_ch0_thr;
  u32 dma_ch0_cnt;
  u32 _dma0[10];
  u32 dma_ch1_sts;
  u32 dma_ch1_ctrl;
  u32 dma_ch1_src;
  u32 dma_ch1_dst;
  u32 dma_ch1_thr;
  u32 dma_ch1_cnt;
  u32 _dma1[234];
  u32 dma_ctrl;
  u32 dma_rst;
  u32 dma_sts;
  u32 _dma3[1];
  u32 dma_ists;
  u32 _dma4[1];
  u32 dma_ien;
  u32 _dma5[1];
  u32 dma_idis;
  u32 _dma6[1];
  u32 dma_iset;
  u32 _dma7[1];
  u32 dma_iclr;
  u32 _dma8[51];
  u32 dma_fcp0;
  u32 dma_fcp1;
  u32 dma_fcp2;
  u32 dma_fcp3;
  u32 _dma9[188];
  u32 dma_modid;
  u32 _dma10[511];

  /* USB core */
  u32 devadr;
  u32 emaxsize;
  u32 etype;
  u32 mode;
  u32 int_cfg;
  u32 _d0[2];
  u32 dcnt;
  u32 data;
  u32 SHORT;
  u32 ectrl;
  u32 eidx;
  u32 _d1[16];
  u32 modid;
  u32 fnb;
  u32 scratch;
  u32 lock;
  u32 _d3[1];
  u32 tst;
  u32 _d4[1];
  u32 sie;
  u32 eie;
  u32 sis;
  u32 eis;
  u32 _d5[1];
  u32 eic;
  u32 eiset;
  u32 eil;
  u32 sic;
  u32 siset;
  u32 sil;
} usb_regs, *pusb_regs;



typedef volatile struct {

  /* USB OTG */
  u32 id;            /*Identification register */
  u32 hwgeneral;     /*General hardware parameters */
  u32 hwhost;        /*Host hardware parameters */
  u32 hwdevice;      /*Device hardware parameters */
  u32 hwtxbuf;       /*TX Buffer hardware parameters */
  u32 hwrxbuf;       /*RX Buffer hardware parameters */
  u32 hwtttxbuf;     /*TT-TX Buffer hardware parameters */
  u32 hwttrxbuf;     /*TT-RX Buffer hardware parameters */
  u32 ip_info_reg;   /*IP number and version number (this register is not present in the original ARC core; it is added for IP_9028) */

  u32 _otg0[55];     /*024h-0FCh 220 Reserved   n/a */

  /*Capability registers */
  u8  caplength;     /*Capability register length */
  u8  _otg1[1];      /*n/a */
  u16 hciversion;    /*Host interface version number */

  u32 hcsparams;     /*Host controller structural parameters */
  u32 hccparams;     /*Host controller capability parameters */

  u32 _otg2[5];      /*10Ch-11Fh 20 Reserved   n/a */

  u16 dciversion;    /*Device interface version number */
  u16 _otg3[1];      /*n/a */

  u32 dccparams;     /*Device controller capability parameters */

  u32 _otg4a[4];     /*128h-134h 16 Reserved   n/a */
  u32 usb_up_int;    /*USB interrupt test mode */
  u32 _otg4b[1];     /*13Ch 4 Reserved   n/a */

  /*Operational registers */
  u32 usbcmd;        /*USB command */
  u32 usbsts;        /*USB status */
  u32 usbintr;       /*USB interrupt enable */
  u32 frindex;       /*USB frame index */
  u32 _otg5[1];      /*n/a */
  u32 periodiclistbase__deviceaddr;    /*Frame list base address__USB device address */
  u32 asynclistaddr__endpointlistaddr; /*Next asynchronous list address__Address of endpoint list in memory */
  u32 ttctrl;        /*Asynchronous buffer status for embedded TT */
  u32 burstsize;     /*Programmable burst size */
  u32 txfilltuning;  /*Host transmit pre-buffer packet tuning */
  u32 txttfilltuning;/*Host TT transmit pre-buffer packet tuning */
  u32 _otg6[1];      /*n/a */
  u32 _otg7[2];      /*170h-174h 8 Reserved n/a */
  u32 endptnak;
  u32 endptnaken;
  u32 configflag;    /*Configured flag register */
  u32 portsc1;       /*Port status/control 1 */
  u32 _otg8[7];      /*190h-1A3h 20 Reserved n/a */
  u32 otgsc;         /*OTG status and control */
  u32 usbmode;       /*USB device mode */
  u32 endptsetupstat;/*Endpoint setup status */
  u32 endptprime;    /*Endpoint initialization */
  u32 endptflush;    /*Endpoint de-initialization */
  u32 endptstatus;   /*Endpoint status */
  u32 endptcomplete; /*Endpoint complete */
  u32 endptctrl0;    /*Endpoint control 0 */
  u32 endptctrl1;    /*ENDPTCTRL1 Endpoint control 1 */
  u32 endptctrl2;    /*ENDPTCTRL1 Endpoint control 2 */
  u32 endptctrl3;    /*ENDPTCTRL1 Endpoint control 3 */

} usb_otg_regs, *pusb_otg_regs;


/* dTD Transfer Description */
typedef struct
{
  u32 nextdtdpointer ;
  u32 totalbytes ;
  u32 bufferpointer0 ;
  u32 bufferpointer1 ;
  u32 bufferpointer2 ;
  u32 bufferpointer3 ;
  u32 bufferpointer4 ;
  u32 reserved;
}  dtd, *pdtd;

/* dQH  Queue Head */
typedef struct
{
  u32 capabilities ;
  u32 currentdtdpointer ;
  u32 nextdtdpointer ;
  u32 totalbytes ;
  u32 bufferpointer0 ;
  u32 bufferpointer1 ;
  u32 bufferpointer2 ;
  u32 bufferpointer3 ;
  u32 bufferpointer4 ;
  u32 reserved ;
  u32 setup[2] ;
  u32 gap[4];
}  dqh, *pdqh;

#endif /* __ASSEMBLY__ */

/*!
 * ----------------------------------------------------------------------------
 */
#define USB_INT_IRQ    0
#define USB_INT_FIQ    1
#define USB_INT_CH0    2
#define USB_INT_CH1    3
#define USB_OTG_IRQ    4
/*!
 * USB2IP9028 DMA Register descriptions
 * ------------------------------------
 */

/* DMA_CH0_STS */

#define USB_DMA_CH0_STS_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH0_STS_REG_POR_VAL       0x00000000
#define USB_DMA_CH0_STS_REG_RW_MSK        0xFFFFFFFF

#define USB_DMA_CH0_STS_REG_STATE_POS     0
#define USB_DMA_CH0_STS_REG_STATE_LEN     2
#define USB_DMA_CH0_STS_REG_STATE_MSK     0x00000003

#define USB_DMA_CH0_STS_REG_ERR_POS       16
#define USB_DMA_CH0_STS_REG_ERR_LEN       8
#define USB_DMA_CH0_STS_REG_ERR_MSK       0x00FF0000

/* DMA_CH0_CTRL */

#define USB_DMA_CH0_CTRL_REG_POR_MSK      0xC7FFFFFF
#define USB_DMA_CH0_CTRL_REG_POR_VAL      0x000C00C0
#define USB_DMA_CH0_CTRL_REG_RW_MSK       0xC7FFFFFC

#define USB_DMA_CH0_CTRL_REG_EN_POS       0
#define USB_DMA_CH0_CTRL_REG_EN_LEN       2
#define USB_DMA_CH0_CTRL_REG_EN_MSK       0x00000003

#define USB_DMA_CH0_CTRL_REG_MODE_POS     2
#define USB_DMA_CH0_CTRL_REG_MODE_LEN     1
#define USB_DMA_CH0_CTRL_REG_MODE_MSK     0x00000004

#define USB_DMA_CH0_CTRL_REG_SRC_POS      3
#define USB_DMA_CH0_CTRL_REG_SRC_LEN      2
#define USB_DMA_CH0_CTRL_REG_SRC_MSK      0x00000018

#define USB_DMA_CH0_CTRL_REG_STYPE_POS    5
#define USB_DMA_CH0_CTRL_REG_STYPE_LEN    2
#define USB_DMA_CH0_CTRL_REG_STYPE_MSK    0x00000060

#define USB_DMA_CH0_CTRL_REG_SADJ_POS     7
#define USB_DMA_CH0_CTRL_REG_SADJ_LEN     2
#define USB_DMA_CH0_CTRL_REG_SADJ_MSK     0x00000180

#define USB_DMA_CH0_CTRL_REG_SFCM_POS     9
#define USB_DMA_CH0_CTRL_REG_SFCM_LEN     2
#define USB_DMA_CH0_CTRL_REG_SFCM_MSK     0x00000600

#define USB_DMA_CH0_CTRL_REG_SFCP_POS     11
#define USB_DMA_CH0_CTRL_REG_SFCP_LEN     4
#define USB_DMA_CH0_CTRL_REG_SFCP_MSK     0x00007800

#define USB_DMA_CH0_CTRL_REG_DST_POS      15
#define USB_DMA_CH0_CTRL_REG_DST_LEN      2
#define USB_DMA_CH0_CTRL_REG_DST_MSK      0x00018000

#define USB_DMA_CH0_CTRL_REG_DTYPE_POS    17
#define USB_DMA_CH0_CTRL_REG_DTYPE_LEN    2
#define USB_DMA_CH0_CTRL_REG_DTYPE_MSK    0x00060000

#define USB_DMA_CH0_CTRL_REG_DADJ_POS     19
#define USB_DMA_CH0_CTRL_REG_DADJ_LEN     2
#define USB_DMA_CH0_CTRL_REG_DADJ_MSK     0x00180000

#define USB_DMA_CH0_CTRL_REG_DFCM_POS     21
#define USB_DMA_CH0_CTRL_REG_DFCM_LEN     2
#define USB_DMA_CH0_CTRL_REG_DFCM_MSK     0x00600000

#define USB_DMA_CH0_CTRL_REG_DFCP_POS     23
#define USB_DMA_CH0_CTRL_REG_DFCP_LEN     4
#define USB_DMA_CH0_CTRL_REG_DFCP_MSK     0x07800000

#define USB_DMA_CH0_CTRL_REG_IEOT_POS     30
#define USB_DMA_CH0_CTRL_REG_IEOT_LEN     1
#define USB_DMA_CH0_CTRL_REG_IEOT_MSK     0x40000000

#define USB_DMA_CH0_CTRL_REG_IERR_POS     31
#define USB_DMA_CH0_CTRL_REG_IERR_LEN     1
#define USB_DMA_CH0_CTRL_REG_IERR_MSK     0x80000000


/* DMA_CH0_SRC */

#define USB_DMA_CH0_SRC_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH0_SRC_REG_POR_VAL       0x00000000
#define USB_DMA_CH0_SRC_REG_RW_MSK        0xFFFFFFFF

/* DMA_CH0_DST */

#define USB_DMA_CH0_DST_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH0_DST_REG_POR_VAL       0x00000000
#define USB_DMA_CH0_DST_REG_RW_MSK        0xFFFFFFFF

/* DMA_CH0_THR */

#define USB_DMA_CH0_THR_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH0_THR_REG_POR_VAL       0x00000000
#define USB_DMA_CH0_THR_REG_RW_MSK        0xFFFFFFFF

#define USB_DMA_CH0_THR_REG_STHR_POS      0
#define USB_DMA_CH0_THR_REG_STHR_LEN      16
#define USB_DMA_CH0_THR_REG_STHR_MSK      0x0000FFFF

#define USB_DMA_CH0_THR_REG_DTHR_POS      16
#define USB_DMA_CH0_THR_REG_DTHR_LEN      16
#define USB_DMA_CH0_THR_REG_DTHR_MSK      0xFFFF0000

/* DMA_CH0_CNT */

#define USB_DMA_CH0_CNT_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH0_CNT_REG_POR_VAL       0x00000000
#define USB_DMA_CH0_CNT_REG_RW_MSK        0xFFFFFFFF




/* DMA_CH1_STS */

#define USB_DMA_CH1_STS_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH1_STS_REG_POR_VAL       0x00000000
#define USB_DMA_CH1_STS_REG_RW_MSK        0xFFFFFFFF

#define USB_DMA_CH1_STS_REG_STATE_POS     0
#define USB_DMA_CH1_STS_REG_STATE_LEN     2
#define USB_DMA_CH1_STS_REG_STATE_MSK     0x00000003

#define USB_DMA_CH1_STS_REG_ERR_POS       16
#define USB_DMA_CH1_STS_REG_ERR_LEN       8
#define USB_DMA_CH1_STS_REG_ERR_MSK       0x00FF0000

/* DMA_CH1_CTRL */

#define USB_DMA_CH1_CTRL_REG_POR_MSK      0xC7FFFFFF
#define USB_DMA_CH1_CTRL_REG_POR_VAL      0x000C00C0
#define USB_DMA_CH1_CTRL_REG_RW_MSK       0xC7FFFFFC

#define USB_DMA_CH1_CTRL_REG_EN_POS       0
#define USB_DMA_CH1_CTRL_REG_EN_LEN       2
#define USB_DMA_CH1_CTRL_REG_EN_MSK       0x00000003

#define USB_DMA_CH1_CTRL_REG_MODE_POS     2
#define USB_DMA_CH1_CTRL_REG_MODE_LEN     1
#define USB_DMA_CH1_CTRL_REG_MODE_MSK     0x00000004

#define USB_DMA_CH1_CTRL_REG_SRC_POS      3
#define USB_DMA_CH1_CTRL_REG_SRC_LEN      2
#define USB_DMA_CH1_CTRL_REG_SRC_MSK      0x00000018

#define USB_DMA_CH1_CTRL_REG_STYPE_POS    5
#define USB_DMA_CH1_CTRL_REG_STYPE_LEN    2
#define USB_DMA_CH1_CTRL_REG_STYPE_MSK    0x00000060

#define USB_DMA_CH1_CTRL_REG_SADJ_POS     7
#define USB_DMA_CH1_CTRL_REG_SADJ_LEN     2
#define USB_DMA_CH1_CTRL_REG_SADJ_MSK     0x00000180

#define USB_DMA_CH1_CTRL_REG_SFCM_POS     9
#define USB_DMA_CH1_CTRL_REG_SFCM_LEN     2
#define USB_DMA_CH1_CTRL_REG_SFCM_MSK     0x00000600

#define USB_DMA_CH1_CTRL_REG_SFCP_POS     11
#define USB_DMA_CH1_CTRL_REG_SFCP_LEN     4
#define USB_DMA_CH1_CTRL_REG_SFCP_MSK     0x00007800

#define USB_DMA_CH1_CTRL_REG_DST_POS      15
#define USB_DMA_CH1_CTRL_REG_DST_LEN      2
#define USB_DMA_CH1_CTRL_REG_DST_MSK      0x00018000

#define USB_DMA_CH1_CTRL_REG_DTYPE_POS    17
#define USB_DMA_CH1_CTRL_REG_DTYPE_LEN    2
#define USB_DMA_CH1_CTRL_REG_DTYPE_MSK    0x00060000

#define USB_DMA_CH1_CTRL_REG_DADJ_POS     19
#define USB_DMA_CH1_CTRL_REG_DADJ_LEN     2
#define USB_DMA_CH1_CTRL_REG_DADJ_MSK     0x00180000

#define USB_DMA_CH1_CTRL_REG_DFCM_POS     21
#define USB_DMA_CH1_CTRL_REG_DFCM_LEN     2
#define USB_DMA_CH1_CTRL_REG_DFCM_MSK     0x00600000

#define USB_DMA_CH1_CTRL_REG_DFCP_POS     23
#define USB_DMA_CH1_CTRL_REG_DFCP_LEN     4
#define USB_DMA_CH1_CTRL_REG_DFCP_MSK     0x07800000

#define USB_DMA_CH1_CTRL_REG_IEOT_POS     30
#define USB_DMA_CH1_CTRL_REG_IEOT_LEN     1
#define USB_DMA_CH1_CTRL_REG_IEOT_MSK     0x40000000

#define USB_DMA_CH1_CTRL_REG_IERR_POS     31
#define USB_DMA_CH1_CTRL_REG_IERR_LEN     1
#define USB_DMA_CH1_CTRL_REG_IERR_MSK     0x80000000


/* DMA_CH1_SRC */

#define USB_DMA_CH1_SRC_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH1_SRC_REG_POR_VAL       0x00000000
#define USB_DMA_CH1_SRC_REG_RW_MSK        0xFFFFFFFF

/* DMA_CH1_DST */

#define USB_DMA_CH1_DST_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH1_DST_REG_POR_VAL       0x00000000
#define USB_DMA_CH1_DST_REG_RW_MSK        0xFFFFFFFF

/* DMA_CH1_THR */

#define USB_DMA_CH1_THR_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH1_THR_REG_POR_VAL       0x00000000
#define USB_DMA_CH1_THR_REG_RW_MSK        0xFFFFFFFF

#define USB_DMA_CH1_THR_REG_STHR_POS      0
#define USB_DMA_CH1_THR_REG_STHR_LEN      16
#define USB_DMA_CH1_THR_REG_STHR_MSK      0x0000FFFF

#define USB_DMA_CH1_THR_REG_DTHR_POS      16
#define USB_DMA_CH1_THR_REG_DTHR_LEN      16
#define USB_DMA_CH1_THR_REG_DTHR_MSK      0xFFFF0000

/* DMA_CH1_CNT */

#define USB_DMA_CH1_CNT_REG_POR_MSK       0xFFFFFFFF
#define USB_DMA_CH1_CNT_REG_POR_VAL       0x00000000
#define USB_DMA_CH1_CNT_REG_RW_MSK        0xFFFFFFFF


/* DMA_CTRL */

#define USB_DMA_CTRL_REG_POR_MSK          0x00000001
#define USB_DMA_CTRL_REG_POR_VAL          0x00000001
#define USB_DMA_CTRL_REG_RW_MSK           0x00000001

#define USB_DMA_CTRL_REG_EN_POS           0
#define USB_DMA_CTRL_REG_EN_LEN           1
#define USB_DMA_CTRL_REG_EN_MSK           0x00000001

/* DMA_RST */

#define USB_DMA_RST_REG_POR_MSK           0x00000000
#define USB_DMA_RST_REG_POR_VAL           0x00000000

#define USB_DMA_RST_REG_CH0_POS           0
#define USB_DMA_RST_REG_CH0_LEN           1
#define USB_DMA_RST_REG_CH0_MSK           0x00000001

#define USB_DMA_RST_REG_CH1_POS           1
#define USB_DMA_RST_REG_CH1_LEN           1
#define USB_DMA_RST_REG_CH1_MSK           0x00000002

/* DMA_STS */

#define USB_DMA_STS_REG_POR_MSK           0xFFFFFFFF
#define USB_DMA_STS_REG_POR_VAL           0x00000000

#define USB_DMA_STS_REG_CH0STA_POS        0
#define USB_DMA_STS_REG_CH0STA_LEN        2
#define USB_DMA_STS_REG_CH0STA_MSK        0x00000003

#define USB_DMA_STS_REG_CH0ERR_POS        2
#define USB_DMA_STS_REG_CH0ERR_LEN        1
#define USB_DMA_STS_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_STS_REG_CH1STA_POS        4
#define USB_DMA_STS_REG_CH1STA_LEN        2
#define USB_DMA_STS_REG_CH1STA_MSK        0x00000030

#define USB_DMA_STS_REG_CH1ERR_POS        6
#define USB_DMA_STS_REG_CH1ERR_LEN        1
#define USB_DMA_STS_REG_CH1ERR_MSK        0x00000040

/* DMA_ISTS */

#define USB_DMA_ISTS_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_ISTS_REG_POR_VAL          0x00000000

#define USB_DMA_ISTS_REG_CH0EOT_POS        1
#define USB_DMA_ISTS_REG_CH0EOT_LEN        1
#define USB_DMA_ISTS_REG_CH0EOT_MSK        0x00000002

#define USB_DMA_ISTS_REG_CH0ERR_POS        2
#define USB_DMA_ISTS_REG_CH0ERR_LEN        1
#define USB_DMA_ISTS_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_ISTS_REG_CH1EOT_POS        5
#define USB_DMA_ISTS_REG_CH1EOT_LEN        1
#define USB_DMA_ISTS_REG_CH1EOT_MSK        0x00000020

#define USB_DMA_ISTS_REG_CH1ERR_POS        6
#define USB_DMA_ISTS_REG_CH1ERR_LEN        1
#define USB_DMA_ISTS_REG_CH1ERR_MSK        0x00000040

/* DMA_IEN */

#define USB_DMA_IEN_REG_POR_MSK           0xFFFFFFFF
#define USB_DMA_IEN_REG_POR_VAL           0x00000000
#define USB_DMA_IEN_REG_RW_MSK            0x00000000

#define USB_DMA_IEN_REG_CH0EOT_POS        1
#define USB_DMA_IEN_REG_CH0EOT_LEN        1
#define USB_DMA_IEN_REG_CH0EOT_MSK        0x00000002

#define USB_DMA_IEN_REG_CH0ERR_POS        2
#define USB_DMA_IEN_REG_CH0ERR_LEN        1
#define USB_DMA_IEN_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_IEN_REG_CH1EOT_POS        5
#define USB_DMA_IEN_REG_CH1EOT_LEN        1
#define USB_DMA_IEN_REG_CH1EOT_MSK        0x00000020

#define USB_DMA_IEN_REG_CH1ERR_POS        6
#define USB_DMA_IEN_REG_CH1ERR_LEN        1
#define USB_DMA_IEN_REG_CH1ERR_MSK        0x00000040

/* DMA_IDIS */

#define USB_DMA_IDIS_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_IDIS_REG_POR_VAL          0x00000000
#define USB_DMA_IDIS_REG_RW_MSK           0x00000000

#define USB_DMA_IDIS_REG_CH0EOT_POS        1
#define USB_DMA_IDIS_REG_CH0EOT_LEN        1
#define USB_DMA_IDIS_REG_CH0EOT_MSK        0x00000002

#define USB_DMA_IDIS_REG_CH0ERR_POS        2
#define USB_DMA_IDIS_REG_CH0ERR_LEN        1
#define USB_DMA_IDIS_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_IDIS_REG_CH1EOT_POS        5
#define USB_DMA_IDIS_REG_CH1EOT_LEN        1
#define USB_DMA_IDIS_REG_CH1EOT_MSK        0x00000020

#define USB_DMA_IDIS_REG_CH1ERR_POS        6
#define USB_DMA_IDIS_REG_CH1ERR_LEN        1
#define USB_DMA_IDIS_REG_CH1ERR_MSK        0x00000040

/* DMA_ISET */

#define USB_DMA_ISET_REG_POR_MSK          0x00000000
#define USB_DMA_ISET_REG_POR_VAL          0x00000000

#define USB_DMA_ISET_REG_CH0EOT_POS        1
#define USB_DMA_ISET_REG_CH0EOT_LEN        1
#define USB_DMA_ISET_REG_CH0EOT_MSK        0x00000002

#define USB_DMA_ISET_REG_CH0ERR_POS        2
#define USB_DMA_ISET_REG_CH0ERR_LEN        1
#define USB_DMA_ISET_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_ISET_REG_CH1EOT_POS        5
#define USB_DMA_ISET_REG_CH1EOT_LEN        1
#define USB_DMA_ISET_REG_CH1EOT_MSK        0x00000020

#define USB_DMA_ISET_REG_CH1ERR_POS        6
#define USB_DMA_ISET_REG_CH1ERR_LEN        1
#define USB_DMA_ISET_REG_CH1ERR_MSK        0x00000040

/* DMA_ICLR */

#define USB_DMA_ICLR_REG_POR_MSK          0x00000000
#define USB_DMA_ICLR_REG_POR_VAL          0x00000000

#define USB_DMA_ICLR_REG_CH0EOT_POS        1
#define USB_DMA_ICLR_REG_CH0EOT_LEN        1
#define USB_DMA_ICLR_REG_CH0EOT_MSK        0x00000002

#define USB_DMA_ICLR_REG_CH0ERR_POS        2
#define USB_DMA_ICLR_REG_CH0ERR_LEN        1
#define USB_DMA_ICLR_REG_CH0ERR_MSK        0x00000004

#define USB_DMA_ICLR_REG_CH1EOT_POS        5
#define USB_DMA_ICLR_REG_CH1EOT_LEN        1
#define USB_DMA_ICLR_REG_CH1EOT_MSK        0x00000020

#define USB_DMA_ICLR_REG_CH1ERR_POS        6
#define USB_DMA_ICLR_REG_CH1ERR_LEN        1
#define USB_DMA_ICLR_REG_CH1ERR_MSK        0x00000040

/* DMA_FCP0 */

#define USB_DMA_FCP0_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_FCP0_REG_POR_VAL          0x00000000
#define USB_DMA_FCP0_REG_RW_MSK           0x801E003B

#define USB_DMA_FCP0_REG_EN_POS           0
#define USB_DMA_FCP0_REG_EN_LEN           1
#define USB_DMA_FCP0_REG_EN_MSK           0x00000001

#define USB_DMA_FCP0_REG_CTR_POS          1
#define USB_DMA_FCP0_REG_CTR_LEN          1
#define USB_DMA_FCP0_REG_CTR_MSK          0x00000002

#define USB_DMA_FCP0_REG_CEOB_POS         3
#define USB_DMA_FCP0_REG_CEOB_LEN         1
#define USB_DMA_FCP0_REG_CEOB_MSK         0x00000008

#define USB_DMA_FCP0_REG_CEOT_POS         4
#define USB_DMA_FCP0_REG_CEOT_LEN         1
#define USB_DMA_FCP0_REG_CEOT_MSK         0x00000010

#define USB_DMA_FCP0_REG_CTE_POS          5
#define USB_DMA_FCP0_REG_CTE_LEN          1
#define USB_DMA_FCP0_REG_CTE_MSK          0x00000020

#define USB_DMA_FCP0_REG_PTA_POS          17
#define USB_DMA_FCP0_REG_PTA_LEN          1
#define USB_DMA_FCP0_REG_PTA_MSK          0x00020000

#define USB_DMA_FCP0_REG_PEOB_POS         18
#define USB_DMA_FCP0_REG_PEOB_LEN         1
#define USB_DMA_FCP0_REG_PEOB_MSK         0x00040000

#define USB_DMA_FCP0_REG_PEOT_POS         19
#define USB_DMA_FCP0_REG_PEOT_LEN         1
#define USB_DMA_FCP0_REG_PEOT_MSK         0x00080000

#define USB_DMA_FCP0_REG_PTE_POS          20
#define USB_DMA_FCP0_REG_PTE_LEN          1
#define USB_DMA_FCP0_REG_PTE_MSK          0x00100000

#define USB_DMA_FCP0_REG_SYNC_POS         31
#define USB_DMA_FCP0_REG_SYNC_LEN         1
#define USB_DMA_FCP0_REG_SYNC_MSK         0x80000000

/* DMA_FCP1 */

#define USB_DMA_FCP1_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_FCP1_REG_POR_VAL          0x00000000
#define USB_DMA_FCP1_REG_RW_MSK           0x801E003B

#define USB_DMA_FCP1_REG_EN_POS           0
#define USB_DMA_FCP1_REG_EN_LEN           1
#define USB_DMA_FCP1_REG_EN_MSK           0x00000001

#define USB_DMA_FCP1_REG_CTR_POS          1
#define USB_DMA_FCP1_REG_CTR_LEN          1
#define USB_DMA_FCP1_REG_CTR_MSK          0x00000002

#define USB_DMA_FCP1_REG_CEOB_POS         3
#define USB_DMA_FCP1_REG_CEOB_LEN         1
#define USB_DMA_FCP1_REG_CEOB_MSK         0x00000008

#define USB_DMA_FCP1_REG_CEOT_POS         4
#define USB_DMA_FCP1_REG_CEOT_LEN         1
#define USB_DMA_FCP1_REG_CEOT_MSK         0x00000010

#define USB_DMA_FCP1_REG_CTE_POS          5
#define USB_DMA_FCP1_REG_CTE_LEN          1
#define USB_DMA_FCP1_REG_CTE_MSK          0x00000020

#define USB_DMA_FCP1_REG_PTA_POS          17
#define USB_DMA_FCP1_REG_PTA_LEN          1
#define USB_DMA_FCP1_REG_PTA_MSK          0x00020000

#define USB_DMA_FCP1_REG_PEOB_POS         18
#define USB_DMA_FCP1_REG_PEOB_LEN         1
#define USB_DMA_FCP1_REG_PEOB_MSK         0x00040000

#define USB_DMA_FCP1_REG_PEOT_POS         19
#define USB_DMA_FCP1_REG_PEOT_LEN         1
#define USB_DMA_FCP1_REG_PEOT_MSK         0x00080000

#define USB_DMA_FCP1_REG_PTE_POS          20
#define USB_DMA_FCP1_REG_PTE_LEN          1
#define USB_DMA_FCP1_REG_PTE_MSK          0x00100000

#define USB_DMA_FCP1_REG_SYNC_POS         31
#define USB_DMA_FCP1_REG_SYNC_LEN         1
#define USB_DMA_FCP1_REG_SYNC_MSK         0x80000000

/* DMA_FCP2 */

#define USB_DMA_FCP2_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_FCP2_REG_POR_VAL          0x00000000
#define USB_DMA_FCP2_REG_RW_MSK           0x801E003B

#define USB_DMA_FCP2_REG_EN_POS           0
#define USB_DMA_FCP2_REG_EN_LEN           1
#define USB_DMA_FCP2_REG_EN_MSK           0x00000001

#define USB_DMA_FCP2_REG_CTR_POS          1
#define USB_DMA_FCP2_REG_CTR_LEN          1
#define USB_DMA_FCP2_REG_CTR_MSK          0x00000002

#define USB_DMA_FCP2_REG_CEOB_POS         3
#define USB_DMA_FCP2_REG_CEOB_LEN         1
#define USB_DMA_FCP2_REG_CEOB_MSK         0x00000008

#define USB_DMA_FCP2_REG_CEOT_POS         4
#define USB_DMA_FCP2_REG_CEOT_LEN         1
#define USB_DMA_FCP2_REG_CEOT_MSK         0x00000010

#define USB_DMA_FCP2_REG_CTE_POS          5
#define USB_DMA_FCP2_REG_CTE_LEN          1
#define USB_DMA_FCP2_REG_CTE_MSK          0x00000020

#define USB_DMA_FCP2_REG_PTA_POS          17
#define USB_DMA_FCP2_REG_PTA_LEN          1
#define USB_DMA_FCP2_REG_PTA_MSK          0x00020000

#define USB_DMA_FCP2_REG_PEOB_POS         18
#define USB_DMA_FCP2_REG_PEOB_LEN         1
#define USB_DMA_FCP2_REG_PEOB_MSK         0x00040000

#define USB_DMA_FCP2_REG_PEOT_POS         19
#define USB_DMA_FCP2_REG_PEOT_LEN         1
#define USB_DMA_FCP2_REG_PEOT_MSK         0x00080000

#define USB_DMA_FCP2_REG_PTE_POS          20
#define USB_DMA_FCP2_REG_PTE_LEN          1
#define USB_DMA_FCP2_REG_PTE_MSK          0x00100000

#define USB_DMA_FCP2_REG_SYNC_POS         31
#define USB_DMA_FCP2_REG_SYNC_LEN         1
#define USB_DMA_FCP2_REG_SYNC_MSK         0x80000000

/* DMA_FCP3 */

#define USB_DMA_FCP3_REG_POR_MSK          0xFFFFFFFF
#define USB_DMA_FCP3_REG_POR_VAL          0x00000000
#define USB_DMA_FCP3_REG_RW_MSK           0x801E003B

#define USB_DMA_FCP3_REG_EN_POS           0
#define USB_DMA_FCP3_REG_EN_LEN           1
#define USB_DMA_FCP3_REG_EN_MSK           0x00000001

#define USB_DMA_FCP3_REG_CTR_POS          1
#define USB_DMA_FCP3_REG_CTR_LEN          1
#define USB_DMA_FCP3_REG_CTR_MSK          0x00000002

#define USB_DMA_FCP3_REG_CEOB_POS         3
#define USB_DMA_FCP3_REG_CEOB_LEN         1
#define USB_DMA_FCP3_REG_CEOB_MSK         0x00000008

#define USB_DMA_FCP3_REG_CEOT_POS         4
#define USB_DMA_FCP3_REG_CEOT_LEN         1
#define USB_DMA_FCP3_REG_CEOT_MSK         0x00000010

#define USB_DMA_FCP3_REG_CTE_POS          5
#define USB_DMA_FCP3_REG_CTE_LEN          1
#define USB_DMA_FCP3_REG_CTE_MSK          0x00000020

#define USB_DMA_FCP3_REG_PTA_POS          17
#define USB_DMA_FCP3_REG_PTA_LEN          1
#define USB_DMA_FCP3_REG_PTA_MSK          0x00020000

#define USB_DMA_FCP3_REG_PEOB_POS         18
#define USB_DMA_FCP3_REG_PEOB_LEN         1
#define USB_DMA_FCP3_REG_PEOB_MSK         0x00040000

#define USB_DMA_FCP3_REG_PEOT_POS         19
#define USB_DMA_FCP3_REG_PEOT_LEN         1
#define USB_DMA_FCP3_REG_PEOT_MSK         0x00080000

#define USB_DMA_FCP3_REG_PTE_POS          20
#define USB_DMA_FCP3_REG_PTE_LEN          1
#define USB_DMA_FCP3_REG_PTE_MSK          0x00100000

#define USB_DMA_FCP3_REG_SYNC_POS         31
#define USB_DMA_FCP3_REG_SYNC_LEN         1
#define USB_DMA_FCP3_REG_SYNC_MSK         0x80000000

/* DMA_MODID */

#define USB_DMA_MODID_REG_POR_MSK         0xFFFFFFFF
#define USB_DMA_MODID_REG_POR_VAL         0x00000000

#define USB_DMA_MODID_REG_MIN_POS         8
#define USB_DMA_MODID_REG_MIN_LEN         4
#define USB_DMA_MODID_REG_MIN_MSK         0x00000F00

#define USB_DMA_MODID_REG_MAX_POS         12
#define USB_DMA_MODID_REG_MAX_LEN         4
#define USB_DMA_MODID_REG_MAX_MSK         0x0000F000

/*!
 * USB2IP9028 core Register descriptions
 * -------------------------------------
 */

/* DEVADR */

#define USB_DEVADR_REG_POR_MSK     0x000000FF
#define USB_DEVADR_REG_POR_VAL     0x00000000
#define USB_DEVADR_REG_RW_MSK      0x000000FF

#define USB_DEVADR_REG_EN_POS      7
#define USB_DEVADR_REG_EN_LEN      1
#define USB_DEVADR_REG_EN_MSK      0x00000080

#define USB_DEVADR_REG_ADR_POS     0
#define USB_DEVADR_REG_ADR_LEN     7
#define USB_DEVADR_REG_ADR_MSK     0x0000007F

/* EMAXSIZE */

#define USB_EMAXSIZE_REG_POR_MSK   0x00001FFF
#define USB_EMAXSIZE_REG_POR_VAL   0x00000008
#define USB_EMAXSIZE_REG_RW_MSK    0x00001bFF
/* MPS: modified FFOSZ to max. 256 */

#define USB_EMAXSIZE_REG_NTRANS_POS    11
#define USB_EMAXSIZE_REG_NTRANS_LEN    2
#define USB_EMAXSIZE_REG_NTRANS_MSK    0x00001800

#define USB_EMAXSIZE_REG_FFOSZ_POS     0
#define USB_EMAXSIZE_REG_FFOSZ_LEN     11
#define USB_EMAXSIZE_REG_FFOSZ_MSK     0x000007FF

/* ETYPE */

#define USB_ETYPE_REG_POR_MSK   0x000000FF
#define USB_ETYPE_REG_POR_VAL   0x00000008
#define USB_ETYPE_REG_RW_MSK    0x0000001F

#define USB_ETYPE_REG_HDLNUM_POS     5
#define USB_ETYPE_REG_HDLNUM_LEN     3
#define USB_ETYPE_REG_HDLNUM_MSK     0x000000E0

#define USB_ETYPE_REG_DISEOT_POS     4
#define USB_ETYPE_REG_DISEOT_LEN     1
#define USB_ETYPE_REG_DISEOT_MSK     0x00000010

#define USB_ETYPE_REG_EN_POS         3
#define USB_ETYPE_REG_EN_LEN         1
#define USB_ETYPE_REG_EN_MSK         0x00000008

#define USB_ETYPE_REG_DBLBUF_POS     2
#define USB_ETYPE_REG_DBLBUF_LEN     1
#define USB_ETYPE_REG_DBLBUF_MSK     0x00000004

#define USB_ETYPE_REG_TYPE_POS       0
#define USB_ETYPE_REG_TYPE_LEN       2
#define USB_ETYPE_REG_TYPE_MSK       0x00000003

/* MODE */

#define USB_MODE_REG_POR_MSK        0x000000FF
#define USB_MODE_REG_POR_VAL        0x00000000
#define USB_MODE_REG_RW_MSK         0x000000DF

#define USB_MODE_REG_CLKAON_POS     7
#define USB_MODE_REG_CLKAON_LEN     1
#define USB_MODE_REG_CLKAON_MSK     0x00000080

#define USB_MODE_REG_SNDRSU_POS     6
#define USB_MODE_REG_SNDRSU_LEN     1
#define USB_MODE_REG_SNDRSU_MSK     0x00000040

#define USB_MODE_REG_GOSUSP_POS     5
#define USB_MODE_REG_GOSUSP_LEN     1
#define USB_MODE_REG_GOSUSP_MSK     0x00000020

#define USB_MODE_REG_SFRESET_POS    4
#define USB_MODE_REG_SFRESET_LEN    1
#define USB_MODE_REG_SFRESET_MSK    0x00000010

#define USB_MODE_REG_GLINTEN_POS    3
#define USB_MODE_REG_GLINTEN_LEN    1
#define USB_MODE_REG_GLINTEN_MSK    0x00000008

#define USB_MODE_REG_WKUP_POS       2
#define USB_MODE_REG_WKUP_LEN       1
#define USB_MODE_REG_WKUP_MSK       0x00000004

#define USB_MODE_REG_PWROFF_POS     1
#define USB_MODE_REG_PWROFF_LEN     1
#define USB_MODE_REG_PWROFF_MSK     0x00000002

#define USB_MODE_REG_SOFTCT_POS     0
#define USB_MODE_REG_SOFTCT_LEN     1
#define USB_MODE_REG_SOFTCT_MSK     0x00000001

/* INT */

#define USB_INT_CFG_REG_POR_MSK   0x000000FF
#define USB_INT_CFG_REG_POR_VAL   0x000000FC
#define USB_INT_CFG_REG_RW_MSK    0x000000FF

#define USB_INT_CFG_REG_DBG_POS   6
#define USB_INT_CFG_REG_DBG_LEN   2
#define USB_INT_CFG_REG_DBG_MSK   0x000000C0

#define USB_INT_CFG_REG_DBGI_POS  4
#define USB_INT_CFG_REG_DBGI_LEN  2
#define USB_INT_CFG_REG_DBGI_MSK  0x00000030

#define USB_INT_CFG_REG_DBGO_POS  2
#define USB_INT_CFG_REG_DBGO_LEN  2
#define USB_INT_CFG_REG_DBGO_MSK  0x0000000C

#define USB_INT_CFG_REG_LVL_POS   1
#define USB_INT_CFG_REG_LVL_LEN   1
#define USB_INT_CFG_REG_LVL_MSK   0x00000002

#define USB_INT_CFG_REG_POL_POS   0
#define USB_INT_CFG_REG_POL_LEN   1
#define USB_INT_CFG_REG_POL_MSK   0x00000001

/* DCNT */

#define USB_DCNT_REG_POR_MSK   0x000007FF
#define USB_DCNT_REG_POR_VAL   0x00000000
#define USB_DCNT_REG_RW_MSK    0x000007FF

#define USB_DCNT_REG_CNT_POS   0
#define USB_DCNT_REG_CNT_LEN   11
#define USB_DCNT_REG_CNT_MSK   0x000007FF

/* DATA */

#define USB_DATA_REG_POR_MSK   0xFFFFFFFF
#define USB_DATA_REG_POR_VAL   0x00000000
#define USB_DATA_REG_RW_MSK    0xFFFFFFFF

/* SHORT */

#define USB_SHORT_REG_POR_MSK   0x0000FFFF
#define USB_SHORT_REG_POR_VAL   0x00000000
#define USB_SHORT_REG_RW_MSK    0x00000000

#define USB_SHORT_REG_EP3_POS   3
#define USB_SHORT_REG_EP3_LEN   1
#define USB_SHORT_REG_EP3_MSK   0x00000008

#define USB_SHORT_REG_EP2_POS   2
#define USB_SHORT_REG_EP2_LEN   1
#define USB_SHORT_REG_EP2_MSK   0x00000004

#define USB_SHORT_REG_EP1_POS   1
#define USB_SHORT_REG_EP1_LEN   1
#define USB_SHORT_REG_EP1_MSK   0x00000002

#define USB_SHORT_REG_EP0_POS   0
#define USB_SHORT_REG_EP0_LEN   1
#define USB_SHORT_REG_EP0_MSK   0x00000001


/* ECTRL */

#define USB_ECTRL_REG_POR_MSK   0x00000037
#define USB_ECTRL_REG_POR_VAL   0x00000000
#define USB_ECTRL_REG_RW_MSK    0x00000000

#define USB_ECTRL_REG_BFULL_POS    5
#define USB_ECTRL_REG_BFULL_LEN    1
#define USB_ECTRL_REG_BFULL_MSK    0x00000020

#define USB_ECTRL_REG_CLBUF_POS    4
#define USB_ECTRL_REG_CLBUF_LEN    1
#define USB_ECTRL_REG_CLBUF_MSK    0x00000010

#define USB_ECTRL_REG_DATA_POS     2
#define USB_ECTRL_REG_DATA_LEN     1
#define USB_ECTRL_REG_DATA_MSK     0x00000004

#define USB_ECTRL_REG_STS_POS      1
#define USB_ECTRL_REG_STS_LEN      1
#define USB_ECTRL_REG_STS_MSK      0x00000002

#define USB_ECTRL_REG_STALL_POS    0
#define USB_ECTRL_REG_STALL_LEN    1
#define USB_ECTRL_REG_STALL_MSK    0x00000001

/* EIDX */

#define USB_EIDX_REG_POR_MSK   0x0000003F
#define USB_EIDX_REG_POR_VAL   0x00000020
#define USB_EIDX_REG_RW_MSK    0x0000000F

#define USB_EIDX_REG_EP0SUP_POS    5
#define USB_EIDX_REG_EP0SUP_LEN    1
#define USB_EIDX_REG_EP0SUP_MSK    0x00000020

#define USB_EIDX_REG_IDX_POS       1
#define USB_EIDX_REG_IDX_LEN       4
#define USB_EIDX_REG_IDX_MSK       0x0000001E

#define USB_EIDX_REG_DIR_POS       0
#define USB_EIDX_REG_DIR_LEN       1
#define USB_EIDX_REG_DIR_MSK       0x00000001

/* MODID */

#define USB_MODID_REG_POR_MSK   0x00FFFFFF
#define USB_MODID_REG_POR_VAL   0x008CF422  /* chip dependent */
#define USB_MODID_REG_RW_MSK    0x00000000

#define USB_MODID_REG_MOD_POS   10
#define USB_MODID_REG_MOD_LEN   14
#define USB_MODID_REG_MOD_MSK   0x00FFFC00

#define USB_MODID_REG_MAJ_POS   4
#define USB_MODID_REG_MAJ_LEN   6
#define USB_MODID_REG_MAJ_MSK   0x000003F0

#define USB_MODID_REG_MIN_POS   0
#define USB_MODID_REG_MIN_LEN   4
#define USB_MODID_REG_MIN_MSK   0x0000000F

/* FNB */

#define USB_FNB_REG_POR_MSK   0x00003FFF
#define USB_FNB_REG_POR_VAL   0x00000000
#define USB_FNB_REG_RW_MSK    0x00000000

#define USB_FNB_REG_USOF_POS   11
#define USB_FNB_REG_USOF_LEN   3
#define USB_FNB_REG_USOF_MSK   0x00003800

#define USB_FNB_REG_HSOF_POS   8
#define USB_FNB_REG_HSOF_LEN   3
#define USB_FNB_REG_HSOF_MSK   0x00000700

#define USB_FNB_REG_LSOF_POS   0
#define USB_FNB_REG_LSOF_LEN   8
#define USB_FNB_REG_LSOF_MSK   0x000000FF

/* SCRATCH */

#define USB_SCRATCH_REG_POR_MSK   0x0000FFFF
#define USB_SCRATCH_REG_POR_VAL   0x00000000
#define USB_SCRATCH_REG_RW_MSK    0x0000FFFF

#define USB_SCRATCH_REG_SFIRH_POS   8
#define USB_SCRATCH_REG_SFIRH_LEN   8
#define USB_SCRATCH_REG_SFIRH_MSK   0x0000FF00

#define USB_SCRATCH_REG_SFIRL_POS   0
#define USB_SCRATCH_REG_SFIRL_LEN   8
#define USB_SCRATCH_REG_SFIRL_MSK   0x000000FF

/* LOCK */

#define USB_LOCK_REG_POR_MSK   0x00000000
#define USB_LOCK_REG_POR_VAL   0x00000000
#define USB_LOCK_REG_RW_MSK    0x0000FFFF

/* TST */

#define USB_TST_REG_POR_MSK   0x0000009F
#define USB_TST_REG_POR_VAL   0x00000000
#define USB_TST_REG_RW_MSK    0x0000009F

#define USB_TST_REG_HS_POS    7
#define USB_TST_REG_HS_LEN    1
#define USB_TST_REG_HS_MSK    0x00000080

#define USB_TST_REG_FS_POS    4
#define USB_TST_REG_FS_LEN    1
#define USB_TST_REG_FS_MSK    0x00000010

#define USB_TST_REG_PRBS_POS  3
#define USB_TST_REG_PRBS_LEN  1
#define USB_TST_REG_PRBS_MSK  0x00000008

#define USB_TST_REG_K_POS     2
#define USB_TST_REG_K_LEN     1
#define USB_TST_REG_K_MSK     0x00000004

#define USB_TST_REG_J_POS     1
#define USB_TST_REG_J_LEN     1
#define USB_TST_REG_J_MSK     0x00000002

#define USB_TST_REG_SE0_POS   0
#define USB_TST_REG_SE0_LEN   1
#define USB_TST_REG_SE0_MSK   0x00000001

/* SIE */

#define USB_SIE_REG_POR_MSK   0x000000FF
#define USB_SIE_REG_POR_VAL   0x00000000
#define USB_SIE_REG_RW_MSK    0x000000FF

#define USB_SIE_REG_EP0SUP_POS   7
#define USB_SIE_REG_EP0SUP_LEN   1
#define USB_SIE_REG_EP0SUP_MSK   0x00000080

#define USB_SIE_REG_DMA_POS      6
#define USB_SIE_REG_DMA_LEN      1
#define USB_SIE_REG_DMA_MSK      0x00000040

#define USB_SIE_REG_HS_POS       5
#define USB_SIE_REG_HS_LEN       1
#define USB_SIE_REG_HS_MSK       0x00000020

#define USB_SIE_REG_RES_POS      4
#define USB_SIE_REG_RES_LEN      1
#define USB_SIE_REG_RES_MSK      0x00000010

#define USB_SIE_REG_SUSP_POS     3
#define USB_SIE_REG_SUSP_LEN     1
#define USB_SIE_REG_SUSP_MSK     0x00000008

#define USB_SIE_REG_PSOF_POS     2
#define USB_SIE_REG_PSOF_LEN     1
#define USB_SIE_REG_PSOF_MSK     0x00000004

#define USB_SIE_REG_SOF_POS      1
#define USB_SIE_REG_SOF_LEN      1
#define USB_SIE_REG_SOF_MSK      0x00000002

#define USB_SIE_REG_BRST_POS     0
#define USB_SIE_REG_BRST_LEN     1
#define USB_SIE_REG_BRST_MSK     0x00000001

/* EIE */

#define USB_EIE_REG_POR_MSK   0xFFFFFFFF
#define USB_EIE_REG_POR_VAL   0x00000000
#define USB_EIE_REG_RW_MSK    0x0000FFFF

#define USB_EIE_REG_EP7TX_POS	15
#define USB_EIE_REG_EP7TX_LEN	1
#define USB_EIE_REG_EP7TX_MSK	0x00008000

#define USB_EIE_REG_EP7RX_POS	14
#define USB_EIE_REG_EP7RX_LEN	1
#define USB_EIE_REG_EP7RX_MSK	0x00004000

#define USB_EIE_REG_EP6TX_POS	13
#define USB_EIE_REG_EP6TX_LEN	1
#define USB_EIE_REG_EP6TX_MSK	0x00002000

#define USB_EIE_REG_EP6RX_POS	12
#define USB_EIE_REG_EP6RX_LEN	1
#define USB_EIE_REG_EP6RX_MSK	0x00001000

#define USB_EIE_REG_EP5TX_POS	11
#define USB_EIE_REG_EP5TX_LEN	1
#define USB_EIE_REG_EP5TX_MSK	0x00000800

#define USB_EIE_REG_EP5RX_POS	10
#define USB_EIE_REG_EP5RX_LEN	1
#define USB_EIE_REG_EP5RX_MSK	0x00000400

#define USB_EIE_REG_EP4TX_POS	9
#define USB_EIE_REG_EP4TX_LEN	1
#define USB_EIE_REG_EP4TX_MSK	0x00000200

#define USB_EIE_REG_EP4RX_POS	8
#define USB_EIE_REG_EP4RX_LEN	1
#define USB_EIE_REG_EP4RX_MSK	0x00000100

#define USB_EIE_REG_EP3TX_POS   7
#define USB_EIE_REG_EP3TX_LEN   1
#define USB_EIE_REG_EP3TX_MSK   0x00000080

#define USB_EIE_REG_EP3RX_POS   6
#define USB_EIE_REG_EP3RX_LEN   1
#define USB_EIE_REG_EP3RX_MSK   0x00000040

#define USB_EIE_REG_EP2TX_POS   5
#define USB_EIE_REG_EP2TX_LEN   1
#define USB_EIE_REG_EP2TX_MSK   0x00000020

#define USB_EIE_REG_EP2RX_POS   4
#define USB_EIE_REG_EP2RX_LEN   1
#define USB_EIE_REG_EP2RX_MSK   0x00000010

#define USB_EIE_REG_EP1TX_POS   3
#define USB_EIE_REG_EP1TX_LEN   1
#define USB_EIE_REG_EP1TX_MSK   0x00000008

#define USB_EIE_REG_EP1RX_POS   2
#define USB_EIE_REG_EP1RX_LEN   1
#define USB_EIE_REG_EP1RX_MSK   0x00000004

#define USB_EIE_REG_EP0TX_POS   1
#define USB_EIE_REG_EP0TX_LEN   1
#define USB_EIE_REG_EP0TX_MSK   0x00000002

#define USB_EIE_REG_EP0RX_POS   0
#define USB_EIE_REG_EP0RX_LEN   1
#define USB_EIE_REG_EP0RX_MSK   0x00000001


/* SIS */

#define USB_SIS_REG_POR_MSK   0x000000FF
#define USB_SIS_REG_POR_VAL   0x00000004
#define USB_SIS_REG_RW_MSK    0x00000000

#define USB_SIS_REG_EP0SUP_POS   7
#define USB_SIS_REG_EP0SUP_LEN   1
#define USB_SIS_REG_EP0SUP_MSK   0x00000080

#define USB_SIS_REG_DMA_POS      6
#define USB_SIS_REG_DMA_LEN      1
#define USB_SIS_REG_DMA_MSK      0x00000040

#define USB_SIS_REG_HS_POS       5
#define USB_SIS_REG_HS_LEN       1
#define USB_SIS_REG_HS_MSK       0x00000020

#define USB_SIS_REG_RES_POS      4
#define USB_SIS_REG_RES_LEN      1
#define USB_SIS_REG_RES_MSK      0x00000010

#define USB_SIS_REG_SUSP_POS     3
#define USB_SIS_REG_SUSP_LEN     1
#define USB_SIS_REG_SUSP_MSK     0x00000008

#define USB_SIS_REG_PSOF_POS     2
#define USB_SIS_REG_PSOF_LEN     1
#define USB_SIS_REG_PSOF_MSK     0x00000004

#define USB_SIS_REG_SOF_POS      1
#define USB_SIS_REG_SOF_LEN      1
#define USB_SIS_REG_SOF_MSK      0x00000002

#define USB_SIS_REG_BRST_POS     0
#define USB_SIS_REG_BRST_LEN     1
#define USB_SIS_REG_BRST_MSK     0x00000001


/* EIS */

#define USB_EIS_REG_POR_MSK   0xFFFFFFFF
#define USB_EIS_REG_POR_VAL   0x00000000
#define USB_EIS_REG_RW_MSK    0x00000000

#define USB_EIS_REG_EP3TX_POS   7
#define USB_EIS_REG_EP3TX_LEN   1
#define USB_EIS_REG_EP3TX_MSK   0x00000080

#define USB_EIS_REG_EP3RX_POS   6
#define USB_EIS_REG_EP3RX_LEN   1
#define USB_EIS_REG_EP3RX_MSK   0x00000040

#define USB_EIS_REG_EP2TX_POS   5
#define USB_EIS_REG_EP2TX_LEN   1
#define USB_EIS_REG_EP2TX_MSK   0x00000020

#define USB_EIS_REG_EP2RX_POS   4
#define USB_EIS_REG_EP2RX_LEN   1
#define USB_EIS_REG_EP2RX_MSK   0x00000010

#define USB_EIS_REG_EP1TX_POS   3
#define USB_EIS_REG_EP1TX_LEN   1
#define USB_EIS_REG_EP1TX_MSK   0x00000008

#define USB_EIS_REG_EP1RX_POS   2
#define USB_EIS_REG_EP1RX_LEN   1
#define USB_EIS_REG_EP1RX_MSK   0x00000004

#define USB_EIS_REG_EP0TX_POS   1
#define USB_EIS_REG_EP0TX_LEN   1
#define USB_EIS_REG_EP0TX_MSK   0x00000002

#define USB_EIS_REG_EP0RX_POS   0
#define USB_EIS_REG_EP0RX_LEN   1
#define USB_EIS_REG_EP0RX_MSK   0x00000001

/* EIC */

#define USB_EIC_REG_POR_MSK   0x00000000
#define USB_EIC_REG_POR_VAL   0x00000000
#define USB_EIC_REG_RW_MSK    0xFFFFFFFF

#define USB_EIC_REG_EP3TX_POS   7
#define USB_EIC_REG_EP3TX_LEN   1
#define USB_EIC_REG_EP3TX_MSK   0x00000080

#define USB_EIC_REG_EP3RX_POS   6
#define USB_EIC_REG_EP3RX_LEN   1
#define USB_EIC_REG_EP3RX_MSK   0x00000040

#define USB_EIC_REG_EP2TX_POS   5
#define USB_EIC_REG_EP2TX_LEN   1
#define USB_EIC_REG_EP2TX_MSK   0x00000020

#define USB_EIC_REG_EP2RX_POS   4
#define USB_EIC_REG_EP2RX_LEN   1
#define USB_EIC_REG_EP2RX_MSK   0x00000010

#define USB_EIC_REG_EP1TX_POS   3
#define USB_EIC_REG_EP1TX_LEN   1
#define USB_EIC_REG_EP1TX_MSK   0x00000008

#define USB_EIC_REG_EP1RX_POS   2
#define USB_EIC_REG_EP1RX_LEN   1
#define USB_EIC_REG_EP1RX_MSK   0x00000004

#define USB_EIC_REG_EP0TX_POS   1
#define USB_EIC_REG_EP0TX_LEN   1
#define USB_EIC_REG_EP0TX_MSK   0x00000002

#define USB_EIC_REG_EP0RX_POS   0
#define USB_EIC_REG_EP0RX_LEN   1
#define USB_EIC_REG_EP0RX_MSK   0x00000001

/* EISET */

#define USB_EISET_REG_POR_MSK   0x00000000
#define USB_EISET_REG_POR_VAL   0x00000000
#define USB_EISET_REG_RW_MSK    0xFFFFFFFF

#define USB_EISET_REG_EP3TX_POS   7
#define USB_EISET_REG_EP3TX_LEN   1
#define USB_EISET_REG_EP3TX_MSK   0x00000080

#define USB_EISET_REG_EP3RX_POS   6
#define USB_EISET_REG_EP3RX_LEN   1
#define USB_EISET_REG_EP3RX_MSK   0x00000040

#define USB_EISET_REG_EP2TX_POS   5
#define USB_EISET_REG_EP2TX_LEN   1
#define USB_EISET_REG_EP2TX_MSK   0x00000020

#define USB_EISET_REG_EP2RX_POS   4
#define USB_EISET_REG_EP2RX_LEN   1
#define USB_EISET_REG_EP2RX_MSK   0x00000010

#define USB_EISET_REG_EP1TX_POS   3
#define USB_EISET_REG_EP1TX_LEN   1
#define USB_EISET_REG_EP1TX_MSK   0x00000008

#define USB_EISET_REG_EP1RX_POS   2
#define USB_EISET_REG_EP1RX_LEN   1
#define USB_EISET_REG_EP1RX_MSK   0x00000004

#define USB_EISET_REG_EP0TX_POS   1
#define USB_EISET_REG_EP0TX_LEN   1
#define USB_EISET_REG_EP0TX_MSK   0x00000002

#define USB_EISET_REG_EP0RX_POS   0
#define USB_EISET_REG_EP0RX_LEN   1
#define USB_EISET_REG_EP0RX_MSK   0x00000001

/* EIL */

#define USB_EIL_REG_POR_MSK   0x00000000
#define USB_EIL_REG_POR_VAL   0x00000000
#define USB_EIL_REG_RW_MSK    0xFFFFFFFF

#define USB_EIL_REG_EP3TX_POS   7
#define USB_EIL_REG_EP3TX_LEN   1
#define USB_EIL_REG_EP3TX_MSK   0x00000080

#define USB_EIL_REG_EP3RX_POS   6
#define USB_EIL_REG_EP3RX_LEN   1
#define USB_EIL_REG_EP3RX_MSK   0x00000040

#define USB_EIL_REG_EP2TX_POS   5
#define USB_EIL_REG_EP2TX_LEN   1
#define USB_EIL_REG_EP2TX_MSK   0x00000020

#define USB_EIL_REG_EP2RX_POS   4
#define USB_EIL_REG_EP2RX_LEN   1
#define USB_EIL_REG_EP2RX_MSK   0x00000010

#define USB_EIL_REG_EP1TX_POS   3
#define USB_EIL_REG_EP1TX_LEN   1
#define USB_EIL_REG_EP1TX_MSK   0x00000008

#define USB_EIL_REG_EP1RX_POS   2
#define USB_EIL_REG_EP1RX_LEN   1
#define USB_EIL_REG_EP1RX_MSK   0x00000004

#define USB_EIL_REG_EP0TX_POS   1
#define USB_EIL_REG_EP0TX_LEN   1
#define USB_EIL_REG_EP0TX_MSK   0x00000002

#define USB_EIL_REG_EP0RX_POS   0
#define USB_EIL_REG_EP0RX_LEN   1
#define USB_EIL_REG_EP0RX_MSK   0x00000001

/* SIC */

#define USB_SIC_REG_POR_MSK   0x00000000
#define USB_SIC_REG_POR_VAL   0x00000000
#define USB_SIC_REG_RW_MSK    0x000000FF

#define USB_SIC_REG_EP0SUP_POS   7
#define USB_SIC_REG_EP0SUP_LEN   1
#define USB_SIC_REG_EP0SUP_MSK   0x00000080

#define USB_SIC_REG_DMA_POS      6
#define USB_SIC_REG_DMA_LEN      1
#define USB_SIC_REG_DMA_MSK      0x00000040

#define USB_SIC_REG_HS_POS       5
#define USB_SIC_REG_HS_LEN       1
#define USB_SIC_REG_HS_MSK       0x00000020

#define USB_SIC_REG_RES_POS      4
#define USB_SIC_REG_RES_LEN      1
#define USB_SIC_REG_RES_MSK      0x00000010

#define USB_SIC_REG_SUSP_POS     3
#define USB_SIC_REG_SUSP_LEN     1
#define USB_SIC_REG_SUSP_MSK     0x00000008

#define USB_SIC_REG_PSOF_POS     2
#define USB_SIC_REG_PSOF_LEN     1
#define USB_SIC_REG_PSOF_MSK     0x00000004

#define USB_SIC_REG_SOF_POS      1
#define USB_SIC_REG_SOF_LEN      1
#define USB_SIC_REG_SOF_MSK      0x00000002

#define USB_SIC_REG_BRST_POS     0
#define USB_SIC_REG_BRST_LEN     1
#define USB_SIC_REG_BRST_MSK     0x00000001

/* SISET */

#define USB_SISET_REG_POR_MSK   0x00000000
#define USB_SISET_REG_POR_VAL   0x00000000
#define USB_SISET_REG_RW_MSK    0x000000FF

#define USB_SISET_REG_EP0SUP_POS   7
#define USB_SISET_REG_EP0SUP_LEN   1
#define USB_SISET_REG_EP0SUP_MSK   0x00000080

#define USB_SISET_REG_DMA_POS      6
#define USB_SISET_REG_DMA_LEN      1
#define USB_SISET_REG_DMA_MSK      0x00000040

#define USB_SISET_REG_HS_POS       5
#define USB_SISET_REG_HS_LEN       1
#define USB_SISET_REG_HS_MSK       0x00000020

#define USB_SISET_REG_RES_POS      4
#define USB_SISET_REG_RES_LEN      1
#define USB_SISET_REG_RES_MSK      0x00000010

#define USB_SISET_REG_SUSP_POS     3
#define USB_SISET_REG_SUSP_LEN     1
#define USB_SISET_REG_SUSP_MSK     0x00000008

#define USB_SISET_REG_PSOF_POS     2
#define USB_SISET_REG_PSOF_LEN     1
#define USB_SISET_REG_PSOF_MSK     0x00000004

#define USB_SISET_REG_SOF_POS      1
#define USB_SISET_REG_SOF_LEN      1
#define USB_SISET_REG_SOF_MSK      0x00000002

#define USB_SISET_REG_BRST_POS     0
#define USB_SISET_REG_BRST_LEN     1
#define USB_SISET_REG_BRST_MSK     0x00000001

/* SIL */

#define USB_SIL_REG_POR_MSK   0x00000000
#define USB_SIL_REG_POR_VAL   0x00000000
#define USB_SIL_REG_RW_MSK    0x000000FF

#define USB_SIL_REG_EP0SUP_POS   7
#define USB_SIL_REG_EP0SUP_LEN   1
#define USB_SIL_REG_EP0SUP_MSK   0x00000080

#define USB_SIL_REG_DMA_POS      6
#define USB_SIL_REG_DMA_LEN      1
#define USB_SIL_REG_DMA_MSK      0x00000040

#define USB_SIL_REG_HS_POS       5
#define USB_SIL_REG_HS_LEN       1
#define USB_SIL_REG_HS_MSK       0x00000020

#define USB_SIL_REG_RES_POS      4
#define USB_SIL_REG_RES_LEN      1
#define USB_SIL_REG_RES_MSK      0x00000010

#define USB_SIL_REG_SUSP_POS     3
#define USB_SIL_REG_SUSP_LEN     1
#define USB_SIL_REG_SUSP_MSK     0x00000008

#define USB_SIL_REG_PSOF_POS     2
#define USB_SIL_REG_PSOF_LEN     1
#define USB_SIL_REG_PSOF_MSK     0x00000004

#define USB_SIL_REG_SOF_POS      1
#define USB_SIL_REG_SOF_LEN      1
#define USB_SIL_REG_SOF_MSK      0x00000002

#define USB_SIL_REG_BRST_POS     0
#define USB_SIL_REG_BRST_LEN     1
#define USB_SIL_REG_BRST_MSK     0x00000001


/*======================================================================== */
/* USB OTG */
#define OTG_ID_REG_POR_MSK		0x00ffffff
#define OTG_ID_REG_POR_VAL		0x40fa05
#define OTG_ID_REG_RW_MSK		0x0

#define OTG_HWGENERAL_REG_POR_MSK	0x000003ff
#define OTG_HWGENERAL_REG_POR_VAL	0x215
#define OTG_HWGENERAL_REG_RW_MSK	0x0

#define OTG_HWHOST_REG_POR_MSK		0xffff000f
#define OTG_HWHOST_REG_POR_VAL		0x10020001
#define OTG_HWHOST_REG_RW_MSK		0x0

#define OTG_HWDEVICE_REG_POR_MSK	0x0000003f
#define OTG_HWDEVICE_REG_POR_VAL	0x9
#define OTG_HWDEVICE_REG_RW_MSK		0x0

#define OTG_HWTXBUF_REG_POR_MSK		0x80ffffff
#define OTG_HWTXBUF_REG_POR_VAL		0x80070910
#define OTG_HWTXBUF_REG_RW_MSK		0x0

#define OTG_HWRXBUF_REG_POR_MSK		0x0000ffff
#define OTG_HWRXBUF_REG_POR_VAL		0x810
#define OTG_HWRXBUF_REG_RW_MSK		0x0

#define OTG_HWTTTXBUF_REG_POR_MSK	0xffffffff
#define OTG_HWTTTXBUF_REG_POR_VAL	0x0
#define OTG_HWTTTXBUF_REG_RW_MSK	0x0

#define OTG_HWTTRXBUF_REG_POR_MSK	0xffffffff
#define OTG_HWTTRXBUF_REG_POR_VAL	0x0
#define OTG_HWTTRXBUF_REG_RW_MSK	0x0

#define OTG_IP_INFO_REG_REG_POR_MSK	0xffffffff
#define OTG_IP_INFO_REG_REG_POR_VAL	0x90282300
#define OTG_IP_INFO_REG_REG_RW_MSK	0x0

/*Capability registers */
#define OTG_CAPLENGTH_REG_POR_MSK	0x000000ff
#define OTG_CAPLENGTH_REG_POR_VAL	0x40
#define OTG_CAPLENGTH_REG_RW_MSK	0x0

#define OTG_HCIVERSION_REG_POR_MSK	0x0000ffff
#define OTG_HCIVERSION_REG_POR_VAL	0x0100
#define OTG_HCIVERSION_REG_RW_MSK	0x0

#define OTG_HCSPARAMS_REG_POR_MSK	0x0ff1ff1f
#define OTG_HCSPARAMS_REG_POR_VAL	0x10011
#define OTG_HCSPARAMS_REG_RW_MSK	0x0

#define OTG_HCCPARAMS_REG_POR_MSK	0x0000fff7
#define OTG_HCCPARAMS_REG_POR_VAL	0x6
#define OTG_HCCPARAMS_REG_RW_MSK	0x0

#define OTG_DCIVERSION_REG_POR_MSK	0x0000ffff
#define OTG_DCIVERSION_REG_POR_VAL	0x1
#define OTG_DCIVERSION_REG_RW_MSK	0x0

#define OTG_DCCPARAMS_REG_POR_MSK	0x0000019f
#define OTG_DCCPARAMS_REG_POR_VAL	0x184
#define OTG_DCCPARAMS_REG_RW_MSK	0x0

/*Operational registers */
#define OTG_USBCMD_REG_POR_MSK		0x00ffbbff
#define OTG_USBCMD_REG_POR_VAL		0x80000
#define OTG_USBCMD_REG_RW_MSK		0x00ff2000

#define OTG_USBSTS_REG_POR_MSK		0x0001f5ff
#define OTG_USBSTS_REG_POR_VAL		0x80
#define OTG_USBSTS_REG_RW_MSK		0x0

#define OTG_USBINTR_REG_POR_MSK		0x000105ff
#define OTG_USBINTR_REG_POR_VAL		0x0
#define OTG_USBINTR_REG_RW_MSK		0x000101d7

#define OTG_FRINDEX_REG_POR_MSK		0x00003fff
#define OTG_FRINDEX_REG_POR_VAL		0x0
#define OTG_FRINDEX_REG_RW_MSK		0x0

#define OTG_PERIODICLISTBASE__DEVICEADDR_REG_POR_MSK	0xfffff000
#define OTG_PERIODICLISTBASE__DEVICEADDR_REG_POR_VAL	0x0
#define OTG_PERIODICLISTBASE__DEVICEADDR_REG_RW_MSK	0xfe000000

#define OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG_POR_MSK	0xffffffe0
#define OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG_POR_VAL	0x0
#define OTG_ASYNCLISTADDR__ENDPOINTLISTADDR_REG_RW_MSK	0xfffff800

#define OTG_TTCTRL_REG_POR_MSK		0x7f000000
#define OTG_TTCTRL_REG_POR_VAL		0x0
#define OTG_TTCTRL_REG_RW_MSK		0x0

#define OTG_BURSTSIZE_REG_POR_MSK	0x0001ffff
#define OTG_BURSTSIZE_REG_POR_VAL	0x1010
#define OTG_BURSTSIZE_REG_RW_MSK	0x0000ffff

#define OTG_TXFILLTUNING_REG_POR_MSK	0x003f1fff
#define OTG_TXFILLTUNING_REG_POR_VAL	0x0
#define OTG_TXFILLTUNING_REG_RW_MSK	0x003f00ff

#define OTG_TXTTFILLTUNING_REG_POR_MSK	0xffffffff
#define OTG_TXTTFILLTUNING_REG_POR_VAL	0x0
#define OTG_TXTTFILLTUNING_REG_RW_MSK	0x0

#define OTG_ENDPTNAK_REG_POR_MSK	0xffffffff
#define OTG_ENDPTNAK_REG_POR_VAL	0x0
#define OTG_ENDPTNAK_REG_RW_MSK		0x0

#define OTG_ENDPTNAKEN_REG_POR_MSK	0xffffffff
#define OTG_ENDPTNAKEN_REG_POR_VAL	0x0
#define OTG_ENDPTNAKEN_REG_RW_MSK	0x000f000f

#define OTG_CONFIGFLAG_REG_POR_MSK	0xffffffff
#define OTG_CONFIGFLAG_REG_POR_VAL	0x1
#define OTG_CONFIGFLAG_REG_RW_MSK	0x0

#define OTG_PORTSC1_REG_POR_MSK		0xffffffff
#define OTG_PORTSC1_REG_POR_VAL		0x3c000804
#define OTG_PORTSC1_REG_RW_MSK		0x0000c000

/*#define OTG_PORTSC2_REG_POR_MSK		0xffffffff */
/*#define OTG_PORTSC2_REG_POR_VAL		0x3c000804 */
/*#define OTG_PORTSC2_REG_RW_MSK		0x0080c000 */
/* */
/*#define OTG_PORTSC3_REG_POR_MSK		0xffffffff */
/*#define OTG_PORTSC3_REG_POR_VAL		0x3c000804 */
/*#define OTG_PORTSC3_REG_RW_MSK		0x0080c000 */
/* */
#define OTG_OTGSC_REG_POR_MSK		0xffffffff
#define OTG_OTGSC_REG_POR_VAL		0x20
#define OTG_OTGSC_REG_RW_MSK		0x7f0000bf

#define OTG_USBMODE_REG_POR_MSK		0x0000001f
#define OTG_USBMODE_REG_POR_VAL		0x0
#define OTG_USBMODE_REG_RW_MSK		0x0000001c

#define OTG_ENDPTSETUPSTAT_REG_POR_MSK	0xffffffff
#define OTG_ENDPTSETUPSTAT_REG_POR_VAL	0x0
#define OTG_ENDPTSETUPSTAT_REG_RW_MSK	0x0000ffff /* R/WC */

#define OTG_ENDPTPRIME_REG_POR_MSK	0xffffffff
#define OTG_ENDPTPRIME_REG_POR_VAL	0x0
#define OTG_ENDPTPRIME_REG_RW_MSK	0xffffffff /* R/WS (write to set) */

#define OTG_ENDPTFLUSH_REG_POR_MSK	0xffffffff
#define OTG_ENDPTFLUSH_REG_POR_VAL	0x0
#define OTG_ENDPTFLUSH_REG_RW_MSK	0xffffffff /* R/WS (write to set) */

#define OTG_ENDPTSTATUS_REG_POR_MSK	0xffffffff
#define OTG_ENDPTSTATUS_REG_POR_VAL	0x0
#define OTG_ENDPTSTATUS_REG_RW_MSK	0x0

#define OTG_ENDPTCOMPLETE_REG_POR_MSK	0xffffffff
#define OTG_ENDPTCOMPLETE_REG_POR_VAL	0x0
#define OTG_ENDPTCOMPLETE_REG_RW_MSK	0xffffffff /* R/WC */

#define OTG_ENDPTCTRL0_REG_POR_MSK	0xffffffff
#define OTG_ENDPTCTRL0_REG_POR_VAL	0x800080
#define OTG_ENDPTCTRL0_REG_RW_MSK	0x00010001

#define OTG_ENDPTCTRL1_REG_POR_MSK	0xffffffff
#define OTG_ENDPTCTRL1_REG_POR_VAL	0x0
#define OTG_ENDPTCTRL1_REG_RW_MSK	0x002d002d

#define OTG_ENDPTCTRL2_REG_POR_MSK	0xffffffff
#define OTG_ENDPTCTRL2_REG_POR_VAL	0x0
#define OTG_ENDPTCTRL2_REG_RW_MSK	0x002d002d

#define OTG_ENDPTCTRL3_REG_POR_MSK	0xffffffff
#define OTG_ENDPTCTRL3_REG_POR_VAL	0x0
#define OTG_ENDPTCTRL3_REG_RW_MSK	0x002d002d


#endif /* _PNX0106_USB2IP9028_H_ */	
