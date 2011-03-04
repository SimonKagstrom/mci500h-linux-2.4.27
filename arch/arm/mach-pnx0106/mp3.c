#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
//#include <linux/dma-mapping.h>
//#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

//#define DEBUG

#ifdef DEBUG
#define printk	epics_printk
#else
#define printk(fmt...)
#endif

#include <asm/arch/tsc.h>
#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/mp3.h>

#define e7b_mp3_fops pnx0106_epics_mp3_fops

struct task_struct *kthread_create(int (*fn)(void*), int x, char *fmt);
int kthread_should_stop(void);
void kthread_stop(struct task_struct *task);
void kthread_init(void);
void *epics_alloc_coherent(void *p, int size, dma_addr_t *phys, int hmmm);
void epics_free_coherent(void *p, int size, void *ptr, dma_addr_t phys);


/* MPEG 1 LI,32kBps, 48 kHz (4 bytes per slot),=> 32*/
/* For MPEG2 MIN_FRAME_SIZE is 24                   */
/* assumed to be a multiple of BYTES_PER WORD !!!   */
#define  E7B_MP3_MIN_FRAME_BYTES      (24)

/*	define a circular read-write buffer for dma transfers */
typedef struct {
	unsigned long		size;	/* size of buffer */
	unsigned char*		data;
	int					rdpos;
	int					wrpos;
	int					count;
} e7b_cbuf;

/*	Some simple circular buffer macros  */
#define CBUF_WRAP(B,X) 	((X) % (B).size)
#define CBUF_RDPTR(B)	((B).data + (B).rdpos)
#define CBUF_WRPTR(B)	((B).data + (B).wrpos)
#define CBUF_COUNT(B)	((B).count)
#define CBUF_FULL(B)	((B).count >= (B).size)
#define CBUF_EMPTY(B)	((B).count <= 0)
#define CBUF_AVAIL(B)	(((B).size>(B).count) ? ((B).size - (B).count) : 0)
#define CBUF_INC(B,N)	((B).wrpos = CBUF_WRAP(B,(B).wrpos + N), (B).count += N)
#define CBUF_DEC(B,N)	((B).rdpos = CBUF_WRAP(B,(B).rdpos + N), (B).count -= N)

#define out_avail(IDX) (mp3_buff->out[IDX].count - consumed[IDX])

static int 					mp3_open_mode = 0;
static u32					mp3_bufsize = 0;
static e7b_mp3_buffer		*mp3_buff = 0;
static dma_addr_t			mp3_buff_bus;
static e7b_cbuf				mp3_input = { 0, 0, 0, 0, 0 };
static wait_queue_head_t 	w84decoder;
static wait_queue_head_t 	w84user;
static spinlock_t			mp3_lock = SPIN_LOCK_UNLOCKED;
static int					out_write_idx = 0;  /* will circle [0..E7B_MP3_NBUF[ */
static int					out_read_idx = 0;  /* will circle [0..E7B_MP3_NBUF[  */
static int				    consumed[E7B_MP3_NBUF];  /* nr samples already consumed (only used for read implementation) */
static u32					tot_size = 0;
static struct task_struct *decode_task = 0;
static e7b_mp3_decoder_state decode_state = e7b_mp3_processing;
static int mp3_reset_request = 0;
static int mp3_reset_complete = 0;

int e7b_mp3_decode_delay = 10; /* ms */
MODULE_PARM(e7b_mp3_decode_delay,"i");

/*
 * 	CBUF functions
 */
static void e7b_cbuf_reset(e7b_cbuf *cbuf)  /* just clear the usage info */
{
	cbuf->wrpos 	  	= 0;
	cbuf->rdpos 	  	= 0;
	cbuf->count 	  	= 0;
}

static int e7b_cbuf_write(e7b_cbuf *cbuf, int nbytes, const char *user_buf)
{
	int avail			= CBUF_AVAIL(*cbuf);
	int copy_size 		= (avail > nbytes) ? nbytes : avail;
	int at_end_avail 	= cbuf->size - cbuf->wrpos;
	int at_end_copy		= (at_end_avail > copy_size) ? copy_size : at_end_avail;
	int at_front_copy 	= copy_size - at_end_copy;

	if (at_end_copy > 0) {
		if (__copy_from_user(CBUF_WRPTR(*cbuf), user_buf, at_end_copy)) {
				return -EFAULT;
		}
	}
	if (at_front_copy > 0) {
		if (__copy_from_user(cbuf->data, user_buf+at_end_copy, at_front_copy)) {
				return -EFAULT;
		}
	}

	spin_lock(&mp3_lock);
	CBUF_INC(*cbuf, copy_size);
	spin_unlock(&mp3_lock);

	return copy_size;
}

static interval_t timeSync;
static interval_t timeHeader;
static interval_t timeDecode;
static interval_t timeInput;
static interval_t timeOutput;
static interval_t timeTotal;

extern interval_t timeSleep;
extern interval_t timeSleep10;

static void
print_interval(char *label, interval_t *pInterval)
{
    INTERVAL_CALC(*pInterval);
    epics_printk("mp3: %-8s total %10llu avg %7llu count %6lu max %7llu min %7llu\n",
	    label, pInterval->total, pInterval->avg, pInterval->count, pInterval->max,
	    pInterval->min);
}

static void print_stats (void)
{
    int totms;
    int decms;
    int sleepms;
    int inms;
    int outms;
    int decpct = 0;
    int sleeppct = 0;
    int inpct = 0;
    int outpct = 0;
    int loadpct = 0;
    unsigned int ticks_per_msec = ReadTSCMHz() * 1000;

    print_interval ("sync", &timeSync);
    print_interval ("header", &timeHeader);
    print_interval ("decode", &timeDecode);
    print_interval ("input", &timeInput);
    print_interval ("output", &timeOutput);
    print_interval ("sleep", &timeSleep);
    print_interval ("sleep10", &timeSleep10);
    print_interval ("overall", &timeTotal);
    totms = timeTotal.total / ticks_per_msec;
    decms = timeDecode.total / ticks_per_msec;
    sleepms = (timeSleep.total + timeSleep10.total) / ticks_per_msec;
    inms = timeInput.total / ticks_per_msec;
    outms = timeOutput.total / ticks_per_msec;

    if (totms) {
	decpct = (decms * 100) / totms;
	sleeppct = (sleepms * 100) / totms;
	inpct = (inms * 100) / totms;
	outpct = (outms * 100) / totms;
	loadpct = 100 - sleeppct - inpct - outpct;
    }

    epics_printk("Total %2d.%03ds Decode %2d.%03ds Sleep %2d.%03ds In %2d.%03ds Out %2d.%03ds\n",
	    totms / 1000, totms % 1000, decms / 1000, decms % 1000,
	    sleepms / 1000, sleepms % 1000, inms / 1000, inms % 1000,
	    outms / 1000, outms % 1000);
    epics_printk("Load     %3d%% Decode    %3d%% Sleep    %3d%% In    %3d%% Out    %3d%%\n",
	    loadpct, decpct, sleeppct, inpct, outpct);
}

/*
 * 	The main decoder loop (take mp3, find sync, get hdr, decode, put pcm, ...)
 */
static int mp3_thread(void* context)
{
	u32 samples_per_channel = 0;
	u32 offset = 0;
	u32 retval = MP3DEC_NO_ERR;
	u32 tot_decoded = 0;
	u32 tot_err = 0;

	INTERVAL_INIT(timeSync);
	INTERVAL_INIT(timeHeader);
	INTERVAL_INIT(timeDecode);
	INTERVAL_INIT(timeInput);
	INTERVAL_INIT(timeOutput);
	INTERVAL_INIT(timeSleep);
	INTERVAL_INIT(timeSleep10);
	INTERVAL_INIT(timeTotal);

	(void)context; /* not used */

	TRACE(1, "starting decoder decodebuf1=%p decodebuf2=%p\n", mp3_buff->out[0].data, mp3_buff->out[1].data);

	decode_state = e7b_mp3_processing; /* default state only changes while blocked */
	retval = mp3decInit();

	if (retval != MP3DEC_NO_ERR) {
	    epics_printk("decoder error=%d, initializing decoder\n", (int)retval);
	    retval = MP3DEC_NO_ERR;
	}

	TRACE(1, "mp3_thread: decoder initialized\n");
restart:
        if (mp3_reset_request)
        {
           int i;
           TRACE(2, "reseting the decoder\n");
           mp3_reset_request = 0;

           for (i = 0; i < E7B_MP3_NBUF; i++)
              mp3_buff->out[i].count = 0;

           out_write_idx = 0;
           e7b_cbuf_reset(&mp3_input);
           mp3_reset_complete = 1;
           wake_up_interruptible(&w84decoder);  /* wakeup anyone waiting for reset to complete */
        }

	mp3decReset();
	TRACE(1, "mp3_thread: decoder reset\n");

	if (kthread_should_stop()) goto finished;

	if (retval != MP3DEC_NO_ERR) {
		tot_err++;
		TRACE(2, "decoder error=%d, resetting decoder\n", (int)retval);
		epics_printk("decoder error=%d, resetting decoder\n", (int)retval);
	}

	while (!kthread_should_stop()) {
                INTERVAL_START(timeTotal);

                if (mp3_reset_request)
                   goto restart;

		/*	Get to sync already */
                INTERVAL_START(timeSync);
		retval=mp3decSync(&offset);
                INTERVAL_STOP(timeSync);
		if ((retval) != MP3DEC_NO_ERR) goto restart;
		if (offset>2) {
			TRACE(2, "found sync at %u\n", offset);
		}
                TRACE(4, "found sync at %u\n", offset);

		/*	Make sure previous output was consumed	*/
		if (mp3_buff->out[out_write_idx].count != 0) {
			// block for output available
			TRACE(4, "blocking for %d w84user\n", out_write_idx);
			decode_state = e7b_mp3_blocked_on_output;
                        INTERVAL_START(timeOutput);
			if ((retval = wait_event_interruptible(w84user, (mp3_reset_request || kthread_should_stop() || (mp3_buff->out[out_write_idx].count == 0))) < 0)) {
				TRACE(1, "output wait interrupted\n");
				decode_state = e7b_mp3_processing;
				goto finished;
			}
                        INTERVAL_STOP(timeOutput);
			decode_state = e7b_mp3_processing;
                        if (mp3_reset_request) goto restart;
			if (kthread_should_stop()) goto finished;
		}
		TRACE(4,"out[%d] ready for decoder\n", out_write_idx);

		/*	Get output (header and PCM) */
                INTERVAL_START(timeHeader);
		retval=mp3decGetHeaderInfo(&mp3_buff->out[out_write_idx].hdr);
                INTERVAL_STOP(timeHeader);
		if ((retval) != MP3DEC_NO_ERR) goto restart;
		TRACE(2, "hdr: samplesPerCh=%lu numChans=%lu bitsRequired=%lu\n",
				mp3_buff->out[out_write_idx].hdr.samplesPerCh,
				mp3_buff->out[out_write_idx].hdr.numChans,
				mp3_buff->out[out_write_idx].hdr.bitsRequired);

                INTERVAL_START(timeDecode);
		retval=mp3decDecode(&samples_per_channel, mp3_buff->out[out_write_idx].data);
                INTERVAL_STOP(timeDecode);
		if ((retval) == MP3DEC_FRAME_DISCARDED) continue;
		if ((retval) != MP3DEC_NO_ERR) goto restart;
		TRACE(3, "decoded %u samples per channel\n", samples_per_channel);

		/*	update info */
		consumed[out_write_idx] = 0;
		mp3_buff->out[out_write_idx].count = (samples_per_channel * mp3_buff->out[out_write_idx].hdr.numChans);
		// debug
		tot_decoded += mp3_buff->out[out_write_idx].count;
		TRACE(3, "out[%d] --> %lu bytes written\n", out_write_idx, mp3_buff->out[out_write_idx].count * 2);
		TRACE(3, "wakeup w84decoder [%d]\n", out_write_idx);

		out_write_idx = (out_write_idx+1) % E7B_MP3_NBUF;
		wake_up_interruptible(&w84decoder);  /* wakeup anyone waiting for pcm to be produced */
                INTERVAL_STOP(timeTotal);

		schedule();
	}

finished:
	TRACE(1, "decoding done (total=%u decode_errors=%d)\n", tot_decoded, tot_err);
	epics_printk("decoding done (total=%u decode_errors=%d)\n", tot_decoded, tot_err);
	print_stats();
        epics_printk("mp3: delay %d\n", e7b_mp3_decode_delay);
	mp3decDeInit();

	TRACE(1, "decoder deinitialized\n");
	__set_current_state(TASK_RUNNING);

	return 0;
}

static int start_decoder(void)
{
	TRACE(2, "starting decoding\n");

	/*
	 * 	Some sanity checks (for debug mostly)
	 */
	if (decode_task) {
		ERRMSG("mp3 decode thread was still flagged as running!\n");
	}
	if (mp3_bufsize) {
		ERRMSG("mp3 buffer seemed already allocated!\n");
	}

	if (e7b_select_firmware(PNX0106_EPICS_MP3_FWID) < 0)
	    return -ENXIO;

	/*
	 * 	First Allocate buffer for decoder
	 */
	mp3_bufsize = PAGE_ALIGN(sizeof(e7b_mp3_buffer)); /* size must be multiple of PAGE_SIZE for mmap */

	TRACE(2, "alloc %d(%d)\n", mp3_bufsize, sizeof(e7b_mp3_buffer));

	mp3_buff = epics_alloc_coherent(NULL, mp3_bufsize, &mp3_buff_bus, 0);
	if (!mp3_buff)
		return -ENOMEM;

	/*
	 * 	Init everything
	 */
	memset(mp3_buff, 0, mp3_bufsize);
	memset(consumed, 0, sizeof(consumed));
	((unsigned int *)mp3_buff)[0] = 0xDEADBEEF;
	((unsigned int *)mp3_buff)[4096/4] = 0xDEADBEE2;
	((unsigned int *)mp3_buff)[8192/4] = 0xDEADBEE3;

	mp3_input.size = sizeof(mp3_buff->inp.data);
	mp3_input.data = mp3_buff->inp.data;
	e7b_cbuf_reset(&mp3_input);
	out_read_idx = 0;
	out_write_idx = 0;
	tot_size = 0;

	printk("start_decoder: data structs reset\n");

	init_waitqueue_head(&w84decoder);
	init_waitqueue_head(&w84user);

	printk("start_decoder: queues initialized\n");

	/*
	 * 	Start Decode Thread
	 */
	decode_state = e7b_mp3_processing; /* default state until blocked */

	/*
	 * 	Create a kernel thread for decoding
	 */
	decode_task = kthread_create(mp3_thread, 0, "mp3_dec");
	printk("start_decoder: created thread %p\n", decode_task);
	if (!decode_task)
		return -ENOMEM;

	wake_up_process(decode_task);
	printk("start_decoder: thread wake up done\n");

	return 0;
}

static void stop_decoder(void)
{
	TRACE(2, "stopping decoding\n");

	if (!decode_task) {
		WARN("mp3 decode thread already flagged as stopped\n");
		return;
	}
	wake_up_interruptible(&w84decoder);  /* wakup decoder */
	wake_up_interruptible(&w84user);  /* wakup anyone waiting for output */
	kthread_stop(decode_task);
	decode_task = 0;

	epics_free_coherent(NULL, mp3_bufsize, mp3_buff, mp3_buff_bus);
	TRACE(2, "total read=%d\n", tot_size);
	mp3_bufsize = 0;
	mp3_buff = 0;
	mp3_buff_bus = 0;
}

/*	returns nr of bytes available or -1 if decoding should stop asap */
int e7b_mp3_wait_for_input(int nbytes)
{
	int retval;

	while (!mp3_reset_request && !kthread_should_stop() && (mp3_input.count < nbytes)) {
		TRACE(4, "waiting for %d bytes (currently at %d)\n", nbytes, mp3_input.count);
		decode_state = e7b_mp3_blocked_on_input; /* default state until blocked */
		INTERVAL_START(timeInput);
		if ((retval = wait_event_interruptible(w84user, (mp3_reset_request || kthread_should_stop() || (mp3_input.count >= nbytes))) < 0)) {
			TRACE(1, "input wait interrupted %d\n", retval);
			decode_state = e7b_mp3_processing; /* default state until blocked */
			return retval;
		}
		INTERVAL_STOP(timeInput);
		TRACE(4, "got in\n");
		decode_state = e7b_mp3_processing; /* default state until blocked */
	}
	retval = ((mp3_reset_request || kthread_should_stop()) ? -1 : mp3_input.count);
	return retval;
}

void
xmemcpy(void *dest, void *src, int bytes)
{
    unsigned int *d = (unsigned int *)dest;
    unsigned int *s = (unsigned int *)src;
    unsigned int *e = (unsigned int *)(&((char *)src)[bytes]);

    TRACE(5, "xmemcpy: dest %p src %p bytes %d end %p\n", dest, src, bytes, e);

    while (s < e)
      *d++ = *s++;

    // flush the temp registers
    switch (bytes % 12)
    {
    case 0:
       break;
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
    case 9:
    case 10:
    case 11:
       epics_printk("xmemcpy: odd number of bytes in xmemcpy %d (%d)\n", bytes, bytes % 12);
       break;
    case 7:
       *d = 0;
       epics_printk("xmemcpy: odd number of bytes in xmemcpy %d (%d)\n", bytes, bytes % 12);
       break;
    case 4:
    case 8:
       *d = 0;
       epics_printk("xmemcpy: odd number of words in xmemcpy %d (%d)\n", bytes, bytes % 12);
       break;
    }
}

int e7b_mp3_copy_input(int nbytes, void *pInBufferEpics)
{
	int copy_size 		= (mp3_input.count > nbytes) ? nbytes : mp3_input.count;
	int at_end_avail 	= mp3_input.size - mp3_input.rdpos;
	int at_end_copy		= (at_end_avail > copy_size) ? copy_size : at_end_avail;
	int at_front_copy 	= copy_size - at_end_copy;
	unsigned long flags;
	unsigned int word0;
	unsigned int word1;
	unsigned int word2;
	unsigned int word3;
	unsigned int word4;
	unsigned int word5;
//	pInBufferEpics = (void *)(((((unsigned int)pInBufferEpics - 0xFC000000) / 4) * 3) + 0xFC000000);

	if (!copy_size) {
		ERRMSG("e7b_mp3_copy_input has no data: count=%d nbytes=%d\n", mp3_input.count, nbytes);
		return 0;
	}

	if (at_end_copy > 0) {
		e7b_change_endian( CBUF_RDPTR(mp3_input), at_end_copy );
	}
	if (at_front_copy > 0) {
		e7b_change_endian( mp3_input.data, at_front_copy );
	}

#define GETWORD(n)							\
	if (at_end_copy > (n * 4))					\
	    word ## n = ((unsigned int *)CBUF_RDPTR(mp3_input))[n];	\
	else								\
	    word ## n = ((unsigned int *)mp3_input.data)[n];

	 GETWORD(0);
	 GETWORD(1);
	 GETWORD(2);
	 GETWORD(3);
	 GETWORD(4);
	 GETWORD(5);

	printk("epics: mp3_copy_input %08x %08x %08x\n", word0, word1, word2);
	printk("epics: mp3_copy_input %08x %08x %08x\n", word3, word4, word5);

	/* copy it to the epics in lossless transfer mode (3 arm words to 4 epics words) */
	local_irq_save(flags);	/* transfer mode is a global setting: may not switch context */
	e7b_transfer_3x32_4x24();
	if (at_end_copy > 0) {
		xmemcpy( (char*)pInBufferEpics, (char*)CBUF_RDPTR(mp3_input), at_end_copy );
	}
	if (at_front_copy > 0) {
		xmemcpy( (char*)pInBufferEpics+at_end_copy, (char*)mp3_input.data, at_front_copy );
	}
	e7b_transfer_32_24();	/* back to default mode */
	local_irq_restore(flags);

	printk("epics: mp3_copy_input: end %d src %p dst %p\n", at_end_copy, CBUF_RDPTR(mp3_input), pInBufferEpics);
	printk("epics: mp3_copy_input: front %d src %p dst %p\n", at_front_copy, mp3_input.data, (char*)pInBufferEpics+at_end_copy);

	/* update circular buffer bookkeeping */
	spin_lock(&mp3_lock);
	CBUF_DEC(mp3_input, copy_size);
	spin_unlock(&mp3_lock);

	TRACE(4, "wakeup w84decoder\n");
	wake_up_interruptible(&w84decoder);  /* wakup anyone waiting for decoder to consume */

	return copy_size;
}


static int mp3_open (struct inode *inode, struct file *fp)
{
	int retval = 0;

	TRACE(1, "%s\n", __FUNCTION__);

	if (pnx0106_epics_decoder_claim(PNX0106_EPICS_MP3_FWID) < 0)
		return -EBUSY;

	mp3_open_mode |= (fp->f_mode & (FMODE_WRITE|FMODE_READ));

	/* we can be opened for reading and writing simultaneous... */
	if (mp3_open_mode != 0 && !decode_task) {
		retval = start_decoder();

		if (retval != 0)
		    pnx0106_epics_decoder_release();
	}

	//fp->private_data = dev;
	return retval;
}

static int mp3_release (struct inode *inode, struct file *fp)
{
	mp3_open_mode &= ~(fp->f_mode & (FMODE_WRITE|FMODE_READ));

	TRACE(1, "%s mode=%x\n", __FUNCTION__, mp3_open_mode);

	if (mp3_open_mode == 0) {
		stop_decoder();
	}

	pnx0106_epics_decoder_release();
	return 0;
}

static int mp3_ioctl (struct inode *inode, struct file *fp, unsigned int command, unsigned long arg)
{
	int retval = 0;

	//struct mp3_device *dev = fp->private_data;
	TRACE(2, "%s 0x%x\n", __FUNCTION__, command);

	switch (command) {
		case E7B_MP3_INC_INPUT:
			{
				int increment;
				TRACE(2, "%s E7B_MP3_INC_INPUT\n", __FUNCTION__);
				if (copy_from_user(&increment, (void*)arg, sizeof(increment)))
					return -EFAULT;

				if (increment < 0 || increment > CBUF_AVAIL(mp3_input)) {
					ERRMSG("E7B_MP3_INC_INPUT(%d) invalid increment (must be in [0..%lu[)\n", increment, CBUF_AVAIL(mp3_input));
					return -EINVAL;
				}
				TRACE(2, "INC_INPUT %d + %d\n", mp3_input.count, increment);
				spin_lock(&mp3_lock);
				CBUF_INC(mp3_input, increment);
				spin_unlock(&mp3_lock);

				wake_up_interruptible(&w84user);  /* wakeup anyone waiting for input */
			}
			break;
		case E7B_MP3_USED_OUTPUT:
			{
				int buf_idx;
				TRACE(2, "%s E7B_MP3_USED_OUTPUT\n", __FUNCTION__);
				if (copy_from_user(&buf_idx, (void*)arg, sizeof(buf_idx)))
					return -EFAULT;

				if (buf_idx != out_read_idx) {
					ERRMSG("E7B_MP3_USED_OUTPUT(%d): expected buf %d\n", buf_idx, out_read_idx);
					return -EINVAL;
				}

				if (mp3_buff->out[buf_idx].count == 0) {
					WARN("E7B_MP3_USED_OUTPUT(%d) was already empty\n", buf_idx);
				}

				mp3_buff->out[buf_idx].count = 0;
				out_read_idx = (out_read_idx+1) % E7B_MP3_NBUF;
				TRACE(3, "USED_OUTPUT %d\n", buf_idx);
				wake_up_interruptible(&w84user);  /* wakeup anyone waiting for output */
			}
			break;
		case E7B_MP3_STATUS:
			{
				e7b_mp3_status status;
				TRACE(2, "%s E7B_MP3_STATUS\n", __FUNCTION__);

				/* block while input full and output empty (user must wait for decoder) */
				if ((retval = wait_event_interruptible(w84decoder, ((mp3_buff->out[out_read_idx].count > 0) || !CBUF_FULL(mp3_input))))) {
					TRACE(2, "interrupted E7B_MP3_STATUS %d\n", retval);
					return retval;
				}

				status.decoder = decode_state;

				spin_lock(&mp3_lock);
				status.input.available = mp3_input.size - mp3_input.count;
				status.input.wrpos = mp3_input.wrpos;
				spin_unlock(&mp3_lock);

				status.output = out_read_idx;
				TRACE(4,"user2do out[%d]=%lu in=%lu\n", out_read_idx, mp3_buff->out[out_read_idx].count, status.input.available);

				if (copy_to_user((void *)arg, &status, sizeof(status)))
					return -EFAULT;
			}
			break;
		case E7B_MP3_DECODE_DELAY:
			{
				int delay;
				TRACE(2, "%s E7B_MP3_DECODE_DELAY\n", __FUNCTION__);
				if (copy_from_user(&delay, (void*)arg, sizeof(delay)))
					return -EFAULT;

				if (delay < 0 || delay > 1000) {
					ERRMSG("E7B_MP3_DECODE_DELAY not in range [0,1000]\n");
					return -EINVAL;
				}
				e7b_mp3_decode_delay = delay;
			}
			break;
		case E7B_MP3_RESET:
			{
				TRACE(2, "%s E7B_MP3_RESET\n", __FUNCTION__);
				out_read_idx = 0;
				mp3_reset_complete = 0;
				mp3_reset_request = 1;
				wake_up_interruptible(&w84user);  /* wakeup anyone waiting for input */

				if ((retval = wait_event_interruptible(w84decoder, mp3_reset_complete)) < 0) {
					TRACE(2, "interrupted E7B_MP3_RESET %d\n", retval);
					return retval;
				}
            TRACE(2, "E7B_MP3_RESET complete\n");
			}
			break;
	}
	return retval;
}

static ssize_t mp3_read(struct file *fp, char __user *buf, size_t count, loff_t *ppos)
{
    int      	  retval = 0;
    ssize_t       bytes_copied = 0;
    unsigned int  copy_size    = 0;
	s16		 *outptr;

	TRACE(2, "%s %d\n", __FUNCTION__, count);

    if (*ppos != fp->f_pos)		return -ESPIPE;

    while (count > 0) {
        while (out_avail(out_read_idx) == 0) { /* buf empty */

            if (fp->f_flags & O_NONBLOCK) {
                return bytes_copied ? bytes_copied : -EAGAIN;
            }

			/* block: only when not enough data available yet to return */
			if (bytes_copied > 0) {
				/* we can return already... */
				TRACE(2, "returning partial filled buffer %d\n", bytes_copied);
				tot_size += bytes_copied;
				return bytes_copied;
			}

			TRACE(3,"waiting for pcm[%d]...\n", out_read_idx);
            /* we have to wait for epics to produce some samples */
            if ((retval = wait_event_interruptible(w84decoder, out_avail(out_read_idx)!=0)) < 0) {
				TRACE(2, "interrupted read %d\n", retval);
				return retval;
			}

        }

		copy_size = sizeof(s16) * out_avail(out_read_idx);
		outptr = mp3_buff->out[out_read_idx].data + consumed[out_read_idx];
		if (count < copy_size) {
			copy_size = count;
		}

		__copy_to_user(buf, outptr, copy_size);
		consumed[out_read_idx] += (copy_size/sizeof(s16));  /* copy_size is bytes, consumed is samples */
		TRACE(4, "buf %d: copy %d bytes to user. consumed = %d\n", out_read_idx, copy_size, consumed[out_read_idx]);
		if (consumed[out_read_idx] >= mp3_buff->out[out_read_idx].count) {
			mp3_buff->out[out_read_idx].count = 0;
			consumed[out_read_idx] = 0;
			out_read_idx = ((out_read_idx+1) % E7B_MP3_NBUF);
			wake_up_interruptible(&w84user);  /* first wakup anyone waiting for input */
		}

		count  -= copy_size;
		buf    += copy_size;
		bytes_copied += copy_size;
		TRACE(7, "	count=%d bc=%d\n", count, bytes_copied);
    }

	tot_size += bytes_copied;
	TRACE(4, "copied %d bytes\n", bytes_copied);

    return bytes_copied;
}

static ssize_t mp3_write(struct file *fp, const char __user *buf, size_t count, loff_t *ppos)
{
    int      retval = 0;
    ssize_t      bytes_copied = 0;
    unsigned int copy_size    = 0;
	//unsigned long flags;

	TRACE(2, "%s %d\n", __FUNCTION__, count);

    if (*ppos != fp->f_pos) return -ESPIPE;

	/* disable interrupt and prevent other use of this stream */
	//spin_lock_irqsave(&mp3_lock, flags);

    while (count > 0) {
        while (CBUF_FULL(mp3_input)) {
			/* release the lock prior to sleeping (or returning) */
			//spin_unlock_irqrestore(&mp3_lock, flags);

            if (fp->f_flags & O_NONBLOCK) {
                return bytes_copied ? bytes_copied : -EAGAIN;
            }

            /* we have to wait for decoder to process some samples */
			TRACE(4, "buf full (%d); wakeup w84user & w84decoder\n", mp3_input.count);
			wake_up_interruptible(&w84user);  /* first wakup anyone waiting for input */
            if ((retval = wait_event_interruptible(w84decoder, !CBUF_FULL(mp3_input))) < 0) {
				return retval;
			}

			//spin_lock_irqsave(&mp3_lock, flags);
        }
		/*	at this point IRQ's should be disabled, and no other thread is accessing the driver structures */

		copy_size = e7b_cbuf_write(&mp3_input, count, buf);
		TRACE(4, "input += %d bytes\n", copy_size);

		count  			-= copy_size;
		buf 			+= copy_size;
		bytes_copied 	+= copy_size;
		/*TRACE(7,"	count=%d\n", count);  */
    }

	//spin_unlock_irqrestore(&mp3_lock, flags);

	/* is the output buffer full enough to start? */
	if (mp3_input.count >= E7B_MP3_MIN_FRAME_BYTES) {
		TRACE(4, "wakeup w84user (%d)\n", mp3_input.count);
		wake_up_interruptible(&w84user);  /* first wakup anyone waiting for input */
	}

    return bytes_copied;
}

static int mp3_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret;

	if (vma->vm_pgoff != 0) {
		ERRMSG("mp3_mmap: pgoff must be 0 (%lu)\n", vma->vm_pgoff);
		return -EINVAL;
	}

	if ((vma->vm_end - vma->vm_start) != mp3_bufsize) {
		ERRMSG("mp3_mmap: vma size mismatch %p %p %d\n", (void*)vma->vm_end, (void*)vma->vm_start, mp3_bufsize);
		return -EINVAL;
	}

	if (!mp3_buff) {
		ERRMSG("mp3_mmap: mem not allocated\n");
		return -ENOMEM;
	}

//	vma->vm_flags |= VM_LOCKED;

	ret = remap_page_range(vma->vm_start, mp3_buff_bus, mp3_bufsize, PAGE_SHARED);
//	ret = remap_page_range(vma->vm_start, mp3_buff_bus, mp3_bufsize,
//		vma->vm_page_prot);
//	ret = remap_pfn_range(vma, vma->vm_start, mp3_buff_bus >> PAGE_SHIFT, mp3_bufsize, vma->vm_page_prot);
	if (ret) {
		ERRMSG("mp3_mmap: remap failed\n");
		return ret;
	}

	//vma->vm_ops = &mp3_vmops;
	return 0;
}

struct file_operations e7b_mp3_fops = {
	.owner	 = THIS_MODULE,
	.open    = mp3_open,
	.read    = mp3_read,
	.write   = mp3_write,
	.ioctl   = mp3_ioctl,
	.release = mp3_release,
	.mmap	 = mp3_mmap,
};

