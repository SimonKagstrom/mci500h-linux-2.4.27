
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
//#include <linux/dma-mapping.h>
//#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

//#define DEBUG
#undef DEBUG

#include <asm/arch/tsc.h>
#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/wma9.h>

//#include <e7b/wma.h>
//#include "epicsAPI.h"
//#include "epics.h"
//#include "wma9epics.h"

void *dma_alloc_coherent(void *p, int size, dma_addr_t *phys, int hmmm);
void dma_free_coherent(void *p, int size, void *ptr, dma_addr_t phys);

#define e7b_wma_fops pnx0106_epics_wma9_fops

static u32					wma_bufsize = 0;
static e7b_wma_buffer		*wma_buff = 0;
static dma_addr_t			wma_buff_bus;
static wait_queue_head_t 	w8Q;
static int input_pos = 0;
static int ninputs = 0;
static volatile int			input_required  = 0; /* epics will be waiting then */
static volatile int			input_available = 0;
static volatile int			unblocked_epics = 0;
static volatile int			unblocked_input = 0;
static volatile int			input_waiting = 0;
static spinlock_t			driver_lock = SPIN_LOCK_UNLOCKED; /* to guard static state variables */

static unsigned long incount = 0; /* DEBUG */

static interval_t timeInit;
static interval_t timeDecode;
static interval_t timeGetPCM;
static interval_t timeReset;
static interval_t timeInputWait;
static interval_t timeInputFetch;
static interval_t timeGetData;
static interval_t timeGetWait;

extern interval_t timeSleep;
extern interval_t timeSleep10;
extern interval_t timeDecoderWait;
extern interval_t timeIsr2Wait;
extern interval_t timeWait2Isr;
extern interval_t timeIsr2Wakeup;

static void unblock(void)
{
	spin_lock(&driver_lock);
	if (input_required) {
		unblocked_epics = 0;
		input_required = 0;
		wake_up_interruptible(&w8Q);  /* wakeup epics */
		spin_unlock(&driver_lock);
		TRACE(3, "unblocking epics\n");
		if (wait_event_interruptible(w8Q, unblocked_epics) < 0) {
			TRACE(1, "interrupted unblocked_epics\n");
			unblocked_epics = 0;
		}
		spin_lock(&driver_lock);
	}
	if (input_waiting) {
		unblocked_input = 0;
		input_waiting = 0;
		wake_up_interruptible(&w8Q);  /* wakeup input */
		spin_unlock(&driver_lock);
		TRACE(3, "unblocking input\n");
		if (wait_event_interruptible(w8Q, unblocked_input) < 0) {
			TRACE(1, "interrupted unblocked_input\n");
			unblocked_input = 0;
		}
	} else {
		spin_unlock(&driver_lock);
	}
}

static int wma_open (struct inode *inode, struct file *fp)
{
	TRACE(5, "%s\n", __FUNCTION__);

	if ((fp->f_mode & (FMODE_WRITE|FMODE_READ)) != (FMODE_WRITE|FMODE_READ)) {
		ERRMSG("wma interface can only be used in RW mode\n");
		return -EINVAL;
	}

	if (pnx0106_epics_decoder_claim(PNX0106_EPICS_WMA9_FWID) < 0)
		return -EBUSY;

	if (pnx0106_epics_select_firmware(PNX0106_EPICS_WMA9_FWID) < 0)
	{
		pnx0106_epics_decoder_release();
		return -ENXIO;
	}

	if (pnx0106_epics_reload_firmware() < 0)
	{
		pnx0106_epics_decoder_release();
		return -ENXIO;
	}

	/*
	 * 	First Allocate buffer for decoder
	 */
	wma_bufsize = PAGE_ALIGN(sizeof(e7b_wma_buffer)); /* size must be multiple of PAGE_SIZE for mmap */

	TRACE(1, "wma_open: alloc %d(%d)\n", wma_bufsize, sizeof(e7b_wma_buffer));

	wma_buff = dma_alloc_coherent(NULL, wma_bufsize, &wma_buff_bus, 0);
	if (!wma_buff)
	{
		pnx0106_epics_decoder_release();
		return -ENOMEM;
	}

	/*
	 * 	Init state vars
	 *
	 */
	init_waitqueue_head(&w8Q);

	input_required  = 0;
	input_available = 0;
	unblocked_epics = 0;
	unblocked_input = 0;
	input_waiting = 0;

	INTERVAL_INIT(timeInit);
	INTERVAL_INIT(timeDecode);
	INTERVAL_INIT(timeDecoderWait);
	INTERVAL_INIT(timeGetPCM);
	INTERVAL_INIT(timeReset);
	INTERVAL_INIT(timeInputWait);
	INTERVAL_INIT(timeInputFetch);
	INTERVAL_INIT(timeGetData);
	INTERVAL_INIT(timeGetWait);
	INTERVAL_INIT(timeSleep);
	INTERVAL_INIT(timeSleep10);
	INTERVAL_INIT(timeWait2Isr);
	INTERVAL_INIT(timeIsr2Wait);
	INTERVAL_INIT(timeIsr2Wakeup);

	/*
	 * 	Start epics decoder
	 */
	wma9decNew();

	return 0;
}

static void
print_interval(char *label, interval_t *pInterval)
{
    INTERVAL_CALC(*pInterval);
    pnx0106_epics_printk("wma9: %-8s total %10llu avg %7llu std. %7lu count %6lu\n    max %7llu min %7llu\n",
	    label, pInterval->total, pInterval->avg, pInterval->std, pInterval->count,
	    pInterval->max, pInterval->min);
}

static void
print_intervals(void)
{
    extern interval_t timeSdma[8];
    extern interval_t timeIrq[8];
    char name[20];
    int i;

    print_interval("init", &timeInit);
    print_interval("reset", &timeReset);
    print_interval("decode", &timeDecode);
    print_interval("decwait", &timeDecoderWait);
    print_interval("inputw", &timeInputWait);
    print_interval("infetch", &timeInputFetch);
    print_interval("getdata", &timeGetData);
    print_interval("getwait", &timeGetWait);
    print_interval("getpcm", &timeGetPCM);
    print_interval("sleep", &timeSleep);
    print_interval("sleep10", &timeSleep10);
    print_interval("wait2isr", &timeWait2Isr);
    print_interval("isr2wait", &timeIsr2Wait);
    print_interval("isr2wake", &timeIsr2Wakeup);

#if 0
    for (i = 0; i < 8; i++)
	if (timeSdma[i].count)
	{
	    sprintf(name, "sdma%d", i);
	    print_interval(name, &timeSdma[i]);
	}

    for (i = 0; i < NR_IRQS; i++)
	if (timeIrq[i].count)
	{
	    sprintf(name, "irq%d", i);
	    print_interval(name, &timeIrq[i]);
	}
#endif
}

static int wma_release (struct inode *inode, struct file *fp)
{
	TRACE(1, "%s\n", __FUNCTION__);
	pnx0106_epics_printk("WMA: wma_release\n");

	unblock();

	wma9decDelete();

	input_required  = 0;
	input_available  = 0;

	dma_free_coherent(NULL, wma_bufsize, wma_buff, wma_buff_bus);
	wma_bufsize = 0;
	wma_buff = 0;
	wma_buff_bus = 0;

	/* will normally have been done in delete() */
	wake_up_interruptible(&w8Q); /* wakeup input callback */

	pnx0106_epics_decoder_release();

	print_intervals();
	pnx0106_epics_printk("WMA: wma_release done\n");
	return 0;
}

static void pop_input(wma9decInputBuf* inbuf)
{
	TRACE(3, "pop %d:%d (%d)\n", input_pos, ninputs, wma_buff->input[input_pos].nused);
	inbuf->data = wma_buff->input[input_pos].data;
	inbuf->size = wma_buff->input[input_pos].nused;
	inbuf->newPacket = ((wma_buff->input[input_pos].flags & E7B_WMA_NEW_PACKET) != 0);
	inbuf->noMoreInput = ((wma_buff->input[input_pos].flags & E7B_WMA_NOMOREINPUT) != 0);
	if (inbuf->noMoreInput) {
		TRACE(1, "No More Input\n");
	}
	input_pos++;
	ninputs--;
}

/* 'callback' function from epics layer (in wma9epics.c) */
void wma9decGetMoreData(wma9decInputBuf* inbuf)
{
	int interrupted = 0;

	TRACE(3, "getMoreData: ninput %d:%d\n", input_pos, ninputs);
	INTERVAL_START(timeGetData);
	spin_lock(&driver_lock);
	if (ninputs > 0) { /* still some data in buffer */
		pop_input(inbuf);
	} else {	/* must get some bytes from userspace */
		input_required = 1;
		input_pos = 0;
		ninputs = 0;

		if (!input_available) {
			/*	wakeup input callback, and put self in waitqueue */
			TRACE(3, "getMoreData: waiting for input %lu\n", incount+1);
			INTERVAL_START(timeGetWait);
			spin_unlock(&driver_lock);
			wake_up_interruptible(&w8Q); /* wakeup input callback */
			if (wait_event_interruptible(w8Q, input_available || !input_required) < 0) {
				TRACE(1, "interrupted getMoreData\n");
				interrupted = 1;
			}
			spin_lock(&driver_lock);
			INTERVAL_STOP(timeGetWait);
		}

		/* input_required can be set to zero in reset */
		if (interrupted || !input_required) { /* terminate nicely ? */
			TRACE(2, "getMoreData: Terminating stream\n");
			if (!input_required) {
				TRACE(1, "unblocked getMoreData\n");
				unblocked_epics = 0;
				wake_up_interruptible(&w8Q); /* wakeup unblocker */
			}
			inbuf->data = wma_buff->input[0].data;
			inbuf->size = 0;
			inbuf->newPacket = 0;
			inbuf->noMoreInput = 1;
		} else if (input_available) {
			TRACE(3, "getMoreData: accepted input %lu\n", incount);
			pop_input(inbuf);
			input_available = 0;
		}

		input_required  = 0;
	}
	spin_unlock(&driver_lock);
	INTERVAL_STOP(timeGetData);
}

static int wma_ioctl (struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
	int retval = 0;

	//TRACE(1, "%s\n", __FUNCTION__);

	switch (command) {
		case E7B_WMA_INIT:
			{
				e7b_wma_init ip;

				TRACE(1, "E7B_WMA_INIT\n");

				if (copy_from_user(&ip, (void*)arg, sizeof(ip)))
					return -EFAULT;

				/* fill references */
				INTERVAL_START(timeInit);
				wma9decInit(&ip.wma_format, &ip.pcm_format, &ip.player_info, &ip.state, &ip.userData);
				INTERVAL_STOP(timeInit);

				/* only copy the state back */
				if (copy_to_user((void *)(arg+offsetof(e7b_wma_init, state)), &ip.state, sizeof(ip.state)))
                    return -EFAULT;

			}
			break;
		case E7B_WMA_DECODE:
			{
				e7b_wma_decode dp;
				int count = 0;

				/* copy the state */
				if (copy_from_user(&dp.state, (void*)(arg+offsetof(e7b_wma_decode, state)), sizeof(dp.state)))
					return -EFAULT;

				TRACE(3, ">>E7B_WMA_DECODE\n");

				while (!input_waiting) {
					if (!count) {
						WARN("decode requested but noone is waiting to provide data; waiting...\n");
					}
					count++;
					/* it is possible that we were scheduled before the INPUT_DATA thread */
					pnx0106_epics_sleep(10);
				}
				if (count) {
					WARN("someone started waiting after %d schedules\n", count);
				}
				INTERVAL_START(timeDecode);
				wma9decDecode(&dp.samplesReady, &dp.state);
				INTERVAL_STOP(timeDecode);
				TRACE(3, "<<E7B_WMA_DECODE sReady=%u state=%d\n", dp.samplesReady, dp.state);

				if (copy_to_user((void *)arg, &dp, sizeof(dp)))
					return -EFAULT;
			}
			break;
		case E7B_WMA_GETPCM:
			{
				e7b_wma_getpcm gp;

				if (copy_from_user(&gp, (void*)arg, sizeof(gp)))
					return -EFAULT;

				TRACE(4, ">>E7B_WMA_GETPCM %u\n", gp.samplesRequested);

				INTERVAL_START(timeGetPCM);
				wma9decGetPCM(
                       gp.samplesRequested, // up to *pcSamplesReady (from wma9decDecode())
                       &gp.samplesReturned, // [out] redundant with *pcbDstUsed below
                       (u8*)wma_buff->output,
                       gp.dstLength, 		// ~ redundant with cSamplesRequested
                       &gp.state   		// [out]
						);
				INTERVAL_STOP(timeGetPCM);
				TRACE(4, "<<E7B_WMA_GETPCM sRet=%u state=%d\n", gp.samplesReturned, gp.state);

				if (copy_to_user((void *)arg, &gp, sizeof(gp)))
					return -EFAULT;
			}
			break;
		case E7B_WMA_RESET:
			TRACE(1, "E7B_WMA_RESET\n");
			unblock();
			INTERVAL_START(timeReset);
			wma9decReset();
			INTERVAL_STOP(timeReset);
			break;
		case E7B_WMA_INPUT_DATA:
			{
				int in_param;
				if (copy_from_user(&in_param, (void*)arg, sizeof(in_param)))
					return -EFAULT;


				TRACE(3, "E7B_WMA_INPUT_DATA %d\n", in_param);

				spin_lock(&driver_lock);
				if (in_param > 0) {
					incount++; /* DEBUG */
					if (input_available) {
						WARN("INPUT_DATA %lu while previous data was not yet consumed (inparam=%d)\n", incount, in_param);
					}
					input_available = 1;
					ninputs = in_param;
					input_pos = 0;
					if (input_required) {
						TRACE(3, "input %lu avail\n", incount);
						wake_up_interruptible(&w8Q);
					} else {
						WARN("Input %lu available but none required\n", incount);
					}
				} else {
					input_available = 0;
				}

				/* wait for decoder (or reset or release) */
				input_waiting = 1;
				spin_unlock(&driver_lock);

				if (timeInputFetch.count == 0)
				    timeInputFetch.starttsc = ReadTSC64();

				INTERVAL_STOP(timeInputFetch);
				INTERVAL_START(timeInputWait);

				TRACE(4, "E7B_WMA_INPUT_DATA waiting for decoder\n");
				if (wait_event_interruptible(w8Q, (input_required && !input_available)|| !input_waiting) < 0) {
					TRACE(1,"INPUT_DATA interrupted\n");
					input_waiting = 0;
					INTERVAL_STOP(timeInputWait);
					INTERVAL_START(timeInputFetch);
					return -EPIPE;	/* only return 0 if data is required */
				}
				if (!input_waiting) {	/* reset or release */
					TRACE(3,"unblocked INPUT_DATA\n");
					unblocked_input = 1;
					wake_up_interruptible(&w8Q);
					INTERVAL_STOP(timeInputWait);
					INTERVAL_START(timeInputFetch);
					return -EPIPE;	/* only return 0 if data is required */
				}
				input_waiting = 0;
				INTERVAL_STOP(timeInputWait);
				INTERVAL_START(timeInputFetch);
				TRACE(3, "E7B_WMA_INPUT_DATA needs input (avail=%d, incount=%lu)\n", input_available, incount);
			}
			break;
	}
	return retval;
}

static int wma_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;

	if (vma->vm_pgoff != 0) {
		ERRMSG("wma_mmap: pgoff must be 0 (%lu)\n", vma->vm_pgoff);
		return -EINVAL;
	}

	if ((vma->vm_end - vma->vm_start) != wma_bufsize) {
		ERRMSG("wma_mmap: vma size mismatch %p %p %d\n", (void*)vma->vm_end, (void*)vma->vm_start, wma_bufsize);
		return -EINVAL;
	}

	if (!wma_buff) {
		ERRMSG("wma_mmap: mem not allocated\n");
		return -ENOMEM;
	}

	ret = remap_page_range(vma->vm_start, wma_buff_bus, wma_bufsize, PAGE_SHARED);
//	ret = remap_pfn_range(vma, vma->vm_start, wma_buff_bus >> PAGE_SHIFT, wma_bufsize, vma->vm_page_prot);
	if (ret) {
		ERRMSG("wma_mmap: remap failed\n");
		return ret;
	}

	//vma->vm_ops = &wma_vmops;
	return 0;
}

struct file_operations e7b_wma_fops = { /* no read/write, just ioctl and mmap */
	.owner	 = THIS_MODULE,
	.open    = wma_open,
	.ioctl   = wma_ioctl,
	.release = wma_release,
	.mmap	 = wma_mmap
};

