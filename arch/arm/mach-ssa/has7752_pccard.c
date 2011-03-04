/*
 * linux/arch/arm/mach-ssa/has7752_pccard.c
 *
 * Copyright (C) 2004 Andre McCurdy, Philips Semiconductors.
 *
 * Based on original implementation by Ranjit Deshpande
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/types.h>
#include <asm/errno.h>
#include <asm/uaccess.h>

#include <asm/arch/has7752_pccard.h>
#include <asm/arch/gpio.h>
#include <asm/arch/tsc.h>


#define DEBUG_LEVEL                     (1)

#define PCCARD_FIFO_SIZE                (2 * 1024)                              /* size in bytes */

#define INT_PENDING_REG                 (IO_ADDRESS_PCCARD_REGS_BASE + 0x00)    /* Pending interrupts */
#define INT_ENABLE_REG                  (IO_ADDRESS_PCCARD_REGS_BASE + 0x02)    /* Interrupt mask */
#define STATUS_REG                      (IO_ADDRESS_PCCARD_REGS_BASE + 0x04)    /* Transaction status */
#define MEM_SPEED_REG                   (IO_ADDRESS_PCCARD_REGS_BASE + 0x06)    /* Common memory mode accesses */
#define IO_SPEED_REG                    (IO_ADDRESS_PCCARD_REGS_BASE + 0x08)    /* IO mode accesses */
#define FIFO_LOW_LIMIT_REG              (IO_ADDRESS_PCCARD_REGS_BASE + 0x10)    /* Low threshold */
#define FIFO_HIGH_LIMIT_REG             (IO_ADDRESS_PCCARD_REGS_BASE + 0x12)    /* High threshold */
#define FIFO_COUNT_REG                  (IO_ADDRESS_PCCARD_REGS_BASE + 0x14)    /* Number of bytes currently in fifo */
#define ADDR_LOW_REG                    (IO_ADDRESS_PCCARD_REGS_BASE + 0x16)    /* Lower byte of mem address */
#define ADDR_HIGH_REG                   (IO_ADDRESS_PCCARD_REGS_BASE + 0x18)    /* Upper 2 bits of mem addr */
#define TRANSFER_COUNT_REG              (IO_ADDRESS_PCCARD_REGS_BASE + 0x1A)    /* Length of transaction (bytes) */
#define CTRL_REG                        (IO_ADDRESS_PCCARD_REGS_BASE + 0x1C)    /* Control bits */
#define FIFO_DATA_REG                   (IO_ADDRESS_PCCARD_REGS_BASE + 0x40)    /* Data port (mapped upto offset 0x80) */

#define STATUS_READ_COMPLETE            (1 << 0)
#define STATUS_WRITE_IN_PROGRESS        (1 << 1)
#define STATUS_AUTO_XFER_IN_PROGRESS    (1 << 2)

#define CTRL_READ                       (1 << 0)
#define CTRL_WRITE                      (0 << 0)
#define CTRL_IO                        ((1 << 1) | (0 << 5))
#define CTRL_MEM                       ((1 << 1) | (1 << 5))
#define CTRL_ATTR                      ((0 << 1) | (0 << 5))                    /* bit1 overrides (ie bit5 is don't care) */
#define CTRL_POSTINC                    (1 << 2)
#define CTRL_RUN                        (1 << 3)
#define CTRL_FIFO_RESET                 (1 << 4)
#define CTRL_WORD_SWAP                  (1 << 6)                                /* swap 16bit words within each 32bit value transferred via the fifo */



#define poll_for_single_read_complete()     { while ((readw (STATUS_REG) & STATUS_READ_COMPLETE        ) == 0) ; }
#define poll_for_single_write_complete()    { while ((readw (STATUS_REG) & STATUS_WRITE_IN_PROGRESS    ) != 0) ; }
#define poll_for_multi_complete()           { while ((readw (STATUS_REG) & STATUS_AUTO_XFER_IN_PROGRESS) != 0) ; }

#if 1
  #define DISABLE_INTERRUPTS()          { (void) flags; disable_irq (INT_PCMCIA); }
  #define ENABLE_INTERRUPTS()           enable_irq (INT_PCMCIA)
  #define TIMEOUT_USEC(bytecount)       (512 * 1024)
#else
  #define DISABLE_INTERRUPTS()          local_irq_save (flags)
  #define ENABLE_INTERRUPTS()           local_irq_restore (flags)
  #define TIMEOUT_USEC(bytecount)       (bytecount * 16)
#endif


static unsigned int saved_waitstates_mem = 0xFFFF;
static unsigned int saved_waitstates_io = 0xFFFF;
static unsigned int word_swap;


static void softreset (void)
{
   writew (EPLD_SYSCONTROL_PCCARD_RESET, IO_ADDRESS_SYSCONTROL_BASE);
   writew (saved_waitstates_mem, MEM_SPEED_REG);
   writew (saved_waitstates_io, IO_SPEED_REG);
}


/*******************************************************************************
*******************************************************************************/

void has7752_pccard_reset_assert (void)
{
   gpio_set_as_output (GPIO_PCMCIA_RESET, 1);	/* assert PCMCIA reset */
}

void has7752_pccard_reset_deassert (void)
{
   gpio_set_as_output (GPIO_PCMCIA_RESET, 0);	/* de-assert PCMCIA reset */
}

int has7752_pccard_read_ready (void)
{
   return gpio_get_value (GPIO_PCMCIA_INTRQ);
}

int has7752_pccard_get_irq (void)
{
   return INT_PCMCIA;
}

int has7752_pccard_init (void)
{
   writew (EPLD_SYSCONTROL_PCCARD_RESET, IO_ADDRESS_SYSCONTROL_BASE);

   has7752_pccard_set_mem_waitstates (15,31,7);

   /*
      Set default IO mode waitstates.
      (2,7,0) should be safe for pc-card assuming an EBI bus speed of 50 MHz.
      (0,2,0) should be safe for Ti TNET1130 NDC card assuming an EBI bus speed of 50 MHz.
      
      To be safe for all cards, use the slower pc-card settings...
      
      Update, 14 Feb 2005... It seems that the USI Agere card is not 100% reliable at
      pc-card settings - ie (2,7,0). Failures were seen when reading from the card
      at 40.5 MHz EBI bus clock. Adding an extra setup waitstate seems to solve the
      problem - although from the Agere datasheet, it shouldn't be needed.

      Update, 19 Feb 2005... After more testing, all Agere cards now seem to be
      fine at (1,2,0) and 48 MHz EBI !! (ie previous failure can't be reproduced).
      This setting is more in line with the timing that the Agere card _should_ be
      able to handle, so try using it for a while and see if anything breaks... !!

      Update, 24 Feb 2005... Use glacial waitstates by default.
      It seems we have HW problems with some boards (??).

      Fixme: If we no longer care about ZyDAS or NDC cards, it may be possible to reduce
      the 7 data strobe waitstates a bit.
   */

// has7752_pccard_set_io_waitstates (2,7,0);        /* (2,7,0) too fast for Agere card at 40.5 MHz EBI */
// has7752_pccard_set_io_waitstates (3,7,0);        /* (3,7,0) fixed previous Agere problems... */
   has7752_pccard_set_io_waitstates (1,2,0);        /* (1,2,0) seems stable now - even at 48 MHz EBI... */

// has7752_pccard_set_io_waitstates (5,10,2);       /* glacial... but may help dodgy boards (?) */
   
   has7752_pccard_word_swap_disable();

   return 0;
}

void has7752_pccard_set_mem_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws)
{
   if (address_setup_ws > MAX_ADDRESS_SETUP_WAITSTATES) {
      printk ("%s: address_setup_ws %d out of range\n", __FUNCTION__, address_setup_ws);
      address_setup_ws = MAX_ADDRESS_SETUP_WAITSTATES;
   }
   if (data_strobe_ws > MAX_DATA_STROBE_WAITSTATES) {
      printk ("%s: data_strobe_ws %d out of range\n", __FUNCTION__, data_strobe_ws);
      data_strobe_ws = MAX_DATA_STROBE_WAITSTATES;
   }
   if (address_hold_ws > MAX_ADDRESS_HOLD_WAITSTATES) {
      printk ("%s: address_hold_ws %d out of range\n", __FUNCTION__, address_hold_ws);
      address_hold_ws = MAX_ADDRESS_HOLD_WAITSTATES;
   }

   saved_waitstates_mem = ((address_setup_ws<<8)|(data_strobe_ws<<3)|(address_hold_ws<<0));

   if (DEBUG_LEVEL > 1)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);

   writew (saved_waitstates_mem, MEM_SPEED_REG);
}

void has7752_pccard_set_io_waitstates (int address_setup_ws, int data_strobe_ws, int address_hold_ws)
{
#if 0
   address_setup_ws = 0;
   data_strobe_ws = 2;
   address_hold_ws = 0;
#endif

   if (address_setup_ws > MAX_ADDRESS_SETUP_WAITSTATES) {
      printk ("%s: address_setup_ws %d out of range\n", __FUNCTION__, address_setup_ws);
      address_setup_ws = MAX_ADDRESS_SETUP_WAITSTATES;
   }
   if (data_strobe_ws > MAX_DATA_STROBE_WAITSTATES) {
      printk ("%s: data_strobe_ws %d out of range\n", __FUNCTION__, data_strobe_ws);
      data_strobe_ws = MAX_DATA_STROBE_WAITSTATES;
   }
   if (address_hold_ws > MAX_ADDRESS_HOLD_WAITSTATES) {
      printk ("%s: address_hold_ws %d out of range\n", __FUNCTION__, address_hold_ws);
      address_hold_ws = MAX_ADDRESS_HOLD_WAITSTATES;
   }

   saved_waitstates_io = ((address_setup_ws<<8)|(data_strobe_ws<<3)|(address_hold_ws<<0));

   if (DEBUG_LEVEL > 0)
      printk ("%s: (%d, %d, %d)\n", __FUNCTION__, address_setup_ws, data_strobe_ws, address_hold_ws);

   writew (saved_waitstates_io, IO_SPEED_REG);
}

int has7752_pccard_word_swap_enable (void)
{
    if ((inl(IO_ADDRESS_EPLD_ID_BASE) & 0xFF) < 19) {
        printk ("EPLD image is too old to support word_swap mode !! Please upgrade");
        return -ENODEV;
    }
    
    word_swap = CTRL_WORD_SWAP;
    return 0;
}

int has7752_pccard_word_swap_disable (void)
{
    word_swap = 0;
    return 0;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read the single 8bit value from PCMCIA attribute memory space at offset 'addr'.
*/
unsigned int has7752_pccard_attribute_read (unsigned int addr)
{
   int timeout;
   unsigned int result;
   unsigned long flags;
   
   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_ATTR | CTRL_READ), CTRL_REG);
   readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_READ_COMPLETE) == 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   result = readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%08x, result = 0x%02x\n", __FUNCTION__, addr, result);

   return result;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Write the single 8bit value 'value' to PCMCIA attribute memory space at offset 'addr'.
*/
unsigned int has7752_pccard_attribute_write (unsigned int addr, unsigned char value)
{
   int timeout;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s: addr = 0x%08x, value = 0x%02x\n", __FUNCTION__, addr, value);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_ATTR | CTRL_WRITE), CTRL_REG);
   writew (value, IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_WRITE_IN_PROGRESS) != 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   ENABLE_INTERRUPTS();
   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout writing to 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read a single 16bit value from PCMCIA IO space at offset 'addr'.
*/
unsigned int has7752_pccard_io_read (unsigned int addr)
{
   int timeout;
   unsigned int result;
   unsigned long flags;

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_IO | CTRL_READ), CTRL_REG);
   readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_READ_COMPLETE) == 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   result = readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   ENABLE_INTERRUPTS();

   if (DEBUG_LEVEL > 2)
      printk ("%s : addr = 0x%08x, result = 0x%04x\n", __FUNCTION__, addr, result);

   return result;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Write the single 16bit value 'value' to PCMCIA IO space at offset 'addr'.
*/
unsigned int has7752_pccard_io_write (unsigned int addr, unsigned short value)
{
   int timeout;
   unsigned long flags;

   if (DEBUG_LEVEL > 2)
      printk ("%s: addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, value);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_IO | CTRL_WRITE), CTRL_REG);
   writew (value, IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_WRITE_IN_PROGRESS) != 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   ENABLE_INTERRUPTS();
   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Reads 'count' bytes from PCMCIA IO space at offset 'addr' into 'buffer'.
   Assumes indirect access mode with auto-increment in target.

   (The Agere driver uses this function).
*/
unsigned int has7752_pccard_io_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1, temp2;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned int delta;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (count == 0)
      return 0;

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (((unsigned long) buffer) & 0x01)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : dst buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped IO mode yet...\n", __FUNCTION__);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew (((count + 1) & ~0x01), TRANSFER_COUNT_REG);
   writew ((CTRL_IO | CTRL_READ | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_IO | CTRL_READ | CTRL_RUN), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

#if 1

   while (1)
   {
      unsigned int available;

      if (count < 2)
         break;

      if ((available = readw (FIFO_COUNT_REG)) != 0) {
         if (DEBUG_LEVEL > 1) {
            if (available & 0x01)
               printk ("%s: odd number of bytes available in fifo (%d) ?!?\n", __FUNCTION__, available);
            if (available > (count + 1))
               printk ("%s: fifo bytecount (%d) greater than remaining transfer size (%d)?!?\n", __FUNCTION__, available, count);
         }
         insw (FIFO_DATA_REG, buffer, (available / 2));
         buffer += (available / 2);
         count -= available;
         continue;
      }
      
      if (ssa_timer_expired (&timer))
         goto timedout;
   }

#else

   while (count >= 2) {                         /* while we require more data */
      if (readw (FIFO_COUNT_REG) >= 2) {        /* if there is data available */
         *buffer++ = readw (FIFO_DATA_REG);     /* read from the fifo */
         count -= 2;
      }
      else {
         if (ssa_timer_expired (&timer))
            goto timedout;
      }
   }

#endif

   if (count != 0)
      *((unsigned char *) buffer) = (readw (FIFO_DATA_REG) & 0xFF);

   if (DEBUG_LEVEL > 2)
      delta = ssa_timer_ticks (&timer);

   if (DEBUG_LEVEL > 1) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s : xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   if (DEBUG_LEVEL > 2)
      printk ("%s : count = %4d, usec = %4d\n", __FUNCTION__, count_initial, (delta / ReadTSCMHz()));

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%04x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}


/*
   Write 'count' bytes from 'buffer' into PCMCIA IO space at offset 'addr'.
   Assumes indirect access mode with auto-increment in target.

   (The Agere driver uses this function).
*/
unsigned int has7752_pccard_io_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
   ssa_timer_t timer;
   int count_initial = count;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s: addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (count == 0)
      return 0;

   if (unlikely (count > PCCARD_FIFO_SIZE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: byte count (%d) must be %d or less\n", __FUNCTION__, count, PCCARD_FIFO_SIZE);
      return -1;
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (((unsigned long) buffer) & 0x01)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: src buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped IO mode yet...\n", __FUNCTION__);
      return -1;
   }

   count = ((count + 1) & ~0x01);

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew ((count), TRANSFER_COUNT_REG);
   writew ((CTRL_IO | CTRL_WRITE | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_IO | CTRL_WRITE | CTRL_RUN), CTRL_REG);

#if 1

   outsw (FIFO_DATA_REG, buffer, (count / 2));              /* outsw() will panic if src buffer is not 16bit aligned... */

#else

   {
      unsigned int temp1 = 0, temp2;
      unsigned long ibufptr = (unsigned long) buffer;
   
      switch (ibufptr & 0x03)
      {
         case 0:
            while (count >= 8) {
               typedef struct { unsigned int t1; unsigned int t2; } ds_t;
               ds_t ds;
               ds = *((ds_t *) ibufptr);
               ibufptr += 8;
               *((ds_t *) FIFO_DATA_REG) = ds;
               count -= 8;
            }
            if (count) {
               temp1 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               if (count > 4) {
                  writel (temp1, FIFO_DATA_REG);
                  temp1 = *((unsigned int *) ibufptr);
                  ibufptr += 4;
                  count -= 4;
               }
            }
            break;
         
         case 1:
            temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 8);
            ibufptr += 3;
   
            while (count > 4) {
               temp2 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               count -= 4;
               temp1 |= (temp2 << 24);
               writel (temp1, FIFO_DATA_REG);
               temp1 = (temp2 >> 8);
            }
   
            temp1 |= (*((unsigned int *) ibufptr) << 24);
            break;
   
         case 2:
            temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 16);
            ibufptr += 2;
      
            while (count > 4) {
               temp2 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               count -= 4;
               temp1 |= (temp2 << 16);
               writel (temp1, FIFO_DATA_REG);
               temp1 = (temp2 >> 16);
            }
      
            temp1 |= (*((unsigned int *) ibufptr) << 16);
            break;
   
         case 3:
            temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 24);
            ibufptr += 1;
      
            while (count > 4) {
               temp2 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               count -= 4;
               temp1 |= (temp2 << 8);
               writel (temp1, FIFO_DATA_REG);
               temp1 = (temp2 >> 24);
            }
      
            temp1 |= (*((unsigned int *) ibufptr) << 8);
            break;
         
         default:
            break;
      }
      
      while (count > 0) {
         writew (temp1, FIFO_DATA_REG);
         count -= 2;
         temp1 >>= 16;
      }
   }

#endif

   ssa_timer_set (&timer, TIMEOUT_USEC(count_initial));                 /* scale timeout depending on amount of data being written */

   while ((readw (STATUS_REG) & STATUS_AUTO_XFER_IN_PROGRESS) != 0)
      if (ssa_timer_expired (&timer))
         goto timedout;

   if (DEBUG_LEVEL > 0) {
      unsigned int temp1 = readw (TRANSFER_COUNT_REG);
      unsigned int temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s: xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Reads 'count' bytes from PCMCIA IO space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
   Data is written into 'buffer'.
*/
unsigned int has7752_pccard_io_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1, temp2;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s : addr:0x%05x, count:%4d, buf:0x%08lx\n", __FUNCTION__, addr, count, (unsigned long) buffer);

   if (count == 0)
      return 0;

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped IO mode yet...\n", __FUNCTION__);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew (((count + 1) & ~0x01), TRANSFER_COUNT_REG);
   writew ((CTRL_IO | CTRL_READ | CTRL_POSTINC | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_IO | CTRL_READ | CTRL_POSTINC | CTRL_RUN), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

   while (count >= 2) {                         /* while we require more data */
      if (readw (FIFO_COUNT_REG) >= 2) {        /* if there is data available */
         *buffer++ = readw (FIFO_DATA_REG);     /* read from the fifo */
         count -= 2;
      }
      else {
         if (ssa_timer_expired (&timer))
            goto timedout;
      }
   }

   if (count != 0)
      *((unsigned char *) buffer) = (readw (FIFO_DATA_REG) & 0xFF);

   if (DEBUG_LEVEL > 0) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s : xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}


/*
   Writes 'count' bytes from buffer into PCMCIA IO space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
*/
unsigned int has7752_pccard_io_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1 = 0, temp2;
   unsigned long ibufptr = (unsigned long) buffer;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s: addr:0x%05x, count:%4d, buf:0x%08lx\n", __FUNCTION__, addr, count, ibufptr);

   if (count == 0)
      return 0;

   if (unlikely (count > PCCARD_FIFO_SIZE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: byte count (%d) must be %d or less\n", __FUNCTION__, count, PCCARD_FIFO_SIZE);
      return -1;
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped IO mode yet...\n", __FUNCTION__);
      return -1;
   }

   /*
      We must always write a multiple of 16bits to the card (a HW limitation).
      Warning: If the original count is odd, one extra byte of data from the
               end of 'buffer' and written into the card !!
   */

   count = ((count + 1) & ~0x01);               /* round up count to a multiple of 2 bytes */

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew ((count), TRANSFER_COUNT_REG);
   writew ((CTRL_IO | CTRL_WRITE | CTRL_POSTINC | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_IO | CTRL_WRITE | CTRL_POSTINC | CTRL_RUN), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

   switch (ibufptr & 0x03)
   {
      case 0:
#if 0
         temp1 = *((unsigned int *) ibufptr);
         ibufptr += 4;

         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            writel (temp1, FIFO_DATA_REG);
            temp1 = temp2;
         }
#elif 0
         temp1 = *((unsigned int *) ibufptr);
         ibufptr += 4;

         while (count > 8) {
            writel (temp1, FIFO_DATA_REG);
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            writel (temp2, FIFO_DATA_REG);
            count -= 8;
         }
         if (count > 4) {
            writel (temp1, FIFO_DATA_REG);
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
         }
#else
         while (count >= 8) {
            typedef struct { unsigned int t1; unsigned int t2; } ds_t;
            ds_t ds;
            ds = *((ds_t *) ibufptr);
            ibufptr += 8;
            *((ds_t *) FIFO_DATA_REG) = ds;
            count -= 8;
         }
         if (count) {
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            if (count > 4) {
               writel (temp1, FIFO_DATA_REG);
               temp1 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               count -= 4;
            }
         }
#endif
         break;
      
      case 1:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 8);
         ibufptr += 3;

         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 24);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 8);
         }

         temp1 |= (*((unsigned int *) ibufptr) << 24);
         break;

      case 2:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 16);
         ibufptr += 2;
   
         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 16);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 16);
         }
   
         temp1 |= (*((unsigned int *) ibufptr) << 16);
         break;

      case 3:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 24);
         ibufptr += 1;
   
         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 8);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 24);
         }
   
         temp1 |= (*((unsigned int *) ibufptr) << 8);
         break;
      
      default:
         break;
   }
   
   while (count > 0) {
      writew (temp1, FIFO_DATA_REG);
      count -= 2;
      temp1 >>= 16;
   }

   while ((readw (STATUS_REG) & STATUS_AUTO_XFER_IN_PROGRESS) != 0)
      if (ssa_timer_expired (&timer))
         goto timedout;

   if (DEBUG_LEVEL > 0) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s: xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}


/*******************************************************************************
*******************************************************************************/

/*
   Read a single 16bit value from PCMCIA Common Memory space at offset 'addr'.
*/
unsigned int has7752_pccard_mem_read (unsigned int addr)
{
   int timeout;
   unsigned int result;
   unsigned long flags;

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_MEM | CTRL_READ), CTRL_REG);
   readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_READ_COMPLETE) == 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   result = readw (IO_ADDRESS_PCCARD_PF_BASE + addr);
   ENABLE_INTERRUPTS();

   if (DEBUG_LEVEL > 2)
      printk ("%s : addr = 0x%08x, result = 0x%04x\n", __FUNCTION__, addr, result);

   return result;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Write the single 16bit value 'value' to PCMCIA Common Memory space at offset 'addr'.
*/
unsigned int has7752_pccard_mem_write (unsigned int addr, unsigned short value)
{
   int timeout;
   unsigned long flags;

   if (DEBUG_LEVEL > 2)
      printk ("%s: addr = 0x%04x, value = 0x%04x\n", __FUNCTION__, addr, value);

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   DISABLE_INTERRUPTS();
   writew ((CTRL_MEM | CTRL_WRITE), CTRL_REG);
   writew (value, IO_ADDRESS_PCCARD_PF_BASE + addr);
   timeout = 32;
   while (--timeout && ((readw (STATUS_REG) & STATUS_WRITE_IN_PROGRESS) != 0))
      udelay (1);
   if (timeout == 0)
      goto timedout;
   ENABLE_INTERRUPTS();
   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%08x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Reads 'count' bytes from PCMCIA Common Memory space at offset 'addr' into 'buffer'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_read_multi (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1, temp2;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned int delta;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s : addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (count == 0)
      return 0;

   if (unlikely (word_swap && (count & 0x3))) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : count (%d) must be a multiple of 4 bytes if word swap is enabled\n", __FUNCTION__, count);
      return -1;
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (((unsigned long) buffer) & (word_swap ? 0x03 : 0x01))) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : dst buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew (((count + 1) & ~0x01), TRANSFER_COUNT_REG);
   writew ((CTRL_MEM | CTRL_READ | CTRL_FIFO_RESET | word_swap), CTRL_REG);
   writew ((CTRL_MEM | CTRL_READ | CTRL_RUN | word_swap), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

   if (word_swap == 0)
   {
      while (1)
      {
         unsigned int available;
   
         if (count < 2)
            break;
   
         if ((available = readw (FIFO_COUNT_REG)) != 0) {
            if (DEBUG_LEVEL > 1) {
               if (available & 0x01)
                  printk ("%s: odd number of bytes available in fifo (%d) ?!?\n", __FUNCTION__, available);
               if (available > (count + 1))
                  printk ("%s: fifo bytecount (%d) greater than remaining transfer size (%d)?!?\n", __FUNCTION__, available, count);
            }
            insw (FIFO_DATA_REG, buffer, (available / 2));
            buffer += (available / 2);
            count -= available;
            continue;
         }
         
         if (ssa_timer_expired (&timer))
            goto timedout;
      }

      /*
         If word swap is disabled, we accept odd byte counts.
         This requires some cleanup...
      */

      if (count != 0)
         *((unsigned char *) buffer) = (readw (FIFO_DATA_REG) & 0xFF);
   }
   else
   {
      while (1)
      {
         unsigned int available;
   
         if (count < 4)
            break;
   
         if ((available = readw (FIFO_COUNT_REG)) >= 4) {
            if (DEBUG_LEVEL > 1) {
               if (available & 0x01)
                  printk ("%s: odd number of bytes available in fifo (%d) ?!?\n", __FUNCTION__, available);
               if (available > count)
                  printk ("%s: fifo bytecount (%d) greater than remaining transfer size (%d)?!?\n", __FUNCTION__, available, count);
            }
            insl (FIFO_DATA_REG, buffer, (available / 4));
            buffer += (available / 4);
            count -= available;
            continue;
         }
         
         if (ssa_timer_expired (&timer))
            goto timedout;
      }
   }
      
   if (DEBUG_LEVEL > 2)
      delta = ssa_timer_ticks (&timer);

   if (DEBUG_LEVEL > 1) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s : xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   if (DEBUG_LEVEL > 2)
      printk ("%s : count = %4d, usec = %4d\n", __FUNCTION__, count_initial, (delta / ReadTSCMHz()));

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%04x\n", __FUNCTION__, addr);

   return (unsigned int) -ETIMEDOUT;
}


/*
   Write 'count' bytes from 'buffer' into PCMCIA Common Memory space at offset 'addr'.
   Assumes indirect access mode with auto-increment in target.
*/
unsigned int has7752_pccard_mem_write_multi (unsigned int addr, int count, unsigned short *buffer)
{
   ssa_timer_t timer;
   int count_initial = count;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s: addr = 0x%04x, count = %4d\n", __FUNCTION__, addr, count);

   if (count == 0)
      return 0;

   if (unlikely (word_swap && (count & 0x3))) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : count (%d) must be a multiple of 4 bytes if word swap is enabled\n", __FUNCTION__, count);
      return -1;
   }

   if (unlikely (count > PCCARD_FIFO_SIZE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: byte count (%d) must be %d or less\n", __FUNCTION__, count, PCCARD_FIFO_SIZE);
      return -1;
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (((unsigned long) buffer) & (word_swap ? 0x03 : 0x01))) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: src buffer (0x%08lx) is unaligned\n", __FUNCTION__, (unsigned long) buffer);
      return -1;
   }

   count = ((count + 1) & ~0x01);

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew ((count), TRANSFER_COUNT_REG);
   writew ((CTRL_MEM | CTRL_WRITE | CTRL_FIFO_RESET | word_swap), CTRL_REG);
   writew ((CTRL_MEM | CTRL_WRITE | CTRL_RUN | word_swap), CTRL_REG);

   if (word_swap == 0)
      outsw (FIFO_DATA_REG, buffer, (count / 2));           /* outsw() will panic if src buffer is not 16bit aligned... */
   else
      outsl (FIFO_DATA_REG, buffer, (count / 4));           /* outsl() will panic if src buffer is not 32bit aligned... */

   ssa_timer_set (&timer, TIMEOUT_USEC(count_initial));     /* scale timeout depending on amount of data being written */

   while ((readw (STATUS_REG) & STATUS_AUTO_XFER_IN_PROGRESS) != 0)
      if (ssa_timer_expired (&timer))
         goto timedout;

   if (DEBUG_LEVEL > 0) {
      unsigned int temp1 = readw (TRANSFER_COUNT_REG);
      unsigned int temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s: xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}

/*
   Reads 'count' bytes from PCMCIA Common Memory space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
   Data is written into 'buffer'.
*/
unsigned int has7752_pccard_mem_read_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1, temp2;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s : addr:0x%05x, count:%4d, buf:0x%08lx\n", __FUNCTION__, addr, count, (unsigned long) buffer);

   if (count == 0)
      return 0;

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s : addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped mem read multi direct mode yet...\n", __FUNCTION__);
      return -1;
   }

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew (((count + 1) & ~0x01), TRANSFER_COUNT_REG);
   writew ((CTRL_MEM | CTRL_READ | CTRL_POSTINC | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_MEM | CTRL_READ | CTRL_POSTINC | CTRL_RUN), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

   while (count >= 2) {                         /* while we require more data */
      if (readw (FIFO_COUNT_REG) >= 2) {        /* if there is data available */
         *buffer++ = readw (FIFO_DATA_REG);     /* read from the fifo */
         count -= 2;
      }
      else {
         if (ssa_timer_expired (&timer))
            goto timedout;
      }
   }

   if (count != 0)
      *((unsigned char *) buffer) = (readw (FIFO_DATA_REG) & 0xFF);

   if (DEBUG_LEVEL > 0) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s : xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s : timeout reading from 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}


/*
   Writes 'count' bytes from buffer into PCMCIA Common Memory space starting at offset 'addr'.
   ie first access will be a 16bit access from 'addr', second access will be a 16bit read from 'addr + 2', etc, etc
*/
unsigned int has7752_pccard_mem_write_multi_direct (unsigned int addr, int count, unsigned short *buffer)
{
   unsigned int temp1 = 0, temp2;
   unsigned long ibufptr = (unsigned long) buffer;
   int count_initial = count;
   ssa_timer_t timer;
   unsigned long flags;

   if (DEBUG_LEVEL > 1)
      printk ("%s: addr:0x%05x, count:%4d, buf:0x%08lx\n", __FUNCTION__, addr, count, ibufptr);

   if (count == 0)
      return 0;

   if (unlikely (count > PCCARD_FIFO_SIZE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: byte count (%d) must be %d or less\n", __FUNCTION__, count, PCCARD_FIFO_SIZE);
      return -1;
   }

   if (unlikely (addr & ~0x3FFFE)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: addr (0x%08x) is out of range or unaligned\n", __FUNCTION__, addr);
      return -1;
   }

   if (unlikely (word_swap)) {
      if (DEBUG_LEVEL > 0)
         printk ("%s: Doh !! Andre hasn't implemented word swapped mem write multi direct mode yet...\n", __FUNCTION__);
      return -1;
   }

   /*
      We must always write a multiple of 16bits to the card (a HW limitation).
      Warning: If the original count is odd, one extra byte of data from the
               end of 'buffer' and written into the card !!
   */

   count = ((count + 1) & ~0x01);               /* round up count to a multiple of 2 bytes */

   DISABLE_INTERRUPTS();

   writew ((addr & 0xFFFF), ADDR_LOW_REG);
   writew ((addr >> 16), ADDR_HIGH_REG);
   writew ((count), TRANSFER_COUNT_REG);
   writew ((CTRL_MEM | CTRL_WRITE | CTRL_POSTINC | CTRL_FIFO_RESET), CTRL_REG);
   writew ((CTRL_MEM | CTRL_WRITE | CTRL_POSTINC | CTRL_RUN), CTRL_REG);

   ssa_timer_set (&timer, TIMEOUT_USEC(count));

   switch (ibufptr & 0x03)
   {
      case 0:
#if 0
         temp1 = *((unsigned int *) ibufptr);
         ibufptr += 4;

         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            writel (temp1, FIFO_DATA_REG);
            temp1 = temp2;
         }
#elif 0
         temp1 = *((unsigned int *) ibufptr);
         ibufptr += 4;

         while (count > 8) {
            writel (temp1, FIFO_DATA_REG);
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            writel (temp2, FIFO_DATA_REG);
            count -= 8;
         }
         if (count > 4) {
            writel (temp1, FIFO_DATA_REG);
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
         }
#else
         while (count >= 8) {
            typedef struct { unsigned int t1; unsigned int t2; } ds_t;
            ds_t ds;
            ds = *((ds_t *) ibufptr);
            ibufptr += 8;
            *((ds_t *) FIFO_DATA_REG) = ds;
            count -= 8;
         }
         if (count) {
            temp1 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            if (count > 4) {
               writel (temp1, FIFO_DATA_REG);
               temp1 = *((unsigned int *) ibufptr);
               ibufptr += 4;
               count -= 4;
            }
         }
#endif
         break;
      
      case 1:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 8);
         ibufptr += 3;

         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 24);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 8);
         }

         temp1 |= (*((unsigned int *) ibufptr) << 24);
         break;

      case 2:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 16);
         ibufptr += 2;
   
         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 16);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 16);
         }
   
         temp1 |= (*((unsigned int *) ibufptr) << 16);
         break;

      case 3:
         temp1 = (*((unsigned int *) (ibufptr & ~0x03)) >> 24);
         ibufptr += 1;
   
         while (count > 4) {
            temp2 = *((unsigned int *) ibufptr);
            ibufptr += 4;
            count -= 4;
            temp1 |= (temp2 << 8);
            writel (temp1, FIFO_DATA_REG);
            temp1 = (temp2 >> 24);
         }
   
         temp1 |= (*((unsigned int *) ibufptr) << 8);
         break;
      
      default:
         break;
   }
   
   while (count > 0) {
      writew (temp1, FIFO_DATA_REG);
      count -= 2;
      temp1 >>= 16;
   }

   while ((readw (STATUS_REG) & STATUS_AUTO_XFER_IN_PROGRESS) != 0)
      if (ssa_timer_expired (&timer))
         goto timedout;

   if (DEBUG_LEVEL > 0) {
      temp1 = readw (TRANSFER_COUNT_REG);
      temp2 = readw (FIFO_COUNT_REG);
      if (unlikely ((temp1 | temp2) != 0))
         printk ("%s: xfercnt = %d, Fifocnt = %d\n", __FUNCTION__, temp1, temp2);
   }

   ENABLE_INTERRUPTS();

   return 0;

timedout:
   softreset();
   ENABLE_INTERRUPTS();
   if (DEBUG_LEVEL > 0)
      printk ("%s: timeout writing to 0x%04x, count = %d, initial = %d\n",
              __FUNCTION__, addr, count, count_initial);

   return (unsigned int) -ETIMEDOUT;
}


/*******************************************************************************
*******************************************************************************/

#ifdef CONFIG_PROC_FS

static int proc_pccardws_io_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   char *p = page;
   unsigned int ws = readw (IO_SPEED_REG);
   
   p += sprintf (p, "(%u,%u,%u)\n", ((ws >> 8) & 0x0F),     /* address setup waitstates */
                                    ((ws >> 3) & 0x1F),     /* minimum datastrobe waitstates (extended by WAIT signal) */
                                    ((ws >> 0) & 0x07));    /* address hold waitstates */

   if ((len = (p - page) - off) < 0)
      len = 0;

   *eof = (len <= count) ? 1 : 0;
   *start = page + off;

   return len;
}

static int proc_pccardws_io_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
   char sbuf[16 + 1];
   int setup_ws, strobe_ws, hold_ws;
   int len;

   if (count > 0) {
      len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
      memset (sbuf, 0, sizeof(sbuf));
      if (copy_from_user (sbuf, buffer, len))
         return -EFAULT;
      if (sscanf (sbuf, "(%u,%u,%u)", &setup_ws, &strobe_ws, &hold_ws) == 3)
         has7752_pccard_set_io_waitstates (setup_ws, strobe_ws, hold_ws);
   }

   return count;
}

static int proc_pccardws_mem_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
   int len;
   char *p = page;
   unsigned int ws = readw (MEM_SPEED_REG);
   
   p += sprintf (p, "(%u,%u,%u)\n", ((ws >> 8) & 0x0F),     /* address setup waitstates */
                                    ((ws >> 3) & 0x1F),     /* minimum datastrobe waitstates (extended by WAIT signal) */
                                    ((ws >> 0) & 0x07));    /* address hold waitstates */

   if ((len = (p - page) - off) < 0)
      len = 0;

   *eof = (len <= count) ? 1 : 0;
   *start = page + off;

   return len;
}

static int proc_pccardws_mem_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
   char sbuf[16 + 1];
   int setup_ws, strobe_ws, hold_ws;
   int len;

   if (count > 0) {
      len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
      memset (sbuf, 0, sizeof(sbuf));
      if (copy_from_user (sbuf, buffer, len))
         return -EFAULT;
      if (sscanf (sbuf, "(%u,%u,%u)", &setup_ws, &strobe_ws, &hold_ws) == 3)
         has7752_pccard_set_mem_waitstates (setup_ws, strobe_ws, hold_ws);
   }

   return count;
}

static int __init proc_pccardws_init (void)
{
   struct proc_dir_entry *res;

   if ((res = create_proc_entry ("pccardws_io", (S_IWUSR | S_IRUGO), NULL)) == NULL)
      return -ENOMEM;

   res->read_proc = proc_pccardws_io_read;
   res->write_proc = proc_pccardws_io_write;

   if ((res = create_proc_entry ("pccardws_mem", (S_IWUSR | S_IRUGO), NULL)) == NULL)
      return -ENOMEM;

   res->read_proc = proc_pccardws_mem_read;
   res->write_proc = proc_pccardws_mem_write;

   return 0;
}

#else

static int __init proc_pccardws_init (void)
{
   return 0;
}

#endif


static int __init pccard_init (void)
{
   printk ("Initialising HAS7752 pccard interface\n");

   has7752_pccard_init();
   proc_pccardws_init();

   return 0;
}

__initcall (pccard_init);


/*******************************************************************************
*******************************************************************************/

EXPORT_SYMBOL(has7752_pccard_init);
EXPORT_SYMBOL(has7752_pccard_reset_assert);
EXPORT_SYMBOL(has7752_pccard_reset_deassert);
EXPORT_SYMBOL(has7752_pccard_read_ready);
EXPORT_SYMBOL(has7752_pccard_get_irq);
EXPORT_SYMBOL(has7752_pccard_set_mem_waitstates);
EXPORT_SYMBOL(has7752_pccard_set_io_waitstates);
EXPORT_SYMBOL(has7752_pccard_word_swap_enable);
EXPORT_SYMBOL(has7752_pccard_word_swap_disable);
EXPORT_SYMBOL(has7752_pccard_attribute_read);
EXPORT_SYMBOL(has7752_pccard_attribute_write);
EXPORT_SYMBOL(has7752_pccard_io_read);
EXPORT_SYMBOL(has7752_pccard_io_write);
EXPORT_SYMBOL(has7752_pccard_io_read_multi);
EXPORT_SYMBOL(has7752_pccard_io_write_multi);
EXPORT_SYMBOL(has7752_pccard_io_read_multi_direct);
EXPORT_SYMBOL(has7752_pccard_io_write_multi_direct);
EXPORT_SYMBOL(has7752_pccard_mem_read);
EXPORT_SYMBOL(has7752_pccard_mem_write);
EXPORT_SYMBOL(has7752_pccard_mem_read_multi);
EXPORT_SYMBOL(has7752_pccard_mem_write_multi);
EXPORT_SYMBOL(has7752_pccard_mem_read_multi_direct);
EXPORT_SYMBOL(has7752_pccard_mem_write_multi_direct);



