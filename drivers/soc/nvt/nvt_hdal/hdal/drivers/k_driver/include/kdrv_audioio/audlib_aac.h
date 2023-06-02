/**
    Header file of AAC Encode/Decode Library

    This file is the Header file of AAC Encode/Decode Library.

    @file       audlib_aac.h
    @ingroup    audlib
	@note       Support Bitrate/sampling rate/channel are as below.
				Sampling rate(kHz) 8       11.025      12        16      22.05       24        32       44.1       48
				Channel Number   1    2    1    2    1    2    1    2    1    2    1    2    1    2    1    2    1    2
				Bit rate(bps)
						16000    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
						32000    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
						48000         O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
						64000              O    O    O    O    O    O    O    O    O    O    O    O    O    O    O    O
						80000                   O         O    O    O    O    O    O    O    O    O    O    O    O    O
						96000                                       O    O    O    O    O    O    O    O    O    O    O
						112000                                      O    O    O    O    O    O    O    O    O    O    O
						128000                                           O    O    O    O    O    O    O    O    O    O
						144000                                                O         O    O    O    O    O    O    O
						160000                                                O         O    O    O    O    O    O    O
						192000                                                                    O    O    O    O    O

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _AUDLIB_AAC_H
#define _AUDLIB_AAC_H

#ifdef __KERNEL__
#include "kdrv_type.h"
#else
#include "kdrv_type.h"
#endif


/**
	AAC Encode/Decode configurations

	This is used at audlib_aac_encode_init / audlib_aac_decode_init to configure configurations.
*/
typedef struct {

	UINT32					encode_bit_rate;    ///< [encode]		bitrate in bps:
												///<				16000/32000/48000/64000/80000/96000/112000/128000/144000/16000/19200.
												///<				This paramter is useless in decode mode.

	UINT32					encode_stop_freq;	///< [encode]		encode stop frequency in Hertz:
												///<				8000/10000/11000/12000/14000/16000/18000/20000/24000
												///<				This paramter is useless in decode mode.
	UINT32					sample_rate;        ///< [encode/decode]AAC encoder sampling rate in Hertz:
												///<				8000/11025/12000/16000/22050/24000/32000/44100/48000.
	UINT32					channel_number;		///< [encode/decode]Only support 1 or 2 channels.
	BOOL					header_enable;		///< [encode/decode]encode with/without aac header or decode with/without aac header.

} AUDLIB_AAC_CFG, *PAUDLIB_AAC_CFG;


/**
	AAC Encode/Decode Buffer information

	This is used at audlib_aac_encode_one_frame / audlib_aac_decode_one_frame
	to specify the audio buffer in/out information for aac encode/decode.
*/
typedef struct {

	UINT32      bitstram_buffer_length;		///< [encode] Audio stream encode length in samples count.
											///< [encode] Must >=1024*channel_number
											///< [decode] Audio stream decode length in bytes count.
											///< [decode] it must greater than one frame size

	UINT32      bitstram_buffer_in;			///< [encode/decode] Audio bitstream buffer address for AAC encode/decode input.
	UINT32      bitstram_buffer_out;		///< [encode/decode] Audio bitstream buffer address for AAC encode/decode output.

} AUDLIB_AAC_BUFINFO, *PAUDLIB_AAC_BUFINFO;


/**
	AAC Encode return information

	This is used at audlib_aac_encode_one_frame to speficy the encoded output bitstream length in byte count.
*/
typedef struct {
	UINT32		output_size;				///< return aac encoded output size in bytes count
} AUDLIB_AACE_RTNINFO, *PAUDLIB_AACE_RTNINFO;



/**
	AAC Dncode return information

	This is used at audlib_aac_decode_one_frame to speficy the decoded output bitstream information.
*/
typedef struct {
	UINT32		output_size;				///< return aac decoded output size in sample count per channel

	UINT32		one_frame_consume_bytes;	///<  return consumed bytes after decoding one frame
	UINT32		channel_number;				///<  return channel number
	UINT32		bit_rate;					///<  return bit rate
	UINT32		sample_rate;				///<  return sampling rate frequency
	UINT32		bits_per_sample;			///<  return bit per sample
} AUDLIB_AACD_RTNINFO, *PAUDLIB_AACD_RTNINFO;


/**
	AAC audio library encode init

	AAC audio library encode init api. This shall be called before any new audio bistream encode start.
*/
INT32 audlib_aac_encode_init(PAUDLIB_AAC_CFG p_encode_cfg);

/**
	AAC audio library decode init

	AAC audio library decode init api. This shall be called before any new audio bistream decode start.
*/
INT32 audlib_aac_decode_init(PAUDLIB_AAC_CFG p_decode_cfg);

/**
	AAC audio library encode one frame

	AAC audio library encode one frame. The frame size is fixed as 1024 audio samples.

    @param[in] p_encode_buf_info	The aac encode in/out buffer information
    @param[in] p_return_info		The aac encoded output information

    @return
     - @b 0:	Done and success.
     - @b 1:	fail to configure the encoder.
     - @b 2:	fail to run the encoder.
     - @b 3:	no input stream.
     - @b 4:	AAC encoder not initial.
     - @b 5:	unknown error.
*/
INT32 audlib_aac_encode_one_frame(PAUDLIB_AAC_BUFINFO p_encode_buf_info, PAUDLIB_AACE_RTNINFO p_return_info);

/**
	AAC audio library decode one frame

	AAC audio library decode one frame. The frame size is fixed as 1024 audio samples.

    @param[in] p_decode_buf_info	The aac decode in/out buffer information
    @param[in] p_return_info		The aac decoded output information

    @return
     - @b 0:	Done and success.
     - @b 1:	fail to configure the decoder.
     - @b 2:	fail to run the decoder.
     - @b 3:	no input stream.
     - @b 4:	AAC decoder not initial.
     - @b 5:	unknown error.
*/
INT32 audlib_aac_decode_one_frame(PAUDLIB_AAC_BUFINFO p_decode_buf_info, PAUDLIB_AACD_RTNINFO p_return_info);

int kdrv_audlib_aac_init(void);

#endif
