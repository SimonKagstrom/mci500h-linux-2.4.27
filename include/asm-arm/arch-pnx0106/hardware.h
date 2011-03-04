/*
 *  linux/include/asm-arm/arch-pnx0106/hardware.h
 *
 *  Copyright (C) 2002-2005 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/config.h>

#ifndef __ASSEMBLY__
extern unsigned int release_id;
extern unsigned int ffasthz;
extern unsigned int cpuclkhz;
extern unsigned int hclkhz;
extern unsigned int uartclkhz;
extern unsigned int bootstart_tsc;
#endif


/*******************************************************************************
*******************************************************************************/

#define MEM_SIZE                 (8 * 1024 * 1024)      /* Safe default MEM_SIZE value (over-ridden with param TAG or cmdline) */
#define UART_TX_FIFO_BYTES                    (64)

#define FFAST_HZ                (12 * 1000 * 1000)      /* default for ffasthz if tag not passed by bootloader */

#define TIMER0_CLK_HZ           (12 * 1000 * 1000)
#define TIMER1_CLK_HZ           (12 * 1000 * 1000)


/*******************************************************************************
  Physical addresses...

*******************************************************************************/

#define INTC_REGS_BASE                  0x00001000      /* bizarre but true... */

#define E7B_MEM_BASE                    0x10000000      /* Epics7b memory */
#define E7B_XMEM_BASE                   0x10000000      /* X-Mem */
#define E7B_DSPCTL_REGS_BASE            0x10030000      /* DSP control regs */
#define E7B_YMEM_BASE                   0x10040000      /* Y-Mem */
#define E7B_PMEM_BASE                   0x10080000      /* P-Mem */
#define E7B_PCACHE_BASE                 0x10084000      /* P-Cache */
#define E7B_VXMEM_BASE                  0x100C0000      /* virtual X-Mem */
#define E7B_DIO_REGS_BASE               0x100FFC00      /* DIO regs */
#define E7B_DMA_REGS_BASE               0x100FFFF8      /* DMA regs */
#define E7B_MEM_LIMIT                   0x10100000
#define E7B_MEM_SIZE                    (E7B_MEM_LIMIT - E7B_MEM_BASE)

#define INTERNAL_SRAM_BASE              0x11000000      /* */
#define INTERNAL_SRAM_SIZE              (96 * 1024)     /* */

#define INTERNAL_ROM_BASE               0x12000000      /* */
#define INTERNAL_ROM_SIZE               (64 * 1024)     /* */

#define VPB0_PHYS_BASE                  0x13000000
#define EVENT_ROUTER_REGS_BASE          0x13000000
#define RTC_REGS_BASE                   0x13002000
#define ADC_REGS_BASE                   0x13002400
#define WATCHDOG_REGS_BASE              0x13002800
#define SYSCREG_REGS_BASE               0x13002C00
#define IOCONF_REGS_BASE                0x13003000
#define CGU_REGS_BASE                   0x13004000      /* CGU Switchbox */
#define CGU_CONFIG_REGS_BASE            0x13004C00      /* Other misc config (soft resets etc) */
#define CGU_HPPLL0_REGS_BASE            0x13004D00
#define CGU_LPPLL0_REGS_BASE            0x13004D38
#define CGU_LPPLL1_REGS_BASE            0x13004D54
#define VPB0_PHYS_LIMIT                 0x13008000      /* VPB0 covers a 32k range (but VPB1 follows directly beyond) */

#define VPB1_PHYS_BASE                  0x13008000
#define TIMER0_REGS_BASE                0x13008000
#define TIMER1_REGS_BASE                0x13008400
#define IIC0_REGS_BASE                  0x13008800
#define IIC1_REGS_BASE                  0x13008C00
#define VPB1_PHYS_LIMIT                 0x13009000      /* VPB1 covers a 4k range */

#define VPB2_PHYS_BASE                  0x15000000
#define LCDIF_REGS_BASE                 0x15000800
#define MCI_REGS_BASE                   0x15001000
#define UART0_REGS_BASE                 0x15002000
#define UART1_REGS_BASE                 0x15003000
#define SPI0_REGS_BASE                  0x15004000
#define SPI1_REGS_BASE                  0x15005000
#define SDMA_REGS_BASE                  0x15006000
#define VPB2_PHYS_LIMIT                 0x15008000      /* VPB2 covers a 32k range */

#define SPIUART_REGS_BASE_DUMMY         0x15100000      /* serial core requires an address to associate with each uart... so make something up */
#define NULLUART_REGS_BASE_DUMMY        0x15200000      /* serial core requires an address to associate with each uart... so make something up */

#define VPB3_PHYS_BASE                  0x16000000
#define AUDIOSS_REGS_BASE		0x16000380
#define AUDIOSS_CACHE_REGS_BASE		0x16000400
#define VPB3_PHYS_LIMIT                 0x16001000      /* VPB3 covers a 2k range */

#define MPMC_NCS0_BASE                  0x20000000      /* also mapped to 0x40000000 */
#define MPMC_NCS1_BASE                  0x24000000      /* also mapped to 0x44000000 */
#define MPMC_NCS2_BASE                  0x28000000      /* also mapped to 0x48000000 */
#define MPMC_NCS3_BASE                  0x2C000000      /* also mapped to 0x4C000000 */

#define SDRAM_BANK0_BASE                0x30000000      /* also mapped to 0x50000000 */

#define USB_REGS_BASE                   0x80000000
#define USB_OTG_REGS_BASE               0x80002000
#define GDMA_REGS_BASE                  0x80003000
#define IEEE1394_REGS_BASE              0x80003400
#define TVOUT_REGS_BASE                 0x80004000
#define MPMC_PL172_REGS_BASE            0x80005000

#define PCCARD_COMMON_PAGEFRAME_BASE    0x90000000      /* 64 MByte window */
#define PCCARD_IO_PAGEFRAME_BASE        0x94000000      /* 64 MByte window */
#define PCCARD_ATTRIB_PAGEFRAME_BASE    0x98000000      /* 64 MByte window */
#define PCCARD_REGS_BASE                0x9C000000

#define MPMC_FLASH_BASE                 MPMC_NCS0_BASE

#define VPB0_SIZE                       (VPB0_PHYS_LIMIT - VPB0_PHYS_BASE)
#define VPB1_SIZE                       (VPB1_PHYS_LIMIT - VPB1_PHYS_BASE)
#define VPB2_SIZE                       (VPB2_PHYS_LIMIT - VPB2_PHYS_BASE)
#define VPB3_SIZE                       (VPB3_PHYS_LIMIT - VPB3_PHYS_BASE)


/*******************************************************************************
  Virtual addresses...

  Note: Certain IO addresses are used directly by assembler code
  which assumes they are aligned on a 16MByte boundary...

*******************************************************************************/

#define IO_ADDRESS_INTC_BASE            0xF0000000      /* 16MByte boundary required */
#define IO_ADDRESS_FLASH_BASE           0xF1000000      /* requires 4MByte window (or more ??) */
#define IO_ADDRESS_ISRAM_BASE           0xF2000000
#define IO_ADDRESS_VPB0_BASE            0xF3000000
#define IO_ADDRESS_VPB1_BASE            0xF4000000
#define IO_ADDRESS_VPB2_BASE            0xF5000000
#define IO_ADDRESS_VPB3_BASE            0xF6000000
#define IO_ADDRESS_PCCARD_BASE          0xF7000000
#define IO_ADDRESS_PCCARD_COMMON_BASE   0xF8000000      /* space is 64 MBytes, but we map less */
#define IO_ADDRESS_PCCARD_ATTRIB_BASE   0xF9000000      /* space is 64 MBytes, but we map less */
#define IO_ADDRESS_PCCARD_IO_BASE       0xFA000000      /* space is 64 MBytes, but we map less */
#define IO_ADDRESS_CPLD_BASE            0xFB000000
#define IO_ADDRESS_E7B_BASE             0xFC000000

#define IO_ADDRESS_EVENT_ROUTER_BASE    (IO_ADDRESS_VPB0_BASE + (EVENT_ROUTER_REGS_BASE - VPB0_PHYS_BASE))
#define IO_ADDRESS_ADC_BASE             (IO_ADDRESS_VPB0_BASE + (ADC_REGS_BASE          - VPB0_PHYS_BASE))
#define IO_ADDRESS_WATCHDOG_BASE        (IO_ADDRESS_VPB0_BASE + (WATCHDOG_REGS_BASE     - VPB0_PHYS_BASE))
#define IO_ADDRESS_SYSCREG_BASE         (IO_ADDRESS_VPB0_BASE + (SYSCREG_REGS_BASE      - VPB0_PHYS_BASE))
#define IO_ADDRESS_IOCONF_BASE          (IO_ADDRESS_VPB0_BASE + (IOCONF_REGS_BASE       - VPB0_PHYS_BASE))
#define IO_ADDRESS_CGU_REGS_BASE        (IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE          - VPB0_PHYS_BASE))
#define IO_ADDRESS_CGU_CONFIG_REGS_BASE (IO_ADDRESS_VPB0_BASE + (CGU_CONFIG_REGS_BASE   - VPB0_PHYS_BASE))
#define IO_ADDRESS_CGU_HPPLL0_REGS_BASE (IO_ADDRESS_VPB0_BASE + (CGU_HPPLL0_REGS_BASE   - VPB0_PHYS_BASE))
#define IO_ADDRESS_CGU_LPPLL0_REGS_BASE (IO_ADDRESS_VPB0_BASE + (CGU_LPPLL0_REGS_BASE   - VPB0_PHYS_BASE))
#define IO_ADDRESS_CGU_LPPLL1_REGS_BASE (IO_ADDRESS_VPB0_BASE + (CGU_LPPLL1_REGS_BASE   - VPB0_PHYS_BASE))

#define IO_ADDRESS_TIMER0_BASE          (IO_ADDRESS_VPB1_BASE + (TIMER0_REGS_BASE       - VPB1_PHYS_BASE))
#define IO_ADDRESS_TIMER1_BASE          (IO_ADDRESS_VPB1_BASE + (TIMER1_REGS_BASE       - VPB1_PHYS_BASE))
#define IO_ADDRESS_IIC0_BASE            (IO_ADDRESS_VPB1_BASE + (IIC0_REGS_BASE         - VPB1_PHYS_BASE))
#define IO_ADDRESS_IIC1_BASE            (IO_ADDRESS_VPB1_BASE + (IIC1_REGS_BASE         - VPB1_PHYS_BASE))

#define IO_ADDRESS_LCDIF_BASE           (IO_ADDRESS_VPB2_BASE + (LCDIF_REGS_BASE        - VPB2_PHYS_BASE))
#define IO_ADDRESS_MCI_BASE             (IO_ADDRESS_VPB2_BASE + (MCI_REGS_BASE          - VPB2_PHYS_BASE))
#define IO_ADDRESS_UART0_BASE           (IO_ADDRESS_VPB2_BASE + (UART0_REGS_BASE        - VPB2_PHYS_BASE))
#define IO_ADDRESS_UART1_BASE           (IO_ADDRESS_VPB2_BASE + (UART1_REGS_BASE        - VPB2_PHYS_BASE))
#define IO_ADDRESS_SPI0_BASE            (IO_ADDRESS_VPB2_BASE + (SPI0_REGS_BASE         - VPB2_PHYS_BASE))
#define IO_ADDRESS_SPI1_BASE            (IO_ADDRESS_VPB2_BASE + (SPI1_REGS_BASE         - VPB2_PHYS_BASE))
#define IO_ADDRESS_SDMA_BASE            (IO_ADDRESS_VPB2_BASE + (SDMA_REGS_BASE         - VPB2_PHYS_BASE))

#define IO_ADDRESS_AUDIOSS_BASE		(IO_ADDRESS_VPB3_BASE + (AUDIOSS_REGS_BASE - VPB3_PHYS_BASE))
#define IO_ADDRESS_AUDIOSS_CACHE_BASE	(IO_ADDRESS_VPB3_BASE + (AUDIOSS_CACHE_REGS_BASE - VPB3_PHYS_BASE))


/*******************************************************************************
  Interrupt controller IRQ numbers

*******************************************************************************/

#define IRQ_SPURIOUS                    0           /* INTC HW design means this can never happen... */
#define IRQ_IEEE1394                    1
#define IRQ_EVENTROUTER_IRQ_0           2
#define IRQ_EVENTROUTER_IRQ_1           3
#define IRQ_EVENTROUTER_IRQ_2           4
#define IRQ_EVENTROUTER_IRQ_3           5
#define IRQ_TIMER_0                     6
#define IRQ_TIMER_1                     7
#define IRQ_RTC_INC                     8
#define IRQ_RTC_ALARM                   9           /* */
#define IRQ_ADC                         10
#define IRQ_MCI_0                       11
#define IRQ_MCI_1                       12
#define IRQ_UART_0                      13
#define IRQ_UART_1                      14
#define IRQ_IIC_0                       15
#define IRQ_IIC_1                       16
#define IRQ_SPI_0                       17
#define IRQ_SPI_1                       18
#define IRQ_E7B2ARM_1                   19
#define IRQ_E7B2ARM_2                   20
#define IRQ_SAI_1                       21
#define IRQ_SAI_2                       22
#define IRQ_SAI_3                       23
#define IRQ_SAI_4                       24
#define IRQ_SAO_1                       25
#define IRQ_SAO_2                       26
#define IRQ_SAO_3                       27
#define IRQ_SPDIF                       28
#define IRQ_LCD                         29
#define IRQ_SDMA                        30
#define IRQ_USB_IRQ                     31
#define IRQ_USB_FIQ                     32
#define IRQ_USB_CHANNEL_0               33
#define IRQ_USB_CHANNEL_1               34
#define IRQ_USB_OTG_IRQ                 35
#define IRQ_PCCARD                      36
#define IRQ_GDMA                        37
#define IRQ_TVOUT                       38

#define NR_INTC_IRQS                    39          /* */

#define FIQ_START                       64          /* offset of the FIQ "IRQ" numbers */


/*******************************************************************************
  GPIO definitions...

*******************************************************************************/

#include <asm/arch/gpio.h>


/*******************************************************************************
*******************************************************************************/

#if defined (CONFIG_PNX0106_GH526)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "Philips GH526 PNX0106 development board"

#define CS8900_REGS_BASE                    (MPMC_NCS2_BASE + (1 << 21))    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /*          (all that PCB space... but still only one LED...) */
#define GPIO_ETH_RESET                      GPIO_DAO_DATA       /* output : Drive high to force CS8900 into reset. 100k pull-up */
#define GPIO_ETH_INTRQ                      GPIO_DAI_DATA       /* input  : Level triggered, active high */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N       /* output : */

/* Dummy WLAN GPIO definitions to allow SPI driver to compile... */

#define GPIO_WLAN_RESET                     GPIO_NULL           /* output : drive low to force BGW2xx into reset. 10k pull-down */
#define GPIO_WLAN_INTRQ                     GPIO_NULL           /* input  : asserted high */
#define GPIO_WLAN_NCS                       GPIO_NULL           /* output : */

#define LED_ON                              1                   /* */
#define LED_OFF                             0                   /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_2

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI0_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI1_BASE
#define GPIO_SPISD_NCS                      GPIO_SPINOR_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_WSA)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "WSA PNX0106 development board"

#define AX88796_REGS_BASE                   (MPMC_NCS1_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /* */
#define GPIO_ETH_RESET                      GPIO_GPIO_3         /* output : Drive high to force AX88796 into reset. 100k pull-up (and internal) */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5    /* input  : Level triggered, active high */

#if 1
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : drive low to force BGW2xx into reset. 10k pull-down */
#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : asserted high */
#define GPIO_WLAN_NCS                       GPIO_SPI1_S_N       /* output : Routed to BGW2xx */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N       /* output : Routed to SPINOR device on factory programming connector */
#else

/* debug : allow connection to external BGW via SPI-NOR pins */

#define GPIO_WLAN_RESET                     GPIO_GPIO_1         /* output : drive low to force BGW2xx into reset. 10k pull-down */
#define GPIO_WLAN_INTRQ                     GPIO_GPIO_0         /* input  : asserted high */
#define GPIO_WLAN_NCS                       GPIO_SPI0_S_N       /* output : Routed to BGW2xx */
#endif

#define LED_ON                              1                   /* */
#define LED_OFF                             0                   /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_2

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI0_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI1_BASE
#define GPIO_SPISD_NCS                      GPIO_SPINOR_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_LAGUNA_REVA)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "Philips Laguna (Rev A) PNX0106 development board"

#define AX88796_REGS_BASE                   (MPMC_NCS0_BASE + (1 << 21))                /* physical address */
#define MPMC_NAND_BASE                      (MPMC_NCS1_BASE + (0 << 20) + (0 << 19))    /* physical address */
#define MPMC_LCD_BASE                       (MPMC_NCS1_BASE + (0 << 20) + (1 << 19))    /* physical address (A18 == 0 -> LCD Register, A18 == 1 -> LCD Data) */
#define MPMC_UART_BASE                      (MPMC_NCS1_BASE + (1 << 20) + (0 << 19))    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /* */
#define GPIO_ETH_RESET                      GPIO_GPIO_3         /* output : Drive high to force AX88796 into reset. 100k pull-up (and internal) */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5    /* input  : Level triggered, active high */
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : drive low to force BGW2xx into reset. 10k pull-down */
#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : asserted high */
#define GPIO_WLAN_NCS                       GPIO_SPI1_S_N       /* output : Routed to BGW2xx on SPI1*/
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N       /* output : Routed to SPINOR device on SPI0 */

#if defined (CONFIG_PNX0106_PCCARD)
#define GPIO_PCMCIA_INTRQ                   GPIO_PCCARDA_INTRQ  /* input  : Level triggered, active low */
#define GPIO_PCMCIA_RESET                   GPIO_PCCARDA_NRESET /* output : Drive high to force PCMCIA into reset. 10k pull-?? */
#define GPIO_ATA_RESET                      GPIO_PCCARDB_NRESET /* output : Drive low to hold ATA devices in reset. 10k pull-?? */
#else
#define GPIO_PCMCIA_INTRQ                   GPIO_NULL
#define GPIO_PCMCIA_RESET                   GPIO_PCCARDB_NRESET /* output : not connected to anything */
#define GPIO_ATA_RESET                      GPIO_PCCARDA_NRESET /* output : Drive high to force PCMCIA into reset. 10k pull-?? */
#endif

#define GPIO_LCD_RESET                      GPIO_UART0_NCTS     /* output : drive low to force LCD controller reset. ??k pull-?? */
#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_19        /* output : drive high to light LCD backlight */
/*
    PDCC modified versions of the Laguna board re-use the
    LCD backlight GPIO as the RC6 receiver input...
*/
#define GPIO_RC6RX                          GPIO_GPIO_19        /* input  : */

#define GPIO_KEYPAD_IN_0                    GPIO_IEEE1394_D1    /* input  : 12k pull-down */
#define GPIO_KEYPAD_IN_1                    GPIO_IEEE1394_D2    /* input  : 12k pull-down */
#define GPIO_KEYPAD_IN_2                    GPIO_IEEE1394_D3    /* input  : 12k pull-down */
#define GPIO_KEYPAD_OUT_0                   GPIO_LCD_DB0        /* output : should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_1                   GPIO_LCD_RW_WR      /* output : should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_2                   GPIO_LCD_E_RD       /* output : should be high by default (when waiting for key press) */

#define GPIO_USB_POWER                      GPIO_GPIO_22        /* output : drive high to turn on power to USB. 47k pull-down */

#define USB_POWER_ON                        1                   /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       0                   /* definition for use with GPIO_USB_POWER */

#define LED_ON                              1                   /* */
#define LED_OFF                             0                   /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_KEYPAD                          IRQ_EVENTROUTER_IRQ_2

//#define IRQ_PCMCIA                        IRQ_EVENTROUTER_IRQ_3   /* temp hack... PCMCIA, RC6 and EPICS must not be enabled together... */
//#define IRQ_RC6RX                         IRQ_EVENTROUTER_IRQ_3   /* temp hack... PCMCIA, RC6 and EPICS must not be enabled together... */
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_3   /* temp hack... PCMCIA, RC6 and EPICS must not be enabled together... */

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI1_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SPINOR_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_LPAS1)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "LPAS1 PNX0106 development board"

#define MPMC_LCD_BASE                       (MPMC_NCS1_BASE)    /* physical address A18 == 0 -> LCD Register, A18 == 1 -> LCD Data */
#define AX88796_REGS_BASE                   (MPMC_NCS2_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /* */

#define GPIO_AMP_GAIN                       GPIO_IEEE1394_D5    /* output : Select AMP gain, high for max, low for min */
#define GPIO_AMP_MUTE                       GPIO_IEEE1394_D6    /* output : Drive high to mute AMP output */
#define GPIO_AMP_POWER                      GPIO_IEEE1394_D7    /* output : Drive high to turn AMP power on */
#define GPIO_AUDIO_IN_SEL                   GPIO_GPIO_16        /* output : Select audio-in source, high for line-in, low for FM */
#define GPIO_ETH_INTRQ                      GPIO_GPIO_19        /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_17        /* output : Drive high to force AX88796 into reset */
#define GPIO_FLASH_A21                      GPIO_GPIO_4         /* output : Flash bank selection upper or lower 4MB (47k pull-down) */
#define GPIO_FM_BUS_ENABLE                  GPIO_IEEE1394_D3    /* output : Driven high to activate the access to FM chip via I2C-1 */

#if 0
#define GPIO_KEYPAD_IN_0                    GPIO_IEEE1394_D1    /* input  : Driven high when key is pressed (12k pull-down) */
#define GPIO_KEYPAD_IN_1                    GPIO_IEEE1394_D2    /* input  : Driven high when key is pressed (12k pull-down) */
#define GPIO_KEYPAD_OUT_0                   GPIO_LCD_DB1        /* output : Should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_1                   GPIO_LCD_DB2        /* output : Should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_2                   GPIO_LCD_DB3        /* output : Should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_3                   GPIO_LCD_DB4        /* output : Should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_4                   GPIO_LCD_DB5        /* output : Should be high by default (when waiting for key press) */
#endif

#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_20        /* output : Drive high to light LCD backlight */
#define GPIO_LCD_BIT_WIDE                   GPIO_GPIO_13        /* output : Set LCD-I/F bit wide, high for 16-bit, low for 8-bit */
#define GPIO_LCD_RESET                      GPIO_UART0_NCTS     /* output : Drive high low to force LCD into reset */
#define GPIO_LINE_IN_STAT                   GPIO_GPIO_18        /* input  : Line-in plug present when driven low */
#define GPIO_RC6RX                          GPIO_IEEE1394_LPS   /* input  : Driven low when IR carrier reception is detected */
#define GPIO_RTC_INTRQ_A                    GPIO_GPIO_12        /* input  : Can be programmed as level triggered, active low */
#define GPIO_RTC_INTRQ_B                    GPIO_GPIO_14        /* input  : Can be programmed as level triggered, active low */
#define GPIO_SD_DETECT                      GPIO_IEEE1394_D0    /* input  : Card detected when driven low */
#define GPIO_SD_PROTECT                     GPIO_GPIO_6         /* input  : Write prohibit when driven high */
#define GPIO_SD_SPI_NCS                     GPIO_SPI0_S_N       /* output : ?? */
#define GPIO_USB_POWER                      GPIO_GPIO_22        /* output : Drive low to turn USB-VBUS power on */
#define GPIO_USB_POWER_FAULT                GPIO_GPIO_21        /* input  : Driven low when the USB-VBUS power switch is in fault */
#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : Level triggered, active high? */
#define GPIO_WLAN_NCS                       GPIO_SPI1_S_N       /* output : Chip select for BGW211, active low */
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : Drive low to force BGW211 into reset */

#define USB_POWER_ON                        0                   /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       1                   /* definition for use with GPIO_USB_POWER */

#define LED_ON                              1                   /* */
#define LED_OFF                             0                   /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_2
#define IRQ_RC6RX                           IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI1_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SD_SPI_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_LDMA1)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "LDMA1 PNX0106 development board"

#define MPMC_LCD_BASE                       (MPMC_NCS1_BASE)    /* physical address A18 == 0 -> LCD Register, A18 == 1 -> LCD Data */
#define AX88796_REGS_BASE                   (MPMC_NCS2_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /* */

#define GPIO_AMP_GAIN                       GPIO_IEEE1394_D5    /* output : Select AMP gain, high for max, low for min */
#define GPIO_AMP_MUTE                       GPIO_IEEE1394_D6    /* output : Drive high to mute AMP output */
#define GPIO_AMP_POWER                      GPIO_IEEE1394_D7    /* output : Drive high to turn AMP power on */
#define GPIO_AUDIO_IN_SEL                   GPIO_GPIO_16        /* output : Select audio-in source, high for line-in, low for FM */
#define GPIO_ETH_INTRQ                      GPIO_GPIO_19        /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_17        /* output : Drive high to force AX88796 into reset */
#define GPIO_FLASH_A21                      GPIO_GPIO_4         /* output : Flash bank selection upper or lower 4MB (47k pull-down) */
#define GPIO_FM_BUS_ENABLE                  GPIO_IEEE1394_D3    /* output : Driven high to activate the access to FM chip via I2C-1 */

#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_20        /* output : Drive high to light LCD backlight */
#define GPIO_LCD_BIT_WIDE                   GPIO_GPIO_13        /* output : Set LCD-I/F bit wide, high for 16-bit, low for 8-bit */
#define GPIO_LCD_RESET                      GPIO_UART0_NCTS     /* output : Drive high low to force LCD into reset */
#define GPIO_LINE_IN_STAT                   GPIO_GPIO_18        /* input  : Line-in plug present when driven low */
#define GPIO_RC6RX                          GPIO_IEEE1394_LPS   /* input  : Driven low when IR carrier reception is detected */
#define GPIO_RTC_INTRQ_A                    GPIO_GPIO_12        /* input  : Can be programmed as level triggered, active low */
#define GPIO_RTC_INTRQ_B                    GPIO_GPIO_14        /* input  : Can be programmed as level triggered, active low */
#define GPIO_SD_DETECT                      GPIO_IEEE1394_D0    /* input  : Card detected when driven low */
#define GPIO_SD_PROTECT                     GPIO_GPIO_6         /* input  : Write prohibit when driven high */
#define GPIO_SD_SPI_NCS                     GPIO_SPI0_S_N       /* output : ?? */
#define GPIO_USB_POWER                      GPIO_GPIO_22        /* output : Drive low to turn USB-VBUS power on */
#define GPIO_USB_POWER_FAULT                GPIO_GPIO_21        /* input  : Driven low when the USB-VBUS power switch is in fault */
#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : Level triggered, active high? */
#define GPIO_WLAN_NCS                       GPIO_SPI1_S_N       /* output : Chip select for BGW211, active low */
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : Drive low to force BGW211 into reset */

#define USB_POWER_ON                        0                   /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       1                   /* definition for use with GPIO_USB_POWER */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_2
#define IRQ_RC6RX                           IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI1_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SD_SPI_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_RSS1)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "RSS1 PNX8706"

#define AX88796_REGS_BASE                   (MPMC_NCS2_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_8         /* output : Drive high to light LED */
#define GPIO_LED_HEARTBEAT                  GPIO_GPIO_9         /* output : Drive high to light LED */

#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5    /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_3         /* output : Drive low to force AX88796 Rev B into reset. ??k pull-?? */
#define GPIO_FLASH_A21                      GPIO_GPIO_4         /* output : 47k pull-down */
#define GPIO_SLAVE_RESET                    GPIO_GPIO_6         /* output : Drive high to de-assert, 47k pull-down */
#define GPIO_TSYNC                          GPIO_GPIO_12        /* output on CPU 0, input on CPUs 1..5. no pull-up or pull-down */
#define GPIO_PLC_PHY_AS3                    GPIO_GPIO_16        /*	  : */
#define GPIO_PLC_PHY_AS4                    GPIO_GPIO_17        /*	  : */
#define GPIO_PLC_RESET                      GPIO_GPIO_18        /*	  : 47k pull-down */
#define GPIO_KS8999_RESET                   GPIO_GPIO_19        /* output : Drive high to de-assert (CPU 0 only, others unconnected), 47k pull-down */
#define GPIO_RUN                            GPIO_GPIO_21        /*	  : 47k pull-down */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N       /* output : */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0

#define NR_IRQS                             NR_INTC_IRQS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_RSC1)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "RSC1 PNX8706"

#define AX88796_REGS_BASE                   (MPMC_NCS2_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_8         /* output : Drive high to light LED */
#define GPIO_LED_HEARTBEAT                  GPIO_GPIO_9         /* output : Drive high to light LED */

#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5    /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_3         /* output : Drive low to force AX88796 Rev B into reset. ??k pull-?? */
#define GPIO_FLASH_A21                      GPIO_GPIO_4         /* output : 47k pull-down */
#define GPIO_CONSOLE_MODE                   GPIO_GPIO_6         /*	  : */
#define GPIO_PA_MODE0                       GPIO_GPIO_10        /*	  : */
#define GPIO_PA_MODE1                       GPIO_GPIO_11        /*	  : */
#define GPIO_LINK_LED0                      GPIO_GPIO_13        /*	  : */
#define GPIO_LINK_LED1                      GPIO_GPIO_14        /*	  : */
#define GPIO_UI_RESET                       GPIO_GPIO_15        /*	  : Active low */
#define GPIO_PLC_PHY_AS3                    GPIO_GPIO_16        /*	  : */
#define GPIO_PLC_PHY_AS4                    GPIO_GPIO_17        /*	  : */
#define GPIO_PLC_RESET                      GPIO_GPIO_18        /*	  : 47k pull-down */

#define GPIO_LOCAL_REMOTE                   GPIO_GPIO_29        /*	  : */
#define GPIO_PROGRAM                        GPIO_GPIO_21        /*	  : */

#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : Level triggered, active high ? */
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : Drive low to force BGW211 into reset */
#define GPIO_WLAN_NCS                       GPIO_SPI0_EXT       /* output : Chip select for BGW211, active low */

#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N       /* output : */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1

#define NR_IRQS                             NR_INTC_IRQS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_WMA100)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "WMA-100 PNX0106 development board"

#define MPMC_LCD_BASE                       (MPMC_NCS1_BASE)    /* physical address */
#define AX88796_REGS_BASE                   (MPMC_NCS2_BASE)    /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2         /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL           /* */
#define GPIO_ETH_RESET                      GPIO_GPIO_3         /* output : Drive high to force AX88796 into reset. 100k pull-up (and internal) */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5    /* input  : Level triggered, active high */
#define GPIO_FLASH_A21                      GPIO_GPIO_4         /* output : 47k pull-down */
#define GPIO_WLAN_RESET                     GPIO_SPI1_EXT       /* output : drive low to force BGW2xx into reset. 10k pull-down */
#define GPIO_WLAN_INTRQ                     GPIO_IEEE1394_D4    /* input  : asserted high */
#define GPIO_WLAN_NCS                       GPIO_SPI1_S_N       /* output : Routed to BGW2xx on SPI1*/

#define GPIO_ATA_RESET                      GPIO_PCCARDA_NRESET /* output : Drive high to force PCMCIA into reset. 10k pull-?? */

#define GPIO_LCD_RESET                      GPIO_UART0_NCTS     /* output : drive low to force LCD controller reset. ??k pull-?? */
#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_19        /* output : drive high to light LCD backlight */

#define GPIO_KEYPAD_IN_0                    GPIO_IEEE1394_D1    /* input  : 12k pull-down */
#define GPIO_KEYPAD_IN_1                    GPIO_IEEE1394_D2    /* input  : 12k pull-down */
#define GPIO_KEYPAD_IN_2                    GPIO_IEEE1394_D3    /* input  : 12k pull-down */
#define GPIO_KEYPAD_OUT_0                   GPIO_LCD_DB3        /* output : should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_1                   GPIO_LCD_DB2        /* output : should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_2                   GPIO_LCD_DB1        /* output : should be high by default (when waiting for key press) */

#define GPIO_SPINOR_NCS                     GPIO_NULL           /* output : ?? */

#define GPIO_USB_POWER                      GPIO_GPIO_22        /* output : drive low to turn on power to USB. ??k pull-?? */
#define GPIO_USB_POWER_FAULT                GPIO_IEEE1394_D6    /* input  : ??? */

#define USB_POWER_ON                        0                   /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       1                   /* definition for use with GPIO_USB_POWER */

#define LED_ON                              1                   /* */
#define LED_OFF                             0                   /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_WLAN                            IRQ_EVENTROUTER_IRQ_1
#define IRQ_KEYPAD                          IRQ_EVENTROUTER_IRQ_2
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI1_BASE
#define GPIO_SPI_NCS                        GPIO_WLAN_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SPINOR_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_W6)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "Philips WessLi-6 PNX0106 A-Hub Client/Server board"

#define AX88796_REGS_BASE                   (MPMC_NCS1_BASE)        /* physical address */
#define CPLD_REGS_BASE                      (MPMC_NCS2_BASE)        /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2             /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL               /* */
#define GPIO_LED_ECOSTANDBY                 GPIO_SPI1_S_N           /* output : Drive low: off, drive high: green, tri-state: red */
#define GPIO_ATA_RESET                      GPIO_PCCARDA_NRESET     /* output : Drive low to hold ATA devices in reset. 10k pull-down */
#define GPIO_CPLD_RESET                     GPIO_IEEE1394_D0        /* output : drive low to hold CPLD logic in reset. 10k pull-down */
#define GPIO_CPLD_TCK                       GPIO_IEEE1394_NRESET    /* output : 10k pull-down */
#define GPIO_CPLD_TDI                       GPIO_IEEE1394_LREQ      /* output : 10k pull-up */
#define GPIO_CPLD_TDO                       GPIO_IEEE1394_SYSCLK    /* input  : 10k pull-up */
#define GPIO_CPLD_TMS                       GPIO_IEEE1394_CTL0      /* output : 10k pull-up */
#define GPIO_ECO_STBY                       GPIO_SPI1_SCK           /* output : drive low to stop main 5v and 12v suppies. 47k pull-up */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5        /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_3             /* output : Drive high to force AX88796 into reset. 100k pull-up */
#define GPIO_FLASH_A21                      GPIO_SPI0_EXT           /* output : 47k pull-down */
#define GPIO_FLASH_A22                      GPIO_SPI1_EXT           /* output : 47k pull-down */
#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_20            /* output : drive high to light LCD backlight */
#define GPIO_LCD_RESET                      GPIO_UART0_NCTS         /* output : drive low to force LCD controller reset. 10k pull-up */
#define GPIO_PCMCIA_INTRQ                   GPIO_IEEE1394_CTL1      /* input  : Level triggered, active low */
#define GPIO_PCMCIA_RESET                   GPIO_IEEE1394_PD        /* output : Drive high to force PCMCIA into reset. 10k pull-up */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N           /* output : Routed to SPINOR device on factory programming connector */
#define GPIO_MUTE                           GPIO_SPI1_MISO          /* output : drive high to mute audio (W6 Production boards) */
#define GPIO_MUTE_OLD                       GPIO_IEEE1394_D1        /* output : drive high to mute audio (W6 TR boards and before) */
#define GPIO_RDS_INTRQ                      GPIO_IEEE1394_D4	    /* input  : */
#define GPIO_SOURCE_SELECT1                 GPIO_DAO_BCK            /* output : */
#define GPIO_SOURCE_SELECT2                 GPIO_DAO_DATA           /* output : 00=FM 01=CD 10=PNX0106 11=AUX*/
#define GPIO_USB_POWER                      GPIO_GPIO_22            /* output : drive low to turn on power to USB. ??k pull-?? */
#define GPIO_USB_POWER_FAULT                GPIO_GPIO_21	    /* input  : Driven low when the USB-VBUS power switch is in fault */
#define GPIO_RC6RX                          GPIO_IEEE1394_LPS       /* input  : */
#define GPIO_V2                             GPIO_GPIO_19            /* input  : platform config... */
#define GPIO_V3                             GPIO_DAO_CLK            /* input  : V2/V3: 0/0=USA, 0/1=China/Korea, 1/0=EU with RDS, 1/1=EU no RDS */
#define GPIO_WLAN_POWERDOWN                 GPIO_IEEE1394_D3        /* output : drive low to turn off power to WiFi card */

#define USB_POWER_ON                        0                       /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       1                       /* definition for use with GPIO_USB_POWER */

#define LED_ON                              1                       /* */
#define LED_OFF                             0                       /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_PCMCIA                          IRQ_EVENTROUTER_IRQ_1
#define IRQ_RC6RX                           IRQ_EVENTROUTER_IRQ_2
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI0_BASE
#define GPIO_SPI_NCS                        GPIO_SPINOR_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SPINOR_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_HASLI7) || \
	  defined (CONFIG_PNX0106_MCIH) || \
	  defined (CONFIG_PNX0106_MCI)


/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "Philips HasLi-7 PNX0106 A-Hub Server board"

#define AX88796_REGS_BASE                   (MPMC_NCS1_BASE)        /* physical address */
#define CPLD_REGS_BASE                      (MPMC_NCS2_BASE)        /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2 	    /* output : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL		    /* */
#define GPIO_LED_ECOSTANDBY                 GPIO_GPIO_13	    /* output : Drive low: off, drive high: green, tri-state: red */
#define GPIO_LED_WIFI                       GPIO_GPIO_17	    /* output : Drive low: off, drive high: on */

#define GPIO_POWER_AMP                      GPIO_DAI_DATA	    /* output : Drive low: AMP off, drive high: AMP on. */
#define GPIO_ATA_RESET                      GPIO_PCCARDA_NRESET     /* output : Drive low to hold ATA devices in reset. 10k pull-down */
#define GPIO_CPLD_RESET                     GPIO_IEEE1394_D0	    /* output : drive low to hold CPLD logic in reset. 10k pull-down */
#define GPIO_CPLD_TCK                       GPIO_IEEE1394_NRESET    /* output : 10k pull-down */
#define GPIO_CPLD_TDI                       GPIO_IEEE1394_LREQ      /* output : 10k pull-up */
#define GPIO_CPLD_TDO                       GPIO_IEEE1394_SYSCLK    /* input  : 10k pull-up */
#define GPIO_CPLD_TMS                       GPIO_IEEE1394_CTL0      /* output : 10k pull-up */
#define GPIO_DEVICE_ID                      GPIO_GPIO_4 	    /* input  : low: go-gear, high: ipod, hi-z: unknown */
#define GPIO_DISABLE_RC                     GPIO_GPIO_9 	    /* output : Drive low: disable, drive high: enable */
#define GPIO_ECO_CORE                       GPIO_IEEE1394_D3	    /* output : drive low to stop 3v3 supply to WiFi etc (but shouldn't be used). 47k pull-up */
#define GPIO_ECO_CORE_OPTION                GPIO_GPIO_14	    /* input  : if connected to ECO_CORE, output if not (initial WAC35000 board is connected) */
#define GPIO_ECO_STBY                       GPIO_GPIO_6 	    /* output : drive low to turn off power to WiFi card */
#define GPIO_ECO_STBY_OPTION                GPIO_IEEE1394_CPS	    /* output : drive low for test purposes (should be open on PCB) */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5	    /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_3 	    /* output : Drive low to force AX88796 Rev B into reset. ??k pull-?? */
#define GPIO_FAN                            GPIO_DAI_WS 	    /* output : Drive low: fan off, drive high: fan on. ??k pull-?? */
#define GPIO_FLASH_A21                      GPIO_SPI0_EXT	    /* output : 47k pull-down */
#define GPIO_FLASH_A22                      GPIO_SPI1_EXT	    /* output : 47k pull-down */
#define GPIO_FM_INTRQ                       GPIO_IEEE1394_D1	    /* input  : */
#define GPIO_FM_STATUS                      GPIO_IEEE1394_D2	    /* input  : */
#define GPIO_HP_DETECT                      GPIO_GPIO_11	    /* input  : high when headphone is present */
#define GPIO_IRQ_MASTER0                    GPIO_GPIO_10	    /* input  : interrupt request from portable dock (go-gear ??) */
#define GPIO_KEYS1                          GPIO_GPIO_15	    /* input  : optional key to wake from eco-standby ?? */
#define GPIO_KEYS2                          GPIO_GPIO_16	    /* input  : optional key to wake from eco-standby ?? */
#define GPIO_LCD_BACKLIGHT                  GPIO_GPIO_20	    /* output : drive ?? to light LCD backlight */
#define GPIO_LCD_RESET                      GPIO_UART0_NCTS	    /* output : drive low to force LCD controller reset. 10k pull-up */
#define GPIO_MUTE                           GPIO_DAI_BCK	    /* output : */
#define GPIO_MUX_AF_A0                      GPIO_DAO_DATA	    /* output : */
#define GPIO_MUX_AF_A1                      GPIO_DAO_BCK	    /* output : 00=?? 01=?? 10=?? 11=?? (pick from FM, CD, PNX0106 and AUX ??) */
#define GPIO_PCMCIA_INTRQ                   GPIO_IEEE1394_CTL1      /* input  : Level triggered, active low */
#define GPIO_PCMCIA_RESET                   GPIO_IEEE1394_PD	    /* output : Drive high to force PCMCIA into reset. 10k pull-up */
#define GPIO_PLASMA                         GPIO_UART0_NRTS	    /* output : ?? */
#define GPIO_POWER_DOWN                     GPIO_IEEE1394_D6	    /* input  : ?? when main power supply is on (only on 7k centre + station and 5k centre) */
#define GPIO_RC6RX                          GPIO_IEEE1394_LPS	    /* input  : */
#define GPIO_RCFAST_RDWR                    GPIO_IEEE1394_D7	    /* output : */
#define GPIO_RDS_INTRQ                      GPIO_IEEE1394_D4	    /* input  : */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N	    /* output : Routed to SPINOR device on factory programming connector */
#define GPIO_SPIUART_INTRQ                  GPIO_GPIO_1             /* input  : Level triggered, active low */
#define GPIO_SPIUART_NCS                    GPIO_SPI0_S_N           /* output : Muxed : Routed to SPI NOR or SPI UART on factory programming connector */
#define GPIO_SPIUART_RESET                  GPIO_GPIO_0             /* output : drive low to force SPI UART reset */
#define GPIO_SPIUART_SELECT                 GPIO_GPIO_2             /* output : Muxed : drive high to select SPI UART, drive low or tri-state to select SPI NOR */

#define GPIO_UART_MODE                      GPIO_GPIO_12	    /* input  : */
#define GPIO_USB_BOOST                      GPIO_IEEE1394_CNA	    /* output : drive high to increase current capacity of the USB vbus supply */
#define GPIO_USB_POWER                      GPIO_GPIO_22	    /* output : drive high to turn on power to USB. ??k pull-?? */
#define GPIO_USB_POWER_FAULT                GPIO_GPIO_21	    /* input  : NOTE: should be driven (low) if USB is not used (eg in wacs5k) */
#define GPIO_V2                             GPIO_GPIO_19	    /* input  : platform config... */
#define GPIO_V3                             GPIO_DAO_CLK	    /* input  : V2/V3: 0/0=USA, 0/1=China/Korea, 1/0=EU with RDS, 1/1=EU no RDS */
#define GPIO_WAKE_UP                        GPIO_GPIO_5 	    /* output : */

#define USB_POWER_ON                        1                       /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       0                       /* definition for use with GPIO_USB_POWER */

#define USB_BOOST_ON                        1                       /* definition for use with GPIO_USB_BOOST */
#define USB_BOOST_OFF                       0                       /* definition for use with GPIO_USB_BOOST */

#define LED_ON                              1                       /* */
#define LED_OFF                             0                       /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_PCMCIA                          IRQ_EVENTROUTER_IRQ_1
#define IRQ_RC6RX                           IRQ_EVENTROUTER_IRQ_2
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI0_BASE
#define GPIO_SPI_NCS                        GPIO_SPINOR_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SPIUART_NCS


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_PNX0106_HACLI7)

/*******************************************************************************
*******************************************************************************/

#define BOARD_NAMESTRING                    "Philips HacLi-7 PNX0106 A-Hub Client board"

#define AX88796_REGS_BASE                   (MPMC_NCS1_BASE)        /* physical address */

#define GPIO_LED_ACTIVITY                   GPIO_GPIO_2             /* output : Muxed : Drive high to light BOOT LED */
#define GPIO_LED_HEARTBEAT                  GPIO_NULL               /* */

#if 1
#define GPIO_ATA_RESET                      GPIO_NULL               /* output : Need to define something for eco-standby... maybe a real pin is even better ?? */
#else
#define GPIO_ATA_RESET                      GPIO_PCCARDB_NRESET     /* output : Drive low to hold ATA devices in reset. 10k pull-?? */
#endif

#define GPIO_BT_POWER                       GPIO_GPIO_5             /* output : Drive low: power-on BT module, drive high: power-off BT module */
#define GPIO_PLASMA                         GPIO_UART0_NRTS         /* output : ?? */
#define GPIO_POWER_DOWN                     GPIO_IEEE1394_D6        /* input  : ?? when main power supply is on (only on 7k centre + station and 5k centre) */
#define GPIO_ETH_INTRQ                      GPIO_IEEE1394_D5        /* input  : Level triggered, active high */
#define GPIO_ETH_RESET                      GPIO_GPIO_3             /* output : Drive low to force AX88796 Rev B into reset. ??k pull-?? */
#define GPIO_ECO_STBY                       GPIO_GPIO_18            /* output : Drive low to turn off power to WiFi card */
#define GPIO_DISABLE_RC                     GPIO_MPMC_BLOUT0        /* output : Drive low: disable, drive high: enable */
#define GPIO_LED_WIFI_BT                    GPIO_MPMC_BLOUT1        /* output : Drive low: BT LED on, drive high: WiFi LED on, hi-z: both LEDs off */
#define GPIO_LCD_BACKLIGHT                  GPIO_IEEE1394_CTL1      /* output : drive ?? to light LCD backlight */
#define GPIO_FLASH_A21                      GPIO_SPI0_EXT           /* output : 47k pull-down */
#define GPIO_FLASH_A22                      GPIO_SPI1_EXT           /* output : 47k pull-down */
#define GPIO_BT_UART_SELECT                 GPIO_PCCARD_A0          /* output : ??? */
#define GPIO_DEVICE_ID                      GPIO_GPIO_4             /* input  : low: go-gear, high: ipod, hi-z: unknown */
#define GPIO_ECO_CORE                       GPIO_IEEE1394_D3        /* output : drive low to stop 3v3 supply to WiFi etc (but shouldn't be used). 47k pull-up */
#define GPIO_FAN                            GPIO_IEEE1394_SYSCLK    /* output : */
#define GPIO_FM_INTRQ                       GPIO_IEEE1394_D1        /* input  : */
#define GPIO_FM_STATUS                      GPIO_IEEE1394_D2        /* input  : */
#define GPIO_HP_DETECT                      GPIO_IEEE1394_CPS       /* input  : high when headphone is present */
#define GPIO_IRQ_MASTER0                    GPIO_GPIO_7             /* input  : interrupt request from portable dock (go-gear ??) */
#define GPIO_LCD_RESET                      GPIO_UART0_NCTS         /* output : drive low to force LCD controller reset. 10k pull-up */
#define GPIO_LED_ECOSTANDBY                 GPIO_IEEE1394_D0        /* output : Drive low: off, drive high: green, tri-state: red */
#define GPIO_MUX_AF_A0                      GPIO_PCCARD_NDIOR       /* output : */
#define GPIO_MUX_AF_A1                      GPIO_PCCARD_NDIOW       /* output : 00=?? 01=?? 10=?? 11=?? (pick from FM, CD, PNX0106 and AUX ??) */
#define GPIO_PCMCIA_RESET                   GPIO_PCCARDA_NRESET     /* output : Drive high to force PCMCIA into reset. 10k pull-?? */
#define GPIO_POWER_AMP                      GPIO_IEEE1394_CNA       /* output : Drive low: AMP off, drive high: AMP on. */
#define GPIO_RC6RX                          GPIO_GPIO_6             /* input  : */
#define GPIO_RCFAST_RDWR                    GPIO_IEEE1394_D7        /* output : */
#define GPIO_RDS_INTRQ                      GPIO_IEEE1394_D4        /* input  : */
#define GPIO_SPINOR_NCS                     GPIO_SPI0_S_N           /* output : Muxed : Routed to SPI NOR or SPI UART on factory programming connector */
#define GPIO_SPIUART_INTRQ                  GPIO_GPIO_1             /* input  : Level triggered, active low */
#define GPIO_SPIUART_NCS                    GPIO_SPI0_S_N           /* output : Muxed : Routed to SPI NOR or SPI UART on factory programming connector */
#define GPIO_SPIUART_RESET                  GPIO_GPIO_0             /* output : drive low to force SPI UART reset */
#define GPIO_SPIUART_SELECT                 GPIO_GPIO_2             /* output : Muxed : drive high to select SPI UART, drive low or tri-state to select SPI NOR */
#define GPIO_UART_MODE                      GPIO_GPIO_8             /* input  : low: debug board selects SPI NOR (or debug board not present), high: debug board selects SPI UART */
#define GPIO_USB_BOOST                      GPIO_GPIO_17            /* output : drive high to increase current capacity of the USB vbus supply */
#define GPIO_USB_POWER                      GPIO_GPIO_22            /* output : drive high to turn on power to USB. ??k pull-?? */
#define GPIO_USB_POWER_FAULT                GPIO_IEEE1394_LREQ      /* input  : NOTE: should be driven (low) if USB is not used (eg in wacs5k) */
#define GPIO_V2                             GPIO_GPIO_19            /* input  : platform config... */
#define GPIO_V3                             GPIO_DAO_CLK            /* input  : V2/V3: 0/0=USA, 0/1=China/Korea, 1/0=EU with RDS, 1/1=EU no RDS */

#define GPIO_PCMCIA_INTRQ                   GPIO_PCCARDA_INTRQ  /* input  : Level triggered, active low */
#define GPIO_PCMCIA_RESET                   GPIO_PCCARDA_NRESET /* output : Drive high to force PCMCIA into reset. 10k pull-?? */


#define USB_POWER_ON                        1                       /* definition for use with GPIO_USB_POWER */
#define USB_POWER_OFF                       0                       /* definition for use with GPIO_USB_POWER */

#define USB_BOOST_ON                        1                       /* definition for use with GPIO_USB_BOOST */
#define USB_BOOST_OFF                       0                       /* definition for use with GPIO_USB_BOOST */

#define LED_ON                              1                       /* */
#define LED_OFF                             0                       /* */

#define IRQ_ETHERNET                        IRQ_EVENTROUTER_IRQ_0
#define IRQ_PCMCIA                          IRQ_EVENTROUTER_IRQ_1
#define IRQ_RC6RX                           IRQ_EVENTROUTER_IRQ_2
#define IRQ_E7B2ARM_IRQ2                    IRQ_EVENTROUTER_IRQ_3

#define NR_IRQS                             NR_INTC_IRQS

/*
    Each SPI driver can currently handle only one SPI interface (and one SPI device...)
    We need to select which SPI interface to use and which GPIO pin drives nCS
*/
#define IO_ADDRESS_SPI_BASE                 IO_ADDRESS_SPI0_BASE
#define GPIO_SPI_NCS                        GPIO_SPINOR_NCS

#define IO_ADDRESS_SPISD_BASE               IO_ADDRESS_SPI0_BASE
#define GPIO_SPISD_NCS                      GPIO_SPIUART_NCS


/*******************************************************************************
*******************************************************************************/

#else
#error "Requested PNX0106 platform not yet implementated"
#endif

/*******************************************************************************
*******************************************************************************/

#if ! defined (LED_ON)
#define LED_ON                              1
#endif

#if ! defined (LED_OFF)
#define LED_OFF                             (!(LED_ON))
#endif

/*******************************************************************************
*******************************************************************************/

#define RELEASE_ID_PLATFORM_SHIFT           28
#define RELEASE_ID_PLATFORM_MASK            0x0F
#define RELEASE_ID_MAJOR_SHIFT              24
#define RELEASE_ID_MAJOR_MASK               0x0F
#define RELEASE_ID_MINOR_SHIFT              20
#define RELEASE_ID_MINOR_MASK               0x0F
#define RELEASE_ID_POINT_SHIFT              16
#define RELEASE_ID_POINT_MASK               0x0F
#define RELEASE_ID_BUILD_SHIFT              0
#define RELEASE_ID_BUILD_MASK               0xFFFF

#define RELEASE_ID_PLATFORM(release_id)     (((release_id) >> RELEASE_ID_PLATFORM_SHIFT) & RELEASE_ID_PLATFORM_MASK)
#define RELEASE_ID_MAJOR(release_id)        (((release_id) >> RELEASE_ID_MAJOR_SHIFT) & RELEASE_ID_MAJOR_MASK)
#define RELEASE_ID_MINOR(release_id)        (((release_id) >> RELEASE_ID_MINOR_SHIFT) & RELEASE_ID_MINOR_MASK)
#define RELEASE_ID_POINT(release_id)        (((release_id) >> RELEASE_ID_POINT_SHIFT) & RELEASE_ID_POINT_MASK)
#define RELEASE_ID_BUILD(release_id)        (((release_id) >> RELEASE_ID_BUILD_SHIFT) & RELEASE_ID_BUILD_MASK)

#define PLATFORM_UNKNOWN                    0x00

#endif


