/*
 *  linux/drivers/char/serial_ssa.c
 *
 *  Driver for the SSA serial port (based on a VLSI uart ??)
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright (C) 2002-2006 Andre McCurdy, Philips Semiconductors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>

#include <asm/hardware.h>


#if defined(CONFIG_SERIAL_SSA_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/serial_core.h>

#if defined (CONFIG_ARCH_PNX0106)
#define INT_UART0 IRQ_UART_0
#endif


/*
   UART Register Offsets.
*/

#define SSA_UARTDR              0x00        /* Data Register */
#define SSA_UARTIER             0x04        /* Interrupt Enable Register */
#define SSA_UARTIIR             0x08        /* Interrupt ID Register (Read  only) */
#define SSA_UARTFCR             0x08        /* Fifo Control Register (Write only) */
#define SSA_UARTLCR             0x0C        /* Line Control Register */
#define SSA_UARTMCR             0x10        /* Modem Control Register */
#define SSA_UARTLSR             0x14        /* Line Status Register */
#define SSA_UARTMSR             0x18        /* Modem Status Register */

/*
   These registers overlay the above when DLAB (bit7 in
   the Line Control Register) is set.
*/

#define SSA_UARTDLL             0x00        /* Divisor Latch Low byte Register */
#define SSA_UARTDLM             0x04        /* Divisor Latch Mid byte Register */
#define SSA_UARTMISC            0x08        /* Misc control Register */


#define MSR_DELTA_CTS           (1 << 0)
#define MSR_DELTA_DSR           (1 << 1)
#define MSR_DELTA_DCD           (1 << 3)
#define MSR_CTS                 (1 << 4)
#define MSR_DSR                 (1 << 5)
#define MSR_DCD                 (1 << 7)

#define MCR_DTR                 (1 << 0)
#define MCR_RTS                 (1 << 1)


#define UART_NR                 1
#define SERIAL_SSA_NR           UART_NR
#define CALLOUT_SSA_NR          UART_NR

/*
   Max chars to fetch (and send ??) per interrupt.
*/

#define SSA_ISR_PASS_LIMIT     256

/*
   Access macros for the UART registers.
*/

#define UART_GET_CHAR(p)        readl(IO_ADDRESS_UART0_BASE + SSA_UARTDR )
#define UART_GET_IER(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTIER)
#define UART_GET_LSR(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTLSR)
#define UART_GET_IIR(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTIIR)
#define UART_GET_MSR(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTMSR)
#define UART_GET_MCR(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTMCR)
#define UART_GET_LCR(p)         readl(IO_ADDRESS_UART0_BASE + SSA_UARTLCR)

#define UART_PUT_CHAR(p, c)     writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTDR )
#define UART_PUT_IER(p,c)       writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTIER)
#define UART_PUT_MCR(p,c)       writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTMCR)
#define UART_PUT_LCR(p,c)       writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTLCR)

/* Set DLB to 1 before calling these functions */
#define UART_PUT_DLL(p,c)       writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTDR)
#define UART_PUT_DLM(p,c)       writel((c), IO_ADDRESS_UART0_BASE + SSA_UARTIER)

#define UART_DUMMY_RSR_RX       256
#define UART_PORT_SIZE          128



static struct tty_driver  normal;
static struct tty_driver  callout;
static struct tty_struct *ssa_table[UART_NR];
static struct termios    *ssa_termios[UART_NR];
static struct termios    *ssa_termios_locked[UART_NR];

#ifdef SUPPORT_SYSRQ
static struct console     ssa_console;
#endif

static void ssauart_tx_one_char (struct uart_port *port);

#if 0

#define UART_LOG_BUFFER_LENGTH  (1024)

unsigned long uart_int_status[UART_LOG_BUFFER_LENGTH];
unsigned long uart_int_status_index;

static void uart_int_buffer_update (unsigned long status)
{
   if (uart_int_status_index < UART_LOG_BUFFER_LENGTH)
   {
      uart_int_status[uart_int_status_index++] = status;
   }
   else
   {
      while (1) ;
   }
}

#endif


/*
   HW flow control faq....

   If we are ready to communicate (either send or receive data) we assert our RTS output.
   We then wait for our CTS input to be asserted before we start sending data.

   If the other end wants us to stop sending, it will de-assert our CTS input.

   If we want to stop the other end sending, we de-assert our RTS output.
*/


static void ssauart_stop_tx (struct uart_port *port, unsigned int from_tty)
{
   unsigned int ier;
   unsigned long flags;

   local_irq_save (flags);          /* AMcCurdy: added paranoid irq protection */

   ier  = UART_GET_IER(port);
   ier &= ~(1 << 1);                /* disable Tx interrupts */
   UART_PUT_IER(port, ier);

   local_irq_restore (flags);
}

static void ssauart_start_tx (struct uart_port *port, unsigned int from_tty)
{
   unsigned int ier;
   unsigned long flags;

   local_irq_save (flags);          /* AMcCurdy: added paranoid irq protection */

   ier  = UART_GET_IER(port);
   ier |= (1 << 1);                 /* enable Tx interrupts */
   UART_PUT_IER(port, ier);

   /* We may have something in the buffer already */
   ssauart_tx_one_char (port);

   local_irq_restore (flags);
}

static void ssauart_stop_rx (struct uart_port *port)
{
   unsigned int ier;
   unsigned long flags;

   local_irq_save (flags);          /* AMcCurdy: added paranoid irq protection */

   ier  = UART_GET_IER(port);
   ier &= ~(1 << 0);                /* disable Rx interrupts */
   UART_PUT_IER(port, ier);

   local_irq_restore (flags);
}

static void ssauart_enable_ms (struct uart_port *port)
{
#if 0

   unsigned int ier;
   unsigned long flags;

   local_irq_save (flags);          /* AMcCurdy: added paranoid irq protection */

   ier  = UART_GET_IER(port);
   ier |= (1 << 3);                 /* enable Modem Status interrupts */
   UART_PUT_IER(port, ier);

   local_irq_restore (flags);

#endif
}


#ifdef SUPPORT_SYSRQ
static void ssauart_rx_chars (struct uart_port *port, struct pt_regs *regs)
#else
static void ssauart_rx_chars (struct uart_port *port)
#endif
{
#if 1

   struct tty_struct *tty = port->info->tty;

   while (1)
   {
      if ((UART_GET_LSR(port) & (1 << 0)) == 0)         /* no more chars available */
         break;

      *tty->flip.char_buf_ptr++ = UART_GET_CHAR(port);
      *tty->flip.flag_buf_ptr++ = TTY_NORMAL;           /* Fixme: we should check for overrun, parity, framing errors etc */
      port->icount.rx++;
      tty->flip.count++;

      if (tty->flip.count >= TTY_FLIPBUF_SIZE)
         break;
   }

   tty_flip_buffer_push (tty);

#else

   struct tty_struct *tty = port->info->tty;

   unsigned int ch;
   unsigned int status;
   unsigned int max_count = 256;

   while (1)
   {
      status = UART_GET_LSR(port);

      if (((status & (1 << 0)) == 0) || (--max_count == 0))
         break;

      if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
         tty->flip.tqueue.routine ((void *) tty);
         if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
            printk (KERN_WARNING "TTY_DONT_FLIP set\n");
            return;
         }
      }

      *tty->flip.char_buf_ptr++ = UART_GET_CHAR(port);
      *tty->flip.flag_buf_ptr++ = TTY_NORMAL;           /* Fixme: we should check for overrun, parity, framing errors etc */
      tty->flip.count++;
      port->icount.rx++;
   }

   tty_flip_buffer_push (tty);

#endif
}

/* Called with interrupts off only from start_tx() */
static void ssauart_tx_one_char (struct uart_port *port)
{
   struct circ_buf *xmit = &port->info->xmit;

   /* The Fifo may already contain data in which case, our work is
    * done since when it drains it will trigger an interrupt.
    */
   if ((UART_GET_LSR(port) & (1 << 6)) == 0)
      return;

   if (port->x_char)
   {
      UART_PUT_CHAR(port, port->x_char);
      port->x_char = 0;
      port->icount.tx++;
      return;
   }

   if (uart_circ_empty (xmit) || uart_tx_stopped (port))
      return;

   UART_PUT_CHAR(port, xmit->buf[xmit->tail]);
   xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
   port->icount.tx++;

   /*
      If the number of bytes remaining in the SW Tx circular buffer is less
      than WAKEUP_CHARS, wake up tasks trying to write to the UART device.
   */

   if (uart_circ_chars_pending (xmit) < WAKEUP_CHARS)
   {
      uart_write_wakeup (port);

      /*
         If there are no more bytes to transmit, disable Tx interrupts.
         (We don't need to refill the UART Tx fifo when it next becomes empty).
      */

      if (uart_circ_empty (xmit))
         ssauart_stop_tx (port, 0);
   }
}

static void ssauart_tx_chars (struct uart_port *port)
{
   struct circ_buf *xmit = &port->info->xmit;
   int count;

   if (port->x_char)
   {
      UART_PUT_CHAR(port, port->x_char);
      port->x_char = 0;
      port->icount.tx++;
      return;
   }

   if (uart_circ_empty (xmit) || uart_tx_stopped (port))
   {
      ssauart_stop_tx (port, 0);
      return;
   }

   /*
      Write upto (port->fifosize) to the UART from the SW Tx buffer.
      Stops early if the SW Tx Buffer has no more data.

      Warning: Does NOT stop early if there is no space in the UART Tx fifo.
               (ie we assume that port->fifosize is correct !!)
   */

   count = port->fifosize;

   do
   {
      UART_PUT_CHAR(port, xmit->buf[xmit->tail]);
      xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
      port->icount.tx++;
      if (uart_circ_empty (xmit))
         break;
   }
   while (--count > 0);

   /*
      If the number of bytes remaining in the SW Tx circular buffer is less
      than WAKEUP_CHARS, wake up tasks trying to write to the UART device.
   */

   if (uart_circ_chars_pending (xmit) < WAKEUP_CHARS)
   {
      uart_write_wakeup (port);

      /*
         If there are no more bytes to transmit, disable Tx interrupts.
         (We don't need to refill the UART Tx fifo when it next becomes empty).
      */

      if (uart_circ_empty (xmit))
         ssauart_stop_tx (port, 0);
   }
}


static void ssauart_modem_status (struct uart_port *port)
{
   /*
      MSR give the values of CTS, DSR, RI and DCD inputs as well as
      an indication of which have changed since the last time that
      the MSR register was read.

      ie there is no need to maintain our own copy of status to detect changes.
   */

   unsigned int status = UART_GET_MSR(port);                    /* reading MSR clears all MSR_DELTA_xxx flags */

   if ((status & (MSR_DELTA_CTS | MSR_DELTA_DCD | MSR_DELTA_DSR)) != 0)
   {
      if (status & MSR_DELTA_CTS)
         uart_handle_cts_change (port, (status & MSR_CTS));    /* pass new value to the change function */

      if (status & MSR_DELTA_DCD)
         uart_handle_dcd_change (port, (status & MSR_DCD));    /* pass new value to the change function */

      if (status & MSR_DELTA_DSR)
         port->icount.dsr++;

      wake_up_interruptible (&port->info->delta_msr_wait);
   }
   else
   {
      /*
         Shouldn't really happen..
         Must be a spurious edge on NRI (modem ring indicator ??)
      */
   }
}


static void ssauart_int (int irq, void *dev_id, struct pt_regs *regs)
{
   struct uart_port *port = dev_id;

   unsigned int pass_counter = SSA_ISR_PASS_LIMIT;
   unsigned int interrupt_id;

   while (1)
   {
      if (--pass_counter == 0)
         break;

      interrupt_id = (UART_GET_IIR(port) & 0x0f);

      if ((interrupt_id == 0x04) ||         /* Rx data available (above desired level in fifo) */
          (interrupt_id == 0x0C))           /* Rx data available (timeout) */
      {
#ifdef SUPPORT_SYSRQ
         ssauart_rx_chars (port, regs);
#else
         ssauart_rx_chars (port);
#endif
      }

      if (interrupt_id == 0x02)             /* Tx fifo is empty */
         ssauart_tx_chars (port);

      if (interrupt_id == 0x01)             /* no more interrupts left to service */
      {
         break;
      }
      else                                  /* if all else fails, check the really unlikely stuff... */
      {
         if (interrupt_id == 0x06)          /* overrun, parity, framing or break error */
            ;                               /* ignore for now (Note that reading the Int ID register clears the int request) */

         if (interrupt_id == 0x00)          /* Modem interrupt (will be cleared by reading MSR) */
            ssauart_modem_status (port);
      }
   }
}


static u_int ssauart_tx_empty (struct uart_port *port)
{
   return UART_GET_LSR(port) & (1 << 6) ? TIOCSER_TEMT : 0;
}


static u_int ssauart_get_mctrl (struct uart_port *port)
{
#if 1

   return 0;

#else

   /*
      Note that reading the Modem Status Register clears the signal
      change indicators, so we will miss possible interrupts if this
      function is called with modem interrupts enabled... !!!
   */

   unsigned int status = UART_GET_MSR(port);
   unsigned int result = 0;

   printk ("ssauart_get_mctrl called (delta flags will get cleared !!)\n");

   if (status & MSR_DCD) result |= TIOCM_CAR;
   if (status & MSR_DSR) result |= TIOCM_DSR;
   if (status & MSR_CTS) result |= TIOCM_CTS;

   return (result);

#endif
}

static void ssauart_set_mctrl (struct uart_port *port, u_int mctrl)
{
#if 0
   unsigned int mcr = 0;

   if (mctrl & TIOCM_DTR) mcr |= (MCR_DTR);
   if (mctrl & TIOCM_RTS) mcr |= (MCR_RTS);

   UART_PUT_MCR(port, mcr);         /* DTR and RTS set as required, all other bits cleared */
#endif
}

static void ssauart_break_ctl (struct uart_port *port, int break_state)
{
#if 0
   unsigned int lcr_h;

   lcr_h = UART_GET_LCRH(port);

   if (break_state == -1)
           lcr_h |= AMBA_UARTLCR_H_BRK;
   else
           lcr_h &= ~AMBA_UARTLCR_H_BRK;
   UART_PUT_LCRH(port, lcr_h);
#endif
}


static int ssauart_startup (struct uart_port *port)
{
   int retval;

#if 1
   if ((retval = request_irq (port->irq, ssauart_int, 0, "uart", port)))
      return (retval);
#else
   if ((retval = request_irq (port->irq, ssauart_int, SA_INTERRUPT, "uart", port)))
      return (retval);
#endif

   UART_PUT_IER(port, 0x03);   /* enable Tx and Rx ints only (modem status enabled elsewhere ?) */

   return 0;
}


static void ssauart_shutdown (struct uart_port *port)
{
   UART_PUT_IER (port, 0x00);   /* disable all interrupts */
   free_irq (port->irq, port);
}

static void ssauart_change_speed (struct uart_port *port, u_int cflag, u_int iflag, u_int quot)
{
   u_int lcr_h;
   unsigned long flags;

#if DEBUG
   printk("ssauart_set_cflag(0x%x) called\n", cflag);
#endif
   /* byte size and parity */
   switch (cflag & CSIZE) {
   case CS5: lcr_h = UART_LCR_WLEN5; break;
   case CS6: lcr_h = UART_LCR_WLEN6; break;
   case CS7: lcr_h = UART_LCR_WLEN7; break;
   default:  lcr_h = UART_LCR_WLEN8; break; // CS8
   }
   if (cflag & CSTOPB)
       lcr_h |= UART_LCR_STOP;
   if (cflag & PARENB) {
       lcr_h |= UART_LCR_PARITY;
   }

   local_irq_save(flags);

   port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
   if (iflag & INPCK)
       port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
   if (iflag & (BRKINT | PARMRK))
       port->read_status_mask |= UART_LSR_BI;

   /*
    * Characters to ignore
    */
   port->ignore_status_mask = 0;
   if (iflag & IGNPAR)
       port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
   if (iflag & IGNBRK) {
       port->ignore_status_mask |= UART_LSR_BI;
       /*
        * If we're ignore parity and break indicators, ignore
        * overruns too.  (For real raw support).
        */
       if (iflag & IGNPAR)
          port->ignore_status_mask |= UART_LSR_OE;
   }

   /*
    * !!! ignore all characters if CREAD is not set
    */
   if ((cflag & CREAD) == 0)
      port->ignore_status_mask |= UART_LSR_DR;

   /* Turn on DLAB */
   UART_PUT_LCR(port, UART_GET_LCR(port) | UART_LCR_DLAB);

   /* Set baud rate */
   UART_PUT_DLM(port, ((quot & 0xff00) >> 8));
   UART_PUT_DLL(port, (quot & 0xff));

   /* Turn off DLAB */
   UART_PUT_LCR(port, UART_GET_LCR(port) & ~UART_LCR_DLAB);

   local_irq_restore(flags);
}

static const char *ssauart_type (struct uart_port *port)
{
   return (port->type == PORT_SSA ? "SSA" : NULL);
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void ssauart_release_port (struct uart_port *port)
{
   release_mem_region (port->mapbase, UART_PORT_SIZE);
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int ssauart_request_port (struct uart_port *port)
{
   if (request_mem_region (port->mapbase, UART_PORT_SIZE, "serial_ssa") != NULL)
      return 0;
   else
      return -EBUSY;
}

/*
 * Configure/autoconfigure the port.
 */
static void ssauart_config_port (struct uart_port *port, int flags)
{
   if (flags & UART_CONFIG_TYPE)
   {
      port->type = PORT_SSA;
      ssauart_request_port (port);
   }
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int ssauart_verify_port (struct uart_port *port, struct serial_struct *ser)
{
   if ((ser->type != PORT_UNKNOWN && ser->type != PORT_SSA) ||
       (ser->irq != INT_UART0))
   {
      return -EINVAL;
   }

   return 0;
}


static struct uart_ops ssauart_pops =
{
   .tx_empty        = ssauart_tx_empty,
   .set_mctrl       = ssauart_set_mctrl,
   .get_mctrl       = ssauart_get_mctrl,
   .stop_tx         = ssauart_stop_tx,
   .start_tx        = ssauart_start_tx,
   .stop_rx         = ssauart_stop_rx,
   .enable_ms       = ssauart_enable_ms,
   .break_ctl       = ssauart_break_ctl,
   .startup         = ssauart_startup,
   .shutdown        = ssauart_shutdown,
   .change_speed    = ssauart_change_speed,
   .type            = ssauart_type,
   .release_port    = ssauart_release_port,
   .request_port    = ssauart_request_port,
   .config_port     = ssauart_config_port,
   .verify_port     = ssauart_verify_port,
};


static struct uart_port ssa_ports[UART_NR] =
{
   {
      iobase:       0,                                  /* driver doesn't use io ports */
      membase:      (void *) IO_ADDRESS_UART0_BASE,     /* read/write[bwl] */
      irq:          INT_UART0,
//    uartclk:      UART_CLK_HZ,                        /* serial core assumes divisor = (uartclk / (16 * baudrate)). This is only true is DL2X_EN is clear !! */
      iotype:       SERIAL_IO_MEM,
      flags:        ASYNC_BOOT_AUTOCONF,                /* autoconfigure port on bootup */
      ops:          &ssauart_pops,
#if defined (CONFIG_ARCH_SSA)
      fifosize:     16,
      mapbase:      SSA_UART0_BASE,
#elif defined (CONFIG_ARCH_PNX0106)
      fifosize:     64,
      mapbase:      UART0_REGS_BASE,
#else
#error "unknown platform"
#endif
   }
};


#ifdef CONFIG_SERIAL_SSA_CONSOLE

static void ssauart_console_write (struct console *co, const char *s, u_int count)
{
   struct uart_port *port;
   unsigned int status, old_ier;
   int i;

   port = ssa_ports + co->index;

   old_ier = UART_GET_IER(port);

// if (old_ier != 0) return;

   UART_PUT_IER(port, 0x00);

   for (i = 0; i < count; i++)
   {
      do
      {
         status = UART_GET_LSR(port);
      }
      while ((status & (1 << 6)) == 0);

      UART_PUT_CHAR(port, s[i]);

      if (s[i] == '\n')
      {
         do
         {
            status = UART_GET_LSR(port);
         }
         while ((status & (1 << 6)) == 0);

         UART_PUT_CHAR(port, '\r');
      }
   }

   do
   {
      status = UART_GET_LSR(port);
   }
   while ((status & (1 << 6)) == 0);

// do { /* nothing */ } while (UART_GET_IIR(port) != 0x01);

   UART_PUT_IER(port, old_ier);
}

static kdev_t ssauart_console_device (struct console *co)
{
   return MKDEV(TTY_MAJOR, 64 + co->index);
}

static int __init ssauart_console_setup (struct console *co, char *options)
{
   struct uart_port *port;
   int baud = 115200;
   int parity = 'n';
   int bits = 8;
   int flow = 'n';

   /*
      Find the first usable port covered by this driver...
      (Always port 0 in this case as we only have/support one uart).
   */

   port = &ssa_ports[0];

   /*
      Init the UART clk here (it's based on the variable uartclkhz which is passed in a tag by the bootloader).
      Note: serial core assumes divisor = (uartclk / (16 * baudrate)). This is only true if the DL2X_EN bit is clear.
   */

   ssa_ports[0].uartclk = uartclkhz;


   if (options)
      uart_parse_options (options, &baud, &parity, &bits, &flow);

   return (uart_set_options (port, co, baud, parity, bits, flow));
}

static struct console ssa_console =
{
   name:   "ttyS",
   write:  ssauart_console_write,
   device: ssauart_console_device,
   setup:  ssauart_console_setup,
   flags:  CON_PRINTBUFFER,
   index:  -1,
};

void __init ssauart_console_init (void)
{
   register_console (&ssa_console);
}

#define SSA_CONSOLE    &ssa_console
#else
#define SSA_CONSOLE    NULL
#endif

static struct uart_driver ssa_reg =
{
   owner:                  THIS_MODULE,
   normal_major:           TTY_MAJOR,
#ifdef CONFIG_DEVFS_FS
   normal_name:            "ttyS%d",
   callout_name:           "cua%d",
#else
   normal_name:            "ttyS",
   callout_name:           "cua",
#endif
   normal_driver:          &normal,
   callout_major:          TTYAUX_MAJOR,
   callout_driver:         &callout,
   table:                  ssa_table,
   termios:                ssa_termios,
   termios_locked:         ssa_termios_locked,
   minor:                  64,
   nr:                     UART_NR,
   cons:                   SSA_CONSOLE,
};

static int __init ssauart_init (void)
{
   int ret = uart_register_driver (&ssa_reg);

   /*
      Init the UART clk here (it's based on the variable uartclkhz which is passed in a tag by the bootloader).
      Note: serial core assumes divisor = (uartclk / (16 * baudrate)). This is only true if the DL2X_EN bit is clear.
   */

   ssa_ports[0].uartclk = uartclkhz;

   if (ret == 0)
      uart_add_one_port (&ssa_reg, &ssa_ports[0]);

   return ret;
}

static void __exit ssauart_exit (void)
{
   uart_remove_one_port (&ssa_reg, &ssa_ports[0]);
   uart_unregister_driver (&ssa_reg);
}



#if 0
void ssa_simple_write (const char *s, int count)
{
   struct uart_port *port = ssa_ports;

   int i;
   unsigned int status, old_ier;

   old_ier = UART_GET_IER(port);       /* save previous int enable flags */

   UART_PUT_IER(port, 0x00);           /* disable all ints */

   for (i = 0; i < count; i++)
   {
      do
      {
    status = UART_GET_LSR(port);
      }
      while ((status & (1 << 6)) == 0);

      UART_PUT_CHAR(port, s[i]);

      if (s[i] == '\n')
      {
    do
    {
        status = UART_GET_LSR(port);
    }
    while ((status & (1 << 6)) == 0);

    UART_PUT_CHAR(port, '\r');
      }
   }

   do
   {
       status = UART_GET_LSR(port);
   }
   while ((status & (1 << 6)) == 0);

   writel( old_ier, port->membase);

   UART_PUT_IER(port, old_ier);
}
#endif


module_init(ssauart_init);
module_exit(ssauart_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Andre McCurdy");
MODULE_DESCRIPTION("Philips SAA7750/52 uart driver");
MODULE_LICENSE("GPL");
