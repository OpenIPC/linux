/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "ax_cipher_adapt.h"
#include "eip130_drv.h"
#include "ax_cipher_ioctl.h"
#include "ax_cipher_crypto.h"
#include "ax_cipher_proc.h"
//#include "ax_printk.h"

#if defined(CONFIG_AX_CIPHER_CRYPTO) || defined(CONFIG_AX_CIPHER_MISC)

#define DRIVER_NAME "ax_cipher"
//#define USE_CLK_RST_FRAMEWORK
#define PERI_SYS_GLB					(0x4870000)
#define PERI_SYS_GLB_CLK_MUX0_SET	   (0xA8)
#define PERI_SYS_GLB_CLK_MUX0_CLR	   (0xAC)
#define PERI_SYS_GLB_CLK_EB0_SET		(0xB0)
#define PERI_SYS_GLB_CLK_EB0_CLR		(0xB4)
#define PERI_SYS_GLB_CLK_EB1_SET		(0xB8)
#define PERI_SYS_GLB_CLK_EB1_CLR		(0xBC)
#define PERI_SYS_GLB_CLK_EB2_SET		(0xC0)
#define PERI_SYS_GLB_CLK_EB2_CLR		(0xC4)
#define PERI_SYS_GLB_CLK_EB3_SET		(0xC8)
#define PERI_SYS_GLB_CLK_EB3_CLR		(0xCC)
#define PERI_SYS_GLB_CLK_RST0_SET	   (0xD8)
#define PERI_SYS_GLB_CLK_RST0_CLR	   (0xDC)
#define PERI_SYS_GLB_CLK_RST1_SET	   (0xE0)
#define PERI_SYS_GLB_CLK_RST1_CLR	   (0xE4)
#define PERI_SYS_GLB_CLK_RST2_SET	   (0xE8)
#define PERI_SYS_GLB_CLK_RST2_CLR	   (0xEC)
#define PERI_SYS_GLB_CLK_RST3_SET	   (0xF0)
#define PERI_SYS_GLB_CLK_RST3_CLR	   (0xF4)



typedef struct {
	void *reg_base;
	void *vir_addr;
	void *base_addr;
	uint32_t irq;
#ifdef USE_CLK_RST_FRAMEWORK
	struct clk *pclk_clk;
	struct clk *core_clk;
	struct clk *cnt_clk;
	struct reset_control *main_sw_rst;
	struct reset_control *cnt_sw_rst;
	struct reset_control *soft_sw_rst;
	struct reset_control *sw_prst;
	struct reset_control *sw_rst;
#else
	void *peri_base;
#endif
#ifdef CONFIG_AX_CIPHER_CRYPTO
	ax_cipher_crypto_priv_s ax_cipher_crypto_priv;
#endif
} ax_cipher_dev;
ax_cipher_dev cipher_dev;

static irqreturn_t cipher_irq_handler(int irq, void *dev_id)
{
	eip130_Interrupt_handle();
	return (s32)IRQ_HANDLED;
}

#ifdef CONFIG_AX_CIPHER_MISC
static int ax_cipher_open(struct inode *node, struct file* fp)
{
	return 0;
}

static int ax_cipher_release(struct inode *node, struct file *fp)
{
	return 0;
}

static long ax_cipher_ioctl(struct file *fp, unsigned int cmd,  unsigned long arg)
{
	return cipher_ioctl(cmd, (void *) arg);
}

static struct file_operations ax_ce_fops = {
	.open		   = ax_cipher_open,
	.release		= ax_cipher_release,
	.unlocked_ioctl = ax_cipher_ioctl,
};

static struct miscdevice g_ce_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ax_cipher",
	.fops = &ax_ce_fops
};

static int cipher_dev_register(void)
{
	int ret;
	ret = misc_register(&g_ce_dev);
	if (ret < 0) {
		CE_LOG_PR(CE_ERR_LEVEL, "register failed.\n");
		return -1;
	}
	return 0;
}

static void cipher_dev_release(void)
{
	misc_deregister(&g_ce_dev);
}
#endif

static int cipher_clk_enable(struct device *dev, ax_cipher_dev *pcipher_dev)
{
#ifdef USE_CLK_RST_FRAMEWORK
	int ret;

	pcipher_dev->pclk_clk = devm_clk_get(dev, "pclk");
	if (IS_ERR(pcipher_dev->pclk_clk)) {
		ret = PTR_ERR(pcipher_dev->pclk_clk);
		if (ret != -EPROBE_DEFER)
			CE_LOG_PR(CE_ERR_LEVEL, "failed to get cipher pclk clk: %d\n",
					  ret);
		return ret;
	}
	ret = clk_prepare_enable(pcipher_dev->pclk_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable pclk clock\n");
		return ret;
	}

	pcipher_dev->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(pcipher_dev->core_clk)) {
		ret = PTR_ERR(pcipher_dev->core_clk);
		if (ret != -EPROBE_DEFER)
			CE_LOG_PR(CE_ERR_LEVEL, "failed to get cipher core clk: %d\n",
					  ret);
		return ret;
	}
	ret = clk_prepare_enable(pcipher_dev->core_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable core clock\n");
		return ret;
	}
	pcipher_dev->cnt_clk = devm_clk_get(dev, "cnt");
	if (IS_ERR(pcipher_dev->cnt_clk)) {
		ret = PTR_ERR(pcipher_dev->cnt_clk);
		if (ret != -EPROBE_DEFER)
			CE_LOG_PR(CE_ERR_LEVEL, "failed to get cipher cnt clk: %d\n",
					  ret);
		return ret;
	}
	ret = clk_prepare_enable(pcipher_dev->cnt_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable cnt clock\n");
		return ret;
	}
#else
	// enalbe ce clks
	//cnt_clk
	ax_cipher_regwrite(0x1 << 0, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB0_SET);
	//aclk
	ax_cipher_regwrite(0x1 << 1, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB1_SET);
	// pclk
	ax_cipher_regwrite(0x1 << 12, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB2_SET);
	// sel clk_ce_bus_sel to 416M [1:0] 2'b10: cpll_312m
	ax_cipher_regwrite(0x1 << 1, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_MUX0_SET);
#endif
	return 0;
}


static void cipher_reset(struct device *dev, ax_cipher_dev *pcipher_dev)
{
#ifdef USE_CLK_RST_FRAMEWORK
	pcipher_dev->main_sw_rst = devm_reset_control_get_optional(dev, "main_sw_rst");
	if (IS_ERR(pcipher_dev->main_sw_rst)) {
		CE_LOG_PR(CE_ERR_LEVEL, "can't get main_sw_rst");
		return;
	}
	pcipher_dev->cnt_sw_rst = devm_reset_control_get_optional(dev, "cnt_sw_rst");
	if (IS_ERR(pcipher_dev->cnt_sw_rst)) {
		CE_LOG_PR(CE_ERR_LEVEL, "can't get cnt_sw_rst");
		return;
	}
	pcipher_dev->soft_sw_rst = devm_reset_control_get_optional(dev, "soft_sw_rst");
	if (IS_ERR(pcipher_dev->soft_sw_rst)) {
		CE_LOG_PR(CE_ERR_LEVEL, "can't get soft_sw_rst");
		return;
	}
	pcipher_dev->sw_prst = devm_reset_control_get_optional(dev, "sw_prst");
	if (IS_ERR(pcipher_dev->sw_prst)) {
		CE_LOG_PR(CE_ERR_LEVEL, "can't get sw_prst");
		return;
	}
	pcipher_dev->sw_rst = devm_reset_control_get_optional(dev, "sw_rst");
	if (IS_ERR(pcipher_dev->sw_rst)) {
		CE_LOG_PR(CE_ERR_LEVEL, "can't get sw_rst");
		return;
	}

	reset_control_deassert(pcipher_dev->main_sw_rst);
	reset_control_deassert(pcipher_dev->cnt_sw_rst);
	reset_control_deassert(pcipher_dev->soft_sw_rst);
	reset_control_deassert(pcipher_dev->sw_prst);
	reset_control_deassert(pcipher_dev->sw_rst);
#else
	// release ce rst
	// mam_rst
	ax_cipher_regwrite(0x1 << 4, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_RST0_CLR);
	//soft_rst, prst, arst, cnt_rst
	ax_cipher_regwrite(0xf << 5, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_RST0_CLR);
#endif
}

int ce_pm_suspend(struct device *dev)
{
	ax_cipher_dev *pcipher_dev = &cipher_dev;
	CE_LOG_PR(CE_ERR_LEVEL, "start");
#ifdef USE_CLK_RST_FRAMEWORK
	clk_disable_unprepare(pcipher_dev->pclk_clk);
	clk_disable_unprepare(pcipher_dev->core_clk);
	clk_disable_unprepare(pcipher_dev->cnt_clk);
#else
	//cnt_clk
	ax_cipher_regwrite(0x1 << 0, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB0_CLR);
	//aclk
	ax_cipher_regwrite(0x1 << 1, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB1_CLR);
	// pclk
	ax_cipher_regwrite(0x1 << 12, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB2_CLR);
#endif
	CE_LOG_PR(CE_ERR_LEVEL, "end");
	return 0;
}

int ce_pm_resume(struct device *dev)
{
	ax_cipher_dev *pcipher_dev = &cipher_dev;
	int ret;
	CE_LOG_PR(CE_ERR_LEVEL, "start");
#ifdef USE_CLK_RST_FRAMEWORK
	ret = clk_prepare_enable(pcipher_dev->pclk_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable pclk clock\n");
		return ret;
	}
	ret = clk_prepare_enable(pcipher_dev->core_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable core clock\n");
		return ret;
	}
	ret = clk_prepare_enable(pcipher_dev->cnt_clk);
	if (ret) {
		CE_LOG_PR(CE_ERR_LEVEL, "failed to enable cnt clock\n");
		return ret;
	}

	reset_control_deassert(pcipher_dev->main_sw_rst);
	reset_control_deassert(pcipher_dev->cnt_sw_rst);
	reset_control_deassert(pcipher_dev->soft_sw_rst);
	reset_control_deassert(pcipher_dev->sw_prst);
	reset_control_deassert(pcipher_dev->sw_rst);
#else
	//cnt_clk
	ax_cipher_regwrite(0x1 << 0, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB0_SET);
	//aclk
	ax_cipher_regwrite(0x1 << 1, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB1_SET);
	// pclk
	ax_cipher_regwrite(0x1 << 12, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_EB2_SET);
	// sel clk_ce_bus_sel to 416M [1:0] 2'b10: cpll_312m
	ax_cipher_regwrite(0x1 << 1, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_MUX0_SET);

	// release ce rst
	// mam_rst
	ax_cipher_regwrite(0x1 << 4, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_RST0_CLR);
	//soft_rst, prst, arst, cnt_rst
	ax_cipher_regwrite(0xf << 5, pcipher_dev->peri_base + PERI_SYS_GLB_CLK_RST0_CLR);
#endif
	ret = ce_resume();
	if (ret != 0) {
		return ret;
	}
	CE_LOG_PR(CE_ERR_LEVEL, "end");
	return 0;
}

static int cipher_probe(struct platform_device *pdev)
{
	int ret = 0;
	ax_cipher_dev *pcipher_dev;

	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res)
		return -EINVAL;

	CE_LOG_PR(CE_INFO_LEVEL, "enter\n");
	memset(&cipher_dev, 0, sizeof(ax_cipher_dev));
	pcipher_dev = &cipher_dev;
	pcipher_dev->reg_base = ioremap(res->start,  resource_size(res));
	if (!pcipher_dev->reg_base) {
		return -ENOMEM;
	}
#ifndef USE_CLK_RST_FRAMEWORK
	pcipher_dev->peri_base = ioremap(PERI_SYS_GLB,  0x1000);
	if (!pcipher_dev->peri_base) {
        iounmap(pcipher_dev->reg_base);
		return -ENOMEM;
	}
#endif
	ret = cipher_clk_enable(&pdev->dev, pcipher_dev);
	if (ret < 0) {
        iounmap(pcipher_dev->reg_base);
        iounmap(pcipher_dev->peri_base);
		CE_LOG_PR(CE_ERR_LEVEL, "clk enable failed\n");
		return -EBUSY;
	}

	cipher_reset(&pdev->dev, pcipher_dev);

	pcipher_dev->irq = platform_get_irq(pdev, 0);
	if (pcipher_dev->irq <= 0) {
        iounmap(pcipher_dev->reg_base);
        iounmap(pcipher_dev->peri_base);
		CE_LOG_PR(CE_ERR_LEVEL, "failed to get irq (%d)\n", ret);
		if (!ret)
			ret = -EINVAL;
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, pcipher_dev->irq,
										   cipher_irq_handler,
										   NULL, IRQF_ONESHOT,
										   "ax_cipher", pcipher_dev);
	if (ret) {
        iounmap(pcipher_dev->reg_base);
        iounmap(pcipher_dev->peri_base);
		CE_LOG_PR(CE_ERR_LEVEL, "Cannot get irq %d: %d\n", pcipher_dev->irq,
				  ret);
		return ret;
	}
	ax_cipher_proc_int();
	ce_init(pcipher_dev->reg_base);
#ifdef CONFIG_AX_CIPHER_MISC
	drv_cipher_init();
	ret = cipher_dev_register();
	if (ret != 0) {
		ax_cipher_proc_deinit();
		drv_cipher_deinit();
        iounmap(pcipher_dev->reg_base);
        iounmap(pcipher_dev->peri_base);
		return -EINVAL;
	}
#endif
#ifdef CONFIG_AX_CIPHER_CRYPTO
	pcipher_dev->ax_cipher_crypto_priv.dev = &pdev->dev;
	ret = ax_cipher_crypto_init(&pcipher_dev->ax_cipher_crypto_priv);
	if (ret) {
		ax_cipher_proc_deinit();
#ifdef CONFIG_AX_CIPHER_MISC
		cipher_dev_release();
		drv_cipher_deinit();
#endif
        iounmap(pcipher_dev->reg_base);
        iounmap(pcipher_dev->peri_base);
		return ret;
	}
#endif
	return 0;
}

static int cipher_remove(struct platform_device *pdev)
{
	CE_LOG_PR(CE_INFO_LEVEL, "remove\n");
	//clk_disable_unprepare(cipher_dev.pclk_clk);
	//clk_disable_unprepare(cipher_dev.core_clk);
	//clk_disable_unprepare(cipher_dev.cnt_clk);
	ax_cipher_proc_deinit();
#ifdef CONFIG_AX_CIPHER_CRYPTO
	ax_cipher_crypto_deinit(&cipher_dev.ax_cipher_crypto_priv);
#endif
#ifdef CONFIG_AX_CIPHER_MISC
	cipher_dev_release();
	drv_cipher_deinit();
#endif
    iounmap(cipher_dev.reg_base);
    iounmap(cipher_dev.peri_base);
	return 0;
}

static const struct of_device_id cipher_of_match[] = {
	{
		.compatible = "axera,cipher"
	},
	{},
};

/*MODULE_DEVICE_TABLE(of, cipher_of_match);*/

static SIMPLE_DEV_PM_OPS(ce_axera_pm_ops, ce_pm_suspend, ce_pm_resume);

static struct platform_driver cipher_driver = {
	.remove = cipher_remove,
	.probe = cipher_probe,
	.driver = {
		.name = "ax_cipher",
		.pm = &ce_axera_pm_ops,
		.of_match_table = cipher_of_match,
	},
};

static int __init cipher_init(void)
{
	return platform_driver_register(&cipher_driver);
}

static void cipher_exit(void)
{
	platform_driver_unregister(&cipher_driver);
}

module_init(cipher_init);
module_exit(cipher_exit);
#endif

MODULE_AUTHOR("axera");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_INFO(intree, "Y");
