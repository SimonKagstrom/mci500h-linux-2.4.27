/*
 *  linux/include/asm-arm/arch-ssa/hardware.h
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
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
extern unsigned int cpuclkhz;
extern unsigned int hclkhz;
extern unsigned int uartclkhz;

extern unsigned int cpuclkmhz;                  /* deprecated - if possible use cpuclkhz instead */
extern unsigned int hclkmhz;                    /* deprecated - if possible use hclkhz instead */
#define HCLK_MHZ hclkmhz

extern unsigned long ata_regs_phys;             /* only used by old SIIG board platform */

#endif

/******************************************************************************/

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
#define PLATFORM_SDB_REVA                   0x0A
#define PLATFORM_SDB_REVC                   0x0C
#define PLATFORM_HAS_LAB1                   0x01
#define PLATFORM_HAS_LAB2                   0x02

#define PLATFORM_W3_LAB1                    0x07


/******************************************************************************/

/*
   Default safe MEM_SIZE value (bootloader should over-ride with param TAG or cmdline)
*/

#define MEM_SIZE                    (8 * 1024 * 1024)

/*
   Physical addresses...
*/

#define SSA_IROM_BASE               0x10000000              /* Physical address: 384k bytes of 32bit wide internal code ROM */
#define SSA_ISRAM_BASE              0x20000000              /* Physical address:  64k bytes of 32bit wide internal SRAM */

#define ATU_CH1_LEFT_FIFO_BASE      0x33000000              /* Physical address: Audio out */
#define ATU_CH1_RIGHT_FIFO_BASE     0x33000040              /* Physical address: Audio out */
#define ATU_CH2_LEFT_FIFO_BASE      0x33000080              /* Physical address: Audio out */
#define ATU_CH2_RIGHT_FIFO_BASE     0x330000C0              /* Physical address: Audio out */

#define SSA_NCS0_BASE               0x40000000              /* Physical address: EBI nCS0 : FLASH */
#define SSA_NCS1_BASE               0x44000000              /* Physical address: EBI nCS1 : */
#define SSA_NCS2_BASE               0x48000000              /* Physical address: EBI nCS2 : */

#define SSA_SMC_BASE                0x80000000              /* Physical address: (external - ie EBI) Static Memory Controller */
#define SSA_ATU_BASE                0x85000000              /* Physical address: Audio Transfer Unit control regs */
#define SSA_CT0_BASE                0x88000000              /* Physical address: Timer 0 */
#define SSA_CT1_BASE                0x88400000              /* Physical address: Timer 1 */
#define SSA_INTC_BASE               0x89000000              /* Physical address: Interrupt Controller */
#define SSA_SDAC_BASE               0x8A800000              /* Physical address: SDAC control registers */
#define SSA_UART0_BASE              0x8B000000              /* Physical address: UART 0 */
#define SSA_CDB_BASE                0x8B800000              /* Physical address: CD Block decoder */
#define SSA_GPIO_BASE               0x8C800000              /* Physical address: GPIO Controller */
#define SSA_IICMASTER_BASE          0x8D800000              /* Physical address: IIC Master interface */
#define SSA_IICSLAVE_BASE           0x8E800000              /* Physical address: IIC Slave interface */
#define SSA_CLOCK_CTRL_BASE         0x8F800000              /* Physical address: PLL and clock control registers */

/*
   Vitual addresses...
   The Physical addresses are so randomly placed that it would be hard to
   devise a standard mapping from phys->virt... so we don't even try.
   Every IO address which is used directly by assembler must be aligned on a 16M boundary.
*/

#define IO_ADDRESS_INTC_BASE        0xF0000000              /* 16MByte boundary required */
#define IO_ADDRESS_GPIO_BASE        0xF1000000              /* 16MByte boundary required */
#define IO_ADDRESS_EPLD_BASE        0xF2000000              /* EPLD for HAS 7750 and 7752, CPLD for W3 */
#define IO_ADDRESS_FLASH_BASE       0xF3000000              /* requires 2MBytes */
#define IO_ADDRESS_UART0_BASE       0xF4000000
#define IO_ADDRESS_CT0_BASE         0xF4100000
#define IO_ADDRESS_CT1_BASE         0xF4200000
#define IO_ADDRESS_SMC_BASE         0xF4300000
#define IO_ADDRESS_ATU_BASE         0xF4400000
#define IO_ADDRESS_ATU_FIFO_BASE    0xF4500000
#define IO_ADDRESS_IICSLAVE_BASE    0xF4600000
#define IO_ADDRESS_CLOCK_CTRL_BASE  0xF4700000
#define IO_ADDRESS_SDAC_BASE        0xF4800000
#define IO_ADDRESS_ISRAM_BASE       0xF4900000
#define IO_ADDRESS_IICMASTER_BASE   0xF4A00000
#define IO_ADDRESS_CDB_BASE         0xF4B00000


/*******************************************************************************
*******************************************************************************/

#if defined (CONFIG_SSA_SIIG)

//#define GPIO_MYSTERY_CD           (1 <<  0)               /* JP4:1 Goes direct to CD engine connector ?? */
//#define GPIO_MYSTERY_IIC_MASTER   (1 <<  1)               /* JP6:1 Fix for CD10 IIC master clock line. 100k pull-up */
#define GPIO_CS8900_INTRQ           (1 <<  2)               /* int request from cs8900 */
#define GPIO_LED10                  (1 <<  3)               /* drive low to light LED D8 (signal also on TP4) */
#define GPIO_INTRQ_FPM              (1 <<  4)               /* our int request to Front Panel Micro */
#define GPIO_CS8900_RESET           (1 <<  5)               /* drive low to bring cs8900 out of reset. 100k pull-up */
//#define GPIO_MYSTERY_IIC_MASTER   (1 <<  6)               /* JP6:2 Fix for CD10 IIC master data line. 100k pull-up */
//#define GPIO_MYSTERY_CD           (1 <<  7)               /* JP6:3 Int request from CD10 ?? */
#define GPIO_ATA_INTRQ              (1 <<  8)               /* int request from ATA interface. level trig, active low */
#define GPIO_ATA_NHRST              (1 <<  9)               /* drive low to hold ATA devices in reset (has pull down) */
#define GPIO_ATA_SELECT             (1 << 10)               /* divides nCS2 between ATA and PCMCIA (low == ATA) */
#define GPIO_PCMCIA_REG             (1 << 11)               /* selects PCMCIA Attribute space for memory accesses (active low) */
#define GPIO_PCMCIA_MEM_SELECT      (1 << 12)               /* selects PCMCIA access mode (low = memory, high == IO) */
#define GPIO_PCMCIA_8BIT_SELECT     (1 << 13)               /* Indicates 8 or 16 bit pcmcia accesses (low == 8bit) */
#define GPIO_SWITCH_2               (1 << 14)               /* debug push/dip switch (low when pressed) */
#define GPIO_SWITCH_3               (1 << 15)               /* debug push/dip switch (low when pressed) */
#define GPIO_SWITCH_4               (1 << 16)               /* debug push/dip switch (low when pressed) */
#define GPIO_SWITCH_5               (1 << 17)               /* debug push/dip switch (low when pressed) */
#define GPIO_PCMCIA_RESET           (1 << 18)               /* drive ?? to force PCMCIA reset */
#define GPIO_PCMCIA_INTRQ           (1 << 19)               /* int request from the PCMCIA interface. Level trig, active low ?? */
#define GPIO_LED_HEARTBEAT          (1 << 20)               /* drive low to light LED D9 (signal also on TP5) */
#define GPIO_LED_ACTIVITY           (1 << 21)               /* drive low to light LED D8 (signal also on TP6) */
#define GPIO_PCMCIA_SPEED           (1 << 22)               /* selects between fast and slow PCMCIA IO accesses (low == slow) */
#define GPIO_PCMCIA_WAIT            (1 << 23)               /* pulsed low during pcmcia accesses if wait is violated */
//#define GPIO_LED3                 (1 << 24)               /* drive low to light LED D3 (signal also on TP1) */

#define LED_ON                      0                       /* */
#define LED_OFF                     1                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ATA                     4                       /* ATA                      (raw int gpio  8) */
#define INT_CS8900                  5                       /* CS8900                   (raw int gpio  2) */
#define INT_PCMCIA                  6                       /* PCMCIA                   (raw int gpio 19) */
#define INT_ATU                     7                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              8                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               9                       /* IIC Slave                (raw int       9) */

#define NR_IRQS                     10


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_WESSLI)

#define GPIO_PCMCIA_RESET           (1 <<  0)               /* drive high to force PCMCIA reset. 10k pull-up */
#define GPIO_CD10_IIC_SCL_HACK      (1 <<  1)               /* hack for CD10 IIC master clock line. 100k pull-up */
#define GPIO_LED_ACTIVITY           (1 <<  2)               /* drive high to light LED_1 */
#define GPIO_LED_HEARTBEAT          (1 <<  3)               /* drive high to light LED_2 */
#define GPIO_INTRQ_FPM              (1 <<  4)               /* drive low to assert our int request to Front Panel Micro */
#define GPIO_CS8900_RESET           (1 <<  5)               /* drive low to bring cs8900 out of reset. 100k pull-up */
#define GPIO_CD10_IIC_SDA_HACK      (1 <<  6)               /* hack for CD10 IIC master data line. 100k pull-up */
#define GPIO_SERVICE_MODE           (1 <<  7)               /* input  used to select service mode (active low) */
#define GPIO_CDDA_FAKE_SBSY         (1 <<  8)               /* output used to generate fake CDDA frame syncs */
#define GPIO_SDRAM_CONFIG           (1 <<  9)               /* input (low == 2MByte SDRAM) (high == 8 or 16 MByte SDRAM) */
#define GPIO_FASTBOOT_CONFIG        (1 << 10)               /* input (low == fast boot) (high == wait for keypress) */
#define GPIO_PCMCIA_REG             (1 << 11)               /* should be left as an input as Lab[01] ties REG# to PCMCIA CE1# (which is tied to EBI nCS2) */
#define GPIO_PCMCIA_MEM_SELECT      (1 << 12)               /* output divides PCMCIA space between memory/attribute and IO spaces (high == IO mode) */
#define GPIO_PCMCIA_8BIT_SELECT     (1 << 13)               /* output indicates 8 or 16 bit pcmcia accesses (low == 8bit) */
#define GPIO_PCMCIA_SPEED           (1 << 14)               /* output selects between fast and slow pcmcia IO accesses (low == slow) */
#define GPIO_PCMCIA_SPEED_B         (1 << 15)               /* output currently unused */
#define GPIO_CPLD_TDO               (1 << 16)               /* leave floating except while actually performing CPLD ISP */
#define GPIO_CPLD_TDI               (1 << 17)               /* leave floating except while actually performing CPLD ISP */
#define GPIO_CPLD_TMS               (1 << 18)               /* leave floating except while actually performing CPLD ISP */
#define GPIO_CPLD_TCK               (1 << 19)               /* leave floating except while actually performing CPLD ISP */
#define GPIO_PCMCIA_INTRQ           (1 << 20)               /* input int request from the PCMCIA interface. Level trig, active low ?? */
#define GPIO_CS8900_INTRQ           (1 << 21)               /* input int request from cs8900 */
#define GPIO_PCMCIA_WAIT            (1 << 22)               /* input pulsed low during pcmcia accesses if wait is violated */
#define GPIO_CPLD_VERSION_A         (1 << 23)               /* input (so far unused) */
#define GPIO_CPLD_VERSION_B         (1 << 24)               /* input (so far unused) */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_CS8900                  4                       /* CS8900                   (raw int gpio 21) */
#define INT_PCMCIA                  5                       /* PCMCIA                   (raw int gpio 20) */
#define INT_ATU                     6                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              7                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               8                       /* IIC Slave                (raw int       9) */

#define NR_IRQS                     9


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_W3) && defined (CONFIG_SSA_W3_PCCARD)

#define GPIO_UNUSED_00              (1 <<  0)               /* 100k pull-down */
#define GPIO_UNUSED_01              (1 <<  1)               /* 100k pull-down */
#define GPIO_LED_ACTIVITY           (1 <<  2)               /* drive high to light LED_1 */
#define GPIO_LED_HEARTBEAT          (1 <<  3)               /* drive high to light LED_2 */
#define GPIO_INTRQ_FPM              (1 <<  4)               /* drive low to assert our int request to Front Panel Micro */
#define GPIO_CPLD_TDI               (1 <<  5)               /* */
#define GPIO_UNUSED_06              (1 <<  6)               /* 100k pull-down */
#define GPIO_UNUSED_07              (1 <<  7)               /* 100k pull-down */
#define GPIO_UNUSED_08              (1 <<  8)               /* 100k pull-down */
#define GPIO_UNUSED_09              (1 <<  9)               /* 100k pull-down */
#define GPIO_UNUSED_10              (1 << 10)               /* 100k pull-down */
#define GPIO_CPLD_RESET             (1 << 11)               /* drive low to force CPLD into reset */
#define GPIO_PCMCIA_RESET           (1 << 12)               /* drive high to force PCMCIA reset. 10k pull-up */
#define GPIO_CPLD_TDO               (1 << 13)               /* */
#define GPIO_ETHERNET_INTRQ         (1 << 14)               /* input. Level triggered, active high */
#define GPIO_UNUSED_15              (1 << 15)               /* 100k pull-down */
#define GPIO_ETHERNET_RESET         (1 << 16)               /* output. drive high to force AX88796 into reset. 100k pull-up */
#define GPIO_UNUSED_17              (1 << 17)               /* 100k pull-down */
#define GPIO_SDRAM_CONFIG           (1 << 18)               /* input (low == 2MByte SDRAM) (high == 8 or 16 MByte SDRAM) */
#define GPIO_FASTBOOT_CONFIG        (1 << 19)               /* input (low == fast boot) (high == wait for keypress) */
#define GPIO_PCMCIA_INTRQ           (1 << 20)               /* input int request from the PCMCIA interface. Level trig, active low ?? */
#define GPIO_UNUSED_21              (1 << 21)               /* 100k pull-down */
#define GPIO_UNUSED_22              (1 << 22)               /* 100k pull-down */
#define GPIO_CPLD_TMS               (1 << 23)               /* */
#define GPIO_CPLD_TCK               (1 << 24)               /* */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ETHERNET                4                       /* AX88796                  (raw int gpio 14) */
#define INT_PCMCIA                  5                       /* PCMCIA (same pin as SPI) (raw int gpio 20) */
#define INT_ATU                     6                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              7                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               8                       /* IIC Slave                (raw int       9) */

#define NR_IRQS                     9


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_W3) && defined (CONFIG_SSA_W3_GPI)

#define GPIO_GPI_GUCIRDY            (1 <<  0)               /* active low flow control input (or 100k pull-down if GPI interface is not in use) */
#define GPIO_GPI_GDFCIRDY           (1 <<  1)               /* active low intrq input (or 100k pull-down if GPI interface is not in use) */
#define GPIO_LED_ACTIVITY           (1 <<  2)               /* drive high to light LED_1 */
#define GPIO_LED_HEARTBEAT          (1 <<  3)               /* drive high to light LED_2 */
#define GPIO_INTRQ_FPM              (1 <<  4)               /* drive low to assert our int request to Front Panel Micro */
#define GPIO_CPLD_TDI               (1 <<  5)               /* */
#define GPIO_UNUSED_06              (1 <<  6)               /* 100k pull-down */
#define GPIO_UNUSED_07              (1 <<  7)               /* 100k pull-down */
#define GPIO_UNUSED_08              (1 <<  8)               /* 100k pull-down */
#define GPIO_UNUSED_09              (1 <<  9)               /* 100k pull-down */
#define GPIO_UNUSED_10              (1 << 10)               /* 100k pull-down */
#define GPIO_CPLD_RESET             (1 << 11)               /* drive low to force CPLD into reset */
#define GPIO_GPI_RESET              (1 << 12)               /* drive low to force GPI reset. ??k pull-?? */
#define GPIO_CPLD_TDO               (1 << 13)               /* */
#define GPIO_ETHERNET_INTRQ         (1 << 14)               /* input. Level triggered, active high */
#define GPIO_UNUSED_15              (1 << 15)               /* 100k pull-down */
#define GPIO_ETHERNET_RESET         (1 << 16)               /* output. drive high to force AX88796 into reset. 100k pull-up */
#define GPIO_UNUSED_17              (1 << 17)               /* 100k pull-down */
#define GPIO_SDRAM_CONFIG           (1 << 18)               /* input (low == 2MByte SDRAM) (high == 8 or 16 MByte SDRAM) */
#define GPIO_FASTBOOT_CONFIG        (1 << 19)               /* input (low == fast boot) (high == wait for keypress) */
#define GPIO_UNUSED_20              (1 << 20)               /* ?? */
#define GPIO_UNUSED_21              (1 << 21)               /* 100k pull-down */
#define GPIO_UNUSED_22              (1 << 22)               /* 100k pull-down */
#define GPIO_CPLD_TMS               (1 << 23)               /* */
#define GPIO_CPLD_TCK               (1 << 24)               /* */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ETHERNET                4                       /* AX88796                  (raw int gpio 14) */
#define INT_ATU                     5                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              6                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               7                       /* IIC Slave                (raw int       9) */
#define INT_GPI                     8                       /* GPI                      (raw int gpio ??) */

#define NR_IRQS                     9


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_W3) && defined (CONFIG_SSA_W3_SPI)

#define GPIO_UNUSED_00              (1 <<  0)               /* 100k pull-down */
#define GPIO_UNUSED_01              (1 <<  1)               /* 100k pull-down */
#define GPIO_LED_ACTIVITY           (1 <<  2)               /* drive high to light LED_1 */
#define GPIO_LED_HEARTBEAT          (1 <<  3)               /* drive high to light LED_2 */
#define GPIO_INTRQ_FPM              (1 <<  4)               /* drive low to assert our int request to Front Panel Micro */
#define GPIO_CPLD_TDI               (1 <<  5)               /* */
#define GPIO_UNUSED_06              (1 <<  6)               /* 100k pull-down */
#define GPIO_UNUSED_07              (1 <<  7)               /* 100k pull-down */
#define GPIO_UNUSED_08              (1 <<  8)               /* 100k pull-down */
#define GPIO_UNUSED_09              (1 <<  9)               /* 100k pull-down */
#define GPIO_UNUSED_10              (1 << 10)               /* 100k pull-down */
#define GPIO_CPLD_RESET             (1 << 11)               /* drive low to force CPLD into reset */
#define GPIO_SPI_RESET              (1 << 12)               /* drive low to force reset of BGW200. ??k pull-?? */
#define GPIO_CPLD_TDO               (1 << 13)               /* */
#define GPIO_ETHERNET_INTRQ         (1 << 14)               /* input. Level triggered, active high */
#define GPIO_UNUSED_15              (1 << 15)               /* 100k pull-down */
#define GPIO_ETHERNET_RESET         (1 << 16)               /* output. drive high to force AX88796 into reset. 100k pull-up */
#define GPIO_UNUSED_17              (1 << 17)               /* 100k pull-down */
#define GPIO_SDRAM_CONFIG           (1 << 18)               /* input (low == 2MByte SDRAM) (high == 8 or 16 MByte SDRAM) */
#define GPIO_FASTBOOT_CONFIG        (1 << 19)               /* input (low == fast boot) (high == wait for keypress) */
#define GPIO_SPI_INTRQ              (1 << 20)               /* input int request from the SPI device (BGW200 ?). Level trig, active high ?? */
#define GPIO_UNUSED_21              (1 << 21)               /* 100k pull-down */
#define GPIO_UNUSED_22              (1 << 22)               /* 100k pull-down */
#define GPIO_CPLD_TMS               (1 << 23)               /* */
#define GPIO_CPLD_TCK               (1 << 24)               /* */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ETHERNET                4                       /* AX88796                  (raw int gpio 14) */
#define INT_SPI                     5                       /* SPI (same pin as PCMCIA) (raw int gpio 20) */
#define INT_ATU                     6                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              7                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               8                       /* IIC Slave                (raw int       9) */

#define NR_IRQS                     9


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_TAHOE)

#define GPIO_NAND_WP                (1 <<  0)               /* output. drive low for write protection */
#define GPIO_NAND_RB                (1 <<  1)               /*  input. (low == busy), (high == ready) */
#define GPIO_LED_ACTIVITY           (1 <<  2)               /* output. drive high to light LED_1 */
#define GPIO_LED_HEARTBEAT          (1 <<  3)               /* output. drive high to light LED_2 */
#define GPIO_KEYPAD_IN_0            (1 <<  4)               /*  input. 12k pull-down */
#define GPIO_KEYPAD_IN_1            (1 <<  5)               /*  input. 12k pull-down */
#define GPIO_KEYPAD_IN_2            (1 <<  6)               /*  input. 12k pull-down */
#define GPIO_KEYPAD_OUT_0           (1 <<  7)               /* output. should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_1           (1 <<  8)               /* output. should be high by default (when waiting for key press) */
#define GPIO_KEYPAD_OUT_2           (1 <<  9)               /* output. should be high by default (when waiting for key press) */
#define GPIO_USB_H_SUSPEND          (1 << 10)               /* output. drive high to suspend USB host controller */
#define GPIO_USB_D_SUSPEND          (1 << 11)               /* output. drive high to suspend USB device controller */
#define GPIO_CPLD_TCK               (1 << 12)               /* output. */
#define GPIO_CPLD_TMS               (1 << 13)               /* output. */
#define GPIO_CPLD_TDI               (1 << 14)               /* output. (tdi name comes from the target device's point of view) */
#define GPIO_LCD_RESET              (1 << 15)               /* output. drive low to force LCD controller reset. ??k pull-?? */
#define GPIO_ETHERNET_RESET         (1 << 16)               /* output. drive high to force AX88796 into reset. ??k pull-?? */
#define GPIO_WLAN_AND_CPLD_RESET    (1 << 17)               /* output. drive low to force CPLD **AND** BGW2xx into reset. ??k pull-?? */
#define GPIO_USB_RESET              (1 << 18)               /* output. drive low to force USB host and device controllers into reset. ??k pull-?? */

#define GPIO_INTRQ_FPM              (1 << 19)               /* output. +++ NOTE MULTIPLE DEFINITION +++ drive low to assert our int request to Front Panel Micro */
#define GPIO_LCD_BACKLIGHT          (1 << 19)               /* output. +++ NOTE MULTIPLE DEFINITION +++ drive high to light LCD backlight */

#define GPIO_ETHERNET_INTRQ         (1 << 20)               /*  input. Level triggered, active high */
#define GPIO_WLAN_INTRQ             (1 << 21)               /*  input. int request from BGW2xx. Level triggered, active high */
#define GPIO_USB_H_INTRQ            (1 << 22)               /*  input. int request from USB Host controller. Level triggered, active low */
#define GPIO_USB_D_INTRQ            (1 << 23)               /*  input. int request from USB Device controller. Level triggered, active low */
#define GPIO_FLASH_BANK_SELECT      (1 << 24)               /* output. drive high to access upper 2MByte of flash. 10k pull-down */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/irq.c and by
   the 'get_irqnr_and_base' macro in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ETHERNET                4                       /* AX88796                  (raw int gpio 20) */
#define INT_WLAN                    5                       /* BGW2xx                   (raw int gpio 21) */
#define INT_USB_HOST                6                       /* USB Host                 (raw int gpio 22) */
#define INT_USB_DEVICE              7                       /* USB Device               (raw int gpio 23) */
#define INT_ATU                     8                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              9                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE              10                       /* IIC Slave                (raw int       9) */
#define INT_KEYPAD_IN_0            11                       /* Keypad input 0           (raw int gpio  4) */
#define INT_KEYPAD_IN_1            12                       /* Keypad input 1           (raw int gpio  5) */
#define INT_KEYPAD_IN_2            13                       /* Keypad input 2           (raw int gpio  6) */

#define NR_IRQS                    14


/*******************************************************************************
*******************************************************************************/

#elif defined (CONFIG_SSA_HAS7752)

/*
   The HAS7752 must be able to run on multiple variations of the same platform.
   Therefore GPIO pins and interrupts are handled slightly differently...
   
   Warning: This list of GPIO definitions must exactly match the table ordering
            in has7752_gpio.c - edit with care !!
*/

#define GPIO_FLASH_BANK_SELECT       0                      /* drive high to access upper 2MByte of flash. pull-down */
#define GPIO_LED_ACTIVITY            1                      /* drive high to light activity LED */
#define GPIO_LED_HEARTBEAT           2                      /* drive high to light heartbeat LED */
#define GPIO_7750_RESET              3                      /* drive low to force 7750 reset. pull-up */
#define GPIO_CMD_TO_52               4                      /* our input from 7750 */
#define GPIO_CMD_TO_50               5                      /* our output to 7750 */
#define GPIO_SUSPEND_1581            6                      /* ?? */
#define GPIO_WAKEUP_1581             7                      /* ?? */
#define GPIO_INTRQ_FPM               8                      /* drive low to assert our int request to Front Panel Micro */
#define GPIO_USB_RESET               9                      /* drive low to force USB slave into reset */
#define GPIO_PCMCIA_RESET           10                      /* drive high to force PCMCIA reset. pull-up */
#define GPIO_PCMCIA_INTRQ           11                      /* input int request from the PCMCIA interface. Level triggered, active low */
#define GPIO_ETHERNET_INTRQ         12                      /* input int request from AX88796. Level triggered, active high */
#define GPIO_ETHERNET_RESET         13                      /* drive high to force AX88796 into reset. AX88796 has internal pull-up */
#define GPIO_ATA_RESET              14                      /* drive low to hold ATA devices in reset. pull-down */
#define GPIO_SWITCH_1               15                      /* ?? */
#define GPIO_SWITCH_2               16                      /* ?? */
#define GPIO_EPLD_INTRQ             17                      /* input int request from EPLD. Level triggered, active high */
#define GPIO_CPLD_TDI               18                      /* leave floating except while actually performing EPLD ISP */
#define GPIO_CPLD_TDO               19                      /* leave floating except while actually performing EPLD ISP */
#define GPIO_CPLD_TMS               20                      /* leave floating except while actually performing EPLD ISP */
#define GPIO_CPLD_TCK               21                      /* leave floating except while actually performing EPLD ISP */
#define GPIO_USB_SENSE              22                      /* ?? */
#define GPIO_USBH_NSELECT           23                      /* ?? */
#define GPIO_USBH_SUSPEND           24                      /* ?? */
#define GPIO_FLASH_NSELECT          25                      /* ?? */
#define GPIO_USBH_RESET             26                      /* ?? */
#define GPIO_USBH_WAKE              27                      /* ?? */
#define GPIO_USBH_INTRQ             28                      /* ?? */

#define NR_HAS7752_GPIOS            29                      /* */

#define LED_ON                      1                       /* */
#define LED_OFF                     0                       /* */

/*
   IRQ definitions - note that these are the cooked versions...
   The raw versions are known by arch/arm/mach-ssa/has7752_irq.c and by
   the 'get_irqnr_and_base' code in entry-armv.S
*/

#define INT_SPURIOUS                0                       /* should never happen...                     */
#define INT_CT0                     1                       /* Timer 0                  (raw int       4) */
#define INT_CT1                     2                       /* Timer 1                  (raw int       5) */
#define INT_UART0                   3                       /* UART 0                   (raw int      28) */
#define INT_ETHERNET                4                       /* AX88796                  (raw int gpio xx) */
#define INT_PCMCIA                  5                       /* PCMCIA                   (raw int gpio xx) */
#define INT_ISP1581                 6                       /* ISP1581 USB device       (raw int epld xx) */
#define INT_ATA                     7                       /* ATA                      (raw int epld xx) */
#define INT_ATU                     8                       /* Audio Transfer Unit      (raw int       1) */
#define INT_IIC_MASTER              9                       /* IIC Master               (raw int      10) */
#define INT_IIC_SLAVE               10                      /* IIC Slave                (raw int       9) */
#define INT_USBH                    11                      /* USB Host                 (raw int gpio xx) */
#define INT_DPRAM                   12                      /* 2nd CPU (via EPLD)       (raw int epld xx) */
#define INT_KTIMER                  13                      /* EPLD 100[0] HZ timer     (raw int epld xx) */

#define NR_IRQS                     14


#define IO_ADDRESS_PCCARD_PF_BASE       (IO_ADDRESS_EPLD_BASE + 0x00000)    /* 256k byte page frame maps directly to card address space */
#define IO_ADDRESS_PCCARD_REGS_BASE     (IO_ADDRESS_EPLD_BASE + 0x40000)    /* interface logic control registers */
#define IO_ADDRESS_TSC64_REGS_BASE      (IO_ADDRESS_EPLD_BASE + 0x48000)    /* hardware 64bit free-running time stamp counter */
#define IO_ADDRESS_KTIMER_REGS_BASE     (IO_ADDRESS_EPLD_BASE + 0x4C000)    /* kernel optimised Hz timer */
#define IO_ADDRESS_DPRAM_BUF_BASE       (IO_ADDRESS_EPLD_BASE + 0x50000)    /* shared memory for communication with 2nd cpu */
#define IO_ADDRESS_DPRAM_REGS_BASE      (IO_ADDRESS_EPLD_BASE + 0x58000)    /* interrupt control for communication with 2nd cpu */
#define IO_ADDRESS_ATA_BUF_BASE         (IO_ADDRESS_EPLD_BASE + 0x60000)    /* 2k byte buffer used for all data transfers */
#define IO_ADDRESS_ATA_REGS_BASE        (IO_ADDRESS_EPLD_BASE + 0x68000)    /* interface logic control registers */
#define EPLD_ISP1581_INT_REGS_BASE      (IO_ADDRESS_EPLD_BASE + 0x68020)
#define IO_ADDRESS_SYSCONTROL_BASE      (IO_ADDRESS_EPLD_BASE + 0x78000)    /* new name for EPLD_SYSCONTROL_REGS_BASE */
#define IO_ADDRESS_PROFILING_REGS_BASE  (IO_ADDRESS_EPLD_BASE + 0x7FFF0)    /* EPLD profiling registers (EPLD r15 and above) */
#define IO_ADDRESS_EPLD_ID_BASE         (IO_ADDRESS_EPLD_BASE + 0x7FFFC)    /* EPLD ID register */


#define EPLD_SYSCONTROL_POWER_DOWN      (1 << 0)            /* power down the system (only when using ATX power supply) */
#define EPLD_SYSCONTROL_ATA_RESET       (1 << 1)            /* reset EPLD ATA logic and state machines */
#define EPLD_SYSCONTROL_PCCARD_RESET    (1 << 2)            /* reset EPLD pccard logic and state machines */
#define EPLD_SYSCONTROL_USB_RESET       (1 << 3)            /* reset EPLD ISP1581 micro interface logic and state machines */


/*******************************************************************************
*******************************************************************************/

#else
#error "Requested SAA platform not yet implementated"
#endif

/*******************************************************************************
*******************************************************************************/

#endif

