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
//	Software and any modification/derivatives thereof.
//	No right, ownership, or interest to MStar Software and any
//	modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//	supplied together with third party`s software and the use of MStar
//	Software may require additional licenses from third parties.
//	Therefore, you hereby agree it is your sole responsibility to separately
//	obtain any and all third party right and license necessary for your use of
//	such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//	MStar`s confidential information and you agree to keep MStar`s
//	confidential information in strictest confidence and not disclose to any
//	third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//	kind. Any warranties are hereby expressly disclaimed by MStar, including
//	without limitation, any warranties of merchantability, non-infringement of
//	intellectual property rights, fitness for a particular purpose, error free
//	and in conformity with any international standard.  You agree to waive any
//	claim against MStar for any loss, damage, cost or expense that you may
//	incur related to your use of MStar Software.
//	In no event shall MStar be liable for any direct, indirect, incidental or
//	consequential damages, including without limitation, lost of profit or
//	revenues, lost or damage of data, and unauthorized system use.
//	You agree that this Section 4 shall still apply without being affected
//	even if MStar Software has been modified by MStar in accordance with your
//	request or instruction for your use, except otherwise agreed by both
//	parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//	services in relation with MStar Software to you for your use of
//	MStar Software in conjunction with your or your customer`s product
//	("Services").
//	You understand and agree that, except otherwise agreed by both parties in
//	writing, Services are provided on an "AS IS" basis and the warranty
//	disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//	or otherwise:
//	(a) conferring any license or right to use MStar name, trademark, service
//		mark, symbol or any other identification;
//	(b) obligating MStar or any of its affiliates to furnish any person,
//		including without limitation, you and your customers, any assistance
//		of any kind whatsoever, or any information; or
//	(c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//	of Taiwan, R.O.C., excluding its conflict of law rules.
//	Any and all dispute arising out hereof or related hereto shall be finally
//	settled by arbitration referred to the Chinese Arbitration Association,
//	Taipei in accordance with the ROC Arbitration Law and the Arbitration
//	Rules of the Association by three (3) arbitrators appointed in accordance
//	with the said Rules.
//	The place of arbitration shall be in Taipei, Taiwan and the language shall
//	be English.
//	The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

//#include <linux/string.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <asm/io.h>

#include "ms_platform.h"
#include "ms_types.h"
#include "../../inc/common/drvNAND.h"
#include "../../inc/common/drvNAND_utl.h"

#if (defined(NC_SEL_FCIE5) && (NC_SEL_FCIE5))

#define CHECK_HAL_PARAM	  1 // normal can be 0

//========================================================
// HAL pre-processors
//========================================================
#if IF_FCIE_SHARE_PINS
  #define NC_PAD_SWITCH(enable)	//nand_pads_switch(enable);
#else
	// NULL to save CPU a JMP/RET time
  #define NC_PAD_SWITCH(enable)
#endif

#if IF_FCIE_SHARE_IP
  // re-config FCIE3 for NFIE mode
  #define NC_RECONFIG()   //{NC_ReConfig(); nand_clock_setting(pNandDrv->u32_Clk);}
#else
	// NULL to save CPU a JMP/RET time
  #define NC_RECONFIG()
#endif

#define NAND_TIMEOUT_RETRY_CNT	 5

#if defined(NEED_FLUSH_MIU_PIPE) && NEED_FLUSH_MIU_PIPE
#define FLUSH_MIU_PIPE()	nand_flush_miu_pipe() // Only used in U4 now
#else
#define FLUSH_MIU_PIPE()
#endif
//========================================================
// HAL misc. function definitions
//========================================================

void NC_DumpRegisters(void);
void NC_DumpDebugBus(void);
U32  NC_CheckEWStatus(U8 u8_OpType);
U32  NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec);
U32 NC_WaitCIFD_Event(U16 u16_WaitEvent, U32	u32_MicroSec);

U32 NC_WaitGetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt, U32 u32_MicroSec);
U32 NC_WaitSetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt);

  #define OPTYPE_ERASE		1
  #define OPTYPE_WRITE		2

// ============================================================
#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT

#define NAND_CMD_READ_ID				0x90
#define NAND_CMD_SET_FEATURE			0xEF
#define NAND_CMD_GET_FEATURE			0xEE
#define NAND_CMD_READ_PARAMETER		 0xEC

#define NAND_ADR_READ_JEDEC			 0x40
#define NAND_ADR_READ_ONFI			  0x20
#define NAND_ADR_READ_PARAMETER		 0x00
#define NAND_ADR_TIMING_MODE			0x01
#define NAND_ADR_INTERFACE_MODE		 0x80

#define NAND_DDR_REMOVE_2NDBYTE_NA	  0
#define NAND_DDR_REMOVE_2NDBYTE_TOGGLE  1
#define NAND_DDR_REMOVE_2NDBYTE_ONFI	2

static U32 NC_RandomIn_1Cmd1Addr(U8 u8_Cmd, U8 u8_Addr, U8 u8_Remove2ndByte, U8 u8_ByteCnt, U8 *pu8_DataBuf);
U32 NC_SetToggleDDRMode(void);
U32 NC_ReadToggleParamPage(void);
#endif

U32 NC_SendReadRetryCmd(U8 u8_RetryIndex, U8 u8_SetToDefault);
// ============================================================
int Disable_Err_log = 0;
static U8 gu8_ReadRetryHappens = 0;

void NC_DumpDebugBus(void)
{
	volatile U16 u16_Reg, u16_i;

	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "\n");
	for(u16_i=0; u16_i<16; u16_i++)
	{
		REG_CLR_BITS_UINT16(NC_DEBUG_DBUS1, BIT_FCIE_DEBUG_MODE);
		REG_SET_BITS_UINT16(NC_DEBUG_DBUS1, u16_i << 8);

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg debug mode: 0x%X\n", u16_i);
		REG_READ_UINT16(NC_DEBUG_DBUS0, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "NC_DEBUG_DBUS0: %X\n", u16_Reg);
		REG_READ_UINT16(NC_DEBUG_DBUS1, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "NC_DEBUG_DBUS1: %X\n", u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "\n");
	}

	REG_CLR_BITS_UINT16(NC_DEBUG_DBUS1, BIT_FCIE_DEBUG_MODE);
	REG_SET_BITS_UINT16(NC_DEBUG_DBUS1, BIT0 << 8);

	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "\n");
	for(u16_i=0; u16_i<16; u16_i++)
	{
		REG_CLR_BITS_UINT16(NC_WIDTH, BIT_NC_DEB_SEL);
		REG_SET_BITS_UINT16(NC_WIDTH, u16_i<<BIT_NC_DEB_SEL_SHIFT);

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "REG_NC_DEB_SEL: %X\n", u16_i);

		REG_READ_UINT16(NC_WIDTH, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "NC_WIDTH: %X\n", u16_Reg);

		REG_READ_UINT16(NC_DEBUG_DBUS0, u16_Reg);

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "NC_DEBUG_DBUS0: %X\n", u16_Reg);

		REG_READ_UINT16(NC_DEBUG_DBUS1, u16_Reg);

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "NC_DEBUG_DBUS1: %X\n", u16_Reg);

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "\n");
	}
	REG_CLR_BITS_UINT16(NC_WIDTH, BIT_NC_DEB_SEL);

}


void NC_DumpRegisters(void)
{
	volatile U16 u16_Reg;
	U16 u16_idx = 0;

	nand_DumpPadClk();
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Dump FCIE Register\n");

	REG_READ_UINT16(NC_RPT_CNT, u16_Reg);
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_RPT_CNT(0x0C) = %04X\n", u16_Reg);

	REG_READ_UINT16(NC_TR_RPT_CNT, u16_Reg);
	nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_TR_RPT_CNT(0x0C) = %04X\n", u16_Reg);

	nand_debug(UNFD_DEBUG_LEVEL,0,"FCIE Reg: ");
	for(u16_idx=0x30 ; u16_idx<0x80; u16_idx++)
	{
		if(0 == u16_idx%8)
			nand_debug(UNFD_DEBUG_LEVEL,0, "\n%02Xh:| ", u16_idx);

		REG_READ_UINT16(GET_REG_ADDR(FCIE_REG_BASE_ADDR, u16_idx), u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL, 0, "%04Xh ", u16_Reg);

		if(7 == u16_idx%8)
			nand_debug(UNFD_DEBUG_LEVEL,0,"| %02Xh ", u16_idx);
	}
	nand_debug(UNFD_DEBUG_LEVEL, 0,"\n");
}

//========================================================
// HAL function definitions
//========================================================
U32 NC_ResetFCIE(void)
{
	//volatile U16 u16_i=0;
	volatile U16 u16Reg, u16Cnt;

	//nand_debug(1, 1, "1: %Xh\n", REG(NC_RESERVED_FOR_SW));

	NC_PlatformResetPre();

	// soft reset
	REG_CLR_BITS_UINT16(NC_FCIE_RST, BIT_FCIE_SOFT_RST); /* active low */

	//As reset is active, Check Reset Status from 0 -> 1
	#if 1
	u16Cnt=0;
	do
	{
		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		if(0x1000 == u16Cnt++)
		{
			nand_DumpPadClk();
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ResetFCIE Err1, ErrCode: 0x%03X\n", UNFD_ST_ERR_NO_NFIE);
			return UNFD_ST_ERR_NO_NFIE;
		}

		REG_READ_UINT16(NC_FCIE_RST, u16Reg);

	}while (BIT_RST_STS_MASK  != (u16Reg  & BIT_RST_STS_MASK));
	#endif

	//nand_hw_timer_delay(HW_TIMER_DELAY_1ms);
	REG_SET_BITS_UINT16(NC_FCIE_RST, BIT_FCIE_SOFT_RST);
	//nand_hw_timer_delay(HW_TIMER_DELAY_1ms);

	//Restore reset bit, check reset status from 1 -> 0
	#if 1
	u16Cnt=0;
	do
	{
		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		if(0x1000 == u16Cnt++)
		{
			nand_DumpPadClk();
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ResetFCIE Err2, ErrCode: 0x%03X\n", UNFD_ST_ERR_NO_NFIE);
			return UNFD_ST_ERR_NO_NFIE;
		}

		REG_READ_UINT16(NC_FCIE_RST, u16Reg);

	}while (0  != (u16Reg  & BIT_RST_STS_MASK));
	#endif

	NC_PlatformResetPost();

	//nand_debug(1, 1, "2: %Xh\n", REG(NC_RESERVED_FOR_SW));

	return UNFD_ST_SUCCESS; // ok
}


U32 NC_ConfigNandFlashContext(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	if (pNandDrv->u16_ECCType < ECC_TYPE_24BIT1KB ||
		pNandDrv->u16_ECCType == ECC_TYPE_32BIT || pNandDrv->u16_ECCType == ECC_TYPE_40BIT || pNandDrv->u16_ECCType == ECC_TYPE_60BIT)
		pNandDrv->u16_SectorByteCnt = 0x200;
	else
		pNandDrv->u16_SectorByteCnt = 0x400;

	pNandDrv->u8_BlkPageCntBits = drvNAND_CountBits(pNandDrv->u16_BlkPageCnt);
	pNandDrv->u8_PageByteCntBits = drvNAND_CountBits(pNandDrv->u16_PageByteCnt);
	//pNandDrv->u8_SpareByteCntBits = drvNAND_CountBits(pNandDrv->u16_SpareByteCnt);
	pNandDrv->u8_SectorByteCntBits = drvNAND_CountBits(pNandDrv->u16_SectorByteCnt);

	pNandDrv->u16_BlkPageCntMask = (1<<pNandDrv->u8_BlkPageCntBits)-1;
	pNandDrv->u16_PageByteCntMask = (1<<pNandDrv->u8_PageByteCntBits)-1;
	pNandDrv->u16_SectorByteCntMask = (1<<pNandDrv->u8_SectorByteCntBits)-1;
	//pNandDrv->u16_SpareByteCntMask = (1<<pNandDrv->u8_SpareByteCntBits)-1;

	pNandDrv->u16_PageSectorCnt = pNandDrv->u16_PageByteCnt >> pNandDrv->u8_SectorByteCntBits;
	pNandDrv->u8_PageSectorCntBits = drvNAND_CountBits(pNandDrv->u16_PageSectorCnt);
	pNandDrv->u16_PageSectorCntMask = (1<<pNandDrv->u8_PageSectorCntBits)-1;
	pNandDrv->u16_SectorSpareByteCnt = pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits;
	pNandDrv->u16_SectorSpareByteCnt &= ~1;
	pNandDrv->u16_SpareByteCnt = pNandDrv->u16_SectorSpareByteCnt << pNandDrv->u8_PageSectorCntBits;
	//pNandDrv->u8_SectorSpareByteCntBits = drvNAND_CountBits(pNandDrv->u16_SectorSpareByteCnt);

	pNandDrv->u8_BlkSectorCntBits = pNandDrv->u8_BlkPageCntBits + pNandDrv->u8_PageSectorCntBits;
	pNandDrv->u16_BlkSectorCnt = 1<<pNandDrv->u8_BlkSectorCntBits;
	pNandDrv->u16_BlkSectorCntMask = pNandDrv->u16_BlkSectorCnt-1;

	return UNFD_ST_SUCCESS;
}


U32 NC_Init(void)
{
	U32 u32_RetVal;
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	// disable NC
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_NC_EN);
	REG_WRITE_UINT16(NC_CTRL , 0);
	// reset NC
	u32_RetVal = NC_ResetFCIE();
	if (UNFD_ST_SUCCESS != u32_RetVal)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_Init, ErrCode:%Xh \r\n", UNFD_ST_ERR_NO_NFIE);
		return u32_RetVal;
	}

	// disable interupts
	//REG_CLR_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END | BIT_NC_JOB_END);
	// clean int events
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	// enable NC
	REG_WRITE_UINT16(NC_FUN_CTL, BIT_NC_EN);

	REG_WRITE_UINT16(NC_SIGNAL, pNandDrv->u16_Reg40_Signal);
	pNandDrv->u16_Reg48_Spare = (pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits);
	if (NC_MAX_SECTOR_SPARE_BYTE_CNT < pNandDrv->u16_Reg48_Spare || (pNandDrv->u16_Reg48_Spare & 3))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Sector Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg48_Spare);
		nand_die();
	}

	#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
	pNandDrv->u16_Reg48_Spare |= BIT_SPARE_ADR_AUTO_INC;
	#endif

	pNandDrv->u16_Reg49_SpareSize = pNandDrv->u16_SpareByteCnt;
	if (NC_MAX_TOTAL_SPARE_BYTE_CNT < pNandDrv->u16_Reg49_SpareSize || (pNandDrv->u16_Reg49_SpareSize & 3))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Total Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg49_SpareSize);
		nand_die();
	}

	pNandDrv->u16_Reg50_EccCtrl = REG50_ECC_CTRL_INIT_VALUE;
	if (pNandDrv->u8_WordMode)
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1 ,"UNFD, WordMode:%X\r\n", pNandDrv->u8_WordMode);

	switch (pNandDrv->u16_PageByteCnt)
	{
		case 0x0200:
			pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_PAGE_SIZE_512Bn;
			pNandDrv->u16_Reg48_Spare |= BIT_NC_ONE_COL_ADDR;
			break;
		case 0x0800:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_2KB;  break;
		case 0x1000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_4KB;  break;
		case 0x2000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_8KB;  break;
		case 0x4000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_16KB;  break;
		case 0x8000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_32KB;  break;
		default:
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Page Size: %Xh bytes!\r\n", pNandDrv->u16_PageByteCnt);
			nand_die();
	}

	switch (pNandDrv->u16_ECCType)
	{
		case ECC_TYPE_4BIT:
			//FCIE5 not support 4bit ecc -> using 8 bit ecc instead. for NNI compatible.
		case ECC_TYPE_8BIT:
			pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_ECC_TYPE_8b512Bn;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_8BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 4;
			pNandDrv->u16_ECCCorretableBit = 8;
			break;
		case ECC_TYPE_16BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_16b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_16BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 8;
			pNandDrv->u16_ECCCorretableBit = 16;
			break;
		case ECC_TYPE_24BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_24b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_24BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 12;
			pNandDrv->u16_ECCCorretableBit = 24;
			break;
		case ECC_TYPE_32BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_32b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_32BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 16;
			pNandDrv->u16_ECCCorretableBit = 32;
			break;
		case ECC_TYPE_40BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_40b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_40BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 20;
			pNandDrv->u16_ECCCorretableBit = 40;
			break;
		case ECC_TYPE_60BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_60b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_60BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 30;
			pNandDrv->u16_ECCCorretableBit = 60;
			break;
		case ECC_TYPE_8BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_8b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_8BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 4;
			pNandDrv->u16_ECCCorretableBit = 8;
			break;
		case ECC_TYPE_16BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_16b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_16BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 8;
			pNandDrv->u16_ECCCorretableBit = 16;
			break;
		case ECC_TYPE_24BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_24b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_24BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 12;
			pNandDrv->u16_ECCCorretableBit = 24;
			break;
		case ECC_TYPE_32BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_32b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_32BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 16;
			pNandDrv->u16_ECCCorretableBit = 32;
			break;
		case ECC_TYPE_40BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_40b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_40BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 20;
			pNandDrv->u16_ECCCorretableBit = 40;
			break;
		case ECC_TYPE_60BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_60b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_60BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 30;
			pNandDrv->u16_ECCCorretableBit = 60;
			break;
		default:
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid ECC Type: %Xh \r\n", pNandDrv->u16_ECCType);
			nand_die();
	}
	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECCERR_NSTOP;

	#if defined(NC_HWCMD_DELAY) && NC_HWCMD_DELAY
	REG_READ_UINT16(NC_RAND_W_CMD, pNandDrv->u16_Reg56_Rand_W_Cmd);
	#endif

	#if defined(NC_TRR_TCS) && NC_TRR_TCS
	REG_READ_UINT16(NC_LFSR_CTRL, pNandDrv->u16_Reg59_LFSRCtrl);
	#endif

	pNandDrv->u8_PadMode =NAND_PAD_BYPASS_MODE;

	// ===================================================
	NC_Config();
	nand_debug(UNFD_DEBUG_LEVEL_LOW,1, "NC_ResetNandFlash\n");
	#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
	u32_RetVal = NC_FCIE5SetInterface(0,0,0,0);
	#else
	u32_RetVal = NC_ResetNandFlash();
	#endif
	if(UNFD_ST_SUCCESS != u32_RetVal)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"ERROR, NAND reset flash, Err Code:%08Xh \n", u32_RetVal);
		return u32_RetVal;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH,1, "NC_ReadID\n");

	u32_RetVal = NC_ReadID();
	if (u32_RetVal != UNFD_ST_SUCCESS) {
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Failed to read ID with EC: 0x%08X\n", u32_RetVal);
		u32_RetVal = UNFD_ST_ERR_UNKNOWN_ID;
		return u32_RetVal;
	}

	// ===================================================
	#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
	REG_READ_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
	REG_READ_UINT16(NC_DDR_CTRL, pNandDrv->u16_Reg58_DDRCtrl);
	// detect if Toggle-DDR
	pNandDrv->u16_Reg58_DDRCtrl &= ~BIT_DDR_MASM;
	u32_RetVal = NC_ProbeIfToggleDDR();
	if(UNFD_ST_ERR_NOT_TOGGLE_DDR == u32_RetVal)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH,1,"NOT Toggle DDR \n");

		if(UNFD_ST_ERR_NOT_ONFI_DDR == NC_ProbeIfONFIDDR())
		{
			nand_debug(UNFD_DEBUG_LEVEL_HIGH,1,"NOT ONFI DDR \n");
			nand_debug(0,1,"SDR NAND is detected\n");
			pNandDrv->u8_PadMode =NAND_PAD_BYPASS_MODE;
		}
		else
		{
			nand_debug(0,1,"YES ONFI DDR \n");

			u32_RetVal = NC_SetONFISyncMode(1);
			if(UNFD_ST_ERR_PAD_UNSUPPORT_DDR_NAND == u32_RetVal)
			{
				nand_debug(0,1, "SDR NAND is detected\n");
				NC_Config();
				return UNFD_ST_SUCCESS;
			}
			else if(UNFD_ST_SUCCESS != u32_RetVal)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"ERROR, NAND SetFeature ONFI-DDR, Err Code:%08Xh \n", u32_RetVal);
				return u32_RetVal;
			}

			nand_pads_switch(NAND_PAD_ONFI_SYNC_MODE);
			pNandDrv->u8_PadMode = NAND_PAD_ONFI_SYNC_MODE;

			u32_RetVal= NC_DetectDDRTiming();
			if(UNFD_ST_SUCCESS != u32_RetVal)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"ERROR, NAND config timing, ONFI-DDR, Err Code:%08Xh \n", u32_RetVal);
				return u32_RetVal;
			}
		}
	}
	else
	{
		nand_debug(0,1,"Yes Toggle DDR \n");

		nand_pads_switch(NAND_PAD_TOGGLE_MODE);
		pNandDrv->u8_PadMode = NAND_PAD_TOGGLE_MODE;

		u32_RetVal = NC_DetectDDRTiming();
		if(UNFD_ST_SUCCESS != u32_RetVal)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"ERROR, NAND config timing, Toggle-DDR, Err Code:%08Xh \n", u32_RetVal);
			return u32_RetVal;
		}
	}
	// ===================================================
	#endif

	NC_Config();
	return UNFD_ST_SUCCESS;
}


U32 NC_EnableLFSR(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	volatile U16 u16_RegVal;

	if(0 == IF_LFSR_ENABLED())
	{
		// tune timing: RE_LATCH
		if(0 == (pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM))
		{
			REG_READ_UINT16(NC_LATCH_DATA, u16_RegVal);
			u16_RegVal = u16_RegVal & BIT_NC_LATCH_DATA_MASK;
			if(u16_RegVal > BIT_NC_LATCH_DATA_6_0_T)
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error, RD_LATCH already > 1T, can NOT use LFSR.\n");
				return UNFD_ST_ERR_LFSR_RD_LATCH;
			}
			pNandDrv->u16_Reg57_RELatch &= ~BIT_NC_LATCH_DATA_MASK;
			pNandDrv->u16_Reg57_RELatch += u16_RegVal + BIT_NC_LATCH_DATA_1_0_T;
		}

		#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
		// tune timing: DDR_TIMING
		REG_READ_UINT16(NC_LATCH_DATA, u16_RegVal);
		u16_RegVal = u16_RegVal & BIT_RE_DDR_TIMING_MASK;
		if(u16_RegVal == BIT_RE_DDR_TIMING_MASK)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error, DDR_TIMING full, can NOT use LFSR.\n");
			return UNFD_ST_ERR_LFSR_DDRTIMING;
		}
		pNandDrv->u16_Reg57_RELatch += 1<<BIT_RE_DDR_TIMING_SHIFT;
		#endif

		// walk around timing bug
		pNandDrv->u16_Reg57_RELatch &= ~BIT_RE_SEC_TURN_CNT_MASK;
		pNandDrv->u16_Reg57_RELatch |= 0xE << BIT_RE_SEC_TURN_CNT_SHIFT;

		REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

		// set LFST
		pNandDrv->u16_Reg59_LFSRCtrl |= BIT_LFSR_ENABLE;
		REG_SET_BITS_UINT16(NC_LFSR_CTRL, BIT_LFSR_ENABLE);
	}

	return UNFD_ST_SUCCESS;
}

U32 NC_DisableLFSR(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	if(IF_LFSR_ENABLED())
	{
		if(0 == (pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM))
			pNandDrv->u16_Reg57_RELatch -= BIT_NC_LATCH_DATA_1_0_T;

		#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
		pNandDrv->u16_Reg57_RELatch -= 1<<BIT_RE_DDR_TIMING_SHIFT;
		#endif

		REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

		// clear LFST
		pNandDrv->u16_Reg59_LFSRCtrl &= ~BIT_LFSR_ENABLE;
		REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_LFSR_ENABLE);
	}
	return UNFD_ST_SUCCESS;
}

U32 NC_RegInit(void)
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();

	// config NC

	pNandDrv->u16_Reg48_Spare = (pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits);
	if (NC_MAX_SECTOR_SPARE_BYTE_CNT < pNandDrv->u16_Reg48_Spare || (pNandDrv->u16_Reg48_Spare & 3))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Sector Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg48_Spare);
		nand_die();
	}

	#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
	pNandDrv->u16_Reg48_Spare |= BIT_SPARE_ADR_AUTO_INC;
	#endif

	pNandDrv->u16_Reg49_SpareSize = pNandDrv->u16_SpareByteCnt;
	if (NC_MAX_TOTAL_SPARE_BYTE_CNT < pNandDrv->u16_Reg49_SpareSize || (pNandDrv->u16_Reg49_SpareSize & 3))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Total Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg49_SpareSize);
		nand_die();
	}

	pNandDrv->u16_Reg50_EccCtrl = REG50_ECC_CTRL_INIT_VALUE;
	if (pNandDrv->u8_WordMode)
		pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_WORD_MODE;
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1 ,"UNFD, WordMode:%X\r\n", pNandDrv->u8_WordMode);

	switch (pNandDrv->u16_PageByteCnt)
	{
		case 0x0200:
			pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_PAGE_SIZE_512Bn;
			pNandDrv->u16_Reg48_Spare |= BIT_NC_ONE_COL_ADDR;
			break;
		case 0x0800:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_2KB;  break;
		case 0x1000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_4KB;  break;
		case 0x2000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_8KB;  break;
		case 0x4000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_16KB;  break;
		case 0x8000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_32KB;  break;
		default:
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid Page Size: %Xh bytes!\r\n", pNandDrv->u16_PageByteCnt);
			nand_die();
	}

	switch (pNandDrv->u16_ECCType)
	{
		case ECC_TYPE_4BIT:
			//FCIE5 not support 4bit ecc -> using 8 bit ecc instead. for NNI compatible.
		case ECC_TYPE_8BIT:
			pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_ECC_TYPE_8b512Bn;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_8BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 4;
			pNandDrv->u16_ECCCorretableBit = 8;
			break;
		case ECC_TYPE_16BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_16b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_16BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 8;
			pNandDrv->u16_ECCCorretableBit = 16;
			break;
		case ECC_TYPE_24BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_24b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_24BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 12;
			pNandDrv->u16_ECCCorretableBit = 24;
			break;
		case ECC_TYPE_32BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_32b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_32BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 16;
			pNandDrv->u16_ECCCorretableBit = 32;
			break;
		case ECC_TYPE_40BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_40b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_40BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 20;
			pNandDrv->u16_ECCCorretableBit = 40;
			break;
		case ECC_TYPE_60BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_60b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_60BIT;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 30;
			pNandDrv->u16_ECCCorretableBit = 60;
			break;
		case ECC_TYPE_8BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_8b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_8BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 4;
			pNandDrv->u16_ECCCorretableBit = 8;
			break;
		case ECC_TYPE_16BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_16b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_16BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 8;
			pNandDrv->u16_ECCCorretableBit = 16;
			break;
		case ECC_TYPE_24BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_24b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_24BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 12;
			pNandDrv->u16_ECCCorretableBit = 24;
			break;
		case ECC_TYPE_32BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_32b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_32BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 16;
			pNandDrv->u16_ECCCorretableBit = 32;
			break;
		case ECC_TYPE_40BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_40b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_40BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 20;
			pNandDrv->u16_ECCCorretableBit = 40;
			break;
		case ECC_TYPE_60BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_60b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_60BIT1KB;
			if(!pNandDrv->u16_BitflipThreshold)
				pNandDrv->u16_BitflipThreshold = 30;
			pNandDrv->u16_ECCCorretableBit = 60;
			break;
		default:
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: invalid ECC Type: %Xh \r\n", pNandDrv->u16_ECCType);
			nand_die();
	}
	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECCERR_NSTOP;
	return UNFD_ST_SUCCESS;
}

void NC_Config(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	REG_WRITE_UINT16(NC_SIGNAL, pNandDrv->u16_Reg40_Signal);
	/*sector spare size*/
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);
	/* page spare size*/
	REG_WRITE_UINT16(NC_SPARE_SIZE, (U16)pNandDrv->u16_Reg49_SpareSize);
	/* page size and ECC type*/
	REG_WRITE_UINT16(NC_ECC_CTRL, pNandDrv->u16_Reg50_EccCtrl);

	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

	#if defined(NC_TWHR_TCLHZ) && NC_TWHR_TCLHZ
	/*set timing for tCLHZ*/
	REG_WRITE_UINT16(NC_NAND_TIMING, pNandDrv->u16_Reg5A_tWHR_tCLHZ);
	#endif

	#if defined(NC_TCWAW_TADL) && NC_TCWAW_TADL
	/*set timing for tADL and tCWAW*/
	REG_WRITE_UINT16(NC_NAND_TIMING1,pNandDrv->u16_Reg5D_tCWAW_tADL);
	#endif

	#if (defined(FCIE_LFSR) && FCIE_LFSR) || (defined(NC_TRR_TCS) && NC_TRR_TCS)
	REG_WRITE_UINT16(NC_LFSR_CTRL, pNandDrv->u16_Reg59_LFSRCtrl);
	#endif

	#if defined(NC_HWCMD_DELAY) && NC_HWCMD_DELAY
	REG_SET_BITS_UINT16(NC_RAND_W_CMD, pNandDrv->u16_Reg56_Rand_W_Cmd & 0xFF00);
	#endif
}

void NC_ReConfig(void) // re-config FCIE3 for NFIE mode
{
	#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	// enable interrupts if system allows
	//nand_enable_intr_mode();
	#else
	// disable interupts
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN,  BIT_NC_JOB_END);
	#endif
	// clean int events
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_FUN_CTL, BIT_NC_EN);

	NC_ResetFCIE();
	NC_Config();
}

// can not cross block
U32 NC_WritePages(	U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt )
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr=0;
	U32 u32_Ret;

	#if CHECK_HAL_PARAM
	// can not cross block
	if ((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt > pNandDrv->u16_BlkPageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_WritePages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_W_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_W_INVALID_PARAM;
	}
	#endif
	//nand_debug(0,0,"w b:%Xh p:%Xh cnt:%Xh\n", u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
	//	u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask, u32_PageCnt);
	//nand_debug(0,0,"T: %X %X\n", FCIE_REG41_VAL, pNandDrv->u16_Reg57_RELatch);

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt, WRITE_TO_NAND);

	if(pu8_SpareBuf ==NULL)
	{
		memset(pNandDrv->PlatCtx_t.pu8_PageSpareBuf, 0xff, pNandDrv->u16_SpareByteCnt);
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pNandDrv->PlatCtx_t.pu8_PageSpareBuf, pNandDrv->u16_SpareByteCnt, WRITE_TO_NAND);
	}
	else
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, WRITE_TO_NAND);

	if ((REG(NC_FUN_CTL) & BIT_BIST_MODE) == BIT_BIST_MODE)		//for bist mode test
	{
		u32_SpareDMAAddr = 0;
		u32_DataDMAAddr = 0;
	}

	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8) | ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_CHKSTATUS);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);
	//nand_debug(1, 1, "W Rpt Cnt: %Xh \r\n", u16_PageCnt-1);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	//while(1)  nand_reset_WatchDog();
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME*u32_PageCnt) == WAIT_WRITE_TIME*u32_PageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_WritePages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	#if defined(FCIE_WRITE_PROTECT_SUPPORT) && FCIE_WRITE_PROTECT_SUPPORT
	if((REG(NC_MIE_EVENT) & BIT_NC_SECURE_ALERT) == BIT_NC_SECURE_ALERT)
	{
		U32 u32_SecureRow;
		nand_debug(0, 1, "Hit Secure range 0x%X\n", u32_PhyRowIdx);
		u32_SecureRow = (REG(NC_SECURE_ERR_ADDR_H) << 16)|(REG(NC_SECURE_ERR_ADDR_L) & 0xFFFF);
		if(u32_SecureRow != u32_PhyRowIdx)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: Alert Address 0x%X!= 0x%X\n", u32_SecureRow, u32_PhyRowIdx);
		}
		NC_ReConfig();
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_SECURE_ALERT);
		u32_Ret = UNFD_ST_ERR_W_PROTECTED;
	}
	else
	#endif

	u32_Ret = NC_CheckEWStatus(OPTYPE_WRITE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"write blk:%Xh p:%Xh fail:%Xh \n",
			u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
			u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask, u32_Ret);

	NC_CLR_DDR_MODE();
	return u32_Ret;
}


void NC_SlowdownClk(void)
{

	#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
	{
		nand_clock_setting(pNandDrv->u32_minClk);
		NC_FCIE5SetInterface(1, pNandDrv->tMinDDR.u8_DqsMode, pNandDrv->tMinDDR.u8_DelayCell, pNandDrv->tMinDDR.u8_DdrTiming);
	}
	else
	#endif
	{	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
		REG_WRITE_UINT16(NC_WIDTH, 0);
		REG_WRITE_UINT16(NC_LATCH_DATA, BIT_NC_LATCH_DATA_2_0_T|BIT_NC_LATCH_STS_2_0_T);
	}
}

void NC_RestoreClk(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
	if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
	{
		nand_clock_setting(pNandDrv->u32_Clk);
		NC_FCIE5SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);
	}
	else
	#endif
	{
		nand_clock_setting(pNandDrv->u32_Clk);
		REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
		REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
	}
}

// do the ReadPages operation, retry for timeout error.
static U32 NC_ReadPages_Ex(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr = 0;
	U16 u16_SpareByteCnt;
	U8  u8_RetryCnt_Events=0;

	nand_redo_read:
	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	// select seeds, while NC_EnableLFSR/NC_DisableLFSR is called outside.
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	//REG_WRITE_UINT16(NC_SER_DIN_BYTECNT_LW, pNandDrv->u16_PageByteCnt & 0xFFFF);
	//REG_WRITE_UINT16(NC_SER_DIN_BYTECNT_HW, pNandDrv->u16_PageByteCnt >> 16);

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt, READ_FROM_NAND );

	#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
	u16_SpareByteCnt = pNandDrv->u16_SpareByteCnt * u32_PageCnt;
	#else
	u16_SpareByteCnt = pNandDrv->u16_SpareByteCnt;
	#endif
	u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, u16_SpareByteCnt, READ_FROM_NAND);

	if ((REG(NC_FUN_CTL) & BIT_BIST_MODE) == BIT_BIST_MODE)	// for bist mode test
	{
		u32_SpareDMAAddr = 0;
		u32_DataDMAAddr = 0;
	}

	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));

	if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR)
	{ // if a page 512B
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_REPEAT);
	}
	else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_SER_DIN);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);

	// check & set - patch for MIU busy case:
	// slow down FCIE clock to wait for MIU service.
	if(0!=u8_RetryCnt_Events)
		NC_SlowdownClk();

	REG_WRITE_UINT16(NC_CTRL,  BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME * u32_PageCnt) ==
	  (WAIT_READ_TIME * u32_PageCnt ))
	{
		#if (0==IF_IP_VERIFY)
		NC_ReConfig();
		NC_ResetNandFlash();
		if(++u8_RetryCnt_Events < NAND_TIMEOUT_RETRY_CNT)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Warn: NC_ReadPages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
			goto nand_redo_read;
		}
		else
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: reset & retry over %d times, stop!\n", NAND_TIMEOUT_RETRY_CNT);

			// restore the normal setting before return
			NC_RestoreClk();
			NC_CLR_DDR_MODE();
			nand_die(); // fatal error
			return UNFD_ST_ERR_R_TIMEOUT;
		}
		#else
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadPages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		//nand_stop();
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_R_TIMEOUT;
		#endif
	}

	// restore - patch for MIU busy case
	// for retry ok, restore the normal setting
	if(0!=u8_RetryCnt_Events)
	{
		NC_RestoreClk();
		//nand_debug(UNFD_DEBUG_LEVEL,1,"restore clk \n");
	}

	return UNFD_ST_SUCCESS;
}


U32 NC_ReadPages(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt)
{
	U16 u16_Tmp=0;
	U32 u32_Err=UNFD_ST_SUCCESS;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U8  u8_RetryCnt_ECCFail=0;
	U8  u8_MaxReadRetryCount = 1; // 1 time for slow clk
	U32 u32_UECCRow, u32_UECCRowTmp, u32_UECCPageCnt;
	U8  *pu8_UECCDataBuf, *pu8_UECCSpareBuf;

	if(pNandDrv->u8_RequireReadRetry)
		u8_MaxReadRetryCount = pNandDrv->ReadRetry_t.u8_MaxRetryTime+1;

	#if CHECK_HAL_PARAM
	// can not cross block
	if ((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt >
	  pNandDrv->u16_BlkPageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ReadPages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_R_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	#endif

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();

	//nand_debug(UNFD_DEBUG_LEVEL, 1, "Blk: %Xh Page: %Xh Cnt: %Xh \r\n",
	//	u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask, u32_PageCnt);
	if(pu8_SpareBuf == NULL)
		pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;

	NC_ReadPages_Ex(u32_PhyRowIdx, pu8_DataBuf, pu8_SpareBuf, u32_PageCnt);

	// =====================================
	// Check ECC
	if(0 == (REG(NC_ECC_STAT0) & BIT_NC_ECC_FAIL))
		goto EXIT;
	else
	{
		nand_debug(UNFD_DEBUG_LEVEL,1,"\n====================================\n");
		u32_UECCRowTmp = 0xFFFFFFFF; // to toggle LFSR
	}

	// debug
	if(pNandDrv->u8_RequireRandomizer==0 && IF_LFSR_ENABLED())
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"NAND Err: no need LFSR but enabled !!! \n\r");
		NC_DumpRegisters();
		dump_nand_driver(pNandDrv);
		nand_die();
	}
	u32_UECCRow = u32_PhyRowIdx;
	u32_UECCPageCnt = u32_PageCnt;
	pu8_UECCDataBuf = pu8_DataBuf;
	pu8_UECCSpareBuf = pu8_SpareBuf;

	while(REG(NC_ECC_STAT0) & BIT_NC_ECC_FAIL)
	{
		#if (0 == IF_IP_VERIFY)
		//----------------------------------
		// 0. get the error address
		u16_Tmp = REG(NC_1ST_UECC_PAGE)&BIT_1ST_UECC_PAGE_MASK;
		u32_UECCRow += u16_Tmp;

		nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND Warn: UECC @ Blk: %Xh, Page: %Xh, PageCnt: %Xh  \n",
			u32_UECCRow>>pNandDrv->u8_BlkPageCntBits, u32_UECCRow&pNandDrv->u16_BlkPageCntMask, u32_UECCPageCnt);
		nand_debug(UNFD_DEBUG_LEVEL, 1, "RequireLFSR: %Xh, EnableLFSR: %Xh  \n\n",
			pNandDrv->u8_RequireRandomizer, IF_LFSR_ENABLED()?1:0);

		u32_UECCPageCnt -= u16_Tmp;
		pu8_UECCDataBuf += u16_Tmp << pNandDrv->u8_PageByteCntBits;
		#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
		pu8_UECCSpareBuf += u16_Tmp * pNandDrv->u16_SpareByteCnt;
		#endif

		//----------------------------------
		// 1. check empty pages
		NC_ReadPages_Ex(u32_UECCRow, pu8_UECCDataBuf, pu8_UECCSpareBuf, 1);

		// FCIE5 check ZERO_COUNT register which records max zero count of a sector during this DMA
		if(pNandDrv->u8_RequireRandomizer && IF_LFSR_ENABLED())
			REG_READ_UINT16(NC_NAND_ZERO_BITCNT,u16_Tmp);
		else
			REG_READ_UINT16(NC_FCIE_ZERO_BITCNT,u16_Tmp);

		if(u16_Tmp <= pNandDrv->u16_ECCCorretableBit)
		{
			// when meet 1 empty page, all rest of pages in the same block are empty.
			memset(pu8_UECCDataBuf, 0xFF, pNandDrv->u16_PageByteCnt*u32_UECCPageCnt);
			#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
			memset(pu8_UECCSpareBuf, 0xFF, pNandDrv->u16_SpareByteCnt*u32_UECCPageCnt);
			#else
			memset(pu8_UECCSpareBuf, 0xFF, pNandDrv->u16_SpareByteCnt);
			#endif

			// No need to reset read retry status
			// Clean False Alarm Status
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: Ok, Bit Flip-Flop \n");
			goto EXIT;
		}

		//----------------------------------
		// 2. try slow clk
		if(0 == u8_RetryCnt_ECCFail)
		{
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: SlowClk, Retry times: %Xh, Max: %Xh \n",
				u8_RetryCnt_ECCFail+1, u8_MaxReadRetryCount);

			NC_SlowdownClk();
			NC_ReadPages_Ex(u32_UECCRow, pu8_UECCDataBuf, pu8_UECCSpareBuf, u32_UECCPageCnt);
			u8_RetryCnt_ECCFail++;
		}
		//----------------------------------
		// 3. read-retry
		else if(pNandDrv->u8_RequireReadRetry && u8_RetryCnt_ECCFail < u8_MaxReadRetryCount)
		{
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: RR, Retry times: %Xh, Max: %Xh \n",
				u8_RetryCnt_ECCFail+1, u8_MaxReadRetryCount);

			NC_SendReadRetryCmd(u8_RetryCnt_ECCFail-1, 0);

			NC_ReadPages_Ex(u32_UECCRow, pu8_UECCDataBuf, pu8_UECCSpareBuf, u32_UECCPageCnt);
			u8_RetryCnt_ECCFail++;
		}
		//----------------------------------
		// 4. toggle en/disable LFSR
		else
		{   if(pNandDrv->u8_RequireRandomizer && u32_UECCRowTmp!=u32_UECCRow)
			{
				u32_UECCRowTmp = u32_UECCRow;

				u8_RetryCnt_ECCFail = 1; // no need to slow clk again
				nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: switch LFSR: ");

				if(pNandDrv->u8_RequireReadRetry)
					NC_SendReadRetryCmd(0, 1);

				if(IF_LFSR_ENABLED())
				{
					NC_DisableLFSR();
					nand_debug(UNFD_DEBUG_LEVEL, 1, "OFF \n");
				}
				else
				{   NC_EnableLFSR();
					nand_debug(UNFD_DEBUG_LEVEL, 1, "ON \n");
				}

				NC_ReadPages_Ex(u32_UECCRow, pu8_UECCDataBuf, pu8_UECCSpareBuf, u32_UECCPageCnt);
			}
			else
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NAND Error: NC_ReadPages ECC Fail \n");
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Blk: %Xh  Page: %X \n\n",
					u32_UECCRow>>pNandDrv->u8_BlkPageCntBits, u32_UECCRow&pNandDrv->u16_BlkPageCntMask);

				NC_DumpRegisters();
				dump_nand_driver(pNandDrv);

				if(pNandDrv->u8_RequireReadRetry)
					NC_SendReadRetryCmd(0, 1);

				NC_CLR_DDR_MODE();
				u32_Err = UNFD_ST_ERR_ECC_FAIL;
				goto EXIT;
			}
		}
		#else // IF_IP_VERIFY==1
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NAND Error: NC_ReadPages ECC Fail \r\n");
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Blk: %Xh  Page: %X \r\n",
			u32_UECCRow>>pNandDrv->u8_BlkPageCntBits, u32_UECCRow&pNandDrv->u16_BlkPageCntMask);
		if(pNandDrv->u8_RequireReadRetry)
			NC_SendReadRetryCmd(0, 1);
		NC_CLR_DDR_MODE();
		if (0==pNandDrv->u8_IfECCTesting)
			u32_Err = UNFD_ST_ERR_ECC_FAIL;

		goto EXIT;
		#endif // IF_IP_VERIFY
	}

	// =====================================
	// ending jobs
	EXIT:
	if(u8_RetryCnt_ECCFail)
	{
		if(UNFD_ST_SUCCESS == u32_Err)
			nand_debug(UNFD_DEBUG_LEVEL,1,"NAND: retry ok. \n");

		if(pNandDrv->u8_RequireReadRetry && pNandDrv->au8_ID[0]!= 0xAD)
		{
			NC_SendReadRetryCmd(0, 1);
			gu8_ReadRetryHappens = 1;
		}

		NC_RestoreClk();
	}

	NC_CLR_DDR_MODE(); // for ONFI, stop clk to save power.

	#if defined(NAND_DRV_TV_MBOOT)&&NAND_DRV_TV_MBOOT
	#if defined(CONFIG_MIPS32)
	Chip_Read_Memory();
	#endif
	#endif

	return u32_Err;
}


// ====================================
// since UECC would not stop, so don't need internal-retry
// ====================================
U32 NC_PageCopy(U32 u32_SrcPhyRowIdx, U32 u32_DstPhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt )
{
	NAND_DRIVER *pNandDrv = drvNAND_get_DrvContext_address();
	U16 u16_Tmp = 0;
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr;
	U32 u32_Ret;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	// select seeds, while NC_EnableLFSR/NC_DisableLFSR is called outside.
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_SrcPhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	//REG_WRITE_UINT16(NC_SER_DIN_BYTECNT_LW, pNandDrv->u16_PageByteCnt & 0xFFFF);
	//REG_WRITE_UINT16(NC_SER_DIN_BYTECNT_HW, pNandDrv->u16_PageByteCnt >> 16);
	#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
	REG_CLR_BITS_UINT16(NC_SPARE, BIT_SPARE_ADR_AUTO_INC);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt, BIDIRECTIONAL );
	u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, BIDIRECTIONAL );

	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR1, u32_SpareDMAAddr >> 16);

	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR1, u32_SpareDMAAddr >> 16);

	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_SrcPhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_SrcPhyRowIdx >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_DstPhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_DstPhyRowIdx >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_PAGECOPY_US << 8) | ACT_WAIT_IDLE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | CMD_0x00);

	if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR) // if a page 512B
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | ACT_WAIT_IDLE);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);

	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAIT_MMA << 8) | ACT_SER_DIN);

	u16_Tmp = (pNandDrv->u8_OpCode_RW_AdrCycle &~ OP_ADR_SET_0) | OP_ADR_SET_1;
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_PAGECOPY_DS<< 8) | ACT_WAIT_IDLE);

	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x80 << 8) | ACT_WAIT_IDLE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DOUT << 8) | u16_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x10);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS << 8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_REPEAT<< 8) | ACT_WAIT_MMA);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_IDLE_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 3);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if (NC_WaitComplete(BIT_NC_JOB_END,
			  (WAIT_WRITE_TIME + WAIT_READ_TIME) * u32_PageCnt) ==
	  ((WAIT_WRITE_TIME + WAIT_READ_TIME) * u32_PageCnt))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_PageCopy Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_ResetNandFlash();
		NC_CLR_DDR_MODE(); // to turn off ONFI clk
		#else
		//nand_stop();
		#endif

		#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
		REG_SET_BITS_UINT16(NC_SPARE, BIT_SPARE_ADR_AUTO_INC);
		#endif

		return UNFD_ST_ERR_R_TIMEOUT;
	}

	/* Check ECC */
	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if (u16_Tmp & BIT_NC_ECC_FAIL)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NAND Err: ECC Fail, Reg51: 0x%04X ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg52: 0x%04X ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg53: 0x%04X\n", u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Blk: %Xh  Page: %Xh\r\n",
			u32_SrcPhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_SrcPhyRowIdx&pNandDrv->u16_BlkPageCntMask);

		NC_CLR_DDR_MODE(); // to turn on ONFI clk

		#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
		REG_SET_BITS_UINT16(NC_SPARE, BIT_SPARE_ADR_AUTO_INC);
		#endif

		#if IF_IP_VERIFY
		if (pNandDrv->u8_IfECCTesting)
			return UNFD_ST_SUCCESS;
		#endif
		return UNFD_ST_ERR_ECC_FAIL;
	}

	NC_CLR_DDR_MODE(); // to turn off ONFI clk

	#if defined(SPARE_DMA_ADDR_AUTO_INC) && SPARE_DMA_ADDR_AUTO_INC
	REG_SET_BITS_UINT16(NC_SPARE, BIT_SPARE_ADR_AUTO_INC);
	#endif

	u32_Ret=NC_CheckEWStatus(OPTYPE_WRITE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,
			"write blk:%Xh p:%Xh fail:%Xh \n",
			u32_DstPhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
			u32_DstPhyRowIdx&pNandDrv->u16_BlkPageCntMask, u32_Ret);

	return u32_Ret;
}


/*--------------------------------------------------
  get ECC corrected bits count

  return: 0xFFFFFFFF -> ECC uncorrectable error,
  other: max ECC corrected bits (within the readed sectors).
  --------------------------------------------------*/
U32  NC_GetECCBits(void)
{
	U16 u16_Tmp;

	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if (u16_Tmp & BIT_NC_ECC_FAIL)
		return(U32)(0-1);

	return(u16_Tmp & BIT_NC_ECC_MAX_BITS_MASK) >> 1;
}

U32  NC_ReadSectors_Retry(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_SectorCnt)
{
	U16 u16_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr;
	U8  u8_RetryCnt_Events=0, u8_RetryCnt_ECCFail=0, u8_MaxReadRetryCount = 1; // 1 time for slow clk

	#if CHECK_HAL_PARAM
	if(u8_SectorInPage+u32_SectorCnt > pNandDrv->u16_PageSectorCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_R_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	#endif

	if(pNandDrv->u8_RequireReadRetry)
		u8_MaxReadRetryCount = pNandDrv->ReadRetry_t.u8_MaxRetryTime+1;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_SectorByteCnt*u32_SectorCnt, READ_FROM_NAND );

	if(pu8_SpareBuf == NULL)
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pNandDrv->PlatCtx_t.pu8_PageSpareBuf, pNandDrv->u16_SpareByteCnt, READ_FROM_NAND );
	else
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, READ_FROM_NAND );

	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	// if not 512B/page, set Partial Mode
	REG_READ_UINT16(NC_ECC_CTRL, u16_Tmp);
	if (u16_Tmp & BIT_NC_PAGE_SIZE_MASK)
	{
		REG_WRITE_UINT16(NC_PART_MODE, BIT_PARTIAL_MODE_EN);
		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_CNT_MASK);
		REG_SET_BITS_UINT16(NC_PART_MODE, (u32_SectorCnt-1)<<BIT_START_SECTOR_CNT_SHIFT);
		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_IDX_MASK);
		REG_SET_BITS_UINT16(NC_PART_MODE, u8_SectorInPage<<BIT_START_SECTOR_IDX_SHIFT);
	}
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	LABEL_READ_SECTOR:
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(0 == pNandDrv->u8_WordMode)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << pNandDrv->u8_SectorByteCntBits);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << (pNandDrv->u8_SectorByteCntBits-1));
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));
	if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR) { // if a page 512B
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);
	} else {
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DIN);
	}

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == (WAIT_READ_TIME))
	{
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_ResetNandFlash();
		if(++u8_RetryCnt_Events < NAND_TIMEOUT_RETRY_CNT)
		{
			NC_SlowdownClk();
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Warn: Timeout, ErrCode:%Xh \n", UNFD_ST_ERR_R_TIMEOUT);
			goto LABEL_READ_SECTOR;
		}
		else
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: reset & retry over %d times, stop!\n", NAND_TIMEOUT_RETRY_CNT);
			// restore the normal setting before return
			NC_RestoreClk();
			NC_CLR_DDR_MODE();
			nand_die(); // fatal error
		}
		#else
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		//nand_stop();
		NC_CLR_DDR_MODE();
		#endif

		REG_WRITE_UINT16(NC_PART_MODE, 0);
		return UNFD_ST_ERR_R_TIMEOUT;
	}

	// =====================================
	// Check ECC
	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if (u16_Tmp & BIT_NC_ECC_FAIL)
	{
		nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND Warn: UECC @ Blk: %Xh, Page: %Xh \n",
			u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask);
		nand_debug(UNFD_DEBUG_LEVEL, 1, "RequireRandomizer: %Xh, LFSR en: %Xh  \n\n",
			pNandDrv->u8_RequireRandomizer, IF_LFSR_ENABLED()?1:0);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Reg51:%04Xh ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg52:%04Xh ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg53:%04Xh \n", u16_Tmp);

		#if !(defined(IF_IP_VERIFY) && IF_IP_VERIFY)
		//----------------------------------
		// 1. check empty pages
		if(pNandDrv->u8_RequireRandomizer && IF_LFSR_ENABLED())
			REG_READ_UINT16(NC_NAND_ZERO_BITCNT,u16_Tmp);
		else
			REG_READ_UINT16(NC_FCIE_ZERO_BITCNT,u16_Tmp);

		if(u16_Tmp <= pNandDrv->u16_ECCCorretableBit)
		{
			// when meet 1 empty page, all rest of pages in the same block are empty.
			memset(pu8_DataBuf, 0xFF, pNandDrv->u16_SectorByteCnt*u32_SectorCnt);
			memset(pu8_SpareBuf, 0xFF, pNandDrv->u16_SectorSpareByteCnt*u32_SectorCnt);
			// No need to reset read retry status
			// Clean False Alarm Status
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: Ok, Bit Flip-Flop \n");
			goto LABEL_EXIT;
		}
		//----------------------------------
		// 2. try slow clk
		if(0 == u8_RetryCnt_ECCFail)
		{
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: SlowClk, Retry times: %Xh, Max: %Xh \n",
				u8_RetryCnt_ECCFail+1, u8_MaxReadRetryCount);

			NC_SlowdownClk();
			u8_RetryCnt_ECCFail++;
			goto LABEL_READ_SECTOR;
		}
		//----------------------------------
		// 3. read-retry
		else if(pNandDrv->u8_RequireReadRetry && u8_RetryCnt_ECCFail < u8_MaxReadRetryCount)
		{
			nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: RR, Retry times: %Xh, Max: %Xh \n",
				u8_RetryCnt_ECCFail+1, u8_MaxReadRetryCount);

			NC_SendReadRetryCmd(u8_RetryCnt_ECCFail-1, 0);
			u8_RetryCnt_ECCFail++;
			goto LABEL_READ_SECTOR;
		}
		//----------------------------------
		// 4. toggle en/disable LFSR
		else
		{   static U16 u16_ToggleLFSR = 0;

			if(pNandDrv->u8_RequireRandomizer && 0==u16_ToggleLFSR)
			{
				u16_ToggleLFSR = 1; // toggle once

				u8_RetryCnt_ECCFail = 1; // no need to slow clk again
				nand_debug(UNFD_DEBUG_LEVEL, 1, "NAND: switch LFSR: ");

				if(pNandDrv->u8_RequireReadRetry)
					NC_SendReadRetryCmd(0, 1);

				if(IF_LFSR_ENABLED())
				{
					NC_DisableLFSR();
					nand_debug(UNFD_DEBUG_LEVEL, 1, "OFF \n");
				}
				else
				{   NC_EnableLFSR();
					nand_debug(UNFD_DEBUG_LEVEL, 1, "ON \n");
				}
				goto LABEL_READ_SECTOR;
			}
			else
			{
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NAND Error: ECC Fail \n");
				nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Blk: %Xh  Page: %X \n\n",
					u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask);

				NC_DumpRegisters();
				dump_nand_driver(pNandDrv);

				if(pNandDrv->u8_RequireReadRetry)
					NC_SendReadRetryCmd(0, 1);

				NC_CLR_DDR_MODE();
			}
		}
		#endif

		NC_CLR_DDR_MODE();
		REG_WRITE_UINT16(NC_PART_MODE, 0);

		#if IF_IP_VERIFY
		if (pNandDrv->u8_IfECCTesting)
			return UNFD_ST_SUCCESS;
		#endif
		return UNFD_ST_ERR_ECC_FAIL;
	}

	// =====================================
	LABEL_EXIT:
	if(u8_RetryCnt_Events || u8_RetryCnt_ECCFail)
	{
		NC_RestoreClk();
		//nand_debug(UNFD_DEBUG_LEVEL,1,"restore clk \n");
	}
	FLUSH_MIU_PIPE(); // Only used in U4 now

	REG_WRITE_UINT16(NC_PART_MODE, 0);
	NC_CLR_DDR_MODE();
	return UNFD_ST_SUCCESS;
}

U32  NC_ReadSectors(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_SectorCnt)
{
	U32 u32_Tmp, u32_SectorCnt_Tmp;
	U16 u16_PageCnt, u16_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr;

	#if CHECK_HAL_PARAM
	u32_Tmp = ((u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask) << pNandDrv->u8_PageSectorCntBits)
			  + u8_SectorInPage + u32_SectorCnt;
	if(u32_Tmp > ((U32)pNandDrv->u16_BlkPageCnt << pNandDrv->u8_PageSectorCntBits))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ReadSectors, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_R_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	#endif

	// HW setting
	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	//-----------------------------------------------
	// read sectors
	u32_SectorCnt_Tmp = pNandDrv->u16_PageSectorCnt - u8_SectorInPage;
	u32_SectorCnt_Tmp = (u32_SectorCnt_Tmp > u32_SectorCnt) ? u32_SectorCnt : u32_SectorCnt_Tmp;

	if (u32_SectorCnt_Tmp)
	{
		//nand_debug(1, 1, "NC_ReadSectors read sectors: %u \r\n", u32_SectorCnt_Tmp);
		//nand_debug(1, 1, "%X %X %X %X %X \r\n",
		//  u32_PhyRowIdx, u8_SectorInPage, pu8_DataBuf, pu8_SpareBuf, u32_SectorCnt_Tmp);

		#if defined(FCIE_LFSR) && FCIE_LFSR
		REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
		REG_SET_BITS_UINT16(NC_LFSR_CTRL,
			((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
		#endif

		u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_SectorByteCnt*u32_SectorCnt_Tmp, READ_FROM_NAND );

		if(pu8_SpareBuf == NULL)
		{
			u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pNandDrv->PlatCtx_t.pu8_PageSpareBuf, pNandDrv->u16_SpareByteCnt, READ_FROM_NAND );
		}
		else
			u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, READ_FROM_NAND );

		REG_WRITE_UINT16(NC_RSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
		REG_WRITE_UINT16(NC_RSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

		// if not 512B/page, set Partial Mode
		REG_READ_UINT16(NC_ECC_CTRL, u16_Tmp);
		if (u16_Tmp & BIT_NC_PAGE_SIZE_MASK)
		{
			REG_WRITE_UINT16(NC_PART_MODE, BIT_PARTIAL_MODE_EN);
			REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_CNT_MASK);
			REG_SET_BITS_UINT16(NC_PART_MODE, (u32_SectorCnt_Tmp-1)<<BIT_START_SECTOR_CNT_SHIFT);
			REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_IDX_MASK);
			REG_SET_BITS_UINT16(NC_PART_MODE, u8_SectorInPage<<BIT_START_SECTOR_IDX_SHIFT);
		}
		REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
		REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		if(0 == pNandDrv->u8_WordMode)
			REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << pNandDrv->u8_SectorByteCntBits);
		else
			REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << (pNandDrv->u8_SectorByteCntBits-1));
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));
		if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR) { // if a page 512B
			REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
			REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);
		} else {
			REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
			REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DIN);
		}

		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == (WAIT_READ_TIME))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadSectors Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
			#if 0==IF_IP_VERIFY
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			NC_ResetNandFlash();
			#else
			//nand_stop();
			#endif
			REG_WRITE_UINT16(NC_PART_MODE, 0);
			return UNFD_ST_ERR_R_TIMEOUT;
		}

		FLUSH_MIU_PIPE(); // Only used in U4 now

		// Check ECC
		REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
		if (u16_Tmp & BIT_NC_ECC_FAIL)
		{
			/*  prevent ecc error print in Search CIS in TV platform and no longer call this function after Search CIS. */
			#if defined(IF_IP_VERIFY) && IF_IP_VERIFY
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadSectors ECC Fail, Reg51:%04Xh ", u16_Tmp);
			REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg52:%04Xh ", u16_Tmp);
			REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "Reg53:%04Xh \r\n", u16_Tmp);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Blk: %Xh  Page: %X\r\n",
				u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask);
			#endif

			NC_CLR_DDR_MODE();
			REG_WRITE_UINT16(NC_PART_MODE, 0);

			#if 0 == IF_IP_VERIFY
			return UNFD_ST_ERR_ECC_FAIL;
			#else
			if (pNandDrv->u8_IfECCTesting)
				return UNFD_ST_SUCCESS;
			else
				return UNFD_ST_ERR_ECC_FAIL;
			#endif
		}

		REG_WRITE_UINT16(NC_PART_MODE, 0);
	}

	//-----------------------------------------------
	// read pages
	u32_SectorCnt -= u32_SectorCnt_Tmp;
	if (0 == u32_SectorCnt)
	{
		NC_CLR_DDR_MODE();
		return UNFD_ST_SUCCESS;
	}

	u32_PhyRowIdx += 1;
	u16_PageCnt = u32_SectorCnt >> pNandDrv->u8_PageSectorCntBits;

	pu8_DataBuf += u32_SectorCnt_Tmp << pNandDrv->u8_SectorByteCntBits;
	if (pu8_SpareBuf)
		//pu8_SpareBuf += u32_SectorCnt_Tmp << pNandDrv->u8_SectorSpareByteCntBits;
		pu8_SpareBuf += u32_SectorCnt_Tmp * pNandDrv->u16_SectorSpareByteCnt;

	if (u16_PageCnt)
	{
		//nand_debug(1, 1, "NC_ReadSectors read pages %X %X %X %X \r\n",
		//  u32_PhyRowIdx, pu8_DataBuf, pu8_SpareBuf, u16_PageCnt);

		u32_Tmp = NC_ReadPages(u32_PhyRowIdx, pu8_DataBuf, pu8_SpareBuf, u16_PageCnt);
		if (UNFD_ST_SUCCESS != u32_Tmp) {
			NC_CLR_DDR_MODE();
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadSectors read pages fail, ErrCode:%Xh \r\n", u32_Tmp);
			return u32_Tmp;
		}
	}

	//-----------------------------------------------
	// read sectors
	u32_SectorCnt_Tmp = u16_PageCnt << pNandDrv->u8_PageSectorCntBits;
	pu8_DataBuf += u32_SectorCnt_Tmp << pNandDrv->u8_SectorByteCntBits;
	if (pu8_SpareBuf)
		//pu8_SpareBuf += u32_SectorCnt_Tmp << pNandDrv->u8_SectorSpareByteCntBits;
		pu8_SpareBuf += u32_SectorCnt_Tmp * pNandDrv->u16_SectorSpareByteCnt;

	u32_SectorCnt -= u32_SectorCnt_Tmp;
	u32_PhyRowIdx += u16_PageCnt;
	if (u32_SectorCnt)
	{
		//nand_debug(1, 1, "NC_ReadSectors read last sectors %X %X %X %X \r\n",
		//  u32_PhyRowIdx, pu8_DataBuf, pu8_SpareBuf, u32_SectorCnt);

		u32_Tmp = NC_ReadSectors(u32_PhyRowIdx, 0, pu8_DataBuf, pu8_SpareBuf, u32_SectorCnt);
		if (UNFD_ST_SUCCESS != u32_Tmp)
		{
				NC_CLR_DDR_MODE();
		  nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadSectors read last sectors fail, ErrCode:%Xh \r\n", u32_Tmp);
		  return u32_Tmp;
		}
	}

	NC_CLR_DDR_MODE();
	return UNFD_ST_SUCCESS;
}


U32  NC_WriteSectors(	 U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_SectorCnt )
{
	U16 u16_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr = 0, u32_SpareDMAAddr;
	U32 u32_Ret;

	#if CHECK_HAL_PARAM
	if(u8_SectorInPage + u32_SectorCnt > pNandDrv->u16_PageSectorCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR, NAND, over one page sector cnt\n");
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "%X %X %X %X %X\n",
			u32_PhyRowIdx, u8_SectorInPage, (U32)pu8_DataBuf, (U32)pu8_SpareBuf, u32_SectorCnt);
		return UNFD_ST_ERR_INVALID_PARAM;
	}
	if(0 == u32_SectorCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR, NAND, zero sector cnt\n");
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "%X %X %X %X %X\n",
			u32_PhyRowIdx, u8_SectorInPage, (U32)pu8_DataBuf, (U32)pu8_SpareBuf, u32_SectorCnt);
		return UNFD_ST_ERR_INVALID_PARAM;
	}
	#endif
	//nand_debug(0,0,"ws b:%Xh p:%Xh \n", u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
	//	u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask);

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_SectorByteCnt, WRITE_TO_NAND);
	if(pu8_SpareBuf == NULL)
	{
		memset(pNandDrv->PlatCtx_t.pu8_PageSpareBuf, 0xff,  pNandDrv->u16_SpareByteCnt);
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pNandDrv->PlatCtx_t.pu8_PageSpareBuf, pNandDrv->u16_SectorSpareByteCnt, WRITE_TO_NAND);
	}
	else
		u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SectorSpareByteCnt, WRITE_TO_NAND);

	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	// if Partial Mode (if not 512B/page, set Partial Mode)
	REG_READ_UINT16(NC_ECC_CTRL, u16_Tmp);
	if (u16_Tmp & BIT_NC_PAGE_SIZE_MASK)
	{
		REG_WRITE_UINT16(NC_PART_MODE, BIT_PARTIAL_MODE_EN);
		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_CNT_MASK);
		REG_SET_BITS_UINT16(NC_PART_MODE, (u32_SectorCnt-1)<<BIT_START_SECTOR_CNT_SHIFT);
		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_IDX_MASK);
		REG_SET_BITS_UINT16(NC_PART_MODE, u8_SectorInPage<<BIT_START_SECTOR_IDX_SHIFT);
	}
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(0 == pNandDrv->u8_WordMode)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << pNandDrv->u8_SectorByteCntBits);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u8_SectorInPage << (pNandDrv->u8_SectorByteCntBits-1));
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8)| ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_CHKSTATUS);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	//while(1)  nand_reset_WatchDog();
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_WriteSectors Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		REG_WRITE_UINT16(NC_PART_MODE, 0);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	NC_CLR_DDR_MODE();
	REG_WRITE_UINT16(NC_PART_MODE, 0);
	u32_Ret = NC_CheckEWStatus(OPTYPE_WRITE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"write blk:%Xh p:%Xh fail:%Xh \n",
			u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
			u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask, u32_Ret);

	return u32_Ret;
}

U32 NC_ReadSector_RIUMode(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf)
{
	volatile U16 u16_Tmp, u16_i, u16_j, u16_k, u16_Cnt, u16_ByteLoc;
	U16 u16_Col;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Tmp, u32_Err;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN); // enable RIU Mode
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	if(pu8_SpareBuf == NULL)
		pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;

	// if Partial Mode (if not 512B/page, set Partial Mode)
	REG_READ_UINT16(NC_ECC_CTRL, u16_Tmp);
	if (u16_Tmp & BIT_NC_PAGE_SIZE_MASK) {
		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_CNT_MASK);

		REG_CLR_BITS_UINT16(NC_PART_MODE, BIT_START_SECTOR_IDX_MASK);
		REG_SET_BITS_UINT16(NC_PART_MODE, (u8_SectorInPage<<BIT_START_SECTOR_IDX_SHIFT));
		REG_SET_BITS_UINT16(NC_PART_MODE,  BIT_PARTIAL_MODE_EN);
	}

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(0 == pNandDrv->u8_WordMode)
		u16_Col = u8_SectorInPage << pNandDrv->u8_SectorByteCntBits;
	else
		u16_Col = u8_SectorInPage << (pNandDrv->u8_SectorByteCntBits-1);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u16_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR) {
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	} else {
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DIN);
	}

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	//read for data
	u16_Cnt = pNandDrv->u16_SectorByteCnt >> 6; // how many 64B per sector
	u16_Cnt = (pNandDrv->u16_SectorByteCnt & (0x40 -1)) ? u16_Cnt + 1 : u16_Cnt;
	for (u16_j=0; u16_j<u16_Cnt; u16_j++)
	{   // read data
		u16_Tmp =( (pNandDrv->u16_SectorByteCnt - (u16_j << 6)) >= 0x40) ?
			0x40 : (pNandDrv->u16_SectorByteCnt - (u16_j << 6) );
		u32_Err = NC_WaitGetCIFD((U8*)((U32)pu8_DataBuf + (u16_j << 6)), u16_Tmp, WAIT_READ_TIME);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return u32_Err;
		}
	}

	//read for spare
	u16_Cnt = pNandDrv->u16_SectorSpareByteCnt >> 6; // how many 64B per sector
	u16_Cnt = (pNandDrv->u16_SectorSpareByteCnt & (0x40 -1)) ? u16_Cnt + 1 : u16_Cnt;
	for (u16_j=0; u16_j<u16_Cnt; u16_j++)
	{   // read data
		u16_Tmp =( (pNandDrv->u16_SectorSpareByteCnt - (u16_j << 6)) >= 0x40) ?
			0x40: (pNandDrv->u16_SectorSpareByteCnt - (u16_j << 6));
		u32_Err = NC_WaitGetCIFD( (U8*)((U32)pu8_SpareBuf + (u16_j << 6)), u16_Tmp, WAIT_READ_TIME);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return u32_Err;
		}
	}

	// wait for events
	u32_Tmp = 0;
	do{
		u16_Tmp = 0; // do not remove this line
		REG_READ_UINT16(NC_MIE_EVENT, u16_Tmp);
		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		u32_Tmp++;
		if(u32_Tmp > WAIT_READ_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"Error: Read_RM timeout 2, ErrCode:%X\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			NC_DumpDebugBus();
			NC_DumpRegisters();
			REG_WRITE_UINT16(NC_PART_MODE, 0);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM;
		}
	}while(((u16_Tmp & BIT_NC_R2N_ECCCOR)==0) && ((u16_Tmp & (BIT_NC_JOB_END )) != (BIT_NC_JOB_END )));

	// if ECC Fail
	REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
	if ((u16_Tmp & BIT_NC_ECC_FLGA_MASK) == BIT_NC_ECC_FLGA_FAIL)
	{
		REG_WRITE_UINT16(NC_PART_MODE, 0);
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Read_RM ECC Fail, ErrCode:%Xh \r\n", UNFD_ST_ERR_ECC_FAIL_RM);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Blk: %Xh  Page: %X\r\n",
		u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask);
		return UNFD_ST_ERR_ECC_FAIL_RM;
	}

	REG_READ_UINT16(NC_MIE_EVENT, u16_Tmp);
	if (u16_Tmp & BIT_NC_R2N_ECCCOR)
	{
//		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "RIU Mode, ECC happens: ");
		// make CIFD available for SW
		// [CAUTION]: before clear BIT_NC_R2N_ECC, JOB_END would not set.
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_R2N_ECCCOR);
		if(NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == WAIT_READ_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"Error: Read_RM timeout 3, ErrCode:%X\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_WRITE_UINT16(NC_PART_MODE, 0);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
		}

		REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
		u16_Cnt = (u16_Tmp & BIT_NC_ECC_CNT_MASK) >> BIT_NC_ECC_CNT_SHIFT;
//		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "%u bits\r\n", u16_Cnt);
		for (u16_i=0; u16_i<u16_Cnt; u16_i++)
		{
			REG_CLR_BITS_UINT16(NC_ECC_STAT2, BIT_NC_ECC_SEL_LOC_MASK);
			REG_SET_BITS_UINT16(NC_ECC_STAT2, u16_i<<BIT_NC_ECC_SEL_LOC_SHIFT);
			LABEL_ECC_LOC_READ:
			//REG_READ_UINT16(NC_ECC_LOC, u16_Tmp);
			REG_READ_UINT16(NC_ECC_LOC, u16_j);
			REG_READ_UINT16(NC_ECC_LOC, u16_k);
			if(u16_j != u16_k)
				goto LABEL_ECC_LOC_READ;
//			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "bit loc: %02Xh \r\n", u16_j);
			u16_ByteLoc = u16_j >> 3;
			if (u16_ByteLoc < pNandDrv->u16_SectorByteCnt)
			{
				pu8_DataBuf[u16_ByteLoc] ^= 1<<(u16_j%8);
			}
			else if (u16_ByteLoc < pNandDrv->u16_SectorByteCnt +
				   pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt)
			{
				pu8_SpareBuf[u16_ByteLoc-pNandDrv->u16_SectorByteCnt] ^= 1<<(u16_j%8);
			}
		}

		u16_Tmp = BIT_NC_R2N_ECCCOR;
	}

	else if(0 == (u16_Tmp & BIT_NC_R2N_ECCCOR)){
		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == WAIT_READ_TIME)
		{
			U16 u16_Reg;
			REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Read_RM timeout 2, ErrCode:%Xh, Reg00h:%Xh\r\n",
				   UNFD_ST_ERR_R_TIMEOUT_RM, u16_Reg);

			REG_WRITE_UINT16(NC_PART_MODE, 0);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

			#if 0 == IF_IP_VERIFY
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			//NC_ResetNandFlash(); // save code size
			#else
			//nand_stop();
			#endif
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}
	}

	NC_CLR_DDR_MODE();
	REG_WRITE_UINT16(NC_PART_MODE, 0);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	return UNFD_ST_SUCCESS;
}

U32  NC_WritePage_RIUMode(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf )
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U16 u16_i, u16_j, u16_DataCnt, u16_SpareCnt, u16_Tmp;
	U32 u32_Err;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	if(pu8_SpareBuf == NULL)
	{
		pu8_SpareBuf = pNandDrv->PlatCtx_t.pu8_PageSpareBuf;
		memset(pu8_SpareBuf, 0xff, pNandDrv->u16_SpareByteCnt);
	}

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8)| ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_CHKSTATUS);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	u16_DataCnt = pNandDrv->u16_SectorByteCnt >> 6; // how many 64B per sector
	u16_DataCnt = (pNandDrv->u16_SectorByteCnt & (0x40 -1)) ? u16_DataCnt + 1 : u16_DataCnt;

	u16_SpareCnt = (pNandDrv->u16_SectorSpareByteCnt -  pNandDrv->u16_ECCCodeByteCnt)>> 6; // how many 64B per sector spare
	u16_SpareCnt = ((pNandDrv->u16_SectorSpareByteCnt -  pNandDrv->u16_ECCCodeByteCnt) & (0x40 -1)) ? u16_SpareCnt + 1 : u16_SpareCnt;

	//---------------------------------------------------
	// fill in DO
	// do filling data for each sector, data and spare are filled separately
	for(u16_i = 0; u16_i <  pNandDrv->u16_PageSectorCnt; u16_i ++)
	{
		//fill for data
		for (u16_j=0; u16_j<u16_DataCnt; u16_j++)
		{
			u16_Tmp =( (pNandDrv->u16_SectorByteCnt - (u16_j << 6)) > 0x40) ?
				0x40: pNandDrv->u16_SectorByteCnt - (u16_j << 6);

			u32_Err = NC_WaitSetCIFD( (U8*)( (U32) pu8_DataBuf + (u16_i << pNandDrv->u8_SectorByteCntBits) + (u16_j << 6)), u16_Tmp);
			if(u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "Transfer Byte %X\n", (u16_i << pNandDrv->u8_SectorByteCntBits) + (u16_j << 6));
				REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
				NC_ReConfig();
				NC_CLR_DDR_MODE();
				return u32_Err;
			}

		}
		//fill for spare
		for (u16_j=0; u16_j<u16_SpareCnt; u16_j++)
		{
			if((REG(NC_ECC_CTRL) & BIT_NC_BYPASS_ECC) ==BIT_NC_BYPASS_ECC)
			{
				u16_Tmp =( ((pNandDrv->u16_SectorSpareByteCnt)- (u16_j << 6)) > 0x40) ?
					0x40: (pNandDrv->u16_SectorSpareByteCnt) - (u16_j << 6);
			}
			else
			{
				u16_Tmp =( ((pNandDrv->u16_SectorSpareByteCnt -  pNandDrv->u16_ECCCodeByteCnt)- (u16_j << 6)) > 0x40) ?
					0x40: (pNandDrv->u16_SectorSpareByteCnt -  pNandDrv->u16_ECCCodeByteCnt) - (u16_j << 6);
			}

			u32_Err = NC_WaitSetCIFD((U8*)((U32)pu8_SpareBuf + (u16_i *  pNandDrv->u16_SectorSpareByteCnt) + (u16_j << 6)),  u16_Tmp);
			if(u32_Err != UNFD_ST_SUCCESS)
			{
				nand_debug(0, 1, "Transfer Spare Byte %X\n", (u16_i *  pNandDrv->u16_SectorSpareByteCnt) + (u16_j << 6));
				REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
				NC_ReConfig();
				NC_CLR_DDR_MODE();
				return u32_Err;
			}
		}
	}

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		U16 u16_Reg;
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Write_RM timeout 3, ErrCode:%Xh, Reg00h:%Xh\r\n",
			   UNFD_ST_ERR_W_TIMEOUT_RM, u16_Reg);
		#if 0 == IF_IP_VERIFY
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		//NC_ResetNandFlash(); // save code size
		#else
		//nand_stop();
		#endif
		return UNFD_ST_ERR_W_TIMEOUT_RM; // timeout
	}

	NC_CLR_DDR_MODE();
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	return NC_CheckEWStatus(OPTYPE_WRITE);
}

U32 NC_Read_RandomIn (	 U32 u32_PhyRow, U32 u32_Col, U8 *pu8DataBuf, U32 u32DataByteCnt )
{
	U32 u32_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U16 u16_Cnt, u16_j, u16_Tmp;


	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();
	u32_Tmp = u32DataByteCnt + (u32DataByteCnt & 0x01); // byet-count needs to be word-alignment


	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u8_WordMode)
		u32_Col >>= 1;
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x00));
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_RAN_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	} else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);
	}

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	/* get data from CIFD */

	u16_Cnt = u32_Tmp >> 6; // how many 64B of random count
	u16_Cnt = (u32_Tmp & (0x40 -1)) ? u16_Cnt + 1 : u16_Cnt;
	for (u16_j=0; u16_j < u16_Cnt; u16_j++)
	{	// read data
		u16_Tmp =( (u32_Tmp - (u16_j << 6)) > 0x40) ? 0x40: u32_Tmp - (u16_j << 6);
		u32_Err = NC_WaitGetCIFD(pu8DataBuf + (u16_j << 6),  u16_Tmp, WAIT_READ_TIME);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return u32_Err;
		}
	}

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == WAIT_READ_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err: drvNAND_ReadRandomData_Ex - timeout \r\n");
		#if 0==IF_IP_VERIFY
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();

		#else
		//nand_stop();
		#endif
		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_CLR_DDR_MODE();


	return UNFD_ST_SUCCESS;
}

U32 NC_Write_RandomOut (U32 u32_PhyRow, U32 u32_Col, U8 *pu8_DataBuf, U32 u32_DataByteCnt )
{
	U32 u32_Tmp;
	U32 u32_Err;
	U16 u16_j, u16_Tmp, u16_Cnt;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	u32_Tmp = u32_DataByteCnt + (u32_DataByteCnt & 0x01); // byet-count needs to be word-alignment

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u8_WordMode)
		u32_Col >>= 1;
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_RAN_DOUT);

	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8)| ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_CHKSTATUS);


	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	u16_Cnt = u32_Tmp >> 6; // how many 64B of random count
	u16_Cnt = (u32_Tmp & (0x40 -1)) ? u16_Cnt + 1 : u16_Cnt;
	for (u16_j=0; u16_j<u16_Cnt; u16_j++)
	{
		u16_Tmp =( (u32_Tmp - (u16_j << 6)) > 0x40) ?
			0x40: u32_Tmp - (u16_j << 6);

		u32_Err = NC_WaitSetCIFD( (U8*)( (U32) pu8_DataBuf + (u16_j << 6)),  u16_Tmp);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			nand_debug(0, 1, "Transfer Spare Byte %X\n", (u16_j << 6));
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return u32_Err;
		}
	}

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_Write_RandomOut Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif

		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_CLR_DDR_MODE();
	return NC_CheckEWStatus(OPTYPE_WRITE);
}

U32 NC_Read_RandomIn_MIU (U32 u32_PhyRow, U32 u32_Col, U8 *pu8DataBuf, U32 u32DataByteCnt )
{
	U32 u32_Tmp, u32_DataDMAAddr;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();
	u32_Tmp = u32DataByteCnt + (u32DataByteCnt & 0x01); // byet-count needs to be word-alignment

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8DataBuf, u32_Tmp, READ_FROM_NAND);

	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u8_WordMode)
		u32_Col >>= 1;
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x00));
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_RAN_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	} else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);
	}

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	/* get data from CIFD */

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == WAIT_READ_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Err: drvNAND_ReadRandomData_Ex - timeout \r\n");
		#if 0==IF_IP_VERIFY
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();

		#else
		//nand_stop();
		#endif
		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_CLR_DDR_MODE();


	return UNFD_ST_SUCCESS;
}

U32 NC_Write_RandomOut_MIU (U32 u32_PhyRow, U32 u32_Col, U8 *pu8_DataBuf, U32 u32_DataByteCnt )
{
	U32 u32_Tmp;
	U32 u32_DataDMAAddr;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	u32_Tmp = u32_DataByteCnt + (u32_DataByteCnt & 0x01); // byet-count needs to be word-alignment

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, u32_Tmp, WRITE_TO_NAND);

	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u8_WordMode)
		u32_Col >>= 1;
	if (pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_RAN_DOUT);

	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8)| ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_CHKSTATUS);


	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_Write_RandomOut Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif

		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_CLR_DDR_MODE();
	return NC_CheckEWStatus(OPTYPE_WRITE);
}

U32 NC_ReadID(void)
{
	volatile U16 u16_Reg;
	U16 u16_i;
	U16 u16_IDByteCnt;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	// data go through CIFD
	REG_SET_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ADR_C2T1S0 << 8) | CMD_0x90);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);

	if( pNandDrv->u8_WordMode)
		u16_IDByteCnt = NAND_ID_BYTE_CNT<<1;
	else
		u16_IDByteCnt = NAND_ID_BYTE_CNT+1;

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u16_IDByteCnt);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); /*offset 0*/

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	//polling for wbuf is full, means wbuffer is full or random data is done
	if(NC_WaitCIFD_Event(BIT_WBUF_FULL, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error! CIFD Event Timeout!\r\n");
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_MIE_EVENT: %Xh \r\n", u16_Reg);
		REG_READ_UINT16(NC_CTRL, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_CTRL: %Xh \r\n", u16_Reg);
		NC_ReConfig();
		NC_ResetNandFlash();
		NC_CLR_DDR_MODE();
		REG_CLR_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);
		return UNFD_ST_ERR_R_TIMEOUT;
	}

	if( pNandDrv->u8_WordMode)
	{
		for(u16_i=0 ; u16_i<NAND_ID_BYTE_CNT ; u16_i++)
		{
			NC_GetCIFD(&pNandDrv->au8_ID[u16_i], u16_i<<1, 1);
		}
	}
	else
	{
		NC_GetCIFD(pNandDrv->au8_ID, 0, NAND_ID_BYTE_CNT);
	}

	REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_WBUF_FULL);
	REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_WBUF_EMPTY_TRI);

	if (NC_WaitComplete(BIT_NC_JOB_END, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error! Read ID timeout!\r\n");

		#if 0==IF_IP_VERIFY
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_MIE_EVENT: %Xh \r\n", u16_Reg);
		REG_READ_UINT16(NC_CTRL, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_CTRL: %Xh \r\n", u16_Reg);
		#else
		//nand_stop();
		#endif
		NC_ReConfig();
		NC_ResetNandFlash();
		NC_CLR_DDR_MODE();
		REG_CLR_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);
		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}

	nand_debug(UNFD_DEBUG_LEVEL_LOW, 1, "ID: %02X ", pNandDrv->au8_ID[0]);

	#if 0
	#if IF_IP_VERIFY
	pNandDrv->u8_IDByteCnt = NAND_DEVICE_ID_LEN;
	#else
	pNandDrv->u8_IDByteCnt = NAND_ID_BYTE_CNT;//u16_i;
	#endif
	#else
	if(0 == pNandDrv->u8_IDByteCnt)
	{
		for (u16_i=2; u16_i<(NAND_ID_BYTE_CNT-1); u16_i++)
			if(pNandDrv->au8_ID[u16_i] == pNandDrv->au8_ID[0] &&
				pNandDrv->au8_ID[u16_i+1] == pNandDrv->au8_ID[1])
				break;
		pNandDrv->u8_IDByteCnt = u16_i;
	}
	#endif

	for (u16_i=1; u16_i<pNandDrv->u8_IDByteCnt; u16_i++)
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "%02X ", pNandDrv->au8_ID[u16_i]);

	nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, ", total %u bytes ID.\r\n", pNandDrv->u8_IDByteCnt);

	NC_CLR_DDR_MODE();
	REG_CLR_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);
	return UNFD_ST_SUCCESS; // ok
}

//=========================================================
// [CAUTION]: FCIE5 do NOT support DDR-NAND with x16 IO
//=========================================================
#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT

static U32 NC_SetFeature_Ex(U8 u8_Cmd, U8 u8_Addr, U8 *pu8_DataBuf, U32 u32_DataByteCnt)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	#if 0==IF_IP_VERIFY
	U16 u16_Reg;
	#endif
	U32 u32_Err;

	if(u32_DataByteCnt & 1)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1,
			"error, NAND, odd bytes (%Xh) to set \n", u32_DataByteCnt);
		return UNFD_ST_ERR_HAL_W_INVALID_PARAM;
	}

	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u8_Cmd);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (u8_Addr<<8)|u8_Addr);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ADR_C2T1S0 << 8) | CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | ACT_RAN_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_DataByteCnt);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);	// offset;

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	u32_Err = NC_WaitSetCIFD(pu8_DataBuf, u32_DataByteCnt);
	if(u32_Err != UNFD_ST_SUCCESS)
	{
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		return u32_Err;
	}

	if (NC_WaitComplete(BIT_NC_JOB_END, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error! timeout!\r\n");

			#if 0==IF_IP_VERIFY
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_MIE_EVENT: %Xh \r\n", u16_Reg);
		REG_READ_UINT16(NC_CTRL, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_CTRL: %Xh \r\n", u16_Reg);

		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		nand_stop();
		#endif
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		return UNFD_ST_ERR_SET_FEATURE_TIMEOUT;
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	return UNFD_ST_SUCCESS;
}


U32 NC_SetONFISyncMode(U8 u8_SetSyncMode)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U8  au8_buf[4*3];
	U8 *au8_PageBuf = (U8*)pNandDrv->pu8_PageDataBuf;
	U8 u8_TimingMode = 0;

	if(u8_SetSyncMode)
	{
		u32_Err = nand_check_DDR_pad();
		if(u32_Err != UNFD_ST_SUCCESS)
			return u32_Err;
	}

	if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
	{
		if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_TOGGLE)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1,
				"Error, NAND, ToggleDDR can not set to Sync Mode\n");
			return UNFD_ST_ERR_TOGGLE_SET_SYNC_MODE;
		}
		return UNFD_ST_SUCCESS;
	}

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();

	// read ONFI parameters
	u32_Err = NC_RandomIn_1Cmd1Addr(
		NAND_CMD_READ_PARAMETER, NAND_ADR_READ_PARAMETER, NAND_DDR_REMOVE_2NDBYTE_NA,
		142, au8_PageBuf);
	if(UNFD_ST_SUCCESS == u32_Err)
	{
		for(u8_TimingMode=5;u8_TimingMode>0;u8_TimingMode--)
		{
			if(u8_SetSyncMode)
			{
				if(au8_PageBuf[141]&(1<<u8_TimingMode))
					break;
			}
			else
			{
				if(au8_PageBuf[129]&(1<<u8_TimingMode))
					break;
			}
		}
		if(u8_SetSyncMode)
			nand_debug(UNFD_DEBUG_LEVEL, 1, "Timing Mode=%d, Parameter Page[142:141]=%Xh,%Xh\n",
				u8_TimingMode, au8_PageBuf[142], au8_PageBuf[141]);
		else
			nand_debug(UNFD_DEBUG_LEVEL, 1, "Timing Mode=%d, Parameter Page[130:129]=%Xh,%Xh\n",
				u8_TimingMode, au8_PageBuf[130], au8_PageBuf[129]);

	}

	// Get Feature
	u32_Err = NC_RandomIn_1Cmd1Addr(
		NAND_CMD_GET_FEATURE, NAND_ADR_TIMING_MODE, NAND_DDR_REMOVE_2NDBYTE_ONFI,
		4, au8_buf);
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "err code: %X\n", u32_Err);
		return u32_Err;
	}

	#if 0
	au8_buf[0] &= ~(3<<4);
	au8_buf[0] |=   1<<4; // set Sync Mode
	#else
	// need to check supported mode fist
	au8_buf[0] &= ~((3<<4)|0xF);
	if(u8_SetSyncMode)
		au8_buf[0] |=   ((1<<4)|u8_TimingMode); // set Sync Mode
	else
		au8_buf[0] |=   (u8_TimingMode); // set Timing Mode only
	#endif

	u32_Err = NC_SetFeature_Ex(
		NAND_CMD_SET_FEATURE, NAND_ADR_TIMING_MODE, au8_buf, 4);
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "err code: %X\n", u32_Err);
		return u32_Err;
	}

	#if 0 // for LA test
	NC_RandomIn_1Cmd1Addr(
		NAND_CMD_GET_FEATURE, NAND_ADR_TIMING_MODE, NAND_DDR_REMOVE_2NDBYTE_ONFI,
		4, au8_buf);
	#endif

	return u32_Err;
}

U32 NC_SetToggleDDRMode(void)
{
	U32 u32_Err;
	U8  au8_buf[8];

	u32_Err = nand_check_DDR_pad();
	if(u32_Err != UNFD_ST_SUCCESS)
		return u32_Err;

	nand_pads_switch(NAND_PAD_BYPASS_MODE);
	NC_RECONFIG();

	// Get Feature
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "Get Feature\n");
	u32_Err = NC_RandomIn_1Cmd1Addr(
		NAND_CMD_GET_FEATURE, NAND_ADR_INTERFACE_MODE, NAND_DDR_REMOVE_2NDBYTE_NA,
		4, au8_buf);
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "err code: %X\n", u32_Err);
		return u32_Err;
	}
	nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0,"DDR Interface : %X\n", au8_buf[0]);

	if((au8_buf[0] & (BIT8-1)) != 0)
	{
		au8_buf[0] &= (U8)((~(BIT8-1))& 0xFF); // set DDR Interface
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 0, "Set Feature\n");
		u32_Err = NC_SetFeature_Ex(
			NAND_CMD_SET_FEATURE, NAND_ADR_INTERFACE_MODE, au8_buf, 4);
		if(UNFD_ST_SUCCESS != u32_Err)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "err code: %X\n", u32_Err);
			return u32_Err;
		}
	}
	return u32_Err;

}


#define JEDEC_TOGGLE_DDR_ID_BYTE_CNT  5
static const U8 sgau8_JEDEC[JEDEC_TOGGLE_DDR_ID_BYTE_CNT]
				= {'J','E','D','E','C'};
#define NAND_TOGGLE_MODE_READ_ID_40h  BIT1

#define ONFI_ID_BYTE_CNT  4
static const U8 sgau8_ONFI[ONFI_ID_BYTE_CNT]
				= {'O','N','F','I'};
#define NAND_ONFI_DDR_MODE_READ_BYTE  6
#define NAND_ONFI_DDR_MODE_READ_BIT   BIT5

static U32 NC_RandomIn_1Cmd1Addr(U8 u8_Cmd, U8 u8_Addr, U8 u8_Remove2ndByte, U8 u8_ByteCnt, U8 *pu8_DataBuf)
{
	//volatile U16 u16_Reg;
	U16 u16_i, u16_Cnt, u16_j, u16_Tmp;
	U32 u32_Err;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	// data go through CIFD

	NC_SET_DDR_MODE();

	//data go through DMA
	if(u8_ByteCnt & 1)
		u8_ByteCnt++;
	REG_SET_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);
	#if 0
	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, u8_ByteCnt, READ_FROM_NAND );

	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);
	#endif
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u8_Cmd);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (u8_Addr<<8)|u8_Addr);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ADR_C2T1S0 << 8) | CMD_REG8L);
	if(0x90 != u8_Cmd)
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_RAN_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);

	#if 0 // FCIE4 can NOT support DDR with x16 IOs
	if( pNandDrv->u8_WordMode)
		u16_IDByteCnt = u8_ByteCnt<<1; // for FCIE: /RE clk count
	else
		u16_IDByteCnt = u8_ByteCnt;
	#endif

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u8_ByteCnt);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);	/*offset 0*/

	REG_WRITE_UINT16(NC_CTRL,  BIT_NC_JOB_START);

	#if 1
	Disable_Err_log = 1;

	u16_Cnt = u8_ByteCnt >> 6; // how many 64B of random count
	u16_Cnt = (u8_ByteCnt & (0x40 -1)) ? u16_Cnt + 1 : u16_Cnt;
	for (u16_j=0; u16_j < u16_Cnt; u16_j++)
	{	// read data
		u16_Tmp =( (u8_ByteCnt - (u16_j << 6)) > 0x40) ? 0x40: u8_ByteCnt - (u16_j << 6);
		u32_Err = NC_WaitGetCIFD(pu8_DataBuf + (u16_j << 6),  u16_Tmp, WAIT_PROBE_TIME);
		if(u32_Err != UNFD_ST_SUCCESS)
		{
			Disable_Err_log = 0;
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return u32_Err;
		}
	}
	#endif

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_PROBE_TIME) == WAIT_PROBE_TIME)
	{
	#if 0==IF_IP_VERIFY
		//REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		//nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "NC_MIE_EVENT: %Xh \r\n", u16_Reg);
		//REG_READ_UINT16(NC_CTRL, u16_Reg);
		//nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "NC_CTRL: %Xh \r\n", u16_Reg);
	#else
		//nand_stop();
	#endif
		Disable_Err_log = 0;

		REG_CLR_BITS_UINT16(NC_FUN_CTL,BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_ResetNandFlash();
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}
	Disable_Err_log = 0;


	#if IF_IP_VERIFY
	nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "ID: %02X ", pu8_DataBuf[0]);

	for (u16_i=1; u16_i<u8_ByteCnt; u16_i++)
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "%02X ", pu8_DataBuf[u16_i]);

	nand_debug(0,0,"\n");
	#endif

	// remove the double bytes if DDR mode
	if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
	switch(u8_Remove2ndByte)
	{
		case NAND_DDR_REMOVE_2NDBYTE_TOGGLE:
			for(u16_i=2; u16_i < u8_ByteCnt; u16_i++)
					pu8_DataBuf[u16_i] = pu8_DataBuf[u16_i<<1];
			break;
		case NAND_DDR_REMOVE_2NDBYTE_ONFI:
			for(u16_i=1; u16_i < u8_ByteCnt; u16_i++)
					pu8_DataBuf[u16_i] = pu8_DataBuf[u16_i<<1];
			break;
	}

	#if IF_IP_VERIFY
	nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "ID: %02X ", pu8_DataBuf[0]);

	for (u16_i=1; u16_i<u8_ByteCnt; u16_i++)
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "%02X ", pu8_DataBuf[u16_i]);

	nand_debug(0,0,"\n\n");
	#endif

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_CLR_DDR_MODE();
	return UNFD_ST_SUCCESS;	// ok
}

static U32 NC_ProbeIfONFIDDR_Ex(void)
{
	//NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U8 u8_i, au8_buf[sizeof(sgau8_ONFI)*4];

	// read ONFI parameters
	u32_Err = NC_RandomIn_1Cmd1Addr(
		NAND_CMD_READ_PARAMETER, NAND_ADR_READ_PARAMETER, NAND_DDR_REMOVE_2NDBYTE_NA,
		sizeof(sgau8_ONFI)*2+2, au8_buf);
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		//nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "err code: %X\n", u32_Err);
		return u32_Err;
	}

	for(u8_i=0; u8_i<sizeof(sgau8_ONFI); u8_i++)
		if(sgau8_ONFI[u8_i] != au8_buf[u8_i])
			break;
	if(sizeof(sgau8_ONFI) != u8_i)
		return UNFD_ST_ERR_NOT_ONFI_DDR;

	if(0 == (au8_buf[NAND_ONFI_DDR_MODE_READ_BYTE] & NAND_ONFI_DDR_MODE_READ_BIT))
		return UNFD_ST_ERR_NOT_ONFI_DDR;

	return UNFD_ST_SUCCESS;
}


U32 NC_ProbeIfONFIDDR(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	//U8 u8_i;

	pNandDrv = pNandDrv;
	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();

	u32_Err = NC_ProbeIfONFIDDR_Ex();
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "err code: %X\n", u32_Err);
		return UNFD_ST_ERR_NOT_ONFI_DDR;
	}

	return u32_Err;

}

static U32 NC_ProbeIfToggleDDR_Ex(void)
{
	//NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;
	U8 u8_i, au8_buf[sizeof(sgau8_JEDEC)*5];

	u32_Err = NC_RandomIn_1Cmd1Addr(
		NAND_CMD_READ_ID, NAND_ADR_READ_JEDEC, NAND_DDR_REMOVE_2NDBYTE_TOGGLE,
		sizeof(sgau8_JEDEC)*2+2, au8_buf);
	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "err code: %X\n", u32_Err);
		return UNFD_ST_ERR_NOT_TOGGLE_DDR;
	}

	// check if repeat "JEDEC"
	for(u8_i=0; u8_i<sizeof(sgau8_JEDEC); u8_i++)
		if(au8_buf[u8_i] != au8_buf[u8_i + sizeof(sgau8_JEDEC)])
			break;
	if(sizeof(sgau8_JEDEC) == u8_i)
		return UNFD_ST_ERR_NOT_TOGGLE_DDR;

	// check if 'J' 'E' 'D' 'E' 'C' Toggle DDR
	for(u8_i=0; u8_i<sizeof(sgau8_JEDEC); u8_i++)
		if(sgau8_JEDEC[u8_i] != au8_buf[u8_i])
			break;
	if(sizeof(sgau8_JEDEC) != u8_i)
		return UNFD_ST_ERR_NOT_TOGGLE_DDR;

	if(0 == (au8_buf[u8_i]& NAND_TOGGLE_MODE_READ_ID_40h) )
		return UNFD_ST_ERR_NOT_TOGGLE_DDR;

	return UNFD_ST_SUCCESS;
}


U32 NC_ProbeIfToggleDDR(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Err;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();

	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);

	u32_Err = NC_ProbeIfToggleDDR_Ex();
	nand_clock_setting(pNandDrv->u32_Clk);

	if(UNFD_ST_SUCCESS != u32_Err)
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "err code: %X\n", u32_Err);
		return u32_Err;
	}

	return u32_Err;

}

#define MIN_PASS_CNT	3
U32 NC_DetectDDRTiming(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_RetVal;
	U8 u8_2Ch_dqsmode;
	U8 u8_max_delay_cell;
	U32 (*pFn_ProbeIfDDR_Ex)(void);
	const U8 au8_dqsmode[DQS_MODE_TABLE_CNT] = DQS_MODE_SEARCH_TABLE;	// defined in platform-depedent .h
	U8 u8_DqsPassCnt;
	U32 u32_DqsMatch;
	U8 u8_DelayCellLen, u8_DelayCellMaxLen;
	U8	u8_DelayCellBegin, u8_DelayCellMaxBegin;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	nand_clock_setting(pNandDrv->u32_Clk);		//set clock to 4x mode
	NC_RECONFIG();

	if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_ONFI)
		pFn_ProbeIfDDR_Ex = NC_ProbeIfONFIDDR_Ex;
	else if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_TOGGLE)
		pFn_ProbeIfDDR_Ex = NC_ProbeIfToggleDDR_Ex;
	else
	{	nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"Error, NAND, no DDR If selected.\n");
		return UNFD_ST_ERR_INVALID_PARAM;
	}

	u8_max_delay_cell = 0;

	// detect DQS_MODE & DQS_DELAY_CELL & RE_DDR_TIMING
	u32_DqsMatch = 0;
	u8_DqsPassCnt = 0;

	//ds mode, scan rxdll
	for(u8_2Ch_dqsmode=0 ; u8_2Ch_dqsmode<DQS_MODE_TABLE_CNT ; u8_2Ch_dqsmode++)
	{
		NC_FCIE5SetInterface(1, au8_dqsmode[u8_2Ch_dqsmode], u8_max_delay_cell, 0);
		u32_RetVal = (*pFn_ProbeIfDDR_Ex)();
		if(UNFD_ST_SUCCESS == u32_RetVal || UNFD_ST_ERR_PROBABLE_TOGGLE_DDR == u32_RetVal )
		{
			//nand_debug(UNFD_DEBUG_LEVEL_ERROR,1, "dqs_mode:%u, 57h:%u\n", au8_dqsmode[u8_2Ch_dqsmode], u8_57h);
			u32_DqsMatch |= (1<<u8_2Ch_dqsmode);
			u8_DqsPassCnt++;
		}
	}
	//nand_debug(UNFD_DEBUG_LEVEL,1, "DqsPass = %d,  u32_DqsMatch = %X \n", u8_DqsPassCnt, u32_DqsMatch);
	if(!u8_DqsPassCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1, "Err, NAND, can't detect right timing \n");
		return UNFD_ST_ERR_NO_TOGGLE_DDR_TIMING;
	}

	if(u8_DqsPassCnt >= 2)
	{
		//find longest sequence and get its central phase
		u8_DelayCellLen = 0;
		u8_DelayCellBegin = 0;
		u8_DelayCellMaxBegin = 0;
		u8_DelayCellMaxLen = 0;
		for(u8_2Ch_dqsmode = 0; u8_2Ch_dqsmode <  DQS_MODE_TABLE_CNT; u8_2Ch_dqsmode ++)
		{
			if(((u32_DqsMatch >> u8_2Ch_dqsmode) & 0x1) == 0x1)
			{
				if(u8_DelayCellLen == 0)
					u8_DelayCellBegin = u8_2Ch_dqsmode;
				u8_DelayCellLen ++;

				if(u8_2Ch_dqsmode == (DQS_MODE_TABLE_CNT - 1))
				{
					if(u8_DelayCellMaxLen < u8_DelayCellLen)
					{
						u8_DelayCellMaxBegin = u8_DelayCellBegin;
						u8_DelayCellMaxLen = u8_DelayCellLen;
					}
				}
			}
			else
			{
				if(u8_DelayCellMaxLen == 0)
				{
					u8_DelayCellMaxBegin = u8_DelayCellBegin;
					u8_DelayCellMaxLen = u8_DelayCellLen;
					nand_debug(UNFD_DEBUG_LEVEL, 0, " len  %d, Begin %d\n",u8_DelayCellMaxLen, u8_DelayCellMaxBegin );
				}
				else
				{
					if(u8_DelayCellMaxLen < u8_DelayCellLen)
					{
						u8_DelayCellMaxBegin = u8_DelayCellBegin;
						u8_DelayCellMaxLen = u8_DelayCellLen;
					}
				}
				u8_DelayCellLen = 0;
			}
		}

		//nand_debug(UNFD_DEBUG_LEVEL, 0, " final len  %d, Begin %d\n",u8_DelayCellMaxLen, u8_DelayCellMaxBegin );
		u8_2Ch_dqsmode = u8_DelayCellMaxBegin + (u8_DelayCellMaxLen >> 1);
	}
	else
	{
		for(u8_2Ch_dqsmode = 0; u8_2Ch_dqsmode <  DQS_MODE_TABLE_CNT; u8_2Ch_dqsmode ++)
			if(((u32_DqsMatch >> u8_2Ch_dqsmode) & 0x1) == 0x1)
			{
				break;
			}
	}

	NC_FCIE5SetInterface(1, au8_dqsmode[u8_2Ch_dqsmode], u8_max_delay_cell, 0);

	pNandDrv->tMinDDR.u8_ClkIdx = 0;
	pNandDrv->tMinDDR.u8_DqsMode = au8_dqsmode[u8_2Ch_dqsmode];
	pNandDrv->tMinDDR.u8_DelayCell = u8_max_delay_cell;
	pNandDrv->tMinDDR.u8_DdrTiming = 0;

	//nand_debug(UNFD_DEBUG_LEVEL,1,"ok, get TDDR timing: Latch Window:%X, RXDLL Phase Sel %X, 57h:%X\n",
	//				pNandDrv->u16_EmmcPllReg62_LatWin, pNandDrv->u16_EmmcPllReg09_PhaseSel, pNandDrv->u16_Reg57_RELatch);

	return UNFD_ST_SUCCESS;
}

//=========================================================
#endif // end of DDR_NAND_SUPPORT

U32 NC_ProbeReadSeq(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	volatile U16 u16_Reg;
	volatile U32 u32_Count;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	// ProbeReadSeq uses polling mode, so disable interupts here
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN, BIT_NC_JOB_END);
	#endif
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));

	if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR)  // if a page 512B
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_WAITRB);
	}
	else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	for (u32_Count=0; u32_Count < WAIT_PROBE_TIME; u32_Count++)
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		if ((u16_Reg & BIT_NC_JOB_END) == BIT_NC_JOB_END)
			break;

		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		nand_reset_WatchDog();
	}

	if (u32_Count < WAIT_PROBE_TIME)
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END); /*clear events*/
	else
	{
		nand_debug(UNFD_DEBUG_LEVEL_HIGH, 1, "Error: NC_ProbeReadSeq Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		NC_ReConfig();
		NC_ResetNandFlash();
		return UNFD_ST_ERR_R_TIMEOUT;
	}

	return UNFD_ST_SUCCESS;
}

U32 NC_EraseBlk(U32 u32_PhyRowIdx)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Ret;

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	//nand_debug(0,0,"e b:%Xh \n", u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits);

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_Erase_AdrCycle << 8) | CMD_0x60);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xD0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS << 8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_ERASE_TIME) == WAIT_ERASE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_EraseBlk Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_E_TIMEOUT);

		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif

		NC_CLR_DDR_MODE();

		return UNFD_ST_ERR_E_TIMEOUT;
	}

	#if defined(FCIE_WRITE_PROTECT_SUPPORT) && FCIE_WRITE_PROTECT_SUPPORT
	if((REG(NC_MIE_EVENT) & BIT_NC_SECURE_ALERT) == BIT_NC_SECURE_ALERT)
	{
		U32 u32_SecureRow;
		nand_debug(0, 1, "Hit Secure range 0x%X\n", u32_PhyRowIdx);
		u32_SecureRow = (REG(NC_SECURE_ERR_ADDR_H) << 16)|(REG(NC_SECURE_ERR_ADDR_L) & 0xFFFF);
		if(u32_SecureRow != u32_PhyRowIdx)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: Alert Address 0x%X!= 0x%X\n", u32_SecureRow, u32_PhyRowIdx);
		}

		NC_ReConfig();
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_SECURE_ALERT);
		u32_Ret = UNFD_ST_ERR_W_PROTECTED;
	}
	else
	#endif
		u32_Ret = NC_CheckEWStatus(OPTYPE_ERASE);

	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"erase blk:%Xh fail:%Xh \n",
			u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_Ret);

	NC_CLR_DDR_MODE();
	return u32_Ret;
}



U32 NC_CheckEWStatus(U8 u8_OpType)
{
	volatile U16 u16_Tmp;
	U32 u32_ErrCode = UNFD_ST_SUCCESS;

	REG_READ_UINT16(NC_ST_READ, u16_Tmp);

	if ((u16_Tmp & BIT_ST_READ_FAIL) == 1) { // if fail
		if (OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_FAIL;
		else if (OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_FAIL;

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_CheckEWStatus Fail, Nand St:%Xh, ErrCode:%Xh \r\n",
			   REG(NC_ST_READ), u32_ErrCode);
		return u32_ErrCode;
	} else if ((u16_Tmp & BIT_ST_READ_BUSYn) == 0) { // if busy
		if (OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_BUSY;
		else if (OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_BUSY;

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_CheckEWStatus Busy, Nand St:%Xh, ErrCode:%Xh \r\n",
			   REG(NC_ST_READ), u32_ErrCode);
		return u32_ErrCode;
	} else if ((u16_Tmp & BIT_ST_READ_PROTECTn) == 0) { // if protected
		if (OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_PROTECTED;
		else if (OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_PROTECTED;

		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_CheckEWStatus Protected, Nand St:%Xh, ErrCode:%Xh \r\n",
			   REG(NC_ST_READ), u32_ErrCode);
		return u32_ErrCode;
	}

	return u32_ErrCode;
}



U32 NC_ResetNandFlash(void)
{
	NAND_DRIVER *pNandDrv=(NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;
	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
	if(pNandDrv->u16_Reg58_DDRCtrl&BIT_DDR_ONFI)
	{
		// set commnad reg
		REG_WRITE_UINT16(NC_AUXREG_ADR, 0x08);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0xFC);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_REG8L);
	}
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xFF);
	#else
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xFF);
	#endif
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ResetNandFlash, ErrCode:%Xh \r\n", UNFD_ST_ERR_RST_TIMEOUT);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_RST_TIMEOUT;
	}

	NC_CLR_DDR_MODE();
	return UNFD_ST_SUCCESS;
}


void NC_SendCmdForLADebug(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;
	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0xDD);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK<<8)|CMD_REG8L);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	NC_CLR_DDR_MODE();
}

U32 NC_WaitCIFD_Event(U16 u16_WaitEvent, U32  u32_MicroSec)
{
	volatile U32 u32_Count;
	volatile U16 u16_Reg;

	for (u32_Count=0; u32_Count < u32_MicroSec; u32_Count++)
	{
		REG_READ_UINT16(NC_CIFD_EVENT, u16_Reg);
		if ((u16_Reg & u16_WaitEvent) == u16_WaitEvent)
			break;

		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
	}
	if (u32_Count >= u32_MicroSec)
	{
		REG_READ_UINT16(NC_CIFD_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "CIFD Event Timout %X\n", u16_Reg);
		if(Disable_Err_log == 0)
		{
			NC_DumpDebugBus();
			NC_DumpRegisters();
		}
	}
	return u32_Count;
}

U32 NC_WaitGetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt, U32 u32_MicroSec)
{
	if(u32_ByteCnt > NC_CIFD_WBUF_BYTE_CNT)
	{
		return UNFD_ST_ERR_INVAL_CIFD_CNT;
	}
	if (NC_WaitCIFD_Event(BIT_WBUF_FULL, u32_MicroSec)== (u32_MicroSec ))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
		REG_WRITE_UINT16(NC_PART_MODE, 0);
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
	}
	NC_GetCIFD(pu8_DataBuf , 0, u32_ByteCnt);

	REG_WRITE_UINT16(NC_CIFD_EVENT,BIT_WBUF_FULL);

	REG_WRITE_UINT16(NC_CIFD_EVENT, BIT_WBUF_EMPTY_TRI);

	return UNFD_ST_SUCCESS;
}

U32 NC_WaitSetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt)
{
	if(u32_ByteCnt > NC_CIFD_RBUF_BYTE_CNT)
	{
		return UNFD_ST_ERR_INVAL_CIFD_CNT;
	}
	NC_SetCIFD(pu8_DataBuf, 0, u32_ByteCnt);

	nand_hw_timer_delay(DELAY_100us_in_us);	//hw bug of 8bit macro ddr mode, if no delay cifd event would timeout

	REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);

	if (NC_WaitCIFD_Event(BIT_RBUF_EMPTY, WAIT_READ_TIME)== (WAIT_READ_TIME ))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
		return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
	}

	REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);

	return UNFD_ST_SUCCESS;
}


//extern void NC_Test_Reg717274_Set(void);
//extern void NC_Test_Reg717274_Get(void);
U32 NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	volatile U32 u32_Count;
	#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	volatile U32 u32_Err;
	#endif
	volatile U16 u16_Reg;

	#if defined(ENABLE_NAND_INTERRUPT_MODE) && ENABLE_NAND_INTERRUPT_MODE
	REG_READ_UINT16(NC_MIE_INT_EN, u16_Reg);
	if(u16_Reg & u16_WaitEvent)
	{
		u32_Err = nand_WaitCompleteIntr(u16_WaitEvent, u32_MicroSec);
		if( u32_Err == UNFD_ST_SUCCESS )
			return 0;
		else
		{
			NC_DumpDebugBus();
			NC_DumpRegisters();

			nand_die();
			return u32_MicroSec;
		}
	}
	#endif

	//PWROFF_Ex0();

	for (u32_Count=0; u32_Count < u32_MicroSec; u32_Count++)
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		if ((u16_Reg & u16_WaitEvent) == u16_WaitEvent)
			break;

		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		nand_reset_WatchDog();
		//PWROFF_Ex1();
	}

	if (u32_Count < u32_MicroSec)
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/
	else
	{
		if(Disable_Err_log == 0)
		{
			NC_DumpDebugBus();
			NC_DumpRegisters();
		}
	}

	return u32_Count;
}


void NC_SetCIFD_Const(U8 u8_Data, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
	U32 u32_i;
	volatile U16 u16_Tmp;

	if (u32_CIFDPos & 1) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u16_Tmp &= 0x00FF;
		u16_Tmp += u8_Data << 8;
		//nand_debug(1, 1, "0: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
	}

	for (u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++) {
		u16_Tmp = u8_Data + (u8_Data << 8);
		//nand_debug(1, 1, "1: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}

	if (u32_ByteCnt - (u32_i<<1)) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		u16_Tmp = (u16_Tmp & 0xFF00) + u8_Data;
		//nand_debug(1, 1, "2: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}
}

/*lint -save -e661 -e662 stops LINT complaining Possible access of out-of-bounds */
void NC_SetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
	U32 u32_i, u32_BufPos;
	volatile U16 u16_Tmp;

	if (u32_CIFDPos & 1) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u16_Tmp &= 0x00FF;
		u16_Tmp += pu8_Buf[0] << 8;
		//nand_debug(1, 1, "0: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
		u32_BufPos = 1;
	} else
		u32_BufPos = 0;

	for (u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++) {
		u16_Tmp = pu8_Buf[(u32_i<<1)+u32_BufPos] +
			  (pu8_Buf[(u32_i<<1)+u32_BufPos+1] << 8);
		//nand_debug(1, 1, "1: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}

	if (u32_ByteCnt - (u32_i<<1)) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		u16_Tmp = (u16_Tmp & 0xFF00) + pu8_Buf[(u32_i<<1)+u32_BufPos];
		//nand_debug(1, 1, "2: %X \r\n", u16_Tmp);
		REG_WRITE_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}
}

void NC_GetRBufCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
	U32 u32_i, u32_BufPos;
	U16 u16_Tmp;

	if (u32_CIFDPos & 1) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		pu8_Buf[0] = (U8)(u16_Tmp >> 8);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
		u32_BufPos = 1;
	} else
		u32_BufPos = 0;

	for (u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
		pu8_Buf[(u32_i<<1)+u32_BufPos+1] = (U8)(u16_Tmp>>8);
	}

	if (u32_ByteCnt - (u32_i<<1)) {
		REG_READ_UINT16(NC_RBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
	}

}

void NC_GetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
	U32 u32_i, u32_BufPos;
	U16 u16_Tmp;

	if (u32_CIFDPos & 1) {
		REG_READ_UINT16(NC_WBUF_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		pu8_Buf[0] = (U8)(u16_Tmp >> 8);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
		u32_BufPos = 1;
	} else
		u32_BufPos = 0;

	for (u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++) {
		REG_READ_UINT16(NC_WBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
		pu8_Buf[(u32_i<<1)+u32_BufPos+1] = (U8)(u16_Tmp>>8);
	}

	if (u32_ByteCnt - (u32_i<<1)) {
		REG_READ_UINT16(NC_WBUF_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
	}

}
/*lint -restore */

// Functions for Linux
U32 NC_ReadStatus(void)
{
	U8 u8Status = 0;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;
	#if (defined(IF_FCIE_SHARE_IP)&&IF_FCIE_SHARE_IP)

	#endif
	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS<<8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if(NC_WaitComplete(BIT_NC_JOB_END, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadStatus Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		NC_CLR_DDR_MODE();
		#if (defined(IF_FCIE_SHARE_IP)&&IF_FCIE_SHARE_IP)

		#endif
		return UNFD_ST_ERR_R_TIMEOUT;
	}

	u8Status = (U8)REG(NC_ST_READ);
	NC_SetCIFD(&u8Status, 0, 1);

	NC_CLR_DDR_MODE();
	#if (defined(IF_FCIE_SHARE_IP)&&IF_FCIE_SHARE_IP)

	#endif

	return UNFD_ST_SUCCESS;
}

void NC_CheckECC(int *s32ECCStatus)
{
	U16 u16RegValue = 0;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	REG_READ_UINT16(NC_ECC_STAT0, u16RegValue);

	if( u16RegValue & BIT_NC_ECC_FAIL ) //Uncorrectable ECC error
		*s32ECCStatus = -1;
	else
	{
		*s32ECCStatus = NC_GetECCBits();
		if (*s32ECCStatus < pNandDrv->u16_BitflipThreshold && *s32ECCStatus >= 0)
		{
			*s32ECCStatus = 0;
		}
	}
	if(gu8_ReadRetryHappens == 1)
	{
		gu8_ReadRetryHappens = 0;
		*s32ECCStatus = pNandDrv->u16_ECCCorretableBit + 1;;
	}
}


/*--------------------------------------------------
	u32_PlaneRowIdx : physical row address of plane1
  --------------------------------------------------*/
U32 NC_EraseBlk2P(U32 u32_PlaneRowIdx)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Ret;
	U16 u16_Tmp;
	U32 u32_PlaneRowIdx2;


	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE();

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	//nand_debug(0,0,"e B:%03X \n", u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits);

	u32_PlaneRowIdx2 = u32_PlaneRowIdx + pNandDrv->u16_BlkPageCnt;
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx2 & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx2 >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	u16_Tmp = (pNandDrv->u8_OpCode_Erase_AdrCycle &~ OP_ADR_SET_0) | OP_ADR_SET_1;

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_Erase_AdrCycle << 8) | CMD_0x60);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (u16_Tmp << 8) | CMD_0x60);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xD0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS << 8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_ERASE_TIME) == WAIT_ERASE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_EraseBlk2Plane Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_E_TIMEOUT);

		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif

		NC_CLR_DDR_MODE();

		return UNFD_ST_ERR_E_TIMEOUT;
	}

	u32_Ret = NC_CheckEWStatus(OPTYPE_ERASE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"erase blk:%Xh fail:%Xh \n",
			u32_PlaneRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_Ret);

	NC_CLR_DDR_MODE();

	return u32_Ret;
}

/*--------------------------------------------------
	u32_PlaneRowIdx : physical row address of plane1
	pu8_DataBuf   : data buffer of page data
					page1(plane1)+page1(plane2)+page2(plane1)+page2(plane2)
					...+pageN(plane1)+pageN(plane2)
	pu8_SpareBuf  : spare buffer of page spare data
					only 1 spare buffer can be set
	u32_PageCnt   : page count, used by repeat function
  --------------------------------------------------*/
U32 NC_WritePages2P
(
	U32 u32_PlaneRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt
)
{
	U16 u16_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr=0;
	U32 u32_Ret;
	U32 u32_PlaneRowIdx2;

	//nand_debug(0,1,"%X %X \n", u32_PhyRowIdx, u32_PageCnt);
	#if CHECK_HAL_PARAM
	// can not cross block
	if ((u32_PlaneRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt >
		pNandDrv->u16_BlkPageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_WritePages2Plane, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_W_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_W_INVALID_PARAM;
	}
	#endif

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PlaneRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, (2 * pNandDrv->u16_PageByteCnt) * u32_PageCnt, WRITE_TO_NAND);
	u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, WRITE_TO_NAND);

	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	REG_WRITE_UINT16(NC_WDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	u32_PlaneRowIdx2 = u32_PlaneRowIdx + pNandDrv->u16_BlkPageCnt;
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx2 & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PlaneRowIdx2 >> 16);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	// set commnad reg
	REG_WRITE_UINT16(NC_AUXREG_ADR, 0x08);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x81<<8 | 0x11);

	u16_Tmp = (pNandDrv->u8_OpCode_RW_AdrCycle &~ OP_ADR_SET_0) | OP_ADR_SET_1;
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8L << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8H << 8) | ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DOUT << 8) | u16_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x10);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS << 8)| CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_REPEAT);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);
	//nand_debug(1, 1, "W Rpt Cnt: %Xh \r\n", u16_PageCnt-1);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	//while(1)  nand_reset_WatchDog();
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME*u32_PageCnt) == WAIT_WRITE_TIME*u32_PageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_WritePages2Plane Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();

		#else
		//nand_stop();
		#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	u32_Ret = NC_CheckEWStatus(OPTYPE_WRITE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"write blk:%Xh p:%Xh fail:%Xh \n",
			u32_PlaneRowIdx>>pNandDrv->u8_BlkPageCntBits,
			u32_PlaneRowIdx&pNandDrv->u16_BlkPageCntMask, u32_Ret);

	NC_CLR_DDR_MODE();

	return u32_Ret;
}

// can not cross block
U32 NC_ADMAWritePages(	U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt )
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr=0;
	U32 u32_Ret;

	#if CHECK_HAL_PARAM
	// can not cross block
	if ((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt > pNandDrv->u16_BlkPageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_WritePages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_W_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_W_INVALID_PARAM;
	}
	#endif

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();
	NC_SET_DDR_MODE(); // to turn on ONFI clk

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	/*
		Main Data DMA Address is the descriptor table buffer address
		FCIE read DMA descriptor from this address, the direction must be read.
	*/
	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt, READ_FROM_NAND);
	u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, WRITE_TO_NAND);

	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_WSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8) | ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_CHKSTATUS);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);
	//nand_debug(1, 1, "W Rpt Cnt: %Xh \r\n", u16_PageCnt-1);

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START|BIT_NC_ADMA_EN);

	//while(1)  nand_reset_WatchDog();
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME*u32_PageCnt) == WAIT_WRITE_TIME*u32_PageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_WritePages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		//nand_stop();
		#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	u32_Ret = NC_CheckEWStatus(OPTYPE_WRITE);
	if(UNFD_ST_SUCCESS != u32_Ret)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"write blk:%Xh p:%Xh fail:%Xh \n",
			u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits,
			u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask, u32_Ret);

	NC_CLR_DDR_MODE();

	return u32_Ret;
}

U32 NC_ADMAReadPages(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt, U8 u8_Zdec)
{
	U16 u16_Tmp=0;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U32 u32_SpareDMAAddr = 0;
	U8  u8_RetryCnt_Events=0, u8_RetryCnt_ECCFail=0, u8_RetryDqs=0;
	#if (0 == IF_IP_VERIFY)
	U8  u8_MaxReadRetryCount = NAND_TIMEOUT_RETRY_CNT;
	#endif

	#if CHECK_HAL_PARAM
	// can not cross block
	if ((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt >
	  pNandDrv->u16_BlkPageCnt)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "ERROR: NC_ReadPages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_R_INVALID_PARAM);
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	#endif

	NC_PAD_SWITCH(pNandDrv->u8_PadMode);
	NC_RECONFIG();

	#if 0==IF_IP_VERIFY
	nand_redo_read:
	#endif

	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	#if defined(FCIE_LFSR) && FCIE_LFSR
	REG_CLR_BITS_UINT16(NC_LFSR_CTRL, BIT_SEL_PAGE_MASK);
	REG_SET_BITS_UINT16(NC_LFSR_CTRL,
		((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) & BIT_SEL_PAGE_MASK>>BIT_SEL_PAGE_SHIFT)<<BIT_SEL_PAGE_SHIFT);
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u16_PageByteCnt * u32_PageCnt, READ_FROM_NAND );
	u32_SpareDMAAddr = nand_translate_Spare_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt, READ_FROM_NAND );

	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RSPARE_DMA_ADR1, u32_SpareDMAAddr >>16);

	REG_WRITE_UINT16(NC_RDATA_DMA_ADR0, u32_DataDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_RDATA_DMA_ADR1, u32_DataDMAAddr >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));

	if (pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR)
	{ // if a page 512B
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_REPEAT);
	}
	else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_SER_DIN);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt - 1);

	// check & set - patch for MIU busy case
	// slow down FCIE clock to wait for MIU service,
	// enhance the retry (more reliable) for error handling.
	if(0!=u8_RetryCnt_Events || 0!=u8_RetryCnt_ECCFail || 0!=u8_RetryDqs)
	{
		#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
		if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
		{
			#if defined( DDR_NAND_SUPPORT_RETRY_DQS) && DDR_NAND_SUPPORT_RETRY_DQS
			if(0!=u8_RetryDqs)
			{
				nand_clock_setting(pNandDrv->u32_minClk);
				NC_FCIE5SetInterface(1, pNandDrv->tMinDDR.u8_DqsMode, pNandDrv->tMinDDR.u8_DelayCell, pNandDrv->tMinDDR.u8_DdrTiming);
			}
			#else
			nand_clock_setting(pNandDrv->u32_minClk);
			NC_FCIE5SetInterface(1, pNandDrv->tMinDDR.u8_DqsMode, pNandDrv->tMinDDR.u8_DelayCell, pNandDrv->tMinDDR.u8_DdrTiming);
			#endif
		}
		else
		#endif
		{
			if(pNandDrv->u8_RequireRandomizer == 0)
			{
				nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
				REG_WRITE_UINT16(NC_WIDTH, 0);
				REG_WRITE_UINT16(NC_LATCH_DATA, (REG(NC_LATCH_DATA) & ~BIT_NC_LATCH_DATA_MASK));
			}
		}
		//nand_printf("check & set - patch for MIU busy case \n");
	}

	if(u8_Zdec)
		REG_WRITE_UINT16(NC_CTRL,  BIT_NC_JOB_START|BIT_NC_ADMA_EN|BIT_NC_ZDEC_EN);
	else
		REG_WRITE_UINT16(NC_CTRL,  BIT_NC_JOB_START|BIT_NC_ADMA_EN);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME * u32_PageCnt) ==
	  (WAIT_READ_TIME * u32_PageCnt ))
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadPages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT);
		#if (0==IF_IP_VERIFY)
		NC_ReConfig();
		NC_ResetNandFlash();
		if( ++u8_RetryCnt_Events < NAND_TIMEOUT_RETRY_CNT )
			  goto nand_redo_read;
		else
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "\033[31mSoft reset over %d times\n, stop!\033[m\n", NAND_TIMEOUT_RETRY_CNT);

			//If soft reset still can not solve this problem, show an alert and return a error
			// restore the normal setting before return
			#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
			if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
			{
				nand_clock_setting(pNandDrv->u32_Clk);
				NC_FCIE5SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);
			}
			else
			#endif
			{
				nand_clock_setting(pNandDrv->u32_Clk);
				REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
				REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
			}
			NC_CLR_DDR_MODE();

			return UNFD_ST_ERR_R_TIMEOUT;
		}
		#else
		//nand_stop();
		#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_R_TIMEOUT;
	}

	// restore - patch for MIU busy case
	// for retry ok, restore the normal setting
	if(0!=u8_RetryCnt_Events || 0!=u8_RetryCnt_ECCFail || 0!=u8_RetryDqs)
	{
		#if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
		if(pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM)
		{
			#if defined( DDR_NAND_SUPPORT_RETRY_DQS) && DDR_NAND_SUPPORT_RETRY_DQS
			if(0!=u8_RetryDqs)
			{
				nand_clock_setting(pNandDrv->u32_Clk);
				NC_FCIE5SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);
			}
			#else
			nand_clock_setting(pNandDrv->u32_Clk);
			NC_FCIE5SetInterface(1, pNandDrv->tDefaultDDR.u8_DqsMode, pNandDrv->tDefaultDDR.u8_DelayCell, pNandDrv->tDefaultDDR.u8_DdrTiming);
			#endif
		}
		else
		#endif
		{
			nand_clock_setting(pNandDrv->u32_Clk);
			REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
			REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);
		}
		//nand_printf("restore - patch for MIU busy case \n");
	}

	//-----------------------------------

	#if defined(NAND_DRV_TV_MBOOT)&&NAND_DRV_TV_MBOOT
	#if defined(CONFIG_MIPS32)
		Chip_Read_Memory();
	#endif
	#endif

	// Check ECC
	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if (u16_Tmp & BIT_NC_ECC_FAIL)
	{
		#if (0 == IF_IP_VERIFY)
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: NC_ReadPages ECC Fail, Reg51:%04Xh ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "Reg52:%04Xh ", u16_Tmp);
		REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "Reg53:%04Xh \r\n", u16_Tmp);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Blk: %Xh  Page: %X\r\n",
				u32_PhyRowIdx>>pNandDrv->u8_BlkPageCntBits, u32_PhyRowIdx&pNandDrv->u16_BlkPageCntMask);
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 1, "ERROR, NAND, RETRY_READ_ECC_FAIL over %d times \n", u8_MaxReadRetryCount);
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_ECC_FAIL;
		#else
		NC_CLR_DDR_MODE();
		if (pNandDrv->u8_IfECCTesting)
			return UNFD_ST_SUCCESS;
		else
			return UNFD_ST_ERR_ECC_FAIL;
		#endif
	}

	NC_CLR_DDR_MODE();

	return UNFD_ST_SUCCESS;
}

#endif // NC_SEL_FCIE3
