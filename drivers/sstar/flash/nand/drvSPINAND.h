/*
 * drvSPINAND.h- Sigmastar
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
#ifndef _DRV_SPINAND_H_
#define _DRV_SPINAND_H_

typedef signed char        s8;
typedef unsigned char      u8;
typedef signed short       s16;
typedef unsigned short     u16;
typedef signed int         s32;
typedef unsigned int       u32;
typedef signed long long   s64;
typedef unsigned long long u64;

typedef enum _SPINAND_ERROR_NUM
{
    ERR_SPINAND_SUCCESS           = 0x00,
    ERR_SPINAND_ECC_CORRECTED     = 0x01,
    ERR_SPINAND_ECC_NOT_CORRECTED = 0x02,
    ERR_SPINAND_ECC_RESERVED      = 0x03,
    ERR_SPINAND_E_FAIL            = 0x04,
    ERR_SPINAND_P_FAIL            = 0x05,
    ERR_SPINAND_TIMEOUT           = 0x06,
    ERR_SPINAND_INVALID           = 0x07,
    ERR_SPINAND_DEVICE_FAILURE    = 0x08,
    ERR_SPINAND_BDMA_FAILURE      = 0x09
} SPINAND_FLASH_ERRNO_e;

// SPI NAND COMMAND
#define SPI_NAND_CMD_RFC       0x03
#define SPI_NAND_CMD_RFC_DUMMY 0x08
#define SPI_NAND_CMD_FRFC      0x0B
#define SPI_NAND_CMD_PGRD      0x13
#define SPI_NAND_CMD_RDID      0x9F
#define SPI_NAND_CMD_WREN      0x06
#define SPI_NAND_CMD_WRDIS     0x04
#define SPI_NAND_CMD_PL        0x02
#define SPI_NAND_CMD_QPL       0x32
#define SPI_NAND_CMD_RPL       0x84
#define SPI_NAND_CMD_QRPL      0x34
#define SPI_NAND_CMD_PE        0x10
#define SPI_NAND_CMD_BE        0xD8
#define SPI_NAND_CMD_GF        0x0F
#define SPI_NAND_CMD_SF        0x1F
#define SPI_NAND_CMD_RESET     0xFF
#define SPI_NAND_REG_PROT      0xA0
#define SPI_NAND_REG_FEAT      0xB0
#define SPI_NAND_REG_STAT      0xC0
#define SPI_NAND_REG_FUT       0xD0
#define SPI_NAND_STAT_E_FAIL   (0x01 << 2)
#define SPI_NAND_STAT_P_FAIL   (0x01 << 3)
#define ECC_STATUS_PASS        (0x00 << 4)
#define ECC_NOT_CORRECTED      (0x02 << 4)
#define ECC_NO_CORRECTED       (0x00 << 4)
#define ECC_STATUS_MASK        (0x03 << 4)
#define ECC_STATUS_RESERVED    (0x03 << 4)
#define SPI_NAND_STAT_OIP      (0x1)

#define FLASH_PAGES_PER_BLOCK_DEFAULT 64
#define FLASH_BLOCK_MASK_DEFAULT      (FLASH_PAGES_PER_BLOCK_DEFAULT - 1)
#define FLASH_SNI_HEADER_SIZE         0x10
#define FLASH_EXT_SNI_HEADER_SIZE     0x10
#define FLASH_MAX_SNI_SIZE            0x200
#define FLASH_EXT_SNI_RESERVED_SIZE   0x1BC
#define FLASH_SNI_TABLE_SIZE          0x800
#define FLASH_SNI_BLOCKS_COUNT        10
#define FLASH_SOC_ECC_CACHE_SIZE      0x1100

typedef struct
{
    u8  u8_IDByteCnt;
    u8  au8_ID[15];
    u16 u16_BlkPageCnt;
    u16 u16_BlkCnt;
    u16 u16_SectorSize;
    u16 u16_PageSize;
    u16 u16_OobSize;
    u32 u32_BlockSize;
    u32 u32_Capacity;
    u8  u8_BL0PBA;
    u8  u8_BL1PBA;
    u8  u8_BLPINB;
    u8  u8_BAKCNT;
    u8  u8_BAKOFS;
} FLASH_NAND_INFO_t;

typedef struct
{
    u8  u8_rfc;
    u8  u8_dummy;
    u8  u8_programLoad;
    u8  u8_randomLoad;
    u32 u32_maxWaitTime;
} DRV_SPINAND_INFO_t;

u8 mdrv_spinand_find_sni_form_dram(u8 *sni_buf, u8 *sni_list);

void mdrv_spinand_info(FLASH_NAND_INFO_t *pst_flash_nand_info);
void mdrv_spinand_socecc_init(u8 *imi_cache_buf, u8 *miu_cache_buf);
u8   mdrv_spinand_hardware_init(u8 *sni_buf);
u8   mdrv_spinand_software_init(u8 *sni_buf);
u8   mdrv_spinand_deinit(void);

u8 mdrv_spinand_reset(void);
u8 mdrv_spinand_set_ecc_mode(u8 u8_enabled);
u8 mdrv_spinand_read_status(u8 *pu8_status);
u8 mdrv_spinand_read_id(u8 *pu8_data, u8 u8_size);
u8 mdrv_spinand_page_read_raw(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinand_page_program_raw(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinand_page_read(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinand_page_program(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinand_cache_program(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinand_block_erase(u32 u32_page);

u8 mdrv_spinand_is_support_otp(void);
u8 mdrv_spinand_is_support_sniecc(void);
u8 mdrv_spinand_is_support_ubibbm(void);
u8 mdrv_spinand_is_socecc(void);

u8  mdrv_spinand_get_otp_layout(u32 *start, u32 *length, u8 mode);
u8  mdrv_spinand_set_otp_mode(u8 u8_enabled);
u8  mdrv_spinand_get_otp_lock(void);
u8  mdrv_spinand_set_otp_lock(void);
u32 mdrv_spinand_read_otp(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);
u32 mdrv_spinand_write_otp(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size);

void mdrv_spinand_set_ecc_correct_flag(u8 u8_value);
u8   mdrv_spinand_get_ecc_correct_flag(void);
u8   mdrv_spinand_page_read_status(u32 u32_page);
u32  mdrv_spinand_block_isbad(u32 u32_page);
u32  mdrv_spinand_pages_read(u32 u32_page, u8 *pu8_data, u32 u32_size);
u32  mdrv_spinand_pages_program(u32 u32_page, u8 *pu8_data, u32 u32_size);
u32  mdrv_spinand_blocks_erase(u32 u32_page, u32 u32_size);
void mdrv_spinand_setup(void);
#endif
