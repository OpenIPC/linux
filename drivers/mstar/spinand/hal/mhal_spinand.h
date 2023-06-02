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
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _HAL_SPINAND_H_
#define _HAL_SPINAND_H_
#include "mdrv_spinand_common.h"

//-------------------------------------------------------------------------------------------------
//  Macro definition
//-------------------------------------------------------------------------------------------------
#define READ_WORD(_reg)                     (*(volatile U16*)(_reg))
#define WRITE_WORD(_reg, _val)              { (*((volatile U16*)(_reg))) = (U16)(_val); }
#define WRITE_WORD_MASK(_reg, _val, _mask)  { (*((volatile U16*)(_reg))) = ((*((volatile U16*)(_reg))) & ~(_mask)) | ((U16)(_val) & (_mask)); }
#define BDMA_READ(addr)                     READ_WORD(_hal_fsp.u32BDMABaseAddr + (addr<<2))
#define BDMA_WRITE(addr, val)               WRITE_WORD(_hal_fsp.u32BDMABaseAddr + (addr<<2),(val))
#define QSPI_READ(addr)                     READ_WORD(_hal_fsp.u32QspiBaseAddr + (addr<<2))
#define QSPI_WRITE(addr, val)               WRITE_WORD(_hal_fsp.u32QspiBaseAddr + (addr<<2),(val))
#define CLK_READ(addr)                      READ_WORD(_hal_fsp.u32CLK0BaseAddr + (addr<<2))
#define CLK_WRITE(addr, val)                WRITE_WORD(_hal_fsp.u32CLK0BaseAddr + (addr<<2),(val))
#define CLK_WRITE_MASK(addr, val, mask)     WRITE_WORD_MASK(_hal_fsp.u32CLK0BaseAddr + ((addr)<<2), (val), (mask))
#define CHIP_READ(addr)                     READ_WORD(_hal_fsp.u32CHIPBaseAddr + (addr<<2))
#define CHIP_WRITE(addr, val)               WRITE_WORD(_hal_fsp.u32CHIPBaseAddr + (addr<<2),(val))
#define CHIP_WRITE_MASK(addr, val, mask)    WRITE_WORD_MASK(_hal_fsp.u32CHIPBaseAddr + ((addr)<<2), (val), (mask))
#define PM_WRITE_MASK(addr, val, mask)      WRITE_WORD_MASK(_hal_fsp.u32PMBaseAddr+ ((addr)<<2), (val), (mask))
#define PM_READ(addr)                       READ_WORD(_hal_fsp.u32PMBaseAddr + (addr<<2))
#define PM_WRITE(addr, val)                 WRITE_WORD(_hal_fsp.u32PMBaseAddr + (addr<<2),(val))
#define ISP_READ(addr)                      READ_WORD(_hal_fsp.u32ISPBaseAddr + (addr<<2))
//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
extern SPINAND_FLASH_INFO_t _gtSpinandInfo;
extern U8 _u8SPINANDDbgLevel;

U8 HAL_SPINAND_ReadStatusRegister(U8 *u8Status, U8 u8Addr);
U8 HAL_SPINAND_WriteStatusRegister(U8 *u8Status, U8 u8Addr);
U32  HAL_SPINAND_Read (U32 u32Addr, U32 u32DataSize, U8 *pu8Data);
U32  HAL_SPINAND_Write( U32 u32_PageIdx, U8 *u8Data, U8 *pu8_SpareBuf);
U32  HAL_SPINAND_ReadID(U32 u32DataSize, U8 *pu8Data);
U32  HAL_SPINAND_RFC(U32 u32Addr, U8 *pu8Data);
void HAL_SPINAND_Config(U32 u32PMRegBaseAddr, U32 u32NonPMRegBaseAddr);
U32  HAL_SPINAND_BLOCKERASE(U32 u32_PageIdx);
U32  HAL_SPINAND_Init(void);
U32  HAL_SPINAND_WriteProtect(BOOL bEnable);
BOOL HAL_SPINAND_PLANE_HANDLER(U32 u32Addr);
U32  HAL_SPINAND_SetMode(SPINAND_MODE eMode);
BOOL HAL_SPINAND_SetCKG(U8 u8CkgSpi);
void HAL_SPINAND_CSCONFIG(void);
BOOL HAL_SPINAND_IsActive(void);
U8 HAL_QSPI_FOR_DEBUG(void);


//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------
/// data type unsigned char, data length 1 byte
#define MS_U8       unsigned char                                            // 1 byte
/// data type unsigned short, data length 2 byte
#define MS_U16      unsigned short                                           // 2 bytes
/// data type unsigned int, data length 4 byte
#define MS_U32      unsigned long                                            // 4 bytes
/// data type unsigned int, data length 8 byte
#define MS_U64      unsigned long long                                       // 8 bytes
/// data type signed char, data length 1 byte
#define MS_S8       signed char                                              // 1 byte
/// data type signed short, data length 2 byte
#define MS_S16      signed short                                             // 2 bytes
/// data type signed int, data length 4 byte
#define MS_S32      signed long                                              // 4 bytes
/// data type signed int, data length 8 byte
#define MS_S64      signed long long                                         // 8 bytes
/// data type float, data length 4 byte
#define MS_FLOAT    float                                                    // 4 bytes
/// data type null pointer
#ifdef NULL
#undef NULL
#endif
#define NULL                        0

#define MS_BOOL     unsigned char

/// data type hardware physical address
#define MS_PHYADDR unsigned long                                    // 32bit physical address



#endif // _HAL_SPINAND_H_
