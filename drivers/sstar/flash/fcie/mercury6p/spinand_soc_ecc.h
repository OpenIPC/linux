/*
 * spinand_soc_ecc.h- Sigmastar
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

#ifndef _SPINAND_SOC_ECC_H_
#define _SPINAND_SOC_ECC_H_

#include <drvSPINAND.h>

#define ADDR_IS_IMI(u32Addr) \
    ((((u32Addr) & (0xF0000000)) == (0xA0000000)) && (((u32Addr) & (0x0FFFFFFF)) < (0x000D0000)))

typedef struct
{
    u16 u16_pageSize;
    u16 u16_spareSize;
    u16 u16_sector_size;
    u8  u8_ecc_bits;
    u8  u8_ecc_steps;
    u16 u16_ecc_bytes;
} stFlashConfig;

typedef struct
{
    u8 *                data_cache_codec;
    u8 *                oob_cache_codec;
    u8 *                data_cache;
    u8 *                oob_cache;
    u8 *                imi_data_cache;
    u8 *                imi_oob_cache;
    u32                 u32_data_size;
    u16                 u16_sector_cnt;
    u16                 u16_sector_size;
    u16                 u16_ecc_bytes;
    u16                 u16_spare_size;
    u16                 u16_col_address;
    u16                 u16_data_address;
    u16                 u16_oob_address;
    u16                 u16_data_offset;
    u8                  u8_ecc_status;
    u8                  u8_ecc_bitflip_count;
    DRV_SPINAND_INFO_t *pst_spinand_drv_info;
    u8 (*read_cache)(u16, u8 *, u32);
    u8 (*program_load)(u16, u8 *, u32);
    u8 (*random_program_load)(u16, u8 *, u32);
} SPINAND_FCIE_CFG_t;

#ifdef CONFIG_SS_SOCECC
u8   drv_spinand_soc_ecc_is_enable(void);
void drv_spinand_soc_ecc_setup(const stFlashConfig *pst_flash_config);
u8   drv_spinand_soc_ecc_read(SPINAND_FCIE_CFG_t *pst_fcie_cfg);
u8   drv_spinand_soc_ecc_write(SPINAND_FCIE_CFG_t *pst_fcie_cfg);
#else
#define drv_spinand_soc_ecc_is_enable(x) 0
#define drv_spinand_soc_ecc_read(x)      0
#define drv_spinand_soc_ecc_write(x)     0
#define drv_spinand_soc_ecc_setup(x)
#endif

#endif /* _SPINAND_SOC_ECC_H_ */
