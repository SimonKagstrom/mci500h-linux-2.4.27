#ifndef _E7B_WMA_H
#define _E7B_WMA_H

/******************************************************************************
 *
 * 		Declaration of the base types
 * 			
 */

#ifndef __KERNEL__
typedef signed   char 		s8;
typedef unsigned char 		u8;
typedef signed   short 		s16;
typedef unsigned short 		u16;
typedef signed   int 		s32;
typedef unsigned int 		u32;
typedef signed   long long 	s64;
typedef unsigned long long 	u64;
#endif /* __KERNEL__ */

/******************************************************************************
 *
 * 		Declaration of the buffer structures (mmaped)
 * 		Note: Thise sizes are defined by the epics decoder 
 * 			
 */
#define E7B_WMA_MAX_SAMPLES 2048   	/* maximum buffer size when retrieving PCM samples */
#define E7B_WMA_MAX_CHANNELS 2

#define E7B_WMA_MAX_INPUT 	128		/* decoder buffer only holds this much bytes */
#define E7B_WMA_MAX_OUTPUT 	(E7B_WMA_MAX_SAMPLES * E7B_WMA_MAX_CHANNELS)
#define E7B_WMA_NINPUT_BUFS 1 /* only 1 supported since parser needs to be in sync with decoder */

/* flags field in wma buffer */
#define E7B_WMA_NEW_PACKET	1
#define E7B_WMA_NOMOREINPUT	2

typedef struct {
	unsigned long output[E7B_WMA_MAX_OUTPUT]; /* stereo sample = 32 bit */
	struct {
		unsigned char data[E7B_WMA_MAX_INPUT];
		unsigned char nused; /* max 255 is sufficient since >= E7B_WMA_MAX_INPUT */
		unsigned char flags; /* E7B_WMA_NEW_PACKET | E7B_WMA_NOMOREINPUT */
	} input[E7B_WMA_NINPUT_BUFS];
} e7b_wma_buffer;

/******************************************************************************
 *
 * 		Declaration of the IOCTLs
 * 			
 */

#define E7B_WMA_IOCTL_TYPE		((char)0xa9)  /* any will do ? */

typedef struct {
    u32 wFormatTag;
    u32 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u32 nBlockAlign;
    u32 nValidBitsPerSample;
    u32 nChannelMask;
    u32 wEncodeOpt;
} e7b_wma_format;

typedef struct {
    u32 nPlayerOpt;
    u32 rgiMixDownMatrix; 
    u32 iPeakAmplitudeRef;
    u32 iRmsAmplitudeRef;
    u32 iPeakAmplitudeTarget;
    u32 iRmsAmplitudeTarget;
    u32 nDRCSetting;       /* Dynamic range control setting */
} e7b_wma_player_info;

typedef struct {
    u32 nSamplesPerSec;               
    u32 nChannels;
    u32 nChannelMask;
    u32 nValidBitsPerSample;
    u32 cbPCMContainerSize;
    u32 pcmData;
} e7b_wma_pcm_format;

typedef struct {
    u16 version;
    u32 samplesPerSec;
    u32 avgBytesPerSec;
    u32 blockAlign;
    u16 channels;
    u32 samplesPerBlock;
    u16 encodeOpt;
} e7b_wma_stream_info;

typedef struct {
    e7b_wma_format 		wma_format; 	/* in */
    e7b_wma_pcm_format 	pcm_format;		/* in */
    e7b_wma_player_info player_info;	/* in */
    u16		 			state;			/* out */
	e7b_wma_stream_info userData; 		/* in */
} e7b_wma_init;

typedef struct {
	u16  samplesReady;   	/* out - this much is available for getpcm */
	u16	 state;		   		/* in/out: caller must pass his state */
} e7b_wma_decode;

typedef struct {
	u16 samplesRequested;  	/* in: up to samplesReady (from decode) */
	u16 dstLength;			/* in: max bytes */
	u16 samplesReturned;  	/* out */
	u16 state;    			/* in/out: caller must pass his state */
} e7b_wma_getpcm;

#define E7B_WMA_INIT			_IOWR(E7B_WMA_IOCTL_TYPE, 1, e7b_wma_init)  
#define E7B_WMA_DECODE			_IOWR(E7B_WMA_IOCTL_TYPE, 2, e7b_wma_decode)  
#define E7B_WMA_GETPCM			_IOWR(E7B_WMA_IOCTL_TYPE, 3, e7b_wma_getpcm)  
#define E7B_WMA_RESET			_IO  (E7B_WMA_IOCTL_TYPE, 4)  

/* report to the driver how much data is ready and block until the driver needs more */
/* userspace can call this with parameter 0 the first time (so it will block until data is required) */
#define E7B_WMA_INPUT_DATA		_IOW (E7B_WMA_IOCTL_TYPE, 5, int)  /* number of ready entries in e7b_wma_buffer.input[] */


/* wrapper layer around epics WMA interface */

typedef struct {
    u8* data;
    u16 size; /* packets are small (normally < 128 bytes) */
    u8  newPacket;
    u8  noMoreInput;
} wma9decInputBuf;

/* callback must be implemented by caller */
extern void wma9decGetMoreData(wma9decInputBuf*); 

/* functions implemented in wma9epics.c */
void wma9decNew (void);
void wma9decDelete (void);
int  wma9decInit (e7b_wma_format* wma_format, e7b_wma_pcm_format* pcm_format,
                                   e7b_wma_player_info *player_info, u16 *paudecState,
                                   e7b_wma_stream_info *userData);
void wma9decReset (void);
void wma9decDecode (u16* pcSamplesReady, /* out: how much is availble from GetPCM() */
                    u16* paudecState  );

void wma9decGetPCM (u16 cSamplesRequested, /* up to *pcSamplesReady (from audecDecode()) */
                    u16 *pcSamplesReturned, /* out */
                    u8* pbDst,
                    u16 cbDstLength, /* ~ redundant with cSamplesRequested */
                    u16 *paudecState);
#endif
