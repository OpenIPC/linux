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
#include "ax_drm_virt_connector.h"

#define encoder_to_virt_connector(x) \
	container_of(x, struct ax_virt_connector, encoder)
#define connector_to_virt_connector(x) \
	container_of(x, struct ax_virt_connector, connector)

/* Connector */

static enum drm_connector_status ax_virt_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

int ax_virt_connector_atomic_set_property(struct drm_connector *connector,
					  struct drm_connector_state *state,
					  struct drm_property *property,
					  uint64_t val)
{
	struct ax_virt_connector *virt_connector =
	    connector_to_virt_connector(connector);

	if (property == virt_connector->props.intf_type)
		virt_connector->intf_type = val;
	else if (property == virt_connector->props.fmt_out)
		virt_connector->fmt_out = val;

	return 0;
}

int ax_virt_connector_atomic_get_property(struct drm_connector *connector, const struct drm_connector_state
					  *state, struct drm_property *property,
					  uint64_t * val)
{
	struct ax_virt_connector *virt_connector =
	    connector_to_virt_connector(connector);

	if (property == virt_connector->props.intf_type)
		*val = virt_connector->intf_type;
	else if (property == virt_connector->props.fmt_out)
		*val = virt_connector->fmt_out;

	return 0;
}

static int ax_virt_connector_modes(struct drm_connector *connector, uint32_t maxX, uint32_t maxY)
{
	return 0;
}

static const struct drm_connector_funcs ax_virt_connector_funcs = {
	.detect = ax_virt_connector_detect,
	.fill_modes = ax_virt_connector_modes,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.atomic_set_property = ax_virt_connector_atomic_set_property,
	.atomic_get_property = ax_virt_connector_atomic_get_property,
};

static int ax_virt_connector_get_modes(struct drm_connector *connector)
{
	struct ax_virt_connector *virt_connector;

	DRM_DEBUG_DRIVER("enter, [connector:%d:%s]\n", connector->base.id,
			 connector->name);

	virt_connector = connector_to_virt_connector(connector);

	return drm_panel_get_modes(virt_connector->panel);
}

static const struct drm_connector_helper_funcs ax_virt_connector_helper_funcs = {
	.get_modes = ax_virt_connector_get_modes,
};

/* Encoder */

static void ax_virt_connector_clk_unprepare(struct ax_virt_connector *virt_connector)
{
	clk_disable_unprepare(virt_connector->common_1x_clk);
	clk_disable_unprepare(virt_connector->common_nx_clk);

	if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
		clk_disable_unprepare(virt_connector->flash_1x_clk);
		clk_disable_unprepare(virt_connector->flash_nx_clk);
	}
}

static int ax_virt_connector_clk_prepare(struct ax_virt_connector *virt_connector)
{
	int ret = 0;

	ret = clk_prepare_enable(virt_connector->common_1x_clk);
	if (ret) {
		DRM_ERROR("enable virt connector common 1x clk failed\n");
		goto exit;
	}

	ret = clk_prepare_enable(virt_connector->common_nx_clk);
	if (ret) {
		DRM_ERROR("enable virt connector common nx clk failed\n");
		goto exit;
	}

	if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
		ret = clk_prepare_enable(virt_connector->flash_1x_clk);
		if (ret) {
			DRM_ERROR("enable virt connector flash 1x clk failed\n");
			goto exit;
		}

		ret = clk_prepare_enable(virt_connector->flash_nx_clk);
		if (ret) {
			DRM_ERROR("enable virt connector flash nx clk failed\n");
			goto exit;
		}
	}

exit:
	if (ret)
		ax_virt_connector_clk_unprepare(virt_connector);

	return ret;
}

static int ax_virt_encoder_atomic_check(struct drm_encoder *encoder,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *conn_state)
{
	struct ax_virt_connector *virt_connector =
	    encoder_to_virt_connector(encoder);
	struct drm_crtc *crtc = crtc_state->crtc;

	DRM_DEBUG_DRIVER("enter, bind crtc%d\n", drm_crtc_index(crtc));

	virt_connector->crtc = crtc_state->crtc;

	return 0;
}

static void ax_virt_encoder_disable(struct drm_encoder *encoder)
{
	int ret;
	struct ax_virt_connector *virt_connector = encoder_to_virt_connector(encoder);

	DRM_DEBUG_DRIVER("enter, [encoder:%d:%s]\n", encoder->base.id,
			 encoder->name);

	if (!IS_ERR(virt_connector->panel)) {
		ret = drm_panel_disable(virt_connector->panel);
		if (ret)
			DRM_ERROR("pannel disable failed, ret = %d\n", ret);

		ret = drm_panel_unprepare(virt_connector->panel);
		if (ret)
			DRM_ERROR("pannel unprepare failed, ret = %d\n", ret);
	}

	ax_virt_connector_clk_unprepare(virt_connector);
	virt_connector->state = VIRT_STATUS_DISABLED;
}

static void ax_virt_encoder_enable(struct drm_encoder *encoder)
{
	int ret;
	struct ax_virt_connector *virt_connector = encoder_to_virt_connector(encoder);

	DRM_DEBUG_DRIVER("enter, [encoder:%d:%s]\n", encoder->base.id,
			 encoder->name);

	if (!IS_ERR(virt_connector->panel)) {
		ret = drm_panel_prepare(virt_connector->panel);
		if (ret)
			DRM_ERROR("pannel prepare failed, ret = %d\n", ret);

		ret = drm_panel_enable(virt_connector->panel);
		if (ret)
			DRM_ERROR("pannel enable failed, ret = %d\n", ret);
	}
	virt_connector->state = VIRT_STATUS_ENABLED;
}

static void ax_virt_encoder_mode_set(struct drm_encoder *encoder,
				     struct drm_display_mode *mode,
				     struct drm_display_mode *adjusted_mode)
{
	struct ax_virt_connector *virt_connector =
	    encoder_to_virt_connector(encoder);
	struct ax_crtc *ax_crtc = to_ax_crtc(virt_connector->crtc);
	struct ax_disp_mode *ax_mode = &ax_crtc->mode;
	int ret;
	int clk;

	DRM_DEBUG_DRIVER("enter, [encoder:%d:%s]\n", encoder->base.id,
			 encoder->name);
	DRM_DEBUG_DRIVER("mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(mode));
	DRM_DEBUG_DRIVER("adjusted_mode: " DRM_MODE_FMT "\n",
			 DRM_MODE_ARG(adjusted_mode));

	if (adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE)
		ax_mode->flags |= MODE_FLAG_INTERLACE;
	else
		ax_mode->flags &= ~(MODE_FLAG_INTERLACE);

	if (virt_connector->bt_8bit_low)
		ax_mode->flags |= MODE_FLAG_BT_8BIT_LOW;

	if (virt_connector->connector.display_info.
	    bus_flags & DRM_BUS_FLAG_DE_LOW)
		ax_mode->de_pol = 1;
	else
		ax_mode->de_pol = 0;

	ax_mode->type = virt_connector->intf_type;
	ax_mode->fmt_out = virt_connector->fmt_out;
	DRM_INFO("virt_connector->id:%d dmux_sel:%d\n",virt_connector->id,virt_connector->dmux_sel);
	if (virt_connector->id == 0) {
		writel(COMM_SYSGLB_LCD_VOMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_CLR);
		writel(1, virt_connector->dispsys_glb_regs + DISPC_SYSGLB_LVDS_CLK_SEL_SET);
	} else if (virt_connector->id == 1) {
		if (virt_connector->dmux_sel == 1) {
			writel(COMM_SYSGLB_LCD_VOMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
			writel(COMM_SYSGLB_DPULITE_DMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
		}
		else {
			writel(COMM_SYSGLB_DPULITE_DMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_CLR);
			writel(COMM_SYSGLB_DPULITE_TX_CLKING_MODE, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
		}
		writel(COMM_SYSGLB_DPULITE_DPHYTX_EN, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
	}
	writel(FLASH_SYSGLB_IMAGE_TX_EN, virt_connector->flashsys_glb_regs + FLASH_SYSGLB_IMAGE_TX_SET);

	clk = adjusted_mode->clock * 1000;

	if (virt_connector->intf_type <= AX_DISP_OUT_MODE_BT1120){
		writel((1<<5), virt_connector->flashsys_glb_regs + FLASH_SYSGLB_IMAGE_TX_SET);
		if (adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE) {
			if (virt_connector->intf_type == AX_DISP_OUT_MODE_BT1120)
				clk /= 2;

		} else if (virt_connector->intf_type == AX_DISP_OUT_MODE_BT601 || virt_connector->intf_type == AX_DISP_OUT_MODE_BT656) {
			clk *= 2;
		}
	}

	if (virt_connector->common_1x_clk) {
		ret = clk_set_rate(virt_connector->common_1x_clk, clk);
		if (ret < 0)
			DRM_ERROR("virt connector common 1x clk clk_set_rate %d failed: %d", clk, ret);
	}

	if (virt_connector->common_nx_clk) {
		ret = clk_set_rate(virt_connector->common_nx_clk, clk);
		if (ret < 0)
			DRM_ERROR("virt connector common nx clk clk_set_rate %d failed: %d", clk, ret);
	}

	if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
		if (virt_connector->flash_1x_clk) {
			ret = clk_set_rate(virt_connector->flash_1x_clk, clk);
			if (ret < 0)
				DRM_ERROR("virt connector flash 1x clk clk_set_rate %d failed: %d", clk, ret);
		}

		if (virt_connector->flash_nx_clk) {
			ret = clk_set_rate(virt_connector->flash_nx_clk, clk);
			if (ret < 0)
				DRM_ERROR("virt connector flash nx clk clk_set_rate %d failed: %d", clk, ret);
		}
	}

	ret = ax_virt_connector_clk_prepare(virt_connector);
	if (ret)
		DRM_ERROR("virt encoder prepare failed, ret = %d\n", ret);

	if (virt_connector->common_1x_clk) {
		ret = clk_set_rate(virt_connector->common_1x_clk, clk);
		if (ret < 0)
			DRM_ERROR("virt connector common 1x clk clk_set_rate %d failed: %d", clk, ret);
	}

	if (virt_connector->common_nx_clk) {
		ret = clk_set_rate(virt_connector->common_nx_clk, clk);
		if (ret < 0)
			DRM_ERROR("virt connector common nx clk clk_set_rate %d failed: %d", clk, ret);
	}

	if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
		if (virt_connector->flash_1x_clk) {
			ret = clk_set_rate(virt_connector->flash_1x_clk, clk);
			if (ret < 0)
				DRM_ERROR("virt connector flash 1x clk clk_set_rate %d failed: %d", clk, ret);
		}

		if (virt_connector->flash_nx_clk) {
			ret = clk_set_rate(virt_connector->flash_nx_clk, clk);
			if (ret < 0)
				DRM_ERROR("virt connector flash nx clk clk_set_rate %d failed: %d", clk, ret);
		}
	}

	virt_connector->clock = clk;

	DRM_DEBUG_DRIVER("comm 1x clk: %ld  comm 1x clk:%ld  flash 1x clk:%ld flash nx clk:%ld\n",
			 clk_get_rate(virt_connector->common_1x_clk),
			 clk_get_rate(virt_connector->common_nx_clk),
			 clk_get_rate(virt_connector->flash_1x_clk),
			 clk_get_rate(virt_connector->flash_nx_clk));

	DRM_DEBUG_DRIVER("done, type: %d, fmt_out: %d\n", ax_mode->type,
			 ax_mode->fmt_out);
}

static const struct drm_encoder_helper_funcs ax_misc_encoder_helper_funcs = {
	.atomic_check = ax_virt_encoder_atomic_check,
	.disable = ax_virt_encoder_disable,
	.enable = ax_virt_encoder_enable,
	.mode_set = ax_virt_encoder_mode_set,
	.atomic_check = ax_virt_encoder_atomic_check,
};

static void ax_virt_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs ax_virt_encoder_funcs = {
	.destroy = ax_virt_encoder_destroy,
};

static int ax_misc_panel_find(struct device *dev,
			      struct ax_virt_connector *virt_connector)
{
	int ret = 0;
	struct drm_panel *panel = NULL;
	struct device_node *port = NULL, *ep = NULL, *panel_node = NULL;

	port = of_get_next_child(dev->of_node, NULL);
	if (!port) {
		DRM_ERROR("no port for np\n");
		ret = -ENODEV;
		goto exit;
	}

	ep = of_get_next_child(port, NULL);
	if (!ep) {
		DRM_ERROR("no endpoint for port\n");
		ret = -ENODEV;
		goto exit;
	}

	DRM_INFO("port: %pOF, ep: %pOF\n", port, ep);

	panel_node = of_graph_get_remote_port_parent(ep);
	if (!panel_node) {
		DRM_ERROR("no remote endpoint for port\n");
		ret = -ENODEV;
		goto exit;
	}

	panel = of_drm_find_panel(panel_node);
	if (IS_ERR(panel)) {
		DRM_WARN("misc panel not found\n");
		ret = 0;
		goto exit;
	}

	DRM_INFO("misc panel found\n");

	virt_connector->panel = panel;

exit:
	of_node_put(ep);
	of_node_put(port);
	of_node_put(panel_node);

	DRM_INFO("misc panel find done, ret = %d\n", ret);

	return ret;
}

static int ax_virt_parse_dt(struct device *dev, struct ax_virt_connector *virt_connector)
{
	int ret = 0;
	u32 id;
	u32 dmux_sel;

	virt_connector->common_1x_rst_ctrl = devm_reset_control_get_optional(dev, "cm_dpu_1x_rst");
	if (IS_ERR(virt_connector->common_1x_rst_ctrl)) {
		DRM_ERROR("get virt common 1x rst control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->common_1x_rst_ctrl);
		goto exit;
	}

	virt_connector->common_nx_rst_ctrl = devm_reset_control_get_optional(dev, "cm_dpu_nx_rst");
	if (IS_ERR(virt_connector->common_nx_rst_ctrl)) {
		DRM_ERROR("get virt common nx rst control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->common_nx_rst_ctrl);
		goto exit;
	}

	virt_connector->flash_1x_rst_ctrl = devm_reset_control_get_optional(dev, "flash_dpu_1x_rst");
	if (IS_ERR(virt_connector->flash_1x_rst_ctrl)) {
		DRM_ERROR("get virt flash 1x rst control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->flash_1x_rst_ctrl);
		goto exit;
	}

	virt_connector->flash_nx_rst_ctrl = devm_reset_control_get_optional(dev, "flash_dpu_nx_rst");
	if (IS_ERR(virt_connector->flash_nx_rst_ctrl)) {
		DRM_ERROR("get virt flash nx rst control failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->flash_nx_rst_ctrl);
		goto exit;
	}

	virt_connector->common_1x_clk = devm_clk_get(dev, "comm_clk_1x");
	if (IS_ERR(virt_connector->common_1x_clk)) {
		DRM_ERROR("get virt common 1x clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->common_1x_clk);
		virt_connector->common_1x_clk = NULL;
		goto exit;
	}

	virt_connector->common_nx_clk = devm_clk_get(dev, "comm_clk_nx");
	if (IS_ERR(virt_connector->common_nx_clk)) {
		DRM_ERROR("get virt common nx clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->common_nx_clk);
		virt_connector->common_nx_clk = NULL;
		goto exit;
	}

	virt_connector->flash_1x_clk = devm_clk_get(dev, "flash_clk_1x");
	if (IS_ERR(virt_connector->flash_1x_clk)) {
		DRM_ERROR("get virt flash 1x clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->flash_1x_clk);
		virt_connector->flash_1x_clk = NULL;
		goto exit;
	}

	virt_connector->flash_nx_clk = devm_clk_get(dev, "flash_clk_nx");
	if (IS_ERR(virt_connector->flash_nx_clk)) {
		DRM_ERROR("get virt flash nx clk failed, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(virt_connector->flash_nx_clk);
		virt_connector->flash_nx_clk = NULL;
		goto exit;
	}

	virt_connector->bt_8bit_low = of_property_read_bool(dev->of_node, "bt-8bit-low");
	if (virt_connector->bt_8bit_low)
		DRM_INFO("virt_connector select bt lower 8bit transmission.\n");

	ret = of_property_read_u32(dev->of_node, "id", &id);
	if (ret) {
		DRM_ERROR("vo id not defined\n");
		goto exit;
	}

	virt_connector->id = id;
	if (id == 1) {
		ret = of_property_read_u32(dev->of_node, "dmux-sel", &dmux_sel);
		if (ret) {
			DRM_ERROR("vo dmux-sel not defined\n");
			goto exit;
		}
		virt_connector->dmux_sel = dmux_sel;
	}
exit:
	return ret;

}

static int ax_virt_connector_create(struct device *dev, void *data)
{
	int ret = 0;
	int connector_type, encoder_type;
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm_dev = (struct drm_device *)data;
	struct ax_virt_connector *virt_connector;
	struct drm_connector *connector = NULL;
	struct drm_encoder *encoder = NULL;
	struct resource *res;

	DRM_INFO("virt connector create enter\n");

	if (!dev || !dev->of_node) {
		DRM_ERROR("can't find of_node\n");
		return -ENODEV;
	}

	virt_connector = devm_kzalloc(dev, sizeof(*virt_connector), GFP_KERNEL);
	if (!virt_connector) {
		DRM_ERROR("alloc misc connector failed\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, virt_connector);

	ret = ax_misc_panel_find(dev, virt_connector);
	if (ret || !virt_connector->panel)
		goto err0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	virt_connector->common_glb_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(virt_connector->common_glb_regs))
		goto err0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	virt_connector->flashsys_glb_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(virt_connector->flashsys_glb_regs))
		goto err0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	virt_connector->dispsys_glb_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(virt_connector->dispsys_glb_regs))
		goto err0;


	ret = ax_virt_parse_dt(dev, virt_connector);
	if (ret)
		goto err0;

	reset_control_deassert(virt_connector->common_1x_rst_ctrl);
	reset_control_deassert(virt_connector->common_nx_rst_ctrl);
	reset_control_deassert(virt_connector->flash_1x_rst_ctrl);
	reset_control_deassert(virt_connector->flash_nx_rst_ctrl);

	connector_type = DRM_MODE_CONNECTOR_VIRTUAL;
	encoder_type = DRM_MODE_ENCODER_VIRTUAL;

	virt_connector->connector_type = connector_type;
	virt_connector->encoder_type = encoder_type;
	connector = &virt_connector->connector;
	encoder = &virt_connector->encoder;

	/* Encoder */
	drm_encoder_helper_add(encoder, &ax_misc_encoder_helper_funcs);
	ret = drm_encoder_init(drm_dev, encoder, &ax_virt_encoder_funcs, encoder_type, "AX_VIRT");
	if (ret) {
		DRM_ERROR("encoder init failed, ret = %d\n", ret);
		goto err0;
	}

	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm_dev, dev->of_node);
	if (encoder->possible_crtcs == 0) {
		DRM_ERROR("can not find crtcs for encoder\n");
		ret = -EPROBE_DEFER;
		goto err0;
	}

	/* Connector */
	connector->interlace_allowed = true;
	drm_connector_helper_add(connector, &ax_virt_connector_helper_funcs);
	ret = drm_connector_init(drm_dev, connector, &ax_virt_connector_funcs,
				 connector_type);
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

	ret = drm_panel_attach(virt_connector->panel, connector);
	if (ret) {
		DRM_ERROR("panel attach failed, ret = %d\n", ret);
		goto err2;
	}

	virt_connector->props.intf_type = drm_property_create_range(drm_dev,
								    DRM_MODE_PROP_ATOMIC,
								    "INTF_TYPE",
								    AX_DISP_OUT_MODE_BT601,
								    AX_DISP_OUT_MODE_DPI);
	if (ret) {
		DRM_ERROR("failed to create intf_type property\n");
		goto err2;
	}

	drm_object_attach_property(&connector->base,
				   virt_connector->props.intf_type,
				   AX_DISP_OUT_MODE_DPI);

	virt_connector->intf_type = AX_DISP_OUT_MODE_DPI;

	virt_connector->props.fmt_out = drm_property_create_range(drm_dev,
								  DRM_MODE_PROP_ATOMIC,
								  "FMT_OUT",
								  AX_DISP_OUT_FMT_RGB565,
								  AX_DISP_OUT_FMT_YUV422);
	if (ret) {
		DRM_ERROR("failed to create fmt_out property\n");
		goto err2;
	}

	drm_object_attach_property(&connector->base,
				   virt_connector->props.fmt_out,
				   AX_DISP_OUT_FMT_RGB565);

	virt_connector->fmt_out = AX_DISP_OUT_FMT_RGB565;

	virt_connector->state = VIRT_STATUS_CREATED;

	DRM_INFO("virt connector create done\n");

	return 0;

err2:
	drm_connector_cleanup(connector);
err1:
	drm_encoder_cleanup(encoder);
err0:
	return ret;
}

static void ax_virt_connector_destroy(struct ax_virt_connector *virt_connector)
{
	if ((virt_connector->state >= VIRT_STATUS_CREATED) && (virt_connector->state < VIRT_STATUS_DESTORY)) {
		if (virt_connector->panel)
			drm_panel_detach(virt_connector->panel);
		drm_connector_cleanup(&virt_connector->connector);
		drm_encoder_cleanup(&virt_connector->encoder);
	}
	reset_control_assert(virt_connector->common_1x_rst_ctrl);
	reset_control_assert(virt_connector->common_nx_rst_ctrl);
	reset_control_assert(virt_connector->flash_1x_rst_ctrl);
	reset_control_assert(virt_connector->flash_1x_rst_ctrl);
	virt_connector->state = VIRT_STATUS_DESTORY;
}

static int ax_bt_dpi_bind(struct device *dev, struct device *master, void *data)
{
	int ret;

	DRM_INFO("bt dpi bind\n");

	ret = ax_virt_connector_create(dev, data);
	if (ret) {
		DRM_ERROR("failed to create virt connector, ret = %d\n", ret);
		goto err0;
	}

	return 0;

err0:
	DRM_ERROR("bt-dpi bind failed, ret = %d\n", ret);

	return ret;
}

static void ax_bt_dpi_unbind(struct device *dev, struct device *master,
			     void *data)
{
	struct ax_virt_connector *virt_connector;

	virt_connector = (struct ax_virt_connector *)dev_get_drvdata(dev);

	ax_virt_connector_destroy(virt_connector);

	DRM_INFO("bt dpi unbind\n");
}

const struct component_ops bt_dpi_component_ops = {
	.bind = ax_bt_dpi_bind,
	.unbind = ax_bt_dpi_unbind,
};

static int ax_bt_dpi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	if (!dev->of_node) {
		DRM_ERROR("can't find bt-dpi device\n");
		return -ENODEV;
	}

	DRM_INFO("bt dpi probe, of_node: 0x%pOF\n", pdev->dev.of_node);

	return component_add(dev, &bt_dpi_component_ops);
}

static int ax_bt_dpi_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	DRM_INFO("bt dpi remove\n");

	component_del(dev, &bt_dpi_component_ops);

	return 0;
}

static const struct of_device_id bt_dpi_drm_dt_ids[] = {
	{
	 .compatible = "axera,bt-dpi",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, bt_dpi_drm_dt_ids);

#ifdef CONFIG_PM_SLEEP
static int ax_bt_dpi_suspend(struct device *dev)
{
	struct ax_virt_connector *virt_connector = (struct ax_virt_connector *)dev_get_drvdata(dev);
	if (virt_connector->state == VIRT_STATUS_ENABLED) {
		ax_virt_connector_clk_unprepare(virt_connector);
		reset_control_assert(virt_connector->common_1x_rst_ctrl);
		reset_control_assert(virt_connector->common_nx_rst_ctrl);
		reset_control_assert(virt_connector->flash_1x_rst_ctrl);
		reset_control_assert(virt_connector->flash_1x_rst_ctrl);
	}

	DRM_INFO("ax_bt_dpi_suspend\n");
	return 0;
}

static int ax_bt_dpi_resume(struct device *dev)
{
	int ret;
	struct ax_virt_connector *virt_connector = (struct ax_virt_connector *)dev_get_drvdata(dev);
	if (virt_connector->state == VIRT_STATUS_ENABLED) {
		reset_control_deassert(virt_connector->common_1x_rst_ctrl);
		reset_control_deassert(virt_connector->common_nx_rst_ctrl);
		reset_control_deassert(virt_connector->flash_1x_rst_ctrl);
		reset_control_deassert(virt_connector->flash_nx_rst_ctrl);

		if (virt_connector->id == 0) {
			writel(COMM_SYSGLB_LCD_VOMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_CLR);
			writel(1, virt_connector->dispsys_glb_regs + DISPC_SYSGLB_LVDS_CLK_SEL_SET);
		} else if (virt_connector->id == 1) {
			if (virt_connector->dmux_sel == 1) {
				writel(COMM_SYSGLB_LCD_VOMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
				writel(COMM_SYSGLB_DPULITE_DMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
			}
			else {
				writel(COMM_SYSGLB_DPULITE_DMUX_SEL, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_CLR);
				writel(COMM_SYSGLB_DPULITE_TX_CLKING_MODE, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
			}
			writel(COMM_SYSGLB_DPULITE_DPHYTX_EN, virt_connector->common_glb_regs + COMM_SYSGLB_VO_CFG_SET);
		}
		writel(FLASH_SYSGLB_IMAGE_TX_EN, virt_connector->flashsys_glb_regs + FLASH_SYSGLB_IMAGE_TX_SET);

		if (virt_connector->common_1x_clk) {
			ret = clk_set_rate(virt_connector->common_1x_clk, virt_connector->clock);
			if (ret < 0)
				DRM_ERROR("virt connector common 1x clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
		}

		if (virt_connector->common_nx_clk) {
			ret = clk_set_rate(virt_connector->common_nx_clk, virt_connector->clock);
			if (ret < 0)
				DRM_ERROR("virt connector common nx clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
		}

		if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
			if (virt_connector->flash_1x_clk) {
				ret = clk_set_rate(virt_connector->flash_1x_clk, virt_connector->clock);
				if (ret < 0)
					DRM_ERROR("virt connector flash 1x clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
			}

			if (virt_connector->flash_nx_clk) {
				ret = clk_set_rate(virt_connector->flash_nx_clk, virt_connector->clock);
				if (ret < 0)
					DRM_ERROR("virt connector flash nx clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
			}
		}

		ret = ax_virt_connector_clk_prepare(virt_connector);
		if (ret)
			DRM_ERROR("virt encoder prepare failed, ret = %d\n", ret);

		if (virt_connector->common_1x_clk) {
			ret = clk_set_rate(virt_connector->common_1x_clk, virt_connector->clock);
			if (ret < 0)
				DRM_ERROR("virt connector common 1x clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
		}

		if (virt_connector->common_nx_clk) {
			ret = clk_set_rate(virt_connector->common_nx_clk, virt_connector->clock);
			if (ret < 0)
				DRM_ERROR("virt connector common nx clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
		}

		if (virt_connector->id == 0 || (virt_connector->id == 1 && virt_connector->dmux_sel == 1)) {
			if (virt_connector->flash_1x_clk) {
				ret = clk_set_rate(virt_connector->flash_1x_clk, virt_connector->clock);
				if (ret < 0)
					DRM_ERROR("virt connector flash 1x clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
			}

			if (virt_connector->flash_nx_clk) {
				ret = clk_set_rate(virt_connector->flash_nx_clk, virt_connector->clock);
				if (ret < 0)
					DRM_ERROR("virt connector flash nx clk clk_set_rate %d failed: %d", virt_connector->clock, ret);
			}
		}
	}

	DRM_INFO("lvds ax_bt_dpi_resume\n");

	return 0;
}
#endif

static const struct dev_pm_ops ax_bt_dpi_pm_ops = {
	.suspend_noirq = ax_bt_dpi_suspend,
	.resume_noirq = ax_bt_dpi_resume,
};

struct platform_driver bt_dpi_platform_driver = {
	.probe = ax_bt_dpi_probe,
	.remove = ax_bt_dpi_remove,
	.driver = {
		   .name = "ax-bt-dpi-drv",
		   .of_match_table = of_match_ptr(bt_dpi_drm_dt_ids),
		   .pm = &ax_bt_dpi_pm_ops,
		   },
};

MODULE_DESCRIPTION("axera dpi driver");
MODULE_LICENSE("GPL v2");
