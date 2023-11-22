/*
 * drv_pcieif.h- Sigmastar
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

#ifndef _DRV_PCIEIF_H_
#define _DRV_PCIEIF_H_

enum ss_pcieif_mode
{
    PCIE_IF_MODE_RC = 0,
    PCIE_IF_MODE_EP,
};

u8 sstar_pcieif_num_of_intf(void);

u16  sstar_pcieif_readw_phy(u8 id, u32 reg);
void sstar_pcieif_writew_phy(u8 id, u32 reg, u16 val);
u16  sstar_pcieif_readw_mac(u8 id, u32 reg);
void sstar_pcieif_writew_mac(u8 id, u32 reg, u16 val);
u32  sstar_pcieif_readl_dbi(u8 id, u32 reg);
u16  sstar_pcieif_readw_dbi(u8 id, u32 reg);
u8   sstar_pcieif_readb_dbi(u8 id, u32 reg);
void sstar_pcieif_writel_dbi(u8 id, u32 reg, u32 val);
void sstar_pcieif_writew_dbi(u8 id, u32 reg, u16 val);
void sstar_pcieif_writeb_dbi(u8 id, u32 reg, u8 val);
void sstar_pcieif_start_link(u8 id);
void sstar_pcieif_set_mode(u8 id, enum ss_pcieif_mode mode);
void sstar_pcieif_clr_memwr_hit_intr_sta(u8 id);
bool sstar_pcieif_memwr_hit_intr_sta(u8 id);
void sstar_pcieif_enable_memwr_hit_intr(u8 id, phys_addr_t addr);
void sstar_pcieif_internalclk_en(u8 id, u8 ssc_enable);
#ifdef CONFIG_SSTAR_FPGA_LPBK
void sstar_pcieif_loopback_link(u8 rc_id, u8 ep_id);
#endif

#endif /* _DRV_PCIEIF_H_ */
