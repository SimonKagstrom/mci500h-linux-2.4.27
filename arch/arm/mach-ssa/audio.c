/*
 * linux/arch/arm/mach-ssa/audio.c
 *
 * Copyright (C) 2003 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/major.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/soundcard.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/tsc.h>


/****************************************************************************
****************************************************************************/

#define DEVICE_NAME     "SAA7752_Audio"

/*
   Assume channel 1 (we don't use channel 2).
*/

#define ATU_FIFO_LT     ((volatile unsigned int *) (IO_ADDRESS_ATU_FIFO_BASE + 0x00))
#define ATU_FIFO_RT     ((volatile unsigned int *) (IO_ADDRESS_ATU_FIFO_BASE + 0x40))


/****************************************************************************
****************************************************************************/

struct frame_buffer
{
   unsigned short left[1152];
   unsigned short right[1152];
};

/*
   Devote all of the internal SRAM to audio output buffers.
   ie 14 buffers (approx 1/3 of a second at 44.1 kHz sample rate).
*/

#define AUDIO_FRAMES    ((64 * 1024) / sizeof (struct frame_buffer))


static struct audio_state
{
   int count;
   spinlock_t lock;
   unsigned int audio_sample_rate;
   wait_queue_head_t wq;

   /*
      frame_head is an index into the 'frame' array of frame buffers.
      It points to the first free frame buffer and is incremented when data
      is written from user space.
      
      frame_tail is also an index into the 'frame' array of frame buffers.
      It points to the frame buffer which still contains unconsumed audio data.
      It is incremented by the audio ISR when all data in a frame has been ouput.
      
      frame_head and frame_tail must be maintained modulo AUDIO_FRAMES.
      
      If (frame_head == frame_tail) there is no data available.
   */
  
   struct frame_buffer *frame;
   volatile int frame_head;
   volatile int frame_tail;
}
audio_state;



/****************************************************************************
****************************************************************************/

typedef struct
{
   volatile unsigned int ClockSelection;        /* RW 0x00 */
   volatile unsigned int DSPPLL;                /* -- 0x04 (not present in SAA7752) */
   volatile unsigned int AudioPLL;              /* RW 0x08 */
   volatile unsigned int MasterPLL;             /* RW 0x0C */
}
PMU_Clock_Registers_t;

#define PMU_Clock_Registers ((PMU_Clock_Registers_t *) IO_ADDRESS_CLOCK_CTRL_BASE)


typedef struct
{
   volatile unsigned int dummy_0[2];
   volatile unsigned int bypass;                /* RW 0x08 */
   volatile unsigned int dummy_1[5];
   volatile unsigned int ctrl_1;                /* RW 0x20 */
   volatile unsigned int ctrl_2;                /* RW 0x24 */
   volatile unsigned int settings;              /* RW 0x28 */
}
SDAC_Registers_t;

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
}
ATU_Registers_t;

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



/****************************************************************************
****************************************************************************/

static void set_audio_samplerate (struct audio_state *state, int samplerate)
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
   unsigned int new_mux_setting = 0x03;                         /* default (should always be over-ridden...) */
   unsigned int new_pll_setting = AUDIO_PLL_REG_32768;          /* default (should always be over-ridden...) */
   unsigned long flags;
   
   for (i = 0; i < (sizeof(asr_tab)/sizeof(asr_tab[0])); i++) {
      if (samplerate >= asr_tab[i].samplerate) {
         state->audio_sample_rate = asr_tab[i].samplerate;
         new_mux_setting = asr_tab[i].mux_setting;
         new_pll_setting = asr_tab[i].pll_setting;
         break;
      }
   }

   local_irq_save (flags);
   PMU_Clock_Registers->AudioPLL = new_pll_setting;
   PMU_Clock_Registers->ClockSelection = ((PMU_Clock_Registers->ClockSelection & 0x3FFFFFFF) | (new_mux_setting << 30));
   local_irq_restore (flags);
}


static int frames_used (struct audio_state *state)
{
   int used = (state->frame_head - state->frame_tail);          /* read volatiles once only... !! */
   return (used < 0) ? (used + AUDIO_FRAMES) : used;
}

static int frames_free (struct audio_state *state)
{
   return AUDIO_FRAMES - frames_used (state) - 1;
}




static void audio_isr (int irq, void *dev_id, struct pt_regs *regs)
{
   /*
      One complete cycle (2Pi radians...) in 44 samples is (44100/44) == 1002.27 kHz
      (plus or minus the few ppm error introduced by running pll input at 10 MHz rather
      than a multiple of 44100... :-)
   */

   int tail = audio_state.frame_tail;
   static int index;
   
   int wakeup_required = 0;

#if 0

   static int count;

   static const unsigned short sinedata[44] =
   {
      0x0000, 0x0ce1, 0x1980, 0x2599, 0x30ef, 0x3b45, 0x4467, 0x4c24, 
      0x5254, 0x56d7, 0x5996, 0x5a82, 0x5996, 0x56d7, 0x5254, 0x4c24, 
      0x4467, 0x3b45, 0x30ef, 0x2599, 0x1980, 0x0ce1, 0x0000, 0xf31f, 
      0xe680, 0xda67, 0xcf11, 0xc4bb, 0xbb99, 0xb3dc, 0xadac, 0xa929, 
      0xa66a, 0xa57e, 0xa66a, 0xa929, 0xadac, 0xb3dc, 0xbb99, 0xc4bb, 
      0xcf11, 0xda67, 0xe680, 0xf31f
   };

   while ((ATU_Registers->ATU_ch1_wr_status & (ATU_STATUS_LEFT_FULL | ATU_STATUS_RIGHT_FULL)) == 0)
   {
#if 1
      *ATU_FIFO_LT = (sinedata[index] << 16);
      *ATU_FIFO_RT = (sinedata[index] << 16);
#else
      *ATU_FIFO_LT = (0 << 16);
      *ATU_FIFO_RT = (0 << 16);
#endif
      
      if (++index == 44)
         index = 0;
   }

   count++;
   
// if ((count % 512 == 0) && frames_used (&audio_state) > 0)
   if ((count %  32 == 0) && frames_used (&audio_state) > 0)
   {
      if (++tail == AUDIO_FRAMES)
         tail = 0;
      audio_state.frame_tail = tail;
      wake_up_interruptible (&audio_state.wq);
   }

#else

   while ((ATU_Registers->ATU_ch1_wr_status & (ATU_STATUS_LEFT_FULL | ATU_STATUS_RIGHT_FULL)) == 0)
   {
//    *ATU_FIFO_LT = audio_state.frame[tail].left[index]  << 16;
//    *ATU_FIFO_RT = audio_state.frame[tail].right[index] << 16;

      *ATU_FIFO_LT = audio_state.frame[tail].left[(index * 2) + 0] << 16;
      *ATU_FIFO_RT = audio_state.frame[tail].left[(index * 2) + 1] << 16;
      
      if (++index == 1152)
      {
         index = 0;
         if (++tail == AUDIO_FRAMES)
            tail = 0;
         audio_state.frame_tail = tail;
         wakeup_required = 1;
      }
   }
   
#endif

   /*
      We have to clear the int request _after_ filling the fifo as half-full
      int requests are generated as the fifo is filled as well as being emptied...
   */

   ATU_Registers->ETU_int_clear = (ETU_INT_WR_CH1L_HALF | ETU_INT_WR_CH1L_EMPTY);

   if (wakeup_required)
      wake_up_interruptible (&audio_state.wq);
}


void audio_topup (void)
{
   if (ATU_Registers->ETU_int_status & ETU_INT_WR_CH1L_HALF)        /* there is already a pending int... leave something for the ISR to do */
      return;
}


static loff_t audio_llseek (struct file *file, loff_t offset, int whence)
{
   return -ESPIPE;      /* audio output behaves like a pipe, therefore is unseekable */
}


static int audio_open (struct inode *inode, struct file *file)
{
   struct audio_state *state = &audio_state;
   int result;
   unsigned long flags;

// if (MINOR(inode->i_rdev) != 4)               /* /dev/audio... fixme: does this have an offical definition somewhere ?? */
//    return -ENODEV;
      
   spin_lock (&state->lock);
   if (state->count) {
      spin_unlock (&state->lock);
      return -EBUSY;
   }
   state->count++;
   spin_unlock (&state->lock);
   MOD_INC_USE_COUNT;   

   state->frame_head = state->frame_tail;       /* purge any stale data left in the audio output buffers */

   if ((result = request_irq (INT_ATU, audio_isr, 0, DEVICE_NAME, NULL)) != 0) {
      printk ("%s: could not register interrupt %d\n", DEVICE_NAME, INT_ATU);
      return result;
   }

   spin_lock_irqsave (&state->lock, flags);

   SDAC_Registers->ctrl_1 = 0x00000000;
   SDAC_Registers->ctrl_2 = 0x00000000;
   SDAC_Registers->settings = (1 << 12);        /* set bit 12 to configure for 2's complement input */

   SDAC_Registers->bypass = (DIO_BYPASS_DSP | ATU1toIIS1out | ATU1toIIS2out | ATU1toATU1out | ATU2toATU2out);
   ATU_Registers->ATU_config = (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);

   /*
      Trying to write to already full fifos generates data aborts...
      Therefore to be certain of not blowing up, we must check fifo status before every write...
   */

   while ((ATU_Registers->ATU_ch1_wr_status & (ATU_STATUS_LEFT_FULL | ATU_STATUS_RIGHT_FULL)) == 0)
   {
      *ATU_FIFO_LT = (0 << 16);
      *ATU_FIFO_RT = (0 << 16);
   }

   /*
      Clear all ETU interrupts as half full int requests are generated during
      filling as well as draining...
   */

   ATU_Registers->ETU_int_clear = 0x7fffffff;
   ATU_Registers->ATU_int_enable = ATU_IRQ_WR_CH1L_HALF | ATU_IRQ_WR_CH1L_EMPTY;
   ATU_Registers->ATU_config = ATU_IFLAG_EN | (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);

   spin_unlock_irqrestore (&state->lock, flags);

   return 0;
}


static int audio_release (struct inode *inode, struct file *file)
{
   struct audio_state *state = &audio_state;
   unsigned long flags;

   printk ("audio_release: goodbye !!\n");

   spin_lock_irqsave (&state->lock, flags);
   ATU_Registers->ATU_int_enable = 0;
   ATU_Registers->ETU_int_clear = 0x7fffffff;
   ATU_Registers->ATU_config = (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);
   spin_unlock_irqrestore (&state->lock, flags);

   free_irq (INT_ATU, NULL);

   state->count--;
   MOD_DEC_USE_COUNT;

   return 0;
}

static ssize_t audio_write (struct file *file, const char *buf, size_t count, loff_t *offset)
{
   struct audio_state *state = &audio_state;
// ssize_t result = 0;
   int head;
   unsigned int start;
   unsigned int stop;

   if (offset != &file->f_pos)
      return -ESPIPE;

   if (count != (1152 * 4))
   {
      /*
         unexpected count... throw away the data.
      */
      printk ("Audio: unexpected count %d\n", count);
      return count;
   }
      
   while (frames_free (state) == 0)             /* no space available... */
   {
      if (file->f_flags & O_NONBLOCK)
         return -EAGAIN;
//    printk ("Audio: sleeping...\n");

      if (wait_event_interruptible (state->wq, frames_free (state) > 0))
         return -ERESTARTSYS;
   }   
   
   head = state->frame_head;
   
// printk ("head,tail,free = %2d,%2d,%2d\n", head, state->frame_tail, frames_free (state));
   
   start = ReadTSC();
   if (copy_from_user (&state->frame[head], buf, count))
      return -EFAULT;
   stop = ReadTSC();
   
// printk ("%d uSec\n", (stop - start) / ReadTSCMHz());

   if (++head >= AUDIO_FRAMES)
      head = 0;   
   
   state->frame_head = head;
   
   return count;
}


static int audio_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
   struct audio_state *state = &audio_state;
   int value;
   
   switch (cmd) 
   {
      case SNDCTL_DSP_SPEED:
         if (get_user (value, (int *) arg))
            return -EFAULT;
         set_audio_samplerate (state, value);
         /* fall through */
      case SOUND_PCM_READ_RATE:
         return put_user (state->audio_sample_rate, (int *) arg);

      case SNDCTL_DSP_SETFMT:
      case SNDCTL_DSP_GETFMTS:
         /*
            The user requested a format...
            As we only support option, there's no need to even look at the user request (if they
            don't like our response, they can convert in user space or abort).
         */
         return put_user (AFMT_S16_LE, (int *) arg);

      case SNDCTL_DSP_CHANNELS:
      case SOUND_PCM_READ_CHANNELS:
         return put_user (2, (int *) arg);
      
      case SNDCTL_DSP_STEREO:
         if (get_user (value, (int *) arg))
            return -EFAULT;
         if (value != 0)
         {
            /* set stereo mode */
         }
         else
         {
            /* set mono mode */
         }
         return 0;

      case SNDCTL_DSP_SYNC:
         /*
            fixme: wait for audio output buffers to drain and reset the audio device
         */
         return 0;

      default:
         printk (DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
         return -EINVAL;
   }
}


static struct file_operations audio_fops =
{
   .llseek  = audio_llseek,
   .write   = audio_write,
// .poll    = ???,
   .ioctl   = audio_ioctl,
   .open    = audio_open,
   .release = audio_release,
// .fsync   = ???,
   .owner   = THIS_MODULE,
};


static int __init audio_dev_init (void)
{
   struct audio_state *state = &audio_state;
   int result;

   set_audio_samplerate (state, 44100);
   init_waitqueue_head (&state->wq);
   state->frame = (struct frame_buffer *) IO_ADDRESS_ISRAM_BASE,        /* <-- hmm, should be ';' not ',' ?? */
   state->lock = SPIN_LOCK_UNLOCKED,                                    /* <-- hmm, should be ';' not ',' ?? */
   
   result = register_chrdev (SOUND_MAJOR, "sound", &audio_fops);
   if (result < 0) {
      printk (DEVICE_NAME ": Can't register major no %d\n", SOUND_MAJOR);
      return result;
   }

   return 0;
}


__initcall(audio_dev_init);



#if 0

static int old_test_open (struct inode *inode, struct file *file)
{
   int minor = MINOR(inode->i_rdev);
   int result;
   unsigned long flags;

   static int count;
   
   static const unsigned int fsdata[9] =
   {
      48000,
      44100,
      32000,
      24000,
      22050,
      16000,
      12000,
      11025,
      8000  
   };

   if (file->f_mode & FMODE_READ)
   {
      /* fixme: fail if someone tries to open audio for reading !! */
   } 
   
   set_audio_samplerate (fsdata[count++]);
   
   if (count >= 9)
      count = 0;


   if ((result = request_irq (INT_ATU, audio_isr, 0, DEVICE_NAME, NULL)) != 0) {
      printk ("%s: could not register interrupt %d\n", DEVICE_NAME, INT_ATU);
      return result;
   }

   local_irq_save (flags);

   SDAC_Registers->ctrl_1 = 0x00000000;
   SDAC_Registers->ctrl_2 = 0x00000000;
   SDAC_Registers->settings = (1 << 12);     /* set bit 12 to configure for 2's complement input */

   SDAC_Registers->bypass = (DIO_BYPASS_DSP | ATU1toIIS1out | ATU1toIIS2out | ATU1toATU1out | ATU2toATU2out);
   ATU_Registers->ATU_config = (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);

   /*
      Trying to write to already full fifos generates data aborts...
      Therefore to be certain of not blowing up, we must check fifo status
      before every write... a great example of crap HW design.
   */

   while ((ATU_Registers->ATU_ch1_wr_status & (ATU_STATUS_LEFT_FULL | ATU_STATUS_RIGHT_FULL)) == 0)
   {
      *ATU_FIFO_LT = (0 << 16);
      *ATU_FIFO_RT = (0 << 16);
   }

   /*
      Clear all ETU interrupts as half full int requests are generated during
      filling as well as draining... (yet) another example of crap HW design.
   */

   ATU_Registers->ETU_int_clear = 0x7fffffff;
   ATU_Registers->ATU_int_enable = ATU_IRQ_WR_CH1L_HALF | ATU_IRQ_WR_CH1L_EMPTY;
   ATU_Registers->ATU_config = ATU_IFLAG_EN | (ATU_WR_CH1L_EN | ATU_WR_CH1R_EN | ATU_WR_CH1_LENGTH);

   local_irq_restore (flags);

   return 0;
}

#endif


