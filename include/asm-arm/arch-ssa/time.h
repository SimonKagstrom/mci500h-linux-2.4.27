/*
 *  linux/include/asm-arm/arch-ssa/time.h
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

static unsigned int ticks_per_usec;

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
   TimerStruct_t *timer = (TimerStruct_t *) IO_ADDRESS_CT0_BASE;
   unsigned int ticks_per_irq; 
   unsigned int ticks1, ticks2, status;

   ticks_per_irq = readl (&timer->Load);
   
   /*
      Double read required to avoid race condition between reading the
      timer and checking for an unhandled timer int request...
   */

   do {
      ticks1 = readl (&timer->Value);
      status = readl (IO_ADDRESS_INTC_BASE + 0x1C);         /* read the interrupt controller status register */
      ticks2 = readl (&timer->Value);
   }
   while (ticks2 > ticks1);                                 /* NB: HW timer counts downwards... */

   ticks1 = ticks_per_irq - ticks2;                         /* NB: HW timer counts downwards... */

   if (status & INTMASK_CT0)                                /* add on reload if int request is pending */
      ticks1 += ticks_per_irq;

   return (ticks1 / ticks_per_usec);                        /* return uSec since last timer irq */
}

/*
   Timer IRQ handler
*/
static void ssa_timer_isr_ct0 (int irq, void *dev_id, struct pt_regs *regs)
{
   TimerStruct_t *timer = (TimerStruct_t *) IO_ADDRESS_CT0_BASE;

   writel (1, &timer->Clear);                               /* writing any value to Clear reg clears int request */

   do_leds();
   do_timer(regs);
   do_profile(regs);
}


/*
   The HAS7752 EPLD provides a dedicated 100 Hz timer especially designed to
   work well as a Linux kernel timer. We use it if possible...
*/

#if defined (CONFIG_SSA_HAS7752)

typedef struct
{
   volatile unsigned short timeoffset;      /* usec since the last (handled) timer interrupt */
   volatile unsigned short control;         /* */
   volatile unsigned short int_status;      /* */
   volatile unsigned short int_enable;      /* */
}
ktimer_t;

#define KTIMER_CONTROL_RESET    (0 << 0)
#define KTIMER_CONTROL_RUN      (1 << 0)
#define KTIMER_CONTROL_100_HZ   (0 << 1)
#define KTIMER_CONTROL_1000_HZ  (1 << 1)
#define KTIMER_INT_JIFFY        (1 << 0)
#define KTIMER_INT_OVERFLOW     (1 << 1)

/*
   Return number of uSec since the last timer interrupt.
   Note: IRQs are disabled by do_gettimeofday() before this function is called.
   
   The ktimer HW tracks how many interrupt requests have been cleared vs how
   many have been generated and 
*/
static unsigned long ssa_gettimeoffset_ktimer_100hz (void)
{
   ktimer_t *ktimer = (ktimer_t *) IO_ADDRESS_KTIMER_REGS_BASE;

   return ktimer->timeoffset;       /* couldn't be simpler... thank you HW designer !! */
}

static unsigned long ssa_gettimeoffset_ktimer_1000hz (void)
{
   ktimer_t *ktimer = (ktimer_t *) IO_ADDRESS_KTIMER_REGS_BASE;

   return ktimer->timeoffset / 10;  /* we could fix this properly in HW, but this is OK for now... */
}

/*
   Timer IRQ handler
*/
static void ssa_timer_isr_ktimer (int irq, void *dev_id, struct pt_regs *regs)
{
   ktimer_t *ktimer = (ktimer_t *) IO_ADDRESS_KTIMER_REGS_BASE;

   writew (KTIMER_INT_JIFFY, &ktimer->int_status);  /* write to status clears request */

   do_leds();
   do_timer(regs);
   do_profile(regs);
   
#ifdef CONFIG_PROC_FS
   {
      extern void epldprof_poll (void);
      epldprof_poll();
   }
#endif

#if 0
   /*
      The overflow bit is set if timeoffset reaches 40000 (ie 40 msec
      in 100 HZ mode, 4 msec in 1000 HZ mode). This should never happen,
      but doesn't do any harm to poll for overflow in debug builds...
      
      Note: this always happens at startup because ints are not enabled
            when the timer is setup so we ignore the first warning.
   */
   if (ktimer->int_status & KTIMER_INT_OVERFLOW) {
      static unsigned int overflows;
      writew (KTIMER_INT_OVERFLOW, &ktimer->int_status);    /* write to status clears request */
      if (overflows)
         printk ("ktimer overflow (int not handled for > 4 jiffies) %d\n", overflows);
      overflows++;
   }
#endif
}

#endif


static inline void setup_timer (void)
{
   TimerStruct_t *timer_ct0;
   unsigned int timer_ct0_reload;
   unsigned int timer_ct0_control;

#if defined (CONFIG_SSA_HAS7752)

   ktimer_t *ktimer;
   unsigned int epld_id = *((unsigned int *) IO_ADDRESS_EPLD_ID_BASE);

   if (((epld_id & 0xFF) >= 3) && ((HZ == 100) || (HZ == 1000)))
   {
      ktimer = (ktimer_t *) IO_ADDRESS_KTIMER_REGS_BASE;

      printk ("Initialising ktimer in %dHz mode\n", HZ);
      ktimer->control = KTIMER_CONTROL_RESET;

      if (HZ == 100) {
         gettimeoffset = ssa_gettimeoffset_ktimer_100hz;
         ktimer->control = KTIMER_CONTROL_RUN | KTIMER_CONTROL_100_HZ;
      }
      else {
         gettimeoffset = ssa_gettimeoffset_ktimer_1000hz;
         ktimer->control = KTIMER_CONTROL_RUN | KTIMER_CONTROL_1000_HZ;
      }
      
      timer_irq.handler = ssa_timer_isr_ktimer;
      setup_arm_irq (INT_KTIMER, &timer_irq);
      ktimer->int_enable = KTIMER_INT_JIFFY;
      return;
   }

#endif

   /*
      ticks_per_usec should be rounded up so that value
      returned by ssa_gettimeoffset() is always rounded down.
   */

   ticks_per_usec = ((hclkhz + 999999) / 1000000);
   timer_ct0_reload = ((hclkhz + HZ/2) / HZ);
   timer_ct0_control = (TIMER_ENABLE | TIMER_MODE_PERIODIC | TIMER_DIVIDE_1);

   gettimeoffset = ssa_gettimeoffset_ct0;
   timer_irq.handler = ssa_timer_isr_ct0;
   setup_arm_irq (INT_CT0, &timer_irq);

   timer_ct0 = (TimerStruct_t *) IO_ADDRESS_CT0_BASE;

   writel (timer_ct0_reload,  &timer_ct0->Load);        /* a write to 'Load' loads both 'Load' and 'Value' HW registers */
   writel (timer_ct0_control, &timer_ct0->Control);
   writel (timer_ct0_control, &timer_ct0->Clear);       /* any write to 'Clear' removes an active int request (actual value is ignored) */
}

#endif

