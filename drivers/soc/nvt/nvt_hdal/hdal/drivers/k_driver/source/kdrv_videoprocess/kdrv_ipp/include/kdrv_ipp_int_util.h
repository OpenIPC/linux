/**
    Public header file for KDRV_IPP_utility

    This file is the header file that define data type for KDRV_IPP.

    @file       kdrv_ipp_int_util.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_IPP_INT_UTIL_H_
#define _KDRV_IPP_INT_UTIL_H_

#include "kdrv_videoprocess/kdrv_ipp_utility.h"
#include "kwrap/list.h"

/**
	atomic operations
*/
#if defined(__LINUX)
/* use linux kernel atomic api */
#else
/* use gcc api */
typedef INT32 atomic_t;

#define ATOMIC_INIT(i) {(i)}
#define atomic_inc(x) __sync_add_and_fetch(x, 1)
#define atomic_dec(x) __sync_sub_and_fetch(x, 1)
#define atomic_read(x) __sync_add_and_fetch(x, 0)
#define atomic_set(x, v) __sync_bool_compare_and_swap(x, *x, v)
#define atomic_add(i, x) __sync_add_and_fetch(x, i)
#define atomic_xchg(x, v) __sync_val_compare_and_swap(x, *x, v)

#endif

typedef struct vos_list_head KDRV_IPP_LIST_HEAD;

#define KDRV_IPP_LL_CMD_SIZE (sizeof(KDRV_IPP_LL_CMD))
#define KDRV_IPP_LL_CMD_TYPE_NULL	(0)
#define KDRV_IPP_LL_CMD_TYPE_NXTLL	(1)
#define KDRV_IPP_LL_CMD_TYPE_NXTUPD	(2)
#define KDRV_IPP_LL_CMD_TYPE_UPD	(4)

/*
	ENABLE: Enable direct mode global config
	DISABLE: 52x flow, use engine frame start load
*/
#define DIRECT_MODE_GLOBAL_LOAD_EN ENABLE

/*
	ENABLE: Enable eng check limitation flow when process end
	DISABLE: Disable eng check limitation flow
*/
#define KDRV_IPP_ENG_CHECK_ENABLE	(ENABLE)

typedef enum {
	KDRV_IPP_WRITE_REG_NONE = 0,
	KDRV_IPP_WRITE_REG_CPU,
	KDRV_IPP_WRITE_REG_LL,
	KDRV_IPP_WRITE_REG_MODE_MAX,
} KDRV_IPP_WRITE_REG_MODE;

typedef enum {
	KDRV_IPP_LL_SET_STAGE1 = 0,
	KDRV_IPP_LL_SET_STAGE2,
	KDRV_IPP_LL_SET_STAGE_MAX,
} KDRV_IPP_LL_SET_STAGE;

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
} KDRV_IPP_LL_CMD;

typedef struct {
	UINT32 max_cmd_num;
	UINT32 cmd_idx[KDRV_IPP_ENG_MAX];
	UINT32 cmd_buf_addr[KDRV_IPP_ENG_MAX];
	UINT32 blk_idx;
	void *p_parent_job_cfg;

	KDRV_IPP_LIST_HEAD pool_list;		/* list for pool free/used list */
	KDRV_IPP_LIST_HEAD ll_wait_list;	/* list for ll wait list */
	KDRV_IPP_LIST_HEAD grp_job_list;	/* list for group job */
} KDRV_IPP_LL_BLK;

typedef struct {
	UINT32 cmd_buf_addr;
	UINT32 max_cmd_num;
	UINT32 cur_cmd_idx;
	UINT32 blk_idx;
	KDRV_IPP_LL_SET_STAGE stage;
} KDRV_IPP_LL_SET_INFO;

typedef struct {
	UINT32 reg_ofs;
} KDRV_IPP_LL_OFS_IDX_MAP;


INT32 kdrv_ipp_util_write_ll_upd_cmd(KDRV_IPP_LL_SET_INFO *p_ll_blk, UINT32 reg_ofs, UINT32 reg_val);
INT32 kdrv_ipp_util_write_ll_null_cmd(KDRV_IPP_LL_SET_INFO *p_ll_blk, UINT32 idx);
INT32 kdrv_ipp_util_write_ll_nxtll_cmd(KDRV_IPP_LL_SET_INFO *p_ll_blk, UINT32 idx, UINT32 next_ll);
void kdrv_ipp_util_dump_ll_cmd(KDRV_IPP_LL_SET_INFO *p_ll_blk);

UINT32 kdrv_ipp_util_get_dtsi_clock(void);

#endif
