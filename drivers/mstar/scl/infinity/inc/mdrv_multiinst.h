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

#ifndef _MDRV_MULTI_INST_H
#define _MDRV_MULTI_INST_H


#define MDRV_MULTI_INST_PRIVATE_ID_NUM    4
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_MDRV_MULTI_INST_ENTRY_ID_HVSP1,
    E_MDRV_MULTI_INST_ENTRY_ID_HVSP2,
    E_MDRV_MULTI_INST_ENTRY_ID_HVSP3,
    E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA1,
    E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA2,
    E_MDRV_MULTI_INST_ENTRY_ID_SCLDMA3,
    E_MDRV_MULTI_INST_ENTRY_ID_DISP,
    E_MDRV_MULTI_INST_ENTRY_ID_MAX,
}EN_MDRV_MULTI_INST_ENTRY_ID_TYPE;


typedef enum
{
    E_MDRV_MULTI_INST_LOCK_ID_SC_1_2,
    E_MDRV_MULTI_INST_LOCK_ID_SC_3,
    E_MDRV_MULTI_INST_LOCK_ID_DISP,
    E_MDRV_MULTI_INST_LOCK_ID_MAX,
}EN_MDRV_MULTI_INST_LOCK_ID_TYPE;


typedef enum
{
    E_MDRV_MULTI_INST_CMD_HVSP_IN_CONFIG,
    E_MDRV_MULTI_INST_CMD_HVSP_SCALING_CONFIG,
    E_MDRV_MULTI_INST_CMD_HVSP_MEM_REQ_CONFIG,
    E_MDRV_MULTI_INST_CMD_HVSP_POST_CROP_CONFIG,
    E_MDRV_MULTI_INST_CMD_HVSP_SET_OSD_CONFIG,
    E_MDRV_MULTI_INST_CMD_SCLDMA_IN_BUFFER_CONFIG,
    E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_BUFFER_CONFIG,
    E_MDRV_MULTI_INST_CMD_SCLDMA_IN_TRIGGER_CONFIG,
    E_MDRV_MULTI_INST_CMD_SCLDMA_OUT_TRIGGER_CONFIG,
    E_MDRV_MULTI_INST_CMD_FORCE_RELOAD_CONFIG,
    E_MDRV_MULTI_INST_CMD_MAX,
}EN_MDRV_MULTI_INST_CMD_TYPE;


typedef enum
{
    E_MDRV_MULTI_INST_STATUS_FAIL,
    E_MDRV_MULTI_INST_STATUS_SUCCESS,
    E_MDRV_MULTI_INST_STATUS_LOCKED,
}EN_MDRV_MULTI_INST_STATUS_TYPE;

typedef struct
{
    unsigned int u32StructSize;
    unsigned int *pVersion;
    unsigned int u32VersionSize;
}ST_MDRV_SCLDMA_VERSIONCHK_CONFIG;
typedef struct
{
    unsigned int u32StructSize;
    unsigned int *pVersion;
    unsigned int u32VersionSize;
}ST_MDRV_HVSP_VERSIONCHK_CONFIG;

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

//===============================
//      LOCK multi instance
//===============================
typedef struct
{
    signed long *ps32PrivateID;
    unsigned char u8IDNum;
}ST_MDRV_MULTI_INST_LOCK_CONFIG;



//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef _MDRV_MULTI_INST_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE unsigned char MDrv_MultiInst_Lock_Init(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID);
INTERFACE unsigned char MDrv_MultiInst_Lock_Exit(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID);
INTERFACE unsigned char MDrv_MultiInst_Lock_IsFree(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, signed long s32PrivateId);
INTERFACE unsigned char MDrv_MultiInst_Lock_Alloc(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, ST_MDRV_MULTI_INST_LOCK_CONFIG stCfg);
INTERFACE unsigned char MDrv_MultiInst_Lock_Free(EN_MDRV_MULTI_INST_LOCK_ID_TYPE enLock_ID, ST_MDRV_MULTI_INST_LOCK_CONFIG *pstCfg);


INTERFACE unsigned char MDrv_MultiInst_Entry_Init_Variable(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID);
INTERFACE unsigned char MDrv_MultiInst_Entry_Alloc(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void **pPrivate_Data);
INTERFACE unsigned char MDrv_MultiInst_Entry_Free(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivate_Data);
INTERFACE unsigned char MDrv_MultiInst_Entry_GetPirvateId(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivateData, signed long *ps32PrivatId);
INTERFACE EN_MDRV_MULTI_INST_STATUS_TYPE MDrv_MultiInst_Entry_FlashData(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivateData, EN_MDRV_MULTI_INST_CMD_TYPE enCmd, void *pData);
INTERFACE EN_MDRV_MULTI_INST_STATUS_TYPE MDrv_MultiInst_Etnry_IsFree(EN_MDRV_MULTI_INST_ENTRY_ID_TYPE enID, void *pPrivateData);

#undef INTERFACE

#endif
