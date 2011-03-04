
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
#include <asm/arch/interval.h>
#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/wma9.h>

/*----------------------------------------------------------------------------
   Typedefs:
   --------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
   Static members and defines:
   --------------------------------------------------------------------------*/
static u32 block_start; /* returned by decoder but not really used yet */
static u32 nr_decoded_samples; /* not really used yet */

static u32 counterPCM = 0;

/* Static globals for data passing between epics and Arm */
static volatile u32* carrier;	/* parameter list for epics commands */
static volatile u32* infXchg;	/* command/return register */
static volatile u32* pWMABuffer; /* Input buffer for Epics */
static volatile u32* pPCMBuffer; /* Output buffer for Epics */

static int drvr_waiting = 0;
static int isr_done = 0;
static int epics_busy = 0;
static int current_nchannels = 2;

interval_t timeDecoderWait;
interval_t timeWait2Isr;
interval_t timeIsr2Wait;
interval_t timeIsr2Wakeup;

/* some WMA codes required for epics decoder */
#define WMA_OK                 0x00000000
#define WMA_S_NEWPACKET        0x00000003
#define WMA_E_NO_MORE_SRCDATA  0xFF840005

/* MAX input size for WMA decoder (defined in DecodersApp.c on Epics) */
#define WMA_MAX_BYTES 128		/* each byte will be copied to 1 epics register */

#define DLOG(L,X) TRACE(L, "%s=%d\n", #X, X)

#define USE_IRQ		/* else polling */
#ifdef USE_IRQ
static wait_queue_head_t 	w8Q;

static int event_callback(void* data)
{
	(void)data;	/* not used */

	if (drvr_waiting)
	    INTERVAL_STOP(timeWait2Isr);
	else
	    INTERVAL_START(timeIsr2Wait);

	if (!isr_done)
	{
	    INTERVAL_START(timeIsr2Wakeup);
	    isr_done = 1;
	}

	if (epics_busy) {
		epics_busy = 0;
		wake_up_interruptible(&w8Q);  /* wakeup */
	}
	return 0;
}
#endif

/*************************************************************************
 * Function            : WAIT_UNTIL_READY
 *************************************************************************/
static inline void WAIT_UNTIL_READY (int condition, int sleeptime)
{
	u32 data;
    do {
        data = infXchg[0];

	    if( data != condition ) {
            pnx0106_epics_sleep( sleeptime );
        }
    }  while ( data != condition );
}

/*************************************************************************
 * Function            : wma9decNew
 *************************************************************************/
void wma9decNew (void)
{
    TRACE(1, "%s\n",__FUNCTION__);

    /* set default transfer mode */
    pnx0106_epics_transfer_32_24();

   	/* Translate epics addresses */
    carrier    = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    pWMABuffer = pnx0106_epics_xram2arm(pnx0106_epics_getlabel("DecoderInput", EPX_DECODERINPUT));
    pPCMBuffer = pnx0106_epics_xram2arm_2x24(pnx0106_epics_getlabel("DecoderOutputL", EPX_DECODEROUTPUTL));
    infXchg    = pnx0106_epics_ctr2arm(pnx0106_epics_infxchg());

    TRACE(1,"carrier = %p\n", carrier);
    TRACE(1,"pWMABuffer = %p\n", pWMABuffer);
    TRACE(1,"pPCMBuffer = %p\n", pPCMBuffer);
    TRACE(1,"infXchg = %p\n", infXchg);

    block_start     = 0;
    nr_decoded_samples  = 0;

    counterPCM = 0;

#ifdef USE_IRQ
    init_waitqueue_head(&w8Q);
    pnx0106_epics_request_irq(event_callback, 0);
#endif
}

/*************************************************************************
 * Function            : epicsSendParam
 *************************************************************************/
static void epicsSendParam (e7b_wma_format* wma_format, e7b_wma_pcm_format* pcm_format, e7b_wma_player_info* player_info)
{
    TRACE(1, "%s\n",__FUNCTION__);
    carrier[0]  = wma_format->wFormatTag;
    carrier[1]  = wma_format->nChannels;
    carrier[2]  = wma_format->nSamplesPerSec;
    carrier[3]  = wma_format->nAvgBytesPerSec;
    carrier[4]  = wma_format->nBlockAlign;
    carrier[5]  = wma_format->nValidBitsPerSample;
    carrier[6]  = wma_format->nChannelMask;
    carrier[7]  = wma_format->wEncodeOpt;

    carrier[8]  = pcm_format->nSamplesPerSec;
    carrier[9]  = pcm_format->nChannels;
    carrier[10] = pcm_format->nChannelMask;
    carrier[11] = pcm_format->nValidBitsPerSample;
    carrier[12] = pcm_format->cbPCMContainerSize;
    carrier[13] = pcm_format->pcmData;

    carrier[14] = player_info->nPlayerOpt;
	carrier[15] = player_info->rgiMixDownMatrix;// ? (*player_info->rgiMixDownMatrix) : 0;
    carrier[16] = player_info->iPeakAmplitudeRef;
    carrier[17] = player_info->iRmsAmplitudeRef;
    carrier[18] = player_info->iPeakAmplitudeTarget;
    carrier[19] = player_info->iRmsAmplitudeTarget;
    carrier[20] = player_info->nDRCSetting;

#ifdef DEBUG
    DLOG(2, wma_format->wFormatTag);
    DLOG(2, wma_format->nChannels);
    DLOG(2, wma_format->nSamplesPerSec);
    DLOG(2, wma_format->nAvgBytesPerSec);
    DLOG(2, wma_format->nBlockAlign);
    DLOG(2, wma_format->nValidBitsPerSample);
    DLOG(2, wma_format->nChannelMask);
    DLOG(2, wma_format->wEncodeOpt);

    DLOG(2, pcm_format->nSamplesPerSec);
    DLOG(2, pcm_format->nChannels);
    DLOG(2, pcm_format->nChannelMask);
    DLOG(2, pcm_format->nValidBitsPerSample);
    DLOG(2, pcm_format->cbPCMContainerSize);
    DLOG(2, pcm_format->pcmData);

    DLOG(2, player_info->nPlayerOpt);
	DLOG(2, player_info->rgiMixDownMatrix);// ? (*player_info->rgiMixDownMatrix) : 0)
    DLOG(2, player_info->iPeakAmplitudeRef);
    DLOG(2, player_info->iRmsAmplitudeRef);
    DLOG(2, player_info->iPeakAmplitudeTarget);
    DLOG(2, player_info->iRmsAmplitudeTarget);
    DLOG(2, player_info->nDRCSetting);
#endif

    /* Send the command */
    infXchg[0] = WMAAUDIODEC_SENDPARAMETERS;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY (0, 0);
}


/*************************************************************************
 * Function            : epicsNew
 *************************************************************************/
static u32 epicsNew (e7b_wma_stream_info *userData)
{
	TRACE(1,"%s\n", __FUNCTION__);

    /* start DSP (TODO shouldn't this be done after loading) */
	pnx0106_epics_set_ready();

    /* Send the NEW command (parameters in comm buffer) */
    carrier[0] = userData->version;
    carrier[1] = userData->samplesPerSec;
    carrier[2] = userData->channels;
    carrier[3] = userData->blockAlign;
    carrier[4] = userData->avgBytesPerSec;
    carrier[5] = userData->encodeOpt;
    carrier[6] = userData->samplesPerBlock;
    infXchg[0] = WMAAUDIODEC_NEW;

    pnx0106_epics_interrupt_req();

    WAIT_UNTIL_READY (WMAAUDIODEC_WAITFORINIT, 0); /* Clear Decoder is being done now */

#if 0
	/*
	 * Patch XMEM and YMEM (fix decoder bug).
	 * Code is in wma_mem_patch.c (?move to userspace/firmware-api)
	 */
	{
		extern const unsigned long xmem_patch[];
		extern const unsigned long ymem_patch[];
		extern const unsigned long xmem_patch_start;
		extern const unsigned long ymem_patch_start;
		extern const unsigned long xmem_patch_size;
		extern const unsigned long ymem_patch_size;

		TRACE(1, "patching XMEM & YMEM for wma\n");
		memcpy((u32*)pnx0106_epics_xram2arm(xmem_patch_start), xmem_patch, xmem_patch_size);
		memcpy((u32*)pnx0106_epics_yram2arm(ymem_patch_start), ymem_patch, ymem_patch_size);
	}
#endif

	/* Send the InitReady */
    infXchg[0] = WMAAUDIODEC_INITREADY;
    WAIT_UNTIL_READY (0, 0);

    /* Read back the result */
    return carrier[0];
}

/*************************************************************************
 * Function            : wma9decInit
 *************************************************************************/
int wma9decInit (e7b_wma_format* wma_format,
                     e7b_wma_pcm_format* pcm_format,
                     e7b_wma_player_info *player_info,
                     u16 *paudecState,
                     e7b_wma_stream_info *userData)
{
    TRACE(1,"%s\n",__FUNCTION__);

    /* Do the epicsNew now that we have the input params */
    if (epicsNew(userData) == 0) {
		TRACE(1,"WMA NEW Failed\n");
	        ERRMSG("WMA NEW failed\n");
		return -1;
	}

    /* Pass the given parames to epics */
    epicsSendParam(wma_format, pcm_format, player_info);
	current_nchannels = wma_format->nChannels;

    /* Send the init */
	infXchg[0] = WMAAUDIODEC_INIT;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY (0, 0);

    /* Read back the result */
    block_start    = carrier[0];
    *paudecState   = carrier[1];

    /* Give back the state var */
	TRACE(3, "after init state=%d\n", *paudecState);

	return 0;
}

/*************************************************************************
 * Function            : wma9decDecode
 *************************************************************************/
void wma9decDecode ( u16* pcSamplesReady, u16* paudecState)
{
#ifndef USE_IRQ
    cycles_t prev_cmd_time = 0;
    int		 check_responsiveness = 0;
#endif
    wma9decInputBuf inbuf;
    u8* pWMARead = NULL;
//    u32 it = 0;
    int  WMA9D_wmaparseResult = WMA_OK;
	u32 data;

    /* Clear the callback structure that will be given to the decoder */
    memset( (char*) &inbuf, 0, sizeof( inbuf ) );

    /* Start decode cycle */
	TRACE(3, "start decoder state=%d\n", *paudecState);
	epics_busy = 1;
	isr_done = 0;
	drvr_waiting = 0;
    carrier[0] = *paudecState;
    infXchg[0] = WMAAUDIODEC_DECODE;
    pnx0106_epics_interrupt_req();

    /* Check for callbacks */
    do {
        data = infXchg[0];
        if( data == WMAAUDIODEC_GETINPUT ) {
			TRACE(3, "decoder needs data\n");
            /* We got a callback from the decoder */
            /* get the data transferred */
            (void)wma9decGetMoreData( &inbuf );
			if (inbuf.size > WMA_MAX_BYTES) {
				ERRMSG("parser provided too much input: %d > %d (truncated)\n", (int)inbuf.size, WMA_MAX_BYTES);
				inbuf.size = WMA_MAX_BYTES;
			}

            /* Give the info from the callback to the epics */

            /* Give parseResult */
            WMA9D_wmaparseResult = WMA_OK;
            if( inbuf.newPacket ) {
                WMA9D_wmaparseResult = WMA_S_NEWPACKET;
            }
            if( inbuf.noMoreInput ) {
                WMA9D_wmaparseResult = WMA_E_NO_MORE_SRCDATA;
            }

            /* Give input parameters before giving data */
            carrier[0] = WMA9D_wmaparseResult;
            carrier[1] = inbuf.size; /* bufferlength */

            /* Give buffer to epics */
            /* One byte in ARM is copied to a 32bit word in EPICS */
            //pWMARead = inbuf.data;
            //for( it = 0; it < inbuf.size; it++ ) {
            //    pWMABuffer[it] = pWMARead[it];
            //}

            // Need to transfer data correctly to Epics - lossless mode
            // This code is stolen from the reference design
            /* Give buffer to epics */
            pWMARead = inbuf.data;

            if ( inbuf.size != 0 )
            {
                u32* pOut;
                u32 it;
                u32 temp;
                pnx0106_epics_transfer_3x32_4x24();
                pOut = (u32*) pWMABuffer;
                if ( ((u32)pWMARead & 0x3) == 0 )
                {
                    // Use 32 bits transfer when input buffer is aligned on 32 bits
                    // memcpy does not work. (maybe due to incompatibility with multiple load/store bytes)
                    it = (inbuf.size-1)/4;
                    do
                    {
                        *pOut++ = *((u32*)pWMARead);
                        pWMARead+=4;
                    }
                    while( it-- != 0 );
                }
                else
                {
                    it = 0;
                    while( it < inbuf.size )
                    {
                              temp=*pWMARead++;
                              temp|=(*pWMARead++)<<8;
                              temp|=(*pWMARead++)<<16;
                              // 32 bits transfert mode must be use in lossless_32_to_24 mode
                              *pOut++= temp|((*pWMARead++)<<24);
                              it+=4;
                    }
                }
                *pOut=0;        // flush the internal saved byte during lossloess_32_to_24 transfer
                pnx0106_epics_transfer_32_24();
            }

            /* Indicate that the wma data is available for the epics decoder */
			epics_busy = 1;  /* it has new data to process */
            infXchg[0] = WMAAUDIODEC_INPUTREADY;
#ifndef USE_IRQ
			prev_cmd_time = get_cycles();
			check_responsiveness = 1;
        } else if (data == WMAAUDIODEC_INPUTREADY && check_responsiveness) { /* check decoder responsiveness */
			/* this is just a tmp hack until interrupt mode gets implemented in epics decoder */
			cycles_t now = get_cycles();
			u32 diff = (now >= prev_cmd_time) ? (now - prev_cmd_time) : (0xFFFFFFFF - prev_cmd_time + now);
			TRACE(6, "diff=%u now=%u prev=%u\n", diff, (u32)now, (u32)prev_cmd_time);
			if (diff > 100) {
				TRACE(4, "sleep@%u:%u-%u\n", (u32)now, diff, (u32)prev_cmd_time);
				pnx0106_epics_sleep(4); /* note: will only be usefull when kernel is compiled with HZ>=250 */
				check_responsiveness = 0; /* only sleep once per input */
			}
#else
        } else if (epics_busy && data == WMAAUDIODEC_INPUTREADY) {
			cycles_t now = get_cycles();
			cycles_t nap_start = now;
			u32 diff;

			INTERVAL_START(timeDecoderWait);

			if (isr_done)
			    INTERVAL_STOP(timeIsr2Wait);
			else
			{
			    INTERVAL_START(timeWait2Isr);
			    drvr_waiting = 1;
			}

			if (wait_event_interruptible(w8Q, !epics_busy) < 0) {
				TRACE(1, "interrupted epics wait\n");
			}

			drvr_waiting = 0;

			if (isr_done)
			{
			    INTERVAL_STOP(timeIsr2Wakeup);
			    isr_done = 0;
			}

			INTERVAL_STOP(timeDecoderWait);
			now = get_cycles();
			diff = (now >= nap_start) ? (now - nap_start) : (0xFFFFFFFF - nap_start + now);
			TRACE(5, "slept %u\n", diff);
#endif
		}
	else
	    schedule();
    } while (data != 0);

	epics_busy = 0;

    /* Read back the result */
    *pcSamplesReady    = carrier[0];
    nr_decoded_samples = carrier[1]; /* TBD what to do with this info ??? */
    *paudecState       = carrier[2];

	//if (*paudecState == 1) {
	//	TRACE(1, "Decode: fixing decoder state==1\n");
	//	*paudecState = 2; /* HACK arround decoder bug */
	//}

	TRACE(3, "decoder result: sr=%u nd=%u ds=%d\n", *pcSamplesReady, nr_decoded_samples, *paudecState);
}

/*************************************************************************
 * Function            : wma9decGetPCM
 *************************************************************************/
void wma9decGetPCM (   u16 cSamplesRequested,
                       u16 *pcSamplesReturned,
                       u8* pbDst,
                       u16 cbDstLength,
                       u16 *paudecState)
{
	if ((u32)pbDst & 0x3) {
		WARN("pbDst %p not a multiple of 4 !!!", pbDst);
	}

    carrier[0] = cSamplesRequested;
    carrier[1] = cbDstLength;
    carrier[2] = *paudecState;

    /* call epics GetPCM() */
    infXchg[0] = WMAAUDIODEC_GETPCM;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY (0, 0);

    /* Read back the result */
    *pcSamplesReturned = carrier[0];
    *paudecState       = carrier[1];
	//if (*paudecState == 1) {
	//	TRACE(1, "GetPCM: fixing decoder state==1\n");
	//	*paudecState = 2; /* HACK arround decoder bug */
	//}

    /* Copy the decoded data from the epics to arm */
	{	/* protected area */
		static const unsigned int epics_bufsize = 0x2000; /* in bytes */
		unsigned long flags;
		unsigned long bytes_returned = *pcSamplesReturned * 2 * current_nchannels;
		u8* pSource = (u8*)pPCMBuffer;

		local_irq_save(flags);
		pnx0106_epics_transfer_32_2x16();

		/* Copy pcm samples in a fast way */
		if(( counterPCM + bytes_returned) > epics_bufsize) { /* We will reach the end of the circular buffer*/
			unsigned int remainder = (epics_bufsize - counterPCM);

			/* First copy the part till the end of the buffer */
			memcpy( pbDst, &pSource[counterPCM], remainder );

			/* Then copy the rest from the beginning of the buffer */
			memcpy( &pbDst[epics_bufsize - counterPCM], pSource, (int)(bytes_returned - remainder) );

			/* And update the source Pointer */
			counterPCM = bytes_returned - remainder;
		} else {
			/* Just copy from the source pointer, will not go over the end */
			memcpy( pbDst, &pSource[counterPCM], (int)bytes_returned );

			/* And update the source Pointer */
			counterPCM += bytes_returned;
		}

		pnx0106_epics_transfer_32_24 ();
		local_irq_restore(flags);
	}

    /* Give command to update output pointers, in case something was returned */
    if( *pcSamplesReturned ) {
        infXchg[0] = WMAAUDIODEC_UPDATEPOINTERS;
        pnx0106_epics_interrupt_req();
        WAIT_UNTIL_READY (0, 0);
    }
}

/*************************************************************************
 * Function            : wma9decDelete
 * Description         : Will be called at end of decoding
 *************************************************************************/
void wma9decDelete (void)
{
    /* Send the Delete command */
    infXchg[0] = WMAAUDIODEC_DELETE;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY (0, 0);

#ifdef USE_IRQ
    pnx0106_epics_free_irq();
#endif
}

/*************************************************************************
 * Function            : wma9decReset
 * Description         : Will be called when seeking (reset coeff)
 *************************************************************************/
void wma9decReset ()
{
	int decoder_state;

    /* Send the Reset command */
    infXchg[0] = WMAAUDIODEC_RESET;
    pnx0106_epics_interrupt_req();
    WAIT_UNTIL_READY (0, 0);

    /* Read back the result */
    decoder_state = carrier[0]; /* ? what to do with it */

	/* checked: we don't need to reset counterPCM here, it keeps on counting */
    //counterPCM = 0;
}

