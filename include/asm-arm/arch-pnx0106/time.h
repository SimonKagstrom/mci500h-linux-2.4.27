/*
 *  linux/include/asm-arm/arch-pnx0106/time.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_TIME_H_
#define __ASM_ARCH_TIME_H_

#include <linux/config.h>
#include <asm/system.h>
#include <asm/arch/hardware.h>
#include <asm/arch/param.h>

typedef struct
{
    volatile unsigned int Load;
    volatile unsigned int Value;
    volatile unsigned int Control;
    volatile unsigned int Clear;
}
TimerStruct_t;

/*
   TICKS_PER_USEC should be rounded up so that value
   returned by ssa_gettimeoffset() is always rounded down.
*/
#define TIMER_CLK_HZ	(TIMER0_CLK_HZ)
#define TICKS_PER_USEC	((TIMER_CLK_HZ + 999999) / 1000000)


/*
    Definitions for use with the timer Control register
*/

#define TIMER_ENABLE            (1 << 7)
#define TIMER_MODE_FREERUNNING  (0 << 6)
#define TIMER_MODE_PERIODIC     (1 << 6)
#define TIMER_DIVIDE_1          (0 << 2)
#define TIMER_DIVIDE_16         (1 << 2)
#define TIMER_DIVIDE_256        (2 << 2)

/*
     Int request bits in the interrupt controller status register.
     Note that these are the real thing - not the virtual irq numbers used everywhere else...
*/

#define INTMASK_CT0             (1 << 4)
#define INTMASK_CT1             (1 << 5)


/*
    Return number of uSec since the last timer interrupt.
    Note: IRQs are disabled by do_gettimeofday() before this function is called.
    
    Fixme: It might be better to record the TSC during the timer ISR and
           then find elapsed time from that ??
*/
static unsigned long ssa_gettimeoffset_ct0 (void)
{
    TimerStruct_t *timer = (TimerStruct_t *) IO_ADDRESS_TIMER0_BASE;
    unsigned int ticks_per_irq; 
    unsigned int ticks1, ticks2, status;

    ticks_per_irq = readl (&timer->Load);
    
    /*
       Double read required to avoid race condition between reading the
       timer and checking for an unhandled timer int request...
    */

    do {
        ticks1 = readl (&timer->Value);

        /*
            Fixme !!
            We need to read the raw interrupt status here somehow...
            Note that the pending register(s) in the Interrupt controller
            are not suitable here as they contain the status after masking (ie
            if the timer interrupt is masked - which it is when this code runs
            - then there will never appear to be a pending timer interrupt
            even if the timer has expired).
        */

        status = 0;

        ticks2 = readl (&timer->Value);
    }
    while (ticks2 > ticks1);                                 /* NB: HW timer counts downwards... */

    ticks1 = ticks_per_irq - ticks2;                         /* NB: HW timer counts downwards... */

    if (status & INTMASK_CT0)                                /* add on reload if int request is pending */
        ticks1 += ticks_per_irq;

    return (ticks1 / TICKS_PER_USEC);                        /* return uSec since last timer irq */
}

/*
    Timer IRQ handler
*/
static void ssa_timer_isr_ct0 (int irq, void *dev_id, struct pt_regs *regs)
{
    TimerStruct_t *timer = (TimerStruct_t *) IO_ADDRESS_TIMER0_BASE;

    writel (1, &timer->Clear);                               /* writing any value to Clear reg clears int request */

    do_leds();
    do_timer(regs);
    do_profile(regs);
}


static inline void setup_timer (void)
{
    TimerStruct_t *timer_ct0;
    unsigned int timer_ct0_reload;
    unsigned int timer_ct0_control;

    timer_ct0_reload = ((TIMER_CLK_HZ + HZ/2) / HZ);
    timer_ct0_control = (TIMER_ENABLE | TIMER_MODE_PERIODIC | TIMER_DIVIDE_1);

    gettimeoffset = ssa_gettimeoffset_ct0;
    timer_irq.handler = ssa_timer_isr_ct0;
    setup_arm_irq (IRQ_TIMER_0, &timer_irq);

    timer_ct0 = (TimerStruct_t *) IO_ADDRESS_TIMER0_BASE;

    writel (timer_ct0_reload,  &timer_ct0->Load);        /* a write to 'Load' loads both 'Load' and 'Value' HW registers */
    writel (timer_ct0_control, &timer_ct0->Control);
    writel (timer_ct0_control, &timer_ct0->Clear);       /* any write to 'Clear' removes an active int request (actual value is ignored) */
}

#endif

