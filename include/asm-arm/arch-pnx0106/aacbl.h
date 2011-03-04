/*******************************************************************************
*  (C) Copyright 2004 Philips Semiconductors, All rights reserved
*
*  This source code and any compilation or derivative thereof is the sole
*  property of Philips Corporation and is provided pursuant to a Software
*  License Agreement.  This code is the proprietary information of Philips
*  Corporation and is confidential in nature.  Its use and dissemination by
*  any party other than Philips Corporation is strictly limited by the
*  confidential information provisions of the Agreement referenced above.
*
*  This product is protected by certain intellectual property rights of
*  Microsoft and cannot be used or further distributed without a license
*  from Microsoft.
*
*******************************************************************************/
#ifndef _PNX0106_AACBL_H
#define _PNX0106_AACBL_H

#include <linux/types.h>


/******************************************************************************
 * Maximum length of PCM returned. This constant defines the maximum number of*
 * PCM samples per channel returned from the decoder. 						  *
 ******************************************************************************/
#define  E7B_AAC_MAX_PCM_LENGTH       (1024)



/******************************************************************************
 *
 * 		Declaration of the buffer structures (mmaped)
 * 			
 */
#define E7B_AAC_MAX_SAMPLES		(E7B_AAC_MAX_PCM_LENGTH * 2)	/* max 2 channels for stereo (note: 2 bytes per sample)*/
#define E7B_AAC_NBUF		1		/* fixed for now */

typedef struct 
{
    unsigned long samplingFreqHz;
    unsigned long nrChan;
    unsigned long transportFormat;
} e7b_aac_header;


typedef struct 
{
	e7b_aac_header 	hdr;
	s16	data[E7B_AAC_MAX_SAMPLES];
	volatile unsigned long	count; /* #samples actually filled in by decoder */
} e7b_aac_output;

/* note: if the read/write interface is used, the structure (including the count field) is updated by the driver */
typedef struct {
//	e7b_aac_input	inp;
	e7b_aac_output	out[E7B_AAC_NBUF];
} e7b_aac_buffer;


/******************************************************************************
 *
 * 		Declaration of the IOCTLs
 * 			
 */

#define E7B_AAC_IOCTL_TYPE		((char)0xe7)  /* any will do ? */

/* structure to communicate data back from the decode call */
typedef struct {
    u32 needInput;
    u32 samplesReady;
    u32 decReturn;
} e7b_aac_decode;


#define E7B_AAC_RESET   		_IO(E7B_AAC_IOCTL_TYPE, 5) /* decoder thread will send a reset command to epics and flush the input and output buffers */
#define E7B_AAC_INIT   			_IO(E7B_AAC_IOCTL_TYPE, 6) /* initialize the AAC decoder */
#define E7B_AAC_INPUT_EOF 		_IO(E7B_AAC_IOCTL_TYPE, 7) /* flag the end of the current input stream */
#define E7B_AAC_STREAM  		_IOW(E7B_AAC_IOCTL_TYPE, 9, int) /* enter stream decoding mode */
#define E7B_AAC_STREAM_GETPCM  		_IOW(E7B_AAC_IOCTL_TYPE, 10, e7b_aac_decode) /* enter stream decoding mode */
#define E7B_AAC_STREAM_RESUME  		_IOW(E7B_AAC_IOCTL_TYPE, 10, int) /* enter stream decoding mode */


u32 aacdecInit             ( void );
u32 aacdecDeInit           ( void );
u32 aacdecReset            ( void );
int aacdecInputData        ( u8 *pData, u32 len );
void aacdecInputEOF	   ( void );
void aacdecOutputPassOwnership( void );
int aacdecContinue         ( void );
int aacdecStreamStart	   ( void );
void aacdecStreamFinish	   ( u32 *pDecReturn );
void aacdecStreamGetPCM	   ( e7b_aac_header *pFrameInfo, u32 *pSamplesReturned, s16 *pDecoded, u32 *pDecReturn );
int aacdecStreamResume(void);
int aacdecStreamSave(void);
int aacdecStreamRestore(void);


#endif // _PNX0106_AACBL_H
