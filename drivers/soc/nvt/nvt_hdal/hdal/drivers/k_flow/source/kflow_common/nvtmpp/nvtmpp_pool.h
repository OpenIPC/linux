/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_blk.h

    @brief      nvtmpp video buffer block handling

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#ifndef _NVTMPP_POOL_H
#define _NVTMPP_POOL_H

#include "nvtmpp_int.h"
#include "nvtmpp_blk.h"


typedef struct _NVTMPP_VB_POOL_S {
	UINT32                      init_tag;                               ///< pool init tag
    UINT32                      pool_type;                              ///< pool type
	UINT32                      pool_addr;                              ///< pool address
	UINT32                      pool_size;                              ///< pool size
	BOOL                        is_common;                              ///< pool is common pool or not
	UINT32                      blk_total_size;                         ///< block total size include block header and end tag size
	UINT32                      blk_size;                               ///< block size
	UINT8                       blk_cnt;                                ///< block total count
	UINT8                       blk_free_cnt;                           ///< block free count
	UINT8                       blk_min_free_cnt;                       ///< block minimal free count
	UINT8                       dump_max_cnt;                           ///< dump nvtmpp status when get block count reaches this number.
	NVTMPP_DDR                  ddr;                                    ///< ddr ID
	NVTMPP_VB_BLK_S            *first_blk;                              ///< first block of this pool
	NVTMPP_VB_BLK_S            *first_free_blk;                         ///< first free block of this pool
	NVTMPP_VB_BLK_S            *first_used_blk;                         ///< first used block of this pool
	struct  _NVTMPP_VB_POOL_S  *next;                                   ///< next pool in the list.
	CHAR                        pool_name[NVTMPP_VB_MAX_POOL_NAME + 1]; ///< pool name
} NVTMPP_VB_POOL_S;

extern void               nvtmpp_vb_pool_insert_node_to_common(NVTMPP_VB_POOL_S **start, NVTMPP_VB_POOL_S *node);
extern NVTMPP_VB_POOL_S  *nvtmpp_vb_pool_search_match_size_from_common(NVTMPP_VB_POOL_S *start, UINT32 blk_size);
extern NVTMPP_VB_POOL_S  *nvtmpp_vb_pool_search_common_by_pool_type(NVTMPP_VB_POOL_S *p_pool, UINT32 pool_count, UINT32 pool_type, UINT32 blk_size, NVTMPP_DDR ddr);
extern void               nvtmpp_vb_pool_set_struct_range(UINT32 start_addr, UINT32 size);
extern NVTMPP_ER          nvtmpp_vb_pool_init(NVTMPP_VB_POOL_S *p_pool, CHAR *pool_name, UINT32 blk_size, UINT32 blk_cnt, NVTMPP_DDR ddr, BOOL is_common, UINT32 pool_type);
extern INT32              nvtmpp_vb_pool_exit(NVTMPP_VB_POOL_S *p_pool);
extern BOOL               nvtmpp_vb_pool_chk_valid(UINT32 pool_addr);
extern void               nvtmpp_vb_pool_set_dump_max_cnt(NVTMPP_VB_POOL_S *p_pool, UINT32 dump_max_cnt);

extern NVTMPP_VB_POOL_S  *nvtmpp_vb_get_free_pool(NVTMPP_VB_POOL_S  *vb_pools, UINT32 total_pools_cnt);
extern NVTMPP_VB_BLK      nvtmpp_vb_get_free_block_from_pool(NVTMPP_VB_POOL_S   *p_pool, UINT32 want_size);
extern void               nvtmpp_vb_release_block_back_pool(NVTMPP_VB_POOL_S   *p_pool, NVTMPP_VB_BLK_S *p_blk);
extern UINT32             nvtmpp_vb_get_pool_reserved_size(void);
#endif

