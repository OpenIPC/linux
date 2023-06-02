/*
    ISE module driver

    NT98520 ISE internal header file.

    @file       kdrv_ise_int.h
    @ingroup    mIIPPISE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_ISE_INT_H_
#define _KDRV_ISE_INT_H_

#include "kwrap/task.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"
#include "kwrap/list.h"
#include "ise_eng.h"
#include "kdrv_gfx2d/kdrv_ise.h"

#define KDRV_ISE_JOB_HEAD_MAX_NUM (20)
#define KDRV_ISE_JOB_CFG_MAX_NUM (20)
#define KDRV_ISE_JOB_ID_MAX ((1 << 16) - 1)
#define KDRV_ISE_TIMEOUT_MS (1000)

#define KDRV_ISE_LL_CMD_SIZE (sizeof(KDRV_ISE_LL_CMD))
#define KDRV_ISE_LL_BLK_MAX_NUM (20)
#define KDRV_ISE_LL_REG_NUMS	(44)	/* for NT98520 */

#define KDRV_ISE_LL_CMD_TYPE_NULL	(0)
#define KDRV_ISE_LL_CMD_TYPE_NXTLL	(1)
#define KDRV_ISE_LL_CMD_TYPE_NXTUPD	(2)
#define KDRV_ISE_LL_CMD_TYPE_UPD	(4)

typedef struct vos_list_head KDRV_ISE_LIST_HEAD;

typedef enum {
	KDRV_ISE_JOB_TS_GENNODE = 0,
	KDRV_ISE_JOB_TS_PUTJOB,
	KDRV_ISE_JOB_TS_START,
	KDRV_ISE_JOB_TS_ENG_LOAD,
	KDRV_ISE_JOB_TS_ENG_END,
	KDRV_ISE_JOB_TS_MAX
} KDRV_ISE_JOB_TS;

typedef struct {
	void *p_parent_job_head;
	KDRV_ISE_IO_CFG io_cfg;

	UINT32 status;
	KDRV_ISE_LIST_HEAD pool_list;
	KDRV_ISE_LIST_HEAD cfg_list;

	ISE_ISR_CB eng_isr_cb;	/* callback fp for engine isr */
} KDRV_ISE_JOB_CFG;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	UINT32 job_id;
	KDRV_ISE_TRIG_PARAM trig_param;

	vk_spinlock_t lock;
	UINT32 job_num;
	UINT32 err_num;
	UINT32 done_num;
	UINT32 proc_num;

	KDRV_ISE_LIST_HEAD job_cfg_root;	/* list head of KDRV_ISE_JOB_CFG (cfg_list) */
	KDRV_ISE_LIST_HEAD ll_blk_root;	/* list head of KDRV_ISE_LL_BLK (grp_job_list) */
	KDRV_ISE_LIST_HEAD pool_list;		/* list for pool free/used list */
	KDRV_ISE_LIST_HEAD proc_list;		/* list for proc list */
	KDRV_ISE_LIST_HEAD cb_list;		/* list for cb list */
	KDRV_ISE_JOB_CFG *p_last_cfg;	/* last processing job cfg */

	KDRV_CALLBACK_FUNC cb_fp;

	UINT32 timestamp[KDRV_ISE_JOB_TS_MAX];
} KDRV_ISE_JOB_HEAD;

typedef union {
	struct {
		UINT64 reg_val			: 32;	/* bits : 31_0 */
		UINT64 reg_ofs			: 12;	/* bits : 43_32 */
		UINT64 byte_en			: 4;	/* bits : 47_44 */
		UINT64					: 13;
		UINT64 cmd				: 3;	/* bits : 63_61 */
	} upd_bit;

	struct {
		UINT64 table_index		: 8;	/* bits : 7_0 */
		UINT64					: 53;
		UINT64 cmd				: 3;	/* bits : 63_61 */
	} null_bit;

	struct {
		UINT64 table_index		: 8;	/* bits : 7_0 */
		UINT64 next_ll_addr		: 32;	/* bits : 39_8 */
		UINT64					: 21;
		UINT64 cmd				: 3;	/* bits : 63_61 */
	} nxtll_bit;

	struct {
		UINT64					: 8;	/* bits : 7_0 */
		UINT64 next_upd_addr	: 32;	/* bits : 39_8 */
		UINT64					: 21;
		UINT64 cmd				: 3;	/* bits : 63_61 */
	} nxtupd_bit;

	UINT64 val;
} KDRV_ISE_LL_CMD;

typedef struct {
	UINT32 cmd_buf_addr;
	UINT32 max_cmd_num;
	UINT32 cur_cmd_idx;
	UINT32 blk_idx;
	KDRV_ISE_JOB_HEAD *p_parent_job_head;

	KDRV_ISE_LIST_HEAD pool_list;		/* list for pool free/used list */
	KDRV_ISE_LIST_HEAD ll_wait_list;	/* list for ll wait list */
	KDRV_ISE_LIST_HEAD grp_job_list;	/* list for group job */

	/* linklist outbuf flush info */
	UINT32 out_buf_flush;
	UINT32 out_buf_addr;
	UINT32 out_buf_size;
} KDRV_ISE_LL_BLK;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	ID sem;
	ID internal_sem;
	UINT8 is_opened;
	UINT8 is_dma_abort;
	ISE_ENG_HANDLE *p_eng;

	vk_spinlock_t lock;
	UINT32 cur_job_id;
	UINT32 job_in_cnt;
	UINT32 job_out_cnt;
	BOOL is_obsolete_mode;
	KDRV_ISE_JOB_HEAD *p_cur_job;
	KDRV_ISE_JOB_CFG *p_fired_cfg;	/* engine processing cfg for cpu mode */
	KDRV_ISE_LL_BLK *p_fired_ll;	/* engine processing ll for ll mode */
	ID eng_sts_flg_id;

	THREAD_HANDLE proc_tsk_id;
	ID proc_tsk_flg_id;
	vk_spinlock_t job_list_lock;
	KDRV_ISE_LIST_HEAD job_list_root;

	THREAD_HANDLE cb_tsk_id;
	ID cb_tsk_flg_id;
	vk_spinlock_t cb_list_lock;
	KDRV_ISE_LIST_HEAD cb_list_root;

	vk_spinlock_t ll_wait_list_lock;
	KDRV_ISE_LIST_HEAD ll_wait_list_root;
} KDRV_ISE_HANDLE;

typedef struct {
	CHAR name[16];
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_size;
	UINT32 blk_num;
	vk_spinlock_t lock;
	UINT32 cur_free_num;
	UINT32 max_used_num;
	KDRV_ISE_LIST_HEAD blk_free_list_root; /* free block list head of buffer struct(depend on pool) */
	KDRV_ISE_LIST_HEAD blk_used_list_root; /* used block list head of buffer struct(depend on pool) */
} KDRV_ISE_MEM_POOL;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;

	KDRV_ISE_HANDLE *p_hdl;

	KDRV_ISE_MEM_POOL job_head_pool;
	KDRV_ISE_MEM_POOL job_cfg_pool;
	KDRV_ISE_MEM_POOL ll_blk_pool;
	ID pool_flg_id;
} KDRV_ISE_CTL;

typedef struct {
	INT32(*set_fp)(KDRV_ISE_HANDLE *p_hdl, void *p_data);
	BOOL set_chk_data;
	BOOL set_chk_cur_job;

	INT32(*get_fp)(KDRV_ISE_HANDLE *p_hdl, void *p_data);
	BOOL get_chk_data;
	BOOL get_chk_cur_job;

	CHAR *msg;
} KDRV_ISE_FUNC_ITEM;

typedef struct {
	UINT32 in_w_min;
	UINT32 in_w_max;
	UINT32 in_w_align;
	UINT32 in_h_min;
	UINT32 in_h_max;
	UINT32 in_h_align;
	UINT32 in_addr_align;
	UINT32 in_lofs_align;

	UINT32 out_w_min;
	UINT32 out_w_max;
	UINT32 out_w_align;
	UINT32 out_h_min;
	UINT32 out_h_max;
	UINT32 out_h_align;
	UINT32 out_addr_align;
	UINT32 out_lofs_align;
} KDRV_ISE_LMT;

INT32 kdrv_ise_sys_init(KDRV_ISE_CTL *p_ise_ctl);
INT32 kdrv_ise_sys_uninit(KDRV_ISE_CTL *p_ise_ctl);
INT32 kdrv_ise_job_process_cpu(KDRV_ISE_JOB_CFG *p_cfg, ISE_ENG_HANDLE *p_eng);
INT32 kdrv_ise_job_process_ll(KDRV_ISE_JOB_CFG *p_cfg, ISE_ENG_HANDLE *p_eng, KDRV_ISE_LL_BLK *p_ll_blk, KDRV_ISE_LL_BLK *p_nxt_ll_blk);

void* kdrv_ise_platform_malloc(UINT32 size);
void kdrv_ise_platform_free(void *ptr);

#endif
