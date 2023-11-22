/*
 * hal_pcieif.h- Sigmastar
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

#ifndef _HAL_PCIEIF_H_
#define _HAL_PCIEIF_H_

/*
 * registers
 */
/* riu mac reg */
#define REG_PCIE_DEVICE_TYPE  (0x02 << 2)
#define PCIE_DEVICE_EP        (0)      // PCI Express Endpoint
#define PCIE_DEVICE_LEGACY_EP (1 << 0) // Legacy PCI Express Endpoint
#define PCIE_DEVICE_RC        (1 << 2) // Root port of PCIe Express Root Complex
#define REG_PCIE_IRQ_MASK     (0x06 << 2)
#define REG_PCIE_IRQ2_MASK    (0x14 << 2)
#define REG_PCIE_IRQ2_CLR     (0x16 << 2)
#define REG_PCIE_IRQ2_TYPE    (0x18 << 2)
#define REG_PCIE_IRQ2_STAT    (0x19 << 2)
#define IRQ2_INTR_MSI         (0x100)
#define IRQ2_INTR_ADDR_MATCH  (0x200)
#define REG_SLV_ARMISC_INFO   (0x28 << 2) // AXI: slave status count slv_armisc_info[4:0] is the TLP type
#define TLP_TYPE_MEM          (0)         // 5'b00000 => Memory
#define TLP_TYPE_IO           (2)         // 5'b00010 => I/O
#define TLP_TYPE0_CFG         (4)         // 5'b00100 => Type0 Configuration
#define TLP_TYPE1_CFG         (5)         // 5'b00101 => Type1 Configuration
#define REG_SLV_WSTRB_MASK    (0x2B << 2) // reg_slv_wstrb_mask
#define REG_DBI_ADDR_20to12   (0x3D << 2) // reg_dbi_addr_20to12
#define REG_DBI_ADDR_32to21   (0x3E << 2) // reg_dbi_addr_32to21
#define REG_PCIE_MAC_CTRL     (0x60 << 2) // PCIe MAC Control Registers
#define PCIE_LTSSM_EN         (1 << 4)    // reg_app_ltssm_enable
#define REG_WR_MATCH_ADDR0    (0x7D << 2) // reg_pcie_mstwr_addr15to0_match
#define REG_WR_MATCH_ADDR1    (0x7E << 2) // reg_pcie_mstwr_addr31to16_match
#define REG_WR_MATCH_ADDR2    (0x7F << 2) // reg_pcie_mstwr_addr35to32_match
#define REG_PHYVREG_BPS       (0x05 << 2) // reg_phy_vreg_bypass,1=1.8V;0=3.3V
/* riu phy reg */
#define REG_SLV_READ_BYPASS (0x27 << 2) // reg_slv_read_bypass
#define REG_PHY_REF_USEPAD  (0x02 << 2) // reg_phy_ref_use_pad 0:internal_clock 1:external_ClockGen[default]
u32  hal_prog_dbi_addr(void __iomem *mac_base, u32 reg);
void hal_rst_dbi_addr(void __iomem *mac_base);
void hal_internalclk_en(u8 id, u8 ssc_enable);

#endif /* _HAL_PCIEIF_H */
