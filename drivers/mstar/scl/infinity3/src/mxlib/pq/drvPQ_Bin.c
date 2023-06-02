// $Change: 661747 $
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
//==============================================================================
#define _DRV_PQ_BIN_C


#ifdef MSOS_TYPE_LINUX
#include <pthread.h>
#endif

#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif


#include "MsCommon.h"
#include "MsOS.h"
#include "MsTypes.h"
#include "hwreg_utility2.h"
#include "color_reg.h"

#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ_Bin.h"
#include "drvPQ_Datatypes.h"
#include "drvPQ.h"

#include "mhal_pq.h"

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#if(ENABLE_PQ_BIN)

#ifndef UNUSED //to avoid compile warnings...
#define UNUSED(var) (void)((var) = (var))
#endif
#define PQ_BIN_DBG_HEADER_DATA      0
#define PQ_BIN_DBG(x)               //(x)
#define PQ_BIN_DBG_HEADER(x)        //(x)
#define PQ_BIN_DUMP_DBG(x)          //(x)
#define PQ_BIN_DUMP_FILTER_DBG(x)   //(x)
#define PQ_BIN_XRULE_DBG(x)         //(x)
#define PQ_BIN_GRULE_DBG(x)         //(x)
#define PQ_TEXT_BIN_DBG_HEADER(x)   //(x)
#define PQ_TEXT_BIN_DBG(x)          //(x)
#define PQ_BIN_DBG_SRAMERROR(x)     //(x)
#define PQ_BIN_PICTURE(x)           //(x)

#if (PQ_BIN_ENABLE_CHECK == 0)
#define PQ_REG_FUNC( u32Reg, u8Value, u8Mask )    MApi_XC_WriteByteMask( u32Reg, u8Value, u8Mask )
#define PQ_REG_MLOAD_FUNC(u32Reg,u8Value,u8Mask) \
    do{ \
        if(u32Reg%2) \
        { \
            MApi_XC_MLoad_WriteCmd_And_Fire(u32Reg-1, ((MS_U16)u8Value)<<8, ((MS_U16)u8Mask)<<8); \
        } \
        else \
        { \
            MApi_XC_MLoad_WriteCmd_And_Fire(u32Reg, u8Value, u8Mask); \
        } \
    }while(0)

#define PQ_REG_MLOAD_WRITE_CMD(u32Reg,u8Value,u8Mask) \
    do{ \
        if(u32Reg%2) \
        { \
            _u32MLoadCmd[_u16MLoadCmdCnt] = u32Reg-1; \
            _u16MLoadMsk[_u16MLoadCmdCnt] = ((MS_U16)u8Mask)<<8; \
            _u16MLoadVal[_u16MLoadCmdCnt] = ((MS_U16)u8Value)<<8; \
        } \
        else \
        { \
           _u32MLoadCmd[_u16MLoadCmdCnt] = u32Reg; \
            _u16MLoadMsk[_u16MLoadCmdCnt] = ((MS_U16)u8Mask); \
            _u16MLoadVal[_u16MLoadCmdCnt] = ((MS_U16)u8Value); \
        } \
        _u16MLoadCmdCnt++; \
    }while(0)

#else // #if(PQ_ENABLE_CHECK == 1)
static MS_U8 _u8PQBinfunction = PQ_FUNC_DUMP_REG;
static void _MDrv_PQBin_SetFunction(MS_U8 u8Func)
{
    _u8PQBinfunction = u8Func;
}

#define PQ_REG_FUNC( u16Reg, u8Value, u8Mask ) \
    do{ \
        if (_u8PQBinfunction == PQ_FUNC_DUMP_REG) \
        { \
            MApi_XC_WriteByteMask( (MS_U32)u16Reg, u8Value, u8Mask ); \
        } \
        else \
        { \
            if ((MApi_XC_ReadByte((MS_U32)u16Reg) & u8Mask) != (u8Value & u8Mask)) \
            { \
                printf("[PQRegErr] "); \
                if ((u16Reg >> 8) == 0x2F) \
                { \
                    printf("bk=%02x, ", (MS_U16)SC_BK_CURRENT); \                } \
                else if ((u16Reg >> 8) == 0x36) \
                { \
                    printf("bk=%02x, ", (MS_U16)COMB_BK_CURRENT); \                } \
                printf("addr=%04x, mask=%02x, val=%02x[%02x]\n", \                    u16Reg, (MS_U16)u8Mask, (MS_U16)MApi_XC_ReadByte((MS_U32)u16Reg), (MS_U16)u8Value); \
            } \        } \
    }while(0)
#endif  //#if (PQ_ENABLE_CHECK)

//-------------------------------------------------------------------------------------------------
//  Local Variable
//-------------------------------------------------------------------------------------------------
static MS_U16 _u16PQTabIdx[PQ_MAX_WINDOW][PQ_BIN_MAX_PNL][PQ_BIN_MAX_IP];    // store all TabIdx of all IPs
static MS_U8 _gPQBinDisplayType[PQ_MAX_WINDOW];
static MS_U8 _gPQBinPnlIdx[PQ_MAX_WINDOW];
static MS_BOOL _bMLoadEn = FALSE;
#define MLOAD_MAX_CMD   30
static MS_U32 _u32MLoadCmd[MLOAD_MAX_CMD];
static MS_U16 _u16MLoadMsk[MLOAD_MAX_CMD];
static MS_U16 _u16MLoadVal[MLOAD_MAX_CMD];
static MS_U16 _u16MLoadCmdCnt = 0;

static MS_U16 u16ICC_CRD_Table[PQ_MAX_WINDOW];
static MS_U16 u16IHC_CRD_Table[PQ_MAX_WINDOW];

static MS_U16 u16SRAM1Table[PQ_MAX_WINDOW];
static MS_U16 u16SRAM2Table[PQ_MAX_WINDOW];
static MS_U16 u16SRAM3Table[PQ_MAX_WINDOW];
static MS_U16 u16SRAM4Table[PQ_MAX_WINDOW];
static MS_U16 u16CSRAM1Table[PQ_MAX_WINDOW];
static MS_U16 u16CSRAM2Table[PQ_MAX_WINDOW];
static MS_U16 u16CSRAM3Table[PQ_MAX_WINDOW];
static MS_U16 u16CSRAM4Table[PQ_MAX_WINDOW];

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
void MDrv_PQBin_Clear_SRAM_Table_Index(void)
{
    MS_U8 i;

    for(i=0;i<PQ_MAX_WINDOW; i++)
    {
        u16ICC_CRD_Table[i]=0xFFFF;
        u16IHC_CRD_Table[i]=0xFFFF;
        u16SRAM1Table[i]=0xFFFF;
        u16SRAM2Table[i]=0xFFFF;
        u16SRAM3Table[i]=0xFFFF;
        u16SRAM4Table[i]=0xFFFF;
        u16CSRAM1Table[i]=0xFFFF;
        u16CSRAM2Table[i]=0xFFFF;
        u16CSRAM3Table[i]=0xFFFF;
        u16CSRAM4Table[i]=0xFFFF;
    }

}



PQ_WIN MDrv_PQBin_TransToPQWin(MS_U8 u8BinID)
{
    PQ_WIN ePQWin;
    if(u8BinID == PQ_BIN_STD_MAIN || u8BinID == PQ_BIN_EXT_MAIN)
    {
        ePQWin = PQ_MAIN_WINDOW;
    }
#if PQ_ENABLE_PIP
    else if(u8BinID == PQ_BIN_STD_SUB || u8BinID == PQ_BIN_EXT_SUB)
    {
        ePQWin = PQ_SUB_WINDOW;
    }
#endif
#if ENABLE_MULTI_SCALER
    else if(u8BinID == PQ_BIN_STD_SC1_MAIN || u8BinID == PQ_BIN_EXT_SC1_MAIN)
    {
        ePQWin = PQ_SC1_MAIN_WINDOW;
    }
    else if(u8BinID == PQ_BIN_STD_SC2_MAIN || u8BinID == PQ_BIN_EXT_SC2_MAIN)
    {
        ePQWin = PQ_SC2_MAIN_WINDOW;
    }
    else if(u8BinID == PQ_BIN_STD_SC2_SUB || u8BinID == PQ_BIN_EXT_SC2_SUB)
    {
        ePQWin = PQ_SC2_SUB_WINDOW;
    }
#endif
    else
    {
        ePQWin = PQ_MAIN_WINDOW;
        printf("MDrv_PQBin_TransToPQWin: Error ID %d \n", u8BinID);
    }

    return ePQWin;
}


MS_U8 MDrv_PQBin_GetByteData(void *pAddrVirt, MS_U32 *u32Offset)
{
    MS_U8 u8ret = *((MS_U8 *)pAddrVirt);
    *u32Offset = *u32Offset + 1;

    return u8ret;
}

MS_U16 MDrv_PQBin_Get2ByteData(void *pAddrVirt, MS_U32 *u32Offset)
{    MS_U16 u16ret = (((MS_U16)*((MS_U8 *)pAddrVirt+0)) << 0) |
                    (((MS_U16)*((MS_U8 *)pAddrVirt+1)) << 8);

    *u32Offset = *u32Offset + 2;


    return u16ret;
}


MS_U32 MDrv_PQBin_Get4ByteData(void *pAddrVirt, MS_U32 *u32Offset)
{
    MS_U32 u32ret = (((MS_U32)*((MS_U8 *)pAddrVirt+0)) << 0)  |
                    (((MS_U32)*((MS_U8 *)pAddrVirt+1)) << 8)  |
                    (((MS_U32)*((MS_U8 *)pAddrVirt+2)) << 16) |
                    (((MS_U32)*((MS_U8 *)pAddrVirt+3)) << 24);

    *u32Offset = *u32Offset + 4;

    return u32ret;
}




void MDrv_PQBin_DumpGeneralRegTable(const MS_PQBin_IP_Table_Info* const pTabInfo)
{
    MS_U8  u8Bank;
    MS_U8  u8Addr;
    MS_U8  u8Mask;
    MS_U8  u8Value;
    MS_U16 i;
    MS_U32 u32Addr, u32Offset, u32RegAddr;

    PQ_BIN_DUMP_DBG(printf("tab: general\n"));
    if (pTabInfo->u16GroupIdx >= pTabInfo->u16GroupNum)
    {
        PQ_BIN_DUMP_DBG(printf("[PQBin]IP_Info error: General Reg Table\n"));
        return;
    }

    u32Addr = pTabInfo->u32TabOffset;
    for(i=0; i< pTabInfo->u16RegNum; i++)
    {
        u32Offset = ( PQ_BIN_BANK_SIZE + PQ_BIN_ADDR_SIZE + PQ_BIN_MASK_SIZE + (MS_U32)pTabInfo->u16GroupNum) * (MS_U32)i;
        u8Bank = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Addr = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Mask = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        u32Offset +=  pTabInfo->u16GroupIdx;
        u8Value = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        PQ_BIN_DBG_SRAMERROR(printf("4  [read addr=%lx], GroupIdx=%x,RegNum=%x,reg=%04x, msk=%02x, val=%02x, u8CurBank=%x\n",
					(u32Addr+u32Offset-(MS_U32)pTabInfo->u16GroupIdx),pTabInfo->u16GroupIdx,pTabInfo->u16RegNum,NON_PM_BASE | (((MS_U16)u8Bank) << 8) | (MS_U16)u8Addr, u8Mask, u8Value, u8Bank);)

        u32RegAddr = NON_PM_BASE | (((MS_U16)u8Bank) << 8) | (MS_U16)u8Addr;

        PQ_BIN_DUMP_DBG(printf("[addr=%04lx, msk=%02x, val=%02x]\n", u32RegAddr, u8Mask, u8Value));

        PQ_REG_FUNC(u32RegAddr, u8Value, u8Mask);
    }
}

void MDrv_PQBin_DumpCombRegTable(const MS_PQBin_IP_Table_Info* const pTabInfo)
{
    MS_U16 i;
    MS_U8 u8Addr;
    MS_U8 u8Mask;
    MS_U8 u8Value;
    MS_U8 u8CurBank = 0xff;
    MS_U32 u32Addr, u32Offset, u32RegAddr;

    COMB_BK_STORE;

    PQ_BIN_DUMP_DBG(printf("tab: comb\n"));

    if(pTabInfo->u16GroupIdx >= pTabInfo->u16GroupNum)
    {
        PQ_BIN_DUMP_DBG(printf("[PQBin]IP_Info error: Comb Reg Table\n"));
        return;
    }

    u32Addr = pTabInfo->u32TabOffset;
    for(i=0; i< pTabInfo->u16RegNum; i++)
    {
        u32Offset = ( PQ_BIN_BANK_SIZE + PQ_BIN_ADDR_SIZE + PQ_BIN_MASK_SIZE + (MS_U32)pTabInfo->u16GroupNum) * (MS_U32)i;
        u8CurBank = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Addr    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Mask    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        u32Offset +=  pTabInfo->u16GroupIdx;
        u8Value = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        PQ_BIN_DBG_SRAMERROR(printf("2  [read addr=%lx], GroupIdx=%x,RegNum=%x,reg=%04lx, msk=%02x, val=%02x, u8CurBank=%x\n",
					(u32Addr+u32Offset-(MS_U32)pTabInfo->u16GroupIdx),pTabInfo->u16GroupIdx,pTabInfo->u16RegNum,COMB_REG_BASE | (MS_U32)u8Addr, u8Mask, u8Value, u8CurBank);)

        if (u8CurBank != COMB_BK_CURRENT)
        {
            PQ_BIN_DUMP_DBG(printf("<<bankswitch=%02x>>\n", u8CurBank));
            COMB_BK_SWITCH(u8CurBank);
        }

        u32RegAddr = COMB_REG_BASE | (MS_U16)u8Addr;

        PQ_BIN_DUMP_DBG(printf("[addr=%04lx, msk=%02x, val=%02x]\n", u32RegAddr, u8Mask, u8Value));
        PQ_REG_FUNC(u32RegAddr, u8Value, u8Mask);

    }
    COMB_BK_RESTORE;
}


void MDrv_PQBin_DumpScalerRegTable(const MS_PQBin_IP_Table_Info* const pTabInfo)
{
    MS_U32 u32RegAddr;
    MS_U8 u8Mask;
    MS_U8 u8Addr;
    MS_U8 u8Value;
    MS_U8 u8CurBank = 0xff;
    MS_U32 u32Addr, u32Offset;
    MS_U16 i;

#ifdef MSOS_TYPE_LINUX
#if(ENABLE_PQ_MLOAD)
    pthread_mutex_lock(&_PQ_MLoad_Mutex);
#endif
#endif
    //consider dump reg amount is very large, so we shouldn't use MApi_XC_W2BYTE(),
    //  because mutex cost much time.
    SC_BK_STORE_MUTEX;

    PQ_BIN_DUMP_DBG(printf("tab: sc\n"));
    if(pTabInfo->u16GroupIdx >= pTabInfo->u16GroupNum)
    {
        PQ_BIN_DBG(printf("[PQBin]IP_Info error: Scaler Reg Table\n"));

        SC_BK_RESTORE_MUTEX;

#ifdef MSOS_TYPE_LINUX
#if(ENABLE_PQ_MLOAD)
        pthread_mutex_unlock(&_PQ_MLoad_Mutex);
#endif
#endif

        return;
    }

    u32Addr = pTabInfo->u32TabOffset;
    for(i=0; i<pTabInfo->u16RegNum; i++)
    {
        u32Offset = ( PQ_BIN_BANK_SIZE + PQ_BIN_ADDR_SIZE + PQ_BIN_MASK_SIZE + (MS_U32)pTabInfo->u16GroupNum) * (MS_U32)i;
        u8CurBank = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Addr    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Mask    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        u32Offset +=  (MS_U32)pTabInfo->u16GroupIdx;
        u8Value = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        PQ_BIN_DBG_SRAMERROR(printf("1  [read addr=%lx], GroupIdx=%x,RegNum=%x,reg=%04lx, msk=%02x, val=%02x, u8CurBank=%x\n",
            (u32Addr+u32Offset-(MS_U32)pTabInfo->u16GroupIdx),pTabInfo->u16GroupIdx,pTabInfo->u16RegNum,BK_SCALER_BASE | (MS_U32)u8Addr, u8Mask, u8Value, u8CurBank);)

        if(_bMLoadEn)
        {
            u32RegAddr = (((MS_U32)u8CurBank) << 8) | (MS_U32)u8Addr;
            //PQ_REG_MLOAD_FUNC(u32RegAddr, u8Value, u8Mask);
            PQ_REG_MLOAD_WRITE_CMD(u32RegAddr, u8Value, u8Mask);
        }
        else
        {
#if (SCALER_REGISTER_SPREAD)
            u32RegAddr = BK_SCALER_BASE | (((MS_U32)u8CurBank) << 8) | (MS_U32)u8Addr;
#else
            if (u8CurBank != SC_BK_CURRENT)
            {
                PQ_BIN_DUMP_DBG(printf("<<bankswitch=%02x>>\n", u8CurBank));
                SC_BK_SWITCH(u8CurBank);
            }

            u32RegAddr = BK_SCALER_BASE | (MS_U32)u8Addr;
#endif
            PQ_REG_FUNC(u32RegAddr, u8Value, u8Mask);
        }
        PQ_BIN_DUMP_DBG(printf("[addr=%04lx, msk=%02x, val=%02x]\n", u32RegAddr, u8Mask, u8Value));

    }

    SC_BK_RESTORE_MUTEX;

#ifdef MSOS_TYPE_LINUX
#if(ENABLE_PQ_MLOAD)
    pthread_mutex_unlock(&_PQ_MLoad_Mutex);
#endif
#endif
}

void MDrv_PQBin_DumpFilterTable(MS_PQBin_IP_Table_Info *pTabInfo)
{
    MS_U32 u32Addr;
    PQ_BIN_DBG(printf("tab: sram\n"));

    if(pTabInfo->u16GroupIdx >= pTabInfo->u16GroupNum)
    {
        PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]IP_Info error: SRAM Table\n"));
        return;
    }

    u32Addr = pTabInfo->u32TabOffset + pTabInfo->u16GroupIdx * pTabInfo->u16RegNum;

    PQ_BIN_DUMP_FILTER_DBG(printf("PQ Filter Table: Win:%d, Type:%d \n", pTabInfo->ePQWin, pTabInfo->u16TableType));
    switch(pTabInfo->u16TableType)
    {
    case PQ_TABTYPE_SRAM1:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM1, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM2:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM2, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM3:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM3, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM4:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM4, (void *)u32Addr);
        break;
    case PQ_TABTYPE_C_SRAM1:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_C_SRAM1, (void *)u32Addr);
        break;
    case PQ_TABTYPE_C_SRAM2:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_C_SRAM2, (void *)u32Addr);
        break;
    case PQ_TABTYPE_C_SRAM3:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_C_SRAM3, (void *)u32Addr);
        break;
    case PQ_TABTYPE_C_SRAM4:
        Hal_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_C_SRAM4, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM_COLOR_INDEX:
        Hal_PQ_set_sram_color_index_table(pTabInfo->ePQWin, SC_FILTER_SRAM_COLOR_INDEX, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM_COLOR_GAIN_SNR:
        Hal_PQ_set_sram_color_gain_snr_table(pTabInfo->ePQWin, SC_FILTER_SRAM_COLOR_GAIN_SNR, (void *)u32Addr);
        break;
    case PQ_TABTYPE_SRAM_COLOR_GAIN_DNR:
        Hal_PQ_set_sram_color_gain_dnr_table(pTabInfo->ePQWin, SC_FILTER_SRAM_COLOR_GAIN_DNR, (void *)u32Addr);
        break;
    case PQ_TABTYPE_VIP_ICC_CRD_SRAM:
        Hal_PQ_set_sram_icc_crd_table(pTabInfo->ePQWin, SC_FILTER_SRAM_ICC_CRD, (void *)u32Addr);
        break;
    case PQ_TABTYPE_VIP_IHC_CRD_SRAM:
        Hal_PQ_set_sram_ihc_crd_table(pTabInfo->ePQWin, SC_FILTER_SRAM_IHC_CRD, (void *)u32Addr);
        break;

    default:
        printf("[PQBin] Unknown sram type %u\n", pTabInfo->u16TableType);
        MS_ASSERT(0);
        break;
    }
}

void MDrv_PQBin_DumpTable(MS_PQBin_IP_Table_Info* pTabInfo)
{
    // to save loading SRAM table time, SRAM are only downloaded
    // when current SRAM table is different to previous SRAM table

    if ((pTabInfo->u16RegNum == 0) || (pTabInfo->u32TabOffset == PQ_BIN_ADDR_NULL))
    {
        PQ_BIN_DUMP_DBG(printf("NULL Table\n"));
        return;
    }

    switch(pTabInfo->u16TableType)
    {
    case PQ_TABTYPE_SCALER:
        MDrv_PQBin_DumpScalerRegTable(pTabInfo);
        break;
    case PQ_TABTYPE_COMB:
        MDrv_PQBin_DumpCombRegTable(pTabInfo);
        break;
    case PQ_TABTYPE_GENERAL:
        MDrv_PQBin_DumpGeneralRegTable(pTabInfo);
        break;
    case PQ_TABTYPE_SRAM1:
        if (u16SRAM1Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  sram1: %u, new  sram1: %u\n",
                u16SRAM1Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16SRAM1Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  sram1: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_SRAM2:
        if (u16SRAM2Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  sram2: %u, new  sram2: %u\n",
                u16SRAM2Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16SRAM2Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  sram2: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_SRAM3:
        if (u16SRAM3Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  sram3: %u, new  sram3: %u\n",
                u16SRAM3Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16SRAM3Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  sram3: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_SRAM4:
        if (u16SRAM4Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  sram4: %u, new  sram4: %u\n",
                u16SRAM2Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16SRAM4Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  sram4: %u\n", pTabInfo->u16GroupIdx));
        }
        break;

    case PQ_TABTYPE_C_SRAM1:
        if (u16CSRAM1Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  sram1: %u, new  Csram1: %u\n",
                u16CSRAM1Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16CSRAM1Table [pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  Csram1: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_C_SRAM2:
        if (u16CSRAM2Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  Csram2: %u, new  Csram2: %u\n",
                u16CSRAM2Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16CSRAM2Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  Csram2: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_C_SRAM3:
        if (u16SRAM3Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old  Csram3: %u, new  Csram3: %u\n",
                u16CSRAM3Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16CSRAM3Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same Csram3: %u\n", pTabInfo->u16GroupIdx));
        }
        break;
    case PQ_TABTYPE_C_SRAM4:
        if (u16CSRAM4Table[pTabInfo->ePQWin] != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old sram4: %u, new  Csram4: %u\n",
                u16CSRAM2Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16CSRAM4Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        else
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]use the same  Csram4: %u\n", pTabInfo->u16GroupIdx));
        }
        break;

    case PQ_TABTYPE_VIP_IHC_CRD_SRAM:
        if(u16IHC_CRD_Table[pTabInfo->ePQWin]  != pTabInfo->u16GroupIdx)
        {
            PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]old IHC_CRD_SRAM: %u, new IHC_CRD_SRAM: %u\n",
                u16IHC_CRD_Table[pTabInfo->ePQWin], pTabInfo->u16GroupIdx));

            u16IHC_CRD_Table[pTabInfo->ePQWin] = pTabInfo->u16GroupIdx;
            MDrv_PQBin_DumpFilterTable(pTabInfo);
        }
        break;

    default:
        PQ_BIN_DUMP_FILTER_DBG(printf("[PQBin]DumpTable:unknown type: %u\n", pTabInfo->u16TableType));
        break;
    }
}


// return total IP count
MS_U16 MDrv_PQBin_GetIPNum(MS_PQBin_Header_Info* pPQBinHeader)
{
    PQ_BIN_DBG(printf("[PQBin]:IPNum=%u\n", pPQBinHeader->u16IP_Comm_Num));
    return pPQBinHeader->u16IP_Comm_Num;
}

// return total table count of given IP
MS_U16 MDrv_PQBin_GetTableNum(MS_U16 u16PQIPIdx, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U16 u16TabNum;

    u32Addr = pPQBinHeader->u32BinStartAddress + pPQBinHeader->u32IP_Comm_Offset;
    u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_IP_GROUP_NUM_OFFSET;
    u16TabNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
    PQ_BIN_DBG(printf("[PQBin]TabNum=%u\n", u16TabNum));
    return u16TabNum;
}

// return current used table index of given IP
MS_U16 MDrv_PQBin_GetCurrentTableIndex(MS_U16 u16PnlIdx, MS_U16 u16PQIPIdx, MS_U8 u8BinID)
{
    PQ_WIN ePQWin = MDrv_PQBin_TransToPQWin(u8BinID);
    //printf("[PQBin]CurrTableIdx=%d\n", _u16PQTabIdx[u16PnlIdx][u16PQIPIdx]);

    return _u16PQTabIdx[ePQWin][u16PnlIdx][u16PQIPIdx];
}


MS_U16 MDrv_PQBin_GetTableIndex(MS_U16 u16PQSrcType, MS_U16 u16PQIPIdx, MS_U16 u16PQPnlIdx, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U32 u32SourceLUT_Addr;
    MS_U32 u32SourceLUT_Offset;
    MS_U16 u16SourceLUT_SourceNum;
    MS_U16 u16SourceLUT_IPNum;
    MS_U32 u32SourceLUT_Data_Pitch;
    MS_U16 u16TableIdx;

    if(u16PQPnlIdx >= pPQBinHeader->u16SourceLUT_PnlNum)
    {
        (printf("[PQBin]invalid panel type\n"));
        return PQ_BIN_IP_NULL;
    }


    u32Addr = pPQBinHeader->u32BinStartAddress + pPQBinHeader->u32SourceLUT_Offset;
    u32Offset =  pPQBinHeader->u32SourceLUT_Pitch * u16PQPnlIdx + PQ_BIN_SOURCELUT_SOURCE_NUM_OFFSET;
    u16SourceLUT_SourceNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
    if (u16PQSrcType >= u16SourceLUT_SourceNum)
    {
        PQ_BIN_DBG(printf("[PQBin]invalid input type\n"));
        return PQ_BIN_IP_NULL;
    }

    u32Offset =  pPQBinHeader->u32SourceLUT_Pitch * u16PQPnlIdx + PQ_BIN_SOURCELUT_IP_NUM_OFFSET;
    u16SourceLUT_IPNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
    if (u16PQIPIdx >= u16SourceLUT_IPNum)
    {
        PQ_BIN_DBG(printf("[PQBin]invalid ip type\n"));
        return PQ_BIN_IP_NULL;
    }


    u32Offset =  pPQBinHeader->u32SourceLUT_Pitch * u16PQPnlIdx + PQ_BIN_SOURCELUT_OFFSET_OFFSET;
    u32SourceLUT_Addr = MDrv_PQBin_Get4ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

    u32SourceLUT_Addr += pPQBinHeader->u32BinStartAddress  + PQ_BIN_HEADER_LEN;
    u32SourceLUT_Data_Pitch = (MS_U32)u16SourceLUT_IPNum * 2;
    u32SourceLUT_Offset =  u32SourceLUT_Data_Pitch * (MS_U32)u16PQSrcType +
                          (MS_U32)u16PQIPIdx * 2;

    u16TableIdx = MDrv_PQBin_Get2ByteData((void *)(u32SourceLUT_Addr+u32SourceLUT_Offset),
                                          &u32SourceLUT_Offset);
    PQ_BIN_DBG(printf("[PQBin]TableId=%u\n",u16TableIdx));
    return  u16TableIdx;
}

void MDrv_PQBin_GetTable(MS_U16 u16PnlIdx, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx, MS_PQBin_IP_Table_Info *pTableInfo, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    PQ_WIN ePQWin = MDrv_PQBin_TransToPQWin(pPQBinHeader->u8BinID);
    if((u16PnlIdx >= PQ_BIN_MAX_PNL) || (u16PQIPIdx >= PQ_BIN_MAX_IP))
    {
        printf("[PQBin]:Out Of Range!! PnlIdx=%u, IPIdx=%u\n", u16PnlIdx, u16PQIPIdx);
        MS_ASSERT(0);
        return;
    }
    _u16PQTabIdx[ePQWin][u16PnlIdx][u16PQIPIdx] = u16TabIdx;

    if (u16TabIdx != PQ_BIN_IP_NULL)
    {
        u32Addr   = pPQBinHeader->u32IP_Comm_Offset +
                    pPQBinHeader->u32BinStartAddress;

        if(u16TabIdx == PQ_BIN_IP_COMM)
        {
            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_TABLE_TYPE_OFFSET;
            pTableInfo->u16TableType = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_COMM_REG_NUM_OFFSET;
            pTableInfo->u16RegNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_COMMOFFSET_OFFSET;
            pTableInfo->u32TabOffset = MDrv_PQBin_Get4ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
            pTableInfo->u32TabOffset += (pPQBinHeader->u32BinStartAddress + PQ_BIN_HEADER_LEN);

            pTableInfo->u16GroupNum = 1;
            pTableInfo->u16GroupIdx = 0;
        }
        else
        {
            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_TABLE_TYPE_OFFSET;
            pTableInfo->u16TableType = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_IP_REG_NUM_OFFSET;
            pTableInfo->u16RegNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_IP_GROUP_NUM_OFFSET;
            pTableInfo->u16GroupNum = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

            u32Offset = pPQBinHeader->u32IP_Comm_Pitch * (MS_U32)u16PQIPIdx + PQ_BIN_IP_COMM_IPOFFSET_OFFSET;
            pTableInfo->u32TabOffset = MDrv_PQBin_Get4ByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
            pTableInfo->u32TabOffset += (pPQBinHeader->u32BinStartAddress + PQ_BIN_HEADER_LEN);

            pTableInfo->u16GroupIdx = u16TabIdx;
        }
    }
    else
    {
        pTableInfo->u16GroupIdx  = 0;
        pTableInfo->u32TabOffset = PQ_BIN_ADDR_NULL;
        pTableInfo->u16RegNum    = 0;
        pTableInfo->u16TableType = 0;
        pTableInfo->u16GroupNum  = 0;
    }

    pTableInfo->ePQWin = MDrv_PQBin_TransToPQWin(pPQBinHeader->u8BinID);

    PQ_BIN_DBG(printf("[PQBin]: GetTable: Offset=%lx, RegNum=%u, GroupIdx=%u, GroupNum=%u, Type=%u eWin=%d\n",
        pTableInfo->u32TabOffset,
        pTableInfo->u16RegNum,
        pTableInfo->u16GroupIdx,
        pTableInfo->u16GroupNum,
        pTableInfo->u16TableType,
        pTableInfo->ePQWin));
}

MS_BOOL MDrv_PQBin_LoadPictureSetting(
    MS_U16 u16PnlIdx,
    MS_U16 u16TabIdx,
    MS_U16 u16PQIPIdx,
    MS_PQBin_Header_Info *pPQBinHeader,
    void *pTable,
    MS_U16 u16TableSize)
{
    MS_PQBin_IP_Table_Info stTableInfo;
    MS_U32 u32Addr = 0;

    MsOS_Memset(&stTableInfo, 0, sizeof(MS_PQBin_IP_Table_Info));

    MDrv_PQBin_GetTable(u16PnlIdx,
                        u16TabIdx,
                        u16PQIPIdx,
                        &stTableInfo,
                        pPQBinHeader);

    PQ_BIN_PICTURE(printf("PQ_Bin: Picture: pnlidx:%d, TabIdx:%d, ipidx:%d \r\n", u16PnlIdx, u16TabIdx, u16PQIPIdx));
    PQ_BIN_PICTURE(printf("PQ_Bin: Picture: RegNum:%d, TabType:%d, GroupIdx:%d \r\n", stTableInfo.u16RegNum, stTableInfo.u16TableType, stTableInfo.u16GroupIdx));
    if(stTableInfo.u16GroupIdx >= stTableInfo.u16GroupNum)
    {
        PQ_BIN_DBG(printf("[PQBin]IP_Info error: LoadTable Data\n"));
        return FALSE;
    }

    if(u16TableSize > stTableInfo.u16RegNum)
    {
        PQ_BIN_DBG(printf("[PQBin]IP_Info error: LoadTable Data is too big\n"));
        return FALSE;
    }

    if(stTableInfo.u16TableType == PQ_TABTYPE_PICTURE_1)
    {
        u32Addr = stTableInfo.u32TabOffset + stTableInfo.u16RegNum * stTableInfo.u16GroupIdx * sizeof(MS_U8);
        MsOS_Memcpy((MS_U8 *)pTable, (MS_U8 *)u32Addr, sizeof(MS_U8)*stTableInfo.u16RegNum);
    }
    else if(stTableInfo.u16TableType == PQ_TABTYPE_PICTURE_2)
    {
        u32Addr = stTableInfo.u32TabOffset + stTableInfo.u16RegNum * stTableInfo.u16GroupIdx * sizeof(MS_U16);
        MsOS_Memcpy((MS_U16 *)pTable, (MS_U16 *)u32Addr, sizeof(MS_U16)*stTableInfo.u16RegNum);
    }
    else
    {
        return FALSE;
    }


#if 0
{
    MS_U32 u32Offset = 0;
    for(u32Offset=0; u32Offset<u16TableSize; u32Offset++)
    {
        if(stTableInfo.u16TableType == PQ_TABTYPE_PICTURE_1)
            printf("Pic:%02d, %02x \r\n", (int)u32Offset, ((MS_U8 *)pTable)[u32Offset]);
        else
            printf("Pic:%02d, %04x \r\n", (int)u32Offset, ((MS_U16 *)pTable)[u32Offset]);
    }
}
#endif

    return TRUE;
}

void MDrv_PQBin_LoadTableData(
    MS_U16 u16PnlIdx,
    MS_U16 u16TabIdx,
    MS_U16 u16PQIPIdx,
    MS_PQBin_Header_Info *pPQBinHeader,
    MS_U8 *pTable,
    MS_U16 u16TableSize)
{
    MS_PQBin_IP_Table_Info stTableInfo;
    MS_U8 u8Mask = 0;
    MS_U8 u8Addr = 0;
    MS_U8 u8Value = 0;
    MS_U8 u8CurBank = 0xff;
    MS_U32 u32Addr = 0, u32Offset = 0;
    MS_U16 i = 0;


    MsOS_Memset(&stTableInfo, 0, sizeof(MS_PQBin_IP_Table_Info));

    MDrv_PQBin_GetTable(u16PnlIdx,
                        u16TabIdx,
                        u16PQIPIdx,
                        &stTableInfo,
                        pPQBinHeader);



    PQ_BIN_DUMP_DBG(printf("LoadTableData: \n"));
    if(stTableInfo.u16GroupIdx >= stTableInfo.u16GroupNum)
    {
        PQ_BIN_DBG(printf("[PQBin]IP_Info error: LoadTable Data\n"));
        return;
    }

    if(u16TableSize > stTableInfo.u16RegNum)
    {
        PQ_BIN_DBG(printf("[PQBin]IP_Info error: LoadTable Data is too big\n"));
        return;
    }
    u32Addr = stTableInfo.u32TabOffset;
    for(i=0; i<stTableInfo.u16RegNum; i++)
    {
        u32Offset = ( PQ_BIN_BANK_SIZE + PQ_BIN_ADDR_SIZE + PQ_BIN_MASK_SIZE + (MS_U32)stTableInfo.u16GroupNum) * (MS_U32)i;
        u8CurBank = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Addr    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);
        u8Mask    = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        u32Offset +=  (MS_U32)stTableInfo.u16GroupIdx;
        u8Value = MDrv_PQBin_GetByteData((void *)((void *)(u32Addr+u32Offset)), &u32Offset);

        pTable[i] = u8Value;
    }
}




void MDrv_PQBin_LoadTable(
    MS_U16 u16PnlIdx,
    MS_U16 u16TabIdx,
    MS_U16 u16PQIPIdx,
    MS_PQBin_Header_Info *pPQBinHeader)
{
    MS_PQBin_IP_Table_Info stTableInfo;
    PQ_BIN_DBG_SRAMERROR(printf("---------MDrv_PQBin_LoadTable: u16TabIdx=%u, u16PQIPIdx=%u\n", u16TabIdx, u16PQIPIdx);)
    MDrv_PQBin_GetTable(u16PnlIdx,
                        u16TabIdx,
                        u16PQIPIdx,
                        &stTableInfo,
                        pPQBinHeader);

    MDrv_PQBin_DumpTable(&stTableInfo);
}

void MDrv_PQBin_LoadCommTable(MS_U16 u16PnlIdx, MS_PQBin_Header_Info *pPQBinHeader)
{
    MS_U16 i;
    MS_PQBin_IP_Table_Info stTableInfo;


    PQ_BIN_DBG(printf("[PQBin]LoadCommTable\n"));
    for (i=0; i< pPQBinHeader->u16IP_Comm_Num; i++)
    {
        PQ_BIN_DBG_SRAMERROR(printf("--------MDrv_PQBin_LoadCommTable: u16TabIdx=%u, u16PQIPIdx=%u\n", PQ_BIN_IP_COMM, i);)
        MDrv_PQBin_GetTable(u16PnlIdx,
                            PQ_BIN_IP_COMM,
                            i,
                            &stTableInfo,
                            pPQBinHeader);

        if(((stTableInfo.u16TableType >= PQ_TABTYPE_SRAM1) && (stTableInfo.u16TableType <= PQ_TABTYPE_C_SRAM1)) ||
            (stTableInfo.u16TableType == PQ_TABTYPE_VIP_IHC_CRD_SRAM) ||
            (stTableInfo.u16TableType == PQ_TABTYPE_VIP_ICC_CRD_SRAM)
           )
        {
           continue;
        }

        MDrv_PQBin_DumpTable(&stTableInfo);
    }
}



void MDrv_PQBin_Parsing(MS_PQBin_Header_Info *pPQBinHeader)
{
    MS_U32 u32PQBin_Addr;
    MS_U32 u32Offset;
#if PQ_BIN_DBG_HEADER_DATA
    MS_U32 u32tmpoffset;
#endif
    MS_U8  i;

    { // for printf
        pPQBinHeader->u8Header[PQ_BIN_HEADER_LEN] = '\0';
        pPQBinHeader->u8Version[PQ_BIN_VERSION_LEN] = '\0';
        pPQBinHeader->u8Dummy[PQ_BIN_DUMMY] = '\0';
        pPQBinHeader->u8GRuleHader[PQ_BIN_HEADER_LEN] = '\0';
        pPQBinHeader->u8XRuleHader[PQ_BIN_HEADER_LEN] = '\0';
        pPQBinHeader->u8SkipRuleHader[PQ_BIN_HEADER_LEN] = '\0';
        pPQBinHeader->u8IP_Comm_Header[PQ_BIN_HEADER_LEN] = '\0';
        pPQBinHeader->u8SourceLUT_Header[PQ_BIN_HEADER_LEN] = '\0';
    }


    u32Offset = 0;
    u32PQBin_Addr = pPQBinHeader->u32BinStartAddress;
   //printf("\n BinStartaDDR ID%u=%lx \n", pPQBinHeader->u8BinID, u32PQBin_Addr);
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8Header[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8Version[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }
    pPQBinHeader->u32StartOffset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    pPQBinHeader->u32EndOffset   = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    for(i=0; i< PQ_BIN_DUMMY; i++)
    {
        pPQBinHeader->u8Dummy[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    pPQBinHeader->u8PQID = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    PQ_BIN_DBG_HEADER(printf("Header :%s \n", pPQBinHeader->u8Header));
    PQ_BIN_DBG_HEADER(printf("Version:%s \n", pPQBinHeader->u8Version));
    PQ_BIN_DBG_HEADER(printf("startADDR=%lx, EndAddr=%lx \n", pPQBinHeader->u32StartOffset, pPQBinHeader->u32EndOffset));
    PQ_BIN_DBG_HEADER(printf("Dummy :%s \n", pPQBinHeader->u8Dummy));
    PQ_BIN_DBG_HEADER(printf("PQID :%d \n", pPQBinHeader->u8PQID));
    //-------------------------------------------------------------------------------------------
    // GRule
    //
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8GRuleHader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    pPQBinHeader->u16GRule_RuleNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    pPQBinHeader->u16GRule_PnlNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);


    PQ_BIN_DBG_HEADER(printf("GRuleHeader:%s\n", pPQBinHeader->u8GRuleHader));
    PQ_BIN_DBG_HEADER(printf("GRule: RuleNum=%u, PnlNum=%u \n", pPQBinHeader->u16GRule_RuleNum, pPQBinHeader->u16GRule_PnlNum));

    pPQBinHeader->u32GRule_Offset = u32Offset;
    pPQBinHeader->u32GRule_Pitch = PQ_BIN_GRULE_INFO_SIZE;
    u32Offset += pPQBinHeader->u32GRule_Pitch * (MS_U32)pPQBinHeader->u16GRule_RuleNum;


    pPQBinHeader->u32GRule_Lvl_Offset = u32Offset;
    pPQBinHeader->u32GRule_Lvl_Pitch = PQ_BIN_GRULE_LEVEL_INFO_SIZE;
    u32Offset += pPQBinHeader->u32GRule_Lvl_Pitch * (MS_U32)pPQBinHeader->u16GRule_RuleNum * (MS_U32)pPQBinHeader->u16GRule_PnlNum;

    PQ_BIN_DBG_HEADER(printf("GRule Rule : offset=%lx, pitch=%lu \n",  pPQBinHeader->u32GRule_Offset,  pPQBinHeader->u32GRule_Pitch));
    PQ_BIN_DBG_HEADER(printf("GRule Level: offset=%lx, pitch=%lu \n", pPQBinHeader->u32GRule_Lvl_Offset, pPQBinHeader->u32GRule_Lvl_Pitch));
    PQ_BIN_DBG_HEADER(printf("XRule Start:%lx\n", u32Offset));

    //-------------------------------------------------------------------------------------------
    // XRule
    //
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8XRuleHader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }
    pPQBinHeader->u16XRuleNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    PQ_BIN_DBG_HEADER(printf("XRuleHeader:%s\n", pPQBinHeader->u8XRuleHader));
    PQ_BIN_DBG_HEADER(printf("XRule: RuleNum=%u\n", pPQBinHeader->u16XRuleNum));

    pPQBinHeader->u32XRule_Offset = u32Offset;
    pPQBinHeader->u32XRUle_Pitch  = PQ_BIN_XRULE_INFO_SIZE;
    u32Offset += pPQBinHeader->u32XRUle_Pitch * (MS_U32)pPQBinHeader->u16XRuleNum;

    PQ_BIN_DBG_HEADER(printf("XRule: offset=%lx, pitch=%lu\n",pPQBinHeader->u32XRule_Offset, pPQBinHeader->u32XRUle_Pitch));
    PQ_BIN_DBG_HEADER(printf("SkipRule Start:%lx\n", u32Offset));

    //-------------------------------------------------------------------------------------------
    // SkipRule
    //
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8SkipRuleHader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    pPQBinHeader->u16SkipRule_IPNum  = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    pPQBinHeader->u32SkipRule_Offset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    PQ_BIN_DBG_HEADER(printf("SkipRuleHeader:%s IpNum=%u, Offset=%lx\n", pPQBinHeader->u8SkipRuleHader, pPQBinHeader->u16SkipRule_IPNum, pPQBinHeader->u32SkipRule_Offset));
    PQ_BIN_DBG_HEADER(printf("IP_Comm Start:%lx\n", u32Offset));

    //-------------------------------------------------------------------------------------------
    // IP & Common
    //
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8IP_Comm_Header[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    pPQBinHeader->u16IP_Comm_Num = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    PQ_BIN_DBG_HEADER(printf("IP_CommHeader:%s\n", pPQBinHeader->u8IP_Comm_Header));
    PQ_BIN_DBG_HEADER(printf("IP_Comm, Num=%u\n", pPQBinHeader->u16IP_Comm_Num));

    pPQBinHeader->u32IP_Comm_Offset = u32Offset;
    pPQBinHeader->u32IP_Comm_Pitch = PQ_BIN_IP_COMM_INFO_SIZE;
    u32Offset += pPQBinHeader->u32IP_Comm_Pitch  * (MS_U32)pPQBinHeader->u16IP_Comm_Num;

    PQ_BIN_DBG_HEADER(printf("IP_Comm, Offset=%lx, pitch=%lu\n", pPQBinHeader->u32IP_Comm_Offset, pPQBinHeader->u32IP_Comm_Pitch));
    PQ_BIN_DBG_HEADER(printf("SourceLUT Start:%lx\n", u32Offset));
    //-------------------------------------------------------------------------------------------
    // SourceLUT
    //
    for(i=0; i< PQ_BIN_HEADER_LEN; i++)
    {
        pPQBinHeader->u8SourceLUT_Header[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
    }

    pPQBinHeader->u16SourceLUT_PnlNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

    PQ_BIN_DBG_HEADER(printf("SourceLUTHeader:%s\n", pPQBinHeader->u8SourceLUT_Header));
    PQ_BIN_DBG_HEADER(printf("SourceLUT, PnlNum=%u\n", pPQBinHeader->u16SourceLUT_PnlNum));

    pPQBinHeader->u32SourceLUT_Offset = u32Offset;
    pPQBinHeader->u32SourceLUT_Pitch = PQ_BIN_SOURCELUT_INFO_SIZE;
    u32Offset += pPQBinHeader->u32SourceLUT_Pitch * (MS_U32)pPQBinHeader->u16SourceLUT_PnlNum;

    PQ_BIN_DBG_HEADER(printf("SourceLUT: offset=%lx, pitch=%lu\n", pPQBinHeader->u32SourceLUT_Offset, pPQBinHeader->u32SourceLUT_Pitch));


#if PQ_BIN_DBG_HEADER_DATA
    // GRule
    u32tmpoffset = pPQBinHeader->u32GRule_Offset;
    for(i=0; i<pPQBinHeader->u16GRule_RuleNum; i++)
    {

        MS_PQBin_GRule_Info stGRuleInfo;

        stGRuleInfo.u16IPNum      = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stGRuleInfo.u16SourceNum  = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stGRuleInfo.u16GroupNum   = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stGRuleInfo.u32IPOffset   = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stGRuleInfo.u32RuleOffset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);

        printf("GRule %u, IPNum=%u, SourceNum=%u, GroupNum=%u, IPOffset=%lx, RuleOffset=%lx\n",
            i,
            stGRuleInfo.u16IPNum,
            stGRuleInfo.u16SourceNum,
            stGRuleInfo.u16GroupNum,
            stGRuleInfo.u32IPOffset,
            stGRuleInfo.u32RuleOffset);
    }

    // GRule Level
    for(i=0; i<pPQBinHeader->u16GRule_RuleNum*pPQBinHeader->u16GRule_PnlNum; i++)
    {
        MS_PQBin_GRule_Level_Info stGRuleLvlInfo;

        stGRuleLvlInfo.u16LvlNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stGRuleLvlInfo.u32Offset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);

        printf("GRule_Lvl %u, u16LvlNum=%u, u32Offset=%lx \n",
            i, stGRuleLvlInfo.u16LvlNum, stGRuleLvlInfo.u32Offset);
    }

    // XRule
    u32tmpoffset = pPQBinHeader->u32XRule_Offset;
    for(i=0; i<pPQBinHeader->u16XRuleNum; i++)
    {
        MS_PQBin_XRule_Info stXRuleInfo;

        stXRuleInfo.u16IPNum       = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stXRuleInfo.u16GroupNum    = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stXRuleInfo.u32IPOffset    = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stXRuleInfo.u32GroupOffset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);

        printf("XRule %u, IPNum=%u, GroupNum=%u, IPOffset=%lx, GroupOffset=%lx\n",
            i, stXRuleInfo.u16IPNum, stXRuleInfo.u16GroupNum, stXRuleInfo.u32IPOffset, stXRuleInfo.u32GroupOffset);
    }

    // IP & Common
    u32tmpoffset = pPQBinHeader->u32IP_Comm_Offset;
    for(i=0; i<pPQBinHeader->u16IP_Comm_Num; i++)
    {
        MS_PQBin_IP_Common_Info stIPCommInfo;

        stIPCommInfo.u16CommRegNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stIPCommInfo.u16IPRegNum   = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stIPCommInfo.u16IPGroupNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stIPCommInfo.u32CommOffset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stIPCommInfo.u32IPOffset   = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);

        printf("IP_Comm %u, CommRegNum=%u, IPRegNum=%u, IPGroupNum=%u, CommOffset=%lx, IPOffset=%lx\n",
            i,
            stIPCommInfo.u16CommRegNum,
            stIPCommInfo.u16IPRegNum,
            stIPCommInfo.u16IPGroupNum,
            stIPCommInfo.u32CommOffset,
            stIPCommInfo.u32IPOffset);
    }

    // SourceLUT
    u32tmpoffset = pPQBinHeader->u32SourceLUT_Offset;
    for(i=0; i<pPQBinHeader->u16SourceLUT_PnlNum; i++)
        {
        MS_PQBin_SourceLUT_Info stSourceLUTInfo;

        stSourceLUTInfo.u16SourceNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stSourceLUTInfo.u16IPNum     = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);
        stSourceLUTInfo.u32Offset    = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32tmpoffset), &u32tmpoffset);

        printf("SourceLUT %u, SourceNum=%u, IPNum=%u, Offset=%lx\n",
            i,
            stSourceLUTInfo.u16SourceNum,
            stSourceLUTInfo.u16IPNum,
            stSourceLUTInfo.u32Offset);

    }
#endif

}


MS_U16 MDrv_PQBin_GetSkipRule(MS_U16 u16PQIPIdx, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U16 u16SkipRet;

    u32Addr = pPQBinHeader->u32SkipRule_Offset +
              pPQBinHeader->u32BinStartAddress +
              PQ_BIN_SKIPRULE_HEADER_LEN;

    u32Offset = u16PQIPIdx * 2;
    u16SkipRet = MDrv_PQBin_Get2ByteData((void *)((void *)(u32Addr+u32Offset)) , &u32Offset);
    return u16SkipRet;
}

void MDrv_PQBin_LoadTableBySrcType(
    MS_U16 u16PQSrcType,
    MS_U16 u16PQIPIdx,
    MS_U16 u16PQPnlIdx,
    MS_PQBin_Header_Info *pPQBinHeader)
{
    MS_U16 QMIPtype_size,i;
    MS_U16 u16TabIdx;
    MS_PQBin_IP_Table_Info stTableInfo;
    //XC_ApiStatusEx stXCStatusEx; Ryan

    if (u16PQIPIdx==PQ_BIN_IP_ALL)
    {
        QMIPtype_size= MDrv_PQBin_GetIPNum(pPQBinHeader);
        u16PQIPIdx=0;
    }
    else
    {
        QMIPtype_size=1;
    }


    for(i=0; i<QMIPtype_size; i++, u16PQIPIdx++)
    {
        if ( MDrv_PQBin_GetSkipRule(u16PQIPIdx, pPQBinHeader))
        {
            PQ_BIN_DBG(printf("skip ip idx:%u\n", u16PQIPIdx));
            continue;
        }
    #if 0//Ryan
        MsOS_Memset(&stXCStatusEx, 0, sizeof(XC_ApiStatusEx));
        stXCStatusEx.u16ApiStatusEX_Length = sizeof(XC_ApiStatusEx);
        stXCStatusEx.u32ApiStatusEx_Version = API_STATUS_EX_VERSION;
        if(MApi_XC_GetStatusEx(&stXCStatusEx, MAIN_WINDOW) == stXCStatusEx.u16ApiStatusEX_Length)
        {
            //if customer scaling, and now it's going to dump HSD_Y and HSD_C, just skip it.
            if((!stXCStatusEx.bPQSetHSD) &&
                ((u16PQIPIdx == PQ_IP_HSD_C_Main) || (u16PQIPIdx == PQ_IP_HSD_Y_Main)))
            {
                PQ_BIN_DBG(printf("skip ip idx:%u because of customer scaling case \n", u16PQIPIdx));
                continue;
            }
        }
    #endif
        u16TabIdx = MDrv_PQBin_GetTableIndex(u16PQSrcType,
                                             u16PQIPIdx,
                                             u16PQPnlIdx,
                                             pPQBinHeader);

        PQ_BIN_DBG(printf("[PQ]SrcType=%u, IPIdx=%u, TabIdx=%u\n",
            u16PQSrcType, u16PQIPIdx, u16TabIdx));
        PQ_BIN_DBG_SRAMERROR(printf("----------MDrv_PQBin_LoadTableBySrcType:u16TabIdx=%u, u16PQIPIdx=%u\n", u16TabIdx, u16PQIPIdx);)

        MDrv_PQBin_GetTable(u16PQPnlIdx,
                            u16TabIdx,
                            u16PQIPIdx,
                            &stTableInfo,
                            pPQBinHeader);

        MDrv_PQBin_DumpTable(&stTableInfo);
    }
}


MS_U16 MDrv_PQBin_GetXRuleTableIndex(MS_U16 u16XRuleType, MS_U16 u16XRuleIdx, MS_U16 u16XRuleIP, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U32 u32Rule_Addr;
    MS_U32 u32Rule_Offset;
    MS_U16 u16IPNum;
    MS_U16 u16TableIdx;

    u32Addr = pPQBinHeader->u32XRule_Offset + pPQBinHeader->u32BinStartAddress;

    u32Offset = pPQBinHeader->u32XRUle_Pitch * u16XRuleType + PQ_BIN_XRULE_IP_NUM_OFFSET;
    u16IPNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    u32Offset = pPQBinHeader->u32XRUle_Pitch * u16XRuleType + PQ_BIN_XRULE_GROUPOFFSET_OFFSET;
    u32Rule_Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                   pPQBinHeader->u32BinStartAddress;


    u32Rule_Offset = (u16IPNum * u16XRuleIdx + u16XRuleIP ) * 2;
    u16TableIdx = MDrv_PQBin_Get2ByteData((void *)(u32Rule_Addr+u32Rule_Offset), &u32Rule_Offset);

    PQ_BIN_XRULE_DBG(printf("[PQBin_XRule]: TableIdx=%u\n", u16TableIdx));
    return u16TableIdx;

}

MS_U16 MDrv_PQBin_GetXRuleIPIndex(MS_U16 u16XRuleType, MS_U16 u16XRuleIP, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U32 u32IP_Addr;
    MS_U32 u32IP_Offset;
    MS_U16 u16IPIdx;

    u32Addr = pPQBinHeader->u32XRule_Offset + pPQBinHeader->u32BinStartAddress;
    u32Offset = pPQBinHeader->u32XRUle_Pitch * u16XRuleType + PQ_BIN_XRULE_IPOFFSET_OFFSET;

    u32IP_Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQBinHeader->u32BinStartAddress + PQ_BIN_XRULE_HEADER_LEN;

    u32IP_Offset =  u16XRuleIP * 2;

    u16IPIdx = MDrv_PQBin_Get2ByteData((void *)(u32IP_Addr+u32IP_Offset), &u32IP_Offset);

    PQ_BIN_XRULE_DBG(printf("[PQBin_XRule]: IPIdx=%u\n", u16IPIdx));
    return u16IPIdx;

}

MS_U16 MDrv_PQBin_GetXRuleIPNum(MS_U16 u16XRuleType, MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr;
    MS_U32 u32Offset;
    MS_U16 u16IPNum;

    u32Addr = pPQBinHeader->u32XRule_Offset + pPQBinHeader->u32BinStartAddress;
    u32Offset = pPQBinHeader->u32XRUle_Pitch * u16XRuleType + PQ_BIN_XRULE_IP_NUM_OFFSET;
    u16IPNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    PQ_BIN_XRULE_DBG(printf("[PQBin_XRule]: IPNum=%u\n", u16IPNum));
    return u16IPNum;
}



MS_U16 MDrv_PQBin_GetGRule_LevelIndex(
    MS_U16 u16PnlIdx,
    MS_U16 u16GRuleType,
    MS_U16 u16GRuleLvlIndex,
    MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr, u32Offset, u32Lvl_Addr;
    MS_U16 u16LvlNum;
    MS_U16 u16LvlIdx;

    if(u16PnlIdx >= pPQBinHeader->u16GRule_PnlNum)
    {
        MS_ASSERT(0);
        printf("GRue: Pnl idx out of rage =%u\n", u16PnlIdx);
        return PQ_BIN_IP_NULL;
    }

    u32Addr = pPQBinHeader->u32GRule_Lvl_Offset + pPQBinHeader->u32BinStartAddress;

    u32Offset = pPQBinHeader->u32GRule_Lvl_Pitch * (MS_U32) (u16PnlIdx + pPQBinHeader->u16GRule_PnlNum * u16GRuleType) +
                PQ_BIN_GRULE_LEVEL_LVL_NUM_OFFSET;
    u16LvlNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    if(u16GRuleLvlIndex >= u16LvlNum)
    {
        MS_ASSERT(0);
        printf("GRule: lvl idx out of rage =%u\n", u16GRuleLvlIndex);
        return PQ_BIN_IP_NULL;
    }

    u32Addr = pPQBinHeader->u32GRule_Lvl_Offset + pPQBinHeader->u32BinStartAddress;
    u32Offset = pPQBinHeader->u32GRule_Lvl_Pitch * (MS_U32) (u16PnlIdx + pPQBinHeader->u16GRule_PnlNum * u16GRuleType) +
                PQ_BIN_GRULE_LEVEL_OFFSET_OFFSET;

    u32Lvl_Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    u32Addr = u32Lvl_Addr + pPQBinHeader->u32BinStartAddress;
    u32Offset = PQ_BIN_GRULE_HEADER_LEN + u16GRuleLvlIndex * 2;

    u16LvlIdx = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    PQ_BIN_GRULE_DBG(printf("[PQBin_GRule: LvlIdx=%u\n", u16LvlIdx));
    return u16LvlIdx;
}


MS_U16 MDrv_PQBin_GetGRule_IPIndex(
    MS_U16 u16GRuleType,
    MS_U16 u16GRuleIPIndex,
    MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    MS_U32 u32IP_Addr, u32IP_Offset;
    MS_U16 u16IPIdx;

    u32Addr = pPQBinHeader->u32GRule_Offset + pPQBinHeader->u32BinStartAddress;
    u32Offset = pPQBinHeader->u32GRule_Pitch * u16GRuleType +
                PQ_BIN_GRULE_IPOFFSET_OFFSET;

    u32IP_Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQBinHeader->u32BinStartAddress + PQ_BIN_GRULE_HEADER_LEN;
    u32IP_Offset = u16GRuleIPIndex * 2;
    u16IPIdx = MDrv_PQBin_Get2ByteData((void *)(u32IP_Addr+u32IP_Offset), &u32IP_Offset);

    PQ_BIN_GRULE_DBG(printf("[PQBin_GRule: IPIdx=%u\n", u16IPIdx));
    return u16IPIdx;
}

MS_U16 MDrv_PQBin_GetGRule_TableIndex(
    MS_U16 u16GRuleType,
    MS_U16 u16PQSrcType,
    MS_U16 u16PQ_NRIdx,
    MS_U16 u16GRuleIPIndex,
    MS_PQBin_Header_Info* pPQBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    MS_U32 u32Rule_Addr, u32Rule_Offset;
    MS_U16 u16GroupNum, u16IPNum, u16SrcNum;
    MS_U16 u16TableIdx;

    if(u16GRuleType >= pPQBinHeader->u16GRule_RuleNum)
    {
        MS_ASSERT(0);
        printf("GRule: ruleid out of range=%u\n", u16GRuleType);
        return PQ_BIN_IP_NULL;
    }

    u32Addr = pPQBinHeader->u32GRule_Offset + pPQBinHeader->u32BinStartAddress;

    u32Offset = pPQBinHeader->u32GRule_Pitch * u16GRuleType + PQ_BIN_GRULE_GROUP_NUM_OFFSET;
    u16GroupNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    u32Offset = pPQBinHeader->u32GRule_Pitch * u16GRuleType + PQ_BIN_GRULE_IP_NUM_OFFSET;
    u16IPNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    u32Offset = pPQBinHeader->u32GRule_Pitch * u16GRuleType + PQ_BIN_GRULE_SOURCE_NUM_OFFSET;
    u16SrcNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);



    if(u16PQSrcType >= u16SrcNum)
    {
        MS_ASSERT(0);
        printf("GRule: SrcIdx out of range =%u\n", u16PQSrcType);
        return PQ_BIN_IP_NULL;
    }

    if(u16PQ_NRIdx >= u16GroupNum)
    {
        MS_ASSERT(0);
        printf("GRule: GroupIdx out of range =%u\n", u16PQ_NRIdx);
        return PQ_BIN_IP_NULL;
    }

    if(u16GRuleIPIndex >= u16IPNum)
    {
        MS_ASSERT(0);
        printf("GRule: IPIdx out of range =%u\n", u16GRuleIPIndex);
        return PQ_BIN_IP_NULL;
    }


    u32Offset = pPQBinHeader->u32GRule_Pitch * u16GRuleType + PQ_BIN_GRULE_RULEOFFSET_OFFSET;
    u32Rule_Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                   pPQBinHeader->u32BinStartAddress + PQ_BIN_GRULE_HEADER_LEN;


    u32Rule_Offset = (u16IPNum * u16GroupNum * u16PQSrcType  +
                      u16IPNum * u16PQ_NRIdx +
                      u16GRuleIPIndex) * 2;
    u16TableIdx = MDrv_PQBin_Get2ByteData((void *)(u32Rule_Addr+u32Rule_Offset), &u32Rule_Offset);
    PQ_BIN_GRULE_DBG(printf("[PQBin_GRule: TabIdx=%u\n", u16TableIdx));
    return u16TableIdx;
}


void MDrv_PQBin_CheckCommTable(MS_U16 u16PnlIdx, MS_PQBin_Header_Info * pPQBinHeader)
{
#if (PQ_ENABLE_CHECK == 1)
    _u8PQBinfunction = PQ_FUNC_CHK_REG;
    MDrv_PQBin_LoadCommTable(u16PnlIdx, pPQBinHeader);
    _u8PQBinfunction = PQ_FUNC_DUMP_REG;
#else
    UNUSED(u16PnlIdx);
    UNUSED(pPQBinHeader);
#endif
}

void MDrv_PQBin_CheckTableBySrcType(
    MS_U16 u16PQSrcType,
    MS_U16 u16PQIPIdx,
    MS_U16 u16PQPnlIdx,
    MS_PQBin_Header_Info * pPQBinHeader)
{
#if (PQ_ENABLE_CHECK == 1)
    _u8PQBinfunction = PQ_FUNC_CHK_REG;
    MDrv_PQBin_LoadTableBySrcType(u16PQSrcType,
                                  u16PQIPIdx,
                                  u16PQPnlIdx,
                                  pPQBinHeader);
    _u8PQBinfunction = PQ_FUNC_DUMP_REG;
#else
    UNUSED(u16PQSrcType);
    UNUSED(u16PQIPIdx);
    UNUSED(u16PQPnlIdx);
    UNUSED(pPQBinHeader);
#endif
}

void MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_TYPE enDisplayType, PQ_WIN ePQWin)
{
    _gPQBinDisplayType[ePQWin] = enDisplayType;
}

PQ_BIN_DISPLAY_TYPE MDrv_PQBin_GetDisplayType(PQ_WIN ePQWin)
{
    return (PQ_BIN_DISPLAY_TYPE)_gPQBinDisplayType[ePQWin];
}

void MDrv_PQBin_SetPanelID(MS_U8 u8PnlIDx, PQ_WIN ePQWin)
{
    _gPQBinPnlIdx[ePQWin] = u8PnlIDx;
}

MS_U8 MDrv_PQBin_GetPanelIdx(PQ_WIN ePQWin)
{
#if (PQ_ENABLE_PIP)
    if(ePQWin == PQ_SC1_MAIN_WINDOW)
        return _gPQBinPnlIdx[ePQWin] * 1 + _gPQBinDisplayType[ePQWin]; // SC1 only has 720 panel
    else
        return _gPQBinPnlIdx[ePQWin] * PQ_BIN_DISPLAY_NUM + _gPQBinDisplayType[ePQWin];
#else
    return _gPQBinPnlIdx[ePQWin] * 1 + _gPQBinDisplayType[ePQWin];
#endif
}

void MDrv_PQBin_Set_MLoadEn(MS_BOOL bEn)
{
    MS_U16 i;

    if(bEn == DISABLE)
    {
        //if spread reg, no need to use mutex, but it's ok to use mutex
        //   (because it's not MApi_XC_W2BYTE(), which has mutex already)
        //if not spread reg, must use mutex to protect MLoad_trigger func.
        SC_BK_STORE_MUTEX;

        if(_u16MLoadCmdCnt)
        {
            for(i=1; i<_u16MLoadCmdCnt; i++)
            {
                if(_u32MLoadCmd[i-1] == _u32MLoadCmd[i])
                {
                    _u16MLoadMsk[i] |= _u16MLoadMsk[i-1];
                    _u16MLoadVal[i] |= _u16MLoadVal[i-1];
                }
            }
        #if(ENABLE_PQ_MLOAD)
            MApi_XC_MLoad_WriteCmds_And_Fire(
                &_u32MLoadCmd[0], &_u16MLoadVal[0], &_u16MLoadMsk[0], _u16MLoadCmdCnt);
        #endif

            _u16MLoadCmdCnt = 0;
        }

        SC_BK_RESTORE_MUTEX;
    }

    _bMLoadEn = bEn;
}

//------------------------------------------------------------------------------
// Text Bin function
//------------------------------------------------------------------------------
MS_BOOL MDrv_PQTextBin_Parsing(MS_PQTextBin_Header_Info *pPQTextBinHeader)
{
        MS_U32 u32PQBin_Addr;
        MS_U32 u32Offset;
#if PQ_BIN_DBG_HEADER_DATA
        MS_U32 u32tmpoffset;
#endif
        MS_U8  i;

        u32Offset = 0;
        u32PQBin_Addr = pPQTextBinHeader->u32BinStartAddress;
        printf("TextBinStartaDDR ID%u=%lx\n", pPQTextBinHeader->u8BinID, u32PQBin_Addr);
        for(i=0; i< PQ_TEXT_BIN_HEADER_LEN; i++)
        {
            pPQTextBinHeader->u8Header[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }

        for(i=0; i< PQ_TEXT_BIN_HEADER_LEN; i++)
        {
            pPQTextBinHeader->u8Version[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }


        pPQTextBinHeader->u32StartOffset = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        pPQTextBinHeader->u32EndOffset   = MDrv_PQBin_Get4ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

        for(i=0; i< PQ_TEXT_BIN_DUMMY; i++)
        {
            pPQTextBinHeader->u8Dummy[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }

        PQ_TEXT_BIN_DBG_HEADER(printf("Header :%s\n", pPQTextBinHeader->u8Header));
        PQ_TEXT_BIN_DBG_HEADER(printf("Version:%s\n", pPQTextBinHeader->u8Version));
        PQ_TEXT_BIN_DBG_HEADER(printf("startADDR=%lx, pPQTextBinHeader=%lx\n", pPQTextBinHeader->u32StartOffset, pPQTextBinHeader->u32EndOffset));
        PQ_TEXT_BIN_DBG_HEADER(printf("Dummy :%s \n", pPQTextBinHeader->u8Dummy));


        if(pPQTextBinHeader->u8Header[0] != 0x43 ||  pPQTextBinHeader->u8Header[0] != 0x33 ||
           pPQTextBinHeader->u8Dummy[0] != 0xA5 || pPQTextBinHeader->u8Dummy[0] != 0x59)
        {
            printf("PQ Parsing Fail \n");
            return FALSE;
        }


        // SrcType
        for(i=0; i< PQ_TEXT_BIN_HEADER_LEN; i++)
        {
            pPQTextBinHeader->u8SrcTypeHeader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }
        pPQTextBinHeader->u16SrcNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        PQ_TEXT_BIN_DBG_HEADER(printf("SrcTypeHeader:%s\n", pPQTextBinHeader->u8SrcTypeHeader));
        PQ_TEXT_BIN_DBG_HEADER(printf("SrcType: RuleNum=%u\n", pPQTextBinHeader->u16SrcNum));

        pPQTextBinHeader->u32SrcType_Offset = u32Offset;
        pPQTextBinHeader->u32SrcType_Pitch  = PQ_TEXT_BIN_SRCTYPE_INFO_SIZE;
        u32Offset += pPQTextBinHeader->u32SrcType_Pitch * (MS_U32)pPQTextBinHeader->u16SrcNum;

        PQ_TEXT_BIN_DBG_HEADER(printf("SrcType: offset=%lx, pitch=%lu\n",pPQTextBinHeader->u32SrcType_Offset, pPQTextBinHeader->u32SrcType_Pitch));
        PQ_TEXT_BIN_DBG_HEADER(printf("IPName Start:%lx\n", u32Offset));

        // IP Name
        for(i=0; i< PQ_TEXT_BIN_HEADER_LEN; i++)
        {
            pPQTextBinHeader->u8IPNameHeader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }
        pPQTextBinHeader->u16IPNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

        PQ_TEXT_BIN_DBG_HEADER(printf("IPNameHeader:%s\n", pPQTextBinHeader->u8IPNameHeader));
        PQ_TEXT_BIN_DBG_HEADER(printf("IPName: IPNum=%u\n", pPQTextBinHeader->u16IPNum));

        pPQTextBinHeader->u32IPName_Offset = u32Offset;
        pPQTextBinHeader->u32IPName_Pitch  = PQ_TEXT_BIN_IPNAME_INFO_SIZE;
        u32Offset += pPQTextBinHeader->u32IPName_Pitch * (MS_U32)pPQTextBinHeader->u16IPNum;

        PQ_TEXT_BIN_DBG_HEADER(printf("IPName: offset=%lx, pitch=%lu\n",pPQTextBinHeader->u32IPName_Offset, pPQTextBinHeader->u32IPName_Pitch));
        PQ_TEXT_BIN_DBG_HEADER(printf("GroupName Start:%lx\n", u32Offset));

        // Group Name
        for(i=0; i< PQ_TEXT_BIN_HEADER_LEN; i++)
        {
            pPQTextBinHeader->u8GroupNameHeader[i] = MDrv_PQBin_GetByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);
        }
        pPQTextBinHeader->u16GroupIPNum = MDrv_PQBin_Get2ByteData((void *)(u32PQBin_Addr+u32Offset), &u32Offset);

        PQ_TEXT_BIN_DBG_HEADER(printf("GroupName:%s\n", pPQTextBinHeader->u8GroupNameHeader));
        PQ_TEXT_BIN_DBG_HEADER(printf("GroupName: IPNum=%u\n", pPQTextBinHeader->u16GroupIPNum));

        pPQTextBinHeader->u32IP_Group_Offset = u32Offset;
        pPQTextBinHeader->u32IP_Group_pitch = PQ_TEXT_BIN_GROUPNAME_INFO_SIZE;
        u32Offset += pPQTextBinHeader->u32IP_Group_pitch * (MS_U32)pPQTextBinHeader->u16GroupIPNum;

        PQ_TEXT_BIN_DBG_HEADER(printf("IPName: offset=%lx, pitch=%lu\n",pPQTextBinHeader->u32IP_Group_Offset, pPQTextBinHeader->u32IP_Group_pitch));
        PQ_TEXT_BIN_DBG_HEADER(printf("Group Start:%lx\n", u32Offset));

        return TRUE;

}

MS_U32 MDrv_PQTextBin_GetSrcType(MS_U16 u16SrcIdx, MS_PQTextBin_Header_Info* pPQTextBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    MS_U32 u32DstAddr;

    if(u16SrcIdx >= pPQTextBinHeader->u16SrcNum)
    {
        MS_ASSERT(0);
        printf("[PQBinText] GetSrcType:: SrcIdx Out of range %u\n", u16SrcIdx);
        u16SrcIdx = 0;
    }
    u32Addr   = pPQTextBinHeader->u32SrcType_Offset +  pPQTextBinHeader->u32BinStartAddress;
    u32Offset = pPQTextBinHeader->u32SrcType_Pitch * u16SrcIdx +
                PQ_TEXT_BIN_SRCTYPE_OFFSET_OFFSET;

    u32DstAddr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQTextBinHeader->u32BinStartAddress;

    PQ_TEXT_BIN_DBG(printf("PQTextBin SrcType: Addr%lx, SrcIdx%u\n", u32DstAddr, u16SrcIdx));
    return u32DstAddr;
}

MS_U32 MDrv_PQTextBin_GetIPName(MS_U16 u16PQIPIdx, MS_PQTextBin_Header_Info* pPQTextBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    MS_U32 u32DstAddr;

    if(u16PQIPIdx >= pPQTextBinHeader->u16IPNum)
    {
        MS_ASSERT(0);
        printf("[PQBinText] GetIPName:: IPIdx Out of range %u\n", u16PQIPIdx);
        u16PQIPIdx = 0;
    }
    u32Addr   = pPQTextBinHeader->u32IPName_Offset + pPQTextBinHeader->u32BinStartAddress;
    u32Offset = pPQTextBinHeader->u32IPName_Pitch * u16PQIPIdx +
                PQ_TEXT_BIN_IPNAME_OFFSET_OFFSET;

    u32DstAddr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQTextBinHeader->u32BinStartAddress;

    PQ_TEXT_BIN_DBG(printf("PQTextBin IPName: %lx\n", u32DstAddr));
    return u32DstAddr;
}

MS_U32 MDrv_PQTextBin_GetTableName(MS_U16 u16PQIPIdx, MS_U16 u16TabIdx, MS_PQTextBin_Header_Info* pPQTextBinHeader)
{
    MS_U32 u32Addr, u32Offset;
    MS_U32 u32DstAddr;
    MS_U16 u16GroupNum;
    MS_U32 u32Total_Len;
    MS_U32 i;

    if(u16PQIPIdx >= pPQTextBinHeader->u16GroupIPNum)
    {
        MS_ASSERT(0);
        printf("[PQBinText] GetGroupName:: IPIdx Out of range %u\n", u16PQIPIdx);
        u16PQIPIdx = 0;
    }

    u32Addr   = pPQTextBinHeader->u32IP_Group_Offset + pPQTextBinHeader->u32BinStartAddress;
    u32Offset = pPQTextBinHeader->u32IP_Group_pitch * u16PQIPIdx +
                PQ_TEXT_BIN_GROUPNAME_IPGROUP_GROUPNUM_OFFSET;
    u16GroupNum = MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);

    if(u16TabIdx >= u16GroupNum)
    {
        MS_ASSERT(0);
        printf("[PQBinText] GetGroupName :: TableIdx Out of range %u\n", u16TabIdx);
        u16TabIdx = 0;
    }
    u32Addr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQTextBinHeader->u32BinStartAddress;

    u32Offset  = PQ_TEXT_BIN_GROUPNAME_GROUPOFFSET_OFFSET;
    u32DstAddr = MDrv_PQBin_Get4ByteData((void *)(u32Addr+u32Offset), &u32Offset) +
                 pPQTextBinHeader->u32BinStartAddress;

    u32Total_Len = 0;
    for(i=0; i< u16TabIdx; i++)
    {
        u32Offset = PQ_TEXT_BIN_GROUPNAME_GROUPOFFSET_LEN +
                    PQ_TEXT_BIN_GROUPNAME_GROUPLENGTH_LEN * i;

        u32Total_Len += MDrv_PQBin_Get2ByteData((void *)(u32Addr+u32Offset), &u32Offset);
    }

    PQ_TEXT_BIN_DBG(printf("PQTextBin GroupName: %lx\n", (u32DstAddr+u32Total_Len)));
    return (u32DstAddr+u32Total_Len);
}


#endif
