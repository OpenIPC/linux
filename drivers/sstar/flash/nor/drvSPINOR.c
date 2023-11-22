/*
 * drvSPINOR.c- Sigmastar
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
#include <drvSPINOR.h>
#include <mdrvFlashOsImpl.h>
#include <drvFSP_QSPI.h>
#include <mdrvSNI.h>

typedef struct
{
    u8  u8_readData;
    u8  u8_dummy;
    u8  u8_pageProgram;
    u32 u32_time_wait;
} DRV_SPINOR_INFO_t;

static u8                    g_u8_ext;
static SPINOR_SNI_t *        g_pst_spinor_sni;
static SPINOR_INFO_t *       g_pst_spinor_info;
static SPINOR_EXT_PROFILE_t *g_pst_spinor_ext_profile;

static DRV_SPINOR_INFO_t st_spinor_drv_info;

const u8 au8_basicMagicData[] = {0x4D, 0x53, 0x54, 0x41, 0x52, 0x53, 0x45, 0x4D,
                                 0x49, 0x55, 0x53, 0x46, 0x44, 0x43, 0x49, 0x53};

static u8 (*drv_spinor_read_data)(u32, u8 *, u32);
static u8 (*drv_spinor_program_load_data)(u8 *, u8, u8 *, u32);

#define SPINOR_FLASH_SIZE       g_pst_spinor_info->u32_Capacity
#define SPINOR_PAGE_SIZE        g_pst_spinor_info->u16_PageByteCnt
#define SPINOR_SECTOR_SIZE      g_pst_spinor_info->u16_SectorByteCnt
#define SPINOR_BLOCK_SIZE       g_pst_spinor_info->u32_BlkBytesCnt
#define SPINOR_PAGE_SIZE_MASK   (g_pst_spinor_info->u16_PageByteCnt - 1)
#define SPINOR_SECTOR_SIZE_MASK (g_pst_spinor_info->u16_SectorByteCnt - 1)
#define SPINOR_BLOCK_SIZE_MASK  (g_pst_spinor_info->u32_BlkBytesCnt - 1)

void __attribute__((weak)) PalSpiflash_SetClk(u8 u8Clk)
{
    FLASH_IMPL_UNUSED_VAR(u8Clk);
}

static u8 drv_spinor_get_cmd_mode(u8 u8_cmd)
{
    switch (u8_cmd)
    {
        case SPI_NOR_CMD_QP:
        case SPI_NOR_CMD_4PP:
        case SPI_NOR_CMD_QR_6B:
        case SPI_NOR_CMD_QR_EB:
            return FSP_QUAD_MODE;
        case 0x3B:
        case 0xBB:
            return FSP_DUAL_MODE;
        default:
            return FSP_SINGLE_MODE;
    }
}

static u8 drv_spinor_return_status(u8 u8_status)
{
    if (ERR_SPINOR_INVALID <= u8_status)
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    return u8_status;
}

#if defined(CONFIG_FLASH_FIX_DI_PULL_DOWN)
static u8 drv_spinor_receive_data(u8 *pu8_buf, u32 u32_size)
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
        return ERR_SPINOR_TIMEOUT;
    }

    return ERR_SPINOR_SUCCESS;
}
#endif

static u8 drv_spinor_read_after_write(u8 *pu8_wbuf, u32 u32_wsize, u8 *pu8_rbuf, u32 u32_rsize)
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
static u8 drv_spinor_simple_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
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
        return ERR_SPINOR_TIMEOUT;
    }

    return ERR_SPINOR_SUCCESS;
}

#else
static u8 drv_spinor_simple_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
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
        return ERR_SPINOR_TIMEOUT;
    }

    return ERR_SPINOR_SUCCESS;
}

#endif

static u8 drv_spinor_complete_transmission(u8 *pu8_wbuf, u8 u8_wsize, u8 *pu8_buf, u32 u32_size)
{
    u8 u8_status;

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinor_simple_transmission(pu8_wbuf, u8_wsize, pu8_buf, u32_size);
    DRV_QSPI_pull_cs(1);

    return drv_spinor_return_status(u8_status);
}

static u8 drv_spinor_complete_read_status(u8 u8_cmd, u8 *pu8_status, u8 u8_size)
{
    u8 u8_status = ERR_SPINOR_SUCCESS;

    DRV_QSPI_pull_cs(0);

    if (!drv_spinor_read_after_write(&u8_cmd, 1, pu8_status, u8_size))
    {
        u8_status = ERR_SPINOR_TIMEOUT;
    }

    DRV_QSPI_pull_cs(1);

    return drv_spinor_return_status(u8_status);
}

#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinor_complete_write_status(u8 u8_cmd, u8 *pu8_status, u8 u8_size)
{
    u8 u8_status;

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinor_simple_transmission(&u8_cmd, 1, pu8_status, u8_size);
    DRV_QSPI_pull_cs(1);

    return u8_status;
}
#else
static u8 drv_spinor_complete_write_status(u8 u8_cmd, u8 *pu8_status, u8 u8_size)
{
    u8 u8_status;

    DRV_QSPI_pull_cs(0);
    if (ERR_SPINOR_SUCCESS == (u8_status = drv_spinor_simple_transmission(NULL, 0, &u8_cmd, 1)))
    {
        u8_status = drv_spinor_simple_transmission(NULL, 0, pu8_status, u8_size);
    }
    DRV_QSPI_pull_cs(1);

    return u8_status;
}
#endif

static u8 drv_spinor_complete_write_enable(void)
{
    u8 u8_cmd = SPI_NOR_CMD_WREN;

    return drv_spinor_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinor_complete_enable_reset(void)
{
    u8 u8_cmd = SPI_NOR_CMD_EN_RESET;

    return drv_spinor_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinor_complete_reset_device(void)
{
    u8 u8_cmd = SPI_NOR_CMD_RESET;

    return drv_spinor_complete_transmission(NULL, 0, &u8_cmd, 1);
}

static u8 drv_spinor_complete_use_ext(u8 u8_ext)
{
    u8 u8_retry = 5;
    u8 au8_wcmd[2];
    u8 u8_rcmd;

    au8_wcmd[0] = SPI_NOR_CMD_WREAR;
    au8_wcmd[1] = (u8_ext == 0xFF) ? 0 : u8_ext;

    do
    {
        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_write_enable())
        {
            break;
        }

        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_transmission(NULL, 0, au8_wcmd, 2))
        {
            break;
        }

        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_read_status(SPI_NOR_CMD_RDEAR, &u8_rcmd, 1))
        {
            break;
        }

        if (au8_wcmd[1] == u8_rcmd)
        {
            return ERR_SPINOR_SUCCESS;
        }
    } while (u8_retry--);

    if (!u8_retry && (u8_ext != 0xFF))
    {
        FLASH_IMPL_PRINTF("[FLASH] Set ext address register fail, retry 5\n");
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    FLASH_IMPL_PRINTF("[FLASH] No ext register\n");

    return ERR_SPINOR_SUCCESS;
}

static u8 drv_spinor_check_status(void)
{
    u32 u32_left_time;
    u32 u32_status;

    u32_left_time = st_spinor_drv_info.u32_time_wait;

    do
    {
        if (0 == u32_left_time)
        {
            FLASH_IMPL_PRINTF_HEX("[SPINOR] check timeout! @0x", st_spinor_drv_info.u32_time_wait, " us\r\n");
            return ERR_SPINOR_TIMEOUT;
        }

        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_read_status(SPI_NOR_CMD_RDSR, (u8 *)&u32_status, 1))
        {
            FLASH_IMPL_PRINTF("[SPINOR] DRV_SPINOR_read_status1 timeout!\r\n");
            return ERR_SPINOR_TIMEOUT;
        }

        if (0 == (u32_status & SPI_NOR_BUSY))
        {
            break;
        }

        FLASH_IMPL_USDELAY(10);

        if (10 < u32_left_time)
        {
            u32_left_time -= 10;
        }
        else
        {
            u32_left_time = 0;
        }

    } while (1);

    return ERR_SPINOR_SUCCESS;
}

static u8 drv_spinor_reset(void)
{
    u8 u8_status;

    if (ERR_SPINOR_SUCCESS == (u8_status = drv_spinor_complete_enable_reset()))
    {
        u8_status = drv_spinor_complete_reset_device();
    }

    if (ERR_SPINOR_SUCCESS == u8_status)
    {
        u8_status = drv_spinor_check_status();
    }

    if (ERR_SPINOR_SUCCESS != u8_status)
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    return u8_status;
}

static u8 drv_spinor_write_status(u8 u8_cmd, u8 *pu8_status, u8 u8_size)
{
    u8 u8_status;

    if (ERR_SPINOR_SUCCESS != drv_spinor_complete_write_enable())
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    u8_status = drv_spinor_complete_write_status(u8_cmd, pu8_status, u8_size);

    if (ERR_SPINOR_SUCCESS == u8_status)
    {
        u8_status = drv_spinor_check_status();
    }

    return drv_spinor_return_status(u8_status);
}

static u8 drv_spinor_read_id(u8 *pu8_ids, u8 u8_bytes)
{
    u8 u8_status;

    u8_status = drv_spinor_complete_read_status(SPI_NOR_CMD_RDID, pu8_ids, u8_bytes);

    return drv_spinor_return_status(u8_status);
}

#if defined(CONFIG_FLASH_BDMA)
#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinor_program_load_data_by_bdma(u8 *pu8_cmd, u8 u8_wsize, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status = ERR_SPINOR_SUCCESS;

    FLASH_IMPL_MEM_FLUSH((void *)pu8_data, u32_size);
    FLASH_IMPL_MIUPIPE_FLUSH();

    DRV_FSP_init(FSP_USE_SINGLE_CMD);

    if (u8_wsize != DRV_FSP_set_which_cmd_size(1, u8_wsize))
        return ERR_SPINOR_DEVICE_FAILURE;

    do
    {
        DRV_FSP_write_wbf(pu8_cmd, u8_wsize);
        DRV_FSP_enable_outside_wbf(FSP_OUTSIDE_MODE_BDMA, 0, u8_wsize, u32_size);

        DRV_FSP_trigger();

        if (DRV_BDMA_PROC_DONE != FLASH_IMPL_BdmaTransfer(BDMA_MIU2SPI, pu8_data, 0, u32_size))
        {
            u8_status = ERR_SPINOR_BDMA_FAILURE;
            break;
        }

        if (!DRV_FSP_is_done())
        {
            u8_status = ERR_SPINOR_TIMEOUT;
        }
    } while (0);

    DRV_FSP_clear_trigger();
    DRV_FSP_disable_outside_wbf();

    return drv_spinor_return_status(u8_status);
}

#else
static u8 drv_spinor_program_load_data_by_bdma(u8 *pu8_cmd, u8 u8_wsize, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status = ERR_SPINOR_SUCCESS;

    FLASH_IMPL_MEM_FLUSH((void *)pu8_data, u32_size);

    DRV_FSP_init(FSP_USE_SINGLE_CMD);
    DRV_FSP_set_which_cmd_size(1, 0);
    DRV_FSP_enable_outside_wbf(FSP_OUTSIDE_MODE_BDMA, 0, 0, u32_size);
    do
    {
        DRV_FSP_trigger();
        FLASH_IMPL_MIUPIPE_FLUSH();

        if (DRV_BDMA_PROC_DONE != FLASH_IMPL_BdmaTransfer(BDMA_MIU2SPI, pu8_data, 0, u32_size))
        {
            u8_status = ERR_SPINOR_BDMA_FAILURE;
            break;
        }

        if (!DRV_FSP_is_done())
        {
            u8_status = ERR_SPINOR_TIMEOUT;
        }
    } while (0);

    DRV_FSP_clear_trigger();
    DRV_FSP_disable_outside_wbf();

    return drv_spinor_return_status(u8_status);
}
#endif
#endif

#if defined(CONFIG_FLASH_HW_CS)
static u8 drv_spinor_complete_program_page(u8 u8_cmd, u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status = ERR_SPINOR_SUCCESS;
    u8 au8_cmd[4];

    au8_cmd[0] = u8_cmd;
    au8_cmd[1] = (u32_address >> 16) & 0xff;
    au8_cmd[2] = (u32_address >> 8) & 0xff;
    au8_cmd[3] = u32_address & 0xff;

    u8_status = drv_spinor_program_load_data(au8_cmd, 4, pu8_data, u32_size);

    return drv_spinor_return_status(u8_status);
}

#else
static u8 drv_spinor_complete_program_page(u8 u8_cmd, u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINOR_SUCCESS;
    u8  au8_cmd[3];
    u32 u32_bytes_to_load;

    au8_cmd[0] = (u32_address >> 16) & 0xff;
    au8_cmd[1] = (u32_address >> 8) & 0xff;
    au8_cmd[2] = u32_address & 0xff;

    DRV_QSPI_pull_cs(0);
    do
    {
        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_simple_transmission(NULL, 0, &u8_cmd, 1)))
        {
            break;
        }

        if (SPI_NOR_CMD_4PP == u8_cmd)
        {
            DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(u8_cmd));
        }

        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_simple_transmission(NULL, 0, au8_cmd, 3)))
        {
            break;
        }

        DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(u8_cmd));

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
                u8_status = drv_spinor_simple_transmission(NULL, 0, pu8_data, u32_bytes_to_load);
            }
            else
            {
                u8_status = drv_spinor_program_load_data(NULL, 0, pu8_data, u32_bytes_to_load);
            }

            if (ERR_SPINOR_SUCCESS != u8_status)
            {
                break;
            }

            u32_size -= u32_bytes_to_load;
            pu8_data += u32_bytes_to_load;
        }

        DRV_FSP_set_access_mode(FSP_SINGLE_MODE);
    } while (0);
    DRV_QSPI_pull_cs(1);

    return drv_spinor_return_status(u8_status);
}
#endif

static u8 drv_spinor_program_page(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 u8_ext;

    u8_ext = (u32_address & ~SPI_NOR_16MB_MASK) ? 1 : 0;

    if (g_u8_ext != u8_ext)
    {
        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_use_ext(u8_ext))
        {
            return ERR_SPINOR_DEVICE_FAILURE;
        }
    }

    g_u8_ext = u8_ext;

    if (ERR_SPINOR_SUCCESS != drv_spinor_complete_write_enable())
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    if (ERR_SPINOR_SUCCESS != drv_spinor_check_status())
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    if (ERR_SPINOR_SUCCESS
        != drv_spinor_complete_program_page(st_spinor_drv_info.u8_pageProgram, (u32_address & SPI_NOR_16MB_MASK),
                                            pu8_data, u32_size))
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    if (ERR_SPINOR_SUCCESS != drv_spinor_check_status())
    {
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    return ERR_SPINOR_SUCCESS;
}

#if defined(CONFIG_FLASH_FIX_DI_PULL_DOWN)
static u8 drv_spinor_complete_read_data_by_riu(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status;
    u8 au8_cmd[4];
    u8 u8_dummy = 0;

    DRV_QSPI_pull_cs(0);
    do
    {
        if (ERR_SPINOR_SUCCESS
            != (u8_status = drv_spinor_simple_transmission(NULL, 0, &st_spinor_drv_info.u8_readData, 1)))
        {
            break;
        }

        au8_cmd[0] = (u32_address >> 16) & 0xff;
        au8_cmd[1] = (u32_address >> 8) & 0xff;
        au8_cmd[2] = u32_address & 0xff;

        u8_dummy = (st_spinor_drv_info.u8_dummy * 1) / 8;

        if (0xEB == st_spinor_drv_info.u8_readData)
        {
            DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(st_spinor_drv_info.u8_readData));
            u8_dummy = (st_spinor_drv_info.u8_dummy * 4) / 8;
        }
        else if (0xBB == st_spinor_drv_info.u8_readData)
        {
            u8_dummy = (st_spinor_drv_info.u8_dummy * 2) / 8;
            DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(st_spinor_drv_info.u8_readData));
        }

        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_simple_transmission(NULL, 0, au8_cmd, 3)))
        {
            break;
        }

        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_simple_transmission(NULL, 0, NULL, u8_dummy)))
        {
            break;
        }

        DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(st_spinor_drv_info.u8_readData));

        if (ERR_SPINOR_SUCCESS == u8_status)
        {
            u8_status = drv_spinor_receive_data(pu8_data, u32_size);
        }

        DRV_FSP_set_access_mode(FSP_SINGLE_MODE);

    } while (0);
    DRV_QSPI_pull_cs(1);

    return drv_spinor_return_status(u8_status);
}
#else
static u8 drv_spinor_complete_read_data_by_riu(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 au8_cmd[8];
    u8 u8_rsize;
    u8 u8_status = ERR_SPINOR_SUCCESS;

    do
    {
        au8_cmd[0] = SPI_NOR_CMD_FASTREAD;       // command
        au8_cmd[1] = (u32_address >> 16) & 0xff; // address
        au8_cmd[2] = (u32_address >> 8) & 0xff;  // address
        au8_cmd[3] = u32_address & 0xff;         // address
        au8_cmd[4] = 0;                          // dummy

        DRV_FSP_set_access_mode(drv_spinor_get_cmd_mode(au8_cmd[0]));
        DRV_QSPI_pull_cs(0);

        u8_rsize = drv_spinor_read_after_write(au8_cmd, 5, pu8_data, u32_size);

        DRV_QSPI_pull_cs(1);

        u32_address += u8_rsize;
        pu8_data += u8_rsize;
        u32_size -= u8_rsize;
    } while (u32_size && u8_rsize);

    DRV_FSP_set_access_mode(FSP_SINGLE_MODE);

    if (!u8_rsize)
        u8_status = ERR_SPINOR_TIMEOUT;

    return drv_spinor_return_status(u8_status);
}
#endif

#if defined(CONFIG_FLASH_BDMA)
static u8 drv_spinor_complete_read_data_by_bdma(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status = ERR_SPINOR_SUCCESS;

    FLASH_IMPL_MEM_INVALIDATE((void *)pu8_data, u32_size);

    DRV_QSPI_pull_cs(0);
    DRV_QSPI_use_3bytes_address_mode(DRV_QSPI_cmd_to_mode(st_spinor_drv_info.u8_readData), st_spinor_drv_info.u8_dummy);

    if (DRV_BDMA_PROC_DONE
        != FLASH_IMPL_BdmaTransfer(BDMA_SPI2MIU, (u8 *)(unsigned long)u32_address, pu8_data, u32_size))
    {
        u8_status = ERR_SPINOR_BDMA_FAILURE;
    }

    DRV_QSPI_use_3bytes_address_mode(DRV_QSPI_cmd_to_mode(SPI_NOR_CMD_FASTREAD), 8);
    DRV_QSPI_pull_cs(1);

    FLASH_IMPL_MEM_INVALIDATE((void *)pu8_data, u32_size);

    DRV_ISP_disable_address_continue();
    return drv_spinor_return_status(u8_status);
}
#endif

#if 0
static u8 drv_spinor_complete_read(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINOR_SUCCESS;
    u32 u32_bytes_to_read;

    while (0 != u32_size)
    {
        u32_bytes_to_read = (u32_size & ~0x3f);
        /*align to 16,patch for bdma*/
        if (0 != (u32_address % 16))
        {
            u32_bytes_to_read = 16 - (u32_address % 16);
        }
        else
        {
            /*align to 64,for cache*/
            if (0 != ((unsigned long)pu8_data % 64))
            {
                u32_bytes_to_read = 64 - ((unsigned long)pu8_data % 64);
            }

            if (0 == u32_bytes_to_read)
            {
                u32_bytes_to_read = u32_size & 0x3f;
            }
        }

        if (u32_size < u32_bytes_to_read)
        {
            u32_bytes_to_read = u32_size;
        }
        /*align to 64,for cache*/
        if (0 != (u32_bytes_to_read % 64))
        {
            u8_status = drv_spinor_complete_read_data_by_riu(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_bytes_to_read);
        }
        else
        {
            u8_status = drv_spinor_read_data(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_bytes_to_read);
        }

        if (ERR_SPINOR_SUCCESS != u8_status)
        {
            break;
        }

        pu8_data += u32_bytes_to_read;
        u32_address += u32_bytes_to_read;
        u32_size -= u32_bytes_to_read;
    }

    return drv_spinor_return_status(u8_status);
}

#else
static u8 drv_spinor_complete_read(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_status = ERR_SPINOR_SUCCESS;
    u32 u32_rsize;

#if !defined(CONFIG_FLASH_FIX_BDMA_ALIGN)
    if (0 != ((unsigned long)pu8_data % 16) || 0 != (u32_address % 16))
    {
        u8_status = drv_spinor_complete_read_data_by_riu(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_size);
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
                u8_status = drv_spinor_complete_read_data_by_riu(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_rsize);
                u32_address += u32_rsize;
                pu8_data += u32_rsize;
                u32_size -= u32_rsize;
            }

            if (u8_status != ERR_SPINOR_SUCCESS || !u32_size)
                break;

            u32_rsize = u32_size & ~0x3F;
            if (u32_rsize)
            {
                u8_status = drv_spinor_read_data(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_rsize);
                u32_address += u32_rsize;
                pu8_data += u32_rsize;
                u32_size -= u32_rsize;
            }

            if (u8_status != ERR_SPINOR_SUCCESS || !u32_size)
                break;

            if (u32_size)
                u8_status = drv_spinor_complete_read_data_by_riu(u32_address & SPI_NOR_16MB_MASK, pu8_data, u32_size);
        } while (0);
    }

    return drv_spinor_return_status(u8_status);
}
#endif

static u8 drv_spinor_read(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8  u8_ext;
    u8  u8_status = ERR_SPINOR_SUCCESS;
    u32 u32_read_size;

    while (0 != u32_size)
    {
        u32_read_size = SPI_NOR_16MB - (u32_address & SPI_NOR_16MB_MASK);
        if (u32_size < u32_read_size)
        {
            u32_read_size = u32_size;
        }

        u8_ext = (u32_address & ~SPI_NOR_16MB_MASK) ? 1 : 0;

        if ((g_u8_ext != u8_ext) && (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_complete_use_ext(u8_ext))))
        {
            break;
        }

        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_complete_read(u32_address, pu8_data, u32_read_size)))
        {
            break;
        }

        g_u8_ext = u8_ext;

        u32_address += u32_read_size;
        pu8_data += u32_read_size;
        u32_size -= u32_read_size;

        if (ERR_SPINOR_SUCCESS != u8_status)
        {
            break;
        }
    }

    return drv_spinor_return_status(u8_status);
}

static u8 drv_spinor_complete_erase(u8 u8_command, u32 u32_address)
{
    u8 u8_status;
    u8 au8_cmd[4];
    u8 u8_wr_size;

    u8_wr_size            = 0;
    au8_cmd[u8_wr_size++] = u8_command;
    au8_cmd[u8_wr_size++] = (u32_address >> 16) & 0xff;
    au8_cmd[u8_wr_size++] = (u32_address >> 8) & 0xff;
    au8_cmd[u8_wr_size++] = u32_address & 0xff;

    DRV_QSPI_pull_cs(0);
    u8_status = drv_spinor_simple_transmission(NULL, 0, au8_cmd, 4);
    DRV_QSPI_pull_cs(1);

    return u8_status;
}

static u8 drv_spinor_erase(u8 u8_command, u32 u32_address)
{
    u8 u8_status;
    u8 u8_ext;

    u8_ext = (u32_address & ~SPI_NOR_16MB_MASK) ? 1 : 0;

    do
    {
        if (g_u8_ext != u8_ext)
        {
            if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_complete_use_ext(u8_ext)))
            {
                break;
            }
        }

        if (ERR_SPINOR_SUCCESS != drv_spinor_complete_write_enable()
            || ERR_SPINOR_SUCCESS != drv_spinor_complete_erase(u8_command, u32_address))
        {
            return ERR_SPINOR_DEVICE_FAILURE;
        }

        g_u8_ext = u8_ext;

        if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_check_status()))
        {
            return ERR_SPINOR_DEVICE_FAILURE;
        }
    } while (0);

    return drv_spinor_return_status(u8_status);
}

static u8 drv_spinor_block_erase(u32 u32_address)
{
    return drv_spinor_erase(SPI_NOR_CMD_64BE, u32_address);
}

static u8 drv_spinor_sector_erase(u32 u32_address)
{
    return drv_spinor_erase(SPI_NOR_CMD_SE, u32_address);
}

static void drv_spinor_use_bdma(u8 u8_enabled)
{
#if defined(CONFIG_FLASH_BDMA)
    if (u8_enabled)
    {
        drv_spinor_program_load_data = drv_spinor_program_load_data_by_bdma;
        drv_spinor_read_data         = drv_spinor_complete_read_data_by_bdma;
    }
    else
#endif
    {
        drv_spinor_program_load_data = drv_spinor_simple_transmission;
        drv_spinor_read_data         = drv_spinor_complete_read_data_by_riu;
    }
}

static void drv_spinor_setup_time_wait(u32 u32_time_wait)
{
    st_spinor_drv_info.u32_time_wait = u32_time_wait;
}

static void drv_spinor_setup_access(u8 u8_read, u8 u8_dummy, u8 u8_program)
{
    st_spinor_drv_info.u8_readData = u8_read;
    st_spinor_drv_info.u8_dummy    = u8_dummy;
#if defined(CONFIG_FLASH_HW_CS)
    st_spinor_drv_info.u8_pageProgram = 0x02;
#else
    st_spinor_drv_info.u8_pageProgram = u8_program;
#endif

    FLASH_IMPL_PRINTF_HEX("[SPINOR] ReadData = 0x", st_spinor_drv_info.u8_readData, "\r\n");
    FLASH_IMPL_PRINTF_HEX("[SPINOR] Dummy = 0x", st_spinor_drv_info.u8_dummy, "\r\n");
    FLASH_IMPL_PRINTF_HEX("[SPINOR] pageProgram = 0x", st_spinor_drv_info.u8_pageProgram, "\r\n");
}

static u8 drv_spinor_init(void)
{
    u8 u8_status = 0;

    DRV_FSP_QSPI_init();
#if defined(CONFIG_FLASH_HW_CS)
    DRV_QSPI_use_sw_cs(0);
    DRV_QSPI_set_timeout(1, 0);
#endif

    st_spinor_drv_info.u8_readData    = SPI_NOR_CMD_FASTREAD;
    st_spinor_drv_info.u8_dummy       = 0x08;
    st_spinor_drv_info.u8_pageProgram = SPI_NOR_CMD_PP;
    st_spinor_drv_info.u32_time_wait  = -1;

    DRV_QSPI_use_3bytes_address_mode(DRV_QSPI_cmd_to_mode(SPI_NOR_CMD_FASTREAD), 0x08);

    drv_spinor_read_data         = drv_spinor_complete_read_data_by_riu;
    drv_spinor_program_load_data = drv_spinor_simple_transmission;

    u8_status = drv_spinor_reset();
    return u8_status;
}

static void drv_spinor_switch_config(FLASH_CMD_SET_t *pt_cmt, u8 u8_enabled)
{
    u8  u8_fn;
    u8  u8_status;
    u8  u8_status_bytes;
    u8  read_reg_cmd[] = {SPI_NOR_CMD_RDSR, SPI_NOR_CMD_RDSR2, SPI_NOR_CMD_RDSR3};
    u32 u32_status;

    u32_status      = 0;
    u8_status_bytes = 0;
    for (u8_fn = 0; sizeof(read_reg_cmd) / sizeof(u8) > u8_fn && pt_cmt->u16_dataBytes > u8_status_bytes; u8_fn++)
    {
        if (0x01 & (pt_cmt->u32_address >> u8_fn))
        {
            if (ERR_SPINOR_SUCCESS != drv_spinor_complete_read_status(read_reg_cmd[u8_fn], &u8_status, 1))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] Read status registers fail!\r\n");
            }
            u32_status |= (u8_status << (u8_status_bytes * 8));
            u8_status_bytes++;
        }
    }

    if (u8_enabled)
    {
        if ((u32_status & pt_cmt->u16_value) != pt_cmt->u16_value)
        {
            u32_status |= pt_cmt->u16_value;
            if (ERR_SPINOR_SUCCESS
                != drv_spinor_write_status(pt_cmt->u8_command, (u8 *)&u32_status, pt_cmt->u16_dataBytes))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] drv_spinor_write_status fail!\r\n");
            }
        }
    }
    else
    {
        if ((u32_status & pt_cmt->u16_value) != 0x0)
        {
            u32_status &= ~pt_cmt->u16_value;
            if (ERR_SPINOR_SUCCESS
                != drv_spinor_write_status(pt_cmt->u8_command, (u8 *)&u32_status, pt_cmt->u16_dataBytes))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] drv_spinor_write_status fail!\r\n");
            }
        }
    }
}

static void drv_spinor_unlock_whole_flash(SPINOR_INFO_t *spinor_info)
{
    SPINOR_INFO_t *          pst_spinor_info;
    SPINOR_PROTECT_STATUS_t *pst_w_protectStatus;
    FLASH_CMD_SET_t *        pt_w_cmt;
    FLASH_CMD_SET_t *        pt_c_cmt;

    pst_spinor_info     = spinor_info;
    pst_w_protectStatus = &pst_spinor_info->st_wProtectStatus;
    pt_w_cmt            = &pst_w_protectStatus->st_blockStatus.st_blocks;
    pt_c_cmt            = &pst_w_protectStatus->st_blockStatus.st_complement;

    if (0 < pt_w_cmt->u16_dataBytes)
    {
        drv_spinor_switch_config(pt_w_cmt, 0);
    }

    if (0 < pt_c_cmt->u16_dataBytes)
    {
        drv_spinor_switch_config(pt_c_cmt, 0);
    }
}

static void drv_spinor_show_protect_status(SPINOR_INFO_t *spinor_info)
{
    u8 u8_status_name_index;

    u32 u32_status_size;
    u32 u32_status;

    SPINOR_INFO_t *          pst_spinor_info;
    SPINOR_PROTECT_STATUS_t *pst_protectStatus;
    FLASH_CMD_SET_t *        pt_cmt;

    const char *pau8_status_name[] = {"[SPINOR] complement = 0x", "[SPINOR] top/buttom = 0x", "[SPINOR] blocks = 0x",
                                      "[SPINOR] SRP0 = 0x", "[SPINOR] SRP1 = 0x"};

    pst_spinor_info   = spinor_info;
    pst_protectStatus = &pst_spinor_info->st_rProtectStatus;

    u32_status_size      = sizeof(SPINOR_PROTECT_STATUS_t);
    pt_cmt               = (FLASH_CMD_SET_t *)&pst_protectStatus->st_blockStatus.st_complement;
    u8_status_name_index = 0;

    while (0 != u32_status_size)
    {
        if (0 < pt_cmt->u16_dataBytes)
        {
            u32_status = 0;

            if (ERR_SPINOR_SUCCESS
                != drv_spinor_complete_read_status(pt_cmt->u8_command, (u8 *)&u32_status, pt_cmt->u16_dataBytes))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] drv_spinor_complete_read_status fail!\r\n");
            }
            FLASH_IMPL_PRINTF_HEX(pau8_status_name[u8_status_name_index], u32_status & pt_cmt->u16_value, "\r\n");
        }

        pt_cmt++;
        u32_status_size -= sizeof(FLASH_CMD_SET_t);
        u8_status_name_index++;
    }
}

static void drv_spinor_setup_by_sni(SPINOR_INFO_t *pst_spinor_info)
{
    drv_spinor_setup_access(pst_spinor_info->st_readData.u8_command, pst_spinor_info->st_readData.u8_dummy,
                            pst_spinor_info->st_program.u8_command);
    drv_spinor_setup_time_wait(pst_spinor_info->u32_MaxWaitTime);

    if (0 == (pst_spinor_info->au8_reserved[0] & 0x01))
    {
        drv_spinor_use_bdma(1);
        FLASH_IMPL_PRINTF("[FLASH] BDMA mode\r\n");
    }
    else
    {
        drv_spinor_use_bdma(0);
        FLASH_IMPL_PRINTF("[FLASH] RIU mode\r\n");
    }
}

static u8 drv_spinor_is_sni_header(u8 *sni_buf)
{
    return FLASH_IMPL_MEMCMP(sni_buf, au8_basicMagicData, FLASH_SNI_HEADER_SIZE);
}

static u8 drv_spinor_is_sni_match(SPINOR_INFO_t *pst_spinor_info)
{
    u8 u8_id_matched = 0;
    u8 u8_i;
    u8 au8_device_id[16];

    if (ERR_SPINOR_SUCCESS != drv_spinor_read_id(((u8 *)&au8_device_id), pst_spinor_info->u8_IDByteCnt))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] read device id fail!\r\n");
        return 0;
    }

    if (0 < pst_spinor_info->u8_IDByteCnt)
    {
        u8_id_matched = 1;
        for (u8_i = 0; pst_spinor_info->u8_IDByteCnt > u8_i; u8_i++)
        {
            if (au8_device_id[u8_i] != pst_spinor_info->au8_ID[u8_i])
            {
                FLASH_IMPL_SHOW_ID(pst_spinor_info->au8_ID, pst_spinor_info->u8_IDByteCnt);
                u8_id_matched = 0;
                break;
            }
        }

        if (u8_id_matched)
        {
            FLASH_IMPL_SHOW_ID(au8_device_id, pst_spinor_info->u8_IDByteCnt);
        }
    }

    return u8_id_matched;
}

u8 drv_spinor_load_sni(u8 *sni_buf)
{
    u32                   u32_sni_address;
    SPINOR_SNI_t *        pst_spinor_sni;
    SPINOR_INFO_t *       pst_spinor_info;
    SPINOR_EXT_PROFILE_t *pst_spinor_ext_profile;

    u32_sni_address        = 0;
    pst_spinor_sni         = (SPINOR_SNI_t *)sni_buf;
    pst_spinor_info        = &pst_spinor_sni->spinor_info;
    pst_spinor_ext_profile = &pst_spinor_sni->spinor_ext_profile;

    if (drv_spinor_is_sni_header(sni_buf))
    {
        return 1;
    }

    if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(FLASH_CIS_LOAD_OFFSET, (u8 *)&u32_sni_address, 2))
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] SNI offset of satrting fail!\r\n");
        return 0;
    }

    u32_sni_address = (u32_sni_address + 0xfff) & ~0xfff;

    do
    {
        for (; FLASH_SEARCH_END > u32_sni_address; u32_sni_address += 0x1000)
        {
            if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(u32_sni_address, sni_buf, FLASH_SNI_HEADER_SIZE))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] Loading SNI header fail!\r\n");
                break;
            }

            if (drv_spinor_is_sni_header(sni_buf))
            {
                break;
            }
        }

        if (!drv_spinor_is_sni_header(sni_buf))
        {
            u32_sni_address = FLASH_DEFAULT_SNI_OFFSET;
        }

        FLASH_IMPL_PRINTF_HEX("[FLASH] find u32_sni_address = 0x", u32_sni_address, "\r\n");

        if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(u32_sni_address, sni_buf, FLASH_SNI_SIZE))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] Loading SNI fail!\r\n");
            break;
        }

        if (!drv_spinor_is_sni_header(sni_buf))
        {
            break;
        }

        if (pst_spinor_sni->u32_crc32
            != FLASH_IMPL_CRC32(0, (void *)sni_buf, (unsigned long)&pst_spinor_sni->u32_crc32 - (unsigned long)sni_buf))
        {
            FLASH_IMPL_PRINTF("[FLASH] SNI checksum fail\r\n");
            break;
        }

        if (!drv_spinor_is_sni_match(pst_spinor_info))
        {
            break;
        }

        if (!pst_spinor_info->u32_extSni)
        {
            FLASH_IMPL_PRINTF("[FLASH] SNI match flash.\r\n");
            return 1;
        }

        FLASH_IMPL_PRINTF("[FLASH] Load EXT SNI\r\n");
        if (ERR_SPINOR_SUCCESS
            != mdrv_spinor_read((u32_sni_address + FLASH_SNI_SIZE), (u8 *)pst_spinor_ext_profile,
                                (FLASH_EXT_SNI_RESERVED_SIZE * pst_spinor_info->u32_extSni)))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] Loading SNI fail!\r\n");
            break;
        }

        if (pst_spinor_ext_profile->u32_crc32
            == FLASH_IMPL_CRC32(0, (void *)((unsigned long)pst_spinor_ext_profile + 4),
                                (unsigned long)(FLASH_EXT_SNI_RESERVED_SIZE * pst_spinor_info->u32_extSni - 4)))
        {
            FLASH_IMPL_PRINTF("[FLASH] SNI match flash.\r\n");
            return 1;
        }
    } while (0);

    return 0;
}

#if defined(CONFIG_FLASH_WRITE_BACK)
static u8 drv_spinor_write_back_sni(u32 u32_sni_offset, u8 *sni_buf, u32 u32_size)
{
    if (ERR_SPINOR_SUCCESS != mdrv_spinor_erase(u32_sni_offset, SPINOR_SECTOR_SIZE))
    {
        FLASH_IMPL_PRINTF_HEX("[FLASH_ERR] erase fail @ 0x", u32_sni_offset, "\r\n");
        return 0;
    }

    if (ERR_SPINOR_SUCCESS != mdrv_spinor_program(u32_sni_offset, sni_buf, u32_size))
    {
        FLASH_IMPL_PRINTF_HEX("[FLASH_ERR] program fail @ 0x", u32_sni_offset, "\r\n");
        return 0;
    }

    FLASH_IMPL_PRINTF_HEX("[FLASH] write back sni from 0x", (unsigned long)sni_buf, "\r\n");
    return 1;
}

static u32 drv_spinor_load_ext_nri(u32 u32_offset, u8 *sni_buf, u32 u32_extSni)
{
    u8 *                  u32_dst;
    u32                   u32_load_size;
    u32                   u32_limit;
    SPINOR_SNI_t *        pst_sni;
    SPINOR_EXT_PROFILE_t *pst_ext_profile;
    SPINOR_EXT_SNI_t *    pst_ext_sni;

    if (!u32_extSni)
        return 0;

    pst_sni         = (SPINOR_SNI_t *)sni_buf;
    pst_ext_profile = &pst_sni->spinor_ext_profile;
    u32_offset += FLASH_SNI_SIZE;
    sni_buf += FLASH_SNI_SIZE;
    u32_dst       = sni_buf;
    u32_limit     = pst_sni->spinor_info.u16_SectorByteCnt - FLASH_SNI_SIZE;
    u32_load_size = 0;

    for (; 0 != u32_extSni && u32_load_size < u32_limit; u32_extSni--)
    {
        pst_ext_sni = (SPINOR_EXT_SNI_t *)sni_buf;

        if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(u32_offset, (u8 *)sni_buf, FLASH_SNI_SIZE))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] Loading EXT Sni fail!\r\n");
            return 0;
        }

        memcpy((void *)u32_dst, (const void *)pst_ext_sni->au8_reserved, sizeof(pst_ext_sni->au8_reserved));

        u32_dst += FLASH_EXT_SNI_RESERVED_SIZE;
        u32_load_size += FLASH_EXT_SNI_RESERVED_SIZE;
        sni_buf += FLASH_SNI_SIZE;
    }

    pst_ext_profile->u32_crc32 =
        FLASH_IMPL_CRC32(0, (void *)((unsigned long)pst_ext_profile + 4), (unsigned long)(u32_load_size - 4));

    return u32_load_size;
}

static u8 drv_spinor_find_sni(u8 *sni_buf)
{
    u32            u32_sni_address;
    u32            u32_avl_address;
    u32            u32_load_size;
    SPINOR_SNI_t * pst_spinor_sni;
    SPINOR_INFO_t *pst_spinor_info;

    u32_sni_address = 0;
    pst_spinor_sni  = (SPINOR_SNI_t *)sni_buf;
    pst_spinor_info = &pst_spinor_sni->spinor_info;

    do
    {
        if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(FLASH_CIS_LOAD_OFFSET, (u8 *)&u32_sni_address, 2))
        {
            FLASH_IMPL_PRINTF("[FLASH_ERR] SNI offset of satrting fail!\r\n");
            break;
        }

        u32_sni_address = (u32_sni_address + 0xfff) & ~0xfff;

        for (; FLASH_SEARCH_END > u32_sni_address; u32_sni_address += 0x1000)
        {
            if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(u32_sni_address, sni_buf, FLASH_SNI_HEADER_SIZE))
            {
                FLASH_IMPL_PRINTF("[FLASH_ERR] Loading SNI header fail!\r\n");
                break;
            }

            if (drv_spinor_is_sni_header(sni_buf))
            {
                break;
            }
        }

        if (!drv_spinor_is_sni_header(sni_buf))
        {
            u32_sni_address = FLASH_DEFAULT_SNI_OFFSET;
        }

        if (ERR_SPINOR_SUCCESS
            != mdrv_spinor_read(u32_sni_address + FLASH_SNI_SIZE, (u8 *)sni_buf, FLASH_SNI_HEADER_SIZE))
        {
            break;
        }

        u32_sni_address =
            drv_spinor_is_sni_header(sni_buf) ? (u32_sni_address - FLASH_AVL_OFFSET_POSITION) : u32_sni_address;
        u32_avl_address = u32_sni_address + FLASH_AVL_OFFSET_POSITION;

        while (ERR_SPINOR_SUCCESS == mdrv_spinor_read(u32_avl_address, sni_buf, FLASH_SNI_SIZE))
        {
            if (!drv_spinor_is_sni_header(sni_buf))
            {
                break;
            }

            if (drv_spinor_is_sni_match(pst_spinor_info))
            {
                FLASH_IMPL_PRINTF_HEX("[FLASH] Find SNI @ 0x", u32_avl_address, " in AVL.\r\n");

                pst_spinor_sni->u32_crc32 = FLASH_IMPL_CRC32(
                    0, (void *)sni_buf, (unsigned long)&pst_spinor_sni->u32_crc32 - (unsigned long)sni_buf);

                u32_load_size = drv_spinor_load_ext_nri(u32_avl_address, sni_buf, pst_spinor_info->u32_extSni);
                drv_spinor_unlock_whole_flash(pst_spinor_info);
                if (!drv_spinor_write_back_sni(u32_sni_address, sni_buf, u32_load_size + FLASH_SNI_SIZE))
                {
                    return 0;
                }
                return 1;
            }

            u32_avl_address += FLASH_SNI_SIZE;
        }

        if (ERR_SPINOR_SUCCESS != mdrv_spinor_read(u32_sni_address, sni_buf, FLASH_SNI_SIZE))
        {
            break;
        }

        if (!drv_spinor_is_sni_header(sni_buf))
        {
            break;
        }

        FLASH_IMPL_PRINTF("[FLASH] No matched sni\r\n");
    } while (0);

    return 0;
}
#else
#define drv_spinor_find_sni(x) 0
#endif

u8 mdrv_spinor_find_sni_form_dram(u8 *sni_buf, u8 *sni_list)
{
    u32               u32_extsni = 0;
    u8 *              pu8_dst;
    SPINOR_SNI_t *    pst_sni     = NULL;
    SPINOR_INFO_t *   pst_info    = NULL;
    SPINOR_EXT_SNI_t *pst_ext_sni = NULL;

    if (!sni_list)
        return ERR_SPINOR_INVALID;

    pu8_dst = sni_buf;

    while (drv_spinor_is_sni_header(sni_list))
    {
        pst_sni  = (SPINOR_SNI_t *)sni_list;
        pst_info = &pst_sni->spinor_info;

        if (drv_spinor_is_sni_match(pst_info))
        {
            memcpy((void *)pu8_dst, (void *)sni_list, FLASH_SNI_SIZE);
            pu8_dst += FLASH_SNI_SIZE;
            break;
        }

        sni_list += FLASH_SNI_SIZE;
    }

    if (!drv_spinor_is_sni_header((u8 *)sni_list))
        return ERR_SPINOR_DEVICE_FAILURE;

    if (pst_info)
        u32_extsni = pst_info->u32_extSni;

    while (u32_extsni)
    {
        sni_list += FLASH_SNI_SIZE;
        pst_ext_sni = (SPINOR_EXT_SNI_t *)sni_list;
        memcpy((void *)pu8_dst, (void *)pst_ext_sni->au8_reserved, sizeof(pst_ext_sni->au8_reserved));
        pu8_dst += sizeof(pst_ext_sni->au8_reserved);
        u32_extsni--;
    }

    return ERR_SPINOR_SUCCESS;
}

void mdrv_spinor_info(FLASH_NOR_INFO_t *pst_flash_nor_info)
{
    pst_flash_nor_info->u32_pageSize   = 0;
    pst_flash_nor_info->u32_sectorSize = 0;
    pst_flash_nor_info->u32_blockSize  = 0;
    pst_flash_nor_info->u32_capacity   = 0;

    if (g_pst_spinor_info)
    {
        pst_flash_nor_info->u32_pageSize   = g_pst_spinor_info->u16_PageByteCnt;
        pst_flash_nor_info->u32_sectorSize = g_pst_spinor_info->u16_SectorByteCnt;
        pst_flash_nor_info->u32_blockSize  = g_pst_spinor_info->u32_BlkBytesCnt;
        pst_flash_nor_info->u32_capacity   = g_pst_spinor_info->u32_Capacity;
    }
}

u8 mdrv_spinor_hardware_init(u8 *sni_buf)
{
    SPINOR_QUAD_CFG_t *pst_quad_enabled;

    g_u8_ext                 = 0;
    g_pst_spinor_sni         = (SPINOR_SNI_t *)sni_buf;
    g_pst_spinor_info        = &g_pst_spinor_sni->spinor_info;
    g_pst_spinor_ext_profile = &g_pst_spinor_sni->spinor_ext_profile;

    pst_quad_enabled = &g_pst_spinor_info->st_QE;

    if (ERR_SPINOR_SUCCESS != drv_spinor_init())
    {
        FLASH_IMPL_PRINTF("[FLASH_ERR] init fail!\r\n");
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    if (!drv_spinor_load_sni(sni_buf) && !drv_spinor_find_sni(sni_buf))
    {
        g_pst_spinor_sni         = NULL;
        g_pst_spinor_info        = NULL;
        g_pst_spinor_ext_profile = NULL;
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    if (pst_quad_enabled->u8_needQE)
    {
        drv_spinor_switch_config(&pst_quad_enabled->st_wQuadEnabled, 1);
    }
    drv_spinor_unlock_whole_flash(g_pst_spinor_info);
    PalSpiflash_SetClk(g_pst_spinor_info->u8_MaxClk);

    drv_spinor_setup_by_sni(g_pst_spinor_info);
    drv_spinor_show_protect_status(g_pst_spinor_info);
    FLASH_IMPL_PRINTF("[FLASH] End flash init.\r\n");
    return ERR_SPINOR_SUCCESS;
}

u8 mdrv_spinor_software_init(u8 *sni_buf)
{
    g_pst_spinor_sni         = (SPINOR_SNI_t *)sni_buf;
    g_pst_spinor_info        = &g_pst_spinor_sni->spinor_info;
    g_pst_spinor_ext_profile = &g_pst_spinor_sni->spinor_ext_profile;

    if (!drv_spinor_is_sni_header(sni_buf))
    {
        g_pst_spinor_sni         = NULL;
        g_pst_spinor_info        = NULL;
        g_pst_spinor_ext_profile = NULL;
        return ERR_SPINOR_DEVICE_FAILURE;
    }

    DRV_FSP_QSPI_BankSel();

    drv_spinor_setup_by_sni(g_pst_spinor_info);
    FLASH_IMPL_PRINTF("[FLASH] End flash init.\r\n");
    return ERR_SPINOR_SUCCESS;
}

u8 mdrv_spinor_deinit(void)
{
    g_pst_spinor_sni         = NULL;
    g_pst_spinor_info        = NULL;
    g_pst_spinor_ext_profile = NULL;

    return ERR_SPINOR_SUCCESS;
}

u8 mdrv_spinor_read(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u8 u8_status;

    if (!g_pst_spinor_sni)
        return ERR_SPINOR_DEVICE_FAILURE;

    u8_status = drv_spinor_read(u32_address, pu8_data, u32_size);

    return u8_status;
}

u8 mdrv_spinor_program(u32 u32_address, u8 *pu8_data, u32 u32_size)
{
    u16 u16_write_size;

    if (!g_pst_spinor_sni)
        return ERR_SPINOR_DEVICE_FAILURE;

    u16_write_size = 0;

    while (0 < u32_size)
    {
#if defined(CONFIG_FLASH_HW_CS)
#if defined(CONFIG_FLASH_BDMA)
        if (0 == (g_pst_spinor_info->au8_reserved[0] & 0x01))
        {
            u16_write_size = SPINOR_PAGE_SIZE - (SPINOR_PAGE_SIZE_MASK & u32_address);
        }
        else
        {
            u16_write_size = 0x8 - (0x7 & u32_address);
        }
#else
        u16_write_size = 0x8 - (0x7 & u32_address);
#endif
#else
        u16_write_size = SPINOR_PAGE_SIZE - (SPINOR_PAGE_SIZE_MASK & u32_address);
#endif

        if (u16_write_size > u32_size)
        {
            u16_write_size = u32_size;
        }

        if (ERR_SPINOR_SUCCESS != drv_spinor_program_page(u32_address, pu8_data, u16_write_size))
        {
            return ERR_SPINOR_DEVICE_FAILURE;
        }

        u32_address += u16_write_size;
        pu8_data += u16_write_size;
        u32_size -= u16_write_size;
    }

    return ERR_SPINOR_SUCCESS;
}

u8 mdrv_spinor_erase(u32 u32_address, u32 u32_size)
{
    u8  u8_status;
    u32 u32_bytes_left;
    u32 u32_erase_size;

    if (!g_pst_spinor_sni)
        return ERR_SPINOR_DEVICE_FAILURE;

    if (!u32_size)
        return ERR_SPINOR_SUCCESS;

    if (0 != (SPINOR_SECTOR_SIZE_MASK & u32_address) || 0 != (SPINOR_SECTOR_SIZE_MASK & u32_size))
    {
        return ERR_SPINOR_INVALID;
    }

    if (SPINOR_FLASH_SIZE < u32_address)
    {
        return ERR_SPINOR_INVALID;
    }

    u32_bytes_left = u32_size;

    while (0 != u32_bytes_left)
    {
        if ((0 != (~SPINOR_BLOCK_SIZE_MASK & u32_bytes_left)) && (0 == (~SPINOR_BLOCK_SIZE_MASK & u32_address)))
        {
            if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_block_erase(u32_address)))
            {
                return ERR_SPINOR_DEVICE_FAILURE;
            }

            u32_erase_size = SPINOR_BLOCK_SIZE;
        }
        else
        {
            if (ERR_SPINOR_SUCCESS != (u8_status = drv_spinor_sector_erase(u32_address)))
            {
                return ERR_SPINOR_DEVICE_FAILURE;
            }

            u32_erase_size = SPINOR_SECTOR_SIZE;
        }

        u32_address += u32_erase_size;
        u32_bytes_left -= u32_erase_size;
    }

    return ERR_SPINOR_SUCCESS;
}
