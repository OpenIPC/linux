/*
 * eMMC_hal_command.c- Sigmastar
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

#include "eMMC.h"
#include "eMMC_hal.h"
#include "eMMC_utl.h"
#include "eMMC_platform.h"
#include "eMMC_err_codes.h"

U32 eMMC_CMD0(eMMC_IP_EmType emmc_ip, U32 u32_Arg)
{
    U32 u32_err;

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_Arg, 0);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(
        emmc_ip, 0, EMMC_EMP, 0, (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_CMD_NO_RSP) ? eMMC_R1 : eMMC_NO);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;

    return u32_err;
}

// send OCR
U32 eMMC_CMD1(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err, u32_arg;

    //[7]     1.7-1.95V 0:high 1:dual
    //[14:08] 2.0-2.6 V
    //[23:15] 2.7-3.6 V
    //[30:29] 2.0-2.6 V (00b:10b)/(BYTE:SEC)
    u32_arg = BIT30
              | (BIT23 | BIT22 | BIT21 | BIT20 | BIT19 | BIT18 | BIT17 | BIT16 | BIT15
                 | BIT7); // ARG[<=2G : >2G]/[x00_FF_80_00 : x40_FF_80_00]

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 1);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 1, EMMC_EMP, 0, eMMC_R3);
    if (!u32_err)
    {                                                                           // CMD1 ok, do things here
        eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, 3, (U16 *)g_eMMCDrv[emmc_ip].au8_Rsp); // RDY(2G):x80_FF_80_80/xC0_FF_80_80
        // eMMC_dump_mem(g_eMMCDrv[emmc_ip].au8_Rsp, eMMC_R3_BYTE_CNT);

        if (0 == (g_eMMCDrv[emmc_ip].au8_Rsp[1] & 0x80))
        {
            u32_err = eMMC_ST_ERR_CMD1_DEV_NOT_RDY;
        }
        else
        {
            g_eMMCDrv[emmc_ip].u8_IfSectorMode = (g_eMMCDrv[emmc_ip].au8_Rsp[1] & BIT6) >> 6;
            eMMC_KEEP_RSP(g_eMMCDrv[emmc_ip].au8_Rsp, 1);
        }
    }

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;

    return u32_err;
}

// send CID
U32 eMMC_CMD2(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err, u32_arg;

    u32_arg = 0;

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 2);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 2, EMMC_EMP, 0, eMMC_R2);
    if (!u32_err)
    { // CMD2 ok, do things here (get CID)
        eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, eMMC_R2_BYTE_CNT >> 1, (U16 *)g_eMMCDrv[emmc_ip].au8_CID);
        // eMMC_dump_mem(g_eMMCDrv[emmc_ip].au8_CID, eMMC_R2_BYTE_CNT);
        eMMC_KEEP_RSP(g_eMMCDrv[emmc_ip].au8_CID, 2);
    }

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;

    return u32_err;
}

// CMD3: assign RCA. CMD7: select device
U32 eMMC_CMD3_CMD7(eMMC_IP_EmType emmc_ip, U16 u16_RCA, U8 u8_CmdIdx)
{
    U32 u32_err, u32_arg, u32_rsp;

    if (7 == u8_CmdIdx)
    {
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_RSP_WAIT_D0H;
    }
    else
    {
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry
    }

    u32_arg = u16_RCA << 16;

    if (7 == u8_CmdIdx && u16_RCA != g_eMMCDrv[emmc_ip].u16_RCA)
        u32_rsp = eMMC_NO;
    else
        u32_rsp = eMMC_R1;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, u8_CmdIdx);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, u8_CmdIdx, EMMC_EMP, 0, u32_rsp);
    if (!u32_err)
        eMMC_KEEP_RSP(g_eMMCDrv[emmc_ip].au8_Rsp, u8_CmdIdx);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~(DRV_FLAG_RSP_WAIT_D0H | DRV_FLAG_TUNING_TTABLE);
    return u32_err;
}

// SWITCH cmd
U32 eMMC_CMD6(eMMC_IP_EmType emmc_ip, U32 u32_Arg)
{
    U32 u32_err;

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_RSP_WAIT_D0H;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_Arg, 6);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 6, EMMC_EMP, 0, eMMC_R1B);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_RSP_WAIT_D0H;
    return u32_err;
}

// send CSD (in R2)
U32 eMMC_CMD9(eMMC_IP_EmType emmc_ip, U16 u16_RCA)
{
    U32 u32_err, u32_arg;

    u32_arg = u16_RCA << 16;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 9);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 9, EMMC_EMP, 0, eMMC_R2);
    if (!u32_err)
    { // CMD2 ok, do things here
        eMMC_FCIE_GetCMDFIFO(emmc_ip, 0, eMMC_R2_BYTE_CNT >> 1, (U16 *)g_eMMCDrv[emmc_ip].au8_CSD);
        // eMMC_dump_mem(g_eMMCDrv[emmc_ip].au8_CSD, eMMC_R2_BYTE_CNT);
        eMMC_KEEP_RSP(g_eMMCDrv[emmc_ip].au8_CSD, 9);
    }

    return u32_err;
}

// CMD8: send EXT_CSD
U32 eMMC_CMD8_MIU(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf)
{
    U32        u32_err, u32_arg;
    dma_addr_t dma_DataMapAddr;

    // -------------------------------
    // send cmd
    u32_arg = 0;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 8);

    dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE, 1);
    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE,
                              eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL), EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 8, EMMC_DMA, eMMC_CMD_READ, eMMC_R1);

    eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE, 1);
    return u32_err;
}

U32 eMMC_CMD8_CIFD(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf)
{
    U32 u32_err, u32_arg;

    // -------------------------------
    // send cmd
    u32_arg = 0;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 8);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE, (uintptr_t)pu8_DataBuf, EMMC_CIF);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 8, EMMC_CIF, eMMC_CMD_READ, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD8(eMMC_IP_EmType emmc_ip, U8 *pu8_DataBuf)
{
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    return eMMC_CMD8_CIFD(emmc_ip, pu8_DataBuf);
#else
    return eMMC_CMD8_MIU(emmc_ip, pu8_DataBuf);
#endif
}

U32 eMMC_CMD12(eMMC_IP_EmType emmc_ip, U16 u16_RCA)
{
    U32 u32_err, u32_arg;

    u32_arg = (u16_RCA << 16) | ((g_eMMCDrv[emmc_ip].u32_eMMCFlag & eMMC_FLAG_HPI_CMD12) ? 1 : 0);

#if defined(eMMC_UPDATE_FIRMWARE) && (eMMC_UPDATE_FIRMWARE)
    u32_arg = 0;
#endif

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= (DRV_FLAG_RSP_WAIT_D0H | DRV_FLAG_TUNING_TTABLE);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 12);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 12, EMMC_EMP, 0, eMMC_R1B);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~(DRV_FLAG_RSP_WAIT_D0H | DRV_FLAG_TUNING_TTABLE);
    return u32_err;
}

// CMD13: send Status
U32 eMMC_CMD13(eMMC_IP_EmType emmc_ip, U16 u16_RCA)
{
    U32 u32_err, u32_arg;

    u32_arg = (u16_RCA << 16);

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_TUNING_TTABLE; // avoid retry

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 13);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 13, EMMC_EMP, 0, eMMC_R1);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_TUNING_TTABLE;

    return u32_err;
}

U32 eMMC_CMD16(eMMC_IP_EmType emmc_ip, U32 u32_BlkLength)
{
    U32 u32_err, u32_arg;

    u32_arg = u32_BlkLength;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 16);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 16, EMMC_EMP, 0, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD17_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
    U32        u32_err, u32_arg;
    dma_addr_t dma_DataMapAddr;

    // -------------------------------
    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
        eMMC_FCIE_ErrHandler_RestoreClk(emmc_ip);

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 17);

    if (REG_FCIE(FCIE_BOOT(emmc_ip)) & BIT2)
        dma_DataMapAddr = (dma_addr_t)__virt_to_phys((uintptr_t)pu8_DataBuf);
    else
        dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE, 1);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE,
                              eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL), EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 17, EMMC_DMA, eMMC_CMD_READ,
                                              (g_eMMCDrv[emmc_ip].u8_make_sts_err == FCIE_MAKE_CMD_RSP_ERR) ? eMMC_R2
                                                                                                            : eMMC_R1);

    if ((REG_FCIE(FCIE_BOOT(emmc_ip)) & BIT2) == 0)
        eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE, 1);
    return u32_err;
}

U32 eMMC_CMD17_CIFD(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
    U32 u32_err, u32_arg;

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 17);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE, (uintptr_t)pu8_DataBuf, EMMC_CIF);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 17, EMMC_CIF, eMMC_CMD_READ, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD17(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    return eMMC_CMD17_CIFD(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf);
#else
    return eMMC_CMD17_MIU(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf);
#endif
}

U32 eMMC_CMD18_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt)
{
    U32        u32_err, u32_arg;
    dma_addr_t dma_DataMapAddr;

    // -------------------------------
    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
        eMMC_FCIE_ErrHandler_RestoreClk(emmc_ip);

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 18);

    dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE * u16_BlkCnt, 1);
    eMMC_FCIE_TransCmdSetting(emmc_ip, u16_BlkCnt, eMMC_SECTOR_512BYTE,
                              eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL), EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 18, EMMC_DMA, eMMC_CMD_READ, eMMC_R1);

    eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE * u16_BlkCnt, 1);

    // stop command ---------------------------------------
    u32_err = eMMC_CMD12(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);

    return u32_err;
}

U32 eMMC_CMD18(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt)
{
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    U16 u16_cnt;
    U32 u32_err;

    for (u16_cnt = 0; u16_cnt < u16_BlkCnt; u16_cnt++)
    {
        u32_err = eMMC_CMD17_CIFD(emmc_ip, u32_eMMCBlkAddr + u16_cnt, pu8_DataBuf + (u16_cnt << 9));
        if (eMMC_ST_SUCCESS != u32_err)
            return u32_err;
    }
    return eMMC_ST_SUCCESS;
#else
    return eMMC_CMD18_MIU(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf, u16_BlkCnt);
#endif
}

// enable Reliable Write
U32 eMMC_CMD23(eMMC_IP_EmType emmc_ip, U16 u16_BlkCnt)
{
    U32 u32_err, u32_arg;

    u32_arg = u16_BlkCnt & 0xFFFF; // don't set BIT24
#if defined(eMMC_FEATURE_RELIABLE_WRITE) && eMMC_FEATURE_RELIABLE_WRITE
    u32_arg |= BIT31; // don't set BIT24
#endif

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 23);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 23, EMMC_EMP, 0, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD24_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
    U32        u32_err, u32_arg;
    dma_addr_t dma_DataMapAddr;

    // -------------------------------
    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
        eMMC_FCIE_ErrHandler_RestoreClk(emmc_ip);

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 24);

    if (REG_FCIE(FCIE_BOOT(emmc_ip)) & BIT2)
        dma_DataMapAddr = (dma_addr_t)__virt_to_phys((uintptr_t)pu8_DataBuf);
    else
        dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE, 0);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE,
                              eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL), EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 24, EMMC_DMA, eMMC_CMD_WRITE, eMMC_R1);

    if ((REG_FCIE(FCIE_BOOT(emmc_ip)) & BIT2) == 0)
        eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE, 0);
    return u32_err;
}

U32 eMMC_CMD24_CIFD(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
    U32 u32_err, u32_arg;

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 24);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, eMMC_SECTOR_512BYTE, (uintptr_t)pu8_DataBuf, EMMC_CIF);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 24, EMMC_CIF, eMMC_CMD_WRITE, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD24(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf)
{
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    return eMMC_CMD24_CIFD(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf);
#else
    return eMMC_CMD24_MIU(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf);
#endif
}

U32 eMMC_CMD25_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt)
{
    U32        u32_err, u32_arg;
    dma_addr_t dma_DataMapAddr;

    // -------------------------------
    // restore clock to max
    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
        eMMC_FCIE_ErrHandler_RestoreClk(emmc_ip);

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 25);

    dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE * u16_BlkCnt, 0);
    eMMC_FCIE_TransCmdSetting(emmc_ip, u16_BlkCnt, eMMC_SECTOR_512BYTE,
                              eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL), EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 25, EMMC_DMA, eMMC_CMD_WRITE, eMMC_R1);

    eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE * u16_BlkCnt, 0);

    // stop command ---------------------------------------
    u32_err = eMMC_CMD12(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);

    return u32_err;
}

U32 eMMC_CMD25(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U16 u16_BlkCnt)
{
    U32 u32_err = eMMC_ST_SUCCESS;
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    U16 u16_cnt;
#endif

#if (defined(eMMC_BURST_LEN_AUTOCFG) && eMMC_BURST_LEN_AUTOCFG) || ENABLE_eMMC_RIU_MODE
    U16 u16_RetryCnt = 0;
LABEL_CMD25:
#endif
// ------------------------------RIU mode
#if defined(ENABLE_eMMC_RIU_MODE) && ENABLE_eMMC_RIU_MODE
    for (u16_cnt = 0; u16_cnt < u16_BlkCnt; u16_cnt++)
    {
        u32_err =
            eMMC_CMD24_CIFD(emmc_ip, u32_eMMCBlkAddr + u16_cnt, pu8_DataBuf + (u16_cnt << eMMC_SECTOR_BYTECNT_BITS));
        if (eMMC_ST_SUCCESS != u32_err)
            goto LABEL_CMD25_END;
    }
#else
    // ------------------------------MIU mode
#if defined(eMMC_BURST_LEN_AUTOCFG) && eMMC_BURST_LEN_AUTOCFG
    {
        U16 u16_blk_cnt, u16_blk_pos;

        u16_blk_pos = 0;

        if (g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen)
            while (u16_blk_pos < u16_BlkCnt)
            {
                u16_blk_cnt = u16_BlkCnt - u16_blk_pos > g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen
                                  ? g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen
                                  : u16_BlkCnt - u16_blk_pos;
                u16_blk_cnt =
                    u16_blk_cnt == g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstBrustLen ? u16_blk_cnt / 2 : u16_blk_cnt;

                u32_err = eMMC_CMD25_MIU(emmc_ip, u32_eMMCBlkAddr + u16_blk_pos,
                                         pu8_DataBuf + (u16_blk_pos << eMMC_SECTOR_BYTECNT_BITS), u16_blk_cnt);
                if (eMMC_ST_SUCCESS != u32_err)
                    goto LABEL_CMD25_END;

                u16_blk_pos += u16_blk_cnt;
            }
        else
            u32_err = eMMC_CMD25_MIU(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf, u16_BlkCnt);
    }
#else
    u32_err = eMMC_CMD25_MIU(emmc_ip, u32_eMMCBlkAddr, pu8_DataBuf, u16_BlkCnt);
#endif
#endif

#if (defined(eMMC_BURST_LEN_AUTOCFG) && eMMC_BURST_LEN_AUTOCFG) || ENABLE_eMMC_RIU_MODE
LABEL_CMD25_END:
    if (eMMC_ST_SUCCESS != u32_err && u16_RetryCnt < eMMC_FCIE_CMD_RSP_ERR_RETRY_CNT)
    {
        u16_RetryCnt++;
        goto LABEL_CMD25;
    }
#endif

    return u32_err;
}

U32 eMMC_CMD28_CMD29(eMMC_IP_EmType emmc_ip, U32 u32_eMMCDataAddress1, U8 u8_CmdIdx)
{
    U32 u32_err, u32_arg;

    u32_arg = u32_eMMCDataAddress1 << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);
    g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_RSP_WAIT_D0H;

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, u8_CmdIdx);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, u8_CmdIdx, EMMC_EMP, 0, eMMC_R1B);
    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_RSP_WAIT_D0H;

    return u32_err;
}

U32 eMMC_CMD30_CMD31_MIU(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 *pu8_DataBuf, U8 u8_CmdIdx)
{
    U32        u32_err, u32_arg, u32_blksize;
    dma_addr_t dma_DataMapAddr;

    if (u8_CmdIdx == 30)
        u32_blksize = 4;
    else
        u32_blksize = 8;

    // -------------------------------
    if (0 == eMMC_IF_TUNING_TTABLE(emmc_ip))
        eMMC_FCIE_ErrHandler_RestoreClk(emmc_ip);

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, u8_CmdIdx);

    dma_DataMapAddr = eMMC_DMA_MAP_address((uintptr_t)pu8_DataBuf, eMMC_SECTOR_512BYTE, 1);

    eMMC_FCIE_TransCmdSetting(emmc_ip, 1, u32_blksize, eMMC_Platform_Trans_Dma_Addr(emmc_ip, dma_DataMapAddr, NULL),
                              EMMC_DMA);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, u8_CmdIdx, EMMC_DMA, eMMC_CMD_READ, eMMC_R1);

    eMMC_DMA_UNMAP_address(dma_DataMapAddr, eMMC_SECTOR_512BYTE, 1);

    return u32_err;
}

U32 eMMC_CMD35_CMD36(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr, U8 u8_CmdIdx)
{
    U32 u32_err, u32_arg;

    u32_arg = u32_eMMCBlkAddr << (g_eMMCDrv[emmc_ip].u8_IfSectorMode ? 0 : eMMC_SECTOR_512BYTE_BITS);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, u8_CmdIdx);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, u8_CmdIdx, EMMC_EMP, 0, eMMC_R1);

    return u32_err;
}

U32 eMMC_CMD38(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err, u32_arg;

    if (g_eMMCDrv[emmc_ip].u32_eMMCFlag & eMMC_FLAG_TRIM)
        u32_arg = 0x1;
    else
        u32_arg = 0x0;

    g_eMMCDrv[emmc_ip].u32_DrvFlag |= (DRV_FLAG_RSP_WAIT_D0H);

    eMMC_FCIE_SetCmdTaken(emmc_ip, u32_arg, 38);

    u32_err = eMMC_FCIE_SendCmdAndWaitProcess(emmc_ip, 38, EMMC_EMP, 0, eMMC_R1B);

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~(DRV_FLAG_RSP_WAIT_D0H);
    return u32_err;
}

/* Erase api */
U32 eMMC_EraseCMDSeq(eMMC_IP_EmType emmc_ip, U32 u32_eMMCBlkAddr_start, U32 u32_eMMCBlkAddr_end)
{
    U32 u32_err;

    u32_err = eMMC_CMD35_CMD36(emmc_ip, u32_eMMCBlkAddr_start, 35);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    u32_err = eMMC_CMD35_CMD36(emmc_ip, u32_eMMCBlkAddr_end, 36);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    u32_err = eMMC_CMD38(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    return eMMC_ST_SUCCESS;
}

//------------------------------------------------
U32 eMMC_CSD_Config(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err;

    u32_err = eMMC_CMD9(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    // ------------------------------
    g_eMMCDrv[emmc_ip].u8_SPEC_VERS = (g_eMMCDrv[emmc_ip].au8_CSD[1] & 0x3C) >> 2;
    g_eMMCDrv[emmc_ip].u8_R_BL_LEN  = g_eMMCDrv[emmc_ip].au8_CSD[6] & 0xF;
    g_eMMCDrv[emmc_ip].u8_W_BL_LEN =
        ((g_eMMCDrv[emmc_ip].au8_CSD[13] & 0x3) << 2) + ((g_eMMCDrv[emmc_ip].au8_CSD[14] & 0xC0) >> 6);

    // ------------------------------
    g_eMMCDrv[emmc_ip].u16_C_SIZE = (g_eMMCDrv[emmc_ip].au8_CSD[7] & 3) << 10;
    g_eMMCDrv[emmc_ip].u16_C_SIZE += g_eMMCDrv[emmc_ip].au8_CSD[8] << 2;
    g_eMMCDrv[emmc_ip].u16_C_SIZE += (g_eMMCDrv[emmc_ip].au8_CSD[9] & 0xC0) >> 6;
    if (0xFFF == g_eMMCDrv[emmc_ip].u16_C_SIZE)
    {
        g_eMMCDrv[emmc_ip].u32_SEC_COUNT = 0;
    }
    else
    {
        g_eMMCDrv[emmc_ip].u8_C_SIZE_MULT =
            ((g_eMMCDrv[emmc_ip].au8_CSD[10] & 3) << 1) + ((g_eMMCDrv[emmc_ip].au8_CSD[11] & 0x80) >> 7);

        g_eMMCDrv[emmc_ip].u32_SEC_COUNT = (g_eMMCDrv[emmc_ip].u16_C_SIZE + 1)
                                               * (1 << (g_eMMCDrv[emmc_ip].u8_C_SIZE_MULT + 2))
                                               * ((1 << g_eMMCDrv[emmc_ip].u8_R_BL_LEN) >> 9)
                                           - 8; // -8: //Toshiba CMD18 access the last block report out of range error
    }
    // ------------------------------
    g_eMMCDrv[emmc_ip].u8_ERASE_GRP_SIZE = (g_eMMCDrv[emmc_ip].au8_CSD[10] & 0x7C) >> 2;
    g_eMMCDrv[emmc_ip].u8_ERASE_GRP_MULT =
        ((g_eMMCDrv[emmc_ip].au8_CSD[10] & 0x03) << 3) + ((g_eMMCDrv[emmc_ip].au8_CSD[11] & 0xE0) >> 5);
    g_eMMCDrv[emmc_ip].u32_EraseUnitSize =
        (g_eMMCDrv[emmc_ip].u8_ERASE_GRP_SIZE + 1) * (g_eMMCDrv[emmc_ip].u8_ERASE_GRP_MULT + 1);
    // ------------------------------
    // others
    g_eMMCDrv[emmc_ip].u8_TAAC       = g_eMMCDrv[emmc_ip].au8_CSD[2];
    g_eMMCDrv[emmc_ip].u8_NSAC       = g_eMMCDrv[emmc_ip].au8_CSD[3];
    g_eMMCDrv[emmc_ip].u8_Tran_Speed = g_eMMCDrv[emmc_ip].au8_CSD[4];
    g_eMMCDrv[emmc_ip].u8_R2W_FACTOR = (g_eMMCDrv[emmc_ip].au8_CSD[13] & 0x1C) >> 2;

    g_eMMCDrv[emmc_ip].u8_WP_GRP_SIZE =
        ((g_eMMCDrv[emmc_ip].au8_CSD[11] & 0xF0) >> 3 | (g_eMMCDrv[emmc_ip].au8_CSD[12] & 0x01));
    g_eMMCDrv[emmc_ip].u8_WP_GRP_ENABLE = (g_eMMCDrv[emmc_ip].au8_CSD[12] & 0x02) >> 1;

    return eMMC_ST_SUCCESS;
}

/* Ext CSD get--set relevant */
U32 eMMC_ExtCSD_Config(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err;

    u32_err = eMMC_CMD8(emmc_ip, gau8_eMMC_SectorBuf);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(0, 1, "eMMC Err: CMD8 fail\n");
        eMMC_dump_mem(gau8_eMMC_SectorBuf, 0x200);
        return u32_err;
    }

    //--------------------------------
    if (0 == g_eMMCDrv[emmc_ip].u32_SEC_COUNT)
        g_eMMCDrv[emmc_ip].u32_SEC_COUNT = ((gau8_eMMC_SectorBuf[215] << 24) | (gau8_eMMC_SectorBuf[214] << 16)
                                            | (gau8_eMMC_SectorBuf[213] << 8) | (gau8_eMMC_SectorBuf[212]))
                                           - 8; //-8: Toshiba CMD18 access the last block report out of range error

    //-------------------------------
    if (0 == g_eMMCDrv[emmc_ip].u32_BOOT_SEC_COUNT)
        g_eMMCDrv[emmc_ip].u32_BOOT_SEC_COUNT = gau8_eMMC_SectorBuf[226] * 128 * 2;

    //--------------------------------
    if (!g_eMMCDrv[emmc_ip].u8_BUS_WIDTH)
    {
        g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = gau8_eMMC_SectorBuf[183];
        switch (g_eMMCDrv[emmc_ip].u8_BUS_WIDTH)
        {
            case 0:
                g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_1;
                break;
            case 1:
                g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_4;
                break;
            case 2:
                g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_8;
                break;
            default:
                eMMC_debug(0, 1, "eMMC Err: eMMC BUS_WIDTH not support\n");
                while (1)
                    ;
        }
    }
    //--------------------------------
    if (gau8_eMMC_SectorBuf[231] & BIT4) // TRIM
        g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_TRIM;
    else
        g_eMMCDrv[emmc_ip].u32_eMMCFlag &= ~eMMC_FLAG_TRIM;

    //--------------------------------
    if (gau8_eMMC_SectorBuf[503] & BIT0) // HPI
    {
        if (gau8_eMMC_SectorBuf[503] & BIT1)
            g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_HPI_CMD12;
        else
            g_eMMCDrv[emmc_ip].u32_eMMCFlag |= eMMC_FLAG_HPI_CMD13;
    }
    else
        g_eMMCDrv[emmc_ip].u32_eMMCFlag &= ~(eMMC_FLAG_HPI_CMD12 | eMMC_FLAG_HPI_CMD13);

    //--------------------------------
    if (gau8_eMMC_SectorBuf[166] & BIT2) // Reliable Write
        g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = BIT_SD_JOB_BLK_CNT_MASK;
    else
    {
#if 0
        g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = gau8_eMMC_SectorBuf[222];
#else
        if ((gau8_eMMC_SectorBuf[503] & BIT0) && 1 == gau8_eMMC_SectorBuf[222])
            g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = 1;
        else if (0 == (gau8_eMMC_SectorBuf[503] & BIT0))
            g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = gau8_eMMC_SectorBuf[222];
        else
        {
            // eMMC_debug(0,1,"eMMC Warn: not support dynamic  Reliable-W\n");
            g_eMMCDrv[emmc_ip].u16_ReliableWBlkCnt = 0; // can not support Reliable Write
        }
#endif
    }

    //--------------------------------
    g_eMMCDrv[emmc_ip].u8_ErasedMemContent = gau8_eMMC_SectorBuf[181];

    //--------------------------------
    g_eMMCDrv[emmc_ip].u8_ECSD184_Stroe_Support  = gau8_eMMC_SectorBuf[184];
    g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming       = gau8_eMMC_SectorBuf[185];
    g_eMMCDrv[emmc_ip].u8_ECSD192_Ver            = gau8_eMMC_SectorBuf[192];
    g_eMMCDrv[emmc_ip].u8_ECSD196_DevType        = gau8_eMMC_SectorBuf[196];
    g_eMMCDrv[emmc_ip].u8_ECSD197_DriverStrength = gau8_eMMC_SectorBuf[197];
    g_eMMCDrv[emmc_ip].u8_ECSD248_CMD6TO         = gau8_eMMC_SectorBuf[248];
    g_eMMCDrv[emmc_ip].u8_ECSD247_PwrOffLongTO   = gau8_eMMC_SectorBuf[247];
    g_eMMCDrv[emmc_ip].u8_ECSD34_PwrOffCtrl      = gau8_eMMC_SectorBuf[34];

    // for GP Partition
    g_eMMCDrv[emmc_ip].u8_ECSD160_PartSupField   = gau8_eMMC_SectorBuf[160];
    g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize = gau8_eMMC_SectorBuf[224];
    g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize    = gau8_eMMC_SectorBuf[221];

    g_eMMCDrv[emmc_ip].GP_Part[0].u32_PartSize =
        ((gau8_eMMC_SectorBuf[145] << 16) | (gau8_eMMC_SectorBuf[144] << 8) | (gau8_eMMC_SectorBuf[143]))
        * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

    g_eMMCDrv[emmc_ip].GP_Part[1].u32_PartSize =
        ((gau8_eMMC_SectorBuf[148] << 16) | (gau8_eMMC_SectorBuf[147] << 8) | (gau8_eMMC_SectorBuf[146]))
        * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

    g_eMMCDrv[emmc_ip].GP_Part[2].u32_PartSize =
        ((gau8_eMMC_SectorBuf[151] << 16) | (gau8_eMMC_SectorBuf[150] << 8) | (gau8_eMMC_SectorBuf[149]))
        * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

    g_eMMCDrv[emmc_ip].GP_Part[3].u32_PartSize =
        ((gau8_eMMC_SectorBuf[154] << 16) | (gau8_eMMC_SectorBuf[153] << 8) | (gau8_eMMC_SectorBuf[152]))
        * (g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * 0x80000);

    // for Max Enhance Size
    g_eMMCDrv[emmc_ip].u8_ECSD157_MaxEnhSize_0 = gau8_eMMC_SectorBuf[157];
    g_eMMCDrv[emmc_ip].u8_ECSD158_MaxEnhSize_1 = gau8_eMMC_SectorBuf[158];
    g_eMMCDrv[emmc_ip].u8_ECSD159_MaxEnhSize_2 = gau8_eMMC_SectorBuf[159];

    g_eMMCDrv[emmc_ip].u8_u8_ECSD155_PartSetComplete = gau8_eMMC_SectorBuf[155];
    g_eMMCDrv[emmc_ip].u8_ECSD166_WrRelParam         = gau8_eMMC_SectorBuf[166];

    g_eMMCDrv[emmc_ip].u8_ECSD175_ERASE_GROUP_DEF = gau8_eMMC_SectorBuf[175];

    // for WP
    g_eMMCDrv[emmc_ip].u8_ECSD174_BootWPStatus = gau8_eMMC_SectorBuf[174];
    g_eMMCDrv[emmc_ip].u8_ECSD173_BootWP       = gau8_eMMC_SectorBuf[173];
    g_eMMCDrv[emmc_ip].u8_ECSD171_UserWP       = gau8_eMMC_SectorBuf[171];
    if (g_eMMCDrv[emmc_ip].u8_IfSectorMode)
        g_eMMCDrv[emmc_ip].u32_WP_group_size =
            (g_eMMCDrv[emmc_ip].u8_ECSD221_HCWpGRPSize * g_eMMCDrv[emmc_ip].u8_ECSD224_HCEraseGRPSize * 0x400);
    else
        g_eMMCDrv[emmc_ip].u32_WP_group_size =
            ((g_eMMCDrv[emmc_ip].u8_ERASE_GRP_SIZE + 1) * (g_eMMCDrv[emmc_ip].u8_WP_GRP_SIZE + 1));

    //--------------------------------
    // set HW RST
    if (0 == gau8_eMMC_SectorBuf[162])
    {
        u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 162, BIT0); // RST_FUNC
        if (eMMC_ST_SUCCESS != u32_err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: %Xh, eMMC, set Ext_CSD[162]: %Xh fail\n", u32_err, BIT0);
            return u32_err;
        }
    }

    return eMMC_ST_SUCCESS;
}

U32 eMMC_ModifyExtCSD(eMMC_IP_EmType emmc_ip, U8 u8_AccessMode, U8 u8_ByteIdx, U8 u8_Value)
{
    U32 u32_arg, u32_err;
    U8  u8_retry_prg = 0;

    u32_arg = ((u8_AccessMode & 3) << 24) | (u8_ByteIdx << 16) | (u8_Value << 8);

    u32_err = eMMC_CMD6(emmc_ip, u32_arg);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: eMMC: %Xh \n", u32_err);
        return u32_err;
    }

    do
    {
        if (u8_retry_prg > 5)
            break;

        u32_err = eMMC_CMD13(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);
        if ((eMMC_ST_ERR_R1_7_0 != u32_err) && (eMMC_ST_SUCCESS != u32_err))
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: %Xh \n", u32_err);
            return u32_err;
        }
        else if (eMMC_ST_SUCCESS == u32_err)
            break;

        u8_retry_prg++;
    } while (eMMC_ST_ERR_R1_7_0 == u32_err);

    return u32_err;
}

U32 eMMC_SetPwrOffNotification(eMMC_IP_EmType emmc_ip, U8 u8_SetECSD34)
{
    U32       u32_err;
    static U8 u8_OldECSD34 = 0;

    if (eMMC_PwrOffNotif_SHORT == u8_OldECSD34 || eMMC_PwrOffNotif_LONG == u8_OldECSD34)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Warn: PwrOffNotif already set: %u, now: %u\n", u8_OldECSD34,
                   u8_SetECSD34);
        return eMMC_ST_SUCCESS;
    }

    u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 34, u8_SetECSD34); // PWR_OFF_NOTIFICATION
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_PwrOffNotif_MASK;
    switch (u8_SetECSD34)
    {
        case eMMC_PwrOffNotif_OFF:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_PwrOffNotif_OFF;
            break;
        case eMMC_PwrOffNotif_ON:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_PwrOffNotif_ON;
            break;
        case eMMC_PwrOffNotif_SHORT:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_PwrOffNotif_SHORT;
            break;
        case eMMC_PwrOffNotif_LONG:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_PwrOffNotif_LONG;
            break;
    }

    return u32_err;
}

U32 eMMC_Sanitize(eMMC_IP_EmType emmc_ip, U8 u8_ECSD165)
{
    U32 u32_err;

    u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 165, u8_ECSD165);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    u32_err = eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH << 2);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    return eMMC_ST_SUCCESS;
}

U32 eMMC_SetBusSpeed(eMMC_IP_EmType emmc_ip, U8 u8_BusSpeed)
{
    U32 u32_err;

    g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming &= ~BITS_MSK_TIMING;
    g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming |= u8_BusSpeed;

    u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 185, g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming);
    if (eMMC_ST_SUCCESS != u32_err)
        return u32_err;

    g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_SPEED_MASK;
    switch (u8_BusSpeed)
    {
        case eMMC_SPEED_HIGH:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HIGH;
            break;
        case eMMC_SPEED_HS200:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HS200;
            break;
        case eMMC_SPEED_HS400:
            g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_SPEED_HS400;
            break;
        default:
            break;
    }

    return u32_err;
}

U32 eMMC_SetDrivingStrength(eMMC_IP_EmType emmc_ip, U8 u8Driving)
{
    U32 u32_err;

    g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming &= ~BITS_MSK_DRIVER_STRENGTH;
    g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming |= u8Driving << 4;

    u32_err =
        eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 185, g_eMMCDrv[emmc_ip].u8_ECSD185_HsTiming); // HS_TIMING, HS200
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: fail: %Xh\n", u32_err);
        return eMMC_ST_ERR_SET_DRV_STRENGTH;
    }

    return eMMC_ST_SUCCESS;
}

// Use CMD6 to set ExtCSD[183] BUS_WIDTH
U32 eMMC_SetBusWidth(eMMC_IP_EmType emmc_ip, U8 u8_BusWidth, U8 u8_IfDDR)
{
    U8  u8_value;
    U32 u32_err;

    // -------------------------------
    switch (u8_BusWidth)
    {
        case 1:
            u8_value = 0;
            break;
        case 4:
            u8_value = 1;
            break;
        case 8:
            u8_value = 2;
            break;
        default:
            return eMMC_ST_ERR_PARAMETER;
    }

    if (u8_IfDDR)
    {
        u8_value |= BIT2;
        g_eMMCDrv[emmc_ip].u32_DrvFlag |= DRV_FLAG_DDR_MODE;
    }
    else
    {
        g_eMMCDrv[emmc_ip].u32_DrvFlag &= ~DRV_FLAG_DDR_MODE;
    }
    if (u8_IfDDR == 2 && g_eMMCDrv[emmc_ip].u8_ECSD184_Stroe_Support)
    {
        eMMC_debug(0, 1, "Enhance Strobe\n");
        u8_value |= BIT7; // Enhanced Storbe
    }

    // -------------------------------
    u32_err = eMMC_ModifyExtCSD(emmc_ip, eMMC_ExtCSD_WByte, 183, u8_value); // BUS_WIDTH
    if (eMMC_ST_SUCCESS != u32_err)
    {
        return u32_err;
    }

    // -------------------------------
    g_eMMCDrv[emmc_ip].u16_Reg10_Mode &= ~BIT_SD_DATA_WIDTH_MASK;
    switch (u8_BusWidth)
    {
        case 1:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH    = BIT_SD_DATA_WIDTH_1;
            g_eMMCDrv[emmc_ip].u16_of_buswidth = 1;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode |= BIT_SD_DATA_WIDTH_1;
            break;
        case 4:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH    = BIT_SD_DATA_WIDTH_4;
            g_eMMCDrv[emmc_ip].u16_of_buswidth = 4;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode |= BIT_SD_DATA_WIDTH_4;
            break;
        case 8:
            g_eMMCDrv[emmc_ip].u8_BUS_WIDTH    = BIT_SD_DATA_WIDTH_8;
            g_eMMCDrv[emmc_ip].u16_of_buswidth = 8;
            g_eMMCDrv[emmc_ip].u16_Reg10_Mode |= BIT_SD_DATA_WIDTH_8;
            break;
    }

    // eMMC_debug(eMMC_DEBUG_LEVEL,1,"set %u bus width\n", u8_BusWidth);
    return u32_err;
}

//========================================================
// Send CMD HAL APIs
//========================================================
U32 eMMC_Identify(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err = eMMC_ST_SUCCESS;
    U16 u16_i, u16_retry = 0;

    g_eMMCDrv[emmc_ip].u16_RCA      = 1;
    g_eMMCDrv[emmc_ip].u8_BUS_WIDTH = BIT_SD_DATA_WIDTH_1;
    g_eMMCDrv[emmc_ip].u16_Reg10_Mode &= ~BIT_SD_DATA_WIDTH_MASK;

LABEL_IDENTIFY_CMD0:
    if (eMMC_ST_SUCCESS != eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH))
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: WaitD0High TO\n", emmc_ip);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
    }
    eMMC_RST_L(emmc_ip);
    eMMC_hw_timer_sleep(1);
    eMMC_RST_H(emmc_ip);
    eMMC_hw_timer_sleep(1);

    if (u16_retry > 10)
    {
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        return u32_err;
    }
    if (u16_retry)
        eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC%d Warn: retry: %u\n", emmc_ip, u16_retry);

    // CMD0
    u32_err = eMMC_CMD0(emmc_ip, 0); // reset to idle state
    if (eMMC_ST_SUCCESS != u32_err)
    {
        u16_retry++;
        goto LABEL_IDENTIFY_CMD0;
    }

    // CMD1
    for (u16_i = 0; u16_i < eMMC_CMD1_RETRY_CNT; u16_i++)
    {
        // eMMC_debug(eMMC_DEBUG_LEVEL, 1, "CMD1 try: %02Xh \n", u16_i);
        u32_err = eMMC_CMD1(emmc_ip);
        if (eMMC_ST_SUCCESS == u32_err)
            break;

        eMMC_hw_timer_sleep(2);

        if (eMMC_ST_ERR_CMD1_DEV_NOT_RDY != u32_err)
        {
            u16_retry++;
            goto LABEL_IDENTIFY_CMD0;
        }
    }
    // eMMC_debug(eMMC_DEBUG_LEVEL, 1, "CMD1 try: %02Xh \n", u16_i);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        u16_retry++;
        eMMC_debug(eMMC_DEBUG_LEVEL, 1, "eMMC%d CMD1 wait eMMC device ready timeout\n", emmc_ip);
        goto LABEL_IDENTIFY_CMD0;
    }

    // CMD2
    u32_err = eMMC_CMD2(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        u16_retry++;
        goto LABEL_IDENTIFY_CMD0;
    }

    // CMD3
    u32_err = eMMC_CMD3_CMD7(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA, 3);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        u16_retry++;
        g_eMMCDrv[emmc_ip].u16_RCA++;
        goto LABEL_IDENTIFY_CMD0;
    }

    return eMMC_ST_SUCCESS;
}

//===================================================
#if defined(eMMC_RSP_FROM_RAM) && eMMC_RSP_FROM_RAM
void eMMC_KeepRsp(U8 *pu8_OneRspBuf, U8 u8_CmdIdx)
{
    U16 u16_idx;
    U8  u8_ByteCnt;

    u16_idx    = u8_CmdIdx * eMMC_CMD_BYTE_CNT;
    u8_ByteCnt = eMMC_CMD_BYTE_CNT;

    if (u8_CmdIdx > 10)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 3;
    else if (u8_CmdIdx > 9)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 2;
    else if (u8_CmdIdx > 2)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 1;

    if (10 == u8_CmdIdx || 9 == u8_CmdIdx || 2 == u8_CmdIdx)
        u8_ByteCnt = eMMC_R2_BYTE_CNT;

    if (u16_idx + u8_ByteCnt > eMMC_SECTOR_512BYTE - 4) // last 4 bytes are CRC
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: KeepRsp over 512B, %Xh, %Xh, %Xh\n", emmc_ip, u8_CmdIdx,
                   u16_idx, u8_ByteCnt);
        eMMC_die("\n"); // should be system fatal error, not eMMC driver
    }

    memcpy(g_eMMCDrv[emmc_ip].au8_AllRsp + u16_idx, pu8_OneRspBuf, u8_ByteCnt);
}

U32 eMMC_ReturnRsp(U8 *pu8_OneRspBuf, U8 u8_CmdIdx)
{
    U16 u16_idx;
    U8  u8_ByteCnt;

    u16_idx    = u8_CmdIdx * eMMC_CMD_BYTE_CNT;
    u8_ByteCnt = eMMC_CMD_BYTE_CNT;

    if (u8_CmdIdx > 10)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 3;
    else if (u8_CmdIdx > 9)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 2;
    else if (u8_CmdIdx > 2)
        u16_idx += (eMMC_R2_BYTE_CNT - eMMC_CMD_BYTE_CNT) * 1;

    if (10 == u8_CmdIdx || 9 == u8_CmdIdx || 2 == u8_CmdIdx)
        u8_ByteCnt = eMMC_R2_BYTE_CNT;

    if (u16_idx + u8_ByteCnt > eMMC_SECTOR_512BYTE - 4) // last 4 bytes are CRC
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC%d Err: KeepRsp over 512B, %Xh, %Xh, %Xh\n", emmc_ip, u8_CmdIdx,
                   u16_idx, u8_ByteCnt);
        return eMMC_ST_ERR_NO_RSP_IN_RAM;
    }

    if (0 == g_eMMCDrv[emmc_ip].au8_AllRsp[u16_idx])
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_LOW, 1, "eMMC Info: no rsp, %u %u \n", u8_CmdIdx, u16_idx);
        return eMMC_ST_ERR_NO_RSP_IN_RAM;
    }

    memcpy(pu8_OneRspBuf, g_eMMCDrv[emmc_ip].au8_AllRsp + u16_idx, u8_ByteCnt);

    return eMMC_ST_SUCCESS;
}

// -------------------------------
U32 eMMC_SaveRsp(void)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_SaveDriverContext(void)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_LoadRsp(U8 *pu8_AllRspBuf)
{
    return eMMC_ST_SUCCESS;
}

U32 eMMC_LoadDriverContext(U8 *pu8_Buf)
{
    return eMMC_ST_SUCCESS;
}

#endif

#if defined(eMMC_BURST_LEN_AUTOCFG) && eMMC_BURST_LEN_AUTOCFG
void eMMC_DumpBurstLenTable(eMMC_IP_EmType emmc_ip)
{
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "  eMMC Burst Len: \n");
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "    best:  %4u blks, %4u.%u MB/s \n",
               g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen, g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestMBPerSec,
               g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestMBPerSecPoint);
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "    worst: %4u blks, %4u.%u MB/s \n",
               g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstBrustLen,
               g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstMBPerSec,
               g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstMBPerSecPoint);
}

U32 eMMC_LoadBurstLenTable(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err, u32_ErrRetry = 0;
    U32 u32_ChkSum;

LABEL_RETRY:
    u32_err = eMMC_CMD18(emmc_ip, eMMC_BURST_LEN_BLK_0, gau8_eMMC_SectorBuf, 1);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC WARN: load Burst Len Table fail, %Xh\n", u32_err);
        goto LABEL_END_OF_NO_TABLE;
    }

    memcpy((U8 *)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, gau8_eMMC_SectorBuf, sizeof(g_eMMCDrv[emmc_ip].BurstWriteLen_t));
    u32_ChkSum = eMMC_ChkSum((U8 *)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, sizeof(g_eMMCDrv[emmc_ip].BurstWriteLen_t) - 4);
    if (u32_ChkSum != g_eMMCDrv[emmc_ip].BurstWriteLen_t.u32_ChkSum
        || 0 == g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: no Burst Len Table. ChkSum: %Xh, BestLen: %Xh\n",
                   g_eMMCDrv[emmc_ip].BurstWriteLen_t.u32_ChkSum, g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen);
        u32_err = eMMC_ST_ERR_BLEN_CHKSUM;
        goto LABEL_END_OF_NO_TABLE;
    }

    if (u32_ErrRetry)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL, 0, "eMMC: retry ok\n");
        // eMMC_dump_mem((U8*)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, sizeof(g_eMMCDrv[emmc_ip].BurstWriteLen_t));
    }
    return u32_err;

// ---------------------------------
LABEL_END_OF_NO_TABLE:

    if (0 == u32_ErrRetry)
    {
        // eMMC_dump_mem((U8*)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, sizeof(g_eMMCDrv[emmc_ip].BurstWriteLen_t));
        u32_ErrRetry++;
        goto LABEL_RETRY;
    }
    return u32_err;
}

U32 eMMC_SaveBurstLenTable(eMMC_IP_EmType emmc_ip)
{
    U32 u32_Err;
    U32 u32_t0, u32_t1, u32_tMax = 0, u32_tMin = (U32)(0 - 1), u32_tmp;
    U16 u16_BurstBlkCnt, u16_BestBlkCnt = 0, u16_WorstBlkCnt = 0;
    U16 u16_i;

    // eMMC_debug(eMMC_DEBUG_LEVEL,0,"\neMMC: burst len...\n");
    //  --------------------------
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u32_ChkSum        = ~g_eMMCDrv[emmc_ip].BurstWriteLen_t.u32_ChkSum;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen  = 0;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstBrustLen = 0;

    // eMMC_CMD25(emmc_ip,eMMC_TEST_BLK_0, gau8_eMMC_SectorBuf, eMMC_TEST_BLK_CNT);

    // --------------------------
    u16_BurstBlkCnt = 0;
LABEL_DETECT:
    switch (u16_BurstBlkCnt)
    {
        // case 0:   u16_BurstBlkCnt = 8;  break; // 4KB
        case 0:
            u16_BurstBlkCnt = 16;
            break; // 8KB
        case 16:
            u16_BurstBlkCnt = 32;
            break; // 16KB
        case 32:
            u16_BurstBlkCnt = 64;
            break; // 32KB
        case 64:
            u16_BurstBlkCnt = 128;
            break; // 64KB
        case 128:
            u16_BurstBlkCnt = 256;
            break; // 128KB
        case 256:
            u16_BurstBlkCnt = MAX_DETECT_BLK_CNT;
    }
    eMMC_hw_timer_start();
    u32_t0 = eMMC_hw_timer_tick();
    for (u16_i = 0; u16_i < MAX_DETECT_BLK_CNT / u16_BurstBlkCnt; u16_i++)
    {
        u32_Err = eMMC_CMD25(emmc_ip, eMMC_TEST_BLK_0, gau8_eMMC_SectorBuf, u16_BurstBlkCnt);
        if (eMMC_ST_SUCCESS != u32_Err)
        {
            eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: w fail: %Xh\n", u32_Err);
            goto LABEL_END;
        }
    }
    eMMC_FCIE_WaitD0High(emmc_ip, TIME_WAIT_DAT0_HIGH);
    u32_t1 = eMMC_hw_timer_tick();
    // eMMC_debug(0,0,"\n%u-%u=%8u, %3u ", u32_t1, u32_t0, u32_t1-u32_t0, u16_BurstBlkCnt);

    if (u32_tMin > u32_t1 - u32_t0)
    {
        u32_tMin       = u32_t1 - u32_t0;
        u16_BestBlkCnt = u16_BurstBlkCnt;
    }
    if (u32_tMax < u32_t1 - u32_t0)
    {
        u32_tMax        = u32_t1 - u32_t0;
        u16_WorstBlkCnt = u16_BurstBlkCnt;
    }

    if (MAX_DETECT_BLK_CNT != u16_BurstBlkCnt)
        goto LABEL_DETECT;

    // --------------------------
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestBrustLen  = u16_BestBlkCnt;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstBrustLen = u16_WorstBlkCnt;

    u32_tmp = (U32)(MAX_DETECT_BLK_CNT << eMMC_SECTOR_512BYTE_BITS) * 4000 / (u32_tMin / (eMMC_HW_TIMER_HZ / 4000));
    u32_tmp /= 0x100000 / 10;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestMBPerSec      = u32_tmp / 10;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_BestMBPerSecPoint = u32_tmp % 10;

    u32_tmp = (U32)(MAX_DETECT_BLK_CNT << eMMC_SECTOR_512BYTE_BITS) * 4000 / (u32_tMax / (eMMC_HW_TIMER_HZ / 4000));
    u32_tmp /= 0x100000 / 10;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstMBPerSec      = u32_tmp / 10;
    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u16_WorstMBPerSecPoint = u32_tmp % 10;

    g_eMMCDrv[emmc_ip].BurstWriteLen_t.u32_ChkSum =
        eMMC_ChkSum((U8 *)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, sizeof(g_eMMCDrv[emmc_ip].BurstWriteLen_t) - 4);

    u32_Err = eMMC_CMD25(emmc_ip, eMMC_BURST_LEN_BLK_0, (U8 *)&g_eMMCDrv[emmc_ip].BurstWriteLen_t, 1);
    if (eMMC_ST_SUCCESS != u32_Err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: save fail: %Xh\n", u32_Err);
        goto LABEL_END;
    }

// --------------------------
LABEL_END:
    return u32_Err;
}
#endif

U32 eMMC_GetR1(eMMC_IP_EmType emmc_ip)
{
    return (g_eMMCDrv[emmc_ip].au8_Rsp[1] << 24) | (g_eMMCDrv[emmc_ip].au8_Rsp[2] << 16)
           | (g_eMMCDrv[emmc_ip].au8_Rsp[3] << 8) | g_eMMCDrv[emmc_ip].au8_Rsp[4];
}

#if defined(eMMC_UPDATE_FIRMWARE) && (eMMC_UPDATE_FIRMWARE)

static U32 eMMC_UpFW_Samsung_Wait(eMMC_IP_EmType emmc_ip)
{
    U32 u32_err, u32_cnt, u32_st;

    for (u32_cnt = 0; u32_cnt < UPFW_SEC_WIAT_CNT; u32_cnt++)
    {
        eMMC_CMD13(emmc_ip, g_eMMCDrv[emmc_ip].u16_RCA);
        // eMMC_dump_mem(g_eMMCDrv[emmc_ip].au8_Rsp, 6);
        u32_st = (g_eMMCDrv[emmc_ip].au8_Rsp[1] << 24) | (g_eMMCDrv[emmc_ip].au8_Rsp[2] << 16)
                 | (g_eMMCDrv[emmc_ip].au8_Rsp[3] << 8) | (g_eMMCDrv[emmc_ip].au8_Rsp[4] << 0);
        if (0x900 == u32_st)
            break;
        else
            eMMC_debug(eMMC_DEBUG_LEVEL, 0, "st: %Xh\n", u32_st);
        eMMC_hw_timer_delay(HW_TIMER_DELAY_1ms);
    }

    if (UPFW_SEC_WIAT_CNT == u32_cnt)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: %Xh\n", eMMC_ST_ERR_SEC_UPFW_TO);
        return eMMC_ST_ERR_SEC_UPFW_TO;
    }
    return eMMC_ST_SUCCESS;
}

U32 eMMC_UpFW_Samsung(eMMC_IP_EmType emmc_ip, U8 *pu8_FWBin)
{
    U16 u16_ctrl;
    U32 u32_err, u32_arg = 0xEFAC60FC;

// ---------------------------
LABEL_CMD60:
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "CMD60: %Xh\n", u32_arg);
    u16_ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;

    eMMC_FCIE_ClearEvents(emmc_ip);
    u32_err = eMMC_FCIE_SendCmd(emmc_ip, g_eMMCDrv[emmc_ip].u16_Reg10_Mode, u16_ctrl, u32_arg, 60, eMMC_R1b_BYTE_CNT);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: CMD60 fail, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
    u32_err = eMMC_UpFW_Samsung_Wait(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: timeout 1, Arg:%Xh \n", u32_arg);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
    if (0xEFAC60FC == u32_arg)
    {
        u32_arg = 0xCBAD1160;
        goto LABEL_CMD60;
    }

#if 1
    // ---------------------------
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "erase ... \n");
    g_eMMCDrv[emmc_ip].u32_eMMCFlag &= ~eMMC_FLAG_TRIM;
    u32_err = eMMC_EraseCMDSeq(0, 0);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: erase fail, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
    u32_err = eMMC_UpFW_Samsung_Wait(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: timeout 2, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
#endif

    // ---------------------------
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "write ... \n");
    u32_err = eMMC_CMD25_MIU(emmc_ip, 0, pu8_FWBin, UPFW_SEC_BYTE_CNT >> eMMC_SECTOR_512BYTE_BITS);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: write fail, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
    u32_err = eMMC_UpFW_Samsung_Wait(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: timeout 3, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }

    // ---------------------------
    eMMC_debug(eMMC_DEBUG_LEVEL, 0, "close ... \n");
    u16_ctrl = BIT_SD_CMD_EN | BIT_SD_RSP_EN;

    eMMC_FCIE_ClearEvents(emmc_ip);
    u32_err = eMMC_FCIE_SendCmd(emmc_ip, g_eMMCDrv[emmc_ip].u16_Reg10_Mode, u16_ctrl, 0, 28, eMMC_R1b_BYTE_CNT);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: CMD28 fail, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }
    u32_err = eMMC_UpFW_Samsung_Wait(emmc_ip);
    if (eMMC_ST_SUCCESS != u32_err)
    {
        eMMC_debug(eMMC_DEBUG_LEVEL_ERROR, 1, "eMMC Err: timeout 4, %Xh \n", u32_err);
        eMMC_FCIE_ErrHandler_Stop(emmc_ip);
        goto LABEL_END_OF_UPFW;
    }

LABEL_END_OF_UPFW:
    return u32_err;
}
#endif // eMMC_UPDATE_FIRMWARE
