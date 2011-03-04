/*
 * linux/arch/arm/mach-ssa/audio.c
 *
 * Copyright (C) 2003 Andre McCurdy, Ranjit Deshpande
 * Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/major.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/mm.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/arch/hardware.h>
#include <asm/arch/tsc.h>


#undef AUDIO_DEBUG
#ifdef AUDIO_DEBUG
static int dbg_level = 3;
#define DBG(level, args...)	if (level <= dbg_level) printk(args)
#else
#define DBG(level, args...)
#endif

/****************************************************************************
****************************************************************************/

#define DEVICE_NAME     "SAA7752 Audio"

/*
   Assume channel 1 (we don't use channel 2).
*/

#define ATU_FIFO_LT     ((volatile unsigned int *) (IO_ADDRESS_ATU_FIFO_BASE + 0x00))
#define ATU_FIFO_RT     ((volatile unsigned int *) (IO_ADDRESS_ATU_FIFO_BASE + 0x40))


/****************************************************************************
****************************************************************************/

#define AUDIO_BUFFER_SZ	(512 * 1024)
#define AUDIO_FRAMES	(AUDIO_BUFFER_SZ / sizeof(u8))

//#define AUDIO_FIFO_SZ	128


static struct audio_state
{
	struct semaphore	sem;		/* Mutex for write operations */
	int			count;		/* Allow only one open(2) */
	unsigned int 		audio_sample_rate;
	wait_queue_head_t 	wq;
	int 			dev_audio;	/* Handle returned by 
						 * soundcore
						 */
	int			channels;	/* Number of channels */
	int			sample_size;	/* 8-bit or 16-bit */


	/* head is an index into the 'frame' array of frame buffers.
	 * It points to the first free frame buffer and is incremented when 
	 * data is written from user space.
	 *
	 * tail is also an index into the 'frame' array of frame buffers.
	 * It points to the frame buffer which still contains unconsumed 
	 * audio data. It is incremented by the audio ISR when all data in a 
	 * frame has been ouput.
	 *
	 * head and tail must be maintained modulo AUDIO_FRAMES.
	 * 
	 * If (head == tail) there is no data available.
	 */

	u8 *			buf;		/* User data */
	int			head;
	int			tail;

	int			first_write;
	int			use_sram;
} audio_state;



/****************************************************************************
****************************************************************************/

typedef struct
{
   volatile unsigned int ClockSelection;        /* RW 0x00 */
   volatile unsigned int DSPPLL;                /* -- 0x04 (not present in SAA7752) */
   volatile unsigned int AudioPLL;              /* RW 0x08 */
   volatile unsigned int MasterPLL;             /* RW 0x0C */
} PMU_Clock_Registers_t;

#define PMU_Clock_Registers ((PMU_Clock_Registers_t *) IO_ADDRESS_CLOCK_CTRL_BASE)


typedef struct
{
   volatile unsigned int dummy_0[2];
   volatile unsigned int bypass;                /* RW 0x08 */
   volatile unsigned int dummy_1[5];
   volatile unsigned int ctrl_1;                /* RW 0x20 */
   volatile unsigned int ctrl_2;                /* RW 0x24 */
   volatile unsigned int settings;              /* RW 0x28 */
} SDAC_Registers_t;

#define SDAC_Registers ((SDAC_Registers_t *) IO_ADDRESS_SDAC_BASE)


typedef struct
{
   volatile unsigned int ETU_int_status;        /* RW 0x00 */
   volatile unsigned int ETU_int_set;           /* RW 0x04 */
   volatile unsigned int ETU_int_clear;         /* RW 0x08 */
   volatile unsigned int dummy_0[22];
   volatile unsigned int ATU_int_enable;        /* RW 0x64 */
   volatile unsigned int ATU_config;            /* RW 0x68 */
   volatile unsigned int dummy_1;
   volatile unsigned int ATU_ch1_wr_status;     /* RW 0x70 */
   volatile unsigned int ATU_ch1_rd_status;     /* RW 0x74 */
   volatile unsigned int ATU_ch2_wr_status;     /* RW 0x78 */
   volatile unsigned int ATU_ch2_rd_status;     /* RW 0x7C */
} ATU_Registers_t;

#define ATU_Registers ((ATU_Registers_t *) IO_ADDRESS_ATU_BASE)


/****************************************************************************
****************************************************************************/

/*
   Audio PLL settings: Fout == 32768000 Hz with 10.0 MHz crystal
   Used for 32, 16 and 8 kHz audio sampling frequencies.
   Actual value is 32767857 Hz (ie error = -143, or 4 ppm too slow).
   Fcco = 524 MHz
   Fin / (Pre-Divider + 1) = 714 kHz (ie safely above 100 kHz limit).
*/

#define AUDIO_PLL_REG_32768                                     \
                                                                \
   ((  0 << 31) |  /* DIRECT Out bit   (default ==   0) */      \
    (  0 << 30) |  /* PD         bit   (default ==   0) */      \
    (  0 << 26) |  /* DIRECT In  bit   (default ==   0) */      \
    (  0 << 25) |  /* BYPASS     bit   (default ==   0) */      \
    (  7 << 20) |  /* Post-Divider     (default ==   3) */      \
    (366 <<  9) |  /* Feedback-Divider (default ==  63) */      \
    ( 13 <<  1) |  /* Pre-Divider      (default ==   2) */      \
    (  1 <<  0))   /* CLK_EN  bit      (default ==   1) */


/*
   Audio PLL settings: Fout == 45158400 Hz with 10.0 MHz crystal
   Used for 44.1, 22.05 and 11.025 kHz audio sampling frequencies.
   Actual value is 45158730 Hz (ie error = +330, or 7 ppm too fast).
   Fcco = 542 MHz
   Fin / (Pre-Divider + 1) = 476 kHz (ie safely above 100 kHz limit).
*/

#define AUDIO_PLL_REG_45158                                     \
                                                                \
   ((  0 << 31) |  /* DIRECT Out bit   (default ==   0) */      \
    (  0 << 30) |  /* PD         bit   (default ==   0) */      \
    (  0 << 26) |  /* DIRECT In  bit   (default ==   0) */      \
    (  0 << 25) |  /* BYPASS     bit   (default ==   0) */      \
    (  5 << 20) |  /* Post-Divider     (default ==   3) */      \
    (568 <<  9) |  /* Feedback-Divider (default ==  63) */      \
    ( 20 <<  1) |  /* Pre-Divider      (default ==   2) */      \
    (  1 <<  0))   /* CLK_EN  bit      (default ==   1) */


/*
   Audio PLL settings: Fout == 49152000 Hz with 10.0 MHz crystal
   Used for 48, 24 and 12 kHz audio sampling frequencies.
   Actual value is 49151515 Hz (ie error = -485, or 10 ppm too slow).
   Fcco = 492 MHz
   Fin / (Pre-Divider + 1) = 303 kHz (ie safely above 100 kHz limit).
*/

#define AUDIO_PLL_REG_49152                                     \
                                                                \
   ((  0 << 31) |  /* DIRECT Out bit   (default ==   0) */      \
    (  0 << 30) |  /* PD         bit   (default ==   0) */      \
    (  0 << 26) |  /* DIRECT In  bit   (default ==   0) */      \
    (  0 << 25) |  /* BYPASS     bit   (default ==   0) */      \
    (  4 << 20) |  /* Post-Divider     (default ==   3) */      \
    (810 <<  9) |  /* Feedback-Divider (default ==  63) */      \
    ( 32 <<  1) |  /* Pre-Divider      (default ==   2) */      \
    (  1 <<  0))   /* CLK_EN  bit      (default ==   1) */


/****************************************************************************
****************************************************************************/

/*
   definitions for the SDAC bypass control register...
*/

enum
{
   DIO_BYPASS_DSP = 0x0001,

   IIS1toIIS1out  = 0x0000,
   IIS2toIIS1out  = 0x0002,
   ATU1toIIS1out  = 0x0004,
   ATU2toIIS1out  = 0x0006,

   IIS1toIIS2out  = 0x0000,
   IIS2toIIS2out  = 0x0010,
   ATU1toIIS2out  = 0x0020,
   ATU2toIIS2out  = 0x0030,

   IIS1toATU1out  = 0x0000,
   IIS2toATU1out  = 0x0080,
   ATU1toATU1out  = 0x0100,
   ATU2toATU1out  = 0x0180,

   IIS1toATU2out  = 0x0000,
   IIS2toATU2out  = 0x0400,
   ATU1toATU2out  = 0x0800,
   ATU2toATU2out  = 0x0C00
};


/*
   definitions for the ATU config register...
*/

#define ATU_WR_CH1L_EN         0x00000001
#define ATU_WR_CH1R_EN         0x00000002
#define ATU_WR_CH1_LENGTH      0x00000004
#define ATU_WR_CH2L_EN         0x00000008
#define ATU_WR_CH2R_EN         0x00000010
#define ATU_WR_CH2_LENGTH      0x00000020
#define ATU_RD_CH1L_EN         0x00000040
#define ATU_RD_CH1R_EN         0x00000080
#define ATU_RD_CH1_LENGTH      0x00000100
#define ATU_RD_CH2L_EN         0x00000200
#define ATU_RD_CH2R_EN         0x00000400
#define ATU_RD_CH2_LENGTH      0x00000800
#define ATU_IFLAG_EN           0x00001000


/*
   definitions for the ATU read and write status registers...
*/

#define ATU_STATUS_LEFT_FULL    0x01
#define ATU_STATUS_LEFT_HALF    0x02
#define ATU_STATUS_LEFT_EMPTY   0x04
#define ATU_STATUS_RIGHT_FULL   0x08
#define ATU_STATUS_RIGHT_HALF   0x10
#define ATU_STATUS_RIGHT_EMPTY  0x20


#define ETU_INT_WR_CH1L_FULL    (1 <<  7)
#define ETU_INT_WR_CH1L_HALF    (1 <<  8)   
#define ETU_INT_WR_CH1L_EMPTY   (1 <<  9)
#define ETU_INT_WR_CH1R_FULL    (1 << 10)
#define ETU_INT_WR_CH1R_HALF    (1 << 11)
#define ETU_INT_WR_CH1R_EMPTY   (1 << 12)
#define ETU_INT_WR_CH2L_FULL    (1 << 13)
#define ETU_INT_WR_CH2L_10      (1 << 14)
#define ETU_INT_WR_CH2L_EMPTY   (1 << 15)
#define ETU_INT_WR_CH2R_FULL    (1 << 16)
#define ETU_INT_WR_CH2R_10      (1 << 17)
#define ETU_INT_WR_CH2R_EMPTY   (1 << 18)

#define ETU_INT_RD_CH1L_FULL    (1 << 19)
#define ETU_INT_RD_CH1L_HALF    (1 << 20)
#define ETU_INT_RD_CH1L_EMPTY   (1 << 21)
#define ETU_INT_RD_CH1R_FULL    (1 << 22)
#define ETU_INT_RD_CH1R_HALF    (1 << 23)
#define ETU_INT_RD_CH1R_EMPTY   (1 << 24)
#define ETU_INT_RD_CH2L_FULL    (1 << 25)
#define ETU_INT_RD_CH2L_90      (1 << 26)
#define ETU_INT_RD_CH2L_EMPTY   (1 << 27)
#define ETU_INT_RD_CH2R_FULL    (1 << 28)
#define ETU_INT_RD_CH2R_90      (1 << 29)
#define ETU_INT_RD_CH2R_EMPTY   (1 << 30)


#define ATU_IRQ_WR_CH1L_HALF    (1 <<  1)
#define ATU_IRQ_WR_CH1L_EMPTY   (1 <<  2)


#ifdef AUDIO_DEBUG
static struct ioctl_str_t {
	unsigned int    cmd;
	const char     *str;
} ioctl_str[] = {
	{SNDCTL_DSP_RESET, "SNDCTL_DSP_RESET"},
	{SNDCTL_DSP_SYNC, "SNDCTL_DSP_SYNC"},
	{SNDCTL_DSP_SPEED, "SNDCTL_DSP_SPEED"},
	{SNDCTL_DSP_STEREO, "SNDCTL_DSP_STEREO"},
	{SNDCTL_DSP_GETBLKSIZE, "SNDCTL_DSP_GETBLKSIZE"},
	{SNDCTL_DSP_CHANNELS, "SNDCTL_DSP_CHANNELS"},
	{SOUND_PCM_WRITE_CHANNELS, "SOUND_PCM_WRITE_CHANNELS"},
	{SOUND_PCM_WRITE_FILTER, "SOUND_PCM_WRITE_FILTER"},
	{SNDCTL_DSP_POST, "SNDCTL_DSP_POST"},
	{SNDCTL_DSP_SUBDIVIDE, "SNDCTL_DSP_SUBDIVIDE"},
	{SNDCTL_DSP_SETFRAGMENT, "SNDCTL_DSP_SETFRAGMENT"},
	{SNDCTL_DSP_GETFMTS, "SNDCTL_DSP_GETFMTS"},
	{SNDCTL_DSP_SETFMT, "SNDCTL_DSP_SETFMT"},
	{SNDCTL_DSP_GETOSPACE, "SNDCTL_DSP_GETOSPACE"},
	{SNDCTL_DSP_GETISPACE, "SNDCTL_DSP_GETISPACE"},
	{SNDCTL_DSP_NONBLOCK, "SNDCTL_DSP_NONBLOCK"},
	{SNDCTL_DSP_GETCAPS, "SNDCTL_DSP_GETCAPS"},
	{SNDCTL_DSP_GETTRIGGER, "SNDCTL_DSP_GETTRIGGER"},
	{SNDCTL_DSP_SETTRIGGER, "SNDCTL_DSP_SETTRIGGER"},
	{SNDCTL_DSP_GETIPTR, "SNDCTL_DSP_GETIPTR"},
	{SNDCTL_DSP_GETOPTR, "SNDCTL_DSP_GETOPTR"},
	{SNDCTL_DSP_MAPINBUF, "SNDCTL_DSP_MAPINBUF"},
	{SNDCTL_DSP_MAPOUTBUF, "SNDCTL_DSP_MAPOUTBUF"},
	{SNDCTL_DSP_SETSYNCRO, "SNDCTL_DSP_SETSYNCRO"},
	{SNDCTL_DSP_SETDUPLEX, "SNDCTL_DSP_SETDUPLEX"},
	{SNDCTL_DSP_GETODELAY, "SNDCTL_DSP_GETODELAY"},
	{SNDCTL_DSP_GETCHANNELMASK, "SNDCTL_DSP_GETCHANNELMASK"},
	{SNDCTL_DSP_BIND_CHANNEL, "SNDCTL_DSP_BIND_CHANNEL"},
	{OSS_GETVERSION, "OSS_GETVERSION"},
	{SOUND_PCM_READ_RATE, "SOUND_PCM_READ_RATE"},
	{SOUND_PCM_READ_CHANNELS, "SOUND_PCM_READ_CHANNELS"},
	{SOUND_PCM_READ_BITS, "SOUND_PCM_READ_BITS"},
	{SOUND_PCM_READ_FILTER, "SOUND_PCM_READ_FILTER"}
};
#endif

/****************************************************************************
****************************************************************************/

static void set_audio_samplerate(int samplerate)
{
   static const struct
   {
      unsigned short samplerate;
      unsigned short mux_setting;
      unsigned int   pll_setting;
   }
   asr_tab[] =
   {
      { 48000, 0x01, AUDIO_PLL_REG_49152 },    /* 48000 kHz : 49.152 MHz / 1024 */
      { 44100, 0x01, AUDIO_PLL_REG_45158 },    /* 44100 kHz : 45.158 MHz / 1024 */
      { 32000, 0x01, AUDIO_PLL_REG_32768 },    /* 32000 kHz : 32.768 MHz / 1024 */
      { 24000, 0x02, AUDIO_PLL_REG_49152 },    /* 24000 kHz : 49.152 MHz / 2048 */
      { 22050, 0x02, AUDIO_PLL_REG_45158 },    /* 22050 kHz : 45.158 MHz / 2048 */
      { 16000, 0x02, AUDIO_PLL_REG_32768 },    /* 16000 kHz : 32.768 MHz / 2048 */
      { 12000, 0x03, AUDIO_PLL_REG_49152 },    /* 12000 kHz : 49.152 MHz / 4096 */
      { 11025, 0x03, AUDIO_PLL_REG_45158 },    /* 11025 kHz : 45.158 MHz / 4096 */
      {  8000, 0x03, AUDIO_PLL_REG_32768 }     /*  8000 kHz : 32.768 MHz / 4096 */
   };

	unsigned int i;
	unsigned int new_mux_setting = 0x03;	/* default (should always be 
						   over-ridden...) */
	unsigned int new_pll_setting = AUDIO_PLL_REG_32768; /* default, always
							     * override.
							     */
	unsigned long flags;
   
	for (i = 0; i < (sizeof(asr_tab)/sizeof(asr_tab[0])); i++) {
		if (samplerate >= asr_tab[i].samplerate) {
			audio_state.audio_sample_rate = asr_tab[i].samplerate;
			new_mux_setting = asr_tab[i].mux_setting;
			new_pll_setting = asr_tab[i].pll_setting;
			break;
		}
	}

	local_irq_save (flags);
	PMU_Clock_Registers->AudioPLL = new_pll_setting;
	PMU_Clock_Registers->ClockSelection = 
		((PMU_Clock_Registers->ClockSelection & 0x3FFFFFFF) | 
		 				(new_mux_setting << 30));
	local_irq_restore (flags);
}


static int frames_used(void)
{
	unsigned long flags;
	int used;

	local_irq_save(flags);
	used = (audio_state.tail - audio_state.head + AUDIO_FRAMES) % 
								AUDIO_FRAMES; 
	local_irq_restore(flags);
	return used;
}

static int frames_free(void)
{
	return AUDIO_FRAMES - frames_used() - 1;
}

static inline void fill_fifo(int used)
{
	register int head = audio_state.head;
	u8 *buf = audio_state.use_sram ? 
			(u8 *)(IO_ADDRESS_ISRAM_BASE + 16384) : audio_state.buf;
	
	while ((ATU_Registers->ATU_ch1_wr_status & 
			(ATU_STATUS_LEFT_FULL | ATU_STATUS_RIGHT_FULL)) == 0 &&
			used >= 2*sizeof(u16)) {
		*ATU_FIFO_LT = *(u16 *)&buf[head] << 16;
		head = (head + sizeof(u16)) % AUDIO_FRAMES;
		*ATU_FIFO_RT = *(u16 *)&buf[head] << 16;
		head = (head + sizeof(u16)) % AUDIO_FRAMES;
		used = (audio_state.tail - head + AUDIO_FRAMES) % AUDIO_FRAMES; 
	}
	audio_state.head = head;
}

static void audio_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	int used;

	used = (audio_state.tail - audio_state.head + AUDIO_FRAMES) % 
								AUDIO_FRAMES; 
	if (used >= 2 * sizeof(u16)) 
		fill_fifo(used);
	else {
		DBG(3, "%s: Empty buffer.\n", __FUNCTION__);
	}
   
	/* We have to clear the int request _after_ filling the fifo as 
	 * half-full int requests are generated as the fifo is filled as 
	 * well as being emptied...
	 */
	ATU_Registers->ETU_int_clear = (ETU_INT_WR_CH1L_HALF | 
						ETU_INT_WR_CH1L_EMPTY);

	wake_up_interruptible (&audio_state.wq);
}

static loff_t audio_llseek (struct file *file, loff_t offset, int whence)
{
	/* audio output behaves like a pipe, therefore is unseekable */
	return -ESPIPE;
}


static int audio_open (struct inode *inode, struct file *file)
{
	unsigned long flags;

	local_irq_save(flags);
	if (audio_state.count) {
		local_irq_restore(flags);
		return -EBUSY;
	}
	audio_state.count++;

	MOD_INC_USE_COUNT;   

	local_irq_save (flags);

	/* Default mode is 2 channel (stereo) 16-bit PCM */
	audio_state.channels = 2;
	audio_state.sample_size = 16;
	audio_state.first_write = 1;
	set_audio_samplerate (44100);

	/* purge any stale data left in the audio output buffers */
	audio_state.head = audio_state.tail = 0;

	SDAC_Registers->ctrl_1 = 0x00000000;
	SDAC_Registers->ctrl_2 = 0x00000000;
	/* set bit 12 to configure for 2's complement input */
	SDAC_Registers->settings = (1 << 12);        

	SDAC_Registers->bypass = (DIO_BYPASS_DSP | ATU1toIIS1out | 
					ATU1toIIS2out | ATU1toATU1out | 
					ATU2toATU2out);
	ATU_Registers->ATU_config = (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | 
					ATU_WR_CH1_LENGTH);

	/* Clear all ETU interrupts as half full int requests are 
	 * generated during filling as well as draining...
	 */
	ATU_Registers->ETU_int_clear = 0x7fffffff;
	ATU_Registers->ATU_int_enable = ATU_IRQ_WR_CH1L_HALF | 
						ATU_IRQ_WR_CH1L_EMPTY;
	ATU_Registers->ATU_config = ATU_IFLAG_EN | (ATU_WR_CH1L_EN | 
					ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);

	local_irq_restore(flags);

	return 0;
}


static int audio_release (struct inode *inode, struct file *file)
{
	unsigned long flags;

	DBG (3, "audio_release: goodbye !!\n");

	local_irq_save(flags);
	ATU_Registers->ATU_int_enable = 0;
	ATU_Registers->ETU_int_clear = 0x7fffffff;
	ATU_Registers->ATU_config = (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | 
						ATU_WR_CH1_LENGTH);
	audio_state.count--;
	local_irq_restore(flags);
	MOD_DEC_USE_COUNT;
	return 0;
}

static inline int copy_userbuf(const char *userbuf, int count)
{
	int tail = audio_state.tail;
	int temp;
	unsigned long flags;
	register int sram = audio_state.use_sram;
	u8 *buf = sram ? (u8 *)(IO_ADDRESS_ISRAM_BASE + 16384) : 
							audio_state.buf;

	if (tail + count <= AUDIO_FRAMES) {
		if (!sram) {
			if (copy_from_user(&buf[tail], userbuf, count))
				return -EFAULT;
		} else {
			memcpy(&buf[tail], (void *)IO_ADDRESS_ISRAM_BASE,
									count);
		}
	} else {
		temp = AUDIO_FRAMES - tail;
		if (!sram) {
			if (copy_from_user(&buf[tail], userbuf, temp))
				return -EFAULT;
			if (copy_from_user(&buf[0], userbuf+temp, 
							count-temp))
				return -EFAULT;
		} else {
			memcpy(&buf[tail], (void *)IO_ADDRESS_ISRAM_BASE, temp);
			memcpy(&buf[0], (void *)(IO_ADDRESS_ISRAM_BASE+temp),
							count-temp);
		}
	}
	tail = (tail + count) % AUDIO_FRAMES;
	local_irq_save(flags);
	audio_state.tail = tail;
	local_irq_restore(flags);
	return 0;
}

static inline int translate_from_user(const char *userbuf, int count)
{
	unsigned long flags;
	register int tail = audio_state.tail, i;
	register u16 left, right = 0;
	u8 temp;

	for (i = 0; i < count; ) {
		if (audio_state.sample_size == 16) {
			if (get_user(left, (u16 *)(&userbuf[i])))
				return -EFAULT;;
			i += 2;
			if (audio_state.channels == 2) {
				printk("Invalid call to %s. Stereo "
					"channels do not need translation.\n",
					__FUNCTION__);
				BUG();
			} else {
				right = left;
			}
		} else {
			if (get_user(temp, (u8 *)&userbuf[i++]))
				return -EFAULT;
			left = (u16)temp;
			if (audio_state.channels == 2) {
				if (get_user(temp, (u8 *)&userbuf[i++]))
					return -EFAULT;
				right = (u16)temp;
			} else {
				right = left;
			}
		}
		audio_state.buf[tail] = left;
		tail = (tail + 1) % AUDIO_FRAMES;
		audio_state.buf[tail] = right;
		tail = (tail + 1) % AUDIO_FRAMES;
	}
	local_irq_save(flags);
	audio_state.tail = tail;
	local_irq_restore(flags);
	return 0;
}

static ssize_t audio_write (struct file *file, const char *buf, size_t count, 
				loff_t *offset)
{
	unsigned int start;
	unsigned int stop;
	int tail, ret = count;
	int required = count;
	unsigned long flags;

	if (audio_state.sample_size == 8) 
		required <<= 1;
	if (audio_state.channels == 1)
		required <<= 1;

	/* No seeking allowed on this device */
	if (offset != &file->f_pos)
		return -ESPIPE;

	DBG(3, "%s: count = %d, sample_size = %d, channels = %d, "
			"required = %d\n",
			__FUNCTION__, count, audio_state.sample_size,
			audio_state.channels, required);
	DBG(3, "%s: INT status = %s\n", __FUNCTION__,
		(ATU_Registers->ETU_int_status & ETU_INT_WR_CH1L_HALF) ?
			"Pending" : "Idle");

	down(&audio_state.sem);

	if (0) {
		/* Special case, madplay using internal SRAM */
		audio_state.use_sram = 1;
		if (count > 16384)
			panic("Madplay's buffer too big\n");
	} else {
		audio_state.use_sram = 0;
	}

	while (frames_free() < required) {
		/* no space available... */
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto wdone;
		}
		DBG (3, "Audio: sleeping...\n");

		if (wait_event_interruptible (audio_state.wq, 
					frames_free() >= required)) {
			ret =  -ERESTARTSYS;
			goto wdone;
		}
	}
   
	DBG (3, "head = %d, tail = %d, free = %d\n", audio_state.head, 
			audio_state.tail, frames_free());
   
	tail = audio_state.tail;
	start = ReadTSC();
	if (audio_state.channels != 2 || audio_state.sample_size != 16) {
		if (translate_from_user(buf, count)) {
			ret = -EFAULT;
			goto wdone;
		}
	} else {
		if (copy_userbuf(buf, count)) {
			ret = -EFAULT;
			goto wdone;
		}
	}
	stop = ReadTSC();
   
	DBG (3, "%d uSec\n", (stop - start) / ReadTSCMHz());
	local_irq_save(flags);
	if (audio_state.first_write) {
		if (frames_used() >= (AUDIO_FRAMES * 3) / 4)  {
			ATU_Registers->ETU_int_set |= ETU_INT_WR_CH1L_EMPTY;
			audio_state.first_write = 0;
		}
	} else {
		ATU_Registers->ETU_int_set |= ETU_INT_WR_CH1L_EMPTY;
	}
	local_irq_restore(flags);
wdone:
	up(&audio_state.sem);
	return ret;
}


static int audio_ioctl (struct inode *inode, struct file *file, 
				unsigned int cmd, unsigned long arg)
{
	int value;
   
#ifdef AUDIO_DEBUG
	for (value = 0; value < sizeof(ioctl_str)/sizeof(ioctl_str[0]); 
								value++) {
		if (ioctl_str[value].cmd == cmd) {
			DBG(3, "%s: cmd = %s\n", __FUNCTION__, 
						ioctl_str[value].str);
			break;
		}
	}
#endif
	
	switch (cmd) {
	case SNDCTL_DSP_SPEED:
		if (get_user (value, (int *) arg))
			return -EFAULT;
		set_audio_samplerate (value);
		/* fall through */
	case SOUND_PCM_READ_RATE:
		return put_user (audio_state.audio_sample_rate, (int *) arg);

	case SNDCTL_DSP_CHANNELS:
		if (get_user (value, (int *) arg))
			return -EFAULT;
		DBG(1, "%s: DSP Set format %d called\n", DEVICE_NAME, value);
		if (value < 0 || value > 2)
			return -EINVAL;
		audio_state.channels = value;
		return put_user(value, (int *)arg);
		
	case SNDCTL_DSP_GETFMTS:
		return put_user(AFMT_S16_LE | AFMT_U8, (int *) arg);

	case SNDCTL_DSP_SETFMT:
		if (get_user(value, (int *) arg))
			return -EFAULT;
		if (value != AFMT_QUERY) {
			if (value == AFMT_S16_LE) {
				audio_state.sample_size = 16;
			} else {
				value = AFMT_U8;
				audio_state.sample_size = 8;
			}
		} else {
			value = (audio_state.sample_size == 16) ?
						AFMT_S16_LE : AFMT_U8;
		}
		return put_user(value, (int *) arg);

	case SOUND_PCM_READ_CHANNELS:
		return put_user (2, (int *) arg);
      
	case SNDCTL_DSP_STEREO:
		if (get_user (value, (int *) arg))
			return -EFAULT;
		if (value != 0) {
            		/* set stereo mode */
			DBG(1, "%s: Set stereo mode\n", DEVICE_NAME);
		} else {
			/* set mono mode */
			DBG(1, "%s: Set mono mode\n", DEVICE_NAME);
		}
		return 0;

	case SNDCTL_DSP_SYNC:
		/* fixme: wait for audio output buffers to drain and reset the 
		 * audio device
		 */
		return 0;

	default:
		printk (DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
		return -EINVAL;
	}
}


static struct file_operations audio_fops =
{
	owner   : THIS_MODULE,
	llseek  : audio_llseek,
	write   : audio_write,
	ioctl   : audio_ioctl,
	open    : audio_open,
	release : audio_release,
};

/* Initialize this modules and register the device. */
static int __init audio_dev_init (void)
{
	int result = 0;

	printk ("%s: output buffer size %dk\n", DEVICE_NAME, (AUDIO_BUFFER_SZ / 1024));

	init_waitqueue_head (&audio_state.wq);
	init_MUTEX(&audio_state.sem);

	audio_state.buf = 
		(u8 *)__get_free_pages(GFP_KERNEL, get_order(AUDIO_BUFFER_SZ));
	if (audio_state.buf == NULL) {
		printk ("%s: could not allocate audio buffer\n", DEVICE_NAME);
		return -ENOMEM;
	}

	audio_state.dev_audio = register_sound_dsp(&audio_fops, -1);
	if (audio_state.dev_audio < 0) {
		result = -ENODEV;
		goto init_error;
	}
	if ((result = request_irq (INT_ATU, audio_isr, 0, 
					DEVICE_NAME, NULL)) != 0) {
		printk ("%s: could not register interrupt %d\n", 
						DEVICE_NAME, INT_ATU);
		goto init_error;
		return result;
	}

	return 0;
init_error:
	free_pages((unsigned long)audio_state.buf, 
					get_order(AUDIO_BUFFER_SZ));
	return result;

}

static void __exit audio_dev_exit(void)
{
	free_irq (INT_ATU, NULL);
	unregister_sound_dsp(audio_state.dev_audio);
	free_pages((unsigned long)audio_state.buf, 
				get_order(AUDIO_BUFFER_SZ));
}


/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */
MODULE_AUTHOR("Ranjit Deshpande <ranjitd@kenati.com>");
MODULE_DESCRIPTION("Audio output driver for SAA7752 on-chip SDAC");
MODULE_LICENSE("GPL");

module_init(audio_dev_init);
module_exit(audio_dev_exit);
