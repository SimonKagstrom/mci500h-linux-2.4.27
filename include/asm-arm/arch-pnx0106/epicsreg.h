/* (C) Copyright Koninklijke Philips Electronics NV 2004, 2005. 
 *     All rights reserved.
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
#ifndef __ASM_ARCH_VHAL_E7B_AHB_IF_H
#define __ASM_ARCH_VHAL_E7B_AHB_IF_H

/*!
 * Standard include files
 * ----------------------
 */



/*!
 * The E7B Memory Structure
 * 0x00000 - 0x0FFFB X-memory
 * 0x0FFFC - 0x0FFFF DSP Control Registers
 * 0x10000 - 0x1FFFF Y-memory
 * 0x20000 - 0x2FFFF P-memory
 * 0x30000 - 0x3FF3F not used
 * 0x3FF40 - 0x3FFFF User defined registers
 * ----------------------------------------
 */

/*!
 * Register offsets from DSP Control Register - X-memory mapped
 * ------------------------------------------------------------
 */

#define VH_E7B_AHB_IF_REG_INPUT_CC   0x30
#define VH_E7B_AHB_IF_REG_INPUT_CD   0x34
#define VH_E7B_AHB_IF_REG_INPUT_CE   0x38
#define VH_E7B_AHB_IF_REG_INPUT_CF   0x3C
#define VH_E7B_AHB_IF_REG_INPUT_D0   0x40
#define VH_E7B_AHB_IF_REG_INPUT_D1   0x44
#define VH_E7B_AHB_IF_REG_INPUT_D2   0x48
#define VH_E7B_AHB_IF_REG_INPUT_D3   0x4C

#define VH_E7B_AHB_IF_REG_OUTPUT_D4  0x50
#define VH_E7B_AHB_IF_REG_OUTPUT_D5  0x54
#define VH_E7B_AHB_IF_REG_OUTPUT_D6  0x58
#define VH_E7B_AHB_IF_REG_OUTPUT_D7  0x5C
#define VH_E7B_AHB_IF_REG_OUTPUT_D8  0x60
#define VH_E7B_AHB_IF_REG_OUTPUT_D9  0x64
#define VH_E7B_AHB_IF_REG_OUTPUT_DA  0x68
#define VH_E7B_AHB_IF_REG_OUTPUT_DB  0x6C
#define VH_E7B_AHB_IF_REG_OUTPUT_DC  0x70
#define VH_E7B_AHB_IF_REG_OUTPUT_DD  0x74
#define VH_E7B_AHB_IF_REG_OUTPUT_DE  0x78
#define VH_E7B_AHB_IF_REG_OUTPUT_DF  0x7C

#define VH_E7B_AHB_IF_REG_DIO_INPUT0    0x00
#define VH_E7B_AHB_IF_REG_DIO_INPUT1    0x04
#define VH_E7B_AHB_IF_REG_DIO_INPUT2    0x08
#define VH_E7B_AHB_IF_REG_DIO_INPUT3    0x0C
#define VH_E7B_AHB_IF_REG_DIO_INPUT4    0x10
#define VH_E7B_AHB_IF_REG_DIO_INPUT5    0x14
#define VH_E7B_AHB_IF_REG_DIO_INPUT6    0x18
#define VH_E7B_AHB_IF_REG_DIO_INPUT7    0x1C
#define VH_E7B_AHB_IF_REG_DIO_INPUT8    0x20
#define VH_E7B_AHB_IF_REG_DIO_INPUT9    0x24
#define VH_E7B_AHB_IF_REG_DIO_INPUT10   0x28
#define VH_E7B_AHB_IF_REG_DIO_INPUT11   0x2C
#define VH_E7B_AHB_IF_REG_DIO_INPUT12   0x30
#define VH_E7B_AHB_IF_REG_DIO_INPUT13   0x34
#define VH_E7B_AHB_IF_REG_DIO_INPUT14   0x38
#define VH_E7B_AHB_IF_REG_DIO_INPUT15   0x3C
#define VH_E7B_AHB_IF_REG_DIO_INPUT16   0x40
#define VH_E7B_AHB_IF_REG_DIO_INPUT17   0x44

#define VH_E7B_AHB_IF_REG_DIO_OUTPUT0    0x00
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT1    0x04
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT2    0x08
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT3    0x0C
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT4    0x10
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT5    0x14
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT6    0x18
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT7    0x1C
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT8    0x20
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT9    0x24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT10   0x28
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT11   0x2C
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT12   0x30
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT13   0x34

#define VH_E7B_AHB_IF_REG_DMA_IRQ_CNTR          0xC8
#define VH_E7B_AHB_IF_REG_INTC_IRQ_USERFLAG     0xCC
#define VH_E7B_AHB_IF_REG_INTC_SW_CLR           0xD0
#define VH_E7B_AHB_IF_REG_INTC_TEST             0xD4
#define VH_E7B_AHB_IF_REG_INTC_STATUS           0xD8
#define VH_E7B_AHB_IF_REG_INTC_MASK             0xDC
#define VH_E7B_AHB_IF_REG_INTC_MODE             0xE0
#define VH_E7B_AHB_IF_REG_INTC_POLARITY         0xE4
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_2          0xE8
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_1          0xEC
#define VH_E7B_AHB_IF_REG_DSP_IRQ_STACK         0xF0
#define VH_E7B_AHB_IF_REG_DSP_STATUS_2          0xF4
#define VH_E7B_AHB_IF_REG_DSP_STATUS_1          0xF8
#define VH_E7B_AHB_IF_REG_DSP_PROG_CNTR         0xFC

/*!
 * Register offsets from DSP Control Register - IO Space mapped
 * ------------------------------------------------------------
 */

#define VH_E7B_AHB_IF_REG_DSP_CONTROL         0xFC
#define VH_E7B_AHB_IF_REG_DSP_EIR             0xF8

/*!
 * Memory/Register Map
 * -------------------
 */
#ifndef PNX0105_1394

#define VH_E7B_AHB_IF_X_MEM_START            0x00000
#define VH_E7B_AHB_IF_Y_MEM_ROM_START        0x40000
#define VH_E7B_AHB_IF_Y_MEM_RAM_START        0x60000
#define VH_E7B_AHB_IF_P_MEM_ROM_START        0x80000
#define VH_E7B_AHB_IF_P_MEM_RAM_START        0xA0000

#else				/* PNX0105_1394 chip */

#define VH_E7B_AHB_IF_X_MEM_START             0x00000
#define VH_E7B_AHB_IF_X_MEM_RAM_START         0x00000
#define VH_E7B_AHB_IF_Y_MEM_RAM_START         0x40000
#define VH_E7B_AHB_IF_P_MEM_RAM_START         0x80000
#define VH_E7B_AHB_IF_P_MEM_CACHE_START       0x84000
#define VH_E7B_AHB_IF_VIRTUAL_X_MEM_RAM_START 0xC0000

#endif

#define VH_E7B_AHB_IF_REG_E7B_CTRL_X_MAPPED  0x3FF00
#define VH_E7B_AHB_IF_REG_E7B_DIO_MAPPED     0xFFC00
#define VH_E7B_AHB_IF_REG_E7B_CTRL_IO_MAPPED 0xFFF00

/*!
 * VH_INPUT_C0 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_CC_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_CC_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_CC_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_CC_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C1 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_CD_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_CD_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_CD_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_CD_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C2 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_CE_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_CE_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_CE_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_CE_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C3 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_CF_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_CF_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_CF_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_CF_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C4 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_D0_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_D0_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_D0_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_D0_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C5 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_D1_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_D1_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_D1_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_D1_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C6 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_D2_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_D2_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_D2_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_D2_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_INPUT_C7 - Input Register 24 bits - W
 * ----------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INPUT_D3_POS              0
#define VH_E7B_AHB_IF_REG_INPUT_D3_LEN             24
#define VH_E7B_AHB_IF_REG_INPUT_D3_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_INPUT_D3_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D0 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D4_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D4_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D4_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D4_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D1 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D5_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D5_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D5_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D5_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D2 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D6_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D6_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D6_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D6_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D3 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D7_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D7_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D7_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D7_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D4 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D8_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D8_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D8_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D8_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D5 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_D9_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_D9_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_D9_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_D9_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D6 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DA_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DA_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DA_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_DA_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D7 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DB_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DB_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DB_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_DB_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D8 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DC_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DC_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DC_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_DC_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_D9 - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DD_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DD_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DD_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_DD_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_DA - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DE_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DE_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DE_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_OUTPUT_DE_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_OUTPUT_DB - Output Register 24 bits - R
 * ------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DF_POS              0
#define VH_E7B_AHB_IF_REG_OUTPUT_DF_LEN             24
#define VH_E7B_AHB_IF_REG_OUTPUT_DF_MSK     0x00FFFFFF
/*#define VH_E7B_AHB_IF_REG_OUTPUT_DF_VAL              0 */
#define VH_E7B_AHB_IF_REG_OUTPUT_DF_VAL          0x101
/* Bits [31:24] reserved */

/*!
 * VH_DMA_IRQ_COUNTER_VALUE - DMA Conter Value
 * -------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DMA_IRQ_CNTR_POS              0
#define VH_E7B_AHB_IF_REG_DMA_IRQ_CNTR_LEN              6
#define VH_E7B_AHB_IF_REG_DMA_IRQ_CNTR_MSK     0x0000003F
#define VH_E7B_AHB_IF_REG_DMA_IRQ_CNTR_VAL              0
/* Bits [31:6] reserved */

/*!
 * VH_INTC_IRQ_USERFLAG - User flag selection Register
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_IRQ_USERFLAG_POS              0
#define VH_E7B_AHB_IF_REG_INTC_IRQ_USERFLAG_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_IRQ_USERFLAG_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_IRQ_USERFLAG_VAL              1
/* Bits [31:18] reserved */

/*!
 * VH_INTC_SW_CLR - SW clear Register
 * ----------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_SW_CLR_POS              0
#define VH_E7B_AHB_IF_REG_INTC_SW_CLR_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_SW_CLR_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_SW_CLR_VAL              0
/* Bits [31:18] reserved */

/*!
 * VH_INTC_TEST - SW clear Register
 * --------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_TEST_DISABLE_POS              0
#define VH_E7B_AHB_IF_REG_INTC_TEST_DISABLE_LEN              1
#define VH_E7B_AHB_IF_REG_INTC_TEST_DISABLE_MSK     0x00000001

#define VH_E7B_AHB_IF_REG_INTC_TEST_ENABLE_POS               1
#define VH_E7B_AHB_IF_REG_INTC_TEST_ENABLE_LEN               1
#define VH_E7B_AHB_IF_REG_INTC_TEST_ENABLE_MSK      0x00000002

#define VH_E7B_AHB_IF_REG_INTC_TEST_INPUT_POS                2
#define VH_E7B_AHB_IF_REG_INTC_TEST_INPUT_LEN               18
#define VH_E7B_AHB_IF_REG_INTC_TEST_INPUT_MSK       0x0003FFFC

#define VH_E7B_AHB_IF_REG_INTC_TEST_MSK             0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_TEST_VAL                      0
/* Bits [31:20] reserved */

/*!
 * VH_INTC_STATUS - Status Registers
 * ---------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_STATUS_POS              0
#define VH_E7B_AHB_IF_REG_INTC_STATUS_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_STATUS_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_STATUS_VAL              0
/* Bits [31:18] reserved */

/*!
 * VH_INTC_MASK - Mask Registers
 * -----------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_MASK_POS              0
#define VH_E7B_AHB_IF_REG_INTC_MASK_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_MASK_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_MASK_VAL     0x0003FFFF
/* Bits [31:18] reserved */

/*!
 * VH_INTC_TYPE - Type Registers
 * -----------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_MODE_POS              0
#define VH_E7B_AHB_IF_REG_INTC_MODE_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_MODE_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_MODE_VAL     0x0003FFFF
/* Bits [31:18] reserved */

/*!
 * VH_INTC_POLARITY - Polarity Registers
 * -------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_INTC_POLARITY_POS              0
#define VH_E7B_AHB_IF_REG_INTC_POLARITY_LEN             18
#define VH_E7B_AHB_IF_REG_INTC_POLARITY_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_INTC_POLARITY_VAL     0x0003FFFF
/* Bits [31:18] reserved */

/*!
 * VH_DSP_CONFIG_2 - DSP Configuration Register 2
 * ----------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_2_POS              0
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_2_LEN             18
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_2_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_2_VAL     0x00000FFD
/* Bits [31:18] reserved */

/*!
 * VH_DSP_CONFIG_1 - DSP Configuration Register 1
 * ----------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_1_POS              0
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_1_LEN             18
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_1_MSK     0x0003FFFF
#define VH_E7B_AHB_IF_REG_DSP_CONFIG_1_VAL     0x0003C002
/* Bits [31:18] reserved */

/*!
 * VH_DSP_IRQ_STACK - DSP Interrupt Stack Register
 * -----------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_IRQ_STACK_POS              0
#define VH_E7B_AHB_IF_REG_DSP_IRQ_STACK_LEN             16
#define VH_E7B_AHB_IF_REG_DSP_IRQ_STACK_MSK     0x0000FFFF
#define VH_E7B_AHB_IF_REG_DSP_IRQ_STACK_VAL              0
/* Bits [31:16] reserved */

/*!
 * VH_DSP_STATUS_2 - DSP Status Register 2
 * ---------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_STATUS_2_POS              0
#define VH_E7B_AHB_IF_REG_DSP_STATUS_2_LEN              5
#define VH_E7B_AHB_IF_REG_DSP_STATUS_2_MSK     0x0000001F
#define VH_E7B_AHB_IF_REG_DSP_STATUS_2_VAL            0x2
/* Bits [31:5] reserved */

/*!
 * VH_DSP_STATUS_1 - DSP Status Register 1
 * ---------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_STATUS_1_POS              0
#define VH_E7B_AHB_IF_REG_DSP_STATUS_1_LEN             24
#define VH_E7B_AHB_IF_REG_DSP_STATUS_1_MSK     0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DSP_STATUS_1_VAL              0
/* Bits [31:24] reserved */

/*!
 * VH_DSP_PROGRAM_COUNTER - DSP Program Counter Register
 * -----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_PROG_CNTR_POS              0
#define VH_E7B_AHB_IF_REG_DSP_PROG_CNTR_LEN             16
#define VH_E7B_AHB_IF_REG_DSP_PROG_CNTR_MSK     0x0000FFFF
#define VH_E7B_AHB_IF_REG_DSP_PROG_CNTR_VAL              0
/* Bits [31:16] reserved */

/*!
 * VH_GENERAL_CONTROL_REG - DSP Control Register
 * ---------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_BIOS_POS              0
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_BIOS_LEN              1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_BIOS_MSK     0x00000001

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_POS             1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_LEN             1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_MSK    0x00000002

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_POS               2
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_LEN               1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_MSK      0x00000004

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_IDLE_POS              3
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_IDLE_LEN              1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_IDLE_MSK     0x00000008

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_MSK                 0xF
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_VAL                 0x1
/* Bits [31:4] reserved */

/*!
 * VH_EIR_REG - EPICS Instruction Register
 * ---------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DSP_EIR_POS              0
#define VH_E7B_AHB_IF_REG_DSP_EIR_LEN             32
#define VH_E7B_AHB_IF_REG_DSP_EIR_MSK     0xFFFFFFFF
#define VH_E7B_AHB_IF_REG_DSP_EIR_VAL     0xFC000000
/*!
 */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT0 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT0_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT0_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT0_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT0_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT1 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT1_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT1_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT1_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT1_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT2 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT2_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT2_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT2_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT2_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT3 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT4 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT4_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT4_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT3_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT5 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT5_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT5_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT5_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT5_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT6 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT6_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT6_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT6_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT6_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT7 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT7_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT7_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT7_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT7_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT8 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT8_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT8_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT8_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT8_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT9 - DIO Register EPICS7B
 * ---------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT9_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT9_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT9_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT9_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT10 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT10_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT10_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT10_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT10_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT11 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT11_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT11_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT11_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT11_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT12 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT12_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT12_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT12_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT12_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT13 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT13_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT13_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT13_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT13_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT14 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT14_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT14_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT14_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT14_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT15 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT15_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT15_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT15_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT15_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT16 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT16_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT16_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT16_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT16_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_INPUT17 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_INPUT17_POS           0
#define VH_E7B_AHB_IF_REG_DIO_INPUT17_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_INPUT17_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_INPUT17_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT0 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT0_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT0_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT0_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT0_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT1 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT1_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT1_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT1_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT1_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT2 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT2_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT2_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT2_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT2_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT3 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT3_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT3_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT3_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT3_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT4 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT4_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT4_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT4_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT4_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT5 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT5_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT5_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT5_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT5_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT6 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT6_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT6_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT6_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT6_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT7 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT7_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT7_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT7_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT7_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT8 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT8_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT8_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT8_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT8_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT9 - DIO Register EPICS7B
 * ----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT9_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT9_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT9_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT9_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT10 - DIO Register EPICS7B
 * -----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT10_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT10_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT10_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT10_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT11 - DIO Register EPICS7B
 * -----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT11_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT11_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT11_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT11_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT12 - DIO Register EPICS7B
 * -----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT12_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT12_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT12_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT12_VAL           0
/* Bits [31:24] reserved */

/*!
 * VH_E7B_AHB_IF_REG_DIO_OUTPUT13 - DIO Register EPICS7B
 * -----------------------------------------------------
 * Bitfields
 */
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT13_POS           0
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT13_LEN          24
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT13_MSK  0x00FFFFFF
#define VH_E7B_AHB_IF_REG_DIO_OUTPUT13_VAL           0
/* Bits [31:24] reserved */

#ifndef __ASSEMBLY__
/*!
 * Generic Types
 * -------------
 */
#define FREQ_5MHz   5000000
#define FREQ_10MHz 10000000
#define FREQ_24MHz 24000000
#define FREQ_40MHz 40000000
#define FREQ_50MHz 50000000

/*!
 * Global variables
 * ----------------
 */

/*!
 * Function Prototypes (Exported)
 * ------------------------------
 */


#endif /* __ASSEMBLY__ */


#endif				/* __ASM_ARCH_VHAL_E7B_AHB_IF_H */



/*!
 * --------------------
 */
