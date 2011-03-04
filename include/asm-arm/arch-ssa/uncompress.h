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

#include <asm/hardware.h>


#define SSA_UARTDR     (*(volatile unsigned int *) (SSA_UART0_BASE + 0x00))     /* Data Register (Physical Address) */
#define SSA_UARTLSR    (*(volatile unsigned int *) (SSA_UART0_BASE + 0x14))     /* Line Status Register (Physical Address) */

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


//      if (*(s + 1) != 0)
//      {
//         for (i = 0; i < 1000; i++) { /* delay */ }
//      }


      if (*s == '\n')
      {
         while ((SSA_UARTLSR & (1 << 6)) == 0)
            barrier();

         SSA_UARTDR = '\r';

         /*
            This is a nasty hack which seems to work.
            Better still would be to find out why polling the Tx Fifo and
            Tx Shift Register empty bit doesn't.
         */

         for (i = 0; i < 1000; i++) { /* delay */ }
      }

      s++;
   }
   
   while ((SSA_UARTLSR & (1 << 6)) == 0) /* wait */;
}

#define arch_decomp_setup()     { /* does nothing */ }
#define arch_decomp_wdog()      { /* does nothing */ }

#endif

