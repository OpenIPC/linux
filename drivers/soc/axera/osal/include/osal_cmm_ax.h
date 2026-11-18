/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_CMM_AX__H__
#define __OSAL_CMM_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"

int ax_cmm_userdev_init(void);
int ax_cmm_userdev_exit(void);

int AX_OSAL_MemAlloc(unsigned long long int *phyaddr, void **ppviraddr, unsigned int size, unsigned int align, char *token);
int AX_OSAL_MemAllocCached(unsigned long long int *phyaddr, void **pviraddr, unsigned int size, unsigned int align, char *token);
int AX_OSAL_MemFlushCache(unsigned long long int phyaddr, void *pviraddr, unsigned int size);
int AX_OSAL_MemInvalidateCache(unsigned long long int phyaddr, void *pviraddr, unsigned int size);
int AX_OSAL_MemFree(unsigned long long int phyaddr, void *pviraddr);
void *AX_OSAL_Mmap(unsigned long long int phyaddr, unsigned int size);
void *AX_OSAL_MmapCache(unsigned long long int phyaddr, unsigned int size);
int AX_OSAL_Munmap(void *pviraddr);
int AX_OSAL_MemGetBlockInfoByPhy(unsigned long long int phyaddr, int *pmemType, void **pviraddr, unsigned int *pblockSize);
int AX_OSAL_MemGetBlockInfoByVirt(void *pviraddr, unsigned long long int *phyaddr, int *pmemType);
#ifdef __cplusplus
}
#endif

#endif /*__OSAL_CMM_AX__H__*/
