#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>
//#include "hwreg.h"
#include "ms_types.h"
#include "drvNAND.h"
#include "drvNAND_utl.h"

U32 drvNAND_Init(void)
{
	U32 u32_Err = UNFD_ST_SUCCESS;
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "\n");

	drvNAND_CheckAlignPack(UNFD_CACHE_LINE);

	memset(pNandDrv, '\0', sizeof(NAND_DRIVER));

	NC_PlatformInit();
	// setup memory for pPartInfo (512B)
	pNandDrv->pPartInfo = drvNAND_get_DrvContext_PartInfo();
	memset(pNandDrv->pPartInfo, '\0', NAND_PARTITAION_BYTE_CNT);

	nand_pads_switch(1);
	nand_clock_setting(pNandDrv->u32_Clk);

	u32_Err = drvNAND_ProbeReadSeq();
	if (u32_Err != UNFD_ST_SUCCESS)
	{
		u32_Err = UNFD_ST_ERR_UNKNOWN_RSEQ;
		goto INIT_END;
	}

	u32_Err = NC_ReadID();
	if (u32_Err != UNFD_ST_SUCCESS) {
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Failed to read ID with EC: 0x%08x\n", u32_Err);
		u32_Err = UNFD_ST_ERR_UNKNOWN_ID;
		goto INIT_END;
	}

	u32_Err = drvNAND_SearchCIS();

INIT_END:

	return u32_Err;
}

U32 drvNAND_GetNandID(U8 *pu8IDByteCnt, U8 *pu8ID)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "\n");

	*pu8IDByteCnt = pNandDrv->u8_IDByteCnt;
	memcpy(pu8ID, pNandDrv->au8_ID, NAND_ID_BYTE_CNT);

	return UNFD_ST_SUCCESS;
}

U32 drvNAND_CheckCIS(U8 *pu8_CISData)
{
	NAND_FLASH_INFO_t *pNandInfo = (NAND_FLASH_INFO_t*)pu8_CISData;
	PARTITION_INFO_t *pPartInfo = (PARTITION_INFO_t*)(pu8_CISData + 0x200);
	U32 u32_ChkSum;

	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "\n");

	if (drvNAND_CompareCISTag(pNandInfo->au8_Tag)) {
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Tag mismatch\n");
		return UNFD_ST_ERR_CIS_NAND_ERR;
	}

	u32_ChkSum = drvNAND_CheckSum(pu8_CISData + 0x24, 0x32 - 0x24);
	if (u32_ChkSum != pNandInfo->u32_ChkSum) {

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NAND Info chksum mismatch: 0x%08x, 0x%08x\n",
				   u32_ChkSum, pNandInfo->u32_ChkSum);

		dump_mem(pu8_CISData, 0x200);
		return UNFD_ST_ERR_CIS_NAND_ERR;
	}

	u32_ChkSum = drvNAND_CheckSum(pu8_CISData + 0x200 + 0x04, 0x200 - 0x04);
	if (u32_ChkSum != pPartInfo->u32_ChkSum) {

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Part Info chksum mismatch: 0x%08x, 0x%08x\n",
				   u32_ChkSum, pPartInfo->u32_ChkSum);

		dump_mem(pu8_CISData + 0x200, 0x200);
		return UNFD_ST_ERR_CIS_PART_ERR;
	}

	dump_nand_info(pNandInfo);
	dump_part_info(pPartInfo);

	return UNFD_ST_SUCCESS;
}


#if 0
U32 drvNAND_WriteCIS(U8 *pu8_CISData)  
{
    NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
    U8 *au8_PageBuf = (U8*)pNandDrv->PlatCtx_t.pu8_PageDataBuf;
    U8 *au8_SpareBuf = (U8*)pNandDrv->PlatCtx_t.pu8_PageSpareBuf;
  NAND_FLASH_INFO_t *pNandInfo = (NAND_FLASH_INFO_t*)pu8_CISData;
	PARTITION_INFO_t *pPartInfo = (PARTITION_INFO_t*)(pu8_CISData + 0x200);
    BLK_INFO_t *pBlkInfo = (BLK_INFO_t*)au8_SpareBuf;
    U32 u32_Err = UNFD_ST_SUCCESS;
    U16 u16_PBA;
    U32 u32_PageIdx;
    U8 u8_CISIdx;

    nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "\n");

	
    memset(pNandDrv->au8_ID, '\0', NAND_ID_BYTE_CNT);
    memcpy(pNandDrv->au8_ID, pNandInfo->au8_ID, pNandInfo->u8_IDByteCnt);
    pNandDrv->u8_IDByteCnt     	= pNandInfo->u8_IDByteCnt;
    pNandDrv->u16_SpareByteCnt 	= pNandInfo->u16_SpareByteCnt;
    pNandDrv->u16_PageByteCnt  	= pNandInfo->u16_PageByteCnt;
    pNandDrv->u16_BlkPageCnt   	= pNandInfo->u16_BlkPageCnt;
    pNandDrv->u16_BlkCnt       	= pNandInfo->u16_BlkCnt;
    pNandDrv->u16_ECCType      	= pNandInfo->u16_ECCType;
    pNandDrv->u32_Config       	= pNandInfo->u32_Config;
    pNandDrv->u16_tRC			= pNandInfo->u16_tRC;
    //memcpy(pNandDrv->u8_Vendor, pNandInfo->u8_Vendor, 16);
    //memcpy(pNandDrv->u8_PartNumber, pNandInfo->u8_PartNumber, 16);
    pNandDrv->u8_CellType 		= pNandInfo->u32_Config&0x01;
    pNandDrv->u8_PairPageMapLoc = pNandInfo->u8_PairPageMapLoc = ga_tPairedPageMap[3].u16_LSB;
	
	
//    dump_nand_info(pNandInfo);

    NC_ConfigContext();
	NC_ReInit();
	NC_Config();
	
		printf("pNandInfo->u16_tRC : %X\r\n", pNandInfo->u16_tRC);
    nand_config_clock(pNandInfo->u16_tRC); 
	
	#if defined(FCIE4_DDR) && FCIE4_DDR
	memcpy((void *) &pNandInfo->tDefaultDDR, (const void *) &pNandDrv->tDefaultDDR, sizeof(DDR_TIMING_GROUP_t));
	memcpy((void *) &pNandInfo->tMaxDDR, (const void *) &pNandDrv->tMaxDDR, sizeof(DDR_TIMING_GROUP_t));
	memcpy((void *) &pNandInfo->tMinDDR, (const void *) &pNandDrv->tMinDDR, sizeof(DDR_TIMING_GROUP_t));
	#endif
	
	u8_CISIdx = 0;

    /* Search for two good blocks within the first 10 physical blocks */
    for (u16_PBA = 0; u16_PBA < 10; u16_PBA++) {

        /* Reeset NAND driver and FCIE to the original settings */
        pNandDrv->u16_SpareByteCnt = pNandInfo->u16_SpareByteCnt;
        pNandDrv->u16_PageByteCnt  = pNandInfo->u16_PageByteCnt;
        pNandDrv->u16_ECCType      = pNandInfo->u16_ECCType;
        NC_ConfigContext();
		NC_ReInit();
        pNandDrv->u16_Reg48_Spare &= ~(1 << 12);
		NC_Config();

        /* Check first page of block */
        u32_PageIdx = u16_PBA << pNandDrv->u8_BlkPageCntBits;
        u32_Err = NC_ReadSectors(u32_PageIdx, 0, au8_PageBuf, au8_SpareBuf, 1);
        if (u32_Err != UNFD_ST_SUCCESS)
            nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_ReadSectors(0x%lX)=0x%lX\n", u32_PageIdx, u32_Err);
            
        if (au8_SpareBuf[0] != 0xFF)
        {
            nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "Skip bad blk 0x%04x\n", u16_PBA);
            continue;
        }

        u32_Err = NC_EraseBlk(u16_PBA << pNandDrv->u8_BlkPageCntBits);
        if (u32_Err != UNFD_ST_SUCCESS) {
            nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Erase blk 0x%04x failed with EC: 0x%08lx\n",
                       u16_PBA, u32_Err);

            drvNAND_MarkBadBlk(u16_PBA);
            continue;

        }

        pNandDrv->u16_PageByteCnt = 2048;
        pNandDrv->u16_SpareByteCnt = 256;
        pNandDrv->u16_ECCType = NANDINFO_ECC_TYPE;
        
		NC_ConfigContext();
        NC_ReInit();
//		NC_Init();

        pNandDrv->u16_Reg48_Spare |= (1 << 12);
		NC_Config();

        memset(au8_PageBuf, '\0', pNandDrv->u16_PageByteCnt);
        memcpy(au8_PageBuf, pNandInfo, 512);
        memset(au8_SpareBuf, 0xFF, pNandDrv->u16_SpareByteCnt);

        pBlkInfo->u8_BadBlkMark = 0xFF;
        pBlkInfo->u8_PartType = 0;

        u32_Err = NC_WriteSectors(u32_PageIdx, 0, au8_PageBuf, au8_SpareBuf, 1);
        
        if (u32_Err != UNFD_ST_SUCCESS)
        {
            nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Write Nand Info failed with EC: 0x%08lx\n", u32_Err);
            drvNAND_MarkBadBlk(u16_PBA);
            continue;
        }

        /* Reset NAND driver and FCIE to the original settings */
		pNandDrv->u16_SpareByteCnt = pNandInfo->u16_SpareByteCnt;
        pNandDrv->u16_PageByteCnt  = pNandInfo->u16_PageByteCnt;
        pNandDrv->u16_ECCType      = pNandInfo->u16_ECCType;
        NC_ConfigContext();
        NC_ReInit();
//		NC_Init();

        pNandDrv->u16_Reg48_Spare &= ~BIT_NC_HW_AUTO_RANDOM_CMD_DISABLE;
		NC_Config();
		
#if 1
		/*
		 *  Write Partition Info to the 2nd page
		 */
		memset(au8_PageBuf, '\0', pNandDrv->u16_PageByteCnt);
		memcpy(au8_PageBuf, pPartInfo, 512);
		u32_Err = NC_WriteSectors(u32_PageIdx+1, 0, au8_PageBuf, au8_SpareBuf, 1);
#endif
		
		
        /*
		**  Write Paired Page Map to the 4th page
		**/
        if(pNandDrv->u8_CellType == 1)  // MLC
        {
            memset(au8_PageBuf, '\0', pNandDrv->u16_PageByteCnt);
            memcpy(au8_PageBuf, &ga_tPairedPageMap, 2048);
			u32_Err = NC_WritePages(u32_PageIdx+pNandInfo->u8_PairPageMapLoc, au8_PageBuf, au8_SpareBuf, 1);
            if (u32_Err != UNFD_ST_SUCCESS) {
                nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Write Paired-Page Map failed with EC: 0x%08lx\n", u32_Err);
                drvNAND_MarkBadBlk(u16_PBA);
                continue;
            }
        }

        printf("CIS%d is written to blk 0x%04x\n", u8_CISIdx, u16_PBA);

		pNandDrv->u16_CISPBA[u8_CISIdx] = u16_PBA;

        if ((++u8_CISIdx) == 2)
            break;
    }

    /* Reset NAND driver and FCIE to the original settings */
    pNandDrv->u16_SpareByteCnt = pNandInfo->u16_SpareByteCnt;
    pNandDrv->u16_PageByteCnt  = pNandInfo->u16_PageByteCnt;
    pNandDrv->u16_ECCType      = pNandInfo->u16_ECCType;
    NC_ConfigContext();
    NC_ReInit();
//	NC_Init();
    pNandDrv->u16_Reg48_Spare &= ~(1 << 12);
	NC_Config();

    switch (u8_CISIdx) {
    case 0:
        u32_Err = UNFD_ST_ERR_NO_BLK_FOR_CIS0;
        break;
    case 1:
        u32_Err = UNFD_ST_ERR_NO_BLK_FOR_CIS1;
        break;
    case 2:
        u32_Err = UNFD_ST_SUCCESS;
        break;
    }

    return u32_Err;
}
#endif

U32 drvNAND_GetNandInfo(NAND_INFO_t *pNandInfo_t)
{
    NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
    const char *str = "MSTARSEMIUNFDCIS";

    memcpy(pNandInfo_t->au8_Tag, 	str, 16);
    pNandInfo_t->u8_IDByteCnt = pNandDrv->u8_IDByteCnt;
    memcpy(pNandInfo_t->au8_ID, pNandDrv->au8_ID, pNandDrv->u8_IDByteCnt);
    pNandInfo_t->u16_SpareByteCnt = pNandDrv->u16_SpareByteCnt;
    pNandInfo_t->u16_PageByteCnt = pNandDrv->u16_PageByteCnt;
    pNandInfo_t->u16_BlkPageCnt = pNandDrv->u16_BlkPageCnt;
    pNandInfo_t->u16_BlkCnt = pNandDrv->u16_BlkCnt;
    pNandInfo_t->u32_Config = pNandDrv->u32_Config;
    pNandInfo_t->u16_ECCType = pNandDrv->u16_ECCType;
    pNandInfo_t->u32_ChkSum=  drvNAND_CheckSum((U8*)&pNandInfo_t->u16_SpareByteCnt, 0x32 - 0x24);
    pNandInfo_t->u16_tRC = pNandDrv->u16_tRC;

    memcpy(pNandInfo_t->u8_Vendor, pNandDrv->u8_Vendor, 16);
    memcpy(pNandInfo_t->u8_PartNumber, pNandDrv->u8_PartNumber, 16);

	pNandInfo_t->u16_ECCCodeByteCnt = pNandDrv->u16_ECCCodeByteCnt;
	pNandInfo_t->u16_PageSectorCnt = pNandDrv->u16_PageSectorCnt;
	pNandInfo_t->u8_WordMode = pNandDrv->u8_WordMode;
    return UNFD_ST_SUCCESS;
}
