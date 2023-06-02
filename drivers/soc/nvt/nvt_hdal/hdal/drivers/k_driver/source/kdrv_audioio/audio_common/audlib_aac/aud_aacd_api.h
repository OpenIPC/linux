/*

Abstract:
   AAC Decoder API

*/

#ifndef _AUD_AACD_API_H
#define _AUD_AACD_API_H


#define AAC_MAX_NCHANS		2		/* set to default max number of channels  */
#define AAC_MAX_NSAMPS		1024
#define AAC_MAINBUF_SIZE	(768 * AAC_MAX_NCHANS)
#define READBUF_SIZE	    (2 * AAC_MAINBUF_SIZE * AAC_MAX_NCHANS)	/* pick something big enough to hold a bunch of frames */


/*
  Audio Error Type
*/
typedef enum _EN_AUDIO_ERROR_CODE
{
  EN_AUDIO_ERROR_NONE          = 0x0000,   // no error
  EN_AUDIO_ERROR_CONFIGUREFAIL = 0x0001,   // configure failed
  EN_AUDIO_ERROR_RUNFAIL       = 0x0002,   // run fail
  EN_AUDIO_ERROR_STREAM_EMPTY  = 0x0003,   // no input stream
  EN_AUDIO_ERROR_NOINITIAL     = 0x0004,   // not initial
  EN_AUDIO_ERROR_UNKNOWN       = 0x0005,   // unknown error
  EN_AUDIO_ERROR_TOTAL
}EN_AUDIO_ERROR_CODE;


/*
  Audio Sampling Frequency
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
}EN_AUDIO_SAMPLING_RATE;

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
  Audio Coding Type
*/
typedef enum _EN_AUDIO_CODING_TYPE
{
	EN_AUDIO_CODING_TYPE_DISABLE = 0,     // 0:skip AAC header decode or not encode AAC header.
	EN_AUDIO_CODING_TYPE_ENABLE  = 1,     // 1: enable AAC header decode or enocde AAC header.
	EN_AUDIO_CODING_TYPE_TOTAL
}EN_AUDIO_CODING_TYPE;

/*
  Audio Decode Buffer Information
*/

typedef struct _ST_AUDIO_AACD_BUFINFO
{
	unsigned int	u32AvailBytes;		  // input avaliable bytes, it must greater than one frame size
	unsigned int   *pu32InBufferAddr;	  // input stream buffer address
	unsigned int   *pu32OutBufferAddr;	  // output decode buffer address
}ST_AUDIO_AACD_BUFINFO, *pST_AUDIO_AACD_BUFINFO;

/*
  Audio Decode Return Information
*/

typedef struct _ST_AUDIO_AACD_RTNINFO
{
	unsigned int u32DecodeOutSamples;       // return output samples per channel
	unsigned int u32OneFrameConsumeBytes;   // return consumed bytes after decoding one frame
	unsigned int u32nChans;                 // return channels
	unsigned int u32bitRate;                // return bit rate
	unsigned int u32SampleFreq;             // return sampling frequency
	unsigned int u32bitsPerSample;          // return bit per sample
} ST_AUDIO_AACD_RTNINFO,*pST_AUDIO_AACD_RTNINFO;

typedef struct _ST_AUDIO_AACDEC_CFG
{
    EN_AUDIO_SAMPLING_RATE    enSampleRate;        // Sampling rate. (EN_AUDIO_SAMPLING_RATE)
    unsigned int              u32nChannels;        // Only support 1 or 2 channels.
	EN_AUDIO_CODING_TYPE      enCodingType;        // Coding type. (EN_AUDIO_CODING_TYPE)
} ST_AUDIO_AACDEC_CFG,*pST_AUDIO_AACDEC_CFG;

/*---------------------------------------------------------------------------
functionname:AACD_InitConfig
description:  initialize a new decoder.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/
EN_AUDIO_ERROR_CODE _aac_decode_init(pST_AUDIO_AACDEC_CFG pstInputCfg);

/*---------------------------------------------------------------------------
functionname:_aac_decode_one_frame
description:  Decode One Frame and feedback information.
returns:     EN_AUDIO_ERROR_CODE
---------------------------------------------------------------------------*/
EN_AUDIO_ERROR_CODE _aac_decode_one_frame(pST_AUDIO_AACD_BUFINFO pstBufInfo, pST_AUDIO_AACD_RTNINFO pRtnINfo );

/*---------------------------------------------------------------------------
functionname:AUD_AACD_GetVersion
description:  Get AACD code version.
returns:     AACD code version
---------------------------------------------------------------------------*/
short int AUD_AACD_GetVersion(void);

EN_AUDIO_ERROR_CODE AACInitDecoder(pST_AUDIO_AACDEC_CFG pstInputCfg);  /* pre-initialized config struct */
int AACDecode(int *Consumedbyte, unsigned char *inbuf, short *outbuf,unsigned int bytesLeft);/*Decode One Frame*/
void AACGetRtnInfo(pST_AUDIO_AACD_RTNINFO pRtnINfo); /*feedback information*/

#endif
