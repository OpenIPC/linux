/*
 * SDIO Wi-Fi slot of Xiongmai's XM533 (SDIO0, 0x50000000) for the arasan
 * driver. XM533 boards wire their SDIO Wi-Fi (the SSV6020 "SSV6158M") to
 * SDIO0 and their card slot to SDIO1 (xm_v200_sd). A module of its own,
 * because the Wi-Fi profile (/etc/wireless/sdio) loads it before
 * load_xiongmai loads the card slot's.
 *
 * Its vendor wifi_pdn.ko registers the slot with its own kernel's arasan
 * platform data, laid out differently from this kernel's (see xm_v200_sd),
 * and powers it up from the probe. This does the same, read from that
 * module:
 *
 *  - set the SDIO clock: the divider of the PLL SYS +0x30 bits 17..18
 *    select (SYS +0x08 + 8 * n control, +0x0C + 8 * n fraction) down to
 *    clk MHz, written with 0xb00 to SYS +0x3C, with SYS +0x00 unlocked;
 *  - leave the bus pads (160..165) as GPIO, write the clock phase to
 *    pad block +0xC30;
 *  - power the chip off through its power pad, unless it already is, and
 *    on again, 10 ms each (pdn: pad, active low; negative: active high);
 *  - mux the bus pads to the controller (0x102).
 *
 * Parameters keep the vendor's names: value (the power pad), phase, clk.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/arasan_plat.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include "arasan.h"

#define XM_PAD(n)		((n) * 4)
#define XM_PADS			1024	/* pad registers in the 4 KiB at GPIO_BASE */
#define XM_PAD_OUT_EN		0x400
#define XM_PAD_OUT_HIGH		0x800
#define XM_WIFI_BUS_PAD		160	/* clock, command, data 0..3: 160..165 */
#define XM_WIFI_BUS_MUX		0x102
#define XM_WIFI_PHASE_REG	0xc30

#define SYS_LOCK		0x00
#define SYS_PLL_SEL		0x30
#define SYS_SDIO0_CLK		0x3c

static int value = 98;
static int phase = 0x80;
static int clk = 25;
module_param(value, int, S_IRUGO);
MODULE_PARM_DESC(value, "power pad, active low; negative: active high");
module_param(phase, int, S_IRUGO);
MODULE_PARM_DESC(phase, "clock phase (pad block +0xC30)");
module_param(clk, int, S_IRUGO);
MODULE_PARM_DESC(clk, "SDIO clock, MHz");

static void __iomem *xm_pads;
static void __iomem *xm_sys;

/* The SDIO clock divider, as the vendor module derives it from the PLL */
static void xm_wifi_set_clock(void)
{
	unsigned int sel = (readl(xm_sys + SYS_PLL_SEL) >> 17) & 3;
	u32 frac = readl(xm_sys + 0x0c + 8 * sel);
	u32 ctrl = readl(xm_sys + 0x08 + 8 * sel);
	/* the PLL in units of 10 Hz: 24 MHz times int.frac, plus 384 MHz */
	u64 f = (((u64)(frac & 0xffff) * 2400000) >> 16) +
		(u64)((frac >> 16) & 0x3ff) * 2400000 + 38400000;
	u32 div;

	do_div(f, ((ctrl >> 9) & 0x3f) + 1);
	do_div(f, (ctrl & 0xf) ? 4 : 2);
	div = (u32)f / (clk * 100000) - 1;

	writel(1, xm_sys + SYS_LOCK);
	writel(div | 0xb00, xm_sys + SYS_SDIO0_CLK);
	writel(0, xm_sys + SYS_LOCK);
	pr_info("xm_v200_sdio_wifi: %u Hz, div 0x%x\n", (u32)f * 10, div);
}

static void xm_wifi_powerup(void)
{
	unsigned int pad = value < 0 ? -value : value;
	u32 on = value < 0 ? XM_PAD_OUT_EN | XM_PAD_OUT_HIGH : XM_PAD_OUT_EN;
	u32 off = value < 0 ? XM_PAD_OUT_EN : XM_PAD_OUT_EN | XM_PAD_OUT_HIGH;
	u32 cur;
	int i;

	xm_wifi_set_clock();

	for (i = 0; i < 6; i++)
		writel(0, xm_pads + XM_PAD(XM_WIFI_BUS_PAD + i));
	writel(phase, xm_pads + XM_WIFI_PHASE_REG);

	cur = readl(xm_pads + XM_PAD(pad));
	if (!(cur & XM_PAD_OUT_EN) || (cur & XM_PAD_OUT_HIGH) != (off & XM_PAD_OUT_HIGH)) {
		writel(off, xm_pads + XM_PAD(pad));
		msleep(10);
	}
	writel(on, xm_pads + XM_PAD(pad));
	msleep(10);

	for (i = 0; i < 6; i++)
		writel(XM_WIFI_BUS_MUX, xm_pads + XM_PAD(XM_WIFI_BUS_PAD + i));
}

static struct arasan_platform_data xm_wifi_pdata = {
	.card_irq = 1,
	.sdio_4bit_data = 1,
	.p_powerup = xm_wifi_powerup,
};

static struct resource xm_wifi_resource[] = {
	{
		.name = "sdio0_addr",
		.start = 0x50000000,
		.end = 0x50000000 + 0x100000 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "sdio0_irq",
		.start = 73,
		.end = 73,
		.flags = IORESOURCE_IRQ,
	},
};

static void xm_wifi_release(struct device *dev)
{
}

static struct platform_device xm_wifi_device = {
	.name = ARASAN_DRIVER_NAME,
	.id = 0,
	.num_resources = ARRAY_SIZE(xm_wifi_resource),
	.resource = xm_wifi_resource,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &xm_wifi_pdata,
		.release = xm_wifi_release,
	},
};

static int __init xm_wifi_init(void)
{
	int ret;

	/* value is a pad number, and its register must fall inside xm_pads */
	if (value <= -XM_PADS || value >= XM_PADS || clk <= 0) {
		pr_err("xm_v200_sdio_wifi: value %d or clk %d out of range\n",
		       value, clk);
		return -EINVAL;
	}

	xm_pads = ioremap(GPIO_BASE, XM_PAD(XM_PADS));
	xm_sys = ioremap(SYS_CTRL_BASE, 0x1000);
	if (!xm_pads || !xm_sys) {
		ret = -ENOMEM;
		goto unmap;
	}

	ret = platform_device_register(&xm_wifi_device);
	if (!ret)
		return 0;
unmap:
	if (xm_sys)
		iounmap(xm_sys);
	if (xm_pads)
		iounmap(xm_pads);
	return ret;
}

static void __exit xm_wifi_exit(void)
{
	unsigned int pad = value < 0 ? -value : value;

	platform_device_unregister(&xm_wifi_device);
	/* power the chip off, as the arasan driver's remove does not */
	writel(value < 0 ? XM_PAD_OUT_EN : XM_PAD_OUT_EN | XM_PAD_OUT_HIGH,
	       xm_pads + XM_PAD(pad));
	iounmap(xm_sys);
	iounmap(xm_pads);
}

module_init(xm_wifi_init);
module_exit(xm_wifi_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SDIO Wi-Fi slot of the XM533 for the arasan driver");
