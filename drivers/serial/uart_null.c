/*
 *  linux/drivers/serial/uart_null.c
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

#include <linux/serial_core.h>


#define DEVNAME 		"ttynull"

#define UART_NR 		(1)

#define SERIAL_MAJOR		(206)
#define SERIAL_MINOR		(16)

#define CALLOUT_MAJOR		(SERIAL_MAJOR + 1)
#define CALLOUT_MINOR		(SERIAL_MINOR)

#define NULL_UART_XTAL_HZ	(3686400)

#define IER_RX                  (1 << 0)
#define IER_TX                  (1 << 1)


/****************************************************************************
****************************************************************************/

static struct tty_driver normal;
static struct tty_driver callout;
static struct tty_struct *null_table[UART_NR];
static struct termios *null_termios[UART_NR];
static struct termios *null_termios_locked[UART_NR];
#if defined (CONFIG_SERIAL_NULL_CONSOLE)
static struct console null_console;
#endif

struct null_uart_state
{
	struct uart_port port;
	struct timer_list timer;
	unsigned int ier;
};

static struct null_uart_state null_uart_state[UART_NR];

#define UART_STATE(p)	((struct null_uart_state *) (p))


/****************************************************************************
****************************************************************************/

static void null_uart_tx_chars (struct uart_port *port);

static void null_uart_timer_run (struct null_uart_state *state)
{
	mod_timer (&state->timer, jiffies + (HZ / 10));
}

static void null_uart_start_tx (struct uart_port *port, unsigned int from_tty)
{
	struct null_uart_state *state = UART_STATE(port);
	unsigned long flags;
	unsigned int ier;

	spin_lock_irqsave (&port->lock, flags);
#if 0
	ier = state->ier;
	state->ier = (ier | IER_TX);		/* enable Tx interrupts */
	if (ier == 0)				/* if previous ier was 0, we need to kick off the timer... */
		null_uart_timer_run (state);
#else
	null_uart_tx_chars (port);		/* Tx all chars here... don't wait for the timer */
#endif
	spin_unlock_irqrestore (&port->lock, flags);
}

static void null_uart_stop_tx (struct uart_port *port, unsigned int from_tty)
{
	struct null_uart_state *state = UART_STATE(port);
	unsigned int ier;

	ier = state->ier;
	ier &= ~IER_TX; 		/* disable Tx interrupts */
	state->ier = ier;

	if (ier == 0)
		del_timer_sync (&state->timer);
}

static void null_uart_stop_rx (struct uart_port *port)
{
	struct null_uart_state *state = UART_STATE(port);
	unsigned int ier;

	ier = state->ier;
	ier &= ~IER_RX; 		/* disable Rx interrupts */
	state->ier = ier;

	if (ier == 0)
		del_timer_sync (&state->timer);
}

static void null_uart_enable_ms (struct uart_port *port)
{
}

static void null_uart_modem_status (struct uart_port *port)
{
}

static unsigned int null_uart_tx_empty (struct uart_port *port)
{
	return TIOCSER_TEMT;
}

static void null_uart_tx_chars (struct uart_port *port)
{
	struct circ_buf *xmit;

	if (port->x_char) {
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	if (uart_tx_stopped (port))
		goto stop_tx;

	xmit = &port->info->xmit;
	while (!(uart_circ_empty (xmit))) {
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
	uart_write_wakeup (port);

stop_tx:
	null_uart_stop_tx (port, 0);
}

static void null_uart_timer_handler (unsigned long data)
{
	struct uart_port *port = (struct uart_port *) data;
	struct null_uart_state *state = UART_STATE(port);
	unsigned long flags;

	spin_lock_irqsave (&port->lock, flags);
	null_uart_tx_chars (port);
	if (state->ier != 0)
		null_uart_timer_run (state);
	spin_unlock_irqrestore (&port->lock, flags);
}

static unsigned int null_uart_get_mctrl (struct uart_port *port)
{
	return 0;
}

static void null_uart_set_mctrl (struct uart_port *port, unsigned int mctrl)
{
}

static void null_uart_break_ctl (struct uart_port *port, int break_state)
{
}

static int null_uart_startup (struct uart_port *port)
{
	struct null_uart_state *state = UART_STATE(port);

	state->ier = (IER_TX | IER_RX);
	null_uart_timer_run (state);
	return 0;
}

static void null_uart_shutdown (struct uart_port *port)
{
	struct null_uart_state *state = UART_STATE(port);
	unsigned long flags;

	spin_lock_irqsave (&port->lock, flags);
	state->ier = 0;
	del_timer_sync (&state->timer);
	spin_unlock_irqrestore (&port->lock, flags);
}

static void null_uart_change_speed (struct uart_port *port, unsigned int cflag, unsigned int iflag, unsigned int quot)
{
}

static const char *null_uart_type (struct uart_port *port)
{
	return (port->type == PORT_SC16IS7X0 ? "NULLUART" : NULL);
}

static void null_uart_release_port (struct uart_port *port)
{
}

static int null_uart_request_port (struct uart_port *port)
{
	return 0;
}

static void null_uart_config_port (struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_NULL;
		null_uart_request_port (port);
	}
}

static int null_uart_verify_port (struct uart_port *port, struct serial_struct *ser)
{
	if ((ser->type != PORT_NULL) && (ser->type != PORT_UNKNOWN))
		return -EINVAL;
	if (ser->baud_base < 9600)
		return -EINVAL;

	return 0;
}

static struct uart_ops null_uart_pops =
{
	.tx_empty	= null_uart_tx_empty,
	.set_mctrl	= null_uart_set_mctrl,
	.get_mctrl	= null_uart_get_mctrl,
	.stop_tx	= null_uart_stop_tx,
	.start_tx	= null_uart_start_tx,
	.stop_rx	= null_uart_stop_rx,
	.enable_ms	= null_uart_enable_ms,
	.break_ctl	= null_uart_break_ctl,
	.startup	= null_uart_startup,
	.shutdown	= null_uart_shutdown,
	.change_speed	= null_uart_change_speed,
	.type		= null_uart_type,
	.release_port	= null_uart_release_port,
	.request_port	= null_uart_request_port,
	.config_port	= null_uart_config_port,
	.verify_port	= null_uart_verify_port,
};

static void __init null_uart_init_ports (void)
{
	int i;
	struct null_uart_state *state;

	if (null_uart_state[0].port.membase != 0)
		return;

	for (i = 0; i < UART_NR; i++){
		state = &null_uart_state[i];
		state->port.line	= i;
		state->port.membase	= (void *) NULLUART_REGS_BASE_DUMMY;
		state->port.mapbase	= NULLUART_REGS_BASE_DUMMY;
		state->port.iotype	= SERIAL_IO_MEM;
		state->port.uartclk	= NULL_UART_XTAL_HZ;
		state->port.fifosize	= 64;				/* Tx Fifo size */
		state->port.flags	= ASYNC_BOOT_AUTOCONF;
		state->port.ops 	= &null_uart_pops;
		init_timer (&state->timer);
		state->timer.function = null_uart_timer_handler;
		state->timer.data = (unsigned long) &state->port;
	}
}

#ifdef CONFIG_SERIAL_NULL_CONSOLE

static void null_uart_console_write (struct console *co, const char *s, unsigned int count)
{
}

static kdev_t null_uart_console_device (struct console *co)
{
	return MKDEV(SERIAL_MAJOR, SERIAL_MINOR + co->index);
}

static int __init null_uart_console_setup (struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int parity = 'n';
	int bits = 8;
	int flow = 'n';

	port = &null_uart_state[0].port; 	/* console always uses first uart */

	if (options)
		uart_parse_options (options, &baud, &parity, &bits, &flow);

	return uart_set_options (port, co, baud, parity, bits, flow);
}

static struct console null_console = {
	name:   DEVNAME,
	write:  null_uart_console_write,
	device: null_uart_console_device,
	setup:  null_uart_console_setup,
	flags:  CON_PRINTBUFFER,
	index:  -1,
};

void __init null_uart_console_init (void)
{
	null_uart_init_ports();
	register_console (&null_console);
}

#define NULL_CONSOLE    &null_console
#else
#define NULL_CONSOLE    NULL
#endif

static struct uart_driver null_reg =
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
	table:                  null_table,
	termios:                null_termios,
	termios_locked:         null_termios_locked,
	minor:                  SERIAL_MINOR,
	nr:                     UART_NR,
	cons:                   NULL_CONSOLE,
};

static int __init null_uart_init (void)
{
	int i;
	int ret;

	null_uart_init_ports();
	if ((ret = uart_register_driver (&null_reg)) == 0)
		for (i = 0; i < UART_NR; i++)
			uart_add_one_port (&null_reg, &null_uart_state[i].port);

	return ret;
}

static void __exit null_uart_exit (void)
{
	int i;

	for (i = 0; i < UART_NR; i++)
		uart_remove_one_port (&null_reg, &null_uart_state[i].port);

	uart_unregister_driver (&null_reg);
}

module_init(null_uart_init);
module_exit(null_uart_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Andre McCurdy");
MODULE_DESCRIPTION("NXP NULL UART serial port driver");
MODULE_LICENSE("GPL");

