/*
 * mdrv_sata_host_ahci.h- Sigmastar
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

#ifndef _MDRV_SATA_HOST_AHCI_H_
#define _MDRV_SATA_HOST_AHCI_H_

struct sstar_platform_data
{
    int (*init)(struct device *dev, void __iomem *addr, int id);
    void (*exit)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    const struct ata_port_info *ata_port_info;
    unsigned int                force_port_map;
    unsigned int                mask_port_map;
};

struct sstar_ahci_priv
{
    struct device *dev;
    struct device *sysfs_dev;     // for node /sys/class/sstar/
    dev_t          dev_no;        // for dynamic major number allocation
    dev_t          attribute_dev; // for dynamic major number allocation

    u32 port_mask;

    void __iomem *res_ahci;
    void __iomem *res_ahci_port0;
    void __iomem *res_ahci_misc;
    void __iomem *mmio;

    int          hd_link;
    unsigned int bFirstOOB;
    int          phy_mode;

    bool ssc_mode;

    u32 tx_gen1_gen2_vpp[2];
    u32 tx_gen3_de_emphasis[3];
    // u32 rx_eq_cs[3];
    u32 rx_eq_ctle;
    u32 rx_eq_rs1[3];
    u32 rx_eq_rs2[3];
};

int  ahci_port_apply_hd_link(struct ata_link *link);
int  ahci_port_phy_fsm_reset(struct ata_link *link);
void ss_sata_ssc_enable(struct ata_link *link, bool enable, unsigned int spd);
void ss_sata_applyRxEq(int portNum, struct sstar_ahci_priv *priv);

#endif
