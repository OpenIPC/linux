/*
 * drvSPINAND.c- Sigmastar
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
#include <drvSPINAND.h>
#include <mdrvFlashOsImpl.h>
#include <drvFSP_QSPI.h>
#include <mdrvSNI.h>
#ifdef CONFIG_FLASH_SOCECC
#include <spinand_soc_ecc.h>
#endif

#define NAND_PAGES_PER_BLOCK      g_pst_spinand_info->u16_BlkPageCnt
#define NAND_PAGES_PER_BLOCK_MASK (g_pst_spinand_info->u16_BlkPageCnt - 1)
#define NAND_BLOCK_COUNT          g_pst_spinand_info->u16_BlkCnt
#define NAND_PAGE_SIZE            g_pst_spinand_info->u16_PageByteCnt
#define NAND_BLOCK_SIZE           (NAND_PAGE_SIZE * g_pst_spinand_info->u16_BlkPageCnt)
#define NAND_PAGE_SIZE_MASK       (g_pst_spinand_info->u16_PageByteCnt - 1)
#define NAND_ECC_NO_CORRECTED     0
#define NAND_ECC_BITFLIPTHRESHOLD (g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_bitflipThreshold)
#define NAND_ECC_NOT_CORRECTED    (g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_eccNotCorrectStatus)
#define NAND_ECC_RESERVED         (g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_eccReserved)
#define NAND_ECC_STATUS_MASK      (g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_eccStatusMask)

#ifdef CONFIG_FLASH_SOCECC
static SPINAND_FCIE_CFG_t st_fcie_cfg;

static const stFlashConfig ast_flash_config_list[] = {{2048, 64, 512, 8, 4, 16},   {2048, 128, 512, 8, 4, 32},
                                                      {2048, 128, 512, 16, 4, 32}, {4096, 128, 512, 8, 8, 16},
                                                      {4096, 256, 512, 8, 8, 32},  {4096, 256, 512, 16, 8, 32}};
static u8 *                g_socecc_cache_buf      = NULL;
static u8                  g_socecc_type           = 0;
#endif

static const u8 au8_basicMagicData[] = {0x4D, 0x53, 0x54, 0x41, 0x52, 0x53, 0x45, 0x4D,
                                        0x49, 0x55, 0x53, 0x46, 0x44, 0x43, 0x49, 0x53};
static const u8 au8_extMagicData[]   = {'S', 'S', 'T', 'A', 'R', 'S', 'E', 'M', 'I', 'S', 'N', 'I', 'V', '0', '0', '0'};

static SPINAND_SNI_t *              g_pst_spinand_sni               = NULL;
static SPINAND_INFO_t *             g_pst_spinand_info              = NULL;
static SPINAND_EXT_INFO_t *         g_pst_spinand_ext_info          = NULL;
static SPINAND_EXT_CONFIGURATION_t *g_pst_spinand_ext_configuration = NULL;

static u8 u8_ecc_corrected;

static DRV_SPINAND_INFO_t st_spinand_drv_info;

static u8 (*_drv_spinand_read_from_cache)(u16, u8 *, u32);
static u8 (*_drv_spinand_program_load_data)(u8 *, u8, u8 *, u32);

static u32 (*drv_spinand_read)(u32, u8 *, u32);
static u8 (*drv_spinand_check_page_status)(void);

void __attribute__((weak)) PalSpiflash_SetClk(u8 u8Clk)
{
    FLASH_IMPL_UNUSED_VAR(u8Clk);
}

static u8 drv_spinand_get_cmd_mode(u8 u8_cmd)
{
    switch (u8_cmd)
    {
        case SPI_NAND_CMD_QPL:
        case 0x6B:
        case SPI_NAND_CMD_QRPL:
        case 0xEB:
            return FSP_QUAD_MODE;
        case 0x3B:
        case 0xBB:
            return FSP_DUAL_MODE;
        default:
            return FSP_SINGLE_MODE;
    }
}

static u8 drv_spinand_return_status(u8 u8_status)
{
    if (ERR_SPINAND_INVALID <= u8_status)
    {
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return u8_status;
}

static u16 drv_spinand_offset_wrap(u32 u32_page, u8 u8_access)
{
    u16                    u16_plane_select = 0;
    SPINAND_EXT_PROFILE_t *pst_profile      = NULL;

    if (!(u32_page & NAND_PAGES_PER_BLOCK))
    {
        return 0;
    }

    if (g_pst_spinand_ext_info)
    {
        pst_profile      = &g_pst_spinand_ext_info->st_profile;
        u16_plane_select = SPINAND_PLANE_SELECT & pst_profile->u16_flags;
    }

    if ((!u16_plane_select) && (1 < g_pst_spinand_info->u8PlaneCnt))
    {
        return (1 << 12);
    }

    if (!u16_plane_select)
    {
        return 0;
    }

    switch (u8_access)
    {
        case SPINAND_READ:
            return ((u16)(pst_profile->st_access.st_read.u32_address));
        case SPINAND_PROGRAM:
            return ((u16)(pst_profile->st_access.st_program.u32_address));
        case SPINAND_RANDOM:
            return ((u16)(pst_profile->st_access.st_random.u32_address));
    }

    return 0;
}

#if defined(CONFIG_FLASH_FIX_DI_PULL_DOWN)
static u8 drv_spinand_receive_data(u8 *pu8_buf, u32 u32_size)
{
    u8 u8_rd_size;

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    while (0 != u32_size)
    {
        DRV_FSP_set_which_cmd_size(1, 0);
        u8_rd_size = DRV_FSP_set_rbf_size_after_which_cmd(1, u32_size);

        DRV_FSP_trigger();
        if (!DRV_FSP_is_done())
        {
            break;
        }

        u32_size -= DRV_FSP_read_rbf(pu8_buf, u8_rd_size);
        pu8_buf += u8_rd_size;

        DRV_FSP_clear_trigger();
    }

    if (0 != u32_size)
    {
        return ERR_SPINAND_TIMEOUT;
    }

    return ERR_SPINAND_SUCCESS;
}
#endif

static u8 drv_spinand_read_after_write(u8 *pu8_wbuf, u32 u32_wsize, u8 *pu8_rbuf, u32 u32_rsize)
{
    u8 u8_wr_size;
    u8 u8_rd_size;

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    u8_wr_size = DRV_FSP_set_which_cmd_size(1, u32_wsize);
    u8_rd_size = DRV_FSP_set_rbf_size_after_which_cmd(1, u32_rsize);

    DRV_FSP_write_wbf(pu8_wbuf, u8_wr_size);

    DRV_FSP_trigger();
    if (!DRV_FSP_is_done())
    {
        return 0;
    }

    DRV_FSP_read_rbf(pu8_rbuf, u8_rd_size);

    DRV_FSP_clear_trigger();

    return u8_rd_size;
}

#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinand_simple_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
{
    u8 u8_wr_size;

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    u8_wr_size = DRV_FSP_set_which_cmd_size(1, u8_wsize + u32_size);

    if (u8_wsize && pu8_wbuf)
    {
        u8_wr_size -= DRV_FSP_write_wbf(pu8_wbuf, u8_wsize);
    }

    if (u8_wr_size && pu8_buf)
    {
        DRV_FSP_write_wbf(pu8_buf, u8_wr_size);
    }

    DRV_FSP_trigger();

    if (!DRV_FSP_is_done())
    {
        return ERR_SPINAND_TIMEOUT;
    }
    DRV_FSP_clear_trigger();

    return ERR_SPINAND_SUCCESS;
}
#else
static u8 drv_spinand_simple_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
{
    u8 u8_wr_size;

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    while (0 != u32_size)
    {
        u8_wr_size = DRV_FSP_set_which_cmd_size(1, u32_size);

        if (pu8_buf)
        {
            DRV_FSP_write_wbf(pu8_buf, u8_wr_size);
            pu8_buf += u8_wr_size;
        }

        DRV_FSP_trigger();

        if (!DRV_FSP_is_done())
        {
            break;
        }

        DRV_FSP_clear_trigger();
        u32_size -= u8_wr_size;
    }

    if (0 != u32_size)
    {
        return ERR_SPINAND_TIMEOUT;
    }

    return ERR_SPINAND_SUCCESS;
}
#endif

static u8 drv_spinand_complete_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
{
    u8 u8_status;

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinand_simple_transmission(pu8_wbuf, u8_wsize, pu8_buf, u32_size);
    DRV_QSPI_pull_cs(1);

    return drv_spinand_return_status(u8_status);
}

static u8 drv_spinand_complete_get_features(u8 u8_command, u8 u8_address, u8 *pu8_data, u8 u8_size)
{
    u8  u8_status = ERR_SPINAND_SUCCESS;
    u16 u16_cmd;

    u16_cmd = u8_command | (u8_address << 8);

    DRV_QSPI_pull_cs(0);

    if (!drv_spinand_read_after_write((u8 *)&u16_cmd, 2, pu8_data, (u32)u8_size))
    {
        u8_status = ERR_SPINAND_TIMEOUT;
    }

    DRV_QSPI_pull_cs(1);

    return drv_spinand_return_status(u8_status);
}

#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinand_complete_set_features(u8 u8_address, u8 *pu8_data, u8 u8_size)
{
    u8  u8_status;
    u16 u16_cmd;

    u16_cmd = SPI_NAND_CMD_SF | (u8_address << 8);

    u8_status = drv_spinand_simple_transmission((u8 *)&u16_cmd, 2, pu8_data, u8_size);

    return drv_spinand_return_status(u8_status);
}
#else
static u8 drv_spinand_complete_set_features(u8 u8_address, u8 *pu8_data, u8 u8_size)
{
    u8  u8_status;
    u16 u16_cmd;

    u16_cmd = SPI_NAND_CMD_SF | (u8_address << 8);

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinand_simple_transmission(NULL, 0, (u8 *)&u16_cmd, 2);

    if (ERR_SPINAND_SUCCESS == u8_status)
    {
        u8_status = drv_spinand_simple_transmission(NULL, 0, pu8_data, u8_size);
    }

    DRV_QSPI_pull_cs(1);
    return drv_spinand_return_status(u8_status);
}
#endif

static u8 drv_spinand_complete_read_id(u8 *pu8_ids, u8 u8_bytes)
{
    u8  u8_status = ERR_SPINAND_SUCCESS;
    u16 u16_cmd;

    u16_cmd = SPI_NAND_CMD_RDID | (0x00 << 8);

    DRV_QSPI_pull_cs(0);

    if (!drv_spinand_read_after_write((u8 *)&u16_cmd, 2, pu8_ids, u8_bytes))
    {
        u8_status = ERR_SPINAND_TIMEOUT;
    }

    DRV_QSPI_pull_cs(1);

    return drv_spinand_return_status(u8_status);
}

static u8 drv_spinand_complete_reset(void)
{
    u8 u8_cmd = SPI_NAND_CMD_RESET;

    return drv_spinand_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinand_complete_write_enable(void)
{
    u8 u8_cmd = SPI_NAND_CMD_WREN;

    return drv_spinand_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinand_complete_execute(void)
{
    u8 u8_cmd;
    u8_cmd = SPI_NAND_CMD_PE;

    return drv_spinand_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinand_complete_page_read(u32 u32_page_address)
{
    u32 u32_cmd;

    u32_cmd = SPI_NAND_CMD_PGRD | (((u32_page_address >> 16) & 0xff) << 8) | (((u32_page_address >> 8) & 0xff) << 16)
              | ((u32_page_address & 0xff) << 24);

    return drv_spinand_complete_transmission(NULL, 0, (u8 *)&u32_cmd, 4);
}

static u8 drv_spinand_complete_program_execute(u32 u32_page_address)
{
    u32 u32_cmd;

    u32_cmd = 0;
    u32_cmd = SPI_NAND_CMD_PE | (((u32_page_address >> 16) & 0xff) << 8) | ((u32_page_address >> 8) & 0xff) << 16
              | (u32_page_address & 0xff) << 24;

    return drv_spinand_complete_transmission(NULL, 0, (u8 *)&u32_cmd, 4);
}

static u8 drv_spinand_complete_block_erase(u32 u32_page_address)
{
    u32 u32_cmd;

    u32_cmd = 0;
    u32_cmd = SPI_NAND_CMD_BE | (((u32_page_address >> 16) & 0xff) << 8) | ((u32_page_address >> 8) & 0xff) << 16
              | (u32_page_address & 0xff) << 24;

    return drv_spinand_complete_transmission(NULL, 0, (u8 *)&u32_cmd, 4);
}

static u8 drv_spinand_check_status(u8 *pu8_status)
{
    u8  u8_i = 0;
    u8  u8_status;
    u32 u32_timeout;

    u32_timeout = st_spinand_drv_info.u32_maxWaitTime;

    do
    {
        u8_status = 0;

        if (ERR_SPINAND_SUCCESS != drv_spinand_complete_get_features(SPI_NAND_CMD_GF, SPI_NAND_REG_STAT, &u8_status, 1))
        {
            return ERR_SPINAND_DEVICE_FAILURE;
        }

        if (!(u8_status & SPI_NAND_STAT_OIP))
        {
            u8_i++;
            if (u8_i == 2)
                break;
        }
        else
            u8_i = 0;

        FLASH_IMPL_USDELAY(10);

        if (10 < u32_timeout)
        {
            u32_timeout -= 10;
        }
        else
        {
            u32_timeout = 0;
        }

        if (0 == u32_timeout)
        {
            return ERR_SPINAND_TIMEOUT;
        }

    } while (1);

    if (u8_status & SPI_NAND_STAT_P_FAIL)
    {
        return ERR_SPINAND_P_FAIL;
    }

    if (u8_status & SPI_NAND_STAT_E_FAIL)
    {
        return ERR_SPINAND_E_FAIL;
    }

    if (pu8_status)
        *pu8_status = u8_status;

    return ERR_SPINAND_SUCCESS;
}

#if defined(CONFIG_FLASH_FIX_DI_PULL_DOWN)
static u8 drv_spinand_complete_read_from_cache_by_riu(u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8 au8_cmd[4];
    u8 u8_dummy;
    u8 u8_status;

    DRV_QSPI_pull_cs(0);
    do
    {
        au8_cmd[0] = (u16_col_address >> 8) & 0xff;
        au8_cmd[1] = u16_col_address & 0xff;

        if (ERR_SPINAND_SUCCESS
            != (u8_status = drv_spinand_simple_transmission(NULL, 0, &st_spinand_drv_info.u8_rfc, 1)))
        {
            break;
        }

        u8_dummy = (st_spinand_drv_info.u8_dummy * 1) / 8;

        if (0xEB == st_spinand_drv_info.u8_rfc)
        {
            u8_dummy = (st_spinand_drv_info.u8_dummy * 4) / 8;
            DRV_FSP_set_access_mode(drv_spinand_get_cmd_mode(st_spinand_drv_info.u8_rfc));
        }
        else if (0xBB == st_spinand_drv_info.u8_rfc)
        {
            u8_dummy = (st_spinand_drv_info.u8_dummy * 2) / 8;
            DRV_FSP_set_access_mode(drv_spinand_get_cmd_mode(st_spinand_drv_info.u8_rfc));
        }

        if (ERR_SPINAND_SUCCESS != drv_spinand_simple_transmission(NULL, 0, au8_cmd, 2))
        {
            break;
        }

        if (ERR_SPINAND_SUCCESS != drv_spinand_simple_transmission(NULL, 0, NULL, u8_dummy))
        {
            break;
        }

        DRV_FSP_set_access_mode(drv_spinand_get_cmd_mode(st_spinand_drv_info.u8_rfc));

        if (ERR_SPINAND_SUCCESS == u8_status)
        {
            u8_status = drv_spinand_receive_data(pu8_data, u32_size);
        }

        DRV_FSP_set_access_mode(FSP_SINGLE_MODE);
    } while (0);

    DRV_QSPI_pull_cs(1);

    if (drv_spinand_soc_ecc_is_enable())
        FLASH_IMPL_MEM_FLUSH(pu8_data, FLASH_IMPL_SIZE_TO_ALIGN_CACHE_SIZE(u32_size));

    return drv_spinand_return_status(u8_status);
}
#else
static u8 drv_spinand_complete_read_from_cache_by_riu(u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8 au8_cmd[4];
    u8 u8_rsize;
    u8 u8_status = ERR_SPINAND_SUCCESS;

    do
    {
        au8_cmd[0] = SPI_NAND_CMD_FRFC;             // command
        au8_cmd[1] = (u16_col_address >> 8) & 0xff; // address
        au8_cmd[2] = u16_col_address & 0xff;        // address
        au8_cmd[3] = 0;                             // dummy

        DRV_FSP_set_access_mode(drv_spinand_get_cmd_mode(au8_cmd[0]));
        DRV_QSPI_pull_cs(0);

        u8_rsize = drv_spinand_read_after_write(au8_cmd, 4, pu8_data, u32_size);

        DRV_QSPI_pull_cs(1);

        u16_col_address += u8_rsize;
        pu8_data += u8_rsize;
        u32_size -= u8_rsize;
    } while (u32_size && u8_rsize);

    DRV_FSP_set_access_mode(FSP_SINGLE_MODE);

    if (!u8_rsize)
        u8_status = ERR_SPINAND_TIMEOUT;

    if (drv_spinand_soc_ecc_is_enable())
        FLASH_IMPL_MEM_FLUSH(pu8_data, FLASH_IMPL_SIZE_TO_ALIGN_CACHE_SIZE(u32_size));

    return drv_spinand_return_status(u8_status);
}
#endif

#if defined(CONFIG_FLASH_BDMA)
static u8 drv_spinand_complete_read_from_cache_by_bdma(u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8            u8_status       = ERR_SPINAND_SUCCESS;
    unsigned long u32_col_address = u16_col_address;

    FLASH_IMPL_MEM_INVALIDATE((void *)pu8_data, u32_size);
    DRV_QSPI_pull_cs(0);
    DRV_QSPI_use_2bytes_address_mode(DRV_QSPI_cmd_to_mode(st_spinand_drv_info.u8_rfc), st_spinand_drv_info.u8_dummy);

    if (DRV_BDMA_PROC_DONE != FLASH_IMPL_BdmaTransfer(BDMA_SPI2MIU, (u8 *)u32_col_address, pu8_data, u32_size))
    {
        u8_status = ERR_SPINAND_BDMA_FAILURE;
    }

    DRV_QSPI_use_2bytes_address_mode(DRV_QSPI_cmd_to_mode(SPI_NAND_CMD_RFC), SPI_NAND_CMD_RFC_DUMMY);
    DRV_QSPI_pull_cs(1);
    FLASH_IMPL_MEM_INVALIDATE((void *)pu8_data, u32_size);
    return drv_spinand_return_status(u8_status);
}
#endif

static u8 drv_spinand_read_from_cache(u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINAND_SUCCESS;
    u32 u32_rsize;

#if !defined(CONFIG_FLASH_FIX_BDMA_ALIGN)
    if (0 != ((unsigned long)pu8_data % 16) || 0 != (u16_col_address % 16))
    {
        u8_status = drv_spinand_complete_read_from_cache_by_riu(u16_col_address, pu8_data, u32_size);
    }
    else
#endif
    {
        do
        {
            if (0 != ((unsigned long)pu8_data % 64))
            {
                u32_rsize = 64 - ((unsigned long)pu8_data % 64);
                u32_rsize = (u32_rsize > u32_size) ? u32_size : u32_rsize;
                u8_status = drv_spinand_complete_read_from_cache_by_riu(u16_col_address, pu8_data, u32_rsize);
                u16_col_address += u32_rsize;
                pu8_data += u32_rsize;
                u32_size -= u32_rsize;
            }

            if (u8_status != ERR_SPINAND_SUCCESS || !u32_size)
                break;

            u32_rsize = u32_size & ~0x3F;
            if (u32_rsize)
            {
                u8_status = _drv_spinand_read_from_cache(u16_col_address, pu8_data, u32_rsize);
                u16_col_address += u32_rsize;
                pu8_data += u32_rsize;
                u32_size -= u32_rsize;
            }

            if (u8_status != ERR_SPINAND_SUCCESS || !u32_size)
                break;

            if (u32_size)
                u8_status = drv_spinand_complete_read_from_cache_by_riu(u16_col_address, pu8_data, u32_size);
        } while (0);
    }

    return drv_spinand_return_status(u8_status);
}

#if defined(CONFIG_FLASH_BDMA)
#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinand_program_load_data_by_bdma(u8 *pu8_cmd, u8 u8_wsize, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINAND_SUCCESS;
    u32 u32_write_bytes;

    FLASH_IMPL_MEM_FLUSH((void *)pu8_data, u32_size);
    FLASH_IMPL_MIUPIPE_FLUSH();

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    if (u8_wsize != DRV_FSP_set_which_cmd_size(1, u8_wsize))
        return ERR_SPINAND_DEVICE_FAILURE;

    while (0 != u32_size)
    {
        DRV_FSP_write_wbf(pu8_cmd, u8_wsize);
        u32_write_bytes = DRV_FSP_enable_outside_wbf(FSP_OUTSIDE_MODE_BDMA, 0, u8_wsize, u32_size);
        DRV_FSP_trigger();

        if (DRV_BDMA_PROC_DONE != FLASH_IMPL_BdmaTransfer(BDMA_MIU2SPI, pu8_data, 0, u32_size))
        {
            printk("ERR_SPINAND_BDMA_FAILURE\r\n");
            u8_status = ERR_SPINAND_BDMA_FAILURE;
            break;
        }

        if (!DRV_FSP_is_done())
        {
            u8_status = ERR_SPINAND_TIMEOUT;
        }

        DRV_FSP_clear_trigger();

        pu8_data += u32_write_bytes;
        u32_size -= u32_write_bytes;
    }

    DRV_FSP_disable_outside_wbf();
    return drv_spinand_return_status(u8_status);
}
#else
static u8 drv_spinand_program_load_data_by_bdma(u8 *pu8_cmd, u8 u8_wsize, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINAND_SUCCESS;
    u32 u32_write_bytes;

    FLASH_IMPL_MEM_FLUSH((void *)pu8_data, u32_size);
    FLASH_IMPL_MIUPIPE_FLUSH();

    while (0 != u32_size)
    {
        u32_write_bytes = DRV_FSP_enable_outside_wbf(FSP_OUTSIDE_MODE_BDMA, 0, 0, u32_size);
        DRV_FSP_init(FSP_USE_SINGLE_CMD);
        DRV_FSP_set_which_cmd_size(1, 0);

        DRV_FSP_trigger();

        if (DRV_BDMA_PROC_DONE != FLASH_IMPL_BdmaTransfer(BDMA_MIU2SPI, pu8_data, 0, u32_write_bytes))
        {
            u8_status = ERR_SPINAND_BDMA_FAILURE;
            break;
        }

        if (!DRV_FSP_is_done())
        {
            u8_status = ERR_SPINAND_TIMEOUT;
        }

        DRV_FSP_clear_trigger();
        pu8_data += u32_write_bytes;
        u32_size -= u32_write_bytes;
    }

    DRV_FSP_disable_outside_wbf();
    return drv_spinand_return_status(u8_status);
}
#endif
#endif

static u8 drv_spinand_complete_read_from_cache(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    if (ERR_SPINAND_SUCCESS
        != drv_spinand_read_from_cache(u16_column | drv_spinand_offset_wrap(u32_page, SPINAND_READ), pu8_data,
                                       u32_size))
    {
        FLASH_IMPL_PRINTF_HEX("[FLASH_ER] RFC fail, page = 0x", u32_page, "\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_load_data_to_cache(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status = ERR_SPINAND_SUCCESS;
#if defined(CONFIG_FLASH_HW_CS)
    u8 u8_bytes_aline = 0;
#endif
    unsigned long u32_bytes_to_load;

    while (0 != u32_size)
    {
        u32_bytes_to_load = u32_size;

        if (0 != ((unsigned long)pu8_data % 16))
        {
            u32_bytes_to_load = 16 - ((unsigned long)pu8_data % 16);

            if (u32_size < u32_bytes_to_load)
            {
                u32_bytes_to_load = u32_size;
            }
        }

        if (u32_bytes_to_load < 64)
        {
#if defined(CONFIG_FLASH_HW_CS)
            u8_status      = drv_spinand_simple_transmission(pu8_wbuf, u8_wsize, pu8_data, u32_bytes_to_load);
            u8_bytes_aline = 1;
#else
            u8_status = drv_spinand_simple_transmission(NULL, 0, pu8_data, u32_bytes_to_load);
#endif
        }
        else
        {
#if defined(CONFIG_FLASH_HW_CS)
            if (u8_bytes_aline)
            {
                u8_status      = _drv_spinand_program_load_data(NULL, 0, pu8_data, u32_bytes_to_load);
                u8_bytes_aline = 0;
            }
            else
                u8_status = _drv_spinand_program_load_data(pu8_wbuf, u8_wsize, pu8_data, u32_bytes_to_load);
#else
            u8_status = _drv_spinand_program_load_data(NULL, 0, pu8_data, u32_bytes_to_load);
#endif
        }

        if (ERR_SPINAND_SUCCESS != u8_status)
        {
            break;
        }

        u32_size -= u32_bytes_to_load;
        pu8_data += u32_bytes_to_load;
    }

    return drv_spinand_return_status(u8_status);
}

#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinand_complete_program_load(u8 u8_cmd, u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status;
    u32 u32_cmd;

    u32_cmd = 0;
    u32_cmd |= u8_cmd;
    u32_cmd |= (u16_col_address >> 8 & 0xff) << 8;
    u32_cmd |= (u16_col_address & 0xff) << 16;

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinand_load_data_to_cache((u8 *)&u32_cmd, 3, pu8_data, u32_size);
    DRV_QSPI_pull_cs(1);

    return drv_spinand_return_status(u8_status);
}
#else
static u8 drv_spinand_complete_program_load(u8 u8_cmd, u16 u16_col_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status;
    u32 u32_cmd;

    u32_cmd = 0;
    u32_cmd |= u8_cmd;
    u32_cmd |= (u16_col_address >> 8 & 0xff) << 8;
    u32_cmd |= (u16_col_address & 0xff) << 16;

    DRV_QSPI_pull_cs(0);
    if (ERR_SPINAND_SUCCESS == (u8_status = drv_spinand_simple_transmission(NULL, 0, (u8 *)&u32_cmd, 3)))
    {
        DRV_FSP_set_access_mode(drv_spinand_get_cmd_mode(u8_cmd));
        u8_status = drv_spinand_load_data_to_cache(NULL, 0, pu8_data, u32_size);
        DRV_FSP_set_access_mode(FSP_SINGLE_MODE);
    }
    DRV_QSPI_pull_cs(1);

    return drv_spinand_return_status(u8_status);
}
#endif

static void drv_spinand_setup_timeout(u32 u32_timeout)
{
    st_spinand_drv_info.u32_maxWaitTime = u32_timeout;
}

static void drv_spinand_use_bdma(u8 u8_enabled)
{
#if defined(CONFIG_FLASH_HW_CS)
    _drv_spinand_read_from_cache   = drv_spinand_complete_read_from_cache_by_bdma;
    _drv_spinand_program_load_data = drv_spinand_program_load_data_by_bdma;
#else
#if defined(CONFIG_FLASH_BDMA)
    if (u8_enabled)
    {
        _drv_spinand_read_from_cache   = drv_spinand_complete_read_from_cache_by_bdma;
        _drv_spinand_program_load_data = drv_spinand_program_load_data_by_bdma;
    }
    else
#endif
    {
        _drv_spinand_read_from_cache   = drv_spinand_complete_read_from_cache_by_riu;
        _drv_spinand_program_load_data = drv_spinand_simple_transmission;
    }
#endif
}

static void drv_spinand_setup_access(u8 u8_read, u8 u8_dummy_cycle, u8 u8_program, u8 u8_random)
{
    if (st_spinand_drv_info.u8_rfc != u8_read || st_spinand_drv_info.u8_dummy != u8_dummy_cycle)
    {
        st_spinand_drv_info.u8_rfc   = u8_read;
        st_spinand_drv_info.u8_dummy = u8_dummy_cycle;
    }

    if (st_spinand_drv_info.u8_programLoad != u8_program || st_spinand_drv_info.u8_randomLoad != u8_random)
    {
#if defined(CONFIG_FLASH_HW_CS)
        st_spinand_drv_info.u8_programLoad = 0x02;
#else
        st_spinand_drv_info.u8_programLoad = u8_program;
#endif
        st_spinand_drv_info.u8_randomLoad = u8_random;
        FLASH_IMPL_PRINTF_HEX("[SPINAND] Program with command 0x", st_spinand_drv_info.u8_programLoad, ".\r\n");
        FLASH_IMPL_PRINTF_HEX("[SPINAND] Random with command 0x", st_spinand_drv_info.u8_randomLoad, ".\r\n");
    }
}

static u8 drv_spinand_init(void)
{
    DRV_FSP_QSPI_init();

#if defined(CONFIG_FLASH_HW_CS)
    DRV_QSPI_use_sw_cs(0);
    DRV_QSPI_set_timeout(1, 0);
#endif

    st_spinand_drv_info.u8_rfc          = SPI_NAND_CMD_RFC;
    st_spinand_drv_info.u8_dummy        = SPI_NAND_CMD_RFC_DUMMY;
    st_spinand_drv_info.u8_programLoad  = SPI_NAND_CMD_PL;
    st_spinand_drv_info.u8_randomLoad   = SPI_NAND_CMD_RPL;
    st_spinand_drv_info.u32_maxWaitTime = -1;
    DRV_QSPI_use_2bytes_address_mode(DRV_QSPI_cmd_to_mode(st_spinand_drv_info.u8_rfc), st_spinand_drv_info.u8_dummy);
    _drv_spinand_read_from_cache   = drv_spinand_complete_read_from_cache_by_riu;
    _drv_spinand_program_load_data = drv_spinand_simple_transmission;
    return mdrv_spinand_reset();
}

static u8 drv_spinand_count_bits(u32 u32_x)
{
    u8 u8_i = 0;

    while (u32_x)
    {
        u8_i++;
        u32_x >>= 1;
    }

    return (u8_i - 1);
}

static u32 drv_spinand_get_bytes_left(u32 u32_bytes, u32 u32_limit)
{
    if (u32_limit > u32_bytes)
    {
        return u32_bytes;
    }

    return u32_limit;
}

static u32 drv_spinand_align_next_block(u32 u32_page)
{
    return ((u32_page + NAND_PAGES_PER_BLOCK) & ~NAND_PAGES_PER_BLOCK_MASK);
}

static u32 drv_spinand_offset_to_align_page_size(u32 u32_offset)
{
    return ((u32_offset + NAND_PAGE_SIZE_MASK) & ~NAND_PAGE_SIZE_MASK);
}

static u32 drv_spinand_offset_to_page_address(u32 u32_offset)
{
    return ((u32_offset & ~NAND_PAGE_SIZE_MASK) >> drv_spinand_count_bits(NAND_PAGE_SIZE));
}

static u8 drv_spinand_wait_device_available(void)
{
    u8 u8_status;

    u8_status = drv_spinand_check_status(NULL);

    if (ERR_SPINAND_TIMEOUT <= u8_status)
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Check status fail!");
        return u8_status;
    }

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_wait_status(FLASH_CMD_SET_t *status)
{
    u32 u32_status;
    u32 u32_timeout;

    u32_timeout = st_spinand_drv_info.u32_maxWaitTime;

    do
    {
        if (ERR_SPINAND_SUCCESS
            != drv_spinand_complete_get_features(SPI_NAND_CMD_GF, status->u32_address, (u8 *)&u32_status,
                                                 status->u16_dataBytes))
        {
            return ERR_SPINAND_DEVICE_FAILURE;
        }

        u32_timeout--;

        if (0 == u32_timeout)
        {
            return ERR_SPINAND_TIMEOUT;
        }

    } while (status->u16_value & u32_status);

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_next_page_read(FLASH_CMD_SET_t *next)
{
    u8 u8_i;
    u8 au8_cmd[4];

    u8_i            = 0;
    au8_cmd[u8_i++] = next->u8_command;

    if (0 < next->u8_addressBytes)
    {
        au8_cmd[u8_i++] = (next->u32_address >> 8) & 0xff;
        au8_cmd[u8_i++] = next->u32_address & 0xff;
    }

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_transmission(NULL, 0, au8_cmd, u8_i))
    {
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return ERR_SPINAND_SUCCESS;
}

static void drv_spinand_switch_config(FLASH_CMD_SET_t *pst_config, u8 u8_enabled)
{
    u32 u32_config;

    if (!pst_config->u16_dataBytes)
        return;

    drv_spinand_wait_device_available();

    if (ERR_SPINAND_SUCCESS
        != drv_spinand_complete_get_features(SPI_NAND_CMD_GF, pst_config->u32_address, (u8 *)&u32_config,
                                             pst_config->u16_dataBytes))
    {
        FLASH_IMPL_PRINTF("[FLASH] Get buf mode status fail!");
    }

    if (u8_enabled && !(u32_config & pst_config->u16_value))
    {
        u32_config |= pst_config->u16_value;
    }
    else if (!u8_enabled && (u32_config & pst_config->u16_value))
    {
        u32_config &= ~pst_config->u16_value;
    }
    else
    {
        return;
    }

    if (ERR_SPINAND_SUCCESS
        != drv_spinand_complete_set_features(pst_config->u32_address, (u8 *)&u32_config, pst_config->u16_dataBytes))
    {
        FLASH_IMPL_PRINTF("[FLASH] Configure buf mode status fail!");
    }

    drv_spinand_wait_device_available();
}

static u8 drv_spinand_die_sel(u32 u32_page)
{
    u32 u32_page_limit;

    if (g_pst_spinand_ext_info && (g_pst_spinand_ext_info->st_profile.u16_flags & SPINAND_MULTI_DIES))
    {
        u8 au8_cmd[2];

        u32_page_limit =
            g_pst_spinand_ext_info->st_profile.st_extConfig.st_dieConfig.u32_dieSize & ~NAND_PAGE_SIZE_MASK;

        au8_cmd[0] = g_pst_spinand_ext_info->st_profile.st_extConfig.st_dieConfig.st_dieCode.u8_command;
        au8_cmd[1] = 0;

        if (u32_page_limit < u32_page)
        {
            au8_cmd[1] = 1;
        }

        if (ERR_SPINAND_SUCCESS != drv_spinand_complete_transmission(NULL, 0, au8_cmd, 2))
        {
            return ERR_SPINAND_DEVICE_FAILURE;
        }
    }

    return ERR_SPINAND_SUCCESS;
}

static void drv_spinand_unlock_whole_block(SPINAND_EXT_INFO_t *pst_ext_sni)
{
    SPINAND_PROTECT_t *pst_protect_status;

    if (0 == (SPINAND_ALL_LOCK & pst_ext_sni->st_profile.u16_flags))
    {
        return;
    }

    pst_protect_status = &pst_ext_sni->st_profile.st_extConfig.st_protectStatus;

    drv_spinand_switch_config(&pst_protect_status->st_blockStatus.st_blocks, 0);
}

static u8 drv_spinand_page_read_status(u32 u32_page)
{
    drv_spinand_die_sel(u32_page);

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_page_read(u32_page))
    {
        FLASH_IMPL_PRINTF("[FLASH] page read fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return drv_spinand_check_page_status();
}

static u32 drv_spinand_read_pages(u32 u32_page, u8 *pu8_data, u32 u32_size)
{
    u32 u32_bytes_left;
    u32 u32_load_size;

    u32_bytes_left = u32_size;

    while (0 != u32_bytes_left)
    {
        u32_load_size = drv_spinand_get_bytes_left(u32_bytes_left, NAND_PAGE_SIZE);

        if (ERR_SPINAND_ECC_NOT_CORRECTED <= mdrv_spinand_page_read(u32_page, 0, pu8_data, u32_load_size))
        {
            break;
        }

        u32_bytes_left -= u32_load_size;
        pu8_data += u32_load_size;
        u32_page++;
    }

    if (0 != u32_bytes_left)
    {
        u32_size -= u32_bytes_left;
    }

    return u32_size;
}

static u32 drv_spinand_read_pages_without_buf_mode(u32 u32_page, u8 *pu8_data, u32 u32_size)
{
    SPINAND_CR_MODE_t *pst_cr_mode;
    u32                u32_bytes_load;

    pst_cr_mode = &g_pst_spinand_ext_info->st_profile.st_access.st_crMode;

    if (NAND_PAGE_SIZE >= u32_size)
    {
        return drv_spinand_read_pages(u32_page, pu8_data, u32_size);
    }

    drv_spinand_switch_config(&pst_cr_mode->un_crProfile.st_noneBufMode.st_noneBufModeCode, 0);

    drv_spinand_setup_access(pst_cr_mode->un_crProfile.st_noneBufMode.st_load.u8_command,
                             pst_cr_mode->un_crProfile.st_noneBufMode.st_load.u8_dummy,
                             g_pst_spinand_ext_info->st_profile.st_access.st_program.u8_command,
                             g_pst_spinand_ext_info->st_profile.st_access.st_random.u8_command);

    do
    {
        u32_bytes_load = u32_size;
        if (ERR_SPINAND_ECC_NOT_CORRECTED <= drv_spinand_page_read_status(u32_page))
        {
            break;
        }

        drv_spinand_complete_read_from_cache_by_bdma(drv_spinand_offset_wrap(u32_page, SPINAND_READ), pu8_data,
                                                     u32_bytes_load);

        if (ERR_SPINAND_ECC_NOT_CORRECTED == drv_spinand_check_page_status())
        {
            break;
        }

        u32_bytes_load = u32_size - u32_bytes_load;
    } while (0);

    drv_spinand_setup_access(g_pst_spinand_ext_info->st_profile.st_access.st_read.u8_command,
                             g_pst_spinand_ext_info->st_profile.st_access.st_read.u8_dummy,
                             g_pst_spinand_ext_info->st_profile.st_access.st_program.u8_command,
                             g_pst_spinand_ext_info->st_profile.st_access.st_random.u8_command);

    drv_spinand_switch_config(&pst_cr_mode->un_crProfile.st_noneBufMode.st_noneBufModeCode, 1);

    if (0 != u32_bytes_load)
    {
        u32_size -= u32_bytes_load;
    }

    return u32_size;
}

static u32 drv_spinand_read_pages_with_buf_mode(u32 u32_page, u8 *pu8_data, u32 u32_size)
{
    SPINAND_BUF_MODE_t st_mode;

    u32 u32_page_end;
    u32 u32_read_end;
    u32 u32_bytes_left;
    u32 u32_load_size;

    u8 u8_block_end_with_last;
    u8 u8_wait_device_avail;
    u8 u8_wait_next;
    u8 u8_wait_next_after_read;
    u8 u8_end_with_reset;

    st_mode                 = g_pst_spinand_ext_info->st_profile.st_access.st_crMode.un_crProfile.st_bufMode;
    u8_block_end_with_last  = st_mode.u8_checkFlag & SPINAND_CR_BLOCK_WITH_LAST;
    u8_wait_device_avail    = st_mode.u8_checkFlag & SPINAND_CR_NEXT_STATUS;
    u8_wait_next            = st_mode.u8_checkFlag & SPINAND_CR_BUSY_AFTER_NEXT;
    u8_wait_next_after_read = st_mode.u8_checkFlag & SPINAND_CR_BUSY_AFTER_READ;
    u8_end_with_reset       = st_mode.u8_checkFlag & SPINAND_CR_END_WITH_REST;

    u32_bytes_left = u32_size;
    u32_page_end   = u32_page + drv_spinand_offset_to_page_address(drv_spinand_offset_to_align_page_size(u32_size));
    u32_read_end   = u32_page_end;

    drv_spinand_wait_device_available();

restart_page_read:
    u32_load_size = 0;
    u32_read_end  = u32_page_end;

    if (u8_block_end_with_last)
    {
        u32_read_end = drv_spinand_align_next_block(u32_page);

        if (u32_read_end > u32_page_end)
        {
            u32_read_end = u32_page_end;
        }
    }

    if (ERR_SPINAND_ECC_NOT_CORRECTED <= drv_spinand_page_read_status(u32_page++)) // has read first page
    {
        return 0;
    }

    st_mode.st_nextPage.u32_address = 0;

    while (u32_read_end > u32_page)
    {
        // prepare next page
        st_mode.st_nextPage.u32_address = u32_page;

        drv_spinand_next_page_read(&st_mode.st_nextPage);

        if (u8_wait_next)
        {
            drv_spinand_wait_status(&st_mode.st_checkBusy);
        }

        if (u8_wait_device_avail)
        {
            drv_spinand_wait_device_available();
        }
        // load current page
        drv_spinand_complete_read_from_cache((u32_page - 1), 0, pu8_data, NAND_PAGE_SIZE);

        if (u8_wait_next_after_read)
        {
            drv_spinand_wait_status(&st_mode.st_checkBusy);
        }

        if (ERR_SPINAND_ECC_NOT_CORRECTED == drv_spinand_check_page_status())
        {
            break;
        }

        u32_bytes_left -= NAND_PAGE_SIZE;
        pu8_data += NAND_PAGE_SIZE;
        u32_page++;
    }

    if (0 != st_mode.st_nextPage.u32_address)
    {
        drv_spinand_next_page_read(&st_mode.st_lastPage);

        if (u8_wait_next)
        {
            drv_spinand_wait_status(&st_mode.st_checkBusy);
        }

        if (u8_wait_device_avail)
        {
            drv_spinand_wait_device_available();
        }
    }

    u32_load_size = drv_spinand_get_bytes_left(u32_bytes_left, NAND_PAGE_SIZE);
    drv_spinand_complete_read_from_cache((u32_page - 1), 0, pu8_data, u32_load_size);

    if (0 != st_mode.st_nextPage.u32_address)
    {
        if (u8_wait_next_after_read)
        {
            drv_spinand_wait_status(&st_mode.st_checkBusy);
        }

        if (u8_end_with_reset)
        {
            if (ERR_SPINAND_SUCCESS != mdrv_spinand_reset())
            {
                u32_size -= u32_bytes_left;
                return u32_size;
            }
        }
    }

    if (u32_read_end != u32_page)
    {
        FLASH_IMPL_PRINTF("[FLASH] buf_mode, meet ecc not corrected!\r\n");
        return 0;
    }

    u32_bytes_left -= u32_load_size;
    pu8_data += u32_load_size;

    if (0 == u32_bytes_left)
    {
        return u32_size;
    }

    if (u32_read_end < u32_page_end)
    {
        goto restart_page_read;
    }

    return 0;
}

static u8 drv_spinand_check_page_status_default(void)
{
    u8                     u8_ret;
    u8                     u8_status;
    SPINAND_EXT_PROFILE_t *pst_profile;

    u8_ret = drv_spinand_check_status(&u8_status);

    if (ERR_SPINAND_TIMEOUT <= u8_ret)
        return u8_ret;

    u8_status &= ECC_STATUS_MASK;

    if (ECC_NOT_CORRECTED == u8_status)
    {
        return ERR_SPINAND_ECC_NOT_CORRECTED;
    }

    if (g_pst_spinand_ext_info)
    {
        pst_profile = &g_pst_spinand_ext_info->st_profile;

        if (ECC_STATUS_RESERVED == u8_status)
        {
            if (pst_profile->u16_flags & SPINAND_ECC_RESERVED_NONE_CORRECTED)
            {
                return ERR_SPINAND_ECC_NOT_CORRECTED;
            }

            if (pst_profile->u16_flags & SPINAND_ECC_RESERVED_CORRECTED)
            {
                return ERR_SPINAND_ECC_CORRECTED;
            }
        }
    }

    if (u8_ecc_corrected != ERR_SPINAND_ECC_CORRECTED && u8_status == ERR_SPINAND_ECC_CORRECTED)
    {
        u8_ecc_corrected = ERR_SPINAND_ECC_CORRECTED;
    }

    return ERR_SPINAND_SUCCESS;
}

static u8 drv_spinand_check_page_status_by_sni(void)
{
    u8                     u8_ret;
    u8                     u8_status;
    SPINAND_EXT_PROFILE_t *pst_profile   = &g_pst_spinand_ext_info->st_profile;
    SPINAND_ECC_CONFIG_t * pst_eccConfig = &pst_profile->st_eccConfig;
    FLASH_CMD_SET_t *      pst_eccStatus = &pst_eccConfig->st_eccStatus;

    u8_ret = drv_spinand_check_status(&u8_status);

    if (ERR_SPINAND_TIMEOUT <= u8_ret)
        return u8_ret;

    u8_status &= NAND_ECC_STATUS_MASK;

    if (NAND_ECC_NO_CORRECTED == u8_status)
        return ERR_SPINAND_SUCCESS;
    else if (NAND_ECC_NOT_CORRECTED == u8_status)
        return ERR_SPINAND_ECC_NOT_CORRECTED;
    else if ((pst_eccConfig->u8_eccType & SPINAND_RESERVED_NONE_CORRECTED) && u8_status == NAND_ECC_RESERVED)
        return ERR_SPINAND_ECC_NOT_CORRECTED;

    if (pst_eccConfig->u8_eccType & SPINAND_THRESHOLD)
    {
        if (ERR_SPINAND_SUCCESS
            != drv_spinand_complete_get_features(pst_eccStatus->u8_command, (u8)(pst_eccStatus->u32_address),
                                                 &u8_status, 1))
            return ERR_SPINAND_DEVICE_FAILURE;

        if ((u8_status & pst_eccStatus->u16_value) != pst_eccStatus->u16_value)
            return ERR_SPINAND_SUCCESS;
    }
    else if (pst_eccConfig->u8_eccType & SPINAND_BITFLIP)
    {
        if (ERR_SPINAND_SUCCESS
            != drv_spinand_complete_get_features(pst_eccStatus->u8_command, (u8)(pst_eccStatus->u32_address),
                                                 &u8_status, 1))
            return ERR_SPINAND_DEVICE_FAILURE;

        if ((u8_status & pst_eccStatus->u16_value) < NAND_ECC_BITFLIPTHRESHOLD)
            return ERR_SPINAND_SUCCESS;
    }

    if (u8_ecc_corrected != ERR_SPINAND_ECC_CORRECTED)
    {
        u8_ecc_corrected = ERR_SPINAND_ECC_CORRECTED;
    }

    return ERR_SPINAND_ECC_CORRECTED;
}

static u8 drv_spinand_is_id_match(u8 *au8_id, u8 u8_id_len)
{
    u8 u8_i;
    u8 au8_device_id[8];

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_read_id(au8_device_id, u8_id_len))
    {
        FLASH_IMPL_PRINTF("[FLASH] read device id fail!\r\n");
        return 0;
    }

    for (u8_i = 0; u8_id_len > u8_i; u8_i++)
    {
        if (au8_device_id[u8_i] != au8_id[u8_i])
        {
            return 0;
        }
    }

    return 1;
}

static u8 drv_spinand_is_sni_header(u8 *sni_buf)
{
    return FLASH_IMPL_MEMCMP(sni_buf, au8_basicMagicData, FLASH_SNI_HEADER_SIZE);
}

static u8 drv_spinand_is_ext_sni_header(u8 *sni_buf)
{
    SPINAND_SNI_t *     pst_sni;
    SPINAND_EXT_INFO_t *pst_ext_sni;

    pst_sni     = (SPINAND_SNI_t *)sni_buf;
    pst_ext_sni = &pst_sni->spinand_ext_info;

    return FLASH_IMPL_MEMCMP(pst_ext_sni->au8_magic, au8_extMagicData, FLASH_EXT_SNI_HEADER_SIZE);
}

static u8 drv_spinand_load_sni(u8 *sni_buf)
{
    u32 u32_page;
    u32 u32_search_page;
    u32 u32_extSni;
    u32 u32_load_size;

    SPINAND_SNI_t *              pst_sni;
    SPINAND_INFO_t *             pst_info;
    SPINAND_EXT_INFO_t *         pst_ext_info;
    SPINAND_EXT_CONFIGURATION_t *pst_ext_configuration;

    pst_sni               = (SPINAND_SNI_t *)sni_buf;
    pst_info              = &pst_sni->spinand_info;
    pst_ext_info          = &pst_sni->spinand_ext_info;
    pst_ext_configuration = &pst_sni->spinand_ext_configuration;
    u32_page              = 10 * FLASH_PAGES_PER_BLOCK_DEFAULT;

    if (!drv_spinand_is_sni_header(sni_buf))
    {
        for (u32_search_page = 0; u32_search_page < u32_page; u32_search_page += (FLASH_PAGES_PER_BLOCK_DEFAULT << 1))
        {
            if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read(u32_search_page, 0, sni_buf, FLASH_MAX_SNI_SIZE))
                continue;

            if (pst_sni->u32_crc32
                == FLASH_IMPL_CRC32(0, (void *)sni_buf, (unsigned long)&pst_sni->u32_crc32 - (unsigned long)sni_buf))
            {
                break;
            }
            FLASH_IMPL_MEMSET(sni_buf, 0, FLASH_MAX_SNI_SIZE);
        }
    }

    if (!drv_spinand_is_sni_header(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] No sni!!!\r\n");
        return 0;
    }

    if (!drv_spinand_is_id_match(pst_info->au8_ID, pst_info->u8_IDByteCnt))
    {
        FLASH_IMPL_PRINTF("[FLASH] SNI not match device!!!\r\n");
        return 0;
    }

    if (!drv_spinand_is_ext_sni_header(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] Version mismatch!!!\r\n");
        return 0;
    }

    sni_buf += FLASH_MAX_SNI_SIZE;
    u32_extSni      = pst_ext_info->u32_extSni;
    u32_search_page = 0;
    u32_page        = 10 * NAND_PAGES_PER_BLOCK;

    if (!u32_extSni)
        return 1;

    FLASH_IMPL_PRINTF("[FLASH] Load EXT SNI\r\n");

    u32_load_size = FLASH_EXT_SNI_RESERVED_SIZE * u32_extSni;

    do
    {
        if (0 == (u32_search_page & NAND_PAGES_PER_BLOCK))
        {
            if (ERR_SPINAND_TIMEOUT
                    > mdrv_spinand_page_read(u32_search_page, FLASH_MAX_SNI_SIZE, sni_buf, u32_load_size)
                && pst_ext_configuration->u32_crc32
                       == FLASH_IMPL_CRC32(0, (void *)((unsigned long)sni_buf + 4), (unsigned long)(u32_load_size - 4)))
            {
                return 1;
            }
        }

        u32_search_page += NAND_PAGES_PER_BLOCK;
    } while (u32_search_page < u32_page);

    return 0;
}

#if defined(CONFIG_FLASH_WRITE_BACK)
static u32 drv_spinand_load_ext_sni(u32 u32_page, u16 u16_offset, u8 *sni_buf, u32 u32_extSni)
{
    u8 *                         u32_dst;
    u32                          u32_load_size = 0;
    u32                          u32_limit;
    SPINAND_SNI_t *              pst_sni;
    SPINAND_INFO_t *             pst_info;
    SPINAND_EXT_CONFIGURATION_t *pst_ext_configuration;
    SPINAND_EXT_SNI_t *          pst_ext_sni;

    if (!u32_extSni)
        return 0;

    pst_sni               = (SPINAND_SNI_t *)sni_buf;
    pst_info              = &pst_sni->spinand_info;
    pst_ext_configuration = &pst_sni->spinand_ext_configuration;
    u16_offset += FLASH_MAX_SNI_SIZE;
    sni_buf += FLASH_MAX_SNI_SIZE;
    u32_dst   = sni_buf; // crc32
    u32_limit = pst_sni->spinand_info.u16_PageByteCnt - FLASH_MAX_SNI_SIZE;

    for (; 0 != (u32_page & (pst_info->u16_BlkPageCnt - 1)) && u32_extSni && u32_load_size < u32_limit; u32_page++)
    {
        for (; u16_offset < pst_info->u16_PageByteCnt && u32_extSni && u32_load_size < u32_limit;
             u16_offset += FLASH_MAX_SNI_SIZE, u32_extSni--)
        {
            pst_ext_sni = (SPINAND_EXT_SNI_t *)sni_buf;

            if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read(u32_page, u16_offset, sni_buf, FLASH_MAX_SNI_SIZE)
                || pst_ext_sni->u32_crc32
                       != FLASH_IMPL_CRC32(0, (void *)sni_buf,
                                           (unsigned long)&pst_ext_sni->u32_crc32 - (unsigned long)sni_buf))
                return 0;

            memcpy((void *)u32_dst, (void *)pst_ext_sni->au8_reserved, sizeof(pst_ext_sni->au8_reserved));
            u32_dst += FLASH_EXT_SNI_RESERVED_SIZE;
            u32_load_size += FLASH_EXT_SNI_RESERVED_SIZE;
            sni_buf += FLASH_MAX_SNI_SIZE;
        }
        u16_offset = 0;
    }

    pst_ext_configuration->u32_crc32 =
        FLASH_IMPL_CRC32(0, (void *)((unsigned long)pst_ext_configuration + 4), (unsigned long)(u32_load_size - 4));

    return u32_load_size;
}

static u8 drv_spinand_write_back_sni(u8 *sni_buf)
{
    u8                  u8_ismatch;
    u16                 u16_bytes;
    u32                 u32_page;
    u32                 u32_search_page = 0;
    u32                 u32_load_size;
    SPINAND_SNI_t *     pst_match_sni;
    SPINAND_INFO_t *    pst_match_info;
    SPINAND_EXT_INFO_t *pst_match_ext_info;
    u8 *                pst_src_sni;

    pst_src_sni        = sni_buf + FLASH_MAX_SNI_SIZE;
    pst_match_sni      = (SPINAND_SNI_t *)pst_src_sni;
    pst_match_info     = &pst_match_sni->spinand_info;
    pst_match_ext_info = &pst_match_sni->spinand_ext_info;
    u32_page           = 10 * FLASH_PAGES_PER_BLOCK_DEFAULT;

    do
    {
        u32_page -= FLASH_PAGES_PER_BLOCK_DEFAULT;

        if (u32_page & FLASH_PAGES_PER_BLOCK_DEFAULT)
        {
            if (g_pst_spinand_info->u8_BLPINB > 0)
            {
                u32_search_page = u32_page + 34;
            }
            else
            {
                u32_search_page = u32_page + 2;
            }

            if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read(u32_search_page, 0, pst_src_sni, FLASH_SNI_HEADER_SIZE))
                continue;

            if (drv_spinand_is_sni_header(pst_src_sni))
                break;
        }
    } while (0 != u32_page);

    if (!drv_spinand_is_sni_header(pst_src_sni))
    {
        return 0;
    }

    u8_ismatch = 0;

    while (1)
    {
        if (!u8_ismatch)
        {
            for (; 0 != (u32_search_page & FLASH_BLOCK_MASK_DEFAULT); u32_search_page++)
            {
                for (u16_bytes = 0;; u16_bytes += FLASH_MAX_SNI_SIZE) // the sni has 512bytes
                {
                    if (ERR_SPINAND_SUCCESS
                        != mdrv_spinand_page_read(u32_search_page, u16_bytes, pst_src_sni, FLASH_MAX_SNI_SIZE))
                    {
                        return 0;
                    }

                    if (pst_match_sni->u32_crc32
                        != FLASH_IMPL_CRC32(0, (void *)pst_src_sni,
                                            (unsigned long)&pst_match_sni->u32_crc32 - (unsigned long)pst_src_sni))
                        break;

                    if (!drv_spinand_is_sni_header(pst_src_sni))
                        break;

                    if (drv_spinand_is_id_match(pst_match_info->au8_ID, pst_match_info->u8_IDByteCnt))
                    {
                        if (!drv_spinand_is_ext_sni_header(pst_src_sni))
                        {
                            FLASH_IMPL_PRINTF("[FLASH] Version mismatch!!!\r\n");
                            return 0;
                        }

                        if (drv_spinand_soc_ecc_is_enable())
                        {
                            pst_match_sni->spinand_ext_info.st_profile.st_eccConfig.u8_eccType = g_socecc_type;
                            pst_match_sni->u32_crc32 =
                                FLASH_IMPL_CRC32(0, (void *)pst_src_sni,
                                                 (unsigned long)&pst_match_sni->u32_crc32 - (unsigned long)pst_src_sni);
                        }

                        FLASH_IMPL_MEMCPY(sni_buf, pst_src_sni, FLASH_MAX_SNI_SIZE);

                        u32_load_size = drv_spinand_load_ext_sni(u32_search_page, u16_bytes, sni_buf,
                                                                 pst_match_ext_info->u32_extSni);
                        if (pst_match_ext_info->u32_extSni && !u32_load_size)
                            break;

                        FLASH_IMPL_PRINTF_HEX("[FLASH] Found matched SNI @ 0x", u32_search_page, "\r\n");
                        FLASH_IMPL_SHOW_ID(pst_match_info->au8_ID, pst_match_info->u8_IDByteCnt);
                        drv_spinand_unlock_whole_block(pst_match_ext_info);
                        u32_load_size += FLASH_MAX_SNI_SIZE;
                        u8_ismatch = 1;
                        break;
                    }
                }

                if (u8_ismatch)
                    break;
            }
        }

        if (u8_ismatch)
        {
            if (0 == (u32_page & FLASH_PAGES_PER_BLOCK_DEFAULT))
            {
                if (!mdrv_spinand_block_isbad(u32_page) && ERR_SPINAND_SUCCESS == mdrv_spinand_block_erase(u32_page)
                    && ERR_SPINAND_SUCCESS == mdrv_spinand_page_program(u32_page, 0, sni_buf, u32_load_size))
                {
                    FLASH_IMPL_PRINTF_HEX("[FLASH] write sni to page 0x", u32_page, "\r\n");
                }
            }
        }
        else if (0 == (u32_search_page & FLASH_BLOCK_MASK_DEFAULT))
        {
            u32_search_page -= FLASH_PAGES_PER_BLOCK_DEFAULT;
            if (u32_search_page)
            {
                u32_search_page -= FLASH_PAGES_PER_BLOCK_DEFAULT;
                u32_search_page += 2;
                FLASH_IMPL_PRINTF_HEX("u32_search_page 0x", u32_search_page, "\r\n");
                continue;
            }
            else
                return 0;
        }

        if (0 == u32_page)
        {
            break;
        }

        u32_page -= FLASH_PAGES_PER_BLOCK_DEFAULT;
    }

    return 1;
}
#else
#define drv_spinand_write_back_sni(x) 0
#endif

#ifdef CONFIG_FLASH_SOCECC
static u8 drv_spinand_page_read_socecc(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status        = ERR_SPINAND_SUCCESS;
    u16 u16_pageSize     = ast_flash_config_list[g_socecc_type].u16_pageSize;
    u16 u16_sectorSize   = ast_flash_config_list[g_socecc_type].u16_sector_size;
    u16 eccbytes         = ast_flash_config_list[g_socecc_type].u16_ecc_bytes;
    u16 u16_sector_start = 0;
    u16 u16_sector_end   = 0;
    u16 u16_data_offset  = 0;
    u16 u16_oob_offset   = 0;
    u32 u32_data_size    = 0;
    u32 u32_oob_size     = 0;

    if (!g_socecc_cache_buf)
        return ERR_SPINAND_DEVICE_FAILURE;

    u32_data_size = (u16_column < u16_pageSize) ? (u16_pageSize - u16_column) : 0;
    u32_data_size = (u32_data_size < u32_size) ? u32_data_size : u32_size;
    u32_oob_size  = u32_size - u32_data_size;
    u32_oob_size  = (u32_oob_size > ast_flash_config_list[g_socecc_type].u16_spareSize)
                        ? ast_flash_config_list[g_socecc_type].u16_ecc_bytes
                        : u32_oob_size;

    if (u32_data_size && (!u32_oob_size))
    {
        u16_data_offset  = u16_column & (u16_sectorSize - 1);
        u16_sector_start = u16_column >> drv_spinand_count_bits(u16_sectorSize);
        u16_sector_end   = (u16_column + u32_size - 1) >> drv_spinand_count_bits(u16_sectorSize);

        st_fcie_cfg.u16_sector_cnt   = u16_sector_end - u16_sector_start + 1;
        st_fcie_cfg.u16_col_address  = drv_spinand_offset_wrap(u32_page, SPINAND_READ);
        st_fcie_cfg.u16_data_address = u16_sector_start * u16_sectorSize;
        st_fcie_cfg.u16_oob_address  = u16_pageSize + (u16_sector_start * eccbytes);
        if (u16_data_offset || ((unsigned long)pu8_data % 64) || (u32_data_size & (u16_sectorSize - 1)))
        {
            st_fcie_cfg.data_cache = g_socecc_cache_buf;
        }
        else
        {
            st_fcie_cfg.data_cache = pu8_data;
        }
    }
    else if ((!u32_data_size) && u32_oob_size)
    {
        u16_sector_start = (u16_column - u16_pageSize) >> drv_spinand_count_bits(eccbytes);
        u16_sector_end   = (u16_column - u16_pageSize + u32_oob_size - 1) >> drv_spinand_count_bits(eccbytes);
        u16_oob_offset   = (u16_column - u16_pageSize) & (eccbytes - 1);

        st_fcie_cfg.u16_sector_cnt   = u16_sector_end - u16_sector_start + 1;
        st_fcie_cfg.u16_col_address  = drv_spinand_offset_wrap(u32_page, SPINAND_READ);
        st_fcie_cfg.u16_data_address = u16_sector_start * u16_sectorSize;
        st_fcie_cfg.u16_oob_address  = u16_pageSize + (u16_sector_start * eccbytes);
        st_fcie_cfg.data_cache       = g_socecc_cache_buf;
    }
    else if (u32_data_size && u32_oob_size)
    {
        st_fcie_cfg.u16_sector_cnt   = u16_pageSize >> drv_spinand_count_bits(u16_sectorSize);
        st_fcie_cfg.u16_col_address  = drv_spinand_offset_wrap(u32_page, SPINAND_READ);
        st_fcie_cfg.u16_data_address = 0;
        st_fcie_cfg.u16_oob_address  = u16_pageSize;
        st_fcie_cfg.data_cache       = g_socecc_cache_buf;
    }
    else
    {
        return ERR_SPINAND_INVALID;
    }

    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.data_cache, st_fcie_cfg.u16_sector_cnt * u16_sectorSize);
    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.oob_cache, ast_flash_config_list[g_socecc_type].u16_spareSize);
    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.data_cache_codec, st_fcie_cfg.u16_sector_cnt * u16_sectorSize);
    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.oob_cache_codec, ast_flash_config_list[g_socecc_type].u16_spareSize);

    if (ERR_SPINAND_SUCCESS != drv_spinand_soc_ecc_read(&st_fcie_cfg))
    {
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    u8_status = (st_fcie_cfg.u8_ecc_status > u8_status) ? st_fcie_cfg.u8_ecc_status : u8_status;

    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.data_cache, st_fcie_cfg.u16_sector_cnt * u16_sectorSize);
    FLASH_IMPL_MEM_INVALIDATE((void *)st_fcie_cfg.oob_cache, ast_flash_config_list[g_socecc_type].u16_spareSize);

    if (u32_data_size && (!u32_oob_size))
    {
        if (u16_data_offset || ((unsigned long)pu8_data % 64) || (u32_data_size & (u16_sectorSize - 1)))
        {
            FLASH_IMPL_MEMCPY(pu8_data, (const u8 *)(st_fcie_cfg.data_cache + u16_data_offset), u32_data_size);
        }
    }
    else if ((!u32_data_size) && u32_oob_size)
    {
        FLASH_IMPL_MEMCPY(pu8_data, (const u8 *)(st_fcie_cfg.oob_cache + u16_oob_offset), u32_oob_size);
    }
    else if (u32_data_size && u32_oob_size)
    {
        FLASH_IMPL_MEMCPY(pu8_data, (const u8 *)(st_fcie_cfg.data_cache + u16_column), u32_data_size);
        FLASH_IMPL_MEMCPY((pu8_data + u32_data_size), (const u8 *)(st_fcie_cfg.oob_cache), u32_oob_size);
    }

    return u8_status;
}

static u8 drv_spinand_page_program_socecc(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u16 eccbytes         = ast_flash_config_list[g_socecc_type].u16_ecc_bytes;
    u16 u16_pageSize     = ast_flash_config_list[g_socecc_type].u16_pageSize;
    u16 u16_sectorSize   = ast_flash_config_list[g_socecc_type].u16_sector_size;
    u16 u16_sector_start = 0;
    u16 u16_sector_end   = 0;
    u16 u16_data_offset  = 0;

    if (!g_socecc_cache_buf)
        return ERR_SPINAND_DEVICE_FAILURE;

    if ((u16_column + u32_size) > u16_pageSize)
        return ERR_SPINAND_INVALID;

    u16_sector_start = u16_column >> drv_spinand_count_bits(u16_sectorSize);
    u16_sector_end   = (u16_column + u32_size - 1) >> drv_spinand_count_bits(u16_sectorSize);

    st_fcie_cfg.u16_sector_cnt   = u16_sector_end - u16_sector_start + 1;
    st_fcie_cfg.u16_col_address  = drv_spinand_offset_wrap(u32_page, SPINAND_PROGRAM);
    st_fcie_cfg.u16_data_address = u16_column & (~(u16_sectorSize - 1));
    st_fcie_cfg.u16_oob_address  = u16_pageSize + ((u16_column >> drv_spinand_count_bits(u16_sectorSize)) * eccbytes);
    u16_data_offset              = u16_column & (u16_sectorSize - 1);

    if (u16_data_offset || ((unsigned long)pu8_data % 64) || (u32_size & (u16_sectorSize - 1)))
    {
        st_fcie_cfg.data_cache = g_socecc_cache_buf;
        FLASH_IMPL_MEMCPY((void *)(st_fcie_cfg.data_cache + u16_data_offset), (const void *)pu8_data, u32_size);
    }
    else
    {
        st_fcie_cfg.data_cache = pu8_data;
    }

    FLASH_IMPL_MEM_FLUSH((void *)st_fcie_cfg.data_cache, st_fcie_cfg.u16_sector_cnt * u16_sectorSize);

    return drv_spinand_soc_ecc_write(&st_fcie_cfg);
}

static u8 drv_spinand_fcie_get_info(u8 *sni_buf)
{
    u8                  u8_ecc_type_cnt;
    u32                 u32_page;
    u32                 u32_search_page;
    SPINAND_SNI_t *     pst_sni;
    SPINAND_EXT_INFO_t *pst_ext_info;

    if (!drv_spinand_soc_ecc_is_enable())
        return 1;

    if (!g_socecc_cache_buf)
        return ERR_SPINAND_DEVICE_FAILURE;

    u8_ecc_type_cnt = sizeof(ast_flash_config_list) / sizeof(stFlashConfig);
    pst_sni         = (SPINAND_SNI_t *)sni_buf;
    pst_ext_info    = &pst_sni->spinand_ext_info;
    u32_page        = 10 * FLASH_PAGES_PER_BLOCK_DEFAULT;

    if (drv_spinand_is_sni_header(sni_buf)
        && (pst_sni->u32_crc32
            == FLASH_IMPL_CRC32(0, (void *)sni_buf, (unsigned long)&pst_sni->u32_crc32 - (unsigned long)sni_buf))
        && drv_spinand_is_ext_sni_header(sni_buf))
    {
        g_socecc_type = pst_ext_info->st_profile.st_eccConfig.u8_eccType;
        FLASH_IMPL_PRINTF_HEX("[FLASH] SOC ECC type 0x", g_socecc_type, "\r\n");

        if (g_socecc_type < u8_ecc_type_cnt)
        {
            st_fcie_cfg.u16_sector_size  = ast_flash_config_list[g_socecc_type].u16_sector_size;
            st_fcie_cfg.u16_ecc_bytes    = ast_flash_config_list[g_socecc_type].u16_ecc_bytes;
            st_fcie_cfg.u16_spare_size   = ast_flash_config_list[g_socecc_type].u16_spareSize;
            st_fcie_cfg.data_cache_codec = g_socecc_cache_buf;
            st_fcie_cfg.data_cache       = g_socecc_cache_buf;
            st_fcie_cfg.oob_cache_codec =
                st_fcie_cfg.data_cache_codec + ast_flash_config_list[g_socecc_type].u16_pageSize;
            st_fcie_cfg.oob_cache = st_fcie_cfg.data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;
            if (st_fcie_cfg.imi_data_cache)
                st_fcie_cfg.imi_oob_cache =
                    st_fcie_cfg.imi_data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;

            drv_spinand_soc_ecc_setup(&ast_flash_config_list[g_socecc_type]);
            return 1;
        }

        return 0;
    }

    for (u32_search_page = 0; u32_search_page < u32_page; u32_search_page += (FLASH_PAGES_PER_BLOCK_DEFAULT << 1))
    {
        for (g_socecc_type = 0; g_socecc_type < u8_ecc_type_cnt; g_socecc_type++)
        {
            NAND_PAGE_SIZE               = ast_flash_config_list[g_socecc_type].u16_pageSize;
            st_fcie_cfg.u16_sector_size  = ast_flash_config_list[g_socecc_type].u16_sector_size;
            st_fcie_cfg.u16_ecc_bytes    = ast_flash_config_list[g_socecc_type].u16_ecc_bytes;
            st_fcie_cfg.u16_spare_size   = ast_flash_config_list[g_socecc_type].u16_spareSize;
            st_fcie_cfg.data_cache_codec = g_socecc_cache_buf;
            st_fcie_cfg.data_cache       = g_socecc_cache_buf;
            st_fcie_cfg.oob_cache_codec =
                st_fcie_cfg.data_cache_codec + ast_flash_config_list[g_socecc_type].u16_pageSize;
            st_fcie_cfg.oob_cache = st_fcie_cfg.data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;
            if (st_fcie_cfg.imi_data_cache)
                st_fcie_cfg.imi_oob_cache =
                    st_fcie_cfg.imi_data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;

            drv_spinand_soc_ecc_setup(&ast_flash_config_list[g_socecc_type]);
            if (ERR_SPINAND_ECC_NOT_CORRECTED > mdrv_spinand_page_read(u32_search_page, 0, sni_buf, FLASH_MAX_SNI_SIZE))
                break;
        }

        if (pst_sni->u32_crc32
            == FLASH_IMPL_CRC32(0, (void *)sni_buf, (unsigned long)&pst_sni->u32_crc32 - (unsigned long)sni_buf))
        {
            break;
        }
    }

    if (g_socecc_type == u8_ecc_type_cnt || !drv_spinand_is_sni_header(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] No sni!!!\r\n");
        return 0;
    }

    pst_ext_info->st_profile.st_eccConfig.u8_eccType = g_socecc_type;

    return 1;
}

void mdrv_spinand_socecc_init(u8 *imi_cache_buf, u8 *miu_cache_buf)
{
    g_socecc_cache_buf = miu_cache_buf;

    st_fcie_cfg.imi_data_cache       = NULL;
    st_fcie_cfg.u8_programLoad       = st_spinand_drv_info.u8_programLoad;
    st_fcie_cfg.u8_randomLoad        = st_spinand_drv_info.u8_randomLoad;
    st_fcie_cfg.pst_spinand_drv_info = &st_spinand_drv_info;
    st_fcie_cfg.read_cache           = drv_spinand_read_from_cache;
    st_fcie_cfg.program_load         = drv_spinand_complete_program_load;

    if (imi_cache_buf)
        st_fcie_cfg.imi_data_cache = imi_cache_buf;
}

#else

static u8 drv_spinand_page_read_socecc(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    return 0;
}

static u8 drv_spinand_page_program_socecc(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    return 0;
}

static u8 drv_spinand_fcie_get_info(u8 *sni_buf)
{
    return 1;
}
void mdrv_spinand_socecc_init(u8 *imi_cache_buf, u8 *miu_cache_buf)
{
    return;
}

#endif
static void drv_spinand_setup_by_sni(SPINAND_EXT_INFO_t *pst_ext_sni)
{
    SPINAND_EXT_PROFILE_t *  pst_profile;
    SPINAND_ACCESS_CONFIG_t *pst_access;

    pst_profile = &pst_ext_sni->st_profile;
    pst_access  = &pst_profile->st_access;

#if defined(CONFIG_FLASH_BDMA)
    if ((SPINAND_NONE_BUF_MODE == pst_access->st_crMode.u8_crType) && (!g_pst_spinand_info->U8RIURead))
    {
        drv_spinand_read = drv_spinand_read_pages_without_buf_mode;
        FLASH_IMPL_PRINTF("[FLASH] Load without buffer mode\r\n");
    }
#endif

    if (SPINAND_BUF_MODE == pst_access->st_crMode.u8_crType)
    {
        drv_spinand_read = drv_spinand_read_pages_with_buf_mode;
        FLASH_IMPL_PRINTF("[FLASH] Load with buffer mode\r\n");
    }

    if (pst_profile->st_eccConfig.u8_eccEn)
    {
        drv_spinand_check_page_status = drv_spinand_check_page_status_by_sni;
        FLASH_IMPL_PRINTF("[FLASH] check page status by sni\r\n");
    }

#ifdef CONFIG_FLASH_SOCECC
    if (drv_spinand_soc_ecc_is_enable())
    {
        drv_spinand_read             = drv_spinand_read_pages;
        g_socecc_type                = pst_profile->st_eccConfig.u8_eccType;
        st_fcie_cfg.u16_sector_size  = ast_flash_config_list[g_socecc_type].u16_sector_size;
        st_fcie_cfg.u16_ecc_bytes    = ast_flash_config_list[g_socecc_type].u16_ecc_bytes;
        st_fcie_cfg.u16_spare_size   = ast_flash_config_list[g_socecc_type].u16_spareSize;
        st_fcie_cfg.data_cache_codec = g_socecc_cache_buf;
        st_fcie_cfg.data_cache       = g_socecc_cache_buf;
        st_fcie_cfg.oob_cache_codec  = st_fcie_cfg.data_cache_codec + ast_flash_config_list[g_socecc_type].u16_pageSize;
        st_fcie_cfg.oob_cache        = st_fcie_cfg.data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;
        if (st_fcie_cfg.imi_data_cache)
            st_fcie_cfg.imi_oob_cache = st_fcie_cfg.imi_data_cache + ast_flash_config_list[g_socecc_type].u16_pageSize;

        drv_spinand_soc_ecc_setup(&ast_flash_config_list[g_socecc_type]);
    }
#endif

    drv_spinand_setup_access(pst_access->st_read.u8_command, pst_access->st_read.u8_dummy,
                             pst_access->st_program.u8_command, pst_access->st_random.u8_command);

    FLASH_IMPL_PRINTF_HEX("[SPINAND] RFC use command 0x", pst_access->st_read.u8_command, "\r\n");
    FLASH_IMPL_PRINTF_HEX("[SPINAND] dummy clock 0x", pst_access->st_read.u8_dummy, "\r\n");

    drv_spinand_setup_timeout(pst_profile->u32_maxWaitTime);
}

u8 mdrv_spinand_find_sni_form_dram(u8 *sni_buf, u8 *sni_list)
{
    u32                 u32_extsni   = 0;
    SPINAND_SNI_t *     pst_sni      = NULL;
    SPINAND_INFO_t *    pst_info     = NULL;
    SPINAND_EXT_INFO_t *pst_ext_info = NULL;
    SPINAND_EXT_SNI_t * pst_ext_sni  = NULL;

    while (drv_spinand_is_sni_header(sni_list))
    {
        pst_sni      = (SPINAND_SNI_t *)sni_list;
        pst_info     = &pst_sni->spinand_info;
        pst_ext_info = &pst_sni->spinand_ext_info;

        if (drv_spinand_is_id_match(pst_info->au8_ID, pst_info->u8_IDByteCnt)
            && pst_sni->u32_crc32
                   == FLASH_IMPL_CRC32(0, (void *)sni_list,
                                       (unsigned long)&pst_sni->u32_crc32 - (unsigned long)sni_list))
        {
#ifdef CONFIG_FLASH_SOCECC
            if (drv_spinand_soc_ecc_is_enable())
            {
                if ((pst_info->u16_PageByteCnt == ast_flash_config_list[g_socecc_type].u16_pageSize)
                    && (pst_info->u16_SpareByteCnt >= ast_flash_config_list[g_socecc_type].u16_spareSize))
                    pst_sni->spinand_ext_info.st_profile.st_eccConfig.u8_eccType = g_socecc_type;
                else if ((pst_info->u16_PageByteCnt == ast_flash_config_list[0].u16_pageSize)
                         && (pst_info->u16_SpareByteCnt >= ast_flash_config_list[0].u16_spareSize))
                    pst_sni->spinand_ext_info.st_profile.st_eccConfig.u8_eccType = 0;
                else if ((pst_info->u16_PageByteCnt == ast_flash_config_list[3].u16_pageSize)
                         && (pst_info->u16_SpareByteCnt >= ast_flash_config_list[3].u16_spareSize))
                    pst_sni->spinand_ext_info.st_profile.st_eccConfig.u8_eccType = 3;
                else
                {
                    FLASH_IMPL_PRINTF("[FLASH] no matched ecc type\n");
                    return 0;
                }

                drv_spinand_switch_config(&pst_ext_info->st_profile.st_extConfig.st_eccEnabled, 0);
            }
#endif
            memcpy((void *)sni_buf, (void *)sni_list, FLASH_MAX_SNI_SIZE);
            sni_buf += FLASH_MAX_SNI_SIZE;
            break;
        }

        sni_list += FLASH_MAX_SNI_SIZE;
    }

    if (!drv_spinand_is_sni_header(sni_list))
        return 0;

    if (pst_ext_info)
        u32_extsni = pst_ext_info->u32_extSni;

    while (u32_extsni)
    {
        sni_list += FLASH_MAX_SNI_SIZE;
        pst_ext_sni = (SPINAND_EXT_SNI_t *)sni_list;
        if (pst_ext_sni->u32_crc32
            == FLASH_IMPL_CRC32(0, (void *)sni_list, (unsigned long)&pst_ext_sni->u32_crc32 - (unsigned long)sni_list))
        {
            memcpy((void *)sni_buf, (void *)pst_ext_sni->au8_reserved, sizeof(pst_ext_sni->au8_reserved));
            sni_buf += sizeof(pst_ext_sni->au8_reserved);
        }
        else
            return 0;

        u32_extsni--;
    }

    return 1;
}

void mdrv_spinand_info(FLASH_NAND_INFO_t *pst_flash_nand_info)
{
    pst_flash_nand_info->u16_BlkPageCnt = 0;
    pst_flash_nand_info->u16_BlkCnt     = 0;
    pst_flash_nand_info->u16_SectorSize = 0;
    pst_flash_nand_info->u16_PageSize   = 0;
    pst_flash_nand_info->u16_OobSize    = 0;
    pst_flash_nand_info->u32_BlockSize  = 0;
    pst_flash_nand_info->u32_Capacity   = 0;
    pst_flash_nand_info->u8_BAKCNT      = 0;
    pst_flash_nand_info->u8_BAKOFS      = 0;
    pst_flash_nand_info->u8_BL0PBA      = 0;
    pst_flash_nand_info->u8_BL1PBA      = 0;
    pst_flash_nand_info->u8_BLPINB      = 0;

    if (g_pst_spinand_info)
    {
        memcpy((void *)(pst_flash_nand_info->au8_ID), (const void *)(g_pst_spinand_info->au8_ID),
               g_pst_spinand_info->u8_IDByteCnt);
        pst_flash_nand_info->u8_IDByteCnt   = g_pst_spinand_info->u8_IDByteCnt;
        pst_flash_nand_info->u16_BlkPageCnt = g_pst_spinand_info->u16_BlkPageCnt;
        pst_flash_nand_info->u16_BlkCnt     = g_pst_spinand_info->u16_BlkCnt;
        pst_flash_nand_info->u16_SectorSize = g_pst_spinand_info->u16_SectorByteCnt;
        pst_flash_nand_info->u16_PageSize   = g_pst_spinand_info->u16_PageByteCnt;
        pst_flash_nand_info->u16_OobSize    = g_pst_spinand_info->u16_SpareByteCnt;
        pst_flash_nand_info->u32_BlockSize  = g_pst_spinand_info->u16_PageByteCnt * g_pst_spinand_info->u16_BlkPageCnt;
        pst_flash_nand_info->u32_Capacity   = pst_flash_nand_info->u32_BlockSize * g_pst_spinand_info->u16_BlkCnt;
        pst_flash_nand_info->u8_BAKCNT      = g_pst_spinand_info->u8_BAKCNT;
        pst_flash_nand_info->u8_BAKOFS      = g_pst_spinand_info->u8_BAKOFS;
        pst_flash_nand_info->u8_BL0PBA      = g_pst_spinand_info->u8_BL0PBA;
        pst_flash_nand_info->u8_BL1PBA      = g_pst_spinand_info->u8_BL1PBA;
        pst_flash_nand_info->u8_BLPINB      = g_pst_spinand_info->u8_BLPINB;
    }
}

u8 mdrv_spinand_hardware_init(u8 *sni_buf)
{
    SPINAND_EXT_PROFILE_t *pst_profile;

    g_pst_spinand_sni               = (SPINAND_SNI_t *)sni_buf;
    g_pst_spinand_info              = &g_pst_spinand_sni->spinand_info;
    g_pst_spinand_ext_info          = NULL;
    g_pst_spinand_ext_configuration = NULL;

    drv_spinand_read              = drv_spinand_read_pages;
    drv_spinand_check_page_status = drv_spinand_check_page_status_default;
    u8_ecc_corrected              = 0;

    if (ERR_SPINAND_SUCCESS != drv_spinand_init())
    {
        FLASH_IMPL_PRINTF("[FLASH] init fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    if (!drv_spinand_fcie_get_info(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] SOC ECC No sni!\r\n");
        g_pst_spinand_sni  = NULL;
        g_pst_spinand_info = NULL;
        return ERR_SPINAND_DEVICE_FAILURE;
    }

#if defined(CONFIG_FLASH_HW_CS)
    drv_spinand_use_bdma(1);
#endif

    if (!drv_spinand_load_sni(sni_buf) && !drv_spinand_write_back_sni(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] No sni!\r\n");
        g_pst_spinand_sni  = NULL;
        g_pst_spinand_info = NULL;
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    g_pst_spinand_ext_info          = &g_pst_spinand_sni->spinand_ext_info;
    g_pst_spinand_ext_configuration = &g_pst_spinand_sni->spinand_ext_configuration;

    pst_profile = &g_pst_spinand_ext_info->st_profile;

    if (pst_profile->u16_flags & SPINAND_NEED_QE)
    {
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_access.st_qeStatus, 1);
    }

    if (pst_profile->st_eccConfig.u8_eccType & SPINAND_THRESHOLD)
    {
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_eccConfig.st_threshold, 1);
    }

    drv_spinand_unlock_whole_block(g_pst_spinand_ext_info);
    PalSpiflash_SetClk(g_pst_spinand_info->u8_MaxClk);

    drv_spinand_setup_by_sni(g_pst_spinand_ext_info);

    if (0 == g_pst_spinand_info->U8RIURead)
    {
        drv_spinand_use_bdma(1);
        FLASH_IMPL_PRINTF("[FLASH] BDMA mode.\r\n");
    }
    else
    {
        drv_spinand_use_bdma(0);
        FLASH_IMPL_PRINTF("[FLASH] RIU mode.\r\n");
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_software_init(u8 *sni_buf)
{
    if (!drv_spinand_is_sni_header(sni_buf) || !drv_spinand_is_ext_sni_header(sni_buf))
    {
        FLASH_IMPL_PRINTF("[FLASH] No sni!\r\n");
        return ERR_SPINAND_INVALID;
    }

    DRV_FSP_QSPI_BankSel();

    g_pst_spinand_sni               = (SPINAND_SNI_t *)sni_buf;
    g_pst_spinand_info              = &g_pst_spinand_sni->spinand_info;
    g_pst_spinand_ext_info          = &g_pst_spinand_sni->spinand_ext_info;
    g_pst_spinand_ext_configuration = &g_pst_spinand_sni->spinand_ext_configuration;

    drv_spinand_read              = drv_spinand_read_pages;
    drv_spinand_check_page_status = drv_spinand_check_page_status_default;
    u8_ecc_corrected              = 0;

    drv_spinand_setup_by_sni(g_pst_spinand_ext_info);

    if (0 == g_pst_spinand_info->U8RIURead)
    {
        drv_spinand_use_bdma(1);
        FLASH_IMPL_PRINTF("[FLASH] BDMA mode.\r\n");
    }
    else
    {
        drv_spinand_use_bdma(0);
        FLASH_IMPL_PRINTF("[FLASH] RIU mode.\r\n");
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_deinit(void)
{
    g_pst_spinand_sni               = NULL;
    g_pst_spinand_info              = NULL;
    g_pst_spinand_ext_info          = NULL;
    g_pst_spinand_ext_configuration = NULL;

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_reset(void)
{
    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_reset())
        return ERR_SPINAND_TIMEOUT;

    if (ERR_SPINAND_TIMEOUT == drv_spinand_check_status(NULL))
        return ERR_SPINAND_DEVICE_FAILURE;

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_set_ecc_mode(u8 u8_enabled)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_eccEnabled, u8_enabled);

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_read_status(u8 *pu8_status)
{
    return drv_spinand_check_status(pu8_status);
}

u8 mdrv_spinand_read_id(u8 *pu8_data, u8 u8_size)
{
    return drv_spinand_complete_read_id(pu8_data, u8_size);
}

u8 mdrv_spinand_page_read_raw(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status;

    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (!drv_spinand_soc_ecc_is_enable())
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_eccEnabled, 0);

    if (ERR_SPINAND_TIMEOUT <= drv_spinand_page_read_status(u32_page))
        return ERR_SPINAND_DEVICE_FAILURE;

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_read_from_cache(u32_page, u16_column, pu8_data, u32_size))
        return ERR_SPINAND_DEVICE_FAILURE;

    if (!drv_spinand_soc_ecc_is_enable())
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_eccEnabled, 1);

    return u8_status;
}

u8 mdrv_spinand_page_program_raw(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (!drv_spinand_soc_ecc_is_enable())
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_eccEnabled, 0);

    drv_spinand_die_sel(u32_page);

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_write_enable())
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Write enable fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    drv_spinand_wait_device_available();

    if (ERR_SPINAND_SUCCESS
        != drv_spinand_complete_program_load(st_spinand_drv_info.u8_programLoad,
                                             u16_column | drv_spinand_offset_wrap(u32_page, SPINAND_PROGRAM), pu8_data,
                                             u32_size))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Program load fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    drv_spinand_wait_device_available();

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_program_execute(u32_page))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Program execute fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    if (ERR_SPINAND_P_FAIL == drv_spinand_wait_device_available())
    {
        FLASH_IMPL_PRINTF("[FLASH] Program fail!\r\n");
        return ERR_SPINAND_P_FAIL;
    }

    if (!drv_spinand_soc_ecc_is_enable())
        drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_eccEnabled, 1);

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_page_read(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status;

    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    u8_status = drv_spinand_page_read_status(u32_page);

    if (drv_spinand_soc_ecc_is_enable())
        return drv_spinand_page_read_socecc(u32_page, u16_column, pu8_data, u32_size);
    else if (ERR_SPINAND_SUCCESS != drv_spinand_complete_read_from_cache(u32_page, u16_column, pu8_data, u32_size))
        return ERR_SPINAND_DEVICE_FAILURE;

    return u8_status;
}

u8 mdrv_spinand_page_program(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    drv_spinand_die_sel(u32_page);

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_write_enable())
    {
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    drv_spinand_wait_device_available();

    if (drv_spinand_soc_ecc_is_enable() && (u16_column < NAND_PAGE_SIZE))
    {
        if (ERR_SPINAND_SUCCESS != drv_spinand_page_program_socecc(u32_page, u16_column, pu8_data, u32_size))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] SOC ECC Program load fail!\r\n");
            return ERR_SPINAND_DEVICE_FAILURE;
        }
    }
    else
    {
        if (ERR_SPINAND_SUCCESS
            != drv_spinand_complete_program_load(st_spinand_drv_info.u8_programLoad,
                                                 u16_column | drv_spinand_offset_wrap(u32_page, SPINAND_PROGRAM),
                                                 pu8_data, u32_size))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] Program load fail!\r\n");
            return ERR_SPINAND_DEVICE_FAILURE;
        }
    }

    drv_spinand_wait_device_available();

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_program_execute(u32_page))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Program execute fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    if (ERR_SPINAND_P_FAIL == drv_spinand_wait_device_available())
    {
        FLASH_IMPL_PRINTF("[FLASH] Program fail!\r\n");
        return ERR_SPINAND_P_FAIL;
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_cache_program(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    drv_spinand_die_sel(u32_page);

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_write_enable())
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Write enable fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }
    drv_spinand_wait_device_available();
    if (ERR_SPINAND_SUCCESS
        != drv_spinand_complete_program_load(st_spinand_drv_info.u8_randomLoad,
                                             u16_column | drv_spinand_offset_wrap(u32_page, SPINAND_RANDOM), pu8_data,
                                             u32_size))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] cache Program load fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }
    drv_spinand_wait_device_available();
    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_program_execute(u32_page))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Program execute fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    if (ERR_SPINAND_P_FAIL == drv_spinand_wait_device_available())
    {
        FLASH_IMPL_PRINTF("[FLASH] Program fail!\r\n");
        return ERR_SPINAND_P_FAIL;
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_block_erase(u32 u32_page)
{
    u16 u16_block;

    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    u16_block = (u32_page & ~NAND_PAGES_PER_BLOCK_MASK) >> drv_spinand_count_bits(NAND_PAGES_PER_BLOCK);

    drv_spinand_die_sel(u32_page);

    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_write_enable())
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] Write enable fail!\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }
    drv_spinand_wait_device_available();
    if (ERR_SPINAND_SUCCESS != drv_spinand_complete_block_erase(u32_page))
    {
        FLASH_IMPL_PRINTF("[FLASH] Erase fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    if (ERR_SPINAND_E_FAIL == drv_spinand_check_status(NULL))
    {
        FLASH_IMPL_PRINTF_HEX("[FLASH] Erase fail @ block 0x", u16_block, "!\r\n");
        return ERR_SPINAND_E_FAIL;
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_is_support_otp(void)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (g_pst_spinand_ext_configuration->st_otpConfig.u8_otpEn)
        return ERR_SPINAND_SUCCESS;
    else
        return ERR_SPINAND_DEVICE_FAILURE;
}

u8 mdrv_spinand_is_support_sniecc(void)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_eccEn)
        return ERR_SPINAND_SUCCESS;
    else
        return ERR_SPINAND_DEVICE_FAILURE;
}

u8 mdrv_spinand_is_support_ubibbm(void)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (SPINAND_ENABLE_UBI_BBM & g_pst_spinand_ext_info->st_profile.st_eccConfig.u8_eccType)
        return ERR_SPINAND_SUCCESS;
    else
        return ERR_SPINAND_DEVICE_FAILURE;
}

u8 mdrv_spinand_is_socecc(void)
{
    return drv_spinand_soc_ecc_is_enable();
}

u8 mdrv_spinand_get_otp_layout(u32 *start, u32 *length, u8 mode)
{
    SPINAND_OTP_CONFIG_t *pst_otpConfig;
    SPINAND_OTP_INFO *    st_factory;
    SPINAND_OTP_INFO *    pst_user;

    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    pst_otpConfig = &g_pst_spinand_ext_configuration->st_otpConfig;
    st_factory    = &pst_otpConfig->st_factory;
    pst_user      = &pst_otpConfig->st_user;

    if (!pst_otpConfig->u8_otpEn)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (mode)
    {
        *start  = pst_user->start;
        *length = pst_user->length;
    }
    else
    {
        *start  = st_factory->start;
        *length = st_factory->length;
    }

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_set_otp_mode(u8 u8_enabled)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    if (!g_pst_spinand_ext_configuration->st_otpConfig.u8_otpEn)
    {
        return ERR_SPINAND_INVALID;
    }

    drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_extConfig.st_otp.st_otpEnabled, u8_enabled);

    return ERR_SPINAND_SUCCESS;
}

u8 mdrv_spinand_get_otp_lock(void)
{
    u32            u32_otp_status;
    SPINAND_OTP_t *pst_otp;

    if (!g_pst_spinand_sni)
        return 0;

    pst_otp = &g_pst_spinand_ext_info->st_profile.st_extConfig.st_otp;

    if (ERR_SPINAND_SUCCESS
        != drv_spinand_complete_get_features(SPI_NAND_CMD_GF, pst_otp->st_otpLock.u32_address, (u8 *)&u32_otp_status,
                                             pst_otp->st_otpEnabled.u16_dataBytes))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR]get features failed\r\n");
        return 0;
    }

    if (u32_otp_status & pst_otp->st_otpLock.u16_value)
    {
        return 1;
    }

    return 0;
}

u8 mdrv_spinand_set_otp_lock(void)
{
    u8             u8_status;
    SPINAND_OTP_t *pst_otp;

    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    pst_otp = &g_pst_spinand_ext_info->st_profile.st_extConfig.st_otp;

    if (!mdrv_spinand_set_otp_mode(1))
    {
        FLASH_IMPL_PRINTF("[FLASH] enable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    do
    {
        if (ERR_SPINAND_SUCCESS != (u8_status = drv_spinand_complete_write_enable()))
        {
            FLASH_IMPL_PRINTF("[FLASH] write enable fail\r\n");
            break;
        }

        drv_spinand_switch_config(&pst_otp->st_otpLock, 1);

        if (ERR_SPINAND_SUCCESS != (u8_status = drv_spinand_complete_execute()))
        {
            FLASH_IMPL_PRINTF("[FLASH] execute fail\r\n");
            break;
        }

        if (ERR_SPINAND_SUCCESS != (u8_status = drv_spinand_wait_device_available()))
        {
            FLASH_IMPL_PRINTF("[FLASH] write otp timeout fail\r\n");
            break;
        }
    } while (0);

    if (!mdrv_spinand_set_otp_mode(0))
    {
        FLASH_IMPL_PRINTF("[FLASH] disable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return u8_status;
}

u32 mdrv_spinand_read_otp(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u32                      u32_read_size;
    SPINAND_ACCESS_CONFIG_t *pst_access;
    SPINAND_OTP_CONFIG_t *   pst_otpConfig;

    if (!g_pst_spinand_sni)
        return 0;

    pst_access    = &g_pst_spinand_ext_info->st_profile.st_access;
    pst_otpConfig = &g_pst_spinand_ext_configuration->st_otpConfig;

    if (!mdrv_spinand_set_otp_mode(1))
    {
        FLASH_IMPL_PRINTF("[FLASH] enable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    drv_spinand_setup_access(pst_otpConfig->st_otpread.u8_command, pst_otpConfig->st_otpread.u8_dummy,
                             pst_access->st_program.u8_command, pst_access->st_random.u8_command);

    u32_read_size = 0;

    if (u16_column & NAND_PAGE_SIZE_MASK)
    {
        u32_read_size = NAND_PAGE_SIZE - u16_column;
        u32_read_size = (u32_read_size > u32_size) ? u32_size : u32_read_size;

        mdrv_spinand_page_read(u32_page, u16_column, pu8_data, u32_read_size);

        u32_page += 1;
        pu8_data += u32_read_size;
        u32_size -= u32_read_size;
    }

    u32_read_size += drv_spinand_read_pages(u32_page, pu8_data, u32_size);

    drv_spinand_setup_access(pst_access->st_read.u8_command, pst_access->st_read.u8_dummy,
                             pst_access->st_program.u8_command, pst_access->st_random.u8_command);

    if (!mdrv_spinand_set_otp_mode(0))
    {
        FLASH_IMPL_PRINTF("[FLASH] disable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return u32_read_size;
}

u32 mdrv_spinand_write_otp(u32 u32_page, u16 u16_column, u8 *pu8_data, u32 u32_size)
{
    u32                      u32_write_size;
    SPINAND_ACCESS_CONFIG_t *pst_access;
    SPINAND_OTP_CONFIG_t *   pst_otpConfig;

    if (!g_pst_spinand_sni)
        return 0;

    pst_access    = &g_pst_spinand_ext_info->st_profile.st_access;
    pst_otpConfig = &g_pst_spinand_ext_configuration->st_otpConfig;

    if (!mdrv_spinand_set_otp_mode(1))
    {
        FLASH_IMPL_PRINTF("[FLASH] enable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    drv_spinand_setup_access(pst_access->st_read.u8_command, pst_access->st_read.u8_dummy,
                             pst_otpConfig->st_otpprogram.u8_command, pst_access->st_random.u8_command);

    u32_write_size = 0;

    if (u16_column & NAND_PAGE_SIZE_MASK)
    {
        u32_write_size = NAND_PAGE_SIZE - u16_column;
        u32_write_size = (u32_write_size > u32_size) ? u32_size : u32_write_size;

        mdrv_spinand_page_program(u32_page, u16_column, pu8_data, u32_write_size);

        u32_page += 1;
        pu8_data += u32_write_size;
        u32_size -= u32_write_size;
    }

    u32_write_size += mdrv_spinand_pages_program(u32_page, pu8_data, u32_size);

    drv_spinand_setup_access(pst_access->st_read.u8_command, pst_access->st_read.u8_dummy,
                             pst_access->st_program.u8_command, pst_access->st_random.u8_command);

    if (!mdrv_spinand_set_otp_mode(0))
    {
        FLASH_IMPL_PRINTF("[FLASH] disable opt fail\r\n");
        return ERR_SPINAND_DEVICE_FAILURE;
    }

    return u32_write_size;
}

void mdrv_spinand_set_ecc_correct_flag(u8 u8_value)
{
    u8_ecc_corrected = u8_value;
}

u8 mdrv_spinand_get_ecc_correct_flag(void)
{
    return u8_ecc_corrected;
}

u8 mdrv_spinand_page_read_status(u32 u32_page)
{
    if (!g_pst_spinand_sni)
        return ERR_SPINAND_DEVICE_FAILURE;

    return drv_spinand_page_read_status(u32_page);
}

u32 mdrv_spinand_block_isbad(u32 u32_page)
{
    u8 u8_bad;

    if (!g_pst_spinand_sni)
        return 0;

    u8_bad = 0x00;

    if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read(u32_page, NAND_PAGE_SIZE, &u8_bad, 1))
    {
        FLASH_IMPL_PRINTF("[FLASH] Read data failed\r\n");
        return 0;
    }

    if (0xFF != u8_bad)
    {
        return 1;
    }

    return 0;
}

u32 mdrv_spinand_pages_read(u32 u32_page, u8 *pu8_data, u32 u32_size)
{
    if (!g_pst_spinand_sni)
        return 0;

    return drv_spinand_read(u32_page, pu8_data, u32_size);
}

u32 mdrv_spinand_pages_program(u32 u32_page, u8 *pu8_data, u32 u32_size)
{
    u32 u32_write_size;
    u32 u32_bytes_left;

    if (!g_pst_spinand_sni)
        return 0;

    u32_bytes_left = u32_size;

    while (0 != u32_bytes_left)
    {
        u32_write_size = drv_spinand_get_bytes_left(u32_bytes_left, NAND_PAGE_SIZE);

        if (ERR_SPINAND_SUCCESS != mdrv_spinand_page_program(u32_page++, 0, pu8_data, u32_write_size))
        {
            break;
        }

        u32_bytes_left -= u32_write_size;
        pu8_data += u32_write_size;
    }

    u32_size -= u32_bytes_left;

    return u32_size;
}

u32 mdrv_spinand_blocks_erase(u32 u32_page, u32 u32_size)
{
    u32 u32_bytes_left;

    if (!g_pst_spinand_sni)
        return 0;

    u32_bytes_left = u32_size;

    while (0 != u32_bytes_left)
    {
        if (ERR_SPINAND_SUCCESS != mdrv_spinand_block_erase(u32_page))
        {
            break;
        }

        u32_bytes_left -= NAND_BLOCK_SIZE;
        u32_page += NAND_PAGES_PER_BLOCK;
    }

    u32_size -= u32_bytes_left;

    return u32_size;
}

void mdrv_spinand_setup(void)
{
    SPINAND_EXT_PROFILE_t *pst_profile;

    if (g_pst_spinand_ext_info)
    {
        if (drv_spinand_is_ext_sni_header((u8 *)g_pst_spinand_sni))
        {
            pst_profile = &g_pst_spinand_ext_info->st_profile;

            if (pst_profile->u16_flags & SPINAND_NEED_QE)
            {
                drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_access.st_qeStatus, 1);
            }
            if (pst_profile->st_eccConfig.u8_eccType & SPINAND_THRESHOLD)
            {
                drv_spinand_switch_config(&g_pst_spinand_ext_info->st_profile.st_eccConfig.st_threshold, 1);
            }
        }
    }
}