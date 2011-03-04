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
#ifndef _PNX0106_MP3BL_H
#define _PNX0106_MP3BL_H

#include <linux/types.h>

/******************************************************************************
 * 	
 * 		VALUES REQUIRED BY THE DECODER
 *
 * 			(check these when a new decoder is delivered!)
 *
 */

/******************************************************************************
 * Maximum number of bits required. This constant defines the maximum number  *
 * of bits required by the decode function. 								  *
 * The value 11520 corresponds to the maximum bit rate of 320kbps and the     *
 * minimum sampling frequency of 32kHz. It is calculated as follows:          *
 * 320000 (bits per second) * 1152 (samples per channel per frame) / 32000    *
 * (samples per channel per second) = 11520 (bits per frame).                 *
 ******************************************************************************/
#define  E7B_MP3_MAX_BITS_REQUIRED    (11520)

/******************************************************************************
 * Maximum length of PCM returned. This constant defines the maximum number of*
 * PCM samples per channel returned from the decoder. 						  *
 ******************************************************************************/
#define  E7B_MP3_MAX_PCM_LENGTH       (1152)



/******************************************************************************
 *
 * 		Declaration of the buffer structures (mmaped)
 * 			
 */
#define E7B_MP3_MAX_INPUTSIZE 	(E7B_MP3_MAX_BITS_REQUIRED / 8) /* 1440 bytes */
//#define E7B_MP3_MAX_INPUTSIZE 	(512 * 3)
#define E7B_MP3_MAX_SAMPLES		(E7B_MP3_MAX_PCM_LENGTH * 2)	/* max 2 channels for stereo (note: 2 bytes per sample)*/
//#define E7B_MP3_NBUF		2		/* fixed for now */
#define E7B_MP3_NBUF		1		/* fixed for now */

typedef struct 
{
    unsigned long samplesPerCh;
    unsigned long numChans;
    unsigned long freeFormat;
    unsigned long bitsRequired;
    unsigned long id;
    unsigned long layer;
    unsigned long protection;
    unsigned long bitRateIndex;       /*  4 */
    unsigned long bitRateKBPS; /* The actual bitrate (in kbps) from table lookup of bitRateIndex and layer*/
    unsigned long sampleRateIndex;
    unsigned long sampleRateHz; /* The actual sample rate (in Hertz) from table lookup of sampleRateIndex and layer*/
    unsigned long paddingBit;
    unsigned long privateBit;
    unsigned long mode;
    unsigned long modeExtension;
    unsigned long copyright;
    unsigned long originalHome;
    unsigned long emphasis;
} e7b_mp3_header;

typedef struct	
{
	char 			data[E7B_MP3_MAX_INPUTSIZE * E7B_MP3_NBUF]; /* one circular buffer to maximize mem usage */
//	char 			data[1440 * E7B_MP3_NBUF]; /* one circular buffer to maximize mem usage */
} e7b_mp3_input;

typedef struct 
{
	e7b_mp3_header 	hdr;
	s16	data[E7B_MP3_MAX_SAMPLES];
	volatile unsigned long	count; /* #samples actually filled in by decoder */
} e7b_mp3_output;

/* note: if the read/write interface is used, the structure (including the count field) is updated by the driver */
typedef struct {
//	e7b_mp3_input	inp;
	e7b_mp3_output	out[E7B_MP3_NBUF];
} e7b_mp3_buffer;


/******************************************************************************
 *
 * 		Declaration of the IOCTLs
 * 			
 */

#define E7B_MP3_IOCTL_TYPE		((char)0xe7)  /* any will do ? */

#if 0
typedef struct {
	unsigned long available;	/* at least this nr of bytes is available for writing mp3 data to circular buffer */
	unsigned long wrpos;		/* position in circular buffer where writing can start */
} e7b_mp3_input_state;

typedef enum {
	e7b_mp3_blocked_on_input,	/* initial state of decoder */
	e7b_mp3_processing,			/* busy doing decoding stuff */
	e7b_mp3_blocked_on_output	/* waiting for user to consume some PCM */
} e7b_mp3_decoder_state;

typedef struct {
	int					  output; /* current output packect (note that it is only ready when its count != 0), this just cycles [0..NBUF[ */
	e7b_mp3_decoder_state decoder;
	e7b_mp3_input_state   input;
} e7b_mp3_status;
#endif

/* structure to communicate data back from the decode call */
typedef struct {
    u32 needInput;
    u32 samplesReady;
    u32 decReturn;
} e7b_mp3_decode;

#if 0
// Obsolete Ioctls
#define E7B_MP3_STATUS				_IOR(E7B_MP3_IOCTL_TYPE, 1, e7b_mp3_status) /* will block while input is full and output empty */
#define E7B_MP3_INC_INPUT			_IOW(E7B_MP3_IOCTL_TYPE, 2, int) /* report amount of bytes that app added to circular buffer */
#define E7B_MP3_USED_OUTPUT			_IOW(E7B_MP3_IOCTL_TYPE, 3, int) /* report that app has consumed the output buffer (index is param) */
#define E7B_MP3_DECODE_DELAY		_IOW(E7B_MP3_IOCTL_TYPE, 4, int) /* decoder thread will be suspended this long for each granule (10ms by default) */
#endif

#define E7B_MP3_RESET   		_IO(E7B_MP3_IOCTL_TYPE, 5) /* decoder thread will send a reset command to epics and flush the input and output buffers */
#define E7B_MP3_INIT   			_IO(E7B_MP3_IOCTL_TYPE, 6) /* initialize the MP3 decoder */
#define E7B_MP3_INPUT_EOF 		_IO(E7B_MP3_IOCTL_TYPE, 7) /* flag the end of the current input stream */
#define E7B_MP3_DECODE  		_IOW(E7B_MP3_IOCTL_TYPE, 8, e7b_mp3_decode) /* decode one frame of audio */
#define E7B_MP3_STREAM  		_IOW(E7B_MP3_IOCTL_TYPE, 9, int) /* enter stream decoding mode */
#define E7B_MP3_STREAM_GETPCM  		_IOW(E7B_MP3_IOCTL_TYPE, 10, e7b_mp3_decode) /* enter stream decoding mode */
#define E7B_MP3_STREAM_RESUME  		_IOW(E7B_MP3_IOCTL_TYPE, 10, int) /* enter stream decoding mode */


u32 mp3decInit             ( void );
u32 mp3decDeInit           ( void );
u32 mp3decReset            ( void );
int mp3decInputData        ( u8 *pData, u32 len );
void mp3decInputEOF	   ( void );
void mp3decOutputPassOwnership( void );
int mp3decContinue         ( void );
u32 mp3decDecode           ( void );
u32 mp3decDecodeFinish     ( e7b_mp3_header *pFrameInfo, u32 *pSamplesDecoded, s16 *pDecoded, u32 *pDecReturn );
int mp3decStreamStart	   ( void );
void mp3decStreamFinish	   ( u32 *pDecReturn );
void mp3decStreamGetPCM	   ( e7b_mp3_header *pFrameInfo, u32 *pSamplesReturned, s16 *pDecoded, u32 *pDecReturn );
int mp3decStreamResume(void);
int mp3decStreamSave(void);
int mp3decStreamRestore(void);


#endif // _PNX0106_MP3BL_H
