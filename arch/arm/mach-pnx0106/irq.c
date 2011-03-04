/*
 *  linux/arch/arm/mach-pnx0106/irq.c
 *
 *  Copyright (C) 2002-2006 Andre McCurdy, Philips Semiconductors.
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
#include <asm/arch/event_router.h>


/*
    Interrupt Controller register access definitions...
*/

#define VA_INTC_PRIORITY_MASK_IRQ               (IO_ADDRESS_INTC_BASE + 0x000)
#define VA_INTC_PRIORITY_MASK_FIQ               (IO_ADDRESS_INTC_BASE + 0x004)
#define VA_INTC_VECTOR_IRQ                      (IO_ADDRESS_INTC_BASE + 0x100)
#define VA_INTC_VECTOR_FIQ                      (IO_ADDRESS_INTC_BASE + 0x104)
#define VA_INTC_PENDING(slice)                  (IO_ADDRESS_INTC_BASE + 0x200 + (4 * (slice)))
#define VA_INTC_FEATURES                        (IO_ADDRESS_INTC_BASE + 0x300)
#define VA_INTC_REQUEST(irq_number)             (IO_ADDRESS_INTC_BASE + 0x400 + (4 * (irq_number)))
#define VA_INTC_MODID                           (IO_ADDRESS_INTC_BASE + 0xFFC)


#define INTC_REQUEST_ACTIVE_LOW                 ((1 << 17) | (1 << 25))
#define INTC_REQUEST_ACTIVE_HIGH                ((0 << 17) | (1 << 25))
#define INTC_REQUEST_MASTER_ENABLE              ((1 << 16) | (1 << 26))
#define INTC_REQUEST_MASTER_DISABLE             ((0 << 16) | (1 << 26))
#define INTC_REQUEST_TARGET_IRQ                 ((0 <<  8) | (1 << 27))
#define INTC_REQUEST_TARGET_FIQ                 ((1 <<  8) | (1 << 27))
#define INTC_REQUEST_PRIORITY(prio)          ((prio <<  0) | (1 << 28))             /* prio should be between 0 and 15 */

/*
    Note: priority based masking and the vector registers seem to be
    broken at a HW level (not fully investigated, but interrupt sources
    sometimes seem to be active when they should be masked).
    Every interrupt controller driver I've seen for the PNX0105 / PNX0106
    ignores priority based masking and uses the 'pending' register(s)
    instead of the 'vector' register to identify active interrupt requests.
*/

#define INTC_REQUEST_PRIORITY_LOWEST            (INTC_REQUEST_PRIORITY(0))
#define INTC_REQUEST_PRIORITY_DEFAULT           (INTC_REQUEST_PRIORITY(1))
#define INTC_REQUEST_PRIORITY_HIGHEST           (INTC_REQUEST_PRIORITY(15))

#define INTC_REQUEST_DEFAULT                    (INTC_REQUEST_ACTIVE_HIGH       | \
                                                 INTC_REQUEST_MASTER_DISABLE    | \
                                                 INTC_REQUEST_TARGET_IRQ        | \
                                                 INTC_REQUEST_PRIORITY_DEFAULT)


/*****************************************************************************
*  Interrupt controller access functions
*****************************************************************************/

static void intc_int_enable (unsigned int irq)
{
    unsigned int enable;

    enable = INTC_REQUEST_MASTER_ENABLE;

    if (irq >= FIQ_START) {
        irq -= FIQ_START;
        enable |= INTC_REQUEST_TARGET_FIQ;
        printk ("%s: enabling IRQ %d as an FIQ\n", __FUNCTION__, irq);
    }

    if ((irq == 0) || (irq >= NR_INTC_IRQS))
        return;

    writel (enable, VA_INTC_REQUEST(irq));
}

static void intc_int_disable (unsigned int irq)
{
    unsigned int disable;

    /*
       Unconditionally put FIQs back to IRQs.
       They will become FIQs again if/when re-enabled.
    */
    disable = INTC_REQUEST_MASTER_DISABLE | INTC_REQUEST_TARGET_IRQ;

    if (irq >= FIQ_START)
        irq -= FIQ_START;

    if ((irq == 0) || (irq >= NR_INTC_IRQS))
        return;

    writel (disable, VA_INTC_REQUEST(irq));
}

#if 0
static void intc_int_mask_ack (unsigned int irq)
{
    /* clear int request latch - for edge triggered interrupts only ?? */
}
#endif


/*****************************************************************************
*****************************************************************************/

void __init pnx0106_init_irq (void)
{
    int i;

    memset (irq_desc, 0, sizeof(irq_desc[0]) * NR_IRQS);

    for (i = 0; i < NR_INTC_IRQS; i++) {
        irq_desc[i].mask_ack = intc_int_disable;
        irq_desc[i].mask = intc_int_disable;
        irq_desc[i].unmask = intc_int_enable;
    }

    irq_desc[IRQ_EVENTROUTER_IRQ_0].valid = 1;
    irq_desc[IRQ_EVENTROUTER_IRQ_1].valid = 1;
    irq_desc[IRQ_EVENTROUTER_IRQ_2].valid = 1;
    irq_desc[IRQ_EVENTROUTER_IRQ_3].valid = 1;
    irq_desc[IRQ_TIMER_0].valid = 1;
    irq_desc[IRQ_TIMER_1].valid = 1;
    irq_desc[IRQ_RTC_INC].valid = 1;
    irq_desc[IRQ_RTC_ALARM].valid = 1;
    irq_desc[IRQ_UART_0].valid = 1;
    irq_desc[IRQ_UART_1].valid = 1;
    irq_desc[IRQ_IIC_0].valid = 1;
    irq_desc[IRQ_IIC_1].valid = 1;
    irq_desc[IRQ_E7B2ARM_1].valid = 1;
    irq_desc[IRQ_E7B2ARM_2].valid = 1;
    irq_desc[IRQ_SDMA].valid = 1;
    irq_desc[IRQ_PCCARD].valid = 1;
    irq_desc[IRQ_USB_OTG_IRQ].valid = 1;

    /*
        Init Interrupt Controller
    */

    for (i = 1; i < NR_INTC_IRQS; i++)
        writel (INTC_REQUEST_DEFAULT, VA_INTC_REQUEST(i));

    writel (0, VA_INTC_VECTOR_IRQ);             /* clear table address (bits 11..31) */
    writel (0, VA_INTC_VECTOR_FIQ);             /* clear table address (bits 11..31) */

    writel (0, VA_INTC_PRIORITY_MASK_IRQ);      /* allow interrupts above this priority to generate IRQ requests (0 == don't mask based on priority) */
    writel (0, VA_INTC_PRIORITY_MASK_FIQ);      /* allow interrupts above this priority to generate FIQ requests (0 == don't mask based on priority) */

    init_FIQ();                                 /* ?? */

    /*
        Init Event Router
        Route platform specific GPIO events to interrupt controller
    */
    er_init();

#if defined (GPIO_ETH_INTRQ) && (GPIO_ETH_INTRQ != GPIO_NULL)
    er_add_gpio_input_event (IRQ_ETHERNET, GPIO_ETH_INTRQ, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH);
#endif

#if defined (GPIO_WLAN_INTRQ) && (GPIO_WLAN_INTRQ != GPIO_NULL)
    er_add_gpio_input_event (IRQ_WLAN, GPIO_WLAN_INTRQ, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH);
#endif

#if defined (GPIO_PCMCIA_INTRQ) && (GPIO_PCMCIA_INTRQ != GPIO_NULL)
    er_add_gpio_input_event (IRQ_PCMCIA, GPIO_PCMCIA_INTRQ, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_LOW);
#endif

#if defined (GPIO_KEYPAD_IN_0) && (GPIO_KEYPAD_IN_0 != GPIO_NULL)
    er_add_gpio_input_event (IRQ_KEYPAD, GPIO_KEYPAD_IN_0, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH);
#endif
#if defined (GPIO_KEYPAD_IN_1) && (GPIO_KEYPAD_IN_1 != GPIO_NULL)
    er_add_gpio_input_event (IRQ_KEYPAD, GPIO_KEYPAD_IN_1, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH);
#endif
#if defined (GPIO_KEYPAD_IN_2) && (GPIO_KEYPAD_IN_2 != GPIO_NULL)
    er_add_gpio_input_event (IRQ_KEYPAD, GPIO_KEYPAD_IN_2, EVENT_TYPE_LEVEL_TIGGERED_ACTIVE_HIGH);
#endif

}

