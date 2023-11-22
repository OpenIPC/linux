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

#ifndef _MDRV_SNI_H_
#define _MDRV_SNI_H_

typedef enum
{
    SPINAND_OTP_AVAIL                   = 0x0001,
    SPINAND_ALL_LOCK                    = 0x0002,
    SPINAND_EXT_ECC                     = 0x0004,
    SPINAND_ECC_RESERVED_NONE_CORRECTED = 0x0008,
    SPINAND_ECC_RESERVED_CORRECTED      = 0x0010,
    SPINAND_NEED_QE                     = 0x0020,
    SPINAND_MULTI_DIES                  = 0x0040,
    SPINAND_VOLATILE                    = 0x0080,
    SPINAND_PLANE_SELECT                = 0x0100,
    SPINAND_CONFIG_READY                = 0x8000
} SPINAND_FUNC_FLAG_t;

typedef enum
{
    SPINAND_NO_CR = 0,
    SPINAND_NONE_BUF_MODE,
    SPINAND_BUF_MODE,
} SPINAND_CR_TYPE_t;

typedef enum
{
    SPINAND_CR_NONE            = 0x00,
    SPINAND_CR_NEXT_STATUS     = 0x01,
    SPINAND_CR_BUSY_AFTER_READ = 0x02,
    SPINAND_CR_BUSY_AFTER_NEXT = 0x04,
    SPINAND_CR_END_WITH_REST   = 0x08,
    SPINAND_CR_BLOCK_WITH_LAST = 0x10,
} SPINAND_CR_CHECK_FLAG_t;

typedef enum
{
    SPINAND_THRESHOLD               = 0x01,
    SPINAND_BITFLIP                 = 0x02,
    SPINAND_SOCECC                  = 0x04,
    SPINAND_ENABLE_UBI_BBM          = 0x40,
    SPINAND_RESERVED_NONE_CORRECTED = 0x80,
} SPINAND_ECC_TYPE_t;

typedef enum
{
    SPINAND_READ = 0x0,
    SPINAND_PROGRAM,
    SPINAND_RANDOM,
} SPINAND_ACCESS_t;

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
    FLASH_CMD_SET_t st_SRP0;
    FLASH_CMD_SET_t st_SRP1;
} SPINAND_SRP_t;

typedef struct
{
    FLASH_CMD_SET_t st_complement;
    FLASH_CMD_SET_t st_topBottom;
    FLASH_CMD_SET_t st_blocks;
} SPINAND_MEMORY_PROTECT_t;

typedef struct
{
    SPINAND_MEMORY_PROTECT_t st_blockStatus;
    SPINAND_SRP_t            st_srp;
} SPINAND_PROTECT_t;

typedef struct
{
    FLASH_CMD_SET_t st_load;
    FLASH_CMD_SET_t st_noneBufModeCode;
} SPINAND_NONE_BUF_t;

typedef struct
{
    FLASH_CMD_SET_t st_nextPage;
    FLASH_CMD_SET_t st_lastPage;
    FLASH_CMD_SET_t st_checkBusy;
    u8              u8_checkFlag; // reference SPINAND_CR_CHECK_FLAG_t
} SPINAND_BUF_MODE_t;

typedef struct
{
    u8 u8_crType; // reference SPINAND_CR_TYPE_t
    union
    {
        SPINAND_NONE_BUF_t st_noneBufMode;
        SPINAND_BUF_MODE_t st_bufMode;
    } un_crProfile;
} SPINAND_CR_MODE_t;

typedef struct
{
    FLASH_CMD_SET_t   st_qeStatus;
    FLASH_CMD_SET_t   st_read;
    FLASH_CMD_SET_t   st_program;
    FLASH_CMD_SET_t   st_random;
    SPINAND_CR_MODE_t st_crMode;
} SPINAND_ACCESS_CONFIG_t;

typedef struct
{
    u32             u32_dieSize;
    FLASH_CMD_SET_t st_dieCode;
} SPINAND_DIE_t;

typedef struct
{
    FLASH_CMD_SET_t st_otpLock;
    FLASH_CMD_SET_t st_otpEnabled;
} SPINAND_OTP_t;

typedef struct
{
    FLASH_CMD_SET_t   st_eccEnabled;
    SPINAND_OTP_t     st_otp;
    SPINAND_DIE_t     st_dieConfig;
    SPINAND_PROTECT_t st_protectStatus;
} SPINAND_EXT_CONFIG_t;

typedef struct
{
    u32 start;
    u32 length;
} SPINAND_OTP_INFO;

typedef struct
{
    u8               u8_otpEn;
    u8               au8_reserved[27];
    SPINAND_OTP_INFO st_factory;
    SPINAND_OTP_INFO st_user;
    FLASH_CMD_SET_t  st_otpread;
    FLASH_CMD_SET_t  st_otpprogram;
} SPINAND_OTP_CONFIG_t;

typedef struct
{
    u8              u8_eccEn;
    u8              u8_eccType; // bit0 : threshold  bit1 : bitflip_table  bit2 : soc_ecc  bit7: reserved_none_corrected
    u8              u8_eccStatusMask;
    u8              u8_eccNotCorrectStatus;
    u8              u8_eccReserved;
    u8              u8_bitflipThreshold;
    u8              au8_reserved[32];
    FLASH_CMD_SET_t st_eccStatus;
    FLASH_CMD_SET_t st_threshold;
} SPINAND_ECC_CONFIG_t;

typedef struct
{
    u16                     u16_flags; // reference SPINAND_FUNC_FLAG_t
    u32                     u32_maxWaitTime;
    SPINAND_EXT_CONFIG_t    st_extConfig;
    SPINAND_ACCESS_CONFIG_t st_access;
    SPINAND_ECC_CONFIG_t    st_eccConfig;
    u8                      au8_reserved[24];
} SPINAND_EXT_PROFILE_t;

typedef struct
{
    u8                    au8_magic[16];
    u32                   u32_checksum;
    SPINAND_EXT_PROFILE_t st_profile;
    u32                   u32_extSni;
} SPINAND_EXT_INFO_t;

typedef struct
{
    u8  u8_IDByteCnt;
    u8  au8_ID[15];
    u16 u16_SpareByteCnt;
    u16 u16_PageByteCnt;
    u16 u16_BlkPageCnt;
    u16 u16_BlkCnt;
    u16 u16_SectorByteCnt;
    u8  u8PlaneCnt;
    u8  u8WrapConfig;
    u8  U8RIURead;
    u8  u8_MaxClk;
    u8  u8_UBOOTPBA;
    u8  u8_BL0PBA;
    u8  u8_BL1PBA;
    u8  u8_BLPINB;
    u8  u8_BAKCNT;
    u8  u8_BAKOFS;
    u8  u8_HashPBA[3]; // NO USED
    u8  u8_BootIdLoc;
    u8  u8_Reserved[24]; // just for aligning to 64bytes + magic[16] = 80bytes
} __attribute__((aligned(16))) SPINAND_INFO_t;

typedef struct
{
    u8 au8_magic[16];
    u8 u8_IDByteCnt;
    u8 au8_ID[15];
} SPINAND_MAGIC_t;

typedef struct
{
    SPINAND_MAGIC_t spinand_magic;
    u8              au8_reserved[0x1BC];
    u32             u32_crc32;
    u8              au8_venderName[16];
    u8              au8_partnumber[16];
} SPINAND_EXT_SNI_t;

typedef struct
{
    u32 u32_crc32;
    /*-------unused------------*/
    SPINAND_OTP_CONFIG_t st_otpConfig;
    /*-------------------------*/
} SPINAND_EXT_CONFIGURATION_t;

typedef struct
{
    u8                          au8_magic[16];
    SPINAND_INFO_t              spinand_info;
    SPINAND_EXT_INFO_t          spinand_ext_info;
    u32                         u32_crc32;
    u8                          au8_venderName[16];
    u8                          au8_partnumber[16];
    SPINAND_EXT_CONFIGURATION_t spinand_ext_configuration;
} SPINAND_SNI_t;

/*
------------------------|
| SPINAND_INFO_t        |
|-----------------------|
| SPINAND_EXT_INFO_t    |
|-----------------------|
*/
#endif /* _MDRV_SNI_H_ */
