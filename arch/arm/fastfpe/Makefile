#
# linux/arch/arm/fastfpe/Makefile
#
# Copyright (C) Peter Teichmann
#

O_TARGET := fast-math-emu.o

AFLAGS_CPDO.o := -march=armv3m

obj-y	:=
obj-m	:=
obj-n	:=
obj-	:=

fastfpe-objs := module.o round.o CPDT.o CPRT.o CPDO.o entry.o

list-multi := fastfpe.o

obj-$(CONFIG_FPE_FASTFPE) += fastfpe.o

USE_STANDARD_AS_RULE := true

include $(TOPDIR)/Rules.make

fastfpe.o: $(fastfpe-objs)
	$(LD) -r -o $@ $(fastfpe-objs)
