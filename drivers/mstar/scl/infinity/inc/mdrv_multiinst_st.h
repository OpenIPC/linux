////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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

#ifndef _MDRV_MULTI_INST_ST_H
#define _MDRV_MULTI_INST_ST_H


#define MDRV_MULTI_INST_HVSP_NUM    4
#define MDRV_MULTI_INST_SCLDMA_NUM  4
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_MDRV_MULTI_INST_HVSP_DATA_ID_1,
    E_MDRV_MULTI_INST_HVSP_DATA_ID_2,
    E_MDRV_MULTI_INST_HVSP_DATA_ID_3,
    E_MDRV_MULTI_INST_HVSP_DATA_ID_MAX,
}EN_MDRV_MULTI_INST_HVSP_DATA_ID_TYPE;


typedef enum
{
    E_MDRV_MULTI_INST_SCLDMA_DATA_ID_1,
    E_MDRV_MULTI_INST_SCLDMA_DATA_ID_2,
    E_MDRV_MULTI_INST_SCLDMA_DATA_ID_3,
    E_MDRV_MULTI_INST_SCLDMA_DATA_ID_4,
    E_MDRV_MULTI_INST_SCLDMA_DATA_ID_MAX,
}EN_MDRV_MULTI_INST_SCLDMA_DATA_ID_TYPE;


//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

//===============================
//      HVSP multi instance
//===============================
typedef struct
{
    unsigned char   bInCfg    : 1;
    unsigned char   bScaleCfg : 1;
    unsigned char   bPostCropCfg : 1;
    unsigned char   bMemCfg      : 1;
    unsigned char   bOSDCfg      : 1;
    unsigned char   Reserved     : 2;
}ST_MDRV_MULTI_INST_HVSP_FLAG_TYPE;

typedef struct
{
    ST_MDRV_MULTI_INST_HVSP_FLAG_TYPE stFlag;
    ST_MDRV_HVSP_INPUT_CONFIG       stInCfg;
    ST_MDRV_HVSP_SCALING_CONFIG    stScaleCfg;
    ST_MDRV_HVSP_IPM_CONFIG         stMemCfg;
    ST_MDRV_HVSP_POSTCROP_CONFIG   stPostCropCfg;
    ST_MDRV_HVSP_OSD_CONFIG         stOSDCfg;
}ST_MDRV_MULTI_INST_HVSP_DATA_TYPE;

typedef struct
{
    signed long s32PivateId;
    unsigned char bUsed;
    ST_MDRV_MULTI_INST_HVSP_DATA_TYPE stData;
}ST_MDRV_MULTI_INST_HVSP_ENTRY_CONFIG;


//===============================
//      SCLDMA multi instance
//===============================
typedef struct
{
    unsigned char   bInBufCfg      ;
    unsigned char   bOutBufCfg[E_MDRV_SCLDMA_MEM_NUM]  ;
    unsigned char   bInTrigCfg     ;
    unsigned char   bOutTrigCfg[E_MDRV_SCLDMA_MEM_NUM] ;
}ST_MDRV_MULTI_INST_SCLDMA_FLAG_TYPE;

typedef struct
{
    ST_MDRV_MULTI_INST_SCLDMA_FLAG_TYPE stFlag;
    ST_MDRV_SCLDMA_BUFFER_CONFIG stInBufCfg;
    ST_MDRV_SCLDMA_BUFFER_CONFIG stOutBufCfg[E_MDRV_SCLDMA_MEM_NUM];
    ST_MDRV_SCLDMA_TRIGGER_CONFIG stInTrigCfg;
    ST_MDRV_SCLDMA_TRIGGER_CONFIG stOutTrigCfg[E_MDRV_SCLDMA_MEM_NUM];
}ST_MDRV_MULTI_INST_SCLDMA_DATA_TYPE;


typedef struct
{
    signed long s32PivateId;
    unsigned char bUsed;
    ST_MDRV_MULTI_INST_SCLDMA_DATA_TYPE stData;
}ST_MDRV_MULTI_INST_SCLDMA_ENTRY_CONFIG;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef _MDRV_MULTI_INST_C
#define INTERFACE extern
#else
#define INTERFACE
#endif


#undef INTERFACE

#endif
