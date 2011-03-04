/*
 * linux/arch/arm/mach-pnx0106/watchdog.c
 *
 * Copyright (C) 2005-2007 Andre McCurdy, NXP Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/config.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/gpio.h>
#include <asm/arch/watchdog.h>

extern void pnx0106_ide_full_soft_reset (void);


/*****************************************************************************
*****************************************************************************/

#define VA_WDT_IR       (IO_ADDRESS_WATCHDOG_BASE + 0x00)
#define VA_WDT_TCR      (IO_ADDRESS_WATCHDOG_BASE + 0x04)
#define VA_WDT_TC       (IO_ADDRESS_WATCHDOG_BASE + 0x08)
#define VA_WDT_PR       (IO_ADDRESS_WATCHDOG_BASE + 0x0C)
#define VA_WDT_MCR      (IO_ADDRESS_WATCHDOG_BASE + 0x14)
#define VA_WDT_MR0      (IO_ADDRESS_WATCHDOG_BASE + 0x18)
#define VA_WDT_MR1      (IO_ADDRESS_WATCHDOG_BASE + 0x1C)
#define VA_WDT_EMR      (IO_ADDRESS_WATCHDOG_BASE + 0x3C)


#define IR_ENABLE_M0                (1 << 0)
#define IR_ENABLE_M1                (1 << 1)

#define TCR_COUNTER_ENABLE          (1 << 0)
#define TCR_COUNTER_RESET           (1 << 1)    /* does not self clear */

#define MCR_MR0_INTERRUPT_ON_MATCH  (1 << 0)    /* */
#define MCR_MR0_RESTART_ON_MATCH    (1 << 1)    /* */
#define MCR_MR0_STOP_ON_MATCH       (1 << 2)    /* will over-rule RESTART_ON_MATCH if both options are enabled */
#define MCR_MR1_INTERRUPT_ON_MATCH  (1 << 3)    /* */
#define MCR_MR1_RESTART_ON_MATCH    (1 << 4)    /* */
#define MCR_MR1_STOP_ON_MATCH       (1 << 5)    /* will over-rule RESTART_ON_MATCH if both options are enabled */

#define EMR_M0_NOTHING              (0 << 4)
#define EMR_M0_SET_LOW              (1 << 4)
#define EMR_M0_SET_HIGH             (2 << 4)
#define EMR_M0_TOGGLE               (3 << 4)
#define EMR_M1_NOTHING              (0 << 6)
#define EMR_M1_SET_LOW              (1 << 6)
#define EMR_M1_SET_HIGH             (2 << 6)
#define EMR_M1_TOGGLE               (3 << 6)


/****************************************************************************
****************************************************************************/

/*
   Returns : 0 if running from power-on reset
             1 if running from watchdog reset (e.g. soft reset from eco-standby).

   Note: watchdog bark register is in the CGU (ie registers which will
   be mapped and used by the clock driver etc). However, its readonly
   so as a quick solution we can map it twice without locking etc...
*/
unsigned int reset_was_via_watchdog (void)
{
	unsigned int wd_bark = readl (IO_ADDRESS_CGU_CONFIG_REGS_BASE + 0x04);
	return (wd_bark & 0x01) ? 1 : 0;
}


/****************************************************************************
****************************************************************************/

static void (*pre_reset_callback) (void);

void __init watchdog_register_pre_reset_callback (void (*handler) (void))
{
	pre_reset_callback = handler;
}

void watchdog_force_reset (int add_extra_delay)
{
    unsigned long flags;

    local_irq_save (flags);

#if defined (IRQ_RC6RX)
    disable_irq (IRQ_RC6RX);        /* this on could be an FIQ... */
#endif

#if 0
    printk ("%s: Resetting CPU...\n", __FUNCTION__);
    if (pre_reset_callback != NULL) {
        (*pre_reset_callback)();
    }
#endif

    if (add_extra_delay)
        mdelay (200);

#if defined (CONFIG_BLK_DEV_IDE_PNX0106_FULL)
    pnx0106_ide_full_soft_reset();
    udelay (10);
#endif

    /*
       Set mode pins to inputs
       If they are outputs at this point and weakly pulled to a different
       state for booting, we need to give some time for weak pull-up / downs
       to take effect.

       Note that on platforms such as hasli7 and hacli7, changing the state
       of the mode pins will cause the SPI uart to stop working. Therefore
       there should be no debug output after this point (and UART TX fifo
       should already be flushed).
    */
    gpio_set_as_input (GPIO_GPIO_0);
    gpio_set_as_input (GPIO_GPIO_1);
    gpio_set_as_input (GPIO_GPIO_2);
    udelay (10);

    /*
        Reset will be triggered when counter in TC register
        matches the value in Match Register 1...

        External Match Register controls the outputs of the watchdog
        block when a match occurs. Therefore it also needs to be setup
        to generate the correct signal to the CGU to generate a reset.
    */

    // clock_enable_watchdog();

    writel (TCR_COUNTER_RESET, VA_WDT_TCR);                     /* stop counter and assert it's reset */
    writel (0, VA_WDT_PR);                                      /* fastest prescaler */
    writel (1, VA_WDT_MR1);                                     /* set the trap... */
    writel (EMR_M1_SET_HIGH, VA_WDT_EMR);                       /* what to do when it happens (EMR_M1_TOGGLE also works) */
    writel (TCR_COUNTER_ENABLE, VA_WDT_TCR);                    /* start counter (and de-assert reset) */

    /*
       Depending on CPU speed, we may execute a few instructions before
       the watchdog match occurs. Ensure they don't have a bad side effect.
    */

    while (1) ;
}

void __init watchdog_init (void)
{
    // clock_enable_watchdog();

    writel (TCR_COUNTER_RESET, VA_WDT_TCR);                     /* stop counter and assert it's reset */
    writel (0, VA_WDT_PR);                                      /* */
    writel (0, VA_WDT_MCR);                                     /* */
    writel (0, VA_WDT_MR0);                                     /* */
    writel (0, VA_WDT_MR1);                                     /* */
    writel ((EMR_M0_NOTHING | EMR_M1_NOTHING), VA_WDT_EMR);     /* don't generate any outputs even if a match occurs... */

    // clock_disable_watchdog();
}

__initcall (watchdog_init);


