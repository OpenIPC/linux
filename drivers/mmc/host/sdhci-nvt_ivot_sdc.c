/*
 * novatek ivot sdc controller driver.
 *
 * Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#include <linux/device.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include "sdhci-pltfm.h"

#define  DRIVER_NAME "nvt_ivot_sdc"
#define VERSION     "1.00.00"

#define SDHCI_SDC_VENDOR0                0x100
#define	SDHCI_PULSE_LATCH_OFF_MASK       0x3F00
#define	SDHCI_PULSE_LATCH_OFF_SHIFT      8
#define	SDHCI_PULSE_LATCH_EN             0x1

#define SDHCI_SDC_VENDOR3                0x10C
#define	SDHCI_AUTO_TUNING_TIMES_MASK     0x1F000000
#define	SDHCI_AUTO_TUNING_TIMES_SHIFT    24
#define	SDHCI_SD_DELAY_VAL_MASK          0x1F0000
#define	SDHCI_SD_DELAY_VAL_SHIFT         16

const u8 tuning_blk_pattern_4bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

const u8 tuning_blk_pattern_8bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

struct nvt_mmc_hwconfig {
	unsigned int pulse_latch_offset;
};

int send_tuning_cmd(struct mmc_host *mmc)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	struct mmc_ios *ios = &mmc->ios;
	const u8 *tuning_block_pattern;
	int size, err = 0;
	u8 *data_buf;
	u32 opcode;

	if (ios->bus_width == MMC_BUS_WIDTH_8) {
		tuning_block_pattern = tuning_blk_pattern_8bit;
		size = sizeof(tuning_blk_pattern_8bit);
		opcode = MMC_SEND_TUNING_BLOCK_HS200;
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		tuning_block_pattern = tuning_blk_pattern_4bit;
		size = sizeof(tuning_blk_pattern_4bit);
		opcode = MMC_SEND_TUNING_BLOCK;
	} else
		return -EINVAL;

	data_buf = kzalloc(size, GFP_KERNEL);
	if (!data_buf)
		return -ENOMEM;

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = opcode;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = size;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	/*
	 * According to the tuning specs, Tuning process
	 * is normally shorter 40 executions of CMD19,
	 * and timeout value should be shorter than 150 ms
	 */
	data.timeout_ns = 150 * NSEC_PER_MSEC;

	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, data_buf, size);

	mmc_wait_for_req(mmc, &mrq);

	if (cmd.error) {
		err = cmd.error;
		goto out;
	}

	if (data.error) {
		err = data.error;
		goto out;
	}

	if (memcmp(data_buf, tuning_block_pattern, size))
		err = -EIO;

out:
	kfree(data_buf);
	return err;
}

static int nvt_ivot_sdc_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	unsigned int ven_def;
	int i, first_val, best_val;
	int rc = 0;
	struct mmc_host *mmc = host->mmc;

	ven_def = sdhci_readl(host, SDHCI_SDC_VENDOR0);
	ven_def = sdhci_readl(host, SDHCI_SDC_VENDOR0);
	ven_def &= ~(SDHCI_PULSE_LATCH_OFF_MASK | SDHCI_PULSE_LATCH_EN);
	sdhci_writel(host, ven_def, SDHCI_SDC_VENDOR0);

	ven_def = sdhci_readl(host, SDHCI_SDC_VENDOR0);
	ven_def &= ~SDHCI_AUTO_TUNING_TIMES_MASK;

	first_val = -1;

	for (i = 0; i <= 31; i++) {
		ven_def &= ~SDHCI_SD_DELAY_VAL_MASK;
		ven_def |= (i << SDHCI_SD_DELAY_VAL_SHIFT);
		sdhci_writel(host, ven_def, SDHCI_SDC_VENDOR3);

		rc = send_tuning_cmd(mmc);
		if (!rc && first_val < 0)
			first_val = i;

		if (rc && first_val >= 0)
			break;
	}

	if (first_val == -1) {
		best_val = 0;
		rc = -EINVAL;
	}
	else {
		best_val = (first_val + i) / 4;
		rc = 0;
	}

	ven_def &= ~SDHCI_SD_DELAY_VAL_MASK;
	ven_def |= (best_val << SDHCI_SD_DELAY_VAL_SHIFT);
	sdhci_writel(host, ven_def, SDHCI_SDC_VENDOR3);

	rc = send_tuning_cmd(mmc);

	dev_dbg(mmc_dev(mmc), "%s: tuning complete, delay = %d\n",
		mmc_hostname(mmc), best_val);

	return rc;
}

static void nvt_ivot_sdc_set_clock(struct sdhci_host *host, unsigned int clock)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct nvt_mmc_hwconfig *hwconfig = sdhci_pltfm_priv(pltfm_host);
	int vendor0;

	sdhci_set_clock(host, clock);
	vendor0 = sdhci_readl(host, SDHCI_SDC_VENDOR0);
	if (host->mmc->actual_clock < 100000000)
		vendor0 |= SDHCI_PULSE_LATCH_EN; /* Enable pulse latch */
	else
		vendor0 &= ~(SDHCI_PULSE_LATCH_OFF_MASK | SDHCI_PULSE_LATCH_EN);

	vendor0 &= ~(0x3f << SDHCI_PULSE_LATCH_OFF_SHIFT);
	vendor0 |= (hwconfig->pulse_latch_offset << SDHCI_PULSE_LATCH_OFF_SHIFT);
	sdhci_writel(host, vendor0, SDHCI_SDC_VENDOR0);
}

static unsigned int nvt_ivot_sdc_get_timeout_clk(struct sdhci_host *host)
{
	return 33;
}

static unsigned int nvt_ivot_sdc_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	return pltfm_host->clock;
}

static struct sdhci_ops nvt_ivot_sdc_ops = {
	.platform_execute_tuning = nvt_ivot_sdc_execute_tuning,
	.set_clock = nvt_ivot_sdc_set_clock,
	.get_max_clock = nvt_ivot_sdc_get_max_clk,
	.get_timeout_clock = nvt_ivot_sdc_get_timeout_clk,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
};

static struct sdhci_pltfm_data nvt_ivot_sdc_pdata = {
	.ops = &nvt_ivot_sdc_ops,
	.quirks =  SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
               SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN |
               SDHCI_QUIRK2_BROKEN_HS200 |
               SDHCI_QUIRK2_BROKEN_DDR50,
};

static void nvt_ivot_sdc_hw_remove(void)
{

}

static int nvt_ivot_sdc_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct device *dev = &pdev->dev;
	struct sdhci_host *host = NULL;
	struct nvt_mmc_hwconfig *hwconfig = NULL;
	int ret = 0;
	struct clk *clk;

	host = sdhci_pltfm_init(pdev, &nvt_ivot_sdc_pdata, 0);
	if (IS_ERR(host))
		return PTR_ERR(host);
	pltfm_host = sdhci_priv(host);
	hwconfig = sdhci_pltfm_priv(pltfm_host);

#ifdef CONFIG_OF
	of_property_read_u32(pdev->dev.of_node, "pulse-latch", \
			&hwconfig->pulse_latch_offset);
#endif
	/* Get sd clock (sdclk1x for nvt_ivot_sdc) */
	clk = clk_get(dev, dev_name(dev));
	if (!IS_ERR(clk)) {
		pltfm_host->clk = clk;
		clk_prepare_enable(clk);
		pltfm_host->clock = clk_get_rate(clk);
	}

	ret = mmc_of_parse(host->mmc);
	if (ret) {
		sdhci_pltfm_free(pdev);
		return ret;
	}

	sdhci_get_of_property(pdev);

	/* Set transfer mode, default use ADMA */
#if defined(Transfer_Mode_PIO)
	host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
	host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
#elif defined(Transfer_Mode_SDMA)
	host->quirks |= SDHCI_QUIRK_FORCE_DMA;
	host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
	host->quirks |= SDHCI_QUIRK_32BIT_DMA_ADDR;
	host->quirks |= SDHCI_QUIRK_32BIT_DMA_SIZE;
#else
	host->quirks |= SDHCI_QUIRK_32BIT_DMA_ADDR;
	host->quirks |= SDHCI_QUIRK_32BIT_ADMA_SIZE;
#endif

	ret = sdhci_add_host(host);
	if (ret)
		sdhci_pltfm_free(pdev);

	return ret;
}

static int nvt_ivot_sdc_remove(struct platform_device *pdev)
{
	nvt_ivot_sdc_hw_remove();
	return sdhci_pltfm_unregister(pdev);
}

static const struct of_device_id sdhci_nvt_ivot_sdc_of_match_table[] = {
        { .compatible = "nvt,nvt_ivot_sdc-sdhci", },
        {}
};
MODULE_DEVICE_TABLE(of, sdhci_nvt_ivot_sdc_of_match_table);

static struct platform_driver nvt_ivot_sdc_driver = {
	.driver		= {
		.name	= "nvt_ivot_sdc",
		.owner	= THIS_MODULE,
		.of_match_table = sdhci_nvt_ivot_sdc_of_match_table,
		.pm     = &sdhci_pltfm_pmops,
	},
	.probe		= nvt_ivot_sdc_probe,
	.remove		= nvt_ivot_sdc_remove,
};

module_platform_driver(nvt_ivot_sdc_driver);

MODULE_DESCRIPTION("SDHCI driver for NVT_IVOT_SDC");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(VERSION);

