/* linux/drivers/net/8390-bast.h
 *
 * Copyright (c) 2003  Simtec Electronics, <linux@simtec.co.uk>
 *   Ben Dooks <ben@simtec.co.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Changelog:
 *  27-May-2003 BJD  Created file
 *  20-Aug-2003 BJD  Tidied header files
*/


/* re-define some of the routines for the 8390 core so that
 * we can build it for bast without clashing with a standard ISA
 * implementation.
*/

#define ei_open asix_ei_open
#define ei_close asix_ei_close
#define ei_tx_timeout asix_ei_tx_timeout
#define ei_interrupt asix_ei_interrupt
#define ethdev_init asix_ethdev_init
#define NS8390_init asix_NS8390_init
