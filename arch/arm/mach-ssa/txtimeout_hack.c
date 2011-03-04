/*
 * linux/arch/arm/mach-ssa/txtimeout_hack.c
 *
 * Copyright (C) 2005 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/*
   Ugly hack to notify user space that the Agere WiFi driver has died
   and needs to be reloaded. Put this code outside of the Agere driver
   module to try to avoid problems of reading the proc entry when the
   driver has been unloaded...
*/
   
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <asm/arch/txtimeout_hack.h>


/*****************************************************************************
*****************************************************************************/

#if defined (CONFIG_SSA_HAS7752) && \
    defined (CONFIG_PROC_FS)

static unsigned int txtimeout_count;

void txtimeout_hack_init (unsigned int value)
{
    unsigned long flags;
    
    local_irq_save (flags);
    txtimeout_count = value;
    local_irq_restore (flags);
}

void txtimeout_hack_inc (void)
{
    unsigned long flags;
    
    local_irq_save (flags);
    txtimeout_count++;
    local_irq_restore (flags);
}


static int txtimeout_hack_proc_read (char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    *eof = 1;
    return sprintf (buf, "%u\n", txtimeout_count);
}

static int txtimeout_hack_proc_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
   char sbuf[16 + 1];
   unsigned int value;
   int len;

   if (count > 0) {
      len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
      memset (sbuf, 0, sizeof(sbuf));
      if (copy_from_user (sbuf, buffer, len))
         return -EFAULT;
      if (sscanf (sbuf, "%u", &value) == 1)
         txtimeout_hack_init (value);
   }

   return count;
}

static int __init txtimeout_hack_proc_init (void)
{
    struct proc_dir_entry *res;

    txtimeout_hack_init (0);

    if ((res = create_proc_entry ("txtimeout_hack", (S_IWUSR | S_IRUGO), NULL)) == NULL)
       return -ENOMEM;

    res->read_proc = txtimeout_hack_proc_read;
    res->write_proc = txtimeout_hack_proc_write;

    return 0;
}


__initcall (txtimeout_hack_proc_init);

EXPORT_SYMBOL(txtimeout_hack_init);
EXPORT_SYMBOL(txtimeout_hack_inc);


/*****************************************************************************
*****************************************************************************/

#endif

