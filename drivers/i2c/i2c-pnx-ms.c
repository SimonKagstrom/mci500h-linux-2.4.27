/*
 * drivers/i2c/i2c-pnx.c
 *
 * Provides i2c support for PNX0106.
 *
 * Author: Srinivasa Mylarappa <srinivasa.mylarapapv@philips.com>
 *
 * 2006 (c) Philips Semicondutors, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/interrupt.h>
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
#include "i2c-pnx-ms.h"

#if defined (CONFIG_I2C_PNX0106)
#error "CONFIG_I2C_PNX0106 and CONFIG_I2C_PNX0106_MS can not be enabled together"
#endif

#define CHIP_NAME	"PNX0106 I2C"
#define ADAPTER0_NAME	"PNX0106 I2C-0"
#define ADAPTER1_NAME	"PNX0106 I2C-1"


#ifndef CONFIG_I2C_PNX_SLAVE_SUPPORT
	#define CONFIG_I2C_PNX_SLAVE_SUPPORT
#endif
/* Define this for debug messages */
#if I2C_PNX_DEBUG
char *i2c_pnx_modestr[] = {
	"Transmit",
	"Receive",
	"No Data",
};
#endif

#ifdef I2C_DRV_DBG
#define SL_printk(fmt, args...) printfk("GM_DBG:" fmt, ##args)
#else
#define SL_printk(fmt, args...)
#endif



#if defined (CONFIG_I2C_PNX_SLAVE_SUPPORT)

static int slave_addr0 = 0x55;	/* slave Address */
static int slave_mode0 = 0x1;	/* mode of operation. default is master mode */
static int slave_addr1 = 0x1d;	/* slave Address */
static int slave_mode1 = 0x0;	/* mode of operation. default is master mode */

int i2c_slave_recv_data (char *buf, int len, void *data)
{
	int count = 0;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) data;

	if ((!buf) || (!len)) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	while (alg_data->slave->rfl && len) {
		buf [count++] = alg_data->slave->fifo.rx;
		len--;
	}

	alg_data->slave->ctl |= scntrl_drsie | scntrl_daie;

	return count;
}

int i2c_slave_send_data (char *buf, int len, void *data)
{
	int i;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) data;

	if (!buf) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	if (len > I2C_BUFFER_SIZE) {
		pr_debug ("%s(): provided buffer too large!\n", __FUNCTION__);
		return -2;
	}

	if (alg_data->buf_index < 0)
	{
		alg_data->slave->ctl |= scntrl_reset;
		while (alg_data->slave->ctl & scntrl_reset);
	}
	else if (alg_data->buf_index > 0) {
		pr_debug ("%s(): another operation is in progress: index %d!\n", __FUNCTION__, alg_data->buf_index);
		return -3;
	}

	/* Reverse copy buffer */
	for (i = 0; i < len; i++)
		alg_data->buffer[i] = buf[len-i-1];

	alg_data->buf_index = len - 1;

	while (!(alg_data->slave->sts & sstatus_tffs) && (alg_data->buf_index >= 0))
		/* push data until FIFO fills up */
		alg_data->slave->txs = 0x000000FF & alg_data->buffer[alg_data->buf_index--];

	/* Enable 'Slave TX FIFO not full' interrupt if TX FIFO is full */
	if (alg_data->slave->sts & sstatus_tffs) {
		alg_data->slave->ctl |= scntrl_tffsie;
		len = 0;
	}

	alg_data->slave->ctl |= scntrl_drsie | scntrl_daie;

	pr_debug ("%s(): S ->> M\n", __FUNCTION__);

	return len;
}
#endif

static void i2c_pnx_arm_timer (struct timer_list *timer, void *data)
{
	del_timer_sync(timer);

	pr_debug("%s: Timer armed at %lu plus %u jiffies.\n",
		__FUNCTION__, jiffies, TIMEOUT );

	timer->expires = jiffies + TIMEOUT;
	timer->data = (unsigned long) data;

	add_timer(timer);
}

static int i2c_pnx_bus_busy (i2c_regs *master)
{
	long timeout;

	timeout = TIMEOUT * 10000;
	if (timeout > 0 && (master->sts & mstatus_active)) {
		udelay(500);
		timeout -= 500;
	}

	return (timeout <= 0) ? 1 : 0;
}

/******************************************************
 * function: i2c_pnx_start
 *
 * description: Generate a START signal in the desired mode.
 * I2C is the master.
 * Return 0 if no error.
 ****************************************************/
static int i2c_pnx_start (unsigned char slave_addr, struct i2c_adapter *adap)
{
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

#if I2C_PNX010X_DEBUG
	pr_debug("%s(%d): %s() addr 0x%x mode %s\n", __FILE__,
		__LINE__, __FUNCTION__, slave_addr,
		i2c_pnx_modestr[alg_data->mif.mode]);
#endif

	/* Check for 7 bit slave addresses only */
	if (slave_addr & ~0x7f) {
		printk(KERN_ERR "%s: Invalid slave address %x. "
			"Only 7-bit addresses are supported\n",
			adap->name, slave_addr);
		return -EINVAL;
	}

	/* First, make sure bus is idle */
	if (i2c_pnx_bus_busy(alg_data->master)) {
		/* Somebody else is monopolising the bus */
		printk(KERN_ERR "%s: Bus busy. Slave addr = %02x, "
			"cntrl = %x, stat = %x\n",
			adap->name, slave_addr,
			alg_data->master->ctl,
			alg_data->master->sts);
		return -EBUSY;
	} else if (alg_data->master->sts & mstatus_afi) {
		/* Sorry, we lost the bus */
		printk(KERN_ERR "%s: Arbitration failure. "
			"Slave addr = %02x\n",
			adap->name, slave_addr);
			return -EIO;
	}

	/* OK, I2C is enabled and we have the bus. */
	/* Clear the current TDI and AFI status flags. */
	alg_data->master->sts |= mstatus_tdi | mstatus_afi;

	pr_debug ("%s(%d): %s() sending %#x\n", __FILE__, __LINE__,
		__FUNCTION__, (slave_addr << 1) | start_bit |
		(alg_data->mif.mode == rcv ? rw_bit : 0));

	/* Write the slave address, START bit and R/W bit */
	alg_data->master->fifo.tx = (slave_addr << 1) | start_bit |
		(alg_data->mif.mode == rcv ? rw_bit: 0);

	pr_debug ("%s(%d): %s() exit\n", __FILE__, __LINE__, __FUNCTION__);

	return 0;
}

/***********************************************************
 * function: i2c_pnx_stop
 *
 * description: Generate a STOP signal to terminate the master
 * transaction.
 * Always returns 0.
 **********************************************************/
static int i2c_pnx_stop (i2c_pnx_algo_data_t *alg_data)
{
	pr_debug ("%s: %s() enter - stat = %04x.\n",
		__FILE__, __FUNCTION__, alg_data->master->sts);

	/* Write a STOP bit to TX FIFO */
	alg_data->master->fifo.tx = 0x000000ff | stop_bit;

	/* Wait until the STOP is seen. */
	while (alg_data->master->sts & mstatus_active);

	pr_debug ("%s: %s() exit - stat = %04x.\n",
		__FILE__, __FUNCTION__, alg_data->master->sts);

	return 0;
}

/****************************************************
 * function: i2c_pnx_master_xmit
 *
 * description: Master sends one byte of data to
 * slave target
 * return 0 if the byte transmitted.
 ***************************************************/
static int i2c_pnx_master_xmit (i2c_pnx_algo_data_t *alg_data)
{
	u32 val;

	pr_debug ("%s(): Entered\n", __FUNCTION__);

	if (alg_data->mif.len > 0) {
		/* We still have somthing to talk about... */
		val = *alg_data->mif.buf++;
		val &= 0x000000ff;

		if (alg_data->mif.len == 1) {
			/* Last byte, issue a stop */
			val |= stop_bit;
		}

		alg_data->mif.len--;
		alg_data->master->fifo.tx = val;

		pr_debug ("%s: xmit %#x [%d]\n", __FUNCTION__, val, alg_data->mif.len + 1);

		if (alg_data->mif.len == 0) {
			/* Wait until the STOP is seen. */
			while (alg_data->master->sts & mstatus_active);

			/* Disable master interrupts */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie);

			del_timer_sync(&alg_data->mif.timer);

			pr_debug("%s: Waking up xfer routine.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}
	} else /* For zero-sized transfers. */
		if (alg_data->mif.len == 0) {
			/* Isue a stop. */
			i2c_pnx_stop(alg_data);

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie);

			/* Stop timer. */
			del_timer_sync(&alg_data->mif.timer);
			pr_debug("%s: Waking up xfer routine after zero-xfer.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}

	pr_debug ("%s(): Exit\n", __FUNCTION__);

	return 0;
}

/***********************************************
 * function: i2c_pnx_master_rcv
 *
 * description: master reads one byte data
 * from slave source
 * return 0 if no error
 ***********************************************/
static int i2c_pnx_master_rcv (i2c_pnx_algo_data_t *alg_data)
{
	unsigned int val = 0;

	pr_debug ("%s(): Entered\n", __FUNCTION__);

	/* Check, whether there is already data,
	 * or we didn't 'ask' for it yet.
	 */
	if (alg_data->master->sts & mstatus_rfe) {
		pr_debug ("%s(): Write dummy data to fill Rx-fifo...\n", __FUNCTION__);

		if (alg_data->mif.len == 1) {
			/* Last byte, do not acknowledge next rcv. */
			val |= stop_bit;

			/* Enable interrupt RFDAIE (= data in Rx-fifo.),
			 * and disable DRMIE (= need data for Tx.)
			 */
			alg_data->master->ctl |= mcntrl_rffie | mcntrl_daie;
			alg_data->master->ctl &= ~(mcntrl_drmie);
		}

		/* Now we'll 'ask' for data:
		 * For each byte we want to receive, we must
		 * write a (dummy) byte to the Tx-FIFO.
		 */
		alg_data->master->fifo.tx = val;

		return 0;
	}

	/* Handle data. */
	if (alg_data->mif.len > 0) {

		pr_debug ("%s(): Get data from Rx-fifo...\n", __FUNCTION__);

		val = alg_data->master->fifo.rx;
		*alg_data->mif.buf++ = (u8)(val & 0xff);
		pr_debug ("%s: rcv 0x%x [%d]\n", __FUNCTION__, val, alg_data->mif.len);

		alg_data->mif.len--;
		if (alg_data->mif.len == 0) {
			/* Wait until the STOP is seen. */
			while (alg_data->master->sts & mstatus_active);

			/* Disable master interrupts */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_rffie | mcntrl_drmie | mcntrl_daie);

			/* Kill timer. */
			del_timer_sync(&alg_data->mif.timer);
			pr_debug("%s: Waking up xfer routine after rcv.\n", __FUNCTION__);

			complete(&alg_data->mif.complete);
		}
	}

	pr_debug ("%s: Exit\n", __FUNCTION__);

	return 0;
}

static irqreturn_t i2c_pnx_interrupt (int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long stat;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *)dev_id;
//	alg_data->slave->ctl &= ~scntrl_drsie;
/*	SL_printk(KERN_INFO "Call the i2c_pnx_interrupt function \n");*/
	if (alg_data->mode == slave) {

/*		SL_printk(KERN_INFO " Enter i2c_pnx_interrupt function slave case \n");		*/

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
		/* process slave-related events */
		SL_printk ("sstat = %x sctrl = %x\n",alg_data->slave->sts, alg_data->slave->ctl);
		stat = alg_data->slave->sts;

		/* Now let's see what kind of event this is */
		if (stat & sstatus_afi) {
			/* We lost arbitration in the midst of a transfer */
			SL_printk (" Arbitration failed!\n");
			alg_data->slave->sts |= sstatus_afi; /* clear AFI flag */
		}

		if (stat & sstatus_nai) {
			/* Master did not acknowledge our transfer */
			SL_printk ("%s(): Master did not acknowledge!\n", __FUNCTION__);
		}

		if ((!(stat & sstatus_stffs)) && (!(stat & sstatus_stfes)))
		{

			if (alg_data->buf_index>=0)
			{
				alg_data->slave->txs = 0x000000FF & alg_data->buffer [alg_data->buf_index--];
				SL_printk("require moe data to transfer!index=%d\n",alg_data->buf_index);
			}
			else
			{
				/* no more need in this interrupt */
				alg_data->slave->ctl &= ~scntrl_tffsie;
				SL_printk("no data to transfer!\n");
			}
		}
		if (stat & sstatus_stfes)
			alg_data->slave->ctl &= ~(scntrl_drsie|scntrl_tffsie);
		if (stat & sstatus_rff)
			SL_printk("rcv fifo full \n");
		if (!(stat & sstatus_rfe)) {
			/* RX FIFO contains data to process */
#if 0
			if ( ((rcv_cbuf.writep+1)&RCV_BUF_LEN_MD)!=rcv_cbuf.readp )
			{
				rcv_cbuf.rcv_buf[rcv_cbuf.writep] = alg_data->slave->fifo.rx;
				SL_printk("rcv a data= %x \n",rcv_cbuf.rcv_buf[rcv_cbuf.writep]);
				rcv_cbuf.writep = (rcv_cbuf.writep+1)&RCV_BUF_LEN_MD;
			}
			else
			{
				SL_printk("rcv buf full \n");
			}
#endif
			SL_printk("rcv data \n");
		} else if (stat & sstatus_drsi) {
			/* I2C master requests data */
			/* request data from higher level code and send it */

			if (alg_data->buf_index>=0)
			{

				alg_data->slave->txs = 0x000000FF & alg_data->buffer [alg_data->buf_index--];
				SL_printk("need data to send when fifio is empty!index=%d\n",alg_data->buf_index);

			}
			else
			{
				alg_data->slave->ctl &= ~scntrl_drsie;
				SL_printk("No data to send in buf!\n");

			}
		}
#endif
	} else {

/*		SL_printk(KERN_INFO " Enter i2c_pnx_interrupt function master case \n");	*/
		/* process master-related events */
#if I2C_PNX010X_DEBUG
		pr_debug ("%s(): mstat = %x mctrl = %x, mmode = %s\n", __FUNCTION__,
			alg_data->master->sts, alg_data->master->ctl,
			i2c_pnx_modestr[alg_data->mif.mode]);
#endif
		stat = alg_data->master->sts;

		/* Now let's see what kind of event this is */
		if (stat & mstatus_afi) {
			/* We lost arbitration in the midst of a transfer */
			alg_data->mif.ret = -EIO;

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie | mcntrl_rffie);

			/* Stop timer, to prevent timeout. */
			del_timer_sync(&alg_data->mif.timer);
			complete(&alg_data->mif.complete);
		} else if (stat & mstatus_nai) {
			/* Slave did not acknowledge, generate a STOP */
			pr_debug ("%s(): Slave did not acknowledge, generating a STOP.\n", __FUNCTION__);
			i2c_pnx_stop(alg_data);

			/* Disable master interrupts. */
			alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie |
				mcntrl_drmie | mcntrl_rffie);

			/* Our return value. */
			alg_data->mif.ret = -EFAULT;

			/* Stop timer, to prevent timeout. */
			del_timer_sync(&alg_data->mif.timer);
			complete(&alg_data->mif.complete);
		} else
			/* Two options:
			 * - Master Tx needs data.
			 * - There is data in the Rx-fifo
			 * The latter is only the case if we have requested for data,
			 * via a dummy write. (See 'i2c_pnx_master_rcv'.)
			 * We therefore check, as a sanity check, whether that interrupt
			 * has been enabled.
			 */
			if ((stat & mstatus_drmi) || !(stat & mstatus_rfe)) {
				if (alg_data->mif.mode == xmit) {
					i2c_pnx_master_xmit(alg_data);
				} else if (alg_data->mif.mode == rcv) {
					i2c_pnx_master_rcv(alg_data);
				}
			}

			/* Clear TDI and AFI bits by writing 1's in the respective position. */
			alg_data->master->sts |= mstatus_tdi | mstatus_afi;
	}

	pr_debug ("%s(): Exit, stat = %x ctrl = %x.\n",
		__FUNCTION__, alg_data->master->sts, alg_data->master->ctl);

	return IRQ_HANDLED;
}

static int i2c_pnx_timeout (unsigned long data)
{
	i2c_pnx_algo_data_t* alg_data = (i2c_pnx_algo_data_t *)data;

	printk(KERN_ERR
		"Master timed out. stat = %04x, cntrl = %04x. Resetting master ...\n",
		alg_data->master->sts, alg_data->master->ctl);

	/* Reset master and disable interrupts */
	alg_data->master->ctl &= ~(mcntrl_afie | mcntrl_naie | mcntrl_drmie | mcntrl_rffie);
	alg_data->master->ctl |= mcntrl_reset;
	while (alg_data->master->ctl & mcntrl_reset);

	alg_data->mif.ret = -EIO;

	complete(&alg_data->mif.complete);
	return 0;
}

#if 1

static void i2c_pnx_mode_switch (struct i2c_adapter *adap, int mode)
{
	unsigned long tmp;

	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	SL_printk(KERN_INFO "Call the i2c_mode_switch function mode=%d new mode =%d \n",alg_data->mode, mode);

	if (alg_data->mode == mode)
		return;


	/* Reset the I2C controller. The reset bit is self clearing. */
/*
	if (alg_data->mode == slave) {
		alg_data->slave->ctl |= scntrl_reset;
		while ((alg_data->slave->ctl & scntrl_reset));
	} else {
		alg_data->master->ctl |= mcntrl_reset;
		while ((alg_data->master->ctl & mcntrl_reset));
	}
*/
	alg_data->mode = mode;


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

	if (alg_data->mode == master) {
		tmp = ((HCLK_MHZ * 1000) / I2C_PNX0106_SPEED_KHZ)/2 - 2;
		alg_data->master->ckh = tmp;
		alg_data->master->ckl = tmp;
	}
	else if (alg_data->slave_addr != 0xff) {
		alg_data->slave->adr = alg_data->slave_addr & 0x7f;
		if (alg_data->slave->adr != (alg_data->slave_addr & 0x7f))
			printk(KERN_ERR
		      		"Failed to program the slave to address %u.\n",
		       		 (alg_data->slave_addr & 0x7f));
	}

	if (alg_data->mode == slave)
	{
		memset (&alg_data->buffer, 0, I2C_BUFFER_SIZE);
		alg_data->buf_index = 0;

		alg_data->slave->ctl = 0; //scntrl_drsie | scntrl_daie;
	}
	else
		alg_data->master->ctl = 0;

	/* Reset the I2C Master/Slave. The reset bit is self clearing. */
	if (alg_data->mode == slave) {
		alg_data->slave->ctl |= scntrl_reset;
		while ((alg_data->slave->ctl & scntrl_reset));
	} else {
		alg_data->master->ctl |= mcntrl_reset;
		while ((alg_data->master->ctl & mcntrl_reset));
	}

	return ;
}

static void i2c_pnx_reset (struct i2c_adapter *adap)
{
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	SL_printk(KERN_INFO "Call the i2c_reset function \n");


	/* Reset the I2C controller. The reset bit is self clearing. */

	if (alg_data->mode == slave) {
		alg_data->slave->ctl |= scntrl_reset;
		while ((alg_data->slave->ctl & scntrl_reset));
	} else {
		alg_data->master->ctl |= mcntrl_reset;
		while ((alg_data->master->ctl & mcntrl_reset));
	}
	if (alg_data->mode == slave)
	{
		memset (&alg_data->buffer, 0, I2C_BUFFER_SIZE);
		alg_data->buf_index = 0;

		alg_data->slave->ctl = 0; //scntrl_drsie | scntrl_daie;
	}
	else
		alg_data->master->ctl = 0;
	return ;
}

static int i2c_pnx_slave_send (struct i2c_adapter *adap, char* buf, int len)
{
	int i;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	SL_printk(KERN_INFO "Befor send txFIFO level = %d sts flag=%x index=%d\n",alg_data->slave->stfl,alg_data->slave->sts,alg_data->buf_index);


	if (!buf) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	if (len > I2C_BUFFER_SIZE) {
		pr_debug ("%s(): provided buffer too large!\n", __FUNCTION__);
		return -2;
	}

	if (alg_data->buf_index < 0)
	{
		SL_printk (KERN_INFO "index<0\n");
		alg_data->slave->ctl |= scntrl_reset;
		while (alg_data->slave->ctl & scntrl_reset);
	}
	else if (alg_data->buf_index > 0) {
		SL_printk (KERN_INFO "buf not empty index= %d!\n", alg_data->buf_index);
		return -3;
	}

	/* Reverse copy buffer */
	for (i = 0; i < len; i++)
	{
		alg_data->buffer[i] = buf[len-i-1];
//		SL_printk(KERN_INFO "|0x%x |",alg_data->buffer[i]);
	}
	alg_data->buf_index = len - 1;

	while (!(alg_data->slave->sts & sstatus_stffs) && (alg_data->buf_index >= 0))
		/* push data until FIFO fills up */
		alg_data->slave->txs = 0x000000FF & alg_data->buffer[alg_data->buf_index--];

	/* Enable 'Slave TX FIFO not full' interrupt if TX FIFO is full */
	if (alg_data->slave->sts & sstatus_stffs) {
		alg_data->slave->ctl |= scntrl_tffsie;
//		len = 0;
	}

	alg_data->slave->ctl |= scntrl_drsie;
	SL_printk(KERN_INFO "send end send len=0x%x | txFIFO level =%d sts flag=0x%x index=%d\n",len,alg_data->slave->stfl,alg_data->slave->sts,alg_data->buf_index);

	return len;
}

static int i2c_pnx_slave_recv (struct i2c_adapter *adap, char* buf, int len)
{
	int count = 0;
	i2c_pnx_algo_data_t *alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	SL_printk(KERN_INFO "Call the i2c_pnx_slave_recv function len=%d rfl=0x%x\n",len,alg_data->slave->rfl );

	if ((!buf) || (!len)) {
		pr_debug ("%s(): check buffer!\n", __FUNCTION__);
		return -1;
	}

	while (alg_data->slave->rfl && len) {
		buf [count++] = alg_data->slave->fifo.rx;
		len--;
		SL_printk(KERN_INFO "0x%x | ",buf[count-1]);
	}

	/*alg_data->slave->ctl |= scntrl_daie;

	SL_printk(KERN_INFO "recv data numb is %d \n",count);*/

	return count;
}

#endif

static int i2c_pnx_ioctl (struct i2c_adapter *adap, unsigned int cmd, unsigned long arg)
{
	struct i2c_msg slave_msg;
	struct i2c_msg rdwr_pa;
	int res = 0;


#ifdef DEBUG
	printk (KERN_DEBUG "i2c-pnx.o: %s ioctl, cmd: 0x%x, arg: %lx.\n", adap->name, cmd, arg);
#endif

	switch (cmd) {

	case I2C_SLAVE_MAST_SWITCH :
		i2c_pnx_mode_switch (adap, arg);
		return 0;

	case I2C_RESET :
		i2c_pnx_reset (adap);
		return 0;

	case I2C_SLAVE_RD:
		if (copy_from_user (&rdwr_pa, (struct i2c_msg *) arg, sizeof(struct i2c_msg)))
			return -EFAULT;

		if (rdwr_pa.len > 8192)
			return -EINVAL;

		slave_msg.len = rdwr_pa.len;

		slave_msg.buf = (u8 *) kmalloc (rdwr_pa.len, GFP_KERNEL);
		if (slave_msg.buf == NULL)
			return -EFAULT;

		res = i2c_pnx_slave_recv (adap, slave_msg.buf, slave_msg.len);

		if (copy_to_user (&rdwr_pa.buf, slave_msg.buf, slave_msg.len))
			res = -EFAULT;

		kfree (slave_msg.buf);
		return res;

	case I2C_SLAVE_WR:
		if (copy_from_user (&rdwr_pa, (struct i2c_msg *) arg, sizeof(struct i2c_msg)))
			return -EFAULT;

		if (rdwr_pa.len > 8192)
			return -EINVAL;

		slave_msg.len = rdwr_pa.len;

		slave_msg.buf = (u8 *) kmalloc (rdwr_pa.len, GFP_KERNEL);
		if (slave_msg.buf == NULL)
			return -EFAULT;

		if (copy_from_user (slave_msg.buf, &rdwr_pa.buf, rdwr_pa.len))
			return -EFAULT;

		res = i2c_pnx_slave_send (adap, slave_msg.buf, slave_msg.len);

		kfree (slave_msg.buf);
		return res;

	default:
		printk (KERN_ERR "%s: IOCTL-functionality not implemented!\n", adap->name);
		break;
	}

	return 0;
}

/*
 * Generic transfer entrypoint
 */
static int i2c_pnx_xfer (struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct i2c_msg *pmsg;
	int rc = 0, completed = 0, i;
	i2c_pnx_algo_data_t * alg_data = (i2c_pnx_algo_data_t *) adap->algo_data;

	pr_debug ("%s: Entered\n", __FUNCTION__);

	/* Give an error if we're suspended. */
	//if (adap->dev.parent->power.power_state)
	//	return -EBUSY;

	down(&alg_data->mif.sem);

	/* If the bus is still active, or there is already
	 * data in one of the fifo's, there is something wrong.
	 * Repair this by a reset ...
	 */
	if ((alg_data->master->sts & mstatus_active)) {
		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	} else if (!(alg_data->master->sts & mstatus_rfe) ||
		!(alg_data->master->sts & mstatus_tfe)) {
		printk(KERN_ERR
			"%s: FIFO's contain already data before xfer. Reset it ...\n",
			adap->name);

		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	}

	/* Process transactions in a loop. */
	for (i = 0; rc >= 0 && i < num;) {
		u8 addr;

		pmsg = &msgs[i++];
		addr = pmsg->addr;

		if (pmsg->flags & I2C_M_TEN) {
			printk(KERN_ERR
				"%s: 10 bits addr not supported !\n", adap->name);
			rc = -EINVAL;
			break;
		}

		/* Check address for sanity. */
		if (alg_data->slave && addr == alg_data->slave->adr) {
			printk("%s: An attempt was made to access our own slave address! :-)\n",
				adap->name);
			rc = -EINVAL;
			break;
		}

		/* Set up address and r/w bit */
		if (pmsg->flags & I2C_M_REV_DIR_ADDR) {
			addr ^= 1;
		}

		alg_data->mif.buf = pmsg->buf;
		alg_data->mif.len = pmsg->len;
		alg_data->mif.mode = (pmsg->flags & I2C_M_RD) ? rcv : xmit;
		alg_data->mif.ret = 0;

#if I2C_PNX010X_DEBUG
		pr_debug("%s: %s mode, %d bytes\n", __FUNCTION__,
			i2c_pnx_modestr[alg_data->mif.mode], alg_data->mif.len);
#endif

		/* Arm timer */
		i2c_pnx_arm_timer(&alg_data->mif.timer, (void *) alg_data);

		/* initialize the completion var */
		init_completion(&alg_data->mif.complete);

		/* Enable master interrupt */
		alg_data->master->ctl = mcntrl_afie | mcntrl_naie | mcntrl_drmie;

		/* Put start-code and slave-address on the bus. */
		rc = i2c_pnx_start(addr, adap);
		if (0 != rc) {
			break;
		}

		pr_debug("%s(%d): stat = %04x, cntrl = %04x.\n",
			__FUNCTION__, __LINE__, alg_data->master->sts, alg_data->master->ctl);

		/* Wait for completion */
		wait_for_completion(&alg_data->mif.complete);

		if (!(rc = alg_data->mif.ret))
			completed++;
		pr_debug("%s: Return code = %d.\n", __FUNCTION__, rc);

		/* Clear TDI and AFI bits in case they are set. */
		if (alg_data->master->sts & mstatus_tdi) {
			printk("%s: TDI still set ... clearing now.\n", adap->name);
			alg_data->master->sts |= mstatus_tdi;
		}
		if (alg_data->master->sts & mstatus_afi) {
			printk("%s: AFI still set ... clearing now.\n", adap->name);
			alg_data->master->sts |= mstatus_afi;
		}
	}

	/* If the bus is still active, reset it to prevent
	 * corruption of future transactions.
	 */
	if ((alg_data->master->sts & mstatus_active)) {
		printk("%s: Bus is still active after xfer. Reset it ...\n",
			adap->name);
		alg_data->master->ctl |= mcntrl_reset;
		while (alg_data->master->ctl & mcntrl_reset);
	} else
		/* If there is data in the fifo's after transfer,
		 * flush fifo's by reset.
		 */
		if (!(alg_data->master->sts & mstatus_rfe) ||
			!(alg_data->master->sts & mstatus_tfe)) {
			alg_data->master->ctl |= mcntrl_reset;
			while (alg_data->master->ctl & mcntrl_reset);
		} else if ((alg_data->master->sts & mstatus_nai)) {
			alg_data->master->ctl |= mcntrl_reset;
			while (alg_data->master->ctl & mcntrl_reset);
		}

	/* Cleanup to be sure ... */
	alg_data->mif.buf = NULL;
	alg_data->mif.len = 0;

	/* Release sem */
	up(&alg_data->mif.sem);
	pr_debug ("%s: Exit\n", __FUNCTION__);

	if (completed != num) {
		return ((rc<0) ? rc : -EREMOTEIO);
	}
	return num ;
}

static u32 i2c_pnx_func(struct i2c_adapter * adapter)
{
	return	I2C_FUNC_I2C |
		I2C_FUNC_SMBUS_WRITE_BYTE_DATA | I2C_FUNC_SMBUS_READ_BYTE_DATA |
		I2C_FUNC_SMBUS_WRITE_WORD_DATA | I2C_FUNC_SMBUS_READ_WORD_DATA |
		I2C_FUNC_SMBUS_QUICK ;
}

/* For now, we only handle combined mode (smbus) */
static struct i2c_algorithm pnx_algorithm = {
	name:		CHIP_NAME,
	id:		I2C_ALGO_SMBUS,
	master_xfer:	i2c_pnx_xfer,
	functionality:	i2c_pnx_func,
	slave_send:	i2c_pnx_slave_send,
	slave_recv:	i2c_pnx_slave_recv,
	algo_control:	i2c_pnx_ioctl,
};

static i2c_pnx_algo_data_t pnx_algo_data0 = {
	.base = IIC0_REGS_BASE,
	.irq = IRQ_IIC_0,
	.mode = master,
	.slave_addr = 0x1d,
	.slave_recv_cb = NULL,
	.slave_send_cb = NULL,
	.master = ((i2c_regs *)IO_ADDRESS_IIC0_BASE),
	.slave = ((i2c_regs *) IO_ADDRESS_IIC0_BASE),
};

static i2c_pnx_algo_data_t pnx_algo_data1 = {
	.base = IIC1_REGS_BASE,
	.irq = IRQ_IIC_1,
	.mode = master,
	.slave_addr = 0x1d,
	.slave_recv_cb = NULL,
	.slave_send_cb = NULL,
	.master = ((i2c_regs *)IO_ADDRESS_IIC1_BASE),
	.slave = ((i2c_regs *)IO_ADDRESS_IIC1_BASE),
};

static struct i2c_adapter pnx_adapter = {
	.name = ADAPTER0_NAME,
	.id = I2C_ALGO_SMBUS,
	.algo = &pnx_algorithm,
	.algo_data = (void *)&pnx_algo_data0,
};

static struct i2c_adapter pnx_adapter1 = {
	.name = ADAPTER1_NAME,
	.id = I2C_ALGO_SMBUS,
	.algo = &pnx_algorithm,
	.algo_data = (void *)&pnx_algo_data1,
};

static struct i2c_adapter *pnx_i2c_adapters[] = {
	&pnx_adapter,
	&pnx_adapter1,
	NULL
};

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
int i2c_slave_set_recv_cb (void (*callback)(void *), unsigned int adap)
{
	i2c_pnx_algo_data_t *alg_data;
	alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;

	alg_data->slave_recv_cb = callback;
	pr_debug ("%s: callback installed\n", __FUNCTION__);

	return 0;
}

int i2c_slave_set_send_cb (void (*callback)(void *), unsigned int adap)
{
	i2c_pnx_algo_data_t *alg_data;
	alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;

	alg_data->slave_send_cb = callback;
	pr_debug ("%s: callback installed\n", __FUNCTION__);

	return 0;
}

int i2c_slave_push_data (char* buf, int len, int adap)
{
	i2c_pnx_algo_data_t *alg_data;

	alg_data =
		(i2c_pnx_algo_data_t *) pnx_i2c_adapters[adap]->algo_data;

	/* flush the fifo */
	alg_data->slave->ctl |= mcntrl_reset;
	while (alg_data->slave->ctl & mcntrl_reset);

	/* send the data through */
	len = i2c_slave_send_data( buf, len, alg_data);

	return len;
}
#endif

int __init i2c_pnx_init (void)
{
	unsigned long tmp, i = 0;
	int ret;
	i2c_pnx_algo_data_t * alg_data;
//	int freq_mhz;

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
	int *m_params_mode[] = {&slave_mode0, &slave_mode1};
	int *m_params_address[] = {&slave_addr0, &slave_addr1};
#endif
	printk ("PNX0106 I2C Master/Slave driver, NXP Semiconductors\n");

	while (pnx_i2c_adapters[i]) {
		alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[i]->algo_data;

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
		if (*m_params_mode[i])
			alg_data->mode = slave;
		else
			alg_data->mode = master;
		alg_data->slave_addr = *m_params_address[i];
#endif

		/* Internal Data structures for Master */
/*		if (alg_data->mode == master)*/
		{
			memset(&alg_data->mif, 0, sizeof(alg_data->mif));
			init_MUTEX(&alg_data->mif.sem);
			init_timer(&alg_data->mif.timer);
			alg_data->mif.timer.function = i2c_pnx_timeout;
			alg_data->mif.timer.data = (unsigned long) alg_data;
		}
/*		else */
		{
			/* Init I2C buffer */
			memset (&alg_data->buffer, 0, I2C_BUFFER_SIZE);
			alg_data->buf_index = 0;
		}

		/* Register I/O resource */
		if (!request_region(alg_data->base, I2C_BLOCK_SIZE, "i2c-pnx")) {
			printk(KERN_ERR
		       		"I/O region 0x%08x for I2C already in use.\n",
		       		alg_data->base);
			return -ENODEV;
		}


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

		if (alg_data->mode == master) {
			tmp = ((HCLK_MHZ * 1000) / I2C_PNX0106_SPEED_KHZ)/2 - 2;
			alg_data->master->ckh = tmp;
			alg_data->master->ckl = tmp;
		}
		else if (alg_data->slave_addr != 0xff) {
			alg_data->slave->adr = alg_data->slave_addr & 0x7f;
			if (alg_data->slave->adr != (alg_data->slave_addr & 0x7f))
				printk(KERN_ERR "%s: "
			      		"Failed to program the slave to address %u.\n",
			       		pnx_i2c_adapters[i]->name, (alg_data->slave_addr & 0x7f));
		}
		/* Master/Slave interrupts init */
		if (alg_data->mode == slave)
			alg_data->slave->ctl = 0; //scntrl_drsie | scntrl_daie;
		else
			alg_data->master->ctl = 0;

		/* Reset the I2C Master/Slave. The reset bit is self clearing. */
		if (alg_data->mode == slave) {
			alg_data->slave->ctl |= scntrl_reset;
			while ((alg_data->slave->ctl & scntrl_reset));
		} else {
			alg_data->master->ctl |= mcntrl_reset;
			while ((alg_data->master->ctl & mcntrl_reset));
		}

/*		if (alg_data->mode != slave) */
		{
			/* initialize the completion var */
			init_completion(&alg_data->mif.complete);
		}

		ret = request_irq(alg_data->irq, i2c_pnx_interrupt, 0,
			  alg_data->mode == slave? CHIP_NAME" Slave":CHIP_NAME" Master",
			  alg_data);
		if (ret) {
			printk(KERN_INFO "IRQ request for I2C failed\n");
			release_region(alg_data->base, I2C_BLOCK_SIZE);
			return ret;
		}

		/* Register this adapter with the I2C subsystem */
		/*if (alg_data->mode == master)*/ {
			ret = i2c_add_adapter(pnx_i2c_adapters[i]);
			if (ret < 0) {
				printk(KERN_INFO "I2C: Failed to add bus\n");
				free_irq(alg_data->irq, alg_data);
				release_region(alg_data->base, I2C_BLOCK_SIZE);
				return ret;
			}
		}

		printk(KERN_INFO "%s: %s at %#8x, irq %d.\n",
	       		pnx_i2c_adapters[i]->name, alg_data->mode == slave?"Slave":"Master",
	       		alg_data->base, alg_data->irq);

		i++;
	}

	return 0;
}

int __exit i2c_pnx_cleanup (void)
{
	int i = 0;
	i2c_pnx_algo_data_t * alg_data;

	while (pnx_i2c_adapters[i]) {
		alg_data = (i2c_pnx_algo_data_t *) pnx_i2c_adapters[i]->algo_data;
		/* Reset the I2C controller. The reset bit is self clearing. */
		if (alg_data->mode == slave) {
			alg_data->slave->ctl |= scntrl_reset;
			while ((alg_data->slave->ctl & scntrl_reset));
		} else {
			alg_data->master->ctl |= mcntrl_reset;
			while ((alg_data->master->ctl & mcntrl_reset));
		}

		free_irq(alg_data->irq, alg_data );
	/*	if (alg_data->mode == master) */ {
			i2c_del_adapter(pnx_i2c_adapters[i]);
		}
		release_region(alg_data->base, I2C_BLOCK_SIZE);
		i++;
	}

	return 0;
}



/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */

MODULE_AUTHOR("Srinivasa Mylarappa <srinivasa.mylarappa@philips.com>");
MODULE_DESCRIPTION("I2C driver for PNX bus");
MODULE_LICENSE("GPL");

MODULE_PARM(slave_addr0, "i");
MODULE_PARM(slave_mode0, "i");
MODULE_PARM(slave_addr1, "i");
MODULE_PARM(slave_mode1, "i");
module_init(i2c_pnx_init);
module_exit(i2c_pnx_cleanup);

#ifdef CONFIG_I2C_PNX_SLAVE_SUPPORT
/*
EXPORT_SYMBOL(i2c_slave_recv_data);
EXPORT_SYMBOL(i2c_slave_send_data);
EXPORT_SYMBOL(i2c_slave_set_send_cb);
EXPORT_SYMBOL(i2c_slave_set_recv_cb);
EXPORT_SYMBOL(i2c_slave_push_data);
*/
#endif
