/*
 *  linux/include/asm-arm/byteorder.h
 *
 * ARM Endian-ness.  In little endian mode, the data bus is connected such
 * that byte accesses appear as:
 *  0 = d0...d7, 1 = d8...d15, 2 = d16...d23, 3 = d24...d31
 * and word accesses (data or instruction) appear as:
 *  d0...d31
 *
 * When in big endian mode, byte accesses appear as:
 *  0 = d24...d31, 1 = d16...d23, 2 = d8...d15, 3 = d0...d7
 * and word accesses (data or instruction) appear as:
 *  d0...d31
 */
#ifndef __ASM_ARM_BYTEORDER_H
#define __ASM_ARM_BYTEORDER_H


#include <asm/types.h>

/*
   Although the ARM can do a 32bit endian swap in 4 instructions, gcc
   doesn't _quite_ get it yet... instead we get 5 as the initial exor
   and rotate is split into 2 steps.
*/
static __inline__ __const__ __u32 ___arch__swab32(__u32 x)
{
    unsigned int t;
        
    t = x ^ ((x << 16) | (x >> 16));    /* eor  r1, r0, r0, ror #16 */
    t &= ~0x00ff0000;                   /* bic  r1, r1, #0x00FF0000 */
    x = (x << 24) | (x >> 8);           /* mov  r0, r0, ror #8      */
    x ^= (t >> 8);                      /* eor  r0, r0, r1, lsr #8  */

    return x;
}

/*
   For 16bit values, the ARM can endian reverse in 3 instructions _if_ the
   the upper 16 bits of the input register can always be assumed to be zero.
   If not, then 4 instructions are needed (which gcc correctly generates).
*/
static __inline__ __const__ __u16 ___arch__swab16(__u16 x)
{
    unsigned int t;

    t = (unsigned int) x << 8;
    t &= ~0x00ff0000;
    x = t | (unsigned int) x >> 8;
    
    return x;
}

#define __arch__swab32(x) ___arch__swab32(x)
#define __arch__swab16(x) ___arch__swab16(x)

#if !defined(__STRICT_ANSI__) || defined(__KERNEL__)
#  define __BYTEORDER_HAS_U64__
#  define __SWAB_64_THRU_32__
#endif

#ifdef __ARMEB__
#include <linux/byteorder/big_endian.h>
#else
#include <linux/byteorder/little_endian.h>
#endif

#endif

