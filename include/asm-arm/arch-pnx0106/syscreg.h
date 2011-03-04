/*
 *  linux/include/asm-arm/arch-pnx0106/syscreg.h
 *
 *  Copyright (C) 2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SYSCREG_H_
#define __ASM_ARCH_SYSCREG_H_

#include <asm/arch/hardware.h>

/*
 * We don't have dedicated SYSCREG driver.
 * Instead, any driver which needs to modify a SYSCREG register should
 * include this header and then simply access the desired register
 * directly with readl() or writel()
 * e.g. 
 *          readl (VA_SYSCREG_CHIP_ID)
 *          writel (0, VA_SYSCREG_ISROM_LATENCY_CFG);
 */

#define VA_SYSCREG_SPARE_REG0                   (IO_ADDRESS_SYSCREG_BASE + 0x00)
#define VA_SYSCREG_ACTIVATE_TESTPINS            (IO_ADDRESS_SYSCREG_BASE + 0x04)
#define VA_SYSCREG_IEEE1394_CONFIG              (IO_ADDRESS_SYSCREG_BASE + 0x08)
#define VA_SYSCREG_SSA1_RTC_CFG                 (IO_ADDRESS_SYSCREG_BASE + 0x0C)
#define VA_SYSCREG_SSA1_ADC_PD_ADC10BITS        (IO_ADDRESS_SYSCREG_BASE + 0x10)
#define VA_SYSCREG_SSA1_MCI_PL180_PRESERVED     (IO_ADDRESS_SYSCREG_BASE + 0x14)
#define VA_SYSCREG_ABC_CFG                      (IO_ADDRESS_SYSCREG_BASE + 0x18)
#define VA_SYSCREG_CGU_DYN_HP0                  (IO_ADDRESS_SYSCREG_BASE + 0x1C)
#define VA_SYSCREG_CGU_DYN_LP0                  (IO_ADDRESS_SYSCREG_BASE + 0x20)
#define VA_SYSCREG_CGU_DYN_LP1                  (IO_ADDRESS_SYSCREG_BASE + 0x24)
#define VA_SYSCREG_SYS_CREG_SDMA_EXT_EN_3       (IO_ADDRESS_SYSCREG_BASE + 0x28)
#define VA_SYSCREG_SYS_CREG_SDMA_EXT_EN_5       (IO_ADDRESS_SYSCREG_BASE + 0x2C)
#define VA_SYSCREG_SELECTION_CFG                (IO_ADDRESS_SYSCREG_BASE + 0x30)
#define VA_SYSCREG_USB_WAKEUP_N                 (IO_ADDRESS_SYSCREG_BASE + 0x34)
#define VA_SYSCREG_USB_1KHZ_SEL                 (IO_ADDRESS_SYSCREG_BASE + 0x38)
#define VA_SYSCREG_USB_ATX_PLL_PD_REG           (IO_ADDRESS_SYSCREG_BASE + 0x3C)
#define VA_SYSCREG_USB_OTG_CFG                  (IO_ADDRESS_SYSCREG_BASE + 0x40)
#define VA_SYSCREG_USB_OTG_PORT_IND_CTL         (IO_ADDRESS_SYSCREG_BASE + 0x44)
#define VA_SYSCREG_SYS_USB_TPR_DYN              (IO_ADDRESS_SYSCREG_BASE + 0x48)
#define VA_SYSCREG_USB_PLL_NDEC                 (IO_ADDRESS_SYSCREG_BASE + 0x4C)
#define VA_SYSCREG_USB_PLL_MDEC                 (IO_ADDRESS_SYSCREG_BASE + 0x50)
#define VA_SYSCREG_USB_PLL_PDEC                 (IO_ADDRESS_SYSCREG_BASE + 0x54)
#define VA_SYSCREG_USB_PLL_SELR                 (IO_ADDRESS_SYSCREG_BASE + 0x58)
#define VA_SYSCREG_USB_PLL_SELI                 (IO_ADDRESS_SYSCREG_BASE + 0x5C)
#define VA_SYSCREG_USB_PLL_SELP                 (IO_ADDRESS_SYSCREG_BASE + 0x60)
#define VA_SYSCREG_PCCARD_FORCE_ADDR            (IO_ADDRESS_SYSCREG_BASE + 0x64)
#define VA_SYSCREG_GDMA_CONFIG                  (IO_ADDRESS_SYSCREG_BASE + 0x68)
#define VA_SYSCREG_TV_OUT_FRAC_DIV              (IO_ADDRESS_SYSCREG_BASE + 0x6C)
#define VA_SYSCREG_TV_OUT_FRAC_BYPASS           (IO_ADDRESS_SYSCREG_BASE + 0x70)
#define VA_SYSCREG_ISRAM0_LATENCY_CFG           (IO_ADDRESS_SYSCREG_BASE + 0x74)
#define VA_SYSCREG_ISRAM1_LATENCY_CFG           (IO_ADDRESS_SYSCREG_BASE + 0x78)
#define VA_SYSCREG_ISROM_LATENCY_CFG            (IO_ADDRESS_SYSCREG_BASE + 0x7C)
#define VA_SYSCREG_AHB_MPMC_PL172_MISC          (IO_ADDRESS_SYSCREG_BASE + 0x80)
#define VA_SYSCREG_MPMP_DELAYMODES              (IO_ADDRESS_SYSCREG_BASE + 0x84)
#define VA_SYSCREG_MPMC_WAITREAD_DELAY_0        (IO_ADDRESS_SYSCREG_BASE + 0x88)
#define VA_SYSCREG_MPMC_WAITREAD_DELAY_1        (IO_ADDRESS_SYSCREG_BASE + 0x8C)
#define VA_SYSCREG_MPMC_WAITREAD_DELAY_2        (IO_ADDRESS_SYSCREG_BASE + 0x90)
#define VA_SYSCREG_MPMC_WAITREAD_DELAY_3        (IO_ADDRESS_SYSCREG_BASE + 0x94)
#define VA_SYSCREG_WIRE_EBI_MSIZE_INIT          (IO_ADDRESS_SYSCREG_BASE + 0x98)
#define VA_SYSCREG_MPMC_TESTMODE0               (IO_ADDRESS_SYSCREG_BASE + 0x9C)
#define VA_SYSCREG_MPMC_TESTMODE1               (IO_ADDRESS_SYSCREG_BASE + 0xA0)
#define VA_SYSCREG_AHB2AHB_BUS_ERROR            (IO_ADDRESS_SYSCREG_BASE + 0xA4)
#define VA_SYSCREG_AHB0_EXTPRIO                 (IO_ADDRESS_SYSCREG_BASE + 0xA8)
#define VA_SYSCREG_ARM926EJS_SHADOW_POINTER     (IO_ADDRESS_SYSCREG_BASE + 0xAC)
#define VA_SYSCREG_SLEEPSTATUS                  (IO_ADDRESS_SYSCREG_BASE + 0xB0)
#define VA_SYSCREG_CHIP_ID                      (IO_ADDRESS_SYSCREG_BASE + 0xB4)
#define VA_SYSCREG_MUX_SPI0_E7B                 (IO_ADDRESS_SYSCREG_BASE + 0xB8)
#define VA_SYSCREG_MUX_LCD_YUV                  (IO_ADDRESS_SYSCREG_BASE + 0xBC)
#define VA_SYSCREG_MUX_PLCD_MCI                 (IO_ADDRESS_SYSCREG_BASE + 0xC0)
#define VA_SYSCREG_MUX_DAI_MCI                  (IO_ADDRESS_SYSCREG_BASE + 0xC4)
#define VA_SYSCREG_MUX_DAO_IOM2_1               (IO_ADDRESS_SYSCREG_BASE + 0xC8)
#define VA_SYSCREG_MUX_GPIO_PCCARD_IOM2_0       (IO_ADDRESS_SYSCREG_BASE + 0xCC)
#define VA_SYSCREG_MUX_UART0_NCTS_PCCARD        (IO_ADDRESS_SYSCREG_BASE + 0xD0)
#define VA_SYSCREG_MUX_SPI1_PCCARD              (IO_ADDRESS_SYSCREG_BASE + 0xD4)
#define VA_SYSCREG_MUX_UART0_NRTS_PCCARD        (IO_ADDRESS_SYSCREG_BASE + 0xD8)
#define VA_SYSCREG_MUX_1394_PCCB_PCCARD_GDMA    (IO_ADDRESS_SYSCREG_BASE + 0xDC)
#define VA_SYSCREG_MUX1_1394_PCCARD_GDMA        (IO_ADDRESS_SYSCREG_BASE + 0xE0)
#define VA_SYSCREG_MUX2_1394_PCCARD_GDMA        (IO_ADDRESS_SYSCREG_BASE + 0xE4)
#define VA_SYSCREG_MUX_GPIO_PCCARD_GDMA         (IO_ADDRESS_SYSCREG_BASE + 0xE8)

#endif
