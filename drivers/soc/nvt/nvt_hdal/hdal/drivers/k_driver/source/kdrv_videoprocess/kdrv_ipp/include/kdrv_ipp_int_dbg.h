#ifndef __KDRV_IPP_INT_DBG_H_
#define __KDRV_IPP_INT_DBG_H_

#define __MODULE__	kdrv_ipp
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "kwrap/error_no.h"

extern unsigned int kdrv_ipp_debug_level;

/* dump global information */
void kdrv_ipp_dbg_dump(void);

/* dump job information */
void kdrv_ipp_dbg_dump_cfg(void *p_data);
void kdrv_ipp_dbg_dump_ll(void *p_data);
void kdrv_ipp_dbg_dump_job(void *p_data);
void kdrv_ipp_dbg_dump_register(void *p_data);
void kdrv_ipp_dbg_dump_fastboot(void *p_data);
UINT8 kdrv_ipp_dbg_job(UINT8 op, UINT8 val);
void kdrv_ipp_dbg_fastboot_pause_stream_en(BOOL en);
void kdrv_ipp_dbg_fastboot_bypass_algo_en(BOOL en);
void kdrv_ipp_dbg_fastboot_stamp_mode_en(BOOL en);
UINT32 kdrv_ipp_dbg_func_en(UINT32 id, UINT32 dbg_func_en, BOOL en);

/* dump timestamp */
void kdrv_ipp_dbg_dump_job_ts(void *p_data);
UINT8 kdrv_ipp_dbg_timestamp(UINT8 op, UINT8 val);

#endif	//__KDRV_IPP_DBG_INT_H_

