#
# Makefile for SSA specific ZyDAS zd1201 driver (generic 802.11 support)
#

O_TARGET := p80211.o

obj-y    := p80211conv.o p80211req.o p80211frm.o p80211wext.o p80211wep.o p80211netdev.o p80211mod.o
obj-m    := $(O_TARGET)

ZD1201_INCLUDE_DEBUG :=
ZD1201_CONFIG_SNIFF :=

ifeq ($(CONFIG_SSA_ZD1201_DEBUG), y)
ZD1201_INCLUDE_DEBUG += -DWLAN_INCLUDE_DEBUG
endif

#ifeq ($(CONFIG_NETLINK), y)
#ZD1201_CONFIG_SNIFF += -DCONFIG_NETLINK=1
#endif
#
#ifeq ($(CONFIG_PACKET), y)
#ZD1201_CONFIG_SNIFF += -DCONFIG_PACKET=1
#endif

EXTRA_CFLAGS += -I../include $(ZD1201_INCLUDE_DEBUG) $(ZD1201_CONFIG_SNIFF)

include $(TOPDIR)/Rules.make

