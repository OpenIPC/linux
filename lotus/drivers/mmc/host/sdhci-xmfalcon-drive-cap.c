// SPDX-License-Identifier: GPL-2.0

#include "sdhci-lotus.h"

static struct sdhci_pad_cell emmc_drive_cap[] = {
// inch:2 layer:2 speed:5
//inch_index*(layer * speed) + layer_index*(speed) + speed_index
//1inch_4L_ls-sdr
{0b0111, 0b0011},
//1inch_4L_hs-sdr
{0b0111, 0b0011},
//1inch_4L_hs-ddr
{0b0111, 0b0011},
//1inch_4L_hs200
{0b1000, 0b0100},
//1inch_4L_hs400
{0b0100, 0b0100},

//1inch_2L_ls-sdr
{0b0111, 0b0011},
//1inch_2L_hs-sdr
{0b0111, 0b0011},
//1inch_2L_hs-ddr
{0b0111, 0b0011},
//1inch_2L_hs200
{0b0110, 0b0010},
//1inch_2L_hs400
{0b0010, 0b0010},
/*****************2inch*************/
//2inch_4L_ls-sdr
{0b0111, 0b0011},
//2inch_4L_hs-sdr
{0b0111, 0b0011},
//2inch_4L_hs-ddr
{0b0111, 0b0011},
//2inch_4L_hs200
{0b1100, 0b1010},
//2inch_4L_hs400
{0b1100, 0b1100},

//2inch_2L_ls-sdr
{0b0111, 0b0011},
//2inch_2L_hs-sdr
{0b0111, 0b0011},
//2inch_2L_hs-ddr
{0b0111, 0b0011},
//2inch_2L_hs200
{0b1010, 0b0110},
//2inch_2L_hs400
{0b1000, 0b1000},
};

static struct sdhci_pad_cell sdio0_drive_cap[] = {
// inch:2 layer:2 speed:5
//inch_index*(layer * speed) + layer_index*(speed) + speed_index
//2inch_4L_hs-sdr
{0b0101, 0b0011},
//2inch_4L_sdr12-sdr25
{0b0101, 0b0011},
//2inch_4L_sdr50
{0b0101, 0b0011},
//2inch_4L_ddr50
{0b0101, 0b0100},
//2inch_4L_sdr104
{0b1100, 0b0100},

//2inch_2L_hs-sdr
{0b0101, 0b0011},
//2inch_2L_sdr12-sdr25
{0b0101, 0b0011},
//2inch_2L_sdr50
{0b0101, 0b0011},
//2inch_2L_ddr50
{0b0101, 0b0010},
//2inch_2L_sdr104
{0b1100, 0b0100},
/*****************4inch*************/
//4inch_4L_hs-sdr
{0b0111, 0b0001},
//4inch_4L_sdr12-sdr25
{0b0111, 0b0001},
//4inch_4L_sdr50
{0b0110, 0b0011},
//4inch_4L_ddr50
{0b0110, 0b0011},
//4inch_4L_sdr104
{0b1110, 0b0110},

//4inch_2L_hs-sdr
{0b0111, 0b0001},
//4inch_2L_sdr12-sdr25
{0b0111, 0b0001},
//4inch_2L_sdr50
{0b0110, 0b0011},
//4inch_2L_ddr50
{0b0110, 0b0011},
//4inch_2L_sdr104
{0b1110, 0b0110},
};

static struct sdhci_pad_cell sdio1_drive_cap[] = {
// inch:2 layer:2 speed:5
//inch_index*(layer * speed) + layer_index*(speed) + speed_index
//2inch_4L_hs-sdr
{0b0101, 0b0011},
//2inch_4L_sdr12-sdr25
{0b0101, 0b0011},
//2inch_4L_sdr50
{0b0101, 0b0011},
//2inch_4L_ddr50
{0b0101, 0b0100},
//2inch_4L_sdr104
{0b1100, 0b0100},

//2inch_2L_hs-sdr
{0b0101, 0b0011},
//2inch_2L_sdr12-sdr25
{0b0101, 0b0011},
//2inch_2L_sdr50
{0b0101, 0b0011},
//2inch_2L_ddr50
{0b0101, 0b0010},
//2inch_2L_sdr104
{0b1100, 0b0100},
/*****************4inch*************/
//4inch_4L_hs-sdr
{0b0111, 0b0001},
//4inch_4L_sdr12-sdr25
{0b0111, 0b0001},
//4inch_4L_sdr50
{0b0110, 0b0011},
//4inch_4L_ddr50
{0b0110, 0b0011},
//4inch_4L_sdr104
{0b1110, 0b0110},

//4inch_2L_hs-sdr
{0b0111, 0b0001},
//4inch_2L_sdr12-sdr25
{0b0111, 0b0001},
//4inch_2L_sdr50
{0b0110, 0b0011},
//4inch_2L_ddr50
{0b0110, 0b0011},
//4inch_2L_sdr104
{0b1110, 0b0110},
};

