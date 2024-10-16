/* sound\soc\sunxi\snd_sunxi_daudio.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN8IW21_DAUDIO_H
#define __SND_SUN8IW21_DAUDIO_H

#define HLOG		"DAUDIO"

struct sunxi_daudio_clk {
	struct clk *pllaudio;
	struct clk *clk_module;
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_daudio_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_daudio_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_daudio_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_daudio_clk *clk);

static inline int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_daudio_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	clk->pllaudio = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "pllaudio get failed\n");
		ret = PTR_ERR(clk->pllaudio);
		goto err_pllaudio;
	}
	clk->clk_module = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(clk->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module get failed\n");
		ret = PTR_ERR(clk->clk_module);
		goto err_clk_module;
	}

	if (clk_set_parent(clk->clk_module, clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "set parent of clk_module to pllaudio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk->clk_module);
err_clk_module:
	clk_put(clk->pllaudio);
err_pllaudio:
	return ret;
}

static inline void snd_sunxi_clk_exit(struct sunxi_daudio_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(clk);
	clk_put(clk->clk_module);
	clk_put(clk->pllaudio);
}

static inline int snd_sunxi_clk_enable(struct sunxi_daudio_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_prepare_enable(clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "pllaudio enable failed\n");
		goto err_enable_pllaudio;
	}

	if (clk_prepare_enable(clk->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module enable failed\n");
		goto err_enable_clk_module;
	}

	return 0;

err_enable_clk_module:
	clk_disable_unprepare(clk->pllaudio);
err_enable_pllaudio:
	return ret;
}

static inline void snd_sunxi_clk_disable(struct sunxi_daudio_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->clk_module);
	clk_disable_unprepare(clk->pllaudio);
}

#endif /* __SND_SUN8IW21_DAUDIO_H */
