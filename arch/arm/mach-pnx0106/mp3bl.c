
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
#include <asm/arch/mp3bl.h>

typedef enum mp3_driver_state
{
    mp3_driver_idle,
    mp3_driver_decoding,
    mp3_driver_streaming

}
mp3_driver_state_t;

mp3_driver_state_t mp3_driver_state = mp3_driver_idle;

static u32 mp3_bufsize = 0;
static e7b_mp3_buffer *mp3_buff = 0;
static dma_addr_t mp3_buff_bus;

static interval_t timeInit;
static interval_t timeReset;
static interval_t timeGetPCM;
static interval_t timeWrite;
       interval_t mp3timeWait;

extern interval_t timeSleep;
extern interval_t timeSleep10;

extern int foregroundUseFlag;
extern int backgroundUseFlag;

typedef struct mp3BackgroundInstanceInfo
{
    int mp3_buff;
    u32 mp3_bufsize;
    dma_addr_t mp3_buff_bus;
    interval_t timeInit;
    interval_t timeReset;
    interval_t timeGetPCM;
    interval_t timeWrite;
    interval_t mp3timeWait;
    interval_t timeSleep;
    interval_t timeSleep10;
    mp3_driver_state_t mp3_driver_state;
} mp3BackgroundInstanceInfo_t;

static mp3BackgroundInstanceInfo_t backgroundInstanceInfo;

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


    backgroundInstanceInfo.mp3_buff = (int)mp3_buff;
    backgroundInstanceInfo.mp3_bufsize = mp3_bufsize;
    backgroundInstanceInfo.mp3_buff_bus = mp3_buff_bus;
    backgroundInstanceInfo.timeInit = timeInit;
    backgroundInstanceInfo.timeReset = timeReset;
    backgroundInstanceInfo.timeGetPCM = timeGetPCM;
    backgroundInstanceInfo.timeWrite = timeWrite;
    backgroundInstanceInfo.mp3timeWait = mp3timeWait;
    backgroundInstanceInfo.timeSleep = timeSleep;
    backgroundInstanceInfo.timeSleep10 = timeSleep10;
    backgroundInstanceInfo.mp3_driver_state = mp3_driver_state;

    // save relevant driver info
    if((ret=mp3decStreamSave())<0)
    {
        return ret;
    }

    // save X mem and zero
    printk(" pnx0106_epics_copy_xmem2sdram...\n");
    pnx0106_epics_copy_xmem2sdram();

    pnx0106_epics_decoder_release();

    printk(" pnx0106_epics_zero_decoder_xmem...\n");
    pnx0106_epics_zero_decoder_xmem();

    // Need to sit and wait until foreground task is finished

    backgroundUseFlag--;
    pnx0106_epics_sleep(100);


    while(foregroundUseFlag)
    {
        pnx0106_epics_sleep(10);
    }

    backgroundUseFlag++;

    if (pnx0106_epics_decoder_claim(PNX0106_EPICS_MP3_FWID) < 0)
        return -EBUSY;

    if (pnx0106_epics_select_firmware(PNX0106_EPICS_MP3_FWID) < 0)
    {
	    pnx0106_epics_decoder_release();
	    return -ENXIO;
    }

    if (pnx0106_epics_reload_firmware() < 0)
    {
	    pnx0106_epics_decoder_release();
	    return -ENXIO;
    }

    printk(" pnx0106_epics_copy_sdram2xmem...\n");
    pnx0106_epics_copy_sdram2xmem();
    pnx0106_epics_sleep(100);

    pnx0106_epics_set_exit_decoder(0);

    if((ret=mp3decStreamRestore())<0)
    {
        return ret;
    }

    mp3_buff = (e7b_mp3_buffer*) backgroundInstanceInfo.mp3_buff;
    mp3_bufsize = backgroundInstanceInfo.mp3_bufsize;
    mp3_buff_bus = backgroundInstanceInfo.mp3_buff_bus;
    timeInit = backgroundInstanceInfo.timeInit;
    timeReset = backgroundInstanceInfo.timeReset;
    timeGetPCM = backgroundInstanceInfo.timeGetPCM;
    timeWrite = backgroundInstanceInfo.timeWrite;
    mp3timeWait = backgroundInstanceInfo.mp3timeWait;
    timeSleep = backgroundInstanceInfo.timeSleep;
    timeSleep10 = backgroundInstanceInfo.timeSleep10;
    mp3_driver_state = backgroundInstanceInfo.mp3_driver_state;

    if(pnx0106_epics_need_to_resume_stream())
    {
        // resume streaming - return does not mean anything to us here.
        mp3decStreamResume();
    }

    return ret;
}

static int
mp3_open(struct inode *inode, struct file *fp)
{
    TRACE(5, "%s\n", __FUNCTION__);

    if ((fp->f_mode & (FMODE_WRITE|FMODE_READ)) != (FMODE_WRITE|FMODE_READ))
    {
	ERRMSG("mp3 interface can only be used in RW mode\n");
	return -EINVAL;
    }

    if (pnx0106_epics_decoder_claim(PNX0106_EPICS_MP3_FWID) < 0)
	return -EBUSY;

    if (pnx0106_epics_select_firmware(PNX0106_EPICS_MP3_FWID) < 0)
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
     * 	Init state vars
     *
     */
    mp3_driver_state = mp3_driver_idle;

    INTERVAL_INIT(timeInit);
    INTERVAL_INIT(timeReset);
    INTERVAL_INIT(timeGetPCM);
    INTERVAL_INIT(timeWrite);
    INTERVAL_INIT(mp3timeWait);

    INTERVAL_INIT(timeSleep);
    INTERVAL_INIT(timeSleep10);

    /*
     * 	Start epics decoder
     */
    if ((int)mp3decInit() < 0)
    {
	pnx0106_epics_decoder_release();
	pnx0106_epics_printk("MP3: firmware image is not compatible with this driver\n");
	return -ENXIO;
    }

    /*
     * 	Allocate buffer for decoder
     */
    // Moved ..

    return 0;
}

static int
mp3_open_fore(struct inode *inode, struct file *fp)
{

    int ret;
    printk("Hello! mp3_open_fore, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

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

    ret = mp3_open(inode, fp);

    if(ret)
    {
        foregroundUseFlag--;
    }
    else
    {
        /*
         * 	Allocate  foreground buffer for decoder
         */
        mp3_bufsize = PAGE_ALIGN(sizeof(e7b_mp3_buffer)); /* size must be multiple of PAGE_SIZE for mmap */
        TRACE(1, "mp3_open: alloc %d(%d)\n", mp3_bufsize, sizeof(e7b_mp3_buffer));
        mp3_buff = pnx0106_epics_alloc_coherent(NULL, mp3_bufsize, &mp3_buff_bus, 0);

        if (!mp3_buff)
        {
	        pnx0106_epics_decoder_release();
	        return -ENOMEM;
        }

        printk("FORE : mp3_buff = 0x%x, mp3_bufsize = %d, \n",mp3_buff, mp3_bufsize);
    }

    return ret;

}

static int
mp3_open_back(struct inode *inode, struct file *fp)
{

    int ret;
    printk("Hello! mp3_open_back, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    while(foregroundUseFlag)
    {
        // wait for foreground use flag to get set to 0
        pnx0106_epics_sleep(10);

    }

    pnx0106_epics_alloc_backup_buffer();

    backgroundUseFlag++;

    ret = mp3_open(inode, fp);

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
        mp3_bufsize = PAGE_ALIGN(sizeof(e7b_mp3_buffer)); /* size must be multiple of PAGE_SIZE for mmap */
        TRACE(1, "mp3_open: alloc %d(%d)\n", mp3_bufsize, sizeof(e7b_mp3_buffer));
        mp3_buff = pnx0106_epics_alloc_coherent(NULL, mp3_bufsize, &mp3_buff_bus, 0);

        if (!mp3_buff)
        {
	        pnx0106_epics_decoder_release();
	        return -ENOMEM;
        }

        printk("BACK : mp3_buff = 0x%x, mp3_bufsize = %d, \n",mp3_buff, mp3_bufsize);
    }

    return ret;

}

static void
print_interval(char *label, interval_t *pInterval)
{
    INTERVAL_CALC(*pInterval);
    pnx0106_epics_printk("mp3bl: %-8s total %10llu avg %7llu std. %7lu count %6lu\n    max %7llu min %7llu\n",
	    label, pInterval->total, pInterval->avg, pInterval->std, pInterval->count,
	    pInterval->max, pInterval->min);
}

static void
print_intervals(void)
{
//    extern interval_t timeIrq[8];
//    char name[20];
//    int i;

    print_interval("init", &timeInit);
    print_interval("reset", &timeReset);
    print_interval("getpcm", &timeGetPCM);
    print_interval("write", &timeWrite);
    print_interval("wait", &mp3timeWait);
}

static void
wait_epics_idle(void)
{
    if (mp3_driver_state != mp3_driver_idle)
    {
	int i;

        mp3decInputEOF();

        if(mp3_driver_state == mp3_driver_streaming)
            mp3decOutputPassOwnership();

	for (i = 0; i < 50; i++)
	{
	    if (mp3decContinue() == 0)
		break;

	    if(mp3_driver_state == mp3_driver_streaming)
		mp3decOutputPassOwnership();

	    mp3decInputData(NULL, 0);
	    pnx0106_epics_sleep(10);
	}

	if (i >= 50)
	    pnx0106_epics_printk("***************** EPICS FAILED TO COMPLETE COMMAND *****************\n");

	mp3_driver_state = mp3_driver_idle;
    }
}

static int
mp3_release(struct inode *inode, struct file *fp)
{
    TRACE(1, "%s\n", __FUNCTION__);


    wait_epics_idle();
    mp3decDeInit();

    pnx0106_epics_free_coherent(NULL, mp3_bufsize, mp3_buff, mp3_buff_bus);
    mp3_bufsize = 0;
    mp3_buff = 0;
    mp3_buff_bus = 0;

    pnx0106_epics_decoder_release();

    print_intervals();
    pnx0106_epics_printk("EPICS OutputBuffer Wait Count:  high %x low %x\n", *((int*)pnx0106_epics_xram2arm(0x5651)),*((int*)pnx0106_epics_xram2arm(0x5650)));
    pnx0106_epics_printk("EPICS InputBuffer Wait Count: high %x low %x\n",*((int*)pnx0106_epics_xram2arm(0x5653)),*((int*)pnx0106_epics_xram2arm(0x5652)));

    return 0;
}

static int
mp3_release_back(struct inode *inode, struct file *fp)
{
    int ret;

    printk("Hello! mp3_release_back, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

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

        printk("Halting at mp3_release_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    pnx0106_epics_free_backup_buffer();
    ret = mp3_release(inode, fp);

    backgroundUseFlag--;

    return ret;

}

static int
mp3_release_fore(struct inode *inode, struct file *fp)
{
    int ret;

    printk("Hello! mp3_release_fore, backgroundUseFlag = %d, foregroundUseFlag = %d\n", backgroundUseFlag, foregroundUseFlag);

    ret = mp3_release(inode, fp);

    foregroundUseFlag--;

    return ret;

}

static int
mp3_ioctl(struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
    int retval = 0;

    TRACE(1, "%s\n", __FUNCTION__);

    switch (command)
    {
    case E7B_MP3_INIT:
	{
	    int ret;
	    TRACE(1, ">>E7B_MP3_INIT\n");

	    // Start of Stream

	    /* fill references */
	    INTERVAL_START(timeInit);
	    ret = mp3decInit();
	    INTERVAL_STOP(timeInit);

	    if (ret < 0)
		return -EIO;
	}

	break;
    case E7B_MP3_DECODE:
	{
	    int ret;
	    e7b_mp3_decode dp;
	    TRACE(1, ">>E7B_MP3_DECODE\n");

	    switch (mp3_driver_state)
	    {
	    case mp3_driver_idle:
		ret = mp3decDecode();
		mp3_driver_state = mp3_driver_decoding;
		break;
	    case mp3_driver_decoding:
		ret = mp3decContinue();
		break;
	    default:
		return -EIO;
	    }

	    dp.samplesReady = 0;
	    dp.needInput = 0;
	    dp.decReturn = 0;

	    if (ret == 0)
	    {
		mp3decDecodeFinish(&mp3_buff->out[0].hdr, &dp.samplesReady, mp3_buff->out[0].data, &dp.decReturn);

		if (dp.decReturn != 0)
		    pnx0106_epics_printk("Decode returned 0x%x\n", dp.decReturn);

		mp3_driver_state = mp3_driver_idle;
	    }
	    else
		dp.needInput = 1;

	    TRACE(3, "<<E7B_MP3_DECODE sReady=%u ret=%d\n", dp.samplesReady, ret);

	    if (copy_to_user((void *)arg, &dp, sizeof(dp)))
		return -EFAULT;
	}

	break;
    case E7B_MP3_RESET:
	TRACE(1, ">>E7B_MP3_RESET\n");
	INTERVAL_START(timeReset);
	wait_epics_idle();
	mp3decReset();
	INTERVAL_STOP(timeReset);
	break;
    case E7B_MP3_INPUT_EOF:
	mp3decInputEOF();
	break;
    case E7B_MP3_STREAM:

	{
	    int ret;
	    u32 decReturn;

        TRACE(4, ">>E7B_MP3_STREAM\n");

	    switch (mp3_driver_state)
	    {
	    case mp3_driver_idle:
		ret = mp3decStreamStart();

		mp3_driver_state = mp3_driver_streaming;
		break;
	    case mp3_driver_streaming:
		ret = mp3decContinue();
		break;
	    default:
		return -EIO;
	    }

	    decReturn = 0;

	    if (ret == 0)
	    {
		mp3decStreamFinish(&decReturn);

		if (decReturn != 0)
		    printk("Stream finish returned 0x%x\n", decReturn);

		mp3_driver_state = mp3_driver_idle;
	    }

	    if (copy_to_user((void *)arg, &decReturn, sizeof decReturn))
		return -EFAULT;
	}

	break;

    case E7B_MP3_STREAM_RESUME:
    {
	    int ret;

        TRACE(4, ">>E7B_MP3_STREAM_RESUME\n");

	    switch (mp3_driver_state)
	    {
	    case mp3_driver_idle:
		case mp3_driver_streaming:
		ret = mp3decStreamResume();
        mp3_driver_state = mp3_driver_streaming;
        break;
	    default:
		return -EIO;
	    }
	}
    break;


    case E7B_MP3_STREAM_GETPCM:
	{
	    e7b_mp3_decode sgp;

	    INTERVAL_START(timeGetPCM);
	    TRACE(4, ">>E7B_MP3_STREAM_GETPCM \n");

	    sgp.needInput = 0;
	    sgp.decReturn = 0;

	    mp3decStreamGetPCM(
		   &mp3_buff->out[0].hdr,
		   &sgp.samplesReady,
		   mp3_buff->out[0].data,
		   &sgp.decReturn); 		// ~ redundant with cSamplesRequested
	    TRACE(4, "<<E7B_MP3_STREAM_GETPCM sRet=%u decReturn=%d\n", sgp.samplesReady, sgp.decReturn);

	    if (copy_to_user((void *)arg, &sgp, sizeof(sgp)))
		retval = -EFAULT;

	    INTERVAL_STOP(timeGetPCM);
	}

	break;
    }


    return retval;
}


static int
mp3_ioctl_back(struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
    int ret;

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

        printk("Halting at mp3_ioctl_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    ret = mp3_ioctl(inode, fp, command, arg);
    return ret;
}


static int
mp3_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    //static char tmp[E7B_MP3_MAX_INPUTSIZE];
    static char tmp[3*1024];
    int n = sizeof tmp;
    int ret;

    INTERVAL_START(timeWrite);

    if (n > count)
	n = count;

    // We should really redefine the lower level interface so that we
    // can avoid this copy.  Rather than tmp we should go straight to
    // the epics.  This is safe though for now.
    if (copy_from_user(tmp, buf, n))
	ret = -EFAULT;
    else
    {

	TRACE(4, ">>MP3_WRITE n=%u\n", n);
	ret = mp3decInputData(tmp, n);
	TRACE(4, "<<MP3_WRITE ret=%u\n", ret);
    }

    INTERVAL_STOP(timeWrite);
    return ret;
}

static int
mp3_write_back(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    int ret;

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

        printk("Halting at mp3_write_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    ret = mp3_write(file, buf, count, ppos);
    return ret;
}

static int
mp3_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret;

    if (vma->vm_pgoff != 0)
    {
	ERRMSG("mp3_mmap: pgoff must be 0 (%lu)\n", vma->vm_pgoff);
	return -EINVAL;
    }

    if ((vma->vm_end - vma->vm_start) != mp3_bufsize)
    {
	ERRMSG("mp3_mmap: vma size mismatch %p %p %d\n", (void*)vma->vm_end, (void*)vma->vm_start, mp3_bufsize);
	return -EINVAL;
    }

    if (!mp3_buff)
    {
	ERRMSG("mp3_mmap: mem not allocated\n");
	return -ENOMEM;
    }

    ret = remap_page_range(vma->vm_start, mp3_buff_bus, mp3_bufsize, PAGE_SHARED);
//    ret = remap_pfn_range(vma, vma->vm_start, mp3_buff_bus >> PAGE_SHIFT, mp3_bufsize, vma->vm_page_prot);

    if (ret)
    {
	ERRMSG("mp3_mmap: remap failed\n");
	return ret;
    }

    //vma->vm_ops = &mp3_vmops;
    return 0;
}

static int
mp3_mmap_back(struct file *file, struct vm_area_struct *vma)
{
    int ret;

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

        printk("Halting at mp3_mmap_back\n");
        if((ret = backup_block_restore())<0)
        {
            return ret;
        }

    }

    ret = mp3_mmap(file, vma);

    return ret;
}

struct file_operations pnx0106_epics_mp3_fops =
{
    /* no read/write, just ioctl and mmap */
    .owner   = THIS_MODULE,
    .open    = mp3_open_fore,
    .ioctl   = mp3_ioctl,
    .release = mp3_release_fore,
    .write   = mp3_write,
    .mmap    = mp3_mmap
};

struct file_operations pnx0106_epics_mp3b_fops =
{
    /* no read/write, just ioctl and mmap */
    .owner   = THIS_MODULE,
    .open    = mp3_open_back,
    .ioctl   = mp3_ioctl_back,
    .release = mp3_release_back,
    .write   = mp3_write_back,
    .mmap    = mp3_mmap_back
};
