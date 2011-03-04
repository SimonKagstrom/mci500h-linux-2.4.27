/*
 *  linux/include/asm-arm/arch-ssa/uncompress.h
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_UNCOMPRESS_H_
#define __ASM_ARCH_UNCOMPRESS_H_

#include <linux/config.h>
#include <asm/hardware.h>

#if defined (CONFIG_PNX0106_HASLI7) || \
	defined (CONFIG_PNX0106_MCIH)

static void puts (const char *s)
{
}

#else

#define SSA_UARTDR     (*(volatile unsigned int *) (UART0_REGS_BASE + 0x00))    /* Data Register (Physical Address) */
#define SSA_UARTLSR    (*(volatile unsigned int *) (UART0_REGS_BASE + 0x14))    /* Line Status Register (Physical Address) */

/*
   This version of puts() does _not_ append a newline
*/
static void puts (const char *s)
{
   int i;

   while (*s)
   {
      while ((SSA_UARTLSR & (1 << 6)) == 0)
         barrier();

      SSA_UARTDR = *s;

      if (*s == '\n')
      {
         while ((SSA_UARTLSR & (1 << 6)) == 0)
            barrier();

         SSA_UARTDR = '\r';

         /*
            Dodgy hack inherited from SAA7752 linux port.
            We should be able to poll SSA_UARTLSR bit6 here as above
            but somehow, at some stage in the past, that didn't work ?!?
            Increase for loop count from 1000 to 2048 to account for
            speed difference between SAA7752 and PNX0106.
            
            FIXME !!
         */

         for (i = 0; i < 2048; i++) { /* delay */ }
      }

      s++;
   }
   
   while ((SSA_UARTLSR & (1 << 6)) == 0) /* wait */;
}

#endif

#define arch_decomp_setup()     do { /* nothing */ } while (0)
#define arch_decomp_wdog()      do { /* nothing */ } while (0)

#endif

