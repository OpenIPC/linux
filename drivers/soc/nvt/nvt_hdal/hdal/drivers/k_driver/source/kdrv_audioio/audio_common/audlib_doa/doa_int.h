/*
    Audio Direction-Of-Arrival (DOA) and Voice-Active-Detection (VAD) library internal header file

	Audio DOA and VAD library internal header file.

    @file       doa_int.h
    @ingroup    mIAudDOA
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _DOA_INT_H
#define _DOA_INT_H

#ifdef __KERNEL__
#include <linux/delay.h>
#include <mach/rcw_macro.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "kdrv_audioio/audlib_doa.h"
#include "kdrv_audioio/audlib_src.h"
//#include "audio/Audio.h"
#include "audlib_doa_dbg.h"

#else
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"

#include "kdrv_audioio/audlib_doa.h"
#include "kdrv_audioio/audlib_src.h"
#include "Audio.h"

#include "cache_protected.h"

#include "dma_protected.h"

#endif


#define doa_square(x)		(UINT64)(((((INT64)x)*((INT64)x))))

#define DOA_FFT_MAX_SZ          8192
#define DOA_FFT_MAX_SZ_POWER2   13
#define DOA_FFT_TABLE_REDUCED	ENABLE
#define DOA_FFT_ROUNDING		DISABLE

#define DOA_FFT					1
#define DOA_IFFT				-1
#define DOA_MAX_TAU_NO			32
#define DOA_DIST_ROUND			(3*AUDUOA_PRECISION/10)

#define DOA_RESAMPLE_TARGET		16000

/* ENABLE: fixed point ; DISABLE: floating point */
#define DOA_NORMALIZATION_FIXED_POINT	ENABLE

typedef struct {
	AUDDOA_MODE mode;
	BOOL		init;
	INT32		tdoa;
	UINT32		maxshift;
	UINT16		doa_op_sz;
	UINT16		doa_fft_op_sz;

	INT32	doa_sampletae_real;
	INT32	doa_sampletae_set;

	UINT8		doa_active_len;
	UINT8		doa_front_ext_len;
	UINT8		doa_back_ext_len;
	UINT8		doa_sample_avg;
	UINT8		vad_mic_select;
	UINT8		debug_print;

	UINT8		resample_en;
	int         resample_hdl1;
	int         resample_hdl2;
	INT32		vad_sensitiveity;
	UINT32		channels;
	BOOL		defaultcfg;
}DOA_PARAM;


#if DOA_FFT_TABLE_REDUCED
extern const int doa_sin_tab[(DOA_FFT_MAX_SZ>>2)+1];
#else
extern const int doa_sin_tab[DOA_FFT_MAX_SZ];
#endif

extern void doa_assign_fft_buffer(int *buffer_R, int *buffer_I);
extern void doa_fft(int fft_points, int *dataR,int *dataI, int flag, int ocnt);

//#define DOA_DEBUG			ENABLE
//#if DOA_DEBUG
//#define doa_msg(msg)		DBG_DUMP msg
//#else
//#define doa_msg(msg)
//#endif
//#define ADOA_MAX_CHANNEL		4
//#define ADOA_BUFFER_SAMPLE_NO	(DOA_FFT_MAX_SZ/2)
//#define DOA_SOUND_SPEED		343.2
//#define DOA_MIC_DISTANCE_4	0.08127
//#define DOA_MAX_TDOA_4		((float)DOA_MIC_DISTANCE_4/(float)DOA_SOUND_SPEED)
#endif

