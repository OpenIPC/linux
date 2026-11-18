// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the GC9307 LCD Controller
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME "fb_gc9307"

#define GC9307_IPS_GAMMA \
    "04 07 07 07 05 26\n" \
    "3b 71 74 26 28 69\n" \
    "04 07 07 07 05 26\n" \
    "3b 71 74 26 28 6C"

static int init_display(struct fbtft_par *par)
{
	par->fbtftops.reset(par);
	mdelay(50);

    write_reg(par, 0xfe);
    write_reg(par, 0xef);
    // write_reg(par, 0x36, 0x48);
    // write_reg(par, 0x3a, 0x05);
    write_reg(par, 0x3a, 0x55);

    write_reg(par, 0xe8, 0x10, 0x00); // 87.5367647059 fps

    write_reg(par, 0xec, 0x33, 0x06, 0x22);
    write_reg(par, 0xc3, 0x20);
    write_reg(par, 0xc4, 0x30);
    write_reg(par, 0xc9, 0x2A);
    write_reg(par, 0xa7, 0x2b, 0x2b);

    /* gamma */
    write_reg(par, 0xF0, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    write_reg(par, 0xF1, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x69);
    write_reg(par, 0xF2, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    write_reg(par, 0xF3, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x6C);

    /* addr win */
    write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
    write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
    write_reg(par, 0x2c);

    // write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
    // write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
    // write_reg(par, 0x2c);
    // write_reg(par, 0xF0, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    // write_reg(par, 0xF1, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x69);
    // write_reg(par, 0xF2, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    // write_reg(par, 0xF3, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x6C);
    // write_reg(par, 0x35, 0x00);
    // write_reg(par, 0x44, 0x00, 0x0a);
    write_reg(par, 0x11);
    mdelay(120);
    write_reg(par, 0x2c);
    write_reg(par, 0x29);
    mdelay(120);

    return 0;
}

static int set_var(struct fbtft_par *par)
{
	struct device *dev = par->info->device;

	#define MADCTL_MH	BIT(2)
    #define MADCTL_BGR	BIT(3)
	#define MADCTL_ML	BIT(4)
    #define MADCTL_MV	BIT(5)
    #define MADCTL_MX	BIT(6)
    #define MADCTL_MY	BIT(7)

    u8 madctl_par = 0;
	// madctl_par |= MADCTL_MH;
	// madctl_par |= MADCTL_ML;
	
	if (!par->bgr)
		madctl_par |= MADCTL_BGR;
	switch (par->info->var.rotate) {
	case 0:
		break;
	case 90:
		madctl_par |= (MADCTL_MV);
		break;
	case 180:
		madctl_par |= (MADCTL_MX | MADCTL_MY);
		break;
	case 270:
		madctl_par |= (MADCTL_MV | MADCTL_MX | MADCTL_MY);
		break;
	default:
		return -EINVAL;
	}
	write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, madctl_par);

	dev_info(dev, "MADCTL: 0x%x\n", madctl_par);

    return 0;
}

static int set_gamma(struct fbtft_par *par, u32 *curves)
{
	(void)par;
	(void)curves;

    write_reg(par, 0xF0, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    write_reg(par, 0xF1, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x69);
    write_reg(par, 0xF2, 0x04, 0x07, 0x07, 0x07, 0x05, 0x26);
    write_reg(par, 0xF3, 0x3B, 0x71, 0x74, 0x26, 0x28, 0x6C);

	return 0;
}

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
    switch (par->info->var.rotate) {
	case 0:
        write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 90:
		write_reg(par, 0x2b, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2a, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 180:
		write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 270:
		write_reg(par, 0x2b, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2a, 0x00, 0x00, 0x01, 0x3f);
		break;
	default:
		return;
	}
    
    write_reg(par, 0x2c);
}

// static int write_vmem(struct fbtft_par *par, size_t offset, size_t len)
// {
// 	struct device *dev = par->info->device;
// 	int ret;

// 	dev_err(dev, "write_vmem len: %ld\n", len);

// 	switch (par->pdata->display.buswidth) {
// 	case 8:
// 		ret = fbtft_write_vmem16_bus8(par, offset, len);
// 		break;
// 	case 9:
// 		ret = fbtft_write_vmem16_bus9(par, offset, len);
// 		break;
// 	case 16:
// 		ret = fbtft_write_vmem16_bus16(par, offset, len);
// 		break;
// 	default:
// 		dev_err(dev, "Unsupported buswidth %d\n",
// 			par->pdata->display.buswidth);
// 		ret = 0;
// 		break;
// 	}

// 	return ret;
// }


static struct fbtft_display display = {
	.regwidth = 8,
	.width = 172,
	.height = 320,
	.gamma_num = 4,
	.gamma_len = 6,
	.gamma = GC9307_IPS_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.set_var = set_var,
		.set_gamma = set_gamma,
		.blank = blank,
        .set_addr_win = set_addr_win,
        // .write_vmem = fbtft_write_vmem16_bus8,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "sitronix,gc9307", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:gc9307");
MODULE_ALIAS("platform:gc9307");

MODULE_DESCRIPTION("FB driver for the GC9307 LCD Controller");
MODULE_AUTHOR("iawak9lkm");
MODULE_LICENSE("GPL");
