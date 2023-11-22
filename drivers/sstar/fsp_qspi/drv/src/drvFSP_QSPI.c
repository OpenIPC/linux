/*
 * drvFSP_QSPI.c- Sigmastar
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

#include <linux/string.h>
#include <halFSP_QSPI.h>

#define FSP_WD_BUF_SIZE 15

u8 g_u8_fsp_wbf_size;

u32 g_u32_fsp_wr_reg;

u16 g_u16_fsp_wbf_cmd_size;
u16 g_u16_fsp_rbf_reply_size;

static void _DRV_FSP_set_wbf_reg(u32 u32_reg_fsp_wd)
{
    g_u32_fsp_wr_reg = (u32_reg_fsp_wd << 1);
}

static void DRV_FSP_clear_wbf_size(void)
{
    g_u8_fsp_wbf_size = 0;
}

static void DRV_FSP_clear_wbf_rbf_size(void)
{
    g_u16_fsp_wbf_cmd_size   = 0;
    g_u16_fsp_rbf_reply_size = 0;
}

void DRV_FSP_set_access_mode(u8 u8_mode)
{
    HAL_FSP_WriteByte(GET_REG16_ADDR(REG_FSP_CTRL2), u8_mode & FSP_CTRL2_MASK);
}
EXPORT_SYMBOL(DRV_FSP_set_access_mode);

u32 DRV_FSP_enable_outside_wbf(u8 u8_src, u8 u8_which_wbf, u8 u8_which_byte_replaced, u32 u32_size)
{
    u32_size = (u32_size > FSP_OUTSIDE_WBF_SIZE(FSP_WBF_SIZE_OUTSIDE_SIZE))
                   ? FSP_OUTSIDE_WBF_SIZE(FSP_WBF_SIZE_OUTSIDE_SIZE)
                   : u32_size;
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_OUTSIDE_WBF_CTRL), FSP_WBF_OUTSIDE_EN | FSP_WBF_MODE(u8_which_wbf)
                                                                     | FSP_WBF_REPLACED(u8_which_byte_replaced)
                                                                     | FSP_WBF_OUTSIDE_SRC(u8_src));
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_OUTSIDE_WBF_SIZE), FSP_OUTSIDE_WBF_SIZE(u32_size + 1));
    return u32_size;
}
EXPORT_SYMBOL(DRV_FSP_enable_outside_wbf);

void DRV_FSP_disable_outside_wbf(void)
{
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_OUTSIDE_WBF_CTRL), 0);
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_OUTSIDE_WBF_SIZE), 0);
}
EXPORT_SYMBOL(DRV_FSP_disable_outside_wbf);

void DRV_FSP_init(u8 u8_cmd_cnt)
{
    u16 u16_fsp_ctrl0;

    u16_fsp_ctrl0 = FSP_ENABLE | FSP_RESET | FSP_INT;

    if (2 == u8_cmd_cnt)
    {
        u16_fsp_ctrl0 |= FSP_ENABLE_SECOND_CMD;
    }
    else if (3 == u8_cmd_cnt)
    {
        u16_fsp_ctrl0 |= FSP_ENABLE_THIRD_CMD;
    }

    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_CTRL), u16_fsp_ctrl0);
    HAL_FSP_WriteByte(GET_REG16_ADDR((REG_FSP_CTRL2 + 1)), 0);
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_CTRL4), 0);

    _DRV_FSP_set_wbf_reg(REG_FSP_WRITE_BUFF);
    DRV_FSP_clear_wbf_size();
    DRV_FSP_clear_wbf_rbf_size();
}
EXPORT_SYMBOL(DRV_FSP_init);

void DRV_FSP_trigger(void)
{
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_WRITE_SIZE), g_u16_fsp_wbf_cmd_size);
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_READ_SIZE), g_u16_fsp_rbf_reply_size);
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_TRIGGER), TRIGGER_FSP);
}
EXPORT_SYMBOL(DRV_FSP_trigger);

void DRV_FSP_clear_trigger(void)
{
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_CLEAR_DONE), FSP_CLEAR_DONE);
    _DRV_FSP_set_wbf_reg(REG_FSP_WRITE_BUFF);
    DRV_FSP_clear_wbf_size();
    DRV_FSP_clear_wbf_rbf_size();
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_WRITE_SIZE), g_u16_fsp_wbf_cmd_size);
    HAL_FSP_Write2Byte(GET_REG16_ADDR(REG_FSP_READ_SIZE), g_u16_fsp_rbf_reply_size);
}
EXPORT_SYMBOL(DRV_FSP_clear_trigger);

u8 DRV_FSP_is_done(void)
{
    // consider as it spend very long time to check if FSP done, so it may implment timeout method to improve
    u16 u16Try = 0;

    while (u16Try < FSP_CHK_NUM_WAITDONE)
    {
        if (HAL_FSP_Read2Byte(GET_REG16_ADDR(REG_FSP_DONE)) & FSP_DONE)
        {
            return 1;
        }

        u16Try++;
    }

    return 0;
}
EXPORT_SYMBOL(DRV_FSP_is_done);

void DRV_QSPI_pull_cs(u8 u8_pull_high)
{
    u8 u8_cs_ctrl;

    u8_cs_ctrl = HAL_QSPI_ReadByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE));

    if (1 == u8_pull_high)
    {
        HAL_QSPI_WriteByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE), REG_SPI_SW_CS_PULL_HIGH | u8_cs_ctrl);
    }
    else
    {
        HAL_QSPI_WriteByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE), ~REG_SPI_SW_CS_PULL_HIGH & u8_cs_ctrl);
    }
}
EXPORT_SYMBOL(DRV_QSPI_pull_cs);

void DRV_QSPI_use_sw_cs(u8 u8_enabled)
{
    u8 u8_cs_ctrl;

    u8_cs_ctrl = HAL_QSPI_ReadByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE));

    if (1 == u8_enabled)
    {
        HAL_QSPI_WriteByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE), REG_SPI_SW_CS_EN | u8_cs_ctrl);
    }
    else
    {
        HAL_QSPI_WriteByte(GET_REG16_ADDR(REG_SPI_BURST_WRITE), ~REG_SPI_SW_CS_EN & u8_cs_ctrl);
    }
}
EXPORT_SYMBOL(DRV_QSPI_use_sw_cs);

void DRV_QSPI_deselected_csz_time(u8 u8_clock)
{
    HAL_QSPI_WriteByte(GET_REG16_ADDR(REG_SPI_FSP_CZ_HIGH), ((u8_clock - 1) & 0x0f));
}
EXPORT_SYMBOL(DRV_QSPI_deselected_csz_time);

u8 DRV_QSPI_cmd_to_mode(u8 u8_cmd)
{
    u8 u8_mode_cmd;

    switch (u8_cmd)
    {
        case 0x03:
            u8_mode_cmd = SPI_NORMAL_MODE;
            break;
        case 0x0B:
            u8_mode_cmd = SPI_FAST_READ;
            break;
        case 0x3B:
            u8_mode_cmd = SPI_CMD_3B;
            break;
        case 0xBB:
            u8_mode_cmd = SPI_CMD_BB;
            break;
        case 0x6B:
            u8_mode_cmd = SPI_CMD_6B;
            break;
        case 0xEB:
            u8_mode_cmd = SPI_CMD_EB;
            break;
        default:
            u8_mode_cmd = SPI_NORMAL_MODE;
    }

    return u8_mode_cmd;
}
EXPORT_SYMBOL(DRV_QSPI_cmd_to_mode);

void DRV_QSPI_use_3bytes_address_mode(u8 u8_cmd, u8 u8_dummy_cyc)
{
    if (0 < u8_dummy_cyc)
    {
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_FUNC_SET), SPI_DUMMY_EN);
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_CKG_SPI), (SPI_USER_DUMMY_EN | (u8_dummy_cyc - 1)));
    }
    else
    {
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_FUNC_SET), 0);
    }

    HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_MODE_SEL), u8_cmd);
}
EXPORT_SYMBOL(DRV_QSPI_use_3bytes_address_mode);

void DRV_QSPI_use_2bytes_address_mode(u8 u8_cmd, u8 u8_dummy_cyc)
{
    if (0 < u8_dummy_cyc)
    {
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_FUNC_SET), SPI_ADDR2_EN | SPI_DUMMY_EN);
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_CKG_SPI), (SPI_USER_DUMMY_EN | (u8_dummy_cyc - 1)));
    }
    else
    {
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_FUNC_SET), SPI_ADDR2_EN);
    }

    HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_MODE_SEL), u8_cmd);
}
EXPORT_SYMBOL(DRV_QSPI_use_2bytes_address_mode);

u8 DRV_FSP_write_wbf(u8 *pu8_buf, u32 u32_size)
{
    u8 u8_bytes_written;

    for (u8_bytes_written = 0; FSP_WD_BUF_SIZE > g_u8_fsp_wbf_size && u32_size > u8_bytes_written; u8_bytes_written++)
    {
        if (FSP_WRITE_BUF_JUMP_OFFSET == g_u8_fsp_wbf_size)
        {
            _DRV_FSP_set_wbf_reg(REG_FSP_WRITE_BUFF2);
        }
        HAL_FSP_WriteByte(GET_REG8_ADDR(g_u32_fsp_wr_reg), *pu8_buf);
        g_u32_fsp_wr_reg++;
        g_u8_fsp_wbf_size++;
        pu8_buf++;
    }

    return u8_bytes_written;
}

EXPORT_SYMBOL(DRV_FSP_write_wbf);

u8 DRV_FSP_set_which_cmd_size(u8 u8_which, u32 u32_count)
{
    u8 u8_count;

    u8_count = u32_count & 0xFF;
    if (FSP_SINGLE_WRITE_SIZE < u32_count)
    {
        u8_count = FSP_SINGLE_WRITE_SIZE;
    }

    g_u16_fsp_wbf_cmd_size |= u8_count << (4 * (u8_which - 1));

    return u8_count;
}
EXPORT_SYMBOL(DRV_FSP_set_which_cmd_size);

u8 DRV_FSP_set_rbf_size_after_which_cmd(u8 u8_which, u32 u32_receive)
{
    u8 u8_rbf_size;

    u8_rbf_size = u32_receive;

    if (FSP_MAX_READ_BUF_CNT < u32_receive)
    {
        u8_rbf_size = FSP_MAX_READ_BUF_CNT;
    }

    g_u16_fsp_rbf_reply_size |= ((u8_rbf_size & 0xff) << (4 * (u8_which - 1)));

    return u8_rbf_size;
}
EXPORT_SYMBOL(DRV_FSP_set_rbf_size_after_which_cmd);

u8 DRV_FSP_read_rbf(u8 *pu8_buf, u32 u32_size)
{
    u8 u8_bytes_read;

    for (u8_bytes_read = 0; u32_size > u8_bytes_read && FSP_MAX_READ_BUF_CNT > u8_bytes_read; u8_bytes_read++)
    {
        *pu8_buf = HAL_FSP_ReadByte(GET_REG8_ADDR((REG_FSP_READ_BUFF << 1) + u8_bytes_read));
        pu8_buf++;
    }

    return u8_bytes_read;
}
EXPORT_SYMBOL(DRV_FSP_read_rbf);

void DRV_QSPI_set_timeout(u8 u8_enable, u32 u32_val)
{
    if (u8_enable)
    {
        HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_TIMEOUT_VAL_L), (u32_val & 0xFFFF));
        HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_TIMEOUT_VAL_H), ((u32_val >> 16) & 0xFF),
                                 REG_SPI_TIMEOUT_VAL_MASK);
        HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_TIMEOUT_CTRL), REG_SPI_TIMEOUT_NRST, REG_SPI_TIMEOUT_RST_MASK);
        HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_TIMEOUT_CTRL), REG_SPI_TIMEOUT_EN, REG_SPI_TIMEOUT_EN_MASK);
    }
    else
    {
        HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_TIMEOUT_CTRL), REG_SPI_TIMEOUT_RST, REG_SPI_TIMEOUT_RST_MASK);
        HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_TIMEOUT_CTRL), REG_SPI_TIMEOUT_DS, REG_SPI_TIMEOUT_EN_MASK);
    }
}
EXPORT_SYMBOL(DRV_QSPI_set_timeout);

void DRV_FSP_QSPI_BankSel(void)
{
    HAL_FSP_QSPI_BankSel();
}
EXPORT_SYMBOL(DRV_FSP_QSPI_BankSel);

void DRV_ISP_disable_address_continue(void)
{
    HAL_ISP_Write2Byte_Mask(GET_REG16_ADDR(0x3F), (0x0 << 2), (0x1 << 2));
    HAL_ISP_Write2Byte_Mask(GET_REG16_ADDR(0x3F), (0x1 << 2), (0x1 << 2));
}
EXPORT_SYMBOL(DRV_ISP_disable_address_continue);

void DRV_FSP_QSPI_init(void)
{
    REG_WRITE_U16(DEBUG_GET_INFO, GET_INFO_CHIP_INIT);
    HAL_FSP_QSPI_BankSel();
    HAL_REG_Write2Byte(GET_BANK_REG16_ADDR(CHIP_BASE_ADDR, 0x50), 0x0000); // disable all pad in
    HAL_QSPI_Write2Byte(GET_REG16_ADDR(REG_SPI_CHIP_SELECT), 0x00);        // chip select SPI#1

    // Fix flash address must 16 align
    HAL_PIU_NONPM_Write2Byte_Mask(GET_REG16_ADDR(0x1a), 0x0, 0x1);
    HAL_PIU_NONPM_Write2Byte_Mask(GET_REG16_ADDR(0x34), 0x0, 0x1);

    HAL_QSPI_Write2Byte_Mask(GET_REG16_ADDR(REG_SPI_DIS_BITCNT), REG_SPI_BITCNT_DS, REG_SPI_BITCNT_DS_MASK);

    DRV_QSPI_set_timeout(0, 0);
    DRV_QSPI_use_sw_cs(1);
    DRV_QSPI_pull_cs(1);
}
EXPORT_SYMBOL(DRV_FSP_QSPI_init);
