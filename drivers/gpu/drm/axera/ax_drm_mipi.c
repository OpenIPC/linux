/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/iopoll.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/mfd/syscon.h>
#include <linux/phy/phy.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <video/mipi_display.h>

#include "ax_drm_crtc.h"
#include "ax_mipi_dsi.h"


/*DISPC_SYS_GLB start.Base Address 0X4600000*/

/* Register : CLK_MUX_0 */
#define DISPC_CLK_MUX0_ADDR 0x0
#define DISPC_CLK_MUX0_DEFAULT 0x100

/* Register : CLK_MUX_0_SET */
#define DISPC_CLK_MUX0_SET_ADDR 0xA0

/* Register : CLK_MUX_0_CLR */
#define DISPC_CLK_MUX0_CLR_ADDR 0xA4

/* Field in register : CLK_MUX_0 */
#define DISPC_CLK_DISPC_GLB_SEL_24M  0x0
#define DISPC_CLK_DISPC_GLB_SEL_100M  0x1
#define DISPC_CLK_DISPC_GLB_SEL_208M  0x2
#define DISPC_CLK_DISPC_GLB_SEL_312M  0x3
#define DISPC_CLK_DISPC_GLB_SEL_416M  0x4

#define DISPC_CLK_DPHY_TX_ESC_SEL_20M  0x8

/* Register : DSI  */
#define DISPC_DSI_ADDR 0x14
#define DISPC_DSI_DEFAULT 0x100

/* Register : DSI_set */
#define DISPC_DSI_SET_ADDR 0xC0

/* Register : DSI_clr */
#define DISPC_DSI_CLR_ADDR 0xC4

/* Field in register : DSI */
#define DISPC_DSI_PPI_C_TX_READY_HS0 BIT(8)
#define DISPC_DSI_DSI0_DPHY_PLL_LOCK BIT(4)
#define DISPC_DSI_DSI0_MODE BIT(0)

/* Register : LVDS_CLK_SEL */
#define DISPC_LVDS_CLK_SEL_ADDR 0x4C
#define DISPC_LVDS_CLK_SEL_ADDR_DEFAULT 0x0

/* Register : LVDS_CLK_SEL_set */
#define DISPC_LVDS_CLK_SEL_SET_ADDR 0x120

/* Register : LVDS_CLK_SEL_clr */
#define DISPC_LVDS_CLK_SEL_CLR_ADDR 0x124

/* Field in register : LVDS_CLK_SEL */
#define DISPC_LVDS_CLK_SEL_BIT BIT(0)


/* Register : dsi_axi2csi_share_mem_sel */
#define DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_ADDR 0x7C
#define DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_ADDR_DEFAULT 0x0

/* Register : dsi_axi2csi_share_mem_sel_set */
#define DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_SET_ADDR 0x178

/* Register : dsi_axi2csi_share_mem_sel_clr */
#define DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_CLR_ADDR 0x17C

/* Field in register : dsi_axi2csi_share_mem_sel */
#define DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_BIT BIT(0)

static const struct of_device_id ax_mipi_dsi_dt_ids[] = {
	{
	 .compatible = "axera,dsi",
	 },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ax_mipi_dsi_dt_ids);

static const struct drm_encoder_funcs ax_dsi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static void ax_dsi_clk_unprepare(struct cdns_dsi *dsi)
{
	clk_disable_unprepare(dsi->dsi_sys_clk);
	clk_disable_unprepare(dsi->dsi_txesc_clk);
	clk_disable_unprepare(dsi->dsi_hs_clk);
	clk_disable_unprepare(dsi->pll_ref_clk);
	clk_disable_unprepare(dsi->dphytx_esc_clk);
	clk_disable_unprepare(dsi->comm_dphytx_tlb_clk);
}

static int ax_dsi_clk_prepare(struct cdns_dsi *dsi)
{
	int ret = 0;

	ret = clk_prepare_enable(dsi->dsi_sys_clk);
	if (ret) {
		DRM_ERROR("enable dsi_sys_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(dsi->dsi_txesc_clk);
	if (ret) {
		DRM_ERROR("enable dsi_txesc_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(dsi->dsi_hs_clk);
	if (ret) {
		DRM_ERROR("enable dsi_hs_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(dsi->pll_ref_clk);
	if (ret) {
		DRM_ERROR("enable pll_ref_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(dsi->dphytx_esc_clk);
	if (ret) {
		DRM_ERROR("enable dphytx_esc_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(dsi->comm_dphytx_tlb_clk);
	if (ret) {
		DRM_ERROR("enable comm_dphytx_tlb_clk failed\n");
		goto exit;
	}


exit:
	if (ret)
		ax_dsi_clk_unprepare(dsi);

	return ret;
}

static void ax_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct ax_mipi_dsi *ax_dsi = container_of(encoder, struct ax_mipi_dsi, encoder);
	struct mipi_dsi_priv *dsi_priv = &ax_dsi->dsi_priv;
	struct cdns_dsi *dsi = (struct cdns_dsi *)(dsi_priv->data);

	ax_dsi_clk_unprepare(dsi);
	ax_dsi->status = false;
	ax_display_reset_bootlogo_mode();
}

static bool ax_dsi_encoder_mode_fixup(struct drm_encoder *encoder,
	const struct drm_display_mode *mode, struct drm_display_mode *adj_mode)
{
	struct ax_mipi_dsi *ax_dsi = container_of(encoder, struct ax_mipi_dsi, encoder);
	struct drm_crtc_state *crtc_state = container_of(mode, struct drm_crtc_state, mode);

	DRM_DEBUG_DRIVER("enter, bind crtc%d\n",
			 drm_crtc_index(crtc_state->crtc));
	DRM_DEBUG_DRIVER("mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(mode));
	DRM_DEBUG_DRIVER("adj_mode: " DRM_MODE_FMT "\n",
			 DRM_MODE_ARG(adj_mode));

	ax_dsi->crtc = crtc_state->crtc;

	adj_mode->flags &= ~(DRM_MODE_FLAG_NVSYNC | DRM_MODE_FLAG_NHSYNC);
	/* Hsync vsync polarity are needed by mipi host syncing with dpu dpi output data.
	 * Dpu sets inverted hsync vsync polarity. And default keep polarity high.
	 * Here should't configure polarity. */
	adj_mode->flags |= (DRM_MODE_FLAG_PVSYNC | DRM_MODE_FLAG_PHSYNC);

	return true;
}
extern void dsi_dphy_config(int lanes, unsigned long long lane_bps, struct cdns_dphy *dphy);
extern int cdns_dsi_mode2cfg(struct cdns_dsi *dsi,
			     const struct drm_display_mode *mode,
			     struct cdns_dsi_cfg *dsi_cfg,
			     struct cdns_dphy_cfg *dphy_cfg,
			     bool mode_valid_check);

static enum drm_mode_status
ax_mipi_drm_mode_valid(struct cdns_dsi *dsi, const struct drm_display_mode *mode)
{
	struct cdns_dsi_cfg dsi_cfg;
	struct cdns_dphy_cfg dphy_cfg;
	int bpp, ret;
	/*
	 * VFP_DSI should be less than VFP_DPI and VFP_DSI should be at
	 * least 1.
	 */
	if (mode->vtotal - mode->vsync_end < 2)
		return MODE_V_ILLEGAL;

	/* VSA_DSI = VSA_DPI and must be at least 2. */
	if (mode->vsync_end - mode->vsync_start < 2)
		return MODE_V_ILLEGAL;

	/* HACT must be 32-bits aligned. */
	bpp = mipi_dsi_pixel_format_to_bpp(dsi->output.dev->format);
	if ((mode->hdisplay * bpp) % 32)
		return MODE_H_ILLEGAL;

	ret = cdns_dsi_mode2cfg(dsi, mode, &dsi_cfg, &dphy_cfg, true);
	if (ret) {
		DRM_INFO("%s ret = %d\n", __func__, ret);
		return MODE_CLOCK_RANGE;
	}

	DRM_INFO("%s done\n", __func__);

	return MODE_OK;
}

static void ax_dsi_encoder_mode_set(struct drm_encoder *encoder,
				    struct drm_display_mode *mode,
				    struct drm_display_mode *adj_mode)
{

	unsigned bpp, nlanes;
	unsigned long long lane_bps;
	struct ax_mipi_dsi *ax_dsi = container_of(encoder, struct ax_mipi_dsi, encoder);
	struct drm_crtc *crtc = ax_dsi->crtc;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	struct mipi_dsi_priv *dsi_priv = &ax_dsi->dsi_priv;
	struct cdns_dsi *dsi = (struct cdns_dsi *)(dsi_priv->data);

	DRM_INFO("mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(mode));
	DRM_INFO("adj_mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(adj_mode));

	bpp = mipi_dsi_pixel_format_to_bpp(dsi->output.dev->format);
	nlanes = dsi->output.dev->lanes;
	lane_bps = (unsigned long long)mode->clock * 1000;
	lane_bps *= bpp;
	lane_bps = DIV_ROUND_DOWN_ULL(lane_bps, nlanes);
	dsi_priv->mode_flags = dsi->output.dev->mode_flags;
	dsi_priv->lanes = nlanes;
	dsi_priv->format = dsi->output.dev->format;

	ax_mipi_drm_mode_valid(dsi,adj_mode);

	if (dsi_priv->mode_flags & MIPI_DSI_MODE_VIDEO)
		ax_crtc->mode.type = AX_DISP_OUT_MODE_DSI_DPI_VIDEO;
	else
		ax_crtc->mode.type = AX_DISP_OUT_MODE_DSI_SDI_CMD;

	if (dp_funs->dispc_check_mode)
		dp_funs->dispc_check_mode(dp_dev->data, ax_crtc->mode.type);

	if (dsi_priv->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		lane_bps *= CDNS_MODE_VIDEO_BURST_CLK_RATIO;

	DRM_INFO("%s bpp = %d, nlanes = %d, lane_bps = %lld\n",
		__func__, bpp, nlanes, lane_bps);

	dsi->dphy->cfg.nlanes = nlanes;
	dsi->dphy->cfg.lane_bps = lane_bps;

	ax_crtc->mode.de_pol = 0;

	switch (dsi_priv->format) {
	case MIPI_DSI_FMT_RGB666:
		ax_crtc->mode.fmt_out = AX_DISP_OUT_FMT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		ax_crtc->mode.fmt_out = AX_DISP_OUT_FMT_RGB666LP;
		break;
	case MIPI_DSI_FMT_RGB565:
		ax_crtc->mode.fmt_out = AX_DISP_OUT_FMT_RGB565;
		break;
	default:
		ax_crtc->mode.fmt_out = AX_DISP_OUT_FMT_RGB888;
	}
}

static void ax_dsi_reset(struct cdns_dsi *dsi)
{
	reset_control_assert(dsi->dsi_p_rst);
	reset_control_assert(dsi->dsi_txpix_rst);
	reset_control_assert(dsi->dsi_txesc_rst);
	reset_control_assert(dsi->dsi_sys_rst);
	reset_control_assert(dsi->dphy2dsi_rst);
	reset_control_assert(dsi->dsi_rx_esc_rst);
	reset_control_assert(dsi->dphytx_rst);

	udelay(100);

	reset_control_deassert(dsi->dsi_p_rst);
	reset_control_deassert(dsi->dsi_txpix_rst);
	reset_control_deassert(dsi->dsi_txesc_rst);
	reset_control_deassert(dsi->dsi_sys_rst);
	reset_control_deassert(dsi->dphy2dsi_rst);
	reset_control_deassert(dsi->dsi_rx_esc_rst);
}

static void ax_dsi_encoder_mode_enable(struct drm_encoder *encoder)
{
	struct ax_mipi_dsi *ax_dsi = container_of(encoder, struct ax_mipi_dsi, encoder);
	struct mipi_dsi_priv *dsi_priv = &ax_dsi->dsi_priv;
	struct cdns_dsi *dsi = (struct cdns_dsi *)(dsi_priv->data);

	ax_dsi_reset(dsi);

	ax_dsi_clk_prepare(dsi);

	dsi_dphy_config(dsi->dphy->cfg.nlanes, dsi->dphy->cfg.lane_bps, dsi->dphy);

	writel((DISPC_CLK_DISPC_GLB_SEL_416M), dsi->dispc_sys_regs + DISPC_CLK_MUX0_SET_ADDR);
	writel((DISPC_DSI_DSI0_MODE | DISPC_DSI_DSI0_DPHY_PLL_LOCK | DISPC_DSI_PPI_C_TX_READY_HS0), dsi->dispc_sys_regs + DISPC_DSI_SET_ADDR);
	writel(DISPC_LVDS_CLK_SEL_BIT, dsi->dispc_sys_regs + DISPC_LVDS_CLK_SEL_SET_ADDR);
	writel(DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_BIT, dsi->dispc_sys_regs + DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_SET_ADDR);
	udelay(100);
	reset_control_deassert(dsi->dphytx_rst);

	ax_dsi->status = true;
}

static const struct drm_encoder_helper_funcs ax_dsi_encoder_helper_funcs = {
	.mode_fixup = ax_dsi_encoder_mode_fixup,
	.mode_set = ax_dsi_encoder_mode_set,
	.enable = ax_dsi_encoder_mode_enable,
	.disable = ax_dsi_encoder_disable,
};

int cdns_mipi_dsi_bind(struct platform_device *pdev, struct drm_encoder *encoder);
void cdns_mipi_dsi_unbind(struct platform_device *pdev);
static int ax_mipi_dsi_bind(struct device *dev, struct device *master, void *data)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm_dev = (struct drm_device *)data;
	struct ax_mipi_dsi *ax_dsi;
	struct mipi_dsi_priv *dsi_priv;
	struct drm_encoder *encoder;

	ax_dsi = devm_kzalloc(&pdev->dev, sizeof(*ax_dsi), GFP_KERNEL);
	if (!ax_dsi) {
		ret = -ENODEV;
		goto exit0;
	}

	if (!pdev->dev.of_node) {
		DRM_ERROR("of_node is null, dev: %px\n", dev);
		ret = -ENODEV;
		goto exit0;
	}

	dsi_priv = &ax_dsi->dsi_priv;

	encoder = &ax_dsi->encoder;
	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm_dev, dev->of_node);
	if (encoder->possible_crtcs == 0) {
		DRM_ERROR("can not find crtcs for encoder\n");
		ret = -EPROBE_DEFER;
		goto exit0;
	}

	drm_encoder_helper_add(encoder, &ax_dsi_encoder_helper_funcs);
	drm_encoder_init(drm_dev, encoder, &ax_dsi_encoder_funcs, DRM_MODE_ENCODER_DSI, NULL);

	ret = cdns_mipi_dsi_bind(pdev, encoder);
	if (ret) {
		DRM_ERROR("bind cdns-mipi dsi failed\n");
		goto exit1;
	}

	dsi_priv->data = platform_get_drvdata(pdev);

	DRM_INFO("cdns-mipi dsi bind done\n");

	return 0;

exit1:
	drm_encoder_cleanup(encoder);
exit0:
	DRM_ERROR("bind cdns-mipi dsi failed, ret = %d\n", ret);

	return ret;
}

static void ax_mipi_dsi_unbind(struct device *dev, struct device *master,
			       void *data)
{
	DRM_INFO("mipi dsi unbind\n");
}

static const struct component_ops ax_mipi_dsi_ops = {
	.bind = ax_mipi_dsi_bind,
	.unbind = ax_mipi_dsi_unbind,
};

static int ax_mipi_dsi_probe(struct platform_device *pdev)
{
	DRM_INFO("mipi dsi probe, of_node: 0x%px\n", pdev->dev.of_node);

	return component_add(&pdev->dev, &ax_mipi_dsi_ops);
}

static int ax_mipi_dsi_remove(struct platform_device *pdev)
{
	DRM_INFO("mipi dsi remove\n");

	component_del(&pdev->dev, &ax_mipi_dsi_ops);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
void cdns_dsi_bridge_enable(struct drm_bridge *bridge);
void cdns_dsi_resume(struct device *dev);
void cdns_dsi_suspend(struct device *dev);
static int ax_mipi_dsi_suspend(struct device *dev)
{
	struct cdns_dsi *dsi = dev_get_drvdata(dev);
	struct ax_mipi_dsi *ax_dsi = container_of(dsi->input.bridge.encoder, struct ax_mipi_dsi, encoder);
	if (ax_dsi->status)
		cdns_dsi_suspend(dev);
	ax_display_reset_bootlogo_mode();

	return 0;
}

static int ax_mipi_dsi_resume(struct device *dev)
{
	struct cdns_dsi *dsi = dev_get_drvdata(dev);
	struct ax_mipi_dsi *ax_dsi = container_of(dsi->input.bridge.encoder, struct ax_mipi_dsi, encoder);

	if (ax_dsi->status) {
		cdns_dsi_resume(dev);
		dsi_dphy_config(dsi->dphy->cfg.nlanes, dsi->dphy->cfg.lane_bps, dsi->dphy);
		writel((DISPC_CLK_DISPC_GLB_SEL_416M), dsi->dispc_sys_regs + DISPC_CLK_MUX0_SET_ADDR);
		writel((DISPC_DSI_DSI0_MODE | DISPC_DSI_DSI0_DPHY_PLL_LOCK | DISPC_DSI_PPI_C_TX_READY_HS0), dsi->dispc_sys_regs + DISPC_DSI_SET_ADDR);
		writel(DISPC_LVDS_CLK_SEL_BIT, dsi->dispc_sys_regs + DISPC_LVDS_CLK_SEL_SET_ADDR);
		writel(DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_BIT, dsi->dispc_sys_regs + DISPC_DSI_AXI2CSI_SHARE_MEM_SEL_SET_ADDR);
		udelay(100);
		reset_control_deassert(dsi->dphytx_rst);
		cdns_dsi_bridge_enable(&dsi->input.bridge);
	}
	return 0;
}
#endif

static const struct dev_pm_ops ax_mipi_dsi_pm_ops = {
	.suspend_noirq = ax_mipi_dsi_suspend,
	.resume_noirq = ax_mipi_dsi_resume,
};

struct platform_driver ax_mipi_dsi_driver = {
	.probe = ax_mipi_dsi_probe,
	.remove = ax_mipi_dsi_remove,
	.driver = {
		   .of_match_table = ax_mipi_dsi_dt_ids,
		   .name = "mipi-dsi-drv",
		   .pm = &ax_mipi_dsi_pm_ops,
	},
};

MODULE_AUTHOR("zhengwanhu@axera-tech.com");
MODULE_DESCRIPTION("Axera mipi dsi driver");
MODULE_LICENSE("GPL v2");
