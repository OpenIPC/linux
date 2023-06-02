/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_heap.h

    @brief      nvtmpp heap memory alloc , free

    @version    V1.00.000
    @author     Novatek FW Team
    @date       2017/02/13
*/
#ifndef _NVTMPP_HEAP_H
#define _NVTMPP_HEAP_H

#include "nvtmpp_int.h"

/* Define the linked list structure.  This is used to link free blocks in order
of their size. */

typedef struct NVTMPP_HEAP_LINK {
	UINT32 init_tag;                ///<  The block init tag.
	UINT32 block_addr;              ///<  The block address.
	UINT32 block_size;              ///<  The size of the free block.
	struct NVTMPP_HEAP_LINK *next;  ///<  The next free block in the list.
} NVTMPP_HEAP_LINK_S;

typedef struct NVTMPP_HEAP_BLOCK {
	UINT32 init_tag;                ///< The block init tag.
	UINT32 block_addr;              ///< The block address.
	UINT32 block_size;              ///< The size of the free block.
	struct NVTMPP_HEAP_LINK *link;  ///< The link for this block.
	UINT32 *end_tag;                ///< The block end tag address.
	#if defined __KERNEL__
	UINT32 reserved[0x200-5];
	#else
	UINT32 reserved[11];
	#endif
} NVTMPP_HEAP_BLOCK_S;

#if (NVTMPP_BLK_4K_ALIGN == ENABLE)
#define NVTMPP_HEAP_START_ADDR_OFFSET 0x0800
#define NVTMPP_HEAP_ALIGN             0x1000
#else
#define NVTMPP_HEAP_START_ADDR_OFFSET  0
#define NVTMPP_HEAP_ALIGN             64
#endif
#define NVTMPP_HEAP_ALIGN_MASK       (NVTMPP_HEAP_ALIGN-1)

//static const unsigned short HEAP_STRUCT_SIZE  = ALIGN_CEIL(sizeof( xBlockLink),NVTMPP_HEAP_ALIGN);
#define HEAP_STRUCT_SIZE    sizeof(NVTMPP_HEAP_BLOCK_S)
#define HEAP_END_TAG_SIZE   16
#define HEAP_MIN_BLOCK_SIZE ((UINT32)(HEAP_STRUCT_SIZE * 2) + HEAP_END_TAG_SIZE)


extern INT32 nvtmpp_heap_init(NVTMPP_DDR ddr, NVTMPP_MEMINFO_S  *heap_mem, UINT32 max_block_num);

extern void  nvtmpp_heap_exit(NVTMPP_DDR ddr);

extern void *nvtmpp_heap_malloc(NVTMPP_DDR ddr, UINT32 wanted_size);

extern void *nvtmpp_heap_malloc_from_max_freeblk_end(NVTMPP_DDR ddr, UINT32 wanted_size);

extern void  nvtmpp_heap_free(NVTMPP_DDR ddr, void *pv);

extern BOOL  nvtmpp_heap_chk_addr_valid(UINT32 addr);

extern UINT32  nvtmpp_heap_get_max_free_block_size(NVTMPP_DDR ddr);

extern UINT32  nvtmpp_heap_get_free_size(NVTMPP_DDR ddr);

extern void    nvtmpp_heap_dump(NVTMPP_DDR ddr);

NVTMPP_HEAP_LINK_S *nvtmpp_heap_get_first_free_block(NVTMPP_DDR ddr);

NVTMPP_HEAP_LINK_S *nvtmpp_heap_get_first_used_block(NVTMPP_DDR ddr);

extern INT32   nvtmpp_heap_check_mem_corrupt(void);
#endif

