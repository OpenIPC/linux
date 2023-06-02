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

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file    drvPQ_Datatypes.h
/// @brief  PQ interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DRVPQ_DATATYPES_H_
#define _DRVPQ_DATATYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MS_422To444_ON,
    MS_422To444_OFF,
    MS_422To444_SIMPLE,
    MS_422To444_BICUBIC,
    MS_422To444_BYPASS,
    MS_422To444_NUMS
}MS_422To444_TYPE;

typedef enum
{
    MS_420_CUP_ON,
    MS_420_CUP_OFF,
    MS_420_CUP_NUM,
}MS_420_CUP_TYPE;

typedef enum
{
    MS_HNONLINEAR_OFF,
    MS_HNONLINEAR_1920_0,
    MS_HNONLINEAR_1920_1,
    MS_HNONLINEAR_1920_2,
    MS_HNONLINEAR_1680,
    MS_HNONLINEAR_1440,
    MS_HNONLINEAR_1366_0,
    MS_HNONLINEAR_1366_1,
    MS_HNONLINEAR_1366_2,
    MS_HNONLINEAR_NUM,
}MS_HNONLINEAR_TYPE;

typedef enum
{
    MS_MADI_24_4R,
    MS_MADI_24_2R,
    MS_MADI_25_4R_MC,
    MS_MADI_25_4R,
    MS_MADI_25_2R,
    MS_MADI_26_4R,
    MS_MADI_26_2R,
    MS_MADI_27_4R,
    MS_MADI_27_2R,
    MS_MADI_P_MODE8,
    MS_MADI_P_MODE10,
    MS_MADI_P_MODE_MOT10,
    MS_MADI_P_MODE_MOT8,
    MS_MADI_24_RFBL_NFILM,
    MS_MADI_24_RFBL_FILM,
    MS_MADI_FBL_DNR,
    MS_MADI_FBL_MIU,
    MS_MADI_P_MODE8_NO_MIU,
    MS_MADI_24_4R_880,
    MS_MADI_24_2R_880,
    MS_MADI_25_4R_MC_NW,
    MS_MADI_25_6R_MC,
    MS_MADI_25_14F_6R_MC,    //Add New MADi mode
    MS_MADI_25_4R_884,
    MS_MADI_25_4R_880,
    MS_MADI_25_2R_884,
    MS_MADI_25_2R_880,
    MS_MADI_25_6R_MC_NW,
    MS_MADI_25_12F_8R_MC,
    MS_MADI_25_14F_8R_MC,
    MS_MADI_25_16F_8R_MC,
    MS_MADI_25_6R_FilmPreDet,
    MS_MADI_24_6R_FilmPreDet,
    MS_MADI_25_6R_FilmPreDet_PAL,
    MS_MADI_24_6R_FilmPreDet_PAL,
    MS_MADI_RFBL_2D,
    MS_MADI_RFBL_25D,
    MS_MADI_RFBL_3D,
    MS_MADI_RFBL_3D_YC,
    MS_MADI_RFBL_3D_FILM,
    MS_MADI_P_MODE8_444,
    MS_MADI_P_MODE10_444,
    MS_MADI_RFBL_P_MODE10_MOT,
    MS_MADI_RFBL_P_MODE8_MOT,
    MS_MADI_P_MODE_MOT10_8Frame,
    MS_MADI_P_MODE_MOT10_4Frame,
    MS_MADI_P_MODE_MC_3Frame,
    MS_MADI_P_MODE_MC_6Frame_6R,
    MS_MADI_P_MODE_MC_12Frame_8R,
    MS_MADI_25_8F_4R_MC,
    MS_MADI_NUM,
}MS_MADI_TYPE;

typedef enum
{
    MS_VD_SAMPLING_STANDARD,
    MS_VD_SAMPLING_1135,
    MS_VD_SAMPLING_1135_MUL_15,
    MS_VD_SAMPLING_NUM,
}MS_VD_SAMPLING_TYPE;

typedef enum
{
    MS_PQ_IP_VD_SAMPLING,
    MS_PQ_IP_HSD_SAMPLING,
    MS_PQ_IP_ADC_SAMPLING,
    MS_PQ_IP_NUM,
}MS_PQ_IP_TYPE;

typedef enum
{
    MS_ADC_SAMPLING_X_1,
    MS_ADC_SAMPLING_X_2,
    MS_ADC_SAMPLING_X_4,
    MS_ADC_SAMPLING_NUM,
}MS_ADC_SAMPLING_TYPE;

typedef enum
{
    MS_CSC_IP_CSC,
    MS_CSC_IP_VIP_CSC,
    MS_CSC_IP_NUMs,
}MS_CSC_IP_TYPE;

typedef enum
{
    MS_HSD_SAMPLING_Div_1_000,
    MS_HSD_SAMPLING_Div_1_125,
    MS_HSD_SAMPLING_Div_1_250,
    MS_HSD_SAMPLING_Div_1_375,
    MS_HSD_SAMPLING_Div_1_500,
    MS_HSD_SAMPLING_Div_1_625,
    MS_HSD_SAMPLING_Div_1_750,
    MS_HSD_SAMPLING_Div_1_875,
    MS_HSD_SAMPLING_Div_2_000,
    MS_HSD_SAMPLING_NUM,
}MS_HSD_SAMPLING_TYPE;

#ifdef __cplusplus
}
#endif

#endif /* _DRVPQ_DATATYPES_H_ */
