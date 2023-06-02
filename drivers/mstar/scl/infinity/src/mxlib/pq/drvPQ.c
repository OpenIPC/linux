// $Change: 628136 $
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

#define  _MDRV_PQ_C_


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
//#include "MsVersion.h"
#include "MsOS.h"
#include "hwreg_utility2.h"
#include "color_reg.h"


#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ.h"
#include "drvPQ_Datatypes.h"
#include "mhal_pq.h"
#include "drvPQ_Bin.h"

#include "QualityMode.c"


#ifndef UNUSED //to avoid compile warnings...
#define UNUSED(var) (void)((var) = (var))
#endif

#define PQTAB_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_PQTAB) x; } while(0);
#define SRULE_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_SRULE) x; } while(0);
#define CSCRULE_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_CSCRULE) x; } while(0);
#define PQGRULE_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_GRULE) x; } while(0);
#define PQBW_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_BW) x; } while(0);
#define PQMADi_DBG(x)   do { if( _u16PQDbgSwitch & PQ_DBG_MADI) x; } while(0);
#define PQINFO_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_INFO) x; } while(0);
#define PQIOCTL_DBG(x)  do { if( _u16PQDbgSwitch & PQ_DBG_IOCTL) x; } while(0);
#define PQP2P_DBG(x)  do { if( _u16PQDbgSwitch & PQ_DBG_P2P) x; } while(0);
#define PQCOLOR_DBG(x) do { if( _u16PQDbgSwitch & PQ_DBG_COLOR) x; } while(0);

#if PQ_ENABLE_UNUSED_FUNC
static MSIF_Version _drv_pq_version = {

    #if (defined( MSOS_TYPE_CE) || defined(MSOS_TYPE_LINUX_KERNEL))
    { PQ_DRV_VERSION },
    #else
    .DDI = { PQ_DRV_VERSION },
    #endif
};

static MS_BOOL _bColorRange0_255[PQ_MAX_WINDOW];
static MS_BOOL _bPointToPointMode = FALSE;
static MS_BOOL _bDS_En = FALSE;
static MS_U16 _u16RW_Method = 0;

#endif

#if PQ_ENABLE_DEBUG
MS_BOOL _u16DbgSwitch = 1;
#endif

static MS_U16 _u16PQSrcType[PQ_MAX_WINDOW];
MS_U16 _u16PQSrcType_DBK_Detect[PQ_MAX_WINDOW];     //For Auto_DBK SW driver used

static PQ_INPUT_SOURCE_TYPE _enInputSourceType[PQ_MAX_WINDOW];


//whether current status is Point-to-point mode

static MS_U16 _u16PQDbgSwitch = 0;



typedef struct
{
    // input timing
    MS_U16 u16input_hsize;
    MS_U16 u16input_vtotal;
    MS_U16 u16input_vfreq;

    // output timing
    MS_U16 u16output_hsize;
    MS_U16 u16output_vtotal;
    MS_U16 u16output_vfreq;

    // memory format
    MS_BOOL bFBL;
    MS_BOOL bMemFmt422;
    MS_BOOL bInterlace;
    MS_BOOL b4RMode;
    MS_U8 u8BitsPerPixel;

    // osd
    MS_BOOL bOSD_On;
    MS_U16 u16OSD_hsize;
    MS_U8 u8OSD_BitsPerPixel;

    // dram
    MS_U32 u32MemBW;
    MS_BOOL bSC_MIUSel;
    MS_BOOL bGOP_MIUSel;
    MS_BOOL bDDR2;
    MS_U32  u32DDRFreq;
    MS_U8   u8BusWidth;
    MS_U32 u32Miu0MemSize;
    MS_U32 u32Miu1MemSize;

    // PVR
    MS_U32 u32PVR_BW;

    //AEON
    MS_U32 u32MISC_BW;
} BW_INFO_t;

#if (PQ_ENABLE_PIP)
#if (PQ_ONLY_SUPPORT_BIN == 0)
static PQ_DISPLAY_TYPE genDisplayType = PQ_DISPLAY_ONE;
#endif
#endif

#if (defined( MSOS_TYPE_CE) || defined(MSOS_TYPE_LINUX_KERNEL))
#if PQ_ENABLE_UNUSED_FUNC
static MS_PQ_INFO   _info           = {128,   217};
static MS_PQ_RFBL_INFO _stRFBL_Info = {FALSE, FALSE, 0 };
#endif
static MS_PQ_Status _status         = {FALSE, FALSE, };

#else
static MS_PQ_INFO   _info = {.u16Input_Src_Num = 128, .u8IP_Num = 217,};
static MS_PQ_Status _status = {.bIsInitialized = FALSE, .bIsRunning = FALSE, };
static MS_PQ_RFBL_INFO _stRFBL_Info = {.bEnable = FALSE, .bFilm = FALSE , .u8MADiType = 0, };
#endif

//static BW_INFO_t bw_info;


#if(ENABLE_PQ_BIN)
static MS_BOOL gbPQBinEnable = 0;
MS_PQBin_Header_Info stPQBinHeaderInfo[MAX_PQ_BIN_NUM];
extern MS_PQTextBin_Header_Info stPQTextBinHeaderInfo[MAX_PQ_TEXT_BIN_NUM];
//extern MS_BOOL gbEnablePQTextBin;
#endif

MS_S32 _PQ_Mutex = -1;

#ifdef MSOS_TYPE_LINUX
pthread_mutex_t _PQ_MLoad_Mutex;
#endif

//////////////////////////////////////////////////////////////////
// PQ Patch
MS_BOOL _bOSD_On = FALSE;

#if PQ_ENABLE_UNUSED_FUNC
static MS_BOOL bSetFrameCount = TRUE;
#endif

#if 0 //def _PDEBUG
#define PTH_PQ_RET_CHK(_pf_) \
    ({ \
        int r = _pf_; \
        if (r != 0 && r != ETIMEDOUT) \
            fprintf(stderr, "[PTHREAD] %s: %d: %s: %s\r\n", __FILE__, __LINE__, #_pf_, strerror(r)); \
        r; \
    })
#else
#define PTH_PQ_RET_CHK(_pf_) //_pf_
#endif


#ifdef MSOS_TYPE_LINUX
#define PQ_MLOAD_ENTRY() PTH_PQ_RET_CHK(pthread_mutex_lock(&_PQ_MLoad_Mutex))
#define PQ_MLOAD_RETURN() PTH_PQ_RET_CHK(pthread_mutex_unlock(&_PQ_MLoad_Mutex))
#else
#define PQ_MLOAD_ENTRY()
#define PQ_MLOAD_RETURN()
#endif

//////////////////////////////////////////////////////////////////
// OS related
//
//
#define XC_PQ_WAIT_MUTEX              (TICK_PER_ONE_MS * 50)              // 50 ms

/*
// Mutex & Lock
static MS_S32 _s32XC_PQMutex = -1;


#define XC_PQ_ENTRY()                 {   if (_s32XC_PQMutex < 0) { return E_XC_ACE_FAIL; }        \
                                        if(OS_OBTAIN_MUTEX(_s32XC_PQMutex, XC_PQ_WAIT_MUTEX) == FALSE) return E_XC_ACE_OBTAIN_MUTEX_FAIL;  }

#define XC_PQ_RETURN(_ret)            {   OS_RELEASE_MUTEX(_s32XC_PQMutex); return _ret; }
*/

//////////////////////////////////////////////////////////////////
void _MDrv_PQ_Clear_SRAM_Table_Index(void)
{
#if(ENABLE_PQ_BIN)

    if(gbPQBinEnable)
    {
        MDrv_PQBin_Clear_SRAM_Table_Index();
    }
    else
#endif
    {
        MDrv_PQ_ClearTableIndex(MAIN);
    #if ENABLE_PQ_EX
        MDrv_PQ_ClearTableIndex(MAINEX);
    #endif

    #if PQ_ENABLE_PIP
        MDrv_PQ_ClearTableIndex(SUB);
        MDrv_PQ_ClearTableIndex(SUBEX);
    #endif

    #if ENABLE_MULTI_SCALER

        MDrv_PQ_ClearTableIndex(SC1_MAIN);
        MDrv_PQ_ClearTableIndex(SC1_MAINEX);

        MDrv_PQ_ClearTableIndex(SC2_MAIN);
        MDrv_PQ_ClearTableIndex(SC2_MAINEX);
        MDrv_PQ_ClearTableIndex(SC2_SUB);
        MDrv_PQ_ClearTableIndex(SC2_SUBEX);
    #endif
    }
}

MS_BOOL MDrv_PQ_Init(MS_PQ_Init_Info *pstPQInitInfo)
{
    PQTABLE_INFO PQTableInfo;
#if(ENABLE_PQ_BIN)
    MS_U8 i;
    MS_U8 u8ID;
#endif
    if(_PQ_Mutex != -1)
    {
        printf("%s, already init\n", __FUNCTION__);
        return TRUE;
    }

    _PQ_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, "_PQ_Mutex", MSOS_PROCESS_SHARED);

    if(_PQ_Mutex == -1)
    {
        (printf("[MAPI PQ][%06d] create mutex fail\r\n", __LINE__));
        return FALSE;
    }

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTableInfo));

#ifdef MSOS_TYPE_LINUX
    #if(ENABLE_PQ_MLOAD)
    PTH_PQ_RET_CHK(pthread_mutexattr_t attr);
    PTH_PQ_RET_CHK(pthread_mutexattr_init(&attr));
    PTH_PQ_RET_CHK(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
    PTH_PQ_RET_CHK(pthread_mutex_init(&_PQ_MLoad_Mutex, &attr));
    #endif
#endif

    _status.bIsInitialized = TRUE;
    _status.bIsRunning     = TRUE;

    _MDrv_PQ_Clear_SRAM_Table_Index();

#if(ENABLE_PQ_BIN)
    if(pstPQInitInfo->u8PQTextBinCnt)
    {
        //gbEnablePQTextBin = 1;
        for(i = 0; i < pstPQInitInfo->u8PQTextBinCnt; i++)
        {
        #if PQ_ENABLE_UNUSED_FUNC
            u8ID = pstPQInitInfo->stPQTextBinInfo[i].u8PQID;
            stPQTextBinHeaderInfo[u8ID].u8BinID = pstPQInitInfo->stPQTextBinInfo[i].u8PQID;
            #if !defined(MSOS_TYPE_LINUX) && !defined(MSOS_TYPE_LINUX_KERNEL)
            stPQTextBinHeaderInfo[u8ID].u32BinStartAddress = MS_PA2KSEG0(pstPQInitInfo->stPQTextBinInfo[i].PQBin_PhyAddr);
            #else
            stPQTextBinHeaderInfo[u8ID].u32BinStartAddress = /*MS_PA2KSEG0*/(pstPQInitInfo->stPQTextBinInfo[i].PQBin_PhyAddr);
            #endif
            if(MDrv_PQTextBin_Parsing(&stPQTextBinHeaderInfo[u8ID]) == FALSE)
            {
                printf("MDrv_PQTextBin_Parsing: Parsing Fail, ID=%d \n", i);
                return FALSE;
            }
        #endif
        }

    }
    else
    {
        //printf("NonPQBin_Text !!\r\n");
        //gbEnablePQTextBin = 0;
    }

    if(pstPQInitInfo->u8PQBinCnt)
    {
        printf("Start PQ Bin Init \n");
        gbPQBinEnable = 1;
        MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, MAIN_WINDOW);
    #if PQ_ENABLE_PIP
        MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, SUB_WINDOW);
    #endif

    #if ENABLE_MULTI_SCALER
        MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, SC1_MAIN_WINDOW);
        MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, SC2_MAIN_WINDOW);
        MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, SC2_SUB_WINDOW);
    #endif

        switch(pstPQInitInfo->u16PnlWidth)
        {
        default:
        case 1366:
            MDrv_PQBin_SetPanelID(1, MAIN_WINDOW);
        #if PQ_ENABLE_PIP
            MDrv_PQBin_SetPanelID(1, SUB_WINDOW);
        #endif
        #if ENABLE_MULTI_SCALER
            MDrv_PQBin_SetPanelID(1, SC2_MAIN_WINDOW);
            MDrv_PQBin_SetPanelID(1, SC2_SUB_WINDOW);
            MDrv_PQBin_SetPanelID(0, SC1_SUB_WINDOW);
        #endif
            break;

        case 1920:
            MDrv_PQBin_SetPanelID(1, MAIN_WINDOW);
        #if PQ_ENABLE_PIP
            MDrv_PQBin_SetPanelID(1, SUB_WINDOW);
        #endif
        #if ENABLE_MULTI_SCALER
            MDrv_PQBin_SetPanelID(1, SC2_MAIN_WINDOW);
            MDrv_PQBin_SetPanelID(1, SC2_SUB_WINDOW);
            MDrv_PQBin_SetPanelID(0, SC1_SUB_WINDOW);
        #endif
            break;
        }

        //because main and sub's common table may be different,
        //so we will take main's common table as correct one.
        //so here load sub first and then use main's overwrite sub's
        //Sub first
        for(i = 0; i < MAX_PQ_BIN_NUM; i++)
        {
            if(pstPQInitInfo->stPQBinInfo[i].PQ_Bin_BufSize > 0)
            {
                PQ_WIN ePQWin;
                u8ID = pstPQInitInfo->stPQBinInfo[i].u8PQID;
                ePQWin = MDrv_PQBin_TransToPQWin(u8ID);
                stPQBinHeaderInfo[u8ID].u8BinID = (MS_U8)pstPQInitInfo->stPQBinInfo[i].u8PQID;
                stPQBinHeaderInfo[u8ID].u32BinStartAddress = (MS_U32)(pstPQInitInfo->stPQBinInfo[i].PQBin_PhyAddr);
                MDrv_PQBin_Parsing(&stPQBinHeaderInfo[u8ID]);
                MDrv_PQBin_LoadCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(ePQWin), &stPQBinHeaderInfo[u8ID]);
            }
            else
            {
                printf("MDrv_PQ_Init()  Error:  PQ_Bin_BufSize < 0 !!  \r\n");
            }
        }
    }
    else
#endif
    {
        #if(PQ_ONLY_SUPPORT_BIN)
            //PQTableInfo = PQTableInfo;
            #if(ENABLE_PQ_BIN)
            gbPQBinEnable = 1;
            #endif

        #else
            MS_BOOL bSkipCommTable = TRUE;

            //printf("NonPQBin !! \r\n");
            #if(ENABLE_PQ_BIN)
            gbPQBinEnable = 0;
            #endif


            MDrv_PQ_Set_DisplayType_Main(pstPQInitInfo->u16PnlWidth, PQ_DISPLAY_ONE, bSkipCommTable);
        #if (PQ_ENABLE_PIP)
            MDrv_PQ_Set_DisplayType_Sub(pstPQInitInfo->u16PnlWidth, PQ_DISPLAY_ONE, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_SC2_Sub(pstPQInitInfo->u16PnlWidth, PQ_DISPLAY_ONE, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_SC2_Main(pstPQInitInfo->u16PnlWidth, PQ_DISPLAY_ONE, bSkipCommTable);

            MDrv_PQ_Set_DisplayType_SC1_Main(pstPQInitInfo->u16PnlWidth, PQ_DISPLAY_ONE, bSkipCommTable);
        #endif

            #if(ENABLE_PQ_LOAD_TABLE_INFO)
            MDrv_PQ_PreInitLoadTableInfo(MAIN);

            #if ENABLE_PQ_EX
            MDrv_PQ_PreInitLoadTableInfo(MAINEX);
            #endif

            #if PQ_ENABLE_PIP
            MDrv_PQ_PreInitLoadTableInfo(SUB);
            MDrv_PQ_PreInitLoadTableInfo(SUBEX);
            #endif
            #endif

        #endif // end of #if(PQ_ONLY_SUPPORT_BIN == 0)

    }

#if 0
    // Set BK12 [40] [1:0] = b'11 to enable hw double buffer write
    // at blanking area for changing memory format by L_BK_SCMI(0x02)
    // this register is added after T2 U04
    Hal_PQ_set_memfmt_doublebuffer(PQ_MAIN_WINDOW, ENABLE);
#if ENABLE_MULTI_SCALER
    Hal_PQ_set_memfmt_doublebuffer(PQ_SC1_MAIN_WINDOW, ENABLE);
    Hal_PQ_set_memfmt_doublebuffer(PQ_SC2_MAIN_WINDOW, ENABLE);
#endif

    MsOS_Memcpy(&_stPQ_Info, pstPQInitInfo, sizeof(MS_PQ_Init_Info));

    MsOS_Memset(&bw_info, 0x00, sizeof(bw_info));
    bw_info.bDDR2 = pstPQInitInfo->bDDR2;
    bw_info.u32DDRFreq = pstPQInitInfo->u32DDRFreq;
    bw_info.u8BusWidth = pstPQInitInfo->u8BusWidth;
    bw_info.u16output_vtotal = pstPQInitInfo->u16Pnl_vtotal;
    bw_info.u16output_hsize = pstPQInitInfo->u16PnlWidth;
    bw_info.u16OSD_hsize = pstPQInitInfo->u16OSD_hsize;
    bw_info.u32Miu0MemSize = pstPQInitInfo->u32miu0em_size;
    bw_info.u32Miu1MemSize = pstPQInitInfo->u32miu1em_size;

    for(i = 0; i < PQ_MAX_WINDOW; i++)
        _bColorRange0_255[i] = TRUE;
#endif

#if (PQ_ENABLE_PIP)
    #if (PQ_ONLY_SUPPORT_BIN == 0)
    genDisplayType = PQ_DISPLAY_ONE;
    #endif
#endif

    return TRUE;
}

void MDrv_PQ_DesideSrcType(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType)
{
    _enInputSourceType[eWindow] = enInputSourceType;
    _u16PQSrcType[eWindow] = QM_InputSourceToIndex(eWindow, enInputSourceType);
    _u16PQSrcType_DBK_Detect[eWindow] = _u16PQSrcType[eWindow];                     //For Auto_DBK SW driver used
    PQTAB_DBG(printf("[PQ_DesideSrcType] window=%u, enInputSrcType=%u, SrcType=%u\r\n",
                     eWindow, enInputSourceType, _u16PQSrcType[eWindow]));

    Hal_PQ_set_sourceidx(eWindow, _u16PQSrcType[eWindow]);

#if(ENABLE_PQ_LOAD_TABLE_INFO)

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            MDrv_PQ_Set_LoadTableInfo_SrcType(MAIN, _u16PQSrcType[eWindow]);
            MDrv_PQ_Set_LoadTableInfo_SrcType(MAINEX, _u16PQSrcType[eWindow]);
        }
    }
#if PQ_ENABLE_PIP
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            MDrv_PQ_Set_LoadTableInfo_SrcType(SUB, _u16PQSrcType[eWindow]);
            MDrv_PQ_Set_LoadTableInfo_SrcType(SUBEX, _u16PQSrcType[eWindow]);
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC1_MAIN, _u16PQSrcType[eWindow]);
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC1_MAINEX, _u16PQSrcType[eWindow]);
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC2_MAIN, _u16PQSrcType[eWindow]);
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC2_MAINEX, _u16PQSrcType[eWindow]);
        }
    }
#if PQ_ENABLE_PIP
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC2_SUB, _u16PQSrcType[eWindow]);
            MDrv_PQ_Set_LoadTableInfo_SrcType(SC2_SUBEX, _u16PQSrcType[eWindow]);
        }
    }
#endif


#endif // ENABLE_MULTI_SCALER
    else
    {

    }

#endif //#if(ENABLE_PQ_LOAD_TABLE_INFO)

}

MS_BOOL MDrv_PQ_Set_DisplayType_Main(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, MS_BOOL bSkipCommTable)
{
#if (PQ_ONLY_SUPPORT_BIN == 0)
    PQTABLE_INFO PQTableInfo;
    PQTABLE_INFO PQTableInfoEx;

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTABLE_INFO));
    MsOS_Memset(&PQTableInfoEx, 0, sizeof(PQTABLE_INFO));

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_Main: QMAP_1920_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_NR_Main: MST_GRule_1920_NR_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_MPEG_NR_Main:MST_GRule_1920_MPEG_NR_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_FILM_MODE_Main:MST_GRule_1920_FILM_MODE_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_DYNAMIC_CONTRAST_Main:MST_GRule_1920_DYNAMIC_CONTRAST_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_ULTRAT_CLEAR_Main:MST_GRule_1920_ULTRAT_CLEAR_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_DDR_SELECT_Main:MST_GRule_1920_DDR_SELECT_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_OSD_BW_Main:MST_GRule_1920_OSD_BW_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PTP_Main:MST_GRule_1920_PTP_Main);
#endif
             break;
#if PQ_ENABLE_PIP
        case PQ_DISPLAY_PIP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_PIP_Main: QMAP_1920_PIP_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_NR_Main: MST_GRule_1920_PIP_NR_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_MPEG_NR_Main:MST_GRule_1920_PIP_MPEG_NR_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_FILM_MODE_Main:MST_GRule_1920_PIP_FILM_MODE_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DYNAMIC_CONTRAST_Main:MST_GRule_1920_PIP_DYNAMIC_CONTRAST_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_ULTRAT_CLEAR_Main:MST_GRule_1920_PIP_ULTRAT_CLEAR_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DDR_SELECT_Main:MST_GRule_1920_PIP_DDR_SELECT_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_OSD_BW_Main:MST_GRule_1920_PIP_OSD_BW_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_PTP_Main:MST_GRule_1920_PIP_PTP_Main);
#endif

            break;

        case PQ_DISPLAY_POP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_POP_Main: QMAP_1920_POP_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_NR_Main: MST_GRule_1920_POP_NR_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_MPEG_NR_Main:MST_GRule_1920_POP_MPEG_NR_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_FILM_MODE_Main:MST_GRule_1920_POP_FILM_MODE_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DYNAMIC_CONTRAST_Main:MST_GRule_1920_POP_DYNAMIC_CONTRAST_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_ULTRAT_CLEAR_Main:MST_GRule_1920_POP_ULTRAT_CLEAR_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DDR_SELECT_Main:MST_GRule_1920_POP_DDR_SELECT_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_OSD_BW_Main:MST_GRule_1920_POP_OSD_BW_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_PTP_Main:MST_GRule_1920_POP_PTP_Main);
#endif
            break;
#endif
    }

    // table config parameter
    PQTableInfo.eWin = PQ_MAIN_WINDOW;
    PQTableInfo.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_Main;
    PQTableInfo.u8PQ_IP_Num = PQ_IP_NUM_Main;
    PQTableInfo.pIPTAB_Info = (void*)PQ_IPTAB_INFO_Main;
    PQTableInfo.pSkipRuleIP = (void*)MST_SkipRule_IP_Main;

#if PQ_ENABLE_HSDRULE
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSD] = PQ_HSDRule_IP_NUM_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSD] = PQ_VSDRule_IP_NUM_Main;
#endif

    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSP] = PQ_HSPRule_IP_NUM_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSP] = PQ_VSPRule_IP_NUM_Main;
#if PQ_ENABLE_CSCRULE
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_CSC] = PQ_CSCRule_IP_NUM_Main;
#endif

#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_NTSC] = PQ_DBRule_NTSC_IP_NUM_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_PAL]  = PQ_DBRule_PAL_IP_NUM_Main;
#endif

#if (PQ_ENABLE_HSDRULE)
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSD] = (void*)MST_HSDRule_IP_Index_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSD] = (void*)MST_VSDRule_IP_Index_Main;
#endif
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSP] = (void*)MST_HSPRule_IP_Index_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSP] = (void*)MST_VSPRule_IP_Index_Main;
#if (PQ_ENABLE_CSCRULE)
    PQTableInfo.pXRule_IP_Index[E_XRULE_CSC] = (void*)MST_CSCRule_IP_Index_Main;
#endif

#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_IP_Index_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_IP_Index_Main;
#endif

#if (PQ_ENABLE_HSDRULE)
    PQTableInfo.pXRule_Array[E_XRULE_HSD] = (void*)MST_HSDRule_Array_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSD] = (void*)MST_VSDRule_Array_Main;
#endif
    PQTableInfo.pXRule_Array[E_XRULE_HSP] = (void*)MST_HSPRule_Array_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSP] = (void*)MST_VSPRule_Array_Main;

#if (PQ_ENABLE_CSCRULE)
    PQTableInfo.pXRule_Array[E_XRULE_CSC] = (void*)MST_CSCRule_Array_Main;
#endif

#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_Array[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_Array_Main;
    PQTableInfo.pXRule_Array[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_Array_Main;
#endif

#if (PQ_GRULE_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_NR] = PQ_GRULE_NR_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_NR] = PQ_GRULE_NR_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_NR] = (void*)MST_GRule_NR_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_NR] = (void*)MST_GRule_NR_Main;
#endif

#if (PQ_GRULE_MPEG_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_Main;
#endif

#if (PQ_GRULE_FILM_MODE_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_Main;
#endif

#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_Main;
#endif

#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_Main;
#endif

#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_Main;
#endif

#if (PQ_GRULE_OSD_BW_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_Main;
#endif

#if (PQ_GRULE_PTP_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_PTP] = PQ_GRULE_PTP_NUM_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_PTP] = PQ_GRULE_PTP_IP_NUM_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_PTP] = (void*)MST_GRule_PTP_IP_Index_Main;
    PQTableInfo.pGRule_Array[E_GRULE_PTP] = (void*)MST_GRule_PTP_Main;
#endif

    MDrv_PQ_AddTable_(MAIN, &PQTableInfo);

    if(!bSkipCommTable)
    {
#if (PQ_ENABLE_UNUSED_FUNC)
        MDrv_PQ_LoadCommTable_(MAIN);
#endif
    }

#if (ENABLE_PQ_EX)
    // table config parameter
    PQTableInfoEx = PQTableInfo;

    PQTableInfoEx.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_Main_Ex;
    PQTableInfoEx.u8PQ_IP_Num = PQ_IP_NUM_Main_Ex;
    PQTableInfoEx.pIPTAB_Info = (void*)PQ_IPTAB_INFO_Main_Ex;
    PQTableInfoEx.pSkipRuleIP = (void*)MST_SkipRule_IP_Main_Ex;

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_Main_Ex;
        }
        break;

        case PQ_DISPLAY_PIP:
#if PQ_ENABLE_PIP
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Main_Ex;
        }
#endif
        break;

        case PQ_DISPLAY_POP:
#if PQ_ENABLE_PIP
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_Main_Ex;
        }
#endif
        break;
    }

    MDrv_PQ_AddTable_(MAINEX, &PQTableInfoEx);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(MAINEX);
    }
    if((MS_U16)QM_INPUTTYPE_NUM_Main != (MS_U16)QM_INPUTTYPE_NUM_Main_Ex)
    {
        printf("PQ INPUTTYPE NUM MISMATCH: MAIN != EX");
        MS_ASSERT(0);
    }
#endif
    return TRUE;
#else
    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
    UNUSED(bSkipCommTable);
    return FALSE;
#endif
}

void MDRv_PQ_Check_Type(PQ_WIN eWindow, PQ_CHECK_TYPE EnCheck)
{
        if(eWindow == PQ_MAIN_WINDOW)
        {
#if(ENABLE_PQ_BIN)
            if(gbPQBinEnable)
            {

            }
            else
#endif
            {
                MDrv_PQ_Check_Type_(MAIN, EnCheck);

            }
        }
#if (PQ_ENABLE_PIP)
        else if(eWindow == PQ_SUB_WINDOW)
        {

        }
#endif
#if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {

        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {

        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {

        }
#endif
        else
        {
            MS_ASSERT(0);
        }
}

void MDRv_PQ_Set_CmdqCfg(PQ_WIN eWindow, MS_BOOL bEnCMDQ, MS_U8 u8FmCnt,MS_BOOL bFire)
{
    PQ_DBG(printf("%s %d, Win:%d, CMDQEn:%d, FmCnt:%d", __FUNCTION__, __LINE__, eWindow, bEnCMDQ, u8FmCnt));

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            PQ_CMDQ_CONFIG CmdqCfg;

            CmdqCfg.bEnFmCnt = bEnCMDQ;
            CmdqCfg.u8FmCnt = u8FmCnt;
            CmdqCfg.bfire = bFire;
            MDrv_PQ_Set_CmdqCfg_(MAIN, CmdqCfg);

        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {

    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {

    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {

    }
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {

    }
#endif
    else
    {
        MS_ASSERT(0);
    }

}
void MDrv_PQ_init_RIU(MS_U32 riu_addr)
{
    Hal_PQ_init_riu_base(riu_addr);
}

void MDrv_PQ_LoadSettingByData(PQ_WIN eWindow, MS_U8 u8PQIPIdx, MS_U8 *pData, MS_U16 u16DataSize)
{
    PQ_DBG(printf("%s %d, Win:%d, IPIdx:%d, Size%d  \n", __FUNCTION__, __LINE__, eWindow, u8PQIPIdx, u16DataSize));

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

        }
        else
#endif
        {
            PQ_DATA_INFO DataInfo;

            DataInfo.pBuf = pData;
            DataInfo.u16BufSize = u16DataSize;
            MDrv_PQ_LoadTableByData_(MAIN, _u16PQSrcType[eWindow], u8PQIPIdx, &DataInfo);
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {

    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {

    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {

    }
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {

    }
#endif
    else
    {
        MS_ASSERT(0);
    }

}


void MDrv_PQ_LoadSettings(PQ_WIN eWindow)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_EXT_MAIN]);

        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            PQTAB_DBG(printf("MAIN table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#if(ENABLE_PQ_EX == 1)
            PQTAB_DBG(printf("MAINEX table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            PQTAB_DBG(printf("...done\r\n"));
#endif
        }

#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, TRUE);
#endif
#if PQ_UC_CTL
        MDrv_PQ_LoadTable(eWindow, MDrv_PQ_GetTableIndex(eWindow, PQ_IP_UC_CTL_Main), PQ_IP_UC_CTL_Main);
#endif
#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, FALSE);
#endif
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_EXT_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            PQTAB_DBG(printf("SUB table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SUB, _u16PQSrcType[eWindow], PQ_IP_ALL);

            PQTAB_DBG(printf("SUB MAINEX table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SUBEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
            PQTAB_DBG(printf("...done\r\n"));
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_EXT_SC1_MAIN]);

        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            PQTAB_DBG(printf("SC1_MAIN table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC1_MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#if(ENABLE_PQ_EX == 1)
            PQTAB_DBG(printf("SC2_MAINEX table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC1_MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            PQTAB_DBG(printf("...done\r\n"));
#endif
        }

#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, TRUE);
#endif
#if PQ_UC_CTL
        MDrv_PQ_LoadTable(eWindow, MDrv_PQ_GetTableIndex(eWindow, PQ_IP_UC_CTL_SC1_Main), PQ_IP_UC_CTL_SC1_Main);
#endif
#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, FALSE);
#endif
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_EXT_SC2_MAIN]);

        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            PQTAB_DBG(printf("SC2_MAIN table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC2_MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#if(ENABLE_PQ_EX == 1)
            PQTAB_DBG(printf("SC2_MAINEX table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC2_MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            PQTAB_DBG(printf("...done\r\n"));
#endif
        }

#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, TRUE);
#endif
#if PQ_UC_CTL
        MDrv_PQ_LoadTable(eWindow, MDrv_PQ_GetTableIndex(eWindow, PQ_IP_UC_CTL_SC2_Main), PQ_IP_UC_CTL_SC2_Main);
#endif
#if(ENABLE_PQ_MLOAD)
        MDrv_PQ_Set_MLoadEn(eWindow, FALSE);
#endif
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);

            MDrv_PQBin_LoadTableBySrcType(_u16PQSrcType[eWindow],
                                          PQ_BIN_IP_ALL,
                                          (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                          &stPQBinHeaderInfo[PQ_BIN_EXT_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            PQTAB_DBG(printf("SC2_SUB table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC2_SUB, _u16PQSrcType[eWindow], PQ_IP_ALL);

            PQTAB_DBG(printf("SC2_SUB MAINEX table\r\n"));
            MDrv_PQ_LoadTableBySrcType_(SC2_SUBEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
            PQTAB_DBG(printf("...done\r\n"));
#endif
        }
    }
#endif


#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }

#if(ENABLE_PQ_LOAD_TABLE_INFO)
    if(eWindow == PQ_MAIN_WINDOW)
    {
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_MAIN);
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_MAIN_EX);
    }
#if PQ_ENABLE_PIP
    else if(eWindow == PQ_SUB_WINDOW)
    {
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SUB);
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SUB_EX);
    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC1_MAIN);
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC1_MAIN_EX);
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC2_MAIN);
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC2_MAIN_EX);
    }
#if PQ_ENABLE_PIP
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC2_SUB);
        MDRV_PQ_PrintLoadTableInfo(PQ_LOAD_TABLE_SC2_SUB_EX);
    }
#endif

#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
#endif
}

#if (PQ_ENABLE_UNUSED_FUNC)

MS_BOOL MDrv_PQ_Exit(void)
{
    _MDrv_PQ_Clear_SRAM_Table_Index();
    return true;
}

MS_U16 MDrv_PQ_GetIPNum(PQ_WIN eWindow)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetIPNum(&stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetIPNum_(MAIN);
#endif
        }
    }

#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetIPNum(&stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetIPNum_(SUB);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER

    // SC1_MAIN
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetIPNum(&stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetIPNum_(SC1_MAIN);
#endif
        }
    }
    // SC2_MAIN
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetIPNum(&stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetIPNum_(SC2_MAIN);
#endif
        }
    }
    // SC2_SUB
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetIPNum(&stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetIPNum_(SC2_SUB);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER

    else
    {
        MS_ASSERT(0);
        return 0xFFFF;
    }
}

MS_U16 MDrv_PQ_GetTableNum(PQ_WIN eWindow, MS_U16 u16PQIPIdx)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetTableNum((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW), &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            UNUSED(u16PQIPIdx);
            return 0xFFFF;
#else
            return MDrv_PQ_GetTableNum_(MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetTableNum((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW), &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetTableNum_(SUB, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER

    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetTableNum((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW), &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            UNUSED(u16PQIPIdx);
            return 0xFFFF;
#else
            return MDrv_PQ_GetTableNum_(SC1_MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetTableNum((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW), &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            UNUSED(u16PQIPIdx);
            return 0xFFFF;
#else
            return MDrv_PQ_GetTableNum_(SC2_MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetTableNum((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW), &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetTableNum_(SC2_SUB, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif
#endif //ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
        return 0xFFFF;
    }
}

MS_U16 MDrv_PQ_GetCurrentTableIndex(PQ_WIN eWindow, MS_U16 u16PQIPIdx)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetCurrentTableIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),  u16PQIPIdx, PQ_BIN_STD_MAIN);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetCurrentTableIndex_(MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetCurrentTableIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),  u16PQIPIdx, PQ_BIN_STD_SUB);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetCurrentTableIndex_(SUB, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetCurrentTableIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),  u16PQIPIdx, PQ_BIN_STD_SC1_MAIN);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetCurrentTableIndex_(SC1_MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }

    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetCurrentTableIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),  u16PQIPIdx, PQ_BIN_STD_SC2_MAIN);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetCurrentTableIndex_(SC2_MAIN, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            return MDrv_PQBin_GetCurrentTableIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),  u16PQIPIdx, PQ_BIN_STD_SC2_SUB);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            return 0xFFFF;
#else
            return MDrv_PQ_GetCurrentTableIndex_(SC2_SUB, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
        return 0xFFFF;
    }
}

//Get main page setting from u16IPIdx(column), according to current input source type(row)
MS_U16 MDrv_PQ_GetTableIndex(PQ_WIN eWindow, MS_U16 u16IPIdx)
{
    MS_U8 u16TabIdx = 0;

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = (MS_U8)MDrv_PQBin_GetTableIndex(_u16PQSrcType[eWindow],
                                                         u16IPIdx,
                                                        (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                                        &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = (MS_U8)MDrv_PQ_GetTableIndex_(MAIN, _u16PQSrcType[eWindow], (MS_U8)u16IPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = (MS_U8)MDrv_PQBin_GetTableIndex(_u16PQSrcType[eWindow],
                                                        u16IPIdx,
                                                        (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                                        &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = (MS_U8)MDrv_PQ_GetTableIndex_(SUB, _u16PQSrcType[eWindow], (MS_U8)u16IPIdx);
#endif
        }
    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = (MS_U8)MDrv_PQBin_GetTableIndex(_u16PQSrcType[eWindow],
                                                        u16IPIdx,
                                                        (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                                        &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = (MS_U8)MDrv_PQ_GetTableIndex_(SC1_MAIN, _u16PQSrcType[eWindow], (MS_U8)u16IPIdx);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = (MS_U8)MDrv_PQBin_GetTableIndex(_u16PQSrcType[eWindow],
                                                        u16IPIdx,
                                                        (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                                        &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = (MS_U8)MDrv_PQ_GetTableIndex_(SC2_MAIN, _u16PQSrcType[eWindow], (MS_U8)u16IPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = (MS_U8)MDrv_PQBin_GetTableIndex(_u16PQSrcType[eWindow],
                                                        u16IPIdx,
                                                        (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                                        &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = (MS_U8)MDrv_PQ_GetTableIndex_(SC2_SUB, _u16PQSrcType[eWindow], (MS_U8)u16IPIdx);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
    return u16TabIdx;
}

static MS_U16 MDrv_PQ_GetXRuleIPIndex(PQ_WIN eWindow, MS_U16 u16XRuleType, MS_U16 u16XRuleIP)
{
    MS_U8 u16IPIdx = 0;
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPIdx = (MS_U8)MDrv_PQBin_GetXRuleIPIndex(u16XRuleType,
                                                         u16XRuleIP,
                                                         &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPIdx = 0;
#else
            u16IPIdx = MDrv_PQ_GetXRuleIPIndex_(MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIP);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPIdx = (MS_U8)MDrv_PQBin_GetXRuleIPIndex(u16XRuleType,
                                                         u16XRuleIP,
                                                         &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPIdx = 0;
#else
            u16IPIdx = MDrv_PQ_GetXRuleIPIndex_(SUB, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIP);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPIdx = (MS_U8)MDrv_PQBin_GetXRuleIPIndex(u16XRuleType,
                                                         u16XRuleIP,
                                                         &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPIdx = 0;
#else
            u16IPIdx = MDrv_PQ_GetXRuleIPIndex_(SC1_MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIP);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPIdx = (MS_U8)MDrv_PQBin_GetXRuleIPIndex(u16XRuleType,
                                                         u16XRuleIP,
                                                         &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPIdx = 0;
#else
            u16IPIdx = MDrv_PQ_GetXRuleIPIndex_(SC2_MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIP);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPIdx = (MS_U8)MDrv_PQBin_GetXRuleIPIndex(u16XRuleType,
                                                         u16XRuleIP,
                                                         &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPIdx = 0;
#else
            u16IPIdx = MDrv_PQ_GetXRuleIPIndex_(SC2_SUB, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIP);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
    return u16IPIdx;
}

static MS_U16 MDrv_PQ_GetXRuleTableIndex(PQ_WIN eWindow, MS_U16 u16XRuleType, MS_U16 u16XRuleIdx, MS_U16 u16XRuleIP)
{
    MS_U16 u16TabIdx = 0;

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = MDrv_PQBin_GetXRuleTableIndex(u16XRuleType,
                        u16XRuleIdx,
                        u16XRuleIP,
                        &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = MDrv_PQ_GetXRuleTableIndex_(MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIdx, (MS_U8)u16XRuleIP);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = MDrv_PQBin_GetXRuleTableIndex(u16XRuleType,
                        u16XRuleIdx,
                        u16XRuleIP,
                        &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = MDrv_PQ_GetXRuleTableIndex_(SUB, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIdx, (MS_U8)u16XRuleIP);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER

    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = MDrv_PQBin_GetXRuleTableIndex(u16XRuleType,
                        u16XRuleIdx,
                        u16XRuleIP,
                        &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = MDrv_PQ_GetXRuleTableIndex_(SC1_MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIdx, (MS_U8)u16XRuleIP);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = MDrv_PQBin_GetXRuleTableIndex(u16XRuleType,
                        u16XRuleIdx,
                        u16XRuleIP,
                        &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = MDrv_PQ_GetXRuleTableIndex_(SC2_MAIN, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIdx, (MS_U8)u16XRuleIP);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16TabIdx = MDrv_PQBin_GetXRuleTableIndex(u16XRuleType,
                        u16XRuleIdx,
                        u16XRuleIP,
                        &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16TabIdx = 0;
#else
            u16TabIdx = MDrv_PQ_GetXRuleTableIndex_(SC2_SUB, (MS_U8)u16XRuleType, (MS_U8)u16XRuleIdx, (MS_U8)u16XRuleIP);
#endif
        }
    }
#endif


#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
    return u16TabIdx;
}

static MS_U16 MDrv_PQ_GetXRuleIPNum(PQ_WIN eWindow, MS_U16 u16XRuleType)
{
    MS_U16 u16IPNum = 0;
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPNum = MDrv_PQBin_GetXRuleIPNum(u16XRuleType,
                                                &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPNum = 0;
#else
            u16IPNum = MDrv_PQ_GetXRuleIPNum_(MAIN, (MS_U8)u16XRuleType);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPNum = MDrv_PQBin_GetXRuleIPNum(u16XRuleType,
                                                &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPNum = 0;
#else
            u16IPNum = MDrv_PQ_GetXRuleIPNum_(SUB, (MS_U8)u16XRuleType);
#endif
        }
    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPNum = MDrv_PQBin_GetXRuleIPNum(u16XRuleType,
                                                &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPNum = 0;
#else
            u16IPNum = MDrv_PQ_GetXRuleIPNum_(SC1_MAIN, (MS_U8)u16XRuleType);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPNum = MDrv_PQBin_GetXRuleIPNum(u16XRuleType,
                                                &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPNum = 0;
#else
            u16IPNum = MDrv_PQ_GetXRuleIPNum_(SC2_MAIN, (MS_U8)u16XRuleType);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16IPNum = MDrv_PQBin_GetXRuleIPNum(u16XRuleType,
                                                &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16IPNum = 0;
#else
            u16IPNum = MDrv_PQ_GetXRuleIPNum_(SC2_SUB, (MS_U8)u16XRuleType);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
    return u16IPNum;
}

#if (PQ_GRULE_NR_ENABLE || PQ_GRULE_OSD_BW_ENABLE)
static MS_U16 MDrv_PQ_GetGRule_LevelIndex(PQ_WIN eWindow, MS_U16 u16GRuleType, MS_U16 u16GRuleLevelIndex)
{
    MS_U16 u16Ret = 0;

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_LevelIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                                             u16GRuleType,
                                                             u16GRuleLevelIndex,
                                                             &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_LevelIndex_(MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleLevelIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_LevelIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                                             u16GRuleType,
                                                             u16GRuleLevelIndex,
                                                             &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_LevelIndex_(SUB, (MS_U8)u16GRuleType, (MS_U8)u16GRuleLevelIndex);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_LevelIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                                             u16GRuleType,
                                                             u16GRuleLevelIndex,
                                                             &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_LevelIndex_(SC1_MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleLevelIndex);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_LevelIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                                             u16GRuleType,
                                                             u16GRuleLevelIndex,
                                                             &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_LevelIndex_(SC2_MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleLevelIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_LevelIndex((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                                             u16GRuleType,
                                                             u16GRuleLevelIndex,
                                                             &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_LevelIndex_(SC2_SUB, (MS_U8)u16GRuleType, (MS_U8)u16GRuleLevelIndex);
#endif
        }
    }
#endif

#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
    return u16Ret;
}

static MS_U16 MDrv_PQ_GetGRule_IPIndex(PQ_WIN eWindow, MS_U16 u16GRuleType, MS_U16 u16GRuleIPIndex)
{
    MS_U16 u16Ret = 0;
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = MDrv_PQBin_GetGRule_IPIndex(u16GRuleType,
                                                 u16GRuleIPIndex,
                                                 &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_IPIndex_(MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = MDrv_PQBin_GetGRule_IPIndex(u16GRuleType,
                                                 u16GRuleIPIndex,
                                                 &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_IPIndex_(SUB, (MS_U8)u16GRuleType, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = MDrv_PQBin_GetGRule_IPIndex(u16GRuleType,
                                                 u16GRuleIPIndex,
                                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_IPIndex_(SC1_MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }

    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = MDrv_PQBin_GetGRule_IPIndex(u16GRuleType,
                                                 u16GRuleIPIndex,
                                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_IPIndex_(SC2_MAIN, (MS_U8)u16GRuleType, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = MDrv_PQBin_GetGRule_IPIndex(u16GRuleType,
                                                 u16GRuleIPIndex,
                                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = MDrv_PQ_GetGRule_IPIndex_(SC2_SUB, (MS_U8)u16GRuleType, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#endif

#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }

    return u16Ret;
}

static MS_U16 MDrv_PQ_GetGRule_TableIndex(PQ_WIN eWindow, MS_U16 u16GRuleType, MS_U16 u16PQ_NRIdx, MS_U16 u16GRuleIPIndex)
{
    MS_U16 u16Ret = 0;
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_TableIndex(u16GRuleType,
                     _u16PQSrcType[eWindow],
                     u16PQ_NRIdx,
                     u16GRuleIPIndex,
                     &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = (MS_U8)MDrv_PQ_GetGRule_TableIndex_(MAIN, (MS_U8)u16GRuleType, (MS_U8)_u16PQSrcType[eWindow], (MS_U8)u16PQ_NRIdx, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_TableIndex(u16GRuleType,
                     _u16PQSrcType[eWindow],
                     u16PQ_NRIdx,
                     u16GRuleIPIndex,
                     &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = (MS_U8)(MDrv_PQ_GetGRule_TableIndex_(SUB, (MS_U8)u16GRuleType, (MS_U8)_u16PQSrcType[eWindow], (MS_U8)u16PQ_NRIdx, (MS_U8)u16GRuleIPIndex));
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_TableIndex(u16GRuleType,
                     _u16PQSrcType[eWindow],
                     u16PQ_NRIdx,
                     u16GRuleIPIndex,
                     &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = (MS_U8)MDrv_PQ_GetGRule_TableIndex_(SC1_MAIN, (MS_U8)u16GRuleType, (MS_U8)_u16PQSrcType[eWindow], (MS_U8)u16PQ_NRIdx, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_TableIndex(u16GRuleType,
                     _u16PQSrcType[eWindow],
                     u16PQ_NRIdx,
                     u16GRuleIPIndex,
                     &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = (MS_U8)MDrv_PQ_GetGRule_TableIndex_(SC2_MAIN, (MS_U8)u16GRuleType, (MS_U8)_u16PQSrcType[eWindow], (MS_U8)u16PQ_NRIdx, (MS_U8)u16GRuleIPIndex);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            u16Ret = (MS_U8) MDrv_PQBin_GetGRule_TableIndex(u16GRuleType,
                     _u16PQSrcType[eWindow],
                     u16PQ_NRIdx,
                     u16GRuleIPIndex,
                     &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN)
            u16Ret = 0;
#else
            u16Ret = (MS_U8)(MDrv_PQ_GetGRule_TableIndex_(SC2_SUB, (MS_U8)u16GRuleType, (MS_U8)_u16PQSrcType[eWindow], (MS_U8)u16PQ_NRIdx, (MS_U8)u16GRuleIPIndex));
#endif
        }
    }
#endif

#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }

    return u16Ret;
}
#endif


void MDrv_PQ_LoadTableData(PQ_WIN eWindow, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx, MS_U8 *pTable, MS_U16 u16TableSize)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableData((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                     u16TabIdx,
                                     u16PQIPIdx,
                                     &stPQBinHeaderInfo[PQ_BIN_STD_MAIN],
                                     pTable,
                                     u16TableSize);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTableData_(MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx, pTable, u16TableSize);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableData((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                     u16TabIdx,
                                     u16PQIPIdx,
                                     &stPQBinHeaderInfo[PQ_BIN_STD_SUB],
                                     pTable,
                                     u16TableSize);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTableData_(SUB, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx, pTable, u16TableSize);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableData((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                     u16TabIdx,
                                     u16PQIPIdx,
                                     &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN],
                                     pTable,
                                     u16TableSize);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTableData_(SC1_MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx, pTable, u16TableSize);
#endif
        }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableData((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                     u16TabIdx,
                                     u16PQIPIdx,
                                     &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN],
                                     pTable,
                                     u16TableSize);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTableData_(SC2_MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx, pTable, u16TableSize);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTableData((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                     u16TabIdx,
                                     u16PQIPIdx,
                                     &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB],
                                     pTable,
                                     u16TableSize);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTableData_(SC2_SUB, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx, pTable, u16TableSize);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
}

void MDrv_PQ_LoadTable(PQ_WIN eWindow, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                 u16TabIdx,
                                 u16PQIPIdx,
                                 &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTable_(MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                 u16TabIdx,
                                 u16PQIPIdx,
                                 &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTable_(SUB, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                 u16TabIdx,
                                 u16PQIPIdx,
                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTable_(SC1_MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx);
#endif
        }
    }

    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                 u16TabIdx,
                                 u16PQIPIdx,
                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTable_(SC2_MAIN, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_LoadTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                 u16TabIdx,
                                 u16PQIPIdx,
                                 &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_LoadTable_(SC2_SUB, (MS_U8)u16TabIdx, (MS_U8)u16PQIPIdx);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
}

void MDrv_PQ_CheckSettings(PQ_WIN eWindow)
{
    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            //ToDo MAINEX
            MDrv_PQBin_CheckCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                      &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);

            MDrv_PQBin_CheckTableBySrcType(_u16PQSrcType[eWindow],
                                           PQ_BIN_IP_ALL,
                                           (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_MAIN_WINDOW),
                                           &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
#if(ENABLE_PQ_EX)
            MDrv_PQ_CheckCommTable_(MAINEX);
            MDrv_PQ_CheckTableBySrcType_(MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            MDrv_PQ_CheckCommTable_(MAIN);
            MDrv_PQ_CheckTableBySrcType_(MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            //ToDo SUBEX
            MDrv_PQBin_CheckCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                      &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);

            MDrv_PQBin_CheckTableBySrcType(_u16PQSrcType[eWindow],
                                           PQ_BIN_IP_ALL,
                                           (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SUB_WINDOW),
                                           &stPQBinHeaderInfo[PQ_BIN_STD_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_CheckCommTable_(SUBEX);
            MDrv_PQ_CheckTableBySrcType_(SUBEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
            MDrv_PQ_CheckCommTable_(SUB);
            MDrv_PQ_CheckTableBySrcType_(SUB, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            //ToDo MAINEX
            MDrv_PQBin_CheckCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                      &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);

            MDrv_PQBin_CheckTableBySrcType(_u16PQSrcType[eWindow],
                                           PQ_BIN_IP_ALL,
                                           (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC1_MAIN_WINDOW),
                                           &stPQBinHeaderInfo[PQ_BIN_STD_SC1_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
#if(ENABLE_PQ_EX)
            MDrv_PQ_CheckCommTable_(SC1_MAINEX);
            MDrv_PQ_CheckTableBySrcType_(SC1_MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            MDrv_PQ_CheckCommTable_(SC1_MAIN);
            MDrv_PQ_CheckTableBySrcType_(SC1_MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
        }
    }

    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            //ToDo MAINEX
            MDrv_PQBin_CheckCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                      &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);

            MDrv_PQBin_CheckTableBySrcType(_u16PQSrcType[eWindow],
                                           PQ_BIN_IP_ALL,
                                           (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_MAIN_WINDOW),
                                           &stPQBinHeaderInfo[PQ_BIN_STD_SC2_MAIN]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
#if(ENABLE_PQ_EX)
            MDrv_PQ_CheckCommTable_(SC2_MAINEX);
            MDrv_PQ_CheckTableBySrcType_(SC2_MAINEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
            MDrv_PQ_CheckCommTable_(SC2_MAIN);
            MDrv_PQ_CheckTableBySrcType_(SC2_MAIN, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
        }
    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            //ToDo SUBEX
            MDrv_PQBin_CheckCommTable((MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                      &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);

            MDrv_PQBin_CheckTableBySrcType(_u16PQSrcType[eWindow],
                                           PQ_BIN_IP_ALL,
                                           (MS_U16)MDrv_PQBin_GetPanelIdx(PQ_SC2_SUB_WINDOW),
                                           &stPQBinHeaderInfo[PQ_BIN_STD_SC2_SUB]);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_CheckCommTable_(SC2_SUBEX);
            MDrv_PQ_CheckTableBySrcType_(SC2_SUBEX, _u16PQSrcType[eWindow], PQ_IP_ALL);
            MDrv_PQ_CheckCommTable_(SC2_SUB);
            MDrv_PQ_CheckTableBySrcType_(SC2_SUB, _u16PQSrcType[eWindow], PQ_IP_ALL);
#endif
        }
    }
#endif

#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }
}


#if(ENABLE_PQ_MLOAD)
void MDrv_PQ_Set_MLoadEn(PQ_WIN eWindow, MS_BOOL bEn)
{
    if(MApi_XC_MLoad_GetStatus(eWindow) != E_MLOAD_ENABLED)
    {
        bEn = FALSE;
    }

    if(bEn)
    {
        PQ_MLOAD_ENTRY();
    }

    if(eWindow == PQ_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)

        if(gbPQBinEnable)
        {
            MDrv_PQBin_Set_MLoadEn(bEn);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_Set_MLoadEn_(MAIN, bEn);
#endif
        }

    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_Set_MLoadEn(bEn);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_Set_MLoadEn_(SUB, bEn);
#endif
        }
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)

        if(gbPQBinEnable)
        {
            MDrv_PQBin_Set_MLoadEn(bEn);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_Set_MLoadEn_(SC1_MAIN, bEn);
#endif
        }

    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
#if(ENABLE_PQ_BIN)

        if(gbPQBinEnable)
        {
            MDrv_PQBin_Set_MLoadEn(bEn);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_Set_MLoadEn_(SC2_MAIN, bEn);
#endif
        }

    }
#if (PQ_ENABLE_PIP)
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
#if(ENABLE_PQ_BIN)
        if(gbPQBinEnable)
        {
            MDrv_PQBin_Set_MLoadEn(bEn);
        }
        else
#endif
        {
#if(PQ_ONLY_SUPPORT_BIN == 0)
            MDrv_PQ_Set_MLoadEn_(SC2_SUB, bEn);
#endif
        }
    }
#endif
#endif // ENABLE_MULTI_SCALER
    else
    {
        MS_ASSERT(0);
    }


    if(bEn == FALSE)
    {
        PQ_MLOAD_RETURN();
    }
}
#endif



void MDrv_PQ_Set_DTVInfo(PQ_WIN eWindow, MS_PQ_Dtv_Info *pstPQDTVInfo)
{
    MsOS_Memcpy(&_stDTV_Info[eWindow], pstPQDTVInfo, sizeof(MS_PQ_Dtv_Info));

    PQINFO_DBG(printf("PQ DTV Info:Win=%u, type=%u\r\n", eWindow, _stDTV_Info[eWindow].eType));
}

void MDrv_PQ_Set_MultiMediaInfo(PQ_WIN eWindow, MS_PQ_MuliMedia_Info *pstPQMMInfo)
{
    MsOS_Memcpy(&_stMultiMedia_Info[eWindow], pstPQMMInfo, sizeof(MS_PQ_MuliMedia_Info));
    PQINFO_DBG(printf("PQ MM Info:Win=%u, type=%u\r\n", eWindow, _stMultiMedia_Info[eWindow].eType));
}

void MDrv_PQ_Set_VDInfo(PQ_WIN eWindow, MS_PQ_Vd_Info *pstPQVDInfo)
{
    MsOS_Memcpy(&_stVD_Info[eWindow], pstPQVDInfo, sizeof(MS_PQ_Vd_Info));

    PQINFO_DBG(printf("PQ VD Info:Win=%u, SigType=%u, bSCARTRGB=%u, VIFIn=%u\r\n",
                      eWindow,
                      _stVD_Info[eWindow].enVideoStandard,
                      _stVD_Info[eWindow].bIsSCART_RGB,
                      _stVD_Info[eWindow].bIsVIFIN));

}

void MDrv_PQ_Set_ModeInfo(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType, MS_PQ_Mode_Info *pstPQModeInfo)
{

    MsOS_Memcpy(&_stMode_Info[eWindow], pstPQModeInfo, sizeof(MS_PQ_Mode_Info));

    //printf("PQ Set Mode Info: %s, Src=%x\r\n", (eWindow)?("SubWin"):("MainWin"), enInputSourceType);

    if(QM_IsSourceYPbPr(enInputSourceType))
    {
        if(((QM_H_Size_Check_x1(pstPQModeInfo->u16input_hsize, 720)) ||
                (QM_H_Size_Check_x2(pstPQModeInfo->u16input_hsize, 720)) ||
                (QM_H_Size_Check_x4(pstPQModeInfo->u16input_hsize, 720)) ||
                (QM_H_Size_Check_x8(pstPQModeInfo->u16input_hsize, 720))) &&
                (pstPQModeInfo->u16input_vsize < 500) &&
                (pstPQModeInfo->u16input_vfreq < 650))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->bInterlace) ?
                                    PQ_MD_720x480_60I :
                                    PQ_MD_720x480_60P;

        }
        else if(((QM_H_Size_Check_x1(pstPQModeInfo->u16input_hsize, 720)) ||
                 (QM_H_Size_Check_x2(pstPQModeInfo->u16input_hsize, 720)) ||
                 (QM_H_Size_Check_x4(pstPQModeInfo->u16input_hsize, 720)) ||
                 (QM_H_Size_Check_x8(pstPQModeInfo->u16input_hsize, 720))) &&
                (pstPQModeInfo->u16input_vsize < 600) &&
                (pstPQModeInfo->u16input_vfreq < 550))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->bInterlace) ?
                                    PQ_MD_720x576_50I :
                                    PQ_MD_720x576_50P;
        }
        else if(((QM_H_Size_Check_x1(pstPQModeInfo->u16input_hsize, 1280)) ||
                 (QM_H_Size_Check_x2(pstPQModeInfo->u16input_hsize, 1280)) ||
                 (QM_H_Size_Check_x4(pstPQModeInfo->u16input_hsize, 1280)) ||
                 (QM_H_Size_Check_x8(pstPQModeInfo->u16input_hsize, 1280))) &&
                (pstPQModeInfo->u16input_vsize < 800) &&
                (pstPQModeInfo->bInterlace == FALSE))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?
                                    PQ_MD_1280x720_50P :
                                    PQ_MD_1280x720_60P;

        }
        else if(((QM_H_Size_Check_x1(pstPQModeInfo->u16input_hsize, 1920)) ||
                 (QM_H_Size_Check_x2(pstPQModeInfo->u16input_hsize, 1920)) ||
                 (QM_H_Size_Check_x4(pstPQModeInfo->u16input_hsize, 1920)) ||
                 (QM_H_Size_Check_x8(pstPQModeInfo->u16input_hsize, 1920))) &&
                (pstPQModeInfo->u16input_vsize < 1100))
        {
            if(pstPQModeInfo->bInterlace)
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?
                                        PQ_MD_1920x1080_50I :
                                        PQ_MD_1920x1080_60I;
            }
            else
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 245) ?  PQ_MD_1920x1080_24P :
                                        (pstPQModeInfo->u16input_vfreq < 270) ?  PQ_MD_1920x1080_25P :
                                        (pstPQModeInfo->u16input_vfreq < 350) ?  PQ_MD_1920x1080_30P :
                                        (pstPQModeInfo->u16input_vfreq < 550) ?  PQ_MD_1920x1080_50P :
                                        PQ_MD_1920x1080_60P;

            }
        }
        else
        {
            _u8ModeIndex[eWindow] = PQ_MD_720x576_50I;
        }
    }
    else if(QM_IsSourceHDMI(enInputSourceType))
    {
        if((pstPQModeInfo->u16input_hsize < 1500) &&
                (pstPQModeInfo->u16input_vsize < 500) &&
                (pstPQModeInfo->u16input_vfreq < 650))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->bInterlace) ?
                                    PQ_MD_720x480_60I :
                                    PQ_MD_720x480_60P;

        }
        else if((pstPQModeInfo->u16input_hsize < 1500) &&
                (pstPQModeInfo->u16input_vsize < 600) &&
                (pstPQModeInfo->u16input_vfreq <  550))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->bInterlace) ?
                                    PQ_MD_720x576_50I :
                                    PQ_MD_720x576_50P;
        }
        else if((pstPQModeInfo->u16input_hsize < 1300) &&
                (pstPQModeInfo->u16input_vsize < 800) &&
                (pstPQModeInfo->bInterlace == FALSE))
        {
            _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?
                                    PQ_MD_1280x720_50P :
                                    PQ_MD_1280x720_60P;

        }
        else if((pstPQModeInfo->u16input_hsize < 1930) &&
                (pstPQModeInfo->u16input_vsize < 1100))
        {
            if(pstPQModeInfo->bInterlace)
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?
                                        PQ_MD_1920x1080_50I :
                                        PQ_MD_1920x1080_60I;
            }
            else
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 245) ?  PQ_MD_1920x1080_24P :
                                        (pstPQModeInfo->u16input_vfreq < 270) ?  PQ_MD_1920x1080_25P :
                                        (pstPQModeInfo->u16input_vfreq < 350) ?  PQ_MD_1920x1080_30P :
                                        (pstPQModeInfo->u16input_vfreq < 550) ?  PQ_MD_1920x1080_50P :
                                        PQ_MD_1920x1080_60P;

            }
        }
        else if(pstPQModeInfo->u16input_hsize < 1350 &&
                pstPQModeInfo->u16input_vsize < 1550)
        {
            //special handle 1280X1470p
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?  PQ_MD_1280x720_50P :
                               PQ_MD_1280x720_60P;
            }
        }
        else if(pstPQModeInfo->u16input_hsize < 1930 &&
                pstPQModeInfo->u16input_vsize < 2300)
        {
            //special handle 1920X2205p
            if(pstPQModeInfo->bInterlace)
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 550) ?
                               PQ_MD_1920x1080_50I :
                               PQ_MD_1920x1080_60I;
            }
            else
            {
                _u8ModeIndex[eWindow] = (pstPQModeInfo->u16input_vfreq < 250) ?  PQ_MD_1920x1080_24P :
                               (pstPQModeInfo->u16input_vfreq < 350) ?  PQ_MD_1920x1080_30P :
                               (pstPQModeInfo->u16input_vfreq < 550) ?  PQ_MD_1920x1080_50P :
                               PQ_MD_1920x1080_60P;

            }
        }
        else
        {
            _u8ModeIndex[eWindow] = PQ_MD_720x576_50I;
        }
    }
    else
    {
        _u8ModeIndex[eWindow] = PQ_MD_Num;
    }

    if(QM_IsSourceYPbPr(enInputSourceType) || QM_IsSourceHDMI(enInputSourceType))
    {
        if(_u8ModeIndex[eWindow] <= (MS_U8)PQ_MD_720x576_50P)
            _gIsSrcHDMode[eWindow] = 0;
        else
            _gIsSrcHDMode[eWindow] = 1;

    }
    else
    {
        if(pstPQModeInfo->u16input_hsize >= 1200)
            _gIsSrcHDMode[eWindow] = 1;
        else
            _gIsSrcHDMode[eWindow] = 0;
    }

    PQINFO_DBG(
        printf("PQ ModeInfo:%d input(%d, %d), disp(%d, %d), ModeIdx=%d, FBL=%u, Interlace=%u, InV=%u, OutV=%u, inVtt=%u\r\n",
               eWindow,
               _stMode_Info[eWindow].u16input_hsize,
               _stMode_Info[eWindow].u16input_vsize,
               _stMode_Info[eWindow].u16display_hsize,
               _stMode_Info[eWindow].u16display_vsize,
               _u8ModeIndex[eWindow],
               _stMode_Info[eWindow].bFBL,
               _stMode_Info[eWindow].bInterlace,
               _stMode_Info[eWindow].u16input_vfreq,
               _stMode_Info[eWindow].u16ouput_vfreq,
               _stMode_Info[eWindow].u16input_vtotal);
    );

}

void MDrv_PQ_SetHDMIInfo(PQ_WIN eWindow, const MS_PQ_Hdmi_Info* const pstPQHDMIInfo)
{
    _stHDMI_Info[eWindow].bIsHDMI = pstPQHDMIInfo->bIsHDMI;
    _stHDMI_Info[eWindow].enColorFmt = pstPQHDMIInfo->enColorFmt;

    PQINFO_DBG(printf("PQ HDMI, bHDMI=%u, colorfmt=%u\r\n",
                      _stHDMI_Info[eWindow].bIsHDMI,
                      _stHDMI_Info[eWindow].enColorFmt));
}

void MDrv_PQ_SetHDMI_PC(PQ_WIN eWindow, MS_BOOL bIsTrue)
{
    _stHDMI_Info[eWindow].bIsHDMIPC = bIsTrue;
}

MS_BOOL MDrv_PQ_GetHDMI_PC_Status(PQ_WIN eWindow)
{
    return _stHDMI_Info[eWindow].bIsHDMIPC;
}



PQ_INPUT_SOURCE_TYPE MDrv_PQ_GetInputSourceType(PQ_WIN eWindow)
{
    return _enInputSourceType[eWindow];
}

MS_U16 MDrv_PQ_GetSrcType(PQ_WIN eWindow)
{
    return _u16PQSrcType[eWindow];
}

// if not HSD case, ignore return value
static MS_BOOL _MDrv_PQ_LoadScalingTable(PQ_WIN eWindow,
        MS_U8 eXRuleType,
        MS_U8 u8XRuleIP,
        MS_BOOL bPreV_ScalingDown,
        MS_BOOL bInterlace,
        MS_BOOL bColorSpaceYUV,
        MS_U16 u16InputSize,
        MS_U16 u16SizeAfterScaling)
{
    MS_U32 u32Ratio;
    MS_U16 u16IPIdx;
#if(ENABLE_PQ_BIN)
    MS_U16 u16TabIdx = gbPQBinEnable == 0 ? PQ_IP_NULL : PQ_BIN_IP_NULL;
#else
    MS_U16 u16TabIdx = PQ_IP_NULL;
#endif
    MS_U8 u8XRuleIdx = 0xFF;

    if(u16InputSize == 0)
        u32Ratio = 2000;
    else
        u32Ratio = ((MS_U32) u16SizeAfterScaling * 1000) / u16InputSize;

    u16IPIdx = MDrv_PQ_GetXRuleIPIndex(eWindow, eXRuleType, u8XRuleIP);

    SRULE_DBG(printf("XRuleIP=%d, IPIdx=%d, input=%d, output=%d, ratio=%ld, \r\n",
                     (MS_U16)u8XRuleIP, (MS_U16)u16IPIdx,
                     u16InputSize, u16SizeAfterScaling, u32Ratio));

#if PQ_ENABLE_HSDRULE
    if(bPreV_ScalingDown && bInterlace)
    {
        u8XRuleIdx = PQ_HSDRule_PreV_ScalingDown_Interlace_Main;
        u16TabIdx = MDrv_PQ_GetXRuleTableIndex(eWindow, (MS_U16)eXRuleType, (MS_U16)u8XRuleIdx, (MS_U16)u8XRuleIP);
    }
    else if(bPreV_ScalingDown && (!bInterlace))
    {
        u8XRuleIdx = PQ_HSDRule_PreV_ScalingDown_Progressive_Main;
        u16TabIdx = MDrv_PQ_GetXRuleTableIndex(eWindow, (MS_U16)eXRuleType, (MS_U16)u8XRuleIdx, (MS_U16)u8XRuleIP);
    }
#endif

#if(ENABLE_PQ_BIN)
    if(((gbPQBinEnable == 0) && (u16TabIdx != PQ_IP_NULL)) ||
            ((gbPQBinEnable == 1) && (u16TabIdx != PQ_BIN_IP_NULL)))
#else
    if(u16TabIdx != PQ_IP_NULL)
#endif
    {

        SRULE_DBG(printf("u8XRuleIdx: PreV down, interlace:%u", bInterlace));
        SRULE_DBG(printf("(a)tabidx=%u\r\n", (MS_U16)u16TabIdx));
    }

#if(ENABLE_PQ_BIN)
    if(((gbPQBinEnable == 1) && (u16TabIdx == PQ_BIN_IP_NULL)) ||
            ((gbPQBinEnable == 0) && (u16TabIdx == PQ_IP_NULL)))
#else
    if(u16TabIdx == PQ_IP_NULL)
#endif
    {
        if(u32Ratio > 1000)
        {
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
            SRULE_DBG(printf("u8XRuleIdx: >x1, "));
            SRULE_DBG(printf("(c)tabidx=%u\r\n", (MS_U16)u16TabIdx));
        }
        else
        {

        #if PQ_ENABLE_HSDRULE
            if(bColorSpaceYUV)
            {
                if(u32Ratio == 1000)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_10x_YUV_Main;
                else if(u32Ratio >= 900)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_09x_YUV_Main;
                else if(u32Ratio >= 800)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_08x_YUV_Main;
                else if(u32Ratio >= 700)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_07x_YUV_Main;
                else if(u32Ratio >= 600)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_06x_YUV_Main;
                else if(u32Ratio >= 500)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_05x_YUV_Main;
                else if(u32Ratio >= 400)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_04x_YUV_Main;
                else if(u32Ratio >= 300)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_03x_YUV_Main;
                else if(u32Ratio >= 200)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_02x_YUV_Main;
                else if(u32Ratio >= 100)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_01x_YUV_Main;
                else
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_00x_YUV_Main;
            }
            else
            {
                if(u32Ratio == 1000)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_10x_RGB_Main;
                else if(u32Ratio >= 900)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_09x_RGB_Main;
                else if(u32Ratio >= 800)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_08x_RGB_Main;
                else if(u32Ratio >= 700)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_07x_RGB_Main;
                else if(u32Ratio >= 600)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_06x_RGB_Main;
                else if(u32Ratio >= 500)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_05x_RGB_Main;
                else if(u32Ratio >= 400)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_04x_RGB_Main;
                else if(u32Ratio >= 300)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_03x_RGB_Main;
                else if(u32Ratio >= 200)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_02x_RGB_Main;
                else if(u32Ratio >= 100)
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_01x_RGB_Main;
                else
                    u8XRuleIdx = PQ_HSDRule_ScalingDown_00x_RGB_Main;
            }
        #endif
            SRULE_DBG(printf("u8XRuleIdx=%u, ", (MS_U16)u8XRuleIdx));
            if(u8XRuleIdx == 0xFF)
            {
                MS_ASSERT(0);
                return 1;
            }

            u16TabIdx = MDrv_PQ_GetXRuleTableIndex(eWindow, (MS_U16)eXRuleType, (MS_U16)u8XRuleIdx, (MS_U16)u8XRuleIP);
#if(ENABLE_PQ_BIN)
            if(((gbPQBinEnable == 1) && (u16TabIdx == PQ_BIN_IP_NULL)) ||
                    ((gbPQBinEnable == 0) && (u16TabIdx == PQ_IP_NULL)))
#else
            if(u16TabIdx == PQ_IP_NULL)
#endif
            {
                u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
                SRULE_DBG(printf("(d)tabidx=%u\r\n", u16TabIdx));
            }
            else
            {
                SRULE_DBG(printf("(e)tabidx=%u\r\n", u16TabIdx));
            }
        }
    }

#if PQ_EN_DMS_SW_CTRL
    // App will call halt_subwin to disconnet sub window.
    // It will casue PQ display type to be PQ_DISPLAY_ONE.
    // But, sub window is still on and DMS_LV_12 will be set
    // to On in HSD rulee for main window.
    // For fix this issue, we need to check sub window wheter
    // on or off to descide DMS_LV_12 off or on.

    //Disable DMS_V12_L
    // 1.VSD : V prescaling, 2. PIP, 3. FBL
        if((((eXRuleType == E_XRULE_VSD)&&(bPreV_ScalingDown))
            ||_stMode_Info[eWindow].bFBL
            || (E_XC_3D_INPUT_MODE_NONE != MApi_XC_Get_3D_Input_Mode(MAIN_WINDOW))
            ) && (PQ_IP_DMS_V_12L_Main == MDrv_PQ_GetXRuleIPIndex(eWindow, eXRuleType, u8XRuleIP))
        )
        {
            //Disable De-Mosquito
            u16TabIdx = PQ_IP_DMS_V_12L_OFF_Main;
        }
    #endif

    SRULE_DBG(printf("LoadScalingTable: TabIdx:%d IPIdx:%d \r\n", u16TabIdx, u16IPIdx));
    MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);

#if PQ_ENABLE_HSDRULE

    if(eXRuleType == E_XRULE_VSD)
        return (u16TabIdx  == PQ_IP_VSD_Bilinear_Main) ; // PreVSDMode: 0:Cb, 1:Bilinear
    else
        return (u16TabIdx != PQ_IP_HSD_Y_CB_Main); // PreHSDMode - 0:Cb, 1:Adv
#else

        return 1;
#endif
}

MS_BOOL MDrv_PQ_LoadScalingTable(PQ_WIN eWindow,
                                 MS_U8 eXRuleType,
                                 MS_BOOL bPreV_ScalingDown,
                                 MS_BOOL bInterlace,
                                 MS_BOOL bColorSpaceYUV,
                                 MS_U16 u16InputSize,
                                 MS_U16 u16SizeAfterScaling)
{
    MS_BOOL bRet = 0; // default is CB mode
    MS_U16 i;

    if(eXRuleType > 3)
        MS_ASSERT(0);

    SRULE_DBG(printf("[PQ_LoadScalingTable] HSD/VSD/HSP/VSP:%u\r\n", (MS_U16)eXRuleType));
    SRULE_DBG(printf("Pre_ScalingDown:%d, bInter:%d, bYUV:%d \r\n", bPreV_ScalingDown, bInterlace, bColorSpaceYUV));

    for(i = 0; i < MDrv_PQ_GetXRuleIPNum(eWindow, (MS_U16)eXRuleType); i++)
    {
        MS_BOOL bSDMode;
        bSDMode = _MDrv_PQ_LoadScalingTable(eWindow,
                                            eXRuleType,
                                            (MS_U8)i,
                                            bPreV_ScalingDown,
                                            bInterlace,
                                            bColorSpaceYUV,
                                            u16InputSize,
                                            u16SizeAfterScaling);
        bRet |= (bSDMode << (1 * i));
    }

    if(eXRuleType == E_XRULE_VSD)
    {
        bRet &= 0x01;
    }
#if 0
    //121 filter
    if(((MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_LINE_ALTERNATIVE) ||
        (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_TOP_BOTTOM)) &&
        (MApi_XC_Get_3D_HW_Version() > 0))
    {
        if((u16InputSize >= u16SizeAfterScaling) && (eXRuleType == E_XRULE_VSP))
        {
            //if post V down, we will load 121 filter, and when it's post scaling case
            //reload 121 filter's VSP_Y=SRAM_1_4Tap, VSP_C=C_SRAM_1,
            //                    SRAM1=InvSinc4Tc4p4Fc50Apass01Astop55, C_SRAM1=C121 for main-win
            //reload 121 filter's VSP_Y=SRAM_3_4Tap, VSP_C=C_SRAM_3,
            //                    SRAM3=InvSinc4Tc4p4Fc50Apass01Astop55, C_SRAM3=C121 for sub-win
            MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VSP_Y_SRAM_1_4Tap_Main, PQ_IP_VSP_Y_Main);
            MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VSP_C_C_SRAM_1_Main, PQ_IP_VSP_C_Main);
            MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_SRAM1_InvSinc4Tc4p4Fc50Apass01Astop55_Main, PQ_IP_SRAM1_Main);
            MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_C_SRAM1_C121_Main, PQ_IP_C_SRAM1_Main);

            MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_VSP_Y_SRAM_3_4Tap_Sub, PQ_IP_VSP_Y_Sub);
            MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_VSP_C_C_SRAM_3_Sub, PQ_IP_VSP_C_Sub);
            MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_SRAM3_InvSinc4Tc4p4Fc50Apass01Astop55_Sub, PQ_IP_SRAM3_Sub);
            MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_C_SRAM3_C121_Sub, PQ_IP_C_SRAM3_Sub);
        }
    }
#endif

    return bRet;
}


//////////////////////////////////////////////////////////////////
// set color range of input source and take effect immediately
//
void MDrv_PQ_SetColorRange(PQ_WIN eWindow, MS_BOOL bColorRange0_255)
{
    _bColorRange0_255[eWindow] = bColorRange0_255;
    MDrv_PQ_SetCSC(eWindow, _gFourceColorFmt[eWindow]);
}

MS_BOOL MDrv_PQ_Get_CSC_XRuleIdx(MS_BOOL *pbInputColorSpaceRGB,
                                            MS_U16 * pu16DoCSC,
                                            PQ_WIN eWindow,
                                            PQ_FOURCE_COLOR_FMT enFourceColor,
                                            MS_BOOL bLoadPQTable)
{
#if  PQ_ENABLE_CSCRULE

    MS_U16 u16XRuleIdx, u16XRuleIP;
    MS_U16 u16IPIdx, u16TabIdx;
    MS_U16 eXRuleType = E_XRULE_CSC;
    MS_BOOL bInputTypeVideo;
    MS_BOOL bInputResolutionHD = _gIsSrcHDMode[eWindow];
    MS_U16 u16YUV_CSC, u16RGB_CSC;

    _gFourceColorFmt[eWindow] = enFourceColor;

#if ENABLE_VGA_EIA_TIMING
    if ((QM_IsSourceVGA(_enInputSourceType[eWindow]) ||
        QM_IsSourceDVI_HDMIPC(_enInputSourceType[eWindow], eWindow)) && !QM_IsInterlaced(eWindow))
#else
    if(QM_IsSourceVGA(_enInputSourceType[eWindow]) ||
       (QM_IsSourceDVI_HDMIPC(_enInputSourceType[eWindow], eWindow) && !QM_IsInterlaced(eWindow)))
#endif
    {
        CSCRULE_DBG(printf("PC mode\r\n"));
        bInputTypeVideo = FALSE;

#if (PQ_ENABLE_PIP == 1)
        if(eWindow == PQ_SUB_WINDOW)
        {
            printf("[%s][%d] VGA in sub video case, force PC mode to video mode \r\n", __FUNCTION__, __LINE__);
            bInputTypeVideo = TRUE;
        }
#endif
    }
    else
    {
        CSCRULE_DBG(printf("VIDEO mode\r\n"));
        bInputTypeVideo = TRUE;
    }

    if(MDrv_PQ_Get_PointToPoint(PQ_MAIN_WINDOW))
    {
        CSCRULE_DBG(printf("PointToPoint mode\r\n"));
        bInputTypeVideo = FALSE;
    }

    if( QM_IsSourceVGA(_enInputSourceType[eWindow]) ||
       (QM_IsSourceDVI_HDMIPC(_enInputSourceType[eWindow],eWindow)) ||
       (QM_IsSourceHDMI_Video(_enInputSourceType[eWindow],eWindow) && (QM_HDMIPC_COLORRGB(eWindow))) )
    {
        CSCRULE_DBG(printf("Input RGB\r\n"));
        (*pbInputColorSpaceRGB) = TRUE;
    }
    else
    {
        CSCRULE_DBG(printf("Input YUV\r\n"));
        (*pbInputColorSpaceRGB) = FALSE;
    }

    if (bInputTypeVideo)
    {
        if (*pbInputColorSpaceRGB)
        {
            if (eWindow == PQ_MAIN_WINDOW
            #if ENABLE_MULTI_SCALER
                || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
            #endif
               )
            {

                if (bInputResolutionHD == FALSE)
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_SD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_RGB_SD_0_255_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_0_255_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_0_255_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC : // fource to RGB and input is RGB, hence no need change
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_SD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_RGB_SD_16_235_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_SC2_Main :
                            #endif
                                                                    0xFFFF;


                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
                else // InputResolutionHD
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_HD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_RGB_HD_0_255_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_HD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_RGB_HD_16_235_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_SC2_Main :
                            #endif
                                                                    0xFFFF;


                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
            }
#if PQ_ENABLE_PIP
            else
            {
                if (bInputResolutionHD == FALSE)
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_SD_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_CSCRule_Video_YUV_SD_SC2_Sub :
                                #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW     ? PQ_CSCRule_Video_RGB_SD_0_255_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_CSCRule_Video_RGB_SD_0_255_SC2_Sub :
                                #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC : // fource to RGB and input is RGB, hence no need change
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_SD_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC2_Sub :
                                #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_SC2_Sub :
                                #endif
                                                                    0xFFFF;


                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
                else // InputResolutionHD
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Sub :
                                #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_SC2_Sub :
                                #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Sub :
                                #endif
                                                                   0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_Sub :
                                #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_SC2_Sub :
                                #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
            }
#endif
        }
        else // InputColorSpaceYUV
        {
            if(enFourceColor == PQ_FOURCE_COLOR_RGB )
            {
                MS_ASSERT(0);
                PQTAB_DBG(printf("InputColorSpace is YUV, Fource Color Space is RGB!!!\r\n"));
            }

            if(eWindow == PQ_MAIN_WINDOW || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW)
            {
                if (bInputResolutionHD == FALSE)
                {
                   u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_SD_Main :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC1_Main :
                                eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC2_Main :
                        #endif
                                                                0xFFFF;
                    u16XRuleIdx = u16YUV_CSC;
                }
                else // InputResolutionHD
                {
                   u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_Video_YUV_HD_Main :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC1_Main :
                                eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Main :
                        #endif
                                                                0xFFFF;

                    u16XRuleIdx = u16YUV_CSC;
                }

                if(enFourceColor == PQ_FOURCE_COLOR_RGB)
                {
                    if(bInputResolutionHD)
                    {
                        if(_bColorRange0_255[eWindow])
                        {
                            u16XRuleIdx = eWindow == PQ_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_Main :
                                #if ENABLE_MULTI_SCALER
                                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_SC1_Main :
                                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_0_255_SC2_Main :
                                #endif
                                                                        0xFFFF;
                        }
                        else
                        {
                            u16XRuleIdx = eWindow == PQ_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_Main :
                                #if ENABLE_MULTI_SCALER
                                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_SC1_Main :
                                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_HD_16_235_SC2_Main :
                                #endif
                                                                        0xFFFF;
                        }
                    }
                    else
                    {
                        if(_bColorRange0_255[eWindow])
                        {
                            u16XRuleIdx = eWindow == PQ_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_0_255_Main :
                                #if ENABLE_MULTI_SCALER
                                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_0_255_SC1_Main :
                                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_0_255_SC2_Main :
                                #endif
                                                                        0xFFFF;
                        }
                        else
                        {
                            u16XRuleIdx = eWindow == PQ_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_Main :
                                #if ENABLE_MULTI_SCALER
                                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_SC1_Main :
                                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_Video_RGB_SD_16_235_SC2_Main :
                                #endif
                                                                        0xFFFF;
                        }
                    }
                }
            }
#if PQ_ENABLE_PIP
            else
            {
                if (bInputResolutionHD == FALSE)
                {
                    u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_SD_Sub :
                            #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_YUV_SD_SC2_Sub :
                            #endif
                                                                0xFFFF;

                    u16XRuleIdx = u16YUV_CSC;
                }
                else // InputResolutionHD
                {
                    u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_Sub :
                            #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_Video_YUV_HD_SC2_Sub :
                            #endif
                                                                0xFFFF;

                    u16XRuleIdx = u16YUV_CSC;
                }
            }
#endif
        }
    }
    else // InputTypePC
    {
        if (*pbInputColorSpaceRGB)
        {
            if (eWindow == PQ_MAIN_WINDOW || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW)
            {
                if (bInputResolutionHD == FALSE)
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_SD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Main :
                            #endif
                                                                    0xFFFF;


                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_RGB_SD_0_255_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_SD_0_255_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_SD_0_255_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_SD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_RGB_SD_16_235_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_SD_16_235_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_SD_16_235_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
                else // InputResolutionHD
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_HD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Main :
                            #endif
                                                                    0xFFFF;

                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_RGB_HD_0_255_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_HD_0_255_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_HD_0_255_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_HD_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Main :
                            #endif
                                                                    0xFFFF;


                       u16RGB_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_RGB_HD_16_235_Main :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_HD_16_235_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_RGB_HD_16_235_SC2_Main :
                            #endif
                                                                    0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
            }
#if PQ_ENABLE_PIP
            else
            {
                if (bInputResolutionHD == FALSE)
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_RGB_SD_0_255_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_RGB_SD_0_255_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_RGB_SD_16_235_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_RGB_SD_16_235_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
                else // InputResolutionHD
                {
                    if (_bColorRange0_255[eWindow])
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_RGB_HD_0_255_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_RGB_HD_0_255_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                    else // bInputDataRange16_235
                    {
                       u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                       u16RGB_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_RGB_HD_16_235_Sub :
                            #if ENABLE_MULTI_SCALER
                                    eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_RGB_HD_16_235_SC2_Sub :
                            #endif
                                                                   0xFFFF;

                        u16XRuleIdx = enFourceColor == PQ_FOURCE_COLOR_RGB ?
                                                       u16YUV_CSC :
                                                       u16RGB_CSC;
                    }
                }
            }
#endif
        }
        else // InputColorSpaceYUV
        {
            if(enFourceColor == PQ_FOURCE_COLOR_RGB )
            {
                MS_ASSERT(0);
                PQTAB_DBG(printf("InputColorSpace is YUV, Fource Color Space is RGB!!!\r\n"));
            }

            if(eWindow == PQ_MAIN_WINDOW || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW)
            {
                if (bInputResolutionHD == FALSE)
                {
                   u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_SD_Main :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC1_Main :
                                eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Main :
                        #endif
                                                                0xFFFF;

                    u16XRuleIdx = u16YUV_CSC;
                }
                else // InputResolutionHD
                {
                   u16YUV_CSC = eWindow == PQ_MAIN_WINDOW     ? PQ_CSCRule_PC_YUV_HD_Main :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC1_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC1_Main :
                                eWindow == PQ_SC2_MAIN_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Main :
                        #endif
                                                                0xFFFF;

                    u16XRuleIdx = u16YUV_CSC;
                }
            }
#if PQ_ENABLE_PIP
            else
            {
                if (bInputResolutionHD == FALSE)
                {
                    u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_Sub :
                            #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_SD_SC2_Sub :
                            #endif
                                                               0xFFFF;
                    u16XRuleIdx = u16YUV_CSC;
                }
                else // InputResolutionHD
                {
                    u16YUV_CSC = eWindow == PQ_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_Sub :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC2_SUB_WINDOW ? PQ_CSCRule_PC_YUV_HD_SC2_Sub :
                        #endif
                                                               0xFFFF;
                    u16XRuleIdx = u16YUV_CSC;
                }
            }
#endif
        }
    }

    for(u16XRuleIP=0; u16XRuleIP<MDrv_PQ_GetXRuleIPNum(eWindow, eXRuleType); u16XRuleIP++)
    {
        u16IPIdx = MDrv_PQ_GetXRuleIPIndex(eWindow, eXRuleType, u16XRuleIP);
        u16TabIdx = MDrv_PQ_GetXRuleTableIndex(eWindow, eXRuleType, u16XRuleIdx, u16XRuleIP);
        CSCRULE_DBG(printf("u16XRuleIdx:%u, u16XRuleIP=%u, IPIdx=%u, TabIdx=%u\r\n", u16XRuleIdx, u16XRuleIP, u16IPIdx, u16TabIdx));

        if(bLoadPQTable)
        {
            MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
        }

        if(u16TabIdx)
        {
            if(u16IPIdx == Hal_PQ_get_csc_ip_idx(MS_CSC_IP_CSC))
            {
                (*pu16DoCSC) |= 0x1;
            }
            else if(u16IPIdx == Hal_PQ_get_csc_ip_idx(MS_CSC_IP_VIP_CSC))
            {
                (*pu16DoCSC) |= 0x02;
            }
            else
            {
                (*pu16DoCSC) |= 0x00;
            }
        }
    }
#endif
    return true;
}

//////////////////////////////////////////////////////////////////
// get memory color format
//
MS_BOOL MDrv_PQ_Get_MemYUVFmt(PQ_WIN eWindow, PQ_FOURCE_COLOR_FMT enFourceColor)
{
    MS_U16 u16DoCSC = 0;
    MS_BOOL bInputColorSpaceRGB = 0;
    MS_BOOL bMemYUV;

    MDrv_PQ_Get_CSC_XRuleIdx(&bInputColorSpaceRGB, &u16DoCSC, eWindow, enFourceColor, FALSE);

    if(bInputColorSpaceRGB)
    {
        if(u16DoCSC & 0x01)
        {
            bMemYUV = TRUE;
        }
        else
        {
            bMemYUV = FALSE;
        }
    }
    else
    {
        bMemYUV = TRUE;
    }

    return bMemYUV;
}


//////////////////////////////////////////////////////////////////
// do color space conversion from RGB to YUV according to input sourcce and context
// return:
//        TRUE: use YUV color space
//        FALSE: otherwise
//
MS_BOOL MDrv_PQ_SetCSC(PQ_WIN eWindow, PQ_FOURCE_COLOR_FMT enFourceColor)
{
    MS_U16 u16DoCSC = 0;
    MS_BOOL bInputColorSpaceRGB = 0;


    MDrv_PQ_Get_CSC_XRuleIdx(&bInputColorSpaceRGB, &u16DoCSC, eWindow, enFourceColor, TRUE);

    u16DoCSC = (u16DoCSC != 0);
    if ((bInputColorSpaceRGB && u16DoCSC) || (!bInputColorSpaceRGB))
    {
        return TRUE;    // use YUV space
    }
    else
    {
        return FALSE;   // use RGB space
    }
}

#if (PQ_GRULE_NR_ENABLE)
static void _MDrv_PQ_LoadNRTable(PQ_WIN eWindow, MS_U16 u16PQ_NRIdx)
{
    MS_U16 i, u16IPIdx = 0, u16TabIdx = 0;

    if (eWindow == PQ_MAIN_WINDOW || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW)
    {
        MS_U16 u16GRule_NR_Num = eWindow == PQ_MAIN_WINDOW     ? PQ_GRULE_NR_NUM_Main :
                        #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRULE_NR_NUM_SC1_Main :
                                 eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRULE_NR_NUM_SC2_Main :
                        #endif
                                                                0xFFFF;

        MS_U16 u16GRule_NR_IP_Num =  eWindow == PQ_MAIN_WINDOW     ? PQ_GRULE_NR_IP_NUM_Main :
                            #if ENABLE_MULTI_SCALER
                                     eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRULE_NR_IP_NUM_SC1_Main :
                                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRULE_NR_IP_NUM_SC2_Main :
                            #endif
                                                                    0xFFFF;


        MS_U16 u16GRule_Type =  eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Main :
                        #if ENABLE_MULTI_SCALER
                                eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_SC1_Main :
                                eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_SC2_Main :
                        #endif
                                                                    0xFFFF;
        if(u16PQ_NRIdx > u16GRule_NR_Num)
        {
            MS_ASSERT(0);
            return;
        }

        for(i = 0; i < u16GRule_NR_IP_Num; i++)
        {
            u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, u16GRule_Type, i);

            if(u16PQ_NRIdx == u16GRule_NR_Num)  // see it as default
                u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
            else
                u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, u16GRule_Type, u16PQ_NRIdx, i);

            PQGRULE_DBG(printf("[NR]SRC: %u, NR: %u, NRIPIdx:%u, IPIdx:%u, u16TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_NRIdx, i, u16IPIdx, u16TabIdx));
            MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
        }
    }
#if PQ_ENABLE_PIP
    else if (eWindow == PQ_SUB_WINDOW)
    {
        MS_U16 u16GRule_NR_Num = eWindow == PQ_SUB_WINDOW     ? PQ_GRULE_NR_NUM_Sub :
                        #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRULE_NR_NUM_SC2_Sub :
                        #endif
                                                                0xFFFF;

        MS_U16 u16GRule_NR_IP_Num =  eWindow == PQ_SUB_WINDOW ? PQ_GRULE_NR_IP_NUM_Sub :
                        #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRULE_NR_IP_NUM_SC2_Sub :
                        #endif
                                                                0xFFFF;


        MS_U16 u16GRule_Type =  eWindow == PQ_SUB_WINDOW      ? PQ_GRule_NR_Sub :
                        #if ENABLE_MULTI_SCALER
                                 eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_SC2_Sub :
                        #endif
                                                                0xFFFF;

        if(u16PQ_NRIdx > u16GRule_NR_Num)
        {
            MS_ASSERT(0);
            return;
        }

        for(i = 0; i < u16GRule_NR_IP_Num; i++)
        {
            u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, u16GRule_Type, i);

            if(u16PQ_NRIdx == u16GRule_NR_Num)  // see it as default
                u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
            else
                u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, u16GRule_Type, u16PQ_NRIdx, i);

            PQGRULE_DBG(printf("[NR]SRC: %u, NR: %u, NRIPIdx:%u, IPIdx:%u, u16TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_NRIdx, i, u16IPIdx, u16TabIdx));
            MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
        }
    }
#endif
    else
    {
        PQGRULE_DBG(printf("[NR] Not support case \r\n"));
    }
}
#endif


void MDrv_PQ_LoadNRTable(PQ_WIN eWindow, PQ_3D_NR_FUNCTION_TYPE en3DNRType)
{
#if (PQ_GRULE_NR_ENABLE)

    MS_U16 u16PQ_NRIdx;
    MS_U16 u16GRule_Type, u16GRule_Level;

    PQGRULE_DBG( printf("[PQ_LoadNRTable] ") );

    if (eWindow == PQ_MAIN_WINDOW || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW)
    {
        u16GRule_Type = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Main :
                    #if ENABLE_MULTI_SCALER
                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_SC1_Main :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_SC2_Main :
                    #endif
                                                        0xFFFF;


        if(en3DNRType == PQ_3D_NR_OFF)
        {
            PQGRULE_DBG(printf("Off\r\n"));
            u16GRule_Level = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Off_Main :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_Off_SC1_Main :
                             eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_Off_SC2_Main :
                        #endif
                                                             0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_AUTO)
        {
            // Auto NR will be process in msAPI_DynamicNR_Handler(), hence we only need to pre-setup the PQ table.
            PQGRULE_DBG(printf("Auto\r\n"));
            u16GRule_Level = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Low_Main :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_Low_SC1_Main :
                             eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_Low_SC2_Main :
                        #endif
                                                             0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_LOW)
        {
            PQGRULE_DBG(printf("Low\r\n"));
            u16GRule_Level = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Low_Main :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_Low_SC1_Main :
                             eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_Low_SC2_Main :
                        #endif
                                                             0xFFFF;

            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_MID)
        {
            PQGRULE_DBG(printf("Mid\r\n"));
            u16GRule_Level = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_Middle_Main :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_Middle_SC1_Main :
                             eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_Middle_SC1_Main :
                        #endif
                                                             0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_HIGH)
        {
            PQGRULE_DBG(printf("High\r\n"));
            u16GRule_Level = eWindow == PQ_MAIN_WINDOW     ? PQ_GRule_NR_High_Main :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRule_NR_High_SC1_Main :
                             eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRule_NR_High_SC2_Main :
                        #endif
                                                             0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_DEFAULT)
        {
            MS_U16 u16NR_Idx = eWindow == PQ_MAIN_WINDOW     ? PQ_GRULE_NR_NUM_Main :
                        #if ENABLE_MULTI_SCALER
                               eWindow == PQ_SC1_MAIN_WINDOW ? PQ_GRULE_NR_NUM_SC1_Main :
                               eWindow == PQ_SC2_MAIN_WINDOW ? PQ_GRULE_NR_NUM_SC2_Main :
                        #endif
                                                                0xFFFF;
            PQGRULE_DBG(printf("Default\r\n"));
            _MDrv_PQ_LoadNRTable(eWindow,  u16NR_Idx);
        }
        else
        {
            MS_ASSERT(0);
        }
    }
#if PQ_ENABLE_PIP
    else if (eWindow == PQ_SUB_WINDOW)
    {
        u16GRule_Type = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_Sub :
                    #if ENABLE_MULTI_SCALER
                        eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_SC2_Sub :
                    #endif
                                                       0xFFFF;

        if(en3DNRType == PQ_3D_NR_OFF)
        {
            PQGRULE_DBG(printf("Off\r\n"));
            u16GRule_Level = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_Off_Sub :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_Off_SC2_Sub :
                        #endif
                                                            0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_AUTO)
        {
            // Auto NR will be process in msAPI_DynamicNR_Handler(), hence we only need to pre-setup the PQ table.
            PQGRULE_DBG(printf("Auto\r\n"));
            u16GRule_Level = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_Low_Sub :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_Low_SC2_Sub :
                        #endif
                                                            0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_LOW)
        {
            PQGRULE_DBG(printf("Low\r\n"));
            u16GRule_Level = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_Low_Sub :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_Low_SC2_Sub :
                        #endif
                                                            0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_MID)
        {
            PQGRULE_DBG(printf("Mid\r\n"));
            u16GRule_Level = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_Middle_Sub :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_Middle_SC2_Sub :
                        #endif
                                                            0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_HIGH)
        {
            PQGRULE_DBG(printf("High\r\n"));
            u16GRule_Level = eWindow == PQ_SUB_WINDOW     ? PQ_GRule_NR_High_Sub :
                        #if ENABLE_MULTI_SCALER
                             eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRule_NR_High_SC2_Sub :
                        #endif
                                                            0xFFFF;
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, u16GRule_Type, u16GRule_Level);
            _MDrv_PQ_LoadNRTable(eWindow, u16PQ_NRIdx);
        }
        else if(en3DNRType == PQ_3D_NR_DEFAULT)
        {
            MS_U16 u16NR_Idx = eWindow == PQ_SUB_WINDOW     ? PQ_GRULE_NR_NUM_Sub :
                        #if ENABLE_MULTI_SCALER
                               eWindow == PQ_SC2_SUB_WINDOW ? PQ_GRULE_NR_NUM_SC2_Sub :
                        #endif
                                                              0xFFFF;
           PQGRULE_DBG(printf("Default\r\n"));
            _MDrv_PQ_LoadNRTable(eWindow, u16NR_Idx);
        }
        else
        {
            MS_ASSERT(0);
        }
    }
#endif
    else
    {
        PQGRULE_DBG(printf("Not support case \r\n"));
    }

#if 0 //trunk don't have janus code, it's obsolete
#ifdef __AEONR2__ //patch for DDR2-800 on Janus
    if(QM_IsInterlaced(eWindow) && QM_IsSourceMultiMedia(MDrv_PQ_GetInputSourceType(eWindow)) && (MApi_XC_ReadByte(0x110D30)==0x66))
    {
        MS_U16 u16Input_HSize;
        MS_U16 u16Input_VSize;
        u16Input_HSize = _stMode_Info[eWindow].u16input_hsize;
        u16Input_VSize = _stMode_Info[eWindow].u16input_vsize;

        if(u16Input_HSize>1280 && u16Input_VSize>=900)
        {
            MApi_XC_WriteByte(0x102F00, 0x12);
            MApi_XC_WriteByte(0x102F03, 0x27);
            MApi_XC_WriteByte(0x102F00, 0x06);
            MApi_XC_WriteByte(0x102F42, MApi_XC_ReadByte(0x102F42)&0xFE);
        }
    }
#endif
#endif
#endif // PQ_GRULE_NR_ENABLE
}

#if PQ_GRULE_OSD_BW_ENABLE
static void _MDrv_PQ_LoadOSD_BWTable(PQ_WIN eWindow, MS_U16 u16PQ_OSD_BW_Idx)
{
    MS_U16 i, u16IPIdx = 0, u16TabIdx = 0;

    if(u16PQ_OSD_BW_Idx > PQ_GRULE_OSD_BW_NUM_Main)
    {
        MS_ASSERT(0);
        return;
    }

    for(i = 0; i < PQ_GRULE_OSD_BW_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_OSD_BW_Main, i);

        if(u16PQ_OSD_BW_Idx == PQ_GRULE_OSD_BW_NUM_Main)  // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_Main, u16PQ_OSD_BW_Idx, i);

        PQGRULE_DBG(printf("[NR]SRC: %u, NR: %u, NRIPIdx:%u, IPIdx:%u, u8TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_OSD_BW_Idx, i, u16IPIdx, u16IPIdx));

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}


void MDrv_PQ_LoadOSD_BWTable(PQ_WIN eWindow, PQ_OSD_BW_FUNCTION_TYPE enOSD_BW_Type)
{
    MS_U16 u16PQ_NRIdx;

    PQGRULE_DBG(printf("[PQ_Load_OSD_BW_Table] "));

    if(enOSD_BW_Type == PQ_OSD_BW_ON)
    {
        PQGRULE_DBG(printf("On\r\n"));

        #if PQ_GRULE_DDR_SELECT_ENABLE
        if(bw_info.u32Miu1MemSize == 0)
       {
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_1DDR_On_Main);
            _MDrv_PQ_LoadOSD_BWTable(eWindow, u16PQ_NRIdx);
        }
        else
        {
            u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_2DDR_On_Main);
            _MDrv_PQ_LoadOSD_BWTable(eWindow, u16PQ_NRIdx);
        }
        #else
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_On_Main);
        _MDrv_PQ_LoadOSD_BWTable(eWindow, u16PQ_NRIdx);
       #endif
    }
    else
    {
        MS_ASSERT(0);
    }
}
#endif

#if PQ_GRULE_FILM_MODE_ENABLE
static void _MDrv_PQ_LoadFilmModeTable(PQ_WIN eWindow, MS_U16 u16PQ_FilmModeIdx)
{
    MS_U16 i, u16IPIdx = 0, u16TabIdx = 0;
    if(u16PQ_FilmModeIdx > PQ_GRULE_FILM_MODE_NUM_Main)
    {
        MS_ASSERT(0);
        return;
    }

    for(i = 0; i < PQ_GRULE_FILM_MODE_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_FILM_MODE_Main, i);

        if(u16PQ_FilmModeIdx == PQ_GRULE_FILM_MODE_NUM_Main)  // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16TabIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_FILM_MODE_Main, u16PQ_FilmModeIdx, i);

        PQGRULE_DBG(printf("[FM]SRC: %u, FM: %u, FMIPIdx:%u, IPIdx:%u, u16TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_FilmModeIdx, i, u16IPIdx, u16TabIdx));
        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif


void MDrv_PQ_LoadFilmModeTable(PQ_WIN eWindow, PQ_FILM_MODE_FUNCTION_TYPE enFilmModeType)
{
#if PQ_GRULE_FILM_MODE_ENABLE
    MS_U16 u16PQ_FilmModeIdx;

    if(enFilmModeType == PQ_FilmMode_OFF)
    {
        u16PQ_FilmModeIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_FILM_MODE_Main, PQ_GRule_FILM_MODE_Off_Main);
        _MDrv_PQ_LoadFilmModeTable(eWindow, u16PQ_FilmModeIdx);

    }
    else if(enFilmModeType == PQ_FilmMode_ON)
    {
        u16PQ_FilmModeIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_FILM_MODE_Main, PQ_GRule_FILM_MODE_On_Main);
        _MDrv_PQ_LoadFilmModeTable(eWindow, u16PQ_FilmModeIdx);
    }
    else
    {
        MS_ASSERT(0);
    }
#else
    UNUSED(eWindow);
    UNUSED(enFilmModeType);
#endif
}

#if PQ_GRULE_DYNAMIC_CONTRAST_ENABLE
static void _MDrv_PQ_DynamicContrastTable(PQ_WIN eWindow, MS_U16 u16PQ_DynContrIdx)
{
    MS_U16 i, u16IPIdx = 0, u16TabIdx = 0;
    if(u16PQ_DynContrIdx > PQ_GRULE_DYNAMIC_CONTRAST_NUM_Main)
    {
        MS_ASSERT(0);
        return;
    }

    for(i = 0; i < PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_DYNAMIC_CONTRAST_Main, i);

        if(u16PQ_DynContrIdx == PQ_GRULE_DYNAMIC_CONTRAST_NUM_Main)  // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16TabIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DYNAMIC_CONTRAST_Main, u16PQ_DynContrIdx, i);

        PQGRULE_DBG(printf("[DC]SRC: %u, DC: %u, DCIPIdx:%u, IPIdx:%u, u16TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_DynContrIdx, i, u16IPIdx, u16TabIdx));
        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif

void MDrv_PQ_LoadDynamicContrastTable(PQ_WIN eWindow, PQ_DYNAMIC_CONTRAST_FUNCTION_TYPE enDynamicContrastType)
{
#if PQ_GRULE_DYNAMIC_CONTRAST_ENABLE
    MS_U16 u16PQ_DynContrIdx;

    if(enDynamicContrastType == PQ_DynContr_OFF)
    {
        u16PQ_DynContrIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DYNAMIC_CONTRAST_Main, PQ_GRule_DYNAMIC_CONTRAST_Off_Main);
        _MDrv_PQ_DynamicContrastTable(eWindow, u16PQ_DynContrIdx);
    }
    else if(enDynamicContrastType == PQ_DynContr_ON)
    {
        u16PQ_DynContrIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DYNAMIC_CONTRAST_Main, PQ_GRule_DYNAMIC_CONTRAST_On_Main);
        _MDrv_PQ_DynamicContrastTable(eWindow, u16PQ_DynContrIdx);
    }
    else
    {
        MS_ASSERT(0);
    }
#else
    UNUSED(eWindow);
    UNUSED(enDynamicContrastType);
#endif
}


#if PQ_GRULE_MPEG_NR_ENABLE
static void _MDrv_PQ_LoadMPEGNRTable(PQ_WIN eWindow, MS_U16 u16PQ_NRIdx)
{
    MS_U16 i, u16IPIdx = 0, u16TabIdx = 0;
    if(u16PQ_NRIdx > PQ_GRULE_MPEG_NR_NUM_Main)
    {
        MS_ASSERT(0);
        return;
    }

    for(i = 0; i < PQ_GRULE_MPEG_NR_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_MPEG_NR_Main, i);

        if(u16PQ_NRIdx == PQ_GRULE_MPEG_NR_NUM_Main)  // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_MPEG_NR_Main, u16PQ_NRIdx, i);

        PQGRULE_DBG(printf("[MPEGNR]SRC: %u, NR: %u, NRIPIdx:%u, IPIdx:%u, u16TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_NRIdx, i, u16IPIdx, u16TabIdx));
        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif

void MDrv_PQ_LoadMPEGNRTable(PQ_WIN eWindow, PQ_MPEG_NR_FUNCTION_TYPE enMPEGNRType)
{
#if(PQ_GRULE_MPEG_NR_ENABLE)
    MS_U16 u16PQ_NRIdx;

    PQGRULE_DBG(printf("[PQ_LoadMPEGNRTable] "));

    if(enMPEGNRType == PQ_MPEG_NR_OFF)
    {
        PQGRULE_DBG(printf("Off\r\n"));
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_MPEG_NR_Main, PQ_GRule_MPEG_NR_Off_Main);
        _MDrv_PQ_LoadMPEGNRTable(eWindow, u16PQ_NRIdx);
    }
    else if( enMPEGNRType == PQ_MPEG_NR_AUTO )
    {
        // Auto NR will be process in msAPI_DynamicNR_Handler(), hence we only need to pre-setup the PQ table.
        PQGRULE_DBG(printf("Auto\r\n"));
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_MPEG_NR_Main, PQ_GRule_MPEG_NR_Low_Main);
        _MDrv_PQ_LoadMPEGNRTable(eWindow, u16PQ_NRIdx);
    }
    else if(enMPEGNRType == PQ_MPEG_NR_LOW)
    {
        PQGRULE_DBG(printf("Low\r\n"));
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_MPEG_NR_Main, PQ_GRule_MPEG_NR_Low_Main);
        _MDrv_PQ_LoadMPEGNRTable(eWindow, u16PQ_NRIdx);
    }
    else if(enMPEGNRType == PQ_MPEG_NR_MID)
    {
        PQGRULE_DBG(printf("Mid\r\n"));
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_MPEG_NR_Main, PQ_GRule_MPEG_NR_Middle_Main);
        _MDrv_PQ_LoadMPEGNRTable(eWindow, u16PQ_NRIdx);
    }
    else if(enMPEGNRType == PQ_MPEG_NR_HIGH)
    {
        PQGRULE_DBG(printf("High\r\n"));
        u16PQ_NRIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_MPEG_NR_Main, PQ_GRule_MPEG_NR_High_Main);
        _MDrv_PQ_LoadMPEGNRTable(eWindow, u16PQ_NRIdx);
    }
    else if(enMPEGNRType == PQ_MPEG_NR_DEFAULT)
    {
        PQGRULE_DBG(printf("Default\r\n"));
        _MDrv_PQ_LoadMPEGNRTable(eWindow, PQ_GRULE_MPEG_NR_NUM_Main);
    }
    else
    {
        MS_ASSERT(0);
    }
#else
    UNUSED(eWindow);
    UNUSED(enMPEGNRType);
#endif
}
#if PQ_GRULE_ULTRAT_CLEAR_ENABLE
static void _MDrv_PQ_Load_ULTRACLEAR_Table(PQ_WIN eWindow, MS_U16 u16PQ_ULTRACLEAR_Idx)
{
    MS_U16 i = 0, u16IPIdx = 0, u16TabIdx = 0;

    if(u16PQ_ULTRACLEAR_Idx > PQ_GRULE_ULTRAT_CLEAR_NUM_Main)
    {
        MS_ASSERT(0);
        return;
    }

    for(i = 0; i < PQ_GRULE_ULTRAT_CLEAR_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_ULTRAT_CLEAR_Main, i);

        if(u16PQ_ULTRACLEAR_Idx == PQ_GRULE_ULTRAT_CLEAR_NUM_Main)  // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_ULTRAT_CLEAR_Main, u16PQ_ULTRACLEAR_Idx, i);

        PQGRULE_DBG(printf("[UC]SRC: %u, UC: %u, UCIPIdx:%u, IPIdx:%u, u8TabIdx:%u\r\n",
                           _u16PQSrcType[eWindow], u16PQ_ULTRACLEAR_Idx, i, u16IPIdx, u16IPIdx));

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif

void MDrv_PQ_Load_ULTRACLEAR_Table(PQ_WIN eWindow, PQ_FEATURE_SWITCH_TYPE enCtrlType)
{
    MS_U16 u16PQ_TableIdx = 0;
#if PQ_GRULE_ULTRAT_CLEAR_ENABLE
    PQGRULE_DBG(printf("[PQ_Load_ULTRACLEAR_Table] "));
    if(enCtrlType == PQ_FEATURE_SWITCH_ON)
    {
        PQGRULE_DBG(printf("On\r\n"));
        u16PQ_TableIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_ULTRAT_CLEAR_Main, PQ_GRule_ULTRAT_CLEAR_On_Main);
        _MDrv_PQ_Load_ULTRACLEAR_Table(eWindow, u16PQ_TableIdx);
    }
    else
    {
        PQGRULE_DBG(printf("Off\r\n"));
        u16PQ_TableIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_ULTRAT_CLEAR_Main, PQ_GRule_ULTRAT_CLEAR_Off_Main);
        _MDrv_PQ_Load_ULTRACLEAR_Table(eWindow, u16PQ_TableIdx);
    }
#else
    UNUSED(u16PQ_TableIdx);
    UNUSED(eWindow);
    UNUSED(enCtrlType);

#endif

}

MS_BOOL MDrv_PQ_GRULE_Get_Support_Status(PQ_SUPPORTED_TYPE enType)
{
    MS_BOOL bStatus = FALSE;
    switch(enType)
    {
        case E_PQ_SUPPORTED_FILM:
            if(PQ_GRULE_FILM_MODE_ENABLE)
                bStatus = TRUE;
            break;
        case E_PQ_SUPPORTED_NR:
            if(PQ_GRULE_NR_ENABLE)
                bStatus = TRUE;
            break;
        case E_PQ_SUPPORTED_MPEG_NR:
            if(PQ_GRULE_MPEG_NR_ENABLE)
                bStatus = TRUE;
            break;
        case E_PQ_SUPPORTED_BLACK_LEVEL:
            bStatus = TRUE;
            break;
        case E_PQ_SUPPORTED_ULTRA_CLEAR:
            if(PQ_GRULE_ULTRAT_CLEAR_ENABLE)
                bStatus = TRUE;
            break;
        default:
            break;
    }

    return bStatus;

}

MS_U8 _MDrv_PQ_wait_output_vsync(PQ_WIN ePQWin, MS_U8 u8NumVSyncs, MS_U16 u16Timeout)
{
    MS_U32 u32Time;
    MS_U8 bVSync;
    //MS_BOOL bMainWin = (ePQWin == PQ_MAIN_WINDOW) ? TRUE : FALSE;

    bVSync = 0;
    u32Time = MsOS_GetSystemTime();

    while(1)
    {
        if(Hal_PQ_get_output_vsync_value(ePQWin) == bVSync)
        {
            u8NumVSyncs--;
            if(bVSync && (u8NumVSyncs == 0))
                break;
            bVSync = !bVSync;
        }

        if((MsOS_GetSystemTime() - u32Time)  >= u16Timeout)
            break;
    }

    return u8NumVSyncs;
}

MS_U8 _MDrv_PQ_wait_input_vsync(PQ_WIN ePQWin,  MS_U8 u8NumVSyncs, MS_U16 u16Timeout)
{
    MS_U32 u32Time;
    //MS_BOOL bMainWin = (ePQWin == PQ_MAIN_WINDOW) ? TRUE : FALSE;
    MS_U8 u8VsyncPolarity = Hal_PQ_get_input_vsync_polarity(ePQWin);
    MS_U8 bVSync = !u8VsyncPolarity;

    u32Time = MsOS_GetSystemTime();

    while(1)
    {
        if(Hal_PQ_get_input_vsync_value(ePQWin) == bVSync)
        {
            u8NumVSyncs--;
            if((bVSync == u8VsyncPolarity) && (u8NumVSyncs == 0))
                break;
            bVSync = !bVSync;
        }

        if((MsOS_GetSystemTime() - u32Time) >= u16Timeout)
        {
            //printf("!!input vsync timeout\r\n");
            break;
        }
    }

    return u8NumVSyncs;
}

#if (PQ_XRULE_DB_ENABLE == 1)
void MDrv_PQ_Set_DBRule(PQ_WIN eWindow, MS_U16 u16MADi_Idx)
{
    MS_U16 eXRuleType = E_XRULE_CSC;
    MS_U16 u16XRuleIdx, u16XRuleIP;
    MS_U16 u16IPIdx, u16TabIdx;

    eXRuleType = (_stMode_Info[eWindow].u16input_vfreq > 550) ? E_XRULE_DB_NTSC : E_XRULE_DB_PAL;

    if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_4R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_4R_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_2R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_2R_Main;
    }
    else if(yHal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R_MC) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_4R_MC_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_4R_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_2R_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_27_4R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_27_4R_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_27_2R) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_27_2R_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_P_MODE8_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE10) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_P_MODE10_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT8) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_P_MODE_MOT8_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT10) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_P_MODE_MOT10_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_4R_880) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_4R_880_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_2R_880) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_2R_880_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_MC) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_6R_MC_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_14F_6R_MC) == u16MADi_Idx)   //Add New MADi mode
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_14F_6R_MC_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R_MC_NW) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_4R_MC_NW_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_MC_NW) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_6R_MC_NW_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R_884) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_4R_884_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R_884) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_25_2R_884_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R_880) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_4R_880_Main;
    }
    else if(Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R_880) == u16MADi_Idx)
    {
        u16XRuleIdx = PQ_DBRule_NTSC_24_2R_880_Main;
    }
    else
    {
        u16XRuleIdx = 0xFF;
    }

    for(u16XRuleIP = 0; u16XRuleIP < MDrv_PQ_GetXRuleIPNum(eWindow, eXRuleType); u16XRuleIP++)
    {
        if(u16XRuleIdx == 0xFF)
            continue;

        u16IPIdx = MDrv_PQ_GetXRuleIPIndex(eWindow, eXRuleType, u16XRuleIP);
        u16TabIdx = MDrv_PQ_GetXRuleTableIndex(eWindow, eXRuleType, u16XRuleIdx, u16XRuleIP);
        //(printf("u16XRuleIdx:%u, u16XRuleIP=%u, IPIdx=%u, TabIdx=%u\r\n", u16XRuleIdx, u16XRuleIP, u16IPIdx, u16TabIdx));

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif

void MDrv_PQ_EnableMADIForce(PQ_WIN eWindow, MS_BOOL bFullMotion)
{
#if PQ_ENABLE_FORCE_MADI
    MS_U16 u16TabIdx, u16IPIdx;
    if(bFullMotion)
    {

        u16TabIdx = eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Force_YC_FullMotion_Main :
                    eWindow == PQ_SUB_WINDOW      ? PQ_IP_MADi_Force_YC_FullMotion_Sub :
            #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_Force_YC_FullMotion_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_Force_YC_FullMotion_SC2_Main :
                    eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_MADi_Force_YC_FullMotion_SC2_Sub :
            #endif
                                                    0xFFFF;

        u16IPIdx  = eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Force_Main :
                    eWindow == PQ_SUB_WINDOW      ? PQ_IP_MADi_Force_Sub :
            #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_Force_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_Force_SC2_Main :
                    eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_MADi_Force_SC2_Sub :
            #endif
                                                    0xFFFF;

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
    else
    {
        u16TabIdx = eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Force_YC_FullStill_Main :
                    eWindow == PQ_SUB_WINDOW      ? PQ_IP_MADi_Force_YC_FullStill_Sub :
            #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_Force_YC_FullStill_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_Force_YC_FullStill_SC2_Main :
                    eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_MADi_Force_YC_FullStill_SC2_Sub :
            #endif
                                                    0xFFFF;

        u16IPIdx  = eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Force_Main :
                    eWindow == PQ_SUB_WINDOW      ? PQ_IP_MADi_Force_Sub :
            #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_Force_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_Force_SC2_Main :
                    eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_MADi_Force_SC2_Sub :
            #endif
                                                    0xFFFF;

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);

    }
#else
UNUSED(eWindow);
UNUSED(bFullMotion);
#endif
}

//////////////////////////////////////////////////////////////////
// Load MADi/444To422/422To444 table
//
// parameter:
//    [IN]  u8PQTabType
//          bMemFmt422: TRUE  - 422
//                      FALSE - 444
//          bFBL:       TRUE  - framebuffer-less mode
//                      FALSE - framebuffer mode
//    [OUT] u8BitsPerPixel
//                      bits per pixel for decided memory format
//
// return:  deinterlace (MADi) mode
//
PQ_DEINTERLACE_MODE MDrv_PQ_SetMemFormat(PQ_WIN eWindow, MS_BOOL bMemFmt422, MS_BOOL bFBL, MS_U8 *pu8BitsPerPixel)
{

#if(PQ_ENABLE_MEMFMT)

    MS_U16 u16TabIdx_MemFormat = 0;
    MS_U16 u16TabIdx_MADi = 0;
    MS_U16 u16TabIdx_MADi_Motion=0xFFFF;
    MS_U16 u16TabIdx_444To422 = 0;
    MS_U16 u16TabIdx_422To444 = 0;
    PQ_DEINTERLACE_MODE eDeInterlaceMode = PQ_DEINT_OFF;
    MS_U8 u8FrameCount = 4;

    if(eWindow == PQ_MAIN_WINDOW
#if ENABLE_MULTI_SCALER
       || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
#endif
      )
    {
#if 0  //Ryan
        SCALER_WIN eSCWin =
                #if ENABLE_MULTI_SCALER
                            eWindow == PQ_SC2_MAIN_WINDOW    ? SC2_MAIN_WINDOW :
                            eWindow == PQ_SC1_MAIN_WINDOW ?    SC1_MAIN_WINDOW :
                #endif
                                                            MAIN_WINDOW;
        MApi_XC_Set_OPWriteOffEnable(ENABLE, eSCWin); //default
#endif
    }
    else
    {
        PQINFO_DBG(printf("No need to enable OP write with sub window, it would change main window setting \r\n"));
    }

    PQTAB_DBG(printf("==>In PQ : eWindow =%d\r\n", eWindow));

    if(!bFBL)
    {
        // if MADi mode change from 25 <=> 27, cropping base address need to be recalculated.
#if PQ_GRULE_OSD_BW_ENABLE
        if(((_stMode_Info[eWindow].u16input_hsize >= 1280) && (_stMode_Info[eWindow].u16input_vsize >= 720)) &&
                (_bOSD_On))
        {
            MDrv_PQ_GetMADiFromGrule(eWindow, &u16TabIdx_MADi_Motion, &u16TabIdx_MADi);
        }
        else
#endif
        {
            MDrv_PQ_GetMADiInGeneral(eWindow, &u16TabIdx_MADi_Motion, &u16TabIdx_MADi);
#if PQ_GRULE_OSD_BW_ENABLE
            MDrv_PQ_Patch2Rto4RForFieldPackingMode(eWindow, u16TabIdx_MADi);
#endif
        }

        MDrv_PQ_GetMADiForRFBL(eWindow, bFBL, &u16TabIdx_MADi_Motion, &u16TabIdx_MADi);
    }
    else // FBL
    {
        MS_U8 u8FblMode = Hal_PQ_get_madi_fbl_mode(bMemFmt422, _stMode_Info[eWindow].bInterlace);
        u16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, (MS_MADI_TYPE)u8FblMode);
    }


    PQMADi_DBG(printf("Memory is %s mode \r\n", (bFBL)?("1.FBL"):("2.FB")));
    PQMADi_DBG(printf("get MADi idx = %u\r\n", u16TabIdx_MADi));

    MDrv_PQ_GetMemFmtInGeneral(eWindow, bMemFmt422, &u16TabIdx_MemFormat, &u16TabIdx_444To422, &u16TabIdx_422To444);

    PQTAB_DBG(printf("%s :u16TabIdx_444To422=%u\r\n", eWindow?"SubWin":"MainWin",u16TabIdx_444To422));
    PQTAB_DBG(printf("%s :u16TabIdx_422To444=%u\r\n", eWindow?"SubWin":"MainWin",u16TabIdx_422To444));
    PQMADi_DBG(printf("MemFmt is 422 : %s\r\n",(bMemFmt422)?("Yes"):("No")));

    MDrv_PQ_GetBPPInfoFromMADi(eWindow, bMemFmt422, u16TabIdx_MADi, u16TabIdx_MemFormat, pu8BitsPerPixel, &eDeInterlaceMode, &u8FrameCount);

    PQMADi_DBG(printf("BitPerPixel =%d, DIMode =%x\r\n", *pu8BitsPerPixel, eDeInterlaceMode));

    _gbMemfmt422[eWindow] = bMemFmt422;

    MDrv_PQ_ChangeMemConfigFor3D(eWindow, bMemFmt422, &u16TabIdx_MemFormat, &u16TabIdx_444To422, &u16TabIdx_422To444,
                                    pu8BitsPerPixel, &u16TabIdx_MADi_Motion, &u16TabIdx_MADi);

    if(eWindow == PQ_MAIN_WINDOW
    #if ENABLE_MULTI_SCALER
       || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
    #endif
      )
    {
        MS_U16 u16IPIdx;
        u16IPIdx =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MemFormat_Main :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MemFormat_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MemFormat_SC2_Main :
                #endif
                                                    0xFFFF;

        MDrv_PQ_LoadTable(eWindow, u16TabIdx_MemFormat, u16IPIdx);

        u16IPIdx =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Main :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_SC2_Main :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_MADi, u16IPIdx);
#if PQ_GRULE_OSD_BW_ENABLE
        if(0xFFFF != u16TabIdx_MADi_Motion)
        {
            PQTAB_DBG(printf("u16TabIdx_MADi_Motion=%u\r\n", u16TabIdx_MADi_Motion));
            u16IPIdx =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_MADi_Motion_Main :
                    #if ENABLE_MULTI_SCALER
                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_MADi_Motion_SC1_Main :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_MADi_Motion_SC2_Main :
                    #endif
                                                        0xFFFF;
            MDrv_PQ_LoadTable(eWindow, u16TabIdx_MADi_Motion, u16IPIdx);
        }
#endif
        u16IPIdx =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_444To422_Main :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_444To422_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_444To422_SC2_Main :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_444To422, u16IPIdx);

        u16IPIdx =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_422To444_Main :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_422To444_SC1_Main :
                    eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_422To444_SC2_Main :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_422To444, u16IPIdx);
    }
#if PQ_ENABLE_PIP
    else
    {
        MS_U16 u16IPIdx;
        u16IPIdx =  eWindow == PQ_SUB_WINDOW     ? PQ_IP_MemFormat_Sub :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_IP_MemFormat_SC2_Sub :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_MemFormat, u16IPIdx);

        u16IPIdx =  eWindow == PQ_SUB_WINDOW     ? PQ_IP_MADi_Sub :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_IP_MADi_SC2_Sub :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_MADi, u16IPIdx);

        u16IPIdx =  eWindow == PQ_SUB_WINDOW     ? PQ_IP_444To422_Sub :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_IP_444To422_SC2_Sub :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_444To422, u16IPIdx);

        u16IPIdx =  eWindow == PQ_SUB_WINDOW     ? PQ_IP_422To444_Sub :
                #if ENABLE_MULTI_SCALER
                    eWindow == PQ_SC2_SUB_WINDOW ?  PQ_IP_422To444_SC2_Sub :
                #endif
                                                    0xFFFF;
        MDrv_PQ_LoadTable(eWindow, u16TabIdx_422To444, u16IPIdx);
    }
#endif

    PQTAB_DBG(printf("u16TabIdx_MemFormat=%u\r\n", u16TabIdx_MemFormat));
    PQTAB_DBG(printf("%s, u16TabIdx_MADi=%u\r\n", eWindow?"SUB_WIN":"MAIN_WIN",u16TabIdx_MADi));
    PQTAB_DBG(printf("%s, u16TabIdx_MADi=%u\r\n", eWindow?"SUB_WIN":"MAIN_WIN",u16TabIdx_MADi));
    PQTAB_DBG(printf("main :u16TabIdx_444To422=%u\r\n", u16TabIdx_444To422));
    PQTAB_DBG(printf("main :u16TabIdx_422To444=%u\r\n", u16TabIdx_422To444));

#if (PQ_XRULE_DB_ENABLE == 1)
    MDrv_PQ_Set_DBRule(eWindow, u16TabIdx_MADi);
#endif

    MDrv_PQ_ForceBPPForDynamicMemFmt(eWindow, bMemFmt422, pu8BitsPerPixel);
    MDrv_PQ_SetFrameNumber(eWindow, u16TabIdx_MADi, u8FrameCount);

    if(_bDS_En)
    {
        _u16RW_Method = Hal_PQ_get_rw_method(eWindow);
        Hal_PQ_set_rw_method(eWindow, 0x4000);
    }
    return eDeInterlaceMode;
#else
    return PQ_DEINT_OFF;
#endif
}

void MDrv_PQ_ReduceBW_ForOSD(PQ_WIN eWindow, MS_BOOL bOSD_On)
{
#if PQ_GRULE_OSD_BW_ENABLE
    MS_U8 u8BitPerPixel = 0;
{
    if((_stMode_Info[eWindow].u16input_hsize >= 1280) && (_stMode_Info[eWindow].u16input_vsize >= 720))
    {
        PQBW_DBG(printf("[PQ_ReduceBW_ForOSD]:%u\r\n", bOSD_On));

        _bOSD_On = bOSD_On;
        bSetFrameCount = FALSE;

        MDrv_PQ_SetMemFormat(
            eWindow,
            _gbMemfmt422[eWindow],
            _stMode_Info[eWindow].bFBL,
            &u8BitPerPixel);
        bSetFrameCount = TRUE;

    }
}
#else
    UNUSED(eWindow);
    UNUSED(bOSD_On);
#endif

    return;
}

void MDrv_PQ_ReduceBW_ForPVR(PQ_WIN eWindow, MS_BOOL bPVR_On)
{
    UNUSED(eWindow);
    UNUSED(bPVR_On);
}

void _MDrv_PQ_Set_MVOP_UVShift(MS_BOOL bEnable)
{
    MS_U8 regval;

    regval = MApi_XC_ReadByte(VOP_MPG_JPG_SWITCH);

    if(((regval & 0x10) == 0x10) && ((regval & 0x3) == 0x2))
    {
        // 422 with MCU control mode
        if(bEnable)
        {
            MS_ASSERT(0);
        }
    }

    // output 420 and interlace
    //[IP - Sheet] : Main Page --- 420CUP
    //[Project] :  Titania2
    //[Description]:   Chroma artifacts when 420to422 is applied duplicate method.
    //[Root cause]: Apply 420to422 average algorithm to all DTV input cases.
    //The average algorithm must cooperate with MVOP.
    MApi_XC_WriteByteMask(VOP_UV_SHIFT, (bEnable) ? 1 : 0, 0x3);
}

void MDrv_PQ_Set420upsampling(PQ_WIN eWindow,
                              MS_BOOL bFBL,
                              MS_BOOL bPreV_ScalingDown,
                              MS_U16 u16V_CropStart)
{
#if PQ_ENABLE_420UPSAMPLING

    MS_U16 u16TabIdx;

    u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_420CUP_Main);

    PQTAB_DBG(printf("[PQ_Set420upsampling]: SrcType:%u, FBL:%u, PreV down:%u, V_CropStart:%u, u8TabIdx=%u, ",
                     _u16PQSrcType[eWindow], bFBL, bPreV_ScalingDown, u16V_CropStart, u16TabIdx));

    if((u16TabIdx == Hal_PQ_get_420_cup_idx(MS_420_CUP_ON)) && (!bPreV_ScalingDown) && (!bFBL))
    {
        PQTAB_DBG(printf("UVShift: on\r\n"));
        _MDrv_PQ_Set_MVOP_UVShift(ENABLE);
        MDrv_PQ_LoadTable(eWindow, Hal_PQ_get_420_cup_idx(MS_420_CUP_ON), PQ_IP_420CUP_Main);
    }
    else    // P mode should not do UV shift
    {
        PQTAB_DBG(printf("UVShift: off\r\n"));
        _MDrv_PQ_Set_MVOP_UVShift(DISABLE);
        MDrv_PQ_LoadTable(eWindow, Hal_PQ_get_420_cup_idx(MS_420_CUP_OFF), PQ_IP_420CUP_Main);
    }

    if((u16V_CropStart & 0x3) == 0)       // crop lines are multiple of 4
        Hal_PQ_set_420upSample(eWindow, 0x6666);
    else if((u16V_CropStart & 0x1) == 0)  // crop lines are multiple of 2
        Hal_PQ_set_420upSample(eWindow, 0x9999);
    else
        MS_ASSERT(0);
#endif
}

void MDrv_PQ_SetFilmMode(PQ_WIN eWindow, MS_BOOL bEnable)
{
#if PQ_ENABLE_SETFILM_MODE
    MS_U16 u16TabIdx;

    PQTAB_DBG(printf("[PQ_SetFilmMode]: PQTabType=%u, enable=%u\r\n", eWindow, bEnable));

    if(bEnable)
        u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_Film32_Main);
    else
        u16TabIdx = PQ_IP_Film32_OFF_Main;

    MDrv_PQ_LoadTable(eWindow, u16TabIdx, PQ_IP_Film32_Main);

    if(bEnable)
        u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_Film22_Main);
    else
        u16TabIdx = PQ_IP_Film22_OFF_Main;

    MDrv_PQ_LoadTable(eWindow, u16TabIdx, PQ_IP_Film22_Main);
#endif
}


void MDrv_PQ_SetNonLinearScaling(PQ_WIN eWindow, MS_U8 u8Level, MS_BOOL bEnable)
{
#if PQ_ENABLE_NONLINEAR_SCALING
    MS_U16 u16TabIdx;
    PQTAB_DBG(printf("[PQ_SetNonLinearScaling]: Level=%u, enable=%u\r\n", u8Level, bEnable));

    if(bEnable)
    {
        switch(_stPQ_Info.u16PnlWidth)
        {
            case 1920:
                switch(u8Level)
                {
                    case 2:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1920_2);
                        break;
                    case 1:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1920_1);
                        break;
                    case 0:
                    default:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1920_0);
                        break;
                }
                break;
            case 1680:
                u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1680);
                break;
            case 1440:
                u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1440);
                break;
            case 1366:
                switch(u8Level)
                {
                    case 2:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1366_2);
                        break;
                    case 1:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1366_1);
                        break;
                    case 0:
                    default:
                        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_1366_0);
                        break;

                }
                break;
            default:
                u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_OFF);
                break;

                break;
        }
    }
    else
    {
        u16TabIdx = Hal_PQ_get_hnonlinear_idx(MS_HNONLINEAR_OFF);
    }

    MDrv_PQ_LoadTable(eWindow, u16TabIdx, PQ_IP_HnonLinear_Main);
#endif
}

void MDrv_PQ_3DCloneforPIP(MS_BOOL bIpSync)
{
#if 0 //Ryan
    //MVOP_Handle stHdl = { E_MVOP_MODULE_MAIN };
    MS_BOOL bEnAltOutputState = FALSE;

//![for temporary A3-WinCE6 porting, 20120109]
#if 0 //ndef MSOS_TYPE_CE

    if (E_MVOP_OK == MDrv_MVOP_GetCommand(&stHdl, E_MVOP_CMD_GET_3DLR_ALT_OUT, &bEnAltOutputState, sizeof(bEnAltOutputState)))
    {
        //printf("%s Get3DLRAltOutput=0x%x\r\n", __FUNCTION__, bEnAltOutputState);
    }
    else
    {
        //printf("Fail to query E_MVOP_CMD_GET_3DLR_ALT_OUT!!\r\n");
    }

#endif

    if((E_XC_3D_OUTPUT_LINE_ALTERNATIVE == MApi_XC_Get_3D_Output_Mode())
        || (E_XC_3D_OUTPUT_TOP_BOTTOM == MApi_XC_Get_3D_Output_Mode())
        || (E_XC_3D_OUTPUT_SIDE_BY_SIDE_HALF == MApi_XC_Get_3D_Output_Mode()))
    {
        #if 0
        MApi_XC_ACE_3DClonePQMap(E_ACE_WEAVETYPE_NONE);
        //printf("3D:confirm to clone main/sub win\r\n");
        #endif

        if((MApi_XC_Get_3D_HW_Version() < 2) && bIpSync)
        {
            MApi_XC_3DMainSub_IPSync();
            //printf("3D:confirm to do main/sub win sync\r\n");
        }
    }
    else if(MApi_XC_IsCurrentFrameBufferLessMode() && bEnAltOutputState)
    {
        #if 0
        MApi_XC_ACE_3DClonePQMap(E_ACE_WEAVETYPE_H);
        #endif
    }
    MApi_XC_3D_PostPQSetting(MAIN_WINDOW);
#endif
}

void MDrv_PQ_DisableFilmMode(PQ_WIN eWindow, MS_BOOL bOn)
{
    if(bOn)
    {
        MDrv_PQ_SetFilmMode(eWindow, DISABLE);
    }
    else
    {
        MDrv_PQ_SetFilmMode(eWindow, ENABLE);
    }
}

MS_BOOL MDrv_PQ_DisableUCFeature(PQ_WIN eWindow)
{
    MS_BOOL bRet = TRUE;

#if PQ_EN_UCNR_OFF
    if(PQ_MAIN_WINDOW == eWindow)
    {
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_UCNR_OFF_Main, PQ_IP_UCNR_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_UC_CTL_OFF_Main, PQ_IP_UC_CTL_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_UCDi_OFF_Main, PQ_IP_UCDi_Main);
    }
#if PQ_ENABLE_PIP
    else if(PQ_SUB_WINDOW == eWindow)
    {
    #if defined(AMBER3)
        MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_UCNR_OFF_Sub, PQ_IP_UCNR_Sub);
        MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_UC_CTL_OFF_Sub, PQ_IP_UC_CTL_Sub);
        MDrv_PQ_LoadTable(PQ_SUB_WINDOW, PQ_IP_UCDi_OFF_Sub, PQ_IP_UCDi_Sub);
    #endif
    }
#endif

#if ENABLE_MULTI_SCALER
    else if(PQ_SC1_MAIN_WINDOW == eWindow)
    {
        MDrv_PQ_LoadTable(PQ_SC1_MAIN_WINDOW, PQ_IP_UCNR_OFF_SC1_Main, PQ_IP_UCNR_SC1_Main);
        MDrv_PQ_LoadTable(PQ_SC1_MAIN_WINDOW, PQ_IP_UC_CTL_OFF_SC1_Main, PQ_IP_UC_CTL_SC1_Main);
        MDrv_PQ_LoadTable(PQ_SC1_MAIN_WINDOW, PQ_IP_UCDi_OFF_SC1_Main, PQ_IP_UCDi_SC1_Main);
    }
    else if(PQ_SC2_MAIN_WINDOW == eWindow)
    {
        MDrv_PQ_LoadTable(PQ_SC2_MAIN_WINDOW, PQ_IP_UCNR_OFF_SC2_Main, PQ_IP_UCNR_SC2_Main);
        MDrv_PQ_LoadTable(PQ_SC2_MAIN_WINDOW, PQ_IP_UC_CTL_OFF_SC2_Main, PQ_IP_UC_CTL_SC2_Main);
        MDrv_PQ_LoadTable(PQ_SC2_MAIN_WINDOW, PQ_IP_UCDi_OFF_SC2_Main, PQ_IP_UCDi_SC2_Main);
    }
#if PQ_ENABLE_PIP
    else if(PQ_SUB_WINDOW == eWindow)
    {
    #if defined(AMBER3)
        MDrv_PQ_LoadTable(PQ_SC2_SUB_WINDOW, PQ_IP_UCNR_OFF_SC2_Sub, PQ_IP_UCNR_SC2_Sub);
        MDrv_PQ_LoadTable(PQ_SC2_SUB_WINDOW, PQ_IP_UC_CTL_OFF_SC2_Sub, PQ_IP_UC_CTL_SC2_Sub);
        MDrv_PQ_LoadTable(PQ_SC2_SUB_WINDOW, PQ_IP_UCDi_OFF_SC2_Sub, PQ_IP_UCDi_SC2_Sub);
    #endif
    }
#endif
#endif

    bRet = TRUE;
#else
    UNUSED(eWindow);
    bRet = FALSE;
#endif
    return bRet;
}

E_DRVPQ_ReturnValue MDrv_PQ_GetLibVer(const MSIF_Version **ppVersion)
{
    if(ppVersion == NULL)
    {
        return E_DRVPQ_RET_FAIL;
    }

    *ppVersion = &_drv_pq_version;
    return E_DRVPQ_RET_OK;
}


PQ_ReturnValue MDrv_PQ_GetInfo(MS_PQ_INFO *pInfo)
{
    MsOS_Memcpy((void*)pInfo, (void*)&_info, sizeof(MS_PQ_INFO));
    return E_PQ_RET_OK;
}

MS_BOOL  MDrv_PQ_GetStatus(MS_PQ_Status *pStatus)
{
    MsOS_Memcpy((void*)pStatus, (void*)&_status, sizeof(MS_PQ_Status));
    return FALSE;
}

MS_BOOL  MDrv_PQ_SetDbgLevel(MS_U16 u16DbgSwitch)
{
    _u16PQDbgSwitch = u16DbgSwitch;
    return TRUE;
}

void MDrv_PQ_Set_DisplayType(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, PQ_WIN ePQWin)
{
    printf("!!!!!SetDisplayType %d %d \r\n", u16DisplayWidth, enDisplaType);
#if (PQ_ENABLE_PIP)
#if(ENABLE_PQ_BIN)
    if(gbPQBinEnable)
    {
        if(ePQWin == PQ_SC1_MAIN_WINDOW)
        {
            MDrv_PQBin_SetPanelID(0, PQ_SC1_MAIN_WINDOW);
            MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_ONE, ePQWin);
        }
        else
        {
            PQ_BIN_DISPLAY_TYPE enBinDispType;

            MDrv_PQBin_SetPanelID(u16DisplayWidth == 1366 ? 1 : 1, ePQWin);

            enBinDispType = enDisplaType == PQ_DISPLAY_ONE ? PQ_BIN_DISPLAY_ONE :
                            enDisplaType == PQ_DISPLAY_PIP ? PQ_BIN_DISPLAY_PIP :
                                                             PQ_BIN_DISPLAY_POP;
            MDrv_PQBin_SetDisplayType(enBinDispType, ePQWin);
        }

    }
    else
#endif
    {
#if (PQ_ONLY_SUPPORT_BIN == 0)
        MS_BOOL bSkipCommTable = TRUE;

        if(genDisplayType != enDisplaType)
        {
            MDrv_PQ_Set_DisplayType_Sub(u16DisplayWidth, enDisplaType, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_Main(u16DisplayWidth, enDisplaType, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_SC2_Sub(u16DisplayWidth, enDisplaType, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_SC2_Main(u16DisplayWidth, enDisplaType, bSkipCommTable);
            MDrv_PQ_Set_DisplayType_SC1_Main(u16DisplayWidth, enDisplaType, bSkipCommTable);

            genDisplayType = enDisplaType;
        }

#endif // #if PQ_ONLY_SUPPORT_BIN

    }

#else
    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
#endif
}

PQ_DISPLAY_TYPE MDrv_PQ_Get_DisplayType(void)
{
#if (PQ_ENABLE_PIP)

    return genDisplayType;
#else
    return PQ_DISPLAY_ONE;
#endif
}

void MDrv_PQ_MADiForceMotionC(PQ_WIN eWindow, MS_BOOL bEnable)
{
    MS_U16 u16DataC = 0;
    u16DataC = Hal_PQ_get_force_c_motion(eWindow);

#if (PQ_ENABLE_PIP)
    if(eWindow == PQ_SUB_WINDOW || eWindow == PQ_SC2_SUB_WINDOW)
    {
        u16DataC  <<= 8;
    }
#endif

    if(bEnable)
    {
        if(eWindow == PQ_MAIN_WINDOW
    #if ENABLE_MULTI_SCALER
           || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
    #endif
         )
        {
            u16DataC |= 0x0080;
        }
        else
        {
            u16DataC |= 0x8000;
        }
    }
    else
    {
        if(eWindow == PQ_MAIN_WINDOW
    #if ENABLE_MULTI_SCALER
           || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
    #endif
          )
        {
            u16DataC &= 0x000F;
        }
        else
        {
            u16DataC &= 0x0F00;
        }
    }
    Hal_PQ_set_force_c_motion(eWindow, u16DataC);

}

void MDrv_PQ_MADiForceMotionY(PQ_WIN eWindow, MS_BOOL bEnable)
{
    MS_U16 u16Data = 0;

    u16Data = Hal_PQ_get_force_y_motion(eWindow);

#if (PQ_ENABLE_PIP)
    if(eWindow == PQ_SUB_WINDOW
    #if ENABLE_MULTI_SCALER
       || eWindow == PQ_SC2_SUB_WINDOW
    #endif
      )
    {
        u16Data <<= 8;
    }
#endif

    if(bEnable)
    {
        if(eWindow == PQ_MAIN_WINDOW
    #if ENABLE_MULTI_SCALER
           || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
    #endif
          )
            u16Data |= 0x0080;
        else
            u16Data |= 0x8000;
    }
    else
    {
        if(eWindow == PQ_MAIN_WINDOW
    #if ENABLE_MULTI_SCALER
           || eWindow == PQ_SC1_MAIN_WINDOW || eWindow == PQ_SC2_MAIN_WINDOW
    #endif
          )
            u16Data &= 0x003F;
        else
            u16Data &= 0x3F00;
    }

    Hal_PQ_set_force_y_motion(eWindow, u16Data);
}

void MDrv_PQ_SetRFblMode(MS_BOOL bEnable, MS_BOOL bFilm)
{
    _stRFBL_Info.bEnable = bEnable;
    _stRFBL_Info.bFilm = bFilm;

    if(!bEnable)    //default mode
    {
        MDrv_PQ_MADiForceMotionY(PQ_MAIN_WINDOW, FALSE);
        MDrv_PQ_MADiForceMotionC(PQ_MAIN_WINDOW, FALSE);

        _stRFBL_Info.u8MADiType = MS_MADI_24_2R;
        //printf("[%s]-[%d] : Y/C motion : ( 0 , 0 ) \r\n", __FUNCTION__, __LINE__);
    }
    else
    {
        _stRFBL_Info.u8MADiType = (bFilm) ? (MS_U8)MS_MADI_24_RFBL_FILM : (MS_U8)MS_MADI_24_RFBL_NFILM;
        MDrv_PQ_MADiForceMotionY(PQ_MAIN_WINDOW, FALSE);
        MDrv_PQ_MADiForceMotionC(PQ_MAIN_WINDOW, (!bFilm));

        //printf("[%s]-[%d] : Y/C motion : ( 0 , %d) \r\n", __FUNCTION__, __LINE__,(!bFilm));
    }
}

void MDrv_PQ_GetRFblMode(MS_PQ_RFBL_INFO * pstInfo)
{
    MsOS_Memcpy(pstInfo, &_stRFBL_Info, sizeof(MS_PQ_RFBL_INFO));
    return;
}

#if (PQ_ENABLE_VD_SAMPLING)

MS_BOOL MDrv_PQ_Get_VDSampling_Info(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType, PQ_VIDEOSTANDARD_TYPE eStandard, MS_PQ_VD_Sampling_Info *pInfo)
{
    PQ_VIDEOSTANDARD_TYPE eCurStandard = _stVD_Info[eWindow].enVideoStandard;
    MS_U16 u16CurSrc = _u16PQSrcType[eWindow];
    MS_U16 u16TableIdx;
    MS_U16 u16IPIdx = Hal_PQ_get_ip_idx(MS_PQ_IP_VD_SAMPLING);
    MS_U8 u8SampleTable[10];

    MsOS_Memset(u8SampleTable, 0, sizeof(u8SampleTable));

    PQINFO_DBG(printf("[PQ] GetVDSampling: win:%d, src:%d, standard:%d, PQIP=%x\r\n", eWindow, enInputSourceType, eStandard, u16IPIdx));
    if((((enInputSourceType >= PQ_INPUT_SOURCE_TV) && (enInputSourceType <= PQ_INPUT_SOURCE_SVIDEO))||(enInputSourceType == PQ_INPUT_SOURCE_SCART)) &&
            (eStandard < E_PQ_VIDEOSTANDARD_NOTSTANDARD) &&
            (u16IPIdx != 0xFFFF))
    {
        _stVD_Info[eWindow].enVideoStandard = eStandard;
        _u16PQSrcType[eWindow] = QM_InputSourceToIndex(eWindow, enInputSourceType);



        u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
        PQINFO_DBG(printf("[PQ] VDSampling: TableIdx=%d\r\n", u16TableIdx));
#if(ENABLE_PQ_BIN)
        if(((gbPQBinEnable == 1) && (u16TableIdx == PQ_BIN_IP_NULL))
                || ((gbPQBinEnable == 0) && (u16TableIdx == PQ_IP_NULL)))
#else
        if(u16TableIdx == PQ_IP_NULL)
#endif
        {
            pInfo->eType = PQ_VD_SAMPLING_OFF;
        }
        else
        {
            MDrv_PQ_LoadTableData(eWindow, u16TableIdx, u16IPIdx, u8SampleTable, 10);
            pInfo->u16Vstart = ((MS_U16)u8SampleTable[0]) | (((MS_U16)u8SampleTable[1])<<8);
            pInfo->u16Hstart = ((MS_U16)u8SampleTable[2]) | (((MS_U16)u8SampleTable[3])<<8);
            pInfo->u16Hsize  = ((MS_U16)u8SampleTable[4]) | (((MS_U16)u8SampleTable[5]) << 8);
            pInfo->u16Vsize  = ((MS_U16)u8SampleTable[6]) | (((MS_U16)u8SampleTable[7]) << 8);
            pInfo->u16Htt    = ((MS_U16)u8SampleTable[8]) | (((MS_U16)u8SampleTable[9]) << 8);
            pInfo->eType = PQ_VD_SAMPLING_ON;

            PQINFO_DBG(printf("VD x:%x, y:%x, w:%x, h:%x, Htt=%x \r\n",
                    pInfo->u16Hstart, pInfo->u16Vstart, pInfo->u16Hsize, pInfo->u16Vsize, pInfo->u16Htt));
        }

        _stVD_Info[eWindow].enVideoStandard  = eCurStandard;
        _u16PQSrcType[eWindow] = u16CurSrc;
    }
    else
    {
        pInfo->eType = PQ_VD_SAMPLING_OFF;
    }
    return TRUE;
}

#endif


MS_BOOL _MDrv_PQ_Get_HSDSampling(PQ_WIN eWindow, MS_U32 *pu32ratio, MS_BOOL *bADVMode)
{
#if PQ_ENABLE_HSDRULE

    MS_U16 u16IPIdx = Hal_PQ_get_ip_idx(MS_PQ_IP_HSD_SAMPLING);
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;

    if(u16IPIdx != 0xFFFF)
    {
        u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
    }
    else
    {
#if(ENABLE_PQ_BIN)
        u16TableIdx = (gbPQBinEnable) ? PQ_BIN_IP_NULL : PQ_IP_NULL;
#else
        u16TableIdx = PQ_IP_NULL;
#endif
    }


#if(ENABLE_PQ_BIN)
    if(((gbPQBinEnable == 1) && (u16TableIdx == PQ_BIN_IP_NULL)) ||
            ((gbPQBinEnable == 0) && (u16TableIdx == PQ_IP_NULL)))
#else
    if(u16TableIdx == PQ_IP_NULL)
#endif
    {
        *pu32ratio = 0;
        bret = FALSE;
    }

#if(PQ_ONLY_SUPPORT_BIN)
    else if((( MDrv_PQBin_GetSkipRule(PQ_IP_HSD_Y_Main, &stPQBinHeaderInfo[PQ_BIN_STD_MAIN])) ||
            ( MDrv_PQBin_GetSkipRule(PQ_IP_HSD_C_Main, &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]))))
#elif(ENABLE_PQ_BIN)
    else if(((gbPQBinEnable == 1) &&
            (( MDrv_PQBin_GetSkipRule(PQ_IP_HSD_Y_Main, &stPQBinHeaderInfo[PQ_BIN_STD_MAIN])) ||
            ( MDrv_PQBin_GetSkipRule(PQ_IP_HSD_C_Main, &stPQBinHeaderInfo[PQ_BIN_STD_MAIN]))))
            ||
            ((MST_SkipRule_IP_Main[PQ_IP_HSD_Y_Main] == 1) || (MST_SkipRule_IP_Main[PQ_IP_HSD_C_Main] == 1))
           )
#else
    else if((MST_SkipRule_IP_Main[PQ_IP_HSD_Y_Main] == 1) || (MST_SkipRule_IP_Main[PQ_IP_HSD_C_Main] == 1))
#endif
    {
        SRULE_DBG(printf("HSD_Y, HSD_C is True in SKipRule, driver consider HSDSampling is not defined in QMap.\r\n"));
        *pu32ratio = 0;
        bret = FALSE;
    }
    else
    {
#if PQ_NEW_HSD_SAMPLING_TYPE
        if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_000))
        {
            *pu32ratio = 1000;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_125))
        {
            *pu32ratio = 875;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_250))
        {
            *pu32ratio = 750;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_375))
        {
            *pu32ratio = 625;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_500))
        {
            *pu32ratio = 500;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_625))
        {
            *pu32ratio = 375;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_750))
        {
            *pu32ratio = 250;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_1_875))
        {
            *pu32ratio = 125;
        }
        else if(u16TableIdx == Hal_PQ_get_hsd_sampling_idx(MS_HSD_SAMPLING_Div_2_000))
        {
            *pu32ratio = 1;
        }
        else
#endif
        {
            *pu32ratio = 0;
        }

        bret = TRUE;
    }

    // Load HSD_Y & HSD_C
    if(bret)
    {
        MS_U16 u16HSD_Y_idx, u16HSD_C_idx;

        u16HSD_Y_idx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_HSD_Y_Main);
        MDrv_PQ_LoadTable(eWindow, u16HSD_Y_idx, PQ_IP_HSD_Y_Main);

        u16HSD_C_idx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_HSD_C_Main);
        MDrv_PQ_LoadTable(eWindow, u16HSD_C_idx, PQ_IP_HSD_C_Main);

        SRULE_DBG(printf("HSD_Y:%d, HSD_C:%d\r\n", u16HSD_Y_idx, u16HSD_C_idx));

        *bADVMode = ((u16HSD_Y_idx == PQ_IP_HSD_Y_CB_Main) &&
                     (u16HSD_C_idx == PQ_IP_HSD_C_CB_Main)) ? FALSE : TRUE;
    }
    else
    {
        *bADVMode = FALSE;
    }


    SRULE_DBG(printf("[PQ] GetHSDSampling, IPIdx=%d, Table=%d, ratio=%ld ret=%d\r\n", u16IPIdx, u16TableIdx, *pu32ratio, bret));

    return bret;
#else
    return TRUE;
#endif

}


MS_BOOL MDrv_PQ_IOCTL(PQ_WIN eWindow, MS_U32 u32Flag, void *pBuf, MS_U32 u32BufSize)
{
    MS_BOOL bret = TRUE;

    switch(u32Flag)
    {
        case E_PQ_IOCTL_HSD_SAMPLING:
        {
            if(u32BufSize == sizeof(PQ_HSD_SAMPLING_INFO))
            {
                MS_U32 u32ratio;
                MS_BOOL bADVMode;

                bret = _MDrv_PQ_Get_HSDSampling(eWindow, &u32ratio, &bADVMode);
                ((PQ_HSD_SAMPLING_INFO *)pBuf)->u32ratio = u32ratio;
                ((PQ_HSD_SAMPLING_INFO *)pBuf)->bADVMode = bADVMode;
                bret = TRUE;

                //printf("ratio %lx, bADVmode=%d\r\n\r\n",((PQ_HSD_SAMPLING_INFO *)pBuf)->u32ratio, ((PQ_HSD_SAMPLING_INFO *)pBuf)->bADVMode);

            }
            else
            {
                //printf("Size is not correct, in=%ld, %d\r\n", u32BufSize, sizeof(PQ_HSD_SAMPLING_INFO));
                ((PQ_HSD_SAMPLING_INFO *)pBuf)->u32ratio = 0;
                ((PQ_HSD_SAMPLING_INFO *)pBuf)->bADVMode = 0;
                bret = FALSE;
            }
            break;
        }

        case E_PQ_IOCTL_PREVSD_BILINEAR:
            bret = TRUE;
            *((MS_BOOL *)pBuf) = TRUE;
            break;

        case E_PQ_IOCTL_ADC_SAMPLING:
            if(u32BufSize == sizeof(PQ_ADC_SAMPLING_INFO))
            {

                PQ_INPUT_SOURCE_TYPE enInputSrc = ((PQ_ADC_SAMPLING_INFO *)pBuf)->enPQSourceType;
                MS_PQ_Mode_Info      *pModeInfo = &(((PQ_ADC_SAMPLING_INFO *)pBuf)->stPQModeInfo);
                MS_PQ_ADC_SAMPLING_TYPE enADCSamplingType = MDrv_PQ_Get_ADCSampling_Info(eWindow, enInputSrc, pModeInfo);

                if((enADCSamplingType == E_PQ_ADC_SAMPLING_NOT_SUPPORT) || (enADCSamplingType == E_PQ_ADC_SAMPLING_NONE))
                {
                    ((PQ_ADC_SAMPLING_INFO *)pBuf)->u16ratio = 1;
                    bret = FALSE;
                }
                else
                {
                    ((PQ_ADC_SAMPLING_INFO *)pBuf)->u16ratio = (MS_U16)enADCSamplingType;
                    bret = TRUE;
                }
            }
            else
            {
                bret = FALSE;
                ((PQ_ADC_SAMPLING_INFO *)pBuf)->u16ratio = 1;
            }
            break;
        case E_PQ_IOCTL_RFBL_CTRL:
            //printf("IOCTL %lx\r\n", u32Flag);
#if PQ_ENABLE_RFBL
            bret = MDrv_PQ_Get_RFBL_Info(eWindow);
#else
            bret = FALSE;
#endif
            UNUSED(pBuf);
            UNUSED(u32BufSize);
            break;

        default:
            printf("unknown IOCTL %lx\r\n", u32Flag);
            UNUSED(pBuf);
            UNUSED(u32BufSize);
            bret = FALSE;
            break;
    }

    //printf("[PQ] IOCTL : win:%x, Flag=%08lx, size=%ld, ret:%x\r\n", eWindow, u32Flag, u32BufSize, bret);

    return bret;
}


MS_PQ_ADC_SAMPLING_TYPE MDrv_PQ_Get_ADCSampling_Info(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType, MS_PQ_Mode_Info *pstPQModeInfo)
{
    MS_PQ_ADC_SAMPLING_TYPE enRetType;
    MS_U16 u16PQSrcType;
    MS_U16 u16TableIdx;
    MS_PQ_Mode_Info stModeInfo;

    u16PQSrcType = _u16PQSrcType[eWindow];
    MsOS_Memcpy(&stModeInfo, &_stMode_Info[eWindow], sizeof(MS_PQ_Mode_Info));

    MDrv_PQ_Set_ModeInfo(eWindow, enInputSourceType, pstPQModeInfo);
    _u16PQSrcType[eWindow] = QM_InputSourceToIndex(eWindow, enInputSourceType);

    if(Hal_PQ_get_ip_idx(MS_PQ_IP_ADC_SAMPLING) == 0xFFFF)
    {
        return E_PQ_ADC_SAMPLING_NOT_SUPPORT;
    }

    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, Hal_PQ_get_ip_idx(MS_PQ_IP_ADC_SAMPLING));

    if(u16TableIdx == Hal_PQ_get_adc_sampling_idx(MS_ADC_SAMPLING_X_1))
    {
        enRetType = E_PQ_ADC_SAMPLING_X_1;
    }
    else if(u16TableIdx == Hal_PQ_get_adc_sampling_idx(MS_ADC_SAMPLING_X_2))
    {
        enRetType = E_PQ_ADC_SAMPLING_X_2;
    }
    else if(u16TableIdx == Hal_PQ_get_adc_sampling_idx(MS_ADC_SAMPLING_X_4))
    {
        enRetType = E_PQ_ADC_SAMPLING_X_4;
    }
    else
    {
        enRetType = E_PQ_ADC_SAMPLING_NONE;
    }

    MsOS_Memcpy(&_stMode_Info[eWindow], &stModeInfo, sizeof(MS_PQ_Mode_Info));
    _u16PQSrcType[eWindow] = u16PQSrcType;

    return enRetType;
}

MS_BOOL MDrv_PQ_Set3D_OnOff(MS_BOOL bEn)
{
    _gIs3D_En = bEn;
    return TRUE;
}

MS_BOOL MDrv_PQ_SetH264_OnOff(MS_BOOL bEn)
{
    _gIsH264 = bEn;
    return TRUE;
}

MS_BOOL MDrv_PQ_SetMM_OnOff(MS_BOOL bEn)
{
    _gIsMMplayer = bEn;
    return TRUE;
}

MS_BOOL MDrv_PQ_GetCaps(MS_PQ_CAP_INFO *pInfo)
{
    MS_BOOL bRet = TRUE;
#if(PQ_ENABLE_PIP)
    const XC_ApiInfo* pXC_ApiInfo = MApi_XC_GetInfo();
    if (pXC_ApiInfo != NULL)
    {
        pInfo->bPIP_Supported = ((pXC_ApiInfo->u8MaxWindowNum) >= 2) ? TRUE : FALSE;
    }
    else
    {
        pInfo->bPIP_Supported = FALSE;
        bRet = FALSE;
    }
#else
    pInfo->bPIP_Supported = FALSE;
#endif

#if(PQ_ENABLE_3D_VIDEO)
    pInfo->b3DVideo_Supported = TRUE;
#else
    pInfo->b3DVideo_Supported = FALSE;
#endif

    return bRet;
}

void MDRV_PQ_PrintLoadTableInfo(MS_PQ_PQLOADTABLE_WIN enPQWin)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)

    MS_U16 u16IPIdx;
    MS_U8  u8TabIdx;
    MS_U16 u16IPNum;

    if(enPQWin == PQ_LOAD_TABLE_MAIN)
        printf("[TableInfo::Main] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_MAIN_EX)
        printf("[TableInfo::Main Ex] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SUB)
        printf("[TableInfo::SUB] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SUB_EX)
        printf("[TableInfo::SUB Ex] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC1_MAIN)
        printf("[TableInfo::SC1_Main] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC1_MAIN_EX)
        printf("[TableInfo::SC1_Main Ex] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC2_MAIN)
        printf("[TableInfo::SC2_Main] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC2_MAIN_EX)
        printf("[TableInfo::SC2_Main Ex] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC2_SUB)
        printf("[TableInfo::SC2_SUB] \r\n");
    else if(enPQWin == PQ_LOAD_TABLE_SC2_SUB_EX)
        printf("[TableInfo::SC2_SUB Ex] \r\n");
    else
        return;

    u16IPNum = enPQWin == PQ_LOAD_TABLE_MAIN        ? PQ_IP_NUM_Main :
               enPQWin == PQ_LOAD_TABLE_MAIN_EX     ? PQ_IP_NUM_Main_Ex :
               enPQWin == PQ_LOAD_TABLE_SUB         ? PQ_IP_NUM_Sub :
               enPQWin == PQ_LOAD_TABLE_SUB_EX      ? PQ_IP_NUM_Sub_Ex :
               enPQWin == PQ_LOAD_TABLE_SC1_MAIN    ? PQ_IP_NUM_SC1_Main :
               enPQWin == PQ_LOAD_TABLE_SC1_MAIN_EX ? PQ_IP_NUM_SC1_Main_Ex :
               enPQWin == PQ_LOAD_TABLE_SC2_MAIN    ? PQ_IP_NUM_SC2_Main :
               enPQWin == PQ_LOAD_TABLE_SC2_MAIN_EX ? PQ_IP_NUM_SC2_Main_Ex :
               enPQWin == PQ_LOAD_TABLE_SC2_SUB     ? PQ_IP_NUM_SC2_Sub :
               enPQWin == PQ_LOAD_TABLE_SC2_SUB_EX  ? PQ_IP_NUM_SC2_Sub_Ex :
                                                      0;

    for(u16IPIdx = 0; u16IPIdx < u16IPNum; ++ u16IPIdx)
    {
        printf(" %u(%s)=", u16IPIdx, MDrv_PQ_GetIPName(u16IPIdx));  // IP Name

        if(enPQWin == PQ_LOAD_TABLE_MAIN)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(MAIN, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("%u(%s)\r\n", u8TabIdx, MDrv_PQ_GetTableName(u16IPIdx, u8TabIdx));
        }
        else if(enPQWin == PQ_LOAD_TABLE_MAIN_EX)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(MAINEX, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("Tab%u\r\n", u8TabIdx);
        }

#if PQ_ENABLE_PIP
        else if(enPQWin == PQ_LOAD_TABLE_SUB)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SUB, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("%u(%s)\r\n", u8TabIdx, MDrv_PQ_GetTableName(u16IPIdx, u8TabIdx));
        }
        else if(enPQWin == PQ_LOAD_TABLE_SUB_EX)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SUBEX, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("Tab%u\r\n", u8TabIdx);

        }
#endif

#if defined(CEDRIC)
        else if(enPQWin == PQ_LOAD_TABLE_SC1_MAIN)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC1_MAIN, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("%u(%s)\r\n", u8TabIdx, MDrv_PQ_GetTableName(u16IPIdx, u8TabIdx));
        }
        else if(enPQWin == PQ_LOAD_TABLE_SC1_MAIN_EX)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC1_MAINEX, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("Tab%u\r\n", u8TabIdx);
        }

        else if(enPQWin == PQ_LOAD_TABLE_SC2_MAIN)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC2_MAIN, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("%u(%s)\r\n", u8TabIdx, MDrv_PQ_GetTableName(u16IPIdx, u8TabIdx));
        }
        else if(enPQWin == PQ_LOAD_TABLE_SC2_MAIN_EX)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC2_MAINEX, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("Tab%u\r\n", u8TabIdx);
        }

#if PQ_ENABLE_PIP
        else if(enPQWin == PQ_LOAD_TABLE_SC2_SUB)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC2_SUB, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("%u(%s)\r\n", u8TabIdx, MDrv_PQ_GetTableName(u16IPIdx, u8TabIdx));
        }
        else if(enPQWin == PQ_LOAD_TABLE_SC2_SUB_EX)
        {
            u8TabIdx = MDrv_PQ_Get_LoadTableInfo_IP_Tab(SC2_SUBEX, u16IPIdx);
            if(u8TabIdx == 0xFF)
                printf("_____\r\n");
            else
                printf("Tab%u\r\n", u8TabIdx);

        }
#endif

#endif // CEDRIC
        else
        {
        }
    }
#else
    UNUSED(enPQWin);
#endif
}

MS_BOOL MDrv_PQ_SetDS_OnOFF(PQ_WIN eWindow, MS_BOOL bEn)
{
    //store the original value
    _u16RW_Method = Hal_PQ_get_rw_method(eWindow);
    if(bEn)
    {
        _bDS_En = TRUE;
    }
    else
    {
        _bDS_En = FALSE;
        Hal_PQ_set_rw_method(eWindow, _u16RW_Method);
    }

    return TRUE;
}

MS_BOOL MDrv_PQ_Get_RFBL_Info(PQ_WIN eWindow)
{
#if 0//Ryan

    MS_BOOL bret = TRUE;

    MS_U16 u16TabIdx_MADi = 0;
//  PQ_WIN eWindow = (bIsMainWindow == TRUE) ? (PQ_MAIN_WINDOW):(PQ_SUB_WINDOW);

    u16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_Main);
    (printf("[PQ] MADI: u16TabIdx_MADi=%d\r\n", u16TabIdx_MADi));

#if(ENABLE_PQ_BIN)
    if(((gbPQBinEnable == 1) && (u16TabIdx_MADi == PQ_BIN_IP_NULL)) ||
            ((gbPQBinEnable == 0) && (u16TabIdx_MADi == PQ_IP_NULL)))
#else
    if(u16TabIdx_MADi == PQ_IP_NULL)
#endif
    {
        MApi_XC_EnableRequest_FrameBufferLess(DISABLE);

        bret = FALSE;
        printf("[PQ] MADI: Get Null\r\n");
        printf("[PQ] MADI: DISABLE RFBL\r\n");

    }
    else
    {
        if((u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_2D))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_25D))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D_YC))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D_FILM))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_P_MODE10_MOT))
                || (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_P_MODE8_MOT))

          )
        {
            MApi_XC_EnableRequest_FrameBufferLess(ENABLE);
            printf("[PQ] MADI: ENABLE RFBL\r\n");

        }
        else
        {
            MApi_XC_EnableRequest_FrameBufferLess(DISABLE);
            printf("[PQ] MADI: DISABLE RFBL\r\n");

        }
        bret = TRUE;
    }

    return bret;
#else
    return TRUE;
#endif
}

MS_BOOL MDrv_PQ_CheckHDMode(PQ_WIN eWindow)
{
    return _gIsSrcHDMode[eWindow];
}

//for Super Resolution interlace and SD mode detected use
MS_BOOL MDrv_PQ_Get_Interlace_SD_mode(PQ_WIN ePQWin)
{
    if(QM_IsInterlaced(ePQWin) && (QM_GetInputHSize(ePQWin) < 1500))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#if PQ_GRULE_PTP_ENABLE
static void _MDrv_PQ_LoadPTPTable(PQ_WIN eWindow, MS_U16 u16PQ_PTP_Idx)
{
    MS_U16 i, u16IPIdx=0, u16TabIdx=0;

    if (u16PQ_PTP_Idx > PQ_GRULE_PTP_LVL_NUM_Main){
        MS_ASSERT(0); return;
    }

    for(i=0; i<PQ_GRULE_PTP_IP_NUM_Main; i++)
    {
        u16IPIdx = MDrv_PQ_GetGRule_IPIndex(eWindow, PQ_GRule_PTP_Main, i);

        if (u16PQ_PTP_Idx == PQ_GRULE_PTP_NUM_Main) // see it as default
            u16TabIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);
        else
            u16TabIdx = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_PTP_Main, u16PQ_PTP_Idx, i);

        PQP2P_DBG(printf("[PTP]SRC: %u, u16PQ_PTP_Idx: %u, PIPIPIdx:%u, IPIdx:%u, u8TabIdx:%u\r\n",
            _u16PQSrcType[eWindow], u16PQ_PTP_Idx, i, u16IPIdx, u16IPIdx);)

        MDrv_PQ_LoadTable(eWindow, u16TabIdx, u16IPIdx);
    }
}
#endif


void MDrv_PQ_LoadPTPTable(PQ_WIN eWindow, PQ_PTP_FUNCTION_TYPE enPTP_Type)
{
#if PQ_GRULE_PTP_ENABLE
    PQGRULE_DBG(printf("[PQ_LoadPTPTable] "));
    MS_U16 u16PQ_PTPIdx;

     if( enPTP_Type == PQ_PTP_PTP )
     {
         PQP2P_DBG(printf("PTP\r\n");)
         u16PQ_PTPIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_PTP_Main, PQ_GRule_Lvl_PTP_On_Main);
         _MDrv_PQ_LoadPTPTable(eWindow, u16PQ_PTPIdx);
     }
     else
     {
         MS_ASSERT(0);
     }
#else
    UNUSED(eWindow);
    UNUSED(enPTP_Type);
#endif
}

#if 0//Ryan
MS_BOOL MDrv_PQ_Set_PointToPoint(XC_SETWIN_INFO *pstXC_SetWin_Info,
                               MS_BOOL bEnable,
                               PQ_WIN eWindow)
{
    UNUSED(eWindow);
#ifdef STB_ENABLE
    UNUSED(bEnable);
    UNUSED(pstXC_SetWin_Info);

    _bPointToPointMode = FALSE;

    return TRUE;
#else

    if(bEnable)
    {
        //Center the display window
        pstXC_SetWin_Info->stDispWin.x = (_stPQ_Info.u16PnlWidth - pstXC_SetWin_Info->stCropWin.width)/2;
        pstXC_SetWin_Info->stDispWin.width = pstXC_SetWin_Info->stCropWin.width;
        pstXC_SetWin_Info->stDispWin.y = (_stPQ_Info.u16PnlHeight- pstXC_SetWin_Info->stCropWin.height)/2;
        pstXC_SetWin_Info->stDispWin.height = pstXC_SetWin_Info->stCropWin.height;

        //RGB HDMI, DVI, VGA
        if((((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == TRUE) && (QM_HDMIPC_COLORRGB(PQ_MAIN_WINDOW))) ||
             ((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == FALSE) && (QM_HDMIPC_COLORRGB(PQ_MAIN_WINDOW))) ||
             (IsSrcTypeVga(pstXC_SetWin_Info->enInputSourceType))) &&
             (!pstXC_SetWin_Info->bInterlace))
        {
            MApi_XC_EnableForceRGBin(TRUE,  MAIN_WINDOW);
        }
        MApi_XC_Set_MemFmt(E_MS_XC_MEM_FMT_444);
    }
    else
    {
        MApi_XC_EnableForceRGBin(FALSE,  MAIN_WINDOW);
        MApi_XC_Set_MemFmt(E_MS_XC_MEM_FMT_AUTO);
    }

    PQP2P_DBG(printf("[MDrv_PQ_Set_PointToPoint] True or False: %u\r\n",bEnable);)
    _bPointToPointMode = bEnable;

    return TRUE;
#endif
}
#endif

MS_BOOL MDrv_PQ_Get_PointToPoint(PQ_WIN eWindow)
{
    UNUSED(eWindow);
    return _bPointToPointMode;
}

#if 0//Ryan
MS_BOOL MDrv_PQ_Check_PointToPoint_Condition(XC_SETWIN_INFO *pstXC_SetWin_Info,PQ_WIN eWindow)
{
    MS_BOOL bRet = FALSE;

    UNUSED(eWindow);

#if PQ_GRULE_PTP_ENABLE
    // HDMI RGB, HDMI YUV444, DVI, YPbPr, VGA.
    // all in progressive mode
    PQP2P_DBG(printf("----HDMI source=%u, HDMIPacket=%u, RGB=%u, YUV444=%u, Ypbpr=%u, VGA=%u, interlace=%u\r\n",
            (IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)),
            (_stHDMI_Info[eWindow].bIsHDMI == TRUE),
            (QM_HDMIPC_COLORRGB(PQ_MAIN_WINDOW)),
            (QM_HDMIPC_COLORYUV444(PQ_MAIN_WINDOW)),
            (IsSrcTypeYPbPr(pstXC_SetWin_Info->enInputSourceType)),
            (IsSrcTypeVga(pstXC_SetWin_Info->enInputSourceType)),
            (pstXC_SetWin_Info->bInterlace)
            );)
    //at this time, the PQ input source has not been inited. so we use XC_SETWIN_INFO to check
    //we cannot do the PTP when timing size is bigger than panel size
    if((((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == TRUE) && (QM_HDMIPC_COLORRGB(PQ_MAIN_WINDOW))) ||
         ((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == TRUE) && (QM_HDMIPC_COLORYUV444(PQ_MAIN_WINDOW))) ||
         ((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == FALSE) && (QM_HDMIPC_COLORYUV444(PQ_MAIN_WINDOW))) ||
         ((IsSrcTypeHDMI(pstXC_SetWin_Info->enInputSourceType)) && (_stHDMI_Info[eWindow].bIsHDMI == FALSE) && (QM_HDMIPC_COLORRGB(PQ_MAIN_WINDOW))) ||
         (IsSrcTypeVga(pstXC_SetWin_Info->enInputSourceType)) ||
         (IsSrcTypeYPbPr(pstXC_SetWin_Info->enInputSourceType))) &&
         (!pstXC_SetWin_Info->bInterlace) &&
         ((_stPQ_Info.u16PnlWidth  >= pstXC_SetWin_Info->stCropWin.width) && (_stPQ_Info.u16PnlHeight  >= pstXC_SetWin_Info->stCropWin.height)))
    {
        bRet = TRUE;
    }
#else
    UNUSED(pstXC_SetWin_Info);
#endif

    PQP2P_DBG(printf("[MDrv_PQ_Check_PointToPoint_Condition] %u\r\n",bRet);)

    return bRet;
}
#endif

MS_BOOL MDrv_PQ_Update_MemFormat(void)
{
    MS_U8 u8BitPerPixel = 0;
    MDrv_PQ_SetMemFormat(
        PQ_MAIN_WINDOW,
        _gbMemfmt422[PQ_MAIN_WINDOW],
        _stMode_Info[PQ_MAIN_WINDOW].bFBL,
        &u8BitPerPixel);

    return TRUE;
}

MS_U16 MDrv_PQ_GetVsize(void)
{
    return _stMode_Info[PQ_MAIN_WINDOW].u16input_vsize;
}

MS_BOOL MDrv_PQ_LOW_3dQuality(MS_BOOL bEnable)
{
    if(bEnable)
    {
#if PQ_VIP_CTL
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_Peaking_band_AV_NTSC_Main, PQ_IP_VIP_Peaking_band_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_VNMR_S2_Main, PQ_IP_VIP_VNMR_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_VLPF_coef1_0x6_Main, PQ_IP_VIP_VLPF_coef1_Main);
#endif
    }
    else
    {
#if PQ_VIP_CTL
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_Peaking_band_Main), PQ_IP_VIP_Peaking_band_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_VNMR_Main), PQ_IP_VIP_VNMR_Main);
        MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_VLPF_coef1_Main), PQ_IP_VIP_VLPF_coef1_Main);
#endif
    }
    return TRUE;
}

MS_BOOL MDrv_PQ_MWE_SetEnhanceQuality(void)
{
#if PQ_VIP_RGBCMY_CTL
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_R_INC_3_Main, PQ_IP_VIP_ICC_R_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_G_INC_8_Main, PQ_IP_VIP_ICC_G_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_B_INC_6_Main, PQ_IP_VIP_ICC_B_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_C_INC_6_Main, PQ_IP_VIP_ICC_C_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_M_INC_6_Main, PQ_IP_VIP_ICC_M_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_Y_INC_7_Main, PQ_IP_VIP_ICC_Y_Main);
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_MWE_RestoreEnhanceQuality(void)
{
#if PQ_VIP_RGBCMY_CTL
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_R_Main), PQ_IP_VIP_ICC_R_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_G_Main), PQ_IP_VIP_ICC_G_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_B_Main), PQ_IP_VIP_ICC_B_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_C_Main), PQ_IP_VIP_ICC_C_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_M_Main), PQ_IP_VIP_ICC_M_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, MDrv_PQ_GetTableIndex(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_Y_Main), PQ_IP_VIP_ICC_Y_Main);
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_MWE_RestoreOffQuality(void)
{
#if PQ_VIP_CTL
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_ON_Main, PQ_IP_VIP_ICC_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_IBC_ON_Main, PQ_IP_VIP_IBC_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_IHC_ON_Main, PQ_IP_VIP_IHC_Main);
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_MWE_SetOffQuality(void)
{
#if PQ_VIP_CTL
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_ICC_OFF_Main, PQ_IP_VIP_ICC_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_IBC_OFF_Main, PQ_IP_VIP_IBC_Main);
    MDrv_PQ_LoadTable(PQ_MAIN_WINDOW, PQ_IP_VIP_IHC_OFF_Main, PQ_IP_VIP_IHC_Main);
#endif
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
// selection to VIP CSC
// Both equation selection rules are
// 0: SDTV(601) R  G  B  : 16-235
// 1: SDTV(601) R  G  B  : 0-255
/// @param bMainWin       \b IN: Enable
//-------------------------------------------------------------------------------------------------
void MDrv_PQ_SetSelectCSC(MS_U16 u16selection, PQ_WIN ePQWin)
{
    HAL_PQ_set_SelectCSC(u16selection, ePQWin);
}

MS_BOOL MDrv_PQ_Set_DisplayType_Sub(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, MS_BOOL bSkipCommTable)
{

#if PQ_ENABLE_PIP && (PQ_ONLY_SUPPORT_BIN == 0)
    PQTABLE_INFO PQTableInfo;
    PQTABLE_INFO PQTableInfoEx;

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTABLE_INFO));
    MsOS_Memset(&PQTableInfoEx, 0, sizeof(PQTABLE_INFO));
    PQTableInfo.eWin = PQ_SUB_WINDOW;
    // Load Sub PQ
    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_NR_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_NR_Sub;
                #endif
            }
            break;

        case PQ_DISPLAY_PIP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_PIP_NR_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_PIP_NR_Sub;
                #endif
            }
            break;

        case PQ_DISPLAY_POP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_POP_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_POP_NR_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_POP_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_POP_NR_Sub;
                #endif
            }
            break;
    }

    // table config parameter
    PQTableInfo.eWin = PQ_SUB_WINDOW;
    PQTableInfo.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_Sub;
    PQTableInfo.u8PQ_IP_Num = PQ_IP_NUM_Sub;
    PQTableInfo.pIPTAB_Info = (void*)PQ_IPTAB_INFO_Sub;
    PQTableInfo.pSkipRuleIP = (void*)MST_SkipRule_IP_Sub;

    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSD] = PQ_HSDRule_IP_NUM_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSD] = PQ_VSDRule_IP_NUM_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSP] = PQ_HSPRule_IP_NUM_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSP] = PQ_VSPRule_IP_NUM_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_CSC] = PQ_CSCRule_IP_NUM_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_NTSC] = PQ_DBRule_NTSC_IP_NUM_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_PAL] = PQ_DBRule_PAL_IP_NUM_Sub;
    #endif

    PQTableInfo.pXRule_IP_Index[E_XRULE_HSD] = (void*)MST_HSDRule_IP_Index_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSD] = (void*)MST_VSDRule_IP_Index_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSP] = (void*)MST_HSPRule_IP_Index_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSP] = (void*)MST_VSPRule_IP_Index_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_CSC] = (void*)MST_CSCRule_IP_Index_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_IP_Index_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_IP_Index_Sub;
    #endif

    PQTableInfo.pXRule_Array[E_XRULE_HSD] = (void*)MST_HSDRule_Array_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_VSD] = (void*)MST_VSDRule_Array_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_HSP] = (void*)MST_HSPRule_Array_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_VSP] = (void*)MST_VSPRule_Array_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_CSC] = (void*)MST_CSCRule_Array_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_Array[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_Array_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_Array_Sub;
    #endif

#if (PQ_GRULE_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_NR] = PQ_GRULE_NR_NUM_Sub;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_NR] = PQ_GRULE_NR_IP_NUM_Sub;
    PQTableInfo.pGRule_IP_Index[E_GRULE_NR] = (void*)MST_GRule_NR_IP_Index_Sub;
    PQTableInfo.pGRule_Array[E_GRULE_NR] = (void*)MST_GRule_NR_Sub;
#endif


    MDrv_PQ_AddTable_(SUB, &PQTableInfo);


    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SUB);
    }

    // table config parameter
    PQTableInfoEx = PQTableInfo;

    PQTableInfoEx.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_Sub_Ex;
    PQTableInfoEx.u8PQ_IP_Num = PQ_IP_NUM_Sub_Ex;
    PQTableInfoEx.pIPTAB_Info = (void*)PQ_IPTAB_INFO_Sub_Ex;
    PQTableInfoEx.pSkipRuleIP = (void*)MST_SkipRule_IP_Sub_Ex;

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_Sub_Ex;
            }
            break;

        case PQ_DISPLAY_PIP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_Sub_Ex;
            }
            break;

        case PQ_DISPLAY_POP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_Sub_Ex;
            }
            break;
    }
    MDrv_PQ_AddTable_(SUBEX, &PQTableInfoEx);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SUBEX);
    }

    return TRUE;


#else

    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
    UNUSED(bSkipCommTable);
    return FALSE;

#endif
}


MS_BOOL MDrv_PQ_Set_DisplayType_SC1_Main(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, MS_BOOL bSkipCommTable)
{
#if 0//Ryan
#if (PQ_ONLY_SUPPORT_BIN == 0)
    PQTABLE_INFO PQTableInfo;
    PQTABLE_INFO PQTableInfoEx;

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTABLE_INFO));
    MsOS_Memset(&PQTableInfoEx, 0, sizeof(PQTABLE_INFO));

    printf("++++MDrv_PQ_Set_DisplayType_SC1_Main++++ \r\n");

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            PQTableInfo.pQuality_Map_Aray = (void*)(QMAP_720_SC1_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)(MST_GRule_720_NR_SC1_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)(MST_GRule_720_MPEG_NR_SC1_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)(MST_GRule_720_FILM_MODE_SC1_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)(MST_GRule_720_DYNAMIC_CONTRAST_SC1_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)(MST_GRule_720_ULTRAT_CLEAR_SC1_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)(MST_GRule_720_DDR_SELECT_SC1_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)(MST_GRule_720_OSD_BW_SC1_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)(MST_GRule_PTP_SC1_Main);
#endif
             break;
#if PQ_ENABLE_PIP_SC1	//// JoshChiang++, 20120730 ////
        case PQ_DISPLAY_PIP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_PIP_SC1_Main: QMAP_1920_PIP_SC1_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_NR_SC1_Main: MST_GRule_1920_PIP_NR_SC1_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_MPEG_NR_SC1_Main:MST_GRule_1920_PIP_MPEG_NR_SC1_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_FILM_MODE_SC1_Main:MST_GRule_1920_PIP_FILM_MODE_SC1_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DYNAMIC_CONTRAST_SC1_Main:MST_GRule_1920_PIP_DYNAMIC_CONTRAST_SC1_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_ULTRAT_CLEAR_SC1_Main:MST_GRule_1920_PIP_ULTRAT_CLEAR_SC1_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DDR_SELECT_SC1_Main:MST_GRule_1920_PIP_DDR_SELECT_SC1_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_OSD_BW_SC1_Main:MST_GRule_1920_PIP_OSD_BW_SC1_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_PTP_SC1_Main:MST_GRule_1920_PIP_PTP_SC1_Main);
#endif

            break;

        case PQ_DISPLAY_POP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_POP_SC1_Main: QMAP_1920_POP_SC1_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_NR_SC1_Main: MST_GRule_1920_POP_NR_SC1_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_MPEG_NR_SC1_Main:MST_GRule_1920_POP_MPEG_NR_SC1_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_FILM_MODE_SC1_Main:MST_GRule_1920_POP_FILM_MODE_SC1_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DYNAMIC_CONTRAST_SC1_Main:MST_GRule_1920_POP_DYNAMIC_CONTRAST_SC1_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_ULTRAT_CLEAR_SC1_Main:MST_GRule_1920_POP_ULTRAT_CLEAR_SC1_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DDR_SELECT_SC1_Main:MST_GRule_1920_POP_DDR_SELECT_SC1_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_OSD_BW_SC1_Main:MST_GRule_1920_POP_OSD_BW_SC1_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_PTP_SC1_Main:MST_GRule_1920_POP_PTP_SC1_Main);
#endif
            break;
#endif
    }

    // table config parameter
    PQTableInfo.eWin = PQ_SC1_MAIN_WINDOW;
    PQTableInfo.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC1_Main;
    PQTableInfo.u8PQ_IP_Num = PQ_IP_NUM_SC1_Main;
    PQTableInfo.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC1_Main;
    PQTableInfo.pSkipRuleIP = (void*)MST_SkipRule_IP_SC1_Main;

    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSD] = PQ_HSDRule_IP_NUM_SC1_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSD] = PQ_VSDRule_IP_NUM_SC1_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSP] = PQ_HSPRule_IP_NUM_SC1_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSP] = PQ_VSPRule_IP_NUM_SC1_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_CSC] = PQ_CSCRule_IP_NUM_SC1_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_NTSC] = PQ_DBRule_NTSC_IP_NUM_SC1_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_PAL]  = PQ_DBRule_PAL_IP_NUM_SC1_Main;
#endif

    PQTableInfo.pXRule_IP_Index[E_XRULE_HSD] = (void*)MST_HSDRule_IP_Index_SC1_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSD] = (void*)MST_VSDRule_IP_Index_SC1_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSP] = (void*)MST_HSPRule_IP_Index_SC1_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSP] = (void*)MST_VSPRule_IP_Index_SC1_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_CSC] = (void*)MST_CSCRule_IP_Index_SC1_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_IP_Index_SC1_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_IP_Index_SC1_Main;
#endif

    PQTableInfo.pXRule_Array[E_XRULE_HSD] = (void*)MST_HSDRule_Array_SC1_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSD] = (void*)MST_VSDRule_Array_SC1_Main;
    PQTableInfo.pXRule_Array[E_XRULE_HSP] = (void*)MST_HSPRule_Array_SC1_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSP] = (void*)MST_VSPRule_Array_SC1_Main;
    PQTableInfo.pXRule_Array[E_XRULE_CSC] = (void*)MST_CSCRule_Array_SC1_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_Array[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_Array_SC1_Main;
    PQTableInfo.pXRule_Array[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_Array_SC1_Main;
#endif

#if (PQ_GRULE_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_NR] = PQ_GRULE_NR_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_NR] = PQ_GRULE_NR_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_NR] = (void*)MST_GRule_NR_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_NR] = (void*)MST_GRule_NR_SC1_Main;
#endif

#if (PQ_GRULE_MPEG_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_SC1_Main;
#endif

#if (PQ_GRULE_FILM_MODE_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_SC1_Main;
#endif

#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_SC1_Main;
#endif

#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_SC1_Main;
#endif

#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_SC1_Main;
#endif

#if (PQ_GRULE_OSD_BW_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_SC1_Main;
#endif

#if (PQ_GRULE_PTP_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_PTP] = PQ_GRULE_PTP_NUM_SC1_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_PTP] = PQ_GRULE_PTP_IP_NUM_SC1_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_PTP] = (void*)MST_GRule_PTP_IP_Index_SC1_Main;
    PQTableInfo.pGRule_Array[E_GRULE_PTP] = (void*)MST_GRule_PTP_SC1_Main;
#endif

    MDrv_PQ_AddTable_(SC1_MAIN, &PQTableInfo);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC1_MAIN);
    }

#if (ENABLE_PQ_EX)
    // table config parameter
    PQTableInfoEx = PQTableInfo;

    PQTableInfoEx.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC1_Main_Ex;
    PQTableInfoEx.u8PQ_IP_Num = PQ_IP_NUM_SC1_Main_Ex;
    PQTableInfoEx.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC1_Main_Ex;
    PQTableInfoEx.pSkipRuleIP = (void*)MST_SkipRule_IP_SC1_Main_Ex;

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_720_SC1_Main_Ex;
            break;

        case PQ_DISPLAY_PIP:
#if PQ_ENABLE_PIP_SC1	//// JoshChiang++, 20120730 ////
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC1_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC1_Main_Ex;
        }
#endif
        break;

        case PQ_DISPLAY_POP:
#if PQ_ENABLE_PIP_SC1	//// JoshChiang++, 20120730 ////
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC1_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC1_Main_Ex;
        }
#endif
        break;
    }

    MDrv_PQ_AddTable_(SC1_MAINEX, &PQTableInfoEx);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC1_MAINEX);
    }
    if((MS_U16)QM_INPUTTYPE_NUM_SC1_Main != (MS_U16)QM_INPUTTYPE_NUM_SC1_Main_Ex)
    {
        printf("PQ INPUTTYPE NUM MISMATCH: MAIN != EX");
        MS_ASSERT(0);
    }
#endif
    return TRUE;
#else
    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
    UNUSED(bSkipCommTable);
    return FALSE;
#endif
#else
    return TRUE;
#endif
}

MS_BOOL MDrv_PQ_Set_DisplayType_SC2_Main(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, MS_BOOL bSkipCommTable)
{
#if 0//Ryan
#if (PQ_ONLY_SUPPORT_BIN == 0)
    PQTABLE_INFO PQTableInfo;
    PQTABLE_INFO PQTableInfoEx;

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTABLE_INFO));
    MsOS_Memset(&PQTableInfoEx, 0, sizeof(PQTABLE_INFO));

    printf("++++MDrv_PQ_Set_DisplayType_SC2_Main++++ \r\n");
    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_SC2_Main: QMAP_1920_SC2_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_NR_SC2_Main: MST_GRule_1920_NR_SC2_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_MPEG_NR_SC2_Main:MST_GRule_1920_MPEG_NR_SC2_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_FILM_MODE_SC2_Main:MST_GRule_1920_FILM_MODE_SC2_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_DYNAMIC_CONTRAST_SC2_Main:MST_GRule_1920_DYNAMIC_CONTRAST_SC2_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_ULTRAT_CLEAR_SC2_Main:MST_GRule_1920_ULTRAT_CLEAR_SC2_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_DDR_SELECT_SC2_Main:MST_GRule_1920_DDR_SELECT_SC2_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_OSD_BW_SC2_Main:MST_GRule_1920_OSD_BW_SC2_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PTP_SC2_Main:MST_GRule_1920_PTP_SC2_Main);
#endif
             break;
#if PQ_ENABLE_PIP
        case PQ_DISPLAY_PIP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_PIP_SC2_Main: QMAP_1920_PIP_SC2_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_NR_SC2_Main: MST_GRule_1920_PIP_NR_SC2_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_MPEG_NR_SC2_Main:MST_GRule_1920_PIP_MPEG_NR_SC2_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_FILM_MODE_SC2_Main:MST_GRule_1920_PIP_FILM_MODE_SC2_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DYNAMIC_CONTRAST_SC2_Main:MST_GRule_1920_PIP_DYNAMIC_CONTRAST_SC2_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_ULTRAT_CLEAR_SC2_Main:MST_GRule_1920_PIP_ULTRAT_CLEAR_SC2_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_DDR_SELECT_SC2_Main:MST_GRule_1920_PIP_DDR_SELECT_SC2_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_OSD_BW_SC2_Main:MST_GRule_1920_PIP_OSD_BW_SC2_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_PIP_PTP_SC2_Main:MST_GRule_1920_PIP_PTP_SC2_Main);
#endif

            break;

        case PQ_DISPLAY_POP:
            PQTableInfo.pQuality_Map_Aray = (void*)((u16DisplayWidth == 1366)?QMAP_1920_POP_SC2_Main: QMAP_1920_POP_SC2_Main);
#if (PQ_GRULE_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_NR_SC2_Main: MST_GRule_1920_POP_NR_SC2_Main);
#endif
#if (PQ_GRULE_MPEG_NR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_MPEG_NR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_MPEG_NR_SC2_Main:MST_GRule_1920_POP_MPEG_NR_SC2_Main);
#endif
#if (PQ_GRULE_FILM_MODE_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_FilmMode] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_FILM_MODE_SC2_Main:MST_GRule_1920_POP_FILM_MODE_SC2_Main);
#endif
#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_DynContr] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DYNAMIC_CONTRAST_SC2_Main:MST_GRule_1920_POP_DYNAMIC_CONTRAST_SC2_Main);
#endif
#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
            PQTableInfo.pGRule_Level[E_GRULE_ULTRACLEAR] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_ULTRAT_CLEAR_SC2_Main:MST_GRule_1920_POP_ULTRAT_CLEAR_SC2_Main);
#endif
#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_DDR_SELECT] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_DDR_SELECT_SC2_Main:MST_GRule_1920_POP_DDR_SELECT_SC2_Main);
#endif
#if (PQ_GRULE_OSD_BW_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_OSD_BW] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_OSD_BW_SC2_Main:MST_GRule_1920_POP_OSD_BW_SC2_Main);
#endif
#if (PQ_GRULE_PTP_ENABLE == 1)
            PQTableInfo.pGRule_Level[E_GRULE_PTP] =
                (void*)((u16DisplayWidth == 1366)?MST_GRule_1920_POP_PTP_SC2_Main:MST_GRule_1920_POP_PTP_SC2_Main);
#endif
            break;
#endif
    }

    // table config parameter
    PQTableInfo.eWin = PQ_SC2_MAIN_WINDOW;
    PQTableInfo.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC2_Main;
    PQTableInfo.u8PQ_IP_Num = PQ_IP_NUM_SC2_Main;
    PQTableInfo.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC2_Main;
    PQTableInfo.pSkipRuleIP = (void*)MST_SkipRule_IP_SC2_Main;

    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSD] = PQ_HSDRule_IP_NUM_SC2_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSD] = PQ_VSDRule_IP_NUM_SC2_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSP] = PQ_HSPRule_IP_NUM_SC2_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSP] = PQ_VSPRule_IP_NUM_SC2_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_CSC] = PQ_CSCRule_IP_NUM_SC2_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_NTSC] = PQ_DBRule_NTSC_IP_NUM_SC2_Main;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_PAL]  = PQ_DBRule_PAL_IP_NUM_SC2_Main;
#endif

    PQTableInfo.pXRule_IP_Index[E_XRULE_HSD] = (void*)MST_HSDRule_IP_Index_SC2_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSD] = (void*)MST_VSDRule_IP_Index_SC2_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSP] = (void*)MST_HSPRule_IP_Index_SC2_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSP] = (void*)MST_VSPRule_IP_Index_SC2_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_CSC] = (void*)MST_CSCRule_IP_Index_SC2_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_IP_Index_SC2_Main;
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_IP_Index_SC2_Main;
#endif

    PQTableInfo.pXRule_Array[E_XRULE_HSD] = (void*)MST_HSDRule_Array_SC2_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSD] = (void*)MST_VSDRule_Array_SC2_Main;
    PQTableInfo.pXRule_Array[E_XRULE_HSP] = (void*)MST_HSPRule_Array_SC2_Main;
    PQTableInfo.pXRule_Array[E_XRULE_VSP] = (void*)MST_VSPRule_Array_SC2_Main;
    PQTableInfo.pXRule_Array[E_XRULE_CSC] = (void*)MST_CSCRule_Array_SC2_Main;
#if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_Array[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_Array_SC2_Main;
    PQTableInfo.pXRule_Array[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_Array_SC2_Main;
#endif

#if (PQ_GRULE_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_NR] = PQ_GRULE_NR_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_NR] = PQ_GRULE_NR_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_NR] = (void*)MST_GRule_NR_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_NR] = (void*)MST_GRule_NR_SC2_Main;
#endif

#if (PQ_GRULE_MPEG_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_MPEG_NR] = PQ_GRULE_MPEG_NR_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_MPEG_NR] = (void*)MST_GRule_MPEG_NR_SC2_Main;
#endif

#if (PQ_GRULE_FILM_MODE_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_FilmMode] = PQ_GRULE_FILM_MODE_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_FilmMode] = (void*)MST_GRule_FILM_MODE_SC2_Main;
#endif

#if (PQ_GRULE_DYNAMIC_CONTRAST_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DynContr] = PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DynContr] = (void*)MST_GRule_DYNAMIC_CONTRAST_SC2_Main;
#endif

#if (PQ_GRULE_ULTRAT_CLEAR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_ULTRACLEAR] = PQ_GRULE_ULTRAT_CLEAR_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_ULTRACLEAR] = (void*)MST_GRule_ULTRAT_CLEAR_SC2_Main;
#endif

#if (PQ_GRULE_DDR_SELECT_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_DDR_SELECT] = PQ_GRULE_DDR_SELECT_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_DDR_SELECT] = (void*)MST_GRule_DDR_SELECT_SC2_Main;
#endif

#if (PQ_GRULE_OSD_BW_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_OSD_BW] = PQ_GRULE_OSD_BW_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_OSD_BW] = (void*)MST_GRule_OSD_BW_SC2_Main;
#endif

#if (PQ_GRULE_PTP_ENABLE == 1)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_PTP] = PQ_GRULE_PTP_NUM_SC2_Main;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_PTP] = PQ_GRULE_PTP_IP_NUM_SC2_Main;
    PQTableInfo.pGRule_IP_Index[E_GRULE_PTP] = (void*)MST_GRule_PTP_IP_Index_SC2_Main;
    PQTableInfo.pGRule_Array[E_GRULE_PTP] = (void*)MST_GRule_PTP_SC2_Main;
#endif

    MDrv_PQ_AddTable_(SC2_MAIN, &PQTableInfo);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC2_MAIN);
    }

#if (ENABLE_PQ_EX)
    // table config parameter
    PQTableInfoEx = PQTableInfo;

    PQTableInfoEx.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC2_Main_Ex;
    PQTableInfoEx.u8PQ_IP_Num = PQ_IP_NUM_SC2_Main_Ex;
    PQTableInfoEx.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC2_Main_Ex;
    PQTableInfoEx.pSkipRuleIP = (void*)MST_SkipRule_IP_SC2_Main_Ex;

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Main_Ex;
        }
        break;

        case PQ_DISPLAY_PIP:
#if PQ_ENABLE_PIP
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Main_Ex;
        }
#endif
        break;

        case PQ_DISPLAY_POP:
#if PQ_ENABLE_PIP
        if(u16DisplayWidth == 1366)
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Main_Ex;
        }
        else
        {
            PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Main_Ex;
        }
#endif
        break;
    }

    MDrv_PQ_AddTable_(SC2_MAINEX, &PQTableInfoEx);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC2_MAINEX);
    }
    if((MS_U16)QM_INPUTTYPE_NUM_SC2_Main != (MS_U16)QM_INPUTTYPE_NUM_SC2_Main_Ex)
    {
        printf("PQ INPUTTYPE NUM MISMATCH: MAIN != EX");
        MS_ASSERT(0);
    }
#endif
    return TRUE;
#else
    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
    UNUSED(bSkipCommTable);
    return FALSE;
#endif
#else
    return TRUE;
#endif
}

MS_BOOL MDrv_PQ_Set_DisplayType_SC2_Sub(MS_U16 u16DisplayWidth, PQ_DISPLAY_TYPE enDisplaType, MS_BOOL bSkipCommTable)
{
#if 0//Ryan
#if PQ_ENABLE_PIP && (PQ_ONLY_SUPPORT_BIN == 0)
    PQTABLE_INFO PQTableInfo;
    PQTABLE_INFO PQTableInfoEx;

    MsOS_Memset(&PQTableInfo, 0, sizeof(PQTABLE_INFO));
    MsOS_Memset(&PQTableInfoEx, 0, sizeof(PQTABLE_INFO));

    printf("++++MDrv_PQ_Set_DisplayType_SC2_Sub++++ \r\n");
    // Load SC2_Sub PQ
    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_NR_SC2_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_NR_SC2_Sub;
                #endif
            }
            break;

        case PQ_DISPLAY_PIP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_PIP_NR_SC2_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_PIP_NR_SC2_Sub;
                #endif
            }
            break;

        case PQ_DISPLAY_POP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_POP_NR_SC2_Sub;
                #endif
            }
            else
            {
                PQTableInfo.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Sub;
                #if (PQ_GRULE_NR_ENABLE)
                PQTableInfo.pGRule_Level[E_GRULE_NR] = (void*)MST_GRule_1920_POP_NR_SC2_Sub;
                #endif
            }
            break;
    }

    // table config parameter
    PQTableInfo.eWin = PQ_SC2_SUB_WINDOW;
    PQTableInfo.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC2_Sub;
    PQTableInfo.u8PQ_IP_Num = PQ_IP_NUM_SC2_Sub;
    PQTableInfo.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC2_Sub;
    PQTableInfo.pSkipRuleIP = (void*)MST_SkipRule_IP_SC2_Sub;

    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSD] = PQ_HSDRule_IP_NUM_SC2_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSD] = PQ_VSDRule_IP_NUM_SC2_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_HSP] = PQ_HSPRule_IP_NUM_SC2_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_VSP] = PQ_VSPRule_IP_NUM_SC2_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_CSC] = PQ_CSCRule_IP_NUM_SC2_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_NTSC] = PQ_DBRule_NTSC_IP_NUM_SC2_Sub;
    PQTableInfo.u8PQ_XRule_IP_Num[E_XRULE_DB_PAL] = PQ_DBRule_PAL_IP_NUM_SC2_Sub;
    #endif

    PQTableInfo.pXRule_IP_Index[E_XRULE_HSD] = (void*)MST_HSDRule_IP_Index_SC2_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSD] = (void*)MST_VSDRule_IP_Index_SC2_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_HSP] = (void*)MST_HSPRule_IP_Index_SC2_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_VSP] = (void*)MST_VSPRule_IP_Index_SC2_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_CSC] = (void*)MST_CSCRule_IP_Index_SC2_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_IP_Index_SC2_Sub;
    PQTableInfo.pXRule_IP_Index[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_IP_Index_SC2_Sub;
    #endif

    PQTableInfo.pXRule_Array[E_XRULE_HSD] = (void*)MST_HSDRule_Array_SC2_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_VSD] = (void*)MST_VSDRule_Array_SC2_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_HSP] = (void*)MST_HSPRule_Array_SC2_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_VSP] = (void*)MST_VSPRule_Array_SC2_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_CSC] = (void*)MST_CSCRule_Array_SC2_Sub;
    #if (PQ_XRULE_DB_ENABLE == 1)
    PQTableInfo.pXRule_Array[E_XRULE_DB_NTSC] = (void*)MST_DBRule_NTSC_Array_SC2_Sub;
    PQTableInfo.pXRule_Array[E_XRULE_DB_PAL]  = (void*)MST_DBRule_PAL_Array_SC2_Sub;
    #endif

#if (PQ_GRULE_NR_ENABLE)
    PQTableInfo.u8PQ_GRule_Num[E_GRULE_NR] = PQ_GRULE_NR_NUM_SC2_Sub;
    PQTableInfo.u8PQ_GRule_IPNum[E_GRULE_NR] = PQ_GRULE_NR_IP_NUM_SC2_Sub;
    PQTableInfo.pGRule_IP_Index[E_GRULE_NR] = (void*)MST_GRule_NR_IP_Index_SC2_Sub;
    PQTableInfo.pGRule_Array[E_GRULE_NR] = (void*)MST_GRule_NR_SC2_Sub;
#endif


    MDrv_PQ_AddTable_(SC2_SUB, &PQTableInfo);


    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC2_SUB);
    }

    // table config parameter
    PQTableInfoEx = PQTableInfo;

    PQTableInfoEx.u8PQ_InputType_Num = QM_INPUTTYPE_NUM_SC2_Sub_Ex;
    PQTableInfoEx.u8PQ_IP_Num = PQ_IP_NUM_SC2_Sub_Ex;
    PQTableInfoEx.pIPTAB_Info = (void*)PQ_IPTAB_INFO_SC2_Sub_Ex;
    PQTableInfoEx.pSkipRuleIP = (void*)MST_SkipRule_IP_SC2_Sub_Ex;

    switch(enDisplaType)
    {
        default:
        case PQ_DISPLAY_ONE:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_SC2_Sub_Ex;
            }
            break;

        case PQ_DISPLAY_PIP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_PIP_SC2_Sub_Ex;
            }
            break;

        case PQ_DISPLAY_POP:
            if(u16DisplayWidth == 1366)
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Sub_Ex;
            }
            else
            {
                PQTableInfoEx.pQuality_Map_Aray = (void*)QMAP_1920_POP_SC2_Sub_Ex;
            }
            break;
    }
    MDrv_PQ_AddTable_(SC2_SUBEX, &PQTableInfoEx);
    if(!bSkipCommTable)
    {
        MDrv_PQ_LoadCommTable_(SC2_SUBEX);
    }

    return TRUE;


#else

    UNUSED(u16DisplayWidth);
    UNUSED(enDisplaType);
    UNUSED(bSkipCommTable);
    return FALSE;

#endif
#else
    return TRUE;
#endif
}


MS_BOOL MDrv_PQ_GetMADiFromGrule(PQ_WIN eWindow, MS_U16 *pu16TabIdx_MADi_Motion, MS_U16 *pu16TabIdx_MADi)
{
#if PQ_GRULE_OSD_BW_ENABLE
    MS_U16 u16PQ_OSDBWIdx = 0;

    if(eWindow == PQ_MAIN_WINDOW)
    {
    #if PQ_GRULE_DDR_SELECT_ENABLE
        {
            if(bw_info.u32Miu1MemSize ==0) // 1 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_1DDR_On_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
            else // 2 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_2DDR_On_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
        }
    #else //DDR select disable
        {
            u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_Main, PQ_GRule_Lvl_OSD_BW_On_Main);
            *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
        }
    #endif

    #if (PQ_GRULE_OSD_BW_IP_NUM_Main > 1)
        *pu16TabIdx_MADi_Motion = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_Main, u16PQ_OSDBWIdx, 1); // Add MADi_Motion ip
    #else
        UNUSED(pu16TabIdx_MADi_Motion);
    #endif
    }
 #if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
    #if PQ_GRULE_DDR_SELECT_ENABLE
        {
            if(bw_info.u32Miu1MemSize ==0) // 1 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, PQ_GRule_Lvl_OSD_BW_1DDR_On_SC1_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
            else // 2 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, PQ_GRule_Lvl_OSD_BW_2DDR_On_SC1_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
        }
    #else //DDR select disable
        {
            u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, PQ_GRule_Lvl_OSD_BW_On_SC1_Main);
            *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
        }
    #endif

    #if (PQ_GRULE_OSD_BW_IP_NUM_SC1_Main > 1)
        *pu16TabIdx_MADi_Motion = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC1_Main, u16PQ_OSDBWIdx, 1); // Add MADi_Motion ip
    #else
        UNUSED(pu16TabIdx_MADi_Motion);
    #endif
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
    #if PQ_GRULE_DDR_SELECT_ENABLE
        {
            if(bw_info.u32Miu1MemSize ==0) // 1 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, PQ_GRule_Lvl_OSD_BW_1DDR_On_SC1_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
            else // 2 DDR
            {
                u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, PQ_GRule_Lvl_OSD_BW_2DDR_On_SC2_Main);
                *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
            }
        }
    #else //DDR select disable
        {
            u16PQ_OSDBWIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, PQ_GRule_Lvl_OSD_BW_On_SC2_Main);
            *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, u16PQ_OSDBWIdx, 0); // only MADi ip
        }
    #endif

    #if (PQ_GRULE_OSD_BW_IP_NUM_SC2_Main > 1)
        *pu16TabIdx_MADi_Motion = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_OSD_BW_SC2_Main, u16PQ_OSDBWIdx, 1); // Add MADi_Motion ip
    #else
        UNUSED(pu16TabIdx_MADi_Motion);
    #endif
    }
#endif // ENABLE_MULTI_SCALER
    else
    {
        // TODO: Should sub window need to consider the OSD BW ??
    }
#else
    UNUSED(eWindow);
    UNUSED(pu16TabIdx_MADi_Motion);
    UNUSED(pu16TabIdx_MADi);
#endif

    return TRUE;
}

MS_BOOL MDrv_PQ_GetMemFmtInGeneral(PQ_WIN eWindow,
                                            MS_BOOL bMemFmt422,
                                            MS_U16 *pu16TabIdx_MemFormat,
                                            MS_U16 *pu16TabIdx_444To422,
                                            MS_U16 *pu16TabIdx_422To444
                                            )
{
#if (PQ_ENABLE_MEMFMT)
    if(bMemFmt422)
    {
        if(eWindow == PQ_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_Sub);
            *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Sub;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#endif

#if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC1_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_ON_SC1_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC2_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_ON_SC2_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC2_Sub);
            *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Sub;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#endif
#endif // ENABLE_MULTI_SCALER
    }
    else
    {
        // use 444_10BIT mode only if < 1440x1080
        // frame buffer 1920x1080x3x2 == 1440x1080x4x2
        // also bandwidth not enough for 1920x1080 444_10BIT
        if(eWindow == PQ_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_OFF_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_Sub);
            *pu16TabIdx_444To422 = PQ_IP_444To422_OFF_Sub;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#endif

#if ENABLE_MULTI_SCALER

        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC1_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_OFF_SC1_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC2_Main);
            *pu16TabIdx_444To422 = PQ_IP_444To422_OFF_SC2_Main;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            *pu16TabIdx_MemFormat = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MemFormat_SC2_Sub);
            *pu16TabIdx_444To422 = PQ_IP_444To422_OFF_SC2_Sub;
            *pu16TabIdx_422To444 = Hal_PQ_get_422To444_idx((MS_422To444_TYPE)Hal_PQ_get_422To444_mode(bMemFmt422));
        }
#endif
#endif // ENABLE_MULTI_SCALER
    }
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_GetMADiInGeneral(PQ_WIN eWindow, MS_U16 *pu16TabIdx_MADi_Motion, MS_U16 *pu16TabIdx_MADi)
{
#if (PQ_ENABLE_MEMFMT)
    UNUSED(pu16TabIdx_MADi_Motion);
    UNUSED(pu16TabIdx_MADi);
#if PQ_GRULE_DDR_SELECT_ENABLE
    if(eWindow == PQ_MAIN_WINDOW)
    {
         MS_U16 u16PQ_DDRSELIdx = 0;
         if(bw_info.u32Miu1MemSize==0) // 1 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_Main, PQ_GRule_Lvl_DDR_SELECT_1DDR_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
         else // 2 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_Main, PQ_GRule_Lvl_DDR_SELECT_2DDR_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
    }
    else if(eWindow == PQ_SUB_WINDOW)
    {
         *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_Sub);
    }

#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
         MS_U16 u16PQ_DDRSELIdx = 0;
         if(bw_info.u32Miu1MemSize==0) // 1 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_SC1_Main, PQ_GRule_Lvl_DDR_SELECT_1DDR_SC1_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_SC1_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
         else // 2 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_SC1_Main, PQ_GRule_Lvl_DDR_SELECT_2DDR_SC1_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_SC1_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
         MS_U16 u16PQ_DDRSELIdx = 0;
         if(bw_info.u32Miu1MemSize==0) // 1 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_SC2_Main, PQ_GRule_Lvl_DDR_SELECT_1DDR_SC2_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_SC2_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
         else // 2 DDR
         {
              u16PQ_DDRSELIdx = MDrv_PQ_GetGRule_LevelIndex(eWindow, PQ_GRule_DDR_SELECT_SC2_Main, PQ_GRule_Lvl_DDR_SELECT_2DDR_SC2_Main);
              *pu16TabIdx_MADi = MDrv_PQ_GetGRule_TableIndex(eWindow, PQ_GRule_DDR_SELECT_SC2_Main, u16PQ_DDRSELIdx, 0); // only MADi ip
         }
    }
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
         *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_SC2_Sub);
    }
#endif // ENABLE_MULTI_SCALER
#else //DDR select disable
    {
        if(eWindow == PQ_MAIN_WINDOW)
        {
            *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_Main);
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_Sub);

        }
#endif

#if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_SC2_Main);
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            *pu16TabIdx_MADi = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_MADi_SC2_Sub);

        }
#endif
#endif
       //printf("$$[%s]:%d, %s, u16TabIdx_MADi=%u\r\n", __FUNCTION__, __LINE__, eWindow?"SUB":"MAIN", u16TabIdx_MADi);
    }
#endif
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_GetMADiForRFBL(PQ_WIN eWindow, MS_BOOL bFBL, MS_U16 *pu16TabIdx_MADi_Motion, MS_U16 *pu16TabIdx_MADi)
{
    UNUSED(pu16TabIdx_MADi_Motion);
    if((_stRFBL_Info.bEnable)&&(!bFBL))
    {
        *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, (MS_MADI_TYPE)_stRFBL_Info.u8MADiType);
        if(!_stMode_Info[eWindow].bInterlace)
        {
            *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8_NO_MIU);

            MDrv_PQ_MADiForceMotionY(eWindow, FALSE);
            MDrv_PQ_MADiForceMotionC(eWindow, FALSE);
            //printf("[%s]-[%d] :  disable Y/C motion\r\n", __FUNCTION__, __LINE__);
        }
        //printf("[%s]-[%d] : u16TabIdx_MADi = %d\r\n",__FUNCTION__,__LINE__,u16TabIdx_MADi);
    }

    return TRUE;
}

MS_BOOL MDrv_PQ_Patch2Rto4RForFieldPackingMode(PQ_WIN eWindow, MS_U16 u16TabIdx_MADi)
{
#if 0//Ryan
#if PQ_GRULE_OSD_BW_ENABLE
    // Patch: in OSD turn off case, when 2R-->4R, J2, M10 with new OPM system will flash(4R-->2R is OK)
    // we need to set 4R first, and wait some time, and do other MAdi setting, the flash will be ok.
    // which is only for non-OS platform patch, since OS platform won't turn off OSD for DFB system.
    // Only FieldPackingMode need this solution
    if(eWindow == PQ_MAIN_WINDOW)
    {
        if((MApi_XC_IsFieldPackingModeSupported() &&
        (_stMode_Info[eWindow].u16input_hsize>=1280)&&
        (_stMode_Info[eWindow].u16input_vsize>=720) &&
        (!_bOSD_On)) &&
        ((u16TabIdx_MADi == PQ_IP_MADi_24_4R_Main) || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_Main) ||
        (u16TabIdx_MADi == PQ_IP_MADi_26_4R_Main) || (u16TabIdx_MADi == PQ_IP_MADi_27_4R_Main)
#if PQ_MADI_88X_MODE
        || (u16TabIdx_MADi == PQ_IP_MADi_24_4R_880_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_880_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_884_Main)
#endif
        ))
        {
            //set to 4R first,
            MApi_XC_W2BYTEMSK(REG_SC_BK12_03_L, 0 , 0x0002 );
            //delay
            _MDrv_PQ_wait_output_vsync(eWindow, 1, 90);
        }
    }

#if ENABLE_MULTI_SCALER
    if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
        if((MApi_XC_IsFieldPackingModeSupported() &&
        (_stMode_Info[eWindow].u16input_hsize>=1280)&&
        (_stMode_Info[eWindow].u16input_vsize>=720) &&
        (!_bOSD_On)) &&
        ((u16TabIdx_MADi == PQ_IP_MADi_24_4R_SC1_Main) || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_SC1_Main) ||
        (u16TabIdx_MADi == PQ_IP_MADi_26_4R_SC1_Main) || (u16TabIdx_MADi == PQ_IP_MADi_27_4R_SC1_Main)
#if PQ_MADI_88X_MODE
        || (u16TabIdx_MADi == PQ_IP_MADi_24_4R_880_SC1_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_880_SC1_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_884_SC1_Main)
#endif
        ))
        {
            //set to 4R first,
            MApi_XC_W2BYTEMSK(REG_SC1_BK12_03_L, 0 , 0x0002 );
            //delay
            _MDrv_PQ_wait_output_vsync(eWindow, 1, 90);
        }
    }

    if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
        if((MApi_XC_IsFieldPackingModeSupported() &&
        (_stMode_Info[eWindow].u16input_hsize>=1280)&&
        (_stMode_Info[eWindow].u16input_vsize>=720) &&
        (!_bOSD_On)) &&
        ((u16TabIdx_MADi == PQ_IP_MADi_24_4R_SC2_Main) || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_SC2_Main) ||
        (u16TabIdx_MADi == PQ_IP_MADi_26_4R_SC2_Main) || (u16TabIdx_MADi == PQ_IP_MADi_27_4R_SC2_Main)
#if PQ_MADI_88X_MODE
        || (u16TabIdx_MADi == PQ_IP_MADi_24_4R_880_SC2_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_880_SC2_Main)
        || (u16TabIdx_MADi == PQ_IP_MADi_25_4R_884_SC2_Main)
#endif
        ))
        {
            //set to 4R first,
            MApi_XC_W2BYTEMSK(REG_SC2_BK12_03_L, 0 , 0x0002 );
            //delay
            _MDrv_PQ_wait_output_vsync(eWindow, 1, 90);
        }
    }
#endif // ENABLE_MULTI_SCALER

#else
    UNUSED(eWindow);
    UNUSED(u16TabIdx_MADi);
#endif
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_GetBPPInfoFromMADi(PQ_WIN eWindow,
                                              MS_BOOL bMemFmt422,
                                              MS_U16 u16TabIdx_MADi,
                                              MS_U16 u16TabIdx_MemFormat,
                                              MS_U8 *pu8BitsPerPixel,
                                              PQ_DEINTERLACE_MODE *peDeInterlaceMode,
                                              MS_U8 *pu8FrameCount
                                              )
{
#if 0//Ryan
    if(bMemFmt422)
    {
        // decide bits/pixel and deinterlace mode flag
        if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_4R))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;

            PQMADi_DBG(printf("24_4R PQ_DEINT_3DDI_HISTORY \r\n"));
        }
        else if( (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_2R)) ||
            (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_2R_880)))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            PQMADi_DBG(printf("24_2R PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            PQMADi_DBG(printf("25 4R PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R_MC))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            PQMADi_DBG(printf("25 4R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_MC))
        {
            *pu8BitsPerPixel = 24;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("25 6R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_14F_6R_MC))  //Add New MADi mode
        {
            *pu8BitsPerPixel = 24;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 14;
            PQMADi_DBG(printf("25 14F 6R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_14F_8R_MC))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *pu8FrameCount = 14;
            PQMADi_DBG(printf("25 14F 8R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_12F_8R_MC))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *pu8FrameCount = 12;
            PQMADi_DBG(printf("25 12F 8R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_16F_8R_MC))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *pu8FrameCount = 16;
            PQMADi_DBG(printf("25 16F 8R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_MC_NW))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("25 6R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_FilmPreDet))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("25 6R FilmPreDet_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_6R_FilmPreDet_PAL))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("25 6R FilmPreDet PAL_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_6R_FilmPreDet))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("24 6R FilmPreDet_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_6R_FilmPreDet_PAL))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("24 6R FilmPreDet PAL_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_8F_4R_MC))
        {
            *pu8BitsPerPixel = 24;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            *pu8FrameCount = 8;
            PQMADi_DBG(printf("25 8F 4R MC_PQ_DEINT_3DDI_HISTORY\r\n"));
        }
#if PQ_ENABLE_RFBL
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_2D)) // Need to ReDefine
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_25D)) // Need to ReDefine
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D)) // Need to ReDefine
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D_YC))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_3D_FILM))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
        }
#endif
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            PQMADi_DBG(printf("25 2 PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_26_4R))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            PQMADi_DBG(printf("26 4 PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_26_2R))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            PQMADi_DBG(printf("26 2 PQ_DEINT_3DDI_HISTORY\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_27_4R))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_3DDI;
            PQMADi_DBG(printf("27 4 PQ_DEINT_3DDI\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_27_2R))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_3DDI;
            PQMADi_DBG(printf("27 2 PQ_DEINT_3DDI\r\n"));
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P 8 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE10))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P 10 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT8))
        {
            *pu8BitsPerPixel = 20;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P M 8 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT10))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P M 10 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_RFBL_FILM))
        {
            *pu8BitsPerPixel = 12;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            //printf("====================================   MS_MADI_24_RFBL_FILM\r\n");
            //printf("24 PQ_DEINT_3DDI_HISTORY\r\n");
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_24_RFBL_NFILM))
        {
            *pu8BitsPerPixel = 12;
            *peDeInterlaceMode = PQ_DEINT_3DDI_HISTORY;
            //printf("====================================   MS_MADI_24_RFBL_NFILM\r\n");
            //printf("24 PQ_DEINT_3DDI_HISTORY\r\n");
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_FBL_DNR))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_2DDI_AVG;
            //printf("====================================   MS_MADI_FBL_DNR\r\n");
            //printf("24 PQ_DEINT_2DDI_AVG\r\n");
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_FBL_MIU))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_2DDI_AVG;
            //printf("====================================   MS_MADI_FBL_MIU\r\n");
            //printf("24 PQ_DEINT_2DDI_AVG\r\n");
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8_NO_MIU))
        {
            *pu8BitsPerPixel = 16;
            *peDeInterlaceMode = PQ_DEINT_2DDI_AVG;
            //printf("====================================   MS_MADI_P_MODE8_NO_MIU\r\n");
            //printf("24 PQ_DEINT_2DDI_AVG\r\n");
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT10_8Frame))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            PQMADi_DBG(printf("P M 10 8Frame PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 8;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MOT10_4Frame))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            MApi_XC_Set_OPWriteOffEnable(DISABLE, MAIN_WINDOW);
            PQMADi_DBG(printf("P M 10 4Frame PQ_DEINT_2DDI_BOB\r\n"));
        }
        else if (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MC_3Frame))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            MApi_XC_Set_OPWriteOffEnable(DISABLE,MAIN_WINDOW);
            PQMADi_DBG(printf("P M 10 3Frame PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 3;
        }
        else if (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MC_6Frame_6R))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            MApi_XC_Set_OPWriteOffEnable(DISABLE,MAIN_WINDOW);
            *pu8FrameCount = 6;
        }
        else if (u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MC_12Frame_8R))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            MApi_XC_Set_OPWriteOffEnable(DISABLE,MAIN_WINDOW);
            *pu8FrameCount = 12;
        }
#if PQ_ENABLE_RFBL
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_P_MODE10_MOT))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("RFBL P M 10 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_RFBL_P_MODE8_MOT))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("RFBL P M 8 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
#endif
        else
        {
            MS_ASSERT(0);
            PQMADi_DBG(printf("None\r\n"));
        }

    }
    else
    {
        //in order to disable user mode,
        if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8_444))
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P 8 444 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }
        else if(u16TabIdx_MADi == Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE10_444))
        {
            *pu8BitsPerPixel = 30;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("P 10 444 PQ_DEINT_2DDI_BOB\r\n"));
            *pu8FrameCount = 2;
        }

        if((u16TabIdx_MemFormat == PQ_IP_MemFormat_444_10BIT_Main)
#if PQ_ENABLE_PIP
            ||(u16TabIdx_MemFormat == PQ_IP_MemFormat_444_10BIT_Sub)
#endif
            )
        {
            *pu8BitsPerPixel = 32;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("PQ_IP_MemFormat_444_10BIT\r\n"));
            *pu8FrameCount = 2;
        }
        else if((u16TabIdx_MemFormat == PQ_IP_MemFormat_444_8BIT_Main)
#if PQ_ENABLE_PIP
            ||(u16TabIdx_MemFormat == PQ_IP_MemFormat_444_8BIT_Sub)
#endif
            )
        {
            *pu8BitsPerPixel = 24;
            *peDeInterlaceMode = PQ_DEINT_2DDI_BOB;
            PQMADi_DBG(printf("PQ_IP_MemFormat_444_8BIT\r\n"));
            *pu8FrameCount = 2;
        }
        else
        {
            *pu8BitsPerPixel = 24;
            PQMADi_DBG(printf("PQ_IP_MemFormat_422MF_Main\r\n"));
            *pu8FrameCount = 2;
        }

        if ( MApi_XC_R2BYTE(REG_SC_BK2A_02_L) & BIT(15) )   //if on need disable.
        {
#if(ENABLE_PQ_MLOAD)
            MDrv_PQ_Set_MLoadEn(eWindow, TRUE);
#endif
#if PQ_UC_CTL
            MDrv_PQ_LoadTable(eWindow, PQ_IP_UC_CTL_OFF_Main, PQ_IP_UC_CTL_Main);
#endif
#if(ENABLE_PQ_MLOAD)
            MDrv_PQ_Set_MLoadEn(eWindow, FALSE);
#endif
        }
    }
#endif
    return TRUE;
}

MS_BOOL MDrv_PQ_ChangeMemConfigFor3D(PQ_WIN eWindow,
                                            MS_BOOL bMemFmt422,
                                            MS_U16 *pu16TabIdx_MemFormat,
                                            MS_U16 *pu16TabIdx_444To422,
                                            MS_U16 *pu16TabIdx_422To444,
                                            MS_U8 *pu8BitsPerPixel,
                                            MS_U16 *pu16TabIdx_MADi_Motion,
                                            MS_U16 *pu16TabIdx_MADi
                                            )
{
#if 0 //Ryan
    UNUSED(pu16TabIdx_MADi_Motion);
    if(eWindow == PQ_MAIN_WINDOW)
    {
        //reduce mem size, put it into 16M, 2 frame mode.
        if((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING)
            && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_FRAME_ALTERNATIVE)
            && (_stMode_Info[eWindow].u16input_hsize == 1920)
            && (_stMode_Info[eWindow].u16input_vsize == 2205))
        {
            *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8_444);
            *pu8BitsPerPixel = 16;

            // DVI is 444 input, we need to conver it to 422, otherwise DVI will use 32 bpp instead, not 16
            // because 444 format combine to the setting of pu16TabIdx_MemFormat
            if(!bMemFmt422)
            {
                //PQ_IP_MemFormat_Main 422
                *pu16TabIdx_MemFormat = PQ_IP_MemFormat_422MF_Main;
                *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Main;
                //PQ_IP_422To444_Main on
                *pu16TabIdx_422To444 = PQ_IP_422To444_ON_Main;
                PQBW_DBG(printf("3D: force 422, 444to422 and 422to444 ON\r\n");)
            }

            printf("3D: modify progressive memory fmt for 1080p framepacking\r\n");
        }

        if(MApi_XC_Get_3D_HW_Version() < 2)
        {
            //PQ main page maybe MADi mode to 8 field mode, but it cannot do madi for line by line output.
            //so we force it back to normal 25_2R like mode.
            if((MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_TOP_BOTTOM) && (TRUE == _stMode_Info[eWindow].bInterlace))
            {
                // we must use MS_MADI_25_4R or 2R to avoid MC madi mode for dnr offset calcuation problem.
                if(TRUE == _stMode_Info[eWindow].bInterlace)
                {
                    if((_stMode_Info[eWindow].u16input_vsize >= 900) &&
                        (_stMode_Info[eWindow].u16input_hsize >= 1440) &&
                        (bw_info.u32Miu1MemSize==0))// 1ddr case, should save bw
                    {
                        *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R);
                        PQBW_DBG(printf("3D: force to 25_2R\r\n");)
                    }
                    else
                    {
                        *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_4R);
                        PQBW_DBG(printf("3D: force to 25_4R\r\n");)
                    }
                }

                //Madi do line by line must use 422 format
                if(!bMemFmt422)
                {
                    //PQ_IP_MemFormat_Main 422
                    *pu16TabIdx_MemFormat = PQ_IP_MemFormat_422MF_Main;
                    *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Main;
                    //PQ_IP_422To444_Main on
                    *pu16TabIdx_422To444 = PQ_IP_422To444_ON_Main;
                    PQBW_DBG(printf("3D: force 422, 444to422 and 422to444 ON\r\n");)
                }

                // we can only process 422 format for E_XC_3D_INPUT_TOP_BOTTOM.
                // so, if it's 444 format, we have to force it to be 422 and the u8BitsPerPixel is also changed to 24.
                *pu8BitsPerPixel = 24;
                PQBW_DBG(printf("3D:force u8BitsPerPixel to 24\r\n");)
            }

            //frame alterntive must use bob mode
            if((E_XC_3D_OUTPUT_FRAME_ALTERNATIVE_NOFRC == MApi_XC_Get_3D_Output_Mode()) ||
               (E_XC_3D_OUTPUT_FRAME_ALTERNATIVE == MApi_XC_Get_3D_Output_Mode()))
            {
                MDrv_PQ_EnableMADIForce(PQ_MAIN_WINDOW, TRUE);
#if PQ_MADI_DFK
                MDrv_PQ_LoadTable(eWindow, PQ_IP_MADi_DFK_OFF_Main, PQ_IP_MADi_DFK_Main);
#endif
                MDrv_PQ_LoadTable(eWindow, PQ_IP_EODi_OFF_Main, PQ_IP_EODi_Main);
            }
            //line by line must use weave mode
            else if(E_XC_3D_OUTPUT_LINE_ALTERNATIVE == MApi_XC_Get_3D_Output_Mode())
            {
                MDrv_PQ_EnableMADIForce(eWindow, FALSE);
            }

            //None 3D case: by Ethan.Lee, saveing memory size for 1920x1200
            if ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_MODE_NONE) &&
                (MApi_XC_Get_3D_Output_Mode()== E_XC_3D_OUTPUT_MODE_NONE) )
            {
                // Saving memory size for 1920x1200P HDMI 20110421EL
                if (  (_stMode_Info[eWindow].u16input_vsize > 1100) && (QM_IsSourceHDMI_Video(_enInputSourceType[eWindow], eWindow)))
                {
                    *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE8_444);
                    *pu8BitsPerPixel = 16;
                    if(!bMemFmt422)
                    {
                        //PQ_IP_MemFormat_Main 422
                        *pu16TabIdx_MemFormat = PQ_IP_MemFormat_422MF_Main;
                        *pu16TabIdx_444To422 = PQ_IP_444To422_ON_Main;
                        //PQ_IP_422To444_Main on
                        *pu16TabIdx_422To444 = PQ_IP_422To444_ON_Main;
                        PQBW_DBG(printf("3D: force 422, 444to422 and 422to444 ON\r\n");)
                    }
                }
            }
        }

        //Frame packing interlace must use 25_2R for
        // A. Madi for line by line output for old 3D engine. Or,
        // B. better quality. (P mode will shaking a little)
        if(((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_FRAME_L) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            || ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_FRAME_R) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            || ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_TOP_BOTTOM) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            || ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_SIDE_BY_SIDE_HALF) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            || ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_LINE_ALTERNATIVE) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            || ((MApi_XC_Get_3D_Input_Mode((SCALER_WIN)eWindow) == E_XC_3D_INPUT_FRAME_PACKING) && (MApi_XC_Get_3D_Output_Mode() == E_XC_3D_OUTPUT_FRAME_ALTERNATIVE) && (_stMode_Info[eWindow].u16input_vsize == 2228))
            )
        {
            //framepacking interlace 1080i
            *pu16TabIdx_MADi = Hal_PQ_get_madi_idx(eWindow, MS_MADI_25_2R);
            _stMode_Info[eWindow].bInterlace = TRUE;
            *pu8BitsPerPixel = 24;

            PQBW_DBG(printf("3D: force to 25_2R\r\n");)
        }
    }
    return TRUE;
#else
    return TRUE;
#endif
}

MS_BOOL MDrv_PQ_ForceBPPForDynamicMemFmt(PQ_WIN eWindow, MS_BOOL bMemFmt422, MS_U8 *pu8BitsPerPixel)
{
    //force Memory FMT and use the same pixel number to avoid the garbage when memory mode is changed.
    if(bMemFmt422)
    {
        MS_U16 u16MemFMT= 0;

        if(!_stMode_Info[eWindow].bInterlace)
        {
            // for dynamically memory format changing, we need to keep it as 24 bits
            if(SUPPORT_DYNAMIC_MEM_FMT)//STB could be false
            {
                *pu8BitsPerPixel = 24;
            }
        }

        if (eWindow == PQ_MAIN_WINDOW)
        {
            u16MemFMT = MApi_XC_R2BYTEMSK(REG_SC_BK12_02_L, 0xFF);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            u16MemFMT = MApi_XC_R2BYTEMSK(REG_SC_BK12_42_L, 0xFF);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if (eWindow == PQ_SC1_MAIN_WINDOW)
        {
            u16MemFMT = MApi_XC_R2BYTEMSK(REG_SC1_BK12_02_L, 0xFF);
        }
        else if (eWindow == PQ_SC2_MAIN_WINDOW)
        {
            u16MemFMT = MApi_XC_R2BYTEMSK(REG_SC2_BK12_02_L, 0xFF);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            u16MemFMT = MApi_XC_R2BYTEMSK(REG_SC2_BK12_42_L, 0xFF);
        }
    #endif // ENABLE_MULTI_SCALER

        if((u16MemFMT == 0xBB) || (u16MemFMT == 0xAA) || (u16MemFMT == 0x99) || (u16MemFMT == 0x88))
        {
                *pu8BitsPerPixel = 24;
        }
        else if((u16MemFMT == 0x55) || (u16MemFMT == 0x66) || (u16MemFMT == 0x44))
        {
                *pu8BitsPerPixel = 20;
        }
        else
        {
            //decide by original setting
        }
    }
    return TRUE;
}

MS_BOOL MDrv_PQ_SetFrameNumber(PQ_WIN eWindow, MS_U16 u16TabIdx_MADi, MS_U8 u8FrameCount)
{
    if(SUPPORT_SCMI_V2)
    {
        //Set Frame number
        if((bSetFrameCount )&& ((_stMode_Info[eWindow].bInterlace)
            ||(u16TabIdx_MADi == (Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MC_6Frame_6R)))
            ||(u16TabIdx_MADi == (Hal_PQ_get_madi_idx(eWindow, MS_MADI_P_MODE_MC_12Frame_8R)))))
        {
            if(eWindow == PQ_MAIN_WINDOW)
            {
                MApi_XC_W2BYTEMSK(REG_SC_BK12_19_L, u8FrameCount , 0x001F);
            }
        #if PQ_ENABLE_PIP
            else if(eWindow == PQ_SUB_WINDOW)
            {
                MApi_XC_W2BYTEMSK(REG_SC_BK12_59_L, u8FrameCount , 0x001F);
            }
        #endif
        #if ENABLE_MULTI_SCALER
            else if(eWindow == PQ_SC1_MAIN_WINDOW)
            {
                MApi_XC_W2BYTEMSK(REG_SC1_BK12_19_L, u8FrameCount , 0x001F);
            }
            else if(eWindow == PQ_SC2_MAIN_WINDOW)
            {
                MApi_XC_W2BYTEMSK(REG_SC2_BK12_19_L, u8FrameCount , 0x001F);
            }
            else if(eWindow == PQ_SC2_SUB_WINDOW)
            {
                MApi_XC_W2BYTEMSK(REG_SC2_BK12_59_L, u8FrameCount , 0x001F);
            }
        #endif // ENABLE_MULTI_SCALER

        }
        else
        {
            //When the frame mode from P_MODE_MC_6Frame_6R or P_MODE_MC_12Frame_8R transform to other  P_MODE , set the frame config to 3 frame.

            if(eWindow == PQ_MAIN_WINDOW
        #if PQ_ENABLE_PIP
               || eWindow == PQ_SUB_WINDOW
        #endif
              )
            {
                if ((MApi_XC_R2BYTEMSK(REG_SC_BK12_19_L , 0x001F ) == 0x0C)||(MApi_XC_R2BYTEMSK(REG_SC_BK12_19_L , 0x001F ) == 0x06))
                {
                    if (eWindow == PQ_MAIN_WINDOW)
                    {
                        MApi_XC_W2BYTEMSK(REG_SC_BK12_19_L, 0x03 , 0x001F );
                    }
                    else
                    {
                        MApi_XC_W2BYTEMSK(REG_SC_BK12_59_L, 0x03 , 0x001F );
                    }
                }
            }
        #if ENABLE_MULTI_SCALER
            else if(eWindow == PQ_SC2_MAIN_WINDOW || eWindow == PQ_SC2_SUB_WINDOW)
            {
                if ((MApi_XC_R2BYTEMSK(REG_SC2_BK12_19_L , 0x001F ) == 0x0C)||(MApi_XC_R2BYTEMSK(REG_SC2_BK12_19_L , 0x001F ) == 0x06))
                {
                    if (eWindow == PQ_MAIN_WINDOW)
                    {
                        MApi_XC_W2BYTEMSK(REG_SC2_BK12_19_L, 0x03 , 0x001F );
                    }
                    else
                    {
                        MApi_XC_W2BYTEMSK(REG_SC2_BK12_59_L, 0x03 , 0x001F );
                    }
                }
            }
            else
            {
                if ((MApi_XC_R2BYTEMSK(REG_SC1_BK12_19_L , 0x001F ) == 0x0C)||(MApi_XC_R2BYTEMSK(REG_SC1_BK12_19_L , 0x001F ) == 0x06))
                {
                    MApi_XC_W2BYTEMSK(REG_SC1_BK12_19_L, 0x03 , 0x001F );
                }
            }
        #endif
        }
    }
    return TRUE;
}

#if (PQ_ENABLE_PICTURE)
static MS_BOOL _MDrv_PQ_Get_Picture_3x3_Matrix(PQ_WIN eWindow, MS_U16 *pBuf)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret;
    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_3x3matrix_PIC2_Main);

    PQCOLOR_DBG(printf("3x3matrix:win%d TabIdx:%d \n",eWindow, u16TableIdx));

#if ENABLE_PQ_BIN
    if(gbPQBinEnable)
    {
        MS_U8 PQBinID = eWindow == PQ_MAIN_WINDOW     ? PQ_BIN_STD_MAIN :
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                                                        PQ_BIN_STD_SC1_MAIN;

        MS_U16 u16TabeSize =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_3x3matrix_PIC2_SIZE_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC2_Sub  :
                                                              PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_3x3matrix_PIC2_SIZE_Main;

        MS_U16 u16PQIPIdx =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_3x3matrix_PIC2_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_3x3matrix_PIC2_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_3x3matrix_PIC2_SC2_Sub  :
                                                              PQ_IP_SRAM_3x3matrix_PIC2_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_3x3matrix_PIC2_Main;

        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIPIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16TabeSize);


    }
    else
#endif
    {
        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_3x3matrix_PIC2_Main, (void *)pBuf, PQ_IP_SRAM_3x3matrix_PIC2_SIZE_Main);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_3x3matrix_PIC2_Sub, (void *)pBuf, PQ_IP_SRAM_3x3matrix_PIC2_SIZE_Sub);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_3x3matrix_PIC2_SC1_Main, (void *)pBuf, PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_3x3matrix_PIC2_SC2_Main, (void *)pBuf, PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC2_Main);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_3x3matrix_PIC2_SC2_Sub, (void *)pBuf, PQ_IP_SRAM_3x3matrix_PIC2_SIZE_SC2_Sub);
        }
    #endif
        else
        {
            bret = FALSE;
        }
    }
    return bret;
}


static MS_BOOL _MDrv_PQ_Get_Picture_DLC(PQ_WIN eWindow, MS_U16 *pBuf)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;

    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_DLC_PIC1_Main);

    PQCOLOR_DBG(printf("DLC Curve:win:%d TabIdx:%d \n",eWindow, u16TableIdx));

#if ENABLE_PQ_BIN
    if(gbPQBinEnable)
    {
        MS_U8 PQBinID = eWindow == PQ_MAIN_WINDOW     ? PQ_BIN_STD_MAIN :
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                                                        PQ_BIN_STD_SC1_MAIN;

        MS_U16 u16TabeSize =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_DLC_PIC1_SIZE_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_DLC_PIC1_SIZE_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_DLC_PIC1_SIZE_SC2_Sub  :
                                                              PQ_IP_SRAM_DLC_PIC1_SIZE_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_DLC_PIC1_SIZE_Main;


        MS_U16 u16PQIPIdx =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_DLC_PIC1_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_DLC_PIC1_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_DLC_PIC1_SC2_Sub  :
                                                              PQ_IP_SRAM_DLC_PIC1_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_DLC_PIC1_Main;


        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIPIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16TabeSize);


    }
    else
#endif
    {
        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_DLC_PIC1_Main, (void *)pBuf, PQ_IP_SRAM_DLC_PIC1_SIZE_Main);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_DLC_PIC1_Sub, (void *)pBuf, PQ_IP_SRAM_DLC_PIC1_SIZE_Sub);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_DLC_PIC1_SC1_Main, (void *)pBuf, PQ_IP_SRAM_DLC_PIC1_SIZE_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_DLC_PIC1_SC2_Main, (void *)pBuf, PQ_IP_SRAM_DLC_PIC1_SIZE_SC2_Main);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_DLC_PIC1_SC2_Sub, (void *)pBuf, PQ_IP_SRAM_DLC_PIC1_SIZE_SC2_Sub);
        }
    #endif
        else
        {
            bret = FALSE;
        }

    }

    return bret;
}

static MS_BOOL _MDrv_PQ_Get_Picture_ColorSetting(PQ_WIN eWindow, MS_U16 *pBuf)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;

    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_Main);
    PQCOLOR_DBG(printf("ColorSetting: win:%d, TabIdx:%d \n",eWindow, u16TableIdx));

#if ENABLE_PQ_BIN
    if(gbPQBinEnable)
    {
        MS_U8 PQBinID = eWindow == PQ_MAIN_WINDOW     ? PQ_BIN_STD_MAIN :
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                                                        PQ_BIN_STD_SC1_MAIN;

        MS_U16 u16TabeSize =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_Sub :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC2_Sub  :
                                                              PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_Main;

        MS_U16 u16PQIPIdx =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC2_Sub  :
                                                              PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_Main;

        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIPIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16TabeSize);


    }
    else
#endif
    {
        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_Main, (void *)pBuf, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_Main);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_Sub, (void *)pBuf, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_Sub);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC1_Main, (void *)pBuf, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC2_Main, (void *)pBuf, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC2_Main);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SC2_Sub, (void *)pBuf, PQ_IP_SRAM_Bri_Con_Hue_Sat_Sha_PIC1_SIZE_SC2_Sub);
        }
    #endif
        else
        {
            bret = FALSE;
        }
    }

    return bret;
}

static MS_BOOL _MDrv_PQ_Get_Picture_Gamma_TBL(PQ_WIN eWindow, MS_U16 *pBuf, PQ_PICTURE_TYPE enPictureType)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;
    MS_U16 u16PQIdx, u16Tabsize;

    u16TableIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_GammaTbl_R_PIC1_Main) :
                  enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_GammaTbl_G_PIC1_Main) :
                                                              MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_GammaTbl_B_PIC1_Main);

    PQCOLOR_DBG(printf("Gamma: TabIdx:%d \n", u16TableIdx));
    if(eWindow == PQ_MAIN_WINDOW)
    {
        u16PQIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_Main :
                   enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_Main :
                                                               PQ_IP_SRAM_GammaTbl_B_PIC1_Main;

        u16Tabsize = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SIZE_Main :
                     enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SIZE_Main :
                                                                 PQ_IP_SRAM_GammaTbl_B_PIC1_SIZE_Main;
    }
#if PQ_ENABLE_PIP
    else if(eWindow == PQ_SUB_WINDOW)
    {
        u16PQIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_Sub :
                   enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_Sub :
                                                               PQ_IP_SRAM_GammaTbl_B_PIC1_Sub;

        u16Tabsize = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SIZE_Sub :
                     enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SIZE_Sub :
                                                                 PQ_IP_SRAM_GammaTbl_B_PIC1_SIZE_Sub;
    }
#endif
#if ENABLE_MULTI_SCALER
    else if(eWindow == PQ_SC1_MAIN_WINDOW)
    {
        u16PQIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SC1_Main :
                   enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SC1_Main :
                                                               PQ_IP_SRAM_GammaTbl_B_PIC1_SC1_Main;

        u16Tabsize = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SIZE_SC1_Main :
                     enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SIZE_SC1_Main :
                                                                 PQ_IP_SRAM_GammaTbl_B_PIC1_SIZE_SC1_Main;
    }
    else if(eWindow == PQ_SC2_MAIN_WINDOW)
    {
        u16PQIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SC2_Main :
                   enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SC2_Main :
                                                               PQ_IP_SRAM_GammaTbl_B_PIC1_SC2_Main;

        u16Tabsize = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SIZE_SC2_Main :
                     enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SIZE_SC2_Main :
                                                                 PQ_IP_SRAM_GammaTbl_B_PIC1_SIZE_SC2_Main;
    }
    else if(eWindow == PQ_SC2_SUB_WINDOW)
    {
        u16PQIdx = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SC2_Sub :
                   enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SC2_Sub :
                                                               PQ_IP_SRAM_GammaTbl_B_PIC1_SC2_Sub;

        u16Tabsize = enPictureType == E_PQ_PICTURE_GAMMA_TBL_R ? PQ_IP_SRAM_GammaTbl_R_PIC1_SIZE_SC2_Sub :
                     enPictureType == E_PQ_PICTURE_GAMMA_TBL_G ? PQ_IP_SRAM_GammaTbl_G_PIC1_SIZE_SC2_Sub :
                                                                 PQ_IP_SRAM_GammaTbl_B_PIC1_SIZE_SC2_Sub;
    }
#endif
    else
    {
        bret = FALSE;
    }

    if(!bret)
    {
        return FALSE;
    }

#if ENABLE_PQ_BIN
    if(gbPQBinEnable)
    {
        MS_U8 PQBinID = eWindow == PQ_MAIN_WINDOW     ? PQ_BIN_STD_MAIN :
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                                                        PQ_BIN_STD_SC1_MAIN;

        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16Tabsize);


    }
    else
#endif
    {

        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, (MS_U8)u16PQIdx, (void *)pBuf, u16Tabsize);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, (MS_U8)u16PQIdx, (void *)pBuf, u16Tabsize);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, (MS_U8)u16PQIdx, (void *)pBuf, u16Tabsize);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, (MS_U8)u16PQIdx, (void *)pBuf, u16Tabsize);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, (MS_U8)u16PQIdx, (void *)pBuf, u16Tabsize);
        }
    #endif
        else
        {
            bret = FALSE;
        }

    }

    return bret;
}


static MS_BOOL _MDrv_PQ_Get_Picture_AutoColor(PQ_WIN eWindow, MS_U16 *pBuf)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;

    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_Auto_Color_PIC2_Main);
    PQCOLOR_DBG(printf("AutoColor: TabIdx:%d \n", u16TableIdx));

#if ENABLE_PQ_BIN
    if(gbPQBinEnable)
    {
        MS_U8 PQBinID =
            #if PQ_ENABLE_PIP
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
            #endif
            #if ENABLE_MULTI_SCALER
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_BIN_STD_SC1_MAIN :
            #endif
                                                        PQ_BIN_STD_MAIN;

        MS_U16 u16TabeSize =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Auto_Color_PIC2_SIZE_Sub :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC2_Sub  :
                              eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Auto_Color_PIC2_SIZE_Main;

        MS_U16 u16PQIPIdx =
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Auto_Color_PIC2_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Auto_Color_PIC2_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Auto_Color_PIC2_SC2_Sub  :
                              eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_SRAM_Auto_Color_PIC2_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Auto_Color_PIC2_Main;

        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIPIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16TabeSize);


    }
    else
#endif
    {

        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Auto_Color_PIC2_Main, (void *)pBuf, PQ_IP_SRAM_Auto_Color_PIC2_SIZE_Main);
        }
    #if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Auto_Color_PIC2_Sub, (void *)pBuf, PQ_IP_SRAM_Auto_Color_PIC2_SIZE_Sub);
        }
    #endif
    #if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Auto_Color_PIC2_SC1_Main, (void *)pBuf, PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Auto_Color_PIC2_SC2_Main, (void *)pBuf, PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC2_Main);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Auto_Color_PIC2_SC2_Sub, (void *)pBuf, PQ_IP_SRAM_Auto_Color_PIC2_SIZE_SC2_Sub);
        }
    #endif
        else
        {
            bret = FALSE;
        }

    }

    return bret;
}


static MS_BOOL _MDrv_PQ_Get_Picture_ColorTemp(PQ_WIN eWindow, MS_U16 *pBuf)
{
    MS_U16 u16TableIdx;
    MS_BOOL bret = TRUE;


    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, PQ_IP_SRAM_Color_Temp_PIC1_Main);
    PQCOLOR_DBG(printf("ColorTemp: TabIdx:%d \n", u16TableIdx));

#if(ENABLE_PQ_BIN)


    if(gbPQBinEnable)
    {

        MS_U8 PQBinID = eWindow == PQ_MAIN_WINDOW     ? PQ_BIN_STD_MAIN :
                #if PQ_ENABLE_PIP
                        eWindow == PQ_SUB_WINDOW      ? PQ_BIN_STD_SUB  :
                #endif
                #if ENABLE_MULTI_SCALER
                        eWindow == PQ_SC2_MAIN_WINDOW ? PQ_BIN_STD_SC2_MAIN :
                        eWindow == PQ_SC2_SUB_WINDOW  ? PQ_BIN_STD_SC2_SUB  :
                        eWindow == PQ_SC1_MAIN_WINDOW ? PQ_BIN_STD_SC1_MAIN :
                #endif
                                                        PQ_BIN_STD_MAIN;

        MS_U16 u16TabeSize =  eWindow == PQ_MAIN_WINDOW     ? PQ_IP_SRAM_Color_Temp_PIC1_SIZE_Main :
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Color_Temp_PIC1_SIZE_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC2_Sub  :
                              eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Color_Temp_PIC1_SIZE_Main;

        MS_U16 u16PQIPIdx =   eWindow == PQ_MAIN_WINDOW     ? PQ_IP_SRAM_Color_Temp_PIC1_Main :
                        #if PQ_ENABLE_PIP
                              eWindow == PQ_SUB_WINDOW      ? PQ_IP_SRAM_Color_Temp_PIC1_Sub  :
                        #endif
                        #if ENABLE_MULTI_SCALER
                              eWindow == PQ_SC2_MAIN_WINDOW ? PQ_IP_SRAM_Color_Temp_PIC1_SC2_Main :
                              eWindow == PQ_SC2_SUB_WINDOW  ? PQ_IP_SRAM_Color_Temp_PIC1_SC2_Sub  :
                              eWindow == PQ_SC1_MAIN_WINDOW ? PQ_IP_SRAM_Color_Temp_PIC1_SC1_Main :
                        #endif
                                                              PQ_IP_SRAM_Color_Temp_PIC1_Main;
        bret = MDrv_PQBin_LoadPictureSetting((MS_U16)MDrv_PQBin_GetPanelIdx(eWindow),
                                      u16TableIdx,
                                      u16PQIPIdx,
                                      &stPQBinHeaderInfo[PQBinID],
                                      (void *)pBuf,
                                      u16TabeSize);


    }
    else
#endif
    {

        if(eWindow == PQ_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Color_Temp_PIC1_Main, (void *)pBuf, PQ_IP_SRAM_Color_Temp_PIC1_SIZE_Main);
        }
#if PQ_ENABLE_PIP
        else if(eWindow == PQ_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Color_Temp_PIC1_Sub, (void *)pBuf, PQ_IP_SRAM_Color_Temp_PIC1_SIZE_Sub);
        }
#endif
#if ENABLE_MULTI_SCALER
        else if(eWindow == PQ_SC1_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC1_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Color_Temp_PIC1_SC1_Main, (void *)pBuf, PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC1_Main);
        }
        else if(eWindow == PQ_SC2_MAIN_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_MAIN, (MS_U8)u16TableIdx, PQ_IP_SRAM_Color_Temp_PIC1_SC2_Main, (void *)pBuf, PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC2_Main);
        }
        else if(eWindow == PQ_SC2_SUB_WINDOW)
        {
            bret = MDrv_PQ_LoadPictureSetting_(SC2_SUB, (MS_U8)u16TableIdx, PQ_IP_SRAM_Color_Temp_PIC1_SC2_Sub, (void *)pBuf, PQ_IP_SRAM_Color_Temp_PIC1_SIZE_SC2_Sub);
        }
#endif
        else
        {
            bret = FALSE;
        }
    }

    return bret;
}

MS_BOOL MDrv_PQ_Get_Sharpness(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType, MS_PQ_Mode_Info * pstPQModeInfo, MS_U8 *pu8Sharpness)
{
    MS_BOOL bret = TRUE;
    MS_U16 u16PQSrcType = 0;
    MS_PQ_Mode_Info stModeInfo;
    MS_U16 u16TableIdx;
    MS_U16 u16IPIdx;
    MS_U8 u8Peaking_Gain[1];


    if(enInputSourceType == PQ_INPUT_SOURCE_NONE)
    {
        u16PQSrcType = _u16PQSrcType[eWindow];
        _u16PQSrcType[eWindow] = QM_AV_PAL_M_Main;
    }
    else if(pstPQModeInfo && (enInputSourceType == PQ_INPUT_SOURCE_VGA || enInputSourceType == PQ_INPUT_SOURCE_YPBPR))
    {

        u16PQSrcType = _u16PQSrcType[eWindow];
        MsOS_Memcpy(&stModeInfo, &_stMode_Info[eWindow], sizeof(MS_PQ_Mode_Info));
        MDrv_PQ_Set_ModeInfo(eWindow, enInputSourceType, pstPQModeInfo);
        _u16PQSrcType[eWindow] = QM_InputSourceToIndex(eWindow, enInputSourceType);
    }

    PQCOLOR_DBG(printf("PQ_Get_Sharpness: win:%d  SRC:%d \n", eWindow, _u16PQSrcType[eWindow]));

    u16IPIdx = PQ_IP_VIP_Peaking_gain_Main;
    u16TableIdx = MDrv_PQ_GetTableIndex(eWindow, u16IPIdx);

#if(ENABLE_PQ_BIN)
    if(((gbPQBinEnable == 1) && (u16TableIdx == PQ_BIN_IP_NULL))
            || ((gbPQBinEnable == 0) && (u16TableIdx == PQ_IP_NULL)))
#else
    if(u16TableIdx == PQ_IP_NULL)
#endif
    {
        *pu8Sharpness = 0xFF;
        bret = FALSE;
    }
    else
    {
        MDrv_PQ_LoadTableData(eWindow, u16TableIdx, u16IPIdx, u8Peaking_Gain, 1);
        *pu8Sharpness = u8Peaking_Gain[0];

    }

    if(enInputSourceType == PQ_INPUT_SOURCE_NONE)
    {
        _u16PQSrcType[eWindow] = u16PQSrcType;
    }
    else if(pstPQModeInfo && (enInputSourceType == PQ_INPUT_SOURCE_VGA || enInputSourceType == PQ_INPUT_SOURCE_YPBPR))
    {
        MsOS_Memcpy(&_stMode_Info[eWindow], &stModeInfo, sizeof(MS_PQ_Mode_Info));
        _u16PQSrcType[eWindow] = u16PQSrcType;
    }

    return bret;
}


MS_BOOL MDrv_PQ_Get_Picture(PQ_WIN eWindow, PQ_INPUT_SOURCE_TYPE enInputSourceType, MS_PQ_Mode_Info * pstPQModeInfo, PQ_PICTURE_TYPE enType, void *pBuf)
{
    MS_BOOL bret = FALSE;
    MS_U16 u16PQSrcType = PQ_INPUT_SOURCE_NONE;
    MS_PQ_Mode_Info stModeInfo;


    if(enInputSourceType == PQ_INPUT_SOURCE_NONE)
    {
        u16PQSrcType = _u16PQSrcType[eWindow];
        _u16PQSrcType[eWindow] = QM_AV_PAL_M_Main;
    }
    else if(pstPQModeInfo && (enInputSourceType == PQ_INPUT_SOURCE_VGA || enInputSourceType == PQ_INPUT_SOURCE_YPBPR))
    {

        u16PQSrcType = _u16PQSrcType[eWindow];
        MsOS_Memcpy(&stModeInfo, &_stMode_Info[eWindow], sizeof(MS_PQ_Mode_Info));
        MDrv_PQ_Set_ModeInfo(eWindow, enInputSourceType, pstPQModeInfo);
        _u16PQSrcType[eWindow] = QM_InputSourceToIndex(eWindow, enInputSourceType);
    }


    PQCOLOR_DBG(printf("PQ_Get_Picture: Type:%d  SRC:%d \n", enType, _u16PQSrcType[eWindow]));

    switch(enType)
    {
    case E_PQ_PICTURE_3x3_MATRIX:
        bret = _MDrv_PQ_Get_Picture_3x3_Matrix(eWindow, pBuf);
        break;

    case E_PQ_PICTURE_DLC_CURVE:
        bret = _MDrv_PQ_Get_Picture_DLC(eWindow, pBuf);
        break;

    case E_PQ_PICTURE_COLOR_SETTING:
        bret = _MDrv_PQ_Get_Picture_ColorSetting(eWindow, pBuf);
        break;

    case E_PQ_PICTURE_GAMMA_TBL_R:
    case E_PQ_PICTURE_GAMMA_TBL_G:
    case E_PQ_PICTURE_GAMMA_TBL_B:
        bret = _MDrv_PQ_Get_Picture_Gamma_TBL(eWindow, pBuf, enType);
        break;

    case E_PQ_PICTURE_AUTO_COLOR:
        bret = _MDrv_PQ_Get_Picture_AutoColor(eWindow, pBuf);
        break;


    case E_PQ_PICTURE_COLOR_TEMP:
        bret = _MDrv_PQ_Get_Picture_ColorTemp(eWindow, pBuf);
        break;
    }

    if(enInputSourceType == PQ_INPUT_SOURCE_NONE)
    {
        _u16PQSrcType[eWindow] = u16PQSrcType;
    }
    else if(pstPQModeInfo && (enInputSourceType == PQ_INPUT_SOURCE_VGA || enInputSourceType == PQ_INPUT_SOURCE_YPBPR))
    {
        MsOS_Memcpy(&_stMode_Info[eWindow], &stModeInfo, sizeof(MS_PQ_Mode_Info));
        _u16PQSrcType[eWindow] = u16PQSrcType;
    }

    return bret;
}
#endif

#endif // PQ_ENABLE_UNUSED_FUNC
