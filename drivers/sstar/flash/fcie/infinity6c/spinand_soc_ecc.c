/*
 * spinand_soc_ecc.c- Sigmastar
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

#include <mdrvFlashOsImpl.h>
#include <drvFCIE.h>
#include <drvSPINAND.h>
#include <drvFSP_QSPI.h>
#include <spinand_soc_ecc.h>

static u8 drv_spinand_soc_ecc_decode(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    DRV_FCIE_set_sectors(pst_fcie_cfg->u16_sector_cnt);
    DRV_FCIE_job_read_sectors();

    if (!DRV_FCIE_job_is_done(FCIE_WAIT_READ_TIME))
    {
        return ERR_SPINAND_TIMEOUT;
    }

    DRV_FCIE_clear_job();

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_soc_ecc_encode(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    DRV_FCIE_set_sectors(pst_fcie_cfg->u16_sector_cnt);
    DRV_FCIE_job_write_sector_start();

    if (!DRV_FCIE_job_is_done(FCIE_WAIT_READ_TIME))
    {
        return ERR_SPINAND_TIMEOUT;
    }

    DRV_FCIE_clear_job();

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_soc_ecc_get_status(void)
{
    return DRV_FCIE_get_ecc_status();
}

static void drv_spinand_soc_ecc_setup_read_address(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    DRV_FCIE_set_ecc_dir(FCIR_DIR_READ);

    if (ADDR_IS_IMI((u32)pst_fcie_cfg->data_cache))
    {
        DRV_FCIE_enable_imi();
        DRV_FCIE_set_brdg_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0xffffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->imi_oob_cache) & 0xffffffff);
        DRV_FCIE_set_read_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0xffffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->imi_oob_cache) & 0xffffffff);
    }
    else
    {
        DRV_FCIE_disable_imi();
        DRV_FCIE_set_brdg_address((u64)(unsigned long)(pst_fcie_cfg->data_cache_codec) & 0x0fffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->oob_cache_codec) & 0x0fffffff);
        DRV_FCIE_set_read_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0x0fffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->oob_cache) & 0x0fffffff);
    }
}

static void drv_spinand_soc_ecc_setup_write_address(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    DRV_FCIE_set_ecc_dir(FCIR_DIR_WRITE);

    if (ADDR_IS_IMI((u32)pst_fcie_cfg->data_cache))
    {
        DRV_FCIE_enable_imi();
        DRV_FCIE_set_brdg_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0xffffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->imi_oob_cache) & 0xffffffff);
        DRV_FCIE_set_write_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0xffffffff,
                                   (u64)(unsigned long)(pst_fcie_cfg->imi_oob_cache) & 0xffffffff);
    }
    else
    {
        DRV_FCIE_disable_imi();
        DRV_FCIE_set_brdg_address((u64)(unsigned long)(pst_fcie_cfg->data_cache_codec) & 0x0fffffff,
                                  (u64)(unsigned long)(pst_fcie_cfg->oob_cache_codec) & 0x0fffffff);
        DRV_FCIE_set_write_address((u64)(unsigned long)(pst_fcie_cfg->data_cache) & 0x0fffffff,
                                   (u64)(unsigned long)(pst_fcie_cfg->oob_cache) & 0x0fffffff);
    }
}

u8 drv_spinand_soc_ecc_is_enable(void)
{
    u16 OTP_SOC_ECC;

    OTP_SOC_ECC = FLASH_IMPL_INREG16((0x101F << 9) + (0x3E << 2)) & 0x03; // bit[1:0]   //OTP_SOC_ECC

    return (OTP_SOC_ECC == 0x00 || OTP_SOC_ECC == 0x03);
}

void drv_spinand_soc_ecc_setup(const stFlashConfig *pst_flash_config)
{
    DRV_FCIE_init();
    DRV_FCIE_reset();
    DRV_FCIE_stetup_ecc_ctrl(pst_flash_config->u16_pageSize, pst_flash_config->u16_spareSize,
                             pst_flash_config->u16_ecc_bytes,
                             DRV_FCIE_get_ecc_mode(pst_flash_config->u16_sector_size, pst_flash_config->u8_ecc_bits));
    DRV_FCIE_enable_ecc(1);
    DRV_FCIE_setup_brdg_ctrl(pst_flash_config->u16_sector_size, pst_flash_config->u16_ecc_bytes);
    DRV_FCIE_disable_imi();
}

u8 drv_spinand_soc_ecc_read(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    if (ADDR_IS_IMI((u32)pst_fcie_cfg->data_cache))
    {
        pst_fcie_cfg->read_cache(pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_oob_address,
                                 pst_fcie_cfg->imi_oob_cache,
                                 pst_fcie_cfg->u16_ecc_bytes * pst_fcie_cfg->u16_sector_cnt);
        pst_fcie_cfg->read_cache(pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_data_address,
                                 pst_fcie_cfg->data_cache,
                                 pst_fcie_cfg->u16_sector_size * pst_fcie_cfg->u16_sector_cnt);
    }
    else
    {
        pst_fcie_cfg->read_cache(pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_oob_address,
                                 pst_fcie_cfg->oob_cache_codec,
                                 pst_fcie_cfg->u16_ecc_bytes * pst_fcie_cfg->u16_sector_cnt);
        pst_fcie_cfg->read_cache(pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_data_address,
                                 pst_fcie_cfg->data_cache_codec,
                                 pst_fcie_cfg->u16_sector_size * pst_fcie_cfg->u16_sector_cnt);
    }

    drv_spinand_soc_ecc_setup_read_address(pst_fcie_cfg);
    drv_spinand_soc_ecc_decode(pst_fcie_cfg);

    pst_fcie_cfg->u8_ecc_status = drv_spinand_soc_ecc_get_status();

    return ERR_SPINAND_SUCCESS;
}

u8 drv_spinand_soc_ecc_write(SPINAND_FCIE_CFG_t *pst_fcie_cfg)
{
    if (ADDR_IS_IMI((u32)pst_fcie_cfg->data_cache))
    {
        FLASH_IMPL_MEMSET((void *)(pst_fcie_cfg->imi_oob_cache), 0xff, pst_fcie_cfg->u16_spare_size);
        FLASH_IMPL_MEM_FLUSH((void *)(pst_fcie_cfg->imi_oob_cache), pst_fcie_cfg->u16_spare_size);
    }
    else
    {
        FLASH_IMPL_MEMSET((void *)(pst_fcie_cfg->oob_cache), 0xff, pst_fcie_cfg->u16_spare_size);
        FLASH_IMPL_MEM_FLUSH((void *)(pst_fcie_cfg->oob_cache), pst_fcie_cfg->u16_spare_size);
    }

    drv_spinand_soc_ecc_setup_write_address(pst_fcie_cfg);
    drv_spinand_soc_ecc_encode(pst_fcie_cfg);

    if (ADDR_IS_IMI((u32)pst_fcie_cfg->data_cache))
    {
        FLASH_IMPL_MEM_INVALIDATE((void *)(pst_fcie_cfg->data_cache),
                                  pst_fcie_cfg->u16_sector_cnt * pst_fcie_cfg->u16_sector_size);
        FLASH_IMPL_MEM_INVALIDATE((void *)(pst_fcie_cfg->imi_oob_cache), pst_fcie_cfg->u16_spare_size);
        pst_fcie_cfg->program_load(
            pst_fcie_cfg->u8_programLoad, pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_data_address,
            pst_fcie_cfg->data_cache, pst_fcie_cfg->u16_sector_size * pst_fcie_cfg->u16_sector_cnt);
        pst_fcie_cfg->program_load(
            pst_fcie_cfg->u8_randomLoad, pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_oob_address,
            pst_fcie_cfg->imi_oob_cache, pst_fcie_cfg->u16_ecc_bytes * pst_fcie_cfg->u16_sector_cnt);
    }
    else
    {
        FLASH_IMPL_MEM_INVALIDATE((void *)(pst_fcie_cfg->data_cache_codec),
                                  pst_fcie_cfg->u16_sector_cnt * pst_fcie_cfg->u16_sector_size);
        FLASH_IMPL_MEM_INVALIDATE((void *)(pst_fcie_cfg->oob_cache_codec), pst_fcie_cfg->u16_spare_size);
        pst_fcie_cfg->program_load(
            pst_fcie_cfg->u8_programLoad, pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_data_address,
            pst_fcie_cfg->data_cache_codec, pst_fcie_cfg->u16_sector_size * pst_fcie_cfg->u16_sector_cnt);
        pst_fcie_cfg->program_load(
            pst_fcie_cfg->u8_randomLoad, pst_fcie_cfg->u16_col_address | pst_fcie_cfg->u16_oob_address,
            pst_fcie_cfg->oob_cache_codec, pst_fcie_cfg->u16_ecc_bytes * pst_fcie_cfg->u16_sector_cnt);
    }

    return ERR_SPINAND_SUCCESS;
}
