/*
 * SD card slot of Xiongmai's V200 series (XM530V200, XM533, XM535)
 * for the arasan driver.
 *
 * Their vendor sdio0_sd.ko / sdio1_sd.ko cannot be used here: the arasan
 * driver of their kernels takes a longer platform data (a card-detect GPIO
 * and its poll period before use_pio, a hw_reset flag after p_powerup), so
 * on this kernel's driver the probe calls a NULL p_powerup. This module
 * registers the same device with this kernel's arasan_platform_data and
 * does what those modules' powerup does, which is per chip:
 *
 *  - power the card off through its power pad (pdn), with the bus pads
 *    as GPIO inputs, for 300 ms;
 *  - mux the clock, command and four data pads to the controller at the
 *    drive strength asked (clk, sdiocur), and the card-detect pad when
 *    detect is set;
 *  - program the clock and data phase registers (clk_phase, data_phase);
 *  - power the card on and wait 20 ms, then reset the controller through
 *    its system control register.
 *
 * Parameters keep the vendor modules' names and meanings: detect, sdiocur
 * and clk (drive in mA: 2, 4, 8 or 12), pdn (power pad, active low; a
 * negative pad number is active high; 0 for none), bit (bus width, 4 or
 * 1), clk_phase and data_phase (-1 for the chip's default), detect_gpio
 * and hw_reset (accepted; this kernel's arasan driver has no use for
 * them). chip selects the table and defaults to this build's machine.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/arasan_plat.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include "arasan.h"

#define XM_PAD_GPIO_IN		0x1000		/* GPIO_IN_EN */
#define XM_PAD_OUT_HIGH		0xc00		/* GPIO_OUT_EN | GPIO_OUT_HIGH */
#define XM_PAD_OUT_LOW		0x400		/* GPIO_OUT_EN | GPIO_OUT_LOW */
#define XM_PAD(n)		((n) * 4)
#define XM_PADS			1024	/* pad registers in the 4 KiB at GPIO_BASE */

struct xm_sd_chip {
	const char *name;
	int slot;
	resource_size_t base;
	int irq;
	unsigned int clk_pad, cmd_pad, data_pad[4];
	unsigned int cd_pad;
	u32 mux;			/* the pads' controller function */
	u32 drive_default;		/* drive bits for a value out of range */
	unsigned int clk_phase_reg;
	u32 clk_phase_default;
	unsigned int clk_phase_reg2;	/* also set to the default, or 0 */
	unsigned int data_phase_reg;	/* four registers from here */
	u32 data_phase_default;
	int pad_2a0;			/* 1: set +0x2a0 to 0x80; 2: if muxed */
	unsigned int rst_reg;		/* SYS register, 0x5d10xxxx then 1 */
	u32 rst_val;
	bool rst_unlock;		/* REG_SYS_SOFT_RSTEN around it */
};

static const struct xm_sd_chip xm_sd_chips[] = {
	{	/* sdio0_sd.ko of an X6C-WEQ, V5.04.R02.20241125 */
		.name = "xm530v200", .slot = 0,
		.base = 0x50000000, .irq = 73,
		.clk_pad = 160, .cmd_pad = 161, .data_pad = { 162, 163, 164, 165 },
		.cd_pad = 166,
		.mux = GPIO_MUX1_EN, .drive_default = GPIO_DRIVE_8MA,
		.clk_phase_reg = 0xc30, .clk_phase_default = 0x1a0,
		.data_phase_reg = 0xca0, .data_phase_default = 0x10,
		.pad_2a0 = 1,
		.rst_reg = 0x108, .rst_val = 0x5d101000,
	},
	{	/* sdio1_sd.ko of an X3-WQ-B, V5.04.R02.20250828 */
		.name = "xm533", .slot = 1,
		.base = 0x50200000, .irq = 74,
		.clk_pad = 84, .cmd_pad = 85, .data_pad = { 80, 81, 82, 83 },
		.cd_pad = 92,
		.mux = GPIO_MUX3_EN, .drive_default = GPIO_DRIVE_8MA,
		.clk_phase_reg = 0xc34, .clk_phase_default = 0x1a0,
		.data_phase_reg = 0xcb0, .data_phase_default = 0x10,
		.rst_reg = 0x110, .rst_val = 0x5d103000, .rst_unlock = true,
	},
	{	/* sdio0_sd.ko of an X6E-WEQ (D1), V5.07.X02.20250906 */
		.name = "xm535", .slot = 0,
		.base = 0x50000000, .irq = 73,
		.clk_pad = 160, .cmd_pad = 161, .data_pad = { 162, 163, 164, 165 },
		.cd_pad = 166,
		.mux = GPIO_MUX1_EN, .drive_default = GPIO_DRIVE_4MA,
		.clk_phase_reg = 0xc30, .clk_phase_default = 8,
		.clk_phase_reg2 = 0xc38,
		.data_phase_reg = 0xca0, .data_phase_default = 0,
		.pad_2a0 = 2,
		.rst_reg = 0x108, .rst_val = 0x5d101000,
	},
};

static char *chip = CONFIG_XM530_MACHINE_NAME;
static int detect;
static int sdiocur;
static int clk;
static int pdn;
static int bit = 4;
static int clk_phase = -1;
static int data_phase = -1;
static int detect_gpio = -1;
static int hw_reset = 1;
module_param(chip, charp, S_IRUGO);
module_param(detect, int, S_IRUGO);
module_param(sdiocur, int, S_IRUGO);
module_param(clk, int, S_IRUGO);
module_param(pdn, int, S_IRUGO);
module_param(bit, int, S_IRUGO);
module_param(clk_phase, int, S_IRUGO);
module_param(data_phase, int, S_IRUGO);
module_param(detect_gpio, int, S_IRUGO);
module_param(hw_reset, int, S_IRUGO);
MODULE_PARM_DESC(chip, "xm530v200, xm533 or xm535");
MODULE_PARM_DESC(pdn, "card power pad, active low; negative: active high; 0: none");

static const struct xm_sd_chip *xm_sd;
static void __iomem *xm_pads;
static void __iomem *xm_sys;

static u32 xm_sd_drive(int ma)
{
	switch (ma) {
	case 2:
		return GPIO_DRIVE_2MA;
	case 4:
		return GPIO_DRIVE_4MA;
	case 8:
		return GPIO_DRIVE_8MA;
	case 12:
		return GPIO_DRIVE_12MA;
	default:
		return xm_sd->drive_default;
	}
}

static void xm_sd_power(bool on)
{
	unsigned int pad = pdn > 0 ? pdn : -pdn;
	bool high = pdn > 0 ? !on : on;

	writel(high ? XM_PAD_OUT_HIGH : XM_PAD_OUT_LOW, xm_pads + XM_PAD(pad));
}

static void xm_sd_powerup(void)
{
	const struct xm_sd_chip *c = xm_sd;
	u32 data = xm_sd_drive(sdiocur) | c->mux;
	u32 clkpad = xm_sd_drive(clk) | c->mux;
	int i;

	if (pdn) {
		xm_sd_power(false);
		writel(XM_PAD_GPIO_IN, xm_pads + XM_PAD(c->clk_pad));
		writel(XM_PAD_GPIO_IN, xm_pads + XM_PAD(c->cmd_pad));
		for (i = 0; i < 4; i++)
			writel(XM_PAD_GPIO_IN, xm_pads + XM_PAD(c->data_pad[i]));
		msleep(300);
	}

	writel(clkpad, xm_pads + XM_PAD(c->clk_pad));
	writel(data, xm_pads + XM_PAD(c->cmd_pad));
	for (i = 0; i < 4; i++)
		writel(data, xm_pads + XM_PAD(c->data_pad[i]));

	if (clk_phase != -1) {
		writel(clk_phase, xm_pads + c->clk_phase_reg);
	} else {
		writel(c->clk_phase_default, xm_pads + c->clk_phase_reg);
		if (c->clk_phase_reg2)
			writel(c->clk_phase_default, xm_pads + c->clk_phase_reg2);
	}
	for (i = 0; i < 4; i++)
		writel(data_phase != -1 ? data_phase : c->data_phase_default,
		       xm_pads + c->data_phase_reg + i * 4);

	/*
	 * XM535's vendor module leaves this pad alone when given detect_gpio,
	 * because it then polls that GPIO instead. This module does not, so the
	 * controller's card detect is all there is, and its pad is always muxed.
	 */
	if (detect)
		writel(GPIO_DRIVE_8MA | c->mux, xm_pads + XM_PAD(c->cd_pad));

	if (c->pad_2a0 == 1 ||
	    (c->pad_2a0 == 2 && (readl(xm_pads + 0x2a0) & GPIO_MUX1_EN)))
		writel(GPIO_DRIVE_8MA, xm_pads + 0x2a0);

	if (pdn) {
		xm_sd_power(true);
		msleep(20);
	}

	if (c->rst_unlock)
		writel(1, xm_sys + REG_SYS_SOFT_RSTEN);
	writel(c->rst_val, xm_sys + c->rst_reg);
	msleep(1);
	writel(1, xm_sys + c->rst_reg);
	if (c->rst_unlock)
		writel(0, xm_sys + REG_SYS_SOFT_RSTEN);
	msleep(1);
}

static struct arasan_platform_data xm_sd_pdata = {
	.need_poll = 0,
	.use_pio = 0,
	.card_irq = 0,
	.auto_cmd12 = 1,
	.p_powerup = xm_sd_powerup,
};

static struct resource xm_sd_resource[2];

static void xm_sd_release(struct device *dev)
{
}

static struct platform_device xm_sd_device = {
	.name = ARASAN_DRIVER_NAME,
	.num_resources = ARRAY_SIZE(xm_sd_resource),
	.resource = xm_sd_resource,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &xm_sd_pdata,
		.release = xm_sd_release,
	},
};

static int __init xm_sd_init(void)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(xm_sd_chips); i++)
		if (!strcmp(chip, xm_sd_chips[i].name))
			xm_sd = &xm_sd_chips[i];
	if (!xm_sd) {
		pr_err("xm_v200_sd: no SD slot known for chip %s\n", chip);
		return -ENODEV;
	}
	/* pdn is a pad number, and its register must fall inside xm_pads */
	if (pdn <= -XM_PADS || pdn >= XM_PADS) {
		pr_err("xm_v200_sd: pdn %d is not a pad\n", pdn);
		return -EINVAL;
	}
	if (detect_gpio != -1)
		pr_warn("xm_v200_sd: detect_gpio is not supported, using the controller's card detect\n");

	xm_pads = ioremap(GPIO_BASE, XM_PAD(XM_PADS));
	xm_sys = ioremap(SYS_CTRL_BASE, 0x1000);
	if (!xm_pads || !xm_sys) {
		ret = -ENOMEM;
		goto unmap;
	}

	xm_sd_pdata.need_detect = !!detect;
	xm_sd_pdata.sdio_4bit_data = bit == 4;

	xm_sd_resource[0].name = "sd_addr";
	xm_sd_resource[0].start = xm_sd->base;
	xm_sd_resource[0].end = xm_sd->base + 0x100000 - 1;
	xm_sd_resource[0].flags = IORESOURCE_MEM;
	xm_sd_resource[1].name = "sd_irq";
	xm_sd_resource[1].start = xm_sd->irq;
	xm_sd_resource[1].end = xm_sd->irq;
	xm_sd_resource[1].flags = IORESOURCE_IRQ;
	xm_sd_device.id = xm_sd->slot;

	ret = platform_device_register(&xm_sd_device);
	if (!ret)
		return 0;
unmap:
	if (xm_sys)
		iounmap(xm_sys);
	if (xm_pads)
		iounmap(xm_pads);
	return ret;
}

static void __exit xm_sd_exit(void)
{
	platform_device_unregister(&xm_sd_device);
	/* the arasan driver's remove only powers down the controller itself */
	if (pdn)
		xm_sd_power(false);
	iounmap(xm_sys);
	iounmap(xm_pads);
}

module_init(xm_sd_init);
module_exit(xm_sd_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SD slot of the XM530V200, XM533 and XM535 for the arasan driver");
