/*
 *  linux/drivers/serial/spi_sc16is7x0.c
 *
 *  Copyright (C) 2007 Andre McCurdy, NXP Semiconductors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/console.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/spisd.h>

#include <linux/serial_core.h>


#define DEVNAME 		"ttyspi"

#define UART_NR 		(1)

#define SERIAL_MAJOR		(204)
#define SERIAL_MINOR		(16)

#define CALLOUT_MAJOR		(SERIAL_MAJOR + 1)
#define CALLOUT_MINOR		(SERIAL_MINOR)

#define SPI_UART_XTAL_HZ	(3686400)

/*
   Register definitions
*/
#define RHR                     (0x00 << 3)     /* R- : Rx Holding Register */
#define THR                     (0x00 << 3)     /* -W : Tx Holding Register */
#define IER                     (0x01 << 3)     /* RW : Interrupt Enable Register */
#define IIR                     (0x02 << 3)     /* R- : Interrupt ID Register */
#define FCR                     (0x02 << 3)     /* -W : Fifo Control Register */
#define LCR                     (0x03 << 3)     /* RW : Line Control Register */
#define MCR                     (0x04 << 3)     /* RW : Modem Control Register */
#define LSR                     (0x05 << 3)     /* R- : Line Status Register */
#define MSR                     (0x06 << 3)     /* R- : Modem Status Register */
#define SPR                     (0x07 << 3)     /* */
#define TCR                     (0x06 << 3)     /* */
#define TLR                     (0x07 << 3)     /* */
#define TXLVL                   (0x08 << 3)     /* R- : Tx FiFo space available (0..64) */
#define RXLVL                   (0x09 << 3)     /* R- : Rx FiFo valid data available (0..64) */

#define IODIR                   (0x0A << 3)     /* */
#define IOSTATE                 (0x0B << 3)     /* */
#define IOINTEN                 (0x0C << 3)     /* */
#define IOCONTROL               (0x0E << 3)     /* */

#define EFCR                    (0x0F << 3)     /* */
#define DLL                     (0x00 << 3)     /* RW : Baud rate divisor lsb (overlays reg 0 when bit7 in LCR is set) */
#define DLH                     (0x01 << 3)     /* RW : Baud rate divisor msb (overlays reg 1 when bit7 in LCR is set) */
#define EFR                     (0x02 << 3)     /* */
#define XON1                    (0x04 << 3)     /* */
#define XON2                    (0x05 << 3)     /* */
#define XOFF1                   (0x06 << 3)     /* */
#define XOFF2                   (0x07 << 3)     /* */

#define IER_RX                  (1 << 0)
#define IER_TX                  (1 << 1)

#define MSR_DELTA_CTS		(1 << 0)
#define MSR_DELTA_DSR		(1 << 1)
#define MSR_DELTA_RI		(1 << 2)
#define MSR_DELTA_DCD		(1 << 3)
#define MSR_CTS 		(1 << 4)
#define MSR_DSR			(1 << 5)
#define MSR_RI			(1 << 6)
#define MSR_DCD			(1 << 7)

#define MCR_DTR			(1 << 0)
#define MCR_RTS			(1 << 1)

#if (UART_NR == 1)

#define UART_GET_CHAR(p)	spi_uart_reg_read (RHR)
#define UART_GET_IER(p)		spi_uart_reg_read (IER)
#define UART_GET_IIR(p)		spi_uart_reg_read (IIR)
#define UART_GET_LCR(p)		spi_uart_reg_read (LCR)
#define UART_GET_MCR(p)		spi_uart_reg_read (MCR)
#define UART_GET_LSR(p)		spi_uart_reg_read (LSR)
#define UART_GET_MSR(p)		spi_uart_reg_read (MSR)
#define UART_GET_TXLVL(p)	spi_uart_reg_read (TXLVL)
#define UART_GET_RXLVL(p)	spi_uart_reg_read (RXLVL)

#define UART_PUT_CHAR(p,c)	spi_uart_reg_write (THR, (c))
#define UART_PUT_IER(p,c)	spi_uart_reg_write (IER, (c))
#define UART_PUT_LCR(p,c)	spi_uart_reg_write (LCR, (c))
#define UART_PUT_MCR(p,c)	spi_uart_reg_write (MCR, (c))
#define UART_PUT_DLL(p,c)	spi_uart_reg_write (DLL, (c))
#define UART_PUT_DLH(p,c)	spi_uart_reg_write (DLH, (c))

#else

#define UART_GET_CHAR(p)	spi_uart_reg_read (RHR | ((p)->line << 1))
#define UART_GET_IER(p)		spi_uart_reg_read (IER | ((p)->line << 1))
#define UART_GET_IIR(p)		spi_uart_reg_read (IIR | ((p)->line << 1))
#define UART_GET_LCR(p)		spi_uart_reg_read (LCR | ((p)->line << 1))
#define UART_GET_MCR(p)		spi_uart_reg_read (MCR | ((p)->line << 1))
#define UART_GET_LSR(p)		spi_uart_reg_read (LSR | ((p)->line << 1))
#define UART_GET_MSR(p)		spi_uart_reg_read (MSR | ((p)->line << 1))
#define UART_GET_TXLVL(p)	spi_uart_reg_read (TXLVL | ((p)->line << 1))
#define UART_GET_RXLVL(p)	spi_uart_reg_read (RXLVL | ((p)->line << 1))

#define UART_PUT_CHAR(p,c)	spi_uart_reg_write (THR | ((p)->line << 1), (c))
#define UART_PUT_IER(p,c)	spi_uart_reg_write (IER | ((p)->line << 1), (c))
#define UART_PUT_LCR(p,c)	spi_uart_reg_write (LCR | ((p)->line << 1), (c))
#define UART_PUT_MCR(p,c)	spi_uart_reg_write (MCR | ((p)->line << 1), (c))
#define UART_PUT_DLL(p,c)	spi_uart_reg_write (DLL | ((p)->line << 1), (c))
#define UART_PUT_DLH(p,c)	spi_uart_reg_write (DLH | ((p)->line << 1), (c))

#endif


/****************************************************************************
****************************************************************************/

static struct tty_driver normal;
static struct tty_driver callout;
static struct tty_struct *spi_table[UART_NR];
static struct termios *spi_termios[UART_NR];
static struct termios *spi_termios_locked[UART_NR];
#if defined (CONFIG_SERIAL_SC16IS7X0_CONSOLE)
static struct console spi_console;
#endif

struct spi_uart_state
{
	struct uart_port port;
	struct timer_list timer;
	unsigned int ier;
};

static struct spi_uart_state spi_uart_state[UART_NR];

#define UART_STATE(p)	((struct spi_uart_state *) (p))


/****************************************************************************
****************************************************************************/

static void spi_uart_access_setup (unsigned int regaddr)
{
	spisd_ncs_drive_low();
	spisd_write_single8 (regaddr);
}

static void spi_uart_access_complete (void)
{
	spisd_ncs_drive_high();
	ndelay (200);		/* delay here to meet nCS minimum high pulse width (100 or 200 nsec, depending on device) */
}

static unsigned int spi_uart_reg_read (unsigned int reg)
{
	unsigned int value;

	spi_uart_access_setup ((1 << 7) | reg);
	value = spisd_read_single8();
	spi_uart_access_complete();
	return value;
}

static void spi_uart_reg_write (unsigned int reg, unsigned int value)
{
	spi_uart_access_setup ((0 << 7) | reg);
	spisd_write_single8 (value);
	spi_uart_access_complete();
}


/****************************************************************************
****************************************************************************/

static void spi_uart_tx_chars (struct uart_port *port);

static void spi_uart_timer_run (struct spi_uart_state *state)
{
	mod_timer (&state->timer, jiffies + 1);
}

static void spi_uart_start_tx (struct uart_port *port, unsigned int from_tty)
{
	struct spi_uart_state *state = UART_STATE(port);
	unsigned long flags;
	unsigned int ier;

	spin_lock_irqsave (&port->lock, flags);
#if 0
	ier = state->ier;
	state->ier = (ier | IER_TX);		/* enable Tx interrupts */
	if (ier == 0)				/* if previous ier was 0, we need to kick off the timer... */
		spi_uart_timer_run (state);
#else
	spi_uart_tx_chars (port);		/* Tx all chars here... don't wait for the timer */
#endif
	spin_unlock_irqrestore (&port->lock, flags);
}

static void spi_uart_stop_tx (struct uart_port *port, unsigned int from_tty)
{
	struct spi_uart_state *state = UART_STATE(port);
	unsigned int ier;

	ier = state->ier;
	ier &= ~IER_TX; 		/* disable Tx interrupts */
	state->ier = ier;

	if (ier == 0)
		del_timer_sync (&state->timer);
}

static void spi_uart_stop_rx (struct uart_port *port)
{
	struct spi_uart_state *state = UART_STATE(port);
	unsigned int ier;

	ier = state->ier;
	ier &= ~IER_RX; 		/* disable Rx interrupts */
	state->ier = ier;

	if (ier == 0)
		del_timer_sync (&state->timer);
}

static void spi_uart_enable_ms (struct uart_port *port)
{
}

static void spi_uart_modem_status (struct uart_port *port)
{
}

static unsigned int spi_uart_tx_empty (struct uart_port *port)
{
	unsigned int result;
	unsigned long flags;

	/*
	   TEMT (bit 6 in the Line Status Register) is set when both
	   the Tx FiFo and the Tx holding register are empty.
	*/
	spin_lock_irqsave (&port->lock, flags);
	result = (UART_GET_LSR(port) & (1 << 6)) ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore (&port->lock, flags);

	return result;
}

static void spi_uart_rx_chars (struct uart_port *port)
{
	struct tty_struct *tty = port->info->tty;
	int count = UART_GET_RXLVL(port);

	if (count == 0)
		return;

	while (--count >= 0) {
		*tty->flip.char_buf_ptr++ = UART_GET_CHAR(port);
		*tty->flip.flag_buf_ptr++ = TTY_NORMAL; 	/* Fixme: we should check for overrun, parity, framing errors etc */
		port->icount.rx++;
		tty->flip.count++;
		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			break;
	}

	tty_flip_buffer_push (tty);
}

static void spi_uart_tx_chars (struct uart_port *port)
{
	struct circ_buf *xmit;
	int count = UART_GET_TXLVL(port);

	if (count == 0)
		return;

	if (port->x_char) {
		UART_PUT_CHAR(port, port->x_char);
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	xmit = &port->info->xmit;
	if (uart_circ_empty (xmit) || uart_tx_stopped (port)) {
		spi_uart_stop_tx (port, 0);
		return;
	}

	while (--count >= 0) {
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
			spi_uart_stop_tx (port, 0);
	}
}

static void spi_uart_timer_handler (unsigned long data)
{
	struct uart_port *port = (struct uart_port *) data;
	struct spi_uart_state *state = UART_STATE(port);
	unsigned long flags;

	spin_lock_irqsave (&port->lock, flags);

	spi_uart_rx_chars (port);
	spi_uart_tx_chars (port);

	if (state->ier != 0)
		spi_uart_timer_run (state);

	spin_unlock_irqrestore (&port->lock, flags);
}

static unsigned int spi_uart_get_mctrl (struct uart_port *port)
{
	return 0;
}

static void spi_uart_set_mctrl (struct uart_port *port, unsigned int mctrl)
{
}

static void spi_uart_break_ctl (struct uart_port *port, int break_state)
{
}

static int spi_uart_startup (struct uart_port *port)
{
	struct spi_uart_state *state = UART_STATE(port);

	state->ier = (IER_TX | IER_RX);
	spi_uart_timer_run (state);
	return 0;
}

static void spi_uart_shutdown (struct uart_port *port)
{
	struct spi_uart_state *state = UART_STATE(port);
	unsigned long flags;

	spin_lock_irqsave (&port->lock, flags);
	state->ier = 0;
	del_timer_sync (&state->timer);
	spin_unlock_irqrestore (&port->lock, flags);
}

static void spi_uart_change_speed (struct uart_port *port, unsigned int cflag, unsigned int iflag, unsigned int quot)
{
#if 0
	unsigned int rsm;				/* read status mask */
	unsigned int ism;				/* ignore status mask */
	unsigned int lcr;
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
		if (iflag & IGNPAR)			/* if we ignore parity and break... */
			ism |= UART_LSR_OE;		/* then ignore over-runs as well */
	}
	if ((cflag & CREAD) == 0)			/* ignore all chars if CREAD is not set... */
		ism |= UART_LSR_DR;

	/* Note: serial core calls change_speed with quot = (uartclk / (16 * baudrate)) */
	/* ie baud = uartclk / (quot * 16); */

	spin_lock_irqsave (&port->lock, flags);

	port->read_status_mask = rsm;
	port->ignore_status_mask = ism;

	lcr = UART_GET_LCR(port);
	UART_PUT_LCR(port, lcr | UART_LCR_DLAB);	/* DLAB on */
	UART_PUT_DLH(port, (quot >> 8) & 0xFF);
	UART_PUT_DLL(port, (quot >> 0) & 0xFF);
	UART_PUT_LCR(port, lcr);			/* DLAB off */

	spin_unlock_irqrestore (&port->lock, flags);
#endif
}

static const char *spi_uart_type (struct uart_port *port)
{
	return (port->type == PORT_SC16IS7X0 ? "SC16IS7X0" : NULL);
}

static void spi_uart_release_port (struct uart_port *port)
{
}

static int spi_uart_request_port (struct uart_port *port)
{
	return 0;
}

static void spi_uart_config_port (struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_SC16IS7X0;
		spi_uart_request_port (port);
	}
}

static int spi_uart_verify_port (struct uart_port *port, struct serial_struct *ser)
{
	if ((ser->type != PORT_SC16IS7X0) && (ser->type != PORT_UNKNOWN))
		return -EINVAL;
	if (ser->irq != port->irq)
		return -EINVAL;
	if (ser->baud_base < 9600)
		return -EINVAL;

	return 0;
}

static struct uart_ops spi_uart_pops =
{
	.tx_empty	= spi_uart_tx_empty,
	.set_mctrl	= spi_uart_set_mctrl,
	.get_mctrl	= spi_uart_get_mctrl,
	.stop_tx	= spi_uart_stop_tx,
	.start_tx	= spi_uart_start_tx,
	.stop_rx	= spi_uart_stop_rx,
	.enable_ms	= spi_uart_enable_ms,
	.break_ctl	= spi_uart_break_ctl,
	.startup	= spi_uart_startup,
	.shutdown	= spi_uart_shutdown,
	.change_speed	= spi_uart_change_speed,
	.type		= spi_uart_type,
	.release_port	= spi_uart_release_port,
	.request_port	= spi_uart_request_port,
	.config_port	= spi_uart_config_port,
	.verify_port	= spi_uart_verify_port,
};

static void __init spi_uart_init_ports (void)
{
	int i;
	struct spi_uart_state *state;

	if (spi_uart_state[0].port.membase != 0)
		return;

	for (i = 0; i < UART_NR; i++){
		state = &spi_uart_state[i];
		state->port.line	= i;
		state->port.membase	= (void *) SPIUART_REGS_BASE_DUMMY;
		state->port.mapbase	= SPIUART_REGS_BASE_DUMMY;
		state->port.iotype	= SERIAL_IO_MEM;
		state->port.uartclk	= SPI_UART_XTAL_HZ;
		state->port.fifosize	= 64;				/* Tx Fifo size */
		state->port.flags	= ASYNC_BOOT_AUTOCONF;
		state->port.ops 	= &spi_uart_pops;
		init_timer (&state->timer);
		state->timer.function = spi_uart_timer_handler;
		state->timer.data = (unsigned long) &state->port;
	}
}

#ifdef CONFIG_SERIAL_SC16IS7X0_CONSOLE

static void spi_uart_console_write (struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &spi_uart_state[co->index].port;
	unsigned long flags;
	unsigned int ch;
	int i;

	spin_lock_irqsave (&port->lock, flags);

	for (i = 0; i < count; i++) {
		ch = *s++;
		if (ch == '\n') {
			while (UART_GET_TXLVL(port) == 0)		/* wait until Tx can accept data */
				;
			UART_PUT_CHAR(port, '\r');
		}
		while (UART_GET_TXLVL(port) == 0)			/* wait until Tx can accept data */
			;
		UART_PUT_CHAR(port, ch);
	}

	while ((UART_GET_LSR(port) & (1 << 6)) == 0)			/* wait until Transmitter Empty is set */
			;

	spin_unlock_irqrestore (&port->lock, flags);
}

static kdev_t spi_uart_console_device (struct console *co)
{
	return MKDEV(SERIAL_MAJOR, SERIAL_MINOR + co->index);
}

static int __init spi_uart_console_setup (struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int parity = 'n';
	int bits = 8;
	int flow = 'n';

	port = &spi_uart_state[0].port; 	/* console always uses first uart */

	if (options)
		uart_parse_options (options, &baud, &parity, &bits, &flow);

	return uart_set_options (port, co, baud, parity, bits, flow);
}

static struct console spi_console = {
	name:   DEVNAME,
	write:  spi_uart_console_write,
	device: spi_uart_console_device,
	setup:  spi_uart_console_setup,
	flags:  CON_PRINTBUFFER,
	index:  -1,
};

void __init spi_uart_console_init (void)
{
	spi_uart_init_ports();
	register_console (&spi_console);
}

#define SPI_CONSOLE    &spi_console
#else
#define SPI_CONSOLE    NULL
#endif

static struct uart_driver spi_reg =
{
	owner:                  THIS_MODULE,
	normal_major:           SERIAL_MAJOR,
#ifdef CONFIG_DEVFS_FS
	normal_name:            DEVNAME "%d",
	callout_name:           DEVNAME "cua%d",
#else
	normal_name:            DEVNAME,
	callout_name:           DEVNAME "cua",
#endif
	normal_driver:          &normal,
	callout_major:          CALLOUT_MAJOR,
	callout_driver:         &callout,
	table:                  spi_table,
	termios:                spi_termios,
	termios_locked:         spi_termios_locked,
	minor:                  SERIAL_MINOR,
	nr:                     UART_NR,
	cons:                   SPI_CONSOLE,
};

static int __init spi_uart_init (void)
{
	int i;
	int ret;

	spi_uart_init_ports();
	if ((ret = uart_register_driver (&spi_reg)) == 0)
		for (i = 0; i < UART_NR; i++)
			uart_add_one_port (&spi_reg, &spi_uart_state[i].port);

	return ret;
}

static void __exit spi_uart_exit (void)
{
	int i;

	for (i = 0; i < UART_NR; i++)
		uart_remove_one_port (&spi_reg, &spi_uart_state[i].port);

	uart_unregister_driver (&spi_reg);
}

module_init(spi_uart_init);
module_exit(spi_uart_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Andre McCurdy");
MODULE_DESCRIPTION("NXP SPI UART serial port driver");
MODULE_LICENSE("GPL");

