/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#define DEVNAME  "xmedia_fmc100"
#define pr_fmt(fmt) DEVNAME": " fmt

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mtd/mtd.h>
#include <linux/delay.h>
#include <linux/mtd/rawnand.h>
#include <linux/mfd/xmedia_fmc.h>
#include "xmedia_fmc100_reg.h"
#include "xmedia_fmc100_host.h"

#define REG_BASE_CRG	0x12010000
#define REG_PERI_CRG_FMC   0x0144

static struct xmedia_fmc_host *xmedia_fmc100_get_resource(struct platform_device *pdev)
{
	struct xmedia_fmc_host *host;
	struct device *dev = &pdev->dev;
	struct bsp_fmc *fmc = dev_get_drvdata(dev->parent);

	host = devm_kzalloc(&pdev->dev, sizeof(struct xmedia_fmc_host), GFP_KERNEL);
	if (host == NULL) {
		pr_err("failed to allocate host structure.\n");
		return NULL;
	}

	platform_set_drvdata(pdev, host);

	host->dev = &pdev->dev;

	host->regbase = fmc->regbase;
	if (IS_ERR_OR_NULL(host->regbase)) {
		pr_err("%s: regbase ioremap fail.\n", __func__);
		return NULL;
	}

	host->iobase = fmc->iobase;
	if (IS_ERR_OR_NULL(host->iobase)) {
		pr_err("%s: read io addr ioremap fail.\n", __func__);
		return NULL;
	}
	host->sz_iobase = fmc->sz_iobase;

	host->irq = fmc->irq;
	if (host->irq < 0) {
		pr_err("no irq defined\n");
		return NULL;
	}

	host->clk = fmc->clk;
	if (IS_ERR_OR_NULL(host->clk)) {
		pr_err("get clock fail.\n");
		return NULL;
	}

	return host;
}

static void xmedia_fmc100_set_ifmode(struct xmedia_fmc_host *host, int ifmode)
{
	u32 regval;

	if (host->ifmode == ifmode)
		return;

	host->ifmode = ifmode;

	xmedia_fmc_write(host, 0, XMEDIA_FMC100_DMA_SADDR_OOB);
	xmedia_fmc_write(host, 0, XMEDIA_FMC100_DMA_SADDR_D0);
	xmedia_fmc_write(host, 0, XMEDIA_FMC100_DMA_SADDR_D1);
	xmedia_fmc_write(host, 0, XMEDIA_FMC100_DMA_SADDR_D2);
	xmedia_fmc_write(host, 0, XMEDIA_FMC100_DMA_SADDR_D3);
	if (host->ifmode == XMEDIA_FMC_IFMODE_INVALID)
		return;

	regval = xmedia_fmc_read(host, XMEDIA_FMC100_CFG);
	regval &= ~XMEDIA_FMC100_CFG_FLASH_SEL_MASK;

	switch (host->ifmode) {
	case XMEDIA_FMC_IFMODE_SPINOR:
		regval |= XMEDIA_FMC100_CFG_FLASH_SEL_SPINOR;
		/* dynamic clock, in every read/write/erase operation */
		break;
	case XMEDIA_FMC_IFMODE_NAND:
		regval |= XMEDIA_FMC100_CFG_FLASH_SEL_NAND;
		clk_set_rate(host->clk, host->nand->clk_rate);
		break;
	case XMEDIA_FMC_IFMODE_SPINAND:
		regval |= XMEDIA_FMC100_CFG_FLASH_SEL_SPINAND;
		clk_set_rate(host->clk, host->spinand->clk_rate);
		break;
	default:
		WARN_ON(1);
	}

	host->fmc_crg_value = readl(host->fmc_crg_addr);
	/* change to normal mode. */
	regval |= XMEDIA_FMC100_CFG_OP_MODE_NORMAL;
	xmedia_fmc_write(host, regval, XMEDIA_FMC100_CFG);

	/* sleep 1ms, change ifmode need delay. */
	msleep(1);
}

static void xmedia_fmc100_irq_enable(struct xmedia_fmc_host *host)
{
	/* clear all interrupt */
	xmedia_fmc_write(host, XMEDIA_FMC100_INT_CLR_ALL, XMEDIA_FMC100_INT_CLR);

	init_completion(&host->dma_done);

	/* only enable dma done interrupt. */
	xmedia_fmc_write(host, XMEDIA_FMC100_INT_EN_OP_DONE, XMEDIA_FMC100_INT_EN);
}

static int xmedia_fmc100_wait_dma_finish(struct xmedia_fmc_host *host)
{
	unsigned long ret;

	ret = wait_for_completion_timeout(&host->dma_done,
		msecs_to_jiffies(2000));
	if (!ret) {
		pr_err("wait dma transfer complete timeout, xmedia_fmc100 register:\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 4,
			host->regbase, 0x200, false);
	}

	return ret;
}

static irqreturn_t xmedia_fmc100_irq_handle(int irq, void *dev_id)
{
	struct xmedia_fmc_host *host = (struct xmedia_fmc_host *)dev_id;

	host->intr_status = xmedia_fmc_read(host, XMEDIA_FMC100_INT);

	/* clear opdone interrupt */
	if (host->intr_status & XMEDIA_FMC100_INT_OP_DONE) {
		xmedia_fmc_write(host, XMEDIA_FMC100_INT_CLR_OP_DONE, XMEDIA_FMC100_INT_CLR);
		complete(&host->dma_done);
	}

	return IRQ_HANDLED;
}

static int xmedia_fmc100_wait_host_ready(struct xmedia_fmc_host *host)
{
	u32 regval;
	ulong deadline = jiffies + 4 * HZ;

	do {
		regval = xmedia_fmc_read(host, XMEDIA_FMC100_OP);
		if (!(regval & XMEDIA_FMC100_OP_REG_OP_START))
			return 0;
	} while (!time_after_eq(jiffies, deadline));

	pr_err("wait host ready timeout, xmedia_fmc100 register:\n");
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 4,
		host->regbase, 0x200, false);

	return -1;
}

int xmedia_fmc100_write_reg(struct flash_regop_info *info)
{
	u32 regval;
	u32 cfg_val;
	u32 global_cfg_val;
	struct xmedia_fmc_host *xmedia_fmc = (struct xmedia_fmc_host *)info->priv;

	if (info == NULL || info->priv == NULL) {
		pr_err("%s: invalid point\n", __func__);
		return -1;
	}

	WARN_ON(!(xmedia_fmc->chipselect == 0 || xmedia_fmc->chipselect == 1));

	/* set chip select and address number */
	regval = 0;
	if (xmedia_fmc->chipselect)
		regval |= XMEDIA_FMC100_OP_CFG_FM_CS;
	regval |= XMEDIA_FMC100_OP_CFG_ADDR_NUM(info->addr_cycle);
	regval |= XMEDIA_FMC100_OP_CFG_DUMMY_NUM(info->dummy);
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_OP_CFG);

	/* disable ecc */
	regval = cfg_val = xmedia_fmc_read(xmedia_fmc, XMEDIA_FMC100_CFG);
	regval &= ~XMEDIA_FMC100_CFG_ECC_TYPE_MASK;
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_CFG);

	xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_OP_PARA);

	/* disable randomizer */
	regval = global_cfg_val = xmedia_fmc_read(xmedia_fmc, XMEDIA_FMC100_GLOBAL_CFG);
	regval &= ~XMEDIA_FMC100_GLOBAL_CFG_RANDOMIZER_EN;
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_GLOBAL_CFG);

	/* set command */
	xmedia_fmc_write(xmedia_fmc, (u32)info->cmd, XMEDIA_FMC100_CMD);

	/* set address */
	if (info->addr_cycle > 0) {
		if (info->addr_cycle > 4)
			xmedia_fmc_write(xmedia_fmc, info->addrh, XMEDIA_FMC100_ADDRH);
		else
			xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRH);

		xmedia_fmc_write(xmedia_fmc, info->addrl, XMEDIA_FMC100_ADDRL);
	} else {
		/* no address parameter */
		xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRH);
		xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRL);
	}

	/* for syncmode, need to config XMEDIA_FMC100_OP_RW_REG.
	 * for asyncmode, do not care this bit.
	 */
	regval = XMEDIA_FMC100_OP_REG_OP_START | XMEDIA_FMC100_OP_RW_REG;

	if (info->dummy)
		regval |= XMEDIA_FMC100_OP_DUMMY_EN;

	/* only nand have wait ready feature, spinand/spinor not support */
	if (info->wait_ready)
		regval |= XMEDIA_FMC100_OP_WAIT_READY_EN;

	if (info->nr_cmd >= 1) {
		regval |= XMEDIA_FMC100_OP_CMD1_EN;
		if (info->nr_cmd >= 2)
			regval |= XMEDIA_FMC100_OP_CMD2_EN;
	}

	/* set data */
	if (info->sz_buf) {
		if (info->sz_buf > xmedia_fmc->sz_iobase)
			info->sz_buf = xmedia_fmc->sz_iobase;
		regval |= XMEDIA_FMC100_OP_WRITE_DATA_EN;
		if (info->buf)
			memcpy(xmedia_fmc->iobase, info->buf, info->sz_buf);
	}
	xmedia_fmc_write(xmedia_fmc, XMEDIA_FMC100_DATA_NUM_CNT(info->sz_buf),
		XMEDIA_FMC100_DATA_NUM);

	if (info->addr_cycle)
		regval |= XMEDIA_FMC100_OP_ADDR_EN;

	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_OP);

	xmedia_fmc100_wait_host_ready(xmedia_fmc);

	/* restore ecc configure for debug */
	xmedia_fmc_write(xmedia_fmc, cfg_val, XMEDIA_FMC100_CFG);
	/* restore randomizer config for debug */
	xmedia_fmc_write(xmedia_fmc, global_cfg_val, XMEDIA_FMC100_GLOBAL_CFG);

	return 0;
}

int xmedia_fmc100_read_reg(struct flash_regop_info *info)
{
	u32 regval;
	u32 cfg_val;
	u32 global_cfg_val;
	struct xmedia_fmc_host *xmedia_fmc = (struct xmedia_fmc_host *)info->priv;

	if (xmedia_fmc == NULL || xmedia_fmc->iobase == NULL) {
		pr_err("%s: invalid point\n", __func__);
		return -1;
	}
	BUG_ON(!(xmedia_fmc->chipselect == 0 || xmedia_fmc->chipselect == 1));

	/* set chip select, address number, dummy */
	regval = 0;
	if (xmedia_fmc->chipselect)
		regval |= XMEDIA_FMC100_OP_CFG_FM_CS;
	regval |= XMEDIA_FMC100_OP_CFG_ADDR_NUM(info->addr_cycle);
	regval |= XMEDIA_FMC100_OP_CFG_DUMMY_NUM(info->dummy);
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_OP_CFG);

	/* disable ecc */
	regval = cfg_val = xmedia_fmc_read(xmedia_fmc, XMEDIA_FMC100_CFG);
	regval &= ~XMEDIA_FMC100_CFG_ECC_TYPE_MASK;
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_CFG);

	xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_OP_PARA);

	/* disable randomizer */
	regval = global_cfg_val = xmedia_fmc_read(xmedia_fmc, XMEDIA_FMC100_GLOBAL_CFG);
	regval &= ~XMEDIA_FMC100_GLOBAL_CFG_RANDOMIZER_EN;
	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_GLOBAL_CFG);

	/* set command */
	xmedia_fmc_write(xmedia_fmc, (u32)info->cmd, XMEDIA_FMC100_CMD);

	/* set address */
	if (info->addr_cycle > 0) {
		if (info->addr_cycle > 4)
			xmedia_fmc_write(xmedia_fmc, info->addrh, XMEDIA_FMC100_ADDRH);
		else
			xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRH);

		xmedia_fmc_write(xmedia_fmc, info->addrl, XMEDIA_FMC100_ADDRL);
	} else {
		xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRH);
		xmedia_fmc_write(xmedia_fmc, 0, XMEDIA_FMC100_ADDRL);
	}

	/* set data length */
	if (info->sz_buf > xmedia_fmc->sz_iobase)
		info->sz_buf = xmedia_fmc->sz_iobase;

	xmedia_fmc_write(xmedia_fmc, XMEDIA_FMC100_DATA_NUM_CNT(info->sz_buf),
		XMEDIA_FMC100_DATA_NUM);

	/* for syncmode, need to config XMEDIA_FMC100_OP_RW_REG.
	 * for asyncmode, do not care this bit.
	 */
	regval = XMEDIA_FMC100_OP_REG_OP_START | XMEDIA_FMC100_OP_RW_REG;

	if (info->dummy)
		regval |= XMEDIA_FMC100_OP_DUMMY_EN;

	/* only nand have wait ready feature, spinand/spinor not support */
	if (info->wait_ready)
		regval |= XMEDIA_FMC100_OP_WAIT_READY_EN;

	if (info->nr_cmd >= 1) {
		regval |= XMEDIA_FMC100_OP_CMD1_EN;
		if (info->nr_cmd >= 2)
			regval |= XMEDIA_FMC100_OP_CMD2_EN;
	}

	if (info->sz_buf)
		regval |= XMEDIA_FMC100_OP_READ_DATA_EN;

	if (info->addr_cycle)
		regval |= XMEDIA_FMC100_OP_ADDR_EN;

	/* for syncmode, need to config XMEDIA_FMC100_OP_RW_REG,
	 * for asyncmode, do not care this bit.
	 */
	if (info->cmd == NAND_CMD_READID)
		regval |= XMEDIA_FMC100_OP_READID;

	xmedia_fmc_write(xmedia_fmc, regval, XMEDIA_FMC100_OP);

	xmedia_fmc100_wait_host_ready(xmedia_fmc);

	if (info->sz_buf && info->buf)
		memcpy(info->buf, xmedia_fmc->iobase, info->sz_buf);

	/* restore ecc configure for debug */
	xmedia_fmc_write(xmedia_fmc, cfg_val, XMEDIA_FMC100_CFG);
	/* restore randomizer config for debug */
	xmedia_fmc_write(xmedia_fmc, global_cfg_val, XMEDIA_FMC100_GLOBAL_CFG);

	return 0;
}

static void xmedia_fmc100_controller_init(struct xmedia_fmc_host *host)
{
	u32 regval;

	/* disable all interrupt */
	regval = xmedia_fmc_read(host, XMEDIA_FMC100_INT_EN);
	regval &= ~XMEDIA_FMC100_INT_EN_ALL;
	xmedia_fmc_write(host, regval, XMEDIA_FMC100_INT_EN);

	/* clean all interrupt */
	xmedia_fmc_write(host, XMEDIA_FMC100_INT_CLR_ALL, XMEDIA_FMC100_INT_CLR);

	/* configure dma burst width */
	xmedia_fmc_write(host, 0xF, XMEDIA_FMC100_DMA_AHB_CTRL);

	/* restore default value. */
	xmedia_fmc_write(host, 0xC4, XMEDIA_FMC100_GLOBAL_CFG);

	/* set nand/spinand default value */
	xmedia_fmc_write(host, 0xaaa, XMEDIA_FMC100_PND_PWIDTH_CFG);

	xmedia_fmc_write(host, 0x6F, XMEDIA_FMC100_TIMING_SPI_CFG);
}

static int xmedia_fmc100_driver_probe(struct platform_device *pdev)
{
	int ret;
	u32 regval;
	struct xmedia_fmc_host *host;

	host = xmedia_fmc100_get_resource(pdev);
	if (host == NULL || host->clk == NULL)
		return -ENODEV;

	clk_prepare_enable(host->clk);

	regval = xmedia_fmc_read(host, XMEDIA_FMC100_VERSION);
	if (regval != XMEDIA_FMC100_VERSION_VALUE)
		return -ENODEV;

	pr_notice("Found flash memory controller xmedia_fmc100.\n");

	xmedia_fmc100_controller_init(host);
	host->ifmode = XMEDIA_FMC_IFMODE_INVALID;
	host->set_ifmode = xmedia_fmc100_set_ifmode;
	host->irq_enable = xmedia_fmc100_irq_enable;
	host->wait_dma_finish = xmedia_fmc100_wait_dma_finish;
	host->wait_host_ready = xmedia_fmc100_wait_host_ready;
#if defined(CONFIG_ARCH_GK6323V100C)
	host->caps |= NAND_MODE_SYNC;
#endif
	host->fmc_crg_addr = ioremap(REG_BASE_CRG + REG_PERI_CRG_FMC, sizeof(u32));
	if (!host->fmc_crg_addr) {
		pr_err("fmc_crg_addr ioremap fail.\n");
		ret = -ENODEV;
		goto fail;
	}

	/*
	 * get ecctype and pagesize from controller,
	 * controller should not reset after boot.
	 */
	regval = xmedia_fmc_read(host, XMEDIA_FMC100_CFG);
	host->reg.fmc_cfg_ecc_type = XMEDIA_FMC100_CFG_ECC_TYPE_MASK & regval;
	host->reg.fmc_cfg_page_size = XMEDIA_FMC100_CFG_PAGE_SIZE_MASK & regval;
#ifdef CONFIG_MTD_XMEDIA_FMC100_NAND
	if ((host->caps)&NAND_MODE_SYNC) {
		/* check if controler is in syncmode. */
		regval &= XMEDIA_FMC100_CFG_NF_MODE_MASK;
		if (regval)
			host->flags |= xmedia_fmc100_syncmode_reg(regval, 0);
	}
#endif
	mutex_init(&host->lock);

	ret = request_irq(host->irq, xmedia_fmc100_irq_handle, 0, DEVNAME"-irq", host);
	if (ret) {
		pr_err("unable to request irq %d\n", host->irq);
		ret = -EIO;
		goto fail;
	}

#ifdef CONFIG_MTD_XMEDIA_FMC100_SPINOR
	ret = xmedia_fmc100_spinor_probe(pdev, host);
	if (ret)
		pr_info("no found spi-nor device.\n");
#endif

#ifdef CONFIG_MTD_XMEDIA_FMC100_NAND
	ret = xmedia_fmc100_xnand_probe(pdev, host, XMEDIA_FMC_IFMODE_NAND);
	if (ret)
		pr_info("no found nand device.\n");
#endif

#ifdef CONFIG_MTD_XMEDIA_FMC100_SPINAND
	ret = xmedia_fmc100_xnand_probe(pdev, host, XMEDIA_FMC_IFMODE_SPINAND);
	if (ret)
		pr_info("no found spi-nand device.\n");
#endif
	if (!host->spinor && !host->nand && !host->spinand) {
		clk_disable(host->clk);
		ret = -ENODEV;
		goto fail;
	}

	return 0;

fail:
	iounmap(host->fmc_crg_addr);

	mutex_destroy(&host->lock);

	return ret;
}

#define DEV_FUN(_host, _dev, _fun) \
	if (_host->_dev && _host->_dev->_fun) _host->_dev->_fun(_host->_dev)

static void xmedia_fmc100_driver_shutdown(struct platform_device *pdev)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return;

	DEV_FUN(host, spinor, shutdown);
	DEV_FUN(host, nand, shutdown);
	DEV_FUN(host, spinand, shutdown);
}

#ifdef CONFIG_PM
static int xmedia_fmc100_driver_suspend(struct platform_device *pdev,
				   pm_message_t state)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return -EINVAL;

	DEV_FUN(host, spinor, suspend);
	DEV_FUN(host, nand, suspend);
	DEV_FUN(host, spinand, suspend);

	return 0;
}

static int xmedia_fmc100_driver_resume(struct platform_device *pdev)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return -EINVAL;

	xmedia_fmc100_controller_init(host);

	DEV_FUN(host, nand, resume);
	DEV_FUN(host, spinand, resume);
	DEV_FUN(host, spinor, resume);

	return 0;
}
#else
#  define xmedia_fmc100_driver_suspend NULL
#  define xmedia_fmc100_driver_resume  NULL
#endif /* CONFIG_PM */

static const struct of_device_id
xmedia_fmc100_match[] __maybe_unused = {
	{ .compatible = "xmedia,fmc-spi-nor", },
	{},
};
MODULE_DEVICE_TABLE(of, xmedia_fmc100_match);

static struct platform_driver xmedia_fmc100_pltm_driver = {
	.probe  = xmedia_fmc100_driver_probe,
	.shutdown = xmedia_fmc100_driver_shutdown,
	.suspend = xmedia_fmc100_driver_suspend,
	.resume = xmedia_fmc100_driver_resume,
	.driver = {
		.name = DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(xmedia_fmc100_match),
	},
};

static int __init xmedia_fmc100_module_init(void)
{
	pr_info("registerd new interface driver xmedia_fmc100.\n");
	return platform_driver_register(&xmedia_fmc100_pltm_driver);
}
module_init(xmedia_fmc100_module_init);

static void __exit xmedia_fmc100_module_exit(void)
{
	platform_driver_unregister(&xmedia_fmc100_pltm_driver);
}
module_exit(xmedia_fmc100_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Xmedia");
MODULE_AUTHOR("Xmedia");
MODULE_DESCRIPTION("Xmedia Flash Controller V100 Device Driver");
