/*
 * linux/arch/arm/mach-pnx0106/blas_spi.c
 *
 * Copyright (C) 2004-2005 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/config.h>
#include <linux/version.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/errno.h>

#include <asm/arch/gpio.h>
#include <asm/arch/spi.h>
#include <asm/arch/tsc.h>


/*****************************************************************************
*****************************************************************************/

#if 0
#define DEBUG_LEVEL             (1)
#else

static unsigned int DEBUG_LEVEL = 1;

void ssa_spi_set_debug_level (unsigned int new)
{
    DEBUG_LEVEL = new;
}

EXPORT_SYMBOL(ssa_spi_set_debug_level);

#endif


/*****************************************************************************
*****************************************************************************/

#define CLK_DIVIDER             (0)
#define FIFO_DEPTH_WORDS16      (64)

#define NCS                     (GPIO_SPI_NCS)

/*
    For a truly generic SPI driver, SPI mode should probably be runtime
    configurable (e.g. via a parameter passed to spi_init()).
    However, for now we just fix at compile time to match the
    requirements of the SPI target device in use (ie 0 or 3 in the case
    of SPI NOR Flash or BGW2xx WiFi devices).
*/

#define SPI_MODE                (3)

/*
    Workaround for fifo read HW bug.
    There are two possible options depending on whether interrupts are enabled
    while the fifo based SPI read transfer is taking place. The version which
    is safe in the face of interrupts is safe in all cases, but slightly lower
    performance.
*/

#if 0
#define FIFO_BACKOFF_WORDS      6                   /* less than FIFO_DEPTH_WORDS16 is only safe if interrupts are disabled */
#else
#define FIFO_BACKOFF_WORDS      FIFO_DEPTH_WORDS16  /* FIFO_DEPTH_WORDS16 should be 100% safe in all conditions, but slightly lower performance */
#endif


/*****************************************************************************
*****************************************************************************/

#define VA_SPI_GLOBAL                   (IO_ADDRESS_SPI_BASE + 0x000)
#define VA_SPI_CON                      (IO_ADDRESS_SPI_BASE + 0x004)
#define VA_SPI_FRM                      (IO_ADDRESS_SPI_BASE + 0x008)
#define VA_SPI_IER                      (IO_ADDRESS_SPI_BASE + 0x00C)
#define VA_SPI_STAT                     (IO_ADDRESS_SPI_BASE + 0x010)
#define VA_SPI_DAT                      (IO_ADDRESS_SPI_BASE + 0x014)

#define GLOBAL_ENABLE                   ( 1 <<  0)
#define GLOBAL_RESET                    ( 1 <<  1)

#define STAT_FIFO_EMPTY                 ( 1 <<  0)
#define STAT_FIFO_THRESHOLD_REACHED     ( 1 <<  1)
#define STAT_FIFO_FULL                  ( 1 <<  2)
#define STAT_ACTIVE                     ( 1 <<  3)
#define STAT_EOT                        ( 1 <<  7)

#define CON_RATE               (CLK_DIVIDER <<  0)
#define CON_MS                          ( 1 <<  7)
#define CON_CS_EN                       ( 1 <<  8)
#define CON_BITNUM_8                    ( 7 <<  9)
#define CON_BITNUM_16                   (15 <<  9)
#define CON_SHIFT_OFF                   ( 1 << 13)
#define CON_THR                         ( 1 << 14)
#define CON_RX                          ( 0 << 15)
#define CON_TX                          ( 1 << 15)
#define CON_MODE                  (SPI_MODE << 16)
#define CON_BIDIR                       ( 1 << 23)

#define CON_TX_SINGLE                   (CON_RATE | CON_MS | CON_BITNUM_8  | CON_TX | CON_MODE | CON_BIDIR)
#define CON_RX_SINGLE                   (CON_RATE | CON_MS | CON_BITNUM_8  | CON_RX | CON_MODE | CON_BIDIR)
#define CON_TX_VIA_FIFO                 (CON_RATE | CON_MS | CON_BITNUM_16 | CON_TX | CON_MODE | CON_BIDIR)
#define CON_RX_VIA_FIFO                 (CON_RATE | CON_MS | CON_BITNUM_16 | CON_RX | CON_MODE | CON_BIDIR)


#define MAX_FRM_BYTE_COUNT              (2 * 0xFFFF)


/*****************************************************************************
*****************************************************************************/

/*
    Note: Different types of device require different nCS handling.
    Currently this driver is written for BGW2xx support (which is
    relatively easy as nCS has no special meaning - ie it can be
    deasserted at any byte boundary).
    SPINOR is more tricky as nCS is used to frame complete command
    sequences. ie nCS handling needs to be more generic if SPINOR
    devices etc need to be handled.
*/

#if 0
#define DRIVE_NCS_LOW()         { unsigned long zq_flags; local_irq_save (zq_flags); gpio_set_low (NCS)
#define DRIVE_NCS_HIGH()        gpio_set_high (NCS); local_irq_restore (zq_flags); }
#else
#define DRIVE_NCS_LOW()         gpio_set_low (NCS)
#define DRIVE_NCS_HIGH()        gpio_set_high (NCS)
#endif


/*****************************************************************************
*****************************************************************************/

void ssa_spi_reset_drive_low (void)                 /* for BGW2xx drive low will assert */
{
    if (DEBUG_LEVEL > 1)
        printk ("%s\n", __FUNCTION__);

    gpio_set_as_output (GPIO_WLAN_RESET, 0);
}

void ssa_spi_reset_drive_high (void)                /* for BGW2xx drive high will de-assert */
{
    if (DEBUG_LEVEL > 1)
        printk ("%s\n", __FUNCTION__);

    gpio_set_as_output (GPIO_WLAN_RESET, 1);
}

/*
   Return the interrupt number which the driver should use when
   registering an interrupt handler for the SPI target device.
*/
int ssa_spi_get_irq (void)
{
    if (DEBUG_LEVEL > 1)
        printk ("%s: %d\n", __FUNCTION__, IRQ_WLAN);

    return IRQ_WLAN;
}

/*
   Perform HW init etc.
   Called during kernel startup.
   May also be called by users of the SPI interface for last resort disaster recovery etc...
*/
int ssa_spi_init (void)
{
    if (DEBUG_LEVEL > 1)
        printk ("%s\n", __FUNCTION__);

    gpio_set_as_output (NCS, 1);                /* default to high (ie inactive) */

    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE | GLOBAL_RESET), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);

//  SPI->CON = ???                              /* reset defaults should be fine ?? */
//  SPI->FRM = 0;                               /* 0 is the reset default (single transfer) */
//  SPI->IER = 0;                               /* 0 is the reset default (disable all interrupts) */

    /*
        Fixme...
        This is really be the job of the WiFi drive startup code.
        Adding it here is temporary hack.
    */
    ssa_spi_reset_drive_high();                 /* drive high to de-assert reset to BGW2xx */
    udelay (200);                               /* BGW211 appnote specifies 125 usec between reset de-assertion and first access */

    return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
    Private bulk data input routine using fifo mode
    NB: 'count' is the number of 16bit words.
*/
static int _spi_fifo_read (unsigned short *dst, unsigned int count)
{
    int i;
    unsigned short *dst_end = &dst[count];
    unsigned int config = CON_RX_VIA_FIFO;
    unsigned int temp;
    unsigned int remaining;

    if (count == 0)
        return 0;

    writel ((GLOBAL_ENABLE | GLOBAL_RESET), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);

    writel (config, VA_SPI_CON);
    writel (count, VA_SPI_FRM);

    (void) readl (VA_SPI_DAT);      /* single dummy read from fifo kicks off external clock cycles */

    /*
        The following code should work around the SPI HW bugs as long as the
        CPU can reliably keep the SPI fifo empty faster than the serial data
        can be received. Should be true in all but the most extreme cases of
        slow CPU and fast SPI...

        Let the races begin... !!
    */

    while ((((unsigned long) dst_end) - ((unsigned long) dst)) > (FIFO_BACKOFF_WORDS * 2)) {
        while (readl (VA_SPI_STAT) & STAT_FIFO_EMPTY) { }                   /* wait for data to arrive in fifo (ie fifo to become non-empty) */
        temp = readl (VA_SPI_DAT);                                          /* external interface MUST still be active when we read from fifo here... !! */
        temp = (((temp & 0xFF00) >> 8) | ((temp & 0x00FF) << 8));
        *dst++ = temp;
    }

    while (readl (VA_SPI_FRM) != 0) { }                                     /* wait for transfer to complete (ie all data to have been received into fifo) */
    writel ((config | CON_SHIFT_OFF), VA_SPI_CON);                          /* disable shift clock output before we read from fifo */

    remaining = (count < FIFO_BACKOFF_WORDS) ? count : FIFO_BACKOFF_WORDS;  /* */
    for (i = 0; i < remaining; i++) {
        temp = readl (VA_SPI_DAT);
        temp = (((temp & 0xFF00) >> 8) | ((temp & 0x00FF) << 8));
        *dst++ = temp;
    }

    return 0;
}

/*
   Perform a single low level 8bit SPI read
*/
static unsigned int _ssa_spi_read_single8 (void)
{
    unsigned int temp;
    unsigned int config = CON_RX_SINGLE;

    writel ((GLOBAL_ENABLE | GLOBAL_RESET), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);

    writel (config, VA_SPI_CON);
//  writel (0, VA_SPI_FRM);                                 /* 0 is the reset default (single transfer) */
    writel (0, VA_SPI_DAT);                                 /* write dummy byte to fifo to generate external clock cycles */

    while (readl (VA_SPI_STAT) & STAT_FIFO_EMPTY) { }       /* wait for data to arrive in fifo (ie fifo to become non-empty) */
    while (readl (VA_SPI_STAT) & STAT_ACTIVE) { }           /* wait for shift in/out to complete (ie status to become inactive) */

    writel ((config | CON_SHIFT_OFF), VA_SPI_CON);          /* disable shift clock output before we read from fifo */

    temp = (readl (VA_SPI_DAT) & 0xFF);                     /* read data byte (masking _does_ seem to be required) */

    if (DEBUG_LEVEL > 3)
        printk ("spird8 : 0x%02x\n", temp);

    return temp;
}

unsigned int ssa_spi_read_single8 (void)
{
    unsigned int result;

    DRIVE_NCS_LOW();
    result = _ssa_spi_read_single8();
    DRIVE_NCS_HIGH();

    return result;
}

/*
   Read 'count' bytes from SPI into 'dst' buffer.
*/
unsigned int ssa_spi_read_multi (unsigned char *dst, unsigned int count)
{
    unsigned char *dst_end = &dst[count];
    unsigned int bytes_remaining;
    unsigned int bytes_xfer;
    unsigned int words_xfer;        /* word == 16bits in this case */

    if (DEBUG_LEVEL > 2)
        printk ("spirdm : dst:0x%08lx, count:%4d\n", (unsigned long) dst, count);

    DRIVE_NCS_LOW();

#if 1

    if ((((unsigned long) dst) & 0x01) && (dst < dst_end))
        *dst++ = _ssa_spi_read_single8();

    while ((bytes_remaining = (dst_end - dst)) >= 2) {
        bytes_xfer = ((bytes_remaining < MAX_FRM_BYTE_COUNT) ? bytes_remaining : MAX_FRM_BYTE_COUNT);
        words_xfer = (bytes_xfer / 2);
        _spi_fifo_read ((unsigned short *) dst, words_xfer);
        dst += (words_xfer * 2);
    }

    if (dst < dst_end)
        *dst++ = _ssa_spi_read_single8();

#else

    while (dst < dst_end)
        *dst++ = _ssa_spi_read_single8();

#endif

    DRIVE_NCS_HIGH();

    return 0;
}

/*
    Perform a single low level 16bit SPI read
    Data read from interface is assumed to be in big endian format.
*/
unsigned int ssa_spi_read_single16_be (void)
{
    unsigned int lo, hi;
    unsigned int result;

    DRIVE_NCS_LOW();
    hi = _ssa_spi_read_single8();
    lo = _ssa_spi_read_single8();
    DRIVE_NCS_HIGH();

    if (DEBUG_LEVEL > 1)
        printk ("spird16 : 0x%02x 0x%02x\n", hi, lo);

    result = (hi << 8) | lo;
    return result;
}

/*
    Perform a single low level 16bit SPI read
    Data read from interface is assumed to be in little endian format.
    (alias for old ssa_spi_read_single16 function on little endian platforms).
*/
unsigned int ssa_spi_read_single16_le (void)
{
    unsigned int lo, hi;
    unsigned int result;

    DRIVE_NCS_LOW();
    lo = _ssa_spi_read_single8();
    hi = _ssa_spi_read_single8();
    DRIVE_NCS_HIGH();

    if (DEBUG_LEVEL > 1)
        printk ("spird16 : 0x%02x 0x%02x\n", lo, hi);

    result = (hi << 8) | lo;
    return result;
}


/*******************************************************************************
*******************************************************************************/

/*
    Private bulk data output routine using fifo mode
    NB: 'count' is the number of 16bit words.
*/
static int _spi_fifo_write (unsigned short *src, unsigned int count)
{
    unsigned short *src_end = &src[count];
    unsigned int config = CON_TX_VIA_FIFO;

    writel ((GLOBAL_ENABLE | GLOBAL_RESET), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);

    writel (config, VA_SPI_CON);
    writel (count, VA_SPI_FRM);

    while (src < src_end) {
        unsigned int temp = *src++;
        temp = (((temp & 0xFF00) >> 8) | ((temp & 0x00FF) << 8));
        while (readl (VA_SPI_STAT) & STAT_FIFO_FULL) { }            /* wait for space in fifo... */
        writel (temp, VA_SPI_DAT);
    }

    while (readl (VA_SPI_FRM) != 0) { }                             /* wait for all frames to be transfered (fixme: at what point does FRM get decremented ??) */
    while (readl (VA_SPI_STAT) & STAT_ACTIVE) { }                   /* wait for shift in/out to complete (ie status to become inactive) */

    return 0;
}

/*
   Perform a single low level 8bit SPI write
*/
static unsigned int _ssa_spi_write_single8 (unsigned char value)
{
    unsigned int temp = (value & 0xFF);
    unsigned int config = CON_TX_SINGLE;

    if (DEBUG_LEVEL > 3)
        printk ("spiwr8 : 0x%02x\n", temp);

    writel ((GLOBAL_ENABLE | GLOBAL_RESET), VA_SPI_GLOBAL);
    writel ((GLOBAL_ENABLE), VA_SPI_GLOBAL);

    writel (config, VA_SPI_CON);
//  writel (0, VA_SPI_FRM);                                 /* 0 is the reset default (single transfer) */
    writel (temp, VA_SPI_DAT);                              /* write data to be sent to fifo */

    /*
        Fixme...
        This next test _does_ trigger if the CPU is fast enough.
        ie data has written to the fifo, but the fifo empty status bit
        is not cleared until sometime after the write completes.

        The un-answered question is: does it matter ??
        ie if we are going to poll STAT_ACTIVE, maybe that does behave in a sensible way ??

        For now, assume not and poll both flags.
    */

#if 0
    if ((readl (VA_SPI_STAT) & STAT_FIFO_EMPTY) == 0)
        printk ("spiwr8 : fifo empty after write !?\n");
#endif

    while (!(readl (VA_SPI_STAT) & STAT_FIFO_EMPTY)) { }    /* wait for data to exit fifo (ie fifo to become empty) */
    while (readl (VA_SPI_STAT) & STAT_ACTIVE) { }           /* wait for shift in/out to complete (ie status to become inactive) */

    return 0;
}

unsigned int ssa_spi_write_single8 (unsigned char value)
{
    unsigned int result;

    DRIVE_NCS_LOW();
    result = _ssa_spi_write_single8 (value);
    DRIVE_NCS_HIGH();

    return result;
}

/*
   Write 'count' bytes from 'src' buffer to SPI.
*/
unsigned int ssa_spi_write_multi (unsigned char *src, unsigned int count)
{
    unsigned char *src_end = &src[count];
    unsigned int bytes_remaining;
    unsigned int bytes_xfer;
    unsigned int words_xfer;        /* word == 16bits in this case */

    if (DEBUG_LEVEL > 2)
        printk ("spiwrm : src:0x%08lx, count:%4d\n", (unsigned long) src, count);

    DRIVE_NCS_LOW();

#if 1

    if ((((unsigned long) src) & 0x01) && (src < src_end))
        _ssa_spi_write_single8 (*src++);

    while ((bytes_remaining = (src_end - src)) >= 2) {
        bytes_xfer = ((bytes_remaining < MAX_FRM_BYTE_COUNT) ? bytes_remaining : MAX_FRM_BYTE_COUNT);
        words_xfer = (bytes_xfer / 2);
        _spi_fifo_write ((unsigned short *) src, words_xfer);
        src += (words_xfer * 2);
    }

    if (src < src_end)
        _ssa_spi_write_single8 (*src++);

#else

    while (src < src_end)
        _ssa_spi_write_single8 (*src++);

#endif

    DRIVE_NCS_HIGH();

    return 0;
}

/*
    Perform a single low level 16bit SPI write
    Data written to the device will be sent in big endian format.
*/
unsigned int ssa_spi_write_single16_be (unsigned short value)
{
    if (DEBUG_LEVEL > 1)
        printk ("spiwr16 : 0x%02x 0x%02x\n", ((value >> 8) & 0xFF), ((value >> 0) & 0xFF));

    DRIVE_NCS_LOW();
    _ssa_spi_write_single8 ((value >> 8) & 0xFF);
    _ssa_spi_write_single8 ((value >> 0) & 0xFF);
    DRIVE_NCS_HIGH();

    return 0;
}

/*
    Perform a single low level 16bit SPI write
    Data written to the device will be sent in little endian format.
    (alias for old ssa_spi_write_single16 function on little endian platforms).
*/
unsigned int ssa_spi_write_single16_le (unsigned short value)
{
    if (DEBUG_LEVEL > 1)
        printk ("spiwr16 : 0x%02x 0x%02x\n", ((value >> 0) & 0xFF), ((value >> 8) & 0xFF));

    DRIVE_NCS_LOW();
    _ssa_spi_write_single8 ((value >> 0) & 0xFF);
    _ssa_spi_write_single8 ((value >> 8) & 0xFF);
    DRIVE_NCS_HIGH();

    return 0;
}


/*******************************************************************************
*******************************************************************************/

static int __init blas_spi_init (void)
{
    printk ("BLAS SPI host driver, (c) Andre McCurdy, Philips Semiconductors\n");
    return ssa_spi_init();
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

__initcall (blas_spi_init);		/* 2.4 */

#else

device_initcall (blas_spi_init);	/* 2.6 */

#endif

/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(ssa_spi_init);
EXPORT_SYMBOL(ssa_spi_get_irq);
EXPORT_SYMBOL(ssa_spi_reset_drive_high);
EXPORT_SYMBOL(ssa_spi_reset_drive_low);
EXPORT_SYMBOL(ssa_spi_read_multi);
EXPORT_SYMBOL(ssa_spi_read_single16_be);
EXPORT_SYMBOL(ssa_spi_read_single16_le);
EXPORT_SYMBOL(ssa_spi_read_single8);
EXPORT_SYMBOL(ssa_spi_write_multi);
EXPORT_SYMBOL(ssa_spi_write_single16_be);
EXPORT_SYMBOL(ssa_spi_write_single16_le);
EXPORT_SYMBOL(ssa_spi_write_single8);

