/*
 * drvFCIE.c- Sigmastar
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
#include <halFCIE_reg.h>
#include <halFCIE.h>
#include <drvFCIE_st.h>
#include <drvSPINAND.h>

static void _DRV_FCIE_job_start(void)
{
    HAL_FCIE_write_bytes(NC_CTRL, HAL_FCIE_read_bytes(NC_CTRL) | BIT_NC_JOB_START);
}

static void _DRV_FCIE_addr_latch(void)
{
    HAL_FCIE_write_bytes(NC_ECC_SPARE_SIZE, HAL_FCIE_read_bytes(NC_ECC_SPARE_SIZE) | BIT_ADDR_LATCH);
}

u8 DRV_FCIE_get_bits_corrected(void)
{
    return ((HAL_FCIE_read_bytes(NC_ECC_STAT0) & BITS_ECC_CORRECT) >> 1);
}

u8 DRV_FCIE_get_ecc_status(void)
{
    u8 status;
    status = HAL_FCIE_read_bytes(NC_ECC_STAT2) & 0x03;
    // printf("[FCIE] NC_ECC_STAT0 = 0x%04x, NC_ECC_STAT2 = 0x%04x\r\n", HAL_FCIE_read_bytes(NC_ECC_STAT0), status);

    if (status == 0x00)
    {
        return ERR_SPINAND_SUCCESS;
    }
    if (status == 0x01)
    {
        return ERR_SPINAND_ECC_CORRECTED;
    }
    if (status == 0x02)
    {
        return ERR_SPINAND_ECC_NOT_CORRECTED;
    }
    return ERR_SPINAND_SUCCESS;
}

void DRV_FCIE_clear_job(void)
{
    HAL_FCIE_write_bytes(NC_MIE_EVENT, (HAL_FCIE_read_bytes(NC_MIE_EVENT) & BIT_NC_JOB_END));
    HAL_FCIE_write_bytes(NC_PART_MODE, 0);
}

u8 DRV_FCIE_job_is_done(u32 u32_timeout)
{
    u32 u32_count;

    for (u32_count = 0; u32_count < u32_timeout; u32_count++)
    {
        if ((HAL_FCIE_read_bytes(NC_MIE_EVENT) & BIT_NC_JOB_END)
            && !(HAL_FCIE_read_bytes(NC_ECC_SPARE_SIZE) & BIT_WRITE_MIU_BUSY))
        {
            return 1;
        }

        FLASH_IMPL_USDELAY(1);
    }

    FLASH_IMPL_PRINTF("[FCIE] Job timeout!\r\n");
    return 0;
}

void DRV_FCIE_disable_imi(void)
{
    HAL_FCIE_write_bytes(NC_BOOT_MODE, 0);
    HAL_FCIE3_write_bytes(0x30, (HAL_FCIE3_read_bytes(0x30) | (~BIT4)));
}

void DRV_FCIE_enable_imi(void)
{
    HAL_FCIE_write_bytes(NC_BOOT_MODE, BIT_IMI_SEL);
    HAL_FCIE3_write_bytes(0x30, (HAL_FCIE3_read_bytes(0x30) | BIT4));
}

void DRV_FCIE_read_cmd(void)
{
    HAL_FCIE_write_bytes(NC_AUXREG_ADR, AUXADR_INSTQUE);
    HAL_FCIE_write_bytes(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DIN);
}
void DRV_FCIE_set_sectors(u16 u16_spare_cnt)
{
    HAL_FCIE_write_bytes(NC_PART_MODE, BIT_PARTIAL_MODE_EN | ((u16_spare_cnt - 1) << 1));
}

void DRV_FCIE_job_write_sector_start(void)
{
    HAL_FCIE_write_bytes(NC_AUXREG_ADR, AUXADR_INSTQUE);
    HAL_FCIE_write_bytes(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DOUT);
    _DRV_FCIE_job_start();
}

void DRV_FCIE_job_read_sectors(void)
{
    HAL_FCIE_write_bytes(NC_AUXREG_ADR, AUXADR_INSTQUE);
    HAL_FCIE_write_bytes(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_SER_DIN);
    _DRV_FCIE_job_start();
}

void DRV_FCIE_set_write_address(u64 u64_data, u64 u64_spare)
{
    HAL_FCIE_write_bytes(NC_WDATA_DMA_ADR0, u64_data & 0xFFFF);
    HAL_FCIE_write_bytes(NC_WDATA_DMA_ADR1, u64_data >> 16);
    HAL_FCIE_write_bytes(NC_DATA_BASE_ADDR_MSB, ((u64_data >> 32) & 0x0f) << 8);
    HAL_FCIE_write_bytes(NC_WSPARE_DMA_ADR0, u64_spare & 0xFFFF);
    HAL_FCIE_write_bytes(NC_WSPARE_DMA_ADR1, u64_spare >> 16);
    HAL_FCIE_write_bytes(NC_SPARE_BASE_ADDR_MSB, ((u64_spare >> 32) & 0x0f) << 8);
}

void DRV_FCIE_set_read_address(u64 u64_data, u64 u64_spare)
{
    HAL_FCIE_write_bytes(NC_RDATA_DMA_ADR0, u64_data & 0xFFFF);
    HAL_FCIE_write_bytes(NC_RDATA_DMA_ADR1, u64_data >> 16);
    HAL_FCIE_write_bytes(NC_DATA_BASE_ADDR_MSB, (u64_data >> 32) & 0x0f);
    HAL_FCIE_write_bytes(NC_RSPARE_DMA_ADR0, u64_spare & 0xFFFF);
    HAL_FCIE_write_bytes(NC_RSPARE_DMA_ADR1, u64_spare >> 16);
    HAL_FCIE_write_bytes(NC_SPARE_BASE_ADDR_MSB, (u64_spare >> 32) & 0x0f);
}

void DRV_FCIE_set_brdg_address(u64 u64_data, u64 u64_spare)
{
    HAL_FCIE_write_bytes(NC_ECC_DATA_ADDR_L, (u64_data >> 4) & 0xFFFF);
    HAL_FCIE_write_bytes(NC_ECC_DATA_ADDR_H, (u64_data >> 20) & 0xFFFF);
    HAL_FCIE_write_bytes(NC_ECC_SPARE_ADDR_L, (u64_spare >> 4) & 0xFFFF);
    HAL_FCIE_write_bytes(NC_ECC_SPARE_ADDR_H, (u64_spare >> 20) & 0xFFFF);
    _DRV_FCIE_addr_latch();
}

void DRV_FCIE_enable_ecc(u8 u8_enabled)
{
    HAL_FCIE_write_bytes(NC_FUN_CTL, u8_enabled | BIT_NC_SPI_MODE);
    HAL_FCIE_write_bytes(NC_DDR_CTRL, BIT_NC_32B_MODE);
    HAL_FCIE_write_bytes(NC_MIE_INT_EN, u8_enabled);
}

u8 DRV_FCIE_get_ecc_mode(u16 u16_sector_size, u8 u8_ecc_bits)
{
    u8 u8_ecc_mode;

    u8_ecc_mode = ECC_BCH_512B_8_BITS;

    if (512 == u16_sector_size)
    {
        switch (u8_ecc_bits)
        {
            case 8:
                u8_ecc_mode = ECC_BCH_512B_8_BITS;
                break;
            case 16:
                u8_ecc_mode = ECC_BCH_512B_16_BITS;
                break;
            case 24:
                u8_ecc_mode = ECC_BCH_512B_24_BITS;
                break;
            case 32:
                u8_ecc_mode = ECC_BCH_512B_32_BITS;
                break;
            case 40:
                u8_ecc_mode = ECC_BCH_512B_40_BITS;
                break;
        }
    }
    else if (1024 == u16_sector_size)
    {
        switch (u8_ecc_bits)
        {
            case 8:
                u8_ecc_mode = ECC_BCH_1024B_8_BITS;
                break;
            case 16:
                u8_ecc_mode = ECC_BCH_1024B_16_BITS;
                break;
            case 24:
                u8_ecc_mode = ECC_BCH_1024B_24_BITS;
                break;
            case 32:
                u8_ecc_mode = ECC_BCH_1024B_32_BITS;
                break;
            case 40:
                u8_ecc_mode = ECC_BCH_1024B_40_BITS;
                break;
        }
    }

    return u8_ecc_mode;
}
/*
void DRV_FCIE_select_spare_dest(u16 u16_dest)
{
    HAL_FCIE_write_bytes(NC_SPARE, HAL_FCIE_read_bytes(NC_SPARE) | u16_dest);
}
*/

void DRV_FCIE_set_ecc_dir(u16 u16_ecc_dir)
{
    HAL_FCIE_write_bytes(NC_ECC_SPARE_SIZE, (HAL_FCIE_read_bytes(NC_ECC_SPARE_SIZE) & ~BIT_ECC_DIR) | u16_ecc_dir);
}

void DRV_FCIE_ecc_bypass(void)
{
    HAL_FCIE_write_bytes(NC_ECC_CTRL, HAL_FCIE_read_bytes(NC_ECC_CTRL) | BIT_BYPASS_ECC(1));
}

void DRV_FCIE_stetup_ecc_ctrl(u16 u16_page_size, u16 u16_spare_size, u16 u16_ecc_bytes, u8 u8_ecc_mode)
{
    u8 u8_page_mode;

    switch (u16_page_size)
    {
        case 512:
            u8_page_mode = 0;
            break;
        case 2048:
            u8_page_mode = 0x01;
            break;
        case 4096:
            u8_page_mode = 0x02;
            break;
        case 8192:
            u8_page_mode = 0x03;
            break;
        case (16 << 10):
            u8_page_mode = 0x04;
            break;
        case (32 << 10):
            u8_page_mode = 0x05;
            break;
        default:
            u8_page_mode = 0;
    }

    HAL_FCIE_write_bytes(NC_ECC_CTRL, u8_page_mode | (u8_ecc_mode << 3));
    HAL_FCIE_write_bytes(NC_SPARE_SIZE, u16_spare_size);
    HAL_FCIE_write_bytes(NC_SPARE, u16_ecc_bytes);
}

void DRV_FCIE_setup_brdg_ctrl(u16 u16_sector_size, u16 u16_spare_per_sector)
{
    u16 u16_ecc_data_mode;
    switch (u16_sector_size)
    {
        case 512:
            u16_ecc_data_mode = 0x00;
            break;
        case 1024:
            u16_ecc_data_mode = 0x01;
            break;
        default:
            u16_ecc_data_mode = 0x00;
    }
    HAL_FCIE_write_bytes(NC_ECC_MODE_SEL, u16_ecc_data_mode);
    HAL_FCIE_write_bytes(NC_ECC_SPARE_SIZE, u16_spare_per_sector);
}

u8 DRV_FCIE_reset(void)
{
    u16 u16_cnt;

    // soft reset
    HAL_FCIE_write_bytes(NC_FCIE_RST, ~BIT_FCIE_SOFT_RST); /* active low */

    // As reset is active, Check Reset Status from 0 -> 1
    u16_cnt = 0;
    do
    {
        FLASH_IMPL_USDELAY(1);

        if (0x1000 == u16_cnt++)
        {
            return 1;
        }

    } while (BIT_RST_STS_MASK != (HAL_FCIE_read_bytes(NC_FCIE_RST) & BIT_RST_STS_MASK));

    HAL_FCIE_write_bytes(NC_FCIE_RST, BIT_FCIE_SOFT_RST);

    // Restore reset bit, check reset status from 1 -> 0
    u16_cnt = 0;
    do
    {
        FLASH_IMPL_USDELAY(1);

        if (0x1000 == u16_cnt++)
        {
            return 1;
        }

    } while (0 != (HAL_FCIE_read_bytes(NC_FCIE_RST) & BIT_RST_STS_MASK));

    return 0;
}

void DRV_FCIE_init(void)
{
    HAL_FCIE_setup_clock();
    DRV_FCIE_reset();
}
