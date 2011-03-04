#ifndef _EPICSAPI_H
#define _EPICSAPI_H

/*
 * 	Epics labels (addresses in epics mem)
 */
/* infXchg is the only thing that should be hard coded */
#define EPX_INFXCHG             0x0000ffd6

/* The rest of these should come from the labels in the */
/* firmware image */

/* These come from the decoder image and only apply to MP3 */
#define EPX_DECODEROUTPUTL      0x00007000
#define EPX_DECODEROUTPUTR      0x00007240
//#define EPX_DECODERCARRIER      0x00006de7
#define EPX_DECODERCARRIER      0x00001799
#define EPX_DECODERINPUT        0x00006e00

/* These come from the post processing (pp) image and apply */
/* to all of the decoders */
#define EPX_DSP_DOWNLOAD_READY  0x0000ffd5
#define EPX_CONTROLADDRESS      0x0000ffd4
//#define EPX_STOP	            0x00000381
#define EPX_STOP	            0x00000260

/*
 * 	MP3 epics application API definitions
 */
#define MP3DEC_INIT                0x101
#define MP3DEC_SYNC                0x102
#define MP3DEC_GETINPUT            0x103
#define MP3DEC_INPUTREADY          0x104
#define MP3DEC_HEADERINFO          0x105
#define MP3DEC_GETPCM              0x106
#define MP3DEC_OUTPUTREADY         0x107
#define MP3DEC_UPDATEINPUTBUFFER   0x108
#define MP3DEC_RESET               0x109
#define MP3DEC_DECODE               0x10a
#define MP3DEC_STREAM               0x10b
#define MP3DEC_STREAM_RESUME        0x10c

#if 0
/*	Decoder defined errors */
#define  MP3DEC_NO_ERR                              (0)
#define  MP3DEC_NO_SYNCWORD                         (1)
#define  MP3DEC_FREEBITRATE_SYNCH                   (2)
#define  MP3DEC_INVALID_MPEG_ID                     (3)
#define  MP3DEC_UNSUPPORTED_LAYER                   (4)
#define  MP3DEC_FROBIDDEN_BIT_RATE                  (5)
#define  MP3DEC_RESERVED_SAMP_FREQ                  (6)
#define  MP3DEC_CRC_ERROR                           (7)
#define  MP3DEC_BROKEN_FRAME                        (8)
#define  MP3DEC_FRAME_DISCARDED                     (9)
#define  MP3DEC_INVALID_HUFFMANBITS                 (10)
#define  MP3DEC_INVALID_ipBitstream_bufptr          (11)
#define  MP3DEC_INVALID_ipBitstream_bitidx          (12)
#define  MP3DEC_INVALID_pOutBuffer                  (13)
#define  MP3DEC_INVALID_pOutBufferR                 (14)
#define  MP3DEC_INVALID_info                        (15)
#define  MP3DEC_INVALID_OpBuffModuloSize            (16)
#define  MP3DEC_INVALID_IpBuffModuloSize            (17)
#endif
/* No error */
#define MP3D_NO_ERR                 (0)
#define MP3D_BAD_ARGUMENT           (1)
/* Synchronization failure */
#define MP3D_NO_SYNCWORD            (2)
/* Synchword with the header containing bitrate index 0, is detected */
#define MP3D_FREEBITRATE_SYNCH      (3)
/* Unsupported id. (i.e. id == 1) */
#define MP3D_UNSUPPORTED_MPEG_ID    (4)
/* Unsupported layer. (i.e. layer != 3) */
#define MP3D_UNSUPPORTED_LAYER      (5)
/* Unsupported bitrate.(bitrate_index == 15) */
#define MP3D_UNSUPPORTED_BIT_RATE   (6)
/* Unsupported sampling frequency. (i.e. sampling_freq_index == 3) */
#define MP3D_UNSUPPORTED_SAMP_FREQ  (7)
#define MP3D_CRC_ERROR              (8) /* CRC check failure */
/* Incomplete Frame (Insufficient bytes in Bit reservoir to decode the frame.) */
#define MP3D_BROKEN_FRAME           (9)
/* Frame is not according to the MP3 standard. [ISO11172-3],[ISO13818-3] & FhG. 
Eg: Invalid Huffman table, invalid block type */
#define MP3D_FRAME_DISCARDED        (10)
#define MP3D_FAILED                 (11) /* Other exceptions */

// Additional error - added by us ..
#define MP3D_END_OF_STREAM          (12)



/*
 * 	epics application: WMA API definitions
 */
#define WMAAUDIODEC_NEW            0x1
#define WMAAUDIODEC_INIT           0x2
#define WMAAUDIODEC_DECODE         0x3
#define WMAAUDIODEC_GETPCM         0x4
#define WMAAUDIODEC_SENDPARAMETERS 0x5
#define WMAAUDIODEC_GETINPUT       0x6
#define WMAAUDIODEC_UPDATEPOINTERS 0x7
#define WMAAUDIODEC_INPUTREADY     0x9
#define WMAAUDIODEC_DELETE         0xA
#define WMAAUDIODEC_RESET          0xB
#define WMAAUDIODEC_WAITFORINIT    0xC
#define WMAAUDIODEC_INITREADY      0xD
#define WMAAUDIODEC_GETHDR         0xE
#define WMAAUDIODEC_STREAMSEEK     0xF 
#define WMAAUDIODEC_RESYNCH        0x10 
#define WMAAUDIODEC_STREAM         0x11
#define WMAAUDIODEC_SET_AES_KEY    0x12
#define WMAAUDIODEC_RESUME_STREAM  0x13


#define AACDEC_INIT                     0x101
#define AACDEC_GETINPUT                 0x103
#define AACDEC_INPUTREADY               0x104
#define AACDEC_RESET                    0x109
#define AACDEC_STREAM              0x10b
#define AACDEC_STREAM_RESUME       0x10c

#endif
