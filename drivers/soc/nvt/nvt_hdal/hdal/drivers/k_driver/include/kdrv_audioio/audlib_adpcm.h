/**
    Header file of ADPCM Encode/Decode Library

    This file is the Header file of IMA ADPCM Encode/Decode Library.

    @file       ADPCM.h
    @ingroup    mIAVADPCM
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _AUDLIB_ADPCM_H
#define _AUDLIB_ADPCM_H

#ifdef __KERNEL__
#include "kdrv_type.h"
#else
#include "kdrv_type.h"
#endif
/**
    @addtogroup mIAVADPCM
*/
//@{

/**
    @name   Full IMA ADPCM packet sample count

    Definition for Full IMA ADPCM packet sample count
*/
//@{
#define ADPCM_PACKET_SAMPLES_8K         0x01F9      ///< One packet sample count of 8K sampling rate
#define ADPCM_PACKET_SAMPLES_11K        0x01F9      ///< One packet sample count of 11K sampling rate
#define ADPCM_PACKET_SAMPLES_22K        0x03F9      ///< One packet sample count of 22K sampling rate
#define ADPCM_PACKET_SAMPLES_44K        0x07F9      ///< One packet sample count of 44K sampling rate
//@}

/**
    @name   Full IMA ADPCM packet PCM data size

    Definition for Full IMA ADPCM packet PCM data size
*/
//@{
#define ADPCM_PACKET_PCMSIZE_8K_MONO    (ADPCM_PACKET_SAMPLES_8K << 1)      ///< PCM data size of one ADPCM packet for 8K sampling rate mono
#define ADPCM_PACKET_PCMSIZE_8K_STEREO  (ADPCM_PACKET_SAMPLES_8K << 2)      ///< PCM data size of one ADPCM packet for 8K sampling rate stereo
#define ADPCM_PACKET_PCMSIZE_11K_MONO   (ADPCM_PACKET_SAMPLES_11K << 1)     ///< PCM data size of one ADPCM packet for 11K sampling rate mono
#define ADPCM_PACKET_PCMSIZE_11K_STEREO (ADPCM_PACKET_SAMPLES_11K << 2)     ///< PCM data size of one ADPCM packet for 11K sampling rate stereo
#define ADPCM_PACKET_PCMSIZE_22K_MONO   (ADPCM_PACKET_SAMPLES_22K << 1)     ///< PCM data size of one ADPCM packet for 22K sampling rate mono
#define ADPCM_PACKET_PCMSIZE_22K_STEREO (ADPCM_PACKET_SAMPLES_22K << 2)     ///< PCM data size of one ADPCM packet for 22K sampling rate stereo
#define ADPCM_PACKET_PCMSIZE_44K_MONO   (ADPCM_PACKET_SAMPLES_44K << 1)     ///< PCM data size of one ADPCM packet for 44K sampling rate mono
#define ADPCM_PACKET_PCMSIZE_44K_STEREO (ADPCM_PACKET_SAMPLES_44K << 2)     ///< PCM data size of one ADPCM packet for 44K sampling rate stereo
//@}

/**
    @name   Full IMA ADPCM packet align size

    Definition for Full IMA ADPCM packet align size
*/
//@{
#define ADPCM_PACKET_ALIGN_8K_MONO      0x0100          ///< Packet align size for 8K sampling rate mono
#define ADPCM_PACKET_ALIGN_8K_STEREO    0x0200          ///< Packet align size for 8K sampling rate stereo
#define ADPCM_PACKET_ALIGN_11K_MONO     0x0100          ///< Packet align size for 11K sampling rate mono
#define ADPCM_PACKET_ALIGN_11K_STEREO   0x0200          ///< Packet align size for 11K sampling rate stereo
#define ADPCM_PACKET_ALIGN_22K_MONO     0x0200          ///< Packet align size for 22K sampling rate mono
#define ADPCM_PACKET_ALIGN_22K_STEREO   0x0400          ///< Packet align size for 22K sampling rate stereo
#define ADPCM_PACKET_ALIGN_44K_MONO     0x0400          ///< Packet align size for 44K sampling rate mono
#define ADPCM_PACKET_ALIGN_44K_STEREO   0x0800          ///< Packet align size for 44K sampling rate stereo
//@}

/**
    @name   IMA ADPCM average bytes per second

    Definition for IMA ADPCM average bytes per second
*/
//@{
#define ADPCM_AVG_BPS_8K_MONO           0x00000FD7      ///< 8K sampling rate mono average bytes per second
#define ADPCM_AVG_BPS_8K_STEREO         0x00001FAE      ///< 8K sampling rate stereo average bytes per second
#define ADPCM_AVG_BPS_11K_MONO          0x000015D4      ///< 11K sampling rate mono average bytes per second
#define ADPCM_AVG_BPS_11K_STEREO        0x00002BA9      ///< 11K sampling rate stereo average bytes per second
#define ADPCM_AVG_BPS_22K_MONO          0x00002B5C      ///< 22K sampling rate mono average bytes per second
#define ADPCM_AVG_BPS_22K_STEREO        0x000056B9      ///< 22K sampling rate stereo average bytes per second
#define ADPCM_AVG_BPS_44K_MONO          0x0000566D      ///< 44K sampling rate mono average bytes per second
#define ADPCM_AVG_BPS_44K_STEREO        0x0000ACDB      ///< 44K sampling rate stereo average bytes per second
//@}

/**
    ADPCM State
*/
typedef struct {
	INT16   l_val_prev;   ///< Previous left channel value
	INT8    l_index;     ///< Left channel index
	INT16   r_val_prev;   ///< Previous right channel value
	INT8    r_index;     ///< Right channel index
} ADPCM_STATE, *PADPCM_STATE;

// Public APIs
extern UINT32   audlib_adpcm_encode_mono(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
extern UINT32   audlib_adpcm_encode_stereo(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);

extern UINT32   audlib_adpcm_decode_mono(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
extern UINT32   audlib_adpcm_decode_stereo(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);

extern UINT32   audlib_adpcm_encode_packet_mono(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
extern UINT32   audlib_adpcm_encode_packet_stereo(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);

extern UINT32   audlib_adpcm_decode_packet_mono(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count);
extern UINT32   audlib_adpcm_decode_packet_stereo(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count);

int kdrv_audlib_adpcm_init(void);

//@}
#endif
