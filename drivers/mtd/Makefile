#
# Out of tree drivers/mtd/Makefile
#
# $Id: Makefile,v 1.74 2003/05/21 15:00:00 dwmw2 Exp $

# We know, but not quite yet...
CFLAGS_nftlcore.o := -Wno-error
CFLAGS_nftlmount.o := -Wno-error

mtd=$(shell pwd)
export mtd
obj=$(mtd)
export obj
# Munge the kernel build environment to suit our needs....
include $(shell pwd)/Makefile.inc
