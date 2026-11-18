/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_MGT_H_
#define _AX_GZIPD_MGT_H_

#include <types.h>

#include "ax_gzipd_log.h"
#include "ax_gzipd_api.h"
#include "ax_gzipd_adapter.h"

#define AX_GZIP_INST_HANDLE_MAX 100

typedef enum {
	GZIPD_INSTANCE_IDLE = 1,
	GZIPD_INSTANCE_RUNNING,
	GZIPD_INSTANCE_LAST_TILE,
	GZIPD_INSTANCE_COMPLETE,
	GZIPD_INSTANCE_MAX,
} GZIPD_INSTANCE_STATUS_E;

typedef enum {
	GZIPD_TILE_STATUS_IDLE = 1,
	GZIPD_TILE_STATUS_ENQUEUED,
	GZIPD_TILE_STATUS_DONE,
	GZIPD_TILE_STATUS_MAX,
} GZIPD_TILE_STATUS_E;

typedef enum {
	GZIPD_TILE_TYPE_NORMAL_TILE = 1,
	GZIPD_TILE_TYPE_LAST_TILE,
	GZIPD_TILE_TYPE_CONFIG_TILE,
} GZIPD_TILE_TYPE_E;

typedef enum {
	GZIPD_INST_COMPLETE_SUCCESS = AX_GZIPD_COMPLETE_SUCCESS,
	GZIPD_INST_COMPLETE_FAIL = AX_GZIPD_COMPLETE_FAIL,
	GZIPD_INST_COMPLETE_MAX = AX_GZIPD_COMPLETE_MAX,
} GZIPD_INST_COMPLETE_STS_E;

typedef struct {
	uint32_t 	handle_idx;
	bool 		available;
} AX_GZIPD_INST_HANDLE_RES_T;

typedef struct {
	uint32_t 			handle;
	struct list_head 	entry;
	uint32_t 			complete_result;
} GZIPD_INSTANCE_COMPLETED_T;

typedef struct GZIPD_INSTANCE_GROUP {
	// gzipd_thread_cond_t completed_cond;
	spinlock_t completed_lock;
	struct list_head completed_list;
	atomic_t refcnt;
} GZIPD_INSTANCE_GROUP_T;

typedef struct GZIPD_CONFIG_INFO {
	uint64_t			u32OutPhyAddr;
	uint32_t 			u32OutDataSize;
	uint32_t 			u32TileCount;
	uint32_t 			u32BlockNum;
	uint32_t 			u32ByPassEn;
} GZIPD_CFG_INFO_T;

typedef struct GZIPD_TILE_INFO {
	struct list_head	list;
	union {
		AX_GZIPD_BUF_INFO_T	stTileStartBuf;
		AX_GZIPD_BUF_INFO_T	stOutDataBuf;
	};
	union {
		uint32_t 		u32TileLen;
		uint32_t 		u32OutDataSize;
	};
	union {
		uint32_t 		u32TileIndex;
		uint32_t 		u32BlockNum;
	};
	uint32_t 			u32TilesNum;
	GZIPD_TILE_TYPE_E	eTileType;
} GZIPD_TILE_INFO_T;

typedef int32_t (* GZIPD_FUNC_CB) (void* p, uint32_t result, void *data);

typedef struct GZIPD_INSTANCE_INFO {
	struct list_head	list;
	uint32_t			u32Handle;
	uint32_t 			u32TileSize;
	uint32_t			u32TilesTotalNum;
	uint32_t			u32LastTilesSZ;
	uint32_t			u32CurrTileIndex;
	uint32_t 			u32EnqueuedTileCnt;
	uint64_t			u64StartTime;
	uint64_t			u64EndTime;
	GZIPD_INSTANCE_STATUS_E	eStatus;
	GZIPD_CFG_INFO_T	stCFGInfo;
	struct mutex 		tile_mutex;
	spinlock_t 			listlock;
	struct list_head	tile_list_head;
	GZIPD_FUNC_CB 		pFinishFunc;
	void 				*pUserData;
	gzipd_thread_cond_t completed_cond;
} GZIPD_INSTANCE_INFO_T;

#define CHECK_HANDEL_VALID(handle) 								\
	do {														\
		if (handle < 0) {										\
			AX_GZIP_DEV_LOG_ERR("Invalid handle [%d]", handle);	\
			return AX_GZIPD_INVALID_PARAM;						\
		}														\
	}while(0)

#define CHECK_POINTER_NULL(p)									\
	do {														\
		if (!(p)) {												\
			AX_GZIP_DEV_LOG_ERR("Pointer is NULL");				\
			return AX_GZIPD_STATUS_NO_MEM;						\
		}														\
	} while(0)

void gzipd_instance_init_handle(void);
int gzipd_thread_init(void);
int gzipd_thread_deinit(void);
void gzipd_irq_cond_broadcast(void);
void gzipd_irq_save_status(uint32_t irq_sts_val);
uint32_t gzipd_irq_get_irq_status(void);
int gzipd_instance_create_handle(uint32_t *handle, uint64_t *gzipdData, AX_GZIPD_HEADER_INFO_T *pInfo, void *private_data);
int gzipd_instance_cfg(uint32_t handle, AX_GZIPD_IO_PARAM_T *pCfgParam, uint64_t *pTileCnt, uint64_t *pLastTileSize);
int gzipd_instance_run(uint32_t handle, uint64_t phyAddr, uint64_t len, uint32_t *completeLen);
int gzipd_instance_lasttile_run(uint32_t handle, uint64_t startAddr, uint32_t lastTileDataLen);
int gzipd_instance_query_result(void __user *argp);
int gzipd_instance_destroy(uint32_t handle);
int gzipd_instance_group_init(GZIPD_INSTANCE_GROUP_T **pInstGroup);
int gzipd_instance_group_deinit(GZIPD_INSTANCE_GROUP_T *pInstGroup);


#endif