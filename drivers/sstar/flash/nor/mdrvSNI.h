/*
 * mdrvSNI.h- Sigmastar
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
#ifndef __MDRV_SNI_H
#define __MDRV_SNI_H

typedef struct
{
    u8  u8_command;
    u32 u32_address;
    u8  u8_addressBytes;
    u8  u8_dummy;
    u16 u16_dataBytes;
    u16 u16_value;
} FLASH_CMD_SET_t;

typedef struct
{
    u8              u8_needQE;
    FLASH_CMD_SET_t st_rQuadEnabled;
    FLASH_CMD_SET_t st_wQuadEnabled;
} SPINOR_QUAD_CFG_t;

typedef struct
{
    FLASH_CMD_SET_t st_SRP0;
    FLASH_CMD_SET_t st_SRP1;
} SPINOR_SRP_t;

typedef struct
{
    FLASH_CMD_SET_t st_complement;
    FLASH_CMD_SET_t st_topBottom;
    FLASH_CMD_SET_t st_blocks;
} SPINOR_MEMORY_PROTECT_t;

typedef struct
{
    SPINOR_MEMORY_PROTECT_t st_blockStatus;
    SPINOR_SRP_t            st_srp;
} SPINOR_PROTECT_STATUS_t;

typedef struct
{
    u8                      u8_IDByteCnt;
    u8                      au8_ID[15];
    u8                      u8_MaxClk;
    u16                     u16_PageByteCnt;
    u16                     u16_SectorByteCnt;
    u32                     u32_BlkBytesCnt;
    u32                     u32_Capacity;
    u32                     u32_MaxWaitTime;
    u8                      au8_reserved[4];
    u32                     u32_extSni;
    SPINOR_PROTECT_STATUS_t st_rProtectStatus;
    SPINOR_PROTECT_STATUS_t st_wProtectStatus;
    SPINOR_QUAD_CFG_t       st_QE;
    FLASH_CMD_SET_t         st_readData;
    FLASH_CMD_SET_t         st_program;
} SPINOR_INFO_t;

typedef struct
{
    u8 au8_magic[16];
    u8 u8_IDByteCnt;
    u8 au8_ID[15];
} SPINOR_MAGIC_t;

typedef struct
{
    SPINOR_MAGIC_t spinor_magic;
    u8             au8_reserved[0x100];
    u8             au8_venderName[12];
    u32            u32_crc32;
    u8             au8_partnumber[16];
} SPINOR_EXT_SNI_t;

typedef struct
{
    /*-------unused------------*/
    u32           u32_crc32;
    SPINOR_INFO_t spinor_info;
    /*-------------------------*/
} SPINOR_EXT_PROFILE_t;

typedef struct
{
    u8                   au8_magic[16];
    SPINOR_INFO_t        spinor_info;
    u8                   au8_venderName[12];
    u32                  u32_crc32;
    u8                   au8_partnumber[16];
    SPINOR_EXT_PROFILE_t spinor_ext_profile;
} SPINOR_SNI_t;

#endif
