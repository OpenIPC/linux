/*
 * drvSPINOR.h- Sigmastar
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
#ifndef _DRV_SPINOR_H_
#define _DRV_SPINOR_H_

typedef signed char        s8;
typedef unsigned char      u8;
typedef signed short       s16;
typedef unsigned short     u16;
typedef signed int         s32;
typedef unsigned int       u32;
typedef signed long long   s64;
typedef unsigned long long u64;

// please refer to the serial flash datasheet
#define SPI_NOR_CMD_READ     (0x03)
#define SPI_NOR_CMD_FASTREAD (0x0B)
#define SPI_NOR_CMD_RDID     (0x9F)
#define SPI_NOR_CMD_WREN     (0x06)
#define SPI_NOR_CMD_WRDIS    (0x04)
#define SPI_NOR_CMD_SE       (0x20)
#define SPI_NOR_CMD_32BE     (0x52)
#define SPI_NOR_CMD_64BE     (0xD8)
#define SPI_NOR_CMD_CE       (0xC7)

#define SPI_NOR_CMD_PP    (0x02)
#define SPI_NOR_CMD_QP    (0x32)
#define SPI_NOR_CMD_4PP   (0x38)
#define SPI_NOR_CMD_QR_6B (0x6B)
#define SPI_NOR_CMD_QR_EB (0xEB)

#define SPI_NOR_CMD_RDSR  (0x05)
#define SPI_NOR_CMD_RDSR2 (0x35)
#define SPI_NOR_CMD_RDSR3 (0x15)
#define SPI_NOR_CMD_WRSR  (0x01)
#define SPI_NOR_CMD_WRSR2 (0x31)
#define SPI_NOR_CMD_WRSR3 (0x11)

// support for 256Mb up MIX flash
#define SPI_NOR_CMD_WREAR    (0xC5)
#define SPI_NOR_CMD_RDEAR    (0xC8)
#define SPI_NOR_CMD_RESET    (0X99)
#define SPI_NOR_CMD_EN_RESET (0X66)

#define SPI_NOR_READ_DUMMY         0x08
#define SPI_NOR_DEFAULT_ERASE_SIZE (4 << 10)
#define SPI_NOR_BUSY               (0x01)
#define SPI_NOR_16MB               (1 << 24)
#define SPI_NOR_16MB_MASK          ((1 << 24) - 1)

#define SPI_NOR_ERASE_SIZE_4K  SPI_NOR_CMD_SE
#define SPI_NOR_ERASE_SIZE_32K SPI_NOR_CMD_32BE
#define SPI_NOR_ERASE_SIZE_64K SPI_NOR_CMD_64BE

typedef enum _SPINOR_ERROR_NUM
{
    ERR_SPINOR_SUCCESS = 0x00,
    ERR_SPINOR_RESET_FAIL,
    ERR_SPINOR_E_FAIL,
    ERR_SPINOR_P_FAIL,
    ERR_SPINOR_INVALID,
    ERR_SPINOR_TIMEOUT,
    ERR_SPINOR_DEVICE_FAILURE,
    ERR_SPINOR_BDMA_FAILURE,
} SPINOR_FLASH_ERRNO_e;

typedef struct
{
    u32 u32_pageSize;
    u32 u32_sectorSize;
    u32 u32_blockSize;
    u32 u32_capacity;
} FLASH_NOR_INFO_t;

#define FLASH_CIS_LOAD_OFFSET       0x08
#define FLASH_SEARCH_END            (0x10000)
#define FLASH_DEFAULT_SNI_OFFSET    0x20000
#define FLASH_SNI_SIZE              0x140
#define FLASH_EXT_SNI_RESERVED_SIZE 0x100
#define FLASH_SNI_HEADER_SIZE       0x10
#define FLASH_AVL_OFFSET_POSITION   0x2000
#define FLASH_SNI_TABLE_SIZE        0x1000

u8 mdrv_spinor_find_sni_form_dram(u8 *sni_buf, u8 *sni_list);

void mdrv_spinor_info(FLASH_NOR_INFO_t *pst_flash_nor_info);
u8   mdrv_spinor_hardware_init(u8 *sni_buf);
u8   mdrv_spinor_software_init(u8 *sni_buf);
u8   mdrv_spinor_deinit(void);

u8 mdrv_spinor_read(u32 u32_address, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinor_program(u32 u32_address, u8 *pu8_data, u32 u32_size);
u8 mdrv_spinor_erase(u32 u32_address, u32 u32_size);

#endif
