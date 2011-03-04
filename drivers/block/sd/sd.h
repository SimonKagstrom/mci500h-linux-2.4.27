
#define SD_MAJOR 240/*LTPE Joe the name of node is /dev/sd0*/
#define SD_MINOR 0


#define MAJOR_NR SD_MAJOR
#define DEVICE_NAME "SD Card"
#define DEVICE_NR(device) (MINOR(device))
#define DEVICE_NO_RANDOM
#define DEVICE_REQUEST SD_request

#include <linux/blk.h>


#define DMA_DATA_OUT 1
#define DMA_DATA_IN  0

#define SD_REG_WR 1
#define SD_REG_RD 0

#define SD_Request_OK 1
#define SD_Request_Fail 0

#define MCI_DataIn 0x2
#define MCI_DataOut 0
#define MCI_DataEnable 1
#define MCI_BlkTransfer 0
#define MCI_StreamTransfer 0x4
#define MCI_DmaEnable 0x8


#define CPSM_WAIT_RSP 0x40
#define CPSM_LONG_RSP 0x80
#define CPSM_WAIT_INT 0x100
#define CPSM_WAIT_PEND 0x200
#define CPSM_ENABLE   0x400 

#define SD_OCR_BUST_MASK 0x80000000

#define U32 unsigned int
#define U8  unsigned char
#define U16 unsigned short

//#define SD_FLOW_DBG/*LTPE Joe the switch of debug message*/
//#define SDMA_DUMP_AUDIO
//#define MCI_DUMP
#define SD_INIT_DBG

//#define SDMA_DUMP
//#define SDMA_DUMP_OUT
//#define SDMA_DUMP_IN

//#define SD_DUMP_IN_DATA

//#define SD_NO_SDMA_IRQ


//#define SD_DMA_MODE

#ifdef SD_FLOW_DBG
    #define SD_Flow_Dbg(fmt,args...)  printk(KERN_INFO fmt,##args)
#else
    #define SD_Flow_Dbg(fmt,args...) 
#endif

#ifdef SD_INIT_DBG
    #define SD_Init_Dbg(fmt,args...)  printk(KERN_INFO fmt,##args)
#else
    #define SD_Init_Dbg(fmt,args...) 
#endif

struct sd_command {
	u32			opcode;
	u32			arg;
	u32			resp[4];
	U32		flags;		/* expected response type */
#define SD_RSP_NONE	(0 << 0)
#define SD_RSP_SHORT	(1 << 0)
#define SD_RSP_LONG	(2 << 0)
#define SD_RSP_MASK	(3 << 0)
#define SD_RSP_CRC	(1 << 3)		/* expect valid crc */
#define SD_RSP_BUSY	(1 << 4)		/* card may send busy */

/*
 * These are the response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define SD_RSP_R1	(SD_RSP_SHORT|SD_RSP_CRC)
#define SD_RSP_R1B	(SD_RSP_SHORT|SD_RSP_CRC|SD_RSP_BUSY)
#define SD_RSP_R2	(SD_RSP_LONG|SD_RSP_CRC)
#define SD_RSP_R3	(SD_RSP_SHORT)
#define SD_RSP_R6	((SD_RSP_SHORT|SD_RSP_CRC)+3)

	U32		retries;	/* max number of retries */
	U32		error;		/* command error */

#define SD_ERR_NONE	0
#define SD_ERR_TIMEOUT	1
#define SD_ERR_BADCRC	2
#define SD_ERR_FIFO	3
#define SD_ERR_FAILED	4
#define SD_ERR_INVALID	5

	void		*data;		/* data segment associated with cmd */
	struct sd_request	*mrq;		/* assoicated request */
};

struct SD_cid {
	U16             oemid;
	U16             manf_date;
	U8		prod_name[5];
	U8		prod_revision;
	U32             serial;
	U8		manfid;
};

struct SD_csd {
       U32 BlkSize;
       U32 BlkSizeFactor;
       U32 BlkNums;
       U32 EraseBlkSize;
       U32 DevSize;
       U32 DataTransSpeed;
       U32 c_size;
       U32 c_size_mult;       
};


/*
 * MMC device
 */
struct SD_card {

	U32		rca;		/* relative card address of device */
	U32		state;		/* (our) card state */
	U32             CardBlkSize;    /*Bytes in one sector*/
#define MMC_STATE_PRESENT	(1<<0)		/* present in sysfs */
#define MMC_STATE_DEAD		(1<<1)		/* device no longer in stack */
#define MMC_STATE_BAD		(1<<2)		/* unrecognised device */
	U32			raw_cid[4];	/* raw card CID */
	U32			raw_csd[4];	/* raw card CSD */
	struct SD_cid		cid;		/* card identification */
	struct SD_csd		csd;		/* card specific */
	void *                  driver_data;    /*LTPE Joe for attaching data*/
};


