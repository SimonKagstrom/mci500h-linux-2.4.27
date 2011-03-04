#
# Makefile for the linux kernel.
#

O_TARGET := nopci.o

export-objs := pcipool.o

obj-y   :=
obj-m   :=
obj-n   :=
obj-	:=

obj-$(CONFIG_USB_OHCI_SA1111)   += pcipool.o
obj-$(CONFIG_USB_OHCI_AT91)	+= pcipool.o
obj-$(CONFIG_USB_EHCI_PNX0106)	+= pcipool.o

include $(TOPDIR)/Rules.make
