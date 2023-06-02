#ifndef _AWB_PARAM_H_
#define _AWB_PARAM_H_

#include "kwrap/type.h"

//=============================================================================
// struct & definition
//=============================================================================
/******************************/
/**
	CT gain
*/
#define AWB_CT_2300K_RGAIN 249
#define AWB_CT_2300K_GGAIN 256
#define AWB_CT_2300K_BGAIN 839

#define AWB_CT_2800K_RGAIN 285
#define AWB_CT_2800K_GGAIN 256
#define AWB_CT_2800K_BGAIN 806

#define AWB_CT_3700K_RGAIN 340
#define AWB_CT_3700K_GGAIN 256
#define AWB_CT_3700K_BGAIN 654

#define AWB_CT_4700K_RGAIN 398
#define AWB_CT_4700K_GGAIN 256
#define AWB_CT_4700K_BGAIN 520

#define AWB_CT_6500K_RGAIN 480
#define AWB_CT_6500K_GGAIN 256
#define AWB_CT_6500K_BGAIN 377

#define AWB_CT_11000K_RGAIN 586
#define AWB_CT_11000K_GGAIN 256
#define AWB_CT_11000K_BGAIN 316

#define AWB_TUNING_CT_WEIGHT_MAX 6
#define AWB_TUNING_TARGET_MAX 3
#define AWB_TUNING_CT_MAX 6
#define AWB_TUNING_MWB_MAX 12
#define AWB_TUNING_BLOCK_MAX 4
#define AWB_TUNING_LUMA_WEIGHT_MAX 8
// AWB alg CA definition
//#define AWB_WIN 32
//#define AWB_CABUFFER_SIZE   AWB_WIN * AWB_WIN

//=============================================================================
// extern functions
//=============================================================================
extern void awb_param_get_param(UINT32 *param);

#endif
