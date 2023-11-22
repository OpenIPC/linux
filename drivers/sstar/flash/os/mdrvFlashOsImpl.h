/*
 * mdrvFlashOsImpl.h- Sigmastar
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

#ifndef _DRV_MDRVFLASHOSIMPL_H_
#define _DRV_MDRVFLASHOSIMPL_H_

#include <linux/delay.h>
#include <ms_platform.h>
#include <linux/crc32.h>
#include <linux/string.h>
#include <cam_os_wrapper.h>

typedef signed char        s8;
typedef unsigned char      u8;
typedef signed short       s16;
typedef unsigned short     u16;
typedef signed int         s32;
typedef unsigned int       u32;
typedef signed long long   s64;
typedef unsigned long long u64;

#define FLASH_IMPL_PRINTF(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define FLASH_IMPL_USDELAY(x)       udelay(x)
#define FLASH_IMPL_CRC32(x, y, z)   (crc32(~x, y, z) ^ 0xFFFFFFFF)
#define FLASH_IMPL_UNUSED_VAR(x)
#define FLASH_IMPL_INREG16(x) (*(volatile unsigned short *)(IO_ADDRESS(0x1F000000) + x))

#define BDMA_MIU2MIU       0x0
#define BDMA_PIU2MIU       0x1
#define BDMA_SPI2MIU       0x2
#define BDMA_IMI2MIU       0x3
#define BDMA_SPI2IMI       0x4
#define BDMA_MIU2SPI       0x5
#define DRV_BDMA_PROC_DONE 0x0

// software function
#if defined(CONFIG_MS_BDMA)
#define CONFIG_FLASH_BDMA
#endif

#ifdef CONFIG_SS_SOCECC
#define CONFIG_FLASH_SOCECC
#else
#define drv_spinand_soc_ecc_is_enable(x) 0
#define drv_spinand_soc_ecc_read(x)      0
#define drv_spinand_soc_ecc_write(x)     0
#define drv_spinand_soc_ecc_setup(x)
#endif

//#define CONFIG_FLASH_WRITE_BACK
//#define CONFIG_FLASH_ENABLE_BBM
//#define CONFIG_FLASH_NO_SUPPORT_MTD

// hardware function
//#define CONFIG_FLASH_FIX_BDMA_ALIGN
#define CONFIG_MIU_ADDR_ALIGN   0x10
#define CONFIG_FLASH_ADDR_ALIGN 0x10
#define CONFIG_FLASH_SIZE_ALIGN 0x10
#define CONFIG_FLASH_HW_CS

static inline u32 FLASH_IMPL_SIZE_TO_ALIGN_CACHE_SIZE(u32 u32_size)
{
    return ((u32_size + (63)) & ~(63));
}

static inline void FLASH_IMPL_MIUPIPE_FLUSH(void)
{
    // Chip_Flush_MIU_Pipe();
}

static inline void FLASH_IMPL_MEM_FLUSH(void *pu8_data, u32 u32_size)
{
    CamOsMemFlush((void *)pu8_data, u32_size);
}

static inline void FLASH_IMPL_MEM_INVALIDATE(void *pu8_data, u32 u32_size)
{
    CamOsMemInvalidate((void *)pu8_data, u32_size);
}

static inline u8 FLASH_IMPL_MEMCMP(u8 *cs, const u8 *ct, u8 u8_size)
{
    return !memcmp((const void *)cs, (const void *)ct, u8_size);
}

static inline void *FLASH_IMPL_MEMCPY(u8 *dst, const u8 *src, u32 u32_size)
{
    return memcpy((void *)dst, (const void *)src, u32_size);
}

static inline void *FLASH_IMPL_MEMSET(u8 *s, u32 c, u32 u32_size)
{
    return memset((void *)s, c, u32_size);
}
u8   FLASH_IMPL_BdmaTransfer(u8 ePath, u8 *pSrcAddr, u8 *nDstAddr, u32 nLength);
void FLASH_IMPL_PRINTF_HEX(const char *pu8_data, u32 u32_value, const char *pu8_data2);
void FLASH_IMPL_SHOW_ID(u8 *pu8_data, u8 u8_cnt);
u8   FLASH_IMPL_count_bits(u32 u32_x);
u32  FLASH_IMPL_checksum(u8 *pu8_data, u32 u32_size);

#endif
