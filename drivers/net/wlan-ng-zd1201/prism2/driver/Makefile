#
# Makefile for SSA specific ZyDAS zd1201 driver (mmio zd1201 specific code)
#

obj-$(CONFIG_SSA_ZD1201) := zd1201_mmio.o

ZD1201_INCLUDE_DEBUG :=

ifeq ($(CONFIG_SSA_ZD1201_DEBUG), y)
ZD1201_INCLUDE_DEBUG += -DWLAN_INCLUDE_DEBUG
endif

EXTRA_CFLAGS += -I../include -I../../include $(ZD1201_INCLUDE_DEBUG)

include $(TOPDIR)/Rules.make

