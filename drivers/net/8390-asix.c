/* linux/drivers/net/bast-8390.c
 *
 * (c) 2003 Simtec Electronics, <linux@simtec.co.uk>
 *   Ben Dooks <ben@simtec.co.uk>
 *
 * Build 8390 core for BAST ASIX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Changelog:
 *  27-May-2003 BJD  Created file
 *  20-Aug-2003 BJD  Tidied header file
*/

/* we need to build an version of the 8390 core suitably configured
 * for the ASIX on the BAST (EB110ITX) board. This has a slightly different
 * bus configuration, and we improve the access functions as we know
 * where it is.
*/

#define COMPILE_ASIX
#include "8390-asix.h"
#include "8390.c"
