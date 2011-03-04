/*
 * linux/arch/arm/mach-pnx0106/tsc.c
 *
 * Copyright (C) 2003-2005 Andre McCurdy, Philips Semiconductors.
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
#include <linux/vmalloc.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/tsc.h>


#define DEVICE_NAME	"tsc"

#define TIMER_CLK_HZ	(TIMER1_CLK_HZ)
#define TICKS_PER_USEC	((TIMER_CLK_HZ + 500000) / 1000000)


/*****************************************************************************
*****************************************************************************/

/*
   A 64bit tsc is emulated using the second of the PNX0106's internal 32bit
   counter/timers (the jiffies timer tick is driven by the first)
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
#ifdef CONFIG_PNX0106_TSCDEV
static volatile unsigned int *irq_count_ptr = 0;
#endif

/*
   Return current TSC value (entire 64bits).
   HW counts down, so we need a subtraction to get an incrementing value.
*/
unsigned long long ReadTSC64 (void)
{
   unsigned int upper, lower1, lower2;

   do {
      lower1 = readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE);
      upper = irq_count;
      lower2 = readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE);
   }
   while (lower2 > lower1);                                     /* NB: timer counts downwards... */

//   return (((unsigned long long) upper) << 32) | (0 - lower2);
//   return (((unsigned long long) upper) << 32) | (-lower2);
//   return (((unsigned long long) upper) << 32) | (~lower2 + 1);
//   return (((unsigned long long) upper) << 32) | (~lower2);
   return (((unsigned long long) upper) << 32) | (unsigned int)(~lower2);
}

/*
   Return current TSC value (lower 32bits only).
   HW counts down, so we need a subtraction to get an incrementing value.
*/
unsigned int ReadTSC (void)
{
   return 0 - readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE);
}

/*
   As well as timing things accurately (which can be done by reading the
   TSC directly) we can also use the TSC to check for timeouts etc.

   To do this we provide some helper functions.
*/

void ssa_timer_set (ssa_timer_t *timer, unsigned int usec)
{
   timer->ticks_initial  = readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE);
   timer->ticks_required = (usec * TICKS_PER_USEC);
}

int ssa_timer_expired (ssa_timer_t *timer)                      /* returns non-zero if timer has expired */
{
   if ((timer->ticks_initial - readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE)) < timer->ticks_required)
      return 0;

   timer->ticks_required = 0;                                   /* timer has expired. make sure it stays that way */
   return 1;
}

unsigned int ssa_timer_ticks (ssa_timer_t *timer)               /* ticks since time was started */
{
   return (timer->ticks_initial - readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE));
}

static void tsc_isr (int irq, void *dev_id, struct pt_regs *regs)
{
   writel (1, IO_ADDRESS_TIMER1_BASE + OFFSET_CLEAR);           /* writing any value to Clear reg clears int request */
   irq_count++;

#ifdef CONFIG_PNX0106_TSCDEV
   if (irq_count_ptr)
   {
       (*irq_count_ptr)++;

       /*
        * we need to flush the modified page out of the cache so that
        * the user mappings can see it.  flush_dcache_page would
        * ordinarily be the right thing to do, but it chokes when called
        * from interrupt context, so we punt and flush the whole
        * bloody cache.  This will happen once every ~30 seconds so a
        * better way should probably be found.
	*/
       /* flush_dcache_page(virt_to_page(irq_count_ptr)); */
       flush_cache_all();
   }
#endif
}

static int __init tsc_init_ct1 (void)
{
   int result;
   unsigned int timer_control;

#if 1
    if (bootstart_tsc != 0)
        printk ("Bootstart to tsc_init: %d usec\n",
                (bootstart_tsc - readl (IO_ADDRESS_TIMER1_BASE + OFFSET_VALUE)) / ((TIMER_CLK_HZ + 500000) / 1000000));
#endif

   timer_control = (TIMER_ENABLE | TIMER_MODE_PERIODIC | TIMER_DIVIDE_1);

   writel (0xFFFFFFFF,    IO_ADDRESS_TIMER1_BASE + OFFSET_LOAD);    /* a write to 'Load' loads both 'Load' and 'Value' HW registers */
   writel (timer_control, IO_ADDRESS_TIMER1_BASE + OFFSET_CONTROL);
   writel (timer_control, IO_ADDRESS_TIMER1_BASE + OFFSET_CLEAR);   /* any write to 'Clear' removes an active int request (actual value is ignored) */

   result = request_irq (IRQ_TIMER_1, tsc_isr, SA_INTERRUPT, DEVICE_NAME, NULL);
   if (result < 0) {
      printk ("%s: Couldn't register irq %d\n", DEVICE_NAME, IRQ_TIMER_1);
      return result;
   }

   return 0;
}


/*****************************************************************************
*****************************************************************************/

/*
   Return current TSC tick rate (in MHz - ie ticks per uSec).
*/
unsigned int ReadTSCMHz (void)
{
   return TICKS_PER_USEC;
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
   return sprintf (buf, "%d\n", TIMER_CLK_HZ);
}

#endif

extern int proc_er_status_read (char *page, char **start, off_t off, int count, int *eof, void *data);

#ifdef CONFIG_PNX0106_TSCDEV

#define PNX0106_TSC_MINOR		230
#define PNX0106_TSC_DEVNAME		"pnx0106_tsc"

struct pnx0106_tsc_state
{
    spinlock_t lock;
    void *tsc_page;
    unsigned long tsc_page_bus_addr;
};

struct pnx0106_tsc_state pnx0106_tsc_state;

static int
pnx0106_tsc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
//    printk(DEVICE_NAME ": ioctl: cmd 0x%08x\n", cmd);

    switch (cmd)
    {
    case PNX0106_TSC_MMAPSIZE:
	{
	    int size = 2*PAGE_SIZE;

	    if (copy_to_user((void *)arg, &size, sizeof size))
		return -EFAULT;
	}

	return 0;
    case PNX0106_TSC_READTSC:
	{
	    unsigned long long tsc = ReadTSC64();;

	    if (copy_to_user((void *)arg, &tsc, sizeof tsc))
		return -EFAULT;
	}

	return 0;
    case PNX0106_TSC_READTSCHZ:
	{
	    unsigned long long tschz = TIMER_CLK_HZ;

	    if (copy_to_user((void *)arg, &tschz, sizeof tschz))
		return -EFAULT;
	}

	return 0;
    }

    printk(DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
    return -EINVAL;
}

static int
pnx0106_tsc_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct pnx0106_tsc_state *state = &pnx0106_tsc_state;
    int i;
    int size = 0;
    int vmsize = vma->vm_end - vma->vm_start;

//    printk(DEVICE_NAME ": mmap\n");

    if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
	return -EINVAL;

    if (pgprot_val(vma->vm_page_prot) & L_PTE_WRITE)
	return -EINVAL;

    /* This is an IO map */
    vma->vm_flags |= VM_IO;

    // remap each page
    for (i = vma->vm_pgoff; i < 2; i++)
    {
	unsigned int bus_addr;

    	if (size >= vmsize)
	    break;

	switch (i)
	{
	case 0:		// map page with count value
	    {
		if (state->tsc_page == 0)
		{
		    void *dma_alloc_coherent(void *p, int size, dma_addr_t *phys, int flags);
		    struct pnx0106_tsc_info *tscinfo;
		    void *page;
		    unsigned int bus_addr;
		    unsigned long flags;

		    page = dma_alloc_coherent(NULL, PAGE_SIZE, &bus_addr, 0);

		    if (page == 0)
			return -ENOMEM;

		    tscinfo = (struct pnx0106_tsc_info *)page;
		    tscinfo->timeroffset = PAGE_SIZE + (TIMER1_REGS_BASE + OFFSET_VALUE) % PAGE_SIZE;
		    tscinfo->tschz = TIMER_CLK_HZ;

		    spin_lock_irqsave(&state->lock, flags);

		    if (state->tsc_page == 0)
		    {
			tscinfo->count = irq_count;
			state->tsc_page = page;
			state->tsc_page_bus_addr = bus_addr;
			irq_count_ptr = &tscinfo->count;
		    }
		    else
		    {
			/* we lost the race and someone else set thing up
			 * at the same time we were trying too.  No harm done.
			 */
			tscinfo = NULL;
		    }

		    spin_unlock_irqrestore(&state->lock, flags);

		    if (tscinfo == NULL)
			dma_free_coherent(NULL, PAGE_SIZE, page, bus_addr);

//		    printk(DEVICE_NAME ": page %p bus_addr %x offset %d\n", page, bus_addr, tscinfo->timeroffset);
		}

//		printk(DEVICE_NAME ": remap start %p bus %x size %d prot %x\n",
//			vma->vm_start + size, state->tsc_page_bus_addr, PAGE_SIZE,
//			vma->vm_page_prot);

		if (remap_page_range(vma->vm_start + size, state->tsc_page_bus_addr, PAGE_SIZE,
			vma->vm_page_prot))
		    return -EAGAIN;
	    }

	    break;
	case 1:		// map page with timer
	    {
		bus_addr = TIMER1_REGS_BASE + OFFSET_VALUE;
		bus_addr -= (TIMER1_REGS_BASE + OFFSET_VALUE) % PAGE_SIZE;

//		printk(DEVICE_NAME ": remap start %p bus %x size %d prot %x\n",
//			vma->vm_start + size, bus_addr, PAGE_SIZE,
//			vma->vm_page_prot);

		if (remap_page_range(vma->vm_start + size, bus_addr, PAGE_SIZE,
			pgprot_noncached(vma->vm_page_prot)))
		    return -EAGAIN;
	    }

	    break;
	default:
	    return -EINVAL;
	}

	size += PAGE_SIZE;
    }

    return 0;
}

static int
pnx0106_tsc_open(struct inode *inode, struct file *file)
{
    struct pnx0106_tsc_state *state = &pnx0106_tsc_state;
//    printk("pnx0106_tsc_open: start\n");

    if ((file->f_mode & (FMODE_WRITE|FMODE_READ)) != FMODE_READ) {
	    printk(DEVICE_NAME ": tsc device can only be used in RO mode\n");
	    return -EINVAL;
    }

    return 0;
}

static int
pnx0106_tsc_release(struct inode *inode, struct file *file)
{
    struct pnx0106_tsc_state *state = &pnx0106_tsc_state;
//    printk("pnx0106_tsc_release: goodbye !!\n");
    return 0;
}

static struct file_operations pnx0106_tsc_fops =
{
//   .llseek            = pnx0106_tsc_llseek,
//   .read              = pnx0106_tsc_read,
//   .write             = pnx0106_tsc_write,
//   .readdir           = pnx0106_tsc_readdir,
//   .poll              = pnx0106_tsc_poll,
   .ioctl             = pnx0106_tsc_ioctl,
   .mmap              = pnx0106_tsc_mmap,
   .open              = pnx0106_tsc_open,
//   .flush             = pnx0106_tsc_flush,
   .release           = pnx0106_tsc_release,
//   .fsync             = pnx0106_tsc_fsync,
//   .fasync            = pnx0106_tsc_fasync,
//   .lock              = pnx0106_tsc_lock,
//   .readv             = pnx0106_tsc_readv,
//   .writev            = pnx0106_tsc_writev,
//   .sendpage          = pnx0106_tsc_sendpage,
//   .get_unmapped_area = pnx0106_tsc_get_unmapped_area,
   .owner             = THIS_MODULE,
};
static struct miscdevice pnx0106_tsc_misc_device =
{
   PNX0106_TSC_MINOR, PNX0106_TSC_DEVNAME, &pnx0106_tsc_fops
};
#endif


static int __init tsc_init (void)
{
#ifdef CONFIG_PNX0106_TSCDEV
    struct pnx0106_tsc_state *state = &pnx0106_tsc_state;
#endif
    int result;

    if ((result = tsc_init_ct1()) < 0)
        return result;

#ifdef CONFIG_PROC_FS
    if (!create_proc_read_entry ("tsc", 0, NULL, tsc_proc_read, NULL))
        return -ENOMEM;
    if (!create_proc_read_entry ("tschz", 0, NULL, tschz_proc_read, NULL))
        return -ENOMEM;
#endif
#ifdef CONFIG_PNX0106_TSCDEV
    // register minor numbers
    state->lock = SPIN_LOCK_UNLOCKED,
    state->tsc_page = NULL;
    state->tsc_page_bus_addr = 0;
    result = misc_register (&pnx0106_tsc_misc_device);

    if (result < 0)
    {
	printk(DEVICE_NAME ": Can't register misc minor no %d\n", PNX0106_TSC_MINOR);
	return result;
    }
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

