//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#ifndef __DRV_eMMC_H__
#define __DRV_eMMC_H__


/*=============================================================*/
// Include files
/*=============================================================*/
#include "../config/eMMC_config.h"

/*=============================================================*/
// Extern definition
/*=============================================================*/
typedef struct _eMMC_INFO
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
	U16	u16_SeqAccessTime;
	U8	padding[12];
	U8	au8_Vendor[16];
	U8	au8_PartNumber[16];

	U16 u16_ECCCodeByteCnt;
	U16 u16_PageSectorCnt;
	U8  u8_WordMode;

} eMMC_INFO_t;

typedef struct _eMMC_CIS {

	U8	au8_eMMC_nni[512];
	U8	au8_eMMC_pni[512];

} eMMC_CIS_t;

/*=============================================================*/
// Macro definition
/*=============================================================*/

/*=============================================================*/
// Data type definition
/*=============================================================*/

/*=============================================================*/
// Variable definition
/*=============================================================*/

/*=============================================================*/
// Global function definition
/*=============================================================*/
extern U32  eMMC_Init(void);
extern U32  eMMC_Init_Device(void);
extern U32  eMMC_Init_Device_Ex(void);
extern U32  eMMC_LoadImages(U32 *pu32_Addr, U32 *pu32_SectorCnt, U32 u32_ItemCnt);
//--------------------------------------------
// CAUTION: u32_DataByteCnt has to be 512B x n
//--------------------------------------------
extern U32  eMMC_WriteData_Ex(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr);
extern U32  eMMC_ReadData_Ex(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr);
// skip driver-reserved area
extern U32  eMMC_WriteData(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr);
extern U32  eMMC_ReadData(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr);
extern U32  eMMC_GetCapacity(U32 *pu32_TotalSectorCnt); // 1 sector = 512B
extern U32  eMMC_EraseBlock(U32 u32_eMMCBlkAddr_start, U32 u32_eMMCBlkAddr_end);
//--------------------------------------------
extern U32  eMMC_GetID(U8 *pu8IDByteCnt, U8 *pu8ID);

extern U32  eMMC_EraseAll(void);

extern U32 eMMC_GetExtCSD(U8* pu8_Ext_CSD);
extern U32 eMMC_SetExtCSD(U8 u8_AccessMode, U8 u8_ByteIdx, U8 u8_Value);

/*=============================================================*/
// internal function definition
/*=============================================================*/
extern U32  eMMC_ReadBootPart(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr, U8 u8_PartNo);
extern U32  eMMC_WriteBootPart(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr, U8 u8_PartNo);
extern U32  eMMC_EraseBootPart(U32 u32_eMMCBlkAddr_start, U32 u32_eMMCBlkAddr_end, U8 u8_PartNo);
extern U32  eMMC_CheckIfReady(void);
extern void eMMC_ResetReadyFlag(void);
extern void eMMC_DumpDriverStatus(void);
extern void eMMC_DumpSpeedStatus(void);
extern U32  eMMC_FCIE_BuildDDRTimingTable(void);
extern U32  eMMC_FCIE_BuildHS200TimingTable(void);
extern void eMMC_FCIE_SetSkew4Value(U32 u32Value);
extern void eMMC_FCIE_SetDelayLatch(U32 u32Value);
extern U32  eMMC_SetEnhanceUserPartition(U32 u32_StartAddr,U32 u32_Size,U8 u8_EnAttr, U8 u8_RelW);
extern U32  eMMC_SetGPPartition(U8 u8_PartNo,U32 u32_PartSize,U8 u8_EnAttr,U8 u8_ExtAttr, U8 u8_RelW);
extern U32  eMMC_SetPartitionComplete(void);
extern U32  eMMC_ReadGPPart(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr, U8 u8_PartNo);
extern U32  eMMC_WriteGPPart(U8* pu8_DataBuf, U32 u32_DataByteCnt, U32 u32_BlkAddr, U8 u8_PartNo);
extern void eMMC_PrintGPPartition(void);

//--------------------------------------------
  #if defined(eMMC_DRV_G2P_UBOOT) && eMMC_DRV_G2P_UBOOT
extern U32 eMMC_SearchDevNodeStartSector(void);
  #endif
//--------------------------------------------

#endif //__DRV_eMMC_H__

