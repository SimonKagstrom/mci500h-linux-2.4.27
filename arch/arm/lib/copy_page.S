/*
 *  linux/arch/arm/lib/copypage.S
 *
 *  Copyright (C) 1995-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  ASM optimised string functions
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/constants.h>

		.text
		.align	4

#if 0

/*
 * StrongARM optimised copy_page routine
 * now 1.78bytes/cycle, was 1.60 bytes/cycle (50MHz bus -> 89MB/s)
 * Note that we probably achieve closer to the 100MB/s target with
 * the core clock switching.
 */
ENTRY(copy_page)
		stmfd	sp!, {r4, lr}			@	2
		mov	r2, #PAGE_SZ/64			@	1
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
1:		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4
		subs	r2, r2, #1			@	1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmneia	r1!, {r3, r4, ip, lr}		@	4
		bne	1b				@	1
		LOADREGS(fd, sp!, {r4, pc})		@	3

#else

/*
 * Optimised 4k bytes block copy.
 * r0 = dst, r1 = src
 * Assumes src and dst are correctly aligned... (use trap handler as fallback...)
 */
ENTRY(copy_page)
		stmdb	sp!, { r4 - r11, lr }		@ stack all callee saved regs (fixme: should probably maintain 8 byte stack alignment...)
		ldr	r3, [r1], #4			@ load first word from src (post increment src ptr)
		add	r2, r0, #PAGE_SZ		@ dst_end = (dst_ptr + PAGE_SZ)
		str	r3, [r0], #4			@ store first word to dst (post increment dst ptr)
1:		ldmia	r1!, { r3 - r12, lr }		@ load 11 words from source
		stmia	r0!, { r3 - r12, lr }		@ store 11 words to destination
		cmp	r0, r2
		bcc	1b				@ keep looping while (src_ptr < end_ptr)
		ldmia	sp!, { r4 - r11, pc }		@ restore all stacked registers and return

#endif

/*
 * Optimised 2k bytes block copy.
 * r0 = dst, r1 = src
 * Fallback to memcpy if either src or dst are not 32bit aligned.
 */
		.align	4

ENTRY(copy_page_2048)
		orr	r2, r1, r0
		tst	r2, #3
		movne	r2, #2048
		bne	SYMBOL_NAME(memcpy)		@ fallback to memcpy if things start to look tricky...
		stmdb	sp!, { r4 - r11, lr }		@ stack all callee saved regs (fixme: should probably maintain 8 byte stack alignment...)
		add	r2, r1, #2048			@ end_ptr = (src_ptr + BlockSize)
		ldmia	r1!, { r3 - r8 }		@ load first 6 words from source
		stmia	r0!, { r3 - r8 }		@ store first 6 words to destination
1:		ldmia	r1!, { r3 - r12, lr }		@ load 11 words from source
		stmia	r0!, { r3 - r12, lr }		@ store 11 words to destination
		cmp	r1, r2
		bcc	1b				@ keep looping while (src_ptr < end_ptr)
		ldmia	sp!, { r4 - r11, pc }		@ restore all stacked registers and return


/*
 * Optimised 64byte block copy.
 * r0 = dst, r1 = src
 * Fallback to memcpy if either src or dst are not 32bit aligned.
 */
		.align	4

ENTRY(block_copy_64)
		orr	r2, r1, r0
		tst	r2, #3
		movne	r2, #64
		bne	SYMBOL_NAME(memcpy)		@ fallback to memcpy if things start to look tricky...
		str	lr, [sp, #-4]!			@ stack all callee saved regs (fixme: should probably maintain 8 byte stack alignment...)
		ldmia	r1!, { r2 - r3, r12, lr }	@
		stmia	r0!, { r2 - r3, r12, lr }	@
		ldmia	r1!, { r2 - r3, r12, lr }	@
		stmia	r0!, { r2 - r3, r12, lr }	@
		ldmia	r1!, { r2 - r3, r12, lr }	@
		stmia	r0!, { r2 - r3, r12, lr }	@
		ldmia	r1!, { r2 - r3, r12, lr }	@
		stmia	r0!, { r2 - r3, r12, lr }	@
		ldr	pc, [sp], #4			@ restore all stacked registers and return

