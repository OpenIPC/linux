#ifndef __KDRV_IFE_DBG_H_
#define __KDRV_IFE_DBG_H_

#define __MODULE__	kdrv_ife
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "comm/hwclock.h"

extern unsigned int kdrv_ife_debug_level;

#define IFE_STATUS_RECORD_NUM (100)


typedef enum {
	KDRV_IFE_DBG_IFE_LINEBUF_STATUS_REC_EN = 0,
	KDRV_IFE_DBG_IFE_FUSION_DBG_EN         = 1,
	KDRV_IFE_DBG_PARAM_ID_MAX
} KDRV_IFE_DBG_PARAM_ID;


typedef struct{
	UINT32  chip_id;
	UINT32  eng_id;
}KDRV_IFE_DBG_CMD_HDL;

typedef struct{

	BOOL    rec_en;
	INT32   rec_num;
	BOOL    bypass_isr_cb;

}KDRV_IFE_DBG_REC_CTL_PARAM;

typedef struct{

	BOOL    fusion_dbg_en;
	UINT32   out_mode;

}KDRV_IFE_DBG_FUSION_OUT_CTL_PARAM;

typedef struct{

	UINT32  ife_overflow_cnt;
	UINT32  ife_buf_overflow_status;
	UINT32 *ife_buf_rec_status;
	UINT32  ife_buf_current_status;
	UINT32  ife_record_idx;
	UINT32  ife_record_num;

}KDRV_IFE_DBG_INFO;


typedef struct{

	BOOL ife_record_en;
	BOOL ife_bypass_isr_cb;
	BOOL ife_fusion_dbg_en;
	UINT32 ife_fusion_out_mode;

}KDRV_IFE_DBG_CTL;

#endif

