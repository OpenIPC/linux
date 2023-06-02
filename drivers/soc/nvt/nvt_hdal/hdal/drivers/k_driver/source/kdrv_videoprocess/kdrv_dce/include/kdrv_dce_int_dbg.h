#ifndef __KDRV_DCE_DBG_H_
#define __KDRV_DCE_DBG_H_

#define __MODULE__	kdrv_dce
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "comm/hwclock.h"

extern unsigned int kdrv_dce_debug_level;

typedef enum {
	KDRV_DCE_DBG_CAC_EN = 0,
	KDRV_DCE_DBG_PARAM_ID_MAX
} KDRV_DCE_DBG_PARAM_ID;


typedef struct{
	UINT32  chip_id;
	UINT32  eng_id;
}KDRV_DCE_DBG_CMD_HDL;

typedef struct{

	UINT32 reserve;

}KDRV_DCE_DBG_INFO;

typedef struct{

	BOOL cac_dbg_mode_en;
	BOOL cac_en;
	BOOL cac_update;

}DBG_CAC_CTL;

typedef struct{

	DBG_CAC_CTL cac_ctl;

}KDRV_DCE_DBG_CTL;

#endif
