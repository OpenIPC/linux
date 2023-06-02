#ifndef _AUDLIB_BACKCOMPATIBLE_H
#define _AUDLIB_BACKCOMPATIBLE_H

#include "kdrv_audioio/audlib_aac.h"
#include "kdrv_audioio/audlib_aec.h"
#include "kdrv_audioio/audlib_agc.h"
#include "kdrv_audioio/audlib_anr.h"
#include "kdrv_audioio/audlib_src.h"
#include "kdrv_audioio/audlib_filt.h"
#include "kdrv_audioio/audlib_adpcm.h"

//
// temp. backward compatible usages
//

#define aac_encode_init			audlib_aac_encode_init
#define aac_decode_init			audlib_aac_decode_init
#define aac_encode_one_frame	audlib_aac_encode_one_frame
#define aac_decode_one_frame	audlib_aac_decode_one_frame

#define Aec_Open				audlib_aec_open
#define Aec_IsOpened			audlib_aec_is_opened
#define Aec_Close				audlib_aec_close
#define Aec_SetConfig			audlib_aec_set_config
#define Aec_Init				audlib_aec_init
#define Aec_Run					audlib_aec_run
#define Aec_GetConfig			audlib_aec_get_config
#define Aec_GetRequireBufSize	audlib_aec_get_required_buffer_size
#define AudNS_Open				audlib_ns_open
#define AudNS_IsOpened			audlib_ns_is_opened
#define AudNS_Close				audlib_ns_close
#define AudNS_SetConfig			audlib_ns_set_config
#define AudNS_Init				audlib_ns_init
#define AudNS_Run				audlib_ns_run
#define AudNS_GetRequireBufSize	audlib_ns_get_required_buffer_size

#define Agc_Open				audlib_agc_open
#define Agc_IsOpened			audlib_agc_is_opened
#define Agc_Close				audlib_agc_close
#define Agc_SetConfig			audlib_agc_set_config
#define Agc_Init				audlib_agc_init
#define Agc_Run					audlib_agc_run

#define adpcm_encodeMono            audlib_adpcm_encode_mono
#define adpcm_encodeStereo          audlib_adpcm_encode_stereo
#define adpcm_decodeMono            audlib_adpcm_decode_mono
#define adpcm_decodeStereo          audlib_adpcm_decode_stereo
#define adpcm_encodePacketMono      audlib_adpcm_encode_packet_mono
#define adpcm_encodePacketStereo    audlib_adpcm_encode_packet_stereo
#define adpcm_decodePacketMono      audlib_adpcm_decode_packet_mono
#define adpcm_decodePacketStereo    audlib_adpcm_decode_packet_stereo

#define Adpcm_EncodeMono			audlib_adpcm_encode_mono
#define Adpcm_EncodeStereo			audlib_adpcm_encode_stereo
#define Adpcm_DecodeMono			audlib_adpcm_decode_mono
#define Adpcm_DecodeStereo			audlib_adpcm_decode_stereo
#define Adpcm_EncodePacketMono		audlib_adpcm_encode_packet_mono
#define Adpcm_EncodePacketStereo	audlib_adpcm_encode_packet_stereo
#define Adpcm_DecodePacketMono		audlib_adpcm_decode_packet_mono
#define Adpcm_DecodePacketStereo	audlib_adpcm_decode_packet_stereo

#define AUD_ANR_GetVersion		audlib_anr_get_version
#define AUD_ANR_PreInit			audlib_anr_pre_init
#define AUD_ANR_Init			audlib_anr_init
#define AUD_ANR_SetSNRI			audlib_anr_set_snri
#define AUD_ANR_GetSNRI			audlib_anr_get_snri
#define AUD_ANR_Detect_Reset	audlib_anr_detect_reset
#define AUD_ANR_Detect			audlib_anr_detect
#define AUD_ANR_Main			audlib_anr_run
#define AUD_ANR_Destroy			audlib_anr_destroy

#define AudFilt_Open			audlib_filt_open
#define AudFilt_IsOpened		audlib_filt_is_opened
#define AudFilt_Close			audlib_filt_close
#define AudFilt_Init			audlib_filt_init
#define AudFilt_SetConfig		audlib_filt_set_config
#define AudFilt_EnableFilt		audlib_filt_enable_filt
#define AudFilt_DesignFilter	audlib_filt_design_filt
#define AudFilt_ApplyFiltering	audlib_filt_run
#define AudFilt_EnableEQ		audlib_filt_enable_eq
#define AudFilt_DesignEQ		audlib_filt_design_eq

#define Aud_SRC_Ver		audlib_src_get_version
#define Aud_SRC_PreInit audlib_src_pre_init
#define Aud_SRC_Init	audlib_src_init
#define Aud_SRC_Main	audlib_src_run
#define Aud_SRC_Destroy	audlib_src_destroy

#endif
