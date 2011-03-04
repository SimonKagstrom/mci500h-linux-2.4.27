/*
 * pnx0106_wave.S - Digital wave form capturer for Philips PNX0106
 *
 * Copyright (C) 2005-2006 PDCC, Philips CE.
 *
 * Revision History :
 * ----------------
 *
 *   Version			DATE		NAME
 *   Initial version		06-Dec-2005	James
 *   Modify			26-June-2007	Relena      
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/arch/hardware.h>
#include <asm/arch/w6_buttons.h>

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define VA_ER_INT_CLEAR(slice)                  (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C20 + (4 * (slice)))
#define VA_ER_APR(slice)                        (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0CC0 + (4 * (slice)))     /* Activation Polarity Register... */

#define ER_EVENT_SLICE(eid)                     ((eid) / ER_EVENTS_PER_SLICE)
#define ER_EVENT_MASK(eid)                      (1 << ((eid) % ER_EVENTS_PER_SLICE))
#define ER_EVENTS_PER_SLICE                     (32)

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*
 * Register usages:
 *
 *
 *  R8_fiq,R9_fiq = (O), the captured wave form,  max 64 bits, the last bit
 *                  captured is stored in lowest bit of R8_fiq, the first bit
 *                  stored in high bit of R9_fiq.
 *
 *  R10_fiq = (scratch), holding the pointer to registers
 *
 *  R11_fiq = (scratch), holding the values for destination registers
 *
 *  R12_fiq = (global variable), holding the pointer to timing table address,
 *            must be cleared to zero initially
 *
 *
 */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
		.text
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


		.global	SYMBOL_NAME(p_wave_capturer_end)
		.global	SYMBOL_NAME(a_wave_timing)
		.global	SYMBOL_NAME(a_pc_table)
		.global	SYMBOL_NAME(wIndex)

ENTRY(wave_capturer)


@ Entering the FIQ
@read timer1 value
		ldr r10, c_timer1_value
		ldr r11,[r10]	@read the timer1 value register

@save timer1 value to timing table		
		ldr r10, c_wave_timing
		add r10,r10,r12	@r12--an index into the timing table
		str r11,[r10]           @save it to the table

@save pc value to pc table, debug only, pc table has the same size of timing table for simplicity
		mov r11,lr
		ldr r10, c_pc_tbl
		add r10,r10,r12 @r12--an index into the timing table
		str r11,[r10]
		
@save r12		
		add r12,r12,#4
		and r12,r12,#255	@make sure r12<64
		ldr r10,c_wrIndex
		str r12,[r10]

@toggle triggle edge
		ldr r10,c_rc6_apr
		ldr r11, [r10]	@load APR
		ands r8,r11, #ER_EVENT_MASK(ERID_RC6RX)	@and r11 with #RC6_APR_MASK, set the flag at the same time
		orreq r11,r11,#ER_EVENT_MASK(ERID_RC6RX)	@if not set, then set it
		andne r11,r11,#~ER_EVENT_MASK(ERID_RC6RX)	@else clear it
		str r11,[r10]		@save APR

@ Clear the GPIO interrupt request
                @ writel (ER_EVENT_MASK(event_id), VA_ER_INT_CLEAR(ER_EVENT_SLICE(event_id)));
                ldr     r10, c_wave_gpio_clear          @ address for the GPIO int clear register
                mov     r11, #ER_EVENT_MASK(ERID_RC6RX)    @ bit mask
                str     r11, [r10]                      @ clear the GPIO int
		
@ Return from FIQ handler
		subs	pc, lr, #4                      @ return from FIQ

/*
 * The constant table.
 */
c_timer1_value:
	.word (IO_ADDRESS_TIMER1_BASE+4)

c_rc6_apr:
	.word VA_ER_APR(ER_EVENT_SLICE(ERID_RC6RX))

c_wave_gpio_clear:
	.word VA_ER_INT_CLEAR(ER_EVENT_SLICE(ERID_RC6RX))
	
c_wrIndex:
	.word wIndex
		
/*
 * This table saves the sample result of the timer1 value register!!, total 64 words
 */
c_wave_timing:
	.word a_wave_timing

c_pc_tbl:
	.word a_pc_table
/*
 * The end address of this piece of code, to be used by 'set_fiq_handler()'
 */
SYMBOL_NAME(p_wave_capturer_end):

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
