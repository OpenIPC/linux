/*
 * mhal_miu.h- Sigmastar
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
#ifndef _MHAL_MIU_H_
#define _MHAL_MIU_H_

typedef enum
{
    E_PROTECT_0 = 0,
    E_PROTECT_1,
    E_PROTECT_2,
    E_PROTECT_3,
    E_PROTECT_4,
    E_MIU_PROTECT_MAX = 15,
    E_MMU_PROTECT_0,
    E_MMU_PROTECT_1,
    E_MMU_PROTECT_2,
    E_MMU_PROTECT_3,
    E_MMU_PROTECT_MAX = 31,
} PROTECT_ID;
//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

#define MIU_MAX_DEVICE     (1)
#define MIU_MAX_GROUP      (8)
#define MIU_MAX_GP_CLIENT  (16)
#define MIU_MAX_TBL_CLIENT (MIU_MAX_GROUP * MIU_MAX_GP_CLIENT)

#define MIU_PAGE_SHIFT           (13)     // Unit for MIU protect (8KB)
#define MIU_PROTECT_ADDRESS_UNIT (0x20UL) // Unit for MIU hitted address
#define MIU_MAX_PROTECT_BLOCK    (5)
#define MMU_MAX_PROTECT_BLOCK    (4)
#define MIU_MAX_PROTECT_ID       (16)

#define IDNUM_KERNELPROTECT   (16)
#define MMU_INVALID_ENTRY_VAL (0x3FF)

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_MIU_BLOCK_0 = 0,
    E_MIU_BLOCK_1,
    E_MIU_BLOCK_2,
    E_MIU_BLOCK_3,
    E_MIU_BLOCK_4,
    E_MIU_BLOCK_NUM,
} MIU_BLOCK_ID;

typedef enum
{
    E_MMU_BLOCK_0 = 0,
    E_MMU_BLOCK_1,
    E_MMU_BLOCK_2,
    E_MMU_BLOCK_3,
    E_MMU_BLOCK_NUM,
} MMU_BLOCK_ID;

typedef enum
{
    E_HAL_MMU_STATUS_NORMAL       = 0,
    E_HAL_MMU_STATUS_RW_COLLISION = 0x1,
    E_HAL_MMU_STATUS_R_INVALID    = 0x2,
    E_HAL_MMU_STATUS_W_INVALID    = 0x4,
    E_HAL_MMU_STATUS_NUM,
} HAL_MMU_STATUS;

typedef struct
{
    U32 size;        // bytes
    U32 dram_freq;   // MHz
    U32 miupll_freq; // MHz
    U8  type;        // 2:DDR2, 3:DDR3
    U8  data_rate;   // 4:4x mode, 8:8x mode,
    U8  bus_width;   // 16:16bit, 32:32bit, 64:64bit
    U8  ssc;         // 0:off, 1:on
} MIU_DramInfo_Hal;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
MS_BOOL     HAL_MIU_CheckGroup(int group);
const char *halClientIDTName(int id);
const char *halWriteClientIDTName(int id);
const char *halReadClientIDTName(int id);

U8      HAL_MIU_GetProtectIdEnVal(U8 u8MiuDev, U8 u8BlockId, U8 u8ProtectIdIndex);
U8      HAL_MIU_Protect(U8 u8Blockx, U16 *pu8ProtectId, U32 u32BusStart, U32 u32BusEnd, U8 bSetFlag);
U8      HAL_MIU_ParseOccupiedResource(void);
U32     HAL_MIU_ProtectDramSize(void);
int     HAL_MIU_Info(MIU_DramInfo_Hal *pDramInfo);
U8      HAL_MIU_GetHitProtectInfo(U8 u8MiuSel, MIU_PortectInfo *pInfo);
U16 *   HAL_MIU_GetDefaultKernelProtectClientID(void);
U16 *   HAL_MIU_GetKernelProtectClientID(U8 u8MiuSel);
MS_U16 *HAL_MMU_GetKernelProtectClientID(MS_U8 u8MiuSel);

U8 HAL_MIU_Protect_Add_Ext_Feature(U8 u8ModeSel, U16 *pu8ProtectId, U8 u8Blockx, U8 bSetFlag, U8 bIdFlag,
                                   U8 bInvertFlag);
U8 HAL_SetAccessFromVpaOnly(U8 u8Blockx, U32 u32BusStart, U32 u32BusEnd, U8 bSetFlag);

// MMU HAL Function
int HAL_MMU_SetPageSize(U8 u8PgSz256En);
int HAL_MMU_SetRegion(U16 u16Region, U16 u16ReplaceRegion);
int HAL_MMU_Map(U16 u16PhyAddrEntry, U16 u16VirtAddrEntry);
U16 HAL_MMU_MapQuery(U16 u16PhyAddrEntry);
int HAL_MMU_UnMap(U16 u16PhyAddrEntry);
int HAL_MMU_Enable(U8 u8Enable);
int HAL_MMU_Reset(void);
U32 HAL_MMU_Status(U16 *u16PhyAddrEntry, U16 *u16ClientId, U8 *u8IsReadCmd);
U8  HAL_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd);
int HAL_MMU_AddClientId(U16 u16ClientId);
int HAL_MMU_RemoveClientId(U16 u16ClientId);
#endif // _MHAL_MIU_H_
