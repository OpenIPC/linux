/*
 * mdrvSpinandBbtBbm.h- Sigmastar
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
#ifndef _MDRVSPINANDBBTBBM_H_
#define _MDRVSPINANDBBTBBM_H_

#define MAX_BBT_TBL_SIZE 0x800

typedef enum SPINAND_BLKTYPE
{
    FACTORY_BAD_BLOCK     = 0x00,
    ECC_CORRECT_BLOCK     = 0x01,
    RUN_TIME_BAD_BLOCK    = 0x02,
    ECC_CORR_NOTFIX_BLOCK = 0x03,
    GOOD_BLOCK            = 0x0f
} BLKTYPE_e;

typedef enum SPINAND_BBTBBM_NUM
{
    E_BBTBBM_SUCCESS = 0x00,
    E_BBTBBM_FAIL,
    E_BBTBBM_NOSUPPORT
} BBTBBM_NUM_e;

u8 MDRV_BBT_init(u8 *bbt_buf);
u8 MDRV_BBT_get_blk_info(u32 u32Offset);
u8 MDRV_BBT_fill_blk_info(u32 u32Offset, BLKTYPE_e BlkType);
u8 MDRV_BBT_save_bbt(u8 u8_update);

#if defined(CONFIG_FLASH_ENABLE_BBM)
#include <mdrvParts.h>
void MDRV_BBM_init(void);
u8   MDRV_BBM_failure_recover(PARTS_INFO_t *pst_PartInfo, u8 *pu8_data);
u8   MDRV_BBM_load_part_bbm(PARTS_INFO_t *pst_PartInfo, u8 *pu8_data, u32 u32_size);
#else
static inline void MDRV_BBM_init(void) {}
static inline void MDRV_BBM_failure_recover(void) {}
static inline void MDRV_BBM_load_part_bbm(void) {}
#endif

#endif /* _MDRVSPINANDBBTBBM_H_ */
