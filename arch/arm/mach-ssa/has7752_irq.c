/*
 *  linux/arch/arm/mach-ssa/has7752_irq.c
 *
 *  Copyright (C) 2002-2004 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <asm/arch/gpio.h>


#define VA_INTC_POLARITY    (IO_ADDRESS_INTC_BASE + 0x00)
#define VA_INTC_MODE0       (IO_ADDRESS_INTC_BASE + 0x04)
#define VA_INTC_MODE1       (IO_ADDRESS_INTC_BASE + 0x08)
#define VA_INTC_CLEAR       (IO_ADDRESS_INTC_BASE + 0x0C)
#define VA_INTC_MASK        (IO_ADDRESS_INTC_BASE + 0x10)
#define VA_INTC_TYPE        (IO_ADDRESS_INTC_BASE + 0x14)
#define VA_INTC_STATUS      (IO_ADDRESS_INTC_BASE + 0x1C)


/*
   Define interrupt controller register bits (only for ints which go directly
   to the interrupt controller - ie mostly on-chip sources rather than GPIOs).
*/

#define INTC_BIT_ATU        (1 <<  1)
#define INTC_BIT_CDB        (1 <<  3)
#define INTC_BIT_CT0        (1 <<  4)
#define INTC_BIT_CT1        (1 <<  5)
#define INTC_BIT_GPIO_IRQ   (1 <<  8)
#define INTC_BIT_IIC_SLAVE  (1 <<  9)
#define INTC_BIT_IIC_MASTER (1 << 10)
#define INTC_BIT_WAKEUP     (1 << 15)
#define INTC_BIT_GPIO_FIQ   (1 << 16)
#define INTC_BIT_GPIO_20    (1 << 17)
#define INTC_BIT_WATCHDOG   (1 << 19)
#define INTC_BIT_GPIO_21    (1 << 22)
#define INTC_BIT_GPIO_22    (1 << 23)
#define INTC_BIT_UART0      (1 << 28)


#if 0
void ssa_dump_irq_regs (void)
{
   unsigned int mask = readl (VA_INTC_MASK);
   unsigned int status = readl (VA_INTC_STATUS);

   printk ("VA_INTC_POLARITY     : 0x%08x %s\n", readl (VA_INTC_POLARITY    ), "(1 == active high / rising edge)");
// printk ("VA_INTC_MODE0        : 0x%08x %s\n", readl (VA_INTC_MODE0       ), "(1 == edge triggered)");
// printk ("VA_INTC_MODE1        : 0x%08x %s\n", readl (VA_INTC_MODE1       ), "(1 == both edges if edge triggered)");
   printk ("VA_INTC_MASK         : 0x%08x %s\n", mask,                         "(1 == source will be masked)");
// printk ("VA_INTC_TYPE         : 0x%08x %s\n", readl (VA_INTC_TYPE        ), "(1 == route to FIQ)");
   printk ("VA_INTC_STATUS       : 0x%08x %s\n", status,                       "(1 == int request is active)");
   printk ("STATUS & ~MASK       : 0x%08x %s\n", status & ~mask,               "(1 == int request is active and unmasked)");
}

EXPORT_SYMBOL(ssa_dump_irq_regs);
#endif


static void intc_int_enable (unsigned int intc_source)
{
   unsigned long flags;

   local_irq_save (flags);
   /* clear mask bit to enable interrupt */
   writel ((readl (VA_INTC_MASK) & ~intc_source), VA_INTC_MASK);
   local_irq_restore (flags);
}      

static void intc_int_disable (unsigned int intc_source)
{
   unsigned long flags;

   local_irq_save (flags);
   /* set mask bit to disable interrupt */
   writel ((readl (VA_INTC_MASK) | intc_source), VA_INTC_MASK);
   local_irq_restore (flags);
}      


/*****************************************************************************
*  Interrupt controller interrupts
*****************************************************************************/

static void mask_irq_CT0 (unsigned int irq)
{
   intc_int_disable (INTC_BIT_CT0);
}

static void unmask_irq_CT0 (unsigned int irq)
{
   intc_int_enable (INTC_BIT_CT0);
}

static void mask_irq_CT1 (unsigned int irq)
{
   intc_int_disable (INTC_BIT_CT1);
}

static void unmask_irq_CT1 (unsigned int irq)
{
   intc_int_enable (INTC_BIT_CT1);
}

static void mask_irq_UART0 (unsigned int irq)
{
   intc_int_disable (INTC_BIT_UART0);
}

static void unmask_irq_UART0 (unsigned int irq)
{
   intc_int_enable (INTC_BIT_UART0);
}

static void mask_irq_ATU (unsigned int irq)
{
   intc_int_disable (INTC_BIT_ATU);
}

static void unmask_irq_ATU (unsigned int irq)
{
   intc_int_enable (INTC_BIT_ATU);
}

static void mask_irq_IIC_MASTER (unsigned int irq)
{
   intc_int_disable (INTC_BIT_IIC_MASTER);
}

static void unmask_irq_IIC_MASTER (unsigned int irq)
{
   intc_int_enable (INTC_BIT_IIC_MASTER);
}

static void mask_irq_IIC_SLAVE (unsigned int irq)
{
   intc_int_disable (INTC_BIT_IIC_SLAVE);
}

static void unmask_irq_IIC_SLAVE (unsigned int irq)
{
   intc_int_enable (INTC_BIT_IIC_SLAVE);
}


/*****************************************************************************
*  GPIO interrupts
*****************************************************************************/

static void mask_irq_ETHERNET (unsigned int irq)
{
   gpio_int_disable (GPIO_ETHERNET_INTRQ);
}

static void unmask_irq_ETHERNET (unsigned int irq)
{
   gpio_int_enable (GPIO_ETHERNET_INTRQ);
}

static void mask_irq_PCMCIA (unsigned int irq)
{
   gpio_int_disable (GPIO_PCMCIA_INTRQ);
}

static void unmask_irq_PCMCIA (unsigned int irq)
{
   gpio_int_enable (GPIO_PCMCIA_INTRQ);
}

static void mask_irq_USBH (unsigned int irq)
{
   gpio_int_disable (GPIO_USBH_INTRQ);
}

static void unmask_irq_USBH (unsigned int irq)
{
   gpio_int_enable (GPIO_USBH_INTRQ);
}


/*****************************************************************************
*  EPLD interrupts
*****************************************************************************/

static void mask_irq_ISP1581 (unsigned int irq)
{
   writew (0, (EPLD_ISP1581_INT_REGS_BASE + 2));
}

static void unmask_irq_ISP1581 (unsigned int irq)
{
   writew (3, (EPLD_ISP1581_INT_REGS_BASE + 2));
}

static void mask_irq_ATA (unsigned int irq)
{
   writew (0, (IO_ADDRESS_ATA_REGS_BASE + 2));          /* driver uses bit 0 only therefore no locking required */
}

static void unmask_irq_ATA (unsigned int irq)
{
   writew (1, (IO_ADDRESS_ATA_REGS_BASE + 2));          /* driver uses bit 0 only therefore no locking required */
}

static void mask_irq_DPRAM (unsigned int irq)
{
   writew (0x0000, (IO_ADDRESS_DPRAM_REGS_BASE + 2));   /* first pass... treat DPRAM as one source */
}

static void unmask_irq_DPRAM (unsigned int irq)
{
   writew (0xFFFF, (IO_ADDRESS_DPRAM_REGS_BASE + 2));   /* first pass... treat DPRAM as one source */
}

static void mask_irq_KTIMER (unsigned int irq)
{
   writew (0x0000, (IO_ADDRESS_KTIMER_REGS_BASE + 6));  /* driver never enables overflow int request, so no locking required */
}

static void unmask_irq_KTIMER (unsigned int irq)
{
   writew (0x0001, (IO_ADDRESS_KTIMER_REGS_BASE + 6));  /* driver never enables overflow int request, so no locking required */
}


/*****************************************************************************
*****************************************************************************/

void __init ssa_init_irq (void)
{
   writel (0xFFFFFFFF, VA_INTC_MASK);

   /* set all on-chip int sources to active high polarity */
   writel (INTC_BIT_ATU          |
           INTC_BIT_CDB          |
           INTC_BIT_CT0          |
           INTC_BIT_CT1          |
           INTC_BIT_GPIO_IRQ     |
           INTC_BIT_IIC_MASTER   |
           INTC_BIT_IIC_SLAVE    |
           INTC_BIT_WAKEUP       |
           INTC_BIT_GPIO_FIQ     |
           INTC_BIT_UART0,
           VA_INTC_POLARITY);

   writel (0xFFFFFFFF, VA_INTC_CLEAR);

   memset (irq_desc, 0, sizeof(irq_desc[0]) * NR_IRQS);

   irq_desc[INT_CT0].valid    = 1;
   irq_desc[INT_CT0].mask_ack = mask_irq_CT0;
   irq_desc[INT_CT0].mask     = mask_irq_CT0;
   irq_desc[INT_CT0].unmask   = unmask_irq_CT0;

   irq_desc[INT_CT1].valid    = 1;
   irq_desc[INT_CT1].mask_ack = mask_irq_CT1;
   irq_desc[INT_CT1].mask     = mask_irq_CT1;
   irq_desc[INT_CT1].unmask   = unmask_irq_CT1;

   irq_desc[INT_UART0].valid    = 1;
   irq_desc[INT_UART0].mask_ack = mask_irq_UART0;
   irq_desc[INT_UART0].mask     = mask_irq_UART0;
   irq_desc[INT_UART0].unmask   = unmask_irq_UART0;

   irq_desc[INT_ATA].valid    = 1;
   irq_desc[INT_ATA].mask_ack = mask_irq_ATA;
   irq_desc[INT_ATA].mask     = mask_irq_ATA;
   irq_desc[INT_ATA].unmask   = unmask_irq_ATA;

   irq_desc[INT_PCMCIA].valid    = 1;
   irq_desc[INT_PCMCIA].mask_ack = mask_irq_PCMCIA;
   irq_desc[INT_PCMCIA].mask     = mask_irq_PCMCIA;
   irq_desc[INT_PCMCIA].unmask   = unmask_irq_PCMCIA;

   irq_desc[INT_USBH].valid    = 1;
   irq_desc[INT_USBH].mask_ack = mask_irq_USBH;
   irq_desc[INT_USBH].mask     = mask_irq_USBH;
   irq_desc[INT_USBH].unmask   = unmask_irq_USBH;

   irq_desc[INT_ATU].valid    = 1;
   irq_desc[INT_ATU].mask_ack = mask_irq_ATU;
   irq_desc[INT_ATU].mask     = mask_irq_ATU;
   irq_desc[INT_ATU].unmask   = unmask_irq_ATU;

   irq_desc[INT_ETHERNET].valid    = 1;
   irq_desc[INT_ETHERNET].mask_ack = mask_irq_ETHERNET;
   irq_desc[INT_ETHERNET].mask     = mask_irq_ETHERNET;
   irq_desc[INT_ETHERNET].unmask   = unmask_irq_ETHERNET;

   irq_desc[INT_IIC_MASTER].valid    = 1;
   irq_desc[INT_IIC_MASTER].mask_ack = mask_irq_IIC_MASTER;
   irq_desc[INT_IIC_MASTER].mask     = mask_irq_IIC_MASTER;
   irq_desc[INT_IIC_MASTER].unmask   = unmask_irq_IIC_MASTER;

   irq_desc[INT_IIC_SLAVE].valid    = 1;
   irq_desc[INT_IIC_SLAVE].mask_ack = mask_irq_IIC_SLAVE;
   irq_desc[INT_IIC_SLAVE].mask     = mask_irq_IIC_SLAVE;
   irq_desc[INT_IIC_SLAVE].unmask   = unmask_irq_IIC_SLAVE;

   irq_desc[INT_ISP1581].valid    = 1;
   irq_desc[INT_ISP1581].mask_ack = mask_irq_ISP1581;
   irq_desc[INT_ISP1581].mask     = mask_irq_ISP1581;
   irq_desc[INT_ISP1581].unmask   = unmask_irq_ISP1581;

   irq_desc[INT_DPRAM].valid    = 1;
   irq_desc[INT_DPRAM].mask_ack = mask_irq_DPRAM;
   irq_desc[INT_DPRAM].mask     = mask_irq_DPRAM;
   irq_desc[INT_DPRAM].unmask   = unmask_irq_DPRAM;

   irq_desc[INT_KTIMER].valid    = 1;
   irq_desc[INT_KTIMER].mask_ack = mask_irq_KTIMER;
   irq_desc[INT_KTIMER].mask     = mask_irq_KTIMER;
   irq_desc[INT_KTIMER].unmask   = unmask_irq_KTIMER;

   gpio_int_init();

   gpio_int_set_type (GPIO_ETHERNET_INTRQ, GPIO_LEVEL_TIGGERED_ACTIVE_HIGH);
   gpio_int_set_type (GPIO_PCMCIA_INTRQ, GPIO_LEVEL_TIGGERED_ACTIVE_LOW);
   gpio_int_set_type (GPIO_EPLD_INTRQ, GPIO_LEVEL_TIGGERED_ACTIVE_HIGH);

   gpio_int_enable (GPIO_EPLD_INTRQ);

   /* it should be safe to only use the GPIO int controller manage gpio ints from here on... */
   intc_int_enable (INTC_BIT_GPIO_IRQ);

// ssa_dump_irq_regs ();
}

