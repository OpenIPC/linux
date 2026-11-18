// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the ST7789P3 LCD Controller
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/printk.h>

#include "fbtft.h"

#ifndef dev_err_probe
#define dev_err_probe(dev, err, fmt, ...)              \
    ({                                                 \
        int __ret = (err);                            \
        if (__ret == -EPROBE_DEFER)                    \
            dev_dbg(dev, fmt, ##__VA_ARGS__);         \
        else                                          \
            dev_err(dev, fmt, ##__VA_ARGS__);         \
        __ret;                                        \
    })
#endif


#define DRVNAME "fb_st7789p3"

#define DEFAULT_GAMMA                                 \
	"F0 05 0E 08 0A 17 39 54 4E 37 12 12 31 37\n" \
	"F0 10 14 0D 0B 05 39 44 4D 38 14 14 2E 35"

/**
 * enum st7789p3_command - ST7789P3 display controller commands
 *
 * @PORCTRL: porch setting
 * @GCTRL: gate control
 * @VCOMS: VCOM setting
 * @VDVVRHEN: VDV and VRH command enable
 * @VRHS: VRH set
 * @VDVS: VDV set
 * @VCMOFSET: VCOM offset set
 * @PWCTRL1: power control 1
 * @PVGAMCTRL: positive voltage gamma control
 * @NVGAMCTRL: negative voltage gamma control
 *
 * The command names are the same as those found in the datasheet to ease
 * looking up their semantics and usage.
 *
 * Note that the ST7789P3 display controller offers quite a few more commands
 * which have been omitted from this list as they are not used at the moment.
 * Furthermore, commands that are compliant with the MIPI DCS have been left
 * out as well to avoid duplicate entries.
 */
enum st7789p3_command {
	PORCTRL = 0xB2,
	GCTRL = 0xB7,
	VCOMS = 0xBB,
	VDVVRHEN = 0xC2,
	VRHS = 0xC3,
	VDVS = 0xC4,
	VCMOFSET = 0xC5,
	PWCTRL1 = 0xD0,
	PVGAMCTRL = 0xE0,
	NVGAMCTRL = 0xE1,
};

#define MADCTL_BGR BIT(3) /* bitmask for RGB/BGR order */
#define MADCTL_MV BIT(5) /* bitmask for page/column order */
#define MADCTL_MX BIT(6) /* bitmask for column address order */
#define MADCTL_MY BIT(7) /* bitmask for page address order */

/* 60Hz for 16.6ms, configured as 2*16.6ms */
#define PANEL_TE_TIMEOUT_MS 33

static struct completion panel_te; /* completion for panel TE line */
static int irq_te; /* Linux IRQ for LCD TE line */
static u8 *frame_buf;

static irqreturn_t panel_te_handler(int irq, void *data)
{
	complete(&panel_te);
	return IRQ_HANDLED;
}

/*
 * init_tearing_effect_line() - init tearing effect line.
 * @par: FBTFT parameter object.
 *
 * Return: 0 on success, or a negative error code otherwise.
 */
static int init_tearing_effect_line(struct fbtft_par *par)
{
	struct device *dev = par->info->device;
	struct gpio_desc *te;
	int rc, irq;

	te = gpiod_get_optional(dev, "te", GPIOD_IN);
	if (IS_ERR(te))
		return dev_err_probe(dev, PTR_ERR(te),
				     "Failed to request te GPIO\n");

	/* if te is NULL, indicating no configuration, directly return success */
	if (!te) {
		irq_te = 0;
		return 0;
	}

	irq = gpiod_to_irq(te);

	/* GPIO is locked as an IRQ, we may drop the reference */
	gpiod_put(te);

	if (irq < 0)
		return irq;

	irq_te = irq;
	init_completion(&panel_te);

	/* The effective state is high and lasts no more than 1000 microseconds */
	rc = devm_request_irq(dev, irq_te, panel_te_handler,
			      IRQF_TRIGGER_RISING, "TE_GPIO", par);
	if (rc)
		return dev_err_probe(dev, rc, "TE IRQ request failed.\n");

	disable_irq_nosync(irq_te);

	return 0;
}

/**
 * init_display() - initialize the display controller
 *
 * @par: FBTFT parameter object
 *
 * Most of the commands in this init function set their parameters to the
 * same default values which are already in place after the display has been
 * powered up. (The main exception to this rule is the pixel format which
 * would default to 18 instead of 16 bit per pixel.)
 * Nonetheless, this sequence can be used as a template for concrete
 * displays which usually need some adjustments.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int init_display(struct fbtft_par *par)
{
	struct device *dev = par->info->device;
	bool skip_init = dev->of_node &&
			 of_property_read_bool(dev->of_node, "skip-init");
	int rc;

	if (skip_init) {
		dev_info(dev, "skip panel reset and init sequence\n");
		par->fbtftops.set_gamma = NULL;
	} else {
		par->fbtftops.reset(par);
	}

	rc = init_tearing_effect_line(par);
	if (rc)
		return rc;

	if (skip_init)
		goto alloc_frame_buf;

	/* Sleep Out */
	write_reg(par, MIPI_DCS_EXIT_SLEEP_MODE);
	mdelay(120);

	/* Interface pixel format - 16bit 65K */
	write_reg(par, MIPI_DCS_SET_PIXEL_FORMAT, MIPI_DCS_PIXEL_FMT_16BIT);

	/* Porch control */
	write_reg(par, PORCTRL, 0x0C, 0x0C, 0x00, 0x33, 0x33);

	/* Display Inversion Off */
	write_reg(par, 0x20);

	/* Gate control */
	write_reg(par, GCTRL, 0x05);

	/* VCOMS Setting */
	write_reg(par, VCOMS, 0x21);

	/* LCM Control */
	write_reg(par, 0xC0, 0x2C);

	/* VDV and VRH Command Enable */
	write_reg(par, VDVVRHEN, 0x01);

	/* VRH Set */
	write_reg(par, VRHS, 0x15);

	/* FR Control 2 */
	write_reg(par, 0xC6, 0x0F);

	/* Power Control 1 */
	write_reg(par, PWCTRL1, 0xA7);
	write_reg(par, PWCTRL1, 0xA4, 0xA1);

	write_reg(par, 0xD6, 0xA1);

	write_reg(par, 0xE0, 0xF0, 0x05, 0x0E, 0x08, 0x0A, 0x17, 0x39, 0x54, 0x4E, 0x37, 0x12, 0x12, 0x31, 0x37);
	write_reg(par, 0xE1, 0xF0, 0x10, 0x14, 0x0D, 0x0B, 0x05, 0x39, 0x44, 0x4D, 0x38, 0x14, 0x14, 0x2E, 0x35);

	write_reg(par, 0xE4, 0x23, 0x00, 0x00);

	/* Display Inversion On */
	write_reg(par, MIPI_DCS_ENTER_INVERT_MODE);

	/* TE line output is off by default when powering on */
	if (irq_te)
		write_reg(par, MIPI_DCS_SET_TEAR_ON, 0x00);

	/* Display On */
	write_reg(par, MIPI_DCS_SET_DISPLAY_ON);

alloc_frame_buf:
	frame_buf = kzalloc(par->info->fix.smem_len, GFP_KERNEL);
	if (!frame_buf)
		return -ENOMEM;

	return 0;
}

static void memcpy_reverse32(u8 *dst, const u8 *src, size_t len)
{
	const u32 *src32 = (const u32 *)src;
	u32 *dst32 = (u32 *)dst;
	size_t i, count = len / 4;

	for (i = 0; i < count; i++)
		dst32[i] = swab32(src32[i]);
}

static int st7789p3_write_buf16_bus8(struct fbtft_par *par, u16 *src,
				     size_t len)
{
	__be16 *txbuf16 = par->txbuf.buf;
	size_t remain;
	size_t to_copy;
	size_t tx_array_size;
	int i;
	int ret = 0;
	size_t startbyte_size = 0;

	remain = len / 2;

	if (par->gpio.dc != -1)
		gpio_set_value(par->gpio.dc, 1);

	if (!par->txbuf.buf)
		return par->fbtftops.write(par, src, len);

	tx_array_size = par->txbuf.len / 2;

	if (par->startbyte) {
		txbuf16 = par->txbuf.buf + 1;
		tx_array_size -= 2;
		*(u8 *)(par->txbuf.buf) = par->startbyte | 0x2;
		startbyte_size = 1;
	}

	while (remain) {
		to_copy = min(tx_array_size, remain);
		for (i = 0; i < to_copy; i++)
			txbuf16[i] = cpu_to_be16(src[i]);

		src += to_copy;
		ret = par->fbtftops.write(par, par->txbuf.buf,
					  startbyte_size + to_copy * 2);
		if (ret < 0)
			return ret;
		remain -= to_copy;
	}

	return ret;
}

/*
 * write_vmem() - write data to display.
 * @par: FBTFT parameter object.
 * @offset: offset from screen_buffer.
 * @len: the length of data to be writte.
 *
 * Return: 0 on success, or a negative error code otherwise.
 */
static int write_vmem(struct fbtft_par *par, size_t offset, size_t len)
{
	struct device *dev = par->info->device;
	int ret;

	dev_dbg(dev, "write_vmem len: %zu\n", len);
	if (irq_te) {
		enable_irq(irq_te);
		reinit_completion(&panel_te);
		ret = wait_for_completion_timeout(
			&panel_te, msecs_to_jiffies(PANEL_TE_TIMEOUT_MS));
		if (ret == 0)
			dev_err(dev, "wait panel TE timeout\n");

		disable_irq(irq_te);
	}

	memcpy_reverse32(frame_buf + offset,
			 par->info->screen_buffer + offset, len);

	ret = st7789p3_write_buf16_bus8(par, (u16 *)(frame_buf + offset),
					len);

	return ret;
}

/**
 * set_var() - apply LCD properties like rotation and BGR mode
 *
 * @par: FBTFT parameter object
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int set_var(struct fbtft_par *par)
{
	u8 madctl_par = 0;

	if (par->bgr)
		madctl_par |= MADCTL_BGR;
	switch (par->info->var.rotate) {
	case 0:
		break;
	case 90:
		madctl_par |= (MADCTL_MV | MADCTL_MY);
		break;
	case 180:
		madctl_par |= (MADCTL_MX | MADCTL_MY);
		break;
	case 270:
		madctl_par |= (MADCTL_MV | MADCTL_MX);
		break;
	default:
		return -EINVAL;
	}
	write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, madctl_par);
	return 0;
}

/**
 * set_gamma() - set gamma curves
 *
 * @par: FBTFT parameter object
 * @curves: gamma curves
 *
 * Before the gamma curves are applied, they are preprocessed with a bitmask
 * to ensure syntactically correct input for the display controller.
 * This implies that the curves input parameter might be changed by this
 * function and that illegal gamma values are auto-corrected and not
 * reported as errors.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int set_gamma(struct fbtft_par *par, u32 *curves)
{
	int i;
	int j;
	int c; /* curve index offset */

	/*
	 * Bitmasks for gamma curve command parameters.
	 * The masks are the same for both positive and negative voltage
	 * gamma curves.
	 */
	static const u8 gamma_par_mask[] = {
		0xFF, /* V63[3:0], V0[3:0]*/
		0x3F, /* V1[5:0] */
		0x3F, /* V2[5:0] */
		0x1F, /* V4[4:0] */
		0x1F, /* V6[4:0] */
		0x3F, /* J0[1:0], V13[3:0] */
		0x7F, /* V20[6:0] */
		0x77, /* V36[2:0], V27[2:0] */
		0x7F, /* V43[6:0] */
		0x3F, /* J1[1:0], V50[3:0] */
		0x1F, /* V57[4:0] */
		0x1F, /* V59[4:0] */
		0x3F, /* V61[5:0] */
		0x3F, /* V62[5:0] */
	};

	for (i = 0; i < par->gamma.num_curves; i++) {
		c = i * par->gamma.num_values;
		for (j = 0; j < par->gamma.num_values; j++)
			curves[c + j] &= gamma_par_mask[j];
		write_reg(par, PVGAMCTRL + i, curves[c + 0], curves[c + 1],
			  curves[c + 2], curves[c + 3], curves[c + 4],
			  curves[c + 5], curves[c + 6], curves[c + 7],
			  curves[c + 8], curves[c + 9], curves[c + 10],
			  curves[c + 11], curves[c + 12], curves[c + 13]);
	}
	return 0;
}

/**
 * blank() - blank the display
 *
 * @par: FBTFT parameter object
 * @on: whether to enable or disable blanking the display
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int blank(struct fbtft_par *par, bool on)
{
	if (on)
		write_reg(par, MIPI_DCS_SET_DISPLAY_OFF);
	else
		write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	if (par->info->var.rotate == 180) {
		ys += 36;
		ye += 36;
	} else if (par->info->var.rotate == 90) {
		xs += 36;
		xe += 36;
	}

	write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS, (xs >> 8) & 0xFF, xs & 0xFF,
		  (xe >> 8) & 0xFF, xe & 0xFF);

	write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS, (ys >> 8) & 0xFF, ys & 0xFF,
		  (ye >> 8) & 0xFF, ye & 0xFF);

	write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = 240,
	.height = 284,
	.txbuflen = 32 * 1024,
	.gamma_num = 2,
	.gamma_len = 14,
	.gamma = DEFAULT_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.write_vmem = write_vmem,
		.set_var = set_var,
		.set_gamma = set_gamma,
		.blank = blank,
		.set_addr_win = set_addr_win,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "sitronix,st7789p3", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:st7789p3");
MODULE_ALIAS("platform:st7789p3");

MODULE_DESCRIPTION("FB driver for the ST7789P3 LCD Controller");
MODULE_AUTHOR("916BGAI,iawak9lkm");
MODULE_LICENSE("GPL");
