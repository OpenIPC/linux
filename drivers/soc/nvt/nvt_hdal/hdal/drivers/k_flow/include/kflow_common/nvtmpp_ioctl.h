/**

    The header file of nvtmpp ioctl operations.

    @file       nvtmpp_ioctl.h
    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __NVTMPP_IOCTL_H
#define __NVTMPP_IOCTL_H
#if defined(__LINUX)
//#include <asm/ioctl.h>
#endif
#include "kwrap/ioctl.h"

#define NVTMPP_IOC_MAGIC 'P'


typedef enum _NVTMPP_IOC_DMA_DATA_DIR {
	NVTMPP_IOC_DMA_BIDIRECTIONAL,                         ///< it means flush operation.
	NVTMPP_IOC_DMA_TO_DEVICE,                             ///< it means clean operation.
	NVTMPP_IOC_DMA_FROM_DEVICE,                           ///< it means invalidate operation.
	ENUM_DUMMY4WORD(NVTMPP_IOC_DMA_DATA_DIR)
} NVTMPP_IOC_DMA_DATA_DIR;


typedef struct {
	UINT32         max_pool_cnt;                          ///< [in]:max count of pools, (0,NVTMPP_VB_MAX_POOLS]
	NVT_VB_CPOOL_S common_pool[NVTMPP_VB_MAX_COMM_POOLS]; ///< [in]:common pools
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_CONF_S;

typedef struct {
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_INIT_S;

typedef struct {
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_EXIT_S;

typedef struct {
	CHAR           pool_name[NVTMPP_VB_MAX_POOL_NAME+1];  ///< [in]: the pool name that want to create.
	UINT32         blk_size;                              ///< [in]: each block size of this pool
	UINT32         blk_cnt;                               ///< [in]: the block count of this pool
	NVTMPP_DDR     ddr;                                   ///< [in]: create pool in which DDR.
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_CREATE_POOL_S;


typedef struct {
	NVTMPP_VB_POOL pool;                                  ///< [in]: the pool handle than want to destroy.
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_DESTROY_POOL_S;

typedef struct {
	NVTMPP_VB_POOL pool;                                  ///< [in]: the pool handle
	UINT32         blk_size;                              ///< [in]: each block size of this pool
	NVTMPP_DDR     ddr;                                   ///< [in]: create pool in which DDR.
	NVTMPP_VB_BLK  rtn;                                   ///< [out]: return value is block handle
} NVTMPP_IOC_VB_GET_BLK_S;

typedef struct {
	NVTMPP_VB_BLK  blk;                                   ///< [in]: the block handle
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_REL_BLK_S;

typedef struct {
	NVTMPP_VB_BLK  blk;                                   ///< [in]: the block handle
	NVTMPP_ER      rtn;                                   ///< [out]:return value
} NVTMPP_IOC_VB_GET_BLK_PA_S;

typedef struct {
	void                    *virt_addr;                   ///< [in]: the virtual memory address
	UINT32                   size;                        ///< [in]: the memory size
	NVTMPP_IOC_DMA_DATA_DIR  dma_dir;                     ///< [in]: the dma direction
	NVTMPP_ER                rtn;                         ///< [out]:return value
} NVTMPP_IOC_VB_CACHE_SYNC_S;

typedef struct {
	UINT32         blk_pa;                                ///< [in]:  the blk physical address
	NVTMPP_VB_POOL pool;                                  ///< [out]: the pool handle.
	UINT32         size;                                  ///< [out]: the blk size
} NVTMPP_IOC_VB_PA_TO_POOL_S;

typedef struct {
	UINT32         pa;                                    ///< [in]:  the physical address
	UINT32         va;                                    ///< [out]:  the kernel virtual address
} NVTMPP_IOC_VB_PA_TO_VA_S;

typedef struct {
	const void     *va;                                   ///< [in]  the user virtual memory
	UINT32          pa;                                   ///< [out] the physical memory
	BOOL            cached;                               ///< [out] if cacheable memory
	NVTMPP_ER       rtn;                                  ///< [out]:return value
} NVTMPP_IOC_VB_GET_USER_VA_INFO_S;


typedef struct {
	CHAR           pool_name[NVTMPP_VB_MAX_POOL_NAME+1];  ///< [in]: the pool name that want to create.
	UINT32         blk_size;                              ///< [in]: each block size of this pool
	UINT32         blk_cnt;                               ///< [in]: the block count of this pool
	NVTMPP_DDR     ddr;                                   ///< [in]: create pool in which DDR.
	UINT32         rtn;                                   ///< [out]:return value is the pa
} NVTMPP_IOC_VB_CREATE_FIXPOOL_S;


typedef struct {
	UINT32         phys_addr;                             ///< [in]: the physical addr of certain memory on fdt description
	UINT32         size;                                  ///< [in]: the physical size of certain memory on fdt description
	UINT32         rtn;                                   ///< [out]:return value is the pa
} NVTMPP_GET_SYSMEM_REGION_S;

typedef struct {
	NVTMPP_DDR     ddr;                                   ///< [in]: create pool in which DDR.
	UINT32         size;                                  ///< [out]: the max free block size
} NVTMPP_IOC_VB_GET_MAX_FREE_S;

typedef struct {
	UINT32         va;                                    ///< [in]:  the kernel virtual address
	UINT32         pa;                                    ///< [out]: the physical address
} NVTMPP_IOC_VB_VA_TO_PA_S;


typedef struct {
	NVTMPP_DDR     ddr;                                   ///< [in]:  which DDR.
	UINT32         phys_addr;                             ///< [out]: the physical addr pool start
	UINT32         size;                                  ///< [out]: the total size
	UINT32         rtn;                                   ///< [out]: return value
} NVTMPP_GET_COMM_POOL_RANGE_S;

typedef struct {
	NVTMPP_DDR     ddr;                                   ///< [in]:  which DDR.
} NVTMPP_DDR_MONITOR_ID_S;

typedef struct {
	NVTMPP_DDR     ddr;                                   ///< [in]:  which DDR.
	UINT64         cnt;                                   ///< [out]: the total ddr data count
	UINT64         byte;                                  ///< [out]: the total ddr data bytes
} NVTMPP_DDR_MONITOR_DATA_S;

typedef struct {
	void                    *virt_addr;                   ///< [in]: the virtual memory address
	UINT32                   size;                        ///< [in]: the memory size
	NVTMPP_IOC_DMA_DATA_DIR  dma_dir;                     ///< [in]: the dma direction
	UINT16                   cpu_count;                   ///< [in]: the cpu binding count
	UINT16                   cpu_id;                      ///< [in]: the cpu binding id
	NVTMPP_ER                rtn;                         ///< [out]:return value
} NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU_S;

typedef struct {
	NVTMPP_DDR     ddr;                                   ///< [in]:  which DDR.
	UINT32         size;                                  ///< [out]: the free size
} NVTMPP_IOC_VB_GET_FREE_S;

#define NVTMPP_IOC_VB_CONF_SET             _VOS_IOWR(NVTMPP_IOC_MAGIC, 0, NVTMPP_IOC_VB_CONF_S)
#define NVTMPP_IOC_VB_CONF_GET             _VOS_IOWR(NVTMPP_IOC_MAGIC, 1, NVTMPP_IOC_VB_CONF_S)
#define NVTMPP_IOC_VB_INIT                 _VOS_IOR(NVTMPP_IOC_MAGIC,  2, NVTMPP_IOC_VB_INIT_S)
#define NVTMPP_IOC_VB_EXIT                 _VOS_IOR(NVTMPP_IOC_MAGIC,  3, NVTMPP_IOC_VB_EXIT_S)
#define NVTMPP_IOC_VB_CREATE_POOL          _VOS_IOWR(NVTMPP_IOC_MAGIC, 4, NVTMPP_IOC_VB_CREATE_POOL_S)
#define NVTMPP_IOC_VB_DESTROY_POOL         _VOS_IOWR(NVTMPP_IOC_MAGIC, 5, NVTMPP_IOC_VB_DESTROY_POOL_S)
#define NVTMPP_IOC_VB_GET_BLK              _VOS_IOWR(NVTMPP_IOC_MAGIC, 6, NVTMPP_IOC_VB_GET_BLK_S)
#define NVTMPP_IOC_VB_REL_BLK              _VOS_IOWR(NVTMPP_IOC_MAGIC, 7, NVTMPP_IOC_VB_REL_BLK_S)
#define NVTMPP_IOC_VB_GET_BLK_PA           _VOS_IOWR(NVTMPP_IOC_MAGIC, 8, NVTMPP_IOC_VB_GET_BLK_PA_S)
#define NVTMPP_IOC_VB_CACHE_SYNC           _VOS_IOWR(NVTMPP_IOC_MAGIC, 9, NVTMPP_IOC_VB_CACHE_SYNC_S)
#define NVTMPP_IOC_VB_PA_TO_POOL           _VOS_IOWR(NVTMPP_IOC_MAGIC, 10, NVTMPP_IOC_VB_PA_TO_POOL_S)
#define NVTMPP_IOC_VB_PA_TO_VA             _VOS_IOWR(NVTMPP_IOC_MAGIC, 11, NVTMPP_IOC_VB_PA_TO_VA_S)
#define NVTMPP_IOC_VB_GET_USER_VA_INFO     _VOS_IOWR(NVTMPP_IOC_MAGIC, 12, NVTMPP_IOC_VB_GET_USER_VA_INFO_S)
#define NVTMPP_IOC_VB_CREATE_FIXED_POOL    _VOS_IOWR(NVTMPP_IOC_MAGIC, 13, NVTMPP_IOC_VB_CREATE_FIXPOOL_S)
#define NVTMPP_IOC_VB_RELAYOUT             _VOS_IOWR(NVTMPP_IOC_MAGIC, 14, NVTMPP_IOC_VB_INIT_S)
#define NVTMPP_IOC_VB_GET_BRIDGE_MEM       _VOS_IOWR(NVTMPP_IOC_MAGIC, 15, NVTMPP_GET_SYSMEM_REGION_S)
#define NVTMPP_IOC_VB_GET_MAX_FREE_BLK_SZ  _VOS_IOWR(NVTMPP_IOC_MAGIC, 16, NVTMPP_IOC_VB_GET_MAX_FREE_S)
#define NVTMPP_IOC_VB_VA_TO_PA             _VOS_IOWR(NVTMPP_IOC_MAGIC, 17, NVTMPP_IOC_VB_VA_TO_PA_S)
#define NVTMPP_IOC_VB_GET_COMM_POOL_RANGE  _VOS_IOWR(NVTMPP_IOC_MAGIC, 18, NVTMPP_GET_COMM_POOL_RANGE_S)
#define NVTMPP_IOC_DDR_MONITOR_START       _VOS_IOWR(NVTMPP_IOC_MAGIC, 19, NVTMPP_DDR_MONITOR_ID_S)
#define NVTMPP_IOC_DDR_MONITOR_STOP        _VOS_IOWR(NVTMPP_IOC_MAGIC, 20, NVTMPP_DDR_MONITOR_ID_S)
#define NVTMPP_IOC_DDR_MONITOR_RESET       _VOS_IOWR(NVTMPP_IOC_MAGIC, 21, NVTMPP_DDR_MONITOR_ID_S)
#define NVTMPP_IOC_DDR_MONITOR_DATA        _VOS_IOWR(NVTMPP_IOC_MAGIC, 22, NVTMPP_DDR_MONITOR_DATA_S)
#define NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU    _VOS_IOWR(NVTMPP_IOC_MAGIC, 23, NVTMPP_IOC_VB_CACHE_SYNC_BY_CPU_S)
#define NVTMPP_IOC_VB_GET_FREE_SZ          _VOS_IOWR(NVTMPP_IOC_MAGIC, 24, NVTMPP_IOC_VB_GET_FREE_S)


#if defined(__FREERTOS)
int nvtmpp_ioctl (int fd, unsigned int cmd, void *p_arg);
#endif

#endif
