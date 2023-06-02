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

#ifndef DRV_SCALER_ADAPTIVE_H
#define DRV_SCALER_ADAPTIVE_H

/******************************************************************************/
/*                     Macro                                                  */
/* ****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/
/*                     definition                                             */
/* ****************************************************************************/

#ifdef DRV_SCALER_NR_C
#define INTERFACE
#else
#define INTERFACE extern
#endif

/******************************************************************************/
/*                           Global Variables                                 */
/******************************************************************************/

//*************************************************************************
//          Enums
//*************************************************************************

//*************************************************************************
//          Structures
//*************************************************************************
typedef enum
{
    Level_Low=0,
    Level_MID,
    Level_High,

    MAX_Level
}XC_CTRL_LEVEL;

/********************************************************************************/
/*                   Function Prototypes                                        */
/********************************************************************************/
INTERFACE MS_U8 MApi_XC_GetCurrentMotionValue(void);
INTERFACE void MApi_XC_AdaptiveTuning(void);
INTERFACE void MApi_XC_FilmMode_AnyCandence_Enable(MS_BOOL bEnable);
INTERFACE void MApi_XC_DBK_Control(MS_BOOL bEnable, XC_CTRL_LEVEL eLevel);
INTERFACE void MApi_XC_DMS_Control(MS_BOOL bEnable, XC_CTRL_LEVEL eLevel);
INTERFACE void MApi_XC_SotreCurrentValue(void);
INTERFACE void MApi_XC_ReSotreToDefault(void);

#undef INTERFACE
#ifdef __cplusplus
}
#endif


#endif /* DRV_SCALER_ADAPTIVE_H */
