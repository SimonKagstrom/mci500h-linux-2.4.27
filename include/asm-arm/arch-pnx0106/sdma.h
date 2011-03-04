/*
 * linux/include/asm/arch/sdma.h
 *
 * Copyright (C) 2006 QM Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SDMA_H_
#define __ASM_ARCH_SDMA_H_

#define TRANFER_SIZE_32      0x0
#define TRANFER_SIZE_16      0x1
#define TRANFER_SIZE_8       0x2

#define DEVICE_MEMORY        0
#define DEVICE_IPINT0_TX     1
#define DEVICE_IPINT0_RX     2
#define DEVICE_IPINT1_TX     3
#define DEVICE_IPINT1_RX     4
#define DEVICE_MCI_SREQ      5
#define DEVICE_MCI_BREQ      6
#define DEVICE_UART0_RX      7
#define DEVICE_UART0_TX      8
#define DEVICE_UART1_RX      9
#define DEVICE_UART1_TX      10
#define DEVICE_I2C0          11
#define DEVICE_I2C1          12
#define DEVICE_SPI0          13
#define DEVICE_SPI1          14
#define DEVICE_SAO1L         15
#define DEVICE_SAO1R         16
#define DEVICE_SAO2L         17
#define DEVICE_SAO2R         18
#define DEVICE_SAO3L         19
#define DEVICE_SAO3R         20
#define DEVICE_SAI1L         21
#define DEVICE_SAI1R         22
#define DEVICE_SAI2L         23
#define DEVICE_SAI2R         24
#define DEVICE_SAI3L         25
#define DEVICE_SAI3R         26
#define DEVICE_SAI4L         27
#define DEVICE_SAI4R         28
#define DEVICE_LCD           29
#define DEVICE_MPMC_A19      30
#define DEVICE_MPMC_A17      31

#define ENDIAN_NORMAL        0x0
#define ENDIAN_INVERT        0x1

#define USE_CIRC_BUF         0x1
#define NO_CIRC_BUF          0x0

#define IRQ_FINISHED         0x1
#define IRQ_HALFWAY          0x2

struct sdma_config
{
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned int len;
	unsigned int size;	// 8,16,32 bits
	unsigned int src_name;	// read_slave device
	unsigned int dst_name;	// write_slave device
	unsigned int endian;
	unsigned int circ_buf;
};

extern int sdma_allocate_channel (void);
extern void sdma_free_channel (int chan);
extern void sdma_setup_channel (int chan, struct sdma_config *sdma, void (*handler)(unsigned int));
extern void sdma_config_irq (int chan, unsigned int mask);
extern void sdma_clear_irq (int chan);
extern void sdma_start_channel (int chan);
extern void sdma_stop_channel (int chan);
extern unsigned int sdma_get_count (int chan);
extern void sdma_reset_count (int chan);
extern void sdma_force_shutdown_all_channels (void);

#endif

