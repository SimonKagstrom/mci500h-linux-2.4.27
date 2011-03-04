/*
 *  linux/drivers/pcmcia/ssa_pcmcia.c
 *
 *  Copyright (C) 2002 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/tqueue.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <asm/io.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/ss.h>
#include <pcmcia/bus_ops.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>

#if 1
#define PCMCIA_DEBUG 10         /* PCMCIA_DEBUG must be defined _before_ including cs_internal.h */
#endif

#include "cs_internal.h"

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#define SSA_PCMCIA_POLL_PERIOD      (5*HZ)
#define SSA_PCMCIA_SOCKET_COUNT     1

#ifdef PCMCIA_DEBUG
static int pc_debug = PCMCIA_DEBUG;
MODULE_PARM(pc_debug, "i");
#endif

/*
   Waitstate values for external (ebi) static memory controller.
   Note that these values are _not_ tunable !!
   (They are carefully calculated in conjuntion with the CPLD code).
   They are correct only for 48 (or 50) MHz HCLK.
*/

#define SMC_WAITSTATES_PCMCIA_IO   ((0x01 << 28) |   /* width == 16bit */          \
                                    (  11 << 11) |   /* write waitstates */        \
                                    (  11 <<  5) |   /* read  waitstates */        \
                                    (   4 <<  0))    /* turnaround waitstates */

#define SMC_WAITSTATES_PCMCIA_MEM  ((0x01 << 28) |   /* width == 16bit */          \
                                    (  30 << 11) |   /* write waitstates */        \
                                    (  30 <<  5) |   /* read  waitstates */        \
                                    (   4 <<  0))    /* turnaround waitstates */

/*
   For now we assume all PCMCIA accesses (IO, MEM and ATTR) will be within a 1MB window.
   fixme: this may need tweaking (with coresponding changes in hardware.h and mm.c etc).
*/

#define MAP_SIZE        (1024 * 1024)

#define MAPPING_IO      (MAP_SIZE * 1)
#define MAPPING_MEM     (MAP_SIZE * 2)
#define MAPPING_ATTR    (MAP_SIZE * 3)
#define MAPPING_MASK    (MAP_SIZE * 3)


struct pcmcia_state
{
   unsigned detect: 1,
            ready:  1,
            bvd1:   1,
            bvd2:   1,
            wrprot: 1,
            vs_3v:  1,
            vs_Xv:  1;
};

struct pcmcia_configure
{
   unsigned sock:    8,
            vcc:     8,
            vpp:     8,
            output:  1,
            speaker: 1,
            reset:   1,
            irq:     1;
};


/*
   This structure maintains 'per-socket' housekeeping state (e.g. last known
   state of card detect pins and callback handler associated with the socket).
*/
typedef struct
{
   socket_state_t       cs_state;
   pccard_io_map        io_map[MAX_IO_WIN];
   pccard_mem_map       mem_map[MAX_WIN];
   void                 (*handler) (void *, unsigned int);
   void                 *handler_info;
   struct pcmcia_state  k_state;
}
ssa_pcmcia_socket_t;

static ssa_pcmcia_socket_t ssa_pcmcia_socket;

static struct timer_list poll_timer;
static struct tq_struct ssa_pcmcia_task;
static struct bus_operations ssa_bus_operations;


/*
   'lowlevel' socket routines...
*/

static void lowlevel_socket_state (struct pcmcia_state *state)
{
   unsigned int levels = readl (VA_GPIO_VALUE);

   memset (state, 0, sizeof(*state));

   state->detect = 1;                                       /* static value of 1 (ie a card is always present) */
   state->ready  = (levels & GPIO_PCMCIA_INTRQ) ? 1 : 0;    /* high value on pin means card is ready */
   state->bvd1   = 1;
   state->bvd2   = 1;
   state->wrprot = 0;
   state->vs_3v  = 1;
   state->vs_Xv  = 0;
}


/*
   Configure the socket to match the requested settings.
*/
static int lowlevel_configure_socket (const struct pcmcia_configure *configure)
{
   unsigned int output_value;
   unsigned long flags;

   if (configure->sock != 0)
      return -1;

   local_irq_save (flags);                      /* lockout anyone else while we read-modify-write gpio registers */
   output_value = readl (VA_GPIO_OUTPUT);

   switch (configure->vcc)
   {
      case 0:                                   /* fixme: what does this mean ?? remove power ?? */
//       printk ("ssa_cs: setting vcc to 0\n");
         break;
      case 33:
//       printk ("ssa_cs: setting vcc to 3v3\n");
         break;
      case 50:
//       printk ("ssa_cs: setting vcc to 5v0\n");
         break;

      default:
         printk ("%s(): unrecognized Vcc %u\n", __FUNCTION__, configure->vcc);
   }

#if 0
   if (configure->reset)
      output_value |=  (GPIO_PCMCIA_RESET);     /* drive GPIO pin high to assert PCMCIA RESET */
   else
      output_value &= ~(GPIO_PCMCIA_RESET);     /* drive GPIO pin low to de-assert PCMCIA RESET */
#else
   if (configure->reset)
      printk ("%s(): ignoring request to assert PCMCIA RESET\n", __FUNCTION__);
   else
      printk ("%s(): ignoring request to de-assert PCMCIA RESET\n", __FUNCTION__);
#endif

   writel (output_value, VA_GPIO_OUTPUT);
   local_irq_restore (flags);

   return 0;
}


/*
   Enable ints for transitions on card detect and voltage detect pins.
*/
static int lowlevel_socket_init (int sock)
{
   return 0;
}

/*
   Disable ints for transitions on card detect and voltage detect pins.
*/
static int lowlevel_socket_suspend (int sock)
{
   return 0;
}


/*
 * ssa_pcmcia_state_to_config
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * Convert PCMCIA socket state to our socket configure structure.
 */
static struct pcmcia_configure ssa_pcmcia_state_to_config (unsigned int sock, socket_state_t *state)
{
   struct pcmcia_configure conf;

   conf.sock    = sock;
   conf.vcc     = state->Vcc;
   conf.vpp     = state->Vpp;
   conf.output  = state->flags & SS_OUTPUT_ENA ? 1 : 0;
   conf.speaker = state->flags & SS_SPKR_ENA ? 1 : 0;
   conf.reset   = state->flags & SS_RESET ? 1 : 0;
   conf.irq     = state->io_irq != 0;

   return conf;
}


/* ssa_pcmcia_init()
 * ^^^^^^^^^^^^^^^^^
 *
 * (Re-)Initialise the socket, turning on status interrupts
 * and PCMCIA bus.  This must wait for power to stabilise
 * so that the card status signals report correctly.
 *
 * Returns: 0
 */
static int ssa_pcmcia_init (unsigned int sock)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   struct pcmcia_configure conf;

   DEBUG(2, "%s(): initialising socket %u\n", __FUNCTION__, sock);

   skt->cs_state = dead_socket;
   conf = ssa_pcmcia_state_to_config (sock, &dead_socket);
   lowlevel_configure_socket (&conf);
   return lowlevel_socket_init (sock);
}


/*
 * sa1100_pcmcia_suspend()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 *
 * Remove power on the socket, disable IRQs from the card.
 * Turn off status interrupts, and disable the PCMCIA bus.
 *
 * Returns: 0
 */
static int ssa_pcmcia_suspend (unsigned int sock)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   struct pcmcia_configure conf;
   int ret;

   DEBUG(2, "%s(): suspending socket %u\n", __FUNCTION__, sock);

   conf = ssa_pcmcia_state_to_config (sock, &dead_socket);
   ret = lowlevel_configure_socket (&conf);
   if (ret == 0) {
      skt->cs_state = dead_socket;
      ret = lowlevel_socket_suspend (sock);
   }
   return ret;
}


/* ssa_pcmcia_events()
 * ^^^^^^^^^^^^^^^^^^^
 * Helper routine to generate a Card Services event mask based on
 * state information obtained from the kernel low-level PCMCIA layer
 * in a recent (and previous) sampling. Updates `prev_state'.
 *
 * Returns: an event mask for the given socket state.
 */
static inline unsigned int
ssa_pcmcia_events(struct pcmcia_state *state,
                  struct pcmcia_state *prev_state,
                  unsigned int mask, unsigned int flags)
{
  unsigned int events = 0;

  if (state->detect != prev_state->detect) {
    DEBUG(3, "%s(): card detect value %u\n", __FUNCTION__, state->detect);
    events |= SS_DETECT;
  }

  if (state->ready != prev_state->ready) {
    DEBUG(3, "%s(): card ready value %u\n", __FUNCTION__, state->ready);
    events |= flags & SS_IOCARD ? 0 : SS_READY;
  }

  if (state->bvd1 != prev_state->bvd1) {
    DEBUG(3, "%s(): card BVD1 value %u\n", __FUNCTION__, state->bvd1);
    events |= flags & SS_IOCARD ? SS_STSCHG : SS_BATDEAD;
  }

  if (state->bvd2 != prev_state->bvd2) {
    DEBUG(3, "%s(): card BVD2 value %u\n", __FUNCTION__, state->bvd2);
    events |= flags & SS_IOCARD ? 0 : SS_BATWARN;
  }

  *prev_state = *state;

  events &= mask;

  if (events != 0)
  {
    DEBUG(2, "events: %s%s%s%s%s%s\n",
          events == 0         ? "<NONE>"   : "",
          events & SS_DETECT  ? "DETECT "  : "",
          events & SS_READY   ? "READY "   : "",
          events & SS_BATDEAD ? "BATDEAD " : "",
          events & SS_BATWARN ? "BATWARN " : "",
          events & SS_STSCHG  ? "STSCHG "  : "");
  }
  
  return events;
}


/* ssa_pcmcia_task_handler()
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 * Processes serviceable socket events using the "eventd" thread context.
 *
 * Event processing (specifically, the invocation of the Card Services event
 * callback) occurs in this thread rather than in the actual interrupt
 * handler due to the use of scheduling operations in the PCMCIA core.
 */
static void ssa_pcmcia_task_handler (void *data)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   struct pcmcia_state state;
   unsigned int events;

   do {
//    DEBUG(4, "%s(): interrogating low-level PCMCIA service\n", __FUNCTION__);

      lowlevel_socket_state (&state);
      events = ssa_pcmcia_events (&state, &skt->k_state, skt->cs_state.csc_mask, skt->cs_state.flags);

      if (events && skt->handler != NULL)
         skt->handler (skt->handler_info, events);
   }
   while (events);
}

static struct tq_struct ssa_pcmcia_task =
{
   routine: ssa_pcmcia_task_handler
};


/* ssa_pcmcia_poll_event()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Poll for events in addition to IRQs since IRQ only is unreliable...
 */
static void ssa_pcmcia_poll_event (unsigned long dummy)
{
// DEBUG(4, "%s(): polling for events\n", __FUNCTION__);
   poll_timer.function = ssa_pcmcia_poll_event;
   poll_timer.expires = jiffies + SSA_PCMCIA_POLL_PERIOD;
   add_timer (&poll_timer);
   schedule_task (&ssa_pcmcia_task);
}


/* ssa_pcmcia_interrupt()
 * ^^^^^^^^^^^^^^^^^^^^^^
 * Service routine for socket driver interrupts (requested by the
 * low-level PCMCIA init() operation via sa1100_pcmcia_thread()).
 * The actual interrupt-servicing work is performed by
 * sa1100_pcmcia_thread(), largely because the Card Services event-
 * handling code performs scheduling operations which cannot be
 * executed from within an interrupt context.
 */
#if 0
static void ssa_pcmcia_interrupt (int irq, void *dev, struct pt_regs *regs)
{
   DEBUG(3, "%s(): servicing IRQ %d\n", __FUNCTION__, irq);
   schedule_task (&ssa_pcmcia_task);
}
#endif

/* ssa_pcmcia_register_callback()
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the register_callback() operation for the in-kernel
 * PCMCIA service (formerly SS_RegisterCallback in Card Services). If 
 * the function pointer `handler' is not NULL, remember the callback 
 * location in the state for `sock', and increment the usage counter 
 * for the driver module. (The callback is invoked from the interrupt
 * service routine, sa1100_pcmcia_interrupt(), to notify Card Services
 * of interesting events.) Otherwise, clear the callback pointer in the
 * socket state and decrement the module usage count.
 *
 * Returns: 0
 */
static int ssa_pcmcia_register_callback (unsigned int sock,
                                         void (*handler)(void *, unsigned int),
                                         void *info)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

   if (handler == NULL) {
      skt->handler = NULL;
      MOD_DEC_USE_COUNT;
   }
   else {
      MOD_INC_USE_COUNT;
      skt->handler_info = info;
      skt->handler = handler;
   }

   return 0;
}


/* ssa_pcmcia_inquire_socket()
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the inquire_socket() operation for the in-kernel PCMCIA
 * service (formerly SS_InquireSocket in Card Services). Of note is
 * the setting of the SS_CAP_PAGE_REGS bit in the `features' field of
 * `cap' to "trick" Card Services into tolerating large "I/O memory" 
 * addresses. Also set is SS_CAP_STATIC_MAP, which disables the memory
 * resource database check. (Mapped memory is set up within the socket
 * driver itself.)
 *
 * In conjunction with the STATIC_MAP capability is a new field,
 * `io_offset', recommended by David Hinds. Rather than go through
 * the SetIOMap interface (which is not quite suited for communicating
 * window locations up from the socket driver), we just pass up
 * an offset which is applied to client-requested base I/O addresses
 * in alloc_io_space().
 *
 * SS_CAP_PAGE_REGS: used by setup_cis_mem() in cistpl.c to set the
 *   force_low argument to validate_mem() in rsrc_mgr.c -- since in
 *   general, the mapped * addresses of the PCMCIA memory regions
 *   will not be within 0xffff, setting force_low would be
 *   undesirable.
 *
 * SS_CAP_STATIC_MAP: don't bother with the (user-configured) memory
 *   resource database; we instead pass up physical address ranges
 *   and allow other parts of Card Services to deal with remapping.
 *
 * SS_CAP_PCCARD: we can deal with 16-bit PCMCIA & CF cards, but
 *   not 32-bit CardBus devices.
 *
 * Return value is irrelevant; the pcmcia subsystem ignores it.
 */
static int ssa_pcmcia_inquire_socket (unsigned int sock, socket_cap_t *cap)
{
// ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   if (sock != 0)
      return -1;

   cap->features  = SS_CAP_PAGE_REGS | SS_CAP_VIRTUAL_BUS | SS_CAP_STATIC_MAP | SS_CAP_PCCARD;
   cap->irq_mask  = 0;
   cap->map_size  = MAP_SIZE;
   cap->io_offset = IO_ADDRESS_PCMCIA_BASE;     /* beware: higher levels access this space directly even if SS_CAP_VIRTUAL_BUS is set !! */
   cap->pci_irq   = INT_PCMCIA;
   cap->cb_dev    = NULL;
   cap->bus       = &ssa_bus_operations;

   return 0;
}


/* ssa_pcmcia_get_status()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the get_status() operation for the in-kernel PCMCIA
 * service (formerly SS_GetStatus in Card Services). Essentially just
 * fills in bits in `status' according to internal driver state or
 * the value of the voltage detect chipselect register.
 *
 * As a debugging note, during card startup, the PCMCIA core issues
 * three set_socket() commands in a row the first with RESET deasserted,
 * the second with RESET asserted, and the last with RESET deasserted
 * again. Following the third set_socket(), a get_status() command will
 * be issued. The kernel is looking for the SS_READY flag (see
 * setup_socket(), reset_socket(), and unreset_socket() in cs.c).
 *
 * Returns: 0
 */
static int ssa_pcmcia_get_status (unsigned int sock, unsigned int *status)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   struct pcmcia_state state;
   unsigned int stat;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   lowlevel_socket_state (&state);

   skt->k_state = state;

   stat  = state.detect ? SS_DETECT : 0;
   stat |= state.ready  ? SS_READY  : 0;
   stat |= state.vs_3v  ? SS_3VCARD : 0;
   stat |= state.vs_Xv  ? SS_XVCARD : 0;

   /* The power status of individual sockets is not available
    * explicitly from the hardware, so we just remember the state
    * and regurgitate it upon request:
    */
   stat |= skt->cs_state.Vcc ? SS_POWERON : 0;

   if (skt->cs_state.flags & SS_IOCARD)
      stat |= state.bvd1 ? SS_STSCHG : 0;
   else {
      if (state.bvd1 == 0)
         stat |= SS_BATDEAD;
      else if (state.bvd2 == 0)
         stat |= SS_BATWARN;
   }

   DEBUG(3, "\tstatus: %s%s%s%s%s%s%s%s\n",
        stat & SS_DETECT  ? "DETECT "  : "",
        stat & SS_READY   ? "READY "   : "", 
        stat & SS_BATDEAD ? "BATDEAD " : "",
        stat & SS_BATWARN ? "BATWARN " : "",
        stat & SS_POWERON ? "POWERON " : "",
        stat & SS_STSCHG  ? "STSCHG "  : "",
        stat & SS_3VCARD  ? "3VCARD "  : "",
        stat & SS_XVCARD  ? "XVCARD "  : "");

   *status = stat;

   return 0;
}


/* ssa_pcmcia_get_socket()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the get_socket() operation for the in-kernel PCMCIA
 * service (formerly SS_GetSocket in Card Services). Not a very 
 * exciting routine.
 *
 * Returns: 0
 */
static int ssa_pcmcia_get_socket (unsigned int sock, socket_state_t *state)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   *state = skt->cs_state;
   return 0;
}

/* ssa_pcmcia_set_socket()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the set_socket() operation for the in-kernel PCMCIA
 * service (formerly SS_SetSocket in Card Services). We more or
 * less punt all of this work and let the kernel handle the details
 * of power configuration, reset, &c. We also record the value of
 * `state' in order to regurgitate it to the PCMCIA core later.
 *
 * Returns: 0
 */
static int ssa_pcmcia_set_socket (unsigned int sock, socket_state_t *state)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   struct pcmcia_configure conf;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   DEBUG(3, "\tmask:  %s%s%s%s%s%s\n\tflags: %s%s%s%s%s%s\n",
        (state->csc_mask == 0)         ? "<NONE>"      : "",
        (state->csc_mask & SS_DETECT)  ? "DETECT "     : "",
        (state->csc_mask & SS_READY)   ? "READY "      : "",
        (state->csc_mask & SS_BATDEAD) ? "BATDEAD "    : "",
        (state->csc_mask & SS_BATWARN) ? "BATWARN "    : "",
        (state->csc_mask & SS_STSCHG)  ? "STSCHG "     : "",
        (state->flags == 0)            ? "<NONE>"      : "",
        (state->flags & SS_PWR_AUTO)   ? "PWR_AUTO "   : "",
        (state->flags & SS_IOCARD)     ? "IOCARD "     : "",
        (state->flags & SS_RESET)      ? "RESET "      : "",
        (state->flags & SS_SPKR_ENA)   ? "SPKR_ENA "   : "",
        (state->flags & SS_OUTPUT_ENA) ? "OUTPUT_ENA " : "");

   DEBUG(3, "\tVcc %d  Vpp %d  irq %d\n",
        state->Vcc, state->Vpp, state->io_irq);

   conf = ssa_pcmcia_state_to_config (sock, state);

   if (lowlevel_configure_socket (&conf) < 0) {
      printk ("ssa_pcmcia: unable to configure socket %d\n", sock);
      return -1;
   }

   skt->cs_state = *state;
   return 0;
}


/* ssa_pcmcia_get_io_map()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the get_io_map() operation for the in-kernel PCMCIA
 * service (formerly SS_GetIOMap in Card Services). Just returns an
 * I/O map descriptor which was assigned earlier by a set_io_map().
 *
 * Returns: 0 on success, -1 if the map index was out of range
 */
static int ssa_pcmcia_get_io_map (unsigned int sock, struct pccard_io_map *map)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   if (map->map >= MAX_IO_WIN)
      return -1;
      
   *map = skt->io_map[map->map];
   return 0;
}


/* ssa_pcmcia_set_io_map()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the set_io_map() operation for the in-kernel PCMCIA
 * service (formerly SS_SetIOMap in Card Services). We configure
 * the map speed as requested, but override the address ranges
 * supplied by Card Services.
 *
 * Returns: 0 on success, -1 on error
 */
static int ssa_pcmcia_set_io_map (unsigned int sock, struct pccard_io_map *map)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

// DEBUG(2, "%s(): for sock %u\n", __FUNCTION__, sock);

   DEBUG(3, "%s(): map %u speed %u start %08lx stop %08lx\n",
         __FUNCTION__, map->map, map->speed, (unsigned long) map->start, (unsigned long) map->stop);
   DEBUG(3, "%s(): flags: %s%s%s%s%s%s%s%s%s\n",
         __FUNCTION__, 
         (map->flags==0)?"<NONE>":"",
         (map->flags&MAP_ACTIVE)?"ACTIVE ":"",
         (map->flags&MAP_16BIT)?"16BIT ":"",
         (map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
         (map->flags&MAP_0WS)?"0WS ":"",
         (map->flags&MAP_WRPROT)?"WRPROT ":"",
         (map->flags&MAP_ATTRIB)?"ATTRIB ":"",
         (map->flags&MAP_PREFETCH)?"PREFETCH ":"",
         (map->flags&MAP_USE_WAIT)?"USE_WAIT ":"");

   if (map->map >= MAX_IO_WIN) {
      printk ("%s(): map (%d) out of range\n", __FUNCTION__, map->map);
      return -1;
   }

   map->start = MAPPING_IO;
   map->stop = map->start + (MAP_SIZE - 1);

   skt->io_map[map->map] = *map;

   return 0;
}


/* ssa_pcmcia_get_mem_map()
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the get_mem_map() operation for the in-kernel PCMCIA
 * service (formerly SS_GetMemMap in Card Services). Just returns a
 * memory map descriptor which was assigned earlier by a
 * set_mem_map() request.
 *
 * Returns: 0 on success, -1 if the map index was out of range
 */
static int ssa_pcmcia_get_mem_map (unsigned int sock, struct pccard_mem_map *map)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

   DEBUG(2, "%s() for sock %u\n", __FUNCTION__, sock);

   if (map->map >= MAX_WIN)
      return -1;
      
   *map = skt->mem_map[map->map];
   return 0;
}


/* ssa_pcmcia_set_mem_map()
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the set_mem_map() operation for the in-kernel PCMCIA
 * service (formerly SS_SetMemMap in Card Services). We configure
 * the map speed as requested, but override the address ranges
 * supplied by Card Services.
 *
 * Returns: 0 on success, -1 on error
 */
static int ssa_pcmcia_set_mem_map (unsigned int sock, struct pccard_mem_map *map)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;

// DEBUG(2, "%s(): for sock %u\n", __FUNCTION__, sock);

#if 0
   DEBUG(3, "%s(): map %u speed %u sys_start %08lx sys_stop %08lx card_start %08x\n",
         __FUNCTION__, map->map, map->speed, map->sys_start, map->sys_stop, map->card_start);
   DEBUG(3, "%s(): flags: %s%s%s%s%s%s%s%s%s\n",
         __FUNCTION__, 
         (map->flags==0)?"<NONE>":"",
         (map->flags&MAP_ACTIVE)?"ACTIVE ":"",
         (map->flags&MAP_16BIT)?"16BIT ":"",
         (map->flags&MAP_AUTOSZ)?"AUTOSZ ":"",
         (map->flags&MAP_0WS)?"0WS ":"",
         (map->flags&MAP_WRPROT)?"WRPROT ":"",
         (map->flags&MAP_ATTRIB)?"ATTRIB ":"",
         (map->flags&MAP_PREFETCH)?"PREFETCH ":"",
         (map->flags&MAP_USE_WAIT)?"USE_WAIT ":"");
#endif

   if (map->map >= MAX_WIN) {
      printk ("%s(): map (%d) out of range\n", __FUNCTION__, map->map);
      return -1;
   }

   /*
      debug: check that required card address is in the first MAP_SIZE bytes.
      This is a an artificial limit if MAP_SIZE is less than 2MB, but we hope that
      all drivers we care about will stay well within this range...
   */

   if (map->card_start != 0)
   {
      printk ("%s(): card_start is greater than MAP_SIZE\n", __FUNCTION__);
      return -1;
   }

   map->sys_start = ((map->flags & MAP_ATTRIB) ? MAPPING_ATTR : MAPPING_MEM);
   map->sys_stop = map->sys_start + (MAP_SIZE - 1);

   skt->mem_map[map->map] = *map;

   return 0;
}


#if defined(CONFIG_PROC_FS)

/* ssa_pcmcia_proc_status()
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the /proc/bus/pccard/??/status file.
 *
 * Returns: the number of characters added to the buffer
 */
static int ssa_pcmcia_proc_status (char *buf, char **start, off_t pos, int count, int *eof, void *data)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
// unsigned int sock = (unsigned int) data;
   char *p = buf;

  p+=sprintf(p, "k_state  : %s%s%s%s%s%s%s\n", 
             skt->k_state.detect ? "detect " : "",
             skt->k_state.ready  ? "ready "  : "",
             skt->k_state.bvd1   ? "bvd1 "   : "",
             skt->k_state.bvd2   ? "bvd2 "   : "",
             skt->k_state.wrprot ? "wrprot " : "",
             skt->k_state.vs_3v  ? "vs_3v "  : "",
             skt->k_state.vs_Xv  ? "vs_Xv "  : "");

  p+=sprintf(p, "status   : %s%s%s%s%s%s%s%s%s\n",
             skt->k_state.detect ? "SS_DETECT " : "",
             skt->k_state.ready  ? "SS_READY " : "",
             skt->cs_state.Vcc   ? "SS_POWERON " : "",
             skt->cs_state.flags & SS_IOCARD ? "SS_IOCARD " : "",
             (skt->cs_state.flags & SS_IOCARD &&
              skt->k_state.bvd1) ? "SS_STSCHG " : "",
             ((skt->cs_state.flags & SS_IOCARD)==0 &&
              (skt->k_state.bvd1==0)) ? "SS_BATDEAD " : "",
             ((skt->cs_state.flags & SS_IOCARD)==0 &&
              (skt->k_state.bvd2==0)) ? "SS_BATWARN " : "",
             skt->k_state.vs_3v  ? "SS_3VCARD " : "",
             skt->k_state.vs_Xv  ? "SS_XVCARD " : "");

  p+=sprintf(p, "mask     : %s%s%s%s%s\n",
             skt->cs_state.csc_mask & SS_DETECT  ? "SS_DETECT "  : "",
             skt->cs_state.csc_mask & SS_READY   ? "SS_READY "   : "",
             skt->cs_state.csc_mask & SS_BATDEAD ? "SS_BATDEAD " : "",
             skt->cs_state.csc_mask & SS_BATWARN ? "SS_BATWARN " : "",
             skt->cs_state.csc_mask & SS_STSCHG  ? "SS_STSCHG "  : "");

  p+=sprintf(p, "cs_flags : %s%s%s%s%s\n",
             skt->cs_state.flags & SS_PWR_AUTO   ? "SS_PWR_AUTO "   : "",
             skt->cs_state.flags & SS_IOCARD     ? "SS_IOCARD "     : "",
             skt->cs_state.flags & SS_RESET      ? "SS_RESET "      : "",
             skt->cs_state.flags & SS_SPKR_ENA   ? "SS_SPKR_ENA "   : "",
             skt->cs_state.flags & SS_OUTPUT_ENA ? "SS_OUTPUT_ENA " : "");

  p+=sprintf(p, "Vcc      : %d\n", skt->cs_state.Vcc);
  p+=sprintf(p, "Vpp      : %d\n", skt->cs_state.Vpp);
  p+=sprintf(p, "IRQ      : %d\n", skt->cs_state.io_irq);

//  p+=sprintf(p, "I/O      : %u (%u)\n", skt->speed_io, 0);
//  p+=sprintf(p, "attribute: %u (%u)\n", skt->speed_attr, 0);
//  p+=sprintf(p, "common   : %u (%u)\n", skt->speed_mem, 0);

  return (p - buf);
}

/* ssa_pcmcia_proc_setup()
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Implements the proc_setup() operation for the in-kernel PCMCIA
 * service (formerly SS_ProcSetup in Card Services).
 *
 * Returns: 0 on success, -1 on error
 */
static void ssa_pcmcia_proc_setup (unsigned int sock, struct proc_dir_entry *base)
{
   struct proc_dir_entry *entry;

   DEBUG(4, "%s() for sock %u\n", __FUNCTION__, sock);

   if ((entry = create_proc_entry("status", 0, base)) == NULL) {
      printk ("unable to install \"status\" procfs entry\n");
      return;
   }

   entry->read_proc = ssa_pcmcia_proc_status;
   entry->data = (void *) sock;
}

#endif  /* defined(CONFIG_PROC_FS) */


static struct pccard_operations ssa_pcmcia_operations =
{
   init:               ssa_pcmcia_init,
   suspend:            ssa_pcmcia_suspend,
   register_callback:  ssa_pcmcia_register_callback,
   inquire_socket:     ssa_pcmcia_inquire_socket,
   get_status:         ssa_pcmcia_get_status,
   get_socket:         ssa_pcmcia_get_socket,
   set_socket:         ssa_pcmcia_set_socket,
   get_io_map:         ssa_pcmcia_get_io_map,
   set_io_map:         ssa_pcmcia_set_io_map,
   get_mem_map:        ssa_pcmcia_get_mem_map,
   set_mem_map:        ssa_pcmcia_set_mem_map,
#ifdef CONFIG_PROC_FS
   proc_setup:         ssa_pcmcia_proc_setup
#endif
};



static u32 ssa_bus_in (void *bus, u32 port, s32 sz)
{
   unsigned int data;

   switch (sz)
   {
//    case  0: data = inb    (port); break;
//    case  1: data = inw    (port); break;
//    case  2: data = inl    (port); break;
//    case -1: data = inw_ns (port); break;
//    case -2: data = inl_ns (port); break;
      default: data = 0;
   }

   DEBUG(5, "%s()  0x%08x %d = 0x%08x\n", __FUNCTION__, port, sz, data);

   return data;
}

static void ssa_bus_ins (void *bus, u32 port, void *buf, u32 count, s32 sz)
{
   DEBUG(5, "%s() 0x%08x %d %d\n", __FUNCTION__, port, sz, count);

   switch (sz)
   {
//    case  0: insb    (port, buf, count); break;
//    case  1: insw    (port, buf, count); break;
//    case  2: insl    (port, buf, count); break;
//    case -1: insw_ns (port, buf, count); break;
//    case -2: insl_ns (port, buf, count); break;
   }
}

static void ssa_bus_out (void *bus, u32 val, u32 port, s32 sz)
{
   DEBUG(5, "%s() 0x%08x 0x%08x %d\n", __FUNCTION__, port, val, sz);

   switch (sz)
   {
//    case  0: outb    (val, port); break;
//    case  1: outw    (val, port); break;
//    case  2: outl    (val, port); break;
//    case -1: outw_ns (val, port); break;
//    case -2: outl_ns (val, port); break;
   }
}

static void ssa_bus_outs (void *bus, u32 port, void *buf, u32 count, s32 sz)
{
   DEBUG(5, "%s() 0x%08x %d %d\n", __FUNCTION__, port, sz, count);

   switch (sz)
   {
//    case  0: outsb    (port, buf, count); break;
//    case  1: outsw    (port, buf, count); break;
//    case  2: outsl    (port, buf, count); break;
//    case -1: outsw_ns (port, buf, count); break;
//    case -2: outsl_ns (port, buf, count); break;
   }
}

static u32 ssa_bus_read (void *bus, void *addr, s32 sz)
{
   unsigned long flags;
   unsigned long vaddr = (((unsigned long) addr) & ~MAPPING_MASK) + IO_ADDRESS_PCMCIA_BASE;
   unsigned int  type  = (((unsigned long) addr) &  MAPPING_MASK);
   unsigned int  mask;
   unsigned int  gpio_output;
   unsigned int  data;

   if (type == MAPPING_ATTR) {
      if (sz == 0)                                                  /* 8bit attribute memory space read */
         mask = GPIO_PCMCIA_REG | GPIO_PCMCIA_MEM_SELECT | GPIO_PCMCIA_8BIT_SELECT;
      else {
         printk ("%s(): unsupported sz (%d) for attribute read\n", __FUNCTION__, sz);
         return 0;
      }
   }
   else if (type == MAPPING_MEM) {
      if (sz == 0)                                                  /* 8bit memory space read */
         mask = GPIO_PCMCIA_MEM_SELECT | GPIO_PCMCIA_8BIT_SELECT;
      else if (sz == 1)                                             /* 16bit memory space read */
         mask = GPIO_PCMCIA_MEM_SELECT;
      else {
         printk ("%s(): unsupported sz (%d) for memory read\n", __FUNCTION__, sz);
         return 0;
      }
   }
   else {
      printk ("%s(): unexpected mapping (in IO space ??) for memory read. sz = %d\n", __FUNCTION__, sz);
      return 0;
   }
   
   local_irq_save (flags);
   gpio_output = readl (VA_GPIO_OUTPUT);
   writel (gpio_output & ~mask, VA_GPIO_OUTPUT);                    /* set mask bits low while performing access */
   writel (SMC_WAITSTATES_PCMCIA_MEM, IO_ADDRESS_SMC_BASE + 8);
   udelay (1);
   data = readw (vaddr);
   udelay (1);

   /* fixme !!! */

// writel (SMC_WAITSTATES_PCMCIA_IO, IO_ADDRESS_SMC_BASE + 8);
   writel (gpio_output, VA_GPIO_OUTPUT);
   local_irq_restore (flags);

// DEBUG(5, "%s() 0x%08x %d = 0x%08x\n", __FUNCTION__, (unsigned int) addr, sz, data);

   return data;
}

static void ssa_bus_write (void *bus, u32 val, void *addr, s32 sz)
{
   unsigned long flags;
   unsigned long vaddr = (((unsigned long) addr) & ~MAPPING_MASK) + IO_ADDRESS_PCMCIA_BASE;
   unsigned int  type  = (((unsigned long) addr) &  MAPPING_MASK);
   unsigned int  mask;
   unsigned int  gpio_output;

   DEBUG(5, "%s(): 0x%08x = 0x%08x (sz == %d)\n", __FUNCTION__, (unsigned long) addr, val, sz);

   if (type == MAPPING_ATTR) {
      if (sz == 0)                                                  /* 8bit attribute memory space write */
         mask = GPIO_PCMCIA_REG | GPIO_PCMCIA_MEM_SELECT | GPIO_PCMCIA_8BIT_SELECT;
      else {
         printk ("%s(): unsupported sz (%d) for attribute write\n", __FUNCTION__, sz);
         return;
      }
   }
   else if (type == MAPPING_MEM) {
      if (sz == 0)                                                  /* 8bit memory space write */
         mask = GPIO_PCMCIA_MEM_SELECT | GPIO_PCMCIA_8BIT_SELECT;
      else if (sz == 1)                                             /* 16bit memory space write */
         mask = GPIO_PCMCIA_MEM_SELECT;
      else {
         printk ("%s(): unsupported sz (%d) for memory write\n", __FUNCTION__, sz);
         return;
      }
   }
   else {
      printk ("%s(): unexpected mapping (in IO space ??) for memory write. sz = %d\n", __FUNCTION__, sz);
      return;
   }

   local_irq_save (flags);
   gpio_output = readl (VA_GPIO_OUTPUT);
   writel (gpio_output & ~mask, VA_GPIO_OUTPUT);                    /* set mask bits low while performing access */
   writel (SMC_WAITSTATES_PCMCIA_MEM, IO_ADDRESS_SMC_BASE + 8);
   udelay (1);
   writew (val, vaddr);
   udelay (1);

   /* fixme !!! */

// writel (SMC_WAITSTATES_PCMCIA_IO, IO_ADDRESS_SMC_BASE + 8);
   writel (gpio_output, VA_GPIO_OUTPUT);
   local_irq_restore (flags);
}

static void ssa_bus_copy_from (void *bus, void *d, void *s, u32 count)
{
   DEBUG(5, "%s()\n", __FUNCTION__);
   memcpy_fromio (d, (u32) s, count);
}

static void ssa_bus_copy_to (void *bus, void *d, void *s, u32 count)
{
   DEBUG(5, "%s()\n", __FUNCTION__);
   memcpy_toio ((u32) d, s, count);
}

static void *ssa_bus_ioremap (void *bus, u_long ofs, u_long sz)
{
// void *result;
// result = ioremap (ofs, sz);
// DEBUG(5, "%s() 0x%08lx 0x%08x = 0x%08lx\n", __FUNCTION__, (unsigned long) ofs, sz, (unsigned long) result);
// return result;

   return (void *) ofs;
}

static void ssa_bus_iounmap (void *bus, void *addr)
{
// DEBUG(5, "%s() 0x%08lx\n", __FUNCTION__, (unsigned long) addr);
// iounmap (addr);
}

static int ssa_bus_request_irq (void *bus, u_int irq, void (*handler)(int, void *, struct pt_regs *), u_long flags, const char *device, void *dev_id)
{
   DEBUG(5, "%s()\n", __FUNCTION__);
   return request_irq (irq, handler, flags, device, dev_id);
}

static void ssa_bus_free_irq (void *bus, u_int irq, void *dev_id)
{
   DEBUG(5, "%s()\n", __FUNCTION__);
   free_irq (irq, dev_id);
}


static struct bus_operations ssa_bus_operations =
{
   priv:            NULL,
   b_in:            ssa_bus_in,
   b_ins:           ssa_bus_ins,
   b_out:           ssa_bus_out,
   b_outs:          ssa_bus_outs,
   b_ioremap:       ssa_bus_ioremap,
   b_iounmap:       ssa_bus_iounmap,
   b_read:          ssa_bus_read,
   b_write:         ssa_bus_write,
   b_copy_from:     ssa_bus_copy_from,
   b_copy_to:       ssa_bus_copy_to,
   b_request_irq:   ssa_bus_request_irq,
   b_free_irq:      ssa_bus_free_irq
};



/* ssa_pcmcia_driver_init()
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * This routine performs a basic sanity check to ensure that this
 * kernel has been built with the appropriate board-specific low-level
 * PCMCIA support, performs low-level PCMCIA initialization, registers
 * this socket driver with Card Services, and then spawns the daemon
 * thread which is the real workhorse of the socket driver.
 *
 * Returns: 0 on success, -1 on error
 */
static int __init ssa_pcmcia_driver_init (void)
{
   ssa_pcmcia_socket_t *skt = &ssa_pcmcia_socket;
   servinfo_t info;
   int ret;

   printk (KERN_INFO "SSA PCMCIA (CS release %s)\n", CS_RELEASE);

   CardServices (GetCardServicesInfo, &info);
   if (info.Revision != CS_RELEASE_CODE) {
      printk ("Card Services release codes do not match\n");
      return -EINVAL;
   }

   /* setup GPIO directions etc ?? */
   /* setup irq to catch card detect and battery status changes... */

   writel (SMC_WAITSTATES_PCMCIA_IO, IO_ADDRESS_SMC_BASE + 8);
   
   lowlevel_socket_state (&skt->k_state);

   ret = register_ss_entry (SSA_PCMCIA_SOCKET_COUNT, &ssa_pcmcia_operations);
   if (ret < 0) {
      printk ("Unable to register sockets\n");
      return ret;
   }

   ssa_pcmcia_poll_event (0);       /* startup the event poll timer. param is not used */
   return 0;
}


/* ssa_pcmcia_driver_shutdown()
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Invokes the low-level kernel service to free IRQs associated with this
 * socket controller and reset GPIO edge detection.
 */
static void __exit ssa_pcmcia_driver_shutdown (void)
{
   del_timer_sync (&poll_timer);
   unregister_ss_entry (&ssa_pcmcia_operations);
   flush_scheduled_tasks();
}

module_init(ssa_pcmcia_driver_init);
module_exit(ssa_pcmcia_driver_shutdown);

MODULE_AUTHOR("Andre McCurdy");
MODULE_DESCRIPTION("PCMCIA Card Services: SAA7752 + CPLD interface");
MODULE_LICENSE("GPL");

