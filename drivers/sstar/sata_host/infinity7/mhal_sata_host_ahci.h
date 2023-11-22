/*
 * mhal_sata_host_ahci.h- Sigmastar
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

#ifndef _MHAL_SATA_HOST_AHCI_H
#define _MHAL_SATA_HOST_AHCI_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "mdrv_sata_host_ahci.h"

#define SATA_CTS_PATTERN_HFTP 0
#define SATA_CTS_PATTERN_MFTP 1
#define SATA_CTS_PATTERN_LFTP 2
#define SATA_CTS_PATTERN_LBP  3
#define SATA_CTS_PATTERN_SSOP 4

#define SATA_GEN1 0
#define SATA_GEN2 1
#define SATA_GEN3 2

#define DE_EMPHASIS_1p5_DB 0
#define DE_EMPHASIS_2p5_DB 1
#define DE_EMPHASIS_3p5_DB 2
#define DE_EMPHASIS_MAX_DB (DE_EMPHASIS_3p5_DB + 1)

#define DE_EMPHASIS_LEVEL_0p4 0
#define DE_EMPHASIS_LEVEL_0p5 1
#define DE_EMPHASIS_LEVEL_0p6 2
#define DE_EMPHASIS_LEVEL_0p7 3
#define DE_EMPHASIS_LEVEL_0p8 4
#define DE_EMPHASIS_LEVEL_0p9 5
#define DE_EMPHASIS_LEVEL_1p0 6
#define DE_EMPHASIS_LEVEL_1p1 7
#define DE_EMPHASIS_LEVEL_MAX (DE_EMPHASIS_LEVEL_1p1 + 1)

struct ahci_platform_data
{
    int (*init)(struct device *dev, void __iomem *addr);
    void (*exit)(struct device *dev);
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);
    const struct ata_port_info *ata_port_info;
    unsigned int                force_port_map;
    unsigned int                mask_port_map;
};

struct _sstar_de_emphasis
{
    int enable;
    int db;
    //    int gen;
    int level;
};

struct _sstar_phy_adjust
{
    int test_gen;
    int cts_pattern;
    int new_de_emphasis;
    int new_cts_pattern;

    struct _sstar_de_emphasis dmhs;
};

int  ss_sata_init(struct sstar_ahci_priv *priv, int phy_mode, int port_num);
void ss_sata_misc_init(void *mmio, unsigned long port_base, unsigned long misc_base, int n_ports);
void ss_sata_phy_init(void *mmio, int phy_mode, int n_ports);
void ss_sata_applySetting(int portNum, struct ahci_host_priv *hpriv);
void hal_sata_set_cts_pattern(int portNum, struct _sstar_phy_adjust *phy_adj);
void hal_sata_set_de_emphasis(int portNum, struct _sstar_phy_adjust *phy_adj);
void hal_sata_set_testGen(int portNum, int gen);
// void ss_sata_applyDmhs(int portNum, u32 *u32_gen1_gen2_vpp, u32 *u32_gen3_de_emphasis,
//                        struct _sstar_phy_adjust *phy_adj);
void ss_sata_applyDmhs(int portNum, struct sstar_ahci_priv *priv, struct _sstar_phy_adjust *phy_adj);

#endif
