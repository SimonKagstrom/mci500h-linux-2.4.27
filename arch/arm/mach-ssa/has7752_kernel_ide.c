/*
 * linux/arch/arm/mach-ssa/has7752_kernel_ide.c
 *
 * Copyright (C) 2005 Andre McCurdy, Philips Semiconductors.
 *
 * Based on original implementation by Ranjit Deshpande
 *
 * Some code taken from drivers/ide/ppc/pmac.c
 * Copyright (C) 1998-2002 Paul Mackerras & Ben. Herrenschmidt
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <asm/arch/hardware.h>
#include <asm/arch/tsc.h>
#include <asm/arch/gpio.h>              /* to allow control of ata reset line for debugging */
#include <asm/io.h>
#include <asm/uaccess.h>

extern void block_copy_64 (void *dst, void *src);

#if 1
#define DEBUG
#endif

#ifdef DEBUG
static unsigned int debug_level = 1;
#define DBG(l,x...)     do { if (debug_level > l) printk(x); } while (0)
#else
#define DBG(l,x...)     do { /* nothing */ } while (0)
#endif


#define EPLD_ATA_BUF_BASE               (IO_ADDRESS_ATA_BUF_BASE)           /* 2k byte buffer used for all data transfers */
#define EPLD_ATA_BUF_SIZE               (2 * 1024)

#define EPLD_ATA_INT_PENDING            (IO_ADDRESS_ATA_REGS_BASE + 0x00)
#define EPLD_ATA_INT_ENABLE             (IO_ADDRESS_ATA_REGS_BASE + 0x02)
#define EPLD_ATA_BUSY                   (IO_ADDRESS_ATA_REGS_BASE + 0x04)
#define EPLD_ATA_BUFFER_OFFSET          (IO_ADDRESS_ATA_REGS_BASE + 0x06)
#define EPLD_ATA_RELEASE_DELAY          (IO_ADDRESS_ATA_REGS_BASE + 0x08)
#define EPLD_ATA_FIFO_COUNT             (IO_ADDRESS_ATA_REGS_BASE + 0x0A)   /* (read only) */
#define EPLD_ATA_MULTIPLE               (IO_ADDRESS_ATA_REGS_BASE + 0x0C)   /* (read only) hda is lsbyte, hdb is msbyte */
#define EPLD_ATA_WAITSTATE_CTRL         (IO_ADDRESS_ATA_REGS_BASE + 0x0E)   /* set to 0x01 if (clock to EPLD > 40 MHz) */

#define EPLD_ATA_TASKFILE_ERROR         (IO_ADDRESS_ATA_REGS_BASE + 0x10)   /* (Taskfile 0x1F1) (TASKFILE_FEATURE when written)     */
#define EPLD_ATA_TASKFILE_NSECTOR       (IO_ADDRESS_ATA_REGS_BASE + 0x12)   /* (Taskfile 0x1F2)                                     */
#define EPLD_ATA_TASKFILE_SECTOR        (IO_ADDRESS_ATA_REGS_BASE + 0x14)   /* (Taskfile 0x1F3)                                     */
#define EPLD_ATA_TASKFILE_LCYL          (IO_ADDRESS_ATA_REGS_BASE + 0x16)   /* (Taskfile 0x1F4)                                     */
#define EPLD_ATA_TASKFILE_HCYL          (IO_ADDRESS_ATA_REGS_BASE + 0x18)   /* (Taskfile 0x1F5)                                     */
#define EPLD_ATA_TASKFILE_SELECT        (IO_ADDRESS_ATA_REGS_BASE + 0x1A)   /* (Taskfile 0x1F6) (TASKFILE_DEVICE_HEAD when read)    */
#define EPLD_ATA_TASKFILE_STATUS        (IO_ADDRESS_ATA_REGS_BASE + 0x1C)   /* (Taskfile 0x1F7) (TASKFILE_COMMAND when written)     */
#define EPLD_ATA_TASKFILE_CONTROL       (IO_ADDRESS_ATA_REGS_BASE + 0x1E)   /* (Taskfile 0x3F6) (TASKFILE_ALTSTATUS when written)   */

#define EPLD_ATA_TASKFILE_FEATURE       (EPLD_ATA_TASKFILE_ERROR)
#define EPLD_ATA_TASKFILE_COMMAND       (EPLD_ATA_TASKFILE_STATUS)
#define EPLD_ATA_TASKFILE_ALTSTATUS     (EPLD_ATA_TASKFILE_CONTROL)



/*
        Note: the timeout as currently coded isn't much help... if we reach the
        timeout and abort the command, all following commands will fail.
        ie we hang forever in either case...
        
        Therefore make the timeouts nice and big and hope the drive does come
        back eventualy...
*/

#define READ_CMD_TIMEOUT_MSEC   (30 * 1000)
#define WRITE_CMD_TIMEOUT_MSEC  (30 * 1000)


/*******************************************************************************
*******************************************************************************/

static unsigned long long last_command_complete_tsc;

/*******************************************************************************
*******************************************************************************/

#if defined (DEBUG)

static void hexdump (unsigned char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (i % 16 == 0)
            printk("%08x: %02x ", i, buf[i]);
        else
            printk("%02x ", buf[i]);
        if (i != 0 && i % 16 == 0)
            printk("\n");
    }
    printk("\n");
}

static char *port_to_reg_string (unsigned long port, int read)
{
    if (read)
    {
        switch (port)   /* definitions for reading */
        {
            case EPLD_ATA_TASKFILE_ERROR   : return "ERROR  ";
            case EPLD_ATA_TASKFILE_NSECTOR : return "NSECTOR";
            case EPLD_ATA_TASKFILE_SECTOR  : return "SECTOR ";
            case EPLD_ATA_TASKFILE_LCYL    : return "LCYL   ";
            case EPLD_ATA_TASKFILE_HCYL    : return "HCYL   ";
            case EPLD_ATA_TASKFILE_SELECT  : return "SELECT ";
            case EPLD_ATA_TASKFILE_STATUS  : return "STATUS ";
            case EPLD_ATA_TASKFILE_CONTROL : return "CONTROL";
        }
    }
    else
    {
        switch (port)   /* definitions for writing */
        {
            case EPLD_ATA_TASKFILE_ERROR   : return "FEATURE";
            case EPLD_ATA_TASKFILE_NSECTOR : return "NSECTOR";
            case EPLD_ATA_TASKFILE_SECTOR  : return "SECTOR ";
            case EPLD_ATA_TASKFILE_LCYL    : return "LCYL   ";
            case EPLD_ATA_TASKFILE_HCYL    : return "HCYL   ";
            case EPLD_ATA_TASKFILE_SELECT  : return "SELECT ";
            case EPLD_ATA_TASKFILE_STATUS  : return "COMMAND";
            case EPLD_ATA_TASKFILE_CONTROL : return "ALTSTAT";
        }
    }

    return "UNKNOWN";
}

#endif


static void has_OUTB (unsigned char value, unsigned long port)
{
    DBG (3, "OUTB: %s = 0x%02x\n", port_to_reg_string (port, 0), value);

    if ((port == (EPLD_ATA_TASKFILE_COMMAND)) &&
        ((value == WIN_SMART) ||
         (value == WIN_IDENTIFY) || 
         (value == WIN_PIDENTIFY)))
    {
        /*
            Hack to reset HW fifo pointer for non read/write commands
            which return some data...
        */
        outw (0, EPLD_ATA_BUFFER_OFFSET);
    }

    outw (value, port);
}


static void has_OUTBSYNC (ide_drive_t *drive, unsigned char val, unsigned long port)
{
    has_OUTB (val, port);
}


static unsigned char has_INB (unsigned long port)
{
    unsigned int temp;

    DBG (3, "INB : %s", port_to_reg_string (port, 1));

    if (port == (EPLD_ATA_TASKFILE_STATUS)) {
//      DBG (3, " (irq clearing hack required)");
        outw (1, EPLD_ATA_INT_PENDING);
    }

    temp = inw (port);          /* use 16bit read, even for 8bit access */
    DBG (3, " = 0x%02x\n", temp);
    return temp;
}


static ide_startstop_t has_ide_isr (ide_drive_t *drive)
{
    unsigned char status = inw (EPLD_ATA_TASKFILE_STATUS);

    if (!OK_STAT(status, DRIVE_READY, (BAD_R_STAT | BAD_W_STAT)))
        return DRIVER(drive)->error(drive, __FUNCTION__, status);

    DBG (3, "%s : command complete\n", __FUNCTION__);

    outw (1, EPLD_ATA_INT_PENDING);

    return ide_stopped;
}


/*
static int has_dma_timer_expiry (ide_drive_t *drive)
{
    unsigned char status = inw (EPLD_ATA_TASKFILE_STATUS);

    printk ("%s: %s: status == 0x%02x\n", drive->name, __FUNCTION__, status);

    HWGROUP(drive)->expiry = NULL;
    if (!OK_STAT(status, DRIVE_READY, BAD_R_STAT|BAD_W_STAT))
        return -1;
        
    return 0;
}
*/


static int has_ide_dma_read (ide_drive_t *drive)
{
    struct request *rq = HWGROUP(drive)->rq;
    ide_hwif_t *hwif = HWIF(drive);
    task_ioreg_t command;
    ssa_timer_t timer;
    int fifo_head_offset;
    int lba48;
    unsigned char status;

    DBG (1, "%s: %ld from %ld\n", __FUNCTION__, rq->nr_sectors, rq->hard_sector);

    lba48 = (drive->addressing == 1) ? 1 : 0;
    command = ((drive->mult_count) ?
               ((lba48) ? WIN_MULTREAD_EXT : WIN_MULTREAD) :
               ((lba48) ? WIN_READ_EXT : WIN_READ));
    
    /*
        disable ints from this interface, reset the fifo buffer pointer and
        issue cmd to drive with proper locking etc (see ide-iops.c)
    */

    disable_irq (hwif->irq);
    fifo_head_offset = 0;                                   /* we need to manage the EPLD fifo head pointer (ie where we read from) */
    outw (fifo_head_offset, EPLD_ATA_BUFFER_OFFSET);
    ide_execute_command (drive, command, &has_ide_isr, WAIT_WORSTCASE, NULL);
    ssa_timer_set (&timer, READ_CMD_TIMEOUT_MSEC * 1000);
    rq->errors = 0;

    /*
        loop while total sectors remaining for this command != 0
    */

    while (rq->nr_sectors)
    {
        int cur_sectors_orig = rq->current_nr_sectors;
        int cur_sectors = cur_sectors_orig;
        int dst_offset = 0;
        unsigned char *dst;
        unsigned long flags;        /* passed to ide_map_buffer() etc, but not actually used ?? */

        dst = ide_map_buffer (rq, &flags);

//      DBG (3, "%s: dst ptr 0x%08lx\n", __FUNCTION__, (unsigned long) dst);
        DBG (2, "%s: %ld, %d\n", __FUNCTION__, rq->nr_sectors, cur_sectors_orig);

        while (cur_sectors)
        {
            unsigned int bytes_available = inw (EPLD_ATA_FIFO_COUNT);

            if (bytes_available >= 64)
            {
                block_copy_64 (&dst[dst_offset], (unsigned char *) (EPLD_ATA_BUF_BASE + fifo_head_offset));
                dst_offset += 64;
                fifo_head_offset += 64;
                if ((fifo_head_offset % SECTOR_SIZE) == 0)
                {
                    if (fifo_head_offset >= EPLD_ATA_BUF_SIZE)
                        fifo_head_offset -= EPLD_ATA_BUF_SIZE;

                    cur_sectors--;
                    DBG (3, "%s: %d (cur_sectors)\n", __FUNCTION__, cur_sectors);
                }
            }
            else
            {
                /*
                    If there are no bytes available in the EPLD, we have breathing space to
                    double check that everything is still going OK (ie no errors or timeouts).
                    
                    An "end of command" status from the ATA interface here means either that the
                    drive returned an error OR that the command has completed successfully and the
                    remaining data is in the EPLD buffer waiting for us. Be careful not to confuse
                    the two... ;-)
                    
                    ie we check "end of command" and "no more data in EPLD buffer" (in that order !!)
                    before even thinking about checking error status.
                */

                if ((inw (EPLD_ATA_INT_PENDING) & 0x01) &&
                    (inw (EPLD_ATA_FIFO_COUNT) == 0))
                {
#if 1
                    gpio_set_as_output (GPIO_ATA_RESET, 0);	    /* drive low to assert ATA reset */
                    status = inw (EPLD_ATA_TASKFILE_STATUS);
                    panic ("ata_read_dma: early complete: status 0x%02x, cs %d, avail %d\n", status, cur_sectors, inw (EPLD_ATA_FIFO_COUNT));

#else
                    status = inw (EPLD_ATA_TASKFILE_STATUS);
                    printk ("ata_read_dma: early complete: status 0x%02x, cs %d, avail %d\n", status, cur_sectors, inw (EPLD_ATA_FIFO_COUNT));
                    ide_unmap_buffer (dst, &flags);
                    rq->errors = 1;
                    enable_irq (hwif->irq);
#endif
                    goto exit;
                }
            }
        }

        ide_unmap_buffer (dst, &flags);

        rq->current_nr_sectors = 0;
        rq->sector += cur_sectors_orig;
        rq->nr_sectors -= cur_sectors_orig;
        DBG (2, "Completing request for %d sectors\n", cur_sectors_orig);
        DRIVER(drive)->end_request(drive, 1);
    } 

exit:
    enable_irq (hwif->irq);
    last_command_complete_tsc = ReadTSC64();
    return 0;
    
readtimeout:
    status = inw (EPLD_ATA_TASKFILE_STATUS);
    printk ("%s: %s: Timeout, status == 0x%02x\n", drive->name, __FUNCTION__, status);
    goto exit;
}


static int has_ide_dma_write (ide_drive_t *drive)
{
    struct request *rq = HWGROUP(drive)->rq;
    ide_hwif_t *hwif = HWIF(drive);
    task_ioreg_t command;
    ssa_timer_t timer;
    int fifo_tail_offset;
    int lba48;
    unsigned char status;

    DBG (1, "%s: %ld from %ld\n", __FUNCTION__, rq->nr_sectors, rq->hard_sector);

    lba48 = (drive->addressing == 1) ? 1 : 0;
    command = ((drive->mult_count) ?
               ((lba48) ? WIN_MULTWRITE_EXT : WIN_MULTWRITE) :
               ((lba48) ? WIN_WRITE_EXT : WIN_WRITE));
    
    /*
        disable ints from this interface, reset the fifo buffer pointer and
        issue cmd to drive with proper locking etc (see ide-iops.c)
    */

    disable_irq (hwif->irq);
    fifo_tail_offset = 0;                                   /* we need to manage the EPLD fifo tail pointer (ie where we write to) */
    outw (fifo_tail_offset, EPLD_ATA_BUFFER_OFFSET);
    ide_execute_command (drive, command, &has_ide_isr, WAIT_WORSTCASE, NULL);
    ssa_timer_set (&timer, WRITE_CMD_TIMEOUT_MSEC * 1000);
    rq->errors = 0;

    /*
        loop while total sectors remaining for this command != 0
    */

    while (rq->nr_sectors)
    {
        int cur_sectors_orig = rq->current_nr_sectors;
        int cur_sectors = cur_sectors_orig;
        int src_offset = 0;
        unsigned char *src;
        unsigned long flags;        /* passed to ide_map_buffer() etc, but not actually used ?? */

        src = ide_map_buffer (rq, &flags);

//      DBG (3, "%s: dst ptr 0x%08lx\n", __FUNCTION__, (unsigned long) dst);
        DBG (2, "%s: %ld, %d\n", __FUNCTION__, rq->nr_sectors, cur_sectors_orig);

        while (cur_sectors)
        {
            unsigned int space_available = (EPLD_ATA_BUF_SIZE - inw (EPLD_ATA_FIFO_COUNT));

            if (space_available >= 64)
            {
                block_copy_64 ((unsigned char *) (EPLD_ATA_BUF_BASE + fifo_tail_offset), &src[src_offset]);
                src_offset += 64;
                fifo_tail_offset += 64;
                if ((fifo_tail_offset % SECTOR_SIZE) == 0)
                {
                    if (fifo_tail_offset >= EPLD_ATA_BUF_SIZE)
                        fifo_tail_offset -= EPLD_ATA_BUF_SIZE;

                    cur_sectors--;
                    DBG (3, "%s: %d (cur_sectors)\n", __FUNCTION__, cur_sectors);
                }
            }
            else
            {
                /*
                    If there is no space available in the EPLD, we have breathing space to
                    double check that everything is still going OK (ie no errors or timeouts).
                    
                    An "end of command" status from the ATA interface here can only mean that the
                    drive returned an error.
                */

                if (inw (EPLD_ATA_INT_PENDING) & 0x01)
                {
#if 1
                    gpio_set_as_output (GPIO_ATA_RESET, 0);	    /* drive low to assert ATA reset */
                    status = inw (EPLD_ATA_TASKFILE_STATUS);
                    panic ("ata_write_dma: early complete: status 0x%02x, cs %d, fifocount %d\n", status, cur_sectors, inw (EPLD_ATA_FIFO_COUNT));

#else
                    status = inw (EPLD_ATA_TASKFILE_STATUS);
                    printk ("ata_write_dma: early complete: status 0x%02x, cs %d, fifocount %d\n", status, cur_sectors, inw (EPLD_ATA_FIFO_COUNT));
                    ide_unmap_buffer (src, &flags);
                    rq->errors = 1;
                    enable_irq (hwif->irq);
#endif
                    goto exit;
                }
            }
        }

        ide_unmap_buffer (src, &flags);

        rq->current_nr_sectors = 0;
        rq->sector += cur_sectors_orig;
        rq->nr_sectors -= cur_sectors_orig;
        DBG (2, "Completing request for %d sectors\n", cur_sectors_orig);
        DRIVER(drive)->end_request(drive, 1);
    } 

exit:
    enable_irq (hwif->irq);
    last_command_complete_tsc = ReadTSC64();
    return 0;
    
writetimeout:
    status = inw (EPLD_ATA_TASKFILE_STATUS);
    printk ("%s: %s: Timeout, status == 0x%02x\n", drive->name, __FUNCTION__, status);
    goto exit;
}


static void has_ata_input_data (ide_drive_t *drive, void *buf, unsigned int count)
{
    unsigned short * epld_buffer = (unsigned short *) EPLD_ATA_BUF_BASE;
    ide_hwif_t *hwif = HWIF(drive);
    
    disable_irq (hwif->irq);
    memcpy (buf, epld_buffer, count << 2);
    enable_irq (hwif->irq);
}


static void has_ata_output_data (ide_drive_t *drive, void *buf, unsigned int count)
{
    unsigned short * epld_buffer = (unsigned short *) EPLD_ATA_BUF_BASE;
    ide_hwif_t *hwif = HWIF(drive);
    
    disable_irq (hwif->irq);
    outw (0, EPLD_ATA_BUFFER_OFFSET);
    memcpy (buf, epld_buffer, count << 2);
    enable_irq (hwif->irq);
}


static int has_ide_dma_begin(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_end(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_check(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_on(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_off(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_off_quietly(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_test_irq(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_host_on(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_host_off(ide_drive_t *drive)
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_bad_drive(ide_drive_t *drive)
{
        /* No question of doing real DMA, so no drive is bad */
        return 0;
}

static int has_ide_dma_good_drive(ide_drive_t *drive)
{
        /* No question of doing real DMA, so all drives are good */
        return 1;
}

static int has_ide_dma_count(ide_drive_t *drive)
{
        /* No question of doing real DMA, so all drives are good */
        return 0;
}

static int has_ide_dma_verbose(ide_drive_t *drive) 
{
//      printk(", PIO-4");
        return 0;
}

static int has_ide_dma_retune(ide_drive_t *drive) 
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_lostirq(ide_drive_t *drive) 
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

static int has_ide_dma_timeout(ide_drive_t *drive) 
{
        /* Do nothing since this is a fake DMA driver */
        return 0;
}

/*******************************************************************************
*******************************************************************************/

#ifdef CONFIG_PROC_FS

static int proc_ata_idle_seconds_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   char *p = page;
   unsigned long long now = ReadTSC64();
   unsigned int idle_seconds = (now - last_command_complete_tsc) / (ReadTSCMHz() * 1000000);
   
   p += sprintf (p, "%u\n", idle_seconds);
   
   if ((len = (p - page) - off) < 0)
      len = 0;

   *eof = (len <= count) ? 1 : 0;
   *start = page + off;

   return len;
}


#if defined (DEBUG)

static int proc_ata_debug_level_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   char *p = page;
   
   p += sprintf (p, "%u\n", debug_level);
   
   if ((len = (p - page) - off) < 0)
      len = 0;

   *eof = (len <= count) ? 1 : 0;
   *start = page + off;

   return len;
}

static int proc_ata_debug_level_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
   char sbuf[16 + 1];
   int value;
   int len;

   if (count > 0) {
      len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
      memset (sbuf, 0, sizeof(sbuf));
      if (copy_from_user (sbuf, buffer, len))
         return -EFAULT;
      if (sscanf (sbuf, "%u", &value) == 1)
         debug_level = value;
   }

   return count;
}

#endif


static int __init proc_init (void)
{
   struct proc_dir_entry *res;

   if ((res = create_proc_entry ("ata_idle_seconds", S_IRUGO, NULL)) == NULL)
      return -ENOMEM;

   res->read_proc = proc_ata_idle_seconds_read;

#if defined (DEBUG)

   if ((res = create_proc_entry ("ata_debug_level", (S_IWUSR | S_IRUGO), NULL)) == NULL)
      return -ENOMEM;

   res->read_proc = proc_ata_debug_level_read;
   res->write_proc = proc_ata_debug_level_write;

#endif

   return 0;
}

#else

static int __init proc_init (void)
{
   return 0;
}

#endif


void __init ide_init_default_hwifs (void)
{
    int i;
    hw_regs_t hw;           
    ide_hwif_t *hwif;

    if ((inl(IO_ADDRESS_EPLD_ID_BASE) & 0xFF) < 11) 
        panic ("EPLD image is too old. Please upgrade.");

    /* Do not reset the ATA individually since this will reset the
     * USB block as well. We expect the bootloader to do all the resets.
     *
     * writew (EPLD_SYSCONTROL_ATA_RESET, IO_ADDRESS_SYSCONTROL_BASE);
     *
     */

    outw (0, EPLD_ATA_BUFFER_OFFSET);
    outw (4096, EPLD_ATA_RELEASE_DELAY);

    memset (&hw, 0, sizeof(hw_regs_t));

    hw.io_ports[IDE_DATA_OFFSET   ] = (ide_ioreg_t) (EPLD_ATA_BUF_BASE);

    hw.io_ports[IDE_ERROR_OFFSET  ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_ERROR  );    /* Feature reg when written */
    hw.io_ports[IDE_NSECTOR_OFFSET] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_NSECTOR);
    hw.io_ports[IDE_SECTOR_OFFSET ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_SECTOR );
    hw.io_ports[IDE_LCYL_OFFSET   ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_LCYL   );
    hw.io_ports[IDE_HCYL_OFFSET   ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_HCYL   );
    hw.io_ports[IDE_SELECT_OFFSET ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_SELECT );    /* DeviceHead reg when read */
    hw.io_ports[IDE_STATUS_OFFSET ] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_STATUS );    /* Command reg when written */
    hw.io_ports[IDE_CONTROL_OFFSET] = (ide_ioreg_t) (EPLD_ATA_TASKFILE_CONTROL);    /* AltStatus reg when read  */

    hw.irq = INT_ATA;

    ide_register_hw (&hw, &hwif);

    /*
        If we want to _disable_ 48bit LBA addressing for this interface (which
        we don't as the interface handles it fine...) we would set
        hwif->addressing to 1 here.
        
        Note that (hwif->addressing == 1) indicates that the interface does not
        support 48bit LBA, whereas (drive->addressing == 1) indicates that the
        drive _does_ support 48bit LBA (and will only be true if hwif->addressing
        is 0) !!.
    */

//  hwif->addressing = 1;       /* set to 1 to disable 48bit LBA addressing for this interface (ie master and slave) */
//  hwif->rqsize = 128;         /* set to less than 128 to force a smaller than normal max request size for this interface */

    /* Init ATA PIO functions */
    hwif->ata_input_data        = has_ata_input_data;
    hwif->ata_output_data       = has_ata_output_data;

    /* Init DMA operations */
    hwif->ide_dma_read          = has_ide_dma_read;
    hwif->ide_dma_write         = has_ide_dma_write;
    hwif->ide_dma_begin         = has_ide_dma_begin;
    hwif->ide_dma_end           = has_ide_dma_end;
    hwif->ide_dma_check         = has_ide_dma_check;
    hwif->ide_dma_on            = has_ide_dma_on;
    hwif->ide_dma_off           = has_ide_dma_off;
    hwif->ide_dma_off_quietly   = has_ide_dma_off_quietly;
    hwif->ide_dma_test_irq      = has_ide_dma_test_irq;
    hwif->ide_dma_host_on       = has_ide_dma_host_on;
    hwif->ide_dma_host_off      = has_ide_dma_host_off;
    hwif->ide_dma_bad_drive     = has_ide_dma_bad_drive;
    hwif->ide_dma_good_drive    = has_ide_dma_good_drive;
    hwif->ide_dma_count         = has_ide_dma_count;
    hwif->ide_dma_verbose       = has_ide_dma_verbose;
    hwif->ide_dma_retune        = has_ide_dma_retune;
    hwif->ide_dma_lostirq       = has_ide_dma_lostirq;
    hwif->ide_dma_timeout       = has_ide_dma_timeout;

    /* Init I/O functions */
    hwif->OUTB                  = has_OUTB;
    hwif->OUTBSYNC              = has_OUTBSYNC;
//  hwif->OUTW                  = ide_outw;
//  hwif->OUTL                  = ide_outl;
//  hwif->OUTSW                 = ide_outsw;
//  hwif->OUTSL                 = ide_outsl;
    hwif->INB                   = has_INB;
//  hwif->INW                   = ide_inw;
//  hwif->INL                   = ide_inl;
//  hwif->INSW                  = ide_insw;
//  hwif->INSL                  = ide_insl;

    /* Force DMA on */
    for (i = 0; i < MAX_DRIVES; i++) {
        hwif->drives[i].using_dma = 1;
        hwif->drives[i].special.b.set_multmode = 0;
        hwif->drives[i].special.b.set_geometry = 0;
        hwif->drives[i].special.b.recalibrate = 0;
        hwif->drives[i].mult_req = hwif->drives[i].mult_count = 0;
    }

    proc_init();
}

