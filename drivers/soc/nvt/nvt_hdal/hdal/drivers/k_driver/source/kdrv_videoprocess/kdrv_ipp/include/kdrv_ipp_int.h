/*
    IPP module driver

    NT98520 IPP internal header file.

    @file       kdrv_ipp_int.h
    @ingroup    mIIPP
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_IPP_INT_H_
#define _KDRV_IPP_INT_H_

#if defined(__LINUX)
#elif defined(__FREERTOS)
#include <string.h>
#endif
#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "kdrv_videoprocess/kdrv_ipp.h"
#include "kdrv_ipp_int_dbg.h"
#include "kdrv_ipp_int_util.h"


#define KDRV_IPP_VIRTUAL_CHIP_NUM	(1)
#define KDRV_IPP_VIRTUAL_ENG_NUM	(1)
#define KDRV_IPP_VIRTUAL_CH_NUM		(1)

#define KDRV_IPP_JOB_ID_MAX ((1 << 16) - 1)
#define KDRV_IPP_TIMEOUT_MS (5000)

typedef struct {
	UINT8 is_direct;
	UINT16 done_eng_bit;
	UINT16 start_eng_bit;
	KDRV_IPP_STATUS status;
	UINT64 frm_st_timestamp;	/* keep last frame start timestamp */
} KDRV_IPP_DIR_INFO;

typedef struct {
	void *p_parent_job_head;
	KDRV_IFE_IO_CFG ife_io_cfg;
	KDRV_DCE_IO_CFG dce_io_cfg;
	KDRV_IPE_IO_CFG ipe_io_cfg;
	KDRV_IME_IO_CFG ime_io_cfg;
	KDRV_IFE_IQ_CFG ife_iq_cfg;
	KDRV_DCE_IQ_CFG dce_iq_cfg;
	KDRV_IPE_IQ_CFG ipe_iq_cfg;
	KDRV_IME_IQ_CFG ime_iq_cfg;
	UINT32 cb_inte[KDRV_IPP_ENG_MAX];
	UINT16 wait_eng_mask;	/* frame end wait mask */
	UINT16 start_eng_mask;	/* frame start wait mask */
	UINT16 done_eng_bit;	/* frame end received bit */
	UINT16 start_eng_bit;	/* frame start received bit */
	UINT16 trig_eng_bit;	/* debug use */

	UINT32 status;
	UINT32 owner;
	KDRV_IPP_LIST_HEAD pool_list;
	KDRV_IPP_LIST_HEAD cfg_list;
} KDRV_IPP_JOB_CFG;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	UINT32 job_id;
	KDRV_IPP_TRIG_PARAM trig_param;

	vk_spinlock_t lock;
	UINT8 job_num;
	UINT8 err_num;
	UINT8 done_num;
	UINT8 proc_num;
	UINT8 start_num;

	KDRV_IPP_LIST_HEAD job_cfg_root;/* list head of KDRV_IPP_JOB_CFG (cfg_list) */
	KDRV_IPP_LIST_HEAD ll_blk_root;	/* list head of KDRV_IPP_LL_BLK (grp_job_list) */
	KDRV_IPP_LIST_HEAD pool_list;	/* list for pool free/used list */
	KDRV_IPP_LIST_HEAD proc_list;	/* list for proc list */
	KDRV_IPP_LIST_HEAD cb_list;		/* list for cb list */
	KDRV_IPP_JOB_CFG *p_last_cfg;	/* last processing job cfg */

	KDRV_CALLBACK_FUNC cb_fp;

	UINT32 timestamp[KDRV_IPP_JOB_TS_MAX];
} KDRV_IPP_JOB_HEAD;

typedef struct {
	VOS_TICK acc_start_tick;
	UINT16 acc_frm_cnt;
	UINT16 fps;

	UINT32 acc_eng_time;
	UINT32 eng_usage;
} KDRV_IPP_DBG_PERF;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	SEM_HANDLE sem;

	vk_spinlock_t lock;
	UINT32 cur_job_id;
	UINT32 job_in_cnt;
	UINT32 job_out_cnt;
	UINT8 is_dma_abort;
	KDRV_IPP_DIR_INFO direct_info;
	KDRV_IPP_JOB_HEAD *p_cur_job;	/* job in configuration, wait for trigger to add to list */
	KDRV_IPP_JOB_CFG *p_fired_cfg;	/* engine processing cfg for cpu mode */
	KDRV_IPP_JOB_CFG *p_ll_fired_cfg;  /* engine processing cfg for ll mode */
	KDRV_IPP_LL_BLK *p_fired_ll;	/* engine processing ll for ll mode */
	ID eng_sts_flg_id;

	THREAD_HANDLE proc_tsk_id;
	ID proc_tsk_flg_id;
	vk_spinlock_t job_list_lock;
	KDRV_IPP_LIST_HEAD job_list_root;

	THREAD_HANDLE cb_tsk_id;
	ID cb_tsk_flg_id;
	vk_spinlock_t cb_list_lock;
	KDRV_IPP_LIST_HEAD cb_list_root;

	vk_spinlock_t ll_wait_list_lock;
	KDRV_IPP_LIST_HEAD ll_wait_list_root;

	KDRV_IPP_DBG_PERF dbg_perf_info;
} KDRV_IPP_HANDLE;

typedef struct {
	CHAR name[16];
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_size;
	UINT8 blk_num;
	vk_spinlock_t lock;
	UINT8 cur_free_num;
	UINT8 max_used_num;
	KDRV_IPP_LIST_HEAD blk_free_list_root; /* free block list head of buffer struct(depend on pool) */
	KDRV_IPP_LIST_HEAD blk_used_list_root; /* used block list head of buffer struct(depend on pool) */
} KDRV_IPP_MEM_POOL;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;

	KDRV_IPP_HANDLE *p_hdl;
	UINT8 start_from_fastboot;

	KDRV_IPP_MEM_POOL job_head_pool;
	KDRV_IPP_MEM_POOL job_cfg_pool;
	KDRV_IPP_MEM_POOL ll_blk_pool;
	UINT32 ll_eng_cmd_num[KDRV_IPP_ENG_MAX];
	ID pool_flg_id;
} KDRV_IPP_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_IPP_HANDLE *p_hdl, void *p_data);
	UINT8 set_chk_data;
	UINT8 set_chk_cur_job;

	INT32(*get_fp)(KDRV_IPP_HANDLE *p_hdl, void *p_data);
	UINT8 get_chk_data;
	UINT8 get_chk_cur_job;

	CHAR *msg;
} KDRV_IPP_FUNC_ITEM;

INT32 kdrv_ipp_int_stripe_preprocess(KDRV_IPP_JOB_CFG *p_cfg);
INT32 kdrv_ipp_job_process_cpu(KDRV_IPP_HANDLE *p_hdl, KDRV_IPP_JOB_CFG *p_cfg, KDRV_IPP_TRIG_PARAM *p_trig_param);
INT32 kdrv_ipp_job_process_ll(KDRV_IPP_HANDLE *p_hdl, KDRV_IPP_TRIG_PARAM *p_trig_param, KDRV_IPP_JOB_CFG *p_cfg, KDRV_IPP_LL_BLK *p_ll_blk, UINT32 *eng_cmd_num, UINT32 stage);
INT32 kdrv_ipp_job_trig_ll(KDRV_IPP_LL_BLK *p_ll_blk);
INT32 kdrv_ipp_job_stop_eng_direct(KDRV_IPP_HANDLE *p_hdl, void *reserve);

#endif //_KDRV_IPP_INT_H_
