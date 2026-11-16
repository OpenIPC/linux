// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
#include "tx-isp-stats.h"
#include "tx-isp-regs.h"

#define T23_STATS_INPUTS		2
#define T23_STATS_SIZE		(T23_STATS_INPUTS * T23_STATS_INPUT_SIZE)

static const u32 sc1a4t_af_filters[] = {
	0x008007ea, 0x0080012c, 0x000007ea, 0x078a07eb,
	0x078a013d, 0x000007eb, 0x02000026, 0x00d60100,
	0x01000793, 0x00560053, 0x060000ee, 0x01da0100,
	0x07000722, 0x00ef00de, 0xff000800, 0x23b0ffeb,
	0xff002400, 0x23a0f0dc, 0xff001e00, 0x0ff0f5e6,
	0xff001900, 0x0f80fadc, 0x01fe00ff, 0x00ff0010,
	0x01fe00ff, 0x00ff0010, 0x01fe00ff, 0x00ff0010,
	0x01fe00ff, 0x00ff0010,
};

int t23_isp_stats_alloc(struct t23_isp_stats *stats, struct device *dev)
{
	if (stats->buffer)
		return stats->dev == dev ? 0 : -EINVAL;
	stats->buffer = dma_alloc_coherent(dev, T23_STATS_SIZE, &stats->dma,
					 GFP_KERNEL);
	if (!stats->buffer)
		return -ENOMEM;
	if ((u64)stats->dma + T23_STATS_SIZE - 1 > U32_MAX) {
		dma_free_coherent(dev, T23_STATS_SIZE, stats->buffer, stats->dma);
		stats->buffer = NULL;
		stats->dma = 0;
		return -ERANGE;
	}
	stats->dev = dev;
	stats->active = false;
	memset(stats->buffer, 0, T23_STATS_SIZE);
	return 0;
}

static void t23_stats_banks(void __iomem *base, unsigned int offset,
			    dma_addr_t address, unsigned int stride)
{
	unsigned int i;

	for (i = 0; i < 4; i++)
		writel(address + i * stride, base + offset + i * 4);
}

static void t23_stats_zone_sizes(void __iomem *base, unsigned int offset,
				unsigned int count, unsigned int size)
{
	unsigned int i, j;
	u32 value;

	for (i = 0; i < 15; i += 4) {
		value = 0;
		for (j = 0; j < 4 && i + j < count; j++)
			value |= size << (j * 8);
		writel(value, base + offset + i);
	}
}

static void t23_stats_boundaries(void __iomem *base, unsigned int offset,
				unsigned int count, unsigned int size)
{
	unsigned int i, low, high;

	for (i = 0; i <= count; i += 2) {
		low = i * (size / count) + min(i, size % count);
		high = i == count ? 0 :
			(i + 1) * (size / count) + min(i + 1, size % count);
		writel(low | high << 16, base + offset + i * 2);
	}
}

static int t23_stats_geometry(void __iomem *base)
{
	u32 geometry = readl(base + ISP_TOP_IMAGE_SIZE);
	unsigned int width = geometry >> 16, height = geometry & 0xffff;
	unsigned int x, y, margin_x, margin_y, offset;

	if (width < 32 || height < 32 || width > 1920 || height > 1080)
		return -EINVAL;

	/* AE and AWB collect 15 by 15 zones in the Bayer half-resolution grid. */
	for (offset = ISP_AE_REG_BASE; offset <= ISP_AWB_REG_BASE; offset += 0x1000) {
		writel(ISP_AE_CTRL_DEFAULT, base + offset + 4);
		t23_stats_zone_sizes(base, offset + 8, 15, width / 30);
		t23_stats_zone_sizes(base, offset + 0x18, 15, height / 30);
	}
	writel(ISP_AE_LUM_TH_FREQ_DEFAULT, base + ISP_AE_LUM_TH_FREQ);
	writel(ISP_AWB_TRIGGER_COMMIT, base + ISP_AWB_TRIGGER);
	writel(ISP_AWB_COLOR_PARAM0_DEFAULT, base + ISP_AWB_COLOR_PARAM0);
	writel(ISP_AWB_TRIGGER_COMMIT, base + ISP_AWB_TRIGGER);
	writel(ISP_AWB_COLOR_PARAM1_DEFAULT, base + ISP_AWB_COLOR_PARAM1);
	writel(ISP_AWB_TRIGGER_COMMIT, base + ISP_AWB_TRIGGER);
	writel(ISP_AWB_COLOR_PARAM2_DEFAULT, base + ISP_AWB_COLOR_PARAM2);
	writel(ISP_AWB_TRIGGER_COMMIT, base + ISP_AWB_TRIGGER);
	writel(ISP_AWB_COLOR_PARAM3_DEFAULT, base + ISP_AWB_COLOR_PARAM3);
	/* Initial luminance threshold and sampling frequency for AWB statistics. */
	writel(ISP_AWB_TRIGGER_COMMIT, base + ISP_AWB_TRIGGER);
	writel(ISP_AWB_LUM_TH_FREQ_DEFAULT, base + ISP_AWB_LUM_TH_FREQ);

	writel(ISP_AF_CTRL_DEFAULT, base + ISP_AF_CONTROL);
	t23_stats_zone_sizes(base, ISP_AF_ZONE_W_BASE, 8, ((width - 15) / 8) & ~1U);
	t23_stats_zone_sizes(base, ISP_AF_ZONE_H_BASE, 8, ((height - 3) / 8) & ~1U);
	writel(ISP_AF_TRIGGER_COMMIT, base + ISP_AF_TRIGGER);
	writel(ISP_AF_THRES0_DEFAULT, base + ISP_AF_THRES0);
	writel(0, base + ISP_AF_THRES1);
	for (offset = 0; offset < ARRAY_SIZE(sc1a4t_af_filters); offset++)
		writel(sc1a4t_af_filters[offset], base + ISP_AF_FILTER_BASE + offset * 4);

	x = (width / 6) & ~1U;
	y = (height / 4) & ~1U;
	margin_x = max((x / 4) & ~1U, 20U);
	margin_y = max((y / 4) & ~1U, 20U);
	writel(x | y << 16, base + ISP_ADR_GRID_STEP);
	writel(y << 16, base + ISP_ADR_GRID_Y0);
	writel(2 * y | 3 * y << 16, base + ISP_ADR_GRID_Y1);
	writel(height, base + ISP_ADR_GRID_HEIGHT);
	writel(x << 16, base + ISP_ADR_GRID_X0);
	writel(2 * x | 3 * x << 16, base + ISP_ADR_GRID_X1);
	writel(4 * x | 5 * x << 16, base + ISP_ADR_GRID_X2);
	writel(width, base + ISP_ADR_GRID_WIDTH);
	writel(margin_y | (height - margin_y) << 16, base + ISP_ADR_MARGIN_Y);
	writel(margin_x | (width - margin_x) << 16, base + ISP_ADR_MARGIN_X);

	t23_stats_boundaries(base, ISP_DEFOG_BOUND_V_BASE, 10, height);
	t23_stats_boundaries(base, ISP_DEFOG_BOUND_H_BASE, 18, width);
	writel(0, base + ISP_DEFOG_MIN_THRES);
	writel(U32_MAX, base + ISP_DEFOG_MAX_THRES);
	writel(0, base + ISP_DEFOG_CONTROL);
	return 0;
}

int t23_isp_stats_configure(struct t23_isp_stats *stats, void __iomem *base,
			    unsigned int input)
{
	dma_addr_t address;
	int ret;

	if (!stats->buffer || input >= T23_STATS_INPUTS)
		return -EINVAL;
	address = stats->dma + input * T23_STATS_INPUT_SIZE;
	dma_wmb();
	/* AE writes 4 KiB of zone data and 2 KiB of histogram data. */
	t23_stats_banks(base, ISP_AE_DMA_ADDR_BASE, address, 0);
	t23_stats_banks(base, ISP_AE_DMA_HIST_ADDR_BASE, address + 0x1000, 0);
	t23_stats_banks(base, ISP_AWB_DMA_ADDR_BASE, address + 0x2000, 0);
	t23_stats_banks(base, ISP_AF_DMA_ADDR_BASE, address + 0x3000, 0);
	t23_stats_banks(base, ISP_ADR_DMA_ADDR_BASE, address + 0x4000, 0x1000);
	t23_stats_banks(base, ISP_DEFOG_DMA_ADDR_BASE, address + 0x8000, 0x1000);
	ret = t23_stats_geometry(base);
	if (ret)
		return ret;
	writel(ISP_AE_DMA_CTRL_DEFAULT, base + ISP_AE_DMA_CTRL);
	writel(ISP_AWB_DMA_CTRL_DEFAULT, base + ISP_AWB_DMA_CTRL);
	writel(ISP_AF_DMA_CTRL_DEFAULT, base + ISP_AF_DMA_CTRL);
	writel(ISP_ADR_DMA_ENABLE, base + ISP_ADR_DMA_CTRL);
	writel(ISP_DEFOG_DMA_ENABLE, base + ISP_DEFOG_DMA_CTRL);
	stats->active = true;
	return 0;
}

int t23_isp_stats_free(struct t23_isp_stats *stats)
{
	if (stats->active)
		return -EBUSY;
	if (!stats->buffer)
		return 0;
	dma_free_coherent(stats->dev, T23_STATS_SIZE, stats->buffer, stats->dma);
	memset(stats, 0, sizeof(*stats));
	return 0;
}
