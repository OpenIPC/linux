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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_graph.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <video/of_display_timing.h>
#include <video/of_videomode.h>
#include <dt-bindings/display/axera_display.h>

#include <drm/drm_print.h>
#include <drm/drm_modes.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_panel.h>
#include <drm/drm_of.h>

#include "ax_drm_crtc.h"
#include "ax_drm_lvds.h"
#include "ax_reg_dphy.h"

#define LVDS_RGB_MODE_REG      0x0
#define LVDS_RGB_MODE_VAL_RGB666           0x0
#define LVDS_RGB_MODE_VAL_RGB888           0x1
#define LVDS_RGB_MODE_VAL_RGB101010        0x2
#define LVDS_RGB_MODE_VAL_RGB666LP         0x3
#define LVDS_RGB_MODE_VAL_PRBS       	   0x7

#define LVDS_MODE_REG          		0x4
#define LVDS_MODE_VAL_JEIDA       	   0x0
#define LVDS_MODE_VAL_VESA       	   0x1

#define LVDS_SHADOW_REG        0x28

/* dispc_sys_glb registers definition start */
#define DISPC_SYSGLB_LVDS_CLK_SEL		0x4C
#define DISPC_SYSGLB_LVDS_CLK_SEL_SET		0x120
#define DISPC_SYSGLB_LVDS_CLK_SEL_CLR		0x124

#define DISPC_SYSGLB_LVDS_CLK_SEL_BIT 		BIT(0)
/* dispc_sys_glb registers definition end */

#define encoder_to_ax_lvds(x) \
	container_of(x, struct ax_lvds, encoder)
#define connector_to_ax_lvds(x) \
	container_of(x, struct ax_lvds, connector)
#define bridge_to_ax_lvds(x) \
	container_of(x, struct ax_lvds, bridge);


/* Connector */

static enum drm_connector_status ax_lvds_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static int ax_lvds_connector_modes(struct drm_connector *connector, uint32_t maxX, uint32_t maxY)
{
	return 0;
}

static const struct drm_connector_funcs ax_lvds_connector_funcs = {
	.detect = ax_lvds_connector_detect,
	.fill_modes = ax_lvds_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static int ax_lvds_connector_get_modes(struct drm_connector *connector)
{
	struct ax_lvds *ax_lvds = connector_to_ax_lvds(connector);


	DRM_DEBUG_DRIVER("enter, [connector:%d:%s]\n", connector->base.id,
			 connector->name);

	return drm_panel_get_modes(ax_lvds->panel);
}

static const struct drm_connector_helper_funcs ax_lvds_connector_helper_funcs = {
	.get_modes = ax_lvds_connector_get_modes,
};


/* Encoder */

static int ax_lvds_encoder_atomic_check(struct drm_encoder *encoder,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *conn_state)
{
	return 0;
}

static void ax_lvds_clk_unprepare(struct ax_lvds *ax_lvds)
{
	clk_disable_unprepare(ax_lvds->lvds_p_clk);
	clk_disable_unprepare(ax_lvds->dphytx_pll_clk);
	clk_disable_unprepare(ax_lvds->dphytx_pll_div7_clk);
	clk_disable_unprepare(ax_lvds->dphytx_ref_clk);
	clk_disable_unprepare(ax_lvds->comm_dphytx_tlb_clk);
}

static int ax_lvds_clk_prepare(struct ax_lvds *ax_lvds)
{
	int ret = 0;

	ret = clk_prepare_enable(ax_lvds->lvds_p_clk);
	if (ret) {
		DRM_ERROR("enable lvds p clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(ax_lvds->dphytx_pll_clk);
	if (ret) {
		DRM_ERROR("enable dphytx_pll_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(ax_lvds->dphytx_pll_div7_clk);
	if (ret) {
		DRM_ERROR("enable dphytx_pll_div7_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(ax_lvds->dphytx_ref_clk);
	if (ret) {
		DRM_ERROR("enable dphytx_ref_clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(ax_lvds->comm_dphytx_tlb_clk);
	if (ret) {
		DRM_ERROR("enable comm_dphytx_tlb_clk failed\n");
		goto exit;
	}

exit:
	if (ret)
		ax_lvds_clk_unprepare(ax_lvds);

	return ret;
}

extern void lvds_dphy_pll_cfg( unsigned long long bps, void __iomem *regs);
static void lvds_dphy_config(unsigned int clk, struct ax_lvds *ax_lvds)
{
	/* leave isolation */
	writel(DPHYTX0_AON_POWER_READY_N_BIT, ax_lvds->comm_sys_regs + DPHY_POWER_READY_CLR_ADDR);
	/* power on */
	writel(DPHYTX0_PWR_OFF_BIT, ax_lvds->comm_sys_regs + DPHY_POWER_OFF_CLR_ADDR);

	lvds_dphy_pll_cfg(clk, ax_lvds->dphytx_regs);

	writel(1, ax_lvds->dphytx_regs + DPHY_TX0_REG59_SET_ADDR);
	udelay(40);
	writel(1, ax_lvds->dphytx_regs + DPHY_TX0_REG60_SET_ADDR);
	udelay(120);
}

static void lvds_dphy_clear(struct ax_lvds *ax_lvds)
{
	/* LVDS BIAS disable */
	writel(1, ax_lvds->dphytx_regs + DPHY_TX0_REG59_CLR_ADDR);
	/* LVDS CLOCK disable */
	writel(1, ax_lvds->dphytx_regs + DPHY_TX0_REG60_CLR_ADDR);

	writel(DPHYTX0_AON_POWER_READY_N_BIT, ax_lvds->comm_sys_regs + DPHY_POWER_READY_SET_ADDR);
	/* power off */
	writel(DPHYTX0_PWR_OFF_BIT, ax_lvds->comm_sys_regs + DPHY_POWER_OFF_SET_ADDR);
}

static void ax_lvds_encoder_disable(struct drm_encoder *encoder)
{
	struct ax_lvds *ax_lvds = encoder_to_ax_lvds(encoder);

	DRM_DEBUG_DRIVER("enter, [encoder:%d:%s]\n", encoder->base.id,
			 encoder->name);

	lvds_dphy_clear(ax_lvds);
	ax_lvds_clk_unprepare(ax_lvds);
	ax_lvds->status = false;

}

static void ax_lvds_encoder_enable(struct drm_encoder *encoder)
{
	struct ax_lvds *ax_lvds = encoder_to_ax_lvds(encoder);
	ax_lvds->status = true;
}

static void ax_lvds_encoder_mode_set(struct drm_encoder *encoder,
				     struct drm_display_mode *mode,
				     struct drm_display_mode *adjusted_mode)
{
	struct ax_lvds *ax_lvds = encoder_to_ax_lvds(encoder);
	struct ax_crtc *ax_crtc = to_ax_crtc(encoder->crtc);
	struct ax_disp_mode *ax_mode = &ax_crtc->mode;
	int ret;
	int clk;

	DRM_DEBUG_DRIVER("enter, [encoder:%d:%s]\n", encoder->base.id,
			 encoder->name);
	DRM_DEBUG_DRIVER("mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(mode));
	DRM_DEBUG_DRIVER("adjusted_mode: " DRM_MODE_FMT "\n",
			 DRM_MODE_ARG(adjusted_mode));

	ret = ax_lvds_clk_prepare(ax_lvds);
	if (ret)
		DRM_ERROR("lvds encoder clk prepare failed, ret = %d\n", ret);

	clk = adjusted_mode->clock * 7 * 1000;

	lvds_dphy_config(clk, ax_lvds);

	writel(DISPC_SYSGLB_LVDS_CLK_SEL_BIT, ax_lvds->dispc_sys_regs + DISPC_SYSGLB_LVDS_CLK_SEL_CLR);

	ax_mode->type = AX_DISP_OUT_MODE_LVDS;
	if (LVDS_VESA_24 == ax_lvds->fmt_out || LVDS_JEIDA_24 == ax_lvds->fmt_out) {
		ax_mode->fmt_out = AX_DISP_OUT_FMT_RGB888;
	} else if (LVDS_JEIDA_18 == ax_lvds->fmt_out) {
		ax_mode->fmt_out = AX_DISP_OUT_FMT_RGB666;
	}
	ax_lvds->clock = clk;

	DRM_DEBUG_DRIVER("done, type: %d, fmt_out: %d\n", ax_mode->type,
			 ax_mode->fmt_out);
}

static const struct drm_encoder_helper_funcs ax_lvds_encoder_helper_funcs = {
	.atomic_check = ax_lvds_encoder_atomic_check,
	.disable = ax_lvds_encoder_disable,
	.enable = ax_lvds_encoder_enable,
	.mode_set = ax_lvds_encoder_mode_set,
};

static const struct drm_encoder_funcs ax_lvds_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static void ax_lvds_bridge_enable(struct drm_bridge *bridge)
{
	struct ax_lvds *ax_lvds = bridge_to_ax_lvds(bridge);

	/* lvdstx reg config */
	if (LVDS_VESA_24 == ax_lvds->fmt_out) {
		writel(LVDS_RGB_MODE_VAL_RGB888, ax_lvds->regs + LVDS_RGB_MODE_REG);
		writel(LVDS_MODE_VAL_VESA, ax_lvds->regs + LVDS_MODE_REG);
	} else if (LVDS_JEIDA_24 == ax_lvds->fmt_out) {
		writel(LVDS_RGB_MODE_VAL_RGB888, ax_lvds->regs + LVDS_RGB_MODE_REG);
		writel(LVDS_MODE_VAL_JEIDA, ax_lvds->regs + LVDS_MODE_REG);
	} else if (LVDS_VESA_18 == ax_lvds->fmt_out) {
		writel(LVDS_RGB_MODE_VAL_RGB666, ax_lvds->regs + LVDS_RGB_MODE_REG);
		writel(LVDS_MODE_VAL_VESA, ax_lvds->regs + LVDS_MODE_REG);
	} else if (LVDS_JEIDA_18 == ax_lvds->fmt_out) {
		writel(LVDS_RGB_MODE_VAL_RGB666, ax_lvds->regs + LVDS_RGB_MODE_REG);
		writel(LVDS_MODE_VAL_JEIDA, ax_lvds->regs + LVDS_MODE_REG);
	}

	writel(1,  ax_lvds->regs + LVDS_SHADOW_REG);
}

static const struct drm_bridge_funcs ax_lvds_bridge_funcs = {
	.enable	      = ax_lvds_bridge_enable,
};

static int ax_lvds_parse_dt(struct device *dev, struct ax_lvds *ax_lvds)
{
	int ret = 0;

	ax_lvds->lvds_prst_ctrl = devm_reset_control_get_optional(dev, "lvds_p_rst");
	if (IS_ERR(ax_lvds->lvds_prst_ctrl)) {
		DRM_ERROR("get lvds prst_ctrl control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->lvds_prst_ctrl);
		goto exit;
	}

	ax_lvds->dphytx_pll_rst_ctrl = devm_reset_control_get_optional(dev, "dphytx_pll_rst");
	if (IS_ERR(ax_lvds->dphytx_pll_rst_ctrl)) {
		DRM_ERROR("get dphytx pll rst ctrl control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->dphytx_pll_rst_ctrl);
		goto exit;
	}

	ax_lvds->dphytx_pll_div7_rst_ctrl = devm_reset_control_get_optional(dev, "dphytx_pll_div7_rst");
	if (IS_ERR(ax_lvds->dphytx_pll_div7_rst_ctrl)) {
		DRM_ERROR("get dphytx pll div7 rst ctrl control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->dphytx_pll_div7_rst_ctrl);
		goto exit;
	}

	ax_lvds->lvds_p_clk = devm_clk_get(dev, "lvds_p_clk");
	if (IS_ERR(ax_lvds->lvds_p_clk)) {
		DRM_ERROR("get lvds p clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->lvds_p_clk);
		ax_lvds->lvds_p_clk = NULL;
		goto exit;
	}

	ax_lvds->dphytx_pll_clk = devm_clk_get(dev, "dphytx_pll_clk");
	if (IS_ERR(ax_lvds->dphytx_pll_clk)) {
		DRM_ERROR("get dphy pll clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->dphytx_pll_clk);
		ax_lvds->dphytx_pll_clk = NULL;
		goto exit;
	}

	ax_lvds->dphytx_pll_div7_clk = devm_clk_get(dev, "dphytx_pll_div7_clk");
	if (IS_ERR(ax_lvds->dphytx_pll_div7_clk)) {
		DRM_ERROR("get dphytx pll div7 clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->dphytx_pll_div7_clk);
		ax_lvds->dphytx_pll_div7_clk = NULL;
		goto exit;
	}

	ax_lvds->dphytx_ref_clk = devm_clk_get(dev, "dphytx_ref_clk");
	if (IS_ERR(ax_lvds->dphytx_ref_clk)) {
		DRM_ERROR("get dphytx ref clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->dphytx_ref_clk);
		ax_lvds->dphytx_ref_clk = NULL;
		goto exit;
	}

	ax_lvds->comm_dphytx_tlb_clk = devm_clk_get(dev, "comm_dphytx_tlb_clk");
	if (IS_ERR(ax_lvds->comm_dphytx_tlb_clk)) {
		DRM_ERROR("get common dphytx tlb clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(ax_lvds->comm_dphytx_tlb_clk);
		ax_lvds->comm_dphytx_tlb_clk = NULL;
		goto exit;
	}

exit:
	return ret;
}

static int ax_lvds_bind(struct device *dev, struct device *master, void *data)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm_dev = (struct drm_device *)data;
	struct ax_lvds *ax_lvds;
	struct drm_encoder *encoder = NULL;
	struct drm_connector *connector = NULL;
	struct device_node *np;
	struct drm_panel *panel;
	struct resource *res;
	const char *mapping;

	if (!dev || !dev->of_node) {
		DRM_ERROR("can't find of_node\n");
		return -ENODEV;
	}

	ax_lvds = devm_kzalloc(dev, sizeof(*ax_lvds), GFP_KERNEL);
	if (!ax_lvds) {
		DRM_ERROR("alloc lvds failed\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ax_lvds->regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(ax_lvds->regs))
		return PTR_ERR(ax_lvds->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ax_lvds->dphytx_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(ax_lvds->dphytx_regs))
		return PTR_ERR(ax_lvds->dphytx_regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	ax_lvds->comm_sys_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(ax_lvds->comm_sys_regs))
		return PTR_ERR(ax_lvds->comm_sys_regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	ax_lvds->dispc_sys_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(ax_lvds->dispc_sys_regs))
		return PTR_ERR(ax_lvds->dispc_sys_regs);

	ret = ax_lvds_parse_dt(dev, ax_lvds);
	if (ret)
		goto err0;

	reset_control_deassert(ax_lvds->lvds_prst_ctrl);
	reset_control_deassert(ax_lvds->dphytx_pll_rst_ctrl);
	reset_control_deassert(ax_lvds->dphytx_pll_div7_rst_ctrl);

	encoder = &ax_lvds->encoder;

	/* Encoder */
	drm_encoder_helper_add(encoder, &ax_lvds_encoder_helper_funcs);
	ret = drm_encoder_init(drm_dev, encoder, &ax_lvds_encoder_funcs, DRM_MODE_ENCODER_LVDS, "AX_LVDS");
	if (ret) {
		DRM_ERROR("lvds encoder init failed, ret = %d\n", ret);
		goto err0;
	}

	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm_dev, dev->of_node);
	if (encoder->possible_crtcs == 0) {
		DRM_ERROR("can not find crtcs for encoder\n");
		ret = -EPROBE_DEFER;
		goto err0;
	}
	/* Connector */
	connector = &ax_lvds->connector;
	drm_connector_helper_add(connector, &ax_lvds_connector_helper_funcs);
	ret = drm_connector_init(drm_dev, connector, &ax_lvds_connector_funcs,
				 DRM_MODE_CONNECTOR_LVDS);
	if (ret) {
		DRM_ERROR("connector init failed, ret = %d\n", ret);
		goto err1;
	}

	ret = drm_connector_attach_encoder(connector, encoder);
	if (ret) {
		drm_encoder_cleanup(encoder);
		DRM_ERROR("connector and encoder attach failed, ret = %d\n",
			  ret);
		goto err2;
	}

	DRM_INFO("[connector:%d:%s], [encoder:%d:%s]\n",
		 connector->base.id, connector->name,
		 encoder->base.id, encoder->name);

	/* panel_bridge bridge is attached to the panel's of_node.*/
	np = of_graph_get_remote_node(dev->of_node, 1, 0);
	if (!np) {
		DRM_ERROR("lvds panel node not found\n");
		goto err2;
	}

	panel = of_drm_find_panel(np);
	of_node_put(np);
	if (IS_ERR(panel)) {
		DRM_ERROR("lvds panel not found\n");
		goto err2;
	}

	ax_lvds->panel = panel;

	/* Bridge attached to our of_node for our user to look up.*/
	ax_lvds->bridge.funcs = &ax_lvds_bridge_funcs;
	ax_lvds->bridge.of_node = pdev->dev.of_node;

	drm_bridge_add(&ax_lvds->bridge);
	drm_bridge_attach(encoder, &ax_lvds->bridge, NULL);
	ret = drm_panel_attach(panel, connector);
	if (ret < 0){
		DRM_ERROR("lvds panel aattach connector fail\n");
		goto err2;
	}

	ret = of_property_read_string(panel->dev->of_node, "data-mapping", &mapping);
	if (ret < 0) {
		DRM_ERROR("%pOF: invalid or missing %s property\n",
			mapping, "data-mapping");
	}

	if (!strcmp(mapping, "jeida-18")) {
		ax_lvds->fmt_out = LVDS_JEIDA_18;
	} else if (!strcmp(mapping, "jeida-24")) {
		ax_lvds->fmt_out = LVDS_JEIDA_24;
	} else if (!strcmp(mapping, "vesa-24")) {
		ax_lvds->fmt_out = LVDS_VESA_24;
	} else {
		ax_lvds->fmt_out = LVDS_VESA_24;
	}

	dev_set_drvdata(dev, ax_lvds);

	return 0;

err2:
	drm_connector_cleanup(connector);
err1:
	drm_encoder_cleanup(encoder);

err0:
	DRM_ERROR("lvds bind failed, ret = %d\n", ret);

	return ret;
}

static void ax_lvds_unbind(struct device *dev, struct device *master,
			     void *data)
{
	struct ax_lvds *ax_lvds;

	ax_lvds = (struct ax_lvds *)dev_get_drvdata(dev);

	DRM_INFO("lvds unbind\n");

	if (ax_lvds->panel)
		drm_panel_detach(ax_lvds->panel);

	reset_control_assert(ax_lvds->lvds_prst_ctrl);
	reset_control_assert(ax_lvds->dphytx_pll_rst_ctrl);
	reset_control_assert(ax_lvds->dphytx_pll_div7_rst_ctrl);

	drm_encoder_cleanup(&ax_lvds->encoder);
	drm_bridge_remove(&ax_lvds->bridge);
}

const struct component_ops ax_lvds_ops = {
	.bind = ax_lvds_bind,
	.unbind = ax_lvds_unbind,
};

static int ax_lvds_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	if (!dev->of_node) {
		DRM_ERROR("can't find bt-dpi device\n");
		return -ENODEV;
	}

	DRM_INFO("lvds probe, of_node: 0x%pOF\n", pdev->dev.of_node);

	return component_add(dev, &ax_lvds_ops);
}

static int ax_lvds_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	DRM_INFO("lvds remove\n");

	component_del(dev, &ax_lvds_ops);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ax_lvds_suspend(struct device *dev)
{
	struct ax_lvds *ax_lvds = (struct ax_lvds *)dev_get_drvdata(dev);
	if (ax_lvds->status) {
		lvds_dphy_clear(ax_lvds);
		ax_lvds_clk_unprepare(ax_lvds);
		reset_control_assert(ax_lvds->lvds_prst_ctrl);
		reset_control_assert(ax_lvds->dphytx_pll_rst_ctrl);
		reset_control_assert(ax_lvds->dphytx_pll_div7_rst_ctrl);
	}

	return 0;
}

static int ax_lvds_resume(struct device *dev)
{
	int ret;
	struct ax_lvds *ax_lvds = (struct ax_lvds *)dev_get_drvdata(dev);
	if (ax_lvds->status) {
		reset_control_deassert(ax_lvds->lvds_prst_ctrl);
		reset_control_deassert(ax_lvds->dphytx_pll_rst_ctrl);
		reset_control_deassert(ax_lvds->dphytx_pll_div7_rst_ctrl);

		ret = ax_lvds_clk_prepare(ax_lvds);
		if (ret)
			DRM_ERROR("lvds encoder clk prepare failed, ret = %d\n", ret);

		lvds_dphy_config(ax_lvds->clock, ax_lvds);

		writel(DISPC_SYSGLB_LVDS_CLK_SEL_BIT, ax_lvds->dispc_sys_regs + DISPC_SYSGLB_LVDS_CLK_SEL_CLR);
		ax_lvds_bridge_enable(&ax_lvds->bridge);
	}

	return 0;
}
#endif

static const struct of_device_id ax_lvds_drm_dt_ids[] = {
	{
	 .compatible = "axera,lvds",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, ax_lvds_drm_dt_ids);

static const struct dev_pm_ops ax_lvds_pm_ops = {
	.suspend_noirq = ax_lvds_suspend,
	.resume_noirq = ax_lvds_resume,
};

struct platform_driver ax_lvds_platform_driver = {
	.probe = ax_lvds_probe,
	.remove = ax_lvds_remove,
	.driver = {
		   .name = "ax-lvds-drv",
		   .of_match_table = of_match_ptr(ax_lvds_drm_dt_ids),
		   .pm = &ax_lvds_pm_ops,
		   },
};

MODULE_AUTHOR("Axera Inc.");
MODULE_DESCRIPTION("Axera lvds driver");
MODULE_LICENSE("GPL v2");
