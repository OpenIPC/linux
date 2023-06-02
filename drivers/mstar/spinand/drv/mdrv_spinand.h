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


#ifndef _DRV_SPINAND_H_
#define _DRV_SPINAND_H_
#include <linux/platform_device.h>
#include "inc/mdrv_spinand_common.h"
#include "../hal/reg_spinand.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define MID_GD          0xC8
#define MID_MICRON      0x2C
#define MID_ATO         0x9B
#define MID_WINBOND     0xEF
#define MID_MXIC        0xC2
#define MID_TOSHIBA     0x98

/* SPI NAND messages */
#if 1
#define spi_nand_msg(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define spi_nand_debug(fmt, ...)
#else
#define spi_nand_msg(fmt, ...)
#define spi_nand_debug(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#endif
#define spi_nand_warn(fmt, ...) printk(KERN_WARNING "%s:warning, " fmt "\n", __func__, ##__VA_ARGS__)
#define spi_nand_err(fmt, ...) printk(KERN_ERR "%s:error, " fmt "\n", __func__, ##__VA_ARGS__)

#define SPINAND_FLASH_INFO_TAG "MSTARSEMIUSFDCIS"

#define DEBUG_SPINAND(debug_level, x)     do { if (_u8SPINANDDbgLevel >= (debug_level)) (x); } while(0)

#define SPINAND_MEASURE_PERFORMANCE 0
#define USE_SPINAND_INFO_TABLE 0

#define UNFD_LOGI_PART    0x8000 // bit-or if the partition needs Wear-Leveling
#define UNFD_HIDDEN_PART  0x4000 // bit-or if this partition is hidden, normally it is set for the LOGI PARTs.

//#define UNFD_PART_MIU			1
#define UNFD_PART_IPL_CUST		1
#define UNFD_PART_BOOTLOGO		2
#define UNFD_PART_IPL			3
#define UNFD_PART_OS			4
#define UNFD_PART_CUS			5
#define UNFD_PART_UBOOT			6
#define UNFD_PART_SECINFO		7
#define UNFD_PART_OTP			8
#define UNFD_PART_RECOVERY		9
#define UNFD_PART_E2PBAK		10
#define UNFD_PART_NVRAMBAK		11
#define UNFD_PART_NPT			12
#define UNFD_PART_ENV			13
#define UNFD_PART_MISC			14


struct mstar_spinand_info
{
    struct mtd_info mtd;
    struct nand_chip nand;
    struct platform_device *pdev;
    struct mtd_partition *parts;
};

    /// Suspend type
typedef enum
{
    E_MSOS_PRIORITY,            ///< Priority-order suspension
    E_MSOS_FIFO,                ///< FIFO-order suspension
} MsOSAttribute;

typedef struct
{
    U16 u16_StartBlk;		// the start block index
    U16 u16_BlkCnt;			// project team defined
    U16 u16_PartType;		// project team defined
    U16 u16_BackupBlkCnt;	// reserved good blocks count for backup, UNFD internal use.
        // e.g. u16BackupBlkCnt  = u16BlkCnt * 0.03 + 2
} SPI_NAND_PARTITION_RECORD_t;

typedef struct
{
    U32 u32_ChkSum;
    U16	u16_SpareByteCnt;
    U16	u16_PageByteCnt;
    U16	u16_BlkPageCnt;
    U16	u16_BlkCnt;
    U16 u16_PartCnt;
    U16 u16_UnitByteCnt;
    SPI_NAND_PARTITION_RECORD_t records[62];
} SPI_NAND_PARTITION_INFO_t;

typedef struct _SPI_NAND_DRIVER
{
    SPINAND_FLASH_INFO_t tSpinandInfo;

    SPI_NAND_PARTITION_INFO_t tPartInfo;
    U8 u8_HasPNI;

    U8 *pu8_pagebuf;
    U8 *pu8_sparebuf;
    U8 *pu8_statusbuf;
    U32 u32_column;
    U8 u8_status;
    U8 u8_statusRequest;
} SPI_NAND_DRIVER_t;

typedef struct
{
    U8  au8_Tag[16];
    SPINAND_FLASH_INFO_t tSpiNandInfo;
    U8 au8_Reserved[];
} SPINAND_FLASH_INFO_TAG_t;


//	BOOL MDrv_SPINAND_Init(SPINAND_FLASH_INFO_t *tSpinandInfo);
//	U32 MDrv_SPINAND_Read(U32 u32_PageIdx, U8 *u8Data, U8 *pu8_SpareBuf);
//	U32 MDrv_SPINAND_Write(U32 u32_PageIdx, U8 *u8Data, U8 *pu8_SpareBuf);
//	U32 MDrv_SPINAND_BLOCK_ERASE(U32 u32_BlkIdx);
//	U8 MDrv_SPINAND_ReadID(U16 u16Size, U8 *u8Data);
//	void _spiNandMain(unsigned int dwSramAddress, unsigned int dwSramSize);
//	U32 MDrv_SPINAND_SetMode(SPINAND_MODE eMode);
//	BOOL MDrv_SPINAND_ForceInit(SPINAND_FLASH_INFO_t *tSpinandInfo);
//	void MDrv_SPINAND_Device(struct device *dev);
//	U32 MDrv_SPINAND_WriteProtect(BOOL bEnable);
//	BOOL MDrv_SPINAND_IsActive(void);
//	U32 MDrv_SPINAND_Read_RandomIn(U32 u32_PageIdx, U32 u32_Column, U32 u32_Byte, U8 *u8Data);
//	U32 MDrv_SPINAND_ReadStatusRegister(U8 *u8Status, U8 u8Addr);
//
//	//	unsigned char MsOS_In_Interrupt (void);
//	inline BOOL MS_SPINAND_IN_INTERRUPT (void);
//	inline U32  MS_SPINAND_CREATE_MUTEX ( MsOSAttribute eAttribute, char *pMutexName, U32 u32Flag);
//	inline BOOL MS_SPINAND_DELETE_MUTEX(S32 s32MutexId);
//	inline BOOL MS_SPINAND_OBTAIN_MUTEX (S32 s32MutexId, U32 u32WaitMs);
//	inline BOOL MS_SPINAND_RELEASE_MUTEX (S32 s32MutexId);
void *drvSPINAND_get_DrvContext_address(void);
extern SPINAND_FLASH_INFO_t _gtSpinandInfo;
extern  U8 _u8SPINANDDbgLevel;
#ifdef __cplusplus
}
#endif

#endif
