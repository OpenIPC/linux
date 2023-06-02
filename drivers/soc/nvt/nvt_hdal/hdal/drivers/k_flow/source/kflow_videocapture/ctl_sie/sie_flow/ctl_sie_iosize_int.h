#ifndef _CTL_SIE_IOSIZE_INT_H_
#define _CTL_SIE_IOSIZE_INT_H_

#include "kflow_videocapture/ctl_sie.h"
#include "ctl_sie_utility_int.h"

#define CTL_SIE_AUTO_CROP_SIZE_ALIGN    4   //auto mode crop size limitation when support raw scale
#define CTL_SIE_MANUAL_CROP_START_ALIGN 2   //manual mode crop start limitation when support raw scale
#define CTL_SIE_DEST_DFT_ALIGN  4
//#define CTL_SIE_CCIR_CRP_ALIGN    16
#define CTL_SIE_CCIR_CRP_ALIGN  4
#define CTL_SIE_ALIGN_MAX       256
#define CTL_SIE_IOSIZE_BASE     10000
//#define CTL_SIE_DEST_MAX_SZ_W 3840 // IC IPP limit
//#define CTL_SIE_DEST_MAX_SZ_H 2160 // IC IPP limit
//#define CTL_SIE_DEST_MAX_FPS  3000 // IC IPP limit
#define CTL_SIE_DFT_WIDTH       640
#define CTL_SIE_DFT_HEIGHT      480

typedef struct {
	URECT(*get_sie_crp_win)(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param);    ///< sie crop window
	USIZE(*get_sie_scl_sz)(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param);      ///< sie scale size, if IC not support sie scale, scale size = crp size
	URECT(*get_dest_crp_win)(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_mode_param);   ///< need to get after get_sie_crp_win()
} CTL_SIE_IOSIZE;

CTL_SIE_IOSIZE *ctl_sie_iosize_get_obj(void);
void ctl_sie_iosize_init(CTL_SIE_ID id);

#endif //_CTL_SIE_IOSIZE_INT_H_
