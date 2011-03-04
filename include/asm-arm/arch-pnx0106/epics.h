#ifndef _EPICS_H
#define _EPICS_H

/**********************************************************************
 *
 *	Linux general kernel headers
 *
 */
#include <linux/kernel.h>
#include <asm/types.h>
/* #define PNX0105_1394	// vhal files expect this for PNX0106 ! (added to Makefile) */
//#include <asm/arch/vhal/e7b_ahb_if.h>
//#include <asm/arch/vhal/audioss_creg.h>
//#include "vhe7b_pcache_config.h" /* not available in linux vhal */


#define PNX0106_EPICS_MINOR		232
#define PNX0106_EPICS_MP3_MINOR		(PNX0106_EPICS_MINOR+1)
#define PNX0106_EPICS_AAC_MINOR		(PNX0106_EPICS_MINOR+2)
#define PNX0106_EPICS_WMA9_MINOR	(PNX0106_EPICS_MINOR+3)
#define PNX0106_EPICS_WMA9BL_MINOR	(PNX0106_EPICS_MINOR+4)
#define PNX0106_EPICS_WMA9B_MINOR       (PNX0106_EPICS_MINOR+5)
#define PNX0106_EPICS_MP3B_MINOR        (PNX0106_EPICS_MINOR+6)
#define PNX0106_EPICS_AACB_MINOR        (PNX0106_EPICS_MINOR+7)
#define PNX0106_EPICS_DEVNAME		"pnx0106_epics"
#define PNX0106_EPICS_MP3_DEVNAME	"pnx0106_mp3"
#define PNX0106_EPICS_MP3B_DEVNAME	"pnx0106_mp3b"
#define PNX0106_EPICS_AAC_DEVNAME	"pnx0106_aac"
#define PNX0106_EPICS_AACB_DEVNAME	"pnx0106_aacb"
#define PNX0106_EPICS_WMA9_DEVNAME	"pnx0106_wma9"
#define PNX0106_EPICS_WMA9BL_DEVNAME	"pnx0106_wma9bl"
#define PNX0106_EPICS_WMA9B_DEVNAME	"pnx0106_wma9b"

#define PNX0106_EPICS_PP_FWID		1
#define PNX0106_EPICS_MP3_FWID		3
#define PNX0106_EPICS_AAC_FWID		4
#define PNX0106_EPICS_WMA9_FWID		9
#define PNX0106_EPICS_WMA9BL_FWID		10

struct pnx0106_epics_fw_info
{
    int index;				// Image number 
    int fwid;				// ID of the image
    char fwname[8];			// Name of the codec
    int fwlen;				// Length in bytes
    int numchunks;			// Num 16KiB chunks used
    int version;			// Firmware image format version 
    char buildstr[16];			// Identifier indicating who/when built
    int header_size;			// size of the header section
    int labels_size;			// size of the labels section
    int xmem_size;			// size of the xmem section
    int ymem_size;			// size of the ymem section
    int pram_size;			// size of the pram section
    int pcache_size;			// size of the pcache section
    int xmem_word;			// starting word of xmem section
    int ymem_word;			// starting word of ymem section
    int pram_word;			// starting word of pram section
    int pcache_word;			// starting word of pcache section
    int labels_csum;			// checksum of the labels section
    int xmem_csum;			// checksum of the xmem section
    int ymem_csum;			// checksum of the ymem section
    int pram_csum;			// checksum of the pram section
    int pcache_csum;			// checksum of the pcache section
};

struct pnx0106_epics_fw_label
{
    int fwindex;
    int labelindex;
    unsigned int word;
    char label[32];
};



#define PNX0106_EPICS_OUTPUT_DAC	0x0001
#define PNX0106_EPICS_OUTPUT_SPDIF	0x0002

#define EPX_EQ5GAIN_MIN15DB                     0x00000000
#define EPX_EQ5GAIN_MIN12DB                     0x00000001
#define EPX_EQ5GAIN_MIN9DB                      0x00000002
#define EPX_EQ5GAIN_MIN6DB                      0x00000003
#define EPX_EQ5GAIN_MIN3DB                      0x00000004
#define EPX_EQ5GAIN_0DB                         0x00000005
#define EPX_EQ5GAIN_PLUS3DB                     0x00000006
#define EPX_EQ5GAIN_PLUS6DB                     0x00000007
#define EPX_EQ5GAIN_PLUS9DB                     0x00000008
#define EPX_EQ5GAIN_PLUS12DB                    0x00000009
#define EPX_EQ5GAIN_PLUS15DB                    0x0000000A



struct pnx0106_epics_memop
{
    int space;
    int offset;
    int count;
    unsigned int words[256];
};

struct pnx0106_epics_equalizer_bands_gain
{
    int band1_gain;
    int band2_gain;
    int band3_gain;
    int band4_gain;
    int band5_gain;
};

#define PNX0106_EPICS_FS_32000 0
#define PNX0106_EPICS_FS_44100 1
#define PNX0106_EPICS_FS_48000 2


#define PNX0106_EPICS_MEMSPACE_PRAM	1
#define PNX0106_EPICS_MEMSPACE_PCACHE	2
#define PNX0106_EPICS_MEMSPACE_XMEM	3
#define PNX0106_EPICS_MEMSPACE_YMEM	4
#define PNX0106_EPICS_MEMSPACE_CTLREG	5
#define PNX0106_EPICS_MEMSPACE_DIOREG	6
#define PNX0106_EPICS_MEMSPACE_DMAREG	7
#define PNX0106_EPICS_MEMSPACE_VIRTXMEM	8

#define PNX0106_EPICS_FW_SAVE		_IOW ('E',  1, int)
#define PNX0106_EPICS_FW_START		_IOW ('E',  2, int)
#define PNX0106_EPICS_FW_CURR		_IOR ('E',  3, int)
#define PNX0106_EPICS_FW_NUM		_IOR ('E',  4, int)
#define PNX0106_EPICS_FW_INFO		_IOWR('E',  5, struct pnx0106_epics_fw_info)
#define PNX0106_EPICS_FW_NUMLABELS	_IOWR('E',  6, int)
#define PNX0106_EPICS_FW_GETLABEL	_IOWR('E',  7, struct pnx0106_epics_fw_label)
#define PNX0106_EPICS_GETUPSCALE	_IOR ('E',  8, int)
#define PNX0106_EPICS_SETUPSCALE	_IOW ('E',  9, int)
#define PNX0106_EPICS_GETOUTENABLE	_IOR ('E', 10, int)
#define PNX0106_EPICS_SETOUTENABLE	_IOW ('E', 11, int)
#define PNX0106_EPICS_READMEM		_IOWR('E', 12, struct pnx0106_epics_memop)
#define PNX0106_EPICS_WRITEMEM		_IOW ('E', 13, struct pnx0106_epics_memop)
#define PNX0106_EPICS_SET_EQUALIZER_BANDS_GAIN		_IOW ('E', 14, struct pnx0106_epics_equalizer_bands_gain)
#define PNX0106_EPICS_GET_EQUALIZER_BANDS_GAIN		_IOR ('E', 15, struct pnx0106_epics_equalizer_bands_gain)
#define PNX0106_EPICS_SET_FS        _IOW ('E', 16, int)
#define PNX0106_EPICS_GET_FS        _IOR ('E', 17, int)


/**********************************************************************
 *
 *		Error handling
 *
 */
#define ERRMSG(string, args...)  printk(KERN_ERR"epics error: " string, ##args)
#define WARN(string, args...)    printk(KERN_WARNING"epics warning: " string, ##args)
#define INFO(string, args...)    printk(KERN_INFO"epics: " string, ##args)
#define MSG(string, args...)     printk(string, ##args)

#ifdef DEBUG
extern int trace_level;
#define TRACE(lvl,string, args...)   do { if (trace_level>=lvl) printk(KERN_INFO"epics: " string, ##args); } while (0)
#else
#define TRACE(lvl,string, args...)	 do { } while (0)
#endif



#ifdef __KERNEL__


/**********************************************************************
 *
 *		Linux mappings to registers and epics-to-arm conversions
 *
 */


#define PNX0106_EPICS_CTR_START_ADDRESS 	0xffc0

//extern vhe7b_ahb_ifregs	*e7b_epics_regs;
//extern vhaudioss_cregs	*e7b_audioss_regs;
//extern pEPICS_PCACHE_CONFIG_REGS e7b_pcache_cfg;

typedef volatile struct {
        u32 _unused0[0x0000];           /* xrom not used */
        u32 e7b_xmemory_ram[0x8000];	/* 8k xram starting at 0 */
        u32 _unused1[0x7fc0];           /* unused part of xmem (total 10k HEX) */
	u32 e7b_control_reg[0x40];      /* ctrl-reg top part of xmem */
        u32 _unused2[0x0000];           /* yrom not used */
        u32 e7b_ymemory_ram[0x8000];    /* 8k yram starting at 0 */
        u32 _unused3[0x8000];           /* unused part of ymem (total 10k HEX) */
        u32 e7b_pmemory_ram[0x1000];    /* 1k pram starting at 0 */
        u32 e7b_pmemory_cache[0xf000];  /* Fk cache above pram */
        u32 e7b_virtual_xmemory_ram[0x8000];    /* virtual x-memory */
        u32 _unused6[0x7f00];           /* unused part of virual x-memory */
        u32 e7b_dio_reg[0x40];          /* dio-reg in pmem */
        u32 _unused7[0x80];             /* unused part of pmem */
        u32 e7b_dma_access_reg[0x40];   /* dma_access_reg top part of pmem */
} pnx0106_epics_ahb_ifregs;

#define e7b_epics_regs			((pnx0106_epics_ahb_ifregs *)IO_ADDRESS_E7B_BASE)

typedef volatile struct {
    u32 cache_reset_status;
    u32 general_cache_settings;
    u32 nr_of_line_reads;
    u32 reserved;
    u32 remap[15];
} pnx0106_epics_pcache_conf_ifregs;

#define e7b_pcache_conf_regs		((pnx0106_epics_pcache_conf_ifregs *)IO_ADDRESS_AUDIOSS_CACHE_BASE)

typedef volatile struct {

      u32  audioss_creg_iis_format_settings;
      u32  audioss_creg_audioss_mux_settings;
      u32  audioss_creg_spdif_in_status;
      u32  audioss_creg_spdif_in_irq_en;
      u32  audioss_creg_spdif_in_irq_status;
      u32  audioss_creg_spdif_in_irq_clear;
      u32  audioss_creg_sdac_ctrl_inti;
      u32  audioss_creg_sdac_ctrl_into;
      u32  audioss_creg_sdac_settings;
      u32  audioss_creg_sadc_ctrl_sdc;
      u32  audioss_creg_sadc_ctrl_adc;
      u32  audioss_creg_sadc_ctrl_deci;
      u32  audioss_creg_sadc_ctrl_deco;
      u32  audioss_creg_e7b_interrupt_req;
      u32  audioss_creg_e7b_ahb_settings;
      u32  audioss_creg_n_sof_counter;
      u32  audioss_creg_chst_l_31_0;
      u32  audioss_creg_chst_l_39_32;
      u32  audioss_creg_chst_r_31_0;
      u32  audioss_creg_chst_r_39_32;

} pnx0106_epics_audioss_cregs, *ppnx0106_epics_audioss_cregs;

#define e7b_audioss_regs		((pnx0106_epics_audioss_cregs *)IO_ADDRESS_AUDIOSS_BASE)

static inline volatile u32 *pnx0106_epics_xram2arm_2x24(u32 epics_xram_addr)  // return physical address on arm bus for an epics xram address in 2x24 mode
{
	/* make even so as to get to a u32 boundary */
	volatile u32 *addr = &(e7b_epics_regs->e7b_xmemory_ram[(epics_xram_addr & ~0x1) / 2]);
	TRACE(7, "xram 0x%x (2x24) -> arm %p\n", epics_xram_addr, addr);
	return addr;
}

static inline volatile u32 *pnx0106_epics_xram2arm(u32 epics_xram_addr)  // return physical address on arm bus for an epics xram address in 1:1 modes
{
	volatile u32 *addr = &(e7b_epics_regs->e7b_xmemory_ram[epics_xram_addr]);
	TRACE(7, "xram 0x%x -> arm %p\n", epics_xram_addr, addr);
	return addr;
}

static inline volatile u32 *pnx0106_epics_yram2arm(u32 epics_yram_addr)  // return physical address on arm bus for an epics yram address
{
	volatile u32 *addr = &(e7b_epics_regs->e7b_ymemory_ram[epics_yram_addr]);
	TRACE(7, "yram 0x%x -> arm %p\n", epics_yram_addr, addr);
	return addr;
}

static inline volatile u32 *pnx0106_epics_pram2arm(u32 epics_pram_addr)  // return physical address on arm bus for an epics pram address
{
	volatile u32 *addr = &(e7b_epics_regs->e7b_pmemory_ram[epics_pram_addr]);
	TRACE(7, "pram 0x%x -> arm %p\n", epics_pram_addr, addr);
	return addr;
}

static inline volatile u32 *pnx0106_epics_ctr2arm(u32 epics_ctr_addr)  // return physical address on arm bus for an epics control address
{
	volatile u32 *addr = &(e7b_epics_regs->e7b_control_reg[epics_ctr_addr - PNX0106_EPICS_CTR_START_ADDRESS]);
	TRACE(7, "ctr 0x%x -> arm %p\n", epics_ctr_addr, addr);
	return addr;
}




/**********************************************************************
 *
 * 		Small utility routines to set the transfer mode, interrupt req etc
 *
 */
/*	ARM (32 bits)			Epics 24 bits	  */
/*  ABCD            <-->	ABC				  */ 
static inline void pnx0106_epics_transfer_32_24 (void) /* loss depends on target memory. xram loss=high 8 bits on arm */
{
    e7b_audioss_regs->audioss_creg_e7b_ahb_settings = 0x0;   /* lossy and unsigned */
}

/*	ARM (32 bits)			Epics 24 bits	  */
/*  ABCD EFGH IJKL   -->	BCD GHA LEF IJK   */
static inline void pnx0106_epics_transfer_3x32_4x24 (void) /* 32 bits -> 24 bits Write only, xmem only */
{
    e7b_audioss_regs->audioss_creg_e7b_ahb_settings = 0x2;  
}

/*	ARM (32 bits)			Epics 24 bits	  */
/*  ABCD   			<-->    AB- CD-			  */
static inline void pnx0106_epics_transfer_32_2x16 (void) /* 32 bits <-> 2x16 bits R/W (also lozes least sig bits on epics) */
{
    e7b_audioss_regs->audioss_creg_e7b_ahb_settings = 0x4;
}

/*	Raise an epics interrupt */
static inline void pnx0106_epics_interrupt_req(void)
{
	e7b_audioss_regs->audioss_creg_e7b_interrupt_req=1;
}

/*	Get value of EPICS interrupt */
static inline int pnx0106_epics_get_interrupt_req(void)
{
	return e7b_audioss_regs->audioss_creg_e7b_interrupt_req;
}

void pnx0106_epics_set_ready(void);

void pnx0106_epics_sleep(u32 millisecs);
void pnx0106_epics_change_endian(u8* buffer, u32 size);


/**********************************************************************
 *
 * 		Firmware loading (codecs can use this for extra init settings) 
 *
 */
//#define E7B_FW_XRAM_LOADED  	0x0001
//#define E7B_FW_YRAM_LOADED  	0x0002
//#define E7B_FW_PCACHE_LOADED  	0x0004
//#define E7B_FW_PRAM_LOADED  	0x0008

//int e7b_request_firmware(const char* name); /*	Request firmware and download it in memory */
//int e7b_select_firmware(int fwid);  /* select firmware image to load */
//int e7b_reload_firmware(void);  /* reload the previous firmware */

int pnx0106_epics_select_firmware(int fwid);  /* select firmware image to load */
int pnx0106_epics_reload_firmware(void);  /* reload the previous firmware */

extern int pnx0106_epics_getlabel(char *name, int defaddr);
extern int pnx0106_epics_pp_getlabel(char *name, int defaddr);
extern int pnx0106_epics_controladdress(void);
extern int pnx0106_epics_infxchg(void);
extern int pnx0106_epics_issue_command(char *name, int defaddr, int arg1, int arg2);
extern int pnx0106_epics_decoder_claim(int codecid);	// claim plugin
extern void pnx0106_epics_decoder_release(void);	// release plugin
extern int pnx0106_epics_request_irq(int (*isr)(void* data), void *arg);
extern void pnx0106_epics_free_irq(void);
extern void pnx0106_epics_printk(char *fmt, ...);

void *pnx0106_epics_alloc_coherent(void *p, int size, dma_addr_t *phys, int hmmm);
void pnx0106_epics_free_coherent(void *p, int size, void *ptr, dma_addr_t phys);

int pnx0106_epics_alloc_backup_buffer(void);
void pnx0106_epics_free_backup_buffer(void);
void pnx0106_epics_copy_xmem2sdram(void);
void pnx0106_epics_copy_sdram2xmem(void);
void pnx0106_epics_zero_decoder_xmem(void);
void pnx0106_epics_set_exit_decoder(int halt);
int pnx0106_epics_get_exit_decoder(void);
int pnx0106_epics_decoder_exited(void);
int pnx0106_epics_need_to_resume_stream(void);



/**********************************************************************
 *
 * 		Internal prototypes for specific decoders
 */

/***	MP3		***/
/* returns the number of bytes in the input buffer (if != bytes_needed the wait was aborted) */
int e7b_mp3_wait_for_input(int nbytes);

/* copy bytes from input buffer to epics memory */
int e7b_mp3_copy_input(int nbytes, void *pInBufferEpics);

extern int e7b_mp3_decode_delay; /* decoder thread will be suspended this long for each granule (10ms by default) */

#endif   /* __KERNEL__ */

#endif

