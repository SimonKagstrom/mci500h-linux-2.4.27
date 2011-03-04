/*
 * linux/arch/arm/mach-pnx0106/uart1_debug.c
 *
 * Copyright (C) 2005 Andre McCurdy, Philips Semiconductors.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/arch/hardware.h>
#include <asm/arch/dbgmsg.h>


/****************************************************************************
****************************************************************************/

typedef struct
{
    volatile unsigned int RBR;      /* R- : 0x000 : Receive Buffer Register */
    #define THR RBR                 /* -W : 0x000 : Transmit Holding Register */
    #define DLL RBR                 /* RW : 0x000 : Divisor Latch Least significant byte (when LCR bit7 is set) */
    volatile unsigned int IER;      /* RW : 0x004 : Interrupt Enable Register */
    #define DLM IER                 /* RW : 0x004 : Divisor Latch Most significant byte (when LCR bit7 is set) */
    volatile unsigned int IIR;      /* R- : 0x008 : Interrupt ID Register */
    #define FCR IIR                 /* -W : 0x008 : Fifo Control Register */
    volatile unsigned int LCR;      /* RW : 0x00C : Line Control Register */
    volatile unsigned int MCR;      /* RW : 0x010 : Modem Control Register */
    volatile unsigned int LSR;      /* R- : 0x014 : Line Status Register */
    volatile unsigned int MSR;      /* R- : 0x018 : Modem Status Register */
    volatile unsigned int SCR;      /* RW : 0x01C : Scratch Register */
    volatile unsigned int ACR;      /* RW : 0x020 : Auto-Baud Control Register */
    volatile unsigned int ICR;      /* RW : 0x024 : IRDA Control Register */
    volatile unsigned int FDR;      /* RW : 0x028 : Fractional Divider Register */

    volatile unsigned int _d0;

    volatile unsigned int POP;      /* -W : 0x030 : NHP POP Register... */
    volatile unsigned int MODE;     /* RW : 0x034 : NHP Mode select Register... */

    volatile unsigned int _d1[(0xFD4 - 0x038) / 4];

    volatile unsigned int CFG;      /* R- : 0xFD4 : Configuration Register... */
    volatile unsigned int INTCE;    /* -W : 0xFD8 : Interrupt Clear Enable Register... */
    volatile unsigned int INTSE;    /* -W : 0xFDC : Interrupt Set Enable Register... */
    volatile unsigned int INTS;     /* R- : 0xFE0 : Interrupt Status Register... */
    volatile unsigned int INTE;     /* R- : 0xFE4 : Interrupt Enable Register... */
    volatile unsigned int INTCS;    /* -W : 0xFE8 : Interrupt Clear Status Register... */
    volatile unsigned int INTSS;    /* -W : 0xFEC : Interrupt Set Status Register... */

    volatile unsigned int _d2[(0xFFC - 0xFF0) / 4];

    volatile unsigned int ID;       /* R- : 0xFFC : Module ID Register */
}
UART_REGS_t;

#define UART ((UART_REGS_t *) IO_ADDRESS_UART1_BASE)


/****************************************************************************
****************************************************************************/

/*
   Receive one character.
   Non-blocking: returns 1 if a char was available, otherwise 0.
*/
#if 0
static int uart_rx_char (unsigned char *buf)
{
    if ((UART->LSR & (1 << 0)) == 0)                /* no data available in the Rx Buffer register */
        return 0;

    *buf = (unsigned char) UART->THR;

    return 1;
}
#endif

/*
   Transmit one character.
   Blocking (for upto one char period... ie approx 87 uSec at 115200 baud)
*/
static void uart_tx_char (unsigned char ch)
{
#if 0
    do { } while ((UART->LSR & (1 << 6)) == 0);     /* wait until Transmitter Empty is set */
#else
    do { } while ((UART->LSR & (1 << 5)) == 0);     /* wait until Tx holding register is empty (fifo may contain some data) */
#endif

    UART->THR = ch;                                 /* write to Tx holding register */
}


/****************************************************************************
****************************************************************************/

static void out_hex (unsigned int value, int digits)
{
    int i;

    for (i = (digits - 1); i >= 0; i--) {
        unsigned int temp = ((value >> (i * 4)) & 0x0F);
        uart_tx_char ((unsigned char) (temp + ((temp < 10) ? '0' : ('A' - 10))));
    }
}

static int out_decimal (unsigned int value, int min_width)
{
    char buf[10 + 2];
    char *ptr = buf;
    int length;

    do {
        *ptr++ = (char) ((value % 10) + '0');
        value = (value / 10);
    }
    while (value);

    length = (ptr - buf);

    while (length < min_width) {
        uart_tx_char (' ');
        min_width--;
    }

    ptr--;

    while (ptr >= buf)
        uart_tx_char (*ptr--);

    return length;
}

static void out_string (char *string)
{
    if (!string)
        string = "(null)";

    while (1) {
        char ch = *string++;
        if (ch == 0)
            break;
        if (ch == '\n')
            uart_tx_char ('\r');
        uart_tx_char (ch);
    }
}

static int bl_strlen (char *s)
{
    char *start = s + 1;

    while (*s++ != 0) {
        /* nothing */
    }

    return (s - start);                         /* length of string, not including nul terminator */
}


/****************************************************************************
****************************************************************************/

void dbgmsg (char const *format, ...)
{
    va_list vl;
    va_start (vl, format);

    while (1)
    {
        unsigned char ch = *format++;

        if (ch == 0x00)
            break;

        if (ch == '%')
        {
            int min_width = 0;

restart_format:

            ch = *format++;

            if ((ch >= '0') && (ch <= '9')) {
                min_width = (min_width * 10) + (ch - '0');
                goto restart_format;
            }

            switch (ch)
            {
                case 'd':                       /* unsigned decimal */
                {
                    out_decimal (va_arg (vl, unsigned int), min_width);
                    break;
                }

                case 's':                       /* string (with optional leading  spaces) */
                case 'S':                       /* string (with optional trailing spaces) */
                {
                    char *string = va_arg (vl, char *);
                    int padding = min_width - bl_strlen (string);

                    while ((ch == 's') && (--padding >= 0))
                        uart_tx_char (' ');
                    out_string (string);
                    while ((ch == 'S') && (--padding >= 0))
                        uart_tx_char (' ');
                    break;
                }

                case 'x':                       /* 32bit hex (8 chars) */
                {
                    if ((min_width == 0) || (min_width >= 8))
                        min_width = 8;
                    out_hex (va_arg (vl, unsigned int), min_width);
                    break;
                }

                default:
                {
                    goto normal_char;
                }
            }
        }
        else
        {
normal_char:

            if (ch == '\n')
                uart_tx_char ('\r');
            uart_tx_char (ch);
        }
    }

    va_end (vl);
}


/****************************************************************************
****************************************************************************/

EXPORT_SYMBOL(dbgmsg);

/****************************************************************************
****************************************************************************/

