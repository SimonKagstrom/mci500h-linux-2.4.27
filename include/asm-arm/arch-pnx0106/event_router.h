/*
 *  linux/include/asm-arm/arch-pnx0106/event_router.h
 *
 *  Copyright (C) 2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_EVENT_ROUTER_H_
#define __ASM_ARCH_EVENT_ROUTER_H_


#define EVENT_ID_GDMA_DMA_REQ_MUX               0
#define EVENT_ID_XT_IEEE1394_PHY_CNA            1
#define EVENT_ID_IEEE1394_NIRQ_MUX              2
#define EVENT_ID_CCP_IPINT0_INT_FSI_IP          3
#define EVENT_ID_CCP_IPINT1_INT_FSI_IP          4
#define EVENT_ID_ARM926EJS_NFIQ                 5
#define EVENT_ID_ARM926EJS_NIRQ                 6

#define EVENT_ID_XT_USB_VBUS                    7       /* PAD_T14 (input only - doesn't pass through GPIO controller) */

#define EVENT_ID_XT_SPI0_EXT                    8
#define EVENT_ID_XT_SPI0_SCK                    9
#define EVENT_ID_XT_SPI0_MISO                   10
#define EVENT_ID_XT_SPI0_MOSI                   11
#define EVENT_ID_XT_SPI0_S_N                    12
#define EVENT_ID_XT_SPI1_EXT                    13
#define EVENT_ID_XT_SPI1_SCK                    14
#define EVENT_ID_XT_SPI1_MISO                   15
#define EVENT_ID_XT_SPI1_MOSI                   16
#define EVENT_ID_XT_SPI1_S_N                    17
#define EVENT_ID_XT_UART0_RXD                   18
#define EVENT_ID_XT_UART0_TXD                   19
#define EVENT_ID_XT_UART0_NRTS                  20
#define EVENT_ID_XT_UART0_NCTS                  21
#define EVENT_ID_XT_UART1_RXD                   22
#define EVENT_ID_XT_UART1_TXD                   23
#define EVENT_ID_XT_LCD_CSB                     24
#define EVENT_ID_XT_LCD_RS                      25
#define EVENT_ID_XT_LCD_DB7                     26
#define EVENT_ID_XT_LCD_DB6                     27
#define EVENT_ID_XT_LCD_DB5                     28
#define EVENT_ID_XT_LCD_DB4                     29
#define EVENT_ID_XT_LCD_DB3                     30
#define EVENT_ID_XT_LCD_DB2                     31
#define EVENT_ID_XT_LCD_DB1                     32
#define EVENT_ID_XT_LCD_DB0                     33
#define EVENT_ID_XT_LCD_RW_WR                   34
#define EVENT_ID_XT_LCD_E_RD                    35
#define EVENT_ID_XT_DAI_DATA                    36
#define EVENT_ID_XT_DAI_WS                      37
#define EVENT_ID_XT_DAI_BCK                     38
#define EVENT_ID_XT_DAO_WS                      39
#define EVENT_ID_XT_DAO_DATA                    40
#define EVENT_ID_XT_DAO_CLK                     41
#define EVENT_ID_XT_MPMC_NSTCS0                 42
#define EVENT_ID_XT_MPMC_NSTCS1                 43
#define EVENT_ID_XT_MPMC_NSTCS2                 44
#define EVENT_ID_XT_MPMC_NDYCS                  45
#define EVENT_ID_XT_MPMC_CKE                    46
#define EVENT_ID_XT_MPMC_DQM0                   47
#define EVENT_ID_XT_MPMC_DQM1                   48
#define EVENT_ID_XT_MPMC_BLOUT0                 49
#define EVENT_ID_XT_MPMC_BLOUT1                 50
#define EVENT_ID_XT_MPMC_CLKOUT                 51
#define EVENT_ID_XT_MPMC_NWE                    52
#define EVENT_ID_XT_MPMC_NCAS                   53
#define EVENT_ID_XT_MPMC_NRAS                   54
#define EVENT_ID_XT_MPMC_NOE                    55
#define EVENT_ID_XT_MPMC_RPOUT                  56      /* no physical pin ?? (or is it perhaps multiplexed and called something else in the FRS pinout ??) */
#define EVENT_ID_XT_MPMC_A0                     57
#define EVENT_ID_XT_MPMC_A1                     58
#define EVENT_ID_XT_MPMC_A2                     59
#define EVENT_ID_XT_MPMC_A3                     60
#define EVENT_ID_XT_MPMC_A4                     61
#define EVENT_ID_XT_MPMC_A5                     62
#define EVENT_ID_XT_MPMC_A6                     63
#define EVENT_ID_XT_MPMC_A7                     64
#define EVENT_ID_XT_MPMC_A8                     65
#define EVENT_ID_XT_MPMC_A9                     66
#define EVENT_ID_XT_MPMC_A10                    67
#define EVENT_ID_XT_MPMC_A11                    68
#define EVENT_ID_XT_MPMC_A12                    69
#define EVENT_ID_XT_MPMC_A13                    70
#define EVENT_ID_XT_MPMC_A14                    71
#define EVENT_ID_XT_MPMC_A15                    72
#define EVENT_ID_XT_MPMC_A16                    73
#define EVENT_ID_XT_MPMC_A17                    74
#define EVENT_ID_XT_MPMC_A18                    75
#define EVENT_ID_XT_MPMC_A19                    76
#define EVENT_ID_XT_MPMC_A20                    77
#define EVENT_ID_XT_PCCARDA_NRESET              78
#define EVENT_ID_XT_PCCARDA_NCS0                79
#define EVENT_ID_XT_PCCARDA_NCS1                80
#define EVENT_ID_XT_PCCARDA_INTRQ               81
#define EVENT_ID_XT_PCCARDA_IORDY               82
#define EVENT_ID_XT_PCCARDA_NDMACK              83
#define EVENT_ID_XT_PCCARDA_DMARQ               84
#define EVENT_ID_XT_PCCARD_NDIOW                85
#define EVENT_ID_XT_PCCARD_NDIOR                86
#define EVENT_ID_XT_PCCARD_A0                   87
#define EVENT_ID_XT_PCCARD_A1                   88
#define EVENT_ID_XT_PCCARD_A2                   89
#define EVENT_ID_XT_PCCARD_DD0                  90
#define EVENT_ID_XT_PCCARD_DD1                  91
#define EVENT_ID_XT_PCCARD_DD2                  92
#define EVENT_ID_XT_PCCARD_DD3                  93
#define EVENT_ID_XT_PCCARD_DD4                  94
#define EVENT_ID_XT_PCCARD_DD5                  95
#define EVENT_ID_XT_PCCARD_DD6                  96
#define EVENT_ID_XT_PCCARD_DD7                  97
#define EVENT_ID_XT_PCCARD_DD8                  98
#define EVENT_ID_XT_PCCARD_DD9                  99
#define EVENT_ID_XT_PCCARD_DD10                 100
#define EVENT_ID_XT_PCCARD_DD11                 101
#define EVENT_ID_XT_PCCARD_DD12                 102
#define EVENT_ID_XT_PCCARD_DD13                 103
#define EVENT_ID_XT_PCCARD_DD14                 104
#define EVENT_ID_XT_PCCARD_DD15                 105
#define EVENT_ID_XT_IEEE1394_PD                 106
#define EVENT_ID_XT_IEEE1394_NRESET             107
#define EVENT_ID_XT_IEEE1394_CTL0               108
#define EVENT_ID_XT_IEEE1394_CTL1               109
#define EVENT_ID_XT_IEEE1394_LREQ               110
#define EVENT_ID_XT_IEEE1394_SYSCLK             111
#define EVENT_ID_XT_IEEE1394_CNA                112
#define EVENT_ID_XT_IEEE1394_LPS                113
#define EVENT_ID_XT_IEEE1394_CPS                114
#define EVENT_ID_XT_IEEE1394_D0                 115
#define EVENT_ID_XT_IEEE1394_D1                 116
#define EVENT_ID_XT_IEEE1394_D2                 117
#define EVENT_ID_XT_IEEE1394_D3                 118
#define EVENT_ID_XT_IEEE1394_D4                 119
#define EVENT_ID_XT_IEEE1394_D5                 120
#define EVENT_ID_XT_IEEE1394_D6                 121
#define EVENT_ID_XT_IEEE1394_D7                 122

#define EVENT_ID_SSA1_TIMER0_INT                123
#define EVENT_ID_SSA1_TIMER1_INT                124
#define EVENT_ID_SSA1_RTC_NINTR                 125     /* Real Time Clock */
#define EVENT_ID_SSA1_ADC_INT                   126

#define EVENT_ID_XT_MCI_DAT0                    127     /* external pin multiplexed with DAI_WS ??      */
#define EVENT_ID_XT_MCI_DAT1                    128     /* external pin multiplexed with DAI_DATA ??    */
#define EVENT_ID_XT_MCI_DAT2                    129     /* external pin multiplexed with LCD_E_RD ??    */
#define EVENT_ID_XT_MCI_DAT3                    130     /* external pin multiplexed with LCD_RW_WR ??   */

#define EVENT_ID_WDOG_M0                        131

#if 0
#define EVENT_ID_XT_UART0_RXD                   132     /* seems to be a duplicate of event ID 18... !?! */
#define EVENT_ID_XT_UART1_RXD                   133     /* seems to be a duplicate of event ID 22... !?! */
#endif

#define EVENT_ID_XT_I2C0_SCL_N                  134     /* PAD_??? (input only - doesn't pass through GPIO controller) */
#define EVENT_ID_XT_I2C1_SCL_N                  135     /* PAD_??? (input only - doesn't pass through GPIO controller) */

#define EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ1       136
#define EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ2       137

#define EVENT_ID_XT_GPIO_0                      138
#define EVENT_ID_XT_GPIO_1                      139
#define EVENT_ID_XT_GPIO_2                      140
#define EVENT_ID_XT_GPIO_3                      141
#define EVENT_ID_XT_GPIO_4                      142
#define EVENT_ID_XT_GPIO_5                      143
#define EVENT_ID_XT_GPIO_6                      144
#define EVENT_ID_XT_GPIO_7                      145
#define EVENT_ID_XT_GPIO_8                      146
#define EVENT_ID_XT_GPIO_9                      147
#define EVENT_ID_XT_GPIO_10                     148
#define EVENT_ID_XT_GPIO_11                     149
#define EVENT_ID_XT_GPIO_12                     150
#define EVENT_ID_XT_GPIO_13                     151
#define EVENT_ID_XT_GPIO_14                     152
#define EVENT_ID_XT_GPIO_15                     153
#define EVENT_ID_XT_GPIO_16                     154
#define EVENT_ID_XT_GPIO_17                     155
#define EVENT_ID_XT_GPIO_18                     156
#define EVENT_ID_XT_GPIO_19                     157
#define EVENT_ID_XT_GPIO_20                     158
#define EVENT_ID_XT_GPIO_21                     159
#define EVENT_ID_XT_GPIO_22                     160

#define EVENT_ID_USB_UC_GOSUSP                  161
#define EVENT_ID_USB_UC_WKUPCS                  162
#define EVENT_ID_USB_UC_PWROFF                  163
#define EVENT_ID_USB_VBUS                       164
#define EVENT_ID_USB_BUS_RESET                  165
#define EVENT_ID_USB_GOOD_LINK                  166
#define EVENT_ID_USB_OTG_AHB_NEEDCLK            167
#define EVENT_ID_USB_ATX_PLL_LOCK               168
#define EVENT_ID_USB_OTG_VBUS_PWR_EN            169

#define ER_NUM_EVENTS                           (170)


/*****************************************************************************
*****************************************************************************/

#define EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_LOW    (0)
#define EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH   (1)
#define EVENT_TYPE_FALLING_EDGE_TRIGGERED       (2)
#define EVENT_TYPE_RISING_EDGE_TRIGGERED        (3)


/*****************************************************************************
*****************************************************************************/

#ifndef __ASSEMBLY__

#include <linux/init.h>

extern int er_gpio_to_event_id (int gpio_id);
extern int er_add_raw_input_event (int irq, int event_id, int event_type);
extern int er_add_gpio_input_event (int irq, int gpio_id, int event_type);

extern void __init er_init (void);

#endif

#endif

