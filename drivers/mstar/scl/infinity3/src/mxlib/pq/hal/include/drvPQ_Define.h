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

#ifndef _DRVPQ_DEFINE_H_
#define _DRVPQ_DEFINE_H_

#define PQ_ONLY_SUPPORT_BIN 0
#define PQ_ENABLE_DEBUG     0
#define PQ_ENABLE_CHECK     0

#define ENABLE_PQ_CMDQ          1
#define ENABLE_PQ_BIN           0
#define ENABLE_PQ_MLOAD         0
#define ENABLE_PQ_EX            0

#define ENABLE_MULTI_SCALER     0

#define PQ_ENABLE_420UPSAMPLING     0
#define PQ_ENABLE_NONLINEAR_SCALING 0
#define PQ_ENABLE_SETFILM_MODE      0
#define PQ_ENABLE_MEMFMT            0
#define PQ_ENABLE_CSCRULE           0
#define PQ_ENABLE_HSDRULE           0
#define PQ_ENABLE_PICTURE           0
#define PQ_ENABLE_UNUSED_FUNC       0

#define PQ_ENABLE_VD_SAMPLING   0
#define PQ_ENABLE_PIP           0
#define PQ_ENABLE_3D_VIDEO      1
#define PQ_ENABLE_IOCTL         1
#define PQ_ENABLE_COLOR         1

//!! for SC1 PIP setting !!//
#define PQ_ENABLE_PIP_SC1		0

#define PQ_QM_CVBS          0
#define PQ_QM_YPBPR         0
#define PQ_QM_PC            0
#define PQ_QM_HMDI          0
#define PQ_QM_HDMI_PC       0
#define PQ_QM_DTV           0
#define PQ_QM_MM_VIDEO      0
#define PQ_QM_MM_PHOTO      0
#define PQ_QM_ISP           1
#define PQ_QM_3D            0
#define PQ_EX_ENABLE        0
#define PQ_SKIPRULE_ENABLE  1

#define PQ_GRULE_NR_ENABLE                0
#define PQ_GRULE_MPEG_NR_ENABLE           0
#define PQ_GRULE_DDR_SELECT_ENABLE        0
#define PQ_GRULE_OSD_BW_ENABLE            0
#define PQ_GRULE_FILM_MODE_ENABLE         0
#define PQ_GRULE_DYNAMIC_CONTRAST_ENABLE  0
#define PQ_GRULE_ULTRAT_CLEAR_ENABLE      0
#define PQ_GRULE_PTP_ENABLE               0
#define PQ_XRULE_DB_ENABLE                0

#define ESTIMATE_AVAILABLE_RATE  85
#define PQ_ENABLE_FORCE_MADI     0
#define PQ_NEW_HSD_SAMPLING_TYPE 0
#define PQ_BW_H264_ENABLE        0
#define PQ_BW_MM_ENABLE          0
#define PQ_MADI_88X_MODE         0
#define PQ_UC_CTL                0
#define PQ_MADI_DFK              0
#define PQ_VIP_CTL               0
#define PQ_VIP_RGBCMY_CTL        0


#define PQ_EN_DMS_SW_CTRL	FALSE
#define PQ_ENABLE_RFBL		FALSE
#define PQ_EN_UCNR_OFF      FALSE

#define MAIN_WINDOW         0
#define SUB_WINDOW          1

extern MS_S32 _PQ_Mutex ;

#if defined (MSOS_TYPE_LINUX)
extern pthread_mutex_t _PQ_MLoad_Mutex;
#endif

#if PQ_ENABLE_DEBUG
extern MS_BOOL _u16DbgSwitch;
#define PQ_DBG(x)  \
do{                \
    if (_u16DbgSwitch){ \
        x;         \
    }              \
}while(0)

#define PQ_DUMP_DBG(x) x
#define PQ_DUMP_FILTER_DBG(x) x
#else
#define PQ_ERR(x) x
#define PQ_DBG(x)
#define PQ_DUMP_DBG(x)
#define PQ_DUMP_FILTER_DBG(x)   //x
#endif

#define PQ_MAP_REG(reg) (((reg)>>8)&0xFF), ((reg)&0xFF)
#define REG_ADDR_SIZE   2
#define REG_MASK_SIZE   1

#define PQ_IP_NULL          0xFF

#define PQTBL_EX            0x10
#define PQTBL_NORMAL        0x11

#define E_XRULE_HSD         0
#define E_XRULE_VSD         1
#define E_XRULE_HSP         2
#define E_XRULE_VSP         3
#define E_XRULE_CSC         4

#if PQ_XRULE_DB_ENABLE
#define E_XRULE_DB_NTSC     5
#define E_XRULE_DB_PAL      6
#define E_XRULE_NUM         7
#else
#define E_XRULE_NUM         5
#endif

//Here must be the same order as MST_GRule_Index_Main
typedef enum {
        E_GRULE_NR,
#if(PQ_GRULE_OSD_BW_ENABLE == 1)
        E_GRULE_OSD_BW,
#endif
#if(PQ_GRULE_MPEG_NR_ENABLE == 1)
        E_GRULE_MPEG_NR,
#endif
#if(PQ_GRULE_FILM_MODE_ENABLE == 1)
        E_GRULE_FilmMode,
#endif
#if(PQ_GRULE_DYNAMIC_CONTRAST_ENABLE == 1)
        E_GRULE_DynContr,
#endif
#if(PQ_GRULE_DDR_SELECT_ENABLE == 1)
        E_GRULE_DDR_SELECT,
#endif
#if(PQ_GRULE_ULTRAT_CLEAR_ENABLE == 1)
        E_GRULE_ULTRACLEAR,
#endif
#if(PQ_GRULE_PTP_ENABLE == 1)
        E_GRULE_PTP,
#endif
        E_GRULE_NUM,
} EN_GRULE_INDEX;


#define _END_OF_TBL_        0xFFFF


typedef enum {
    PQ_TABTYPE_GENERAL,
    PQ_TABTYPE_COMB,
    PQ_TABTYPE_SCALER,
    PQ_TABTYPE_SRAM1,
    PQ_TABTYPE_SRAM2,
    PQ_TABTYPE_SRAM3,
    PQ_TABTYPE_SRAM4,
    PQ_TABTYPE_C_SRAM1,
    PQ_TABTYPE_C_SRAM2,
    PQ_TABTYPE_C_SRAM3,
    PQ_TABTYPE_C_SRAM4,
    PQ_TABTYPE_SRAM_COLOR_INDEX,
    PQ_TABTYPE_SRAM_COLOR_GAIN_SNR,
    PQ_TABTYPE_SRAM_COLOR_GAIN_DNR,
    PQ_TABTYPE_VIP_IHC_CRD_SRAM,
    PQ_TABTYPE_VIP_ICC_CRD_SRAM,
    PQ_TABTYPE_XVYCC_DE_GAMMA_SRAM,
    PQ_TABTYPE_XVYCC_GAMMA_SRAM,
    PQ_TABTYPE_SWDRIVER,
    PQ_TABTYPE_PICTURE_1,
    PQ_TABTYPE_PICTURE_2,
    PQ_TABTYPE_WDR_Loc_TBL_0_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_1_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_2_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_3_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_4_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_5_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_6_SRAM,
    PQ_TABTYPE_WDR_Loc_TBL_7_SRAM,
    PQ_TABTYPE_PRIVACY_MASK_SRAM,
    PQ_TABTYPE_YUV_Gamma_tblU_SRAM,
    PQ_TABTYPE_YUV_Gamma_tblV_SRAM,
    PQ_TABTYPE_YUV_Gamma_tblY_SRAM,
    PQ_TABTYPE_ColorEng_GM10to12_Tbl_R_SRAM,
    PQ_TABTYPE_ColorEng_GM10to12_Tbl_G_SRAM,
    PQ_TABTYPE_ColorEng_GM10to12_Tbl_B_SRAM,
    PQ_TABTYPE_ColorEng_GM12to10_CrcTbl_R_SRAM,
    PQ_TABTYPE_ColorEng_GM12to10_CrcTbl_G_SRAM,
    PQ_TABTYPE_ColorEng_GM12to10_CrcTbl_B_SRAM,
    PQ_TABTYPE_Max,
} EN_PQ_TABTYPE;


typedef enum
{
    PQ_MD_720x480_60I,      // 00
    PQ_MD_720x480_60P,      // 01
    PQ_MD_720x576_50I,      // 02
    PQ_MD_720x576_50P,      // 03
    PQ_MD_1280x720_50P,     // 04
    PQ_MD_1280x720_60P,     // 05
    PQ_MD_1920x1080_50I,    // 06
    PQ_MD_1920x1080_60I,    // 07
    PQ_MD_1920x1080_24P,    // 08
    PQ_MD_1920x1080_25P,    // 09
    PQ_MD_1920x1080_30P,    // 10
    PQ_MD_1920x1080_50P,    // 11
    PQ_MD_1920x1080_60P,    // 12
    PQ_MD_Num,
}PQ_MODE_INDEX;

typedef enum
{
    PQ_CHECK_OFF  = 0,  // reset before test
    PQ_CHECK_REG  = 1,  // test register mode
    PQ_CHECK_SIZE = 2,  // test size mode
    PQ_CHECK_NUM,
}PQ_CHECK_TYPE;

typedef struct
{
    MS_U8 *pIPCommTable;
    MS_U8 *pIPTable;
    MS_U8 u8TabNums;
    MS_U8 u8TabType;
} EN_IPTAB_INFO;

typedef struct
{
    MS_U8 *pIPTable;
    MS_U8 u8TabNums;
    MS_U8 u8TabType;
    MS_U8 u8TabIdx;
} EN_IP_Info;


enum
{
    PQ_FUNC_DUMP_REG,
    PQ_FUNC_CHK_REG,
};

#define PQ_MUX_DEBUG    0

#define SCALER_REGISTER_SPREAD 1

#if(SCALER_REGISTER_SPREAD)
    #define BK_SCALER_BASE              0x120000

    // no need to store bank for spread reg
    #define SC_BK_STORE_NOMUTEX

    #define SC_BK_RESTORE_NOMUTEX


    #if(PQ_MUX_DEBUG)
    // no need to store bank for spread reg
    #define SC_BK_STORE_MUTEX     \
            if(!MsOS_ObtainMutex(_PQ_Mutex, MSOS_WAIT_FOREVER))                     \
            {                                                                        \
                printf("==========================\n");                              \
                printf("[%s][%s][%06d] Mutex taking timeout\n",__FILE__,__FUNCTION__,__LINE__);    \
            }

    // restore bank
    #define SC_BK_RESTORE_MUTEX   \
            MsOS_ReleaseMutex(_PQ_Mutex);
    #else
    #define SC_BK_STORE_MUTEX     \
            if(!MsOS_ObtainMutex(_PQ_Mutex, MSOS_WAIT_FOREVER))                     \
            {                                                                        \
            }

    // restore bank
    #define SC_BK_RESTORE_MUTEX   \
            MsOS_ReleaseMutex(_PQ_Mutex);
    #endif



    // switch bank
    #define SC_BK_SWITCH(_x_)

    #define SC_BK_CURRENT   (u8CurBank)
#else
    #define BK_SCALER_BASE              0x102F00

    #define SC_BK_STORE_NOMUTEX     \
            MS_U8 u8Bank;      \
            u8Bank = MApi_XC_ReadByte(BK_SCALER_BASE)

    // restore bank
    #define SC_BK_RESTORE_NOMUTEX   MApi_XC_WriteByte(BK_SCALER_BASE, u8Bank);

#if(PQ_MUX_DEBUG)
// store bank
    #define SC_BK_STORE_MUTEX     \
        MS_U8 u8Bank;      \
        if(!MsOS_ObtainMutex(_PQ_Mutex, MSOS_WAIT_FOREVER))                     \
        {                                                                        \
            printf("==========================\n");                              \
            printf("[%s][%s][%06d] Mutex taking timeout\n",__FILE__,__FUNCTION__,__LINE__);    \
        }        \
        u8Bank = MApi_XC_ReadByte(BK_SCALER_BASE)

// restore bank
    #define SC_BK_RESTORE_MUTEX   \
        MApi_XC_WriteByte(BK_SCALER_BASE, u8Bank); \
        MsOS_ReleaseMutex(_PQ_Mutex);
#else
    #define SC_BK_STORE_MUTEX     \
        MS_U8 u8Bank;      \
        if(!MsOS_ObtainMutex(_PQ_Mutex, MSOS_WAIT_FOREVER))                     \
        {                                                                        \
        }        \
        u8Bank = MApi_XC_ReadByte(BK_SCALER_BASE)

// restore bank
    #define SC_BK_RESTORE_MUTEX   \
        MApi_XC_WriteByte(BK_SCALER_BASE, u8Bank); \
        MsOS_ReleaseMutex(_PQ_Mutex);
#endif

// switch bank
#define SC_BK_SWITCH(_x_)\
        MApi_XC_WriteByte(BK_SCALER_BASE, _x_)

#define SC_BK_CURRENT   \
        MApi_XC_ReadByte(BK_SCALER_BASE)
#endif



// store bank
#define COMB_BK_STORE     \
            MS_U8 u8Bank;      \
            u8Bank = MApi_XC_ReadByte(COMB_REG_BASE)

// restore bank
#define COMB_BK_RESTORE   \
            MApi_XC_WriteByte(COMB_REG_BASE, u8Bank)
// switch bank
#define COMB_BK_SWITCH(_x_)\
            MApi_XC_WriteByte(COMB_REG_BASE, _x_)

#define COMB_BK_CURRENT   \
            MApi_XC_ReadByte(COMB_REG_BASE)



#define PQ_IP_COMM  0xfe
#define PQ_IP_ALL   0xff


typedef enum
{
    /// Main window
    PQ_MAIN_WINDOW=0,

    /// The max support window of PQ
    PQ_MAX_WINDOW
}PQ_WIN;


typedef struct {
  PQ_WIN eWin;
  MS_U8 u8PQ_InputType_Num;
  MS_U8 u8PQ_IP_Num;
  MS_U8 *pQuality_Map_Aray;
  EN_IPTAB_INFO *pIPTAB_Info;
  MS_U8 *pSkipRuleIP;

  MS_U8 u8PQ_XRule_IP_Num[E_XRULE_NUM];
  MS_U8 *pXRule_IP_Index[E_XRULE_NUM];
  MS_U8 *pXRule_Array[E_XRULE_NUM];

  MS_U8 u8PQ_GRule_Num[E_GRULE_NUM];
  MS_U8 u8PQ_GRule_IPNum[E_GRULE_NUM];
  MS_U8 u8PQ_GRule_LevelNum[E_GRULE_NUM];
  MS_U8 *pGRule_IP_Index[E_GRULE_NUM];
  MS_U8 *pGRule_Array[E_GRULE_NUM];
  MS_U8 *pGRule_Level[E_GRULE_NUM];
} PQTABLE_INFO;

/**
 *  FOURCE COLOR FMT
 */
typedef enum
{
    /// Default
    PQ_FOURCE_COLOR_DEFAULT,
    /// RGB
    PQ_FOURCE_COLOR_RGB,
    /// YUV
    PQ_FOURCE_COLOR_YUV,
}PQ_FOURCE_COLOR_FMT;

/**
 * PQ DEINTERLACE MODE
 */
typedef enum
{
    /// off
    PQ_DEINT_OFF=0,
    /// 2DDI BOB
    PQ_DEINT_2DDI_BOB,
    /// 2DDI AVG
    PQ_DEINT_2DDI_AVG,
    /// 3DDI History(24 bit)
    PQ_DEINT_3DDI_HISTORY,
    /// 3DDI(16 bit)
    PQ_DEINT_3DDI,
} PQ_DEINTERLACE_MODE;
/**
 *  Video data information to supply when in PQ mode.
 */
typedef struct
{
    /// is FBL or not
    MS_BOOL bFBL;
    /// is interlace mode or not
    MS_BOOL bInterlace;
    /// input Horizontal size
    MS_U16  u16input_hsize;
    /// input Vertical size
    MS_U16  u16input_vsize;
    /// input Vertical total
    MS_U16  u16input_vtotal;
    /// input Vertical frequency
    MS_U16  u16input_vfreq;
    /// output Vertical frequency
    MS_U16  u16ouput_vfreq;
    /// Display Horizontal size
    MS_U16  u16display_hsize;
    /// Display Vertical size
    MS_U16  u16display_vsize;
}MS_PQ_Mode_Info;

/**
 *  INPUT SOURCE TYPE
 */
typedef enum
{
    /// VGA
    PQ_INPUT_SOURCE_VGA,
    /// TV
    PQ_INPUT_SOURCE_TV,

    /// CVBS
    PQ_INPUT_SOURCE_CVBS,

    /// S-video
    PQ_INPUT_SOURCE_SVIDEO,

    /// Component
    PQ_INPUT_SOURCE_YPBPR,
    /// Scart
    PQ_INPUT_SOURCE_SCART,


    /// HDMI
    PQ_INPUT_SOURCE_HDMI,

    /// DTV
    PQ_INPUT_SOURCE_DTV,

    /// DVI
    PQ_INPUT_SOURCE_DVI,

    // Application source
    /// Storage
    PQ_INPUT_SOURCE_STORAGE,
    /// KTV
    PQ_INPUT_SOURCE_KTV,
    /// JPEG
    PQ_INPUT_SOURCE_JPEG,

    // BT656
    PQ_INPUT_SOURCE_BT656,

    // ISP
    PQ_INPUT_SOURCE_ISP,

    /// The max support number of PQ input source
    PQ_INPUT_SOURCE_NUM,
    /// None
    PQ_INPUT_SOURCE_NONE = PQ_INPUT_SOURCE_NUM,
} PQ_INPUT_SOURCE_TYPE;


// PQ Function
typedef enum
{
    E_PQ_IOCTL_NONE             = 0x00000000,
    E_PQ_IOCTL_HSD_SAMPLING     = 0x00000001,
    E_PQ_IOCTL_PREVSD_BILINEAR  = 0x00000002,
    E_PQ_IOCTL_ADC_SAMPLING     = 0x00000003,
    E_PQ_IOCTL_RFBL_CTRL        = 0x00000004,

    E_PQ_IOCTL_NUM,
}PQ_IOCTL_FLAG_TYPE;


typedef struct
{
    MS_U32 u32ratio;
    MS_BOOL bADVMode;
}PQ_HSD_SAMPLING_INFO;

typedef struct
{
    PQ_INPUT_SOURCE_TYPE enPQSourceType;
    MS_PQ_Mode_Info stPQModeInfo;
    MS_U16 u16ratio;
}PQ_ADC_SAMPLING_INFO;

typedef struct
{
    MS_U8 *pBuf;
    MS_U16 u16BufSize;
}PQ_DATA_INFO;

typedef struct
{
    MS_BOOL bEnFmCnt;
    MS_U8 u8FmCnt;
    MS_BOOL bfire;
}PQ_CMDQ_CONFIG;

#endif /* _DRVPQ_DEFINE_H_ */
