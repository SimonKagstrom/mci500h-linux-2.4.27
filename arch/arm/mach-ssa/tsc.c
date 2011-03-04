/*
 * linux/arch/arm/mach-ssa/tsc.c
 *
 * Copyright (C) 2003-2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/*
   Use the second HW counter/timer to provide a 64bit Time Stamp Counter (TSC).
   It is useful for profiling and high precision timing etc.
*/

#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/arch/tsc.h>


#define DEVICE_NAME "tsc"

static unsigned int ticks_per_usec;


/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_SSA_HAS7752)

/*
   The HAS7752 EPLD (r14 and above) provides a freerunning 64bit counter
   in hardware. We use it if possible...
*/

/*
   Return current TSC value (entire 64bits).
   The 64bit value is spread across 4 16bit registers in the EPLD.
   Reading the first (ie bits [0..15]) causes the entire value to be latched.
*/
unsigned long long ReadTSC64 (void)
{
   unsigned long flags;
   unsigned long long tsc;

   local_irq_save (flags);
   tsc = *((volatile unsigned long long *) IO_ADDRESS_TSC64_REGS_BASE);     /* is there a more 'correct' way ?? */
   local_irq_restore (flags);

   return tsc;
}

/*
   Return current TSC value (lower 32bits only).
*/
unsigned int ReadTSC (void)
{
   return ReadTSC64();
}

/*
   As well as timing things accurately (which can be done by reading the
   TSC directly) we can also use the TSC to check for timeouts etc.
   
   To do this we provide some helper functions.
*/

void ssa_timer_set (ssa_timer_t *timer, unsigned int usec)
{
   timer->ticks_initial = ReadTSC();
   timer->ticks_required = (usec * ticks_per_usec);
}

int ssa_timer_expired (ssa_timer_t *timer)                      /* returns non-zero if timer has expired */
{
   if ((ReadTSC() - timer->ticks_initial) < timer->ticks_required)
      return 0;
   
   timer->ticks_required = 0;                                   /* timer has expired. make sure it stays that way */
   return 1;
}

unsigned int ssa_timer_ticks (ssa_timer_t *timer)               /* ticks since time was started */
{
   return (ReadTSC() - timer->ticks_initial);
}

static int __init tsc_init_epld (void)
{
   return 0;
}

#define TSC_HW_INIT tsc_init_epld


/*****************************************************************************
*****************************************************************************/

#else

/*
   For targets other than the HAS7752, the 64bit tsc is emulated using
   the second of the SAA7752's internal 32bit counter/timers (the jiffies
   timer tick is driven by the first)
*/

/*
   HW timer register offsets
*/

#define OFFSET_LOAD     0x00
#define OFFSET_VALUE    0x04
#define OFFSET_CONTROL  0x08
#define OFFSET_CLEAR    0x0C

/*
   Definitions for use with the timer Control register
*/

#define TIMER_ENABLE            (1 << 7)
#define TIMER_MODE_FREERUNNING  (0 << 6)
#define TIMER_MODE_PERIODIC     (1 << 6)
#define TIMER_DIVIDE_1          (0 << 2)
#define TIMER_DIVIDE_16         (1 << 2)
#define TIMER_DIVIDE_256        (2 << 2)


static volatile unsigned int irq_count;

/*
   Return current TSC value (entire 64bits).
   HW counts down, so we need a subtraction to get an incrementing value.
*/
unsigned long long ReadTSC64 (void)
{
   unsigned int upper, lower1, lower2;

   do {
      lower1 = readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE);
      upper = irq_count;
      lower2 = readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE);
   }
   while (lower2 > lower1);                                     /* NB: timer counts downwards... */

   return (((unsigned long long) upper) << 32) | (0 - lower2);
}

/*
   Return current TSC value (lower 32bits only).
   HW counts down, so we need a subtraction to get an incrementing value.
*/
unsigned int ReadTSC (void)
{
   return 0 - readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE);
}

/*
   As well as timing things accurately (which can be done by reading the
   TSC directly) we can also use the TSC to check for timeouts etc.
   
   To do this we provide some helper functions.
*/

void ssa_timer_set (ssa_timer_t *timer, unsigned int usec)
{
   timer->ticks_initial  = readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE);
   timer->ticks_required = (usec * ticks_per_usec);
}

int ssa_timer_expired (ssa_timer_t *timer)                      /* returns non-zero if timer has expired */
{
   if ((timer->ticks_initial - readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE)) < timer->ticks_required)
      return 0;
   
   timer->ticks_required = 0;                                   /* timer has expired. make sure it stays that way */
   return 1;
}

unsigned int ssa_timer_ticks (ssa_timer_t *timer)               /* ticks since time was started */
{
   return (timer->ticks_initial - readl (IO_ADDRESS_CT1_BASE + OFFSET_VALUE));
}

static void tsc_isr (int irq, void *dev_id, struct pt_regs *regs)
{
   writel (1, IO_ADDRESS_CT1_BASE + OFFSET_CLEAR);              /* writing any value to Clear reg clears int request */
   irq_count++;
}

static int __init tsc_init_ct1 (void)
{
   int result;
   unsigned int timer_control;

   timer_control = (TIMER_ENABLE | TIMER_MODE_PERIODIC | TIMER_DIVIDE_1);

   writel (0xFFFFFFFF,    IO_ADDRESS_CT1_BASE + OFFSET_LOAD);       /* a write to 'Load' loads both 'Load' and 'Value' HW registers */
   writel (timer_control, IO_ADDRESS_CT1_BASE + OFFSET_CONTROL);
   writel (timer_control, IO_ADDRESS_CT1_BASE + OFFSET_CLEAR);      /* any write to 'Clear' removes an active int request (actual value is ignored) */

   result = request_irq (INT_CT1, tsc_isr, SA_INTERRUPT, DEVICE_NAME, NULL);
   if (result < 0) {
      printk ("%s: Couldn't register irq %d\n", DEVICE_NAME, INT_CT1);
      return result;
   }

   return 0;
}

#define TSC_HW_INIT tsc_init_ct1

#endif


/*****************************************************************************
*****************************************************************************/

/*
   Return current TSC tick rate (in MHz - ie ticks per uSec).
*/
unsigned int ReadTSCMHz (void)
{
   return ticks_per_usec;
}

/*
   Use the TSC to provide a very accurate busy-wait function.
   Its probably better to use the generic udelay() in most cases, but this more
   accurate and maybe useful for something...
*/
void ssa_udelay (unsigned int usec)                             /* accurate busy wait for usec micro seconds */
{
   ssa_timer_t timer;
   
   ssa_timer_set (&timer, usec);
   do { /* nothing */ } while (!(ssa_timer_expired (&timer)));
}


#ifdef CONFIG_PROC_FS

static int tsc_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
   unsigned long long tsc = ReadTSC64();
   unsigned int upper = tsc >> 32;
   unsigned int lower = tsc;
   
   *eof = 1;
   return sprintf (buf, "0x%08x%08x\n", upper, lower);
}

static int tschz_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
   *eof = 1;
   return sprintf (buf, "%d\n", hclkhz);
}

#endif


static int __init tsc_init (void)
{
   int result;

   ticks_per_usec = ((hclkhz + 500000) / 1000000);

   if ((result = TSC_HW_INIT()) < 0)
      return result;

#ifdef CONFIG_PROC_FS
   if (!create_proc_read_entry ("tsc", 0, NULL, tsc_proc_read, NULL))
      return -ENOMEM;
   if (!create_proc_read_entry ("tschz", 0, NULL, tschz_proc_read, NULL))
      return -ENOMEM;
#endif

   return 0;
}

__initcall (tsc_init);


EXPORT_SYMBOL(ReadTSC);
EXPORT_SYMBOL(ReadTSC64);
EXPORT_SYMBOL(ReadTSCMHz);
EXPORT_SYMBOL(ssa_timer_set);
EXPORT_SYMBOL(ssa_timer_expired);
EXPORT_SYMBOL(ssa_timer_ticks);
EXPORT_SYMBOL(ssa_udelay);

