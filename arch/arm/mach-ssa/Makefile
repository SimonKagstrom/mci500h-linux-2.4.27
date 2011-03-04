#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).

USE_STANDARD_AS_RULE	:= true

O_TARGET		:= ssa.o

# Object files which are always linked
obj-y			:= arch.o mm.o cpu.o tsc.o
obj-m			:=
obj-n			:=
obj-			:=

# Object files which are conditionally linked
obj-$(CONFIG_SSA_SIIG) += irq.o gpio.o
obj-$(CONFIG_SSA_WESSLI) += irq.o gpio.o
obj-$(CONFIG_SSA_W3) += irq.o gpio.o
obj-$(CONFIG_SSA_TAHOE) += irq.o gpio.o
obj-$(CONFIG_SSA_HAS7752) += has7752_irq.o has7752_gpio.o has7752_pccard.o has7752_epldprof.o txtimeout_hack.o

obj-$(CONFIG_SSA_W3_PCCARD) += w3_pccard.o
obj-$(CONFIG_SSA_W3_GPI) += w3_gpi.o
obj-$(CONFIG_SSA_W3_SPI) += w3_spi.o
obj-$(CONFIG_SSA_TAHOE) += w3_spi.o

obj-$(CONFIG_SSA_HAS7750_CUSTOM_IDE) += has7750_custom_ide.o
obj-$(CONFIG_SSA_HAS7752_CUSTOM_IDE) += has7752_custom_ide.o

obj-$(CONFIG_SSA_HAS7752_USBDEV) += has7752_usbdev.o
obj-$(CONFIG_SSA_HAS7752_DPRAM) += has7752_dpram.o

obj-$(CONFIG_LEDS) += leds.o

ide-y := 
ide-$(CONFIG_SSA_SIIG) := siig_ide.o
ide-$(CONFIG_SSA_HAS7752) := has7752_kernel_ide.o
obj-$(CONFIG_IDE) += $(ide-y)

# Simple pccard api test.
# Should be linked after has7752_pccard.o or compiled as a module
obj-$(CONFIG_SSA_HAS7752_PCCARD_TEST) += has7752_pccard_test.o

# Object files which are allowed to export symbols
export-objs :=	cpu.o tsc.o irq.o gpio.o leds.o \
		w3_pccard.o w3_gpi.o w3_spi.o \
		has7752_irq.o has7752_gpio.o has7752_pccard.o has7752_epldprof.o txtimeout_hack.o

include $(TOPDIR)/Rules.make
