////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (; MStar; Confidential; Information; ) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
//****************************************************
//   Quality Map CodeGen Version 4.0
// 1920
// Infinity
// 8/12/2016  4:59:11 PM
// 39630.43853
//****************************************************

#ifndef _INFINITY_MAIN_H_
#define _INFINITY_MAIN_H_

#define PQ_IP_NUM_Main 115
#define PQ_IP_VIP_IHC_CRD_SRAM_SIZE_Main 2178
#define PQ_IP_VIP_ICC_CRD_SRAM_SIZE_Main 2178
#define PQ_IP_SRAM1_SIZE_Main 320
#define PQ_IP_SRAM2_SIZE_Main 320
#define PQ_IP_SRAM3_SIZE_Main 640
#define PQ_IP_SRAM4_SIZE_Main 640
#define PQ_IP_C_SRAM1_SIZE_Main 320
#define PQ_IP_C_SRAM2_SIZE_Main 320
#define PQ_IP_C_SRAM3_SIZE_Main 320
#define PQ_IP_C_SRAM4_SIZE_Main 320

typedef enum
{
    #if PQ_QM_ISP
    QM_FHD_YUV422_Main, //0
    #endif
    QM_INPUTTYPE_NUM_Main, // 1
} SC_QUALITY_MAP_INDEX_e_Main;

typedef enum
{
PQ_IP_PreSNR_Main,  //0
PQ_IP_DNR_Main,  //1
PQ_IP_DNR_Y_Main,  //2
PQ_IP_DNR_C_Main,  //3
PQ_IP_LDC_Main,  //4
PQ_IP_LDC_422_444_422_Main,  //5
PQ_IP_NLM_Main,  //6
PQ_IP_422to444_Main,  //7
PQ_IP_VIP_Main,  //8
PQ_IP_VIP_pseudo_Main,  //9
PQ_IP_VIP_LineBuffer_Main,  //10
PQ_IP_VIP_HLPF_Main,  //11
PQ_IP_VIP_HLPF_dither_Main,  //12
PQ_IP_VIP_VLPF_coef1_Main,  //13
PQ_IP_VIP_VLPF_coef2_Main,  //14
PQ_IP_VIP_VLPF_dither_Main,  //15
PQ_IP_VIP_Peaking_Main,  //16
PQ_IP_VIP_Peaking_band_Main,  //17
PQ_IP_VIP_Peaking_adptive_Main,  //18
PQ_IP_VIP_Peaking_Pcoring_Main,  //19
PQ_IP_VIP_Peaking_Pcoring_ad_Y_Main,  //20
PQ_IP_VIP_Peaking_gain_Main,  //21
PQ_IP_VIP_Peaking_gain_ad_Y_Main,  //22
PQ_IP_VIP_LCE_Main,  //23
PQ_IP_VIP_LCE_dither_Main,  //24
PQ_IP_VIP_LCE_setting_Main,  //25
PQ_IP_VIP_LCE_curve_Main,  //26
PQ_IP_VIP_DLC_His_range_Main,  //27
PQ_IP_VIP_DLC_Main,  //28
PQ_IP_VIP_DLC_dither_Main,  //29
PQ_IP_VIP_DLC_His_rangeH_Main,  //30
PQ_IP_VIP_DLC_His_rangeV_Main,  //31
PQ_IP_VIP_DLC_PC_Main,  //32
PQ_IP_VIP_YC_gain_offset_Main,  //33
PQ_IP_VIP_UVC_Main,  //34
PQ_IP_VIP_FCC_full_range_Main,  //35
PQ_IP_VIP_FCC_bdry_dist_Main,  //36
PQ_IP_VIP_FCC_T1_Main,  //37
PQ_IP_VIP_FCC_T2_Main,  //38
PQ_IP_VIP_FCC_T3_Main,  //39
PQ_IP_VIP_FCC_T4_Main,  //40
PQ_IP_VIP_FCC_T5_Main,  //41
PQ_IP_VIP_FCC_T6_Main,  //42
PQ_IP_VIP_FCC_T7_Main,  //43
PQ_IP_VIP_FCC_T8_Main,  //44
PQ_IP_VIP_FCC_T9_Main,  //45
PQ_IP_VIP_IHC_Main,  //46
PQ_IP_VIP_IHC_Ymode_Main,  //47
PQ_IP_VIP_IHC_dither_Main,  //48
PQ_IP_VIP_IHC_CRD_SRAM_Main,  //49
PQ_IP_VIP_IHC_SETTING_Main,  //50
PQ_IP_VIP_ICC_Main,  //51
PQ_IP_VIP_ICC_Ymode_Main,  //52
PQ_IP_VIP_ICC_dither_Main,  //53
PQ_IP_VIP_ICC_CRD_SRAM_Main,  //54
PQ_IP_VIP_ICC_SETTING_Main,  //55
PQ_IP_VIP_Ymode_Yvalue_ALL_Main,  //56
PQ_IP_VIP_Ymode_Yvalue_SETTING_Main,  //57
PQ_IP_VIP_IBC_Main,  //58
PQ_IP_VIP_IBC_dither_Main,  //59
PQ_IP_VIP_IBC_SETTING_Main,  //60
PQ_IP_VIP_ACK_Main,  //61
PQ_IP_VIP_YCbCr_Clip_Main,  //62
PQ_IP_VSP_Y_SC1_Main,  //63
PQ_IP_VSP_C_SC1_Main,  //64
PQ_IP_VSP_CoRing_SC1_Main,  //65
PQ_IP_VSP_DeRing_SC1_Main,  //66
PQ_IP_VSP_Dither_SC1_Main,  //67
PQ_IP_VSP_PreVBound_SC1_Main,  //68
PQ_IP_AntiPAL_filter_SC1_Main,  //69
PQ_IP_422To444_SC1_Main,  //70
PQ_IP_HSP_Y_SC1_Main,  //71
PQ_IP_HSP_C_SC1_Main,  //72
PQ_IP_HSP_CoRing_SC1_Main,  //73
PQ_IP_HSP_DeRing_SC1_Main,  //74
PQ_IP_HSP_Dither_SC1_Main,  //75
PQ_IP_HnonLinear_SC1_Main,  //76
PQ_IP_SRAM1_Main,  //77
PQ_IP_SRAM2_Main,  //78
PQ_IP_SRAM3_Main,  //79
PQ_IP_SRAM4_Main,  //80
PQ_IP_C_SRAM1_Main,  //81
PQ_IP_C_SRAM2_Main,  //82
PQ_IP_C_SRAM3_Main,  //83
PQ_IP_C_SRAM4_Main,  //84
PQ_IP_VSP_Y_SC2_Main,  //85
PQ_IP_VSP_C_SC2_Main,  //86
PQ_IP_VSP_CoRing_SC2_Main,  //87
PQ_IP_VSP_DeRing_SC2_Main,  //88
PQ_IP_VSP_Dither_SC2_Main,  //89
PQ_IP_VSP_PreVBound_SC2_Main,  //90
PQ_IP_AntiPAL_filter_SC2_Main,  //91
PQ_IP_422To444_SC2_Main,  //92
PQ_IP_HSP_Y_SC2_Main,  //93
PQ_IP_HSP_C_SC2_Main,  //94
PQ_IP_HSP_CoRing_SC2_Main,  //95
PQ_IP_HSP_DeRing_SC2_Main,  //96
PQ_IP_HSP_Dither_SC2_Main,  //97
PQ_IP_HnonLinear_SC2_Main,  //98
PQ_IP_VSP_Y_SC3_Main,  //99
PQ_IP_VSP_C_SC3_Main,  //100
PQ_IP_VSP_CoRing_SC3_Main,  //101
PQ_IP_VSP_DeRing_SC3_Main,  //102
PQ_IP_VSP_Dither_SC3_Main,  //103
PQ_IP_VSP_PreVBound_SC3_Main,  //104
PQ_IP_AntiPAL_filter_SC3_Main,  //105
PQ_IP_422To444_SC3_Main,  //106
PQ_IP_HSP_Y_SC3_Main,  //107
PQ_IP_HSP_C_SC3_Main,  //108
PQ_IP_HSP_CoRing_SC3_Main,  //109
PQ_IP_HSP_DeRing_SC3_Main,  //110
PQ_IP_HSP_Dither_SC3_Main,  //111
PQ_IP_HnonLinear_SC3_Main,  //112
PQ_IP_SWDriver_Main,  //113
PQ_IP_SC_End_Main,  //114
 }   PQ_IPTYPE_Main;

typedef enum
{
PQ_IP_PreSNR_COM_Main,  //0
PQ_IP_DNR_COM_Main,  //1
PQ_IP_DNR_Y_COM_Main,  //2
PQ_IP_DNR_C_COM_Main,  //3
PQ_IP_LDC_COM_Main,  //4
PQ_IP_LDC_422_444_422_COM_Main,  //5
PQ_IP_NLM_COM_Main,  //6
PQ_IP_422to444_COM_Main,  //7
PQ_IP_VIP_COM_Main,  //8
PQ_IP_VIP_pseudo_COM_Main,  //9
PQ_IP_VIP_LineBuffer_COM_Main,  //10
PQ_IP_VIP_HLPF_COM_Main,  //11
PQ_IP_VIP_HLPF_dither_COM_Main,  //12
PQ_IP_VIP_VLPF_coef1_COM_Main,  //13
PQ_IP_VIP_VLPF_coef2_COM_Main,  //14
PQ_IP_VIP_VLPF_dither_COM_Main,  //15
PQ_IP_VIP_Peaking_COM_Main,  //16
PQ_IP_VIP_Peaking_band_COM_Main,  //17
PQ_IP_VIP_Peaking_adptive_COM_Main,  //18
PQ_IP_VIP_Peaking_Pcoring_COM_Main,  //19
PQ_IP_VIP_Peaking_Pcoring_ad_Y_COM_Main,  //20
PQ_IP_VIP_Peaking_gain_COM_Main,  //21
PQ_IP_VIP_Peaking_gain_ad_Y_COM_Main,  //22
PQ_IP_VIP_LCE_COM_Main,  //23
PQ_IP_VIP_LCE_dither_COM_Main,  //24
PQ_IP_VIP_LCE_setting_COM_Main,  //25
PQ_IP_VIP_LCE_curve_COM_Main,  //26
PQ_IP_VIP_DLC_His_range_COM_Main,  //27
PQ_IP_VIP_DLC_COM_Main,  //28
PQ_IP_VIP_DLC_dither_COM_Main,  //29
PQ_IP_VIP_DLC_His_rangeH_COM_Main,  //30
PQ_IP_VIP_DLC_His_rangeV_COM_Main,  //31
PQ_IP_VIP_DLC_PC_COM_Main,  //32
PQ_IP_VIP_YC_gain_offset_COM_Main,  //33
PQ_IP_VIP_UVC_COM_Main,  //34
PQ_IP_VIP_FCC_full_range_COM_Main,  //35
PQ_IP_VIP_FCC_bdry_dist_COM_Main,  //36
PQ_IP_VIP_FCC_T1_COM_Main,  //37
PQ_IP_VIP_FCC_T2_COM_Main,  //38
PQ_IP_VIP_FCC_T3_COM_Main,  //39
PQ_IP_VIP_FCC_T4_COM_Main,  //40
PQ_IP_VIP_FCC_T5_COM_Main,  //41
PQ_IP_VIP_FCC_T6_COM_Main,  //42
PQ_IP_VIP_FCC_T7_COM_Main,  //43
PQ_IP_VIP_FCC_T8_COM_Main,  //44
PQ_IP_VIP_FCC_T9_COM_Main,  //45
PQ_IP_VIP_IHC_COM_Main,  //46
PQ_IP_VIP_IHC_Ymode_COM_Main,  //47
PQ_IP_VIP_IHC_dither_COM_Main,  //48
PQ_IP_VIP_IHC_CRD_SRAM_COM_Main,  //49
PQ_IP_VIP_IHC_SETTING_COM_Main,  //50
PQ_IP_VIP_ICC_COM_Main,  //51
PQ_IP_VIP_ICC_Ymode_COM_Main,  //52
PQ_IP_VIP_ICC_dither_COM_Main,  //53
PQ_IP_VIP_ICC_CRD_SRAM_COM_Main,  //54
PQ_IP_VIP_ICC_SETTING_COM_Main,  //55
PQ_IP_VIP_Ymode_Yvalue_ALL_COM_Main,  //56
PQ_IP_VIP_Ymode_Yvalue_SETTING_COM_Main,  //57
PQ_IP_VIP_IBC_COM_Main,  //58
PQ_IP_VIP_IBC_dither_COM_Main,  //59
PQ_IP_VIP_IBC_SETTING_COM_Main,  //60
PQ_IP_VIP_ACK_COM_Main,  //61
PQ_IP_VIP_YCbCr_Clip_COM_Main,  //62
PQ_IP_VSP_Y_SC1_COM_Main,  //63
PQ_IP_VSP_C_SC1_COM_Main,  //64
PQ_IP_VSP_CoRing_SC1_COM_Main,  //65
PQ_IP_VSP_DeRing_SC1_COM_Main,  //66
PQ_IP_VSP_Dither_SC1_COM_Main,  //67
PQ_IP_VSP_PreVBound_SC1_COM_Main,  //68
PQ_IP_AntiPAL_filter_SC1_COM_Main,  //69
PQ_IP_422To444_SC1_COM_Main,  //70
PQ_IP_HSP_Y_SC1_COM_Main,  //71
PQ_IP_HSP_C_SC1_COM_Main,  //72
PQ_IP_HSP_CoRing_SC1_COM_Main,  //73
PQ_IP_HSP_DeRing_SC1_COM_Main,  //74
PQ_IP_HSP_Dither_SC1_COM_Main,  //75
PQ_IP_HnonLinear_SC1_COM_Main,  //76
PQ_IP_SRAM1_COM_Main,  //77
PQ_IP_SRAM2_COM_Main,  //78
PQ_IP_SRAM3_COM_Main,  //79
PQ_IP_SRAM4_COM_Main,  //80
PQ_IP_C_SRAM1_COM_Main,  //81
PQ_IP_C_SRAM2_COM_Main,  //82
PQ_IP_C_SRAM3_COM_Main,  //83
PQ_IP_C_SRAM4_COM_Main,  //84
PQ_IP_VSP_Y_SC2_COM_Main,  //85
PQ_IP_VSP_C_SC2_COM_Main,  //86
PQ_IP_VSP_CoRing_SC2_COM_Main,  //87
PQ_IP_VSP_DeRing_SC2_COM_Main,  //88
PQ_IP_VSP_Dither_SC2_COM_Main,  //89
PQ_IP_VSP_PreVBound_SC2_COM_Main,  //90
PQ_IP_AntiPAL_filter_SC2_COM_Main,  //91
PQ_IP_422To444_SC2_COM_Main,  //92
PQ_IP_HSP_Y_SC2_COM_Main,  //93
PQ_IP_HSP_C_SC2_COM_Main,  //94
PQ_IP_HSP_CoRing_SC2_COM_Main,  //95
PQ_IP_HSP_DeRing_SC2_COM_Main,  //96
PQ_IP_HSP_Dither_SC2_COM_Main,  //97
PQ_IP_HnonLinear_SC2_COM_Main,  //98
PQ_IP_VSP_Y_SC3_COM_Main,  //99
PQ_IP_VSP_C_SC3_COM_Main,  //100
PQ_IP_VSP_CoRing_SC3_COM_Main,  //101
PQ_IP_VSP_DeRing_SC3_COM_Main,  //102
PQ_IP_VSP_Dither_SC3_COM_Main,  //103
PQ_IP_VSP_PreVBound_SC3_COM_Main,  //104
PQ_IP_AntiPAL_filter_SC3_COM_Main,  //105
PQ_IP_422To444_SC3_COM_Main,  //106
PQ_IP_HSP_Y_SC3_COM_Main,  //107
PQ_IP_HSP_C_SC3_COM_Main,  //108
PQ_IP_HSP_CoRing_SC3_COM_Main,  //109
PQ_IP_HSP_DeRing_SC3_COM_Main,  //110
PQ_IP_HSP_Dither_SC3_COM_Main,  //111
PQ_IP_HnonLinear_SC3_COM_Main,  //112
PQ_IP_SWDriver_COM_Main,  //113
PQ_IP_SC_End_COM_Main,  //114
PQ_IP_COM_NUMS_Main
} PQ_IP_COM_Group_Main;

typedef enum
{
PQ_IP_PreSNR_OFF_Main,
PQ_IP_PreSNR_NUMS_Main
} PQ_IP_PreSNR_Group_Main;

typedef enum
{
PQ_IP_DNR_NR_OFF_Main,
PQ_IP_DNR_DNR_OFF_Main,
PQ_IP_DNR_NUMS_Main
} PQ_IP_DNR_Group_Main;

typedef enum
{
PQ_IP_DNR_Y_OFF_Main,
PQ_IP_DNR_Y_NUMS_Main
} PQ_IP_DNR_Y_Group_Main;

typedef enum
{
PQ_IP_DNR_C_OFF_Main,
PQ_IP_DNR_C_NUMS_Main
} PQ_IP_DNR_C_Group_Main;

typedef enum
{
PQ_IP_LDC_OFF_Main,
PQ_IP_LDC_ON_Main,
PQ_IP_LDC_NUMS_Main
} PQ_IP_LDC_Group_Main;

typedef enum
{
PQ_IP_LDC_422_444_422_md1434_avg_Main,
PQ_IP_LDC_422_444_422_NUMS_Main
} PQ_IP_LDC_422_444_422_Group_Main;

typedef enum
{
PQ_IP_NLM_OFF_Main,
PQ_IP_NLM_NUMS_Main
} PQ_IP_NLM_Group_Main;

typedef enum
{
PQ_IP_422to444_OFF_Main,
PQ_IP_422to444_NUMS_Main
} PQ_IP_422to444_Group_Main;

typedef enum
{
PQ_IP_VIP_OFF_Main,
PQ_IP_VIP_NUMS_Main
} PQ_IP_VIP_Group_Main;

typedef enum
{
PQ_IP_VIP_pseudo_OFF_Main,
PQ_IP_VIP_pseudo_NUMS_Main
} PQ_IP_VIP_pseudo_Group_Main;

typedef enum
{
PQ_IP_VIP_LineBuffer_OFF_Main,
PQ_IP_VIP_LineBuffer_NUMS_Main
} PQ_IP_VIP_LineBuffer_Group_Main;

typedef enum
{
PQ_IP_VIP_HLPF_OFF_Main,
PQ_IP_VIP_HLPF_NUMS_Main
} PQ_IP_VIP_HLPF_Group_Main;

typedef enum
{
PQ_IP_VIP_HLPF_dither_OFF_Main,
PQ_IP_VIP_HLPF_dither_NUMS_Main
} PQ_IP_VIP_HLPF_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_VLPF_coef1_OFF_Main,
PQ_IP_VIP_VLPF_coef1_NUMS_Main
} PQ_IP_VIP_VLPF_coef1_Group_Main;

typedef enum
{
PQ_IP_VIP_VLPF_coef2_OFF_Main,
PQ_IP_VIP_VLPF_coef2_NUMS_Main
} PQ_IP_VIP_VLPF_coef2_Group_Main;

typedef enum
{
PQ_IP_VIP_VLPF_dither_OFF_Main,
PQ_IP_VIP_VLPF_dither_NUMS_Main
} PQ_IP_VIP_VLPF_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_OFF_Main,
PQ_IP_VIP_Peaking_W2_Main,
PQ_IP_VIP_Peaking_NUMS_Main
} PQ_IP_VIP_Peaking_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_band_OFF_Main,
PQ_IP_VIP_Peaking_band_NUMS_Main
} PQ_IP_VIP_Peaking_band_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_adptive_OFF_Main,
PQ_IP_VIP_Peaking_adptive_NUMS_Main
} PQ_IP_VIP_Peaking_adptive_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_Pcoring_OFF_Main,
PQ_IP_VIP_Peaking_Pcoring_NUMS_Main
} PQ_IP_VIP_Peaking_Pcoring_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_Pcoring_ad_Y_OFF_Main,
PQ_IP_VIP_Peaking_Pcoring_ad_Y_NUMS_Main
} PQ_IP_VIP_Peaking_Pcoring_ad_Y_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_gain_0x10_Main,
PQ_IP_VIP_Peaking_gain_NUMS_Main
} PQ_IP_VIP_Peaking_gain_Group_Main;

typedef enum
{
PQ_IP_VIP_Peaking_gain_ad_Y_OFF_Main,
PQ_IP_VIP_Peaking_gain_ad_Y_NUMS_Main
} PQ_IP_VIP_Peaking_gain_ad_Y_Group_Main;

typedef enum
{
PQ_IP_VIP_YC_gain_offset_OFF_Main,
PQ_IP_VIP_YC_gain_offset_NUMS_Main
} PQ_IP_VIP_YC_gain_offset_Group_Main;

typedef enum
{
PQ_IP_VIP_LCE_OFF_Main,
PQ_IP_VIP_LCE_NUMS_Main
} PQ_IP_VIP_LCE_Group_Main;

typedef enum
{
PQ_IP_VIP_LCE_dither_OFF_Main,
PQ_IP_VIP_LCE_dither_NUMS_Main
} PQ_IP_VIP_LCE_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_LCE_setting_S3_Main,
PQ_IP_VIP_LCE_setting_NUMS_Main
} PQ_IP_VIP_LCE_setting_Group_Main;

typedef enum
{
PQ_IP_VIP_LCE_curve_CV1_Main,
PQ_IP_VIP_LCE_curve_NUMS_Main
} PQ_IP_VIP_LCE_curve_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_OFF_Main,
PQ_IP_VIP_DLC_NUMS_Main
} PQ_IP_VIP_DLC_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_dither_OFF_Main,
PQ_IP_VIP_DLC_dither_NUMS_Main
} PQ_IP_VIP_DLC_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_His_range_OFF_Main,
PQ_IP_VIP_DLC_His_range_NUMS_Main
} PQ_IP_VIP_DLC_His_range_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_His_rangeH_90pa_1920_Main,
PQ_IP_VIP_DLC_His_rangeH_NUMS_Main
} PQ_IP_VIP_DLC_His_rangeH_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_His_rangeV_90pa_1920_Main,
PQ_IP_VIP_DLC_His_rangeV_NUMS_Main
} PQ_IP_VIP_DLC_His_rangeV_Group_Main;

typedef enum
{
PQ_IP_VIP_DLC_PC_OFF_Main,
PQ_IP_VIP_DLC_PC_NUMS_Main
} PQ_IP_VIP_DLC_PC_Group_Main;

typedef enum
{
PQ_IP_VIP_UVC_OFF_Main,
PQ_IP_VIP_UVC_NUMS_Main
} PQ_IP_VIP_UVC_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_full_range_OFF_Main,
PQ_IP_VIP_FCC_full_range_NUMS_Main
} PQ_IP_VIP_FCC_full_range_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_bdry_dist_OFF_Main,
PQ_IP_VIP_FCC_bdry_dist_NUMS_Main
} PQ_IP_VIP_FCC_bdry_dist_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T1_OFF_Main,
PQ_IP_VIP_FCC_T1_NUMS_Main
} PQ_IP_VIP_FCC_T1_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T2_OFF_Main,
PQ_IP_VIP_FCC_T2_NUMS_Main
} PQ_IP_VIP_FCC_T2_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T3_OFF_Main,
PQ_IP_VIP_FCC_T3_NUMS_Main
} PQ_IP_VIP_FCC_T3_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T4_OFF_Main,
PQ_IP_VIP_FCC_T4_NUMS_Main
} PQ_IP_VIP_FCC_T4_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T5_OFF_Main,
PQ_IP_VIP_FCC_T5_NUMS_Main
} PQ_IP_VIP_FCC_T5_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T6_OFF_Main,
PQ_IP_VIP_FCC_T6_NUMS_Main
} PQ_IP_VIP_FCC_T6_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T7_OFF_Main,
PQ_IP_VIP_FCC_T7_NUMS_Main
} PQ_IP_VIP_FCC_T7_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T8_OFF_Main,
PQ_IP_VIP_FCC_T8_NUMS_Main
} PQ_IP_VIP_FCC_T8_Group_Main;

typedef enum
{
PQ_IP_VIP_FCC_T9_OFF_Main,
PQ_IP_VIP_FCC_T9_NUMS_Main
} PQ_IP_VIP_FCC_T9_Group_Main;

typedef enum
{
PQ_IP_VIP_IHC_OFF_Main,
PQ_IP_VIP_IHC_NUMS_Main
} PQ_IP_VIP_IHC_Group_Main;

typedef enum
{
PQ_IP_VIP_IHC_Ymode_OFF_Main,
PQ_IP_VIP_IHC_Ymode_NUMS_Main
} PQ_IP_VIP_IHC_Ymode_Group_Main;

typedef enum
{
PQ_IP_VIP_IHC_dither_OFF_Main,
PQ_IP_VIP_IHC_dither_NUMS_Main
} PQ_IP_VIP_IHC_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_IHC_CRD_SRAM_15wins3_Main,
PQ_IP_VIP_IHC_CRD_SRAM_NUMS_Main
} PQ_IP_VIP_IHC_CRD_SRAM_Group_Main;

typedef enum
{
PQ_IP_VIP_IHC_SETTING_HDMI_HD_Main,
PQ_IP_VIP_IHC_SETTING_NUMS_Main
} PQ_IP_VIP_IHC_SETTING_Group_Main;

typedef enum
{
PQ_IP_VIP_ICC_OFF_Main,
PQ_IP_VIP_ICC_NUMS_Main
} PQ_IP_VIP_ICC_Group_Main;

typedef enum
{
PQ_IP_VIP_ICC_Ymode_HDMI_HD_Main,
PQ_IP_VIP_ICC_Ymode_NUMS_Main
} PQ_IP_VIP_ICC_Ymode_Group_Main;

typedef enum
{
PQ_IP_VIP_ICC_dither_OFF_Main,
PQ_IP_VIP_ICC_dither_NUMS_Main
} PQ_IP_VIP_ICC_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_ICC_CRD_SRAM_15wins3_Main,
PQ_IP_VIP_ICC_CRD_SRAM_NUMS_Main
} PQ_IP_VIP_ICC_CRD_SRAM_Group_Main;

typedef enum
{
PQ_IP_VIP_ICC_SETTING_HDMI_HD_Main,
PQ_IP_VIP_ICC_SETTING_NUMS_Main
} PQ_IP_VIP_ICC_SETTING_Group_Main;

typedef enum
{
PQ_IP_VIP_Ymode_Yvalue_ALL_Y1_Main,
PQ_IP_VIP_Ymode_Yvalue_ALL_NUMS_Main
} PQ_IP_VIP_Ymode_Yvalue_ALL_Group_Main;

typedef enum
{
PQ_IP_VIP_Ymode_Yvalue_SETTING_Y2_Main,
PQ_IP_VIP_Ymode_Yvalue_SETTING_NUMS_Main
} PQ_IP_VIP_Ymode_Yvalue_SETTING_Group_Main;

typedef enum
{
PQ_IP_VIP_IBC_OFF_Main,
PQ_IP_VIP_IBC_NUMS_Main
} PQ_IP_VIP_IBC_Group_Main;

typedef enum
{
PQ_IP_VIP_IBC_dither_OFF_Main,
PQ_IP_VIP_IBC_dither_NUMS_Main
} PQ_IP_VIP_IBC_dither_Group_Main;

typedef enum
{
PQ_IP_VIP_IBC_SETTING_OFF_Main,
PQ_IP_VIP_IBC_SETTING_NUMS_Main
} PQ_IP_VIP_IBC_SETTING_Group_Main;

typedef enum
{
PQ_IP_VIP_ACK_OFF_Main,
PQ_IP_VIP_ACK_NUMS_Main
} PQ_IP_VIP_ACK_Group_Main;

typedef enum
{
PQ_IP_VIP_YCbCr_Clip_OFF_Main,
PQ_IP_VIP_YCbCr_Clip_NUMS_Main
} PQ_IP_VIP_YCbCr_Clip_Group_Main;

typedef enum
{
PQ_IP_VSP_Y_SC1_Bypass_Main,
PQ_IP_VSP_Y_SC1_NUMS_Main
} PQ_IP_VSP_Y_SC1_Group_Main;

typedef enum
{
PQ_IP_VSP_C_SC1_Bypass_Main,
PQ_IP_VSP_C_SC1_NUMS_Main
} PQ_IP_VSP_C_SC1_Group_Main;

typedef enum
{
PQ_IP_VSP_CoRing_SC1_OFF_Main,
PQ_IP_VSP_CoRing_SC1_NUMS_Main
} PQ_IP_VSP_CoRing_SC1_Group_Main;

typedef enum
{
PQ_IP_VSP_DeRing_SC1_OFF_Main,
PQ_IP_VSP_DeRing_SC1_NUMS_Main
} PQ_IP_VSP_DeRing_SC1_Group_Main;

typedef enum
{
PQ_IP_VSP_Dither_SC1_OFF_Main,
PQ_IP_VSP_Dither_SC1_NUMS_Main
} PQ_IP_VSP_Dither_SC1_Group_Main;

typedef enum
{
PQ_IP_VSP_PreVBound_SC1_OFF_Main,
PQ_IP_VSP_PreVBound_SC1_NUMS_Main
} PQ_IP_VSP_PreVBound_SC1_Group_Main;

typedef enum
{
PQ_IP_AntiPAL_filter_SC1_OFF_Main,
PQ_IP_AntiPAL_filter_SC1_NUMS_Main
} PQ_IP_AntiPAL_filter_SC1_Group_Main;

typedef enum
{
PQ_IP_422To444_SC1_ON_Main,
PQ_IP_422To444_SC1_NUMS_Main
} PQ_IP_422To444_SC1_Group_Main;

typedef enum
{
PQ_IP_HSP_Y_SC1_Bypass_Main,
PQ_IP_HSP_Y_SC1_NUMS_Main
} PQ_IP_HSP_Y_SC1_Group_Main;

typedef enum
{
PQ_IP_HSP_C_SC1_Bypass_Main,
PQ_IP_HSP_C_SC1_NUMS_Main
} PQ_IP_HSP_C_SC1_Group_Main;

typedef enum
{
PQ_IP_HSP_CoRing_SC1_OFF_Main,
PQ_IP_HSP_CoRing_SC1_NUMS_Main
} PQ_IP_HSP_CoRing_SC1_Group_Main;

typedef enum
{
PQ_IP_HSP_DeRing_SC1_OFF_Main,
PQ_IP_HSP_DeRing_SC1_NUMS_Main
} PQ_IP_HSP_DeRing_SC1_Group_Main;

typedef enum
{
PQ_IP_HSP_Dither_SC1_OFF_Main,
PQ_IP_HSP_Dither_SC1_NUMS_Main
} PQ_IP_HSP_Dither_SC1_Group_Main;

typedef enum
{
PQ_IP_HnonLinear_SC1_OFF_Main,
PQ_IP_HnonLinear_SC1_NUMS_Main
} PQ_IP_HnonLinear_SC1_Group_Main;

typedef enum
{
PQ_IP_SRAM1_InvSinc4Tc4p4Fc85Fstop134Apass01Astop50G13_Main,
PQ_IP_SRAM1_NUMS_Main
} PQ_IP_SRAM1_Group_Main;

typedef enum
{
PQ_IP_SRAM2_InvSinc4Tc4p4Fc45Apass01Astop40_Main,
PQ_IP_SRAM2_NUMS_Main
} PQ_IP_SRAM2_Group_Main;

typedef enum
{
PQ_IP_SRAM3_OFF_Main,
PQ_IP_SRAM3_NUMS_Main
} PQ_IP_SRAM3_Group_Main;

typedef enum
{
PQ_IP_SRAM4_OFF_Main,
PQ_IP_SRAM4_NUMS_Main
} PQ_IP_SRAM4_Group_Main;

typedef enum
{
PQ_IP_C_SRAM1_C121_Main,
PQ_IP_C_SRAM1_NUMS_Main
} PQ_IP_C_SRAM1_Group_Main;

typedef enum
{
PQ_IP_C_SRAM2_C2121_Main,
PQ_IP_C_SRAM2_NUMS_Main
} PQ_IP_C_SRAM2_Group_Main;

typedef enum
{
PQ_IP_C_SRAM3_C161_Main,
PQ_IP_C_SRAM3_NUMS_Main
} PQ_IP_C_SRAM3_Group_Main;

typedef enum
{
PQ_IP_C_SRAM4_C2121_Main,
PQ_IP_C_SRAM4_NUMS_Main
} PQ_IP_C_SRAM4_Group_Main;

typedef enum
{
PQ_IP_VSP_Y_SC2_Bypass_Main,
PQ_IP_VSP_Y_SC2_NUMS_Main
} PQ_IP_VSP_Y_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_C_SC2_Bypass_Main,
PQ_IP_VSP_C_SC2_NUMS_Main
} PQ_IP_VSP_C_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_CoRing_SC2_OFF_Main,
PQ_IP_VSP_CoRing_SC2_NUMS_Main
} PQ_IP_VSP_CoRing_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_DeRing_SC2_OFF_Main,
PQ_IP_VSP_DeRing_SC2_NUMS_Main
} PQ_IP_VSP_DeRing_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_Dither_SC2_OFF_Main,
PQ_IP_VSP_Dither_SC2_NUMS_Main
} PQ_IP_VSP_Dither_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_PreVBound_SC2_OFF_Main,
PQ_IP_VSP_PreVBound_SC2_NUMS_Main
} PQ_IP_VSP_PreVBound_SC2_Group_Main;

typedef enum
{
PQ_IP_AntiPAL_filter_SC2_OFF_Main,
PQ_IP_AntiPAL_filter_SC2_NUMS_Main
} PQ_IP_AntiPAL_filter_SC2_Group_Main;

typedef enum
{
PQ_IP_422To444_SC2_ON_Main,
PQ_IP_422To444_SC2_NUMS_Main
} PQ_IP_422To444_SC2_Group_Main;

typedef enum
{
PQ_IP_HSP_Y_SC2_Bypass_Main,
PQ_IP_HSP_Y_SC2_NUMS_Main
} PQ_IP_HSP_Y_SC2_Group_Main;

typedef enum
{
PQ_IP_HSP_C_SC2_Bypass_Main,
PQ_IP_HSP_C_SC2_Bilinear_Main,
PQ_IP_HSP_C_SC2_C_SRAM_1_Main,
PQ_IP_HSP_C_SC2_C_SRAM_2_Main,
PQ_IP_HSP_C_SC2_C_SRAM_3_Main,
PQ_IP_HSP_C_SC2_C_SRAM_4_Main,
PQ_IP_HSP_C_SC2_SRAM_1_4Tap_Main,
PQ_IP_HSP_C_SC2_SRAM_2_4Tap_Main,
PQ_IP_HSP_C_SC2_NUMS_Main
} PQ_IP_HSP_C_SC2_Group_Main;

typedef enum
{
PQ_IP_HSP_CoRing_SC2_OFF_Main,
PQ_IP_HSP_CoRing_SC2_NUMS_Main
} PQ_IP_HSP_CoRing_SC2_Group_Main;

typedef enum
{
PQ_IP_HSP_DeRing_SC2_OFF_Main,
PQ_IP_HSP_DeRing_SC2_NUMS_Main
} PQ_IP_HSP_DeRing_SC2_Group_Main;

typedef enum
{
PQ_IP_HSP_Dither_SC2_OFF_Main,
PQ_IP_HSP_Dither_SC2_NUMS_Main
} PQ_IP_HSP_Dither_SC2_Group_Main;

typedef enum
{
PQ_IP_HnonLinear_SC2_OFF_Main,
PQ_IP_HnonLinear_SC2_NUMS_Main
} PQ_IP_HnonLinear_SC2_Group_Main;

typedef enum
{
PQ_IP_VSP_Y_SC3_Bypass_Main,
PQ_IP_VSP_Y_SC3_NUMS_Main
} PQ_IP_VSP_Y_SC3_Group_Main;

typedef enum
{
PQ_IP_VSP_C_SC3_Bypass_Main,
PQ_IP_VSP_C_SC3_NUMS_Main
} PQ_IP_VSP_C_SC3_Group_Main;

typedef enum
{
PQ_IP_VSP_CoRing_SC3_OFF_Main,
PQ_IP_VSP_CoRing_SC3_NUMS_Main
} PQ_IP_VSP_CoRing_SC3_Group_Main;

typedef enum
{
PQ_IP_VSP_DeRing_SC3_OFF_Main,
PQ_IP_VSP_DeRing_SC3_NUMS_Main
} PQ_IP_VSP_DeRing_SC3_Group_Main;

typedef enum
{
PQ_IP_VSP_Dither_SC3_OFF_Main,
PQ_IP_VSP_Dither_SC3_NUMS_Main
} PQ_IP_VSP_Dither_SC3_Group_Main;

typedef enum
{
PQ_IP_VSP_PreVBound_SC3_OFF_Main,
PQ_IP_VSP_PreVBound_SC3_NUMS_Main
} PQ_IP_VSP_PreVBound_SC3_Group_Main;

typedef enum
{
PQ_IP_AntiPAL_filter_SC3_OFF_Main,
PQ_IP_AntiPAL_filter_SC3_NUMS_Main
} PQ_IP_AntiPAL_filter_SC3_Group_Main;

typedef enum
{
PQ_IP_422To444_SC3_ON_Main,
PQ_IP_422To444_SC3_NUMS_Main
} PQ_IP_422To444_SC3_Group_Main;

typedef enum
{
PQ_IP_HSP_Y_SC3_Bypass_Main,
PQ_IP_HSP_Y_SC3_NUMS_Main
} PQ_IP_HSP_Y_SC3_Group_Main;

typedef enum
{
PQ_IP_HSP_C_SC3_Bypass_Main,
PQ_IP_HSP_C_SC3_NUMS_Main
} PQ_IP_HSP_C_SC3_Group_Main;

typedef enum
{
PQ_IP_HSP_CoRing_SC3_OFF_Main,
PQ_IP_HSP_CoRing_SC3_NUMS_Main
} PQ_IP_HSP_CoRing_SC3_Group_Main;

typedef enum
{
PQ_IP_HSP_DeRing_SC3_OFF_Main,
PQ_IP_HSP_DeRing_SC3_NUMS_Main
} PQ_IP_HSP_DeRing_SC3_Group_Main;

typedef enum
{
PQ_IP_HSP_Dither_SC3_OFF_Main,
PQ_IP_HSP_Dither_SC3_NUMS_Main
} PQ_IP_HSP_Dither_SC3_Group_Main;

typedef enum
{
PQ_IP_HnonLinear_SC3_OFF_Main,
PQ_IP_HnonLinear_SC3_NUMS_Main
} PQ_IP_HnonLinear_SC3_Group_Main;

typedef enum
{
PQ_IP_SWDriver_OFF_Main,
PQ_IP_SWDriver_ON_Main,
PQ_IP_SWDriver_SD_FRC_Main,
PQ_IP_SWDriver_NUMS_Main
} PQ_IP_SWDriver_Group_Main;

typedef enum
{
PQ_IP_SC_End_End_Main,
PQ_IP_SC_End_NUMS_Main
} PQ_IP_SC_End_Group_Main;

extern  unsigned char MST_SkipRule_IP_Main[PQ_IP_NUM_Main];
extern  EN_IPTAB_INFO PQ_IPTAB_INFO_Main[];
extern  unsigned char MST_VIP_IHC_CRD_SRAM_Main[][PQ_IP_VIP_IHC_CRD_SRAM_SIZE_Main];
extern  unsigned char MST_VIP_ICC_CRD_SRAM_Main[][PQ_IP_VIP_ICC_CRD_SRAM_SIZE_Main];
extern  unsigned char MST_SRAM1_Main[][PQ_IP_SRAM1_SIZE_Main];
extern  unsigned char MST_C_SRAM1_Main[][PQ_IP_C_SRAM1_SIZE_Main];
extern  unsigned char MST_SRAM2_Main[][PQ_IP_SRAM2_SIZE_Main];
extern  unsigned char MST_C_SRAM2_Main[][PQ_IP_C_SRAM2_SIZE_Main];
extern  unsigned char QMAP_1920_Main[QM_INPUTTYPE_NUM_Main][PQ_IP_NUM_Main];

#endif
