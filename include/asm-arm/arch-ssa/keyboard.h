/*
 * linux/include/asm-arm/arch-clps711x/keyboard.h
 *
 * Copyright (C) 1998-2001 Russell King
 */
#include <asm/mach-types.h>

#define NR_SCANCODES 128

#define kbd_disable_irq()	do { } while (0)
#define kbd_enable_irq()	do { } while (0)

#define kbd_init_hw()	do { } while (0)
