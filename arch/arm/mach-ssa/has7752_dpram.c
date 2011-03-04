/*
 * linux/arch/arm/mach-ssa/dpram.c
 *
 * Copyright (C) 2004 Thomas J. Merritt, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/major.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm-arm/arch-ssa/has7752_dpram.h>


/****************************************************************************
****************************************************************************/

#define DEVICE_NAME     "HAS7752_DPRAM"

#define HAS7752_DPRAM_BASE	((volatile unsigned int *) (IO_ADDRESS_EPLD_BASE + 0x50000))
#define HAS7752_DPRAM_IRQ_BASE	((volatile unsigned int *) (IO_ADDRESS_EPLD_BASE + 0x58000))


/****************************************************************************
****************************************************************************/

static struct has7752_dpram_state
{
   int count;
   spinlock_t lock;
   wait_queue_head_t wq;
   volatile int pending;
   int dpram_size;
}
has7752_dpram_state;



/****************************************************************************
****************************************************************************/

typedef struct
{
   volatile unsigned short DPRAM_intr_pend;        /* RW 0x00 */
   volatile unsigned short DPRAM_intr_enb;         /* RW 0x02 */
   volatile unsigned short DPRAM_intr_assert;      /* WO 0x04 */
}
HAS7752_DPRAM_IRQ_Registers_t;

#define HAS7752_DPRAM_IRQ_Registers ((HAS7752_DPRAM_IRQ_Registers_t *) HAS7752_DPRAM_IRQ_BASE)


/****************************************************************************
****************************************************************************/


/*
   definitions for the DPRAM INTC read and write status registers...
*/

#define HAS7752_DPRAM_PENDING_FILL	(1 << 0)
#define HAS7752_DPRAM_PENDING_DRAIN	(1 << 1)
#define HAS7752_DPRAM_PENDING_WRAP_CFG	(1 << 7)

#define HAS7752_DPRAM_INTENB_FILL	(1 << 0)
#define HAS7752_DPRAM_INTENB_DRAIN	(1 << 1)
#define HAS7752_DPRAM_INTENB_WRAP_CFG	(1 << 7)

#define HAS7752_DPRAM_INTASSERT_WR	(1 << 0)
#define HAS7752_DPRAM_INTASSERT_RD	(1 << 1)


/****************************************************************************
****************************************************************************/


static void has7752_dpram_isr (int irq, void *dev_id, struct pt_regs *regs)
{
   struct has7752_dpram_state *state = &has7752_dpram_state;

   int pending = HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_pend;
   int enb = HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_enb;
//   printk ("has7752_dpram_isr: &pend=%x &enb=%x\n", 
//	 &HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_pend,
//	 &HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_enb);
//   printk ("has7752_dpram_isr: pend=%x enb=%x\n", pending, enb);

   enb = 3;		// XXX: Cheap hack for now

   pending &= enb;

   if (pending)
   {
      /* clear interrupts */
      HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_pend = pending;

      /* wake up any waiters */
      state->pending |= pending;
//      printk("Poll: ISR: PEND %x pending %x\n", pending, state->pending);
      wake_up_interruptible (&state->wq);
   }
}


static int has7752_dpram_open (struct inode *inode, struct file *file)
{
   struct has7752_dpram_state *state = &has7752_dpram_state;
   int result;
   unsigned long flags;

   printk ("has7752_dpram_open: start\n");

   spin_lock (&state->lock);
   if (state->count) {
      spin_unlock (&state->lock);
      return -EBUSY;
   }
   state->count++;
   spin_unlock (&state->lock);
   MOD_INC_USE_COUNT;   

   if ((result = request_irq (INT_DPRAM, has7752_dpram_isr, 0, DEVICE_NAME, NULL)) != 0) {
      printk ("%s: could not register interrupt %d\n", DEVICE_NAME, INT_DPRAM);
      return result;
   }

   spin_lock_irqsave (&state->lock, flags);

   /* disable interrupts */
   HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_enb = 0;

   /* clear any pending interrupts */
   HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_pend = HAS7752_DPRAM_PENDING_FILL|HAS7752_DPRAM_PENDING_DRAIN;

   /* enable interrupts */
   HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_enb = HAS7752_DPRAM_INTENB_FILL|HAS7752_DPRAM_INTENB_DRAIN|
					  HAS7752_DPRAM_INTENB_WRAP_CFG;

   spin_unlock_irqrestore (&state->lock, flags);

   return 0;
}

static unsigned int has7752_dpram_poll (struct file *file,
      struct poll_table_struct *pt)
{
   int mask = 0;
   struct has7752_dpram_state *state = &has7752_dpram_state;

   poll_wait(file, &state->wq, pt);


   if (state->pending & HAS7752_DPRAM_PENDING_FILL)
      mask |= POLLIN | POLLRDNORM;

   if (state->pending & HAS7752_DPRAM_PENDING_DRAIN)
      mask |= POLLOUT | POLLWRNORM;

   if (state->pending & HAS7752_DPRAM_PENDING_WRAP_CFG)
      mask |= POLLPRI;

//   printk("Poll: pending %x mask %x\n", state->pending, mask);
   return mask;
}


static int has7752_dpram_release (struct inode *inode, struct file *file)
{
   struct has7752_dpram_state *state = &has7752_dpram_state;
   unsigned long flags;

//   printk ("has7752_dpram_release: goodbye !!\n");

   spin_lock_irqsave (&state->lock, flags);
   HAS7752_DPRAM_IRQ_Registers->DPRAM_intr_enb = 0;
   spin_unlock_irqrestore (&state->lock, flags);

   free_irq (INT_DPRAM, NULL);

   state->count--;
   MOD_DEC_USE_COUNT;

   return 0;
}

static int has7752_dpram_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
   struct has7752_dpram_state *state = &has7752_dpram_state;
   
   switch (cmd) 
   {
      case HAS7752_DPRAM_GET_SIZE:
         return put_user (state->dpram_size, (int *) arg);

      case HAS7752_DPRAM_CLR_WR:
	 state->pending &= ~HAS7752_DPRAM_PENDING_DRAIN;
//	 printk("Poll: ioctl CLR_WR pending %x\n", state->pending);
	 return 0;

      case HAS7752_DPRAM_CLR_RD:
	 state->pending &= ~HAS7752_DPRAM_PENDING_FILL;
//	 printk("Poll: ioctl CLR_RD pending %x\n", state->pending);
	 return 0;

      case HAS7752_DPRAM_CLR_WRAP_CFG:
	 state->pending &= ~HAS7752_DPRAM_PENDING_WRAP_CFG;
	 return 0;
	
      default:
         printk (DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
         return -EINVAL;
   }
}


static struct file_operations has7752_dpram_fops =
{
//   .llseek  = has7752_dpram_llseek,
//   .read    = has7752_dpram_read,
//   .write   = has7752_dpram_write,
   .poll    = has7752_dpram_poll,
   .ioctl   = has7752_dpram_ioctl,
   .open    = has7752_dpram_open,
   .release = has7752_dpram_release,
// .fsync   = ???,
   .owner   = THIS_MODULE,
};

static struct miscdevice has7752_dpram_misc_device =
{
   DPRAM_MINOR, DEVICE_NAME, &has7752_dpram_fops
};


static int __init has7752_dpram_dev_init (void)
{
   struct has7752_dpram_state *state = &has7752_dpram_state;
   int result;

   init_waitqueue_head (&state->wq);
   state->dpram_size = 3*1024;		// should really read the register here
   state->lock = SPIN_LOCK_UNLOCKED,
   
   result = misc_register (&has7752_dpram_misc_device);
   if (result < 0) {
      printk (DEVICE_NAME ": Can't register misc minor no %d\n", DPRAM_MINOR);
      return result;
   }

   return 0;
}

__initcall(has7752_dpram_dev_init);
