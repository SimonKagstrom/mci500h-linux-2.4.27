
O_TARGET := idedriver-arm.o

obj-y		:=
obj-m		:=

obj-$(CONFIG_BLK_DEV_IDE_ICSIDE)	+= icside.o
obj-$(CONFIG_BLK_DEV_IDE_RAPIDE)	+= rapide.o
obj-$(CONFIG_BLK_DEV_IDE_RISCSTATION)	+= rstation-ide.o

EXTRA_CFLAGS	:= -I../

include $(TOPDIR)/Rules.make
