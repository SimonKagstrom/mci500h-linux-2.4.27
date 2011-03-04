
/*
   Early include of wlan/version.h is needed to pickup
   definition of WLAN_MMIO
*/

#include <wlan/version.h>

#define ZD1201_CHIPSET

#define WLAN_HOSTIF WLAN_MMIO

#if (WLAN_HOSTIF == WLAN_PCMCIA)
#error "WLAN_PCMCIA interface not supported"
#endif

#if (WLAN_HOSTIF == WLAN_ISA)
#error "WLAN_ISA interface not supported"
#endif

#if (WLAN_HOSTIF == WLAN_PCI)
#error "WLAN_PCI interface not supported"
#endif

#if (WLAN_HOSTIF == WLAN_USB)
#error "WLAN_USB interface not supported"
#endif

#if (WLAN_HOSTIF == WLAN_PLX)
#error "WLAN_PLX interface not supported"
#endif

#if (WLAN_HOSTIF == WLAN_SLAVE)
#error "WLAN_SLAVE interface not supported"
#endif


#include "hfa384x.c"
#include "prism2mgmt.c"
#include "prism2mib.c"
#include "prism2sta.c"


#if (WLAN_CPU_FAMILY != WLAN_ARM)
#error "Huh ? arm architecture was not detected correctly..."
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,21) )
#if (WLAN_CPU_FAMILY == WLAN_Ix86)
#ifndef CONFIG_ISA
#warning "You may need to enable ISA support in your kernel."
#endif
#endif
#endif
