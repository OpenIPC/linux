/*

Abstract:
   AAC Encoder API

*/

#ifndef _AUD_AACE_API_H
#define _AUD_AACE_API_H

#define MAX_CHANNELS 2
#define AACENC_BLOCKSIZE    1024   /*! encoder only takes BLOCKSIZE samples at a time */
#define MIN_OUTSIZE_BYTE ((6144/8)*MAX_CHANNELS)


/*
  Audio Error Type
*/
typedef enum _EN_AUDIO_ERROR_CODE
{
  EN_AUDIO_ERROR_NONE          = 0x0000,   // no error
  EN_AUDIO_ERROR_CONFIGUREFAIL = 0x0001,   // fail to configure the encoder
  EN_AUDIO_ERROR_RUNFAIL       = 0x0002,   // fail to run the encoder
  EN_AUDIO_ERROR_STREAM_EMPTY  = 0x0003,   // no input stream
  EN_AUDIO_ERROR_NOINITIAL     = 0x0004,   // AACE not initial
  EN_AUDIO_ERROR_UNKNOWN       = 0x0005,   // unknown error
  EN_AUDIO_ERROR_TOTAL
}EN_AUDIO_ERROR_CODE;

/*
  Audio Support Sampling Frequency
*/
typedef enum _EN_AUDIO_SAMPLING_RATE
{
  EN_AUDIO_SAMPLING_RATE_NONE  = 0x0000,
  EN_AUDIO_SAMPLING_RATE_8000  = 0x0001,
  EN_AUDIO_SAMPLING_RATE_11025 = 0x0002,
  EN_AUDIO_SAMPLING_RATE_12000 = 0x0003,
  EN_AUDIO_SAMPLING_RATE_16000 = 0x0004,
  EN_AUDIO_SAMPLING_RATE_22050 = 0x0005,
  EN_AUDIO_SAMPLING_RATE_24000 = 0x0006,
  EN_AUDIO_SAMPLING_RATE_32000 = 0x0007,
  EN_AUDIO_SAMPLING_RATE_44100 = 0x0008,
  EN_AUDIO_SAMPLING_RATE_48000 = 0x0009,
  EN_AUDIO_SAMPLING_RATE_TOTAL
} EN_AUDIO_SAMPLING_RATE;

/*
  Audio Support Bit Rate
*/
typedef enum _EN_AUDIO_BIT_RATE
{
  EN_AUDIO_BIT_RATE_NONE = 0x0000,
  EN_AUDIO_BIT_RATE_16k  = 0x0001,
  EN_AUDIO_BIT_RATE_32k  = 0x0002,
  EN_AUDIO_BIT_RATE_48k  = 0x0003,
  EN_AUDIO_BIT_RATE_64k  = 0x0004,
  EN_AUDIO_BIT_RATE_80k  = 0x0005,
  EN_AUDIO_BIT_RATE_96k  = 0x0006,
  EN_AUDIO_BIT_RATE_112k = 0x0007,
  EN_AUDIO_BIT_RATE_128k = 0x0008,
  EN_AUDIO_BIT_RATE_144k = 0x0009,
  EN_AUDIO_BIT_RATE_160k = 0x000a,
  EN_AUDIO_BIT_RATE_192k = 0x000b,
  EN_AUDIO_BIT_RATE_TOTAL
} EN_AUDIO_BIT_RATE;

/*
  Audio Support Enocde Stop Frequency
*/
typedef enum _EN_AUDIO_ENC_STOP_FREQ
{
    EN_AUDIO_ENC_STOP_FREQ_08K = 0x0000,
    EN_AUDIO_ENC_STOP_FREQ_10K = 0x0001,
    EN_AUDIO_ENC_STOP_FREQ_11K = 0x0002,
    EN_AUDIO_ENC_STOP_FREQ_12K = 0x0003,
    EN_AUDIO_ENC_STOP_FREQ_14K = 0x0004,
    EN_AUDIO_ENC_STOP_FREQ_16K = 0x0005,
    EN_AUDIO_ENC_STOP_FREQ_18K = 0x0006,
    EN_AUDIO_ENC_STOP_FREQ_20K = 0x0007,
    EN_AUDIO_ENC_STOP_FREQ_24K = 0x0008,
    EN_AUDIO_ENC_STOP_FREQ_TOTAL
} EN_AUDIO_ENC_STOP_FREQ;
/*
  Audio Coding Type
*/
typedef enum _EN_AUDIO_CODING_TYPE
{
	EN_AUDIO_CODING_TYPE_DISABLE = 0,     // 0:skip AAC header decode or not encode AAC header.
	EN_AUDIO_CODING_TYPE_ENABLE  = 1,     // 1: enable AAC header decode or enocde AAC header.
	EN_AUDIO_CODING_TYPE_TOTAL
}EN_AUDIO_CODING_TYPE;

/*
Support Bitrate/sampling rate/channel are as below.
Sampling rate(kHz)  8       11.025      12        16      22.05       24        32       44.1       48
Channel Number    1    2    1    2    1    2    1    2    1    2    1    2    1    2    1    2    1    2
Bit rate(kbps)
            16    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
            32    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
            48         O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
            64              O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
            80                   O         O    O    O    O    O    O    O    O    O    O    O    O    O
            96                                       O    O    O    O    O    O    O    O    O    O    O
            112                                      O    O    O    O    O    O    O    O    O    O    O
            128                                           O    O    O    O    O    O    O    O    O    O
            144                                                O         O    O    O    O    O    O    O
            160                                                O         O    O    O    O    O    O    O
            192                                                                    O    O    O    O    O
*/
typedef struct _ST_AUDIO_AACENC_CFG
{
    EN_AUDIO_SAMPLING_RATE    enSampleRate;        // AAC encoder output sampling rate
    unsigned int              u32nChannels;        // Only support 1 or 2 channels.
    EN_AUDIO_BIT_RATE         enBitRate;           // bitrate.
    EN_AUDIO_CODING_TYPE      enCodingType;        // Coding type.
    EN_AUDIO_ENC_STOP_FREQ    enEncStopFreq;
} ST_AUDIO_AACENC_CFG,*pST_AUDIO_AACENC_CFG;

typedef struct _ST_AUDIO_AACENC_BUFINFO
{
    unsigned int    u32nSamples;            //input total samples, it must >=1024*u32nChannels
    unsigned int   *pu32InBufferAddr;       //input stream buffer address (1024*2ch*2bytes)
    unsigned int   *pu32OutBufferAddr;      //output encode buffer address (1536 bytes)
}ST_AUDIO_AACENC_BUFINFO,*pST_AUDIO_AACENC_BUFINFO;

typedef struct _ST_AUDIO_AACENC_RTNINFO
{
    unsigned int u32EncodeOutBytes; //return encode output bytes
} ST_AUDIO_AACENC_RTNINFO,*pST_AUDIO_AACENC_RTNINFO;

/*---------------------------------------------------------------------------
functionname:_aac_encode_init
description:  initialize a new encoder configuration.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/
EN_AUDIO_ERROR_CODE _aac_encode_init(pST_AUDIO_AACENC_CFG pstInputCfg);

/*---------------------------------------------------------------------------
functionname:_aac_encode_one_frame
description:  Encode One Frame and feedback information.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/
EN_AUDIO_ERROR_CODE _aac_encode_one_frame(pST_AUDIO_AACENC_BUFINFO pstBufInfo, pST_AUDIO_AACENC_RTNINFO pRtnINfo );

/*---------------------------------------------------------------------------
functionname:AUD_AACE_GetVersion
description:  Get AACE code version.
returns:     AACE code version
---------------------------------------------------------------------------*/
short int AUD_AACE_GetVersion(void);

EN_AUDIO_ERROR_CODE  AacEncOpen (pST_AUDIO_AACENC_CFG pstInputCfg);        /* pre-initialized config struct */

EN_AUDIO_ERROR_CODE AacEncEncode(
                    ST_AUDIO_AACENC_BUFINFO stBufInfo,  /*!<  audio buffer information */
                    const unsigned char *ancBytes,     /*!< pointer to ancillary data bytes */
                    short *numAncBytes,       /*!< number of ancillary Data Bytes */
                    int *numOutBytes         /*!< number of bytes in output buffer after processing */
                    );

#endif
