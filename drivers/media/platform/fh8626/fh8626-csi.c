// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 MIPI CSI-2 receiver and D-PHY subdevice driver
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kref.h>
#include <media/fh8626-subdev.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#include "fh8626-csi-regs.h"
#include "fh8626-pipeline.h"

#ifndef MEDIA_ENT_F_VID_IF_BRIDGE
#define MEDIA_ENT_F_VID_IF_BRIDGE	MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN
#endif

struct fh8626_csi_dev {
	struct device		*dev;
	struct v4l2_subdev	sd;
#if defined(CONFIG_MEDIA_CONTROLLER)
	struct media_pad	pads[2];	/* 0: sink, 1: source */
#endif

	void __iomem		*csi_base;
	void __iomem		*wrap_base;

	struct clk		*dphy_clk;
	struct clk		*wrap_clk;
	struct clk		*pclk;
	struct phy		*dphy;

	struct v4l2_mbus_framefmt fmt;
	struct mutex		lock;
	struct kref		ref;
	struct fh8626_subdev_lifetime lifetime;
	u8			num_lanes;
	u8			frq_range;
	bool			streaming;
	bool			phy_initialized, phy_powered, clocks_enabled;
	bool			entity_initialized;
};

static inline struct fh8626_csi_dev *to_fh8626_csi(struct v4l2_subdev *sd)
{
	return container_of(sd, struct fh8626_csi_dev, sd);
}

static void fh8626_csi_free(struct kref *ref)
{
	struct fh8626_csi_dev *csi =
		container_of(ref, struct fh8626_csi_dev, ref);

#if defined(CONFIG_MEDIA_CONTROLLER)
	if (csi->entity_initialized)
		media_entity_cleanup(&csi->sd.entity);
#endif
	mutex_destroy(&csi->lock);
	kfree(csi);
}

static int fh8626_csi_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_fh8626_csi(sd)->ref);
	return 0;
}

static void fh8626_csi_release(struct v4l2_subdev *sd)
{
	kref_put(&to_fh8626_csi(sd)->ref, fh8626_csi_free);
}

static const struct v4l2_subdev_internal_ops fh8626_csi_internal_ops = {
	.registered = fh8626_csi_registered,
	.unregistered = fh8626_csi_release,
};

static void fh8626_csi_dphy_write(struct fh8626_csi_dev *csi, u32 test_code)
{
	writel(test_code, csi->csi_base + CSI_PHY_TEST_CTRL1);
	writel(2, csi->csi_base + CSI_PHY_TEST_CTRL0);
	writel(0, csi->csi_base + CSI_PHY_TEST_CTRL0);
}

static void fh8626_csi_init_hw(struct fh8626_csi_dev *csi)
{
	u32 wrap;

	/* Release the PHY test interface, then the D-PHY/controller resets */
	writel(0, csi->csi_base + CSI_PHY_TEST_CTRL1);
	writel(1, csi->csi_base + CSI_PHY_TEST_CTRL0);
	writel(1, csi->csi_base + CSI_PHY_SHUTDOWNZ);
	writel(1, csi->csi_base + CSI_DPHY_RSTZ);
	writel(0xffffffff, csi->csi_base + CSI_RESETN);
	writel(0, csi->csi_base + CSI_PHY_TEST_CTRL0);
	writel(0, csi->csi_base + CSI_PHY_TEST_CTRL0);

	fh8626_csi_dphy_write(csi, 0x10034);
	fh8626_csi_dphy_write(csi, 0x14);
	fh8626_csi_dphy_write(csi, 0x10044);
	/* Data rate range code, shifted into the test data field */
	fh8626_csi_dphy_write(csi, csi->frq_range << 1);

	usleep_range(100, 200);

	writel(1, csi->csi_base + CSI_N_LANES_OFFSET);
	writel(csi->num_lanes - 1, csi->csi_base + CSI_N_LANES_OFFSET);

	writel(0xffffffff, csi->csi_base + CSI_MASK_PHY_FATAL);
	writel(0xffffffff, csi->csi_base + CSI_MASK_PKT_FATAL);
	writel(0xffffffff, csi->csi_base + CSI_MASK_FRAME_FATAL);
	writel(0xffffffff, csi->csi_base + CSI_MASK_PHY);
	writel(0xffffffff, csi->csi_base + CSI_MASK_PKT);
	writel(0xffffffff, csi->csi_base + CSI_MASK_LINE);

	/* Zero in both fields selects linear RAW10. */
	wrap = readl(csi->wrap_base + WRAP_CTRL);
	writel(wrap & ~(WRAP_CTRL_MODE_MASK | WRAP_CTRL_RAW_TYPE),
	       csi->wrap_base + WRAP_CTRL);
	writel(1, csi->wrap_base + WRAP_INT_EN);
}

static int fh8626_csi_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct fh8626_csi_dev *csi = to_fh8626_csi(sd);
	int ret = 0, err;

	mutex_lock(&csi->lock);
	if (csi->streaming && enable)
		goto out;
	if (enable) {
		/*
		 * A failed stop leaves the PHY initialized; refuse to start
		 * again until it has been unwound.
		 */
		if (csi->phy_initialized) {
			ret = -EIO;
			goto out;
		}
		/* PHY control precedes the bus clocks and receiver setup */
		ret = phy_init(csi->dphy);
		if (ret)
			goto out;
		csi->phy_initialized = true;
		ret = clk_prepare_enable(csi->dphy_clk);
		if (ret)
			goto exit_phy;
		ret = clk_prepare_enable(csi->wrap_clk);
		if (ret)
			goto disable_dphy;
		ret = clk_prepare_enable(csi->pclk);
		if (ret)
			goto disable_wrap;
		csi->clocks_enabled = true;
		ret = phy_power_on(csi->dphy);
		if (ret)
			goto disable_pclk;
		csi->phy_powered = true;
		fh8626_csi_init_hw(csi);
		csi->streaming = true;
	} else {
		if (csi->streaming) {
			writel(0, csi->csi_base + CSI_PHY_SHUTDOWNZ);
			writel(0, csi->csi_base + CSI_DPHY_RSTZ);
			writel(0, csi->csi_base + CSI_RESETN);
			writel(0, csi->wrap_base + WRAP_INT_EN);
			writel(0, csi->wrap_base + WRAP_CTRL);
			csi->streaming = false;
		}
		if (csi->phy_powered) {
			ret = phy_power_off(csi->dphy);
			if (ret)
				goto out;
			csi->phy_powered = false;
		}
		if (csi->clocks_enabled)
			goto disable_pclk;
		goto exit_phy;
	}
	goto out;

disable_pclk:
	clk_disable_unprepare(csi->pclk);
	csi->clocks_enabled = false;
disable_wrap:
	clk_disable_unprepare(csi->wrap_clk);
disable_dphy:
	clk_disable_unprepare(csi->dphy_clk);
exit_phy:
	if (!csi->phy_initialized)
		goto out;
	err = phy_exit(csi->dphy);
	if (!err)
		csi->phy_initialized = false;
	if (!ret)
		ret = err;
out:
	mutex_unlock(&csi->lock);
	return ret;
}

static int fh8626_csi_get_fmt(struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *fmt)
{
	struct fh8626_csi_dev *csi = to_fh8626_csi(sd);

	if (fmt->pad > 1 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;
	mutex_lock(&csi->lock);
	fmt->format = fmt->which == V4L2_SUBDEV_FORMAT_TRY ?
		*v4l2_subdev_get_try_format(sd, cfg, fmt->pad) : csi->fmt;
	mutex_unlock(&csi->lock);
	return 0;
}

static int fh8626_csi_set_fmt(struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *fmt)
{
	struct fh8626_csi_dev *csi = to_fh8626_csi(sd);

	if (fmt->pad > 1 ||
	    (fmt->which != V4L2_SUBDEV_FORMAT_TRY &&
	     fmt->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;
	mutex_lock(&csi->lock);
	if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE &&
	    (csi->streaming || sd->entity.stream_count)) {
		mutex_unlock(&csi->lock);
		return -EBUSY;
	}
	if (fmt->pad) {
		fmt->format = fmt->which == V4L2_SUBDEV_FORMAT_TRY ?
			*v4l2_subdev_get_try_format(sd, cfg, 0) : csi->fmt;
	} else {
		switch (fmt->format.code) {
		case MEDIA_BUS_FMT_SRGGB10_1X10:
		case MEDIA_BUS_FMT_SGRBG10_1X10:
		case MEDIA_BUS_FMT_SGBRG10_1X10:
		case MEDIA_BUS_FMT_SBGGR10_1X10:
			break;
		default:
			fmt->format.code = MEDIA_BUS_FMT_SRGGB10_1X10;
		}
		if (fmt->format.width > 1280 || fmt->format.height > 720) {
			fmt->format.width = 1920;
			fmt->format.height = 1080;
		} else {
			fmt->format.width = 1280;
			fmt->format.height = 720;
		}
		fmt->format.field = V4L2_FIELD_NONE;
		fmt->format.colorspace = V4L2_COLORSPACE_RAW;
		fmt->format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
		fmt->format.quantization = V4L2_QUANTIZATION_FULL_RANGE;
		fmt->format.xfer_func = V4L2_XFER_FUNC_NONE;
		memset(fmt->format.reserved, 0, sizeof(fmt->format.reserved));
	}
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		*v4l2_subdev_get_try_format(sd, cfg, 0) = fmt->format;
		*v4l2_subdev_get_try_format(sd, cfg, 1) = fmt->format;
	} else {
		csi->fmt = fmt->format;
	}
	mutex_unlock(&csi->lock);
	return 0;
}

static int fh8626_csi_init_cfg(struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.format.code = MEDIA_BUS_FMT_SRGGB10_1X10,
	};
	return fh8626_csi_set_fmt(sd, cfg, &fmt);
}

static int fh8626_csi_enum_code(struct v4l2_subdev *sd,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_mbus_code_enum *code)
{
	static const u32 codes[] = {
		MEDIA_BUS_FMT_SBGGR10_1X10,
		MEDIA_BUS_FMT_SGBRG10_1X10,
		MEDIA_BUS_FMT_SGRBG10_1X10,
		MEDIA_BUS_FMT_SRGGB10_1X10,
	};

	if (code->pad > 1 || code->index >= ARRAY_SIZE(codes))
		return -EINVAL;
	code->code = codes[code->index];
	return 0;
}

static int fh8626_csi_enum_frame_size(struct v4l2_subdev *sd,
				      struct v4l2_subdev_pad_config *cfg,
				      struct v4l2_subdev_frame_size_enum *fse)
{
	static const struct {
		u32 width;
		u32 height;
	} sizes[] = {
		{ 1280, 720 },
		{ 1920, 1080 },
	};

	if (fse->pad > 1 || fse->index >= ARRAY_SIZE(sizes))
		return -EINVAL;

	switch (fse->code) {
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		break;
	default:
		return -EINVAL;
	}

	fse->min_width = sizes[fse->index].width;
	fse->max_width = sizes[fse->index].width;
	fse->min_height = sizes[fse->index].height;
	fse->max_height = sizes[fse->index].height;

	return 0;
}

static const struct media_entity_operations fh8626_csi_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_pad_ops fh8626_csi_pad_ops = {
	.init_cfg = fh8626_csi_init_cfg,
	.enum_mbus_code = fh8626_csi_enum_code,
	.enum_frame_size = fh8626_csi_enum_frame_size,
	.get_fmt = fh8626_csi_get_fmt,
	.set_fmt = fh8626_csi_set_fmt,
};

static const struct v4l2_subdev_video_ops fh8626_csi_video_ops = {
	.s_stream = fh8626_csi_s_stream,
};

static const struct v4l2_subdev_ops fh8626_csi_subdev_ops = {
	.video = &fh8626_csi_video_ops,
	.pad = &fh8626_csi_pad_ops,
};

static int fh8626_csi_probe(struct platform_device *pdev)
{
	struct fh8626_csi_dev *csi;
	struct resource *res;
	struct device_node *endpoint;
	struct v4l2_fwnode_endpoint bus = { .bus_type = V4L2_MBUS_CSI2 };
	unsigned int i;
	u32 frq = 0;
	u8 frq_range;
	int ret;

	endpoint = of_graph_get_endpoint_by_regs(pdev->dev.of_node, 0, -1);
	if (!endpoint)
		return -EINVAL;
	ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(endpoint), &bus);
	if (ret) {
		of_node_put(endpoint);
		return ret;
	}
	if (bus.bus_type != V4L2_MBUS_CSI2 ||
	    (bus.bus.mipi_csi2.num_data_lanes != 1 &&
	     bus.bus.mipi_csi2.num_data_lanes != 2) ||
	    bus.bus.mipi_csi2.clock_lane != 0) {
		of_node_put(endpoint);
		return -EINVAL;
	}

	for (i = 0; i <= bus.bus.mipi_csi2.num_data_lanes; i++) {
		if (bus.bus.mipi_csi2.lane_polarities[i]) {
			of_node_put(endpoint);
			return -EINVAL;
		}
	}

	if (!of_property_read_u32(endpoint, "fullhan,frq-range", &frq))
		frq_range = frq;
	else
		frq_range = bus.bus.mipi_csi2.num_data_lanes == 2 ? 6 : 5;
	of_node_put(endpoint);

	csi = kzalloc(sizeof(*csi), GFP_KERNEL);
	if (!csi)
		return -ENOMEM;

	csi->dev = &pdev->dev;
	csi->num_lanes = bus.bus.mipi_csi2.num_data_lanes;
	csi->frq_range = frq_range;
	mutex_init(&csi->lock);
	kref_init(&csi->ref);
	csi->lifetime.ref = &csi->ref;
	csi->lifetime.release = fh8626_csi_free;
	v4l2_subdev_init(&csi->sd, &fh8626_csi_subdev_ops);
	v4l2_set_subdev_hostdata(&csi->sd, &csi->lifetime);
	csi->sd.internal_ops = &fh8626_csi_internal_ops;
	csi->fmt.width = FH_FRAME_WIDTH_DEFAULT;
	csi->fmt.height = FH_FRAME_HEIGHT_DEFAULT;
	csi->fmt.code = MEDIA_BUS_FMT_SRGGB10_1X10;
	csi->fmt.field = V4L2_FIELD_NONE;
	csi->fmt.colorspace = V4L2_COLORSPACE_RAW;
	csi->fmt.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	csi->fmt.xfer_func = V4L2_XFER_FUNC_NONE;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "csi");
	if (!res)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	csi->csi_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(csi->csi_base)) {
		ret = PTR_ERR(csi->csi_base);
		goto err_put;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wrap");
	if (!res)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	csi->wrap_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(csi->wrap_base)) {
		ret = PTR_ERR(csi->wrap_base);
		goto err_put;
	}

	csi->dphy_clk = devm_clk_get(&pdev->dev, "dphy");
	if (IS_ERR(csi->dphy_clk)) {
		ret = PTR_ERR(csi->dphy_clk);
		goto err_put;
	}
	csi->wrap_clk = devm_clk_get(&pdev->dev, "wrap");
	if (IS_ERR(csi->wrap_clk)) {
		ret = PTR_ERR(csi->wrap_clk);
		goto err_put;
	}
	csi->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(csi->pclk)) {
		ret = PTR_ERR(csi->pclk);
		goto err_put;
	}
	csi->dphy = devm_phy_get(&pdev->dev, "dphy");
	if (IS_ERR(csi->dphy)) {
		ret = PTR_ERR(csi->dphy);
		goto err_put;
	}

	csi->sd.owner = THIS_MODULE;
	csi->sd.dev = &pdev->dev;
	csi->sd.fwnode = of_fwnode_handle(pdev->dev.of_node);
	strlcpy(csi->sd.name, "fh8626-csi", sizeof(csi->sd.name));
	csi->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

#if defined(CONFIG_MEDIA_CONTROLLER)
	csi->pads[0].flags = MEDIA_PAD_FL_SINK;
	csi->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	csi->sd.entity.function = MEDIA_ENT_F_VID_IF_BRIDGE;
	csi->sd.entity.ops = &fh8626_csi_entity_ops;

	ret = media_entity_pads_init(&csi->sd.entity, 2, csi->pads);
	if (ret)
		goto err_put;
	csi->entity_initialized = true;
#endif

	ret = v4l2_async_register_subdev(&csi->sd);
	if (ret) {
		dev_err(&pdev->dev, "failed to register async subdev: %d\n",
			ret);
		goto err_put;
	}

	platform_set_drvdata(pdev, csi);
	return 0;

err_put:
	kref_put(&csi->ref, fh8626_csi_free);
	return ret;
}

static int fh8626_csi_remove(struct platform_device *pdev)
{
	struct fh8626_csi_dev *csi = platform_get_drvdata(pdev);
	int ret;

	v4l2_async_unregister_subdev(&csi->sd);
	ret = fh8626_csi_s_stream(&csi->sd, 0);
	if (ret)
		dev_err(csi->dev, "failed to stop receiver: %d\n", ret);
	platform_set_drvdata(pdev, NULL);
	kref_put(&csi->ref, fh8626_csi_free);
	return 0;
}

static const struct of_device_id fh8626_csi_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-csi" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_csi_of_match);

struct platform_driver fh8626_csi_driver = {
	.probe = fh8626_csi_probe,
	.remove = fh8626_csi_remove,
	.driver = {
		.name = "fh8626-csi",
		.suppress_bind_attrs = true,
		.of_match_table = fh8626_csi_of_match,
	},
};
