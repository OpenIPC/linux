/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/gfp.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#include "ax_gzipd_api.h"
#include "ax_gzipd_drv.h"
#include "ax_gzipd_mng.h"
#include "ax_gzipd_adapter.h"
#include "ax_gzipd_sys.h"

static AX_GZIPD_INST_HANDLE_RES_T inst_handle[AX_GZIP_INST_HANDLE_MAX];
static DEFINE_SPINLOCK(gzipd_handle_spinlock);

static DEFINE_MUTEX(gzipd_instance_mutex);
static DEFINE_MUTEX(gzipd_tiles_mutex);

static LIST_HEAD(gzip_instanc_listhead);
static LIST_HEAD(gzip_tile_listhead);
static LIST_HEAD(gzipd_inst_group_listhead);

static gzipd_thread_t g_tile_thread_handle;
static gzipd_thread_cond_t start_tile_cond;
static DEFINE_SPINLOCK(gzipd_irq_spinlock);
static gzipd_thread_cond_t gzipd_irq_cond;
static volatile bool is_run_thread = false;
DEFINE_MUTEX(tile_mutex);

static volatile uint32_t gzipd_intr_status;

static volatile bool gzipd_irq_wait_exit = false;

void gzipd_instance_init_handle(void)
{
	int32_t i;
	for (i = 0; i < AX_GZIP_INST_HANDLE_MAX; i++) {
		inst_handle[i].available = true;
		inst_handle[i].handle_idx = i;
	}
}

static uint32_t gzipd_instance_request_handle(void)
{
	int32_t i;
	spin_lock(&gzipd_handle_spinlock);
	for (i = 0; i < AX_GZIP_INST_HANDLE_MAX; i++) {
		if (inst_handle[i].available) {
			inst_handle[i].available = false;
			break;
		}
	}
	spin_unlock(&gzipd_handle_spinlock);

	if (i == AX_GZIP_INST_HANDLE_MAX) {
		return -1;
	}
	return i;
}

static void gzipd_instance_recovery_handle(uint32_t handle)
{
	if (handle > AX_GZIP_INST_HANDLE_MAX) {
		AX_GZIP_DEV_LOG_ERR("invalid handle %d", handle);
		return;
	}

	spin_lock(&gzipd_handle_spinlock);
	if (!inst_handle[handle].available) {
		inst_handle[handle].available = true;
	}
	spin_unlock(&gzipd_handle_spinlock);
}

static GZIPD_INSTANCE_INFO_T *gzipd_instance_find_ertry(uint32_t handle)
{
	struct list_head *iter;

	mutex_lock(&gzipd_instance_mutex);
	list_for_each(iter, &gzip_instanc_listhead) {
		GZIPD_INSTANCE_INFO_T *pInst = list_entry(iter, GZIPD_INSTANCE_INFO_T, list);
		AX_GZIP_DEV_LOG_DBG("find pInst=0x%px", pInst);
		if (pInst->u32Handle == handle) {
			mutex_unlock(&gzipd_instance_mutex);
			return pInst;
		}
	}
	mutex_unlock(&gzipd_instance_mutex);
	return NULL;
}


static GZIPD_INSTANCE_INFO_T *gzipd_instanc_find_first_ready_entry(void)
{
	struct list_head *iter;

	list_for_each(iter, &gzip_instanc_listhead) {
		GZIPD_INSTANCE_INFO_T *pInst = list_entry(iter, GZIPD_INSTANCE_INFO_T, list);
		if (pInst->eStatus != GZIPD_INSTANCE_COMPLETE) {
			return pInst;
		}
	}
	return NULL;
}

static int gzipd_instance_init(GZIPD_INSTANCE_INFO_T *pstInst)
{
	CHECK_POINTER_NULL(pstInst);

	INIT_LIST_HEAD(&pstInst->tile_list_head);
	mutex_init(&pstInst->tile_mutex);
	spin_lock_init(&pstInst->listlock);
	gzipd_thread_cond_init(&pstInst->completed_cond);
	return AX_GZIPD_STATUS_OK;
}

static int gzipd_instance_deinit(GZIPD_INSTANCE_INFO_T *pstInst)
{
	CHECK_POINTER_NULL(pstInst);
	gzipd_thread_cond_deinit(&pstInst->completed_cond);
	return 	AX_GZIPD_STATUS_OK;
}

static void push_tile_to_queue(GZIPD_TILE_INFO_T *pstTile, GZIPD_INSTANCE_INFO_T *pstInst)
{
	list_add_tail(&pstTile->list, &pstInst->tile_list_head);
	if (pstTile->eTileType != GZIPD_TILE_TYPE_CONFIG_TILE) {
		pstInst->u32CurrTileIndex++;
	}
}

static void pop_tile_from_queue(GZIPD_INSTANCE_INFO_T *pstInst, GZIPD_TILE_INFO_T **pstTile)
{
	GZIPD_TILE_INFO_T *pTile = NULL;
	if (!list_empty(&pstInst->tile_list_head)) {
		pTile = list_first_entry(&pstInst->tile_list_head, GZIPD_TILE_INFO_T, list);
		if (pTile)
			list_del_init(&pTile->list);
	}
	*pstTile = pTile;
}

static int gzipd_instance_finish(void *pstInst, unsigned int result, void *private_data)
{
	uint32_t irqsts;
	GZIPD_INSTANCE_INFO_T *pInst;
	GZIPD_INSTANCE_COMPLETED_T *pInstfinish;
	GZIPD_INSTANCE_GROUP_T	*pInstGroup = (GZIPD_INSTANCE_GROUP_T *)private_data;

	CHECK_POINTER_NULL(pstInst);
	if (result != GZIPD_INSTANCE_COMPLETE) {
		AX_GZIP_DEV_LOG_ERR(" gzipd instance error status ");
		return 0;
	}

	pInstfinish = kzalloc(sizeof(GZIPD_INSTANCE_COMPLETED_T), GFP_KERNEL);
	CHECK_POINTER_NULL(pInstfinish);

	irqsts = gzipd_irq_get_irq_status();
	irqsts &= 0x1;
	pInstfinish->complete_result = irqsts? GZIPD_INST_COMPLETE_SUCCESS : GZIPD_INST_COMPLETE_FAIL;
	pInst = (GZIPD_INSTANCE_INFO_T *)pstInst;
	pInstfinish->handle = pInst->u32Handle;

	spin_lock(&pInstGroup->completed_lock);
	atomic_inc(&pInstGroup->refcnt);
	list_add_tail(&pInstfinish->entry, &gzipd_inst_group_listhead);
	AX_GZIP_DEV_LOG_DBG("finish callback handle = %d", pInstfinish->handle);
	gzipd_thread_cond_broadcast(&pInst->completed_cond);
	spin_unlock(&pInstGroup->completed_lock);

	return 0;
}

static int gzipd_tile_thread(void *arg)
{
	unsigned long irqflag;
	GZIPD_INSTANCE_STATUS_E result = GZIPD_INSTANCE_IDLE;
	(void)arg;
	for (; is_run_thread; ) {
		int ret;
		bool waitflag = true;
		GZIPD_INSTANCE_INFO_T *cur_inst;
		GZIPD_TILE_INFO_T *cur_tile;

		mutex_lock(&gzipd_instance_mutex);
		if (list_empty(&gzip_instanc_listhead)) {
			waitflag = true;
		} else {
			cur_inst = gzipd_instanc_find_first_ready_entry();
			if (cur_inst) {
				mutex_lock(&cur_inst->tile_mutex);
				if (list_empty(&cur_inst->tile_list_head)) {
					waitflag = true;
				} else {
					waitflag = false;
				}
				mutex_unlock(&cur_inst->tile_mutex);
			} else {
				waitflag = true;
			}
		}

		if (waitflag) {
			do {
				ret = gzipd_thread_cond_waittime(&start_tile_cond, &gzipd_instance_mutex, &irqflag, gzipd_lock_type_schedule, 100);
				if (!list_empty(&gzip_instanc_listhead)) {
					ret = 0;
					break;
				}
				if (!is_run_thread) {
					INIT_LIST_HEAD(&gzip_instanc_listhead);
					mutex_unlock(&gzipd_instance_mutex);
					return 0;
				}
			}while (ret == GZIPD_CONDITION_TIMEOUT);
		} else {
			/* clear the condition when handle more tile continously */
			start_tile_cond.condition = 0;
		}

		/* again get new instance when new tile wakeup it, or instance is NULL */
		cur_inst = gzipd_instanc_find_first_ready_entry();
		if (!cur_inst) {
			continue;
		}

		mutex_lock(&cur_inst->tile_mutex);
		spin_lock(&cur_inst->listlock);
		pop_tile_from_queue(cur_inst, &cur_tile);
		spin_unlock(&cur_inst->listlock);
		if (NULL != cur_tile) {
			uint32_t tilelen;
			// AX_GZIP_DEV_LOG_DBG("tiletype[%d], cur_tile[%d].startAddr = 0x%llx, tile_len = %d",
			// 		cur_tile->eTileType, cur_tile->u32TileIndex, cur_tile->stTileStartBuf.pPhyAddr, cur_tile->u32TileLen);
			if (cur_tile->eTileType == GZIPD_TILE_TYPE_CONFIG_TILE) {
				gzipd_dev_tile_config(cur_tile->stOutDataBuf.pPhyAddr, cur_tile->u32OutDataSize,
										cur_tile->u32BlockNum, cur_tile->u32TilesNum);
			} else {
				cur_inst->u32EnqueuedTileCnt++;
				if (cur_inst->u32EnqueuedTileCnt) {
					gzipd_dev_trigger_once();
				}

				tilelen = cur_tile->u32TileLen;
retry:
				ret = gzipd_dev_tile_cmdq(cur_tile->stTileStartBuf.pPhyAddr, tilelen);
				if (ret) { /* enqueue one tile fail when cmdq is full, retry */
					gzipd_udelay(1000);
					goto retry;
				}

				if (cur_tile->eTileType ==  GZIPD_TILE_TYPE_LAST_TILE) {
					AX_GZIP_DEV_LOG_DBG("wait gzipd complete irq....");
					ret = gzipd_irq_cond_wait();
					if (ret == 0) {
						cur_inst->eStatus = GZIPD_INSTANCE_COMPLETE;
						result = cur_inst->eStatus;
					}
					gzipd_dev_trigger_restore();
				}
			}
			kfree(cur_tile);
		}
		mutex_unlock(&cur_inst->tile_mutex);

		if (!is_run_thread) {
			INIT_LIST_HEAD(&gzip_instanc_listhead);
			mutex_unlock(&gzipd_instance_mutex);
			return 0;
		}
		mutex_unlock(&gzipd_instance_mutex);

		if (cur_inst->eStatus == GZIPD_INSTANCE_COMPLETE && cur_inst->pFinishFunc != NULL) {
			AX_GZIP_DEV_LOG_DBG("complete cur_inst. handle = %d", cur_inst->u32Handle);
			cur_inst->eStatus = GZIPD_INSTANCE_IDLE;
			cur_inst->pFinishFunc(cur_inst, result, cur_inst->pUserData);
		}

	}

	AX_GZIP_DEV_LOG_DBG("gzipd tile thread exit");
	return 0;
}

void gzipd_irq_save_status(uint32_t irq_sts_val)
{
	gzipd_intr_status = irq_sts_val;
}

uint32_t gzipd_irq_get_irq_status(void)
{
	return gzipd_intr_status;
}

void gzipd_irq_cond_broadcast(void)
{
	unsigned long irqflag;

	spin_lock_irqsave(&gzipd_irq_spinlock, irqflag);
	gzipd_thread_cond_broadcast(&gzipd_irq_cond);
	spin_unlock_irqrestore(&gzipd_irq_spinlock, irqflag);
}

int32_t gzipd_irq_cond_wait(void)
{
	uint32_t ret = 0;
	unsigned long irqflag;

	spin_lock_irqsave(&gzipd_irq_spinlock, irqflag);
	do {
		ret = gzipd_thread_cond_waittime(&gzipd_irq_cond, &gzipd_irq_spinlock, &irqflag, gzipd_lock_type_inturrupt, 500);
		if (gzipd_irq_wait_exit == true || !ret ) {
			spin_unlock_irqrestore(&gzipd_irq_spinlock, irqflag);
			return ret;
		}
	} while (ret == GZIPD_CONDITION_TIMEOUT);

	spin_unlock_irqrestore(&gzipd_irq_spinlock, irqflag);
	return ret;
}

void gzipd_interrupt_wait_enter(void)
{
	gzipd_irq_wait_exit = false;
}

void gzipd_interrupt_wait_exit(void)
{
	gzipd_irq_wait_exit = true;
}

int gzipd_thread_init(void)
{
	is_run_thread = true;
	gzipd_thread_cond_init(&start_tile_cond);
	gzipd_thread_cond_init(&gzipd_irq_cond);
	g_tile_thread_handle = gzipd_thread_create_v2(gzipd_tile_thread, NULL, "gzipd_tile_thread", gzipd_thread_priority_1);
	if (!g_tile_thread_handle) {
		AX_GZIP_DEV_LOG_ERR("create gzipd thread fail");
		return AX_GZIPD_STATUS_FAIL;
	}
	gzipd_interrupt_wait_enter();
	return AX_GZIPD_STATUS_OK;
}

int gzipd_thread_deinit(void)
{
	AX_GZIP_DEV_LOG_DBG("enter");
	is_run_thread = false;
	gzipd_interrupt_wait_exit();
	if (g_tile_thread_handle) {
		kthread_stop(g_tile_thread_handle);
		g_tile_thread_handle = NULL;
	}
	gzipd_thread_cond_deinit(&gzipd_irq_cond);
	gzipd_thread_cond_deinit(&start_tile_cond);
	AX_GZIP_DEV_LOG_DBG("exit");
	return 0;
}

int gzipd_instance_group_init(GZIPD_INSTANCE_GROUP_T **pInstGroup)
{
	GZIPD_INSTANCE_GROUP_T * pGroup;
	pGroup = kzalloc(sizeof(GZIPD_INSTANCE_GROUP_T), GFP_KERNEL);
	CHECK_POINTER_NULL(pGroup);

	// gzipd_thread_cond_init(&pGroup->completed_cond);
	atomic_set(&pGroup->refcnt, 0);
	spin_lock_init(&pGroup->completed_lock);
	INIT_LIST_HEAD(&pGroup->completed_list);
	*pInstGroup = pGroup;

	return AX_GZIPD_STATUS_OK;
}

int gzipd_instance_group_deinit(GZIPD_INSTANCE_GROUP_T *pInstGroup)
{
	CHECK_POINTER_NULL(pInstGroup);

	if (atomic_read(&pInstGroup->refcnt) == 0) {
		spin_lock(&pInstGroup->completed_lock);
		while(!list_empty(&gzipd_inst_group_listhead)) {
			GZIPD_INSTANCE_COMPLETED_T *node = list_first_entry(&gzipd_inst_group_listhead, GZIPD_INSTANCE_COMPLETED_T, entry);
			if (node) {
				list_del(&node->entry);
				kfree(node);
			}
		}
		spin_unlock(&pInstGroup->completed_lock);
		kfree(pInstGroup);
		AX_GZIP_DEV_LOG_DBG("free instance group, pInstGroup=0x%px", pInstGroup);
	}
	return AX_GZIPD_STATUS_OK;
}

int	Gzipd_Instance_Create(uint32_t handle, void *private_data)
{
	GZIPD_INSTANCE_INFO_T *pstInstance;
	GZIPD_INSTANCE_INFO_T *pInst;

	CHECK_HANDEL_VALID(handle);

	pstInstance = kzalloc(sizeof(GZIPD_INSTANCE_INFO_T), GFP_KERNEL);
	if(!pstInstance) {
		AX_GZIP_DEV_LOG_ERR(" not enough mem for alloc ");
		return AX_GZIPD_STATUS_NO_MEM;
	}

	pstInstance->eStatus = GZIPD_INSTANCE_IDLE;
	pstInstance->u32Handle = handle;
	pstInstance->pFinishFunc = gzipd_instance_finish;
	pstInstance->pUserData = private_data;
	AX_GZIP_DEV_LOG_DBG("current handle [%d] 's instance = 0x%px", handle, pstInstance);
	mutex_lock(&gzipd_instance_mutex);
	gzipd_instance_init(pstInstance);
	list_add_tail(&pstInstance->list, &gzip_instanc_listhead);

	pInst = list_last_entry(&gzip_instanc_listhead, GZIPD_INSTANCE_INFO_T, list);
	mutex_unlock(&gzipd_instance_mutex);

	return AX_GZIPD_STATUS_OK;
}

static int gzipd_inst_get_header_info(void *pData, AX_GZIPD_HEADER_INFO_T *pInfo)
{
	AX_U8 type[2];
	if (pData == NULL || pInfo == NULL) {
		AX_GZIP_DEV_LOG_ERR(" input param is NULL, pData = 0x%p, pInfo = 0x%p", pData, pInfo);
		return AX_GZIPD_INVALID_PARAM;
	}

	memcpy(&type[0], pData, 2);
	if (memcmp(&type[0], "20", 2)) {
		AX_GZIP_DEV_LOG_ERR("This data is without gzipd header info [%x, %x]", *(char *)pData, *((char *)pData + 1));
		return AX_GZIPD_INVALID_PARAM;
	}
	memcpy(pInfo, pData, sizeof(AX_GZIPD_HEADER_INFO_T));
	AX_GZIP_DEV_LOG_DBG(" AXZIP data info .blk_num = %d, .isize = %d, osize = %d, icrc = %d",
					pInfo->u16BlkNum, pInfo->u32InSize, pInfo->u32OutSize, pInfo->u32Crc32);

	return AX_GZIPD_STATUS_OK;
}

int gzipd_instance_create_handle(AX_U32 *handle, AX_U64 *gzipdData, AX_GZIPD_HEADER_INFO_T *pInfo, void *private_data)
{
	AX_U32 found_handle;
	if (gzipd_inst_get_header_info(gzipdData, pInfo)) {
		AX_GZIP_DEV_LOG_ERR("Invalid parameter");
		return AX_GZIPD_INVALID_PARAM;
	}

	found_handle = gzipd_instance_request_handle();
	if (found_handle == -1) {
		AX_GZIP_DEV_LOG_ERR(" Insufficient handle for new gzipd work");
		return AX_GZIPD_NO_ENOUGH_RES;
	}

	*handle = found_handle;
	return Gzipd_Instance_Create(*handle, private_data);
}

static int gzipd_tile_submit(GZIPD_INSTANCE_INFO_T *pstInst, GZIPD_TILE_INFO_T *pstTile)
{
	CHECK_POINTER_NULL(pstInst);
	CHECK_POINTER_NULL(pstTile);

	push_tile_to_queue(pstTile, pstInst);

	return AX_GZIPD_STATUS_OK;
}

static int gzipd_tile_submit_configtile(GZIPD_INSTANCE_INFO_T *pInst)
{
	GZIPD_TILE_INFO_T *pCfgTile;
	GZIPD_CFG_INFO_T *pCfgInfo = &pInst->stCFGInfo;
	pCfgTile = kzalloc(sizeof(GZIPD_TILE_INFO_T), GFP_ATOMIC);
	CHECK_POINTER_NULL(pCfgTile);

	spin_lock(&pInst->listlock);
	pCfgTile->stOutDataBuf.pPhyAddr = pCfgInfo->u32OutPhyAddr;
	pCfgTile->u32OutDataSize = pCfgInfo->u32OutDataSize;
	pCfgTile->u32BlockNum = pCfgInfo->u32BlockNum;
	pCfgTile->u32TilesNum = pCfgInfo->u32TileCount;
	pCfgTile->eTileType = GZIPD_TILE_TYPE_CONFIG_TILE;
	gzipd_tile_submit(pInst, pCfgTile);
	gzipd_thread_cond_broadcast(&start_tile_cond);
	spin_unlock(&pInst->listlock);

	return 0;
}

int gzipd_instance_cfg(uint32_t handle, AX_GZIPD_IO_PARAM_T *pCfgParam, uint64_t *pTileCnt, uint64_t *pLastTileSize)
{
	uint64_t tileSize = pCfgParam->u32TileSize;
	uint64_t out_addr = pCfgParam->stOutBuf.pPhyAddr;
	uint64_t isize = pCfgParam->headerInfo.u32InSize;
	uint64_t osize = pCfgParam->headerInfo.u32OutSize;
	uint64_t blk_num = pCfgParam->headerInfo.u16BlkNum;
	uint64_t tile_cnt;
	uint64_t remainder;
	const uint64_t MinTileSz = 8 * 1024;
	uint64_t validDataLen;
	GZIPD_INSTANCE_INFO_T *pInst;
	AX_GZIP_DEV_LOG_DBG("handle = %d, enter", handle);

	CHECK_HANDEL_VALID(handle);
	CHECK_POINTER_NULL(pCfgParam);

	if (tileSize % MinTileSz && !(tileSize / MinTileSz)) {
		AX_GZIP_DEV_LOG_ERR("tile size MUST more than 8KB and a intergral multiple of it \n");
		return gzipd_error;
	}

	if (out_addr == 0) {
		AX_GZIP_DEV_LOG_ERR("output address is zero");
		return gzipd_error;
	}

	if (isize == 0 || osize == 0 || blk_num == 0) {
		AX_GZIP_DEV_LOG_ERR("isize , osize or blk_num is 0");
		return gzipd_error;
	}

	pInst = gzipd_instance_find_ertry(handle);
	CHECK_POINTER_NULL(pInst);

	validDataLen = isize - sizeof(GZIPD_FILE_HEADER_T);
	tile_cnt = validDataLen;
	remainder = do_div(tile_cnt, tileSize);
	if (tile_cnt && remainder < MinTileSz) {
		tile_cnt = tile_cnt - 1;
	}

	*pTileCnt = tile_cnt;
	*pLastTileSize = tile_cnt? validDataLen - tile_cnt * tileSize : validDataLen;

	pCfgParam->tilesNum = tile_cnt;
	pCfgParam->lastTileLen = *pLastTileSize;

	pInst->u32CurrTileIndex = 0;
	pInst->u32TilesTotalNum = pCfgParam->tilesNum;
	pInst->u32LastTilesSZ = pCfgParam->lastTileLen;
	pInst->u32TileSize = pCfgParam->u32TileSize;
	pInst->u64EndTime = pInst->u64StartTime = 0;

	pInst->stCFGInfo.u32BlockNum = pCfgParam->headerInfo.u16BlkNum;
	pInst->stCFGInfo.u32ByPassEn = 0;
	pInst->stCFGInfo.u32OutDataSize = osize;
	pInst->stCFGInfo.u32OutPhyAddr = out_addr;
	pInst->stCFGInfo.u32TileCount = tile_cnt;

	gzipd_tile_submit_configtile(pInst);
	AX_GZIP_DEV_LOG_DBG("exit");
	return AX_GZIPD_STATUS_OK;
}

static int gzipd_tile_submit_onetile(GZIPD_INSTANCE_INFO_T *pInst, uint64_t startAddr, uint32_t tilelen, bool bLastTile)
{
	GZIPD_TILE_INFO_T *pTile;
	pTile = kzalloc(sizeof(GZIPD_TILE_INFO_T), GFP_ATOMIC);
	CHECK_POINTER_NULL(pTile);

	spin_lock(&pInst->listlock);
	pTile->stTileStartBuf.pPhyAddr = startAddr;
	pTile->u32TileLen = tilelen;
	pTile->u32TileIndex = pInst->u32CurrTileIndex;
	pTile->eTileType = GZIPD_TILE_TYPE_NORMAL_TILE;
	if (bLastTile) {
		pTile->eTileType = GZIPD_TILE_TYPE_LAST_TILE;
	}
	gzipd_tile_submit(pInst, pTile);
	gzipd_thread_cond_broadcast(&start_tile_cond);
	spin_unlock(&pInst->listlock);

	return 0;
}

int gzipd_instance_run(uint32_t handle, uint64_t phyAddr, uint64_t len, uint32_t *completeLen)
{
	int ret;
	uint32_t idx;
	uint32_t tiles_cnt;
	uint32_t tileSize;
	uint32_t leftDataLen;
	uint64_t startAddr;
	bool bLastTile;
	GZIPD_INSTANCE_INFO_T *pInst;
	AX_GZIP_DEV_LOG_DBG("handle = %d, enter", handle);
	CHECK_HANDEL_VALID(handle);

	pInst = gzipd_instance_find_ertry(handle);
	CHECK_POINTER_NULL(pInst);

	pInst->eStatus = GZIPD_INSTANCE_RUNNING;

	tileSize = pInst->u32TileSize;
	tiles_cnt = pInst->u32TilesTotalNum;
	leftDataLen =  pInst->u32LastTilesSZ;

	AX_GZIP_DEV_LOG_DBG("tileSz = 0x%x, tiles_cnt = %d, LeftDataLen = %d", tileSize, tiles_cnt, leftDataLen);
	for(idx = 0; idx < tiles_cnt; idx++) {
		startAddr = phyAddr + idx * tileSize;
		bLastTile = false;
		if (leftDataLen == 0 && idx == tiles_cnt - 1) {
			bLastTile = true;
		}
		gzipd_tile_submit_onetile(pInst, startAddr,tileSize, bLastTile);
	}

	if ((pInst->u32CurrTileIndex == pInst->u32TilesTotalNum) && leftDataLen) {
		startAddr = phyAddr + idx * tileSize;
		pInst->eStatus = GZIPD_INSTANCE_LAST_TILE;
		gzipd_tile_submit_onetile(pInst, startAddr, leftDataLen, true);
		*completeLen = len;
		AX_GZIP_DEV_LOG_DBG("last tile.startAddr Phy = 0x%llx, Inst.eStatus = %d", startAddr, pInst->eStatus);
		ret = AX_GZIPD_STATUS_OK;
	} else {
		*completeLen = len - leftDataLen;
		ret = AX_GZIPD_PART_COMPLETE;
	}
	AX_GZIP_DEV_LOG_DBG("exit");

	return ret;
}

int gzipd_instance_lasttile_run(uint32_t handle, uint64_t startAddr, uint32_t lastTileDataLen)
{
	GZIPD_INSTANCE_INFO_T *pInst;
	AX_GZIP_DEV_LOG_DBG("handle = %d, enter", handle);
	CHECK_HANDEL_VALID(handle);

	pInst = gzipd_instance_find_ertry(handle);
	CHECK_POINTER_NULL(pInst);

	if (lastTileDataLen == 0) {
		AX_GZIP_DEV_LOG_ERR(" last tile size equal to zero");
		return AX_GZIPD_INVALID_PARAM;
	}

	pInst->eStatus = GZIPD_INSTANCE_LAST_TILE;
	gzipd_tile_submit_onetile(pInst, startAddr, lastTileDataLen, true);
	AX_GZIP_DEV_LOG_DBG("exit");
	return AX_GZIPD_STATUS_OK;
}

static int gzipd_instance_wait_finish(AX_GZIPD_RESULT_INFO_T *info)
{
	int ret = GZIPD_CONDITION_TIMEOUT;
	uint32_t handle;
	GZIPD_INSTANCE_INFO_T *pInst;
	GZIPD_INSTANCE_GROUP_T	*pInstGroup;
	struct list_head *pos;
	struct list_head *next;
	GZIPD_INSTANCE_COMPLETED_T *pCompleted;
	GZIPD_INST_COMPLETE_STS_E result = GZIPD_INST_COMPLETE_MAX;
	bool  found = false;

	CHECK_POINTER_NULL(info);
	handle = info->u32Handle;
	AX_GZIP_DEV_LOG_DBG("enter, handle = %d", handle);
	CHECK_HANDEL_VALID(handle);

	pInst = gzipd_instance_find_ertry(handle);
	CHECK_POINTER_NULL(pInst);

	pInstGroup = (GZIPD_INSTANCE_GROUP_T *)pInst->pUserData;
	AX_GZIP_DEV_LOG_DBG("pid[%d].instgroup=0x%px, completed_cond=0x%px", AX_GZIPD_GET_PID, pInstGroup, &pInst->completed_cond);
	spin_lock(&pInstGroup->completed_lock);
	while ((list_empty(&gzipd_inst_group_listhead) || ret == GZIPD_CONDITION_TIMEOUT) && !found) {
		ret = gzipd_thread_cond_waittime(&pInst->completed_cond, &pInstGroup->completed_lock, NULL, gzipd_lock_type_no_schedule, 500);
		if (ret == 0) {
			list_for_each_prev_safe(pos, next, &gzipd_inst_group_listhead) {
				pCompleted = list_entry(pos, GZIPD_INSTANCE_COMPLETED_T, entry);
				if (pCompleted) {
					if (pCompleted->handle == handle) {
						result = pCompleted->complete_result;
						found = true;
						AX_GZIP_DEV_LOG_DBG("handle = %d, complete result = %d", handle, result);
					}
				}
			}
		}

		if (!found) {
			ret = GZIPD_CONDITION_TIMEOUT;
		}
	}

	info->u32Result = result;
	list_del(&pCompleted->entry);
	kfree(pCompleted);
	atomic_dec(&pInstGroup->refcnt);
	spin_unlock(&pInstGroup->completed_lock);
	AX_GZIP_DEV_LOG_DBG("handle = %d, exit", handle);

	return AX_GZIPD_STATUS_OK;

}

int gzipd_instance_query_result(void __user *argp)
{
	AX_GZIPD_RESULT_INFO_T info;
	AX_GZIPD_RESULT_INFO_T __user *pInfo = (AX_GZIPD_RESULT_INFO_T __user *)argp;

	if (copy_from_user(&info, argp, sizeof(AX_GZIPD_RESULT_INFO_T))) {
		AX_GZIP_DEV_LOG_ERR("copy from user fail");
		return AX_GZIPD_INVALID_PARAM;
	}

	gzipd_instance_wait_finish(&info);

	if (copy_to_user(pInfo, &info, sizeof(AX_GZIPD_RESULT_INFO_T))) {
		AX_GZIP_DEV_LOG_ERR("copy to user fail");
		return AX_GZIPD_INVALID_PARAM;
	}

	return AX_GZIPD_WORK_FINISH;
}

int gzipd_instance_destroy(uint32_t handle)
{
	GZIPD_INSTANCE_INFO_T *pInst;
	CHECK_HANDEL_VALID(handle);

	AX_GZIP_DEV_LOG_DBG("enter, handle = %d", handle);
	pInst = gzipd_instance_find_ertry(handle);
	CHECK_POINTER_NULL(pInst);

	mutex_lock(&gzipd_instance_mutex);
	gzipd_instance_deinit(pInst);
	if (pInst->eStatus == GZIPD_INSTANCE_IDLE) {
		spin_lock(&pInst->listlock);
		list_del(&pInst->list);
		spin_unlock(&pInst->listlock);
		kfree(pInst);
		gzipd_instance_recovery_handle(handle);
	}
	mutex_unlock(&gzipd_instance_mutex);
	AX_GZIP_DEV_LOG_DBG("exit, handle = %d", handle);

	return AX_GZIPD_STATUS_OK;
}