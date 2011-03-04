
#include <stdarg.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#include <asm/io.h>

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define printk	pnx0106_epics_printk
#else
#define printk(fmt, args...)
#endif

#include <asm/arch/hardware.h>
#include <asm/arch/event_router.h>
#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsreg.h>
#include <asm/arch/epicsfw.h>

// Max size is X+Y+P+Pcache+16K (for alignment)
// 	X-mem is 96K (32K 24 bit words)
// 	Y-mem is 48K (32K 12 bit words)
// 	P-mem is 16K (4K 32 bit words)
// 	P-cache is 240K (15 16K pages)

#define PNX0106_EPICS_FW_MAX_SIZE	(416*1024)
#define PNX0106_EPICS_FW_CHUNK_SIZE	(16*1024)
#define PNX0106_EPICS_FW_MAX_CHUNKS	(PNX0106_EPICS_FW_MAX_SIZE/PNX0106_EPICS_FW_CHUNK_SIZE)

struct pnx0106_epics_fw
{
    struct list_head node;
    int fwid;
    int fwlen;
    int adjusted;
    int numchunks;
    char *chunks[PNX0106_EPICS_FW_MAX_CHUNKS];
    dma_addr_t phys[PNX0106_EPICS_FW_MAX_CHUNKS];
    int version;
    int labels_off;
    int xmem_off;
    int ymem_off;
    int pram_off;
    int pcache_off;
    int header_size;
    int labels_size;
    int xmem_size;
    int ymem_size;
    int pram_size;
    int pcache_size;
    int xmem_word;
    int ymem_word;
    int pram_word;
    int pcache_word;
};

struct pnx0106_epics_state
{
    int count;
    wait_queue_head_t wq;
    spinlock_t lock;
    spinlock_t printklock;
    volatile unsigned int pending;
    struct pnx0106_epics_fw *newfw;
    struct pnx0106_epics_fw *curfw;
    struct list_head fw_list;
    int codecid;
    int (*isr)(void* data);
    void *israrg;
    int upscale;
    int enables;
    char *shared_block0;
    int sharedsize_block0;
    dma_addr_t sharedphys_block0;
    char *shared_block1;
    int sharedsize_block1;
    dma_addr_t sharedphys_block1;
    unsigned char *xmemBackup;
    struct pnx0106_epics_equalizer_bands_gain equalizer_bands;
    int sfrequency;
};

struct pnx0106_epics_state pnx0106_epics_state;

#define DEVICE_NAME     	PNX0106_EPICS_DEVNAME

//#define e7b_epics_regs			((pnx0106_epics_ahb_ifregs *)IO_ADDRESS_E7B_BASE)

#define VH_E7B_AHB_IF_REG_DSP_CONTROL         0xFC

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_POS             1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_LEN             1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_MSK    0x00000002

#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_POS               2
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_LEN               1
#define VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_MSK      0x00000004

#define VH_E7B_AHB_IF_REG_INTC_MASK             0xDC


//      Define register index in e7b_epics_regs->e7b_dma_access_reg[0..0x40]
#define DMA_REG_CONTROL         (VH_E7B_AHB_IF_REG_DSP_CONTROL/4) // byte offset to u32 index
#define DMA_REG_INSTRUCT        (VH_E7B_AHB_IF_REG_DSP_EIR/4)
//      Define register index in e7b_epics_regs->e7b_control_reg[0..0x40]
#define CTL_REG_INTC_MASK       (VH_E7B_AHB_IF_REG_INTC_MASK/4)
// Defines for Epics DSP control register
# define DSP_RESET         VH_E7B_AHB_IF_REG_DSP_CONTROL_RESET_MSK
# define DSP_EIR_EN        VH_E7B_AHB_IF_REG_DSP_CONTROL_EIR_MSK
# define DSP_IDLE                  VH_E7B_AHB_IF_REG_DSP_CONTROL_IDLE_MSK

static volatile u32* debug;
static volatile u32* debug1;
static volatile u32* debug2;

// Epics7b device driver
static void pnx0106_epics_isr (int irq, void *dev_id, struct pt_regs *regs)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

#define VA_ER_INT_CLEAR(slice)                  (IO_ADDRESS_EVENT_ROUTER_BASE + 0x0C20 + (4 * (slice)))
#define ER_EVENTS_PER_SLICE                     (32)
#define ER_EVENT_SLICE(eid)                     ((eid) / ER_EVENTS_PER_SLICE)
#define ER_EVENT_MASK(eid)                      (1 << ((eid) % ER_EVENTS_PER_SLICE))

    writel(ER_EVENT_MASK(EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ2), VA_ER_INT_CLEAR(ER_EVENT_SLICE(EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ2)));

    if (unlikely (state->isr == NULL)) {
        pnx0106_epics_printk("Unexpected interrupt (no handler)\n");
        return;
    }

    (*state->isr) (state->israrg);
}

static void
pnx0106_epics_reset(struct pnx0106_epics_state *state)
{
    printk("Epics reset: regs %p audioss %p pcache %p\n", e7b_epics_regs, e7b_audioss_regs, e7b_pcache_conf_regs);
    printk("Epics reset: reseting Epics: dma_access_reg %p = 0x%x\n", &e7b_epics_regs->e7b_dma_access_reg[DMA_REG_CONTROL], DSP_RESET);
    e7b_epics_regs->e7b_dma_access_reg[DMA_REG_CONTROL] = DSP_RESET;
    printk("Epics reset: reseting Epics: intc_mask %p = 0x%x\n", &e7b_epics_regs->e7b_control_reg[CTL_REG_INTC_MASK], 0x3FFFF);
    e7b_epics_regs->e7b_control_reg[CTL_REG_INTC_MASK] = 0x3FFFF;
    printk("Epics reset: reseting Epics: audioss_creg_ahb_settings %p = 0x%x\n", &e7b_audioss_regs->audioss_creg_e7b_ahb_settings, 0);
    e7b_audioss_regs->audioss_creg_e7b_ahb_settings = 0;

    // clear all memory
    printk("Epics reset: clearing xmemory\n");
    memset((char*)e7b_epics_regs->e7b_xmemory_ram, 0, sizeof (e7b_epics_regs->e7b_xmemory_ram));
    printk("Epics reset: clearing ymemory\n");
    memset((char*)e7b_epics_regs->e7b_ymemory_ram, 0, sizeof (e7b_epics_regs->e7b_ymemory_ram));
    printk("Epics reset: clearing pmemory\n");
    memset((char*)e7b_epics_regs->e7b_pmemory_ram, 0, sizeof (e7b_epics_regs->e7b_pmemory_ram));
    printk("Epics reset: clearing control regs\n");
    memset((char*)e7b_epics_regs->e7b_control_reg, 0, sizeof (e7b_epics_regs->e7b_control_reg));

    printk("Epics reset: reseting Epics: intc_mask %p = 0x%x\n", &e7b_epics_regs->e7b_control_reg[CTL_REG_INTC_MASK], 0x3FFFF);
    e7b_epics_regs->e7b_control_reg[CTL_REG_INTC_MASK] = 0x3FFFF;

}

static int
pnx0106_epics_fw_expand(struct pnx0106_epics_fw *fw, loff_t pos, size_t size)
{
    int lastchunknum;
    int i;

    if (pos < 0 || pos > PNX0106_EPICS_FW_MAX_SIZE ||
	    pos + size > PNX0106_EPICS_FW_MAX_SIZE)
    {
	pnx0106_epics_printk(DEVICE_NAME ": image too large %d+%d %d\n", (int)pos, (int)size, PNX0106_EPICS_FW_MAX_SIZE);
	return -EINVAL;
    }

    printk(DEVICE_NAME ": fw %p, pos %d, size %d\n", fw, (int)pos, (int)size);
    lastchunknum = (pos + size - 1) / PNX0106_EPICS_FW_CHUNK_SIZE;
    printk(DEVICE_NAME ": lastchunknum %d\n", lastchunknum);

    for (i = fw->numchunks; i <= lastchunknum; i++)
    {
	// allocate a 16k chunk of DMA able memory for firmware
	printk(DEVICE_NAME ": allocating chunk #%d\n", i);

	fw->chunks[i] = consistent_alloc(GFP_DMA | GFP_KERNEL, PNX0106_EPICS_FW_CHUNK_SIZE, &fw->phys[i]);
//	fw->chunks[i] = dma_alloc_coherent((dev_t *)0, PNX0106_EPICS_FW_CHUNK_SIZE, &fw->phys[i], GFP_KERNEL | GFP_DMA);

//	dump_vmlist();
	printk(DEVICE_NAME ": virt %p, phys 0x%x\n", fw->chunks[i], (int)fw->phys[i]);

	if (fw->chunks[i] == 0)
	{
	    pnx0106_epics_printk(KERN_ERR "epics error: failed to allocate firmware buffer #%d\n", i);
	    return -ENOMEM;
	}

	// touch the pages
//	printk(DEVICE_NAME ": touching page @%p\n", fw->chunks[i]);
	fw->chunks[i][0] = 0;
//	dump_vmlist();
//	printk(DEVICE_NAME ": touching page @%p\n", fw->chunks[i] + 4*1024);
	fw->chunks[i][4*1024] = 0;
//	printk(DEVICE_NAME ": touching page @%p\n", fw->chunks[i] + 8*1024);
	fw->chunks[i][8*1024] = 0;
//	printk(DEVICE_NAME ": touching page @%p\n", fw->chunks[i] + 12*1024);
	fw->chunks[i][12*1024] = 0;
//	printk(DEVICE_NAME ": pages accessible\n");
    }

    if (fw->numchunks <= lastchunknum)
	fw->numchunks = lastchunknum + 1;

    if (fw->fwlen < pos + size)
	fw->fwlen = pos + size;

    return 0;
}

static int
pnx0106_epics_fw_find_chunk(struct pnx0106_epics_fw *fw, loff_t pos, size_t *maxsize, char **pbuf)
{
    int chunknum = pos / PNX0106_EPICS_FW_CHUNK_SIZE;
    int off = pos % PNX0106_EPICS_FW_CHUNK_SIZE;
    size_t size = *maxsize;

    if (chunknum >= fw->numchunks)
	return -EINVAL;

    if (size > PNX0106_EPICS_FW_CHUNK_SIZE - off)
	size = PNX0106_EPICS_FW_CHUNK_SIZE - off;

    *maxsize = size;
    *pbuf = &fw->chunks[chunknum][off];
    return 0;
}

static void
pnx0106_epics_fw_free(struct pnx0106_epics_fw *fw)
{
    int i;

    printk(DEVICE_NAME ": fw %p\n", fw);

    if (fw)
    {
	for (i = 0; i < fw->numchunks; i++)
	{
	    // free a 16k chunk of DMA able memory
	    printk(DEVICE_NAME ": free chunk #%d\n", i);

	    if (fw->chunks[i])
		consistent_free(fw->chunks[i], PNX0106_EPICS_FW_CHUNK_SIZE,
			fw->phys[i]);
	}

	kfree(fw);
    }
}

#define E7B_MAGIC		0x9eb1c601

typedef struct {
    unsigned long   magic;
    unsigned long   version;
    unsigned long   xmem_size;
    unsigned long   ymem_size;
    unsigned long   pcache_size;
    unsigned long   pram_size;
} e7b_firmware_header_v1;

static int
pnx0106_epics_fw_verify_v1(struct pnx0106_epics_fw *fw)
{
    e7b_firmware_header_v1 *hdr = (e7b_firmware_header_v1 *)fw->chunks[0];
    int fwsize;

    if (fw->fwlen < sizeof *hdr)
    {
	ERRMSG("Firmware image is too small %d\n", (int)fw->fwlen);
	return -EINVAL;
    }

    if (hdr->magic != E7B_MAGIC)
    {
	ERRMSG("Firmware doesn't contain correct ID: 0x%x\n", (int)hdr->magic);
	return -EINVAL;
    }

    if (hdr->version != 1)
    {
	ERRMSG("Firmware format uses other version(%d) than driver (%d)\n", (int)hdr->version, 1);
	return -EINVAL;
    }

    fwsize = sizeof *hdr + hdr->xmem_size + hdr->ymem_size + hdr->pram_size + hdr->pcache_size;

    if (fw->fwlen != fwsize)
    {
	ERRMSG("Firmware size (%d) differs from size in header (%d)\n", fw->fwlen, fwsize);
	return -EINVAL;
    }

    // verify that section sizes are within bounds
    if (hdr->pram_size > PNX0106_EPICS_FW_CHUNK_SIZE)
    {
	ERRMSG("Firmware PRAM section is too large (%d)\n", (int) hdr->pram_size);
	return -EINVAL;
    }

    fw->version = hdr->version;

    fw->labels_off = sizeof *hdr;
    fw->xmem_off = sizeof *hdr;
    fw->ymem_off = fw->xmem_off + hdr->xmem_size;
    fw->pcache_off = fw->ymem_off + hdr->ymem_size;
    fw->pram_off = fw->pcache_off + hdr->pcache_size;

    fw->header_size = sizeof *hdr;
    fw->labels_size = 0;
    fw->xmem_size = hdr->xmem_size;
    fw->ymem_size = hdr->ymem_size;
    fw->pcache_size = hdr->pcache_size;
    fw->pram_size = hdr->pram_size;

    fw->xmem_word = 0;
    fw->ymem_word = 0;
    fw->pcache_word = 0x1000;
    fw->pram_word = 0;

    return 0;
}

typedef struct {
    unsigned long   magic;
    unsigned long   version;
    unsigned long   header_size;
    unsigned long   labels_size;
    unsigned long   xmem_size;
    unsigned long   ymem_size;
    unsigned long   pcache_size;
    unsigned long   pram_size;
    unsigned long   xmem_word;
    unsigned long   ymem_word;
    unsigned long   pcache_word;
    unsigned long   pram_word;
    unsigned long   pad[4];
} e7b_firmware_header_v2;

static int
pnx0106_epics_fw_verify_labels(struct pnx0106_epics_fw *fw)
{
    char *labels = &fw->chunks[0][fw->labels_off];
    int sz = fw->labels_size;
    int i;
    int len;

    for (i = 0; i + 2 < sz; i += len)
    {
	len = 2;

	while (i + len <= sz)
	{
	    if (labels[i + len] == '\0')
		break;

	    len++;
	}

	len++;			// add one for the nul byte

//	printk("%d + %d > %d || labels[%d + %d - 1] (%d) != 0\n", i, len, sz, i, len, labels[i + len - 1]);
	if (i + len > sz || labels[i + len - 1] != '\0')
	    return -EINVAL;
    }

    // labels are ok
    return 0;
}

static int
pnx0106_epics_fw_verify_v2(struct pnx0106_epics_fw *fw, int headersz)
{
    e7b_firmware_header_v2 *hdr = (e7b_firmware_header_v2 *)fw->chunks[0];
    int fwsize;

    if (fw->fwlen < sizeof *hdr)
    {
	ERRMSG("Firmware image is too small %d\n", (int)fw->fwlen);
	return -EINVAL;
    }

    if (hdr->header_size != headersz)
    {
	ERRMSG("Firmware header length (%d) is incorrect (%d)\n", (int)hdr->header_size, sizeof *hdr);
	return -EINVAL;
    }

    if (hdr->header_size + hdr->labels_size > PNX0106_EPICS_FW_CHUNK_SIZE)
    {
	ERRMSG("Firmware Labels section is too large (%d)\n", (int) hdr->labels_size);
	return -EINVAL;
    }

    fwsize = headersz + hdr->labels_size + hdr->xmem_size + hdr->ymem_size + hdr->pram_size + hdr->pcache_size;

    if (fw->fwlen != fwsize)
    {
	ERRMSG("Firmware size (%d) differs from size in header (%d)\n", fw->fwlen, fwsize);
	return -EINVAL;
    }

    // verify that section sizes are within bounds
    if (hdr->pram_size > PNX0106_EPICS_FW_CHUNK_SIZE)
    {
	ERRMSG("Firmware PRAM section is too large (%d)\n", (int) hdr->pram_size);
	return -EINVAL;
    }

    fw->version = hdr->version;

    fw->labels_off = hdr->header_size;
    fw->xmem_off = fw->labels_off + hdr->labels_size;
    fw->ymem_off = fw->xmem_off + hdr->xmem_size;
    fw->pcache_off = fw->ymem_off + hdr->ymem_size;
    fw->pram_off = fw->pcache_off + hdr->pcache_size;

    fw->header_size = hdr->header_size;
    fw->labels_size = hdr->labels_size;
    fw->xmem_size = hdr->xmem_size;
    fw->ymem_size = hdr->ymem_size;
    fw->pcache_size = hdr->pcache_size;
    fw->pram_size = hdr->pram_size;

    fw->xmem_word = hdr->xmem_word;
    fw->ymem_word = hdr->ymem_word;
    fw->pcache_word = hdr->pcache_word;
    fw->pram_word = hdr->pram_word;

    if (pnx0106_epics_fw_verify_labels(fw) < 0)
    {
	ERRMSG("Firmware invalid labels section\n");
	return -EINVAL;
    }

    return 0;
}

typedef struct {
    unsigned long   magic;			/* 0 */
    unsigned long   version;			/* 4 */
    unsigned long   header_size;		/* 8 */
    unsigned long   labels_size;		/* 12 */
    unsigned long   xmem_size;			/* 16 */
    unsigned long   ymem_size;			/* 20 */
    unsigned long   pcache_size;		/* 24 */
    unsigned long   pram_size;			/* 28 */
    unsigned long   xmem_word;			/* 32 */
    unsigned long   ymem_word;			/* 36 */
    unsigned long   pcache_word;		/* 40 */
    unsigned long   pram_word;			/* 44 */
    unsigned long   fw_id;			/* 48 */
    char            fw_name[8];			/* 52 */
    char            buildstr[16];		/* 60 */
    unsigned long   pad[5];			/* 76 */
} e7b_firmware_header_v3;			/* 96 */

#define E7B_VERSION		3

static int
pnx0106_epics_fw_verify_v3(struct pnx0106_epics_fw *fw)
{
    e7b_firmware_header_v3 *hdr = (e7b_firmware_header_v3 *)fw->chunks[0];

    if (pnx0106_epics_fw_verify_v2(fw, sizeof *hdr) < 0)
	return -EINVAL;

    fw->fwid = hdr->fw_id;
    return 0;
}

static int
pnx0106_epics_fw_verify(struct pnx0106_epics_fw *fw, int *version)
{
    e7b_firmware_header_v1 *hdr = (e7b_firmware_header_v1 *)fw->chunks[0];

    if (fw->fwlen < sizeof (e7b_firmware_header_v1))
    {
	ERRMSG("Firmware image is too small %d\n", (int)fw->fwlen);
	return -EINVAL;
    }

    if (hdr->magic != E7B_MAGIC)
    {
	ERRMSG("Firmware doesn't contain correct ID: 0x%x\n", (int)hdr->magic);
	return -EINVAL;
    }

    if (version)
	*version = hdr->version;

    switch (hdr->version)
    {
    case 1:
	return pnx0106_epics_fw_verify_v1(fw);
    case 2:
	return pnx0106_epics_fw_verify_v2(fw, sizeof (e7b_firmware_header_v2));
    case 3:
	return pnx0106_epics_fw_verify_v3(fw);
    default:
	ERRMSG("Firmware format uses other version(%d) than driver (%d)\n", (int)hdr->version, E7B_VERSION);
	break;
    }

    return -EINVAL;
}

static void
pnx0106_epics_fw_memmove(struct pnx0106_epics_fw *fw, int dest, int src, int len)
{
    // within the firmware chunks move from src to dest with them potentially
    // overlapping
//    printk("fw_memmove: fwlen %d\n", fw->fwlen);

    if (dest > src)
    {
	while (len)
	{
	    int dend = dest + len - 1;
	    int send = src + len - 1;
	    int dchunk = dend / PNX0106_EPICS_FW_CHUNK_SIZE;
	    int schunk = send / PNX0106_EPICS_FW_CHUNK_SIZE;
	    int dmax = dend - dchunk * PNX0106_EPICS_FW_CHUNK_SIZE + 1;
	    int smax = send - schunk * PNX0106_EPICS_FW_CHUNK_SIZE + 1;
	    int max = dmax;
	    int doff;
	    int soff;

	    if (max > smax)
		max = smax;

	    if (max > len)
		max = len;

	    doff = dmax - max;
	    soff = smax - max;

//	    printk("fw_memmove: dest %p, src %p, len %d\n", &fw->chunks[dchunk][doff], &fw->chunks[schunk][soff], max);
//	    printk("fw_memmove: dest %d src %d dend %d send %d dchunk %d schunk %d dmax %d smax %d doff %d soff %d max %d\n", dest, src, dend, send, dchunk, schunk, dmax, smax, doff, soff, max);
	    memmove(&fw->chunks[dchunk][doff], &fw->chunks[schunk][soff], max);
	    len -= max;
	}
    }
    else
    {
	while (len)
	{
	    int dchunk = dest / PNX0106_EPICS_FW_CHUNK_SIZE;
	    int schunk = src / PNX0106_EPICS_FW_CHUNK_SIZE;
	    int doff = dest % PNX0106_EPICS_FW_CHUNK_SIZE;
	    int soff = src % PNX0106_EPICS_FW_CHUNK_SIZE;
	    int dmax = PNX0106_EPICS_FW_CHUNK_SIZE - doff;
	    int smax = PNX0106_EPICS_FW_CHUNK_SIZE - soff;
	    int max = dmax;

	    if (max > smax)
		max = smax;

	    if (max > len)
		max = len;

//	    printk("fw_memmove: dest %p, src %p, len %d\n", &fw->chunks[dchunk][doff], &fw->chunks[schunk][soff], max);
	    memmove(&fw->chunks[dchunk][doff], &fw->chunks[schunk][soff], max);
	    len -= max;
	    dest += max;
	    src += max;
	}
    }
}

static void
pnx0106_epics_fw_memcpy(struct pnx0106_epics_fw *fw, char *dest, int src, int len)
{
    while (len)
    {
	int schunk = src / PNX0106_EPICS_FW_CHUNK_SIZE;
	int soff = src % PNX0106_EPICS_FW_CHUNK_SIZE;
	int max = PNX0106_EPICS_FW_CHUNK_SIZE - soff;

	if (max > len)
	    max = len;

	printk("fw_memcpy: dest %p, src %p, len %d\n", dest, &fw->chunks[schunk][soff], max);
	memcpy(dest, &fw->chunks[schunk][soff], max);
	len -= max;
	dest += max;
	src += max;
    }
}

static void
pnx0106_epics_load_pram(void)
{
    e7b_epics_regs->e7b_dma_access_reg[DMA_REG_INSTRUCT] = 0xFC000000;
    e7b_epics_regs->e7b_dma_access_reg[DMA_REG_CONTROL] = DSP_EIR_EN | DSP_IDLE;
    e7b_epics_regs->e7b_dma_access_reg[DMA_REG_CONTROL] &= ~DSP_IDLE;
    e7b_epics_regs->e7b_dma_access_reg[DMA_REG_CONTROL] &= ~DSP_EIR_EN;
}

static void
pnx0106_epics_reset_mmu(void)
{
    e7b_pcache_conf_regs->general_cache_settings = 0x01;
    e7b_pcache_conf_regs->general_cache_settings = 0x00;

    while (e7b_pcache_conf_regs->cache_reset_status != 0)
	;

    e7b_pcache_conf_regs->general_cache_settings = 0x02;
}

static void
pnx0106_epics_load_mmu(int start, dma_addr_t phys[], int count)
{
    int i;

    printk("load_mmu: start %d phys %p count %d\n", start, phys, count);

    if (start < 1)
	return;

    for (i = 0; i < count && start + i < 15; i++)
    {
	printk("load_mmu: entry %d phys 0x%x\n", start + i - 1, phys[i]);
	printk("load_mmu: reg %p value 0x%x\n", &e7b_pcache_conf_regs->remap[start + i - 1], (unsigned int)phys[i] >> 14);
	// We should flush the chunk out of cache and into SDRAM at this point.
	e7b_pcache_conf_regs->remap[start + i - 1] = (unsigned int)phys[i] >> 14;
    }
}

static int
pnx0106_epics_buffer_checksum(struct pnx0106_epics_fw *fw, int off, int len)
{
    int sum = len;
    int chunk = off / PNX0106_EPICS_FW_CHUNK_SIZE;
    int coff = off % PNX0106_EPICS_FW_CHUNK_SIZE;

    if (off + len > fw->fwlen)
    {
	if (off > fw->fwlen)
	    len = 0;
	else
	    len = fw->fwlen - off;
    }

    while (len)
    {
	int num = PNX0106_EPICS_FW_CHUNK_SIZE - coff;
	unsigned char *cp = &fw->chunks[chunk][coff];

	if (num > len)
	    num = len;

	len -= num;

	while (num--)
	    sum += *cp++;

	chunk++;
	coff = 0;
    }


    return sum;
}

#if 0
static void
pnx0106_epics_fw_checksum(struct pnx0106_epics_state *state, struct pnx0106_epics_fw *fw)
{
    printk("Epics: checksum %p X %x Y %x Pcache %x P %x\n", fw,
	    pnx0106_epics_buffer_checksum(fw, fw->xmem_off, fw->xmem_size),
	    pnx0106_epics_buffer_checksum(fw, fw->ymem_off, fw->ymem_size),
	    pnx0106_epics_buffer_checksum(fw, fw->pcache_off, fw->pcache_size),
	    pnx0106_epics_buffer_checksum(fw, fw->pram_off, fw->pram_size));
}
#endif

static void
pnx0106_epics_fw_dump(struct pnx0106_epics_state *state, struct pnx0106_epics_fw *fw)
{
    printk(DEVICE_NAME " epics_fw: id %d version %d size %d chunks %d adjusted %d\n", fw->fwid, fw->version, fw->fwlen, fw->numchunks, fw->adjusted);
    printk(DEVICE_NAME " epics_fw: xmem off %d size %d checksum %x\n", fw->xmem_off, fw->xmem_size,
	    pnx0106_epics_buffer_checksum(fw, fw->xmem_off, fw->xmem_size));
    printk(DEVICE_NAME " epics_fw: ymem off %d size %d checksum %x\n", fw->ymem_off, fw->ymem_size,
	    pnx0106_epics_buffer_checksum(fw, fw->ymem_off, fw->ymem_size));
    printk(DEVICE_NAME " epics_fw: pram off %d size %d checksum %x\n", fw->pram_off, fw->pram_size,
	    pnx0106_epics_buffer_checksum(fw, fw->pram_off, fw->pram_size));
    printk(DEVICE_NAME " epics_fw: pcache off %d size %d checksum %x\n", fw->pcache_off, fw->pcache_size,
	    pnx0106_epics_buffer_checksum(fw, fw->pcache_off, fw->pcache_size));
}

static int
pnx0106_epics_fw_load(struct pnx0106_epics_state *state, struct pnx0106_epics_fw *fw)
{
    int r;
    printk(DEVICE_NAME ": fw_load\n");
    pnx0106_epics_fw_dump(state, fw);

    // Load the firmware into the Epics7b
    if (!fw->adjusted)
    {
	r = pnx0106_epics_fw_verify(fw, NULL);

	if (r < 0)
	    return r;

	pnx0106_epics_fw_dump(state, fw);

	// Move pcache section to be 16KB aligned.
	if (fw->pcache_size && fw->pcache_off % (16*1024) != 0)
	{
	    int shift = 16*1024 - (fw->pcache_off % (16*1024));
	    int len = fw->fwlen;

	    r = pnx0106_epics_fw_expand(fw, len, shift);

	    if (r < 0)
		return r;

	    printk(DEVICE_NAME ": fw_load: pcache moving from 0x%x to 0x%x\n", fw->pcache_off, fw->pcache_off + shift);
	    pnx0106_epics_fw_memmove(fw, fw->pcache_off + shift, fw->pcache_off, len - fw->pcache_off);
	    fw->pcache_off += shift;
	    fw->pram_off += shift;
	}

	fw->adjusted = 1;
	pnx0106_epics_fw_dump(state, fw);
    }

    // Reset
    if (fw->fwid == PNX0106_EPICS_PP_FWID)
    {
	printk(DEVICE_NAME ": fw_load: reseting epics\n");
	pnx0106_epics_reset(state);
    }
    else
	printk(DEVICE_NAME ": fw_load: NOT reseting epics\n");

    // Write X & Y Memories
    printk(DEVICE_NAME ": fw_load: loading X memory\n");
    pnx0106_epics_fw_memcpy(fw, (char *)pnx0106_epics_xram2arm(fw->xmem_word), fw->xmem_off, fw->xmem_size);
    printk(DEVICE_NAME ": fw_load: loading Y memory\n");
    pnx0106_epics_fw_memcpy(fw, (char *)pnx0106_epics_yram2arm(fw->ymem_word), fw->ymem_off, fw->ymem_size);

    // Write P memory
    if(fw->pram_size)
    {
        printk(DEVICE_NAME ": fw_load: loading P memory\n");
        pnx0106_epics_fw_memcpy(fw, (char *)pnx0106_epics_pram2arm(fw->pram_word), fw->pram_off, fw->pram_size);
        pnx0106_epics_load_pram();
    }

    // Load PCache MMU
    printk(DEVICE_NAME ": fw_load: reseting pcache MMU\n");
    pnx0106_epics_reset_mmu();
    printk(DEVICE_NAME ": fw_load: loading pcache MMU\n");
    pnx0106_epics_load_mmu((fw->pcache_word * sizeof (unsigned int)) / PNX0106_EPICS_FW_CHUNK_SIZE,
	    &fw->phys[fw->pcache_off / PNX0106_EPICS_FW_CHUNK_SIZE],
	    (fw->pcache_size + PNX0106_EPICS_FW_CHUNK_SIZE - 1) / PNX0106_EPICS_FW_CHUNK_SIZE);

    // mark that this firmware image is loaded
    state->curfw = fw;
    printk(DEVICE_NAME ": fw_load: loaded!\n");

    if (fw->fwid == PNX0106_EPICS_PP_FWID)
    {
	printk(DEVICE_NAME ": EPX_INFXCHG %x\n", pnx0106_epics_infxchg());
	printk(DEVICE_NAME ": EPX_CONTROLADDRESS %x\n", pnx0106_epics_controladdress());
	printk(DEVICE_NAME ": EPX_DSP_DOWNLOAD_READY %x\n", pnx0106_epics_pp_getlabel("DSP_DOWNLOAD_READY", EPX_DSP_DOWNLOAD_READY));
	printk(DEVICE_NAME ": EPX_STOP %x\n", pnx0106_epics_pp_getlabel("STOP", EPX_STOP));
    }

    return 0;
}

static void
pnx0106_epics_fw_save(struct pnx0106_epics_state *state, struct pnx0106_epics_fw *fw)
{
    struct list_head *entry;
    struct list_head *n;
    int fwid = fw->fwid;

    // remove any prior firmware image with the same id
    list_for_each_safe(entry, n, &state->fw_list)
    {
	struct pnx0106_epics_fw *xfw;
	xfw = list_entry(entry, struct pnx0106_epics_fw, node);

	if (xfw->fwid == fwid)
	    list_del(&xfw->node);
    }

    // add the firmware image to the list of firmware images
    list_add(&fw->node, &state->fw_list);
    state->newfw = NULL;
}

static int
pnx0106_epics_fw_segment(struct pnx0106_epics_state *state, loff_t offset, const char *buffer, size_t length)
{
    struct pnx0106_epics_fw *fw = state->newfw;
    int r;

    printk(DEVICE_NAME ": fw_segment\n");

    // get load structure
    if (fw == NULL)
    {
	fw = kmalloc(sizeof *fw, GFP_KERNEL);

	if (fw == NULL)
	    return -ENOMEM;

	memset(fw, 0, sizeof *fw);
	state->newfw = fw;
    }

    printk(DEVICE_NAME ": fw %p, fwlen %d\n", fw, (int)fw->fwlen);
    printk(DEVICE_NAME ": offset %d, length %d\n", (int)offset, length);

    // expand the firmware buffer if necessary
    r = pnx0106_epics_fw_expand(fw, offset, length);

    printk(DEVICE_NAME ": fw_expand r %d, fwlen %d\n", r, fw->fwlen);

    if (r < 0)
	return r;

    // copy from user to the fw buffer
    while (length)
    {
	size_t chunklen = length;
	char *buf;

	r = pnx0106_epics_fw_find_chunk(fw, offset, &chunklen, &buf);
	printk(DEVICE_NAME ": r %d, chunklen %d, buf %p\n", r, chunklen, buf);

	if (r < 0)
	    return r;

	copy_from_user(buf, buffer, chunklen);
	offset += chunklen;
	buffer += chunklen;
	length -= chunklen;
    }

    return 0;
}

static loff_t
pnx0106_epics_llseek(struct file *file, loff_t offset, int origin)
{
    printk(DEVICE_NAME ": llseek???\n");
    return (loff_t)0;
}

static ssize_t
pnx0106_epics_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    loff_t pos = *ppos;
    int r;

    printk(DEVICE_NAME ": write, file %p, buf %p, count %d, ppos %p, *ppos %d\n", file, buf, count, ppos, (int)pos);

    r = pnx0106_epics_fw_segment(state, pos, buf, count);
    printk(DEVICE_NAME ": r %d, newfw %p, fw max size %d\n", r, state->newfw, state->newfw ? state->newfw->fwlen : 0);

    if (r < 0)
	return r;

    *ppos += count;
    printk(DEVICE_NAME ": write done!\n");
    return count;
}

static unsigned int
pnx0106_epics_poll(struct file *file, struct poll_table_struct *pt)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    printk(DEVICE_NAME ": poll\n");
    poll_wait(file, &state->wq, pt);
    return state->pending;
}

static int
pnx0106_epics_ioctl_fw_save(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct pnx0106_epics_fw *fw = state->newfw;
    int fwid;
    int version;
    printk(DEVICE_NAME ": ioctl_fw_save: state %p arg %lu\n", state, arg);
    printk(DEVICE_NAME ": ioctl_fw_save: fw %p\n", fw);

    if (arg < 0x10)
	fwid = arg;
    else if (copy_from_user(&fwid, (void *)arg, sizeof fwid))
	return -EFAULT;

    // validate the firmware image
    if (fw == NULL)
	return -EINVAL;

    if (pnx0106_epics_fw_verify(fw, &version) < 0)
	return -EINVAL;

    // set the firmware image id
    if (version < 3)
	fw->fwid = fwid;

    // check the firmware image id
    if (fw->fwid != fwid)
    {
	printk(DEVICE_NAME ": ioctl_fw_save: Firmware ID does not match (%d,%d)\n", fw->fwid, fwid);
	return -EINVAL;
    }

    printk(DEVICE_NAME ": ioctl_fw_save: &fw_list %p node %p\n", &state->fw_list, &fw->node);

    // add the firmware image to the list of firmware images
    pnx0106_epics_fw_save(state, fw);

    printk(DEVICE_NAME ": ioctl_fw_save: done\n");
    return 0;
}

static int
pnx0106_epics_ioctl_fw_start(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct list_head *entry;
    struct pnx0106_epics_fw *fw = NULL;
    int fwid;

    printk(DEVICE_NAME ": ioctl_fw_start: state %p arg %lu\n", state, arg);

    if (arg < 0x10)
	fwid = arg;
    else if (copy_from_user(&fwid, (void *)arg, sizeof fwid))
	return -EFAULT;

    // find the firmware image based upon the firmware id and load it
    list_for_each(entry, &state->fw_list)
    {
	fw = list_entry(entry, struct pnx0106_epics_fw, node);

	if (fw->fwid == fwid)
	    break;
    }

    if (fw == NULL)
	return -EINVAL;

    return pnx0106_epics_fw_load(state, fw);
}

#if 0
static int
pnx0106_epics_ioctl_fw_unload(struct pnx0106_epics_state *state, unsigned long arg)
{
    printk(DEVICE_NAME ": ioctl_fw_unload: state %p arg %lu\n", state, arg);
    // unload the current firmware image
    // we can't really do this so just turn off the Epics and put it in
    // state such that a load command will allow it to run again.
    state->curfw = NULL;
    return 0;
}

static int
pnx0106_epics_ioctl_fw_get(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct pnx0106_epics_fw *fw = state->curfw;
    printk(DEVICE_NAME ": ioctl_fw_get: state %p arg %lu\n", state, arg);

    // Return the image id of the currently loaded firmware image
    if (fw == NULL)
	return -EINVAL;

    *(int *)arg = fw->fwid;
    return 0;
}
#endif

static int
pnx0106_epics_ioctl_fw_curr(struct pnx0106_epics_state *state, unsigned long arg)
{
    int fwid = 0;

    if (state->curfw != NULL)
	fwid = state->curfw->fwid;

    if (copy_to_user((void *)arg, &fwid, sizeof fwid))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_ioctl_fw_num(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct list_head *entry;
    int count = 0;

    list_for_each(entry, &state->fw_list)
	count++;

    if (copy_to_user((void *)arg, &count, sizeof count))
	return -EFAULT;

    return 0;
}

static struct pnx0106_epics_fw *
pnx0106_epics_fw_find_id(struct pnx0106_epics_state *state, int fwid)
{
    struct list_head *entry;
    struct pnx0106_epics_fw *fw;

    list_for_each(entry, &state->fw_list)
    {
	fw = list_entry(entry, struct pnx0106_epics_fw, node);

	if (fw->fwid == fwid)
	    return fw;
    }

    return NULL;
}

static struct pnx0106_epics_fw *
pnx0106_epics_fw_find_index(struct pnx0106_epics_state *state, int index)
{
    struct list_head *entry;
    int i = 0;

    list_for_each(entry, &state->fw_list)
    {
	if (i == index)
	    return list_entry(entry, struct pnx0106_epics_fw, node);

	i++;
    }

    return NULL;
}

static int
pnx0106_epics_ioctl_fw_info(struct pnx0106_epics_state *state, unsigned long arg)
{
    int index = 0;
    struct pnx0106_epics_fw *fw = NULL;
    struct pnx0106_epics_fw_info info;

    if (copy_from_user(&index, (void *)arg, sizeof index))
	return -EFAULT;

    fw = pnx0106_epics_fw_find_index(state, index);

    if (fw == NULL)
	return -EINVAL;

    memset(&info, 0, sizeof info);
    info.index = index;
    info.fwid = fw->fwid;

    if (fw->version >= 3)
	strncpy(info.fwname, ((e7b_firmware_header_v3 *)fw->chunks[0])->fw_name, sizeof info.fwname);

    info.fwlen = fw->fwlen;
    info.numchunks = fw->numchunks;
    info.version = fw->version;

    if (fw->version >= 3)
	strncpy(info.buildstr, ((e7b_firmware_header_v3 *)fw->chunks[0])->buildstr, sizeof info.buildstr);

    info.header_size = fw->header_size;
    info.labels_size = fw->labels_size;
    info.xmem_size = fw->xmem_size;
    info.ymem_size = fw->ymem_size;
    info.pram_size = fw->pram_size;
    info.pcache_size = fw->pcache_size;
    info.xmem_word = fw->xmem_word;
    info.ymem_word = fw->ymem_word;
    info.pram_word = fw->pram_word;
    info.pcache_word = fw->pcache_word;
    info.labels_csum = pnx0106_epics_buffer_checksum(fw, fw->labels_off, fw->labels_size);
    info.xmem_csum = pnx0106_epics_buffer_checksum(fw, fw->xmem_off, fw->xmem_size);
    info.ymem_csum = pnx0106_epics_buffer_checksum(fw, fw->ymem_off, fw->ymem_size);
    info.pram_csum = pnx0106_epics_buffer_checksum(fw, fw->pram_off, fw->pram_size);
    info.pcache_csum = pnx0106_epics_buffer_checksum(fw, fw->pcache_off, fw->pcache_size);

    if (copy_to_user((void *)arg, &info, sizeof info))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_fw_count_labels(struct pnx0106_epics_fw *fw)
{
    char *labels = &fw->chunks[0][fw->labels_off];
    int sz = fw->labels_size;
    int count = 0;

    while (sz >= 4)
    {
	int len = 3 + strlen(&labels[2]);

	if (len > sz)
	    break;

	sz -= len;
	labels += len;
        count++;
    }

    return count;
}

static int
pnx0106_epics_ioctl_fw_numlabels(struct pnx0106_epics_state *state, unsigned long arg)
{
    int index = 0;
    struct pnx0106_epics_fw *fw = NULL;
    int count;

    if (copy_from_user(&index, (void *)arg, sizeof index))
	return -EFAULT;

    fw = pnx0106_epics_fw_find_index(state, index);

    if (fw == NULL)
	return -EINVAL;

    count = pnx0106_epics_fw_count_labels(fw);

    if (copy_to_user((void *)arg, &count, sizeof count))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_fw_getlabel(struct pnx0106_epics_fw *fw, char *name)
{
    char *labels = &fw->chunks[0][fw->labels_off];
    int sz = fw->labels_size;

    printk("pnx0106_epics_fw_getlabel sz = %d\n",sz);

    while (sz >= 4)
    {
	int len = 3 + strlen(&labels[2]);

	if (len <= sz && strcmp(&labels[2], name) == 0)
	    return ((labels[1] & 0xFF) << 8) | (labels[0] & 0xFF);

	sz -= len;
	labels += len;
    }

    return -1;
}

int
pnx0106_epics_getlabel(char *name, int defaddr)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    struct pnx0106_epics_fw *fw = state->curfw;
    int addr;

    if (fw == NULL)
	return -1;

    addr = pnx0106_epics_fw_getlabel(fw, name);
    return (addr < 0) ? defaddr : addr;
}

int
pnx0106_epics_pp_getlabel(char *name, int defaddr)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    struct pnx0106_epics_fw *fw;
    int addr;

    fw = pnx0106_epics_fw_find_id(state, PNX0106_EPICS_PP_FWID);

    if (fw == NULL)
	return -1;

    addr = pnx0106_epics_fw_getlabel(fw, name);
    return (addr < 0) ? defaddr : addr;
}

int
pnx0106_epics_controladdress(void)
{
    // prefer the information from the firmware image if we have it
    return pnx0106_epics_pp_getlabel("CONTROLADDRESS", EPX_CONTROLADDRESS);
}

int
pnx0106_epics_infxchg(void)
{
    // prefer the information from the firmware image if we have it
    return pnx0106_epics_pp_getlabel("INFXCHG", EPX_INFXCHG);
}

int
pnx0106_epics_issue_command(char *name, int defaddr, int arg1, int arg2)
{
    int cmdaddr = pnx0106_epics_pp_getlabel(name, defaddr);

    if (cmdaddr < 0)
	return -EINVAL;

    *(pnx0106_epics_ctr2arm(pnx0106_epics_controladdress())) = cmdaddr;
    pnx0106_epics_interrupt_req();
    return 0;
}

static int
pnx0106_epics_fw_getlabel_index(struct pnx0106_epics_fw *fw, int index, char **name)
{
    char *labels = &fw->chunks[0][fw->labels_off];
    int sz = fw->labels_size;
    int len;
    int i = 0;

    while (sz >= 4)
    {
	len = 3 + strlen(&labels[2]);

	if (len <= sz && i == index)
	{
	    if (name)
		*name = &labels[2];

	    return ((labels[1] & 0xFF) << 8) | (labels[0] & 0xFF);
	}

	sz -= len;
	labels += len;
	i++;
    }

    return -1;
}

static int
pnx0106_epics_ioctl_fw_getlabel(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct pnx0106_epics_fw *fw = NULL;
    struct pnx0106_epics_fw_label label;
    char *name;

    if (copy_from_user(&label, (void *)arg, sizeof label))
	return -EFAULT;

    fw = pnx0106_epics_fw_find_index(state, label.fwindex);

    if (fw == NULL)
	return -EINVAL;

    label.word = pnx0106_epics_fw_getlabel_index(fw, label.labelindex, &name);

    if (label.word < 0)
	return -EINVAL;

    strncpy(label.label, name, sizeof label.label);

    if (copy_to_user((void *)arg, &label, sizeof label))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_ioctl_getupscale(struct pnx0106_epics_state *state, unsigned long arg)
{
    int upscale = state->upscale;

    // Copy value to user
    if (copy_to_user((void *)arg, &upscale, sizeof upscale))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_ioctl_setupscale(struct pnx0106_epics_state *state, unsigned long arg)
{
    int upscale;

    if (copy_from_user(&upscale, (void *)arg, sizeof upscale))
	return -EFAULT;

    // Check value
    switch (upscale)
    {
    case 1:
    case 2:
    case 4:
	break;
    default:
	return -EINVAL;
    }

    // save in state structure
    state->upscale = upscale;

    debug    = pnx0106_epics_xram2arm(0x2fa);

    // Tell Epics what to do
    switch (upscale)
    {
    case 1:
	// Turn off upscaling
        printk("Set No upsampling\n");
	pnx0106_epics_issue_command("NO_UPSAMPLING", -1, 0, 0);
        break;
    case 2:
        // Set upsampling X2
	printk("Set Upsampling X 2\n");
	pnx0106_epics_issue_command("UPSAMPLINGx2", -1, 0, 0);
	break;
    case 4:
        // Set upsampling X4
        printk("Set Upsampling X 4\n");
	pnx0106_epics_issue_command("UPSAMPLINGx4", -1, 0, 0);
	break;
    }

    {
        // Print out value from Epics
        int i = 5000;
        while (i--)
            ;
        printk("EPICS PP Upscale info = %x\n",debug[0]);
    }

    return 0;
}

static int
pnx0106_epics_ioctl_getoutenable(struct pnx0106_epics_state *state, unsigned long arg)
{
    int enables = state->enables;

    // Copy value to user
    if (copy_to_user((void *)arg, &enables, sizeof enables))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_ioctl_setoutenable(struct pnx0106_epics_state *state, unsigned long arg)
{
    int enables;

    if (copy_from_user(&enables, (void *)arg, sizeof enables))
	return -EFAULT;

    debug1    = pnx0106_epics_xram2arm(0x2fb);
    debug2    = pnx0106_epics_xram2arm(0x2fc);


    // Check value
    switch (enables)
    {
    case 0:
    case PNX0106_EPICS_OUTPUT_DAC:
    case PNX0106_EPICS_OUTPUT_SPDIF:
    case PNX0106_EPICS_OUTPUT_SPDIF|PNX0106_EPICS_OUTPUT_DAC:
	break;
    default:
	return -EINVAL;
    }

    // save in state structure
    state->enables = enables;


    // Check value
    switch (enables)
    {
    case 0:
        printk("Mute DAC and SPdif\n");
        pnx0106_epics_issue_command("DISABLENEWSAMPLEISR", -1, 0, 0);
        pnx0106_epics_issue_command("MUTEDACOUT", -1, 0, 0);
        pnx0106_epics_issue_command("MUTESPDIFOUT", -1, 0, 0);

        break;

    case PNX0106_EPICS_OUTPUT_DAC:
        printk("Unmute DAC and Mute SPdif\n");
        pnx0106_epics_issue_command("UNMUTEDACOUT", -1, 0, 0);
        pnx0106_epics_issue_command("MUTESPDIFOUT", -1, 0, 0);
        pnx0106_epics_issue_command("ENABLENEWSAMPLEISR", -1, 0, 0);

        break;

    case PNX0106_EPICS_OUTPUT_SPDIF:
	printk("Mute DAC and Unmute SPdif\n");
        pnx0106_epics_issue_command("MUTEDACOUT", -1, 0, 0);
        pnx0106_epics_issue_command("UNMUTESPDIFOUT", -1, 0, 0);
        pnx0106_epics_issue_command("ENABLENEWSAMPLEISR", -1, 0, 0);

        break;

    case PNX0106_EPICS_OUTPUT_SPDIF|PNX0106_EPICS_OUTPUT_DAC:
	printk("Unmute DAC and SPdif\n");
        pnx0106_epics_issue_command("UNMUTEDACOUT", -1, 0, 0);
        pnx0106_epics_issue_command("UNMUTESPDIFOUT", -1, 0, 0);
        pnx0106_epics_issue_command("ENABLENEWSAMPLEISR", -1, 0, 0);

        break;

    default:
	return -EINVAL;
    }

    {
        // Print out value from Epics
        int i = 5000;

        while (i--)
            ;

        printk("EPICS PP DAC Mute Info = %x\n",debug1[0]);
        printk("EPICS PP SPDIF Mute Info = %x\n",debug2[0]);

    }

    return 0;
}

static int
pnx0106_epics_ioctl_readmem(struct pnx0106_epics_state *state, unsigned long arg)
{
    static struct pnx0106_epics_memop op;
    int i;

    if (copy_from_user(&op, (void *)arg, sizeof op - sizeof op.words))
	return -EFAULT;

    if (op.count < 1 || op.count > sizeof op.words / sizeof op.words[0])
	return -EINVAL;

    switch (op.space)
    {
    case PNX0106_EPICS_MEMSPACE_PRAM:
    	if (op.offset < 0 || op.offset >= 0x1000 || op.offset + op.count > 0x1000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_pmemory_ram[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_PCACHE:
    	if (op.offset < 0x1000 || op.offset >= 0x10000 || op.offset + op.count > 0x10000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_pmemory_cache[op.offset + i - 0x1000];

	break;
    case PNX0106_EPICS_MEMSPACE_XMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_xmemory_ram[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_VIRTXMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_virtual_xmemory_ram[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_CTLREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_control_reg[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_DIOREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_dio_reg[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_DMAREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_dma_access_reg[op.offset + i];

	break;
    case PNX0106_EPICS_MEMSPACE_YMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    op.words[i] = e7b_epics_regs->e7b_ymemory_ram[op.offset + i];

	break;
    default:
	return -EINVAL;
    }

    // copy the data back
    if (copy_to_user(((struct pnx0106_epics_memop *)arg)->words, op.words, op.count * sizeof op.words[0]))
	return -EFAULT;

    return 0;
}

static int
pnx0106_epics_ioctl_writemem(struct pnx0106_epics_state *state, unsigned long arg)
{
    static struct pnx0106_epics_memop op;
    int i;

    if (copy_from_user(&op, (void *)arg, sizeof op - sizeof op.words))
	return -EFAULT;

    if (op.count < 1 || op.count > sizeof op.words / sizeof op.words[0])
	return -EINVAL;

    // copy the data in
    if (copy_from_user(op.words, ((struct pnx0106_epics_memop *)arg)->words, op.count * sizeof op.words[0]))
	return -EFAULT;

    switch (op.space)
    {
    case PNX0106_EPICS_MEMSPACE_PRAM:
    	if (op.offset < 0 || op.offset >= 0x1000 || op.offset + op.count > 0x1000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_pmemory_ram[op.offset + i] = op.words[i];

	break;
#if 0
    case PNX0106_EPICS_MEMSPACE_PCACHE:
    	if (op.offset < 0x1000 || op.offset >= 0x10000 || op.offset + op.count > 0x10000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_pmemory_cache[op.offset + i - 0x1000] = op.words[i];

	break;
#endif
    case PNX0106_EPICS_MEMSPACE_XMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_xmemory_ram[op.offset + i] = op.words[i];

	break;
    case PNX0106_EPICS_MEMSPACE_VIRTXMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_virtual_xmemory_ram[op.offset + i] = op.words[i];

	break;
    case PNX0106_EPICS_MEMSPACE_CTLREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_control_reg[op.offset + i] = op.words[i];

	break;
    case PNX0106_EPICS_MEMSPACE_DIOREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_dio_reg[op.offset + i] = op.words[i];

	break;
#if 0
    case PNX0106_EPICS_MEMSPACE_DMAREG:
    	if (op.offset < 0 || op.offset >= 0x40 || op.offset + op.count > 0x40)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_dma_access_reg[op.offset + i] = op.words[i];

	break;
#endif
    case PNX0106_EPICS_MEMSPACE_YMEM:
    	if (op.offset < 0 || op.offset >= 0x8000 || op.offset + op.count > 0x8000)
	    return -EINVAL;

	for (i = 0; i < op.count; i++)
	    e7b_epics_regs->e7b_ymemory_ram[op.offset + i] = op.words[i];

	break;
    default:
	return -EINVAL;
    }

    return 0;
}

static int
pnx0106_epics_ioctl_getequalizerbandsgain(struct pnx0106_epics_state *state, unsigned long arg)
{
    struct pnx0106_epics_equalizer_bands_gain bands = state->equalizer_bands;

    // Copy value to user
    if (copy_to_user((void *)arg, &bands, sizeof bands))
	return -EFAULT;

    return 0;
}


static int
pnx0106_epics_ioctl_setequalizerbandsgain(struct pnx0106_epics_state *state, unsigned long arg)
{
    int i, local_bands[5];
    int currentBand;
    struct pnx0106_epics_equalizer_bands_gain bands;

    static const char *band_label[5] =
    {
        "EQ5BAND1GAINADDR_1",
        "EQ5BAND2GAINADDR_2",
        "EQ5BAND3GAINADDR_3",
        "EQ5BAND4GAINADDR_4",
        "EQ5BAND5GAINADDR_5"
    };

    if (copy_from_user (&bands, (void *)arg, sizeof bands))
        return -EFAULT;

    local_bands[0] = bands.band1_gain;
    local_bands[1] = bands.band2_gain;
    local_bands[2] = bands.band3_gain;
    local_bands[3] = bands.band4_gain;
    local_bands[4] = bands.band5_gain;

    // Check values
    for (i = 0; i < 5; i++)
    {
        switch (local_bands[i])
        {
            case EPX_EQ5GAIN_MIN15DB:
            case EPX_EQ5GAIN_MIN12DB:
            case EPX_EQ5GAIN_MIN9DB:
            case EPX_EQ5GAIN_MIN6DB:
            case EPX_EQ5GAIN_MIN3DB:
            case EPX_EQ5GAIN_0DB:
            case EPX_EQ5GAIN_PLUS3DB:
            case EPX_EQ5GAIN_PLUS6DB:
            case EPX_EQ5GAIN_PLUS9DB:
            case EPX_EQ5GAIN_PLUS12DB:
            case EPX_EQ5GAIN_PLUS15DB:
                break;

            default:
                return -EINVAL;
        }
    }

    // save in state structure  - prob don't need this
    state->equalizer_bands = bands;

    // Tell Epics what to do
    // Set all bands ..
    pnx0106_epics_printk ("Set Eq bands %d %d %d %d %d\n", bands.band1_gain, bands.band2_gain, bands.band3_gain, bands.band4_gain, bands.band5_gain);

    if ((bands.band1_gain==EPX_EQ5GAIN_0DB)&&
        (bands.band2_gain==EPX_EQ5GAIN_0DB)&&
        (bands.band3_gain==EPX_EQ5GAIN_0DB)&&
        (bands.band4_gain==EPX_EQ5GAIN_0DB)&&
        (bands.band5_gain==EPX_EQ5GAIN_0DB))
    {
        pnx0106_epics_issue_command ("SETMODEEQ5_OFF", -1, 0, 0);
    }
    else
    {
        pnx0106_epics_issue_command ("SETMODEEQ5_ON", -1, 0, 0);
    }

    for (i = 0; i < 5; i++) {
        currentBand = pnx0106_epics_pp_getlabel (band_label[i], -1);
        if (currentBand >= 0)
            *(pnx0106_epics_ctr2arm (currentBand)) = local_bands[i];
        else
            printk ("%s: label %s not found !!\n", __FUNCTION__, band_label[i]);
    }

    // Issue command
    pnx0106_epics_issue_command ("SETEQ5ALLBANDSGAIN", -1, 0, 0);
    return 0;
}

static int
pnx0106_epics_ioctl_getfs(struct pnx0106_epics_state *state, unsigned long arg)
{
    int frequency = state->sfrequency;

    // Copy value to user
    if (copy_to_user((void *)arg, &frequency, sizeof frequency))
	return -EFAULT;

    return 0;
}

pnx0106_epics_ioctl_setfs(struct pnx0106_epics_state *state, unsigned long arg)
{
    int frequency;

    if (copy_from_user(&frequency, (void *)arg, sizeof frequency))
	return -EFAULT;

    // Check value
    switch (frequency)
    {
    case PNX0106_EPICS_FS_32000:
    case PNX0106_EPICS_FS_44100:
    case PNX0106_EPICS_FS_48000:
	break;
    default:
	return -EINVAL;
    }
    state->sfrequency = frequency;


    // Tell Epics what to do
    switch (frequency)
    {
    case PNX0106_EPICS_FS_32000:
	// Set FS=32000
    printk("Set FS=32000\n");
	pnx0106_epics_issue_command("SET_32_KHZ", -1, 0, 0);
        break;
    case PNX0106_EPICS_FS_44100:
        // Set FS=44100
	printk("Set FS=44100\n");
    pnx0106_epics_issue_command("SET_44_KHZ", -1, 0, 0);
	break;
    case PNX0106_EPICS_FS_48000:
        // Set FS=48000
    printk("Set FS=48000\n");
    pnx0106_epics_issue_command("SET_48_KHZ", -1, 0, 0);
	break;
    }

    return 0;

}

static int
pnx0106_epics_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    printk(DEVICE_NAME ": ioctl: cmd 0x%08x\n", cmd);

    switch (cmd)
    {
    case PNX0106_EPICS_FW_SAVE:
	return pnx0106_epics_ioctl_fw_save(state, arg);
    case PNX0106_EPICS_FW_START:
	return pnx0106_epics_ioctl_fw_start(state, arg);
    case PNX0106_EPICS_FW_CURR:
	return pnx0106_epics_ioctl_fw_curr(state, arg);
    case PNX0106_EPICS_FW_NUM:
	return pnx0106_epics_ioctl_fw_num(state, arg);
    case PNX0106_EPICS_FW_INFO:
	return pnx0106_epics_ioctl_fw_info(state, arg);
    case PNX0106_EPICS_FW_NUMLABELS:
	return pnx0106_epics_ioctl_fw_numlabels(state, arg);
    case PNX0106_EPICS_FW_GETLABEL:
	return pnx0106_epics_ioctl_fw_getlabel(state, arg);
    case PNX0106_EPICS_GETUPSCALE:
	return pnx0106_epics_ioctl_getupscale(state, arg);
    case PNX0106_EPICS_SETUPSCALE:
	return pnx0106_epics_ioctl_setupscale(state, arg);
    case PNX0106_EPICS_GETOUTENABLE:
	return pnx0106_epics_ioctl_getoutenable(state, arg);
    case PNX0106_EPICS_SETOUTENABLE:
	return pnx0106_epics_ioctl_setoutenable(state, arg);
    case PNX0106_EPICS_READMEM:
	return pnx0106_epics_ioctl_readmem(state, arg);
    case PNX0106_EPICS_WRITEMEM:
	return pnx0106_epics_ioctl_writemem(state, arg);
    case PNX0106_EPICS_SET_EQUALIZER_BANDS_GAIN:
	return pnx0106_epics_ioctl_setequalizerbandsgain(state, arg);
    case PNX0106_EPICS_GET_EQUALIZER_BANDS_GAIN:
	return pnx0106_epics_ioctl_getequalizerbandsgain(state, arg);
    case PNX0106_EPICS_SET_FS:
	return pnx0106_epics_ioctl_setfs(state, arg);
    case PNX0106_EPICS_GET_FS:
	return pnx0106_epics_ioctl_getfs(state, arg);

    }

    printk(DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
    return -EINVAL;
}

static int
pnx0106_epics_mmap(struct file *file, struct vm_area_struct *vmarea)
{
    printk(DEVICE_NAME ": mmap\n");
    return -EINVAL;
}

static int
pnx0106_epics_open(struct inode *inode, struct file *file)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    printk("pnx0106_epics_open: start\n");

    spin_lock (&state->lock);

    if (state->count) {
	printk("%s: device busy count %d\n", DEVICE_NAME, state->count);
	spin_unlock (&state->lock);
	return -EBUSY;
    }

    state->count++;
    spin_unlock (&state->lock);
    MOD_INC_USE_COUNT;
    return 0;
}

static int
pnx0106_epics_release(struct inode *inode, struct file *file)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    if (state->newfw)
    {
	int version;

	if (pnx0106_epics_fw_verify(state->newfw, &version) < 0 || version < 3)
	{
	    printk(DEVICE_NAME ": Freeing unsaved firmware image\n");
	    pnx0106_epics_fw_free(state->newfw);
	}
	else
	    pnx0106_epics_fw_save(state, state->newfw);

	state->newfw = NULL;
    }

    printk("pnx0106_epics_release: goodbye !!\n");

    state->count--;
    MOD_DEC_USE_COUNT;

    return 0;
}

int
pnx0106_epics_decoder_claim(int codecid)
{
    int prevcodecid = 0;
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    spin_lock(&state->lock);
    prevcodecid = state->codecid;

    if (prevcodecid == 0)
	state->codecid = codecid;

    spin_unlock(&state->lock);
    return prevcodecid ? -EBUSY : 0;
}

void
pnx0106_epics_decoder_release(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    spin_lock(&state->lock);
    state->codecid = 0;
    spin_unlock(&state->lock);
}

int
pnx0106_epics_request_irq(int (*isr)(void* data), void *arg)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    int result;
#if 0
    unsigned long flags;
#endif

    if (state->isr != NULL)
	return -EBUSY;

    state->isr = isr;
    state->israrg = arg;

    if ((result = request_irq(IRQ_E7B2ARM_IRQ2, pnx0106_epics_isr, 0, DEVICE_NAME, NULL)) != 0) {
	pnx0106_epics_printk("%s: could not register interrupt %d\n", DEVICE_NAME, IRQ_E7B2ARM_IRQ2);
	state->isr = NULL;
	return result;
    }

#if 0
    /* disable interrupts */
    spin_lock_irqsave (&state->lock, flags);

    /* clear any pending interrupts */

    /* enable interrupts */
    spin_unlock_irqrestore (&state->lock, flags);
#endif

    return 0;
}

void
pnx0106_epics_free_irq(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
#if 0
    unsigned long flags;

    spin_lock_irqsave (&state->lock, flags);
    // Disable the hardware interrupt
    spin_unlock_irqrestore (&state->lock, flags);
#endif

    free_irq (IRQ_E7B2ARM_IRQ2, NULL);
    state->isr = NULL;
}

static struct file_operations pnx0106_epics_fops =
{
   .llseek            = pnx0106_epics_llseek,
//   .read              = pnx0106_epics_read,
   .write             = pnx0106_epics_write,
//   .readdir           = pnx0106_epics_readdir,
   .poll              = pnx0106_epics_poll,
   .ioctl             = pnx0106_epics_ioctl,
   .mmap              = pnx0106_epics_mmap,
   .open              = pnx0106_epics_open,
//   .flush             = pnx0106_epics_flush,
   .release           = pnx0106_epics_release,
//   .fsync             = pnx0106_epics_fsync,
//   .fasync            = pnx0106_epics_fasync,
//   .lock              = pnx0106_epics_lock,
//   .readv             = pnx0106_epics_readv,
//   .writev            = pnx0106_epics_writev,
//   .sendpage          = pnx0106_epics_sendpage,
//   .get_unmapped_area = pnx0106_epics_get_unmapped_area,
   .owner             = THIS_MODULE,
};

extern struct file_operations pnx0106_epics_mp3_fops;
extern struct file_operations pnx0106_epics_mp3b_fops;
extern struct file_operations pnx0106_epics_aac_fops;
extern struct file_operations pnx0106_epics_aacb_fops;
extern struct file_operations pnx0106_epics_wma9_fops;
extern struct file_operations pnx0106_epics_wma9bl_fops;
extern struct file_operations pnx0106_epics_wma9b_fops;

struct pnx0106_codec
{
    char *name;
    struct miscdevice mdev;
};

struct pnx0106_codec pnx0106_codecs[] =
{
    { "epics", { PNX0106_EPICS_MINOR, PNX0106_EPICS_DEVNAME, &pnx0106_epics_fops }, },
#ifdef CONFIG_PNX0106_EPICS_MP3
    { "mp3", { PNX0106_EPICS_MP3_MINOR, PNX0106_EPICS_MP3_DEVNAME, &pnx0106_epics_mp3_fops }, },
    { "mp3b", { PNX0106_EPICS_MP3B_MINOR, PNX0106_EPICS_MP3B_DEVNAME, &pnx0106_epics_mp3b_fops }, },
#endif
#ifdef CONFIG_PNX0106_EPICS_AAC
    { "aac", { PNX0106_EPICS_AAC_MINOR, PNX0106_EPICS_AAC_DEVNAME, &pnx0106_epics_aac_fops }, },
    { "aacb", { PNX0106_EPICS_AACB_MINOR, PNX0106_EPICS_AACB_DEVNAME, &pnx0106_epics_aacb_fops }, },
#endif
#ifdef CONFIG_PNX0106_EPICS_WMA
    { "wma9", { PNX0106_EPICS_WMA9_MINOR, PNX0106_EPICS_WMA9_DEVNAME, &pnx0106_epics_wma9_fops }, },
    { "wma9bl", { PNX0106_EPICS_WMA9BL_MINOR, PNX0106_EPICS_WMA9BL_DEVNAME, &pnx0106_epics_wma9bl_fops }, },
    { "wma9b", { PNX0106_EPICS_WMA9B_MINOR, PNX0106_EPICS_WMA9B_DEVNAME, &pnx0106_epics_wma9b_fops }, },
#endif
};

static int __init pnx0106_epics_init(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    int result;
    int i;

    state->printklock = SPIN_LOCK_UNLOCKED;
    printk("pnx0106_epics_init\n");

    init_waitqueue_head (&state->wq);
    state->lock = SPIN_LOCK_UNLOCKED,
    state->curfw = NULL;
    state->newfw = NULL;
    state->upscale = 1;
    state->enables = 0;
    state->shared_block0 = NULL;
    state->sharedsize_block0 = 0;
    state->sharedphys_block0 = 0;
    state->shared_block1 = NULL;
    state->sharedsize_block1 = 0;
    state->sharedphys_block1 = 0;
    state->xmemBackup = 0;

    // Tricky to request this info from EPICS... So hardcode it for the moment
    // will need to change this if f/w changes value
    state->equalizer_bands.band1_gain=EPX_EQ5GAIN_0DB;
    state->equalizer_bands.band2_gain=EPX_EQ5GAIN_0DB;
    state->equalizer_bands.band3_gain=EPX_EQ5GAIN_0DB;
    state->equalizer_bands.band4_gain=EPX_EQ5GAIN_0DB;
    state->equalizer_bands.band5_gain=EPX_EQ5GAIN_0DB;
    state->sfrequency = PNX0106_EPICS_FS_44100;

    INIT_LIST_HEAD(&state->fw_list);

    // setup clocks?
    // ....

    for (i = 0; i < sizeof pnx0106_codecs / sizeof pnx0106_codecs[0]; i++)
    {


    pnx0106_epics_printk("register dev %s\n", pnx0106_codecs[i].name);
	// register minor numbers
	result = misc_register (&pnx0106_codecs[i].mdev);

    if (result < 0) {
	    pnx0106_epics_printk(DEVICE_NAME ": Can't register misc minor no %d for %s\n", pnx0106_codecs[i].mdev.minor, pnx0106_codecs[i].name);

	    while (--i >= 0)
		misc_deregister(&pnx0106_codecs[i].mdev);

	    return result;
	}
    }

    if (er_add_raw_input_event(IRQ_E7B2ARM_IRQ2, EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ2, EVENT_TYPE_RISING_EDGE_TRIGGERED) != 0)
    {
	pnx0106_epics_printk("%s: could not route interrupt %d --> %d\n", DEVICE_NAME, EVENT_ID_MELODY_ADSS_E7B2ARM_IRQ1, IRQ_E7B2ARM_IRQ2);
	return -EINVAL;
    }

    printk("pnx0106_epics_init: device registered\n");
    return 0;
}

static void __exit pnx0106_epics_exit(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    int i;
    struct list_head *entry;
    struct list_head *n;

    printk("pnx0106_epics_exit\n");

    // release all misc devices
    for (i = 0; i < sizeof pnx0106_codecs / sizeof pnx0106_codecs[0]; i++)
	misc_deregister(&pnx0106_codecs[i].mdev);

    // remove all firmware images
    list_for_each_safe(entry, n, &state->fw_list)
    {
	struct pnx0106_epics_fw *xfw;

	xfw = list_entry(entry, struct pnx0106_epics_fw, node);
	pnx0106_epics_fw_free(xfw);
	list_del(&xfw->node);
    }

    if (state->sharedsize_block0)
	consistent_free(state->shared_block0, state->sharedsize_block0, state->sharedphys_block0);
    if (state->sharedsize_block1)
	consistent_free(state->shared_block1, state->sharedsize_block1, state->sharedphys_block1);

    if(state->xmemBackup)
    {
        kfree(state->xmemBackup);
        state->xmemBackup=0;
    }
//	dma_free_coherent(NULL, state->sharedsize, state->shared, state->sharedphys);
}

#define MAX_EPICS_ALLOC		(16*1024)

void *
pnx0106_epics_alloc_coherent(void *p, int size, dma_addr_t *phys, int flags)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    if(flags)
    {
        if (state->sharedsize_block1 == 0)
        {
    //	state->shared = dma_alloc_coherent(NULL, MAX_EPICS_ALLOC, &state->sharedphys, 0);
	    state->shared_block1 = consistent_alloc(0, MAX_EPICS_ALLOC, &state->sharedphys_block1);

	    if (state->shared_block1 == NULL)
	        return NULL;

	    state->sharedsize_block1 = MAX_EPICS_ALLOC;
        }

        if (state->sharedsize_block1 < size)
	    return NULL;

        *phys = state->sharedphys_block1;
        return state->shared_block1;
    }
    else
    {

        if (state->sharedsize_block0 == 0)
        {
    //	state->shared = dma_alloc_coherent(NULL, MAX_EPICS_ALLOC, &state->sharedphys, 0);
	    state->shared_block0 = consistent_alloc(0, MAX_EPICS_ALLOC, &state->sharedphys_block0);

	    if (state->shared_block0 == NULL)
	        return NULL;

	    state->sharedsize_block0 = MAX_EPICS_ALLOC;
        }

        if (state->sharedsize_block0 < size)
	    return NULL;

        *phys = state->sharedphys_block0;
        return state->shared_block0;
    }
}

void
pnx0106_epics_free_coherent(void *p, int size, void *ptr, dma_addr_t phys)
{
    // Never free the buffer so that we don't fragment it.
    // other Epics drivers can reuse it so it isn't a complete waste.
}

int
pnx0106_epics_alloc_backup_buffer(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    printk("call epics_alloc_backup_buffer\n");
    if(state->xmemBackup==0)
    {
        // Initialize memory for swapping out driver
        state->xmemBackup = kmalloc(sizeof (e7b_epics_regs->e7b_xmemory_ram), GFP_KERNEL);

        if (state->xmemBackup == NULL)
        {
            printk("kmalloc returned 0 \n");
	        return -ENOMEM;
        }

        printk("kmalloc allocated %d \n",sizeof (e7b_epics_regs->e7b_xmemory_ram) );
    }
    return 0;
}

void
pnx0106_epics_free_backup_buffer(void)
{
    // Free at exit to avoid fragmentation

    //if(state->xmemBackup)
    //{
    //    kfree(state->xmemBackup);
    //    state->xmemBackup=0;
    //}
}

void
pnx0106_epics_copy_xmem2sdram(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    memcpy(state->xmemBackup, (void*)&e7b_epics_regs->e7b_xmemory_ram[0x1580], sizeof (e7b_epics_regs->e7b_xmemory_ram)-0x1580);
}

void
pnx0106_epics_copy_sdram2xmem(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;

    memcpy((void*)&e7b_epics_regs->e7b_xmemory_ram[0x1580], state->xmemBackup,  sizeof (e7b_epics_regs->e7b_xmemory_ram)-0x1580);
}


void
pnx0106_epics_zero_decoder_xmem(void)
{
    memset((char*)&e7b_epics_regs->e7b_xmemory_ram[0x1580], 0, sizeof (e7b_epics_regs->e7b_xmemory_ram)-0x1580);

}

/*************************************************************************
 * Function            : pnx0106_epics_set_exit_decoder
 * Description         : Sets exit decoder flag set by the
 *                       foreground task when it wishes to take control of the EPICS)
 *************************************************************************/
void pnx0106_epics_set_exit_decoder(int halt)
{
    pnx0106_epics_printk("Set Epics ExitDecoder flag = %d\n",halt?1:0);
    if(halt)
    {
        *pnx0106_epics_xram2arm(0x5655) = 1;
    }
    else
    {
        *pnx0106_epics_xram2arm(0x5655) = 0;
    }
}

/*************************************************************************
 * Function            : pnx0106_epics_get_exit_decoder
 * Description         : Gets exit decoder flag (i.e. the flag that the
 *                       foreground task sets when it wishes to take control of the EPICS)
 *************************************************************************/
int pnx0106_epics_get_exit_decoder(void)
{
    if( *pnx0106_epics_xram2arm(0x5655))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*************************************************************************
 * Function            : pnx0106_epics_decoder_exited
 * Description         : Checks to see if the Decoder has exited. This flag is
 *                       set by the EPICS decoder code on exit and cleared
 *                       on entrance.
 *************************************************************************/
int pnx0106_epics_decoder_exited(void)
{
    pnx0106_epics_printk("pnx0106_epics_decoder_exited\n");
    if( *pnx0106_epics_xram2arm(0x5656))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*************************************************************************
 * Function            : pnx0106_epics_need_to_resume_stream
 * Description         : Checks if streaming was interrupted and needs to be resumed. This flag is
 *                       set by the EPICS decoder code on exit from stream loop cleared
 *                       on entrance.
 *************************************************************************/
int pnx0106_epics_need_to_resume_stream(void)
{
    pnx0106_epics_printk("pnx0106_epics_need_to_resume_stream\n");
    if( *pnx0106_epics_xram2arm(0x5657))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void pnx0106_epics_change_endian (u8* buffer, u32 size)
{
    s32 *pointer;
    s32 *limit;
    s32 t1, t2;

    if (((u32)buffer % 4) || (size % 4))
        ERRMSG("buffer for endian swap not 32bit aligned! (%p %d)\n", buffer, size);

    pointer = (s32 *) buffer;
    limit = (s32 *) (buffer + size);

    while (pointer < limit) {
        asm ("ldr %1, [%0];\n\t"
             "eor %2, %1, %1, ror #16;\n\t"
             "bic %2, %2, #0x00FF0000;\n\t"
             "mov %1, %1, ror #8;\n\t"
             "eor %1, %1, %2, lsr #8;\n\t"
             "str %1, [%0], #4;\n\t"
             : "=r" (pointer), "=r" (t1), "=r" (t2)
             : "0" (pointer)
             : "memory");
    }
}

int
pnx0106_epics_select_firmware(int fwid)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    struct pnx0106_epics_fw *fw;

    fw = pnx0106_epics_fw_find_id(state, fwid);

    if (fwid < 0)
	return -EINVAL;

    state->curfw = fw;
    return 0;
}

int
pnx0106_epics_reload_firmware(void)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    struct pnx0106_epics_fw *fw = state->curfw;

    if (fw == NULL)
	return -EINVAL;

    printk(DEVICE_NAME ": fw_load\n");
    return pnx0106_epics_fw_load(state, fw);
}

interval_t timeSleep;
interval_t timeSleep10;

void
pnx0106_epics_sleep(u32 millisecs)
{
    if (millisecs)
    {
        unsigned long timeout;
        INTERVAL_START(timeSleep10);
	set_current_state(TASK_UNINTERRUPTIBLE);
	timeout = schedule_timeout(0);
        INTERVAL_STOP(timeSleep10);
    }
    else
    {
        INTERVAL_START(timeSleep);
	schedule();
        INTERVAL_STOP(timeSleep);
    }
}

static void
e7b_wait4ready(void)
{
    int max = 100;
    volatile u32 *ready = pnx0106_epics_ctr2arm(pnx0106_epics_pp_getlabel("DSP_DOWNLOAD_READY", EPX_DSP_DOWNLOAD_READY));

    while (max-- > 0)
    {
	if (*ready == 1 || *ready == 2)
	    break;

	schedule();
    }

    if (max == 0)
	printk("pnx0106_epics_wait4ready: invalid dsp state\n");
}

//void
//pnx0106_epics_interrupt_req(void)
//{
//    e7b_audioss_regs->audioss_creg_e7b_interrupt_req = 1;
//}

void
pnx0106_epics_set_ready(void)
{
    e7b_wait4ready();

    /* start Epics */
    *(pnx0106_epics_ctr2arm(pnx0106_epics_pp_getlabel("DSP_DOWNLOAD_READY", EPX_DSP_DOWNLOAD_READY))) = 1;
    pnx0106_epics_interrupt_req();
}

#undef printk

void
pnx0106_epics_printk(char *fmt, ...)
{
    struct pnx0106_epics_state *state = &pnx0106_epics_state;
    static char buf[1024];
    va_list args;
    static volatile int n = 0;

    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    spin_lock(&state->printklock);
    buf[strlen(buf) - 1] = 0;
    printk("%s === %d\n", buf, n++);
    spin_unlock(&state->printklock);
}

unsigned long
llsqrt(unsigned long long x)
{
    unsigned long y = 1ULL << 31;
    unsigned long v = 0;

    while (y)
    {
	unsigned long t = v + y;

	if ((unsigned long long)t * t <= x)
	    v  = t;

	y >>= 1;
    }

    return v;
}

module_init(pnx0106_epics_init);
module_exit(pnx0106_epics_exit);
