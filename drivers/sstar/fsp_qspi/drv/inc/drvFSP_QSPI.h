/*
 * drvFSP_QSPI.h- Sigmastar
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

#ifndef _DRV_FSP_QSPI_H_
#define _DRV_FSP_QSPI_H_

#define FSP_USE_SINGLE_CMD 1
#define FSP_USE_TWO_CMDS   2
#define FSP_USE_THREE_CMDS 3

#define FSP_SINGLE_MODE 0
#define FSP_QUAD_MODE   1
#define FSP_DUAL_MODE   2

#define FSP_OUTSIDE_MODE_BDMA     0x00
#define FSP_OUTSIDE_MODE_SPI2FCIE 0x01

void DRV_FSP_set_access_mode(u8 u8_mode);
void DRV_FSP_init(u8 u8_cmd_cnt);
u8   DRV_FSP_is_done(void);
void DRV_FSP_use_outside_buf(u8 u8_which_cmd, u8 u8_replace_which_bytes, u32 u32_size);
void DRV_QSPI_use_sw_cs(u8 u8_enabled);
u8   DRV_QSPI_cmd_to_mode(u8 u8_cmd);
void DRV_QSPI_use_3bytes_address_mode(u8 u8_Cmd, u8 u8_dummy_cyc);
void DRV_QSPI_use_2bytes_address_mode(u8 u8_Cmd, u8 u8_dummy_cyc);
u8   DRV_FSP_set_which_cmd_size(u8 u8_which, u32 u32_count);
u8   DRV_FSP_set_rbf_size_after_which_cmd(u8 u8_which, u32 u32_receive);
u8   DRV_FSP_write_wbf(u8 *pu8_buf, u32 u32_size);
void DRV_FSP_trigger(void);
void DRV_FSP_clear_trigger(void);
u8   DRV_FSP_read_rbf(u8 *pu8_buf, u32 u32_size);
void DRV_QSPI_deselected_csz_time(u8 u8_clock);
void DRV_QSPI_pull_cs(u8 u8_pull_high);
u32  DRV_FSP_enable_outside_wbf(u8 u8_src, u8 u8_which_wbf, u8 u8_which_byte_replaced, u32 u32_size);
void DRV_FSP_disable_outside_wbf(void);
void DRV_FSP_QSPI_init(void);
void DRV_QSPI_set_timeout(u8 u8_enable, u32 u32_val);
void DRV_FSP_QSPI_BankSel(void);
void DRV_ISP_disable_address_continue(void);
#endif
