////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
* @file    drv_unfd.h
* @version
* @brief
*
*/

#ifndef __DRV_UNFD_H__
#define __DRV_UNFD_H__

/*=============================================================*/
// Include files
/*=============================================================*/

/*=============================================================*/
// Extern definition
/*=============================================================*/

/*=============================================================*/
// Macro definition
/*=============================================================*/
#ifndef U32
#define U32  unsigned long
#endif
#ifndef U16
#define U16  unsigned short
#endif
#ifndef U8
#define U8   unsigned char
#endif
#ifndef S32
#define S32  signed long
#endif
#ifndef S16
#define S16  signed short
#endif
#ifndef S8
#define S8   signed char
#endif

/*=============================================================*/
// Data type definition
/*=============================================================*/
typedef struct _NAND_INFO
{
	U8	au8_Tag[16];
	U8	u8_IDByteCnt;
	U8	au8_ID[15];
	U32	u32_ChkSum;
	U16	u16_SpareByteCnt;
	U16	u16_PageByteCnt;
	U16	u16_BlkPageCnt;
	U16	u16_BlkCnt;
	U32	u32_Config;
	U16	u16_ECCType;
	U16	u16_tRC;
	U8	padding[12];
	U8	u8_Vendor[16];
	U8	u8_PartNumber[16];

	U16 u16_ECCCodeByteCnt;
	U16 u16_PageSectorCnt;
	U8  u8_WordMode;

} NAND_INFO_t;

typedef struct _PAIRED_PAGE_MAP {
    U16 u16_LSB;
    U16 u16_MSB;
} PAIRED_PAGE_MAP_t, *P_PAIRED_PAGE_MAP_t;

extern PAIRED_PAGE_MAP_t ga_tPairedPageMap[];
/*=============================================================*/
// Variable definition
/*=============================================================*/

/*=============================================================*/
// Global function definition
/*=============================================================*/
extern U32 drvNAND_Init(void);
extern U32 drvNAND_IPVerify_Main(void);


extern U32 drvNAND_GetNandInfo(NAND_INFO_t *pNandInfo);


extern U32 drvNAND_ChkRdy(U32 u32_PartNo);
extern int drv_unfd_init(void);
extern U32 drvNAND_LoadBootLoader(U8 *pu8_DestAddr, U32 u32_StartSector, U32 u32_SectorCnt, U32 u32_LogicIdx);
extern U32 drvNAND_InitProgrammer(void);
extern U32 drvNAND_GetNandID(U8 *pu8IDByteCnt, U8 *pu8ID);
extern U32 drvNAND_CheckCIS(U8 *pu8_CISData);
extern U32 drvNAND_WriteCIS(U8 *pu8_CISData);
extern U32 drvNAND_ReadCIS(U8 *pu8_CISData);
extern U32 drvNAND_WriteMiuParam(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_LoadMiuParam(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_EraseMiuParam(void);
extern U32 drvNAND_GetMiuParamCapacity(void);
extern U32 drvNAND_WriteBootLoader(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_EraseBootLoader(void);
extern U32 drvNAND_GetBootLoaderCapacity(U32 u32_LogicIdx);
extern U32 drvNAND_WriteOS(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_LoadOS(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt, void (*pfCB)(U32 XferSize));
extern U32 drvNAND_EraseOS(void);
extern U32 drvNAND_GetOSCapacity(void);
extern U32 drvNAND_WriteCUS(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_LoadCUS(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_EraseCUS(void);
extern U32 drvNAND_GetCUSCapacity(void);
extern U32 drvNAND_EraseAllPartitions(void);
extern U32 drvNAND_WriteNRAI(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadNRAI(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetNRAICapacity(void);
extern U32 drvNAND_WriteE2P0(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadE2P0(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetE2P0Capacity(void);
extern U32 drvNAND_WriteE2P1(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadE2P1(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetE2P1Capacity(void);
extern U32 drvNAND_WriteBacktrace(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadBacktrace(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetBacktraceCapacity(void);
extern U32 drvNAND_WriteFAT(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadFAT(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetFATCapacity(void);
extern U32 drvNAND_WriteNVRAM0(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadNVRAM0(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetNVRAM0Capacity(void);
extern U32 drvNAND_WriteNVRAM1(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_ReadNVRAM1(U32 u32_SrcPhyAddr, U32 u32_StartSector, U32 u32_SectorCnt);
extern U32 drvNAND_GetNVRAM1Capacity(void);
extern U32 drvNAND_FormatFTL(void);
extern U32 drvNAND_GetPhyCapacity(void);

extern U32 drvNAND_EraseAllPhyBlk(void);
extern U32 drvNAND_EraseAllGoodPhyBlk(U16 *pu16_GoodBlkCnt, U16 *pu16_BadBlkIdxList, U16 *pu16_BadBlkCnt);
extern U32 drvNAND_ErasePhysicalBlk(U16 u16_BlkIdx);
extern U32 drvNAND_ReadPhysicalPage(U32 u32_PageIdx, U8 *pu8_Data, U8 *pu8_Spare);
extern U32 drvNAND_WritePhysicalPage(U32 u32_PageIdx, U8 *pu8_Data, U8 *pu8_Spare);
extern U32 drvNAND_CheckGoodBlk(U16 u16_BlkIdx);
extern U32 drvNAND_ResetNandFlash(void);
extern U32 drvNAND_FlushCache(void);

#endif //__DRV_UNFD_H__

