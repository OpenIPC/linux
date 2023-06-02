#ifndef __KDRV_ISE_INT_DBG_H__
#define __KDRV_ISE_INT_DBG_H__

#include "kwrap/perf.h"
#include "kwrap/error_no.h"
#include "kdrv_ise_int.h"
#include "kdrv_gfx2d/kdrv_ise.h"

#define __MODULE__    kdrv_ise
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include "kwrap/debug.h"

extern unsigned int kdrv_ise_debug_level;

void kdrv_ise_dbg_init(KDRV_ISE_CTL *p_ctl);
void kdrv_ise_dbg_uninit(void);

void kdrv_ise_dump_ll_blk_cmd(KDRV_ISE_LL_BLK *p_ll_blk);
void kdrv_ise_dump_cfg(KDRV_ISE_JOB_CFG *p_job_cfg);
void kdrv_ise_dump_all(void);

void kdrv_ise_set_dump_ts(UINT32 cnt);
void kdrv_ise_dump_ts(KDRV_ISE_JOB_HEAD *p_job);

void kdrv_ise_set_dbg_level(UINT32 lv);

#endif