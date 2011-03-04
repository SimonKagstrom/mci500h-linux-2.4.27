/*
 *  linux/arch/arm/mach-pnx0106/event_router.c
 *
 *  Copyright (C) 2005-2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <asm/arch/gpio.h>
#include <asm/arch/event_router.h>


/*
    Event Router register access definitions...
*/

#define VA_ER_PENDING(slice)                    (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C00 + (4 * (slice)))
#define VA_ER_INT_CLEAR(slice)                  (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C20 + (4 * (slice)))
#define VA_ER_INT_SET(slice)                    (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C40 + (4 * (slice)))
#define VA_ER_MASK(slice)                       (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C60 + (4 * (slice)))
#define VA_ER_MASK_CLEAR(slice)                 (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C80 + (4 * (slice)))
#define VA_ER_MASK_SET(slice)                   (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0CA0 + (4 * (slice)))
#define VA_ER_APR(slice)                        (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0CC0 + (4 * (slice)))     /* Activation Polarity Register... */
#define VA_ER_ATR(slice)                        (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0CE0 + (4 * (slice)))     /* Activation Type Register... (0 == level triggered, 1 == edge triggered) */
#define VA_ER_RAW_STATUS(slice)                 (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0D20 + (4 * (slice)))

/*
    Note: The INTOUT_MASK register is misnamed.
    It should really be INOUT_ENABLE (ie setting a bit in this register
    causes an event to be active or unmasked).
*/

#define VA_ER_INTOUT_PENDING(intout, slice)     (IO_ADDRESS_EVENT_ROUTER_BASE + 0x1000 + (0x20 * (intout)) + (4 * (slice)))
#define VA_ER_INTOUT_MASK(intout, slice)        (IO_ADDRESS_EVENT_ROUTER_BASE + 0x1400 + (0x20 * (intout)) + (4 * (slice)))
#define VA_ER_INTOUT_MASK_CLEAR(intout, slice)  (IO_ADDRESS_EVENT_ROUTER_BASE + 0x1800 + (0x20 * (intout)) + (4 * (slice)))
#define VA_ER_INTOUT_MASK_SET(intout, slice)    (IO_ADDRESS_EVENT_ROUTER_BASE + 0x1C00 + (0x20 * (intout)) + (4 * (slice)))


#define ER_EVENTS_PER_SLICE                     (32)
#define ER_NUM_SLICES                           ((ER_NUM_EVENTS + (ER_EVENTS_PER_SLICE - 1)) / ER_EVENTS_PER_SLICE)     /* should equal 6... ;-) */

#define ER_EVENT_SLICE(eid)                     ((eid) / ER_EVENTS_PER_SLICE)
#define ER_EVENT_MASK(eid)                      (1 << ((eid) % ER_EVENTS_PER_SLICE))

#define ER_NUM_INTOUTS                          (4)     /* Actually, there are 5... but only the first 4 can be routed to the interrupt controller (INTOUT_4 goes to CGU) */


static const unsigned char gpio_lut[] =
{
    [GPIO_GPIO_0]          = 1 + EVENT_ID_XT_GPIO_0,
    [GPIO_GPIO_1]          = 1 + EVENT_ID_XT_GPIO_1,
    [GPIO_GPIO_2]          = 1 + EVENT_ID_XT_GPIO_2,
    [GPIO_GPIO_3]          = 1 + EVENT_ID_XT_GPIO_3,
    [GPIO_GPIO_4]          = 1 + EVENT_ID_XT_GPIO_4,
    [GPIO_GPIO_5]          = 1 + EVENT_ID_XT_GPIO_5,
    [GPIO_GPIO_6]          = 1 + EVENT_ID_XT_GPIO_6,
    [GPIO_GPIO_7]          = 1 + EVENT_ID_XT_GPIO_7,
    [GPIO_GPIO_8]          = 1 + EVENT_ID_XT_GPIO_8,
    [GPIO_GPIO_9]          = 1 + EVENT_ID_XT_GPIO_9,
    [GPIO_GPIO_10]         = 1 + EVENT_ID_XT_GPIO_10,
    [GPIO_GPIO_11]         = 1 + EVENT_ID_XT_GPIO_11,
    [GPIO_GPIO_12]         = 1 + EVENT_ID_XT_GPIO_12,
    [GPIO_GPIO_13]         = 1 + EVENT_ID_XT_GPIO_13,
    [GPIO_GPIO_14]         = 1 + EVENT_ID_XT_GPIO_14,
    [GPIO_GPIO_15]         = 1 + EVENT_ID_XT_GPIO_15,
    [GPIO_GPIO_16]         = 1 + EVENT_ID_XT_GPIO_16,
    [GPIO_GPIO_17]         = 1 + EVENT_ID_XT_GPIO_17,
    [GPIO_GPIO_18]         = 1 + EVENT_ID_XT_GPIO_18,
    [GPIO_GPIO_19]         = 1 + EVENT_ID_XT_GPIO_19,
    [GPIO_GPIO_20]         = 1 + EVENT_ID_XT_GPIO_20,
    [GPIO_GPIO_21]         = 1 + EVENT_ID_XT_GPIO_21,
    [GPIO_GPIO_22]         = 1 + EVENT_ID_XT_GPIO_22,

    [GPIO_MPMC_A0]         = 1 + EVENT_ID_XT_MPMC_A0,
    [GPIO_MPMC_A1]         = 1 + EVENT_ID_XT_MPMC_A1,
    [GPIO_MPMC_A2]         = 1 + EVENT_ID_XT_MPMC_A2,
    [GPIO_MPMC_A3]         = 1 + EVENT_ID_XT_MPMC_A3,
    [GPIO_MPMC_A4]         = 1 + EVENT_ID_XT_MPMC_A4,
    [GPIO_MPMC_A5]         = 1 + EVENT_ID_XT_MPMC_A5,
    [GPIO_MPMC_A6]         = 1 + EVENT_ID_XT_MPMC_A6,
    [GPIO_MPMC_A7]         = 1 + EVENT_ID_XT_MPMC_A7,
    [GPIO_MPMC_A8]         = 1 + EVENT_ID_XT_MPMC_A8,
    [GPIO_MPMC_A9]         = 1 + EVENT_ID_XT_MPMC_A9,
    [GPIO_MPMC_A10]        = 1 + EVENT_ID_XT_MPMC_A10,
    [GPIO_MPMC_A11]        = 1 + EVENT_ID_XT_MPMC_A11,
    [GPIO_MPMC_A12]        = 1 + EVENT_ID_XT_MPMC_A12,
    [GPIO_MPMC_A13]        = 1 + EVENT_ID_XT_MPMC_A13,
    [GPIO_MPMC_A14]        = 1 + EVENT_ID_XT_MPMC_A14,
    [GPIO_MPMC_A15]        = 1 + EVENT_ID_XT_MPMC_A15,
    [GPIO_MPMC_A16]        = 1 + EVENT_ID_XT_MPMC_A16,
    [GPIO_MPMC_A17]        = 1 + EVENT_ID_XT_MPMC_A17,
    [GPIO_MPMC_A18]        = 1 + EVENT_ID_XT_MPMC_A18,
    [GPIO_MPMC_A19]        = 1 + EVENT_ID_XT_MPMC_A19,
    [GPIO_MPMC_A20]        = 1 + EVENT_ID_XT_MPMC_A20,
    [GPIO_MPMC_D0]         = 0,
    [GPIO_MPMC_D1]         = 0,
    [GPIO_MPMC_D2]         = 0,
    [GPIO_MPMC_D3]         = 0,
    [GPIO_MPMC_D4]         = 0,
    [GPIO_MPMC_D5]         = 0,
    [GPIO_MPMC_D6]         = 0,
    [GPIO_MPMC_D7]         = 0,
    [GPIO_MPMC_D8]         = 0,
    [GPIO_MPMC_D9]         = 0,
    [GPIO_MPMC_D10]        = 0,
    [GPIO_MPMC_D11]        = 0,
    [GPIO_MPMC_D12]        = 0,
    [GPIO_MPMC_D13]        = 0,
    [GPIO_MPMC_D14]        = 0,
    [GPIO_MPMC_D15]        = 0,
    [GPIO_MPMC_BLOUT0]     = 1 + EVENT_ID_XT_MPMC_BLOUT0,
    [GPIO_MPMC_BLOUT1]     = 1 + EVENT_ID_XT_MPMC_BLOUT1,
    [GPIO_MPMC_CKE]        = 1 + EVENT_ID_XT_MPMC_CKE,
    [GPIO_MPMC_CLKOUT]     = 1 + EVENT_ID_XT_MPMC_CLKOUT,
    [GPIO_MPMC_DQM0]       = 1 + EVENT_ID_XT_MPMC_DQM0,
    [GPIO_MPMC_DQM1]       = 1 + EVENT_ID_XT_MPMC_DQM1,
    [GPIO_MPMC_NCAS]       = 1 + EVENT_ID_XT_MPMC_NCAS,
    [GPIO_MPMC_NDYCS]      = 1 + EVENT_ID_XT_MPMC_NDYCS,
    [GPIO_MPMC_NOE]        = 1 + EVENT_ID_XT_MPMC_NOE,
    [GPIO_MPMC_NRAS]       = 1 + EVENT_ID_XT_MPMC_NRAS,
    [GPIO_MPMC_NSTCS0]     = 1 + EVENT_ID_XT_MPMC_NSTCS0,
    [GPIO_MPMC_NSTCS1]     = 1 + EVENT_ID_XT_MPMC_NSTCS1,
    [GPIO_MPMC_NSTCS2]     = 1 + EVENT_ID_XT_MPMC_NSTCS2,
    [GPIO_MPMC_NWE]        = 1 + EVENT_ID_XT_MPMC_NWE,

    [GPIO_PCCARD_A0]       = 1 + EVENT_ID_XT_PCCARD_A0,
    [GPIO_PCCARD_A1]       = 1 + EVENT_ID_XT_PCCARD_A1,
    [GPIO_PCCARD_A2]       = 1 + EVENT_ID_XT_PCCARD_A2,
    [GPIO_PCCARD_DD0]      = 1 + EVENT_ID_XT_PCCARD_DD0,
    [GPIO_PCCARD_DD1]      = 1 + EVENT_ID_XT_PCCARD_DD1,
    [GPIO_PCCARD_DD2]      = 1 + EVENT_ID_XT_PCCARD_DD2,
    [GPIO_PCCARD_DD3]      = 1 + EVENT_ID_XT_PCCARD_DD3,
    [GPIO_PCCARD_DD4]      = 1 + EVENT_ID_XT_PCCARD_DD4,
    [GPIO_PCCARD_DD5]      = 1 + EVENT_ID_XT_PCCARD_DD5,
    [GPIO_PCCARD_DD6]      = 1 + EVENT_ID_XT_PCCARD_DD6,
    [GPIO_PCCARD_DD7]      = 1 + EVENT_ID_XT_PCCARD_DD7,
    [GPIO_PCCARD_DD8]      = 1 + EVENT_ID_XT_PCCARD_DD8,
    [GPIO_PCCARD_DD9]      = 1 + EVENT_ID_XT_PCCARD_DD9,
    [GPIO_PCCARD_DD10]     = 1 + EVENT_ID_XT_PCCARD_DD10,
    [GPIO_PCCARD_DD11]     = 1 + EVENT_ID_XT_PCCARD_DD11,
    [GPIO_PCCARD_DD12]     = 1 + EVENT_ID_XT_PCCARD_DD12,
    [GPIO_PCCARD_DD13]     = 1 + EVENT_ID_XT_PCCARD_DD13,
    [GPIO_PCCARD_DD14]     = 1 + EVENT_ID_XT_PCCARD_DD14,
    [GPIO_PCCARD_DD15]     = 1 + EVENT_ID_XT_PCCARD_DD15,
    [GPIO_PCCARDA_DMARQ]   = 1 + EVENT_ID_XT_PCCARDA_DMARQ,
    [GPIO_PCCARDA_INTRQ]   = 1 + EVENT_ID_XT_PCCARDA_INTRQ,
    [GPIO_PCCARDA_IORDY]   = 1 + EVENT_ID_XT_PCCARDA_IORDY,
    [GPIO_PCCARDA_NCS0]    = 1 + EVENT_ID_XT_PCCARDA_NCS0,
    [GPIO_PCCARDA_NCS1]    = 1 + EVENT_ID_XT_PCCARDA_NCS1,
    [GPIO_PCCARDA_NDMACK]  = 1 + EVENT_ID_XT_PCCARDA_NDMACK,
    [GPIO_PCCARDA_NRESET]  = 1 + EVENT_ID_XT_PCCARDA_NRESET,
    [GPIO_PCCARD_NDIOR]    = 1 + EVENT_ID_XT_PCCARD_NDIOR,
    [GPIO_PCCARD_NDIOW]    = 1 + EVENT_ID_XT_PCCARD_NDIOW,

    [GPIO_LCD_CSB]         = 1 + EVENT_ID_XT_LCD_CSB,
    [GPIO_LCD_DB0]         = 1 + EVENT_ID_XT_LCD_DB0,
    [GPIO_LCD_DB1]         = 1 + EVENT_ID_XT_LCD_DB1,
    [GPIO_LCD_DB2]         = 1 + EVENT_ID_XT_LCD_DB2,
    [GPIO_LCD_DB3]         = 1 + EVENT_ID_XT_LCD_DB3,
    [GPIO_LCD_DB4]         = 1 + EVENT_ID_XT_LCD_DB4,
    [GPIO_LCD_DB5]         = 1 + EVENT_ID_XT_LCD_DB5,
    [GPIO_LCD_DB6]         = 1 + EVENT_ID_XT_LCD_DB6,
    [GPIO_LCD_DB7]         = 1 + EVENT_ID_XT_LCD_DB7,
    [GPIO_LCD_E_RD]        = 1 + EVENT_ID_XT_LCD_E_RD,
    [GPIO_LCD_RS]          = 1 + EVENT_ID_XT_LCD_RS,
    [GPIO_LCD_RW_WR]       = 1 + EVENT_ID_XT_LCD_RW_WR,

    [GPIO_IEEE1394_CNA]    = 1 + EVENT_ID_XT_IEEE1394_CNA,
    [GPIO_IEEE1394_CPS]    = 1 + EVENT_ID_XT_IEEE1394_CPS,
    [GPIO_IEEE1394_CTL0]   = 1 + EVENT_ID_XT_IEEE1394_CTL0,
    [GPIO_IEEE1394_CTL1]   = 1 + EVENT_ID_XT_IEEE1394_CTL1,
    [GPIO_IEEE1394_D0]     = 1 + EVENT_ID_XT_IEEE1394_D0,
    [GPIO_IEEE1394_D1]     = 1 + EVENT_ID_XT_IEEE1394_D1,
    [GPIO_IEEE1394_D2]     = 1 + EVENT_ID_XT_IEEE1394_D2,
    [GPIO_IEEE1394_D3]     = 1 + EVENT_ID_XT_IEEE1394_D3,
    [GPIO_IEEE1394_D4]     = 1 + EVENT_ID_XT_IEEE1394_D4,
    [GPIO_IEEE1394_D5]     = 1 + EVENT_ID_XT_IEEE1394_D5,
    [GPIO_IEEE1394_D6]     = 1 + EVENT_ID_XT_IEEE1394_D6,
    [GPIO_IEEE1394_D7]     = 1 + EVENT_ID_XT_IEEE1394_D7,
    [GPIO_IEEE1394_LPS]    = 1 + EVENT_ID_XT_IEEE1394_LPS,
    [GPIO_IEEE1394_LREQ]   = 1 + EVENT_ID_XT_IEEE1394_LREQ,
    [GPIO_IEEE1394_NRESET] = 1 + EVENT_ID_XT_IEEE1394_NRESET,
    [GPIO_IEEE1394_PD]     = 1 + EVENT_ID_XT_IEEE1394_PD,
    [GPIO_IEEE1394_SYSCLK] = 1 + EVENT_ID_XT_IEEE1394_SYSCLK,

    [GPIO_UART0_TXD]       = 1 + EVENT_ID_XT_UART0_TXD,
    [GPIO_UART0_RXD]       = 1 + EVENT_ID_XT_UART0_RXD,
    [GPIO_UART0_NCTS]      = 1 + EVENT_ID_XT_UART0_NCTS,
    [GPIO_UART0_NRTS]      = 1 + EVENT_ID_XT_UART0_NRTS,

    [GPIO_UART1_TXD]       = 1 + EVENT_ID_XT_UART1_TXD,
    [GPIO_UART1_RXD]       = 1 + EVENT_ID_XT_UART1_RXD,

    [GPIO_SPI0_EXT]        = 1 + EVENT_ID_XT_SPI0_EXT,
    [GPIO_SPI0_MISO]       = 1 + EVENT_ID_XT_SPI0_MISO,
    [GPIO_SPI0_MOSI]       = 1 + EVENT_ID_XT_SPI0_MOSI,
    [GPIO_SPI0_SCK]        = 1 + EVENT_ID_XT_SPI0_SCK,
    [GPIO_SPI0_S_N]        = 1 + EVENT_ID_XT_SPI0_S_N,

    [GPIO_SPI1_EXT]        = 1 + EVENT_ID_XT_SPI1_EXT,
    [GPIO_SPI1_MISO]       = 1 + EVENT_ID_XT_SPI1_MISO,
    [GPIO_SPI1_MOSI]       = 1 + EVENT_ID_XT_SPI1_MOSI,
    [GPIO_SPI1_SCK]        = 1 + EVENT_ID_XT_SPI1_SCK,
    [GPIO_SPI1_S_N]        = 1 + EVENT_ID_XT_SPI1_S_N,

    [GPIO_DAI_DATA]        = 1 + EVENT_ID_XT_DAI_DATA,
    [GPIO_DAI_WS]          = 1 + EVENT_ID_XT_DAI_WS,
    [GPIO_DAI_BCK]         = 1 + EVENT_ID_XT_DAI_BCK,

    [GPIO_DAO_BCK]         = 0,
    [GPIO_DAO_CLK]         = 1 + EVENT_ID_XT_DAO_CLK,
    [GPIO_DAO_DATA]        = 1 + EVENT_ID_XT_DAO_DATA,
    [GPIO_DAO_WS]          = 1 + EVENT_ID_XT_DAO_WS,

    [GPIO_SPDIF_OUT]       = 0,
};

int er_gpio_to_event_id (int gpio_id)
{
    if (gpio_id >= sizeof(gpio_lut)/sizeof(gpio_lut[0]))
        return -1;

    return gpio_lut[gpio_id] - 1;
}

int er_add_raw_input_event (int irq, int event_id, int event_type)
{
    unsigned int intout;
    unsigned int slice, mask;
    unsigned int apr, atr;
    unsigned long flags;

    intout = (irq - IRQ_EVENTROUTER_IRQ_0);                     /* convert interrupt controller IRQ number to event router interrupt output... */

    if ((intout >= ER_NUM_INTOUTS) || (event_id >= ER_NUM_EVENTS))
        return -1;

    slice = ER_EVENT_SLICE(event_id);
    mask = ER_EVENT_MASK(event_id);

#if 0
    printk ("er_eid  : %d\n", event_id);
    printk ("er_slice: 0x%08x\n", slice);
    printk ("er_mask : 0x%08x\n", mask);
#endif

    local_irq_save (flags);                                     /* protection while we read-modify-write type and polarity registers */

    /*
        Setup event type...
    */

    atr = readl (VA_ER_ATR(slice));
    apr = readl (VA_ER_APR(slice));

    if ((event_type == EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_LOW) ||
        (event_type == EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH))
    {
        atr &= ~mask;                                           /* clear bit in type register to make event level triggered */
    }
    else
    {
        atr |= mask;                                            /* set bit in type register to make event edge triggered */
    }

    if ((event_type == EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_LOW) ||
        (event_type == EVENT_TYPE_FALLING_EDGE_TRIGGERED))
    {
        apr &= ~mask;                                           /* clear bit in polarity register to make event falling edge / low level triggered */
    }
    else
    {
        apr |= mask;                                            /* set bit in polarity register to make event rising edge / high level triggered */
    }

    writel (atr, VA_ER_ATR(slice));
    writel (apr, VA_ER_APR(slice));

    writel (mask, VA_ER_INT_CLEAR(slice));                      /* clear any spurious latched int requests (only required for edge triggered ints - and maybe not even then... HW docs are very unclear on details) */

    local_irq_restore (flags);

    writel (mask, VA_ER_MASK_SET(slice));                       /* enable input event */


    /*
        Add the newly configured event to the set of events which
        the selected interrupt output takes notice of...
    */

    writel (mask, VA_ER_INTOUT_MASK_SET(intout,slice));         /* */

    return 0;
}

int er_add_gpio_input_event (int irq, int gpio_id, int event_type)
{
    int event_id;

    event_id = er_gpio_to_event_id (gpio_id);

    //printk ("%s: gpio_id 0x%02x, event_id %d\n", __FUNCTION__, gpio_id, event_id);

    if (event_id == -1) {
        printk ("%s: GPIO_ID 0x%02x is not a valid event router event\n", __FUNCTION__, gpio_id);
        return -1;
    }

    gpio_set_as_input (gpio_id);
    return er_add_raw_input_event (irq, event_id, event_type);
}

int er_clr_gpio_int (int gpio_id)/*LTPE Joe to clear the INT*/
{
    int event_id;
    unsigned int slice, mask, result;

    event_id = er_gpio_to_event_id (gpio_id);

    //printk ("%s: gpio_id 0x%02x, event_id %d\n", __FUNCTION__, gpio_id, event_id);

    if (event_id == -1) {
        printk ("%s: GPIO_ID 0x%02x is not a valid event router event\n", __FUNCTION__, gpio_id);
        return -1;
    }

    slice = ER_EVENT_SLICE(event_id);
    mask = ER_EVENT_MASK(event_id);
#if 0
    printk ("er_eid  : %d\n", event_id);
    printk ("er_slice: 0x%08x\n", slice);
    printk ("er_mask : 0x%08x\n", mask);
#endif
    result = readl(VA_ER_PENDING(slice));
    //printk(KERN_INFO "INT Status=%x\n",result);

    writel (mask, VA_ER_INT_CLEAR(slice));

    return 0;
}

unsigned int er_read_int_status (int gpio_id)/*LTPE Joe to read the INT status*/
{
    int event_id;
    unsigned int slice, result;

    event_id = er_gpio_to_event_id (gpio_id);

    //printk ("%s: gpio_id 0x%02x, event_id %d\n", __FUNCTION__, gpio_id, event_id);

    if (event_id == -1) {
        printk ("%s: GPIO_ID 0x%02x is not a valid event router event\n", __FUNCTION__, gpio_id);
        return (unsigned int) -1;
    }

    slice = ER_EVENT_SLICE(event_id);

    result = readl(VA_ER_PENDING(slice));

    return result;
}
/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_PROC_FS)

static int proc_er_status_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int i,j;
    char *p = page;

    /*
        We expect to return all data in a single read call.
        ie things may go badly wrong if (amount of data > 4096)...
    */

    p += sprintf (p, "Pending         :"); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_PENDING(j))   ); p += sprintf (p, "\n");
    p += sprintf (p, "Enable          :"); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_MASK(j))      ); p += sprintf (p, "\n");
    p += sprintf (p, "Polarity        :"); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_APR(j))       ); p += sprintf (p, "\n");
    p += sprintf (p, "Type            :"); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_ATR(j))       ); p += sprintf (p, "\n");
    p += sprintf (p, "RawStatus       :"); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_RAW_STATUS(j))); p += sprintf (p, "\n");

    for (i = 0; i < ER_NUM_INTOUTS; i++) {
        p += sprintf (p, "Intout %d Enable :", i); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_INTOUT_MASK(i,j)));    p += sprintf (p, "\n");
        p += sprintf (p, "Intout %d Pending:", i); for (j = (ER_NUM_SLICES - 1); j >= 0; j--) p += sprintf (p, " %08x", readl (VA_ER_INTOUT_PENDING(i,j))); p += sprintf (p, "\n");
    }

    *eof = 1;
    return (p - page);
}

static int __init er_init_proc_entry (void)
{
    /*
        er_init() runs too early in the startup sequence to setup
        proc entries, so we need a separate init function...
    */
    create_proc_read_entry ("er_status", 0, NULL, proc_er_status_read, NULL);

    return 0;
}

__initcall (er_init_proc_entry);

#endif


/*****************************************************************************
*****************************************************************************/

void __init er_init (void)
{
    int i,j;

    for (i = 0; i < ER_NUM_INTOUTS; i++)
        for (j = 0; j < ER_NUM_SLICES; j++)
            writel (0xFFFFFFFF, VA_ER_INTOUT_MASK_CLEAR(i,j));  /* for all interrupt outputs, mask (ie ignore) all input events */

    for (j = 0; j < ER_NUM_SLICES; j++) {
        writel (0x00000000, VA_ER_MASK(j));                     /* set all input events to masked */
        writel (0x00000000, VA_ER_ATR(j));                      /* set all input events to level triggered */
        writel (0x00000000, VA_ER_APR(j));                      /* set all input events to active low */
        writel (0xFFFFFFFF, VA_ER_INT_CLEAR(j));                /* clear any edge triggered events which may have been latched previously */
    }
}


/*****************************************************************************
*****************************************************************************/

#if 1
EXPORT_SYMBOL(er_gpio_to_event_id);
EXPORT_SYMBOL(er_clr_gpio_int);/*LTPE Joe to clear the INT*/
EXPORT_SYMBOL(er_read_int_status);/*LTPE Joe to read the INT status*/
EXPORT_SYMBOL(er_add_gpio_input_event);
EXPORT_SYMBOL(er_add_raw_input_event);
#endif

