/*
 * drvFCIE.h- Sigmastar
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

#ifndef _DRV_FCIE_H_
#define _DRV_FCIE_H_

#define FCIE_WAIT_WRITE_TIME (5000000) // us
#define FCIE_WAIT_READ_TIME  (5000000) // us

u8   DRV_SPI2FCIE_boundary_is_hit(u32 u32_timeout);
void DRV_SPI2FCIE_boundary_refresh_spi_offset(u32 u32_spi_offset);
void DRV_SPI2FCIE_boundary_offset_reload(u32 u32_spi_offset, u32 u32_boundary);
void DRV_SPI2FCIE_boundary_enable_pre_set(u32 u32_spi_offset, u32 u32_boundary);
void DRV_SPI2FCIE_set_upstream_size(u16 u16_size);
void DRV_SPI2FCIE_resume_downstream(void);
void DRV_SPI2FCIE_enable_downstream(void);
void DRV_SPI2FCIE_disable_stream(void);
void DRV_SPI2FCIE_enable_upstream_size(u16 u16_size);
void DRV_SPI2FCIE_reset(void);
u8   DRV_FCIE_get_bits_corrected(void);
u8   DRV_FCIE_get_ecc_status(void);
void DRV_FCIE_clear_job(void);
u8   DRV_FCIE_job_is_done(u32 u32_timeout);
void DRV_FCIE_disable_imi(void);
void DRV_FCIE_enable_imi(void);
void DRV_FCIE_job_write_page_start(u16 u16_sector_conut);
void DRV_FCIE_job_read_page_start(u16 u16_sector_conut);
void DRV_FCIE_set_sectors(u16 u16_spare_cnt);
void DRV_FCIE_job_write_sector_start(void);
void DRV_FCIE_job_read_sectors(void);
void DRV_FCIE_set_write_address(u64 u64_data, u64 u64_spare);
void DRV_FCIE_set_read_address(u64 u64_data, u64 u64_spare);
void DRV_FCIE_set_brdg_address(u64 u64_data, u64 u64_spare);
void DRV_FCIE_enable_ecc(u8 u8_enabled);
u8   DRV_FCIE_get_ecc_mode(u16 u16_sector_size, u8 u8_ecc_bits);
void DRV_FCIE_select_spare_dest(void);
void DRV_FCIE_set_ecc_dir(u16 u16_ecc_dir);
void DRV_FCIE_ecc_bypass(void);
void DRV_FCIE_stetup_ecc_ctrl(u16 u16_page_size, u16 u16_spare_size, u16 u16_ecc_bytes, u8 u8_ecc_mode);
void DRV_FCIE_setup_brdg_ctrl(u16 u16_sector_size, u16 u16_spare_per_sector);
u8   DRV_FCIE_reset(void);
void DRV_FCIE_init(void);
void DRV_SPI2FCIE_block_downstream(void);

#define FCIE_SPI_MODE  (1 << 4)
#define FCIR_DIR_WRITE (1 << 9)
#define FCIR_DIR_READ  (0 << 9)

#endif /* _DRV_FCIE_H_ */
