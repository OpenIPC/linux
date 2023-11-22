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
    E_MIU_PROTECT_MAX = 31,
    E_MMU_PROTECT_0,
    E_MMU_PROTECT_1,
    E_MMU_PROTECT_2,
    E_MMU_PROTECT_3,
    E_MMU_PROTECT_MAX = 63,
} PROTECT_ID;
//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

#define MIU_MAX_DEVICE     (1)
#define MIU_MAX_GROUP      (9) // normal way group(8) + high way group(1)
#define MIU_MAX_GP_CLIENT  (16)
#define MIU_MAX_TBL_CLIENT (MIU_MAX_GROUP * MIU_MAX_GP_CLIENT)

#define MIU_PAGE_SHIFT           (13)     // Unit for MIU protect (8KB)
#define MIU_PROTECT_ADDRESS_UNIT (0x10UL) // Unit for MIU hitted address
#define MIU_MAX_PROTECT_BLOCK    (32)
#define MMU_MAX_PROTECT_BLOCK    (32)
#define MIU_MAX_PROTECT_ID       (32)

#define IDNUM_KERNELPROTECT   (32)
#define MMU_INVALID_ENTRY_VAL (0xFFF)

#define MIU_MMU_ENTRY_BITS (12)

#ifndef BIT0
#define BIT0  0x0001UL
#define BIT1  0x0002UL
#define BIT2  0x0004UL
#define BIT3  0x0008UL
#define BIT4  0x0010UL
#define BIT5  0x0020UL
#define BIT6  0x0040UL
#define BIT7  0x0080UL
#define BIT8  0x0100UL
#define BIT9  0x0200UL
#define BIT10 0x0400UL
#define BIT11 0x0800UL
#define BIT12 0x1000UL
#define BIT13 0x2000UL
#define BIT14 0x4000UL
#define BIT15 0x8000UL
#endif

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
    E_MMU_BLOCK_4,
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
    MS_U64 size;        // bytes
    MS_U32 dram_freq;   // MHz
    MS_U32 miupll_freq; // MHz
    MS_U8  type;        // 0:LPDDR4, 1:DDR4
    MS_U8  data_rate;   // 4:4x mode, 8:8x mode,
    MS_U8  bus_width;   // 16:16bit, 32:32bit, 64:64bit
    MS_U8  ssc;         // 0:off, 1:on
} MIU_DramInfo_Hal;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------
MS_BOOL HAL_MIU_CheckGroup(int group);
MS_BOOL HAL_MIU_GetProtectIdEnVal(MS_U8 u8MiuDev, MS_U8 u8BlockId, MS_U8 u8ProtectIdIndex);
MS_BOOL HAL_MIU_Protect(MS_U8 u8Blockx, MS_U16 *pu8ProtectId, ss_phys_addr_t u64BusStart, ss_phys_addr_t u64BusEnd,
                        MS_BOOL bSetFlag);
MS_BOOL HAL_MIU_ParseOccupiedResource(void);
unsigned long long HAL_MIU_ProtectDramSize(void);
const char *       halWriteClientIDTName(int id);
const char *       halReadClientIDTName(int id);
// need to remove for debug
const char *halClientIDTName(int id);
#if 0
int HAL_MIU_ClientIdToName(MS_U16 clientId, char *clientName);
#endif
int     HAL_MIU_Info(MIU_DramInfo_Hal *pDramInfo);
U8      HAL_MIU_GetHitProtectInfo(U8 u8MiuSel, MIU_PortectInfo *pInfo);
U16 *   HAL_MIU_GetDefaultKernelProtectClientID(void);
U16 *   HAL_MIU_GetKernelProtectClientID(U8 u8MiuSel);
MS_U16 *HAL_MMU_GetKernelProtectClientID(MS_U8 u8MiuSel);

U8 HAL_MIU_Protect_Add_Ext_Feature(U8 u8ModeSel, U16 *pu8ProtectId, U8 u8Blockx, U8 bSetFlag, U8 bIdFlag,
                                   U8 bInvertFlag);
U8 HAL_SetAccessFromVpaOnly(U8 u8Blockx, ss_phys_addr_t u32BusStart, ss_phys_addr_t u32BusEnd, U8 bSetFlag);

// MMU HAL Function
int            HAL_MMU_SetPageSize(unsigned char u8PgszMode);
int            HAL_MMU_SetRegion(unsigned short u16Region, unsigned short u16ReplaceRegion);
int            HAL_MMU_Map(unsigned short u16PhyAddrEntry, unsigned short u16VirtAddrEntry);
unsigned short HAL_MMU_MapQuery(unsigned short u16PhyAddrEntry);
int            HAL_MMU_UnMap(unsigned short u16PhyAddrEntry);
int            HAL_MMU_Enable(unsigned char u8Enable);
int            HAL_MMU_Reset(void);
unsigned int   HAL_MMU_Status(unsigned short *u16PhyAddrEntry, unsigned short *u16ClientId, unsigned char *u8IsReadCmd);
U8             HAL_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd);
int            HAL_MMU_AddClientId(U16 u16ClientId);
int            HAL_MMU_RemoveClientId(U16 u16ClientId);
#endif // _MHAL_MIU_H_
