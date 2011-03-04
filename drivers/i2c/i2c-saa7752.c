/*
 * i2c Support for Philips SAA7752 I2C Controller
 * Copyright (c) 2003 Philips Semiconductor
 *
 * Author: Ranjit Deshpande <Ranjit@Kenati.com>
 * 
 * $Id: i2c-saa7752.c,v 1.4 2003/10/17 04:28:32 ranjitd Exp $
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
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

/* Define this for debug messages */
#undef I2C_SAA7752_DEBUG 		/* Master debug */
#undef I2C_SAA7752_SLAVE_DEBUG		/* Slave debug */

#ifdef I2C_SAA7752_DEBUG
#define DBG(args...)	printk(args)
#else
#define DBG(args...)
#endif

#ifdef I2C_SAA7752_SLAVE_DEBUG
#define SDBG(args...)	printk(args)
#else
#define SDBG(args...)
#endif

#include "i2c-saa7752.h"

static int 			scan;		/* Scan bus (or not) */
static int			slave_addr = 0xff;	/* Slave Address */
static i2c_saa7752_mif_t	master_if;	/* Master interface */
static i2c_saa7752_sif_t	slave_if;	/* Slave interface */
static char			def_nodata[] = { 1, 0, 2};

static inline int
i2c_saa7752_slave_buf_len(i2c_saa7752_sif_desc_t *desc)
{
	if (desc->head <= desc->tail)
		return desc->tail - desc->head;
	else
		return SLAVE_BUFFER_SIZE - (desc->head - desc->tail);
}

static inline void 
i2c_saa7752_arm_timer(struct timer_list *timer, void *data)
{
	del_timer_sync(timer);		
	timer->expires = jiffies + TIMEOUT;
	timer->data = (unsigned long)data;
	add_timer(timer);
} 

static inline int 
i2c_saa7752_bus_busy(void)
{
	long timeout_usec = (250 * 1000);		/* fixme... */

	while (timeout_usec > 0 && (master.status & mstatus_active)) {
		udelay (500);
		timeout_usec -= 500;
	}

	return (timeout_usec <= 0) ? 1 : 0;
}

/******************************************************
 * function: i2c_saa7752_start
 *
 * description: Generate a START signal in the desired mode.
 *              I2C is the master.
 *
 *              Return 0 if no error.
 *
 * note:
 ****************************************************/
static int 
i2c_saa7752_start(unsigned char slave_addr, i2c_saa7752_mode_t mode)
{					
	DBG ("%s(%d): %s() addr 0x%x mode %s\n", __FILE__,
		   __LINE__, __FUNCTION__, slave_addr, 
		   i2c_saa7752_modestr[mode]);
	/* Check for 7 bit slave addresses only */
	if (slave_addr & ~0x7f) {
		printk(KERN_ERR "%s(): %s: Invalid slave address %x. "
				"Only 7-bit addresses are supported\n",
				__FUNCTION__, ADAPTER_NAME, slave_addr);
		return -EINVAL;
	}

	/* First, make sure bus is idle */
	if (i2c_saa7752_bus_busy()) {
		/* Somebody else is monopolizing the bus */
		printk(KERN_ERR "%s(): %s: Bus busy. Slave addr = %02x, "
				"IIC_CNTRL = %x IIC_STATUS = %x\n", 
				ADAPTER_NAME, __FUNCTION__, 
				slave_addr, master.cntrl, master.status);
		return -EBUSY;
	} else if (master.status & mstatus_afi) {
		/* Sorry, we lost the bus */
		printk(KERN_ERR "%s: %s(): Arbitration failure. "
				"Slave addr = %02x\n",
				ADAPTER_NAME, __FUNCTION__, slave_addr);
		return -EIO;
	}

	/* OK, I2C is enabled and we have the bus */
	/* Clear any pending interrupts */
	master.status |= mstatus_tdi | mstatus_afi;

	DBG ("%s(%d): %s() sending %#x\n", __FILE__, __LINE__, 
				__FUNCTION__, (slave_addr << 1) | 
						start_bit | 
						(mode == rcv ? rw_bit : 0));

	/* Write the slave address, START bit and R/W bit */
	master.fifo = (slave_addr << 1) | start_bit | 
					(mode == rcv ? rw_bit : 0);

	DBG ("%s(%d): %s() exit\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

/***********************************************************
 * function: i2c_saa7752_stop
 *
 * description: Generate a STOP signal to terminate the master
 *              transaction.
 *              Always returns 0.
 *
 **********************************************************/
static int 
i2c_saa7752_stop(void)
{
	DBG ("%s(%d): %s() enter\n", __FILE__, __LINE__, __FUNCTION__);
	/* Write a STOP bit to TX FIFO */
	master.fifo = 0x000000ff | stop_bit;
	DBG ("%s(%d): %s() exit\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

/****************************************************
 * function: i2c_saa7752_master_xmit
 *
 * description: Master sends one byte of data to
 *              slave target
 *
 *              return 0 if the byte transmitted.
 *
 ***************************************************/
static int 
i2c_saa7752_master_xmit(void)
{
	u32 val;

	if (master_if.len > 0) {
		/* We still have somthing to talk about... */
		val = *master_if.buf++;
		val &= 0x000000ff;

		if (master_if.stop && master_if.len == 1)
			/* Last byte, issue a stop */
			val |= stop_bit;

		master_if.len--;
		master.fifo = val;
		DBG ("%s: xmit %#x [%d]\n", __FUNCTION__, val, master_if.len+1);
		if (master_if.len == 0) {
			/* Disable master interrupts */
			master.cntrl &= ~(mcntrl_afie | mcntrl_naie | 
							mcntrl_drmie);
			del_timer_sync(&master_if.timer);
			DBG("%s: Waking up xfer routine\n", __FUNCTION__);
			complete(&master_if.complete);
		}
	}
	return 0;
}

/***********************************************
 * function: i2c_saa7752_master_rcv
 *
 * description: master reads one byte data
 *              from slave source
 *
 *              return 0 if no error
 *
 ***********************************************/
static int 
i2c_saa7752_master_rcv(void)
{
	unsigned int val = 0;

	DBG ("%s(): Entered\n", __FUNCTION__);
	if (master.status & mstatus_rfe) {
		if (master_if.stop && master_if.len == 1) {
			/* Last byte, issue a stop */
			val |= stop_bit;
			master_if.stop = 0;
		}
		master.fifo = 0;
		return 0;
	}

	if (master_if.len > 0) {
		if (master_if.stop && master_if.len == 1)
			/* Last byte, issue a stop */
			val |= stop_bit;

		master.fifo = val;
		val = master.fifo;
		*master_if.buf++ = (u8)(val & 0xff);
		DBG ("%s: rcv 0x%02x [%02x]\n", __FUNCTION__, val, 
							master_if.len);
		if (--master_if.len == 0) {
			/* Disable master interrupts */
			master.cntrl &= ~(mcntrl_afie | mcntrl_naie | 
								mcntrl_drmie);
			del_timer_sync(&master_if.timer);
			complete(&master_if.complete);
		}
	}
	DBG ("%s: Exit\n", __FUNCTION__);
	return 0;

}

/****************************************************
 * function: i2c_saa7752_slave_xmit
 *
 * description: Slave sends one byte of data to
 *              requesting destination
 *
 *        return 0 if the byte transmitted.
 *
 *
 ***************************************************/
static int 
i2c_saa7752_slave_xmit(void)
{
	unsigned int val;

	if (i2c_saa7752_slave_buf_len(&slave_if.txd) == 0 && 
					slave_if.mode != nodata) {
		slave_if.mode = nodata;
		slave_if.ndi = 0;
	}

	if (slave_if.mode == nodata) {
		if (slave_if.ndi == slave_if.nodata.len) {
			if (i2c_saa7752_slave_buf_len(&slave_if.txd) > 0) {
				slave_if.mode = xmit;
			}
			slave_if.ndi = 0;
		} 
		if (slave_if.mode == nodata) {
			while (!(slave.status & sstatus_tff) && 
					slave_if.ndi != slave_if.nodata.len) {
				SDBG ("%s: No data, writing %d, buf_len = %d\n",
					__FUNCTION__, 
					slave_if.nodata.buf[slave_if.ndi],
					i2c_saa7752_slave_buf_len(&slave_if.txd));
				slave.fifo = slave_if.nodata.buf[slave_if.ndi++];
			}
			return 0;
		}
	}

	SDBG ("%s: xmit 0x%02x, buf_len = %d\n", __FUNCTION__, 
			slave_if.txd.buf[slave_if.txd.head],
			i2c_saa7752_slave_buf_len(&slave_if.txd));
	val = slave_if.txd.buf[slave_if.txd.head];
	slave_if.txd.head = (slave_if.txd.head + 1) % SLAVE_BUFFER_SIZE;
	val &= 0x000000ff;
	slave.fifo = val;
	if (slave_if.txlen && i2c_saa7752_slave_buf_len(&slave_if.txd) == 0) {
		/* no more data to send */
		SDBG("%s: Sending Tx complete\n", __FUNCTION__);
		slave_if.txlen = 0;
		complete(&slave_if.tx_complete);
	}
	return 0;
}

/***********************************************
 * function: i2c_saa7752_slave_rcv
 *
 * description: slave reads one byte data
 *              from master source
 *
 *              return 0 if no error otherwise non-zero
 *
 ***********************************************/
static int 
i2c_saa7752_slave_rcv(void)
{
	unsigned int val;

	if (slave.status & sstatus_rfe) {
		SDBG("Dummy byte for slave\n");
		slave.fifo = 0x000000ff;
		return 0;
	}
	if (i2c_saa7752_slave_buf_len(&slave_if.rxd) < SLAVE_BUFFER_SIZE) {
		val = slave.fifo;
		slave_if.rxd.buf[slave_if.rxd.tail] = (u8)(val & 0xff);
		SDBG ("%s: rcv 0x%02x\n", __FUNCTION__, 
				slave_if.rxd.buf[slave_if.rxd.tail]);
		slave_if.rxd.tail = (slave_if.rxd.tail + 1) % SLAVE_BUFFER_SIZE;
		if (slave_if.rxlen &&
			i2c_saa7752_slave_buf_len(&slave_if.rxd) >= 
							slave_if.rxlen) {
			/* all done */
			SDBG("%s: Sending Rx complete\n", __FUNCTION__);
			slave_if.rxlen = 0;
			complete(&slave_if.rx_complete);
		}
	} 
	return 0;
}

static void 
i2c_saa7752_slave_interrupt(int irq, void *dev_id, 
					struct pt_regs *regs)
{
	SDBG ("%s(): stat = %x ctrl = %x\n", __FUNCTION__, 
			slave.status, slave.cntrl);
	/* Let's see what caused the interrupt */
	if (slave.status & sstatus_drsi) {
		i2c_saa7752_slave_xmit();
	}
	if (!(slave.status & sstatus_rfe)) {
		i2c_saa7752_slave_rcv();
	} 
}

static void 
i2c_saa7752_master_interrupt(int irq, void *dev_id, 
					struct pt_regs *regs)
{
	unsigned long stat;

	DBG ("%s(): stat = %x ctrl = %x, mode = %s\n", __FUNCTION__, 
			master.status, master.cntrl, 
			i2c_saa7752_modestr[master_if.mode]);
	stat = master.status;
	/* Now let's see what kind of event this is */
	if (stat & mstatus_afi) {
		/* We lost arbitration in the midst of a transfer */
		master_if.ret = -EIO;
	} 
	else if (stat & mstatus_nai) {
		/* Slave did not acknowledged, generate a STOP */
		i2c_saa7752_stop();
	} else if (stat & mstatus_drmi) {
		if (master_if.mode == xmit) {
			i2c_saa7752_master_xmit();
		} else if (master_if.mode == rcv) {
			i2c_saa7752_master_rcv();
		}
	} 
	/* Clear TDI and AFI bits by writing 1's in the respective position */
	master.status |= mstatus_tdi | mstatus_afi;
	return;
}

static 
void i2c_saa7752_timeout(unsigned long data)
{
	i2c_saa7752_mif_t *iface = (i2c_saa7752_mif_t *)data;
	printk("I2C Master timed out, stat = %04x, cntrl = %04x, "
			"resetting...\n",
			master.status, master.cntrl);
	/* Reset master and disable interrupts */
	master.cntrl |= mcntrl_reset;
	master.cntrl &= ~(mcntrl_afie | mcntrl_naie | mcntrl_drmie);
	while (master.cntrl & mcntrl_reset);
	iface->ret = -EIO;
	complete(&iface->complete);
}

static int 
i2c_saa7752_ioctl(struct i2c_adapter *adap, unsigned int cmd, 
			unsigned long arg)
{
	i2c_saa7752_no_data_t ndarg;
	unsigned long flags;
	char *ptr;

	switch (cmd) {
	case I2C_START:
		return i2c_saa7752_start((unsigned char)(arg>>1), arg&1);
	case I2C_STOP:
		master_if.stop = 1;
	case I2C_NO_DATA:
		save_and_cli(flags);
		if (slave_if.mode == nodata) {
			restore_flags(flags);
			return -EAGAIN;
		}
		if (copy_from_user(&ndarg, (void *)arg, sizeof(ndarg)))
			return -EFAULT;
		if ((ptr = kmalloc(ndarg.len, GFP_KERNEL)) == NULL)
			return -ENOMEM;
		if (copy_from_user(ptr, ndarg.buf, ndarg.len)) {
			kfree(ptr);
			return -ENOMEM;
		}
		save_and_cli(flags);
		slave_if.nodata.len = ndarg.len;
		slave_if.nodata.buf = ptr;
		slave_if.ndi = 0;
		restore_flags(flags);
		return 0;
	}
	return -EINVAL;
}

/* 
 * Generic transfer entrypoint 
 */
static int
i2c_saa7752_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct i2c_msg *pmsg;
	int rc = 0, completed = 0, i;

	DBG ("%s: Entering\n", __FUNCTION__);
	down(&master_if.sem);

	for (i = 0; rc >= 0 && i < num;) {
		u8 addr;

		pmsg = &msgs[i++];
		addr = pmsg->addr;	

		if (pmsg->flags & I2C_M_TEN) {
			printk(KERN_ERR "%s: 10 bits addr not "
					"supported !\n", ADAPTER_NAME);
			rc = -EINVAL;
			break;
		}
    
		/* Set up address and r/w bit */
		if (pmsg->flags & I2C_M_REV_DIR_ADDR)
			addr ^= 1;		

		master_if.buf = pmsg->buf;
		master_if.len = pmsg->len;
		master_if.mode = (pmsg->flags & I2C_M_RD) ? rcv : xmit;
		master_if.ret = 0;

		DBG("%s: %s mode, %d bytes\n", __FUNCTION__, 
				i2c_saa7752_modestr[master_if.mode], 
				master_if.len);

		/* Arm timer */
		i2c_saa7752_arm_timer(&master_if.timer, &master_if);

		/* Enable master interrupt */
		master.cntrl = mcntrl_afie | mcntrl_naie | mcntrl_drmie;

		/* Wait for completion */
		wait_for_completion(&master_if.complete);
		if (!(rc = master_if.ret))
			completed++;
		DBG("%s: Return code = %d\n", __FUNCTION__, rc);
	}

	master_if.buf = NULL;
	master_if.len = 0;
	master_if.stop = 0;
	/* Release sem */
	up(&master_if.sem);

	DBG ("%s: Exit\n", __FUNCTION__);
	return completed;
}

static u32
i2c_saa7752_func(struct i2c_adapter * adapter)
{
	return I2C_FUNC_I2C;
}

static void 
i2c_saa7752_inc_use(struct i2c_adapter *a)
{
#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif
}

static void 
i2c_saa7752_dec_use(struct i2c_adapter *a)
{
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
}

static ssize_t
i2c_saa7752_slave_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	int ret = 0;

	/* Can't seek (pread) on this device */
	if (ptr != &file->f_pos)
		return -ESPIPE;

	SDBG("%s(%d): %s Entered\n", __FILE__, __LINE__, __FUNCTION__);
	/* This is not re-entrant */
	down(&slave_if.sem);
	disable_irq(INT_IIC_SLAVE);

	SDBG("%s: count = %d, buf_len = %d\n", __FUNCTION__, count,
				i2c_saa7752_slave_buf_len(&slave_if.rxd));
	/* Wait till the read completes if buffer does not contain the 
	 * requested number of bytes.
	 */
	if (i2c_saa7752_slave_buf_len(&slave_if.rxd) < count) {
		slave_if.rxlen = count;
		enable_irq(INT_IIC_SLAVE);
		SDBG("%s: Waiting for completion.\n", __FUNCTION__);
		wait_for_completion(&slave_if.rx_complete);
		disable_irq(INT_IIC_SLAVE);
		ret = slave_if.ret;
	}

	if (!ret) {
		enable_irq(INT_IIC_SLAVE);
		if (slave_if.rxd.head + count < SLAVE_BUFFER_SIZE) {
			if (copy_to_user(buf, 
					&slave_if.rxd.buf[slave_if.rxd.head], 
					count))
				slave_if.ret = -EFAULT;
		} else {
			int tlen, hlen;

			tlen = SLAVE_BUFFER_SIZE - slave_if.rxd.head;
			hlen = count - tlen;

			if (copy_to_user(buf, 
					&slave_if.rxd.buf[slave_if.rxd.head], 
					tlen))
				slave_if.ret = -EFAULT;
			if (copy_to_user(buf+tlen, &slave_if.rxd.buf[0], hlen))
				slave_if.ret = -EFAULT;
		}
		slave_if.rxd.head = (slave_if.rxd.head + count) % 
							SLAVE_BUFFER_SIZE;
		disable_irq(INT_IIC_SLAVE);
	}
	enable_irq(INT_IIC_SLAVE);

	/* Release semaphore */
	up(&slave_if.sem);

	SDBG("%s(%d): %s Exit\n", __FILE__, __LINE__, __FUNCTION__);
	return ret == 0 ? count : ret;
}

static ssize_t
i2c_saa7752_slave_write(struct file *file, const char *buf, size_t count, 
								loff_t *ptr)
{
	int ret = 0;

	/* Can't seek (pread) on this device */
	if (ptr != &file->f_pos)
		return -ESPIPE;

	SDBG("%s(%d): %s Entered\n", __FILE__, __LINE__, __FUNCTION__);
	/* This is not re-entrant */
	down(&slave_if.sem);

	SDBG("%s(%d): %s: count = %d, sbuf_len = %d\n", __FILE__, 
				__LINE__, __FUNCTION__, count,
				i2c_saa7752_slave_buf_len(&slave_if.rxd));

	disable_irq(INT_IIC_SLAVE);
	/* Check if buffer has enough space */
	if (SLAVE_BUFFER_SIZE - i2c_saa7752_slave_buf_len(&slave_if.txd) < 
								count) {
		SDBG("%s: Waiting for completetion, stat = %#x\n", 
				__FUNCTION__, slave.status);
		/* Wait till the buffer empties completes */
		slave_if.txlen = count;
		slave_if.ret = 0;
		enable_irq(INT_IIC_SLAVE);
		wait_for_completion(&slave_if.tx_complete);
		disable_irq(INT_IIC_SLAVE);
		ret = slave_if.ret;
	}
	enable_irq(INT_IIC_SLAVE);

	/* Copy user buffer */
	if (slave_if.txd.tail + count < SLAVE_BUFFER_SIZE) {
		if (copy_from_user(&slave_if.txd.buf[slave_if.txd.tail], buf, 
								count)) {
			slave_if.ret = -EFAULT;
			goto wdone;
		}
	} else {
		int tlen, hlen;

		tlen = SLAVE_BUFFER_SIZE - slave_if.txd.tail;
		hlen = count - tlen;

		if (copy_from_user(&slave_if.txd.buf[slave_if.txd.tail], buf, 
								tlen)) {
			slave_if.ret = -EFAULT;
			goto wdone;
		}
		if (copy_from_user(&slave_if.txd.buf[0], buf+tlen, hlen)) {
			slave_if.ret = -EFAULT;
			goto wdone;
		}
	}
	slave_if.txd.tail = (slave_if.txd.tail + count) % SLAVE_BUFFER_SIZE;
wdone:
	/* Release semaphore */
	up(&slave_if.sem);

	SDBG("%s(%d): %s Exit\n", __FILE__, __LINE__, __FUNCTION__);
	return ret == 0 ? count : ret;
}

/* For now, we only handle combined mode (smbus) */
static struct i2c_algorithm saa7752_algorithm = {
	name:		"SAA7752 i2c",
	id:		I2C_ALGO_SMBUS,
	master_xfer:	i2c_saa7752_xfer,
	functionality:	i2c_saa7752_func,
	algo_control:	i2c_saa7752_ioctl,
};

static struct i2c_adapter saa7752_adapter = {
	name:		ADAPTER_NAME,
	id:		I2C_ALGO_SMBUS,
	algo:		&saa7752_algorithm,
	inc_use:	i2c_saa7752_inc_use,
	dec_use:	i2c_saa7752_dec_use,
};

static struct file_operations i2c_saa7752_fops = {
	owner:		THIS_MODULE,
	read:		i2c_saa7752_slave_read,
	write:		i2c_saa7752_slave_write,
};

static struct miscdevice i2c_saa7752_miscdev = {
	I2C_SAA7752_MINOR,
	"i2c_saa7752_slave",
	&i2c_saa7752_fops
};

static int 
i2c_saa7752_scan(void)
{
	int i;
	char data;

	/* Scan bus to see who is there */
	printk(KERN_INFO "%s: Scanning bus:   ", saa7752_adapter.name);

	/* Lock interface */
	down(&master_if.sem);

	for (i = 0x00; i < 0xff; i+=2) {

		/* Do not scan our own slave address */
		if ((i >> 1) == SLAVE_ADDR)
			continue;

		/* Wait till bus is free */
		if (i2c_saa7752_bus_busy()) {
			printk("\n%s: Scan timed out", ADAPTER_NAME);
			break;
		}

		master_if.buf = &data;
		master_if.len = sizeof(data);

		/* Arm timer */
		i2c_saa7752_arm_timer(&master_if.timer, &master_if);

		if (i2c_saa7752_start(i >> 1, rcv) == 0)
			wait_for_completion(&master_if.complete);
		del_timer(&master_if.timer);
		i2c_saa7752_stop();

		if (master_if.buf != &data) {
			printk("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
				"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%s found device "
				"id: %02x                    \n", 
					ADAPTER_NAME, i >> 1);
			printk("%s: Scanning bus:   ", ADAPTER_NAME);

		} else {
			printk("\b\b%02x", i >> 1);
		}
	}

	printk("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
			"\b\b\b\b\b\b\b\b\b\b\b\b\b\n");

	up(&master_if.sem);
	return 0;
}

static int __init i2c_saa7752_init(void)
{
	unsigned long tmp;
	int ret;

	/* Internal Data structures for Master */
	memset(&master_if, 0, sizeof(master_if));
	init_MUTEX(&master_if.sem);
	init_completion(&master_if.complete);
	init_timer(&master_if.timer);
	master_if.timer.function = i2c_saa7752_timeout;
	master_if.timer.data = 0;

	/* Internal Data structures for Slave */
	memset(&slave_if, 0, sizeof(slave_if));
	init_MUTEX(&slave_if.sem);
	init_completion(&slave_if.tx_complete);
	init_completion(&slave_if.rx_complete);
	slave_if.txd.buf = kmalloc(SLAVE_BUFFER_SIZE, GFP_KERNEL);
	if (slave_if.txd.buf == NULL)
		return -ENOMEM;
	slave_if.rxd.buf = kmalloc(SLAVE_BUFFER_SIZE, GFP_KERNEL);
	if (slave_if.rxd.buf == NULL) {
		kfree(slave_if.txd.buf);
		return -ENOMEM;
	}
	slave_if.nodata.buf = def_nodata;
	slave_if.nodata.len = sizeof(def_nodata);

	/* Register I/O resource */
  	if (check_region(SSA_IICMASTER_BASE, I2C_BLOCK_SIZE) < 0 ) {
		return -ENODEV;
  	} else {
		request_region(SSA_IICMASTER_BASE, I2C_BLOCK_SIZE, 
							ADAPTER_NAME);
  	}

	/* Enable I2C Master SDA and SCL Pins. These are shared with
	 * the UART in the CDB.
	 */
	*(u32 *)(IO_ADDRESS_CDB_BASE + CDB_ENGINE_CMD_IF) = 1;
	
	/* Clock Divisor High This value is the number of system clocks 
	 * the serial clock (SCL) will be high. n is defined at compile 
	 * time based on the system clock (PCLK) and minimum serial frequency. 
	 * For example, if the system clock period is 50 ns and the maximum 
	 * desired serial period is 10000 ns (100 kHz), then CLKHI would be 
	 * set to 0.5*(f_sys/f_i2c)-2=0.5*(20e6/100e3)-2=98. The actual value 
	 * programmed into CLKHI will vary from this slightly due to 
	 * variations in the output pad s rise and fall times as well as 
	 * the deglitching filter length. In this example, n = 7, since 
	 * eight bits are needed to hold the clock divider count.
	 */
	tmp = ((HCLK_MHZ * 1000) / I2C_SAA7752_SPEED_KHZ)/2 - 2;
	master.clockhi = tmp;
	master.clocklo = tmp;
	slave.clock = tmp;

	/* We use the default slave address for the slave interface which
	 * is 0x34/0x36 for writing and 0x35/0x37 for reading.
	 *
	 * Therefore nothing is written to the ADDRESS register for the
	 * slave interface.
	 *
	 */
	if (slave_addr != 0xff)
		slave.address = slave_addr & 0x7f;
	
	/* Master interrupts are only enabled when we have data to transfer */
	master.cntrl = 0;

	/* Enable slave interrupts */
	slave.cntrl = scntrl_drsie | scntrl_daie;

	/* Reset the IIC Master and Slave. The reset bit is self clearing. */
	master.cntrl |= mcntrl_reset;
	slave.cntrl |= scntrl_reset;

	while ((master.cntrl & mcntrl_reset) || (slave.cntrl & scntrl_reset));

	DBG("%s: cntrl = %08x, mstatus = %08x\n", ADAPTER_NAME, 
			master.cntrl, master.status);
	request_irq(INT_IIC_MASTER, i2c_saa7752_master_interrupt, 0, 
			CHIP_NAME" Master", NULL);
	request_irq(INT_IIC_SLAVE, i2c_saa7752_slave_interrupt, 0, 
			CHIP_NAME" Slave", NULL);

	/* Register this adapter with the I2C subsystem */
	saa7752_adapter.data = NULL;
	i2c_add_adapter(&saa7752_adapter);

	/* Register slave driver */
	if ((ret = misc_register(&i2c_saa7752_miscdev)) != 0) {
		printk(KERN_ERR "%s: Error registering slave device\n",
				ADAPTER_NAME);
		kfree(slave_if.txd.buf);
		kfree(slave_if.rxd.buf);
		i2c_del_adapter(&saa7752_adapter);
		free_irq(INT_IIC_MASTER, 0);
		free_irq(INT_IIC_SLAVE, 0);
		release_region(SSA_IICMASTER_BASE, I2C_BLOCK_SIZE);
		return ret;
	}

	printk(KERN_INFO "%s: %s at %#8x, irq %d\n",
		ADAPTER_NAME, "Master", SSA_IICMASTER_BASE, INT_IIC_MASTER);
	printk(KERN_INFO "%s: %s at %#8x, irq %d, Address %#x\n",
		ADAPTER_NAME, "Slave", SSA_IICSLAVE_BASE, INT_IIC_SLAVE,
		slave.address);
#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif
	if (scan == 1)
		i2c_saa7752_scan();

	return ret;
}

static void __exit i2c_saa7752_cleanup(void)
{
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
	kfree(slave_if.txd.buf);
	kfree(slave_if.rxd.buf);
	i2c_del_adapter(&saa7752_adapter);
	misc_deregister(&i2c_saa7752_miscdev);
	free_irq(INT_IIC_MASTER, 0);
	free_irq(INT_IIC_SLAVE, 0);
	release_region(SSA_IICMASTER_BASE, I2C_BLOCK_SIZE);
}

/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */
MODULE_AUTHOR("Ranjit Deshpande <ranjit@kenati.com>");
MODULE_DESCRIPTION("I2C driver for SAA7752 on-chip I2C unit");
MODULE_LICENSE("GPL");

MODULE_PARM(scan, "i");
MODULE_PARM(slave, "i");
module_init(i2c_saa7752_init);
module_exit(i2c_saa7752_cleanup);

#ifndef MODULE
static int __init i2c_saa7752_parse_cmdline(char *options)
{
	if (!options || !*options)
		return 0;

	scan = simple_strtoul(options, NULL, 10);
	return 1;
}

__setup("i2c_scan=", i2c_saa7752_parse_cmdline);
#endif

