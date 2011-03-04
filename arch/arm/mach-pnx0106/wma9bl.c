
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define printk	pnx0106_epics_printk
#else
#define printk(fmt...)
#endif

#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/wma9bl.h>

typedef enum wma9bl_driver_state
{
    wma9bl_driver_idle,
    wma9bl_driver_getting_header,
    wma9bl_driver_decoding,
    wma9bl_driver_seeking,
    wma9bl_driver_resyncing,
    wma9bl_driver_streaming

}
wma9bl_driver_state_t;

wma9bl_driver_state_t wma9bl_driver_state = wma9bl_driver_idle;

static u32 wma_bufsize = 0;
static e7b_wma_buffer *wma_buff = 0;
static dma_addr_t wma_buff_bus;

static interval_t timeInit;
static interval_t timeReset;

extern interval_t timeSleep;
extern interval_t timeSleep10;

extern int wma9bl_timeout_counter;

// Will want these to be externs eventually ....
int foregroundUseFlag =0;
int backgroundUseFlag =0;

typedef struct wmaBackgroundInstanceInfo
{
    int wma_buff;
    u32 wma_bufsize;
    dma_addr_t wma_buff_bus;
    interval_t timeInit;
    interval_t timeReset;
    interval_t timeSleep;
    interval_t timeSleep10;
    wma9bl_driver_state_t wma9bl_driver_state;
} wmaBackgroundInstanceInfo_t;

static wmaBackgroundInstanceInfo_t backgroundInstanceInfo;

static int
backup_block_restore(void)
{
    int ret = 0;

    // wait for EPICS decoder to exit;
    // Need to let it do it in its own time - cannot block
    // here since EPICS might need some data before it
    // completes
    if(!pnx0106_epics_decoder_exited())
    {
        //pnx0106_epics_printk("Epics debug = %x\n", *((int*)pnx0106_epics_xram2arm(0xffd8)));
        //pnx0106_epics_sleep(10);
        return 0;
    }

    backgroundInstanceInfo.wma_buff = (int)wma_buff;
    backgroundInstanceInfo.wma_bufsize = wma_bufsize;
    backgroundInstanceInfo.wma_buff_bus = wma_buff_bus;
    backgroundInstanceInfo.timeInit = timeInit;
    backgroundInstanceInfo.timeReset = timeReset;
    backgroundInstanceInfo.timeSleep = timeSleep;
    backgroundInstanceInfo.timeSleep10 = timeSleep10;
    backgroundInstanceInfo.wma9bl_driver_state = wma9bl_driver_state;
    // save relevant driver info
    if((ret=wma9bldecStreamSave())<0)
    {
        return ret;
    }

    // save X mem and zero
    printk(" pnx0106_epics_copy_xmem2sdram...\n");
    pnx0106_epics_copy_xmem2sdram();

    pnx0106_epics_decoder_release();

    printk(" epics_zero_xmem...\n");
    pnx0106_epics_zero_decoder_xmem();

    // Need to sit and wait until foreground task is finished

    backgroundUseFlag--;
    pnx0106_epics_sleep(100);


    while(foregroundUseFlag)
    {
        pnx0106_epics_sleep(10);
    }

    backgroundUseFlag++;

    if (pnx0106_epics_decoder_claim(PNX0106_EPICS_WMA9BL_FWID) < 0)
        return -EBUSY;

    if (pnx0106_epics_select_firmware(PNX0106_EPICS_WMA9BL_FWID) < 0)
    {
	    pnx0106_epics_decoder_release();
	    return -ENXIO;
    }

    if (pnx0106_epics_reload_firmware() < 0)
    {
	    pnx0106_epics_decoder_release();
	    return -ENXIO;
    }

    printk("pnx0106_epics_copy_sdram2xmem...\n");
    pnx0106_epics_copy_sdram2xmem();
    pnx0106_epics_sleep(100);

    pnx0106_epics_set_exit_decoder(0);

    if((ret=wma9bldecStreamRestore())<0)
    {
        return ret;
    }

    wma_buff = (e7b_wma_buffer *)(backgroundInstanceInfo.wma_buff);
    wma_bufsize = backgroundInstanceInfo.wma_bufsize;
    wma_buff_bus = backgroundInstanceInfo.wma_buff_bus;

    timeInit = backgroundInstanceInfo.timeInit;
    timeReset = backgroundInstanceInfo.timeReset;
    timeSleep = backgroundInstanceInfo.timeSleep;
    timeSleep10 = backgroundInstanceInfo.timeSleep10;
    wma9bl_driver_state = backgroundInstanceInfo.wma9bl_driver_state;

    if(pnx0106_epics_need_to_resume_stream())
    {
        // resume streaming - return does not mean anything to us here ..
        wma9bldecStreamResume();
    }
    return ret;
}



static int
wma_open(struct inode *inode, struct file *fp)
{
    TRACE(5, "%s\n", __FUNCTION__);



    if ((fp->f_mode & (FMODE_WRITE|FMODE_READ)) != (FMODE_WRITE|FMODE_READ))
    {
	ERRMSG("wma interface can only be used in RW mode\n");
	return -EINVAL;
    }

    if (pnx0106_epics_decoder_claim(PNX0106_EPICS_WMA9BL_FWID) < 0)
	    return -EBUSY;

    if (pnx0106_epics_select_firmware(PNX0106_EPICS_WMA9BL_FWID) < 0)
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
    // Moved ..

    /*
     * 	Init state vars
     *
     */
    wma9bl_driver_state = wma9bl_driver_idle;

    INTERVAL_INIT(timeInit);
    INTERVAL_INIT(timeReset);
    INTERVAL_INIT(timeSleep);
    INTERVAL_INIT(timeSleep10);

    /*
     * 	Start epics decoder
     */
    wma9bldecNew();


    return 0;
}

static int
wma_open_fore(struct inode *inode, struct file *fp)
{

    int ret;
    printk("Hello! wma_open_fore, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    if(backgroundUseFlag)
    {
        // Send command to the Epics to Stop
        pnx0106_epics_set_exit_decoder(1);


        // wait for global use flag to get set to 0
        while(backgroundUseFlag)
        {
            pnx0106_epics_sleep(10);
        }
    }
    foregroundUseFlag++;

    //return 0;

    ret = wma_open(inode, fp);

    if(ret)
    {
        foregroundUseFlag--;
    }
    else
    {
        /*
        * 	Allocate foreground buffer for decoder
        */
        wma_bufsize = PAGE_ALIGN(sizeof(e7b_wma_buffer)); /* size must be multiple of PAGE_SIZE for mmap */
        TRACE(1, "wma_open: alloc %d(%d)\n", wma_bufsize, sizeof(e7b_wma_buffer));
        wma_buff = pnx0106_epics_alloc_coherent(NULL, wma_bufsize, &wma_buff_bus, 0);

        if (!wma_buff)
        {
	        pnx0106_epics_decoder_release();
	        return -ENOMEM;
        }

        printk("FORE: wma_buff = 0x%x, wma_bufsize = %d\n",wma_buff, wma_bufsize);

    }

    return ret;

}

static int
wma_open_back(struct inode *inode, struct file *fp)
{

    int ret;
    printk("Hello! wma_open_back, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }

    pnx0106_epics_alloc_backup_buffer();

    backgroundUseFlag++;

    ret = wma_open(inode, fp);

    if(ret)
    {
        backgroundUseFlag--;
        if(!backgroundUseFlag)
        {
            pnx0106_epics_free_backup_buffer();
        }
    }
    else
    {
        /*
        * 	Allocate background buffer for decoder
        */
        wma_bufsize = PAGE_ALIGN(sizeof(e7b_wma_buffer)); /* size must be multiple of PAGE_SIZE for mmap */
        TRACE(1, "wma_open: alloc %d(%d)\n", wma_bufsize, sizeof(e7b_wma_buffer));
        wma_buff = pnx0106_epics_alloc_coherent(NULL, wma_bufsize, &wma_buff_bus, 1);

        if (!wma_buff)
        {
	        pnx0106_epics_decoder_release();
	        return -ENOMEM;
        }

        printk("BACK: wma_buff = 0x%x, wma_bufsize = %d\n",wma_buff, wma_bufsize);
    }

    return ret;

}

static void
print_interval(char *label, interval_t *pInterval)
{
    INTERVAL_CALC(*pInterval);
    pnx0106_epics_printk("wma9bl: %-8s total %10llu avg %7llu std. %7lu count %6lu\n    max %7llu min %7llu\n",
	    label, pInterval->total, pInterval->avg, pInterval->std, pInterval->count,
	    pInterval->max, pInterval->min);
}

static void
print_intervals(void)
{
    print_interval("init", &timeInit);
    print_interval("reset", &timeReset);
    pnx0106_epics_printk("wma9bl: timeout count = %d\n", wma9bl_timeout_counter);
    wma9bl_timeout_counter = 0;
}

static void
wait_epics_idle(void)
{
    if (wma9bl_driver_state != wma9bl_driver_idle)
    {
	int i;

        wma9bldecInputEOF();

        if(wma9bl_driver_state == wma9bl_driver_streaming)
            wma9bldecOutputPassOwnership();

	for (i = 0; i < 50; i++)
	{
	    if (wma9bldecContinue() == 0)
		break;

	    if(wma9bl_driver_state == wma9bl_driver_streaming)
		wma9bldecOutputPassOwnership();

	    wma9bldecInputData(NULL, 0);
	    pnx0106_epics_sleep(10);
	}

	if (i >= 50)
	    pnx0106_epics_printk("***************** EPICS FAILED TO COMPLETE COMMAND *****************\n");

	wma9bl_driver_state = wma9bl_driver_idle;
    }
}

static int
wma_release(struct inode *inode, struct file *fp)
{
    TRACE(1, "%s\n", __FUNCTION__);

    wait_epics_idle();
    wma9bldecDelete();

    pnx0106_epics_free_coherent(NULL, wma_bufsize, wma_buff, wma_buff_bus);
    wma_bufsize = 0;
    wma_buff = 0;
    wma_buff_bus = 0;

    pnx0106_epics_decoder_release();




    print_intervals();
    pnx0106_epics_printk("EPICS OutputBuffer Wait Count:  high %x low %x\n", *((int*)pnx0106_epics_xram2arm(0x5651)),*((int*)pnx0106_epics_xram2arm(0x5650)));
    pnx0106_epics_printk("EPICS InputBuffer Wait Count: high %x low %x\n",*((int*)pnx0106_epics_xram2arm(0x5653)),*((int*)pnx0106_epics_xram2arm(0x5652)));

    return 0;
}

static int
wma_release_back(struct inode *inode, struct file *fp)
{
    int ret;

    printk("Hello! wma_release_back, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }

    // Also, need to check if a foreground task is requesting use of the EPICS
    // If so, need to backup and block and only unblock and restore when
    // it has finished
    if(pnx0106_epics_get_exit_decoder())
    {

        printk("Halting at wma_release_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    pnx0106_epics_free_backup_buffer();
    ret = wma_release(inode, fp);

    backgroundUseFlag--;

    return ret;

}

static int
wma_release_fore(struct inode *inode, struct file *fp)
{
    int ret;

    printk("Hello! wma_release_fore, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    ret = wma_release(inode, fp);

    foregroundUseFlag--;

    return ret;

}

static int
wma_ioctl(struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
    int retval = 0;

    TRACE(1, "%s\n", __FUNCTION__);

    switch (command)
    {
    case E7B_WMA_INIT:
	{
	    int ret;
	    TRACE(1,"E7B_WMA_INIT\n");

	    // Start of Stream

	    /* fill references */
	    INTERVAL_START(timeInit);
	    ret = wma9bldecInit();
	    INTERVAL_STOP(timeInit);

	    if (ret < 0)
		return -EIO;
	}

	break;
    case E7B_WMA_GET_ASF_HDR:
	{
	    int ret;
	    e7b_wma_get_hdr gh;

	    TRACE(1, "E7B_WMA_GET_ASF_HDR\n");

	    switch (wma9bl_driver_state)
	    {
	    case wma9bl_driver_idle:
		ret = wma9bldecGetAsfHdr();
		wma9bl_driver_state = wma9bl_driver_getting_header;
		break;
	    case wma9bl_driver_getting_header:
		ret = wma9bldecContinue();
		break;
	    default:
		return -EIO;
	    }

	    gh.input.needInput = 0;

	    if (ret == 0)
	    {
		wma9bldecGetAsfHdrFinish(&gh.hdr, &gh.decReturn);
		printk("GetAsfHdr returned 0x%x\n", gh.decReturn);

		gh.ready = 1;
		wma9bl_driver_state = wma9bl_driver_idle;
	    }
	    else
	    {
		gh.input.needInput = 1;
		gh.input.inputOffset = wma9bldecGetStreamByteOffset();
		gh.ready = 0;
	    }

	    if (copy_to_user((void *)arg, &gh, sizeof(gh)))
		return -EFAULT;

	}

	break;
    case E7B_WMA_DECODE:
	{
	    int ret;
	    e7b_wma_decode dp;

	    switch (wma9bl_driver_state)
	    {
	    case wma9bl_driver_idle:
		ret = wma9bldecDecode();
		wma9bl_driver_state = wma9bl_driver_decoding;
		break;
	    case wma9bl_driver_decoding:
		ret = wma9bldecContinue();
		break;
	    default:
		return -EIO;
	    }

	    dp.samplesReady = 0;
	    dp.filedone = 0;
	    dp.input.needInput = 0;

	    if (ret == 0)
	    {
		wma9bldecDecodeFinish(&dp.samplesReady, &dp.filedone, &dp.decReturn);

		if (dp.decReturn != 0)
		    printk("Decode returned 0x%x\n",dp.decReturn);

		wma9bl_driver_state = wma9bl_driver_idle;
	    }
	    else
	    {
		dp.input.needInput = 1;
		dp.input.inputOffset = wma9bldecGetStreamByteOffset();
	    }

	    TRACE(3, "<<E7B_WMA_DECODE sReady=%u state=%d\n", dp.samplesReady, dp.filedone);

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

	    if (wma9bl_driver_state != wma9bl_driver_idle)
		return -EIO;

	    wma9bldecGetPCM(gp.samplesRequested,	// up to *pcSamplesReady (from wma9bldecDecode())
		   &gp.samplesReturned, // [out] redundant with *pcbDstUsed below
		   (u8*)wma_buff->output,
		   gp.dstLength,
           &gp.state); 		// ~ redundant with cSamplesRequested
	    TRACE(4, "<<E7B_WMA_GETPCM sRet=%u state=%d\n", gp.samplesReturned, gp.state);

	    if (copy_to_user((void *)arg, &gp, sizeof(gp)))
		return -EFAULT;
	}

	break;
    case E7B_WMA_RESET:
	TRACE(1, "E7B_WMA_RESET\n");
	INTERVAL_START(timeReset);
	wait_epics_idle();
	wma9bldecReset();
	INTERVAL_STOP(timeReset);
	break;
    case E7B_WMA_STREAM_SEEK:
	{
	    int ret;
	    e7b_wma_streamseek ss;

	    if (copy_from_user(&ss, (void*)arg, sizeof(ss)))
		return -EFAULT;

	    TRACE(4, ">>E7B_WMA_STREAM_SEEK %u\n", ss.seekTimeFromStart);

	    switch (wma9bl_driver_state)
	    {
	    case wma9bl_driver_idle:
		ret = wma9bldecStreamSeek(ss.seekTimeFromStart);
		wma9bl_driver_state = wma9bl_driver_seeking;
		break;
	    case wma9bl_driver_seeking:
		ret = wma9bldecContinue();
		break;
	    default:
		return -EIO;
	    }

	    ss.input.needInput = 0;

	    if (ret == 0)
	    {
		wma9bldecStreamSeekFinish(&ss.actualSeekTimeFromStart, &ss.decReturn);
		printk("Seeking returned 0x%x\n", ss.decReturn);
		wma9bl_driver_state = wma9bl_driver_idle;
	    }
	    else
	    {
		ss.input.needInput = 1;
		ss.input.inputOffset = wma9bldecGetStreamByteOffset();
	    }

	    TRACE(4, "<<E7B_WMA_STREAM_SEEK complete=%d \n", !ss.input.needInput );

	    if (copy_to_user((void *)arg, &ss, sizeof(ss)))
		return -EFAULT;
	}

	break;
    case E7B_WMA_RESYNCH:
	{
	    int ret;
	    e7b_wma_resynch rs;

	    if (copy_from_user(&rs, (void*)arg, sizeof(rs)))
		return -EFAULT;

	    TRACE(4, ">>E7B_WMA_STREAM_SEEK %u\n", rs.seekOffsetInBytes);

	    switch (wma9bl_driver_state)
	    {
	    case wma9bl_driver_idle:
		ret = wma9bldecResynch(rs.seekOffsetInBytes);
		wma9bl_driver_state = wma9bl_driver_resyncing;
		break;
	    case wma9bl_driver_resyncing:
		ret = wma9bldecContinue();
		break;
	    default:
		return -EIO;
	    }

	    rs.complete = 0;
	    rs.input.needInput = 0;

	    if (ret == 0)
	    {
		wma9bldecResynchFinish(&rs.decReturn);
		printk("Resynching returned 0x%x\n", rs.decReturn);
		wma9bl_driver_state = wma9bl_driver_idle;
		rs.complete = 1;
	    }
	    else
	    {
		rs.input.needInput = 1;
		rs.input.inputOffset = wma9bldecGetStreamByteOffset();
	    }

	    if (copy_to_user((void *)arg, &rs, sizeof(rs)))
		return -EFAULT;
	}

	break;
    case E7B_WMA_INPUT_EOF:
	wma9bldecInputEOF();
	break;

    case E7B_WMA_STREAM:
	{
	    int ret;
	    e7b_wma_stream st;

	    TRACE(4, ">>E7B_WMA_STREAM\n");

	    switch (wma9bl_driver_state)
	    {
	    case wma9bl_driver_idle:
		ret = wma9bldecStreamStart();
		wma9bl_driver_state = wma9bl_driver_streaming;
		break;
	    case wma9bl_driver_streaming:
		ret = wma9bldecContinue();
		break;
	    default:
		return -EIO;
	    }

	    st.decReturn = 0;

	    if (ret == 0)
	    {
		    wma9bldecStreamFinish(&st.decReturn);

		    if (st.decReturn != 0)
            {
		        pnx0106_epics_printk("Stream finish returned 0x%x\n", st.decReturn);

            }
            wma9bl_driver_state = wma9bl_driver_idle;

	    }

	    if (copy_to_user((void *)arg, &st, sizeof(st)))
		return -EFAULT;
	}

	break;

    case E7B_WMA_STREAM_GETPCM:
	{
	    e7b_wma_streamgetpcm sgp;

	    TRACE(4, ">>E7B_WMA_GETPCM \n");

	    sgp.samplesReturned=0;
	    sgp.decReturn = 0;

	    wma9bldecStreamGetPCM(
		   &sgp.samplesReturned,
		   (u8*)wma_buff->output,
		   &sgp.decReturn); 		// ~ redundant with cSamplesRequested
	    TRACE(4, "<<E7B_WMA_GETPCM sRet=%u decReturn=%d\n", sgp.samplesReturned, sgp.decReturn);

        if (copy_to_user((void *)arg, &sgp, sizeof(sgp)))
		return -EFAULT;
	}

	break;

    case E7B_WMA_SET_AES_KEY:
	{
	    int ret;
        e7b_wma_set_aes_key sak;

	    if (copy_from_user(&sak, (void*)arg, sizeof(sak)))
		return -EFAULT;

	    TRACE(1, "E7B_WMA_SET_AES_KEY\n");


	    /* fill references */
        ret = wma9bldecSetAesKey(sak.key);


	    if (ret)
		return -EIO;
    }

	break;
    }

    return retval;
}

static int
wma_ioctl_back(struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
    int ret;

    // If there's a foreground task already running, need to block
    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }

    // Also, need to check if a foreground task is requesting use of the EPICS
    // If so, need to backup and block and only unblock and restore when
    // it has finished
    if(pnx0106_epics_get_exit_decoder())
    {

        printk("Halting at wma_ioctl_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }


    return wma_ioctl(inode, fp, command, arg);
}

static int
wma_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    static char tmp[E7B_WMA_MAX_INPUT];
    int n = sizeof tmp;

    if (n > count)
	n = count;

    // We should really redefine the lower level interface so that we
    // can avoid this copy.  Rather than tmp we should go straight to
    // the epics.  This is safe though for now.
    if (copy_from_user(tmp, buf, n))
	return -EFAULT;

    return wma9bldecInputData(tmp, n);

}



static int
wma_write_back(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    int ret;

    // If there's a foreground task already running, need to block
    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }

    // Also, need to check if a foreground task is requesting use of the EPICS
    // If so, need to backup and block and only unblock and restore when
    // it has finished
    if(pnx0106_epics_get_exit_decoder())
    {

        printk("Halting at wma_write_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    return wma_write(file, buf, count, ppos);
}

static int
wma_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret;

    if (vma->vm_pgoff != 0)
    {
	ERRMSG("wma_mmap: pgoff must be 0 (%lu)\n", vma->vm_pgoff);
	return -EINVAL;
    }

    if ((vma->vm_end - vma->vm_start) != wma_bufsize)
    {
	ERRMSG("wma_mmap: vma size mismatch %p %p %d\n", (void*)vma->vm_end, (void*)vma->vm_start, wma_bufsize);
	return -EINVAL;
    }

    if (!wma_buff)
    {
	ERRMSG("wma_mmap: mem not allocated\n");
	return -ENOMEM;
    }

    ret = remap_page_range(vma->vm_start, wma_buff_bus, wma_bufsize, PAGE_SHARED);
//    ret = remap_pfn_range(vma, vma->vm_start, wma_buff_bus >> PAGE_SHIFT, wma_bufsize, vma->vm_page_prot);

    if (ret)
    {
	ERRMSG("wma_mmap: remap failed\n");
	return ret;
    }

    //vma->vm_ops = &wma_vmops;
    return 0;
}

static int
wma_mmap_back(struct file *file, struct vm_area_struct *vma)
{
    int ret;

    // If there's a foreground task already running, need to block
    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }
    // Also, need to check if a foreground task is requesting use of the EPICS
    // If so, need to backup and block and only unblock and restore when
    // it has finished
    if(pnx0106_epics_get_exit_decoder())
    {

        printk("Halting at wma_mmap_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    return wma_mmap(file, vma);
}

struct file_operations pnx0106_epics_wma9bl_fops =
{
    /* no read/write, just ioctl and mmap */
    .owner   = THIS_MODULE,
    .open    = wma_open_fore,
    .ioctl   = wma_ioctl,
    .release = wma_release_fore,
    .write   = wma_write,
    .mmap    = wma_mmap
};

struct file_operations pnx0106_epics_wma9b_fops =
{
    /* no read/write, just ioctl and mmap */
    .owner   = THIS_MODULE,
    .open    = wma_open_back,
    .ioctl   = wma_ioctl_back,
    .release = wma_release_back,
    .write   = wma_write_back,
    .mmap    = wma_mmap_back
};

