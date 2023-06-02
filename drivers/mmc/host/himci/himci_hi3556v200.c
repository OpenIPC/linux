/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#define MMC_CRG_MIN		25000000

#define TUNING_START_PHASE	0
#define TUNING_END_PHASE	7
#define HIMCI_PHASE_SCALE	8
#define DRV_PHASE_DFLT		(0x4<<23)
#define SMPL_PHASE_DFLT		(0x0<<16)

/* eMMC pad ctrl reg */
#define REG_CTRL_EMMC_START     (0x10ff0000 + 0x0)
/* sdio0 pad ctrl reg */
#define REG_CTRL_SDIO0_START	(0x10ff0000 + 0x24)
/* sdio1 pad ctrl reg */
#define REG_CTRL_SDIO1_START    (0x112f0000 + 0x8)

static unsigned int pad_ctrl_start[] = {REG_CTRL_EMMC_START, REG_CTRL_SDIO0_START, REG_CTRL_SDIO1_START};

                                    /*  clk   cmd   data0  data1  data2  data3 */
static unsigned int emmc_hs200_drv[] = {0x2b0, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0};
static unsigned int emmc_hs_drv[] = {0x6b0, 0x5e0, 0x5e0, 0x5e0, 0x5e0, 0x5e0};
static unsigned int emmc_ds_drv[] = {0x6b0, 0x5f0, 0x5f0, 0x5f0, 0x5f0, 0x5f0};
static unsigned int emmc_ds_400k_drv[] = {0x6c0, 0x5f0, 0x5f0, 0x5f0, 0x5f0, 0x5f0};

static unsigned int sdio0_sdr104_drv[] = {0x290, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0};
static unsigned int sdio0_sdr50_drv[] = {0x290, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0};
static unsigned int sdio0_sdr25_drv[] = {0x6b0, 0x5d0, 0x5d0, 0x5d0, 0x5d0, 0x5d0};
static unsigned int sdio0_sdr12_drv[] = {0x6b0, 0x5e0, 0x5e0, 0x5e0, 0x5e0, 0x5e0};
static unsigned int sdio0_hs_drv[] = {0x6d0, 0x5f0, 0x5f0, 0x5f0, 0x5f0, 0x5f0};
static unsigned int sdio0_ds_drv[] = {0x6b0, 0x5e0, 0x5e0, 0x5e0, 0x5e0, 0x5e0};

static unsigned int sdio1_sdr104_drv[] = {0x290, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0};
static unsigned int sdio1_sdr50_drv[] = {0x290, 0x1c0, 0x1c0, 0x1c0, 0x1c0, 0x1c0};
static unsigned int sdio1_sdr25_drv[] = {0x6b0, 0x5d0, 0x5d0, 0x5d0, 0x5d0, 0x5d0};
static unsigned int sdio1_sdr12_drv[] = {0x6b0, 0x5e0, 0x5e0, 0x5e0, 0x5e0, 0x5e0};
static unsigned int sdio1_hs_drv[] = {0x6d0, 0x5f0, 0x5f0, 0x5f0, 0x5f0, 0x5f0};
static unsigned int sdio1_ds_drv[] = {0x6b0, 0x5e0, 0x5e0, 0x5e0, 0x5e0, 0x5e0};

static void himci_set_drv_cap(struct himci_host *host, unsigned int vdd_180)
{
	struct mmc_host *mmc = host->mmc;
	struct mmc_ios *ios = &(mmc->ios);
	unsigned int devid = host->devid;
	unsigned char timing = ios->timing;
    unsigned int i, j, start;
    unsigned int *pin_drv_cap = NULL;

    /*eMMC*/
    if(devid == 0) {
        if(timing == MMC_TIMING_MMC_HS200)
            pin_drv_cap = emmc_hs200_drv;
        else if(timing == MMC_TIMING_MMC_HS)
            pin_drv_cap = emmc_hs_drv;
        else {
            if(ios->clock == 400000)/*400K*/
                pin_drv_cap = emmc_ds_400k_drv;
            else
                pin_drv_cap = emmc_ds_drv;
        }
    } else if(devid == 1) {/* SDIO0 */
        if(timing == MMC_TIMING_UHS_SDR104)
            pin_drv_cap = sdio0_sdr104_drv;
        else if(timing == MMC_TIMING_UHS_SDR50)
            pin_drv_cap = sdio0_sdr50_drv;
        else if(timing == MMC_TIMING_UHS_SDR25)
            pin_drv_cap = sdio0_sdr25_drv;
        else if(timing == MMC_TIMING_UHS_SDR12)
            pin_drv_cap = sdio0_sdr12_drv;
        else if(timing == MMC_TIMING_SD_HS)
            pin_drv_cap = sdio0_hs_drv;
        else
            pin_drv_cap = sdio0_ds_drv;
    } else {/* SDIO1 */
        if(timing == MMC_TIMING_UHS_SDR104)
            pin_drv_cap = sdio1_sdr104_drv;
        else if(timing == MMC_TIMING_UHS_SDR50)
            pin_drv_cap = sdio1_sdr50_drv;
        else if(timing == MMC_TIMING_UHS_SDR25)
            pin_drv_cap = sdio1_sdr25_drv;
        else if(timing == MMC_TIMING_UHS_SDR12)
            pin_drv_cap = sdio1_sdr12_drv;
        else if(timing == MMC_TIMING_SD_HS)
            pin_drv_cap = sdio1_hs_drv;
        else
            pin_drv_cap = sdio1_ds_drv;
    }

    start = (unsigned int)(long)ioremap((resource_size_t)pad_ctrl_start[devid], (size_t)0x1000);
    
    for (i = start, j = 0; j < 6; i = i+4, j++) {
        unsigned int reg = himci_readl(i);
        /*
         * [10]:SR
         * [9]:internel pull down
         * [8]:internel pull up
         * [7:4]:
         * */
        reg = reg & (~(0x7f0));
        reg |= pin_drv_cap[j];
        himci_writel(reg, i);
    }
    iounmap((void *)(long)start);
}

#define DRV_PHASE_180   (0x4<<23)
#define DRV_PHASE_135   (0x3<<23)
#define DRV_PHASE_90    (0x2<<23)

#define SMP_PHASE_45    (0x1<<16)
#define SMP_PHASE_0     (0x0<<16)

static void himci_set_default_phase(struct himci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct mmc_ios *ios = &(mmc->ios);
	unsigned int devid = host->devid;
	unsigned char timing = ios->timing;
    unsigned int phase_cfg, reg_value;
    /*eMMC*/
    if(devid == 0) {
        if(timing == MMC_TIMING_MMC_HS200)
            phase_cfg = DRV_PHASE_135 | SMP_PHASE_0;
        else if(timing == MMC_TIMING_MMC_HS)
            phase_cfg = DRV_PHASE_180 | SMP_PHASE_45;
        else {
            phase_cfg = DRV_PHASE_180 | SMP_PHASE_0;
        }
    } else {
        if(timing == MMC_TIMING_UHS_SDR104)
            phase_cfg = DRV_PHASE_135 | SMP_PHASE_0;
        else if(timing == MMC_TIMING_UHS_SDR50)
            phase_cfg = DRV_PHASE_90 | SMP_PHASE_0;
        else if(timing == MMC_TIMING_UHS_SDR25)
            phase_cfg = DRV_PHASE_180 | SMP_PHASE_45;
        else if(timing == MMC_TIMING_SD_HS)
            phase_cfg = DRV_PHASE_135 | SMP_PHASE_45;
        else
            phase_cfg = DRV_PHASE_180 | SMP_PHASE_0;
    }

	reg_value = himci_readl(host->base + MCI_UHS_REG_EXT);
	reg_value &= ~CLK_SMPL_PHS_MASK;
	reg_value &= ~CLK_DRV_PHS_MASK;
	reg_value |= phase_cfg;
	himci_writel(reg_value, host->base + MCI_UHS_REG_EXT);
}

