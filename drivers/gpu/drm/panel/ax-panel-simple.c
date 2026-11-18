/*
 * Copyright (C) 2013, NVIDIA Corporation.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include <video/mipi_display.h>
#include <linux/ax_display_hal.h>

#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#ifndef MIPI_DSI_ESD_CHECK_READ_REG_MODE
#define MIPI_DSI_ESD_CHECK_READ_REG_MODE       0
#endif

/**
 * struct panel_desc - Describes a simple panel.
 */
struct panel_desc {
	/**
	 * @modes: Pointer to array of fixed modes appropriate for this panel.
	 *
	 * If only one mode then this can just be the address of the mode.
	 * NOTE: cannot be used with "timings" and also if this is specified
	 * then you cannot override the mode in the device tree.
	 */
	const struct drm_display_mode *modes;

	/** @num_modes: Number of elements in modes array. */
	unsigned int num_modes;

	/**
	 * @timings: Pointer to array of display timings
	 *
	 * NOTE: cannot be used with "modes" and also these will be used to
	 * validate a device tree override if one is present.
	 */
	//const struct display_timing *timings;

	/** @num_timings: Number of elements in timings array. */
	//unsigned int num_timings;

	struct display_timings *timing;

	/** @bpc: Bits per color. */
	unsigned int bpc;

	/** @size: Structure containing the physical size of this panel. */
	struct {
		/**
		 * @size.width: Width (in mm) of the active display area.
		 */
		unsigned int width;

		/**
		 * @size.height: Height (in mm) of the active display area.
		 */
		unsigned int height;
	} size;

	/** @delay: Structure containing various delay values for this panel. */
	struct {
		/**
		 * @delay.prepare: Time for the panel to become ready.
		 *
		 * The time (in milliseconds) that it takes for the panel to
		 * become ready and start receiving video data
		 */
		unsigned int prepare;

		/**
		 * @delay.hpd_absent_delay: Time to wait if HPD isn't hooked up.
		 *
		 * Add this to the prepare delay if we know Hot Plug Detect
		 * isn't used.
		 */
		unsigned int hpd_absent_delay;

		/**
		 * @delay.prepare_to_enable: Time between prepare and enable.
		 *
		 * The minimum time, in milliseconds, that needs to have passed
		 * between when prepare finished and enable may begin. If at
		 * enable time less time has passed since prepare finished,
		 * the driver waits for the remaining time.
		 *
		 * If a fixed enable delay is also specified, we'll start
		 * counting before delaying for the fixed delay.
		 *
		 * If a fixed prepare delay is also specified, we won't start
		 * counting until after the fixed delay. We can't overlap this
		 * fixed delay with the min time because the fixed delay
		 * doesn't happen at the end of the function if a HPD GPIO was
		 * specified.
		 *
		 * In other words:
		 *   prepare()
		 *     ...
		 *     // do fixed prepare delay
		 *     // wait for HPD GPIO if applicable
		 *     // start counting for prepare_to_enable
		 *
		 *   enable()
		 *     // do fixed enable delay
		 *     // enforce prepare_to_enable min time
		 */
		unsigned int prepare_to_enable;

		/**
		 * @delay.enable: Time for the panel to display a valid frame.
		 *
		 * The time (in milliseconds) that it takes for the panel to
		 * display the first valid frame after starting to receive
		 * video data.
		 */
		unsigned int enable;

		/**
		 * @delay.disable: Time for the panel to turn the display off.
		 *
		 * The time (in milliseconds) that it takes for the panel to
		 * turn the display off (no content is visible).
		 */
		unsigned int disable;

		/**
		 * @delay.unprepare: Time to power down completely.
		 *
		 * The time (in milliseconds) that it takes for the panel
		 * to power itself down completely.
		 *
		 * This time is used to prevent a future "prepare" from
		 * starting until at least this many milliseconds has passed.
		 * If at prepare time less time has passed since unprepare
		 * finished, the driver waits for the remaining time.
		 */
		unsigned int unprepare;
	} delay;

	/** @bus_format: See MEDIA_BUS_FMT_... defines. */
	u32 bus_format;

	/** @bus_flags: See DRM_BUS_FLAG_... defines. */
	u32 bus_flags;

	/** @connector_type: LVDS, eDP, DSI, DPI, etc. */
	int connector_type;
};

struct panel_simple {
	struct drm_panel base;
	bool enabled;
	bool no_hpd;

	bool prepared;

	ktime_t prepared_time;
	ktime_t unprepared_time;

	struct panel_desc *desc;

	struct regulator *supply;
	struct i2c_adapter *ddc;

	struct gpio_desc *reset_gpio;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *hpd_gpio;

	struct edid *edid;

	struct drm_display_mode override_mode;

	enum drm_panel_orientation orientation;
};

struct panel_esd_dsi {
	struct device  *dev;
	u8 enabled;
	u8 check_mode;
	u8 check_reg;
	u8 reg_read_len;
	u8 max_error_cnt;
	u8 error_cnt;
	u8 reg_status_value;
	u32 check_internal_time;
	struct delayed_work esd_work;
	void (*esd_check)(struct panel_esd_dsi *dsi_esd);
};

struct panel_desc_dsi {
	struct panel_desc desc;

	u32 num_init_seqs;
	u8 *init_seq;
	u32 num_exit_seqs;
	u8 *exit_seq;

	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;

	struct mipi_dsi_device *dsi;
	struct panel_esd_dsi dsi_esd;
};

static inline struct panel_simple *to_panel_simple(struct drm_panel *panel)
{
	return container_of(panel, struct panel_simple, base);
}

static inline struct panel_desc_dsi *to_desc_dsi(struct panel_desc *desc)
{
	return container_of(desc, struct panel_desc_dsi, desc);
}

static int panel_simple_get_fixed_modes(struct panel_simple *panel)
{
	struct drm_connector *connector = panel->base.connector;
	struct drm_device *drm = panel->base.drm;
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	if (!panel->desc)
		return 0;

	if (panel->desc->timing) {
		for (i = 0; i < panel->desc->timing->num_timings; i++) {
			const struct display_timing *dt = panel->desc->timing->timings[i];
			struct videomode vm;

			videomode_from_timing(dt, &vm);
			mode = drm_mode_create(drm);
			if (!mode) {
				dev_err(drm->dev, "failed to add mode %ux%u\n",
					dt->hactive.typ, dt->vactive.typ);
				continue;
			}

			drm_display_mode_from_videomode(&vm, mode);

			mode->type |= DRM_MODE_TYPE_DRIVER;

			if (panel->desc->timing->num_timings == 1)
				mode->type |= DRM_MODE_TYPE_PREFERRED;

			drm_mode_probed_add(connector, mode);
			num++;
		}
	}

	for (i = 0; i < panel->desc->num_modes; i++) {
		const struct drm_display_mode *m = &panel->desc->modes[i];

		mode = drm_mode_duplicate(drm, m);
		if (!mode) {
			dev_err(drm->dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay, m->vrefresh);
			continue;
		}

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (panel->desc->num_modes == 1)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);

		drm_mode_probed_add(connector, mode);
		num++;
	}

	connector->display_info.bpc = panel->desc->bpc;
	connector->display_info.width_mm = panel->desc->size.width;
	connector->display_info.height_mm = panel->desc->size.height;
	if (panel->desc->bus_format)
		drm_display_info_set_bus_formats(&connector->display_info,
						 &panel->desc->bus_format, 1);
	connector->display_info.bus_flags = panel->desc->bus_flags;

	return num;
}

static void panel_simple_wait(ktime_t start_ktime, unsigned int min_ms)
{
	ktime_t now_ktime, min_ktime;

	if (!min_ms)
		return;

	min_ktime = ktime_add(start_ktime, ms_to_ktime(min_ms));
	now_ktime = ktime_get();

	if (ktime_before(now_ktime, min_ktime))
		msleep(ktime_to_ms(ktime_sub(min_ktime, now_ktime)) + 1);
}

static int panel_send_dsi_cmds(struct mipi_dsi_device *dsi, u8 *data, int len)
{
	int i, delay, cmd_len, ret;

	for (i = 0; i < len; i += (cmd_len + 3)) {
		delay = data[i + 1];
		cmd_len = data[i + 2];
		if (cmd_len + i + 3 > len) {
			print_hex_dump(KERN_ERR, "cmd overflow, cmd: ", DUMP_PREFIX_NONE,
			               16, 1, &data[i], len - i, false);
			return -EINVAL;
		}

		switch (data[i]) {
		case MIPI_DSI_DCS_SHORT_WRITE:
		case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		case MIPI_DSI_DCS_LONG_WRITE:
			ret = mipi_dsi_dcs_write_buffer(dsi, &data[i + 3], cmd_len);
			if (ret < 0) {
				print_hex_dump(KERN_ERR, "dcs send failed, cmd: ", DUMP_PREFIX_NONE,
				               16, 1, &data[i], cmd_len + 3, false);
				return ret;
			}
			break;

		case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		case MIPI_DSI_GENERIC_LONG_WRITE:
			ret = mipi_dsi_generic_write(dsi, &data[i + 3], cmd_len);
			if (ret < 0) {
				print_hex_dump(KERN_ERR, "generic send failed, cmd: ", DUMP_PREFIX_NONE,
				               16, 1, &data[i], cmd_len + 3, false);
				return ret;
			}
			break;

		default:
			pr_err("%s unsupported data type, type = 0x%x\n", __func__, data[i]);
			return -EINVAL;
		}

		print_hex_dump(KERN_DEBUG, "send success, cmd: ", DUMP_PREFIX_NONE,
		               16, 1, &data[i], cmd_len + 3, false);

		if (delay)
			mdelay(delay);
	}

	return 0;
}

static void panel_esd_start_check(struct panel_esd_dsi *dsi_esd)
{
	if (dsi_esd->esd_check != NULL)
		schedule_delayed_work(&dsi_esd->esd_work,
				   msecs_to_jiffies(dsi_esd->check_internal_time));
}

static void panel_esd_stop_check(struct panel_esd_dsi *dsi_esd)
{
	if (dsi_esd->esd_check != NULL)
		cancel_delayed_work_sync(&dsi_esd->esd_work);
}

static int panel_simple_disable(struct drm_panel *panel)
{
	struct panel_simple *p = to_panel_simple(panel);
	struct panel_desc_dsi *desc_dsi;
	int err;

	if (!p->enabled)
		return 0;

	desc_dsi = to_desc_dsi(p->desc);

	if (p->desc->connector_type == DRM_MODE_CONNECTOR_DSI) {
		if (desc_dsi->dsi_esd.enabled) {
			panel_esd_stop_check(&desc_dsi->dsi_esd);
		}

		if (desc_dsi->exit_seq && desc_dsi->num_exit_seqs) {
			err = panel_send_dsi_cmds(desc_dsi->dsi, desc_dsi->exit_seq, desc_dsi->num_exit_seqs);
			if (err)
				return err;
		}
	}

	if (p->desc->delay.disable)
		msleep(p->desc->delay.disable);

	p->enabled = false;

	return 0;
}
#if 0
static int panel_simple_suspend(struct device *dev)
{
	struct panel_simple *p = dev_get_drvdata(dev);

	if (p->enable_gpio)
		gpiod_set_value_cansleep(p->enable_gpio, 0);
	if (p->supply)
		regulator_disable(p->supply);
	p->unprepared_time = ktime_get();

	kfree(p->edid);
	p->edid = NULL;

	return 0;
}
#endif
static int panel_simple_unprepare(struct drm_panel *panel)
{
	struct panel_simple *p = to_panel_simple(panel);

	/* Unpreparing when already unprepared is a no-op */
	if (!p->prepared)
		return 0;

	if (p->reset_gpio) {
		gpiod_set_value(p->reset_gpio, 0);
	}

	if (p->desc->delay.unprepare)
		msleep(p->desc->delay.unprepare);

	p->prepared = false;

	return 0;
}

static int panel_simple_get_hpd_gpio(struct device *dev, struct panel_simple *p)
{
	int err;

	p->hpd_gpio = devm_gpiod_get_optional(dev, "hpd", GPIOD_IN);
	if (IS_ERR(p->hpd_gpio)) {
		err = PTR_ERR(p->hpd_gpio);

		if (err != -EPROBE_DEFER)
			dev_err(dev, "failed to get 'hpd' GPIO: %d\n", err);

		return err;
	}

	return 0;
}

#if 0
static int panel_simple_prepare_once(struct panel_simple *p)
{
	struct device *dev = p->base.dev;
	unsigned int delay;
	int err;
	int hpd_asserted;
	unsigned long hpd_wait_us;

	panel_simple_wait(p->unprepared_time, p->desc->delay.unprepare);

	if (p->supply) {
		err = regulator_enable(p->supply);
		if (err < 0) {
			dev_err(dev, "failed to enable supply: %d\n", err);
			return err;
		}
	}

	if (p->enable_gpio)
		gpiod_set_value_cansleep(p->enable_gpio, 1);

	delay = p->desc->delay.prepare;
	if (p->no_hpd)
		delay += p->desc->delay.hpd_absent_delay;
	if (delay)
		msleep(delay);

	if (p->hpd_gpio) {
		if (p->desc->delay.hpd_absent_delay)
			hpd_wait_us = p->desc->delay.hpd_absent_delay * 1000UL;
		else
			hpd_wait_us = 2000000;

		err = readx_poll_timeout(gpiod_get_value_cansleep, p->hpd_gpio,
					 hpd_asserted, hpd_asserted,
					 1000, hpd_wait_us);
		if (hpd_asserted < 0)
			err = hpd_asserted;

		if (err) {
			if (err != -ETIMEDOUT)
				dev_err(dev,
					"error waiting for hpd GPIO: %d\n", err);
			goto error;
		}
	}

	p->prepared_time = ktime_get();

	return 0;

error:
	if (p->enable_gpio)
		gpiod_set_value_cansleep(p->enable_gpio, 0);
	if (p->supply)
		regulator_disable(p->supply);
	p->unprepared_time = ktime_get();

	return err;
}

/*
 * Some panels simply don't always come up and need to be power cycled to
 * work properly.  We'll allow for a handful of retries.
 */

#define MAX_PANEL_PREPARE_TRIES		5

static int panel_simple_resume(struct device *dev)
{
	struct panel_simple *p = dev_get_drvdata(dev);
	int ret;
	int try;

	for (try = 0; try < MAX_PANEL_PREPARE_TRIES; try++) {
		ret = panel_simple_prepare_once(p);
		if (ret != -ETIMEDOUT)
			break;
	}

	if (ret == -ETIMEDOUT)
		dev_err(dev, "Prepare timeout after %d tries\n", try);
	else if (try)
		dev_warn(dev, "Prepare needed %d retries\n", try);

	return ret;
}
#endif
extern void tp2803_mode_sel(void);
static int panel_simple_prepare(struct drm_panel *panel)
{
	struct panel_simple *p = to_panel_simple(panel);
	struct panel_desc *desc = p->desc;

  	/* Preparing when already prepared is a no-op */
	if (p->prepared)
		return 0;

	if (p->reset_gpio) {
		pr_info("%s pull reset pin\n", __func__);
		gpiod_set_value(p->reset_gpio, 0);
		gpiod_set_value(p->reset_gpio, 1);
	}

	if (desc->connector_type == 0) {
		tp2803_mode_sel();
	}

	if (desc->delay.prepare)
		msleep(p->desc->delay.prepare);

	p->prepared = true;

	return 0;
}

static int panel_simple_enable(struct drm_panel *panel)
{
	struct panel_simple *p = to_panel_simple(panel);
	struct panel_desc *desc = p->desc;
	struct panel_desc_dsi *desc_dsi;
	int ret;

	if (p->enabled)
		return 0;

	if (p->desc->delay.enable)
		msleep(p->desc->delay.enable);

	panel_simple_wait(p->prepared_time, p->desc->delay.prepare_to_enable);

	if (desc->connector_type == DRM_MODE_CONNECTOR_DSI) {
		desc_dsi = to_desc_dsi(desc);
		if (desc_dsi->init_seq && desc_dsi->num_init_seqs) {
			ret = panel_send_dsi_cmds(desc_dsi->dsi, desc_dsi->init_seq, desc_dsi->num_init_seqs);
			if (ret)
				return ret;
		}

		if (desc_dsi->dsi_esd.enabled) {
			ret = mipi_dsi_set_maximum_return_packet_size(desc_dsi->dsi, desc_dsi->dsi_esd.reg_read_len);
			if (ret < 0) {
				dev_err(desc_dsi->dsi_esd.dev, "error %d setting maximum return packet size to %d\n",
					ret, desc_dsi->dsi_esd.reg_read_len);
			}

			panel_esd_start_check(&desc_dsi->dsi_esd);
		}
	}

	p->enabled = true;

	return 0;
}

static int panel_simple_get_modes(struct drm_panel *panel)
{
	struct panel_simple *p = to_panel_simple(panel);
	int num = 0;

	/* probe EDID if a DDC bus is available */
	if (p->ddc) {
		pm_runtime_get_sync(panel->dev);

		if (!p->edid)
			p->edid = drm_get_edid(panel->connector, p->ddc);

		if (p->edid)
			num += drm_add_edid_modes(panel->connector, p->edid);

		pm_runtime_mark_last_busy(panel->dev);
		pm_runtime_put_autosuspend(panel->dev);
	}

	/* add hard-coded panel modes */
	num += panel_simple_get_fixed_modes(p);

	return num;
}

static int panel_simple_get_timings(struct drm_panel *panel,
				    unsigned int num_timings,
				    struct display_timing *timings)
{
	struct panel_simple *p = to_panel_simple(panel);
	unsigned int i;

	if (p->desc->timing == NULL) {
		return 0;
	}

	if (p->desc->timing->num_timings < num_timings)
		num_timings = p->desc->timing->num_timings;

	if (timings)
		for (i = 0; i < num_timings; i++)
			timings[i] = *(p->desc->timing->timings[i]);

	return p->desc->timing->num_timings;
}

static const struct drm_panel_funcs panel_simple_funcs = {
	.disable = panel_simple_disable,
	.unprepare = panel_simple_unprepare,
	.prepare = panel_simple_prepare,
	.enable = panel_simple_enable,
	.get_modes = panel_simple_get_modes,
	.get_timings = panel_simple_get_timings,
};

static int panel_simple_probe(struct device *dev, struct panel_desc *desc)
{
	struct panel_simple *panel;
	struct device_node *ddc;
	int connector_type;
	u32 bus_flags;
	int err;

	panel = devm_kzalloc(dev, sizeof(*panel), GFP_KERNEL);
	if (!panel) {
		dev_err(dev, "malloc for panel_simple failed.\n");
		return -ENOMEM;
	}

	panel->enabled = false;
	panel->prepared_time = 0;
	panel->desc = desc;

	panel->no_hpd = of_property_read_bool(dev->of_node, "no-hpd");
	if (!panel->no_hpd) {
		err = panel_simple_get_hpd_gpio(dev, panel);
		if (err)
			dev_info(dev, "panel get hpd gpio failed\n");
	}

	panel->supply = devm_regulator_get(dev, "power");
	if (IS_ERR(panel->supply))
		dev_info(dev, "%pOF: panel without regulator power\n", dev->of_node);

	panel->reset_gpio = devm_gpiod_get_optional(dev, "reset", 0);
	if (IS_ERR(panel->reset_gpio))
		DRM_WARN("%pOF: panel without reset_gpio\n", dev->of_node);
	else
		gpiod_direction_output(panel->reset_gpio, 1);

	panel->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;

	ddc = of_parse_phandle(dev->of_node, "ddc-i2c-bus", 0);
	if (ddc) {
		panel->ddc = of_find_i2c_adapter_by_node(ddc);
		of_node_put(ddc);

		if (!panel->ddc)
			return -EPROBE_DEFER;
	}

	desc->timing = of_get_display_timings(dev->of_node);

	connector_type = desc->connector_type;
	/* Catch common mistakes for panels. */
	switch (connector_type) {
	case 0:
		dev_warn(dev, "Specify missing connector_type\n");
		connector_type = DRM_MODE_CONNECTOR_DPI;
		break;
	case DRM_MODE_CONNECTOR_DSI:
		if (desc->bpc != 6 && desc->bpc != 8)
			dev_warn(dev, "Expected bpc in {6,8} but got: %u\n", desc->bpc);
		break;
	case DRM_MODE_CONNECTOR_DPI:
#if 0
		bus_flags = DRM_BUS_FLAG_DE_LOW |
			    DRM_BUS_FLAG_DE_HIGH |
			    DRM_BUS_FLAG_PIXDATA_SAMPLE_POSEDGE |
			    DRM_BUS_FLAG_PIXDATA_SAMPLE_NEGEDGE |
			    DRM_BUS_FLAG_DATA_MSB_TO_LSB |
			    DRM_BUS_FLAG_DATA_LSB_TO_MSB |
			    DRM_BUS_FLAG_SYNC_SAMPLE_POSEDGE |
			    DRM_BUS_FLAG_SYNC_SAMPLE_NEGEDGE;
#else
		bus_flags = 0;
#endif
		if (desc->bus_flags & ~bus_flags)
			dev_warn(dev, "Unexpected bus_flags(%d)\n", desc->bus_flags & ~bus_flags);
		if (!(desc->bus_flags & bus_flags))
			dev_warn(dev, "Specify missing bus_flags\n");
		if (desc->bus_format == 0)
			dev_warn(dev, "Specify missing bus_format\n");
		if (desc->bpc != 6 && desc->bpc != 8)
			dev_warn(dev, "Expected bpc in {6,8} but got: %u\n", desc->bpc);
		break;
	default:
		dev_warn(dev, "Specify a valid connector_type: %d\n", desc->connector_type);
		connector_type = DRM_MODE_CONNECTOR_DPI;
		break;
	}

	dev_set_drvdata(dev, panel);

	/*
	 * We use runtime PM for prepare / unprepare since those power the panel
	 * on and off and those can be very slow operations. This is important
	 * to optimize powering the panel on briefly to read the EDID before
	 * fully enabling the panel.
	 */
	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, 1000);
	pm_runtime_use_autosuspend(dev);

	drm_panel_init(&panel->base);
	panel->base.dev = dev;
	panel->base.funcs = &panel_simple_funcs;

	drm_panel_add(&panel->base);

	return 0;
}

static int panel_simple_remove(struct device *dev)
{
	struct panel_simple *panel = dev_get_drvdata(dev);

	drm_panel_remove(&panel->base);
	drm_panel_disable(&panel->base);
	drm_panel_unprepare(&panel->base);

	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_disable(dev);
	if (panel->ddc)
		put_device(&panel->ddc->dev);

	return 0;
}

static void panel_simple_shutdown(struct device *dev)
{
	struct panel_simple *panel = dev_get_drvdata(dev);

	drm_panel_disable(&panel->base);
	drm_panel_unprepare(&panel->base);
}

static const struct of_device_id platform_of_match[] = {
	{
		/* Must be the last entry */
		.compatible = "axera,panel-dpi",
		.data = NULL,
	}, {
		.compatible = "axera,simple-panel",
		.data = NULL,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, platform_of_match);

static int panel_simple_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct panel_desc *desc;

	id = of_match_node(platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	desc = devm_kzalloc(&pdev->dev, sizeof(*desc), GFP_KERNEL);
		if (!desc)
			return -ENOMEM;

	return panel_simple_probe(&pdev->dev, desc);
}

static int panel_simple_platform_remove(struct platform_device *pdev)
{
	return panel_simple_remove(&pdev->dev);
}

static void panel_simple_platform_shutdown(struct platform_device *pdev)
{
	panel_simple_shutdown(&pdev->dev);
}

#if 0
static const struct dev_pm_ops panel_simple_pm_ops = {
	SET_RUNTIME_PM_OPS(panel_simple_suspend, panel_simple_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};
#endif
static struct platform_driver panel_simple_platform_driver = {
	.driver = {
		.name = "ax-panel-simple",
		.of_match_table = platform_of_match,
	},
	.probe = panel_simple_platform_probe,
	.remove = panel_simple_platform_remove,
	.shutdown = panel_simple_platform_shutdown,
};

static int panel_dsi_dts_parse(struct device *dev, struct panel_desc_dsi *desc_dsi)
{
	int len, ret = 0;
	u32 val;

	ret = of_property_read_u32(dev->of_node, "dsi,format", &val);
	if (ret || val > MIPI_DSI_FMT_RGB565) {
		pr_warn("dsi format invalid, use default value\n");
		val = MIPI_DSI_FMT_RGB888;
	}

	desc_dsi->format = val;

	ret = of_property_read_u32(dev->of_node, "dsi,lanes", &val);
	if (ret || val == 0 || val > 4) {
		pr_info("dsi lanes invalid, use default value\n");
		val = 4;
	}

	desc_dsi->lanes = val;

	ret = of_property_read_u32(dev->of_node, "dsi,flags", &val);
	if (ret) {
		pr_info("dsi flags not defined, use default value\n");
		val = MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_LPM;
	}

	desc_dsi->flags = val;

	pr_info("dsi panel format = %d, lanes = %d, flags = 0x%lx\n",
		desc_dsi->format, desc_dsi->lanes, desc_dsi->flags);

	len = of_property_count_elems_of_size(dev->of_node, "panel-init-seq", 1);
	if (len > 0) {
		pr_info("dsi num_init_seqs = %d\n", len);
		desc_dsi->num_init_seqs = len;
		desc_dsi->init_seq = devm_kzalloc(dev, len, GFP_KERNEL);
		if (!desc_dsi->init_seq) {
			pr_err("alloc init_seq memory failed\n");
			return -ENOMEM;
		}

		ret = of_property_read_u8_array(dev->of_node, "panel-init-seq",
			desc_dsi->init_seq, len);
		if (ret) {
			pr_err("parse dsi panel init-seq failed, ret = %d\n", ret);
			return ret;
		}

		print_hex_dump(KERN_INFO, "panel-init-seq: ", DUMP_PREFIX_NONE,
		               16, 1, desc_dsi->init_seq, len, false);

	} else {
		pr_info("dsi panel has no init-seq\n");
	}

	len = of_property_count_elems_of_size(dev->of_node, "panel-exit-seq", 1);
	if (len > 0) {
		pr_info("dsi num_exit_seqs = %d\n", len);
		desc_dsi->num_exit_seqs = len;
		desc_dsi->exit_seq = devm_kzalloc(dev, len, GFP_KERNEL);
		if (!desc_dsi->exit_seq) {
			pr_err("alloc exit_seq memory failed\n");
			return -ENOMEM;
		}

		ret = of_property_read_u8_array(dev->of_node, "panel-exit-seq",
		                                desc_dsi->exit_seq, len);
		if (ret) {
			pr_err("read dsi panel exit-seq failed, ret = %d\n", ret);
			return ret;
		}

		print_hex_dump(KERN_DEBUG, "panel-exit-seq: ", DUMP_PREFIX_NONE,
		               16, 1, desc_dsi->exit_seq, len, false);

	} else {
		pr_info("dsi panel has no exit-seq\n");
	}

	of_property_read_u32(dev->of_node, "enable-delay-ms", &desc_dsi->desc.delay.enable);
	of_property_read_u32(dev->of_node, "disable-delay-ms", &desc_dsi->desc.delay.disable);
	of_property_read_u32(dev->of_node, "prepare-delay-ms", &desc_dsi->desc.delay.prepare);
	of_property_read_u32(dev->of_node, "unprepare-delay-ms", &desc_dsi->desc.delay.unprepare);

	return 0;
}

static const struct of_device_id dsi_of_match[] = {
	{
		.compatible = "axera,simple-dsi-panel",
		.data = NULL
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, dsi_of_match);

static void panel_esd_recovery(struct panel_esd_dsi *dsi_esd)
{
	int ret;
	struct panel_simple *p = dev_get_drvdata(dsi_esd->dev);
	struct panel_desc_dsi * desc_dsi = container_of(dsi_esd, struct panel_desc_dsi, dsi_esd);

	dev_info(dsi_esd->dev, "%s an abnormal screen is detected and recovery is initiated!\n", __func__);
	ax_display_dpu_close();
	if (p->reset_gpio) {
		gpiod_set_value(p->reset_gpio, 0);
		mdelay(100);
		gpiod_set_value(p->reset_gpio, 1);
		mdelay(10);
	}

	if (desc_dsi->init_seq && desc_dsi->num_init_seqs) {
		ret = panel_send_dsi_cmds(desc_dsi->dsi, desc_dsi->init_seq,
						desc_dsi->num_init_seqs);
		if (ret)
			dev_err(dsi_esd->dev, "panel_send_dsi_cmds fail\n");
	}

	ax_display_dpu_open();

	dsi_esd->error_cnt = 0;
}

static void panel_esd_check(struct panel_esd_dsi *dsi_esd)
{
	struct panel_desc_dsi * desc_dsi = container_of(dsi_esd, struct panel_desc_dsi, dsi_esd);
	struct mipi_dsi_device *dsi = desc_dsi->dsi;
	int ret;
	u8 reg_data, reg_len;
	reg_len = dsi_esd->reg_read_len;

	ret = mipi_dsi_dcs_read(dsi, dsi_esd->check_reg, &reg_data, reg_len);
	if (ret < 0) {
		dev_err(dsi_esd->dev, "error %d reading dcs seq(%#x)\n", ret, dsi_esd->check_reg);
	}

	if (reg_data != dsi_esd->reg_status_value) {
		dsi_esd->error_cnt++;
	} else
		dsi_esd->error_cnt = 0;

	if (dsi_esd->error_cnt >= dsi_esd->max_error_cnt)
		panel_esd_recovery(dsi_esd);
}

static void panel_esd_work(struct work_struct *work)
{
	struct panel_esd_dsi *dsi_esd = container_of(work, struct panel_esd_dsi,
						     esd_work.work);

	dsi_esd->esd_check(dsi_esd);
	panel_esd_start_check(dsi_esd);
}

static int panel_simple_dsi_esd_check_init(struct device *dev, struct panel_esd_dsi *dsi_esd)
{
	int ret = 0;

	dsi_esd->dev = dev;

	ret = of_property_read_u8(dev->of_node, "dsi-panel_esd_check_enable", &dsi_esd->enabled);
	if (ret) {
		dev_err(dev, "dsi esd check enabled invalid\n");
		goto error;
	}

	ret = of_property_read_u8(dev->of_node, "dsi-panel-check-mode", &dsi_esd->check_mode);
	if (ret || (dsi_esd->check_mode != MIPI_DSI_ESD_CHECK_READ_REG_MODE)) {
		dev_err(dev, "dsi esd mode invalid dsi_esd->check_mode:%d ret:%d\n",dsi_esd->check_mode,ret);
		goto error;
	}

	ret = of_property_read_u8(dev->of_node, "dsi-panel-read-reg", &dsi_esd->check_reg);
	if (ret) {
		dev_err(dev, "dsi esd read reg invalid\n");
		goto error;
	}

	ret = of_property_read_u8(dev->of_node, "dsi-panel-read-length", &dsi_esd->reg_read_len);
	if (ret) {
		dev_err(dev, "dsi esd read len invalid\n");
		goto error;
	}

	ret = of_property_read_u8(dev->of_node, "dsi-panel-max-error-count", &dsi_esd->max_error_cnt);
	if (ret) {
		dev_err(dev, "dsi esd max error count invalid, use default value\n");
		dsi_esd->max_error_cnt = 3;
	}

	ret = of_property_read_u8(dev->of_node, "dsi-panel-status-value", &dsi_esd->reg_status_value);
	if (ret) {
		dev_err(dev, "dsi esd reg status value invalid, use default value\n");
		goto error;
	}

	ret = of_property_read_u32(dev->of_node, "dsi-panel-check-interval-ms", &dsi_esd->check_internal_time);
	if (ret) {
		dev_err(dev, "dsi esd reg status value invalid, use default value\n");
		dsi_esd->check_internal_time = 5000;
	}

	dsi_esd->esd_check = panel_esd_check;
	INIT_DELAYED_WORK(&dsi_esd->esd_work, panel_esd_work);

error:
	return ret;
}

static void panel_simple_dsi_esd_check_deinit(struct panel_esd_dsi *dsi_esd)
{
	if (!dsi_esd->enabled) {
		return;
	}

	panel_esd_stop_check(dsi_esd);

	dsi_esd->enabled = 0;
}

static int panel_simple_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct panel_desc_dsi *desc_dsi;
	const struct of_device_id *id;
	int err;

	id = of_match_node(dsi_of_match, dsi->dev.of_node);
	if (!id)
		return -ENODEV;

	desc_dsi = devm_kzalloc(&dsi->dev, sizeof(*desc_dsi), GFP_KERNEL);
	if (!desc_dsi) {
		pr_err("alloc panel desc dsi failed\n");
		return -ENOMEM;
	}

	desc_dsi->dsi = dsi;
	desc_dsi->desc.connector_type = DRM_MODE_CONNECTOR_DSI;

	err = panel_dsi_dts_parse(&dsi->dev, desc_dsi);
	if (err < 0) {
		pr_err("parse dsi panel dts failed, ret = %d\n", err);
		return err;
	}

	err = panel_simple_probe(&dsi->dev, &desc_dsi->desc);
	if (err < 0)
		return err;

	dsi->mode_flags = desc_dsi->flags;
	dsi->format = desc_dsi->format;
	dsi->lanes = desc_dsi->lanes;

	panel_simple_dsi_esd_check_init(&dsi->dev, &desc_dsi->dsi_esd);

	err = mipi_dsi_attach(dsi);
	if (err) {
		struct panel_simple *panel = mipi_dsi_get_drvdata(dsi);

		drm_panel_remove(&panel->base);
	}

	pr_info("%s probe %s\n", __func__, err ? "failed" : "success");
	return err;
}

static int panel_simple_dsi_remove(struct mipi_dsi_device *dsi)
{
	int err;
	struct panel_simple *panel = dev_get_drvdata(&dsi->dev);
	struct panel_desc *desc = panel->desc;
	struct panel_desc_dsi *desc_dsi = to_desc_dsi(desc);

	err = mipi_dsi_detach(dsi);
	if (err < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n", err);

	panel_simple_dsi_esd_check_deinit(&desc_dsi->dsi_esd);
	return panel_simple_remove(&dsi->dev);
}

static void panel_simple_dsi_shutdown(struct mipi_dsi_device *dsi)
{
	panel_simple_shutdown(&dsi->dev);
}

static struct mipi_dsi_driver panel_simple_dsi_driver = {
	.driver = {
		.name = "panel-simple-dsi",
		.of_match_table = dsi_of_match,
	},
	.probe = panel_simple_dsi_probe,
	.remove = panel_simple_dsi_remove,
	.shutdown = panel_simple_dsi_shutdown,
};

static const struct of_device_id panel_lvds_match[] = {
	{ .compatible = "axera,lvds-panel", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, panel_lvds_of_table);

static int panel_lvds_probe(struct platform_device *pdev)
{
	int ret;
	const struct of_device_id *id;
	struct panel_desc *desc;
	const char *mapping;
	int err;

	id = of_match_node(panel_lvds_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	desc = devm_kzalloc(&pdev->dev, sizeof(*desc), GFP_KERNEL);
		if (!desc)
			return -ENOMEM;

	desc->connector_type = DRM_MODE_CONNECTOR_LVDS;

	ret = of_property_read_string(pdev->dev.of_node, "data-mapping", &mapping);
	if (ret < 0) {
		pr_err("%pOF: invalid or missing %s property\n",
			mapping, "data-mapping");
		return -ENODEV;
	}

	if (!strcmp(mapping, "jeida-18")) {
		desc->bus_format = MEDIA_BUS_FMT_RGB666_1X7X3_SPWG;
	} else if (!strcmp(mapping, "jeida-24")) {
		desc->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA;
	} else if (!strcmp(mapping, "vesa-24")) {
		desc->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
	} else {
		pr_err("%pOF: invalid or missing %s property\n",
			mapping, "data-mapping");
		return -EINVAL;
	}

	err =  panel_simple_probe(&pdev->dev, desc);
	if (err < 0)
		return err;

	return 0;
}

static int panel_lvds_remove(struct platform_device *pdev)
{
	return panel_simple_remove(&pdev->dev);
}

static struct platform_driver panel_lvds_driver = {
	.probe		= panel_lvds_probe,
	.remove		= panel_lvds_remove,
	.driver		= {
		.name	= "panel-lvds",
		.of_match_table = panel_lvds_match,
	},
};

static int __init panel_simple_init(void)
{
	int err;

	err = platform_driver_register(&panel_simple_platform_driver);
	if (err < 0)
		return err;

	if (IS_ENABLED(CONFIG_DRM_MIPI_DSI)) {
		err = mipi_dsi_driver_register(&panel_simple_dsi_driver);
		if (err < 0)
			goto err_did_platform_register;
	}

	err = platform_driver_register(&panel_lvds_driver);
	if (err < 0)
		goto err_lvds_platform_register;

	return 0;

err_lvds_platform_register:
	if (IS_ENABLED(CONFIG_DRM_MIPI_DSI))
		mipi_dsi_driver_unregister(&panel_simple_dsi_driver);

err_did_platform_register:
	platform_driver_unregister(&panel_simple_platform_driver);

	return err;
}
module_init(panel_simple_init);

static void __exit panel_simple_exit(void)
{
	if (IS_ENABLED(CONFIG_DRM_MIPI_DSI))
		mipi_dsi_driver_unregister(&panel_simple_dsi_driver);

	platform_driver_unregister(&panel_simple_platform_driver);

	platform_driver_unregister(&panel_lvds_driver);
}
module_exit(panel_simple_exit);

MODULE_DESCRIPTION("DRM Driver for Axera Simple Panels");
MODULE_LICENSE("GPL and additional rights");
