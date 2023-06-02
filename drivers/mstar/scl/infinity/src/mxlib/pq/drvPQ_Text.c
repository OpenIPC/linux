// $Change: 600891 $
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


#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif


#include "MsCommon.h"
#include "MsTypes.h"

#include "drvPQ_Define.h"
#include "drvPQ.h"

//#include "QualityMap_Text.c"
#include "drvPQ_Bin.h"


#if(ENABLE_PQ_BIN)
MS_PQTextBin_Header_Info stPQTextBinHeaderInfo[MAX_PQ_TEXT_BIN_NUM];
MS_BOOL gbEnablePQTextBin = 0;
#endif

#if 0
char* MDrv_PQ_GetSrcTypeName(PQ_WIN eWindow)
{
#if(ENABLE_PQ_BIN)
    if(gbEnablePQTextBin)
    {
        MS_U32 u32Addr;
        MS_U16 u16SrcIDx = MDrv_PQ_GetSrcType(eWindow);

        u32Addr = MDrv_PQTextBin_GetSrcType(u16SrcIDx, &stPQTextBinHeaderInfo[eWindow]);
        return ((char *)u32Addr);
    }
    else
#endif
    {
    #if(PQ_ONLY_SUPPORT_BIN)
        return NULL;
    #else
        return PQ_INPUTTYPE_TEXT_Main[MDrv_PQ_GetSrcType(eWindow)];
    #endif
    }
}

char* MDrv_PQ_GetIPName(MS_U8 u8PQIPIdx)
{
#if(ENABLE_PQ_BIN)

    if(gbEnablePQTextBin)
    {
        MS_U32 u32Addr;;
        u32Addr = MDrv_PQTextBin_GetIPName((MS_U16)u8PQIPIdx,  &stPQTextBinHeaderInfo[PQ_MAIN_WINDOW]);
        return ((char *)u32Addr);
    }
    else
#endif
    {
    #if(PQ_ONLY_SUPPORT_BIN)
            return NULL;
    #else
        return PQ_IP_Text_Main[u8PQIPIdx];
    #endif
    }
}

char* MDrv_PQ_GetTableName(MS_U8 u8PQIPIdx, MS_U8 u8TabIdx )
{
#if(ENABLE_PQ_BIN)
    if(gbEnablePQTextBin)
    {
        MS_U32 u32Addr;;
        u32Addr = MDrv_PQTextBin_GetTableName((MS_U16)u8PQIPIdx,
                                              (MS_U16)u8TabIdx,
                                              &stPQTextBinHeaderInfo[PQ_MAIN_WINDOW]);
        return ((char *)u32Addr);

    }
    else
#endif
    {
    #if(PQ_ONLY_SUPPORT_BIN)
            return NULL;
    #else
        char** PQ_Stream;

        PQ_Stream = PQ_IPTAB_TEXT_Main[u8PQIPIdx];
        return PQ_Stream[u8TabIdx];
    #endif
    }
}

#if 0
void MDrv_PQ_Text_Demo(SCALER_WIN eWindow)
{
    MS_U8 u8IPIdx, u8TabIdx, u8IPNum, u8TabNum;
    U16 u16PQSrcType;

    u16PQSrcType = MDrv_PQ_GetSrcType(eWindow);
    printf("u16PQSrcType=%u\n", u16PQSrcType);

    u8IPNum = MDrv_PQ_GetIPNum(eWindow);

    for( u8IPIdx = 0; u8IPIdx < u8IPNum; u8IPIdx++)
    {
        u8TabNum = MDrv_PQ_GetTableNum(eWindow, u8IPIdx);
        printf("IP:%u [%s], tabnum=%u\n", u8IPIdx, MDrv_PQ_GetIPName(u8IPIdx), u8TabNum);

        u8TabIdx = MDrv_PQ_GetCurrentTableIndex(eWindow, u8IPIdx);
        printf("TAB:%u [%s]\n", u8TabIdx,
        (u8TabIdx == PQ_IP_NULL) ? "null" : MDrv_PQ_GetTableName(u8IPIdx, u8TabIdx));
    }
}
#endif
#endif
