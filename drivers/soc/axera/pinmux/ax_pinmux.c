#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/device.h>
// ### SIPEED EDIT ###
#include <linux/irqflags.h>
// ### SIPEED EDIT END ###

#include "ax_pinmux.h"

static unsigned int ax620Q_EVB_pinmux[] = {
#include "AX620Q_EVB_pinmux.h"
};

static unsigned int ax620Q_Demo_pinmux[] = {
#include "AX620Q_DEMO_pinmux.h"
};

static unsigned int ax630C_EVB_pinmux[] = {
#include "AX630C_EVB_pinmux.h"
};

static unsigned int ax630C_Demo_pinmux[] = {
#include "AX630C_DEMO_pinmux.h"
};

static unsigned int ax620QZ_Demo_pinmux[] = {
#include "AX620QZ_DEMO_pinmux.h"
};

static struct pinmux ax620E_pinmux_tbl[AX620E_BOARD_MAX] = {
	[AX620Q_LP4_EVB_V1_0] =
	    {ax620Q_EVB_pinmux,
	     sizeof(ax620Q_EVB_pinmux) / sizeof(unsigned int)},
	[AX620Q_LP4_DEMO_V1_0] =
	    {ax620Q_Demo_pinmux,
	     sizeof(ax620Q_Demo_pinmux) / sizeof(unsigned int)},
	[AX630C_EVB_V1_0] =
	    {ax630C_EVB_pinmux,
	     sizeof(ax630C_EVB_pinmux) / sizeof(unsigned int)},
	[AX630C_DEMO_V1_0] =
	    {ax630C_Demo_pinmux,
	     sizeof(ax630C_Demo_pinmux) / sizeof(unsigned int)},
	[AX620QZ_DEMO_LP4_V1_0] =
	    {ax620QZ_Demo_pinmux,
	     sizeof(ax620QZ_Demo_pinmux) / sizeof(unsigned int)},
};

static int ax_get_board_id(void)
{
	int board_id;
	misc_info_t *regs;

	regs = (misc_info_t *)ioremap(MISC_INFO_ADDR, sizeof(misc_info_t));
	board_id = regs->board_id;
	iounmap((void *)regs);

	return board_id;
}

static int ax_pinmux_sleep_mode_en(void)
{
	void __iomem *regs = NULL;

	regs = ioremap(PIN_BASE_G6, 0x100);
	writel(BIT(PIN_GROUP_SLEEP_EN_SHIFT), regs + PIN_GROUP_SLEEP_EN_OFFECT);
	iounmap((void *)regs);
	return 0;
}

static int ax_pinmux_index_conv(int index)
{
	int ret;

	switch (index) {
// ### SIPEED EDIT ###
	case AX630C_AX631_MAIXCAM2_SOM_0_5G:
	case AX630C_AX631_MAIXCAM2_SOM_1G:
	case AX630C_AX631_MAIXCAM2_SOM_2G:
	case AX630C_AX631_MAIXCAM2_SOM_4G:
	case AX630C_DEMO_DDR3_V1_0:
	case AX630C_DEMO_LP4_V1_0:
		/* fall through */
	case AX630C_DEMO_V1_1:
		ret = AX630C_DEMO_V1_0;
		break;
	case AX620Q_LP4_DEMO_V1_1:
	case AX620Q_LP4_NANOAGENT_256M:
		ret = AX620Q_LP4_DEMO_V1_0;
		break;
	case AX620QF_LP4_NANOAGENT_256M:
	case AX620QE_LP4_NANOAGENT_512M:
		ret = AX620QZ_DEMO_LP4_V1_0;
		break;
	default :
		ret = index;
		break;
	}
// ### SIPEED EDIT END ###
	return ret;
}

static int ax_pin_init(struct platform_device *pdev)
{
	int i, ret = 0;
	int index = ax_get_board_id();
	u32 offset, base_reg = 0;
	u8 is_dphytx;
	void __iomem *reg = NULL;
	void __iomem *dphy_rst_set = NULL;
	void __iomem *dphy_mipi_en = NULL;

	// ### SIPEED EDIT ###
	int match_count = 0;
	int uart3_short = 0;
	int rx_high = 0;
	int rx_low = 0;
	unsigned long flags;
	u32 val;
	misc_info_t *info = NULL;
	void __iomem *gpio_reg = NULL;
	void __iomem *pin_reg = NULL;
	// ### SIPEED EDIT END ###

	if (index < 0 || index > AX620E_BOARD_MAX - 1) {
		ret = -EFAULT;
		goto end;
	}
	index = ax_pinmux_index_conv(index);

	// ### SIPEED EDIT ###
	info = (misc_info_t *)ioremap(MISC_INFO_ADDR, sizeof(misc_info_t));
	if (info &&
	    ((info->chip_type == 0x8 && info->board_id == 0x3 &&
	      info->phy_board_id == 0x3) ||
	     (info->chip_type == 0x9 && info->board_id == 0x1 &&
	      info->phy_board_id == 0x1))) {
		gpio_reg = ioremap(0x04801000, 0x100);
		pin_reg = ioremap(0x02304000, 0x100);
		if (gpio_reg && pin_reg) {
			val = readl(gpio_reg + 0x0c);
			val = (val | BIT(1)) & ~BIT(0);
			writel(val, gpio_reg + 0x0c);
			val = readl(gpio_reg + 0x10);
			writel(val & ~BIT(1), gpio_reg + 0x10);

			writel(0x00060043, pin_reg + 0x90);
			writel(0x00060003, pin_reg + 0x84);

			for (i = 0; i < 3; i++) {
				val = readl(gpio_reg + 0x0c);
				writel(val & ~BIT(0), gpio_reg + 0x0c);
				udelay(10);
				rx_low = !(readl(gpio_reg + 0x8c) & BIT(3));
				local_irq_save(flags);
				val = readl(gpio_reg + 0x0c);
				writel(val | BIT(0), gpio_reg + 0x0c);
				udelay(10);
				rx_high = !!(readl(gpio_reg + 0x8c) & BIT(3));
				val = readl(gpio_reg + 0x0c);
				writel(val & ~BIT(0), gpio_reg + 0x0c);
				local_irq_restore(flags);
				if (rx_low && rx_high)
					match_count++;
				if (i < 2)
					mdelay(30);
			}
			writel(0x00060043, pin_reg + 0x84);
			writel(0x00060043, pin_reg + 0x90);
			val = readl(gpio_reg + 0x0c);
			writel(val & ~BIT(1), gpio_reg + 0x0c);
			uart3_short = match_count == 3;
		}
	}
	if (info)
		iounmap((void *)info);
	if (gpio_reg)
		iounmap(gpio_reg);
	if (pin_reg)
		iounmap(pin_reg);
	// ### SIPEED EDIT END ###

	dphy_rst_set = ioremap(DPHYTX_SW_RST_SET, 0x4);
	if (!dphy_rst_set) {
		pr_err("%s:ioremap(DPHYTX_SW_RST_SET) failed\n", __func__);
		ret = - ENOMEM;
		goto end;
	}
	dphy_mipi_en = ioremap(DPHYTX_MIPI_EN, 0x4);
	if (!dphy_rst_set) {
		pr_err("%s:ioremap(DPHYTX_MIPI_EN) failed\n", __func__);
		ret = - ENOMEM;
		goto err_ioremap;
	}
	for (i = 0; i < ax620E_pinmux_tbl[index].size; i += 2) {
		// ### SIPEED EDIT ###
		if (uart3_short &&
		    (ax620E_pinmux_tbl[index].data[i] == 0x02304084 ||
		     ax620E_pinmux_tbl[index].data[i] == 0x02304090))
			continue;
		// ### SIPEED EDIT END ###

		is_dphytx =
		    ax620E_pinmux_tbl[index].data[i] - DPHYTX_BASE <
		    DPHY_REG_LEN ? 1 : 0;
		offset = ax620E_pinmux_tbl[index].data[i] - base_reg;
		if (!base_reg || offset >= REG_REMAP_SIZE) {
			base_reg = ax620E_pinmux_tbl[index].data[i] & (~0xfff);
			offset = ax620E_pinmux_tbl[index].data[i] - base_reg;
			if (reg)
				iounmap(reg);
			reg = ioremap(base_reg, REG_REMAP_SIZE);
			if (!reg) {
				pr_err("%s:ioremap(pinmux) failed\n", __func__);
				ret = - ENOMEM;
				goto err_ioremap;
			}
		}
		//when dphytx select gpio func 1.set reset 2.mipi disable 3.func sel & config
		if (is_dphytx
		    && (ax620E_pinmux_tbl[index].data[i + 1] & PINMUX_FUNC_SEL)) {
			writel(DPHYTX_SW_RST_SHIFT, dphy_rst_set);
			writel(0, dphy_mipi_en);
		}
		writel(ax620E_pinmux_tbl[index].data[i + 1], reg + offset);
	}
err_ioremap:
	if (reg)
		iounmap(reg);
	if (dphy_rst_set)
		iounmap(dphy_rst_set);
	if (dphy_mipi_en)
		iounmap(dphy_mipi_en);
end:
	pr_debug("ax_pin_init ret %d index %d\n", ret, index);
	return ret;
}

static int ax_pinmux_probe(struct platform_device *pdev)
{
	int ret;

	ax_pinmux_sleep_mode_en();
	ret = ax_pin_init(pdev);
	if (ret)
		pr_err("err: ax_pin_init failed!\n");
	return ret;
}

static int ax_pinmux_remove(struct platform_device *pdev)
{
	return 0;
}

static void ax_pinmux_release(struct device *dev)
{
	return;
}

static struct platform_device ax_pinmux_dev = {
	.name = "ax_pinmux",
	.id = PLATFORM_DEVID_AUTO,
	.dev = {
		.release=ax_pinmux_release,
	},
};

static struct platform_driver ax_pinmux_drv = {
	.probe = ax_pinmux_probe,
	.remove = ax_pinmux_remove,
	.driver = {
		.name = "ax_pinmux",
	},
};

static int __init ax_pinmux_init(void)
{
	platform_device_register(&ax_pinmux_dev);
	return platform_driver_register(&ax_pinmux_drv);
}

static void ax_pinmux_exit(void)
{
	platform_device_unregister(&ax_pinmux_dev);
	platform_driver_unregister(&ax_pinmux_drv);
}

arch_initcall(ax_pinmux_init);
module_exit(ax_pinmux_exit);
MODULE_DESCRIPTION("axera ax_pinmux driver");
MODULE_AUTHOR("Axera Inc.");
MODULE_LICENSE("GPL v2");
