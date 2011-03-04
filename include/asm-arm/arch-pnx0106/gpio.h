/*
 *  linux/include/asm-arm/arch-pnx0106/gpio.h
 *
 *  Copyright (C) 2003-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <linux/config.h>

#define GPIO_MPMC_D0            ((0 << 5) | ( 0 << 0))      /* PAD_D2       */
#define GPIO_MPMC_A0            ((0 << 5) | ( 1 << 0))      /* PAD_C6       */
#define GPIO_MPMC_D1            ((0 << 5) | ( 2 << 0))      /* PAD_D1       */
#define GPIO_MPMC_A1            ((0 << 5) | ( 3 << 0))      /* PAD_B6       */
#define GPIO_MPMC_D2            ((0 << 5) | ( 4 << 0))      /* PAD_E4       */
#define GPIO_MPMC_A2            ((0 << 5) | ( 5 << 0))      /* PAD_A6       */
#define GPIO_MPMC_D3            ((0 << 5) | ( 6 << 0))      /* PAD_E3       */
#define GPIO_MPMC_A3            ((0 << 5) | ( 7 << 0))      /* PAD_D5       */
#define GPIO_MPMC_D4            ((0 << 5) | ( 8 << 0))      /* PAD_E2       */
#define GPIO_MPMC_A4            ((0 << 5) | ( 9 << 0))      /* PAD_C5       */
#define GPIO_MPMC_D5            ((0 << 5) | (10 << 0))      /* PAD_E1       */
#define GPIO_MPMC_A5            ((0 << 5) | (11 << 0))      /* PAD_B5       */
#define GPIO_MPMC_D6            ((0 << 5) | (12 << 0))      /* PAD_F4       */
#define GPIO_MPMC_A6            ((0 << 5) | (13 << 0))      /* PAD_A5       */
#define GPIO_MPMC_D7            ((0 << 5) | (14 << 0))      /* PAD_F3       */
#define GPIO_MPMC_A7            ((0 << 5) | (15 << 0))      /* PAD_C4       */
#define GPIO_MPMC_D8            ((0 << 5) | (16 << 0))      /* PAD_F2       */
#define GPIO_MPMC_A8            ((0 << 5) | (17 << 0))      /* PAD_B4       */
#define GPIO_MPMC_D9            ((0 << 5) | (18 << 0))      /* PAD_F1       */
#define GPIO_MPMC_A9            ((0 << 5) | (19 << 0))      /* PAD_A4       */
#define GPIO_MPMC_D10           ((0 << 5) | (20 << 0))      /* PAD_G4       */
#define GPIO_MPMC_A10           ((0 << 5) | (21 << 0))      /* PAD_C3       */
#define GPIO_MPMC_D11           ((0 << 5) | (22 << 0))      /* PAD_G3       */
#define GPIO_MPMC_A11           ((0 << 5) | (23 << 0))      /* PAD_B3       */
#define GPIO_MPMC_D12           ((0 << 5) | (24 << 0))      /* PAD_G2       */
#define GPIO_MPMC_A12           ((0 << 5) | (25 << 0))      /* PAD_A3       */
#define GPIO_MPMC_D13           ((0 << 5) | (26 << 0))      /* PAD_G1       */
#define GPIO_MPMC_A13           ((0 << 5) | (27 << 0))      /* PAD_A2       */
#define GPIO_MPMC_D14           ((0 << 5) | (28 << 0))      /* PAD_H3       */
#define GPIO_MPMC_A14           ((0 << 5) | (29 << 0))      /* PAD_A1       */
#define GPIO_MPMC_D15           ((0 << 5) | (30 << 0))      /* PAD_H2       */
#define GPIO_MPMC_NDYCS         ((0 << 5) | (31 << 0))      /* PAD_K3       */

#define GPIO_MPMC_CKE           ((1 << 5) | ( 0 << 0))      /* PAD_L3       */
#define GPIO_MPMC_DQM0          ((1 << 5) | ( 1 << 0))      /* PAD_J1       */
#define GPIO_MPMC_DQM1          ((1 << 5) | ( 2 << 0))      /* PAD_K4       */
#define GPIO_MPMC_CLKOUT        ((1 << 5) | ( 3 << 0))      /* PAD_J2       */
#define GPIO_MPMC_NWE           ((1 << 5) | ( 4 << 0))      /* PAD_L4       */
#define GPIO_MPMC_NCAS          ((1 << 5) | ( 5 << 0))      /* PAD_K1       */
#define GPIO_MPMC_NRAS          ((1 << 5) | ( 6 << 0))      /* PAD_K2       */

#define GPIO_SPI0_S_N           ((2 << 5) | ( 0 << 0))      /* PAD_E14      */
#define GPIO_SPI0_MISO          ((2 << 5) | ( 1 << 0))      /* PAD_F17      */
#define GPIO_SPI0_MOSI          ((2 << 5) | ( 2 << 0))      /* PAD_E13      */
#define GPIO_SPI0_SCK           ((2 << 5) | ( 3 << 0))      /* PAD_F16      */
#define GPIO_SPI0_EXT           ((2 << 5) | ( 4 << 0))      /* PAD_F15      */
#define GPIO_PCCARDA_NRESET     ((2 << 5) | ( 5 << 0))      /* PAD_B17      */
#define GPIO_PCCARDA_NCS0       ((2 << 5) | ( 6 << 0))      /* PAD_B16      */
#define GPIO_PCCARDA_NCS1       ((2 << 5) | ( 7 << 0))      /* PAD_A16      */
#define GPIO_PCCARDA_INTRQ      ((2 << 5) | ( 8 << 0))      /* PAD_B15      */
#define GPIO_PCCARDA_IORDY      ((2 << 5) | ( 9 << 0))      /* PAD_C14      */
#define GPIO_PCCARDA_NDMACK     ((2 << 5) | (10 << 0))      /* PAD_A15      */
#define GPIO_PCCARD_A0          ((2 << 5) | (11 << 0))      /* PAD_D9       */
#define GPIO_PCCARD_A1          ((2 << 5) | (12 << 0))      /* PAD_C9       */
#define GPIO_PCCARD_A2          ((2 << 5) | (13 << 0))      /* PAD_B9       */
#define GPIO_PCCARDA_DMARQ      ((2 << 5) | (14 << 0))      /* PAD_D14      */
#define GPIO_PCCARD_NDIOW       ((2 << 5) | (15 << 0))      /* PAD_B14      */
#define GPIO_PCCARD_NDIOR       ((2 << 5) | (16 << 0))      /* PAD_A14      */
#define GPIO_PCCARD_DD0         ((2 << 5) | (17 << 0))      /* PAD_D13      */
#define GPIO_PCCARD_DD1         ((2 << 5) | (18 << 0))      /* PAD_C13      */
#define GPIO_PCCARD_DD2         ((2 << 5) | (19 << 0))      /* PAD_B13      */
#define GPIO_PCCARD_DD3         ((2 << 5) | (20 << 0))      /* PAD_A13      */
#define GPIO_PCCARD_DD4         ((2 << 5) | (21 << 0))      /* PAD_D12      */
#define GPIO_PCCARD_DD5         ((2 << 5) | (22 << 0))      /* PAD_C12      */
#define GPIO_PCCARD_DD6         ((2 << 5) | (23 << 0))      /* PAD_B12      */
#define GPIO_PCCARD_DD7         ((2 << 5) | (24 << 0))      /* PAD_A12      */
#define GPIO_PCCARD_DD8         ((2 << 5) | (25 << 0))      /* PAD_D11      */
#define GPIO_PCCARD_DD9         ((2 << 5) | (26 << 0))      /* PAD_C11      */
#define GPIO_PCCARD_DD10        ((2 << 5) | (27 << 0))      /* PAD_B11      */
#define GPIO_PCCARD_DD11        ((2 << 5) | (28 << 0))      /* PAD_A11      */
#define GPIO_PCCARD_DD12        ((2 << 5) | (29 << 0))      /* PAD_D10      */
#define GPIO_PCCARD_DD13        ((2 << 5) | (30 << 0))      /* PAD_C10      */
#define GPIO_PCCARD_DD14        ((2 << 5) | (31 << 0))      /* PAD_B10      */

#define GPIO_PCCARD_DD15        ((3 << 5) | ( 0 << 0))      /* PAD_A10      */
#define GPIO_LCD_RS             ((3 << 5) | ( 1 << 0))      /* PAD_M13      */
#define GPIO_LCD_CSB            ((3 << 5) | ( 2 << 0))      /* PAD_N17      */
#define GPIO_LCD_DB1            ((3 << 5) | ( 3 << 0))      /* PAD_P14      */
#define GPIO_LCD_DB2            ((3 << 5) | ( 4 << 0))      /* PAD_R17      */
#define GPIO_LCD_DB3            ((3 << 5) | ( 5 << 0))      /* PAD_R16      */
#define GPIO_LCD_DB4            ((3 << 5) | ( 6 << 0))      /* PAD_T17      */
#define GPIO_LCD_DB5            ((3 << 5) | ( 7 << 0))      /* PAD_T16      */
#define GPIO_LCD_DB6            ((3 << 5) | ( 8 << 0))      /* PAD_M15      */
#define GPIO_LCD_DB7            ((3 << 5) | ( 9 << 0))      /* PAD_M14      */
#define GPIO_LCD_DB0            ((3 << 5) | (10 << 0))      /* PAD_P15      */
#define GPIO_LCD_RW_WR          ((3 << 5) | (11 << 0))      /* PAD_P16      */
#define GPIO_LCD_E_RD           ((3 << 5) | (12 << 0))      /* PAD_P17      */
#define GPIO_DAI_DATA           ((3 << 5) | (13 << 0))      /* PAD_N14      */
#define GPIO_DAI_WS             ((3 << 5) | (14 << 0))      /* PAD_N15      */
#define GPIO_DAI_BCK            ((3 << 5) | (15 << 0))      /* PAD_N16      */
#define GPIO_UART0_TXD          ((3 << 5) | (16 << 0))      /* PAD_E17      */
#define GPIO_UART0_RXD          ((3 << 5) | (17 << 0))      /* PAD_E16      */
#define GPIO_UART1_TXD          ((3 << 5) | (18 << 0))      /* PAD_M17      */
#define GPIO_UART1_RXD          ((3 << 5) | (19 << 0))      /* PAD_M16      */
#define GPIO_DAO_WS             ((3 << 5) | (20 << 0))      /* PAD_L13      */
#define GPIO_DAO_BCK            ((3 << 5) | (21 << 0))      /* PAD_L14      */
#define GPIO_DAO_DATA           ((3 << 5) | (22 << 0))      /* PAD_L15      */
#define GPIO_DAO_CLK            ((3 << 5) | (23 << 0))      /* PAD_L16      */
#define GPIO_GPIO_22            ((3 << 5) | (24 << 0))      /* PAD_B7       */
#define GPIO_GPIO_21            ((3 << 5) | (25 << 0))      /* PAD_C7       */
#define GPIO_GPIO_20            ((3 << 5) | (26 << 0))      /* PAD_A8       */
#define GPIO_GPIO_19            ((3 << 5) | (27 << 0))      /* PAD_B8       */
#define GPIO_UART0_NCTS         ((3 << 5) | (28 << 0))      /* PAD_D15      */
#define GPIO_SPI1_SCK           ((3 << 5) | (29 << 0))      /* PAD_D16      */
#define GPIO_SPI1_MISO          ((3 << 5) | (30 << 0))      /* PAD_D17      */
#define GPIO_SPI1_MOSI          ((3 << 5) | (31 << 0))      /* PAD_C15      */

#define GPIO_SPI1_EXT           ((4 << 5) | ( 0 << 0))      /* PAD_C16      */
#define GPIO_SPI1_S_N           ((4 << 5) | ( 1 << 0))      /* PAD_C17      */
#define GPIO_UART0_NRTS         ((4 << 5) | ( 2 << 0))      /* PAD_E15      */
#define GPIO_IEEE1394_PD        ((4 << 5) | ( 3 << 0))      /* PAD_K14      */
#define GPIO_IEEE1394_NRESET    ((4 << 5) | ( 4 << 0))      /* PAD_K15      */
#define GPIO_IEEE1394_CTL0      ((4 << 5) | ( 5 << 0))      /* PAD_K16      */
#define GPIO_IEEE1394_CTL1      ((4 << 5) | ( 6 << 0))      /* PAD_K17      */
#define GPIO_IEEE1394_LREQ      ((4 << 5) | ( 7 << 0))      /* PAD_J15      */
#define GPIO_IEEE1394_SYSCLK    ((4 << 5) | ( 8 << 0))      /* PAD_J16      */
#define GPIO_IEEE1394_D0        ((4 << 5) | ( 9 << 0))      /* PAD_J17      */
#define GPIO_IEEE1394_D1        ((4 << 5) | (10 << 0))      /* PAD_H15      */
#define GPIO_IEEE1394_D2        ((4 << 5) | (11 << 0))      /* PAD_H16      */
#define GPIO_IEEE1394_D3        ((4 << 5) | (12 << 0))      /* PAD_H17      */
#define GPIO_IEEE1394_D4        ((4 << 5) | (13 << 0))      /* PAD_G14      */
#define GPIO_IEEE1394_D5        ((4 << 5) | (14 << 0))      /* PAD_G15      */
#define GPIO_IEEE1394_D6        ((4 << 5) | (15 << 0))      /* PAD_G16      */
#define GPIO_IEEE1394_D7        ((4 << 5) | (16 << 0))      /* PAD_G17      */
#define GPIO_IEEE1394_CNA       ((4 << 5) | (17 << 0))      /* PAD_F13      */
#define GPIO_IEEE1394_LPS       ((4 << 5) | (18 << 0))      /* PAD_L17      */
#define GPIO_IEEE1394_CPS       ((4 << 5) | (19 << 0))      /* PAD_F14      */
#define GPIO_GPIO_0             ((4 << 5) | (20 << 0))      /* PAD_T9       */
#define GPIO_GPIO_1             ((4 << 5) | (21 << 0))      /* PAD_U9       */
#define GPIO_GPIO_2             ((4 << 5) | (22 << 0))      /* PAD_A7       */
#define GPIO_GPIO_3             ((4 << 5) | (23 << 0))      /* PAD_N13      */
#define GPIO_GPIO_4             ((4 << 5) | (24 << 0))      /* PAD_U12      */
#define GPIO_GPIO_5             ((4 << 5) | (25 << 0))      /* PAD_T12      */
#define GPIO_GPIO_6             ((4 << 5) | (26 << 0))      /* PAD_R12      */
#define GPIO_GPIO_7             ((4 << 5) | (27 << 0))      /* PAD_P12      */
#define GPIO_GPIO_18            ((4 << 5) | (28 << 0))      /* PAD_C8       */
#define GPIO_GPIO_17            ((4 << 5) | (29 << 0))      /* PAD_P10      */
#define GPIO_GPIO_16            ((4 << 5) | (30 << 0))      /* PAD_R10      */
#define GPIO_GPIO_15            ((4 << 5) | (31 << 0))      /* PAD_T10      */

#define GPIO_GPIO_14            ((5 << 5) | ( 0 << 0))      /* PAD_U10      */
#define GPIO_GPIO_13            ((5 << 5) | ( 1 << 0))      /* PAD_N11      */
#define GPIO_GPIO_12            ((5 << 5) | ( 2 << 0))      /* PAD_P11      */
#define GPIO_GPIO_11            ((5 << 5) | ( 3 << 0))      /* PAD_R11      */
#define GPIO_GPIO_10            ((5 << 5) | ( 4 << 0))      /* PAD_T11      */
#define GPIO_GPIO_9             ((5 << 5) | ( 5 << 0))      /* PAD_U11      */
#define GPIO_GPIO_8             ((5 << 5) | ( 6 << 0))      /* PAD_N12      */

#define GPIO_MPMC_A15           ((6 << 5) | ( 0 << 0))      /* PAD_B1       */
#define GPIO_MPMC_A16           ((6 << 5) | ( 1 << 0))      /* PAD_B2       */
#define GPIO_MPMC_A17           ((6 << 5) | ( 2 << 0))      /* PAD_C1       */
#define GPIO_MPMC_A18           ((6 << 5) | ( 3 << 0))      /* PAD_C2       */
#define GPIO_MPMC_A19           ((6 << 5) | ( 4 << 0))      /* PAD_D4       */
#define GPIO_MPMC_A20           ((6 << 5) | ( 5 << 0))      /* PAD_D3       */
#define GPIO_MPMC_NSTCS0        ((6 << 5) | ( 6 << 0))      /* PAD_L2       */
#define GPIO_MPMC_NSTCS1        ((6 << 5) | ( 7 << 0))      /* PAD_L1       */
#define GPIO_MPMC_NSTCS2        ((6 << 5) | ( 8 << 0))      /* PAD_M5       */
#define GPIO_MPMC_BLOUT0        ((6 << 5) | ( 9 << 0))      /* PAD_H1       */
#define GPIO_MPMC_BLOUT1        ((6 << 5) | (10 << 0))      /* PAD_J4       */
#define GPIO_MPMC_NOE           ((6 << 5) | (11 << 0))      /* PAD_J3       */

#define GPIO_SPDIF_OUT          ((7 << 5) | ( 0 << 0))      /* PAD_D6       */

#define GPIO_NULL               ((7 << 5) | (31 << 0))      /* no external pad... */


/****************************************************************************
****************************************************************************/

/*
    Chip away at the tip of the padmux iceberg...
*/

#define GPIO_PCCARDB_NRESET     GPIO_IEEE1394_PD
#define GPIO_PCCARDB_NCS0       GPIO_IEEE1394_NRESET
#define GPIO_PCCARDB_NCS1       GPIO_IEEE1394_CTL0
#define GPIO_PCCARDB_INTRQ      GPIO_IEEE1394_CTL1
#define GPIO_PCCARDB_NDMACK     GPIO_IEEE1394_LREQ
#define GPIO_PCCARDB_DMARQ      GPIO_IEEE1394_SYSCLK
#define GPIO_PCCARDB_IORDY      GPIO_IEEE1394_D0


/****************************************************************************
****************************************************************************/

#ifndef __ASSEMBLY__

extern void  gpio_set_as_input    (unsigned int pin);
extern void  gpio_set_as_output   (unsigned int pin, unsigned int initial_value);
extern void  gpio_set_as_function (unsigned int pin);
extern int   gpio_get_value       (unsigned int pin);
extern void  gpio_set_high        (unsigned int pin);
extern void  gpio_set_low         (unsigned int pin);
extern void  gpio_set_output      (unsigned int pin, unsigned int value);
extern int   gpio_toggle_output   (unsigned int pin);
extern char *gpio_pin_to_padname  (unsigned int pin);

extern void  gpio_dump_state      (unsigned int pin);
extern void  gpio_dump_state_all  (void);

#endif

/****************************************************************************
****************************************************************************/

#endif

