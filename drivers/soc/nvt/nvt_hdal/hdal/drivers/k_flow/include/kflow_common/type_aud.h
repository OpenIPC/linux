/**
    Audio frame related types.

    This module include audio frame related types

    @file       type_aud.h
    @ingroup    mInvtaud
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _TYPE_AUD_H_
#define _TYPE_AUD_H_

#include "kwrap/platform.h"
#include "kwrap/type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUD_DATA_DESC_SIZE  38 ///< size of data desc in WORDS

//------------------------------------------------------------------------------
// metadata structures
//------------------------------------------------------------------------------
typedef struct _AUD_METADATA {
	UINT32 sign;				///< signature = MAKEFOURCC(?,?,?,?)
	struct _AUD_METADATA* p_next;      ///< pointer to next meta
} AUD_METADATA;


//------------------------------------------------------------------------------
// data structures
//------------------------------------------------------------------------------

typedef enum _AUD_BIT_WIDTH {
	AUD_BIT_WIDTH_8  = 0, ///< 8bit width
	AUD_BIT_WIDTH_16 = 1, ///< 16bit width
	AUD_BIT_WIDTH_32 = 2, ///< 32bit width
	AUD_BIT_WIDTH_MAX,
} AUD_BIT_WIDTH;

typedef enum _AUD_SOUND_MODE {
	AUD_SOUND_MODE_MONO          = 0x00010001, ///< Mono
	AUD_SOUND_MODE_STEREO_PLANAR = 0x00010002, ///< Stereo planar. Each channel is stored in the different buffer.
	AUD_SOUND_MODE_STEREO        = 0x00020002, ///< Stereo packed
	AUD_SOUND_MODE_MAX,
} AUD_SOUND_MODE;

#define AUD_CHANNEL_COUNT(sound_mode)	((sound_mode) & 0x0000ffff)

typedef enum _AUD_SR {
	AUD_SR_8000   = 8000,     ///< 8 KHz
	AUD_SR_11025  = 11025,    ///< 11.025 KHz
	AUD_SR_12000  = 12000,    ///< 12 KHz
	AUD_SR_16000  = 16000,    ///< 16 KHz
	AUD_SR_22050  = 22050,    ///< 22.05 KHz
	AUD_SR_24000  = 24000,    ///< 24 KHz
	AUD_SR_32000  = 32000,    ///< 32 KHz
	AUD_SR_44100  = 44100,    ///< 44.1 KHz
	AUD_SR_48000  = 48000,    ///< 48 KHz
} AUD_SR;


typedef struct _AUD_FRAME {
	UINT32          sign;          ///< signature MAKEFOURCC('A','F','R','M')
	AUD_METADATA*   p_next;        ///< pointer to next meta
	UINT32          addr[2];       ///< address of each channel
	UINT32          phyaddr[2];    ///< physical address of each channel
	UINT32          size;          ///< data size (byte per channel)
	AUD_BIT_WIDTH   bit_width;     ///< audio sample bit length
	AUD_SOUND_MODE  sound_mode;    ///< audio channel
	AUD_SR          sample_rate;   ///< audio sample rate
	UINT64          count;         ///< frame count
	UINT64          timestamp;     ///< time stamp
	UINT32          pool_type;     ///< memory pool type
	INT32           blk;           ///< memory block
	UINT32          reserved[22];  ///< reserved
} AUD_FRAME, *PAUD_FRAME;
STATIC_ASSERT(sizeof(AUD_FRAME) / sizeof(UINT32) == AUD_DATA_DESC_SIZE);





//------------------------------------------------------------------------------
// data structures
//------------------------------------------------------------------------------


/**
     Stream audio bitstream data descriptor.
*/
typedef struct _AUD_BITSTREAM {
	UINT32			sign;            	///< MAKEFOURCC('A','S','T','M')
	AUD_METADATA* 	p_next;      		///< pointer to next meta
	UINT32			ChannelCnt;		///< ChannelCnt
	UINT32			CodecType;		///< codec type
	UINT64			count;			///< frame count
	UINT64			timestamp;		///< time stamp
	UINT32			DataAddr;		///< data address
	UINT32			DataSize;		///< data size = SampePerSecond * ChannelCnt * BitsPerSample / 8
	UINT32			SampePerSecond;	///< SampePerSecond
	UINT32			BitsPerSample;	///< BitsPerSample
	UINT32          pool_type;      ///< memory pool type
	INT32           blk;            ///< memory block
	UINT32			resv[24];		///<
} AUD_BITSTREAM, *PAUD_BITSTREAM; ///< size = 20 words
STATIC_ASSERT(sizeof(AUD_BITSTREAM) / sizeof(UINT32) == AUD_DATA_DESC_SIZE);


#ifdef __cplusplus
} //extern "C"
#endif

#endif //_TYPE_AUD_H_
