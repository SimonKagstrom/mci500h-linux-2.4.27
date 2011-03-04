/*
 * linux/drivers/sound/pnx0106.c
 *
 * Copyright (C) 2006 QM  Philips Semiconductors.
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
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <asm/arch/audio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/hardware.h>
#include <asm/arch/platform_info.h>
#include <asm/arch/sdma.h>
#include <asm/arch/tsc.h>


u64 prevTsc ;

#undef AUDIO_DEBUG
#ifdef AUDIO_DEBUG
static int dbg_level = 3;
#define DBG(level, args...) if (level <= dbg_level) printk(args)
#else
#define DBG(level, args...)
#endif

/****************************************************************************
****************************************************************************/

#define DEVICE_NAME     "PNX0106 Audio"
//#define AUDIO_BUFFER_SZ (512 * 1024)
//#define DMA_BUFSIZE     (48 * 1024)  //add a task for toggling the spdif copy protection bit if a different buffer size is used
#define AUDIO_BUFFER_SZ (72 * 1024)
#define DMA_BUFSIZE     (8 * 1024)
#define DMA_HALFSIZE    (DMA_BUFSIZE/2)
#define READ_DMA_BUFSIZE (64 * 1024 )
#define READ_DMA_HALFSIZE ( READ_DMA_BUFSIZE / 2 )


#define AUDIOSS_MUX_SETTINGS        IO_ADDRESS_VPB3_BASE + 0x300 + 0x84     //0x3BF poweron spdif in, bypassmode for all (SAO2->DAC)
#define SPDIF_IN_STATUS             IO_ADDRESS_VPB3_BASE + 0x300+ 0x88
#define SPDIF_IN_IRQ_EN             IO_ADDRESS_VPB3_BASE + 0x300+ 0x8C
#define SPDIF_IN_IRQ_STATUS         IO_ADDRESS_VPB3_BASE + 0x300+ 0x90
#define SPDIF_IN_IRQ_CLEAR          IO_ADDRESS_VPB3_BASE + 0x300+ 0x94
#define SPDIF_IN_STATUS             IO_ADDRESS_VPB3_BASE + 0x300+ 0x88
#define SPDIF_IN_STATUS             IO_ADDRESS_VPB3_BASE + 0x300+ 0x88
#define SDAC_CTRL_INTI              IO_ADDRESS_VPB3_BASE + 0x300+ 0x98
#define SDAC_CTRL_INTO              IO_ADDRESS_VPB3_BASE + 0x300+ 0x9C
#define SDAC_SETTINGS               IO_ADDRESS_VPB3_BASE + 0x300+ 0xA0
#define SADC_CTRL_SDC               IO_ADDRESS_VPB3_BASE + 0x300+ 0xA4
#define SADC_CTRL_ADC               IO_ADDRESS_VPB3_BASE + 0x300+ 0xA8
#define SADC_CTRL_DECI              IO_ADDRESS_VPB3_BASE + 0x300+ 0xAC
#define SADC_CTRL_DECO              IO_ADDRESS_VPB3_BASE + 0x300+ 0xB0

#define SPDIF_OUT_CHST_L_31_0       IO_ADDRESS_VPB3_BASE + 0x300+ 0xC0
#define SPDIF_OUT_CHST_L_39_32      IO_ADDRESS_VPB3_BASE + 0x300+ 0xC4
#define SPDIF_OUT_CHST_R_31_0       IO_ADDRESS_VPB3_BASE + 0x300+ 0xC8
#define SPDIF_OUT_CHST_R_39_32      IO_ADDRESS_VPB3_BASE + 0x300+ 0xCC


#define CFG_SAI1_INTERLEAVE         IO_ADDRESS_VPB3_BASE + 0x60
#define CFG_SAI1_STATUS             IO_ADDRESS_VPB3_BASE + 0x10
#define CFG_SAI1_IRQ_MSK            IO_ADDRESS_VPB3_BASE + 0x14

#define CFG_SAO1_INTERLEAVE         IO_ADDRESS_VPB3_BASE + 0x200 +0x60
#define CFG_SAO1_STATUS             IO_ADDRESS_VPB3_BASE + 0x200 +0x10
#define CFG_SAO1_IRQ_MSK            IO_ADDRESS_VPB3_BASE + 0x200 +0x14

#define CFG_SAO2_LEFT16             IO_ADDRESS_VPB3_BASE + 0x280 +0x0
#define CFG_SAO2_RIGHT16            IO_ADDRESS_VPB3_BASE + 0x280 +0x4
#define CFG_SAO2_INTERLEAVE         IO_ADDRESS_VPB3_BASE + 0x280 +0x60
#define CFG_SAO2_STATUS             IO_ADDRESS_VPB3_BASE + 0x280 +0x10
#define CFG_SAO2_IRQ_MSK            IO_ADDRESS_VPB3_BASE + 0x280 +0x14

#define CFG_SAO3_LEFT16             IO_ADDRESS_VPB3_BASE + 0x300 +0x0
#define CFG_SAO3_RIGHT16            IO_ADDRESS_VPB3_BASE + 0x300 +0x4
#define CFG_SAO3_INTERLEAVE         IO_ADDRESS_VPB3_BASE + 0x300 +0x60
#define CFG_SAO3_STATUS             IO_ADDRESS_VPB3_BASE + 0x300 +0x10
#define CFG_SAO3_IRQ_MSK            IO_ADDRESS_VPB3_BASE + 0x300 +0x14

#define SAI1_INPUT_SEL_DAI1	    ((1<<2)UL)
#define SAI3_INPUT_SEL_SPDIF	    ((1<<4)UL)
#define SAI4_INPUT_SEL_ADC	    ((1<<5)UL)
#define DAI1_OE_N	    	    ((1<<7)UL)
#define DAI2_OE_N	    	    ((1<<8)UL)
#define SPDIF_IN_POWER_ON	    ((1<<9)UL)

#define CFG_SAI4_LEFT16             IO_ADDRESS_VPB3_BASE + 0x180 +0x0
#define CFG_SAI4_RIGHT16            IO_ADDRESS_VPB3_BASE + 0x180 +0x4
#define CFG_SAI4_INTERLEAVE         IO_ADDRESS_VPB3_BASE + 0x180 +0x60
#define CFG_SAI4_STATUS             IO_ADDRESS_VPB3_BASE + 0x180 +0x10
#define CFG_SAI4_IRQ_MSK            IO_ADDRESS_VPB3_BASE + 0x180 +0x14

#define PCR_ADSS_AHB_SLV_CLK        IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x18c
#define PCR_ADSS_AHB_MST_CLK        IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x190
#define PCR_ADSS_CACHE_PCLK         IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x274
#define PCR_ADSS_CONFIG_PCLK        IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x230
#define PCR_ADSS_DSS_PCLK           IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x240
#define PCR_ADSS_EDGE_DET_PCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x26c


#define PCR_ADSS_SAO2_PCLK          IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x258
#define PCR_ADSS_DAC_NS_DAC_CLK     IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x298
#define PCR_ADSS_DAC_DSPCLK         IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x29C
#define PCR_ADSS_DAC_IPOL_PCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x268

#define PCR_ADSS_ADC_SAI4_PCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x250
#define PCR_ADSS_ADC_DEC_PCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x270
#define PCR_ADSS_ADC_DECCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x2A0
#define PCR_ADSS_ADC_SYSCLK      IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x2A4

#define SRC_CLK_512FS               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x2C
#define FS1_CLK_512FS               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x78
#define FS2_CLK_512FS               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0xC4
#define SSR_CLK_512FS               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x110
#define ESR_DAC_NS_DAC_CLK          IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x5F0
#define ESR_DAC_DSPCLK              IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x5F4
#define BCR_CLK_512FS               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x638
#define CLK1024FS_FD4               IO_ADDRESS_VPB0_BASE + (CGU_REGS_BASE - VPB0_PHYS_BASE) + 0x68C


#define CONFIG_HP0_FIN_SELECT       IO_ADDRESS_VPB0_BASE + 0x4d00
#define CONFIG_HP0_MDEC             IO_ADDRESS_VPB0_BASE + 0x4d04
#define CONFIG_HP0_NDEC             IO_ADDRESS_VPB0_BASE + 0x4d08
#define CONFIG_HP0_PDEC             IO_ADDRESS_VPB0_BASE + 0x4d0c
#define CONFIG_HP0_MODE             IO_ADDRESS_VPB0_BASE + 0x4d10
#define CONFIG_HP0_STATUS           IO_ADDRESS_VPB0_BASE + 0x4d14
#define CONFIG_HP0_ACK              IO_ADDRESS_VPB0_BASE + 0x4d18
#define CONFIG_HP0_REQ              IO_ADDRESS_VPB0_BASE + 0x4d1c
#define CONFIG_HP0_INSELR           IO_ADDRESS_VPB0_BASE + 0x4d20
#define CONFIG_HP0_INSELI           IO_ADDRESS_VPB0_BASE + 0x4d24
#define CONFIG_HP0_INSELP           IO_ADDRESS_VPB0_BASE + 0x4d28
#define CONFIG_HP0_SELR             IO_ADDRESS_VPB0_BASE + 0x4d2c
#define CONFIG_HP0_SELI             IO_ADDRESS_VPB0_BASE + 0x4d30
#define CONFIG_HP0_SELP             IO_ADDRESS_VPB0_BASE + 0x4d34


static struct audio_state
{
    struct semaphore    sem;        /* Mutex for write operations */
    int                 write_count;      /* Allow only one write open(2) */
    int                 read_count;      /* Allow only one read open(2) */
    unsigned int        audio_sample_rate;
    unsigned int        Spdif_0_31; /* same as audio_sample_rate */
    wait_queue_head_t   wq;
    int                 dev_audio;  /* Handle returned by  soundcore*/
    int                 channels;   /* Number of channels */
    unsigned int        output;     /* DSP_BIND_SPDIF | DSP_BIND_FRONT*/
    int                 dac_sdma_channel;
    int                 spdo_sdma_channel;
    int                 adc_sdma_channel;

    /* head is an index into the 'frame' array of frame buffers.
     * It points to the first free frame buffer and is incremented when
     * data is written from user space.
     *
     * tail is also an index into the 'frame' array of frame buffers.
     * It points to the frame buffer which still contains unconsumed
     * audio data. It is incremented by the audio ISR when all data in a
     * frame has been ouput.
     *
     * head and tail must be maintained modulo AUDIO_BUFFER_SZ.
     *
     * If (head == tail) there is no data available.
     */
    u64             tsc;
    int             insert_bytes;
    u8 *            buf;        /* User data */
    int             head;
    int             tail;
    u8 *            mono_buf;        /* User data */
    u8 *            dma_buf;
    dma_addr_t      bus_addr;
    unsigned int    dma_len;
    int		    stalled;		

    u8 *	     read_dma_buf;
    dma_addr_t   read_bus_addr;
    unsigned int read_dma_len;
    u8 *	     read_buf;
    int          read_pos;
    wait_queue_head_t  read_wq;
    int 	    read_state;
    int 	    read_overruns;

    int             norm_fill;              /* normal DMA fill */
    int             norm_underrun_empty;    /* Underrun w/no data */
    int             norm_underrun_partial;  /* Underrun w/data */
    int             norm_stalled;  	    /* Underrun after underrun */
    int             sync_late_norm;         /* normal late synchronization */
    int             sync_late_nodata;       /* late sync w/no data */
    int             sync_late_underrun;     /* late sync w/partial data */
    int             sync_wait;              /* normal sync */
    int             sync_underrun;          /* sync w/no data */
    int             sync_fill;              /* sync w/full data */
} audio_state;

static struct mixer_state
{
    int           dev_mixer;    /* Handle returned by  soundcore*/
    unsigned int  base_attenuation;
}
mixer_state;

/****************************************************************************
****************************************************************************/

static void set_audio_samplerate (int value);
static void *dma_alloc_coherent( size_t size,dma_addr_t *dma_handle, int gfp);
static inline void fill_dma_buffer(int used,u8* dmabuf);
static inline int copy_userbuf(const char *userbuf, int count);
static int frames_used(void);
static int frames_free(void);
static inline int translate_from_user(const char *userbuf, int count);
static void set_output_routing(unsigned int value);
static void SetupSyncPlayback(u64 tsc);
static void FlushPlaybackBuf();

static int pnx0106_mixer_open (struct inode *inode, struct file *file);
static int pnx0106_mixer_release (struct inode *inode, struct file *file);
static int pnx0106_mixer_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);

static void start_input(int value);
static ssize_t pnx0106_audio_read(struct file *file, char *buf, size_t count, loff_t *offset);
static void pnx0106_audio_read_isr (unsigned int irqStat);
static void pnx0106_dummy_isr (unsigned int irqStat);
/****************************************************************************
****************************************************************************/

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


/*
	sdac_dboa2regval() and sdac_regval2dboa() can be used to convert
	back and forth between SDAC register values and db attenuation.

	The SDAC supports the following attenuation values:

	db gain 	db attenuation		SDAC register value
	--------	--------------		-------------------

	  0.00		 0.00			  0
	 -0.25		 0.25			  1
	 -0.50		 0.50			  2
	 -0.75		 0.75			  3
	 -1.00		 1.00			  4
	 -1.25		 1.25			  5
	 -1.50		 1.50			  6
	 -1.75		 1.75			  7
	 -2.00		 2.00			  8
	 -2.25		 2.25			  9
	 -2.50		 2.50			 10
	 -2.75		 2.75			 11
	 -3.00		 3.00			 12

	......		.....			...
	......		.....			...

	-49.00		49.00			196
	-49.25		49.25			197
	-49.50		49.50			198
	-49.75		49.75			199
	-50.00		50.00			200
	-53.00		53.00			204
	-56.00		56.00			208
	-59.00		59.00			212
	-62.00		62.00			216
	-65.00		65.00			220
	-68.00		68.00			224
	-71.00		71.00			228
	-74.00		74.00			232
	-77.00		77.00			236
	-80.00		80.00			240
	-83.00		83.00			244
	-90.00		90.00			252
*/

static unsigned int sdac_dboa2regval (unsigned int dboa)
{
	/* dboa == 'db of attenuation' (ie a value of 3 means -3 db gain) */

	return (dboa <= 50) ? (dboa * 4) :
	       (dboa <= 85) ? (200 + (((dboa - 50) / 3) * 4)) :
	       (dboa <= 88) ? 244 : 252;
}

static int sdac_regval2dboa (unsigned int regval)
{
	return (regval < 200) ? (regval / 4) :
	       (regval < 244) ? ((((regval - 200) / 4) * 3) + 50) :
	       (regval < 252) ? 83 : 90;
}

static inline void fill_dma_buffer(int used,u8* dmabuf)
{
    register int head = audio_state.head;
    int bytes ;
    int stalled = audio_state.stalled;

    audio_state.stalled = 0;

    if (audio_state.insert_bytes < 0)
    {
        if ( used + audio_state.insert_bytes >= 0)
        {//we are late, throw away some samples
            head = (head - audio_state.insert_bytes)%AUDIO_BUFFER_SZ;//- since insert_bytes is negative
            used += audio_state.insert_bytes;//+ since insert_bytes is negative
            audio_state.insert_bytes = 0;
            audio_state.tsc = 0;
            audio_state.sync_late_norm++;
        }
        else
        {
            if(used == 0)//some serious problems here, we are late and the buffer is empty
            {   //switch to normal playback mode, we may not be able to catch up at all
                audio_state.insert_bytes = 0;
                audio_state.tsc = 0;
                audio_state.sync_late_nodata++;
                printk("TOO LATE  switch to normal playback mode ######\n");
            }
            else
            {
                head = (head+used)%AUDIO_BUFFER_SZ;
                audio_state.insert_bytes += used;
                used = 0;//hopefully we have more data to throw away in the next round
                memset(dmabuf,0x0,DMA_HALFSIZE);
                audio_state.sync_late_underrun++;
                audio_state.stalled = 1;
            }
        }
    }

    if (audio_state.insert_bytes == 0) //normal playback mode
    {
        if(used >= DMA_HALFSIZE)//normal case
        {
            if(audio_state.tail > head)
            {
                memcpy(dmabuf,&audio_state.buf[head],DMA_HALFSIZE);
            }
            else
            {
                bytes = AUDIO_BUFFER_SZ - head ;
                if(bytes < DMA_HALFSIZE)
                {
                    memcpy(dmabuf,&audio_state.buf[head],bytes);
                    memcpy(dmabuf+bytes,audio_state.buf,DMA_HALFSIZE-bytes);
                }
                else
                {
                    memcpy(dmabuf,&audio_state.buf[head],DMA_HALFSIZE);
                }
            }
            head += DMA_HALFSIZE;
            head %= AUDIO_BUFFER_SZ;
            audio_state.norm_fill++;
        }
        else //buffer underrun, fill rest with 0
        {
            if(audio_state.tail >= head)
            {
                memcpy(dmabuf,&audio_state.buf[head],used);
                memset(dmabuf+used,0x0,DMA_HALFSIZE - used);
            }
            else
            {
                bytes = AUDIO_BUFFER_SZ - head ;
                memcpy(dmabuf,&audio_state.buf[head],bytes);
                memcpy(dmabuf+bytes,audio_state.buf,used-bytes);
                memset(dmabuf+used,0x0,DMA_HALFSIZE - used);
            }
            head +=used;
            head %=AUDIO_BUFFER_SZ;

            if (used != 0)
               audio_state.norm_underrun_partial++;
	    else
	    {
		if (stalled)
		    audio_state.norm_stalled++;
		else
		    audio_state.norm_underrun_empty++;

		audio_state.stalled = 1;
	    }
        }
    }else
    if (audio_state.insert_bytes > 0) //sync playback mode
    {
        if (audio_state.insert_bytes >= DMA_HALFSIZE)
        {
            memset(dmabuf,0x0,DMA_HALFSIZE);
            audio_state.insert_bytes -= DMA_HALFSIZE;
            audio_state.sync_wait++;
        }
        else
        {
            bytes = DMA_HALFSIZE - audio_state.insert_bytes;
            if (used < bytes)//something wrong, we should have more data
            {
                printk("pnx0106: sync playback mode - UNDERRUN !!!!!!!!!!!!!!!!!!!\n");
                memset(dmabuf,0x0,DMA_HALFSIZE-used);
                //we flushed the buffer before, it's safe to assume head=0 < tail
                memcpy(dmabuf+(DMA_HALFSIZE-used),&audio_state.buf[head],used);
                head = (head+used)%AUDIO_BUFFER_SZ;
                audio_state.sync_underrun++;
	        audio_state.stalled = 1;
            }
            else
            {
                memset(dmabuf,0x0,audio_state.insert_bytes);
                //we flushed the buffer before, it's safe to assume head=0 < tail
                memcpy(dmabuf+audio_state.insert_bytes,&audio_state.buf[head],bytes);
                head = (head+bytes)%AUDIO_BUFFER_SZ;
               audio_state.sync_fill++;
            }
            printk("pnx0106: sync playback mode - START PLAYING used=%d insert=%d!!!\n",used,audio_state.insert_bytes);
            audio_state.insert_bytes = 0;
            audio_state.tsc = 0;
        }
    }
    audio_state.head = head;
}

static void pnx0106_audio_isr (unsigned int irqStat)
{
    int used;
    u8 *dmabuf = NULL;
    int channel ;
    static int cnt = 0;
    //printk("Irq 0x%x #######\n",irqStat);
#if 1
    if( (audio_state.output & DSP_BIND_FRONT) != 0)
    {
        channel = audio_state.dac_sdma_channel;
    }
    else
    {
        channel = audio_state.spdo_sdma_channel;
    }


    if (irqStat == 0x2)//more than half way
    {
        dmabuf = audio_state.dma_buf;//first half
    }else
    if (irqStat == 0x1)//finished
    {
        dmabuf = audio_state.dma_buf+DMA_HALFSIZE;//second half
    }

    cnt++;
    if(audio_state.audio_sample_rate == 32000)
    {
        if (cnt == 1)
        {
            writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_L_31_0);//toggle copy protection bit
            writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_R_31_0);
        }
        else
        {
            writel((audio_state.Spdif_0_31 | 0x4),SPDIF_OUT_CHST_L_31_0);//toggle copy protection bit
            writel((audio_state.Spdif_0_31 | 0x4),SPDIF_OUT_CHST_R_31_0);
            cnt = 0;
        }
    }
    else
    {
        if ( cnt == 1)
        {
            //current dma buffer is 8K -> interrupt frequencies between 5.3Hz(44.1kHz) and 5.8Hz(48kHz)
            writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_L_31_0);//toggle copy protection bit
            writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_R_31_0);
        }else
        if ( cnt == 3)
        {
            writel((audio_state.Spdif_0_31 | 0x4),SPDIF_OUT_CHST_L_31_0);//toggle copy protection bit
            writel((audio_state.Spdif_0_31 | 0x4),SPDIF_OUT_CHST_R_31_0);
        }else
        if ( cnt == 4)
        {
            cnt = 0;
        }
    }

    //underrun
    if (irqStat == 0x3)
    {
        printk ("pnx0106_audio_write UNDERRUN @@@@@@ %d \n", ((audio_state.tail - audio_state.head + AUDIO_BUFFER_SZ) % AUDIO_BUFFER_SZ));
        if( sdma_get_count(channel) < DMA_HALFSIZE/4)
        {
            dmabuf = audio_state.dma_buf+DMA_HALFSIZE;//second half
        }
        else
        {
            dmabuf = audio_state.dma_buf;//first half
        }
    }
    used = (audio_state.tail - audio_state.head + AUDIO_BUFFER_SZ) % AUDIO_BUFFER_SZ;
    fill_dma_buffer(used,dmabuf);
    wake_up_interruptible (&audio_state.wq);
#else
    u64 curTsc;
    u64    us;
    u32 ticks_per_usec;
    curTsc = ReadTSC64();
    if (irqStat == 0x3)
    {
        ticks_per_usec = ReadTSCMHz();
        us = (curTsc - prevTsc)/ticks_per_usec;
        printk ("pnx0106_audio_write UNDERRUN @@@@@@xx@@ %lu ",curTsc);
        printk ("%lu ",prevTsc);
        printk (" ms = %lu\n",us);
    }
    prevTsc = ReadTSC64();
#endif
}


//borrowed from Andre's driver
static inline int copy_userbuf(const char *userbuf, int count)
{
    int tail = audio_state.tail;
    int temp;
    unsigned long flags;
    u8 *buf = audio_state.buf;

    if (tail + count <= AUDIO_BUFFER_SZ)
    {
        if (copy_from_user(&buf[tail], userbuf, count))
        {
            return -EFAULT;
        }
    }
    else
    {
        temp = AUDIO_BUFFER_SZ - tail;

        if (copy_from_user(&buf[tail], userbuf, temp))
        {
            return -EFAULT;
        }
        if (copy_from_user(&buf[0], userbuf+temp,count-temp))
        {
            return -EFAULT;
        }
    }
    tail = (tail + count) % AUDIO_BUFFER_SZ;
    local_irq_save(flags);
    audio_state.tail = tail;
    local_irq_restore(flags);
    return 0;
}

static int frames_used(void)
{
    unsigned long flags;
    int used;

    local_irq_save(flags);
    used = (audio_state.tail - audio_state.head + AUDIO_BUFFER_SZ) %
                                AUDIO_BUFFER_SZ;
    local_irq_restore(flags);
    return used;
}

static int frames_free(void)
{
    return AUDIO_BUFFER_SZ - frames_used() - 1;
}


static inline int translate_from_user(const char *userbuf, int count)
{
#if 0
    unsigned long flags;
    register int tail = audio_state.tail, i;
    register u16 left = 0;

    for (i = 0; i < count; )
    {
        if (get_user(left, (u16 *)(&userbuf[i])))
            return -EFAULT;
        *(u16 *)&audio_state.buf[tail] = left;
        tail = (tail + 2) % AUDIO_BUFFER_SZ;
        *(u16 *)&audio_state.buf[tail] = 0;//left;
        tail = (tail + 2) % AUDIO_BUFFER_SZ;
        i += 2;
    }
    local_irq_save(flags);
    audio_state.tail = tail;
    local_irq_restore(flags);
#else
    int  i;
    u16 *pCur;
    u16 *pBufEnd;

    pCur = (u16 *)&audio_state.buf[audio_state.tail];
    pBufEnd = (u16 *)&audio_state.buf[AUDIO_BUFFER_SZ-2];
    if (copy_from_user(audio_state.mono_buf, userbuf, count))
         return -EFAULT;
    for (i = 0; i < count; )
    {
        *pCur = *((u16 *)&audio_state.mono_buf[i]);
        if (pCur >= pBufEnd)
             pCur = (u16 *)audio_state.buf;
        else
          pCur++;

        *pCur = *((u16 *)&audio_state.mono_buf[i]);
        if (pCur >= pBufEnd)
             pCur = (u16 *)audio_state.buf;
        else
          pCur++;
        i += 2;
    }
    audio_state.tail = (audio_state.tail + count*2) % AUDIO_BUFFER_SZ;
#endif
    return 0;
}


static loff_t pnx0106_audio_llseek (struct file *file, loff_t offset, int whence)
{
    printk ("pnx0106_audio_llseek\n" );
    return -ESPIPE;
}


static int pnx0106_audio_open (struct inode *inode, struct file *file)
{
    unsigned long flags;

//  printk(" pnx0106_audio_open\n" );

    local_irq_save (flags);

    if ((file->f_mode & FMODE_WRITE) && audio_state.write_count) {
        local_irq_restore (flags);
        return -EBUSY;
    }

    if ((file->f_mode & FMODE_READ) && audio_state.read_count) {
        local_irq_restore (flags);
        return -EBUSY;
    }

    MOD_INC_USE_COUNT;

    if (file->f_mode & FMODE_WRITE) {
        audio_state.write_count++;

        /* Default mode is 2 channel (stereo) 16-bit PCM */
        audio_state.channels = 2;
#if 1
        audio_state.output = DSP_BIND_SPDIF | DSP_BIND_FRONT;
#else
        audio_state.output = DSP_BIND_SPDIF;
#endif
        /* purge any stale data left in the audio output buffers */
        audio_state.head = audio_state.tail = 0;
        audio_state.insert_bytes = 0;
        audio_state.tsc = 0;

        /* temp debug: be extra sure not to output stale data... */
        memset (audio_state.dma_buf, 0, audio_state.dma_len);
    }

    set_audio_samplerate (44100);

    if (file->f_mode & FMODE_READ) {
        audio_state.read_count++;
        memset (audio_state.read_dma_buf, 0, READ_DMA_BUFSIZE);
        audio_state.read_pos = READ_DMA_HALFSIZE;
        audio_state.read_overruns = 0;
    }

    local_irq_restore (flags);
    return 0;
}


static int pnx0106_audio_release (struct inode *inode, struct file *file)
{
    unsigned long flags;

    printk ("pnx0106_audio_release\n");

    local_irq_save(flags);
    if( file->f_mode & FMODE_WRITE ) {
    	audio_state.write_count--;
    	sdma_stop_channel(audio_state.dac_sdma_channel);
    	sdma_stop_channel(audio_state.spdo_sdma_channel);
    	MOD_DEC_USE_COUNT;
    }

    if( file->f_mode & FMODE_READ ) {

    	sdma_stop_channel(audio_state.adc_sdma_channel);
    	sdma_clear_irq(audio_state.adc_sdma_channel);
    	sdma_reset_count(audio_state.adc_sdma_channel);
    	//audio_state.input_readers = 0;
	    audio_state.read_count--;
	    audio_state.read_state = 0;
	    audio_state.read_overruns = 0;
    	memset(audio_state.read_dma_buf,0x0,READ_DMA_BUFSIZE);
    	MOD_DEC_USE_COUNT;
    }

    local_irq_restore(flags);
    return 0;
}

static ssize_t pnx0106_audio_write (struct file *file, const char *buf, size_t count,
                loff_t *offset)
{
#if 1
    int ret = 0;
    int required = count;
    int num;

    if (audio_state.channels == 1)
        required <<= 1;

    required = ( required >> 2 ) <<2 ;
    //printk("pnx0106_audio about to get semaphore, required %d\n", required);
    down(&audio_state.sem);
    num = ( frames_free() >> 2 ) <<2 ;
    //printk("pnx0106_audio audio buffer space %d\n", num);
    while (1)
    {
        if (num > required - ret)
            num = required - ret;
        if (num > 0)
        {
            //printk("pnx0106_audio writing %d bytes to audio buffer\n", num);
            if (audio_state.channels == 1)
            {
                if (translate_from_user(buf, num/2))
                {
                    if (ret == 0)
                        ret = -EFAULT;
                    goto wdone;
                }
            }
            else
            {
                if (copy_userbuf(buf, num))
                {
                    if (ret == 0)
                        ret = -EFAULT;
                    goto wdone;
                }
            }
        }
        buf += num;
        ret += num;
        if (ret >= required)
            break;
        if (file->f_flags & O_NONBLOCK)
        {
            if (ret == 0)
                ret = -EAGAIN;
            goto wdone;
        }
        num = required - ret;
        if (num > (AUDIO_BUFFER_SZ/2))
            num = (AUDIO_BUFFER_SZ/2);
        //printk("pnx0106_audio about to wait for %d bytes free in the audio buffer\n", num);
        if (wait_event_interruptible(audio_state.wq, frames_free() >= num))
        {
            if (ret == 0)
                ret =  -ERESTARTSYS;
            goto wdone;
        }
        num = ( frames_free() >> 2 ) <<2 ;
        //printk("pnx0106_audio audio buffer space %d\n", num);
    }
wdone:
    up(&audio_state.sem);
    //printk("pnx0106_audio put semaphore, ret %d\n", ret);

    if (ret > 0 && audio_state.channels == 1)
       return ret / 2;

    return ret;
#else
    udelay((count*1000000L)/(44100*4L));
    return count;
#endif
}


static int pnx0106_audio_ioctl (struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
    int value;

//  printk ("pnx0106_audio_ioctl\n" );

    switch (cmd)
    {
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
            if (value < 1 || value > 2)
                return -EINVAL;
            audio_state.channels = value;
            return put_user(value, (int *)arg);

        case SNDCTL_DSP_GETFMTS:
            return put_user(AFMT_S16_LE, (int *) arg);

        case SNDCTL_DSP_SETFMT:
            if (get_user(value, (int *) arg))
                return -EFAULT;
            value = AFMT_S16_LE;//ignore user's value, we only support 16-bit
            return put_user(value, (int *) arg);

        case SOUND_PCM_READ_CHANNELS:
            return put_user (2, (int *) arg);

        case SNDCTL_DSP_STEREO:
            return 0;

        case SNDCTL_DSP_SYNC:
            /* Open sound manual:"This call is practically useless...", misuse it for flushing the audio buffer*/
            FlushPlaybackBuf();
            return 0;

        case SNDCTL_DSP_POST:
            /*misuse the OSS POST command for playback with precise timing */
            {
                u64 val;
                if (copy_from_user(&val,(char*)arg,sizeof(u64)))
                    return -EFAULT;
                SetupSyncPlayback(val);
            }
            return 0;
        //case SNDCTL_DSP_SET_PLAYTGT:
        case SNDCTL_DSP_BIND_CHANNEL:
            if (get_user (value, (int *) arg))
                return -EFAULT;
            if (value == DSP_BIND_QUERY)
            {
                return put_user(audio_state.output, (int *)arg);
            }
            value &= DSP_BIND_FRONT |DSP_BIND_SPDIF;
            if (value== 0)
                value = DSP_BIND_FRONT |DSP_BIND_SPDIF;
            audio_state.output = value;
            set_output_routing(value);
            return put_user(value, (int *)arg);
	case PNX0106_AUDIO_STATS:
	    {
		unsigned long flags;
		struct pnx0106_audio_stats stats;

		local_irq_save(flags);
		stats.norm_fill = audio_state.norm_fill;
		stats.norm_underrun_empty = audio_state.norm_underrun_empty;
		stats.norm_underrun_partial = audio_state.norm_underrun_partial;
		stats.norm_stalled = audio_state.norm_stalled;
		stats.sync_late_norm = audio_state.sync_late_norm;
		stats.sync_late_nodata = audio_state.sync_late_nodata;
		stats.sync_late_underrun = audio_state.sync_late_underrun;
		stats.sync_wait = audio_state.sync_wait;
		stats.sync_underrun = audio_state.sync_underrun;
		stats.sync_fill = audio_state.sync_fill;
		local_irq_restore(flags);

		if (copy_to_user(arg, &stats, sizeof stats))
		    return -EFAULT;

		return 0;
	    }
        default:
            printk (DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
            return -EINVAL;
    }
    return 0;
}

#define MAX_MUSECONDS       30000000L
static void SetupSyncPlayback(u64 tsc)
{
    u64 curTSC;
    u32 ticks_per_usec = ReadTSCMHz();
    u64 ms;
    unsigned long flags;
    int dma_remainder;

    down(&audio_state.sem);
    local_irq_save(flags);
    //flush buffer....
    audio_state.tail = audio_state.head = 0;
    curTSC = ReadTSC64();
    if( (audio_state.output & DSP_BIND_FRONT) != 0)//output to dac, channel 0
    {
        dma_remainder = 4*(((DMA_BUFSIZE/4)- sdma_get_count(audio_state.dac_sdma_channel))%(DMA_HALFSIZE/4));
    }
    else//output to spdif only
    {
        dma_remainder = 4*(((DMA_BUFSIZE/4)- sdma_get_count(audio_state.spdo_sdma_channel))%(DMA_HALFSIZE/4));
    }
    if ( curTSC <= tsc)
    {
        ms = (tsc - curTSC)/ticks_per_usec;
        audio_state.tsc = tsc;
        //make sure that it's a multiple of 4
        audio_state.insert_bytes = (sizeof(u16)*2)*((audio_state.audio_sample_rate*ms)/1000000L);
        printk("\npnx0106: SetupSyncPlayback insert %d  bytes!!!\n\n",audio_state.insert_bytes);
    }
    else
    {//throw away samples if we are late
        ms = (curTSC - tsc )/ticks_per_usec;
        if (ms > MAX_MUSECONDS)//limit to 30 sec
        {
            ms = MAX_MUSECONDS;
        }
        audio_state.tsc = tsc;
        //make sure that it's a multiple of 4
        audio_state.insert_bytes = -(sizeof(u16)*2)*((audio_state.audio_sample_rate*ms)/1000000L);
        printk("\npnx0106: SetupSyncPlayback skip %d  bytes!!!\n\n",audio_state.insert_bytes);
    }
    //adjust to dma remainder
    audio_state.insert_bytes -= dma_remainder;
    printk("\npnx0106: SetupSyncPlayback adjust to dma_remainder=%d -> %d  bytes ****\n\n",dma_remainder,audio_state.insert_bytes);
    local_irq_restore(flags);
    up(&audio_state.sem);
}


static void FlushPlaybackBuf()
{
    unsigned long flags;
    down(&audio_state.sem);
    local_irq_save(flags);
    if ( audio_state.insert_bytes != 0)
    {
         //flush the audio buffer only if we are still in delayed playback mode
         printk("pnx0106: FlushPlaybackBuf ****\n");
         audio_state.tail = audio_state.head = 0;
         audio_state.insert_bytes = 0;
         audio_state.tsc = 0;
    }
    local_irq_restore(flags);
    up(&audio_state.sem);
}

static struct file_operations audio_fops =
{
    owner   : THIS_MODULE,
    llseek  : pnx0106_audio_llseek,
    write   : pnx0106_audio_write,
    read    : pnx0106_audio_read,
    ioctl   : pnx0106_audio_ioctl,
    open    : pnx0106_audio_open,
    release : pnx0106_audio_release,
};

static struct file_operations mixer_fops =
{
    owner   : THIS_MODULE,
    ioctl   : pnx0106_mixer_ioctl,
    open    : pnx0106_mixer_open,
    release : pnx0106_mixer_release,
};


/* Initialize this modules and register the device. */
static int __init pnx0106_audio_dev_init (void)
{
    struct sdma_config dma;
    unsigned int timeout;
    unsigned int regval;
    int result = 0;

    printk ("pnx0106_audio_dev_init !\n");

    init_waitqueue_head (&audio_state.wq);
    init_waitqueue_head (&audio_state.read_wq);
    init_MUTEX(&audio_state.sem);

    audio_state.write_count = 0;
    audio_state.read_count = 0;
    audio_state.buf =
        (u8 *)__get_free_pages(GFP_KERNEL, get_order(AUDIO_BUFFER_SZ));
    if (audio_state.buf == NULL) {
        printk ("%s: could not allocate audio buffer\n", DEVICE_NAME);
        return -ENOMEM;
    }
    audio_state.mono_buf =
        (u8 *)__get_free_pages(GFP_KERNEL, get_order(AUDIO_BUFFER_SZ));
    if (audio_state.mono_buf == NULL) {
        printk ("%s: could not allocate audio (mono)buffer\n", DEVICE_NAME);
        return -ENOMEM;
    }
    audio_state.dev_audio = register_sound_dsp(&audio_fops, -1);
    if (audio_state.dev_audio < 0)
    {
        printk ("pnx0106: could not register sound device !\n");
        result = -ENODEV;
    }

    /* default settings */
    writel(0x3BF,AUDIOSS_MUX_SETTINGS);

    /* power on sequence... */

    writel (readl (SDAC_CTRL_INTI) | (1 << 24), SDAC_CTRL_INTI);	/* set long PD slope */
    writel (readl (SDAC_CTRL_INTI) | (1 << 25), SDAC_CTRL_INTI);	/* power down interpolator */

    timeout = 1000000;  						/* expected time is a few msec (depending on DAC config and clocks) */
    while (1) {
    	udelay (1);
    	if ((readl (SDAC_CTRL_INTO) & (1 << 1)) != 0)			/* Wait until interpolator reports power down status */
    	    break;
    	if (--timeout == 0) {
    	    printk ("pnx0106 audio: timeout interpolator power down\n");
    	    break;
    	}
    }

    writel (readl (SDAC_SETTINGS) | (3 << 8), SDAC_SETTINGS);		/* Power on DAC left + right channels */
    writel (readl (SDAC_CTRL_INTI) & (~(1 << 25)), SDAC_CTRL_INTI);	/* Power on interpolator */

    timeout = 1000000;  						/* expected time is a few msec (depending on DAC config and clocks) */
    while (1) {
    	udelay (1);
    	if ((readl (SDAC_CTRL_INTO) & (1 << 0)) == 0)			/* wait until interpolator reports 'mute state' as inactive */
    	    break;
    	if (--timeout == 0) {
    	    printk ("pnx0106 audio: timeout interpolator 'mute state' inactive\n");
    	    break;
    	}
    }

    writel (readl (SDAC_CTRL_INTI) | (1 << 19), SDAC_CTRL_INTI);	/* Mute interpolator */

    timeout = 1000000;  						/* expected time is a few msec (depending on DAC config and clocks) */
    while (1) {
    	udelay (1);
    	if ((readl (SDAC_CTRL_INTO) & (1 << 0)) != 0)			/* wait until interpolator reports 'mute state' as active */
    	    break;
    	if (--timeout == 0) {
    	    printk ("pnx0106 audio: timeout interpolator 'mute state' active\n");
    	    break;
    	}
    }

    writel (readl (SDAC_SETTINGS) | (1 << 13), SDAC_SETTINGS);  	/* power on HP common output amp */
    writel (readl (SDAC_SETTINGS) | (3 << 14), SDAC_SETTINGS);  	/* power on HP L and R output amps */

    mdelay (5);

    writel (readl (SDAC_CTRL_INTI) & (~(1 << 19)), SDAC_CTRL_INTI);	/* Un-mute interpolator */


    printk ("pnx0106_audio_dev_init: SDAC_SETTINGS 0x%08x\n", readl (SDAC_SETTINGS));
    writel( (1<<12)| (0x3 << 10) | (1<<13) | (3<<14) | (3<<8),SDAC_SETTINGS);   /* set two's complement*/
    printk ("pnx0106_audio_dev_init: SDAC_SETTINGS 0x%08x\n", readl (SDAC_SETTINGS));


    mixer_state.base_attenuation = platform_info_default_audio_attenuation_db();
    regval = sdac_dboa2regval (mixer_state.base_attenuation);
    regval |= (regval << 8);
    writel ((readl (SDAC_CTRL_INTI) &  0xFFFF0000) | (regval & 0x0000FFFF), SDAC_CTRL_INTI);

    /*  init ADC */
    /* Do we have to set default PGA values ? */
    //writel(( (0x0 <<3) | (0x0 << 8) | (0x0 << 3) | (3 << 17) ) , SADC_CTRL_SDC);     /*QMxxx normal operation, power up all*/
    writel(( (0x2 <<3) | (0x2 << 8) | (0x2 << 13) | (0 << 17) ) , SADC_CTRL_SDC);     /*QMxxx normal operation, power up all, PGA 6db gain left/right/MIC*/
    /* what is input 2 or input 1 means ?*/
    writel(0x11, SADC_CTRL_ADC);    /*analog L/R to input 1 L/R */

    //writel(0x3131, SADC_CTRL_DECI); /*QMxxx 24 db gain ??? */
    writel(0x0000, SADC_CTRL_DECI); /*QMxxx 0 db gain ??? */

    audio_state.adc_sdma_channel = sdma_allocate_channel();
    printk("SDMA Channel allocated for input = %d\n",audio_state.adc_sdma_channel);
    if ( audio_state.adc_sdma_channel == -1)
        goto init_error;

    audio_state.dac_sdma_channel = sdma_allocate_channel();
    printk("SDMA Channel allocated = %d\n",audio_state.dac_sdma_channel);
    if ( audio_state.dac_sdma_channel == -1)
        goto init_error;

    audio_state.spdo_sdma_channel = sdma_allocate_channel();
    if ( audio_state.spdo_sdma_channel == -1)
        goto init_error;

    /* SDMA setup*/
    audio_state.dma_len = DMA_BUFSIZE;
    if ( (audio_state.dma_buf = dma_alloc_coherent(audio_state.dma_len, &(audio_state.bus_addr), GFP_KERNEL|GFP_DMA)) == 0)
    {
        printk ("%s: could not allocate dma memory \n",DEVICE_NAME);
        goto init_error;
    }
    //printk("%s: allocated DMA buf for audio write addr: %x\n", DEVICE_NAME, audio_state.dma_buf );
    memset(audio_state.dma_buf,0x0,DMA_BUFSIZE);

    audio_state.read_dma_len = READ_DMA_BUFSIZE;
    if ( (audio_state.read_dma_buf = dma_alloc_coherent(audio_state.read_dma_len, &(audio_state.read_bus_addr), GFP_KERNEL|GFP_DMA)) == 0)
    {
        printk ("%s: could not allocate dma memory for read\n",DEVICE_NAME);
        goto init_error;
    }
    //printk("%s: allocated DMA buf for audio read addr: %x\n", DEVICE_NAME, audio_state.read_dma_buf );
    memset(audio_state.read_dma_buf,0x0,READ_DMA_BUFSIZE);

    //chan0-soa2-dac
    writel(0x0,CFG_SAO2_STATUS);//clear sao2 status reg
    dma.src_addr = audio_state.bus_addr;
    dma.dst_addr = VPB3_PHYS_BASE + 0x280 + 0x60; //mem - > SAO2 -> DAC
    dma.len      = DMA_BUFSIZE/4;
    dma.size     = TRANFER_SIZE_32;
    dma.src_name = DEVICE_MEMORY;
    dma.dst_name = DEVICE_SAO2R;
    dma.endian   = ENDIAN_NORMAL;
    dma.circ_buf = USE_CIRC_BUF;
    sdma_setup_channel(audio_state.dac_sdma_channel,&dma,pnx0106_audio_isr);

    //chan1-sao3-spdif
    writel(0x0,CFG_SAO3_STATUS);//clear sao3 status reg
    dma.src_addr = audio_state.bus_addr;
    dma.dst_addr = VPB3_PHYS_BASE + 0x300 + 0x60; //mem - > SAO3 -> SPDO
    dma.len      = DMA_BUFSIZE /4;
    dma.size     = TRANFER_SIZE_32;
    dma.src_name = DEVICE_MEMORY;
    dma.dst_name = DEVICE_SAO3R;
    dma.endian   = ENDIAN_NORMAL;
    dma.circ_buf = USE_CIRC_BUF;
    sdma_setup_channel(audio_state.spdo_sdma_channel,&dma,pnx0106_audio_isr);

    //chan - ADC to SAI4
    writel(0x0,CFG_SAI4_STATUS);//clear sai4 irq status reg
    dma.src_addr = VPB3_PHYS_BASE + 0x180 + 0x60;
    dma.dst_addr = audio_state.read_bus_addr;
    dma.len      = (READ_DMA_BUFSIZE)/4;
    dma.size     = TRANFER_SIZE_32;
    dma.src_name = DEVICE_SAI4L;
    dma.dst_name = DEVICE_MEMORY;
    dma.endian   = ENDIAN_NORMAL;
    dma.circ_buf = USE_CIRC_BUF;
    sdma_setup_channel(audio_state.adc_sdma_channel, &dma, pnx0106_audio_read_isr);

    mixer_state.dev_mixer = register_sound_mixer(&mixer_fops, -1);
    if (mixer_state.dev_mixer < 0)
    {
        printk ("pnx0106: could not register mixer device !\n");
        result = -ENODEV;
    }
    audio_state.read_state = 0;
    audio_state.read_overruns = 0;

#if defined (CONFIG_PNX0106_W6)
    gpio_set_as_output (GPIO_SOURCE_SELECT2, 1);    // pnx0106 is output source
    gpio_set_as_output (GPIO_SOURCE_SELECT1, 0);
    gpio_set_as_output (GPIO_MUTE, 0);              // drive GPIO_MUTE low to unmute (Production boards)
    gpio_set_as_output (GPIO_MUTE_OLD, 0);          // drive GPIO_MUTE_OLD low to unmute (TR boards and before)
#endif

    return result;
init_error:
    free_pages((unsigned long)audio_state.buf,get_order(AUDIO_BUFFER_SZ));
    return result;
}


static void __exit pnx0106_audio_dev_exit(void)
{
    printk ("pnx0106_audio_dev_exit\n" );
    sdma_free_channel(audio_state.dac_sdma_channel);
    sdma_free_channel(audio_state.spdo_sdma_channel);
    sdma_free_channel(audio_state.adc_sdma_channel);
    unregister_sound_dsp(audio_state.dev_audio);
    unregister_sound_mixer(mixer_state.dev_mixer);
    free_pages((unsigned long)audio_state.buf,get_order(AUDIO_BUFFER_SZ));
    free_pages((unsigned long)audio_state.dma_buf,get_order(DMA_BUFSIZE));
    free_pages((unsigned long)audio_state.read_dma_buf,get_order(READ_DMA_BUFSIZE));
}

/****************************************************************************/
/* dma_alloc_coherent / dma_free_coherent ported from 2.5                  */
/****************************************************************************/

static void *dma_alloc_coherent( size_t size,dma_addr_t *dma_handle, int gfp)
{
    void *ret;
    /* ignore region specifiers */
    gfp &= ~(__GFP_DMA | __GFP_HIGHMEM);
    gfp |= GFP_DMA;
    ret = (void *)__get_free_pages(gfp, get_order(size));
    if (ret != NULL)
    {
        memset(ret, 0, size);
        *dma_handle = virt_to_phys(ret);
    }
    return ret;
}


static void set_output_routing(unsigned int value)
{
    unsigned long flags;
    local_irq_save(flags);

    sdma_stop_channel(audio_state.dac_sdma_channel);
    sdma_stop_channel(audio_state.spdo_sdma_channel);
    sdma_clear_irq(audio_state.dac_sdma_channel);
    sdma_clear_irq(audio_state.spdo_sdma_channel);
    sdma_reset_count(audio_state.dac_sdma_channel);
    sdma_reset_count(audio_state.spdo_sdma_channel);
    if (value == DSP_BIND_FRONT)
    {
        sdma_start_channel(audio_state.dac_sdma_channel);
        sdma_stop_channel(audio_state.spdo_sdma_channel);
        sdma_config_irq(audio_state.dac_sdma_channel,IRQ_FINISHED | IRQ_HALFWAY);
    }else
    if (value == DSP_BIND_SPDIF)
    {
        sdma_stop_channel(audio_state.dac_sdma_channel);
        sdma_start_channel(audio_state.spdo_sdma_channel);
        sdma_config_irq(audio_state.spdo_sdma_channel,IRQ_FINISHED | IRQ_HALFWAY);
    }
    else
    {
        sdma_start_channel(audio_state.dac_sdma_channel);
        sdma_start_channel(audio_state.spdo_sdma_channel);
        sdma_config_irq(audio_state.dac_sdma_channel,IRQ_FINISHED | IRQ_HALFWAY);
        sdma_config_irq(audio_state.spdo_sdma_channel,0);//disable interrupt on this channel
    }
    local_irq_restore(flags);
}

struct sample_rate_info
{
   unsigned int sample_rate;
   unsigned int fs;
   unsigned int ndec;
   unsigned int mdec;
   unsigned int pdec;
   unsigned int selr;
   unsigned int seli;
   unsigned int selp;
   unsigned int spdif;
   unsigned int outmask;
} sample_rates[] =
{
   {  8000, 1024, 102,  8194, 31, 7,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 11025,  512, 187, 10024,  6, 9,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 12000,  512,   5,    34,  6, 0, 14, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 16000,  512, 102,  8194, 31, 7,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 22050,  512, 131,  1408,  7, 8,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 24000,  512,  63, 16973, 24, 2,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 32000,  256, 102,  8194, 31, 7,  2, 31, 0x03008100, DSP_BIND_SPDIF | DSP_BIND_FRONT },
   { 44100,  256, 131,  1408,  7, 8,  2, 31, 0x00008100, DSP_BIND_SPDIF | DSP_BIND_FRONT },
   { 48000,  256,  63, 16973, 24, 2,  2, 31, 0x02008100, DSP_BIND_SPDIF | DSP_BIND_FRONT },
   { 64000,  256, 102,  8194, 14, 7,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 88200,  256, 131,  1408, 23, 8,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
   { 96000,  256,  63, 16416, 14, 3,  2, 31, 0x00000000,                  DSP_BIND_FRONT },
};

static void set_audio_samplerate (int value)
{
    unsigned long flags;
    int i;
    unsigned int FS,NDEC,MDEC,PDEC,SELR,SELI,SELP ;

    printk("PNX0106: set_audio_samplerate** %d",value);

    local_irq_save(flags);
    sdma_stop_channel(audio_state.dac_sdma_channel);
    sdma_stop_channel(audio_state.spdo_sdma_channel);
    sdma_stop_channel(audio_state.adc_sdma_channel);
    audio_state.audio_sample_rate = value;

    for (i = 0; i < sizeof sample_rates / sizeof sample_rates[0]; i++)
       if (sample_rates[i].sample_rate == value)
          break;

    if (i >= sizeof sample_rates / sizeof sample_rates[0])
       for (i = 0; i < sizeof sample_rates / sizeof sample_rates[0]; i++)
          if (sample_rates[i].sample_rate == 44100)
             break;

    FS = sample_rates[i].fs;
    NDEC = sample_rates[i].ndec;
    MDEC = sample_rates[i].mdec;
    PDEC = sample_rates[i].pdec;
    SELR = sample_rates[i].selr;
    SELI = sample_rates[i].seli;
    SELP = sample_rates[i].selp;
    audio_state.Spdif_0_31 = sample_rates[i].spdif;

    printk("NDEC %d MDEC %d PDEC %d SELR %d SELI %d SELP %d SPDIF %x\n", NDEC, MDEC, PDEC, SELR, SELI, SELP, sample_rates[i].spdif);

    // Turn off SPDIF if necessary
    audio_state.output &= sample_rates[i].outmask;

    if (audio_state.output == 0)
       audio_state.output = sample_rates[i].outmask;

    writel((1<<2), CONFIG_HP0_MODE); //power down PLL
    writel(0x1, CONFIG_HP0_FIN_SELECT); // 0x01 == hardcode to FFAST
    writel(MDEC, CONFIG_HP0_MDEC);
    writel(NDEC, CONFIG_HP0_NDEC);
    writel(PDEC, CONFIG_HP0_PDEC);
    writel(SELR, CONFIG_HP0_SELR);
    writel(SELI, CONFIG_HP0_SELI);
    writel(SELP, CONFIG_HP0_SELP);
    writel(0x1, CONFIG_HP0_MODE);

    for (i = 0; i < 4096; i++)
    {
        if ((readl(CONFIG_HP0_STATUS) & 0x01) != 0)
            break;
    }

#if 0
    // reset fractional divide
    writel(2, CLK1024FS_FD4);
    readl(CLK1024FS_FD4);

    switch (FS)
    {
    case 256:
        writel(0x7F9FE5, CLK1024FS_FD4);
	break;
    case 512:
        writel(0x7FDFF5, CLK1024FS_FD4);
	break;
    case 1024:
        writel(0x7FFFFD, CLK1024FS_FD4);
	break;
    }
#endif

    writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_L_31_0);//spdif sample frequency
    writel(audio_state.Spdif_0_31,SPDIF_OUT_CHST_R_31_0);
    local_irq_restore(flags);
    set_output_routing(audio_state.output);
}
#if 0
void DumpRegs()
{
    printk("PNX0106: PCR_ADSS_AHB_SLV_CLK=0x%x\n",readl(PCR_ADSS_AHB_SLV_CLK));
    printk("PNX0106: PCR_ADSS_AHB_MST_CLK=0x%x\n",readl(PCR_ADSS_AHB_MST_CLK));
    printk("PNX0106: PCR_ADSS_CACHE_PCLK=0x%x\n",readl(PCR_ADSS_CACHE_PCLK));
    printk("PNX0106: PCR_ADSS_CONFIG_PCLK=0x%x\n",readl(PCR_ADSS_CONFIG_PCLK));
    printk("PNX0106: PCR_ADSS_DSS_PCLK=0x%x\n",readl(PCR_ADSS_DSS_PCLK));
    printk("PNX0106: PCR_ADSS_EDGE_DET_PCLK=0x%x\n",readl(PCR_ADSS_EDGE_DET_PCLK));

    printk("PNX0106: PCR_ADSS_SAO2_PCLK=0x%x\n",readl(PCR_ADSS_SAO2_PCLK));
    printk("PNX0106: PCR_ADSS_DAC_NS_DAC_CLK=0x%x\n",readl(PCR_ADSS_DAC_NS_DAC_CLK));
    printk("PNX0106: PCR_ADSS_DAC_DSPCLK=0x%x\n",readl(PCR_ADSS_DAC_DSPCLK));
    printk("PNX0106: PCR_ADSS_DAC_IPOL_PCLK=0x%x\n",readl(PCR_ADSS_DAC_IPOL_PCLK));

    printk("PNX0106: AUDIOSS_MUX_SETTINGS=0x%x\n",readl(AUDIOSS_MUX_SETTINGS));
    printk("PNX0106: SDAC_SETTINGS=0x%x\n",readl(SDAC_SETTINGS));
    printk("PNX0106: SDAC_CTRL_INTI=0x%x\n",readl(SDAC_CTRL_INTI));
    printk("PNX0106: SDAC_CTRL_INTO=0x%x\n",readl(SDAC_CTRL_INTO));
    printk("PNX0106: SADC_CTRL_SDC=0x%x\n",readl(SADC_CTRL_SDC));
    printk("PNX0106: SADC_CTRL_ADC=0x%x\n",readl(SADC_CTRL_ADC));
    printk("PNX0106: SADC_CTRL_DECI=0x%x\n",readl(SADC_CTRL_DECI));

    printk("PNX0106: SRC_CLK_512FS=0x%x\n",readl(SRC_CLK_512FS));
    printk("PNX0106: FS1_CLK_512FS=0x%x\n",readl(FS1_CLK_512FS));
    printk("PNX0106: FS2_CLK_512FS=0x%x\n",readl(FS2_CLK_512FS));
    printk("PNX0106: SSR_CLK_512FS=0x%x\n",readl(SSR_CLK_512FS));
    printk("PNX0106: ESR_DAC_NS_DAC_CLK=0x%x\n",readl(ESR_DAC_NS_DAC_CLK));
    printk("PNX0106: ESR_DAC_DSPCLK=0x%x\n",readl(ESR_DAC_DSPCLK));
    printk("PNX0106: BCR_CLK_512FS=0x%x\n",readl(BCR_CLK_512FS));

    printk("audio_dev_init: CFG_SAO1_STATUS=0x%x\n",readl(CFG_SAO1_STATUS));
    printk("audio_dev_init: CFG_SAO1_IRQ_MSK=0x%x\n",readl(CFG_SAO1_IRQ_MSK));
    printk("audio_dev_init: CFG_SAO2_STATUS=0x%x\n",readl(CFG_SAO2_STATUS));
    printk("audio_dev_init: CFG_SAO2_IRQ_MSK=0x%x\n",readl(CFG_SAO2_IRQ_MSK));

}
#endif

static int pnx0106_mixer_open (struct inode *inode, struct file *file)
{
    //printk ("pnx0106_mixer_open\n" );
    return 0;
}

static int pnx0106_mixer_release (struct inode *inode, struct file *file)
{
    //printk ("pnx0106_mixer_release\n" );
    return 0;
}

static int pnx0106_mixer_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;
    unsigned int left, right, val;
    unsigned int default_attenuation;

    /* FIXME: locking is required !! */

    switch (cmd)
    {
#if defined (CONFIG_PNX0106_W6)
    case SOUND_MIXER_PRIVATE1:
        if (get_user (value, (int *) arg))
            return -EFAULT;
        {
            //printk ("pnx0106_mixer_ioctl: out=%d\n",value);
            switch(value)
            {
                case 0:
                gpio_set_low(GPIO_SOURCE_SELECT1);
                gpio_set_low(GPIO_SOURCE_SELECT2);
                break;
                case 1:
                gpio_set_high(GPIO_SOURCE_SELECT1);
                gpio_set_low(GPIO_SOURCE_SELECT2);
                /* CD */
                break;
                case 2:
                gpio_set_low(GPIO_SOURCE_SELECT1);
                gpio_set_high(GPIO_SOURCE_SELECT2);
                /* pnx out */
                break;
                case 3:
                gpio_set_high(GPIO_SOURCE_SELECT1);
                gpio_set_high(GPIO_SOURCE_SELECT2);
                /* FM / ipod */
                break;
            }
        }
        return put_user (value, (int *) arg);
#endif

        case SOUND_MIXER_WRITE_VOLUME:
            /*
               Volume level passed from userspace is expressed in terms
               of db attenuation. Left and right channel attenuation
               values are passed as unsigned 8bit values. Negative
               attenuation (ie gain) is not supported.
            */
            if (get_user (value, (int *) arg))
                return -EFAULT;

            left = (value >> 0) & 0xFF;
            right = (value >> 8) & 0xFF;

            printk ("pnx0106_mixer_ioctl: attenuation left %d, right %d\n", left, right);
            if (mixer_state.base_attenuation != 0) {
                left += mixer_state.base_attenuation;
                right += mixer_state.base_attenuation;
                printk ("pnx0106_mixer_ioctl: attenuation left %d, right %d\n", left, right);
            }

            left = sdac_dboa2regval (left);
            right = sdac_dboa2regval (right);
            printk ("pnx0106_mixer_ioctl: regvalue left %d, right %d\n", left, right);
            val = (right << 8) | left;
            writel ((readl (SDAC_CTRL_INTI) &  0xFFFF0000) | (val & 0x0000FFFF), SDAC_CTRL_INTI);
            break;

        case SOUND_MIXER_READ_VOLUME:
            val = readl (SDAC_CTRL_INTI);
            left = sdac_regval2dboa ((val >> 0) & 0xFF);
            right = sdac_regval2dboa ((val >> 8) & 0xFF);

            printk ("pnx0106_mixer_ioctl: attenuation left %d, right %d\n", left, right);
            if (mixer_state.base_attenuation != 0) {
                left = (left < mixer_state.base_attenuation) ? 0 : (left - mixer_state.base_attenuation);
                right = (right < mixer_state.base_attenuation) ? 0 : (right - mixer_state.base_attenuation);
                printk ("pnx0106_mixer_ioctl: attenuation left %d, right %d\n", left, right);
            }

            value = (right << 8) | left;
            return put_user (value, (int *) arg);

        case SOUND_MIXER_PRIVATE2:  //arg 1=mute 0=unmute 2=get mute status
            if (get_user (value, (int *) arg))
                return -EFAULT;
            //printk ("pnx0106_mixer_ioctl: mute=%d\n",value);

            val = readl (SDAC_CTRL_INTO) & 0x00000001;   // initial mute status

            switch (value)
            {
                case 0:
                    //printk ("pnx0106_mixer_ioctl: unmute\n");
                    writel ((readl(SDAC_CTRL_INTI) & 0xFFF7FFFF), SDAC_CTRL_INTI);
                    break;
                case 1:
                    //printk ("pnx0106_mixer_ioctl: mute\n");
                    writel ((readl(SDAC_CTRL_INTI) & 0xFFF7FFFF) | (1 << 19), SDAC_CTRL_INTI);
                    break;
                case 2:
                    //printk ("pnx0106_mixer_ioctl: mute stat=%d\n", val);
                    break;

                default:
                    printk ("pnx0106_mixer_ioctl: mute=%d !?!\n", value);
                    break;
            }
            value = val;
            return put_user (value, (int *) arg);

        case SOUND_MIXER_PRIVATE3:
        if (get_user (value, (int *) arg))
            return -EFAULT;
        if (value == 1)
        {
            printk ("pnx0106: Connect SAO3 to DAC **********!\n");
            writel(0x3BF,AUDIOSS_MUX_SETTINGS);
        }
        else
        {
            printk ("pnx0106: Connect SAO3 to EPICS **********!\n");
            writel(0x3B5,AUDIOSS_MUX_SETTINGS);
        }
        return put_user (value, (int *) arg);

#if defined (CONFIG_PNX0106_LPAS1)
    case SOUND_MIXER_PRIVATE4:
        if (get_user (value, (int *) arg))
            return -EFAULT;
        {
            struct sdma_config dma;
            printk ("pnx0106_mixer_ioctl: PRIVATE4 val=%d !!!\n",value);
            switch(value)
            {
                case 0:
                sdma_stop_channel(audio_state.dac_sdma_channel);
                sdma_clear_irq(audio_state.dac_sdma_channel);
                sdma_reset_count(audio_state.dac_sdma_channel);
                //chan0-soa2-dac
                writel(0x0,CFG_SAO2_STATUS);//clear sao2 status reg
                dma.src_addr = audio_state.bus_addr;
                dma.dst_addr = VPB3_PHYS_BASE + 0x280 + 0x60; //mem - > SAO2 -> DAC
                dma.len      = DMA_BUFSIZE/4;
                dma.size     = TRANFER_SIZE_32;
                dma.src_name = DEVICE_MEMORY;
                dma.dst_name = DEVICE_SAO2R;
                dma.endian   = ENDIAN_NORMAL;
                dma.circ_buf = USE_CIRC_BUF;
                sdma_setup_channel(audio_state.dac_sdma_channel,&dma,pnx0106_audio_isr);
                sdma_config_irq(audio_state.dac_sdma_channel,IRQ_FINISHED | IRQ_HALFWAY);
                sdma_start_channel(audio_state.dac_sdma_channel);
                break;

                case 1:
                sdma_stop_channel(audio_state.dac_sdma_channel);
                sdma_clear_irq(audio_state.dac_sdma_channel);
                sdma_reset_count(audio_state.dac_sdma_channel);
                writel(0x0,CFG_SAO2_STATUS);//clear sao2 status reg
                writel(0x0,CFG_SAI4_STATUS);//clear sai4 irq status reg
                dma.src_addr = VPB3_PHYS_BASE + 0x180 + 0x60; //ADC  -> SAI4
                dma.dst_addr = VPB3_PHYS_BASE + 0x280 + 0x60; //SAO2 -> DAC
                dma.len      = 1024000;
                dma.size     = TRANFER_SIZE_32;
                dma.src_name = DEVICE_SAI4L;
                dma.dst_name = DEVICE_SAO2L;
                dma.endian   = ENDIAN_NORMAL;
                dma.circ_buf = USE_CIRC_BUF;
                sdma_setup_channel(audio_state.dac_sdma_channel,&dma,pnx0106_dummy_isr);
                sdma_config_irq(audio_state.dac_sdma_channel,0);
                sdma_start_channel(audio_state.dac_sdma_channel);
                break;
            }
        }
        return put_user (value, (int *) arg);
#endif
        default:
            printk ("pnx0106_mixer_ioctl: Mystery IOCTL called (0x%08x)\n", cmd);
            return -EINVAL;
    }

    return 0;
}


static ssize_t pnx0106_audio_read(struct file *file, char *buf, size_t bytes, loff_t *offset)
{    	
    unsigned long flags;
    int num,count,copied,pos;

    if( bytes == 0 )
         return 0;
    if (audio_state.read_state == 0)
    {
        audio_state.read_pos = READ_DMA_HALFSIZE;
        audio_state.read_state = 1;
        start_input(1); /* default input is line in */
    }	

    count = bytes;
    copied = 0;
    while ( count > 0)
    {
        /* if audio fragment is not filled yet wait */	
        if( audio_state.read_pos == READ_DMA_HALFSIZE)
    	        wait_event_interruptible(audio_state.read_wq,audio_state.read_pos == 0);
        //printk ("read_pos=%d count=%d copied=%d\n",audio_state.read_pos,count,copied);
        local_irq_save(flags);
        if ( count > (READ_DMA_HALFSIZE - audio_state.read_pos))
        {
            num = (READ_DMA_HALFSIZE - audio_state.read_pos);
        }
        else
        {
            num = count;
        }
        pos = audio_state.read_pos;
        audio_state.read_pos += num;
        local_irq_restore(flags);
        copy_to_user((void *)buf+copied,&audio_state.read_buf[pos],num);			
        copied += num;
        count -= num;
        //local_irq_restore(flags);
    }
    return bytes;
}

static void start_input( int value )
{
    unsigned long flags;

    local_irq_save(flags);
    sdma_stop_channel(audio_state.adc_sdma_channel);
    sdma_clear_irq(audio_state.adc_sdma_channel);
    sdma_reset_count(audio_state.adc_sdma_channel);
    sdma_config_irq(audio_state.adc_sdma_channel,IRQ_FINISHED | IRQ_HALFWAY);
    sdma_start_channel(audio_state.adc_sdma_channel);

    local_irq_restore(flags);
}

static void pnx0106_audio_read_isr (unsigned int irqStat)
{	
    if(irqStat == 0x2)
    {
        audio_state.read_buf = audio_state.read_dma_buf;//first half
    }else
    if (irqStat == 0x1)
    {		
        audio_state.read_buf = audio_state.read_dma_buf + READ_DMA_HALFSIZE;
    }else
    if( (irqStat == 0x3) )
    {
         printk("%s pnx0106_audio_read_isr: err missing interrupt !!!!\n", DEVICE_NAME);
         if( sdma_get_count(audio_state.adc_sdma_channel) < READ_DMA_HALFSIZE/4)
         {
            audio_state.read_buf = audio_state.read_dma_buf + READ_DMA_HALFSIZE;//second half
         }
         else
         {
            audio_state.read_buf = audio_state.read_dma_buf;//first half
         }
    }
    audio_state.read_pos = 0;
    wake_up_interruptible (&audio_state.read_wq);
}

static void pnx0106_dummy_isr (unsigned int irqStat)
{
     printk ("pnx0106_dummy_isr: sh.....  (0x%08x)\n",irqStat);
}


/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */
MODULE_AUTHOR("QM");
MODULE_DESCRIPTION("Audio output driver for pnx0106 on-chip SDAC");
MODULE_LICENSE("GPL");

module_init(pnx0106_audio_dev_init);
module_exit(pnx0106_audio_dev_exit);
