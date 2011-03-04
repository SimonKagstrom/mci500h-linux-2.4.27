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

#ifdef DEBUG
#define printk	epics_printk
#else
#define printk(fmt...)
#endif

#include <asm/arch/epics.h>
#include <asm/arch/epicsfw.h>
#include <asm/arch/mp3.h>

//#include "epics.h"  	/* utility functions for epics: e7b_...	*/
//#include "epicsAPI.h"	/* API (defines) of epics application 	*/

//#include <asm/arch/vhal/audioss_creg.h>

/*----------------------------------------------------------------------------
   Typedefs:
   --------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
   Static members and defines:
   --------------------------------------------------------------------------*/

//#define GRANULE_BASED                /* determined by epics code */

static volatile u32 dataMP3;
static u32 g_MP3NumChannels = 0;
static u32 DecodedFrames = 0;
#ifdef GRANULE_BASED
static u32 nrOfGranules = 0;
#endif

/* Static globals for data passing between epics and Arm */
static volatile u32* CarrierMP3;
static volatile u32* infXchgMP3;

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
		dataMP3 = infXchgMP3[0];
		TRACE(5, "WAIT_UNTIL_READY: dataMP3 0x%X\n", dataMP3);
		if( dataMP3 != 0 ) {
			e7b_sleep( 0 ); /* will just schedule so other threads can still run */
		}
	} while ( dataMP3 != 0 );
}

/*************************************************************************
 * Function            : mp3decInit
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decInit ()
{
    TRACE(2, "%s\n", __FUNCTION__);

    if (e7b_reload_firmware() < 0) {
	    return -1;
    }
    TRACE(2, "%s firmware reloaded\n", __FUNCTION__);

    /* set the default transfer mode */
    e7b_transfer_32_24();
    TRACE(2, "%s transfer mode set\n", __FUNCTION__);

    /* allow DSP to start */
    e7b_set_ready();
    TRACE(2, "%s DSP started\n", __FUNCTION__);

    /* Translate epics addresses for commands passing */
    CarrierMP3 = xram2arm(pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    infXchgMP3 = ctr2arm(pnx0106_epics_infxchg());

    epics_printk("EPX_INFXCHG %x\n", pnx0106_epics_pp_getlabel("INFXCHG", EPX_INFXCHG));
    epics_printk("EPX_DECODERCARRIER %x\n", pnx0106_epics_getlabel("DecoderCarrier", EPX_DECODERCARRIER));
    epics_printk("EPX_DECODERINPUT %x\n", pnx0106_epics_getlabel("DecoderInput", EPX_DECODERINPUT));
    epics_printk("EPX_DECODEROUTPUTL %x\n", pnx0106_epics_getlabel("DecoderOutputL", EPX_DECODEROUTPUTL));
    epics_printk("EPX_DECODEROUTPUTR %x\n", pnx0106_epics_getlabel("DecoderOutputR", EPX_DECODEROUTPUTR));

    TRACE(2, "%s CarrierMP3 %p infXchgMP3 %p\n", __FUNCTION__, CarrierMP3, infXchgMP3);

    /* Send the init */
    infXchgMP3[0] = MP3DEC_INIT;
    TRACE(2, "%s INIT command send\n", __FUNCTION__);
    e7b_interrupt_req();
    TRACE(2, "%s interrupt_req called, about to wait_until_ready\n", __FUNCTION__);
    WAIT_UNTIL_READY ();

    TRACE(2, "%s decoder initialized\n", __FUNCTION__);
    return 0;
}
/*************************************************************************
 * Function            : mp3decDeInit
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decDeInit (void)
{
	TRACE(2, "%s\n", __FUNCTION__);

    return 0;
}

/*************************************************************************
 * Function            : copy_input
 * Description         :
 *	Copies input from driver circular buffer to xram.
 *	Will put process to sleep when no input is available yet;
 *	Assumes it is called in MP3DEC_GETINPUT context;
 *	returns 1 iff all bytes that epics wanted were copied (0 is bug or user termination)
 *************************************************************************/
static int copy_input(void)
{
    int bytesAvail  = 0;
    int bytesRead   = 0;
    u32 bytesNeeded = 3 * CarrierMP3[0];
    u32 pInBufferEpics = (u32)xram2arm(CarrierMP3[1]);

    TRACE(5, "copy_input: CarrierMP3[0] %d [1] %d\n", CarrierMP3[0], CarrierMP3[1]);

	/* get data from the driver input queue (blocks until available) */
	TRACE(2, "epics needs %d bytes\n", bytesNeeded);
	bytesAvail = e7b_mp3_wait_for_input(bytesNeeded);
	if (bytesAvail < 0) { /* decode must stop ASAP */
		CarrierMP3[0] = 0;
		infXchgMP3[0] = MP3DEC_INPUTREADY;
		return 0;
	} else if (bytesAvail < bytesNeeded) {
		/* epics needs those bytes, if we don't have them, abort and reset epics
		 * (can't do that here since epics is in a loop waiting for INPUTREADY) */
		WARN("not enough data ready %d %d\n", bytesAvail, bytesNeeded);
	}
	bytesRead  = e7b_mp3_copy_input(bytesNeeded, (void*)pInBufferEpics);
	if ((bytesAvail == bytesNeeded) && (bytesRead < bytesNeeded)) { /* bug? */
		ERRMSG("not enough data read %d %d\n", bytesRead, bytesAvail);
	}

	/* Indicate that the new data is available */
	//TRACE(0, "-> %d (/%d)\n", bytesRead, bytesNeeded);
	CarrierMP3[0] = bytesRead / 3;
	infXchgMP3[0] = MP3DEC_INPUTREADY;
	if (bytesRead % 3 != 0) {
		WARN("bytes lost: %d (%d, %d)\n", bytesRead % 3, bytesRead, bytesNeeded);
	}

	return (bytesRead==bytesNeeded);
}

/*************************************************************************
 * Function            : mp3decSync
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decSync (u32* pOffset)
{
    u32 syncError = 0;

	TRACE(4, "%s\n", __FUNCTION__);

    /* Do the sync */
    infXchgMP3[0] = MP3DEC_SYNC;
    e7b_interrupt_req();

    /* Check if more input data is needed */
    do {
        dataMP3 = infXchgMP3[0];
        if( ( dataMP3 != 0 ) && ( dataMP3 != MP3DEC_GETINPUT ) ) {
            TRACE(5, "WAIT_UNTIL_READY: mp3decSync dataMP3 0x%X\n", dataMP3);
            e7b_sleep( 0 );
        } else if( dataMP3 == MP3DEC_GETINPUT ) {
			if (!copy_input()) /* caller should probably reset the decoder now */
				return MP3DEC_NO_SYNCWORD;
        }
    }  while ( dataMP3 != 0 );

    syncError = CarrierMP3[0];
    /* Calculate nr of bytes before sync, pointer is now after the sync word */
    /* bitoffset/8 + (byteoffset - bufferstart) * 3 bytes per word - 4 bytes per sync */
    *pOffset = (CarrierMP3[1] / 8) + (CarrierMP3[2] - CarrierMP3[3]) * 3 - 4;

    TRACE(5, "  sync: err=%d bito=%d byto=%d bs=%d  offset=%d\n", syncError, CarrierMP3[1], CarrierMP3[2], CarrierMP3[3], *pOffset);

#if 0
    {
	int InpBsBuf = CarrierMP3[3];
	u32 pInBufferEpics = (u32)xram2arm(InpBsBuf);

	if (InpBsBuf >= 0 && InpBsBuf < 65536)
	    TRACE(5, "        xmem[%d] %08x\n", InpBsBuf, *((u32 *)pInBufferEpics));
    }
#endif

    return syncError;
}


/*************************************************************************
 * Function            : mp3decGetHeaderInfo
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decGetHeaderInfo (e7b_mp3_header* pMP3Dec_info )
{
	TRACE(4, "%s\n", __FUNCTION__);

	/* Do the getHeaderInfo */
	infXchgMP3[0] = MP3DEC_HEADERINFO;
	e7b_interrupt_req();

	/* Check if more input data is needed */
	do {
		dataMP3 = infXchgMP3[0];
		if( ( dataMP3 != 0 ) && ( dataMP3 != MP3DEC_GETINPUT ) ) {
			e7b_sleep( 0 );
		} else if( dataMP3 == MP3DEC_GETINPUT ) {
			if (!copy_input())
				return MP3DEC_BROKEN_FRAME;
		}
	}  while ( dataMP3 != 0 );

	/* Read back the headerinfo */
	//    nrOfGranules =                CarrierMP3[0];
	pMP3Dec_info->samplesPerCh =    CarrierMP3[0];
	pMP3Dec_info->numChans = g_MP3NumChannels = CarrierMP3[1];
	pMP3Dec_info->freeFormat =      CarrierMP3[2];
	pMP3Dec_info->bitsRequired =    CarrierMP3[3];
	pMP3Dec_info->id =              CarrierMP3[4];
	pMP3Dec_info->layer =           CarrierMP3[5];
	pMP3Dec_info->protection =      CarrierMP3[6];
	pMP3Dec_info->bitRateIndex =    CarrierMP3[7];
	pMP3Dec_info->bitRateKBPS =     CarrierMP3[8];
	pMP3Dec_info->sampleRateIndex = CarrierMP3[9];
	pMP3Dec_info->sampleRateHz =    CarrierMP3[10];
	pMP3Dec_info->paddingBit =      CarrierMP3[11];
	pMP3Dec_info->privateBit =      CarrierMP3[12];
	pMP3Dec_info->mode =            CarrierMP3[13];
	pMP3Dec_info->modeExtension =   CarrierMP3[14];
	pMP3Dec_info->copyright =       CarrierMP3[15];
	pMP3Dec_info->originalHome =    CarrierMP3[16];
	pMP3Dec_info->emphasis =        CarrierMP3[17];

#ifdef GRANULE_BASED
	if( pMP3Dec_info->samplesPerCh == 1152 ) {
		nrOfGranules = 2;
	} else {
		nrOfGranules = 1;
	}
        TRACE(1, "nrOfGranules=%d spc=%lu\n", nrOfGranules, pMP3Dec_info->samplesPerCh);
#endif

	return MP3DEC_NO_ERR;
}

/*************************************************************************
 * Function            : copy_output
 * Description         :
 *	Copies pcm from epics L&R buffers to interleaved output buffer
 *************************************************************************/
static s16* copy_output(int numChannels, int samplesPerChannel, u32 epicsL, u32 epicsR, s16 *outptr)
{
    u32 it;
	unsigned long flags;
	volatile u32* armL = xram2arm_2x24(epicsL);
	volatile u32* armR = xram2arm_2x24(epicsR);

	/* Use the optimized PCM transfer mode */
	/* prevent interrupts since transfer mode is a global setting */
	local_irq_save(flags);
	e7b_transfer_32_2x16();

	if( numChannels > 1 ) { /* stereo -> interleave the samples */
		/* process 2 stereo samples (8 bytes) per loop */
		for( it = 0; it < samplesPerChannel/2; it++ ) {
			u32 left2  = *armL++;
			u32 right2 = *armR++;
			*outptr++ = (s16)(left2  & 0xFFFF);
			*outptr++ = (s16)(right2 & 0xFFFF);
			*outptr++ = (s16)(left2  >> 16);
			*outptr++ = (s16)(right2 >> 16);
		}
	} else { /* mono */
		/* process 2 mono samples (4 bytes) per loop */
		for( it = 0; it < samplesPerChannel/2; it++ ) {
			u32 left2  = *armL++;
			*outptr++ = (s16)(left2  & 0xFFFF);
			*outptr++ = (s16)(left2  >> 16);
		}
	}

	e7b_transfer_32_24 (); /* back to default mode */
	local_irq_restore(flags);

	return outptr;
}

/*************************************************************************
 * Function            : mp3decDecode
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decDecode ( u32* pTotSamplesPerCh, s16* pDecoded)
{
    u32 decodeError = 0;
    u32 inBuffer = 0;
    u32 wordsInBuffer = 0;
    u32 samplesPerChannel = 0;
	s16* outptr = pDecoded;

	TRACE(4, "%s\n", __FUNCTION__);

    *pTotSamplesPerCh = 0;

#ifdef GRANULE_BASED
    while( nrOfGranules--)
#endif
	{
      int max_delay = 21;        /* estimated completion time */
        /* Do the decoding */
        infXchgMP3[0] = MP3DEC_GETPCM;
        e7b_interrupt_req();

        do {
            dataMP3 = infXchgMP3[0];
            if( ( dataMP3 != 0 ) && ( dataMP3 != MP3DEC_GETINPUT ) ) {
				/* Let the decoder thread sleep a bit since this drastically lowers cpu usage (>20%)
				 * Default is one tick (10 ms) since it is fast enough for playback.
				 * It can be changed by an insmod parameter and an ioctl call to increase decode time */
                e7b_sleep( max_delay >= e7b_mp3_decode_delay ? e7b_mp3_decode_delay : 0);
                if (max_delay >= e7b_mp3_decode_delay)
                   max_delay -= e7b_mp3_decode_delay;
            } else if( dataMP3 == MP3DEC_GETINPUT ) {
                /* Epics decoding was successfull, read out result */
                samplesPerChannel = CarrierMP3[0];
                *pTotSamplesPerCh += samplesPerChannel;

				outptr = copy_output(g_MP3NumChannels, samplesPerChannel, CarrierMP3[1], CarrierMP3[2], outptr);

                /* Indicate that the data is copied */
                infXchgMP3[0] = MP3DEC_INPUTREADY;
            }
        }  while ( dataMP3 != 0 );

        /* Read back the decoding result */
        decodeError |= CarrierMP3[0];
        /* TBD just for testing */
        DecodedFrames = CarrierMP3[1];
    }
	if ((outptr-pDecoded) != (*pTotSamplesPerCh * g_MP3NumChannels)) {
		ERRMSG("incorrect number of samples written: %d != %d\n", (outptr-pDecoded), (*pTotSamplesPerCh * g_MP3NumChannels));
	}

    /* After decoding we have to update the epics pointers */
    mp3decUpdateInputBuffer( &inBuffer, &wordsInBuffer);

    return decodeError;
}



/*************************************************************************
 * Function            : mp3decUpdateInputBuffer
 * Description         :
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decUpdateInputBuffer (u32 *pInBuffer, u32 *WordsInBuffer)
{
	TRACE(3, "%s\n", __FUNCTION__);

    infXchgMP3[0] = MP3DEC_UPDATEINPUTBUFFER;
    e7b_interrupt_req();
    WAIT_UNTIL_READY();

    *pInBuffer = CarrierMP3[0];
    *WordsInBuffer = CarrierMP3[1];

    return 0;
}



/*************************************************************************
 * Function            : mp3decReset
 * Description         : Will be called when seeking (reset coeff)
 * input parameters    :
 * return parameter    :
 *************************************************************************/
u32 mp3decReset (void)
{
	TRACE(2, "%s\n", __FUNCTION__);

    /* Send the Reset command */
    infXchgMP3[0] = MP3DEC_RESET;
    e7b_interrupt_req();
    WAIT_UNTIL_READY ();

    return 0;
}



/* EOF */
