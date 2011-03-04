/*
 *  linux/arch/arm/mach-pnx0106/clocks.c
 *
 *  Copyright (C) 2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <asm/arch/clocks.h>
#include <asm/arch/gpio.h>
#include <asm/arch/syscreg.h>


/****************************************************************************
****************************************************************************/

#define CGU_NR_PLL            3
#define CGU_NR_BASE          19
#define CGU_NR_BASE_BCR       4
#define CGU_NR_FD            28
#define CGU_NR_CLK_ESR      104

typedef struct
{
    volatile unsigned int base_scr[19];     /* 0x000 .. 0x048 : Switch control */
    volatile unsigned int base_fs[2][19];   /* 0x04C .. 0x0E0 : Frequency select side 1, side 2 */
    volatile unsigned int base_ssr[19];     /* 0x0E4 .. 0x12C : Switch status */
    volatile unsigned int clk_pcr[108];     /* 0x130 .. 0x2DC : Clock power control / enable */
    volatile unsigned int clk_psr[108];     /* 0x2E0 .. 0x48C : Clock power status */
    volatile unsigned int clk_esr[104];     /* 0x490 .. 0x62C : Clock Fracdiv selection */
    volatile unsigned int base_bcr[4];      /* 0x630 .. 0x63C : Fracdiv master enable on base-wide basis */
    volatile unsigned int base_fdc[28];     /* 0x640 .. 0x6AC : Fracdiv config */
    volatile unsigned int base_dyn_fdc[2];  /* 0x6B0 .. 0x6B4 : Dynamic fracdiv config */
    volatile unsigned int _d1[2];           /* 0x6B8 .. 0x6BC : Don't ask (nothing to do with CGU...) */
    volatile unsigned int base_dyn_sel[2];  /* 0x6C0 .. 0x6C4 : Dynamic fracdiv high speed enable selection config */
    volatile unsigned int _d2[2];           /* 0x6C8 .. 0x6CC : Don't ask (nothing to do with CGU...) */
}
CGU_REGS_t;

#define CGU ((CGU_REGS_t *) IO_ADDRESS_CGU_REGS_BASE)

#define SCR_ENABLE1     (1 << 0)
#define SCR_ENABLE2     (1 << 1)
#define SCR_RESET       (1 << 2)
#define SCR_STOP        (1 << 3)

#define PCR_RUN         (1 << 0)
#define PCR_AUTO        (1 << 1)
#define PCR_WAKE_EN     (1 << 2)
#define PCR_EXTEN_EN    (1 << 3)
#define PCR_ENOUT_EN    (1 << 4)

#define FDC_RUN         (1 << 0)
#define FDC_RESET       (1 << 1)
#define FDC_STRETCH     (1 << 2)

#define DYN_FDC_RUN     (1 << 0)
#define DYN_FDC_ALLOW   (1 << 1)
#define DYN_FDC_STRETCH (1 << 2)

/* DYN_FDC_MADD == bits  3..10 */
/* DYN_FDC_MSUB == bits 11..18 */

#define DYN_SEL_ADSS            (1 <<  0)
#define DYN_SEL_SDMA            (1 <<  2)
#define DYN_SEL_ARM926_I        (1 <<  4)
#define DYN_SEL_ARM926_D        (1 <<  6)
#define DYN_SEL_USB             (1 <<  8)
#define DYN_SEL_USB_OTG         (1 << 10)
#define DYN_SEL_PCCARD          (1 << 12)
#define DYN_SEL_GDMA            (1 << 14)
#define DYN_SEL_TVOUT           (1 << 16)
#define DYN_SEL_EXT_REFRESH     (1 << 18)
#define DYN_SEL_EPICS_SYNC_OUT  (1 << 19)
#define DYN_SEL_AHB2AHB         (1 << 20)

#define BCR_RUN         (1 << 0)

#define ESR_ENABLE      (1 << 0)


/****************************************************************************
****************************************************************************/

typedef struct
{
    volatile unsigned int power_mode;
    volatile unsigned int wd_bark;
    volatile unsigned int f32khz_on;
    volatile unsigned int f32khz_bypass;
    volatile unsigned int ffast_on;
    volatile unsigned int ffast_bypass;

    /* more registers here... but ignored for now */
}
CGU_CONFIG_REGS_t;

#define CGU_CONFIG ((CGU_CONFIG_REGS_t *) IO_ADDRESS_CGU_CONFIG_REGS_BASE)


/****************************************************************************
****************************************************************************/

typedef struct
{
    volatile unsigned int fin_select;       /* 0x00 : */
    volatile unsigned int mdec;             /* 0x04 : */
    volatile unsigned int ndec;             /* 0x08 : */
    volatile unsigned int pdec;             /* 0x0C : */
    volatile unsigned int mode;             /* 0x10 : */
    volatile unsigned int status;           /* 0x14 : */
    volatile unsigned int ack;              /* 0x18 : */
    volatile unsigned int req;              /* 0x1C : */
    volatile unsigned int inselr;           /* 0x20 : */
    volatile unsigned int inseli;           /* 0x24 : */
    volatile unsigned int inselp;           /* 0x28 : */
    volatile unsigned int nselr;            /* 0x2C : */
    volatile unsigned int nseli;            /* 0x30 : */
    volatile unsigned int nselp;            /* 0x34 : */
}
HPPLL_REGS_t;

#define HPPLL ((HPPLL_REGS_t *) IO_ADDRESS_CGU_HPPLL0_REGS_BASE)

#define HPPLL_MODE_CLKEN        (1 << 0)
#define HPPLL_MODE_SKEWEN       (1 << 1)
#define HPPLL_MODE_PD           (1 << 2)
#define HPPLL_MODE_DIRECTO      (1 << 3)
#define HPPLL_MODE_DIRECTI      (1 << 4)
#define HPPLL_MODE_FRM          (1 << 5)
#define HPPLL_MODE_BANDSEL      (1 << 6)
#define HPPLL_MODE_LIMUPOFF     (1 << 7)
#define HPPLL_MODE_BYBASS       (1 << 8)


/****************************************************************************
****************************************************************************/

typedef struct
{
    volatile unsigned int fin_select;       /* 0x00 : */
    volatile unsigned int pwd;              /* 0x04 : */
    volatile unsigned int bypass;           /* 0x08 : */
    volatile unsigned int lock;             /* 0x0C : */
    volatile unsigned int direct;           /* 0x10 : */
    volatile unsigned int msel;             /* 0x14 : */
    volatile unsigned int psel;             /* 0x18 : */
}
LPPLL_t;

typedef struct
{
    LPPLL_t pll[2];
}
LPPLL_REGS_t;

#define LPPLL ((LPPLL_REGS_t *) IO_ADDRESS_CGU_LPPLL0_REGS_BASE)


/****************************************************************************
****************************************************************************/

#define CGU_REFID_FSLOW                         0
#define CGU_REFID_FFAST                         1
#define CGU_REFID_XT_IEEE1394_PHY_SCLK_INV      2
#define CGU_REFID_XT_MCI_CLK                    3
#define CGU_REFID_XT_DAI_BCK                    4
#define CGU_REFID_XT_DAI_WS                     5
#define CGU_REFID_MELODY_ADSS_SPD3_BCK          6
#define CGU_REFID_MELODY_ADSS_SPD3_WS           7
#define CGU_REFID_HP0_FOUT                      8
#define CGU_REFID_LP0_FOUT                      9
#define CGU_REFID_LP1_FOUT                     10

#define CGU_REFID_DISABLE                      11


/****************************************************************************
****************************************************************************/

#define CGU_BASEID_SYS                          0
#define CGU_BASEID_AHB0_VPB0                    1
#define CGU_BASEID_AHB0_VPB1                    2
#define CGU_BASEID_AHB0_VPB3                    3
#define CGU_BASEID_PHY1394                      4
#define CGU_BASEID_CCP_IPINT0                   5
#define CGU_BASEID_CCP_IPINT1                   6
#define CGU_BASEID_CLK_RTC                      7
#define CGU_BASEID_MCI_CLK                      8
#define CGU_BASEID_UARTCLK0                     9
#define CGU_BASEID_UARTCLK1                    10
#define CGU_BASEID_CLK1024FS                   11
#define CGU_BASEID_DAI_BCK                     12
#define CGU_BASEID_SPD3_BCK                    13
#define CGU_BASEID_ATACLK                      14
#define CGU_BASEID_GDMACLK                     15
#define CGU_BASEID_TV_OUT_XCLK                 16
#define CGU_BASEID_CLKOUT0                     17
#define CGU_BASEID_CLKOUT1                     18


/****************************************************************************
****************************************************************************/

#define CGU_FDID_SYS_FD0                        0
#define CGU_FDID_SYS_FD1                        1
#define CGU_FDID_SYS_FD2                        2
#define CGU_FDID_SYS_FD3                        3
#define CGU_FDID_SYS_FD4                        4
#define CGU_FDID_SYS_FD5                        5
#define CGU_FDID_AHB0_VPB0_FD0                  6
#define CGU_FDID_AHB0_VPB0_FD1                  7
#define CGU_FDID_AHB0_VPB1_FD0                  8
#define CGU_FDID_AHB0_VPB3_FD0                  9
#define CGU_FDID_PHY1394_FD0                   10
#define CGU_FDID_CCP_IPINT0_FD0                11
#define CGU_FDID_CCP_IPINT1_FD0                12
#define CGU_FDID_UARTCLK0_FD0                  13
#define CGU_FDID_UARTCLK1_FD0                  14
#define CGU_FDID_CLK1024FS_FD0                 15
#define CGU_FDID_CLK1024FS_FD1                 16
#define CGU_FDID_CLK1024FS_FD2                 17
#define CGU_FDID_CLK1024FS_FD3                 18
#define CGU_FDID_CLK1024FS_FD4                 19       /* this one is special... it has 10bit Madd and Msub controls rather than 8bits */
#define CGU_FDID_CLK1024FS_FD5                 20
#define CGU_FDID_CLK1024FS_FD6                 21
#define CGU_FDID_ATACLK_FD0                    22
#define CGU_FDID_ATACLK_FD1                    23
#define CGU_FDID_GDMACLK_FD0                   24
#define CGU_FDID_TV_OUT_XCLK_FD0               25
#define CGU_FDID_CLKOUT0_FD0                   26
#define CGU_FDID_CLKOUT1_FD0                   27

#define CGU_FDID_SYS_DYN_FD0                   28      /* partners with CGU_FDID_SYS_FD0 */
#define CGU_FDID_SYS_DYN_FD1                   29      /* partners with CGU_FDID_SYS_FD1 */


/****************************************************************************
****************************************************************************/

#define CGU_CLKID_VPB0_CLK                      0
#define CGU_CLKID_VPB1_CLK                      1
#define CGU_CLKID_VPB2_CLK                      2
#define CGU_CLKID_VPB3_CLK                      3
#define CGU_CLKID_AHB2MMIO_CLK                  4
#define CGU_CLKID_AHB0_CLK                      5
#define CGU_CLKID_AHB1_CLK                      6
#define CGU_CLKID_CCP_IPINT0_PCLK               7
#define CGU_CLKID_CCP_IPINT0_VPB_PCLK           8
#define CGU_CLKID_CCP_IPINT1_PCLK               9
#define CGU_CLKID_CCP_IPINT1_VPB_PCLK           10
#define CGU_CLKID_SSA1_MCI_PL180_PCLK           11
#define CGU_CLKID_SSA1_MCI_PL180_MCLK           12
#define CGU_CLKID_SSA1_MCI_PL180_MCLK_N         CGU_PCRID_SSA1_MCI_PL180_MCLK
#define CGU_CLKID_UART0_VPB_CLK                 13
#define CGU_CLKID_UART1_VPB_CLK                 14
#define CGU_CLKID_SPI0_PCLK                     15
#define CGU_CLKID_SPI0_PCLK_GAT                 16
#define CGU_CLKID_SPI0_FIFO_PCLK                17
#define CGU_CLKID_SPI0_DUMMY_VPBCLK             18
#define CGU_CLKID_SPI1_PCLK                     19
#define CGU_CLKID_SPI1_PCLK_GAT                 20
#define CGU_CLKID_SPI1_FIFO_PCLK                21
#define CGU_CLKID_SPI1_DUMMY_VPBCLK             22
#define CGU_CLKID_ADSS_AHB_SLV_CLK              23
#define CGU_CLKID_ADSS_AHB_MST_CLK              24
#define CGU_CLKID_ADSS_SPD_SYSCLK               25
#define CGU_CLKID_LCD_INTERFACE_PCLK            26
#define CGU_CLKID_LCD_INTERFACE_LCDCLK          27
#define CGU_CLKID_SDMA_PCLK                     28
#define CGU_CLKID_SDMA_PCLK_GATED               29
#define CGU_CLKID_ARM926EJS_CORE_CLK            30
#define CGU_CLKID_ARM926EJS_BUSIF_CLK           31
#define CGU_CLKID_ARM926EJS_RETIME_CLK          32
#define CGU_CLKID_USB_AHB_CLK                   33
#define CGU_CLKID_USB_OTG_AHB_CLK               34
#define CGU_CLKID_PCCARD_MMBD_AHB_CLK           35
#define CGU_CLKID_PCCARD_MMIO_AHB_CLK           36
#define CGU_CLKID_GDMA_CLK                      37
#define CGU_CLKID_GDMA_CFG_CLK                  38
#define CGU_CLKID_TV_OUT_CLK                    39
#define CGU_CLKID_TV_OUT_CFG_CLK                40
#define CGU_CLKID_ISRAM0_CLK                    41
#define CGU_CLKID_ISRAM1_CLK                    42
#define CGU_CLKID_ISROM_CLK                     43
#define CGU_CLKID_AHB_MPMC_PL172_CFG_CLK        44
#define CGU_CLKID_AHB_MPMC_PL172_CFG_CLK2       45
#define CGU_CLKID_AHB_MPMC_PL172_CFG_CLK3       46
#define CGU_CLKID_AHB2AHB_CLK                   47
#define CGU_CLKID_MMIOINTC_CLK                  48

#define CGU_CLKID_AHB2VPB0_ASYNC_PCLK           49
#define CGU_CLKID_EVENT_ROUTER_PCLK             50
#define CGU_CLKID_RTC_PCLK                      51
#define CGU_CLKID_ADC_PCLK                      52
#define CGU_CLKID_ADC_CLK                       53
#define CGU_CLKID_WDOG_PCLK                     54
#define CGU_CLKID_IOCONF_PCLK                   55
#define CGU_CLKID_CGU_PCLK                      56
#define CGU_CLKID_SYSCREG_PCLK                  57

#define CGU_CLKID_AHB2VPB1_ASYNC_PCLK           58
#define CGU_CLKID_TIMER0_PCLK                   59
#define CGU_CLKID_TIMER1_PCLK                   60
#define CGU_CLKID_I2C0_PCLK                     61
#define CGU_CLKID_I2C1_PCLK                     62

#define CGU_CLKID_AHB2VPB3_ASYNC_PCLK           63
#define CGU_CLKID_ADSS_CONFIG_PCLK              64
#define CGU_CLKID_ADSS_DAI_PCLK                 65
#define CGU_CLKID_ADSS_DAI2_PCLK                66
#define CGU_CLKID_ADSS_DAO_PCLK                 67
#define CGU_CLKID_ADSS_DSS_PCLK                 68
#define CGU_CLKID_ADSS_SAI1_PCLK                69
#define CGU_CLKID_ADSS_SAI2_PCLK                70
#define CGU_CLKID_ADSS_SAI3_PCLK                71
#define CGU_CLKID_ADSS_SAI4_PCLK                72
#define CGU_CLKID_ADSS_SAO1_PCLK                73
#define CGU_CLKID_ADSS_SAO2_PCLK                74
#define CGU_CLKID_ADSS_SAO3_PCLK                75
#define CGU_CLKID_ADSS_SPDIF_PCLK               76
#define CGU_CLKID_ADSS_SPDIFO_PCLK              77
#define CGU_CLKID_ADSS_DAC_IPOL_PCLK            78
#define CGU_CLKID_ADSS_EDGE_DET_PCLK            79
#define CGU_CLKID_ADSS_ADC_DEC_PCLK             80
#define CGU_CLKID_ADSS_CACHE_PCLK               81

#define CGU_CLKID_IEEE1394_SCLK                 82
#define CGU_CLKID_IEEE1394_CLK_25               83

#define CGU_CLKID_IPINT0_CLK_IP                 84

#define CGU_CLKID_IPINT1_CLK_IP                 85

#define CGU_CLKID_RTC_CLK32K                    86

#define CGU_CLKID_MCI_PL180_MCIFBCLK            87

#define CGU_CLKID_UART0_U_CLK                   88

#define CGU_CLKID_UART1_U_CLK                   89

#define CGU_CLKID_ADSS_DAC_NS_DAC_CLK           90
#define CGU_CLKID_ADSS_DAC_DSPCLK               91
#define CGU_CLKID_ADSS_ADC_DECCLK               92
#define CGU_CLKID_ADSS_ADC_SYSCLK               93
#define CGU_CLKID_CR_DAI_BCK                    94
#define CGU_CLKID_CR_DAI_WS                     95
#define CGU_CLKID_CR_DAO_CLK                    96
#define CGU_CLKID_CR_DAO_WS                     97
#define CGU_CLKID_CR_DAO_BCK                    98
#define CGU_CLKID_ADSS_SPDIFO_CLK               99

#define CGU_CLKID_ADSS_DAI_BCK                  100

#define CGU_CLKID_ADSS_DAI_SPD_BCK              101

#define CGU_CLKID_PCCARD_CLK_62                 102
#define CGU_CLKID_PCCARD_CLK_187                103

#define CGU_CLKID_GDMA_CLK_62                   104

#define CGU_CLKID_TV_OUT_XCLK                   105

#define CGU_CLKID_CLKOUT0                       106

#define CGU_CLKID_CLKOUT1                       107


#define CLKID_2_ESRID(clkid)                    ((clkid) <= CGU_CLKID_IPINT1_CLK_IP         ? (clkid)   : \
                                                 (clkid) == CGU_CLKID_RTC_CLK32K            ? (-1)      : \
                                                 (clkid) == CGU_CLKID_MCI_PL180_MCIFBCLK    ? (-1)      : \
                                                 (clkid) <= CGU_CLKID_ADSS_SPDIFO_CLK       ? (clkid-2) : \
                                                 (clkid) == CGU_CLKID_ADSS_DAI_BCK          ? (-1)      : \
                                                 (clkid) == CGU_CLKID_ADSS_DAI_SPD_BCK      ? (-1)      : \
                                                                                              (clkid-4))


/****************************************************************************
****************************************************************************/

static unsigned int cgu_base_reference_get (unsigned int baseid)
{
    unsigned int scr;                                       /* config: bit0 == enable side1, bit1 == enable side2, bit2 == reset, bit3 == stop */
    unsigned int refid;

    scr = CGU->base_scr[baseid];

    if ((scr & (SCR_RESET | SCR_STOP)) != 0)
        return CGU_REFID_DISABLE;

    refid = CGU->base_fs[(scr & 0x01) ^ 0x01][baseid];      /* read refid from whichever side is currently selected */

    return refid;
}

static void cgu_base_reference_select (unsigned int baseid, unsigned int refid)
{
    int i;
    unsigned int scr;                                       /* config: bit0 == enable side1, bit1 == enable side2, bit2 == reset, bit3 == stop */

    scr = CGU->base_scr[baseid];
    scr &= ~SCR_RESET;                                      /* clear reset bit. N1C bootrom bug sets RESET bit when it should set STOP bit (both bits kill output clock) */

    if (refid == CGU_REFID_DISABLE) {
        CGU->base_scr[baseid] = (scr | SCR_STOP);
        return;
    }
    CGU->base_fs[scr & 0x01][baseid] = refid;               /* setup whichever side is not selected already */
    CGU->base_scr[baseid] = ((scr ^ 0x03) & ~SCR_STOP);     /* swap side selection and ensure that stop bit is clear */

    for (i = 0; i < 2048; i++)
        if ((CGU->base_scr[baseid] & 0x03) == (CGU->base_ssr[baseid] & 0x03))
            break;
}

static void cgu_base_fracdivs_control (unsigned int baseid, unsigned int bcr_value)
{
    if (baseid == CGU_BASEID_SYS)
        CGU->base_bcr[0] = bcr_value;
    if (baseid == CGU_BASEID_AHB0_VPB0)
        CGU->base_bcr[1] = bcr_value;
    if (baseid == CGU_BASEID_CLK1024FS)
        CGU->base_bcr[2] = bcr_value;
    if (baseid == CGU_BASEID_ATACLK)
        CGU->base_bcr[3] = bcr_value;
}

static void cgu_base_fracdivs_enable (unsigned int baseid)
{
    cgu_base_fracdivs_control (baseid, BCR_RUN);
}

static void cgu_base_fracdivs_disable (unsigned int baseid)
{
    cgu_base_fracdivs_control (baseid, 0);
}


/****************************************************************************
****************************************************************************/

static unsigned int gcd (unsigned int u, unsigned int v)
{
    unsigned int k = 0;

    if (u == 0)
        return v;
    if (v == 0)
        return u;

    while (((u | v) & 0x01) == 0) {
        u >>= 1;
        v >>= 1;
        k++;
    }

    do {
             if ((u & 0x01) == 0) u = u >> 1;
        else if ((v & 0x01) == 0) v = v >> 1;
        else if (u >= v)          u = (u - v) >> 1;
        else                      v = (v - u) >> 1;
    }
    while (u > 0);

    return (v << k);
}

static void cgu_dyn_fracdiv_allow (unsigned int fdid)
{
    if ((fdid < CGU_FDID_SYS_DYN_FD0) && (fdid > CGU_FDID_SYS_DYN_FD1)) {
        printk ("%s: illegal fdid %d\n", __FUNCTION__, fdid);
        return;
    }

    fdid -= CGU_FDID_SYS_DYN_FD0;                           /* rebase fdid before first register access... */
    CGU->base_dyn_fdc[fdid] |= DYN_FDC_ALLOW;               /* enable fracdiv with new settings */
}

static void cgu_dyn_fracdiv_set_ratio (unsigned int fdid, unsigned int in, unsigned int out)
{
    int madd, msub;
    unsigned int temp, config;
    unsigned int fdwidth, fdwidth_mask;

    if ((fdid < CGU_FDID_SYS_DYN_FD0) && (fdid > CGU_FDID_SYS_DYN_FD1)) {
        printk ("%s: illegal fdid %d\n", __FUNCTION__, fdid);
        return;
    }

    temp = gcd (in, out);                                   /* reduce... */
    in /= temp;
    out /= temp;

    madd = (in - out);
    msub = out;                                             /* defer making msub negative until after expansion */

    fdwidth = (fdid == CGU_FDID_CLK1024FS_FD4) ? 10 : 8;    /* every fracdiv apart from one has 8bit config settings... */
    fdwidth_mask = (1 << fdwidth) - 1;

    while (((madd | msub) & ~(fdwidth_mask >> 1)) == 0) {   /* expand to fit available width */
        madd <<= 1;                                         /* not strictly necessary but apparently good for power consumption... */
        msub <<= 1;
    }

    config = DYN_FDC_RUN | ((((0 - msub) & fdwidth_mask) << (3 + fdwidth)) | ((madd & fdwidth_mask) << 3));

    if (out <= (in / 2))                                    /* Stretch does a good job of evening out the duty cycle... */
        config |= DYN_FDC_STRETCH;                          /* but it can only be used if input:output ratio is >= 2:1 !!! */

    printk ("fdid: %2d, gcd: %3d, in/out: %3d/%d, madd: 0x%02x, msub: 0x%02x %s\n", fdid, temp, in, out, madd, ((0 - msub) & 0xFF), (config & DYN_FDC_STRETCH) ? "(stretch enabled)" : "");

    fdid -= CGU_FDID_SYS_DYN_FD0;                           /* rebase fdid before first register access... */

    CGU->base_dyn_fdc[fdid] = config;                       /* enable fracdiv with new settings */
}

static void cgu_fracdiv_set_ratio (unsigned int fdid, unsigned int in, unsigned int out)
{
    int madd, msub;
    unsigned int temp, config;
    unsigned int fdwidth, fdwidth_mask;

    if ((fdid >= CGU_FDID_SYS_DYN_FD0) && (fdid <= CGU_FDID_SYS_DYN_FD1)) {
        cgu_dyn_fracdiv_set_ratio (fdid, in, out);
        return;
    }

    if ((fdid < CGU_FDID_SYS_FD0) && (fdid > CGU_FDID_CLKOUT1_FD0)) {
        printk ("%s: illegal fdid %d\n", __FUNCTION__, fdid);
        return;
    }

    temp = gcd (in, out);                                   /* reduce... */
    in /= temp;
    out /= temp;

    madd = (in - out);
    msub = out;                                             /* defer making msub negative until after expansion */

    fdwidth = (fdid == CGU_FDID_CLK1024FS_FD4) ? 10 : 8;    /* every fracdiv apart from one has 8bit config settings... */
    fdwidth_mask = (1 << fdwidth) - 1;

    while (((madd | msub) & ~(fdwidth_mask >> 1)) == 0) {   /* expand to fit available width */
        madd <<= 1;                                         /* not strictly necessary but apparently good for power consumption... */
        msub <<= 1;
    }

    config = ((((0 - msub) & fdwidth_mask) << (3 + fdwidth)) | ((madd & fdwidth_mask) << 3));

    if (in != out)
        config |= FDC_RUN;

    if (out <= (in / 2))                                    /* Stretch does a good job of evening out the duty cycle... */
        config |= FDC_STRETCH;                              /* but it can only be used if input:output ratio is >= 2:1 !!! */

    printk ("fdid: %2d, gcd: %3d, in/out: %3d/%d, madd: 0x%02x, msub: 0x%02x %s\n", fdid, temp, in, out, madd, ((0 - msub) & 0xFF), (config & FDC_STRETCH) ? "(stretch enabled)" : "");

    CGU->base_fdc[fdid] = FDC_RESET;
    (void) CGU->base_fdc[fdid];                             /* read back to ensure reset write really has reached the fracdiv... is this necessary ?!? */

    CGU->base_fdc[fdid] = config;                           /* enable fracdiv with new settings */
}

static void cgu_fracdiv_disable (unsigned int fdid)
{
    CGU->base_fdc[fdid] = 0;
}


/****************************************************************************
****************************************************************************/

#define cgu_clock_enable(clkid)     CGU->clk_pcr[(clkid)] = (PCR_WAKE_EN | PCR_AUTO | PCR_RUN)
#define cgu_clock_disable(clkid)    CGU->clk_pcr[(clkid)] = (PCR_WAKE_EN | PCR_AUTO)


/****************************************************************************
****************************************************************************/

static int lppll_set_speed_mhz (unsigned int id, int mhz)
{
    int i;
    unsigned int m;
    unsigned int ffast_mhz;
    LPPLL_t *pll;

    if (id > 1)
        return -1;

    pll = &LPPLL->pll[id];
    pll->pwd = 1;                           /* power down PLL */

    if (mhz == 0)
        return 0;

    ffast_mhz = ((ffasthz + 500000) / 1000000);

    for (m = 1; m <= 25; m++)
        if ((mhz >= (m * ffast_mhz)) && (mhz < ((m + 1) * ffast_mhz)))
            break;

    pll->fin_select = 0x01;                 /* 0x01 == hardcode to FFAST... */
    pll->bypass = 0;                        /* disable bypass */

    pll->direct = 0;
    pll->msel = (m - 1);
    pll->psel = 0;

#if 0
    bl_printf ("lppll: direct: %d, msel: %2d, psel: %d\n", pll->direct, pll->msel, pll->psel);
#endif

    pll->pwd = 0;                           /* power up PLL */

    for (i = 0; i < 4096; i++)
        if ((pll->lock & 0x01) != 0)
            break;

    return 0;
}

static int lppll_power_down (unsigned int id)
{
    return lppll_set_speed_mhz (id, 0);
}

static int hppll_power_down (unsigned int id)
{
    if (id != 0)
        return -1;

    HPPLL->mode = HPPLL_MODE_PD;            /* power down PLL */
    return 0;
}


/****************************************************************************
****************************************************************************/

static void twiddle_sdram_refresh_rate (void)
{
    unsigned int regval;
    unsigned int regval_new;
    unsigned int hsclocks;
    unsigned int ratio;

    regval = readl (VA_SYSCREG_MPMC_TESTMODE0);

    if ((regval & (1 << 12)) == 0) {
        printk ("Huh ?!? SDRAM external refresh is not enabled... ?!?\n");
    }

    regval &= ((1 << 12) - 1);

#if 0

    /*
       SDRAM refresh is currently running from CPU clock (ie CPU PLL
       with no divider...). It will soon be running from FFAST (ie
       crystal input with no dividers).

       To compensate, we need to reduce the clocks per refresh by the
       the ratio of these two clocks...
    */

    ratio = 1;
    while (1) {
        if (((cpuclkhz + ratio - 1) / ratio) <= ffasthz)
            break;
        if (++ratio == 32)          /* 32 is arbitrary, but covers expected range (e.g. 6 * 32 == 192) */
            break;
    }

    regval_new = (regval / ratio);

    if (regval_new == 0)            /* filter out invalid settings... */
        regval_new = 1;

#else

    /*
       In reality, we lose too much to rounding errors etc.
       Run the refresh as fast as possible...
    */

    regval_new = 1;                                 /* */

#endif

    hsclocks = 32;                                  /* this is very critical... */
    writel (hsclocks, VA_SYSCREG_MPMC_TESTMODE1);

    printk ("External_refresh: %d -> %d (%d high speed clocks)\n", regval, regval_new, hsclocks);
    writel ((regval_new | (1 << 12)), VA_SYSCREG_MPMC_TESTMODE0);
}


#define DEFDIV      128


void clocks_enter_low_power_state (void)
{
    int i;

    twiddle_sdram_refresh_rate();

#if 1

    /*
       Shuffle the SYS base clock to an intermediate speed and disable fracdivs
       before moving it to FFAST. Fixme: We assume that SYS and PCCARD bases are
       both running from known PLLs... Bad things will happen if these hardcoded
       assumptions are wrong !!
    */

    for (i = 0; i < CGU_NR_BASE; i++) {
        if (cgu_base_reference_get (i) == CGU_REFID_LP1_FOUT) {
            printk ("Base %2d to DISABLE...\n", i);
            cgu_base_reference_select (i, CGU_REFID_DISABLE);           /* disable all bases running from 'PCCARD' PLL (should be PCCARD base only !!) */
        }
    }

    lppll_set_speed_mhz (1, 36);                                        /* set new speed for former PCCARD PLL */

    cgu_base_reference_select (CGU_BASEID_SYS, CGU_REFID_LP1_FOUT);     /* move SYS base over to safe intermediate speed before touching fracdivs... */
    cgu_base_fracdivs_disable (CGU_BASEID_SYS);                         /* force disable all SYS base fracdivs */

    for (i = CGU_FDID_SYS_FD0; i <= CGU_FDID_SYS_FD5; i++)
        cgu_fracdiv_disable (i);                                        /* not really necessary as fracdivs have been force disabled at the base level... */

#endif

    for (i = CGU_BASEID_AHB0_VPB0; i < CGU_NR_BASE; i++) {
        if (i == CGU_BASEID_CLK_RTC)                            /* don't touch RTC base here... */
            continue;
        if (cgu_base_reference_get (i) == CGU_REFID_DISABLE)    /* don't re-enable bases which are already disabled */
            continue;
        printk ("Base %2d to FFAST...\n", i);
        cgu_base_reference_select (i, CGU_REFID_FFAST);         /* bypass all PLLs (switch each base to 12 MHz xtal input) */
        cgu_base_fracdivs_disable (i);                          /* for bases with more than one fracdiv, force disable all of them */
    }

    i = CGU_BASEID_SYS;                                         /* Once all other bases have been switched to FFAST, do the same for SYS base... */
    printk ("Base %2d to FFAST...\n", i);
    cgu_base_reference_select (i, CGU_REFID_FFAST);             /* */
    cgu_base_fracdivs_disable (i);                              /* */

    for (i = 0; i < CGU_NR_CLK_ESR; i++)                        /* clear any fracdiv selections previously made for each individual clock */
        CGU->clk_esr[i] = 0;

    for (i = 0; i < CGU_NR_FD; i++)                             /* disable all fracdivs individually */
        cgu_fracdiv_disable (i);

    hppll_power_down (0);
    lppll_power_down (0);
    lppll_power_down (1);

    writel (0x01, VA_SYSCREG_USB_ATX_PLL_PD_REG);               /* power down USB PLL */

    printk ("PLLs powered down...\n");


    /*************************************************************************

        AHB0_VPB0 Base

    *************************************************************************/

    cgu_fracdiv_set_ratio (CGU_FDID_AHB0_VPB0_FD0, DEFDIV, 1);                      /* 12 MHz / 128 == 94 kHz, etc */

    for (i = CGU_CLKID_AHB2VPB0_ASYNC_PCLK; i <= CGU_CLKID_SYSCREG_PCLK; i++) {     /* for all AHB0_VPB0 base clocks... */
        CGU->clk_esr[CLKID_2_ESRID(i)] = (0 << 1) | ESR_ENABLE;                     /* select and enable first fracdiv for this base */
        cgu_clock_enable (i);                                                       /* enable the clock... */
    }


    /*************************************************************************

        AHB0_VPB1 Base

    *************************************************************************/

    cgu_base_reference_select (CGU_BASEID_AHB0_VPB1, CGU_REFID_DISABLE);


    /*************************************************************************

        AHB0_VPB3 Base

        Fixme... it might be nice to completely stop the clock to this base.
        However, it's an AHB bus master and we don't know what state it's in
        (ie could be accessing SDRAM ??) therefore stopping at the wrong
        moment would hang the system...

    *************************************************************************/

    cgu_fracdiv_set_ratio (CGU_FDID_AHB0_VPB3_FD0, DEFDIV, 1);                      /* 12 MHz / 128 == 94 kHz, etc */

    for (i = CGU_CLKID_AHB2VPB3_ASYNC_PCLK; i <= CGU_CLKID_ADSS_CACHE_PCLK; i++) {  /* for all AHB0_VPB0 base clocks... */
        CGU->clk_esr[CLKID_2_ESRID(i)] = (0 << 1) | ESR_ENABLE;                     /* select and enable first fracdiv for this base */
//      cgu_clock_enable (i);                                                       /* enable the clock... */
    }

//  cgu_clock_disable (CGU_CLKID_AHB2VPB3_ASYNC_PCLK    );
    cgu_clock_disable (CGU_CLKID_ADSS_CONFIG_PCLK       );
    cgu_clock_disable (CGU_CLKID_ADSS_DAI_PCLK          );
    cgu_clock_disable (CGU_CLKID_ADSS_DAI2_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_DAO_PCLK          );
    cgu_clock_disable (CGU_CLKID_ADSS_DSS_PCLK          );
    cgu_clock_disable (CGU_CLKID_ADSS_SAI1_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAI2_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAI3_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAI4_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAO1_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAO2_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SAO3_PCLK         );
    cgu_clock_disable (CGU_CLKID_ADSS_SPDIF_PCLK        );
    cgu_clock_disable (CGU_CLKID_ADSS_SPDIFO_PCLK       );
    cgu_clock_disable (CGU_CLKID_ADSS_DAC_IPOL_PCLK     );
    cgu_clock_disable (CGU_CLKID_ADSS_EDGE_DET_PCLK     );
    cgu_clock_disable (CGU_CLKID_ADSS_ADC_DEC_PCLK      );
//  cgu_clock_disable (CGU_CLKID_ADSS_CACHE_PCLK        );


    /*************************************************************************

        Misc...

    *************************************************************************/

#if 0
    cgu_base_reference_select (CGU_BASEID_CLK_RTC,     CGU_REFID_DISABLE);
    CGU_CONFIG->f32khz_on = 0;                                              /* stop the 32khz oscillator... */
#endif

//  cgu_base_reference_select (CGU_BASEID_UARTCLK0,    CGU_REFID_DISABLE);

    cgu_base_reference_select (CGU_BASEID_UARTCLK1,    CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_CLK1024FS,   CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_ATACLK,      CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_CLKOUT0,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_CLKOUT1,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_PHY1394,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_CCP_IPINT0,  CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_CCP_IPINT1,  CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_MCI_CLK,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_DAI_BCK,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_SPD3_BCK,    CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_GDMACLK,     CGU_REFID_DISABLE);
    cgu_base_reference_select (CGU_BASEID_TV_OUT_XCLK, CGU_REFID_DISABLE);


    /*************************************************************************

        SYS Base

        - we should really use CLKID_2_ESRID() macro when accessing esr
          regs, but we cheat for efficiency... (all SYS base CLKIDs map
          directly to ESRIDs).

    *************************************************************************/

    for (i = CGU_CLKID_VPB0_CLK; i <= CGU_CLKID_MMIOINTC_CLK; i++) {        /* for all SYS base clocks... */
        CGU->clk_esr[/*CLKID_2_ESRID(*/i/*)*/] = (0 << 1) | ESR_ENABLE;     /* select and enable fracdiv 0 (doesn't take effect yet due to BCR) */
        cgu_clock_enable (i);                                               /* enable the clock... */
    }

    CGU->clk_pcr[CGU_CLKID_ARM926EJS_BUSIF_CLK] |= PCR_ENOUT_EN;            /* seems to be vital... not sure why (N1C bootrom does this by default, N1B doesn't) */
    CGU->clk_pcr[CGU_CLKID_AHB_MPMC_PL172_CFG_CLK] |= PCR_ENOUT_EN;         /* ??? (the clock for the logic which generates the external fresh counter for SDRAM ???) */

#if 0
    CGU->clk_esr[CGU_CLKID_ARM926EJS_CORE_CLK] = (1 << 1) | ESR_ENABLE;         /* to fracdiv 1 and enable (doesn't take effect yet due to BCR) */
    CGU->clk_esr[CGU_CLKID_ARM926EJS_RETIME_CLK] = (1 << 1) | ESR_ENABLE;       /* to fracdiv 1 and enable (doesn't take effect yet due to BCR) */
    CGU->clk_esr[CGU_CLKID_AHB_MPMC_PL172_CFG_CLK3] = (1 << 1) | ESR_ENABLE;    /* to fracdiv 1 and enable (doesn't take effect yet due to BCR) */
#else
    CGU->clk_esr[CGU_CLKID_ARM926EJS_CORE_CLK] = (1 << 1) | ESR_ENABLE;         /* to fracdiv 1 and enable (doesn't take effect yet due to BCR) */
    CGU->clk_esr[CGU_CLKID_ARM926EJS_RETIME_CLK] = 0;                           /* 0 == bypass fracdivs */
    CGU->clk_esr[CGU_CLKID_AHB_MPMC_PL172_CFG_CLK3] = 0;                        /* 0 == bypass fracdivs */
#endif

//  cgu_clock_disable (CGU_CLKID_VPB0_CLK);
    cgu_clock_disable (CGU_CLKID_VPB1_CLK);
//  cgu_clock_disable (CGU_CLKID_VPB2_CLK);
//  cgu_clock_disable (CGU_CLKID_VPB3_CLK);
    cgu_clock_disable (CGU_CLKID_AHB2MMIO_CLK);
//  cgu_clock_disable (CGU_CLKID_AHB0_CLK);
//  cgu_clock_disable (CGU_CLKID_AHB1_CLK);
    cgu_clock_disable (CGU_CLKID_CCP_IPINT0_PCLK);
    cgu_clock_disable (CGU_CLKID_CCP_IPINT0_VPB_PCLK);
    cgu_clock_disable (CGU_CLKID_CCP_IPINT1_PCLK);
    cgu_clock_disable (CGU_CLKID_CCP_IPINT1_VPB_PCLK);
    cgu_clock_disable (CGU_CLKID_SSA1_MCI_PL180_PCLK);
    cgu_clock_disable (CGU_CLKID_SSA1_MCI_PL180_MCLK);
//  cgu_clock_disable (CGU_CLKID_UART0_VPB_CLK);
    cgu_clock_disable (CGU_CLKID_UART1_VPB_CLK);

#if defined (CONFIG_SERIAL_SC16IS7X0) && (IO_ADDRESS_SPISD_BASE == IO_ADDRESS_SPI0_BASE)
    /* leave SPI_0 clocks running... */
#else
    cgu_clock_disable (CGU_CLKID_SPI0_PCLK);
    cgu_clock_disable (CGU_CLKID_SPI0_PCLK_GAT);
    cgu_clock_disable (CGU_CLKID_SPI0_FIFO_PCLK);
    cgu_clock_disable (CGU_CLKID_SPI0_DUMMY_VPBCLK);
#endif

#if defined (CONFIG_SERIAL_SC16IS7X0) && (IO_ADDRESS_SPISD_BASE == IO_ADDRESS_SPI1_BASE)
    /* leave SPI_1 clocks running... */
#else
    cgu_clock_disable (CGU_CLKID_SPI1_PCLK);
    cgu_clock_disable (CGU_CLKID_SPI1_PCLK_GAT);
    cgu_clock_disable (CGU_CLKID_SPI1_FIFO_PCLK);
    cgu_clock_disable (CGU_CLKID_SPI1_DUMMY_VPBCLK);
#endif

//  cgu_clock_disable (CGU_CLKID_ADSS_AHB_SLV_CLK);
//  cgu_clock_disable (CGU_CLKID_ADSS_AHB_MST_CLK);
//  cgu_clock_disable (CGU_CLKID_ADSS_SPD_SYSCLK);
    cgu_clock_disable (CGU_CLKID_LCD_INTERFACE_PCLK);
    cgu_clock_disable (CGU_CLKID_LCD_INTERFACE_LCDCLK);
    cgu_clock_disable (CGU_CLKID_SDMA_PCLK);
    cgu_clock_disable (CGU_CLKID_SDMA_PCLK_GATED);
//  cgu_clock_disable (CGU_CLKID_ARM926EJS_CORE_CLK);
//  cgu_clock_disable (CGU_CLKID_ARM926EJS_BUSIF_CLK);
//  cgu_clock_disable (CGU_CLKID_ARM926EJS_RETIME_CLK);
    cgu_clock_disable (CGU_CLKID_USB_AHB_CLK);
    cgu_clock_disable (CGU_CLKID_USB_OTG_AHB_CLK);
    cgu_clock_disable (CGU_CLKID_PCCARD_MMBD_AHB_CLK);
    cgu_clock_disable (CGU_CLKID_PCCARD_MMIO_AHB_CLK);
    cgu_clock_disable (CGU_CLKID_GDMA_CLK);
    cgu_clock_disable (CGU_CLKID_GDMA_CFG_CLK);
    cgu_clock_disable (CGU_CLKID_TV_OUT_CLK);
    cgu_clock_disable (CGU_CLKID_TV_OUT_CFG_CLK);
    cgu_clock_disable (CGU_CLKID_ISRAM0_CLK);
    cgu_clock_disable (CGU_CLKID_ISRAM1_CLK);
    cgu_clock_disable (CGU_CLKID_ISROM_CLK);
//  cgu_clock_disable (CGU_CLKID_AHB_MPMC_PL172_CFG_CLK);
//  cgu_clock_disable (CGU_CLKID_AHB_MPMC_PL172_CFG_CLK2);
//  cgu_clock_disable (CGU_CLKID_AHB_MPMC_PL172_CFG_CLK3);
//  cgu_clock_disable (CGU_CLKID_AHB2AHB_CLK);
    cgu_clock_disable (CGU_CLKID_MMIOINTC_CLK);

    cgu_fracdiv_set_ratio (CGU_FDID_SYS_FD0, 2, 1);                         /* SYS base fracdiv 0 : AHB (ahb_fd_ratio:1 integer fraction of CPU) */
    cgu_fracdiv_set_ratio (CGU_FDID_SYS_FD1, 2, 1);                         /* SYS base fracdiv 1 : CPU (           1:1 integer fraction of CPU ;-) */

//  CGU->base_dyn_sel[0] = (DYN_SEL_ARM926_I | DYN_SEL_ARM926_D | DYN_SEL_EXT_REFRESH);
    CGU->base_dyn_sel[0] = (DYN_SEL_EXT_REFRESH);

    CGU->base_dyn_sel[1] = CGU->base_dyn_sel[0];
//  CGU->base_dyn_sel[1] = 0;

    cgu_dyn_fracdiv_set_ratio (CGU_FDID_SYS_DYN_FD0, DEFDIV, 1);            /* SYS base dyn fracdiv 0 : slow speed setting for SYS base fracdiv 0 */
    cgu_dyn_fracdiv_allow (CGU_FDID_SYS_DYN_FD0);

    cgu_dyn_fracdiv_set_ratio (CGU_FDID_SYS_DYN_FD1, 2, 1);                 /* SYS base dyn fracdiv 1 : SHOULD MATCH SETTINGS FOR SYS base fracdiv 1 !! */
    cgu_dyn_fracdiv_allow (CGU_FDID_SYS_DYN_FD1);

    cgu_base_fracdivs_enable (CGU_BASEID_SYS);                              /* re-enable all SYS base fracdivs */
}

