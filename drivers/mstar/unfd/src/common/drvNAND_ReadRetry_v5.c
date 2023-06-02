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



//========================================================
// HAL misc. function definitions
//========================================================
#if (defined(NC_SEL_FCIE5) && (NC_SEL_FCIE5))
U32  NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec);
U32 NC_WaitCIFD_Event(U16 u16_WaitEvent, U32	u32_MicroSec);

U32 NC_WaitGetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt, U32 u32_MicroSec);
U32 NC_WaitSetCIFD(U8 * pu8_DataBuf, U32 u32_ByteCnt);



  #define OPTYPE_ERASE        1
  #define OPTYPE_WRITE        2
U32 NC_Toshiba_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_Samsung_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_Micron_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_SetReadRetryRegValue(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_GetRegDefaultValueDefault(void);
U32 NC_SendReadRetryCmdDefault(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_Hynix_20nm32GBGetRegDefaultValue(void);
U32 NC_Hynix_20nm32GBReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);
U32 NC_Hynix_20nm64GBAGetRegDefaultValue(void);
U32 NC_Hynix_20nm64GBAReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);

U32 NC_Hynix_GetRegDefaultValue(void);
U32 NC_Hynix_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault);


U32 NC_SendCustCmd
(
    U8* pu8_CmdType,
    U8* pu8_CmdValue,
    U32* pu32_AddrRow,
    U32* pu32_AddrCol,
    U8 *pu8_DataBuf, U8 *pu8_SpareBuf,
    U8 u8_CmdCount, U8 u8_CacheOp, U8 u8_plane, U32 u32_PageCnt
);


typedef struct NandReadRetryHal_t
{
	U32 (*NC_GetRegDefaultValue)(void);
	U32(*NC_SendReadRetryCmd)(U8 u8_RetryIndex, U8 u8_SetToDefault);
}NandReadRetryHal_t;

static NandReadRetryHal_t NandReadRetryHal;

void NC_SetupReadRetryCmd(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	nand_debug(UNFD_DEBUG_LEVEL_WARNING, 1, "Using build-in Read Retry Sequence for Type %d ", pNandDrv->u8_ReadRetryType);

	if(pNandDrv->au8_ID[0] == 0x98)
	{
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "Toshiba MLC\n");
		//Toshiba no need to Get Reg Default Value when initialization
		NandReadRetryHal.NC_GetRegDefaultValue = NC_GetRegDefaultValueDefault;
		switch(pNandDrv->u8_ReadRetryType)
		{
			case 0:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = TSB_READRETRY_TIME;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Toshiba_ReadRetrySequence;
				break;
			default:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = TSB_READRETRY_TIME;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Toshiba_ReadRetrySequence;
				break;
		}
	}
	else if(pNandDrv->au8_ID[0] == 0xEC)
	{
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "Samsung MLC\n");
		//Samsung no need to Get Reg Default Value when initialization
		NandReadRetryHal.NC_GetRegDefaultValue = NC_GetRegDefaultValueDefault;
		switch(pNandDrv->u8_ReadRetryType)
		{
			case 0:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = SAMSUNG_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Samsung_ReadRetrySequence;
				break;
			default:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = SAMSUNG_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Samsung_ReadRetrySequence;
				break;
		}
	}
	else if(pNandDrv->au8_ID[0] == 0x2C)
	{
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "Micron MLC\n");
		//Micron no need to Get Reg Default Value when initialization
		NandReadRetryHal.NC_GetRegDefaultValue = NC_GetRegDefaultValueDefault;
		switch(pNandDrv->u8_ReadRetryType)
		{
			case 0:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = 4 - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Micron_ReadRetrySequence;
				break;
			case 1:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = MCIRON_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Micron_ReadRetrySequence;
				break;
			default:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = MCIRON_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Micron_ReadRetrySequence;
				break;
		}

        #if defined(DDR_NAND_SUPPORT) && DDR_NAND_SUPPORT
        if(pNandDrv->u8_PadMode == NAND_PAD_BYPASS_MODE)
        {
            U32 u32_RetVal;
            u32_RetVal = NC_SetONFISyncMode(0);
            if(UNFD_ST_SUCCESS != u32_RetVal)
            {
                nand_debug(UNFD_DEBUG_LEVEL_ERROR,1,"ERROR, NAND SetFeature Timing Mode, Err Code:%08lXh \n", u32_RetVal);
            }
        }
        #endif
	}
	else if(pNandDrv->au8_ID[0] == 0xAD)
	{
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "Hynix MLC\n");

		switch(pNandDrv->u8_ReadRetryType)
		{
			case 0:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = HYNIX20nm32GBMLC_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Hynix_20nm32GBReadRetrySequence;
				NandReadRetryHal.NC_GetRegDefaultValue = NC_Hynix_20nm32GBGetRegDefaultValue;
				break;
			case 1:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = HYNIX20nm32GBMLC_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Hynix_20nm64GBAReadRetrySequence;
				NandReadRetryHal.NC_GetRegDefaultValue = NC_Hynix_20nm64GBAGetRegDefaultValue;
				break;

			default:
				pNandDrv->ReadRetry_t.u8_MaxRetryTime = HYNIX20nm32GBMLC_READRETRY_TIME - 1;
				NandReadRetryHal.NC_SendReadRetryCmd = NC_Hynix_ReadRetrySequence;
				NandReadRetryHal.NC_GetRegDefaultValue = NC_Hynix_GetRegDefaultValue;
				break;
		}
	}
	else
	{
		nand_debug(UNFD_DEBUG_LEVEL_WARNING, 0, "UNKOWN MLC\n");

		NandReadRetryHal.NC_GetRegDefaultValue = NC_GetRegDefaultValueDefault;
		NandReadRetryHal.NC_SendReadRetryCmd = NC_SendReadRetryCmdDefault;
	}
}

U32 NC_SendReadRetryCmd(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	return NandReadRetryHal.NC_SendReadRetryCmd(u8_RetryIndex, u8_SetToDefault);
}

U32 NC_GetRegDefaultValue(void)
{
	return NandReadRetryHal.NC_GetRegDefaultValue();
}


U32 NC_GetRegDefaultValueDefault(void)
{
	return UNFD_ST_SUCCESS;
}
U32 NC_SendReadRetryCmdDefault(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	return UNFD_ST_SUCCESS;
}
//==========================TOSHIBA==============================
U8 au8_TSBReadRetryValue[TSB_READRETRY_TIME] = {
    0x00, 0x04, 0x7C, 0x78, 0x74, 0x08
};

U8 au8_TSBReadRetryRegister[TSB_REGISTER_NUMBER] = {
    0x4, 0x5, 0x6, 0x7
};

U32 NC_Toshiba_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	U8  u8_idx;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();

	if(u8_SetToDefault)
	{
		return NC_ResetNandFlash();
	}

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	NC_ReConfig();

	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
	REG_WRITE_UINT16(NC_WIDTH, 0);
	REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT0|BIT1|BIT2);

	if(u8_RetryIndex == 0)
	{
		//send pre condition
		REG_WRITE_UINT16(NC_AUXREG_ADR, 0x08);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0xC5<<8 | 0x5C);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8H << 8) | CMD_REG8L);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);

		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Pre Condition Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
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
	}


	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, 0x08);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x55 << 8 | 0x55);

	for(u8_idx = 0; u8_idx < TSB_REGISTER_NUMBER; u8_idx ++)
	{

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		REG_WRITE_UINT16(NC_AUXREG_DAT, au8_TSBReadRetryRegister[u8_idx]);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ((OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) | (CMD_REG8L));
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DOUT);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

		NC_SetCIFD(&au8_TSBReadRetryValue[u8_RetryIndex], 0, 1);

		REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);

		if (NC_WaitCIFD_Event(BIT_RBUF_EMPTY, WAIT_READ_TIME)== (WAIT_READ_TIME ))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}

		REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);


		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout RetryIdx %d, ErrCode:%Xh \r\n", u8_RetryIndex, UNFD_ST_ERR_W_TIMEOUT);
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
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, 0x08);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x5D << 8 | 0x26);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8H << 8) | (CMD_REG8L));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout RetryIdx %d, ErrCode:%Xh \r\n", u8_RetryIndex, UNFD_ST_ERR_W_TIMEOUT);
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
	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

	return UNFD_ST_SUCCESS;
}

//==========================SAMSUNG==============================

//21nm MLC Read Retry Sequence
U8 au8_SSReadRetryRegister[SAMSUNG_REGISTER_NUMBER] = {
	0xA7, 0xA4, 0xA5, 0xA6
};

U8 au8_SSReadRetryValue[SAMSUNG_READRETRY_TIME][SAMSUNG_REGISTER_NUMBER] = {
		{0x00, 0x00, 0x00, 0x00},
		{0x05, 0x0A, 0x00, 0x00},
		{0x28, 0x00, 0xEC, 0xD8},
		{0xED, 0xF5, 0xED, 0xE6},
		{0x0A, 0x0F, 0x05, 0x00},
		{0x0F, 0x0A, 0xFB, 0xEC},
		{0xE8, 0xEF, 0xE8, 0xDC},
		{0xF1, 0xFB, 0xFE, 0xF0},
		{0x0A, 0x00, 0xFB, 0xEC},
		{0xD0, 0xE2, 0xD0, 0xC2},
		{0x14, 0x0F, 0xFB, 0xEC},
		{0xE8, 0xFB, 0xE8, 0xDC},
		{0x1E, 0x14, 0xFB, 0xEC},
		{0xFB, 0xFF, 0xFB, 0xF8},
		{0x07, 0x0C, 0x02, 0x00}
};

U32 NC_Samsung_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	U8  u8_idx, u8_ValueIdx;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;
	if(u8_SetToDefault)
	{
		u8_ValueIdx = 0;
	}
	else
	{
		u8_ValueIdx = u8_RetryIndex + 1;
	}

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	NC_ReConfig();
	// data go through CIFD
	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0xA1 << 8 | 0xA1);

	for(u8_idx = 0; u8_idx < SAMSUNG_REGISTER_NUMBER; u8_idx ++)
	{
		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_RAN_DOUT<< 8) | (CMD_REG8L));
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_BREAK);



		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 3);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

		NC_SetCIFD_Const(0x00, 0, 1);
		NC_SetCIFD(&au8_SSReadRetryRegister[u8_idx], 1, 1);
		NC_SetCIFD(&au8_SSReadRetryValue[u8_ValueIdx][u8_idx], 2, 1);

		REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);

		//wait for CIFD Event
		if (NC_WaitCIFD_Event(BIT_RBUF_EMPTY, WAIT_READ_TIME)== (WAIT_READ_TIME ))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}

		REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);

		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout RetryIdx %d, ErrCode:%Xh \r\n", u8_RetryIndex, UNFD_ST_ERR_W_TIMEOUT);
			#if 0==IF_IP_VERIFY
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			NC_ResetNandFlash();
			#else
			//nand_stop();
			#endif
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_W_TIMEOUT; // timeout
		}
		nand_hw_timer_delay(30);		//delay for 30us
	}
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_Config();

	return UNFD_ST_SUCCESS;
}

//==========================MCIRON==============================


U8 au8_MicronReadRetryValue[MCIRON_READRETRY_TIME][MICRON_REIGSTER_NUMBER] ={
	{0,0,0,0},
	{1,0,0,0},
	{2,0,0,0},
	{3,0,0,0},
	{4,0,0,0},
	{5,0,0,0},
	{6,0,0,0},
	{7,0,0,0}
};

U32 NC_Micron_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	U8   u8_ValueIdx;
	#if 0==IF_IP_VERIFY
	volatile U16 u16_Reg;
	#endif
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
    U8 au8_buf[8];
    U8 u8_DDR, u8_i;
	pNandDrv = pNandDrv;	//fix warning

    u8_DDR = (pNandDrv->u16_Reg58_DDRCtrl & BIT_DDR_MASM) ? 1 : 0;

	if(u8_SetToDefault)
	{
		u8_ValueIdx = 0;
	}
	else
	{
		u8_ValueIdx = u8_RetryIndex + 1;
	}

    for(u8_i =0; u8_i < 4; u8_i ++)
    {
        au8_buf[(3 - u8_i) * 2 + 1] = au8_buf[(3 - u8_i) * 2] = au8_MicronReadRetryValue[u8_ValueIdx][3 - u8_i];
    }

	// data go through CIFD

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);
	NC_ReConfig();

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_SET_DDR_MODE();

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0xEF);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (0x89<<8)|0x89);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ADR_C2T1S0 << 8) | CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | ACT_RAN_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);


	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, MICRON_REIGSTER_NUMBER<<u8_DDR);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);	/*offset 0*/

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

    if(u8_DDR)
        NC_WaitSetCIFD(&au8_buf[0], MICRON_REIGSTER_NUMBER<<u8_DDR);
    else
		NC_WaitSetCIFD(&au8_MicronReadRetryValue[u8_ValueIdx][0], MICRON_REIGSTER_NUMBER);

	REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);
	REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);


	if (NC_WaitComplete(BIT_NC_JOB_END, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error! timeout!\r\n");

		#if 0==IF_IP_VERIFY
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_MIE_EVENT: %Xh \r\n", u16_Reg);
		REG_READ_UINT16(NC_CTRL, u16_Reg);
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "NC_CTRL: %Xh \r\n", u16_Reg);
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
		#else
		nand_stop();
		#endif
		return UNFD_ST_ERR_SET_FEATURE_TIMEOUT;
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	NC_CLR_DDR_MODE();
	return UNFD_ST_SUCCESS;	// ok
}

//==========================Hynix==============================

U32 NC_Hynix_GetRegDefaultValue(void)
{
	return UNFD_ST_SUCCESS;
}
U32 NC_Hynix_ReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	return UNFD_ST_SUCCESS;
}

U32 NC_Hynix_GetReadRetryParam(U8* pu8_ReadRetryRegister, int RegisterCount)
{
	U8  u8_idx;
//	U8	au8_ParamValue[8];
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;

	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
	REG_WRITE_UINT16(NC_WIDTH, 0);
	REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT0|BIT1|BIT2);

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x16<<8 | 0x37);

	for(u8_idx = 0 ; u8_idx < RegisterCount; u8_idx ++)
	{
		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		REG_WRITE_UINT16(NC_AUXREG_DAT, pu8_ReadRetryRegister[u8_idx]);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ((OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) | CMD_REG8L);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) |(ACT_RAN_DIN));

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 2);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

		if (NC_WaitCIFD_Event(BIT_WBUF_FULL, WAIT_READ_TIME)== (WAIT_READ_TIME ))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_WRITE_UINT16(NC_PART_MODE, 0);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}
		//NC_GetCIFD(pu8_DataBuf + (u16_j << 6), 0, u16_Tmp);
		REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_WBUF_FULL);
		REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_WBUF_EMPTY_TRI);

		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 2, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			NC_ResetNandFlash();
		#else
			//nand_stop();
		#endif
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_RST_TIMEOUT; // timeout
		}
		//NC_GetCIFD(&au8_ParamValue[u8_idx], 0, 1);
		//nand_debug(UNFD_DEBUG_LEVEL_ERROR, 0, "0x%Xh ", au8_ParamValue[u8_idx]);
	}

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	NC_Config();
	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

	return UNFD_ST_SUCCESS;
}

//For 20nm 32Gb MLC

U8 au8_Hynix20nm32GBMLC_ReadRetryRegister[HYNIX20nm32GBMLC_REGISTER_NUMBER] = {
    0xB0,0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7
};

U8 au8_Hynix20nm32GBMLC_ReadRetryValue[HYNIX20nm32GBMLC_READRETRY_TIME][HYNIX20nm32GBMLC_REGISTER_NUMBER];

U8 au8_Hynix20nm32GBMLC_RRIOTPWout[2] = {
	0x00, 0x4D
};

U8	gu8_CurIdx = 0;
U8	gu8_DefaultIdx = 0;

U32 NC_Hynix_20nm32GBGetRegDefaultValue(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Row = 0x000200;
	U32 u32_Err;
	U16 u16_i, u16_j;
	U8	*pu8_DataBuf = pNandDrv->PlatCtx_t.pu8_PageDataBuf;
	U8	*pu8_RRTable1, *pu8_RRInvTable1, *pu8_RRTable2, *pu8_RRInvTable2;

	//Read R-R Table from OTP BLOCK and Check for inverse version
	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT,0xAE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_DAT,0xB0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x16<<8 | 0x36);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG9);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x04<<8 | 0x17);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREGA);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x19<<8 | 0x19);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( (OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) |CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_RAN_DOUT);
//	NC_SetCIFD(&au8_Hynix20nm32GBMLC_RRIOTPWout[0], 0, 1);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	NC_WaitSetCIFD(&au8_Hynix20nm32GBMLC_RRIOTPWout[0], 1);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 1, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
	#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
	#else
		//nand_stop();
	#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_RST_TIMEOUT; // timeout
	}


	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_RAN_DOUT << 8) | (OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_1));
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( CMD_REG9L << 8) |CMD_REG8H);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( CMD_REGAL << 8) |CMD_REG9H);

	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);

	NC_WaitSetCIFD(&au8_Hynix20nm32GBMLC_RRIOTPWout[1],  1);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 2, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
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

	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	u32_Err = NC_Read_RandomIn(u32_Row, 0, pu8_DataBuf, 260);
	if (u32_Err !=UNFD_ST_SUCCESS)
	{
		return u32_Err;
	}

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x38<<8 | 0x38);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 3, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
	#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
	#else
		//nand_stop();
	#endif
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_RST_TIMEOUT; // timeout
	}
	NC_Config();

//	dump_mem(pu8_DataBuf , 260);
	//parse Read Retry Value
	if(pu8_DataBuf[0] != HYNIX20nm32GBMLC_READRETRY_TIME || pu8_DataBuf[1] != HYNIX20nm32GBMLC_REGISTER_NUMBER)
	{
		 nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Mismatch R-R Count %X:%X or R-R Reg Count %X:%X\n", pu8_DataBuf[0], HYNIX20nm32GBMLC_READRETRY_TIME,
				pu8_DataBuf[1], HYNIX20nm32GBMLC_REGISTER_NUMBER);
		 pNandDrv->u8_RequireReadRetry = 0;
		return UNFD_ST_ERR_SETUP_READ_RETRY_FAIL;
	}
	pu8_RRTable1 = pu8_DataBuf + 2;
	pu8_RRInvTable1 = pu8_RRTable1 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER;
	pu8_RRTable2 = pu8_DataBuf + 2 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER * 2;
	pu8_RRInvTable2 = pu8_RRTable2 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER;

	for(u16_i = 0; u16_i < HYNIX20nm32GBMLC_READRETRY_TIME; u16_i ++)
	{
		for(u16_j = 0; u16_j < HYNIX20nm32GBMLC_REGISTER_NUMBER; u16_j ++)
		{
			if((pu8_RRTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ^
				pu8_RRInvTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j]) ==  0xFF)
			{
				au8_Hynix20nm32GBMLC_ReadRetryValue[u16_i][u16_j] = pu8_RRTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ;
			}
			else
			{
				if((pu8_RRTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ^
				pu8_RRInvTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j]) ==  0xFF)
				{
					au8_Hynix20nm32GBMLC_ReadRetryValue[u16_i][u16_j] = pu8_RRTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ;
				}
				else
				{

					 nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Fail Parsing R-R value %X:%X \n", u16_i, u16_j);
					 pNandDrv->u8_RequireReadRetry = 0;
					return UNFD_ST_ERR_SETUP_READ_RETRY_FAIL;
				}
			}
			nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "0x%02X, ", au8_Hynix20nm32GBMLC_ReadRetryValue[u16_i][u16_j] );
		}
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "\n");
	}
	#if defined(IF_SPARE_AREA_DMA) &&IF_SPARE_AREA_DMA
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);
	#endif
	return UNFD_ST_SUCCESS;
}

U32 NC_Hynix_20nm32GBReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	U8  u8_idx, u8_ValueIdx;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;

	NC_Hynix_GetReadRetryParam(au8_Hynix20nm32GBMLC_ReadRetryRegister, HYNIX20nm32GBMLC_REGISTER_NUMBER);
	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
	REG_WRITE_UINT16(NC_WIDTH, 0);
	REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT0|BIT1|BIT2);

	if(u8_SetToDefault)
	{
		gu8_CurIdx = u8_ValueIdx = gu8_DefaultIdx;
	}
	else
	{
		if(u8_RetryIndex == 0)
		{
			gu8_DefaultIdx = gu8_CurIdx;
		}
		gu8_CurIdx ++;
		if(gu8_CurIdx == 8)
			gu8_CurIdx = 0;

		u8_ValueIdx = gu8_CurIdx;
	}

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x16<<8 | 0x36);

	for(u8_idx = 0 ; u8_idx < HYNIX20nm32GBMLC_REGISTER_NUMBER; u8_idx ++)
	{
		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		REG_WRITE_UINT16(NC_AUXREG_DAT,au8_Hynix20nm32GBMLC_ReadRetryRegister[u8_idx]);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ((OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) | CMD_REG8L);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8H << 8) |(ACT_RAN_DOUT));
		REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);


		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

		NC_SetCIFD(&au8_Hynix20nm32GBMLC_ReadRetryValue[u8_ValueIdx][u8_idx], 0, 1);
		NC_SetCIFD(&au8_Hynix20nm32GBMLC_ReadRetryValue[u8_ValueIdx][u8_idx], 1, 1);
		REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);

		if (NC_WaitCIFD_Event(BIT_RBUF_EMPTY, WAIT_READ_TIME)== (WAIT_READ_TIME ))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
            NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}

		REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);


		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 2, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			NC_ResetNandFlash();
		#else
			//nand_stop();
		#endif
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_RST_TIMEOUT; // timeout
		}
	}

	NC_Config();
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

	return UNFD_ST_SUCCESS;
}


//For 20nm 64Gb A die MLC

U8 au8_Hynix20nm64GBMLCA_ReadRetryRegister[HYNIX20nm32GBMLC_REGISTER_NUMBER] = {
    0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF
};

U8 au8_Hynix20nm64GBMLCA_ReadRetryValue[HYNIX20nm32GBMLC_READRETRY_TIME][HYNIX20nm32GBMLC_REGISTER_NUMBER];

U8 au8_Hynix20nm64GBMLCA_RRIOTPWout[2] = {
	0x40, 0x4D
};

U32 NC_Hynix_20nm64GBAGetRegDefaultValue(void)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	U32 u32_Row = 0x000200;
	U32 u32_Err;
	U16 u16_i, u16_j;
	U8	*pu8_DataBuf = pNandDrv->PlatCtx_t.pu8_PageDataBuf;
	U8	*pu8_RRTable1, *pu8_RRInvTable1, *pu8_RRTable2, *pu8_RRInvTable2;

	//Read R-R Table from OTP BLOCK and Check for inverse version
	// data go through CIFD

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT,0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_DAT,0xCC);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x16<<8 | 0x36);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG9);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x04<<8 | 0x17);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREGA);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x19<<8 | 0x19);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( (OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) |CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_RAN_DOUT);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	NC_WaitSetCIFD(&au8_Hynix20nm32GBMLC_RRIOTPWout[0], 1);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 1, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
	#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
	#else
		//nand_stop();
	#endif

		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_RST_TIMEOUT; // timeout
	}

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_RAN_DOUT << 8) | (OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_1));
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( CMD_REG9L << 8) |CMD_REG8H);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( CMD_REGAL << 8) |CMD_REG9H);

	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	NC_WaitSetCIFD(&au8_Hynix20nm64GBMLCA_RRIOTPWout[1], 1);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 2, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
	#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
	#else
		//nand_stop();
	#endif
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	u32_Err = NC_Read_RandomIn(u32_Row, 0, pu8_DataBuf, 260);
	if (u32_Err !=UNFD_ST_SUCCESS)
	{
		return u32_Err;
	}

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x38<<8 | 0x38);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB<< 8) | CMD_REG8L);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 3, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
	#if 0==IF_IP_VERIFY
		NC_ReConfig();
		NC_CLR_DDR_MODE();
		NC_ResetNandFlash();
	#else
		//nand_stop();
	#endif
		REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
		NC_CLR_DDR_MODE();
		return UNFD_ST_ERR_RST_TIMEOUT; // timeout
	}
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
	NC_Config();

//	dump_mem(pu8_DataBuf , 260);
	//parse Read Retry Value
	if(pu8_DataBuf[0] != HYNIX20nm32GBMLC_READRETRY_TIME || pu8_DataBuf[1] != HYNIX20nm32GBMLC_REGISTER_NUMBER)
	{
		 nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Mismatch R-R Count %X:%X or R-R Reg Count %X:%X\n", pu8_DataBuf[0], HYNIX20nm32GBMLC_READRETRY_TIME,
						pu8_DataBuf[1], HYNIX20nm32GBMLC_REGISTER_NUMBER);
		 pNandDrv->u8_RequireReadRetry = 0;
		return UNFD_ST_ERR_SETUP_READ_RETRY_FAIL;
	}
	pu8_RRTable1 = pu8_DataBuf + 2;
	pu8_RRInvTable1 = pu8_RRTable1 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER;
	pu8_RRTable2 = pu8_DataBuf + 2 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER * 2;
	pu8_RRInvTable2 = pu8_RRTable2 + HYNIX20nm32GBMLC_READRETRY_TIME * HYNIX20nm32GBMLC_REGISTER_NUMBER;

	for(u16_i = 0; u16_i < HYNIX20nm32GBMLC_READRETRY_TIME; u16_i ++)
	{
		for(u16_j = 0; u16_j < HYNIX20nm32GBMLC_REGISTER_NUMBER; u16_j ++)
		{
			if((pu8_RRTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ^
				pu8_RRInvTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j]) ==  0xFF)
			{
				au8_Hynix20nm64GBMLCA_ReadRetryValue[u16_i][u16_j] = pu8_RRTable1[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ;
			}
			else
			{
				if((pu8_RRTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ^
				pu8_RRInvTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j]) ==  0xFF)
				{
					au8_Hynix20nm64GBMLCA_ReadRetryValue[u16_i][u16_j] = pu8_RRTable2[u16_i * HYNIX20nm32GBMLC_REGISTER_NUMBER + u16_j] ;
				}
				else
				{
					nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Fail Parsing R-R value %X:%X \n", u16_i, u16_j);
					pNandDrv->u8_RequireReadRetry = 0;
					return UNFD_ST_ERR_SETUP_READ_RETRY_FAIL;
				}
			}
			nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "0x%02X, ", au8_Hynix20nm64GBMLCA_ReadRetryValue[u16_i][u16_j] );
		}
		nand_debug(UNFD_DEBUG_LEVEL_LOW, 0, "\n");
	}

	return UNFD_ST_SUCCESS;
}


U32 NC_Hynix_20nm64GBAReadRetrySequence(U8 u8_RetryIndex, U8 u8_SetToDefault)
{
	U8  u8_idx, u8_ValueIdx;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNAND_get_DrvContext_address();
	pNandDrv = pNandDrv;

	NC_Hynix_GetReadRetryParam(au8_Hynix20nm64GBMLCA_ReadRetryRegister, HYNIX20nm32GBMLC_REGISTER_NUMBER);

	nand_clock_setting(FCIE3_SW_SLOWEST_CLK);
	REG_WRITE_UINT16(NC_WIDTH, 0);
	REG_CLR_BITS_UINT16(NC_LATCH_DATA, BIT0|BIT1|BIT2);

	if(u8_SetToDefault)
	{
		gu8_CurIdx = u8_ValueIdx = gu8_DefaultIdx;
	}
	else
	{
		if(u8_RetryIndex == 0)
		{
			gu8_DefaultIdx = gu8_CurIdx;
		}
		gu8_CurIdx ++;
		if(gu8_CurIdx == 8)
			gu8_CurIdx = 0;

		u8_ValueIdx = gu8_CurIdx;
	}

	REG_SET_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_AUTO|BIT_NC_CE_H|BIT_NC_WP_AUTO);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_CMDREG8);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0x16<<8 | 0x36);

	for(u8_idx = 0 ; u8_idx < HYNIX20nm32GBMLC_REGISTER_NUMBER; u8_idx ++)
	{
		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
		REG_WRITE_UINT16(NC_AUXREG_DAT, au8_Hynix20nm64GBMLCA_ReadRetryRegister[u8_idx]);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ((OP_ADR_CYCLE_00|OP_ADR_TYPE_COL|OP_ADR_SET_0) << 8) | CMD_REG8L);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_REG8H << 8) |(ACT_RAN_DOUT));
		REG_WRITE_UINT16(NC_AUXREG_DAT, ( ACT_BREAK << 8) |ACT_BREAK);

		REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 1);
		REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset
		REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

		NC_SetCIFD(&au8_Hynix20nm64GBMLCA_ReadRetryValue[u8_ValueIdx][u8_idx], 0, 1);
		NC_SetCIFD(&au8_Hynix20nm64GBMLCA_ReadRetryValue[u8_ValueIdx][u8_idx], 1, 1);

		REG_SET_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_FULL_TRI);

		if (NC_WaitCIFD_Event(BIT_RBUF_EMPTY, WAIT_READ_TIME)== (WAIT_READ_TIME ))
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: CIFD timeout 0, ErrCode:%Xh\r\n", UNFD_ST_ERR_R_TIMEOUT_RM);
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_R_TIMEOUT_RM; // timeout
		}


		REG_W1C_BITS_UINT16(NC_CIFD_EVENT, BIT_RBUF_EMPTY);


		if (NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
		{
			nand_debug(UNFD_DEBUG_LEVEL_ERROR, 1, "Error: Timeout 2, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT);
		#if 0==IF_IP_VERIFY
			NC_ReConfig();
			NC_CLR_DDR_MODE();
			NC_ResetNandFlash();
		#else
			//nand_stop();
		#endif
			REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);
			NC_CLR_DDR_MODE();
			return UNFD_ST_ERR_RST_TIMEOUT; // timeout
		}
	}

	NC_Config();
	REG_CLR_BITS_UINT16(NC_FUN_CTL, BIT_R2N_MODE_EN);

	nand_clock_setting(pNandDrv->u32_Clk);
	REG_WRITE_UINT16(NC_WIDTH, FCIE_REG41_VAL);
	REG_WRITE_UINT16(NC_LATCH_DATA, pNandDrv->u16_Reg57_RELatch);

	return UNFD_ST_SUCCESS;
}

#endif // NC_SEL_FCIE3
