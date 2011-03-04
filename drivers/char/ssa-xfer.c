/*
 * 				SSA-XFER
 *  This driver is used to transfer data from the "shared memory" region
 *  between the two CPUs and userspace.
 *
 *  This driver is registered as a "miscellaneous" characted device and
 *  should be visible under /proc/misc. This way we piggy-back onto the
 *  miscellaneous device MAJOR number.
 *
 *  (C) 2003 Philips Semiconductor.
 *  Author: Ranjit Deshpande
 *
 * $Id: ssa-xfer.c,v 1.1 2003/10/28 01:47:12 ranjitd Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

/* Define XFER_DEBUG if you want debug output */
#undef XFER_DEBUG

#ifdef XFER_DEBUG
static int dbg_level;
#define DBG(lvl, args...)	if (lvl >= dbg_level) printk(args)
#else
#define DBG(lvl, args...)
#endif

#define XFER_DRV_NAME		"ssa-xfer"
#define IRQ_FREQ		3500		/* usecs */

/*
   HW timer register offsets
*/

#define OFFSET_LOAD     0x00
#define OFFSET_VALUE    0x04
#define OFFSET_CONTROL  0x08
#define OFFSET_CLEAR    0x0C


/* The CD-Ripping engine rips CD's at 4x (600KB/sec). Each CD block is
 * ~2kb so data will arrive every ~3.5ms. The "other" CPU is expected to
 * buffer this data somewhat, but preparing for the worst case, we buffer
 * enough data so that the user application when scheduled reads this data
 * and writes it to a file.
 *
 * Assuming that the user application gets around to reading data every 100ms 
 * we need a buffer that can hold 100/3.5 * 2kb = ~58kb. So we allocate a 
 * nicely rounded, page size-aligned, 64KB buffer.
 */
#define XFER_BUF_SZ	(512 * 1024)
#define IO_SHARED_MEM	IO_ADDRESS_ISRAM_BASE

static u8 *			xfer_buf;
static int			head, tail; /* Head and tail for the xfer_buf */
static 				DECLARE_WAIT_QUEUE_HEAD(xfer_wq);
static 				DECLARE_MUTEX(xfer_sem);
#ifdef XFER_DEBUG
static int			last_read;
#endif

static int ssa_xfer_open(struct inode *inode, struct file *filp);
static int ssa_xfer_release(struct inode *inode, struct file *filp);
static ssize_t ssa_xfer_read (struct file *file, char *buf, size_t count, 
							loff_t *offset);
static unsigned int ssa_xfer_poll(struct file *file, poll_table *wait);

/* Structure:	ssa_xfer_fops
 * Description:	Define file operations that can be called on this driver.
 */
struct file_operations ssa_xfer_fops = {
	owner	: THIS_MODULE,
	open	: ssa_xfer_open,
	release	: ssa_xfer_release,
	read	: ssa_xfer_read,
	poll	: ssa_xfer_poll
};

/* Structure:	ssa_xfer_miscdev
 * Description:	Used to register this driver with the miscdevice driver.
 */
struct miscdevice ssa_xfer_miscdev = {
	SSA_XFER_MINOR,
	XFER_DRV_NAME,
	&ssa_xfer_fops
};

/*
 * Function: 	ssa_xfer_buf_used()
 * Arguments:	None.
 * Returns:	Number of bytes in the xfer buffer.
 * Purpose:	Return number of bytes in the xfer buffer.
 * Actions:	Calculate number of bytes in xfer buffer.
 * Context:	Interrupt or User.
 * Locks:	Disable interrupts for reading head and tail pointers.
 */
static int 
ssa_xfer_buf_used(void)
{
	unsigned long flags;
	int len;

	save_and_cli(flags);
	len = (tail - head + XFER_BUF_SZ) % XFER_BUF_SZ;
	restore_flags(flags);
	return len;
}

/*
 * Function: 	ssa_xfer_buf_free()
 * Arguments:	None.
 * Returns:	Number of free bytes in the xfer buffer.
 * Purpose:	Return number of free bytes in the xfer buffer.
 * Actions:	Calculate number of free bytes in xfer buffer.
 * Context:	Interrupt or User.
 * Locks:	Disable interrupts for reading head and tail pointers.
 */
static int 
ssa_xfer_buf_free(void)
{
	unsigned long flags;
	int free;

	save_and_cli(flags);
	free = XFER_BUF_SZ - ((tail - head + XFER_BUF_SZ) % XFER_BUF_SZ) - 1;
	restore_flags(flags);
	return free;
}

/*
 * Function: 	ssa_xfer_interrupt()
 * Arguments:	irq 	- Interrupt Request number
 * 		dev_id	- Pointer to private data
 * 		regs	- Pointer to register contents when interrupt occured.
 * Returns:	Nothing.
 * Purpose:	Handle an interrupt signalling that data is now available in
 * 		shared memory.
 * Actions:	Transfer data to internal buffer, acknowledge interrupt
 * 		and notify userspace that some data is available.
 * Context:	Interrupt
 */
static void 
ssa_xfer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int count, len;
	static unsigned int last;

	/* Ack the timer, remove this hack */
	writel(1, IO_ADDRESS_CT1_BASE + OFFSET_CLEAR);

	/* Check if we are too late */
	if (last) {
		int latency, ticks_per_jiffy;

		ticks_per_jiffy = HCLK_MHZ * 10000;
		latency = last - readl(IO_ADDRESS_CT0_BASE + OFFSET_VALUE);
		/* Account for wrap around */
		latency = (latency + ticks_per_jiffy) % ticks_per_jiffy;
		latency -= IRQ_FREQ * HCLK_MHZ;

		if (latency > (int)(IRQ_FREQ * HCLK_MHZ))
			printk(KERN_ERR "%s: Interrupt latency greater "
					"than %dms: %dusec\n", XFER_DRV_NAME,
					IRQ_FREQ/1000, latency/HCLK_MHZ);
	}
	last = readl(IO_ADDRESS_CT0_BASE + OFFSET_VALUE);

	/* How many bytes did we get ? */
	count = 2252;

	/* Check if we have enough space in the buffer */
	if (ssa_xfer_buf_free() < count) {
		printk(KERN_ERR "%s: Not enough space in internal buffer. "
				"Dropping %d bytes of data.\n", XFER_DRV_NAME,
				count);
		return;
	}
	
	/* Copy bytes from shared memory to internal buffer */
	len = (tail + count > XFER_BUF_SZ) ? XFER_BUF_SZ - tail : count;

	memcpy(&xfer_buf[tail], (void *)IO_SHARED_MEM, len);
	if (tail + len > XFER_BUF_SZ) 
		memcpy(&xfer_buf[0], (void *)(IO_SHARED_MEM + len), count-len);
	tail = (tail + count) % XFER_BUF_SZ;

	/* Wakeup user space */
	wake_up_interruptible(&xfer_wq);
}

static int
ssa_xfer_open(struct inode *inode, struct file *filp)
{
	int ret;
	static int first;

	if (down_trylock(&xfer_sem))
		return -EBUSY;

	/* Hack to free TSC IRQ, take this out */
	if (!first) {
		disable_irq(INT_CT1);
		free_irq(INT_CT1, 0);
		first = 1;
	}

	/* Generate an IRQ every 3ms */
   	writel (IRQ_FREQ*HCLK_MHZ, IO_ADDRESS_CT1_BASE + OFFSET_LOAD);
   	if ((ret = request_irq(INT_CT1, ssa_xfer_interrupt, 0,
					"tsc", NULL))) {
		up(&xfer_sem);
		return ret;
	}

	/* Purge current buffer contents */
	head = tail = 0;

	MOD_INC_USE_COUNT;
	return 0;
}

static int
ssa_xfer_release(struct inode *inode, struct file *filp)
{
	free_irq(INT_CT1, 0);
   	writel (0xffffffff, IO_ADDRESS_CT1_BASE + OFFSET_LOAD);
	up(&xfer_sem);
	MOD_DEC_USE_COUNT;
	return 0;
}

/*
 * Function: 	ssa_xfer_read()
 * Arguments:	file 	- Pointer to file structure
 * 		buf	- User buffer where data will be read
 * 		count	- Number of bytes to be read
 * 		offset	- File offset to read from.
 * Returns:	[*] Number of bytes read on success.
 * 		[*] -ESPIPE if current file offset is not equal to 'offset'.
 * 		[*] -EAGAIN if file was opened in non-blocking mode and there
 * 		    isn't enough data to be read.
 * 		[*] -ERESTARTSYS if a signal was received.
 * Purpose:	Transfer data from internal buffer to user buffer
 * Actions:	Check if data is available and then transfer it.
 * Context:	User.
 */
static ssize_t 
ssa_xfer_read (struct file *file, char *buf, size_t count, loff_t *offset)
{
	int len, ret = count;

	/* No seeking allowed */
	if (offset != &file->f_pos)
		return -ESPIPE;

	/* Check if there is enough data in the buffer */
	while (ssa_xfer_buf_used() < count) {
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto rdone;
		}

		if (wait_event_interruptible(xfer_wq, 
						ssa_xfer_buf_used() >= count)) {
			ret = -ERESTARTSYS;
			goto rdone;
		}
	}

	len = (head + count > XFER_BUF_SZ) ? XFER_BUF_SZ - head : count;
	copy_to_user(buf, &xfer_buf[head], len);
	if (head + count > XFER_BUF_SZ)
		copy_to_user(buf + len, &xfer_buf[0], count - len);
	head = (head + count) % XFER_BUF_SZ;
#ifdef XFER_DEBUG
	if (last_read && jiffies - last_read > 2)
		DBG(1, "Last read was %ld jiffies ago\n", jiffies - last_read);
	last_read = jiffies;
#endif
rdone:
	return ret;
}

/*
 * Function: 	ssa_xfer_poll()
 * Arguments:	file		- Pointer to file structure
 * 		poll_table 	- Poll table.
 * Returns:	[*] 0 on success.
 * Purpose:	Wait till some data becomes available.
 * Actions:	Same as above.
 * Context:	User
 */
static unsigned int 
ssa_xfer_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &xfer_wq, wait);
	return ssa_xfer_buf_used() > 0 ? POLLIN | POLLRDNORM : 0;
}

/*
 * Function: 	ssa_xfer_init()
 * Arguments:	None
 * Returns:	[*] 0 on success.
 * 		[*] -ENOMEM if internal buffers cannot be allocated.
 * Purpose:	Driver initialization
 * Actions:	Allocate memory for internal buffers.
 * Context:	Init or user (if compiled as a module)
 */
static int __init 
ssa_xfer_init(void)
{
	int ret;

	if ((xfer_buf = (u8 *)__get_free_pages(GFP_KERNEL, 
					get_order(XFER_BUF_SZ))) == NULL)
		return -ENOMEM;
	if ((ret = misc_register(&ssa_xfer_miscdev)) != 0) {
		printk(KERN_ERR "%s: Could not register driver.\n",
					XFER_DRV_NAME);
		free_pages((unsigned long)xfer_buf, get_order(XFER_BUF_SZ));
		return ret;
	}
	return 0;
}

/*
 * Function: 	ssa_xfer_exit()
 * Arguments:	None
 * Returns:	Nothing.
 * Purpose:	Driver exit.
 * Actions:	Free allocated memory and unregister driver.
 * Context:	User (if compiled as a module).
 */
static void __exit 
ssa_xfer_exit(void)
{
	free_pages((unsigned long)xfer_buf, get_order(XFER_BUF_SZ));
	misc_deregister(&ssa_xfer_miscdev);
}

MODULE_AUTHOR("Ranjit Deshpande <ranjitd@kenati.com>");
MODULE_DESCRIPTION("Driver to transfer data between shared memory and applications");
MODULE_LICENSE("GPL");

module_init(ssa_xfer_init);
module_exit(ssa_xfer_exit);
