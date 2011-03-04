
/**********************************************************************************
 *  (C) Copyright 2003 Philips Semiconductors 						              *
 **********************************************************************************/

/*----------------------------------------------------------------------------
   Standard include files:
   --------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/string.h>

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
#include <asm/arch/aacbl.h>

/*----------------------------------------------------------------------------
   Typedefs:
   --------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Static members and defines:
   --------------------------------------------------------------------------*/

static volatile u32 dataAAC;
static u32 g_AACNumChannels = 0;
static u32 DecodedFrames = 0;
static int epics_busy = 0;
static int input_eof = 0;
static int pass_db = 0;
static int inputOffset = 0;

/* Static globals for data passing between epics and Arm */
static volatile u32* CarrierAAC;
static volatile u32* infXchgAAC;

typedef struct debugStuff
{
    int before;
    unsigned int p_ipRead;
    unsigned int p_ipWrite;
    unsigned int ipReadBitIdx;
    unsigned int ipWriteBitIdx;
    unsigned int p_opRead;
    unsigned int p_opWrite;
    int noBits;
    int bytes[24];
} debugStuff_t;

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
    int bytesConsumed;
} decInputBufferInfo_t;

typedef struct AAC_HeaderInfo
{
    int AACD_samplingFreqHz;
    int AACD_nrChan;
    int AACD_transportFormat;
} AAC_HeaderInfo_t;

typedef volatile struct decOutputBufferInfo
{
    int bufferOwner;
    int bufferFlags;
    int bufferSizeInBytes;
    int* bufferStart;
    int bufferLengthInBytes;
    AAC_HeaderInfo_t headerInfo;
    int inputOffset;
    int epicsDebug;
    int armDebug;
} decOutputBufferInfo_t;

static decInputBufferInfo_t *decInputBuffers[2];
static decOutputBufferInfo_t *decOutputBuffers[4];
static int inputBufferUsed;

static volatile u32 *decNoInBuffers;
static volatile u32 *decNoOutBuffers;

static decInputBufferInfo_t *currentInputBuffer;
static decOutputBufferInfo_t *currentOutputBuffer;
static int nextOutputBuffer = 0;
static int nextInputBuffer = 0;

#define EPICS_OWNS_BUFFER 0
#define ARM_OWNS_BUFFER 1

#define SET_EOF 0xffffff

static wait_queue_head_t w8Q;

typedef struct aacHaltSaveInfoStruct
{
    int currentInputBufferAddr;
    int currentOutputBufferAddr;
    int nextInputBuffer;
    int nextOutputBuffer;
    wait_queue_head_t w8Q;
    int pass_db;
    int mini_input_buf_len;
    unsigned char mini_input_buf[12];
    int input_eof;
    int epics_busy;
    u32 g_AACNumChannels;
    u32 DecodedFrames;
    int inputOffset;
    int inputBufferUsed;
} aacHaltSaveInfo_t;

static aacHaltSaveInfo_t haltSaveInfo;

/*************************************************************************
 * Function            : aac_isr
 *************************************************************************/
static int
aac_isr(void* data)
{
    (void)data;	/* not used */

    if (epics_busy)
	epics_busy = 0;

    wake_up_interruptible(&w8Q);  /* wakeup */
    return 0;
}

/*************************************************************************
 * Function            : WAIT_UNTIL_READY
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
static __inline void WAIT_UNTIL_READY (void)
{
    TRACE(5, "WAIT_UNTIL_READY\n");
    do {
	dataAAC = infXchgAAC[0];
	TRACE(5, "WAIT_UNTIL_READY: dataAAC 0x%X\n", dataAAC);
	if (dataAAC != 0)
	{
		pnx0106_epics_sleep(0); /* will just schedule so other threads can still run */
	}
    } while (dataAAC != 0);
}


static void
xmemcpy2(void *dest, void *src, int bytes)
{
    unsigned int volatile *d = (unsigned int volatile *)dest;
    unsigned int *s = (unsigned int *)src;
    unsigned int *e = (unsigned int *)&((char *)dest)[(4 * bytes) / 3];
    unsigned int temp = 0;
    int i = 0;
    int word1=0;


    TRACE(5, "xmemcpy2: dest %p src %p bytes %d end %p\n", d, s, bytes, e);
    pnx0106_epics_change_endian(src, bytes);

    while (d < e)
    {
        //int word1;

        switch (i++ & 3)
        {
        case 0:
            temp = *s++;
            word1 = temp >> 8;
            break;
        case 1:
            word1 = (temp & 0xFF) << 16;
            temp = *s++;
            word1 |= (temp >> 16);
            break;
        case 2:
            word1 = (temp & 0xFFFF) << 8;
            temp = *s++;
            word1 |= (temp >> 24);
            break;
        case 3:
            word1 = temp & 0xFFFFFF;
            break;
        }

        *d = word1;
        d++;
    }
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
	u32 data = infXchgAAC[0];

	switch (data)
	{
	case 0:
        //case AACDEC_GETINPUT:
        case AACDEC_STREAM:
	    return;
	//case AACDEC_INPUTREADY:
	//    if (epics_busy)
	//    {
	//	if (wait_event_interruptible(w8Q, !epics_busy) < 0)
	//	    TRACE(1, "interrupted epics wait\n");
	//    }
	//    else
	//	schedule();

	//    break;
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
    nextInputBuffer = (nextInputBuffer + 1) % (*decNoInBuffers);
    currentInputBuffer = decInputBuffers[nextInputBuffer];
    inputBufferUsed = 0;
}


/*************************************************************************
 * Function            : assignNextOutputBuffer
 *************************************************************************/
static void assignNextOutputBuffer(void)
{
    nextOutputBuffer = (nextOutputBuffer + 1) % (*decNoOutBuffers);
    currentOutputBuffer = decOutputBuffers[nextOutputBuffer];
}

/*************************************************************************
 * Function            : aacstartCmd
 *************************************************************************/
int
aacstartCmd(int cmd)
{
    epics_busy = 1;
    infXchgAAC[0] = cmd;
    pnx0106_epics_interrupt_req();
    wait_irq();
    epics_busy = 0;
    return infXchgAAC[0];
}

/*************************************************************************
 * Function            : aacdecInputEOF
 *************************************************************************/
void
aacdecInputEOF(void)
{
    printk("Call aacdecInputEOF\n");
    input_eof = 1;
}

/*************************************************************************
 * Function            : aacdecContinue
 *************************************************************************/
int
aacdecContinue(void)
{
    if (epics_busy)
    {
	wait_irq();
	epics_busy = 0;
    }

    return infXchgAAC[0];
}

/*************************************************************************
 * Function            : aacdecOutputPassOwnership
 * Description         : On a release, aacdecInputData is called continuously
                         which deals with the input double buffers but not the
                         output which means that on the epics side, it may enter a situation
                         where the EPICS is waiting for an output buffer and never gets it
 *************************************************************************/
void
aacdecOutputPassOwnership(void)
{
    printk("Call aacdecEmptyDoubleBuffers\n");
    pass_db = 1;
}

#ifdef DEBUG
void
dump_debug_log(void)
{
    int i, j;
    int logoffset = pnx0106_epics_getlabel("DecoderDebugLog", 0);
    debugStuff_t volatile *debuginfo = (debugStuff_t volatile *)pnx0106_epics_xram2arm(logoffset);

    pnx0106_epics_printk("Dumping debug log @%04X\n", logoffset);

    if (logoffset == 0)
	return;

    for (i = 0; ; i++)
    {
	char *type;
	char buf[128];

	switch (debuginfo[i].before)
	{
	case 0xFFFFF0:
	    type = "strmInEncDat: end";
	    break;
	case 0xFFFFF1:
	    type = "strmInEncDat: begin";
	    break;
	case 0xFFFF80:
	    type = "strmInDbEncDat: begin";
	    break;
	case 0xFFFF81:
	    type = "strmInDbEncDat: end";
	    break;
	case 0xFFFF82:
	    type = "strmInDbEncDat: curr EOF";
	    break;
	case 0xFFFF83:
	    type = "strmInDbEncDat: curr short";
	    break;
	case 0xFFFF84:
	    type = "strmInDbEncDat: next short";
	    break;
	case 0:
	    continue;
	default:
	    if (i >= 80)
		return;

	    type = "unknown";
	    break;
	}

	pnx0106_epics_printk("%s\tRecord #%d 0x%x from %s, num bits %d\n", i ? "\n" : "",
		i, debuginfo[i].before, type, debuginfo[i].noBits);
	pnx0106_epics_printk("\tin read %d:%d write %d:%d\n", debuginfo[i].p_ipRead,
                debuginfo[i].ipReadBitIdx,
                debuginfo[i].p_ipWrite,
                debuginfo[i].ipWriteBitIdx);
	pnx0106_epics_printk("\tout read %d write %d\n", debuginfo[i].p_opRead,
                debuginfo[i].p_opWrite);

	strcpy(buf, "");

	for (j = 0; j < 10; j++)
	    sprintf(&buf[strlen(buf)], "%s%X", j > 0 ? "." : "",
		    debuginfo[i].bytes[j]);

	pnx0106_epics_printk("\twords %s\n", buf);
    }
}
#else

#define dump_debug_log()

#endif


void
show_aac_buffers(void)
{
    int i;

    printk("#input %d #output %d\n", *decNoInBuffers, *decNoOutBuffers);
    printk("next input %d next output %d\n", nextInputBuffer, nextOutputBuffer);

    for (i = 0; i < 2; i++)
    {
	decInputBufferInfo_t *buf = decInputBuffers[i];
	int cur = (buf == currentInputBuffer);

	printk("    input #%d%s: owner %d flags %d size %d start 0x%x len %d\n", i, cur ? "*" : "",
		buf->bufferOwner, buf->bufferFlags, buf->bufferSizeInBytes,
		(u32)buf->bufferStart, buf->bufferLengthInBytes);
	printk("\t\toffset %d edebug %d adebug %d consumed %d\n",
		buf->bufferOffset, buf->epicsDebug, buf->armDebug, buf->bytesConsumed);
    }

    for (i = 0; i < 4; i++)
    {
	decOutputBufferInfo_t *buf = decOutputBuffers[i];
	int cur = (buf == currentOutputBuffer);

	printk("    output #%d%s: owner %d flags %d size %d start 0x%x len %d\n", i, cur ? "*" : "",
		buf->bufferOwner, buf->bufferFlags, buf->bufferSizeInBytes,
		(u32)buf->bufferStart, buf->bufferLengthInBytes);
	printk("\t\tinputOffset %d edebug %d adebug %d\n", buf->inputOffset, buf->epicsDebug, buf->armDebug);
    }
}

static unsigned char mini_input_buf[12];
static int mini_input_buf_len = 0;

static int
copy_to_epics(unsigned char *pData, int len)
{
    int offset = inputBufferUsed;
    int space = currentInputBuffer->bufferSizeInBytes - offset;
    int wordoffset = offset / 3;

    printk("copy_to_epics: pData %p, len %d, buf %p, offset %d, space %d wordoffset %d\n",
	  pData, len, currentInputBuffer, offset, space, wordoffset);

    // Fill out Length
    printk("inputBufferUsed = %d\n", offset);

    if (len > space)
	len = space;

    printk("copy_to_epics: revised len %d\n", len);

    if (!input_eof)
	len -= len % 12;

    printk("copy_to_epics: revised len 2 %d\n", len);

    // Fill out data
    if (len != 0)
    {
	u32 xaddr = (u32)currentInputBuffer->bufferStart;

	printk("copy_to_epics: copying %d, start 0x%x offset %d\n", len, xaddr, wordoffset);
	xaddr += wordoffset;
	printk("\txaddr 0x%x\n", xaddr);
        show_aac_buffers();
	xmemcpy2((char*)pnx0106_epics_xram2arm(xaddr), pData, len);
	inputBufferUsed += len;
//	dump_debug_log();
        show_aac_buffers();
    }

    if (inputBufferUsed == currentInputBuffer->bufferSizeInBytes || input_eof)
    {
        printk("copy_to_epics: passing buffer with %d\n", inputBufferUsed);

	// Pass control to Epics
	inputOffset += inputBufferUsed;
	currentInputBuffer->bufferLengthInBytes = inputBufferUsed;
	currentInputBuffer->armDebug++;
	currentInputBuffer->bufferOwner = EPICS_OWNS_BUFFER;

	// Get Next buffer
	assignNextInputBuffer();
    }

//    dump_debug_log();
    printk("copy_to_epics: returns %d\n", len);
    show_aac_buffers();
    return len;
}

/*************************************************************************
 * Function            : aacdecInputData
 *************************************************************************/
int
aacdecInputData(u8 *pData, u32 len)
{
    u32 location;
    u32 bytesNeeded;
    int sendEOF = 0;
    int currOffset = inputOffset + inputBufferUsed;

    printk("+aacdecInputData, input_eof = %d\n", input_eof);
    if (len == 0 && !input_eof)
        return 0;

    int used = 0;

    printk("currentInputBuffer = %x\n", (int)currentInputBuffer);
    printk("nextInputBuffer = %d\n", nextInputBuffer);
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
    if (currentInputBuffer->bufferOwner != ARM_OWNS_BUFFER)
    {
        return 0;
    }

	// Handle buffer alignment issues here
	if (mini_input_buf_len > 0)
	{
	    int space = sizeof mini_input_buf - mini_input_buf_len;
	    int mlen = len;

	    if (mlen > space)
		mlen = space;

	    printk("AAC: filling mini buffer space %d, len %d\n", space, mlen);
	    memcpy(&mini_input_buf[mini_input_buf_len], pData, mlen);
	    mini_input_buf_len += mlen;
	    pData += mlen;
	    len -= mlen;
	    used += mlen;

	    if (mini_input_buf_len == sizeof mini_input_buf)
	    {
		// copy it to Epics
		printk("AAC: copying mini buffer to Epics\n");
		copy_to_epics(mini_input_buf, sizeof mini_input_buf);
		mini_input_buf_len = 0;
	    }
	}

	if (len && len < sizeof mini_input_buf && !input_eof)
	{
	    printk("AAC: filling mini buffer %d\n", len);
	    memcpy(mini_input_buf, pData, len);
	    mini_input_buf_len = len;
	    used += len;
	    len = 0;
	}

	if (len > 0 || input_eof)
	{
	    printk("AAC: copying data to Epics len %d\n", len);
	    used += copy_to_epics(pData, len);
	    printk("AAC: input data processed %d\n", used);
	}

	len = used;

    //printk("+aacdecInputData, len = %d\n", len);
    printk("+aacdecInputData, len = %d, offset = %d, new offset %d\n", len, currOffset,
	    inputOffset + inputBufferUsed);
    return len;
}

/*************************************************************************
 * Function            : aacdecInit
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 aacdecInit ()
{
    int i;
    int xmemaddr;

    TRACE(2, "%s\n", __FUNCTION__);

    if (pnx0106_epics_reload_firmware() < 0) {
	    return -1;
    }

    TRACE(2, "%s firmware reloaded\n", __FUNCTION__);

    /* set the default transfer mode */
    pnx0106_epics_transfer_32_24();
    TRACE(2, "%s transfer mode set\n", __FUNCTION__);

    /* allow DSP to start */
    pnx0106_epics_set_ready();
    TRACE(2, "%s DSP started\n", __FUNCTION__);

    /* Translate epics addresses for commands passing */
    CarrierAAC = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    infXchgAAC = pnx0106_epics_ctr2arm(pnx0106_epics_infxchg());

    pnx0106_epics_printk("EPX_INFXCHG %x\n", pnx0106_epics_pp_getlabel("INFXCHG", EPX_INFXCHG));
    pnx0106_epics_printk("EPX_DECODERCARRIER %x\n", pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    pnx0106_epics_printk("EPX_DECODERINPUT %x\n", pnx0106_epics_getlabel("DecoderInput", EPX_DECODERINPUT));
    pnx0106_epics_printk("EPX_DECODEROUTPUTL %x\n", pnx0106_epics_getlabel("DecoderOutputL", EPX_DECODEROUTPUTL));
    pnx0106_epics_printk("EPX_DECODEROUTPUTR %x\n", pnx0106_epics_getlabel("DecoderOutputR", EPX_DECODEROUTPUTR));
    pnx0106_epics_printk("EPX_DECODERNOINBUFFERS %x\n", pnx0106_epics_getlabel("DecoderNoInBuffers", 0));
    pnx0106_epics_printk("EPX_DECODERINBUFFER0 %x\n", pnx0106_epics_getlabel("DecoderInBuffer0", 0));
    pnx0106_epics_printk("EPX_DECODERINBUFFER1 %x\n", pnx0106_epics_getlabel("DecoderInBuffer1", 0));
    pnx0106_epics_printk("EPX_DECODERNOOUTBUFFERS %x\n", pnx0106_epics_getlabel("DecoderNoOutBuffers", 0));
    pnx0106_epics_printk("EPX_DECODEROUTBUFFER0 %x\n", pnx0106_epics_getlabel("DecoderOutBuffer0", 0));
    pnx0106_epics_printk("EPX_DECODEROUTBUFFER1 %x\n", pnx0106_epics_getlabel("DecoderOutBuffer1", 0));
    pnx0106_epics_printk("EPX_DECODEROUTBUFFER2 %x\n", pnx0106_epics_getlabel("DecoderOutBuffer2", 0));
    pnx0106_epics_printk("EPX_DECODEROUTBUFFER3 %x\n", pnx0106_epics_getlabel("DecoderOutBuffer3", 0));

    TRACE(2, "%s CarrierAAC %p infXchgAAC %p\n", __FUNCTION__, CarrierAAC, infXchgAAC);

    xmemaddr = pnx0106_epics_getlabel("DecoderNoInBuffers", 0);

    if (xmemaddr <= 0)
	return -1;

    decNoInBuffers = pnx0106_epics_xram2arm(xmemaddr);
    xmemaddr = pnx0106_epics_getlabel("DecoderNoOutBuffers", 0);

    if (xmemaddr <= 0)
	return -1;

    decNoOutBuffers = pnx0106_epics_xram2arm(xmemaddr);
    TRACE(2, "%s decNoInBuffers %p decNoOutBuffers%p\n", __FUNCTION__, decNoInBuffers, decNoOutBuffers);

    for (i = 0; i < 2; i++)
    {
	char label[40];
	int xloc;

	snprintf(label, sizeof label, "DecoderInBuffer%d", i);
	xloc = pnx0106_epics_getlabel(label, 0);

	if (xloc > 0)
	    decInputBuffers[i] = (decInputBufferInfo_t *)pnx0106_epics_xram2arm(xloc);
	else
	    decInputBuffers[i] = NULL;

	TRACE(1, "%s %s = %p\n", __FUNCTION__, label, decInputBuffers[i]);
    }

    for (i = 0; i < 4; i++)
    {
	char label[40];
	int xloc;

	snprintf(label, sizeof label, "DecoderOutBuffer%d", i);
	xloc = pnx0106_epics_getlabel(label, 0);

	if (xloc > 0)
	    decOutputBuffers[i] = (decOutputBufferInfo_t *)pnx0106_epics_xram2arm(xloc);
	else
	    decOutputBuffers[i] = NULL;

	TRACE(1, "%s %s = %p\n", __FUNCTION__, label, decOutputBuffers[i]);
    }

    currentInputBuffer = decInputBuffers[0];
    inputOffset = 0;
    inputBufferUsed = 0;
    currentOutputBuffer = decOutputBuffers[0];
    nextOutputBuffer = 0;
    nextInputBuffer = 0;

    TRACE(1, "%s CarrierAAC = %p\n", __FUNCTION__, CarrierAAC);
    TRACE(1, "%s infXchgAAC = %p\n", __FUNCTION__, infXchgAAC);

    init_waitqueue_head(&w8Q);
    pnx0106_epics_request_irq(aac_isr, 0);

    /* Send the init */
    infXchgAAC[0] = AACDEC_INIT;
    TRACE(2, "%s INIT command send\n", __FUNCTION__);
    pnx0106_epics_interrupt_req();
    TRACE(2, "%s interrupt_req called, about to wait_until_ready\n", __FUNCTION__);
    WAIT_UNTIL_READY ();

    TRACE(2, "%s decoder initialized\n", __FUNCTION__);

    input_eof = 0;
    pass_db = 0;
    mini_input_buf_len = 0;

    return 0;
}

/*************************************************************************
 * Function            : aacdecDeInit
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 aacdecDeInit (void)
{

    pnx0106_epics_free_irq();
    TRACE(2, "%s\n", __FUNCTION__);
    return 0;
}


/*************************************************************************
 * Function            : aacdecReset
 * Description         : Will be called when seeking (reset coeff)
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 aacdecReset (void)
{
    TRACE(2, "%s\n", __FUNCTION__);

    /* Send the Reset command */
    infXchgAAC[0] = AACDEC_RESET;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY ();

    input_eof = 0;
    mini_input_buf_len = 0;
    inputOffset = 0;
    inputBufferUsed = 0;
    return 0;
}

/*************************************************************************
 * Function            : aacdecStreamStart
 * Description         : Sets up streaming functionality
 *************************************************************************/
int
aacdecStreamStart(void)
{
    printk("call aacdecStreamStart\n");

    currentInputBuffer = decInputBuffers[0];
    inputOffset = 0;
    inputBufferUsed = 0;
    currentOutputBuffer = decOutputBuffers[0];
    nextInputBuffer = 0;
    nextOutputBuffer = 0;

    return aacstartCmd(AACDEC_STREAM);
}

/*************************************************************************
 * Function            : aacdecStreamResume
 * Description         : Resumes streaming functionality
 *************************************************************************/
int
aacdecStreamResume(void)
{
    printk("call aacdecStreamResume\n");
    currentInputBuffer = decInputBuffers[0];
    inputOffset = 0;
    inputBufferUsed = 0;
    currentOutputBuffer = decOutputBuffers[0];
    nextInputBuffer = 0;
    nextOutputBuffer = 0;
    return aacstartCmd(AACDEC_STREAM_RESUME);
}
/*************************************************************************
 * Function            : aacdecStreamFinish
 * Description         : seeks in stream
 *************************************************************************/
void
aacdecStreamFinish(u32 *pDecReturn)
{
    int numoutbufs;
    int numinbufs;
    int phase;
    int bytesconsumed;
    int skipcount;

    printk("call aacdecStreamFinish\n");
    *pDecReturn = CarrierAAC[0];
    phase = CarrierAAC[1];
    numoutbufs = CarrierAAC[2];
    skipcount = CarrierAAC[3];
    numinbufs = CarrierAAC[4];
    bytesconsumed = CarrierAAC[5];

    pnx0106_epics_printk("Streaming ended error %d in phase %d skipped %d\n", *pDecReturn, phase, skipcount);
    pnx0106_epics_printk("\toutput: %d buffers produced\n", numoutbufs);
    pnx0106_epics_printk("\tinput: %d buffers consumed plus %d byte\n", numinbufs, bytesconsumed);
    dump_debug_log();

    // Need to reset input_eof and pass_db for any future operations ..
    input_eof = 0;
    pass_db = 0;
    mini_input_buf_len = 0;
}

/*************************************************************************
 * Function            : aacdecStreamGetPCM
 *************************************************************************/
void
aacdecStreamGetPCM(e7b_aac_header *pFrameInfo,
       	u32* pSamplesReturned,
       	s16 *pDecoded,
       	u32 *pDecReturn)
{
    u8* pSource = (u8*)pnx0106_epics_xram2arm_2x24((u32)(currentOutputBuffer->bufferStart));
    unsigned long flags;
    unsigned int noOfBytes;
    unsigned int owner;
    AAC_HeaderInfo_t volatile *pHeader;
    extern interval_t aactimeWait;

    if ((u32)pDecoded & 0x3)
	WARN("pDecoded %p not a multiple of 4 !!!", pDecoded);

    printk("Call GetPCM ...\n");
    show_aac_buffers();
    printk("currentOutputBuffer = %x\n", (int)currentOutputBuffer);
    printk("nextOutputBuffer = %d\n", nextOutputBuffer);
    owner = currentOutputBuffer->bufferOwner;
    printk("currentOutputBuffer->bufferOwner = %x\n", owner);

    // Check if I own current buffer
    if (owner != ARM_OWNS_BUFFER)
    {
	int ret;

	printk("currentOutputBuffer is still owned by EPICS\n");

        // Check if EPICS needs data i.e ARM owns input buffer
        if (currentInputBuffer->bufferOwner == ARM_OWNS_BUFFER)
        {
            // Need to return
	        printk("currentInputBuffer is owned by ARM, returning\n");
            *pSamplesReturned = 0;
            *pDecReturn = 0;
            return;
        }

	// Wait till we either own an input buffer, output buffer, or
	// The Epics decided to finish the command.
	INTERVAL_START(aactimeWait);
	ret = wait_event_interruptible(w8Q, infXchgAAC[0] != AACDEC_STREAM ||
		currentInputBuffer->bufferOwner == ARM_OWNS_BUFFER ||
		currentOutputBuffer->bufferOwner == ARM_OWNS_BUFFER);
	INTERVAL_STOP(aactimeWait);

	if (infXchgAAC[0] != AACDEC_STREAM ||
		(currentInputBuffer->bufferOwner != ARM_OWNS_BUFFER &&
		currentOutputBuffer->bufferOwner != ARM_OWNS_BUFFER))
	    pnx0106_epics_printk("&&&&&&&&&&&&& ret %d infXchg %x input %d output %d\n", ret,
		    infXchgAAC[0] , currentInputBuffer->bufferOwner,
		    currentOutputBuffer->bufferOwner);

	if (ret < 0)
	    printk("interrupted epics wait\n");

	if (ret == 0)
	    printk("wait_event timeout\n");


	// Need to check and see which buffer has changed ownership
	// if is output buffer can continue
	// if input buffer => need to return
	if (currentOutputBuffer->bufferOwner != ARM_OWNS_BUFFER)
	{
	    // Need to return
	    *pSamplesReturned = 0;

	    if (infXchgAAC[0] == 0)
		*pDecReturn = CarrierAAC[0];
	    else
		*pDecReturn = 0;

	    return;
	}
    }

    // Copy FrameInfo
    pHeader = &currentOutputBuffer->headerInfo;
    pFrameInfo->samplingFreqHz =    pHeader->AACD_samplingFreqHz;
    pFrameInfo->nrChan = 	  pHeader->AACD_nrChan;
    pFrameInfo->transportFormat =      pHeader->AACD_transportFormat;


    g_AACNumChannels = pFrameInfo->nrChan;

    noOfBytes = currentOutputBuffer->bufferLengthInBytes;
    printk("currentOutputBuffer->bufferLengthInBytes = %d\n", noOfBytes);
    printk("currentOutputBuffer->bufferSizeInBytes = %d\n", currentOutputBuffer->bufferSizeInBytes);

    // check length
    if (noOfBytes != 0)
    {
	if (noOfBytes <= currentOutputBuffer->bufferSizeInBytes)
	{
	    // Copy
	    local_irq_save(flags);
	    pnx0106_epics_transfer_32_2x16();

	    memcpy(pDecoded, pSource, noOfBytes);

	    pnx0106_epics_transfer_32_24();
	    local_irq_restore(flags);
	}
	else
	    pnx0106_epics_printk("*********** bufferLengthInBytes is hosed %d, %d\n", noOfBytes, currentOutputBuffer->bufferSizeInBytes);
    }

    // Fill out decoder return and no of samples returned
    *pDecReturn = currentOutputBuffer->bufferFlags;

    if (g_AACNumChannels == 0)
	*pSamplesReturned = 0;
    else
	*pSamplesReturned = ((int)currentOutputBuffer->bufferLengthInBytes) / (2 * g_AACNumChannels);


    // Pass control to Epics
    currentOutputBuffer->bufferOwner = EPICS_OWNS_BUFFER;
    currentOutputBuffer->armDebug++;

    // Get Next buffer
    assignNextOutputBuffer();

}

/*************************************************************************
 * Function            : aacdecStreamRestore
 * Description         : Restores current setup and resumes
 *************************************************************************/
int
aacdecStreamRestore(void)
{
    int i;

    pnx0106_epics_printk("call aacdecStreamRestore\n");


    currentInputBuffer = (decInputBufferInfo_t *)haltSaveInfo.currentInputBufferAddr;
    currentOutputBuffer = (decOutputBufferInfo_t *)haltSaveInfo.currentOutputBufferAddr;

    nextInputBuffer = haltSaveInfo.nextInputBuffer;
    nextOutputBuffer = haltSaveInfo.nextOutputBuffer;

    w8Q = haltSaveInfo.w8Q;

    pass_db = haltSaveInfo.pass_db;
    mini_input_buf_len = haltSaveInfo.mini_input_buf_len;
    for(i=0;i<12;i++)
    {
        mini_input_buf[i] = haltSaveInfo.mini_input_buf[i];
    }
    input_eof = haltSaveInfo.input_eof;
    epics_busy = haltSaveInfo.epics_busy;
    g_AACNumChannels = haltSaveInfo.g_AACNumChannels;
    DecodedFrames = haltSaveInfo.DecodedFrames;
    inputOffset = haltSaveInfo.inputOffset;
    inputBufferUsed = haltSaveInfo.inputBufferUsed;

    printk("currentInputBuffer = %x\n",currentInputBuffer);
    printk("currentInputBuffer->bufferOwner = %x\n", currentInputBuffer->bufferOwner);
    printk("currentInputBuffer->bufferFlags = %x\n", currentInputBuffer->bufferFlags);

    printk("currentOutputBuffer = %x\n",currentOutputBuffer);
    printk("currentOutputBuffer->bufferOwner = %x\n", currentOutputBuffer->bufferOwner);
    printk("currentOutputBuffer->bufferFlags = %x\n", currentOutputBuffer->bufferFlags);

    printk("nextOutputBuffer = %x\n",nextOutputBuffer);
    printk("nextInputBuffer = %x\n",nextInputBuffer);


    CarrierAAC[0]=0;

    return pnx0106_epics_request_irq(aac_isr, 0);

}

/*************************************************************************
 * Function            : aacdecStreamSave
 * Description         : Saves current setup
 *************************************************************************/
int
aacdecStreamSave(void)
{
    int i;

    pnx0106_epics_printk("call aacdecdecStreamSave\n");


    haltSaveInfo.currentInputBufferAddr = (int)currentInputBuffer;
    haltSaveInfo.currentOutputBufferAddr = (int)currentOutputBuffer;

    haltSaveInfo.nextInputBuffer = nextInputBuffer;
    haltSaveInfo.nextOutputBuffer = nextOutputBuffer;

    haltSaveInfo.w8Q = w8Q;

    haltSaveInfo.pass_db = pass_db;
    haltSaveInfo.mini_input_buf_len = mini_input_buf_len;
    for(i=0;i<12;i++)
    {
        haltSaveInfo.mini_input_buf[i] = mini_input_buf[i];
    }
    haltSaveInfo.input_eof = input_eof;
    haltSaveInfo.epics_busy = epics_busy;
    haltSaveInfo.g_AACNumChannels = g_AACNumChannels;
    haltSaveInfo.DecodedFrames = DecodedFrames;
    haltSaveInfo.inputOffset = inputOffset;
    haltSaveInfo.inputBufferUsed = inputBufferUsed;

    printk("currentInputBuffer = %x\n",currentInputBuffer);
    printk("currentInputBuffer->bufferOwner = %x\n", currentInputBuffer->bufferOwner);
    printk("currentInputBuffer->bufferFlags = %x\n", currentInputBuffer->bufferFlags);

    printk("currentOutputBuffer = %x\n",currentOutputBuffer);
    printk("currentOutputBuffer->bufferOwner = %x\n", currentOutputBuffer->bufferOwner);
    printk("currentOutputBuffer->bufferFlags = %x\n", currentOutputBuffer->bufferFlags);

    printk("nextOutputBuffer = %x\n",nextOutputBuffer);
    printk("nextInputBuffer = %x\n",nextInputBuffer);

    CarrierAAC[0]=0;

    pnx0106_epics_free_irq();

    return 0;
}

