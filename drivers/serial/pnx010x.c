/*
 *  linux/drivers/serial/pnx010x.c
 *
 *  Driver for the PNX010x serial ports (based on a VLSI uart ??)
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
 *
 *  UART HW flow control FAQ...
 *
 *  If we are ready to communicate (either send or receive data) we
 *  assert our RTS output. We then wait for our CTS input to be asserted
 *  before we start sending data.
 *  If the other end wants us to stop sending, it will de-assert our CTS input.
 *  If we want to stop the other end sending, we de-assert our RTS output.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/console.h>
#include <linux/sysrq.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>

#if defined(CONFIG_SERIAL_PNX010X_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/serial_core.h>

#define UART_NR                 2

#define SERIAL_PNX010X_MAJOR	TTY_MAJOR
#define SERIAL_PNX010X_MINOR	64

#define CALLOUT_PNX010X_MAJOR	TTYAUX_MAJOR
#define CALLOUT_PNX010X_MINOR	64

static struct tty_driver normal;
static struct tty_driver callout;
static struct tty_struct *pnx010x_table[UART_NR];
static struct termios *pnx010x_termios[UART_NR];
static struct termios *pnx010x_termios_locked[UART_NR];

#if defined (CONFIG_SERIAL_PNX010X_CONSOLE)
static struct console pnx010x_console;
#if defined (CONFIG_PNX0106_HASLI7) || defined (CONFIG_PNX0106_HACLI7) || defined (CONFIG_PNX0106_MCIH) || defined (CONFIG_PNX0106_MCI)
int pnx010x_console_in_use;
#define CONSOLE_IN_USE pnx010x_console_in_use
#endif
#endif

#define PNX010X_ISR_PASS_LIMIT	(256)
#define UART_VIRTUAL_CLK	(115200 * 4 * 16)

/*
	Register definitions
*/
#define PNX010X_UARTDR		0x00	/* Data Register */
#define PNX010X_UARTIER 	0x04	/* Interrupt Enable Register */
#define PNX010X_UARTIIR 	0x08	/* Interrupt ID Register (Read  only) */
#define PNX010X_UARTFCR 	0x08	/* Fifo Control Register (Write only) */
#define PNX010X_UARTLCR 	0x0C	/* Line Control Register */
#define PNX010X_UARTMCR 	0x10	/* Modem Control Register */
#define PNX010X_UARTLSR 	0x14	/* Line Status Register */
#define PNX010X_UARTMSR 	0x18	/* Modem Status Register */
#define PNX010X_UARTFDR 	0x28	/* Fractional Divider Register */

/*
	These registers overlay the above when DLAB (bit7 in
	the Line Control Register) is set.
*/
#define PNX010X_UARTDLL 	0x00	/* Divisor Latch Low byte Register */
#define PNX010X_UARTDLM 	0x04	/* Divisor Latch Mid byte Register */
#define PNX010X_UARTMISC	0x08	/* Misc control Register */

#define UART_PORT_SIZE		0x80

#define MSR_DELTA_CTS		(1 << 0)
#define MSR_DELTA_DSR		(1 << 1)
#define MSR_DELTA_DCD		(1 << 3)
#define MSR_CTS			(1 << 4)
#define MSR_DSR			(1 << 5)
#define MSR_DCD			(1 << 7)

#define MCR_DTR			(1 << 0)
#define MCR_RTS			(1 << 1)


#define UART_GET_CHAR(p)	readl((p)->membase + PNX010X_UARTDR)
#define UART_GET_IER(p)		readl((p)->membase + PNX010X_UARTIER)
#define UART_GET_LSR(p)		readl((p)->membase + PNX010X_UARTLSR)
#define UART_GET_IIR(p)		readl((p)->membase + PNX010X_UARTIIR)
#define UART_GET_MSR(p)		readl((p)->membase + PNX010X_UARTMSR)
#define UART_GET_MCR(p)		readl((p)->membase + PNX010X_UARTMCR)
#define UART_GET_LCR(p)		readl((p)->membase + PNX010X_UARTLCR)
#define UART_GET_FDR(p)		readl((p)->membase + PNX010X_UARTFDR)

#define UART_PUT_CHAR(p,c)	writel((c), (p)->membase + PNX010X_UARTDR)
#define UART_PUT_IER(p,c)	writel((c), (p)->membase + PNX010X_UARTIER)
#define UART_PUT_MCR(p,c)	writel((c), (p)->membase + PNX010X_UARTMCR)
#define UART_PUT_LCR(p,c)	writel((c), (p)->membase + PNX010X_UARTLCR)
#define UART_PUT_DLL(p,c)	writel((c), (p)->membase + PNX010X_UARTDR)
#define UART_PUT_DLM(p,c)	writel((c), (p)->membase + PNX010X_UARTIER)
#define UART_PUT_FDR(p,c)	writel((c), (p)->membase + PNX010X_UARTFDR)


static void pnx010x_uart_tx_chars (struct uart_port *port);


static void pnx010x_uart_stop_tx (struct uart_port *port, unsigned int from_tty)
{
	unsigned int ier;
	unsigned long flags;

	local_irq_save (flags);
	ier = UART_GET_IER(port);
	ier &= ~(1 << 1);                /* disable Tx interrupts */
	UART_PUT_IER(port, ier);
	local_irq_restore (flags);
}

static void pnx010x_uart_start_tx (struct uart_port *port, unsigned int from_tty)
{
	unsigned int ier;
	unsigned long flags;

	local_irq_save (flags);
	ier = UART_GET_IER(port);
	ier |= (1 << 1);                 /* enable Tx interrupts */
	UART_PUT_IER(port, ier);
	pnx010x_uart_tx_chars (port);
	local_irq_restore (flags);
}

static void pnx010x_uart_stop_rx (struct uart_port *port)
{
	unsigned int ier;
	unsigned long flags;

	local_irq_save (flags);
	ier = UART_GET_IER(port);
	ier &= ~(1 << 0);                /* disable Rx interrupts */
	UART_PUT_IER(port, ier);
	local_irq_restore (flags);
}

static void pnx010x_uart_enable_ms (struct uart_port *port)
{
#if 0
	unsigned int ier;
	unsigned long flags;

	local_irq_save (flags);
	ier = UART_GET_IER(port);
	ier |= (1 << 3);                 /* enable Modem Status interrupts */
	UART_PUT_IER(port, ier);
	local_irq_restore (flags);
#endif
}

static void pnx010x_uart_modem_status (struct uart_port *port)
{
	unsigned int status;

	/*
	   MSR give the values of CTS, DSR, RI and DCD inputs as well as
	   an indication of which have changed since the last time that
	   the MSR register was read.

	   ie there is no need to maintain our own copy of status to detect changes.
	*/

	status = UART_GET_MSR(port);						/* NB: reading MSR clears all MSR_DELTA_xxx flags */

	if ((status & (MSR_DELTA_CTS | MSR_DELTA_DCD | MSR_DELTA_DSR)) != 0) {
		if (status & MSR_DELTA_CTS)
			uart_handle_cts_change (port, (status & MSR_CTS));	/* pass new value to the change function */
		if (status & MSR_DELTA_DCD)
			uart_handle_dcd_change (port, (status & MSR_DCD));	/* pass new value to the change function */
		if (status & MSR_DELTA_DSR)
			port->icount.dsr++;
		wake_up_interruptible (&port->info->delta_msr_wait);
	}
	else {
		/*
		   Shouldn't really happen..
		   Must be a spurious edge on NRI (modem ring indicator ??)
		*/
	}
}

static inline unsigned int pnx010x_uart_rx_char_available (struct uart_port *port)
{
	/*
	   Bit 0 in Line Status Register is set when Rx data is available.
	*/
	return (UART_GET_LSR(port) & (1 << 0)) ? 1 : 0;
}

#ifdef SUPPORT_SYSRQ
static void pnx010x_uart_rx_chars (struct uart_port *port, struct pt_regs *regs)
#else
static void pnx010x_uart_rx_chars (struct uart_port *port)
#endif
{
	struct tty_struct *tty = port->info->tty;

	while (1) {
		if (pnx010x_uart_rx_char_available (port) == 0) 	/* no more chars available */
			break;

		*tty->flip.char_buf_ptr++ = UART_GET_CHAR(port);
		*tty->flip.flag_buf_ptr++ = TTY_NORMAL; 		/* Fixme: we should check for overrun, parity, framing errors etc */
		port->icount.rx++;
		tty->flip.count++;
		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			break;
	}

	tty_flip_buffer_push (tty);
}

static inline unsigned int pnx010x_uart_tx_space_available (struct uart_port *port)
{
	/*
	   THRE (bit 5 in the Line Status Register) is set when the
	   Tx holding register is empty.
	*/
	return (UART_GET_LSR(port) & (1 << 5)) ? 1 : 0;
}

static unsigned int pnx010x_uart_tx_empty (struct uart_port *port)
{
	/*
	   TEMT (bit 6 in the Line Status Register) is set when both
	   the Tx FiFo and the Tx holding register are empty.
	*/
	return (UART_GET_LSR(port) & (1 << 6)) ? TIOCSER_TEMT : 0;
}

static void pnx010x_uart_tx_chars (struct uart_port *port)
{
	int count;
	struct circ_buf *xmit;

	if (port->x_char) {
		UART_PUT_CHAR(port, port->x_char);
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	xmit = &port->info->xmit;
	if (uart_circ_empty (xmit) || uart_tx_stopped (port)) {
		pnx010x_uart_stop_tx (port, 0);
		return;
	}

	/*
	   If Tx path is initially empty, we can safely write enough chars to fill
	   the Tx Fifo without fear of overflowing. If not, then at least the
	   Tx holding register must be clear before we can send the next char.
	*/
	count = pnx010x_uart_tx_empty (port) ? port->fifosize : 0;
	while ((--count >= 0) || pnx010x_uart_tx_space_available (port)) {
		UART_PUT_CHAR(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty (xmit))
			break;
	}

	/*
	   If the number of bytes remaining in the SW Tx circular buffer is less
	   than WAKEUP_CHARS, wake up tasks trying to write to the UART device.
	   If there are no more bytes to transmit, disable Tx interrupts.
	   (We don't need to refill the UART Tx fifo when it next becomes empty).
	*/
	if (uart_circ_chars_pending (xmit) < WAKEUP_CHARS) {
		uart_write_wakeup (port);
		if (uart_circ_empty (xmit))
			pnx010x_uart_stop_tx (port, 0);
	}
}

static void pnx010x_uart_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_port *port = dev_id;
	unsigned int interrupt_id;
//	unsigned int pass_counter = PNX010X_ISR_PASS_LIMIT;

	while (1) {
//		if (--pass_counter == 0)
//			break;

		interrupt_id = (UART_GET_IIR(port) & 0x0F);

		if ((interrupt_id == 0x04) ||		/* Rx data available (above desired level in fifo) */
		    (interrupt_id == 0x0C))		/* Rx data available (timeout) */
		{
#ifdef SUPPORT_SYSRQ
			pnx010x_uart_rx_chars (port, regs);
#else
			pnx010x_uart_rx_chars (port);
#endif
		}

		if (interrupt_id == 0x02)		/* Tx fifo is empty */
			pnx010x_uart_tx_chars (port);

		if (interrupt_id == 0x01)		/* no more interrupts left to service */
			break;
		else {					/* if all else fails, check the really unlikely stuff... */
			if (interrupt_id == 0x06)	/* overrun, parity, framing or break error */
				;			/* ignore for now (Note that reading the Int ID register clears the int request) */
			if (interrupt_id == 0x00)	/* Modem interrupt (will be cleared by reading MSR) */
				pnx010x_uart_modem_status (port);
		}
	}
}

static unsigned int pnx010x_uart_get_mctrl (struct uart_port *port)
{
	return 0;
}

static void pnx010x_uart_set_mctrl (struct uart_port *port, unsigned int mctrl)
{
}

static void pnx010x_uart_break_ctl (struct uart_port *port, int break_state)
{
}

static int pnx010x_uart_startup (struct uart_port *port)
{
	int retval;
#if 1
	unsigned int saint = 0;
#else
	unsigned int saint = SA_INTERRUPT;
#endif

	if ((retval = request_irq (port->irq, pnx010x_uart_isr, saint, (port->line == 0) ? "uart0" : "uart1", port))) {
		printk ("%s: request_irq() failed for port %d (irq == %d)\n", __FUNCTION__, port->line, port->irq);
		return retval;
	}

	UART_PUT_IER(port, 0x03);	/* enable Tx and Rx ints only (modem status enabled elsewhere ?) */

	return 0;
}

static void pnx010x_uart_shutdown (struct uart_port *port)
{
	UART_PUT_IER (port, 0x00);	/* disable all interrupts */
	free_irq (port->irq, port);
}


/*
    Baud rate settings are slightly complex...
    Between the base clock supplied to the UART (normally 12 MHz, but could be
    higher if the base is connected to one of the PLL outputs instead of to
    the input crystal directly), we have have the following knobs to tweak:

	a) The full-featured fractional divider in the CGU
	b) The simple fractional divider in the UART
	c) The standard uart Baud Rate Divider (integer divider only).

    To reduce power consumption, we want to:

	- Keep the UARTCLK base as low as possible.
	  ie use 12 MHz from crystal input rather than a PLL output.
	- Divide early rather than late.
	  ie its better to divide in a) rather than c) if the desired output
	  clock can be obtained in both cases.

    However... to keep things simple, we ignore the CGU fractional divider
    completely and use only the two dividers within the UART (which is good
    enough for most case).
*/

static void pnx010x_uart_change_speed (struct uart_port *port, unsigned int cflag, unsigned int iflag, unsigned int quot)
{
	struct bdlut {
		unsigned int baud;
		unsigned short fdr;
		unsigned short brd;
	};

	static const struct bdlut dividers_lut_12mhz[] =
	{
		{ 460800, ( 8 << 4) | ( 5 << 0),  1 },
		{ 230400, (11 << 4) | ( 7 << 0),  2 },
		{ 115200, (13 << 4) | ( 4 << 0),  5 },
		{  57600, (15 << 4) | (13 << 0),  7 },
		{  38400, ( 9 << 4) | (13 << 0),  8 },
		{  28800, (15 << 4) | ( 8 << 0), 17 },
		{  19200, (10 << 4) | (13 << 0), 17 },
		{  14400, (12 << 4) | (13 << 0), 25 },
		{   9600, (11 << 4) | ( 9 << 0), 43 },
		{      0,                     0,  0 }
	};

	static const struct bdlut dividers_lut_10mhz[] =
	{
		{ 460800, (14 << 4) | ( 5 << 0),  1 },
		{ 230400, ( 7 << 4) | (12 << 0),  1 },
		{ 115200, ( 7 << 4) | (12 << 0),  2 },
		{  57600, ( 7 << 4) | (12 << 0),  4 },
		{  38400, ( 7 << 4) | (12 << 0),  6 },
		{  28800, ( 7 << 4) | (12 << 0),  8 },
		{  19200, ( 7 << 4) | (12 << 0), 12 },
		{  14400, ( 5 << 4) | ( 2 << 0), 31 },
		{   9600, (10 << 4) | (11 << 0), 31 },
		{      0,                     0,  0 }
	};

	struct bdlut *dividers_lut;

	unsigned int rsm;	/* read status mask */
	unsigned int ism;	/* ignore status mask */
	unsigned int baud;
	unsigned long flags;

	rsm = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (iflag & INPCK)
		 rsm |= UART_LSR_FE | UART_LSR_PE;
	if (iflag & (BRKINT | PARMRK))
		 rsm |= UART_LSR_BI;

	ism = 0;
	if (iflag & IGNPAR)
		ism |= UART_LSR_PE | UART_LSR_FE;
	if (iflag & IGNBRK) {
		ism |= UART_LSR_BI;
		if (iflag & IGNPAR)		/* if we ignore parity and break... */
			ism |= UART_LSR_OE;	/* then ignore over-runs as well */
	}
	if ((cflag & CREAD) == 0)		/* ignore all chars if CREAD is not set... */
		ism |= UART_LSR_DR;

	baud = UART_VIRTUAL_CLK / (quot * 16);	/* serial core calls change_speed with quot = (uartclk / (16 * baudrate)) */

//	printk ("pnx010x_uart_change_speed: baud = %d\n", baud);

	if (uartclkhz == (115200 * 80))
		dividers_lut = dividers_lut_12mhz;
	else if (uartclkhz == (115200 * 32))
		dividers_lut = dividers_lut_10mhz;
	else
		return;

	spin_lock_irqsave (&port->lock, flags);

	port->read_status_mask = rsm;
	port->ignore_status_mask = ism;

	while (1) {
		if (dividers_lut->baud == 0)
			break;
		if (dividers_lut->baud == baud) {
			UART_PUT_LCR(port, UART_GET_LCR(port) | UART_LCR_DLAB); 	/* DLAB on */
			UART_PUT_DLM(port, dividers_lut->brd >> 8);
			UART_PUT_DLL(port, dividers_lut->brd & 0xFF);
			UART_PUT_LCR(port, UART_GET_LCR(port) & ~UART_LCR_DLAB);	/* DLAB off */
			UART_PUT_FDR(port, dividers_lut->fdr);
			break;
		}
		dividers_lut++;
	}

	spin_unlock_irqrestore (&port->lock, flags);
}

static const char *pnx010x_uart_type (struct uart_port *port)
{
	return (port->type == PORT_PNX010X ? "PNX010X" : NULL);
}

static void pnx010x_uart_release_port (struct uart_port *port)
{
	release_mem_region (port->mapbase, UART_PORT_SIZE);
}

static int pnx010x_uart_request_port (struct uart_port *port)
{
	if (request_mem_region (port->mapbase, UART_PORT_SIZE, "pnx010x_uart") == NULL)
		return -EBUSY;

	return 0;
}

static void pnx010x_uart_config_port (struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_PNX010X;
		pnx010x_uart_request_port (port);
	}
}

static int pnx010x_uart_verify_port (struct uart_port *port, struct serial_struct *ser)
{
	if ((ser->type != PORT_PNX010X) && (ser->type != PORT_UNKNOWN))
		return -EINVAL;
	if (ser->irq != port->irq)
		return -EINVAL;
	if (ser->baud_base < 9600)
		return -EINVAL;

	return 0;
}

static struct uart_ops pnx010x_uart_pops =
{
	.tx_empty	= pnx010x_uart_tx_empty,
	.set_mctrl	= pnx010x_uart_set_mctrl,
	.get_mctrl	= pnx010x_uart_get_mctrl,
	.stop_tx	= pnx010x_uart_stop_tx,
	.start_tx	= pnx010x_uart_start_tx,
	.stop_rx	= pnx010x_uart_stop_rx,
	.enable_ms	= pnx010x_uart_enable_ms,
	.break_ctl	= pnx010x_uart_break_ctl,
	.startup	= pnx010x_uart_startup,
	.shutdown	= pnx010x_uart_shutdown,
	.change_speed	= pnx010x_uart_change_speed,
	.type		= pnx010x_uart_type,
	.release_port	= pnx010x_uart_release_port,
	.request_port	= pnx010x_uart_request_port,
	.config_port	= pnx010x_uart_config_port,
	.verify_port	= pnx010x_uart_verify_port,
};

static struct uart_port pnx010x_ports[UART_NR] =
{
	{
		.iobase 	= 0,					/* driver doesn't use io ports */
		.membase	= (void *) IO_ADDRESS_UART0_BASE,	/* read/write[bwl] */
		.mapbase	= UART0_REGS_BASE,
		.iotype		= SERIAL_IO_MEM,
		.irq		= IRQ_UART_0,
		.uartclk	= UART_VIRTUAL_CLK,			/* serial core calls change_speed with quot = (uartclk / (16 * baudrate)) */
		.fifosize	= UART_TX_FIFO_BYTES,			/* PNX0106 fifo size == 64 (ssa was 16) */
		.flags		= ASYNC_BOOT_AUTOCONF,			/* autoconfigure port on bootup */
		.ops		= &pnx010x_uart_pops,
		.line		= 0,
	},
	{
		.iobase 	= 0,					/* driver doesn't use io ports */
		.membase	= (void *) IO_ADDRESS_UART1_BASE,	/* read/write[bwl] */
		.mapbase	= UART1_REGS_BASE,
		.iotype		= SERIAL_IO_MEM,
		.irq		= IRQ_UART_1,
		.uartclk	= UART_VIRTUAL_CLK,			/* serial core calls change_speed with quot = (uartclk / (16 * baudrate)) */
		.fifosize	= UART_TX_FIFO_BYTES,			/* PNX0106 fifo size == 64 (ssa was 16) */
		.flags		= ASYNC_BOOT_AUTOCONF,			/* autoconfigure port on bootup */
		.ops		= &pnx010x_uart_pops,
		.line		= 1,
	}
};


#if defined (CONFIG_DEBUG_PANIC_POSTMORTEM)

int uart_dppm_tx_char (char ch)
{
	struct uart_port *port = &pnx010x_ports[0];

	if (pnx010x_uart_tx_space_available (port)) {
		UART_PUT_CHAR(port, ch);
		return 1;
	}

	return 0;					/* Tx not possible (no space in FiFo) */
}

int uart_dppm_rx_char (char *ch)
{
	struct uart_port *port = &pnx010x_ports[0];

	if (pnx010x_uart_rx_char_available (port)) {
		*ch = UART_GET_CHAR(port);
		return 1;
	}

	return 0;					/* Rx not possible (no data available) */
}

int uart_dppm_rx_line (char *linebuf)
{
	char ch;
	int index = 0;

	/* Fixme... how much data can be written to linebuf ?? */

	while (1) {					/* block until complete line is received */
		if (uart_dppm_rx_char (&ch)) {
			uart_dppm_tx_char (ch);
			linebuf[index++] = ch;
			if (ch == '\r') {		/* fixme... what about '\n' ?!? */
				linebuf[--index] = 0;
				return index;
			}
		}
	}
}

#endif


#ifdef CONFIG_SERIAL_PNX010X_CONSOLE

static void pnx010x_uart_console_write (struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port;
	unsigned int ier_saved;
	unsigned int ch;
	int i;

	port = pnx010x_ports + co->index;

	ier_saved = UART_GET_IER(port);
	UART_PUT_IER(port, 0x00);

	for (i = 0; i < count; i++) {
		ch = *s++;
		if (ch == '\n') {
			while ((UART_GET_LSR(port) & (1 << 5)) == 0)	/* wait until Tx holding register is empty (fifo may contain some data) */
				;
			UART_PUT_CHAR(port, '\r');
		}
		while ((UART_GET_LSR(port) & (1 << 5)) == 0)		/* wait until Tx holding register is empty (fifo may contain some data) */
			;
		UART_PUT_CHAR(port, ch);
	}

//	if (tty_driver_started)
		while ((UART_GET_LSR(port) & (1 << 6)) == 0)		/* wait until Transmitter Empty is set */
			;

	UART_PUT_IER(port, ier_saved);
}

static kdev_t pnx010x_uart_console_device (struct console *co)
{
	return MKDEV(SERIAL_PNX010X_MAJOR, SERIAL_PNX010X_MINOR + co->index);
}

static int __init pnx010x_uart_console_setup (struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int parity = 'n';
	int bits = 8;
	int flow = 'n';

#if defined (CONSOLE_IN_USE)
	printk ("%s: console in use\n", __FUNCTION__);
	CONSOLE_IN_USE = 1;
#endif

	port = &pnx010x_ports[0];	/* console always uses uart0 */
	if (options)
		uart_parse_options (options, &baud, &parity, &bits, &flow);

	return uart_set_options (port, co, baud, parity, bits, flow);
}

static struct console pnx010x_console =
{
	name:   "ttyS",
	write:  pnx010x_uart_console_write,
	device: pnx010x_uart_console_device,
	setup:  pnx010x_uart_console_setup,
	flags:  CON_PRINTBUFFER,
	index:  -1,
};

void __init pnx010x_uart_console_init (void)
{
	register_console (&pnx010x_console);
}

#define PNX010X_CONSOLE    &pnx010x_console

#else
#define PNX010X_CONSOLE    NULL
#endif


static struct uart_driver pnx010x_reg =
{
	owner:                  THIS_MODULE,
	normal_major:           SERIAL_PNX010X_MAJOR,
#ifdef CONFIG_DEVFS_FS
	normal_name:            "ttyS%d",
	callout_name:           "cua%d",
#else
	normal_name:            "ttyS",
	callout_name:           "cua",
#endif
	normal_driver:          &normal,
	callout_major:          CALLOUT_PNX010X_MAJOR,
	callout_driver:         &callout,
	table:                  pnx010x_table,
	termios:                pnx010x_termios,
	termios_locked:         pnx010x_termios_locked,
	minor:                  SERIAL_PNX010X_MINOR,
	nr:                     UART_NR,
	cons:                   PNX010X_CONSOLE,
};

static int __init pnx010x_uart_init (void)
{
	int i;
	int ret;

	if ((ret = uart_register_driver (&pnx010x_reg)) == 0)
		for (i = 0; i < UART_NR; i++)
			uart_add_one_port (&pnx010x_reg, &pnx010x_ports[i]);

	return ret;
}

static void __exit pnx010x_uart_exit (void)
{
	int i;

	for (i = 0; i < UART_NR; i++)
		uart_remove_one_port (&pnx010x_reg, &pnx010x_ports[i]);

	uart_unregister_driver (&pnx010x_reg);
}

module_init(pnx010x_uart_init);
module_exit(pnx010x_uart_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Andre McCurdy");
MODULE_DESCRIPTION("Philips PNX010X serial port driver");
MODULE_LICENSE("GPL");

