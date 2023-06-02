/**
    Internal header file of NvtAnr

    Internal header file of NvtAnr.

    @file       nvtanr_int.h
    @ingroup    mIAPPNVTANR
    @version    V1.00.000
    @date       2016/02/26

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _NVTANR_H
#define _NVTANR_H

#if defined(__FREERTOS) || defined __UITRON || defined __ECOS
#include <string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../include/nvtanr_api.h"

#define dma_getCacheAddr(parm) parm
#else
#include <linux/string.h>
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../include/nvtanr_api.h"

#define dma_getCacheAddr(parm) parm
#endif

typedef struct {
	int blk_size_w;
	int sampling_rate;
	int stereo;
	int nr_db;
	int hpf_cutoff_freq;
	int bias_sensitive;
	int tone_min_time;
} NVTANR_AUD_INFO, *PNVTANR_AUD_INFO;

extern NVTANR_AUD_INFO anr_info;
extern BOOL anr_en;

/**
    @addtogroup mIAPPNVTANR
*/
//@{

void xNvtAnr_InstallCmd(void);

//@}


#endif
