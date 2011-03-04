/*
 * linux/arch/arm/mach-ssa/has7752_epldprof.c
 *
 * Copyright (C) 2004 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/types.h>
#include <asm/errno.h>
#include <asm/uaccess.h>

/*
    Each HW counter is 24bits wide, but only the upper 16bits can be accessed by the CPU.
    To prevent overflows, SW should poll the HW registers regularly and transfer any
    accumulated count to SW counters. Reading the upper 16bits of the HW counters
    causes them to be cleared.
    
    The count represents the number of clock cycles spent accessing the given target
    (not the number of accesses).
*/

struct profiling_counters_hardware
{
    volatile unsigned short ethernet;       /* phys 0x4807FFF0 */
    volatile unsigned short wifi;           /* phys 0x4807FFF2 */
    volatile unsigned short dpram;          /* phys 0x4807FFF4 */
    volatile unsigned short ata;            /* phys 0x4807FFF6 */
};

#define pch ((struct profiling_counters_hardware *) IO_ADDRESS_PROFILING_REGS_BASE)


struct profiling_counters_software
{
    spinlock_t lock;
    int init_complete;
    unsigned long long ethernet;
    unsigned long long wifi;
    unsigned long long dpram;
    unsigned long long ata;
};

static struct profiling_counters_software pcs_global;

#define pcs ((struct profiling_counters_software *) &pcs_global)


/*******************************************************************************
*******************************************************************************/

#ifdef CONFIG_PROC_FS

void epldprof_poll (void)
{
    unsigned long flags;

    if (pcs->init_complete) {
        spin_lock_irqsave (&pcs->lock, flags);
        pcs->ethernet += ((unsigned long long) pch->ethernet) << 8;
        pcs->wifi += ((unsigned long long) pch->wifi) << 8;
        pcs->dpram += ((unsigned long long) pch->dpram) << 8;
        pcs->ata += ((unsigned long long) pch->ata) << 8;
        spin_unlock_irqrestore (&pcs->lock, flags);
    }
    else {
        /* this happens... */
//      printk ("epldprof_poll: called before proc_epldprof_init() has run\n");
    }
}

static void epldprof_clear (void)
{
    unsigned long flags;

    spin_lock_irqsave (&pcs->lock, flags);
    pcs->ethernet = 0;
    pcs->wifi = 0;
    pcs->dpram = 0;
    pcs->ata = 0;
    spin_unlock_irqrestore (&pcs->lock, flags);
}

static int proc_epldprof_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;
    char *p;
    unsigned long flags;
    struct profiling_counters_software pcs_local;

    spin_lock_irqsave (&pcs->lock, flags);
    memcpy (&pcs_local, pcs, sizeof(struct profiling_counters_software));
    spin_unlock_irqrestore (&pcs->lock, flags);
    
    p = page;
    p += sprintf (p, "ethernet\t% *llu\n", 10, pcs_local.ethernet);
    p += sprintf (p, "wifi\t\t% *llu\n", 10, pcs_local.wifi);
    p += sprintf (p, "dpram\t\t% *llu\n", 10, pcs_local.dpram);
    p += sprintf (p, "ata\t\t% *llu\n", 10, pcs_local.ata);

   if ((len = (p - page) - off) < 0)
      len = 0;

   *eof = (len <= count) ? 1 : 0;
   *start = page + off;

   return len;
}

static int proc_epldprof_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
   char sbuf[16 + 1];
   int len;

   if (count > 0) {
      len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
      memset (sbuf, 0, sizeof(sbuf));
      if (copy_from_user (sbuf, buffer, len))
         return -EFAULT;
      if (strcmp (sbuf, "clear") == 0)
         epldprof_clear();
   }

   return count;
}

static int __init proc_epldprof_init (void)
{
   struct proc_dir_entry *res;

   printk ("Initialising HAS7752 EPLD profiling counters\n");

   pcs->lock = SPIN_LOCK_UNLOCKED;
   pcs->init_complete = 1;

   if ((res = create_proc_entry ("epldprof", (S_IWUSR | S_IRUGO), NULL)) == NULL)
      return -ENOMEM;

   res->read_proc = proc_epldprof_read;
   res->write_proc = proc_epldprof_write;

   return 0;
}

__initcall (proc_epldprof_init);


/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(epldprof_poll);

/*******************************************************************************
*******************************************************************************/

#endif

