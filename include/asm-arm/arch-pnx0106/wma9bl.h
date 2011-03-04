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
#define E7B_WMA_MAX_SAMPLES (6144/2)   	/* maximum buffer size when retrieving PCM samples */
#define E7B_WMA_MAX_CHANNELS 2

#define E7B_WMA_MAX_INPUT 	3072		/* decoder buffer only holds this much bytes */
#define E7B_WMA_MAX_OUTPUT 	(E7B_WMA_MAX_SAMPLES * E7B_WMA_MAX_CHANNELS * sizeof (signed short))

#define E7B_WMA_NINPUT_BUFS 1 /* only 1 supported since parser needs to be in sync with decoder */

/* flags field in wma buffer */
#define E7B_WMA_NEW_PACKET	1
#define E7B_WMA_NOMOREINPUT	2

typedef struct {
	unsigned char output[E7B_WMA_MAX_OUTPUT]; /* stereo sample = 32 bit */
	struct {
		unsigned char data[E7B_WMA_MAX_INPUT];
		unsigned long nused; 
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
    u16 needInput;
    u32 inputOffset;
} e7b_wma_input_info;

typedef struct {
    u32 ver;                /* version of the decoder */
    u32 sampleRate;         /* sampling rate */
    u32 numOfChans;         /* number of audio channels */
    u32 duration[2];        /* of the file in milliseconds */
    u32 pktSize[2];         /* size of an ASF packet */
    u32 firstPktOffset[2];  /* byte offset to the first ASF packet */
    u32 lastPktOffset[2];   /* byte offset to the last ASF packet */
    u32 hasDRM;             /* does it have DRM encryption? */
    u32 licenseLen[2];      /* License Length in the header */
    u32 bitrate;            /* bit_rate of the WMA bitstream */
    u32 pktAllowed;         /* flag if packets might occur in bit stream */
    u32 subFrmAllowed;      /* flag if sub-frames might occur in bit stream */;
    u32 bitsPerSample;
} e7b_asf_hdr_format;

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
    e7b_wma_format wma_format; 			/* in */
    e7b_wma_pcm_format pcm_format;		/* in */
    e7b_wma_player_info player_info;		/* in */
    u16 state;					/* out */
    e7b_wma_stream_info userData; 		/* in */
} e7b_wma_init;

typedef struct {
    u16 samplesReady;   	/* out - this much is available for getpcm */
    u16 state;		   	/* in/out: caller must pass his state */
    u16 filedone;       /* out indicated decoding of the file is complete */
    e7b_wma_input_info input; /*out - this indicates if the decoder needs input and where it needs it from */
    u16 decReturn;
} e7b_wma_decode;

typedef struct {
	u16 samplesRequested;  	/* in: up to samplesReady (from decode) */
	u16 dstLength;			/* in: max bytes */
	u16 samplesReturned;  	/* out */
	u16 state;    			/* in/out: caller must pass his state */
} e7b_wma_getpcm;

typedef struct {
    e7b_asf_hdr_format hdr;
    u32 ready;              /* flag, indicates that header is fully parsed */
    e7b_wma_input_info input; /*out - this indicates if the decoder needs input and where it needs it from */
    u16 decReturn;
} e7b_wma_get_hdr;

typedef struct {
    u32 seekTimeFromStart;
    u32 actualSeekTimeFromStart;
    e7b_wma_input_info input; /*out - this indicates if the decoder needs input and where it needs it from */
    u16 decReturn;
} e7b_wma_streamseek;

typedef struct {
    u32 seekOffsetInBytes;
    u32 complete;
    e7b_wma_input_info input; /*out - this indicates if the decoder needs input and where it needs it from */
    u16 decReturn;
} e7b_wma_resynch;

typedef struct {
    u16 decReturn;
} e7b_wma_stream;

typedef struct {
    u16 samplesReturned;
    u16 decReturn;
} e7b_wma_streamgetpcm;

typedef struct {
    u8 key[16];
} e7b_wma_set_aes_key;

#define E7B_WMA_INIT			_IOWR(E7B_WMA_IOCTL_TYPE, 1, e7b_wma_init)  
#define E7B_WMA_DECODE			_IOWR(E7B_WMA_IOCTL_TYPE, 2, e7b_wma_decode)  
#define E7B_WMA_GETPCM			_IOWR(E7B_WMA_IOCTL_TYPE, 3, e7b_wma_getpcm)  
#define E7B_WMA_RESET			_IO  (E7B_WMA_IOCTL_TYPE, 4)  
#define E7B_WMA_GET_ASF_HDR		_IOW (E7B_WMA_IOCTL_TYPE, 6, e7b_wma_get_hdr)  
#define E7B_WMA_STREAM_SEEK		_IOWR(E7B_WMA_IOCTL_TYPE, 7, e7b_wma_streamseek)
#define E7B_WMA_RESYNCH		    _IOWR(E7B_WMA_IOCTL_TYPE, 8, e7b_wma_resynch)
#define E7B_WMA_INPUT_EOF		_IO  (E7B_WMA_IOCTL_TYPE, 9)  
#define E7B_WMA_STREAM		    _IOW (E7B_WMA_IOCTL_TYPE, 10, e7b_wma_stream)  
#define E7B_WMA_STREAM_GETPCM   _IOW (E7B_WMA_IOCTL_TYPE, 11, e7b_wma_streamgetpcm)
#define E7B_WMA_SET_AES_KEY     _IOWR(E7B_WMA_IOCTL_TYPE, 12, e7b_wma_set_aes_key)

	

/* wrapper layer around epics WMA interface */

typedef struct {
    u8* data;
    u16 size; /* packets are small (normally < 128 bytes) */
    u8  newPacket;
    u8  noMoreInput;
} wma9decInputBuf;

/* functions implemented in wma9epics.c */
void wma9bldecNew (void);
void wma9bldecDelete (void);
int  wma9bldecInit (void);
void wma9bldecReset (void);
int wma9bldecDecode (void);
void wma9bldecDecodeFinish (u16* pcSamplesReady, /* out: how much is availble from GetPCM() */
                    u16* pEof,
                    u16* pDecReturn);

void wma9bldecGetPCM (u16 cSamplesRequested, /* up to *pcSamplesReady (from audecDecode()) */
                    u16 *pcSamplesReturned, /* out */
                    u8* pbDst,
                    u16 cbDstLength,
                    u16 *pState);

int wma9bldecGetAsfHdr(void);
void wma9bldecGetAsfHdrFinish(e7b_asf_hdr_format* phdr,
                            u16* pDecReturn);
int wma9bldecStreamSeek(u32 seekTimeFromStart);
void wma9bldecStreamSeekFinish(u32 *pActualSeekTimeFromStart, u16* pDecReturn);
int wma9bldecResynch(u32 seekOffsetInBytes);
void wma9bldecResynchFinish(u16* pDecReturn);
u32 wma9bldecGetStreamByteOffset(void);
int wma9bldecInputData(u8 *pData, 
                       u32 len);
void wma9bldecNewPacket(void);
void wma9bldecInputEOF(void);
void wma9bldecOutputPassOwnership(void);
int wma9bldecContinue(void);
int wma9bldecStreamStart(void);
void wma9bldecStreamFinish(u16* pDecReturn);
void wma9bldecStreamGetPCM(u16 *pcSamplesReturned, u8* pbDst, u16* pDecReturn);
int wma9bldecSetAesKey(u8* key);
int wma9bldecStreamSave(void);
int wma9bldecStreamRestore(void);
int wma9bldecStreamResume(void);

#endif
