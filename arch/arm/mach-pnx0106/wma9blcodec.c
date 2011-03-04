
/**********************************************************************************
 *  (C) Copyright 2003 Philips Semiconductors						              *
 **********************************************************************************/

/*----------------------------------------------------------------------------
   Standard include files:
   --------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define printk	pnx0106_epics_printk
#else
#define printk(fmt...)
#endif

#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/wma9bl.h>

/*----------------------------------------------------------------------------
   Typedefs:
   --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
   Static members and defines:
   --------------------------------------------------------------------------*/
/* Static globals for data passing between epics and Arm */
static volatile u32 *carrier;	/* parameter list for epics commands */
static volatile u32 *infXchg;	/* command/return register */

static int epics_busy = 0;
static int current_nchannels = 2;

/* some WMA codes required for epics decoder */
#define WMA_OK                 0x00000000
#define WMA_S_NEWPACKET        0x00000003
#define WMA_E_NO_MORE_SRCDATA  0xFF840005

/* MAX input size for WMA decoder (defined in DecodersApp.c on Epics) */
#define WMA_MAX_BYTES E7B_WMA_MAX_INPUT		/* each byte will be copied to 1 epics register */

typedef volatile struct decInputBufferInfo
{
    int bufferOwner;
    int bufferFlags;
    int bufferSizeInBytes;
    int* bufferStart;
    int bufferLengthInBytes;
    int bufferOffset;
    int epicsDebug;
    int armDebug;
} decInputBufferInfo_t;

typedef volatile struct decOutputBufferInfo
{
    int bufferOwner;
    int bufferFlags;
    int bufferSizeInBytes;
    int* bufferStart;
    int bufferLengthInBytes;
    int epicsDebug;
    int armDebug;
} decOutputBufferInfo_t;

decInputBufferInfo_t *decInputBuffer0;
decInputBufferInfo_t *decInputBuffer1;
decOutputBufferInfo_t *decOutputBuffer0;
decOutputBufferInfo_t *decOutputBuffer1;

volatile u32* decNoInBuffers;
volatile u32* decNoOutBuffers;

decInputBufferInfo_t *currentInputBuffer;
decOutputBufferInfo_t *currentOutputBuffer;
static int nextOutputBuffer = 0;
static int nextInputBuffer = 0;

#define EPICS_OWNS_BUFFER 0
#define ARM_OWNS_BUFFER 1

#define SET_EOF 0xffffff

typedef struct haltSaveInfoStruct
{
    int currentInputBufferAddr;
    int currentOutputBufferAddr;
    int nextInputBuffer;
    int nextOutputBuffer;
    wait_queue_head_t w8Q;
    int pass_db;
    int input_newpacket;
    int input_eof;
    int epics_busy;
    int current_nchannels;

} haltSaveInfo_t;

haltSaveInfo_t haltSaveInfo;

static wait_queue_head_t w8Q;

int wma9bl_timeout_counter = 0;

#ifndef wait_event_interruptible_timeout
// retval > 0; condition met; we're good.
// retval < 0; interrupted by signal.
// retval == 0; timed out.

#define __wait_event_interruptible_timeout(wq, condition, ret)            \
do {                                                                      \
          wait_queue_t __wait;                                            \
          init_waitqueue_entry(&__wait, current);                         \
                                                                          \
          add_wait_queue(&wq, &__wait);                                   \
          for (;;) {                                                      \
                  set_current_state(TASK_INTERRUPTIBLE);                  \
                  if (condition)                                          \
                          break;                                          \
                  if (signal_pending(current)) {                          \
			  ret = -ERESTARTSYS;                             \
			  break;                                          \
                  }                                                       \
		  ret = schedule_timeout(ret);                		  \
		  if (!ret)						  \
			 break;						  \
          }                                                               \
          set_current_state(TASK_RUNNING);                                \
          remove_wait_queue(&wq, &__wait);                                \
} while (0)

#define wait_event_interruptible_timeout(wq, condition, timeout)	  \
({									  \
	long __ret = timeout;						  \
	if ((!condition))						  \
		__wait_event_interruptible_timeout(wq, condition, __ret); \
	__ret;								  \
})

#endif


/*************************************************************************
 * Function            : wma9bl_isr
 *************************************************************************/
static int
wma9bl_isr(void* data)
{
    (void)data;	/* not used */
    printk("wma9bl_isr, epics_busy = %x\n", epics_busy);

    if (epics_busy == 1)
	epics_busy = 0;

    wake_up_interruptible(&w8Q);  /* wakeup */
    return 0;
}

/*************************************************************************
 * Function            : WAIT_UNTIL_READY
 *************************************************************************/
static inline void
WAIT_UNTIL_READY(int condition)
{
    while (1)
    {
	u32 data = infXchg[0];

	if (data == condition)
	    break;

	schedule();
    }
}

/*************************************************************************
 * Function            : copy_input
 *************************************************************************/
static void
copy_input(u32 *pOut, u8 *pIn, u32 size)
{
    u32 i = 0;
    u32 temp;

    pnx0106_epics_transfer_3x32_4x24();

    for (i = 0; i < size; i += 4)
    {
	temp = *pIn++;
	temp = (temp << 8) | *pIn++;
	temp = (temp << 8) | *pIn++;
	temp = (temp << 8) | *pIn++;

	// 32 bits transfer mode must be use in lossless_32_to_24 mode
	*pOut++ = temp;
    }

    *pOut = 0;        // flush the internal saved byte during lossloess_32_to_24 transfer
    pnx0106_epics_transfer_32_24();
}

/*************************************************************************
 * Function            : wait_irq
 * Description         : wait for the epics to interrupt the ARM
 *************************************************************************/
static void
wait_irq(void)
{
    while (1)
    {
	u32 data = infXchg[0];

	switch (data)
	{
	case 0:
        case WMAAUDIODEC_GETINPUT:
        case WMAAUDIODEC_STREAM:
	    return;
	case WMAAUDIODEC_INPUTREADY:
	    if (epics_busy)
	    {
		int ret;
		ret = wait_event_interruptible_timeout(w8Q, !epics_busy, 25);

		if (ret < 0)
		    TRACE(1, "interrupted epics wait\n");

	    	if (ret == 0)
		{
//		    pnx0106_epics_printk("WMA9BL: Epics request TIMED OUT, RESET REQUIRED\n");
		    TRACE(1, "timedout epics wait\n");
		    wma9bl_timeout_counter++;
		}
	    }
	    else
		schedule();

	    break;
	default:
	    schedule();
	    break;
	}
    }
}

/*************************************************************************
 * Function            : assignNextInputBuffer
 *************************************************************************/
static void assignNextInputBuffer(void)
{
    nextInputBuffer = (nextInputBuffer+1)%(*decNoInBuffers);

    switch(nextInputBuffer)
    {
    default:
    case 0:
        currentInputBuffer = decInputBuffer0;
        break;

    case 1:
        currentInputBuffer = decInputBuffer1;
        break;
    }

}


/*************************************************************************
 * Function            : assignNextOutputBuffer
 *************************************************************************/
static void assignNextOutputBuffer(void)
{

    nextOutputBuffer = (nextOutputBuffer+1)%(*decNoOutBuffers);

    switch(nextOutputBuffer)
    {
    default:
    case 0:
        currentOutputBuffer = decOutputBuffer0;
        break;
    case 1:
        currentOutputBuffer = decOutputBuffer1;
        break;
    }


}

/*************************************************************************
 * Function            : wma9bldecNew
 *************************************************************************/
void
wma9bldecNew(void)
{
    TRACE(1, "%s\n", __FUNCTION__);

    /* set default transfer mode */
    pnx0106_epics_transfer_32_24();

    /* Translate epics addresses */
    carrier    = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    infXchg    = pnx0106_epics_ctr2arm(pnx0106_epics_infxchg());

    decInputBuffer0 = (decInputBufferInfo_t *)pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderInBuffer0", 0));
    decOutputBuffer0 = (decOutputBufferInfo_t *)pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderOutBuffer0", 0));
    decInputBuffer1 = (decInputBufferInfo_t *)pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderInBuffer1", 0));
    decOutputBuffer1 = (decOutputBufferInfo_t *)pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderOutBuffer1", 0));

    printk("decInputBuffer0 = %x\n", (int)decInputBuffer0);
    printk("decOutputBuffer0 = %x\n", (int)decOutputBuffer0);
    printk("decInputBuffer1 = %x\n", (int)decInputBuffer1);
    printk("decOutputBuffer1 = %x\n", (int)decOutputBuffer1);


    decNoInBuffers = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderNoInBuffers", 0));
    decNoOutBuffers = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderNoOutBuffers", 0));

    currentInputBuffer = decInputBuffer0;
    currentOutputBuffer = decOutputBuffer0;
    nextOutputBuffer = 0;
    nextInputBuffer = 0;


    TRACE(1, "carrier = %p\n", carrier);
    TRACE(1, "infXchg = %p\n", infXchg);

    init_waitqueue_head(&w8Q);
    pnx0106_epics_request_irq(wma9bl_isr, 0);
}

/*************************************************************************
 * Function            : epicsNew
 *************************************************************************/
static u32
epicsNew(void)
{
    TRACE(1, "%s\n", __FUNCTION__);

    /* start DSP (TODO shouldn't this be done after loading) */
    pnx0106_epics_set_ready();

    /* Send the NEW command (parameters in comm buffer) */
    infXchg[0] = WMAAUDIODEC_NEW;
    pnx0106_epics_interrupt_req();

    WAIT_UNTIL_READY(WMAAUDIODEC_WAITFORINIT); /* Clear Decoder is being done now */

    /* Send the InitReady */
    infXchg[0] = WMAAUDIODEC_INITREADY;
    WAIT_UNTIL_READY(0);

    printk("decInputBuffer0->bufferStart = %x\n", (int)decInputBuffer0->bufferStart);
    printk("decOutputBuffer0->bufferStart = %x\n", (int)decOutputBuffer0->bufferStart);

    /* Read back the result */
    return carrier[0];
}

static int input_newpacket = 0;
static int input_eof = 0;
static int pass_db = 0;

/*************************************************************************
 * Function            : wma9bldecInit
 *************************************************************************/
int
wma9bldecInit(void)
{
    TRACE(1, "%s\n", __FUNCTION__);

    /* Do the epicsNew now that we have the input params */
    if (epicsNew() == 0)
    {
	TRACE(1, "WMA NEW Failed\n");
	ERRMSG("WMA NEW failed\n");
	return -1;
    }

    /* Send the init */
    infXchg[0] = WMAAUDIODEC_INIT;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY(0);

    /* Give back the state var */
    TRACE(3, "after init \n");

    input_newpacket = 1;
    input_eof = 0;
    pass_db = 0;

    return 0;
}

/*************************************************************************
 * Function            : wma9blstartCmd
 *************************************************************************/
int
wma9blstartCmd(int cmd)
{
    printk("+wma9blstartCmd %x\n", cmd);
    epics_busy = (((cmd == WMAAUDIODEC_STREAM)||(cmd == WMAAUDIODEC_RESUME_STREAM)) ? 2 : 1);
    infXchg[0] = cmd;
    pnx0106_epics_interrupt_req();
    wait_irq();

    if ((cmd != WMAAUDIODEC_STREAM)&&(cmd != WMAAUDIODEC_RESUME_STREAM))
	epics_busy = 0;

    printk("-wma9blstartCmd %x\n", cmd);
    return infXchg[0] != 0;
}

/*************************************************************************
 * Function            : wma9bldecNewPacket
 *************************************************************************/
void
wma9bldecNewPacket(void)
{
    input_newpacket = 1;
}

/*************************************************************************
 * Function            : wma9bldecInputEOF
 *************************************************************************/
void
wma9bldecInputEOF(void)
{
    printk("Call wma9bldecInputEOF\n");
    input_eof = 1;
}

/*************************************************************************
 * Function            : wma9bldecOutputPassOwnership
 * Description         : On a release, wma9bldecInputData is called continuously
                         which deals with the input double buffers but not the
                         output which means that on the epics side, it may enter a situation
                         where the EPICS is waiting for an output buffer and never gets it
 *************************************************************************/
void
wma9bldecOutputPassOwnership(void)
{
    printk("Call wma9bldecEmptyDoubleBuffers\n");
    pass_db = 1;
}

/*************************************************************************
 * Function            : wma9bldecInputData
 *************************************************************************/
int
wma9bldecInputData(u8 *pData, u32 len)
{
    int WMA9D_wmaparseResult;

    printk("+wma9bldecInputData\n");
    if (len == 0 && !input_eof)
	return 0;

    // Need to check if there is double buffering or not ...
    if (*decNoInBuffers > 1)
    {
        printk("currentInputBuffer = %x\n", (int)currentInputBuffer);
        printk("currentInputBuffer->bufferOwner = %x\n", currentInputBuffer->bufferOwner);

        if (input_eof)
        {
            printk("pass_db = %x\n", pass_db);

            if (pass_db)
            {
                // Need to pass ownership of the output buffers so that they do not enter a hang
                // situation in EPICS
                if (currentOutputBuffer->bufferOwner == ARM_OWNS_BUFFER)
                {
                    currentOutputBuffer->bufferFlags = SET_EOF;
                    currentOutputBuffer->bufferOwner = EPICS_OWNS_BUFFER;
                }

                assignNextOutputBuffer();
            }

            printk("Set Length = 0 ...\n");
	    len = 0;
        }


        // Check if I own current buffer and exit if I don't
        if(currentInputBuffer->bufferOwner!=ARM_OWNS_BUFFER)
        {
            return 0;
        }

        // Fill out Length
        currentInputBuffer->bufferLengthInBytes = len;
        printk("currentInputBuffer->bufferLengthInBytes = %d\n", currentInputBuffer->bufferLengthInBytes);


        // Fill out data
        if (len != 0)
	    copy_input((u32*)pnx0106_epics_xram2arm((u32)currentInputBuffer->bufferStart), pData, len);

        // Pass control to Epics
        currentInputBuffer->bufferOwner=EPICS_OWNS_BUFFER;
        currentInputBuffer->armDebug++;

        // Get Next buffer
        assignNextInputBuffer();
    }
    else
    {
        if (epics_busy)
	    return 0;

        if (infXchg[0] != WMAAUDIODEC_GETINPUT)
	    return 0;

        if((carrier[3])||(carrier[4]))
        {
            printk("wma9bldecInputData: bytesToSkip[0] = %x\n", carrier[3]);
            printk("wma9bldecInputData: bytesToSkip[1] = %x\n", carrier[4]);

        }
        printk("wma9bldecInputData: byteOffset[0] = %x\n", carrier[0]);
        printk("wma9bldecInputData: byteOffset[1] = %x\n", carrier[1]);


        if (len > WMA_MAX_BYTES)
	    len = WMA_MAX_BYTES;

        /* Give parseResult */
        WMA9D_wmaparseResult = WMA_OK;

        if (input_newpacket)
        {
	    WMA9D_wmaparseResult = WMA_S_NEWPACKET;
	    input_newpacket = 0;
        }

        if (input_eof)
        {
	        WMA9D_wmaparseResult = WMA_E_NO_MORE_SRCDATA;
            len = 0;
        }

        /* Give input parameters before giving data */
        //carrier[0] = WMA9D_wmaparseResult;
        carrier[2] = len; /* bufferlength */

        // Need to transfer data correctly to Epics - lossless mode
        // This code is stolen from the reference design
        /* Give buffer to epics  - copy to buffer 0 */
        if (len != 0)
	    copy_input((u32*)pnx0106_epics_xram2arm((u32)(decInputBuffer0->bufferStart)), pData, len);
        {
            int i;
            printk("First 10 bytes of data in = :");

            for(i=0;i<10;i++)
                printk(" %x", pData[i]);

            printk("\n");
        }

        /* Indicate that the wma data is available for the epics decoder */
        epics_busy = 1;  /* it has new data to process */
        infXchg[0] = WMAAUDIODEC_INPUTREADY;

    }
    //printk("+wma9bldecInputData, len = %d\n", len);

    return len;
}

/*************************************************************************
 * Function            : wma9bldecContinue
 *************************************************************************/
int
wma9bldecContinue()
{
    if (epics_busy)
    {
	wait_irq();
	epics_busy = 0;
    }

    return infXchg[0] != 0;
}

/*************************************************************************
 * Function            : wma9bldecDecode
 *************************************************************************/
int
wma9bldecDecode(void)
{
    /* Start decode cycle */
    TRACE(3, "start decoder\n");
    return wma9blstartCmd(WMAAUDIODEC_DECODE);
}

/*************************************************************************
 * Function            : wma9bldecDecodeFinish
 *************************************************************************/
void
wma9bldecDecodeFinish(u16* pcSamplesReady, u16 *pEof, u16 *pDecReturn)
{
    int WMA9D_appReturn;
    int WMA9D_return;

    /* Read back the result */
    *pcSamplesReady    = carrier[0];
    WMA9D_appReturn = carrier[1]; /* TBD what to do with this info ??? */
    WMA9D_return = carrier[2]; /* TBD what to do with this info ??? */

    // If decoder returns End of Stream, need to flag EOF
    // Probably don't need both but it's easier for now ..
    *pEof = (WMA9D_return == 0x06);
    *pDecReturn = WMA9D_return;
    TRACE(3, "decoder result: sr=%u \n", *pcSamplesReady);
}

/*************************************************************************
 * Function            : checksum_bytes
 *************************************************************************/
#if CHECKSUM
static u32
checksum_bytes(u8 *pData, u32 len)
{
    int i;
    u32 sum = 0;
    u32 word;

    for (i = 0; i < len; i += 2)
    {
	word = *pData++;
	word |= *pData++ << 8;
	sum += word;
    }

    return sum;
}
#endif

/*************************************************************************
 * Function            : wma9bldecGetPCM
 *************************************************************************/
void
wma9bldecGetPCM(u16 cSamplesRequested,
	u16 *pcSamplesReturned,
	u8* pbDst,
	u16 cbDstLength,
    u16 *pState)
{
    int csum_epics;
#ifdef CHECKSUM
    int csum_arm = 0;
#endif

    if ((u32)pbDst & 0x3)
	WARN("pbDst %p not a multiple of 4 !!!", pbDst);

    carrier[0] = cSamplesRequested;
    carrier[1] = cbDstLength;
    carrier[2] = 1;			// request the Epics to checksum the result

    /* call epics GetPCM() */
    infXchg[0] = WMAAUDIODEC_GETPCM;
    pnx0106_epics_interrupt_req();

    // Should really wait for IRQ here
    WAIT_UNTIL_READY(0);

    /* Read back the result */
    *pcSamplesReturned = carrier[0];
    // carrier[1] is unused
    csum_epics = carrier[2];

    /* Copy the decoded data from the epics to arm */
    {
	    /* protected area */
	    //static const unsigned int epics_bufsize = 0x3000; /* in bytes */
	    unsigned long flags;
        unsigned long bytes_returned = *pcSamplesReturned * 2 * current_nchannels;
        u8* pSource = (u8*)pnx0106_epics_xram2arm_2x24((u32)(decOutputBuffer0->bufferStart));
	    //int temp;

	    local_irq_save(flags);
	    pnx0106_epics_transfer_32_2x16();
        /* Just copy from the source pointer, will not go over the end */
        memcpy(pbDst, pSource, (int)bytes_returned);
        {
            int i;
            printk("First 10 bytes of data out = :");

            for(i=0;i<10;i++)
                printk(" %x", pbDst[i]);

            printk("\n");
        }


#ifdef CHECKSUM
       csum_arm += checksum_bytes(pbDst, bytes_returned);

        if ((csum_arm & 0xffffff) != csum_epics)
            printk("Different CSUMS csum_arm = %x, csum EPICS = %x\n", csum_arm, csum_epics);
#endif

       pnx0106_epics_transfer_32_24();
       local_irq_restore(flags);
    }

    /* Give command to update output pointers, in case something was returned */
    if (*pcSamplesReturned)
    {
        infXchg[0] = WMAAUDIODEC_UPDATEPOINTERS;
        pnx0106_epics_interrupt_req();
        WAIT_UNTIL_READY(0);

    }
}

/*************************************************************************
 * Function            : wma9bldecDelete
 * Description         : Will be called at end of decoding
 *************************************************************************/
void
wma9bldecDelete(void)
{
    /* Send the Delete command */
    // The delete command does not exist for this decoder

    pnx0106_epics_free_irq();
}

/*************************************************************************
 * Function            : wma9bldecReset
 * Description         : Will be called when seeking (reset coeff)
 *************************************************************************/
void
wma9bldecReset(void)
{
    // If we're using double buffering - we don't use
    // infXchg = need just to bypass the infXchg command
    if(*decNoInBuffers==1)
    {
        /* Send the Reset command */
        infXchg[0] = WMAAUDIODEC_RESET;
        pnx0106_epics_interrupt_req();
        WAIT_UNTIL_READY(0);
    }

    input_newpacket = 1;
    input_eof = 0;


}

/*************************************************************************
 * Function            : wma9bldecGetAsfHdr
 * Description         : Gets ASF header information
 *************************************************************************/
int
wma9bldecGetAsfHdr(void)
{
    return wma9blstartCmd(WMAAUDIODEC_GETHDR);
}

/*************************************************************************
 * Function            : wma9bldecGetAsfHdrFinish
 * Description         : Gets ASF header information
 *************************************************************************/
void
wma9bldecGetAsfHdrFinish(e7b_asf_hdr_format* phdr, u16 *pDecReturn)
{
    phdr->ver = carrier[0];
    phdr->sampleRate = carrier[1];
    phdr->numOfChans = carrier[2];
    phdr->duration[0] = carrier[3];
    phdr->duration[1] = carrier[4];
    phdr->pktSize[0] = carrier[5];
    phdr->pktSize[1] = carrier[6];
    phdr->firstPktOffset[0] = carrier[7];
    phdr->firstPktOffset[1] = carrier[8];
    phdr->lastPktOffset[0] = carrier[9];
    phdr->lastPktOffset[1] = carrier[10];
    phdr->hasDRM = carrier[11];
    phdr->licenseLen[0] = carrier[12];
    phdr->licenseLen[1] = carrier[13];
    phdr->bitrate = carrier[14];
    phdr->pktAllowed = carrier[15];
    phdr->subFrmAllowed = carrier[16];
    phdr->bitsPerSample = carrier[17];
    current_nchannels = phdr->numOfChans;

    *pDecReturn = carrier[18];

}

/*************************************************************************
 * Function            : wma9bldecStreamSeek
 * Description         : seeks in stream
 *************************************************************************/
int
wma9bldecStreamSeek(u32 seekTimeFromStart)
{
    carrier[0] = seekTimeFromStart&0x7fffff;
    carrier[1] = (seekTimeFromStart>>23)&0x1ff;

    return wma9blstartCmd(WMAAUDIODEC_STREAMSEEK);
}

/*************************************************************************
 * Function            : wma9bldecStreamSeekFinish
 * Description         : seeks in stream
 *************************************************************************/
void
wma9bldecStreamSeekFinish(u32 *pActualSeekTimeFromStart, u16 *pDecReturn)
{
    *pActualSeekTimeFromStart = (carrier[1] << 23) | (carrier[0]&0x7fffff);
    printk("wma9bldecStreamSeekFinish : returnOffsetInBytes = %d\n", *pActualSeekTimeFromStart);
    *pDecReturn = carrier[2];
}

/*************************************************************************
 * Function            : wma9bldecResynch
 * Description         : seeks in stream
 *************************************************************************/
int
wma9bldecResynch(u32 seekOffsetInBytes)
{
    carrier[0] = seekOffsetInBytes & 0x7fffff;
    carrier[1] = (seekOffsetInBytes>>23) & 0x1ff;

    return wma9blstartCmd(WMAAUDIODEC_RESYNCH);
}

/*************************************************************************
 * Function            : wma9bldecResynchFinish
 * Description         : seeks in stream
 *************************************************************************/
void
wma9bldecResynchFinish(u16 *pDecReturn)
{
    u32 returnOffsetInBytes;

    returnOffsetInBytes = (carrier[1] << 23) | (carrier[0]&0x7fffff);
    *pDecReturn = carrier[2];
}

/*************************************************************************
 * Function            : wma9bldecGetStreamByteOffset
 * Description         : Gets the location of the seeking offset
 *************************************************************************/
u32 wma9bldecGetStreamByteOffset(void)
{
    u32 seekOffsetInBytes;

    seekOffsetInBytes = (carrier[1] << 23) | (carrier[0] & 0x7fffff);

    printk("wma9bldecGetStreamByteOffset: byteOffset[0] = %x\n", carrier[0]);
    printk("wma9bldecGetStreamByteOffset: byteOffset[1] = %x\n", carrier[1]);


    return seekOffsetInBytes;


}

/*************************************************************************
 * Function            : wma9bldecStreamStart
 * Description         : Sets up streaming functionality
 *************************************************************************/
int
wma9bldecStreamStart(void)
{
    printk("call wma9bldecStreamStart\n");

    currentInputBuffer = decInputBuffer1;
    currentOutputBuffer = decOutputBuffer0;
    nextOutputBuffer = 0;
    nextInputBuffer = 1;

    return wma9blstartCmd(WMAAUDIODEC_STREAM);
}

int
wma9bldecStreamResume(void)
{
    printk("call wma9bldecStreamResume\n");
    return wma9blstartCmd(WMAAUDIODEC_RESUME_STREAM);


}

int
wma9bldecStreamRestore(void)
{
    pnx0106_epics_printk("call wma9bldecStreamRestore\n");


    currentInputBuffer = (decInputBufferInfo_t *)haltSaveInfo.currentInputBufferAddr;
    currentOutputBuffer = (decOutputBufferInfo_t *)haltSaveInfo.currentOutputBufferAddr;

    nextInputBuffer = haltSaveInfo.nextInputBuffer;
    nextOutputBuffer = haltSaveInfo.nextOutputBuffer;

    w8Q = haltSaveInfo.w8Q;

    pass_db = haltSaveInfo.pass_db;
    input_newpacket = haltSaveInfo.input_newpacket;
    input_eof = haltSaveInfo.input_eof;
    epics_busy = haltSaveInfo.epics_busy;
    current_nchannels = haltSaveInfo.current_nchannels;


    printk("currentInputBuffer = %x\n",currentInputBuffer);
    printk("currentInputBuffer->bufferOwner = %x\n", currentInputBuffer->bufferOwner);
    printk("currentInputBuffer->bufferFlags = %x\n", currentInputBuffer->bufferFlags);

    printk("currentOutputBuffer = %x\n",currentOutputBuffer);
    printk("currentOutputBuffer->bufferOwner = %x\n", currentOutputBuffer->bufferOwner);
    printk("currentOutputBuffer->bufferFlags = %x\n", currentOutputBuffer->bufferFlags);

    printk("nextOutputBuffer = %x\n",nextOutputBuffer);
    printk("nextInputBuffer = %x\n",nextInputBuffer);


    carrier[2]=0;

    return pnx0106_epics_request_irq(wma9bl_isr, 0);

}

int
wma9bldecStreamSave(void)
{
    pnx0106_epics_printk("call wma9bldecStreamSave\n");


    haltSaveInfo.currentInputBufferAddr = (int)currentInputBuffer;
    haltSaveInfo.currentOutputBufferAddr = (int)currentOutputBuffer;

    haltSaveInfo.nextInputBuffer = nextInputBuffer;
    haltSaveInfo.nextOutputBuffer = nextOutputBuffer;

    haltSaveInfo.w8Q = w8Q;

    haltSaveInfo.pass_db = pass_db;
    haltSaveInfo.input_newpacket = input_newpacket;
    haltSaveInfo.input_eof = input_eof;
    haltSaveInfo.epics_busy = epics_busy;
    haltSaveInfo.current_nchannels = current_nchannels;

    printk("currentInputBuffer = %x\n",currentInputBuffer);
    printk("currentInputBuffer->bufferOwner = %x\n", currentInputBuffer->bufferOwner);
    printk("currentInputBuffer->bufferFlags = %x\n", currentInputBuffer->bufferFlags);

    printk("currentOutputBuffer = %x\n",currentOutputBuffer);
    printk("currentOutputBuffer->bufferOwner = %x\n", currentOutputBuffer->bufferOwner);
    printk("currentOutputBuffer->bufferFlags = %x\n", currentOutputBuffer->bufferFlags);

    printk("nextOutputBuffer = %x\n",nextOutputBuffer);
    printk("nextInputBuffer = %x\n",nextInputBuffer);

    carrier[2]=0;

    pnx0106_epics_free_irq();

    return 0;


};

/*************************************************************************
 * Function            : wma9bldecStreamFinish
 * Description         : seeks in stream
 *************************************************************************/
void
wma9bldecStreamFinish(u16 *pDecReturn)
{
    printk("call wma9bldecStreamFinish\n");
    *pDecReturn = carrier[2];
    epics_busy = 0;

    // Need to reset input_eof and pass_db for any future operations ..
    input_eof = 0;
    pass_db = 0;
}


/*************************************************************************
 * Function            : wma9bldecStreamGetPCM
 *************************************************************************/
void
wma9bldecStreamGetPCM(u16 *pcSamplesReturned,
	u8* pbDst,
	u16 *pDecReturn)
{

    u8* pSource = (u8*)pnx0106_epics_xram2arm_2x24((u32)(currentOutputBuffer->bufferStart));
    unsigned long flags;
    unsigned int noOfBytes;

    if ((u32)pbDst & 0x3)
	WARN("pbDst %p not a multiple of 4 !!!", pbDst);

    printk("Call GetPCM ...\n");

    printk("currentOutputBuffer = %x\n", (int)currentOutputBuffer);


    printk("currentOutputBuffer->bufferOwner = %x\n", currentOutputBuffer->bufferOwner);

    // Check if I own current buffer
    if (currentOutputBuffer->bufferOwner != ARM_OWNS_BUFFER)
    {
	int ret;

        // Check if EPICS needs data i.e ARM owns input buffer
        if (currentInputBuffer->bufferOwner == ARM_OWNS_BUFFER)
        {
            // Need to return
            *pcSamplesReturned = 0;
            *pDecReturn = 0;
            return;
        }

        // otherwise wait for interrupt
        ret = wait_event_interruptible_timeout(w8Q,
		(infXchg[0] != WMAAUDIODEC_STREAM ||
		currentInputBuffer->bufferOwner == ARM_OWNS_BUFFER ||
		currentOutputBuffer->bufferOwner == ARM_OWNS_BUFFER), 25);

        if (ret < 0)
        {
	    TRACE(1, "interrupted epics wait\n");
        }

        if (ret == 0)
        {
	    int owner = currentInputBuffer->bufferOwner;

	    pnx0106_epics_printk("WMA9BL: Epics stream TIMED OUT, RESET REQUIRED\n");
	    pnx0106_epics_printk("WMA9BL: current input buffer owner %d, current output buffer owner %d infXchg[0] %x\n", owner, currentOutputBuffer->bufferOwner, infXchg[0]);
	    TRACE(1, "timedout epics wait\n");
	    wma9bl_timeout_counter++;
        }

        // Need to check and see which buffer has changed ownership
        // if is output buffer can continue
        // if input buffer => need to return
        if (currentOutputBuffer->bufferOwner != ARM_OWNS_BUFFER)
        {
            // Need to return
            *pcSamplesReturned = 0;
            *pDecReturn = 0;

	    if (infXchg[0] != WMAAUDIODEC_STREAM)
	    {
		*pDecReturn = carrier[2];
		pnx0106_epics_printk("Epics snuk out of streaming state infXchg %x, carrier[2] = %d\n", infXchg[0], *pDecReturn);
	    }
	    else
		*pDecReturn = 0;

            return;
        }
    }

    noOfBytes = currentOutputBuffer->bufferLengthInBytes;
    printk("currentOutputBuffer->bufferLengthInBytes = %d\n", currentOutputBuffer->bufferLengthInBytes);

    // check length
    if (noOfBytes != 0)
    {
        // Copy
        local_irq_save(flags);
        pnx0106_epics_transfer_32_2x16();

        memcpy(pbDst, pSource, noOfBytes);

        pnx0106_epics_transfer_32_24();
        local_irq_restore(flags);
    }

    // Fill out decoder return and no of samples returned
    *pDecReturn = currentOutputBuffer->bufferFlags;
    *pcSamplesReturned = (int)currentOutputBuffer->bufferLengthInBytes / (2 * current_nchannels);

    // Pass control to Epics
    currentOutputBuffer->bufferOwner = EPICS_OWNS_BUFFER;
    currentOutputBuffer->armDebug++;

    // Get Next buffer
    assignNextOutputBuffer();
}

/*************************************************************************
 * Function            : wma9bldecSetAesKey
 *************************************************************************/

int wma9bldecSetAesKey(u8* key)
{
    printk("wma9bldecSetAesKey, key = %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",key[0],key[1],key[2],key[3],key[4],key[5],key[6],key[7],key[8],key[9],key[10],key[11],key[12],key[13],key[14],key[15]);
    int i;
    for(i=0;i<16;i++)
    {
        carrier[i] = key[i];
    }

    return wma9blstartCmd(WMAAUDIODEC_SET_AES_KEY);

}


