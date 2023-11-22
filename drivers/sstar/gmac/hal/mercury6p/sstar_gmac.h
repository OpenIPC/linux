/*
 * sstar_gmac.h- Sigmastar
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

#ifndef _SSTAR_GMAC_H_
#define _SSTAR_GMAC_H_

#include "ms_platform.h"
#include "mstar_chip.h"
#include "cam_os_wrapper.h"
#include "registers.h"

#define REG_BANK_GMAC_TOE 0x1514

#define BASE_REG_INTR_CTRL1_1_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x101700)
#define BASE_REG_GMACPLL_PA      GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103A00)
#define BASE_REG_PAD_GPIO_PA     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103E00)
#define BASE_REG_CLKGEN_2_PA     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x103F00)
#define BASE_REG_PAD_GPIO2_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x110400)
#define BASE_REG_GMAC0_PA        GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151000)
#define BASE_REG_GMAC1_PA        GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151100)
#define BASE_REG_NET_GP_CTRL_PA  GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151200)
#define BASE_REG_ALBANY          GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x151800)
#define BASE_REG_X32_GMAC0_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A5000)
#define BASE_REG_X32_GMAC1_PA    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A5A00)

#define GMAC0         0
#define GMAC1         1
#define GMAC_NUM      2
#define GMAC_BANK_NUM 8

#define GMAC0_Base             0xFD34A000
#define GMAC1_Base             0xFD34B400
#define GMAC1_BANK_ADDR_OFFSET 0x0A

#ifdef CONFIG_ARM64
#define rebase_based 0xFFFFFF801F000000
#else
#define rebase_based 0x1F000000
#endif

#define pd_cnnt 0x1
#define pd_bind 0x2
#define pd_acpt 0x4

#define pd_ipv4 0
#define pd_ipv6 1

#define pd_tcp 0
#define pd_udp 1

#define SSTAR_DBG              0
#define SSTAR_DUMP_DESC        0
#define SSTAR_REBASE_WRITE_DBG 0
#define SSTAR_REBASE_READ_DBG  0

#if SSTAR_REBASE_WRITE_DBG
#define rbs_wrte_dbg(fmt, arg...) printk(fmt, ##arg)
#else
#define rbs_wrte_dbg(fmt, arg...)
#endif

#if SSTAR_REBASE_READ_DBG
#define rbs_read_dbg(fmt, arg...) printk(fmt, ##arg)
#else
#define rbs_read_dbg(fmt, arg...)
#endif

#if SSTAR_DBG
#undef pr_debug
#undef pr_warn
#undef pr_err
#undef pr_info
#undef dev_err
#undef dev_warn

#define pr_debug(fmt, arg...)    printk(fmt, ##arg)
#define pr_warn(fmt, arg...)     printk(fmt, ##arg)
#define pr_err(fmt, arg...)      printk(fmt, ##arg)
#define pr_info(fmt, arg...)     printk(fmt, ##arg)
#define dev_err(x, fmt, arg...)  printk(fmt, ##arg)
#define dev_warn(x, fmt, arg...) printk(fmt, ##arg)

#endif

#define MIU0_HIGH_BUS_BASE         0x1000000000L
#define MIU0_BUS_BASE              ARM_MIU0_BUS_BASE
#define GMAC_USING_MIU_MAPPING_API 1

#if GMAC_USING_MIU_MAPPING_API
#define PA2BUS(a) Chip_Phys_to_MIU(a)
#define BUS2PA(a) Chip_MIU_to_Phys(a)
#else
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
#define MIU_4G 0x100000000L

#define PA2BUS(a) ((a >= MIU_4G) ? ((a) - (MIU0_HIGH_BUS_BASE)) : ((a) - (MIU0_BUS_BASE)))
#define BUS2PA(a) ((a >= MIU_4G) ? ((a) + (MIU0_HIGH_BUS_BASE)) : ((a) + (MIU0_BUS_BASE)))
#else
#define PA2BUS(a) ((a) - (MIU0_BUS_BASE))
#define BUS2PA(a) ((a) + (MIU0_BUS_BASE))
#endif
#endif

#define DYN_PHASE_CALB         1
#define MERCURY6P_CHIP_ID      0xF8
#define GMAC_CHIP_REVISION_U01 0x01
#define NEED_CALB              ((CamOsChipId() == MERCURY6P_CHIP_ID && CamOsChipRevision() == GMAC_CHIP_REVISION_U01) ? 1 : 0)

#define TXC_PHASE_0   0x0000
#define TXC_PHASE_90  0x1800
#define TXC_PHASE_180 0x800
#define TXC_PHASE_270 0x1000

#define GMAC_CALB_MSK       (BIT(2) | BIT(3))
#define GMAC_RESV_SPEED_MSK (BIT(14) | BIT(15))
#define GMAC_SEL_MSK        (GMAC_CALB_MSK | GMAC_RESV_SPEED_MSK)
#define GMAC_TX_PHASE_MSK   (BIT(11) | BIT(12))

struct sstar_dwmac
{
    struct clk *            gmac_clkgen;
    struct clk *            gmac_mclk;
    int                     ext_phyclk;
    int                     enable_eth_ck;
    int                     eth_clk_sel_reg;
    int                     eth_ref_clk_sel_reg;
    int                     irq_pwr_wakeup;
    u32                     mode_reg; /* MAC glue-logic mode register */
    struct regmap *         regmap;
    u32                     speed;
    struct proc_dir_entry * gmac_root_dir;
    struct device_node *    np;
    const struct stm32_ops *ops;
    struct device *         dev;
    int                     interface;
    int                     id;
    void __iomem *          base_addr;
    int                     bank[GMAC_BANK_NUM];
    u8                      disable_txfc;
    u8                      mclk_freq;
    u8                      mclk_refmode;
    u8                      reset_io;
#ifdef CONFIG_SSTAR_SNPS_GMAC_STORM_PROTECT
    u8           bcast_prct_enable;
    u8           mcast_prct_enable;
    u8           bcast_prct_on;
    u8           mcast_prct_on;
    unsigned int bcast_thresh;
    unsigned int mcast_thresh;
    unsigned int storm_ms_timer;
#endif
};

void mhal_gmac_probe(struct sstar_dwmac *dwmac);
void mhal_gmac_tx_clk_pad_sel(struct sstar_dwmac *dwmac, unsigned int speed);

#ifdef CONFIG_SSTAR_SNPS_GMAC_CODING_OPTIMIZE
#define SSTAR_GMAC_MAC_ADDR_NUM 32
extern void *GBase[GMAC_NUM];
extern u32   mhal_axi_2_sstar_tbl[4][32];
void         mhal_gmac_axi_tbl(int gmacId, int *bank, int nbank);

#define sstar_gmac_iomem2id(addr) ((u8)((unsigned long)(addr)&0x3))
static inline u32 prior_to_rebase(void __iomem *addr)
{
    int id       = sstar_gmac_iomem2id(addr);
    int axi_bank = (addr - GBase[id]) >> 8;

    return ((mhal_axi_2_sstar_tbl[id][axi_bank] | (((unsigned long)addr) & 0xfc)));
}

int sstar_gmac_int_owner(int gmacId);
// #define sstar_gmac_int_owner(gmacId) (INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x12)) & (1 << (gmacId)))

#else
u32 prior_to_rebase(void __iomem *addr);
u8  sstar_gmac_iomem2id(void __iomem *ioaddr);
#endif

void mDev_pd_add_cnx_table(u32 saddr, u16 sport, u32 daddr, u16 dport, u16 pd_criteria, u16 pd_ipver);
void mhal_gmac_disable_padmux(void);
void sstar_gmac_disable_and_clean_int(void *par);
#if (DYN_PHASE_CALB)
void mhal_gmac_dyncalibrat(struct stmmac_priv *priv);
#endif

#if 1
#undef writel
#undef readl

#define writel(val, reg) OUTREG32(prior_to_rebase(reg), val)
#define readl(reg)       INREG32(prior_to_rebase(reg))
#endif

#endif /* _SSTAR_GMAC_H_ */
