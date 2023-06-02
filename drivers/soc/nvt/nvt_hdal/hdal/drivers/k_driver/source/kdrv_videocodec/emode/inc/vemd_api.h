#ifndef _VEMD_API_H_
#define _VEMD_API_H_

#include "kwrap/type.h"

#include "h26xenc_int.h"
#include "h264enc_api.h"
#include "h265enc_api.h"

typedef struct _vemd_func_t_{
	CODEC_TYPE eCodecType;
	
	H26XEncSliceSplit	stSliceSplit;
	H26XEncGdr			stGdr;
	H26XEncRoi			stRoi;
	H26XEncRowRc		stRowRc;
	H26XEncAq			stAq;
	H26XEncLpm			stLpm;
	h26XEncRnd			stRnd;
	H26XEncMotAq		stMAq;
	H26XEncJnd			stJnd;
	H26XEncQpRelatedCfg	stRqp;	
	H26XEncVar		    stVar;	
	
	union {
		H264EncFroCfg st264;
		H265EncFroCfg st265;
	} stFro;

	union {
		H264EncRdo st264;
		H265EncRdo st265;
	} stRdo;
} vemd_func_t;

typedef struct _vemd_info_t_{		
	BOOL enable;			
	
	int enc_id;
	int slice_en;
	int gdr_en;
	int roi_en;
	int rrc_en;	
	int aq_en;
	int lpm_en;
	int rnd_en;
	int maq_en;
	int jnd_en;
	int rqp_en;	
	int var_en;
	int fro_en;
	int rdo_en;

	vemd_func_t func;
} vemd_info_t;

extern vemd_info_t g_vemd_info;

int vemd_get_func_info(vemd_func_t *pstfunc);


#endif // _VEMD_API_H_
