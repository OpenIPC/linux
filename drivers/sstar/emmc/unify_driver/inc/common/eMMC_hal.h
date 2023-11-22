/*
 * eMMC_hal.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __eMMC_HAL_H__
#define __eMMC_HAL_H__

#include "eMMC.h"

#define U32BEND2LEND(X) \
    (((X & 0x000000FF) << 24) + ((X & 0x0000FF00) << 8) + ((X & 0x00FF0000) >> 8) + ((X & 0xFF000000) >> 24))
#define U16BEND2LEND(X) (((X & 0x00FF) << 8) + ((X & 0xFF00) >> 8))

#define eMMC_IF_TUNING_TTABLE(emmc_ip) (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_TUNING_TTABLE)
#define eMMC_Error_Retry(emmc_ip)      (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_ERROR_RETRY)
#define eMMC_IF_DDRT_TUNING(emmc_ip)   (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_TUNING_TTABLE)
#define eMMC_SPEED_MODE(emmc_ip)       (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_SPEED_MASK)
#define eMMC_IF_NORMAL_SDR(emmc_ip)                            \
    (0 == (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_DDR_MODE) \
     && DRV_FLAG_SPEED_HIGH == (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_SPEED_MASK))

//========================================================
// HAL pre-processors
//========================================================
#if IF_FCIE_SHARE_IP
// re-config FCIE3 for NFIE mode
#define eMMC_RECONFIG() // eMMC_ReConfig();
#else
// NULL to save CPU a JMP/RET time
#define eMMC_RECONFIG()
#endif

#define eMMC_FCIE_CLK_DIS(emmc_ip)      REG_FCIE_CLRBIT(FCIE_SD_MODE(emmc_ip), BIT_CLK_EN)
#define eMMC_FCIE_CHECK_STSERR(emmc_ip) (REG_FCIE(FCIE_SD_STATUS(emmc_ip)) & BIT_SD_FCIE_ERR_FLAGS)

#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
#define eMMC_KEEP_RSP(pu8_OneRspBuf, u8_CmdIdx)                    \
    if (g_eMMCDrv[emmc_ip].u32_DrvFlag & DRV_FLAG_RSPFROMRAM_SAVE) \
    eMMC_KeepRsp(pu8_OneRspBuf, u8_CmdIdx)
#else
#define eMMC_KEEP_RSP(pu8_OneRspBuf, u8_CmdIdx) // NULL to save CPU a JMP/RET time
#endif

#define _START_TXMIT 0x40 // transmission bit

#define eMMC_GO_IDLE_STATE   (_START_TXMIT + 0)
#define eMMC_SEND_OP_COND    (_START_TXMIT + 1)
#define eMMC_ALL_SEND_CID    (_START_TXMIT + 2)
#define eMMC_SET_RLT_ADDR    (_START_TXMIT + 3)
#define eMMC_SEL_DESEL_CARD  (_START_TXMIT + 7)
#define eMMC_SEND_EXT_CSD    (_START_TXMIT + 8)
#define eMMC_SEND_CSD        (_START_TXMIT + 9)
#define eMMC_SWITCH          (_START_TXMIT + 6)
#define eMMC_ERASE_GROUP_S   (_START_TXMIT + 35)
#define eMMC_ERASE_GROUP_E   (_START_TXMIT + 36)
#define eMMC_ERASE           (_START_TXMIT + 38)
#define eMMC_SEND_STATUS     (_START_TXMIT + 13)
#define eMMC_R_SINGLE_BLOCK  (_START_TXMIT + 17)
#define eMMC_R_MULTIP_BLOCK  (_START_TXMIT + 18)
#define eMMC_STOP_TRANSMIT   (_START_TXMIT + 12)
#define eMMC_W_SINGLE_BLOCK  (_START_TXMIT + 24)
#define eMMC_W_MULTIP_BLOCK  (_START_TXMIT + 25)
#define eMMC_SEND_TUNING_BLK (_START_TXMIT + 21)

#define STUFF_BITS 0x00000000

#define BITS_MSK_DRIVER_STRENGTH 0xF0
#define BITS_MSK_TIMING          0x0F

#define UPFW_SEC_WIAT_CNT 0x1000000
#define UPFW_SEC_BYTE_CNT (128 * 1024)

#define MAX_DETECT_BLK_CNT 2048 // 0.5MB

#define eMMC_CMD1_RETRY_CNT 0x8000
#define eMMC_CMD3_RETRY_CNT 0x5

#define eMMC_FCIE_CMD_RSP_ERR_RETRY_CNT     20
#define eMMC_CMD_API_ERR_RETRY_CNT          20
#define eMMC_CMD_API_WAIT_FIFOCLK_RETRY_CNT 20

#define FCIE_WAIT_RST_DONE_D1  0x5555
#define FCIE_WAIT_RST_DONE_D2  0xAAAA
#define FCIE_WAIT_RST_DONE_CNT 3

extern U8 sgu8_IfNeedRestorePadType; // = 0xff
extern U8 u8_sdr_retry_count;        // = 0

extern U8 gu8_emmcPADOrderSlot[EMMC_NUM_TOTAL];

/* eMMC FCIE setting */
U32  eMMC_FCIE_WaitEvents(eMMC_IP_EmType emmc_ip, uintptr_t ulongRegAddr, U16 u16_Events, U32 u32_MicroSec);
void eMMC_FCIE_ErrHandler_Stop(eMMC_IP_EmType emmc_ip);
U32  eMMC_FCIE_ErrHandler_Retry(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_ErrHandler_RestoreClk(eMMC_IP_EmType emmc_ip);
U32  eMMC_FCIE_ErrHandler_ReInit_Ex(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_ErrHandler_ReInit(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_CheckResetDone(eMMC_IP_EmType emmc_ip);
U32  eMMC_FCIE_Reset(eMMC_IP_EmType emmc_ip);
U32  eMMC_FCIE_Init(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_ClearEvents(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_ClearEvents_Reg0(eMMC_IP_EmType emmc_ip);
U32  eMMC_FCIE_WaitD0High_Ex(eMMC_IP_EmType emmc_ip, U32 u32_us);
U32  eMMC_FCIE_WaitD0High(eMMC_IP_EmType emmc_ip, U32 u32_us);
void eMMC_FCIE_TransCmdSetting(eMMC_IP_EmType emmc_ip, U16 u16BlkCnt, U16 u16BlkSize, dma_addr_t sizeBufAddr,
                               eMMC_Trans_EmType e_TransType);
void eMMC_FCIE_SetCmdTaken(eMMC_IP_EmType emmc_ip, U32 u32_Arg, U8 u8_CmdIdx);
U32  eMMC_FCIE_JobStartAndWaitEvent(eMMC_IP_EmType emmc_ip, eMMC_Trans_EmType e_TransType, eMMC_Cmd_EmType e_CmdType,
                                    U16 u16_waitEvent, U32 u32_waitTimes);
U32  eMMC_FCIE_SendCmdAndWaitProcess(eMMC_IP_EmType emmc_ip, U8 u8_CmdIdx, eMMC_Trans_EmType e_TransType,
                                     eMMC_Cmd_EmType e_CmdType, eMMC_Rsp_EmType e_RspType);
void eMMC_FCIE_GetCMDFIFO(eMMC_IP_EmType emmc_ip, U16 u16_WordPos, U16 u16_WordCnt, U16 *pu16_Buf);
U32  eMMC_WaitCIFD_Event(eMMC_IP_EmType emmc_ip, U16 u16_WaitEvent, U32 u32_MicroSec);
U32  eMMC_WaitSetCIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf, U32 u32_ByteCnt);
void eMMC_FCIE_GetCIFD(eMMC_IP_EmType emmc_ip, U16 u16_WordPos, U16 u16_WordCnt, U16 *pu16_Buf);
U32  eMMC_WaitGetCIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf, U32 u32_ByteCnt);
U8   eMMC_FCIE_DataFifoGet(U8 emmc_id, U8 u8addr);
U32  eMMC_FCIE_CIFD_WaitProcess(eMMC_IP_EmType emmc_ip, eMMC_Cmd_EmType e_CmdType, volatile U8 *u8Buf);
U32  eMMC_CheckR1Error(eMMC_IP_EmType emmc_ip);
void eMMC_PreparePowerSavingModeQueue(eMMC_IP_EmType emmc_ip);

/* eMMC_hal_speed.c */
U32 eMMC_FCIE_EnableSDRMode(eMMC_IP_EmType emmc_ip);
#if !(defined(ENABLE_eMMC_ATOP) && ENABLE_eMMC_ATOP)
void eMMC_DumpDDR48TTable(eMMC_IP_EmType emmc_ip);
void eMMC_FCIE_SetDDR48TimingReg(U8 u8_DQS, U8 u8_DelaySel);
#else // DDR52 (ATOP)
void eMMC_DumpATopTable(void);
void eMMC_FCIE_SetATopTimingReg(U8 u8_SetIdx);
void eMMC_FCIE_Apply_Reg(U8 u8_SetIdx);
void eMMC_DumpGeneralTable(void);
#endif
U32  eMMC_FCIE_ChooseSpeedMode(void);
void eMMC_FCIE_ApplyTimingSet(eMMC_IP_EmType emmc_ip, U8 u8_Idx);
void eMMC_DumpTimingTable(eMMC_IP_EmType emmc_ip);
U32  eMMC_LoadTimingTable(eMMC_IP_EmType emmc_ip, U8 u8_PadType);
U32  eMMC_FCIE_EnableFastMode_Ex(eMMC_IP_EmType emmc_ip, U8 u8_PadType);
U32  eMMC_FCIE_EnableFastMode(eMMC_IP_EmType emmc_ip, U8 u8_PadType);

/* eMMC command function */
U32 eMMC_CMD0(eMMC_IP_EmType emmc_ip, U32 u32_Arg);
U32 eMMC_CMD1(eMMC_IP_EmType emmc_ip);
U32 eMMC_CMD2(eMMC_IP_EmType emmc_ip);
U32 eMMC_CMD3_CMD7(eMMC_IP_EmType emmc_ip, U16 u16_RCA, U8 u8_CmdIdx);
U32 eMMC_CMD6(eMMC_IP_EmType emmc_ip, U32 u32_Arg);
U32 eMMC_CMD9(eMMC_IP_EmType emmc_ip, U16 u16_RCA);
U32 eMMC_CMD8_MIU(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf);
U32 eMMC_CMD8_CIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf);
U32 eMMC_CMD8(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf);
U32 eMMC_CMD12(eMMC_IP_EmType emmc_ip, U16 u16_RCA);
U32 eMMC_CMD13(eMMC_IP_EmType emmc_ip, U16 u16_RCA);
U32 eMMC_CMD16(eMMC_IP_EmType emmc_ip, U32 u32_BlkLength);
U32 eMMC_CMD17_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD17_CIFD(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD17(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD18_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt);
U32 eMMC_CMD18(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt);
U32 eMMC_CMD23(eMMC_IP_EmType emmc_ip, U16 u16_BlkCnt);
U32 eMMC_CMD24_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD24_CIFD(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD24(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf);
U32 eMMC_CMD25_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt);
U32 eMMC_CMD25(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt);
U32 eMMC_CMD35_CMD36(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 u8_CmdIdx);
U32 eMMC_CMD38(eMMC_IP_EmType emmc_ip);
U32 eMMC_EraseCMDSeq(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr_start, U32 u32_eMMCBlkAddr_end);
U32 eMMC_CSD_Config(eMMC_IP_EmType emmc_ip);
U32 eMMC_ExtCSD_Config(eMMC_IP_EmType emmc_ip);
U32 eMMC_ModifyExtCSD(eMMC_IP_EmType emmc_ip, U8 u8_AccessMode, U8 u8_ByteIdx, U8 u8_Value);
U32 eMMC_SetPwrOffNotification(eMMC_IP_EmType emmc_ip, U8 u8_SetECSD34);
U32 eMMC_Sanitize(eMMC_IP_EmType emmc_ip, U8 u8_ECSD165);
U32 eMMC_SetBusSpeed(eMMC_IP_EmType emmc_ip, U8 u8_BusSpeed);
U32 eMMC_SetDrivingStrength(eMMC_IP_EmType emmc_ip, U8 u8Driving);
U32 eMMC_SetBusWidth(eMMC_IP_EmType emmc_ip, U8 u8_BusWidth, U8 u8_IfDDR);
U32 eMMC_Identify(eMMC_IP_EmType emmc_ip);
U32 eMMC_CMD28_CMD29(eMMC_IP_EmType emmc_ip, U32 u32_eMMCDataAddress1, U8 u8_CmdIdx);
U32 eMMC_CMD30_CMD31_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U8 u8_CmdIdx);

#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
void eMMC_KeepRsp(U8 *pu8_OneRspBuf, U8 u8_CmdIdx);
U32  eMMC_ReturnRsp(U8 *pu8_OneRspBuf, U8 u8_CmdIdx);
U32  eMMC_SaveRsp(void);
U32  eMMC_SaveDriverContext(void);
U32  eMMC_LoadRsp(U8 *pu8_AllRspBuf);
U32  eMMC_LoadDriverContext(U8 *pu8_Buf);
#endif
#if defined(eMMC_BURST_LEN_AUTOCFG) && eMMC_BURST_LEN_AUTOCFG
void eMMC_DumpBurstLenTable(eMMC_IP_EmType emmc_ip);
U32  eMMC_LoadBurstLenTable(eMMC_IP_EmType emmc_ip);
U32  eMMC_SaveBurstLenTable(eMMC_IP_EmType emmc_ip);
#endif
U32 eMMC_GetR1(eMMC_IP_EmType emmc_ip);
#if defined(eMMC_UPDATE_FIRMWARE) && (eMMC_UPDATE_FIRMWARE)
U32 eMMC_UpFW_Samsung(eMMC_IP_EmType emmc_ip, U8 *pu8_FWBin);
#endif

#endif // __eMMC_HAL_H__
