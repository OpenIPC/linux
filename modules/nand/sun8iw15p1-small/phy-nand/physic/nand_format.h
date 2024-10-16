/*
 * SPDX-License-Identifier: GPL-2.0
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __NAND_FORMAT_H__
#define __NAND_FORMAT_H__

#include "nand_type_spinand.h"
#include "nand_physic.h"

extern struct __LogicArchitecture_t LogicArchiPar;

//define the page buffer for cache the super page data for read or write
#define FORMAT_PAGE_BUF     (PageCachePool.PageCache0)
#define FORMAT_SPARE_BUF    (PageCachePool.SpareCache)

//==============================================================================
//  define the logical architecture export parameter
//==============================================================================

//define the count of the sectors in the logical page
#define SECTOR_CNT_OF_LOGIC_PAGE    SECTOR_CNT_OF_SUPER_PAGE

//define the full bitmap of sector in the logical page
#define FULL_BITMAP_OF_LOGIC_PAGE   FULL_BITMAP_OF_SUPER_PAGE

//define the count of the pages in a logical block
#define PAGE_CNT_OF_LOGIC_BLK       (LogicArchiPar.PageCntPerLogicBlk) //(NandDriverInfo.LogicalArchitecture->PageCntPerLogicBlk)

//define the count of the pages in a super physical block, size is same as the logical block
#define PAGE_CNT_OF_SUPER_BLK       PAGE_CNT_OF_LOGIC_BLK

//define the total count of inter-leave banks
#define INTERLEAVE_BANK_CNT         (PAGE_CNT_OF_LOGIC_BLK / NandStorageInfo.PageCntPerPhyBlk) //(PAGE_CNT_OF_LOGIC_BLK / NandDriverInfo.NandStorageInfo->PageCntPerPhyBlk)

#define LOGIC_DIE_CNT				(LogicArchiPar.LogicDieCnt)//(NandDriverInfo.LogicalArchitecture->LogicDieCnt)

#define BLK_CNT_OF_LOGIC_DIE		(LogicArchiPar.LogicBlkCntPerLogicDie)//(BLOCK_CNT_OF_ZONE*ZONE_CNT_OF_DIE)

#define LOGIC_PAGE_SIZE			(SECTOR_CNT_OF_SUPER_PAGE * 512)

/*
 ************************************************************************************************************************
 *                                   FORMAT NAND FLASH DISK MODULE INIT
 *
 *Description: Init the nand disk format module, initiate some variables and request resource.
 *
 *Arguments  : none
 *
 *Return     : init result;
 *               = 0     format module init successful;
 *               < 0     format module init failed.
 ************************************************************************************************************************
 */
__s32 FMT_Init(void);


/*
 ************************************************************************************************************************
 *                                   FORMAT NAND FLASH DISK MODULE EXIT
 *
 *Description: Exit the nand disk format module, release some resource.
 *
 *Arguments  : none
 *
 *Return     : exit result;
 *               = 0     format module exit successful;
 *               < 0     format module exit failed.
 ************************************************************************************************************************
 */
__s32 FMT_Exit(void);

void ClearNandStruct(void);

#endif  //ifndef __NAND_FORMAT_H__
