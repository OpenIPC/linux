/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#define DEVNAME  "lotus_fmc100"
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
#include <linux/lotus/fmc_mfd.h>
#include "fmc100_reg.h"
#include "fmc100_host.h"

#define REG_BASE_CRG	0x12010000
#define REG_PERI_CRG_FMC   0x0144

static struct lotus_fmc_host *lotus_fmc100_get_resource(struct platform_device *pdev)
{
	struct lotus_fmc_host *host;
	struct device *dev = &pdev->dev;
	struct lotus_fmc *fmc = dev_get_drvdata(dev->parent);

	host = devm_kzalloc(&pdev->dev, sizeof(struct lotus_fmc_host), GFP_KERNEL);
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

static void lotus_fmc100_set_ifmode(struct lotus_fmc_host *host, int ifmode)
{
	u32 regval;

	if (host->ifmode == ifmode)
		return;

	host->ifmode = ifmode;

	lotus_fmc_write(host, 0, LOTUS_FMC100_DMA_SADDR_OOB);
	lotus_fmc_write(host, 0, LOTUS_FMC100_DMA_SADDR_D0);
	lotus_fmc_write(host, 0, LOTUS_FMC100_DMA_SADDR_D1);
	lotus_fmc_write(host, 0, LOTUS_FMC100_DMA_SADDR_D2);
	lotus_fmc_write(host, 0, LOTUS_FMC100_DMA_SADDR_D3);
	if (host->ifmode == LOTUS_FMC_IFMODE_INVALID)
		return;

	regval = lotus_fmc_read(host, LOTUS_FMC100_CFG);
	regval &= ~LOTUS_FMC100_CFG_FLASH_SEL_MASK;

	switch (host->ifmode) {
	case LOTUS_FMC_IFMODE_SPINOR:
		regval |= LOTUS_FMC100_CFG_FLASH_SEL_SPINOR;
		/* dynamic clock, in every read/write/erase operation */
		break;
	case LOTUS_FMC_IFMODE_NAND:
		regval |= LOTUS_FMC100_CFG_FLASH_SEL_NAND;
		clk_set_rate(host->clk, host->nand->clk_rate);
		break;
	case LOTUS_FMC_IFMODE_SPINAND:
		regval |= LOTUS_FMC100_CFG_FLASH_SEL_SPINAND;
		clk_set_rate(host->clk, host->spinand->clk_rate);
		break;
	default:
		WARN_ON(1);
	}

	host->fmc_crg_value = readl(host->fmc_crg_addr);
	/* change to normal mode. */
	regval |= LOTUS_FMC100_CFG_OP_MODE_NORMAL;
	lotus_fmc_write(host, regval, LOTUS_FMC100_CFG);

	/* sleep 1ms, change ifmode need delay. */
	msleep(1);
}

static void lotus_fmc100_irq_enable(struct lotus_fmc_host *host)
{
	/* clear all interrupt */
	lotus_fmc_write(host, LOTUS_FMC100_INT_CLR_ALL, LOTUS_FMC100_INT_CLR);

	init_completion(&host->dma_done);

	/* only enable dma done interrupt. */
	lotus_fmc_write(host, LOTUS_FMC100_INT_EN_OP_DONE, LOTUS_FMC100_INT_EN);
}

static int lotus_fmc100_wait_dma_finish(struct lotus_fmc_host *host)
{
	unsigned long ret;

	ret = wait_for_completion_timeout(&host->dma_done,
		msecs_to_jiffies(2000));
	if (!ret) {
		pr_err("wait dma transfer complete timeout, lotus_fmc100 register:\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 4,
			host->regbase, 0x200, false);
	}

	return ret;
}

static irqreturn_t lotus_fmc100_irq_handle(int irq, void *dev_id)
{
	struct lotus_fmc_host *host = (struct lotus_fmc_host *)dev_id;

	host->intr_status = lotus_fmc_read(host, LOTUS_FMC100_INT);

	/* clear opdone interrupt */
	if (host->intr_status & LOTUS_FMC100_INT_OP_DONE) {
		lotus_fmc_write(host, LOTUS_FMC100_INT_CLR_OP_DONE, LOTUS_FMC100_INT_CLR);
		complete(&host->dma_done);
	}

	return IRQ_HANDLED;
}

static int lotus_fmc100_wait_host_ready(struct lotus_fmc_host *host)
{
	u32 regval;
	ulong deadline = jiffies + 4 * HZ;

	do {
		regval = lotus_fmc_read(host, LOTUS_FMC100_OP);
		if (!(regval & LOTUS_FMC100_OP_REG_OP_START))
			return 0;
	} while (!time_after_eq(jiffies, deadline));

	pr_err("wait host ready timeout, lotus_fmc100 register:\n");
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 4,
		host->regbase, 0x200, false);

	return -1;
}

int lotus_fmc100_write_reg(struct flash_regop_info *info)
{
	u32 regval;
	u32 cfg_val;
	u32 global_cfg_val;
	struct lotus_fmc_host *lotus_fmc = (struct lotus_fmc_host *)info->priv;

	if (info == NULL || info->priv == NULL) {
		pr_err("%s: invalid point\n", __func__);
		return -1;
	}

	WARN_ON(!(lotus_fmc->chipselect == 0 || lotus_fmc->chipselect == 1));

	/* set chip select and address number */
	regval = 0;
	if (lotus_fmc->chipselect)
		regval |= LOTUS_FMC100_OP_CFG_FM_CS;
	regval |= LOTUS_FMC100_OP_CFG_ADDR_NUM(info->addr_cycle);
	regval |= LOTUS_FMC100_OP_CFG_DUMMY_NUM(info->dummy);
	regval |= LOTUS_FMC100_OP_CFG_OEN_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP_CFG);

	/* disable ecc */
	regval = cfg_val = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_CFG);
	regval &= ~LOTUS_FMC100_CFG_ECC_TYPE_MASK;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_CFG);

	lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_OP_PARA);

	/* disable randomizer */
	regval = global_cfg_val = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_GLOBAL_CFG);
	regval &= ~LOTUS_FMC100_GLOBAL_CFG_RANDOMIZER_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_GLOBAL_CFG);

	/* set command */
	lotus_fmc_write(lotus_fmc, (u32)info->cmd, LOTUS_FMC100_CMD);

	/* set address */
	if (info->addr_cycle > 0) {
		if (info->addr_cycle > 4)
			lotus_fmc_write(lotus_fmc, info->addrh, LOTUS_FMC100_ADDRH);
		else
			lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRH);

		lotus_fmc_write(lotus_fmc, info->addrl, LOTUS_FMC100_ADDRL);
	} else {
		/* no address parameter */
		lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRH);
		lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRL);
	}

	/* for syncmode, need to config LOTUS_FMC100_OP_RW_REG.
	 * for asyncmode, do not care this bit.
	 */
	regval = LOTUS_FMC100_OP_REG_OP_START | LOTUS_FMC100_OP_RW_REG;

	if (info->dummy)
		regval |= LOTUS_FMC100_OP_DUMMY_EN;

	/* only nand have wait ready feature, spinand/spinor not support */
	if (info->wait_ready)
		regval |= LOTUS_FMC100_OP_WAIT_READY_EN;

	if (info->nr_cmd >= 1) {
		regval |= LOTUS_FMC100_OP_CMD1_EN;
		if (info->nr_cmd >= 2)
			regval |= LOTUS_FMC100_OP_CMD2_EN;
	}

	/* set data */
	if (info->sz_buf) {
		if (info->sz_buf > lotus_fmc->sz_iobase)
			info->sz_buf = lotus_fmc->sz_iobase;
		regval |= LOTUS_FMC100_OP_WRITE_DATA_EN;
		if (info->buf)
			memcpy(lotus_fmc->iobase, info->buf, info->sz_buf);
	}
	lotus_fmc_write(lotus_fmc, LOTUS_FMC100_DATA_NUM_CNT(info->sz_buf),
		LOTUS_FMC100_DATA_NUM);

	if (info->addr_cycle)
		regval |= LOTUS_FMC100_OP_ADDR_EN;

	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP);

	lotus_fmc100_wait_host_ready(lotus_fmc);

	/* restore ecc configure for debug */
	lotus_fmc_write(lotus_fmc, cfg_val, LOTUS_FMC100_CFG);
	/* restore randomizer config for debug */
	lotus_fmc_write(lotus_fmc, global_cfg_val, LOTUS_FMC100_GLOBAL_CFG);

	return 0;
}

int lotus_fmc100_read_reg(struct flash_regop_info *info)
{
	u32 regval;
	u32 cfg_val;
	u32 global_cfg_val;
	struct lotus_fmc_host *lotus_fmc = (struct lotus_fmc_host *)info->priv;

	if (lotus_fmc == NULL || lotus_fmc->iobase == NULL) {
		pr_err("%s: invalid point\n", __func__);
		return -1;
	}
	BUG_ON(!(lotus_fmc->chipselect == 0 || lotus_fmc->chipselect == 1));

	/* set chip select, address number, dummy */
	regval = 0;
	if (lotus_fmc->chipselect)
		regval |= LOTUS_FMC100_OP_CFG_FM_CS;
	regval |= LOTUS_FMC100_OP_CFG_ADDR_NUM(info->addr_cycle);
	regval |= LOTUS_FMC100_OP_CFG_DUMMY_NUM(info->dummy);
	regval |= LOTUS_FMC100_OP_CFG_OEN_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP_CFG);

	/* disable ecc */
	regval = cfg_val = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_CFG);
	regval &= ~LOTUS_FMC100_CFG_ECC_TYPE_MASK;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_CFG);

	lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_OP_PARA);

	/* disable randomizer */
	regval = global_cfg_val = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_GLOBAL_CFG);
	regval &= ~LOTUS_FMC100_GLOBAL_CFG_RANDOMIZER_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_GLOBAL_CFG);

	/* set command */
	lotus_fmc_write(lotus_fmc, (u32)info->cmd, LOTUS_FMC100_CMD);

	/* set address */
	if (info->addr_cycle > 0) {
		if (info->addr_cycle > 4)
			lotus_fmc_write(lotus_fmc, info->addrh, LOTUS_FMC100_ADDRH);
		else
			lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRH);

		lotus_fmc_write(lotus_fmc, info->addrl, LOTUS_FMC100_ADDRL);
	} else {
		lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRH);
		lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRL);
	}

	/* set data length */
	if (info->sz_buf > lotus_fmc->sz_iobase)
		info->sz_buf = lotus_fmc->sz_iobase;

	lotus_fmc_write(lotus_fmc, LOTUS_FMC100_DATA_NUM_CNT(info->sz_buf),
		LOTUS_FMC100_DATA_NUM);

	/* for syncmode, need to config LOTUS_FMC100_OP_RW_REG.
	 * for asyncmode, do not care this bit.
	 */
	regval = LOTUS_FMC100_OP_REG_OP_START | LOTUS_FMC100_OP_RW_REG;

	if (info->dummy)
		regval |= LOTUS_FMC100_OP_DUMMY_EN;

	/* only nand have wait ready feature, spinand/spinor not support */
	if (info->wait_ready)
		regval |= LOTUS_FMC100_OP_WAIT_READY_EN;

	if (info->nr_cmd >= 1) {
		regval |= LOTUS_FMC100_OP_CMD1_EN;
		if (info->nr_cmd >= 2)
			regval |= LOTUS_FMC100_OP_CMD2_EN;
	}

	if (info->sz_buf)
		regval |= LOTUS_FMC100_OP_READ_DATA_EN;

	if (info->addr_cycle)
		regval |= LOTUS_FMC100_OP_ADDR_EN;

	/* for syncmode, need to config LOTUS_FMC100_OP_RW_REG,
	 * for asyncmode, do not care this bit.
	 */
	if (info->cmd == NAND_CMD_READID)
		regval |= LOTUS_FMC100_OP_READID;

	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP);

	lotus_fmc100_wait_host_ready(lotus_fmc);

	if (info->sz_buf && info->buf)
		memcpy(info->buf, lotus_fmc->iobase, info->sz_buf);

	/* restore ecc configure for debug */
	lotus_fmc_write(lotus_fmc, cfg_val, LOTUS_FMC100_CFG);
	/* restore randomizer config for debug */
	lotus_fmc_write(lotus_fmc, global_cfg_val, LOTUS_FMC100_GLOBAL_CFG);

	return 0;
}

int lotus_fmc100_cmd_opt(struct mtd_info *mtd, struct mtd_cmd_opt *cmd_opt)
{
#ifdef CONFIG_LOTUS_FMC100_SPINOR
	if (mtd->type == MTD_NORFLASH)
		lotus_fmc100_spinor_cmd_opt(mtd, cmd_opt);
#endif

#ifdef CONFIG_LOTUS_FMC100_SPINAND
	if (mtd->type == MTD_NANDFLASH)
		lotus_fmc100_spinand_cmd_opt(mtd, cmd_opt);
#endif

	return 0;
}
EXPORT_SYMBOL_GPL(lotus_fmc100_cmd_opt);

static void lotus_fmc100_controller_init(struct lotus_fmc_host *host)
{
	u32 regval;

	/* disable all interrupt */
	regval = lotus_fmc_read(host, LOTUS_FMC100_INT_EN);
	regval &= ~LOTUS_FMC100_INT_EN_ALL;
	lotus_fmc_write(host, regval, LOTUS_FMC100_INT_EN);

	/* clean all interrupt */
	lotus_fmc_write(host, LOTUS_FMC100_INT_CLR_ALL, LOTUS_FMC100_INT_CLR);

	/* configure dma burst width */
	lotus_fmc_write(host, LOTUS_FMC100_DMA_AHB_CTRL_DEF, LOTUS_FMC100_DMA_AHB_CTRL);

	/* restore default value. */
	lotus_fmc_write(host, LOTUS_FMC100_GLOBAL_CFG_DEF, LOTUS_FMC100_GLOBAL_CFG);

	/* set nand/spinand default value */
	lotus_fmc_write(host, LOTUS_FMC100_PND_PWIDTH_CFG_DEF, LOTUS_FMC100_PND_PWIDTH_CFG);

	lotus_fmc_write(host, LOTUS_FMC100_TIMING_SPI_CFG_DEF, LOTUS_FMC100_TIMING_SPI_CFG);
}

static int lotus_fmc100_driver_probe(struct platform_device *pdev)
{
	int ret;
	u32 regval;
	struct lotus_fmc_host *host;

	host = lotus_fmc100_get_resource(pdev);
	if (host == NULL || host->clk == NULL)
		return -ENODEV;

	clk_prepare_enable(host->clk);

	regval = lotus_fmc_read(host, LOTUS_FMC100_VERSION);
	if (regval != LOTUS_FMC100_VERSION_VALUE)
		return -ENODEV;

	pr_notice("Found flash memory controller lotus_fmc100.\n");

	lotus_fmc100_controller_init(host);
	host->ifmode = LOTUS_FMC_IFMODE_INVALID;
	host->set_ifmode = lotus_fmc100_set_ifmode;
	host->irq_enable = lotus_fmc100_irq_enable;
	host->wait_dma_finish = lotus_fmc100_wait_dma_finish;
	host->wait_host_ready = lotus_fmc100_wait_host_ready;
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
	regval = lotus_fmc_read(host, LOTUS_FMC100_CFG);
	host->reg.fmc_cfg_ecc_type = LOTUS_FMC100_CFG_ECC_TYPE_MASK & regval;
	host->reg.fmc_cfg_page_size = LOTUS_FMC100_CFG_PAGE_SIZE_MASK & regval;
#ifdef CONFIG_LOTUS_FMC100_NAND
	if ((host->caps)&NAND_MODE_SYNC) {
		/* check if controler is in syncmode. */
		regval &= LOTUS_FMC100_CFG_NF_MODE_MASK;
		if (regval)
			host->flags |= lotus_fmc100_syncmode_reg(regval, 0);
	}
#endif
	mutex_init(&host->lock);

	ret = request_irq(host->irq, lotus_fmc100_irq_handle, 0, DEVNAME"-irq", host);
	if (ret) {
		pr_err("unable to request irq %d\n", host->irq);
		ret = -EIO;
		goto fail;
	}

#ifdef CONFIG_LOTUS_FMC100_SPINOR
	ret = lotus_fmc100_spinor_probe(pdev, host);
	if (ret)
		pr_info("no found spi-nor device.\n");
#endif

#ifdef CONFIG_LOTUS_FMC100_NAND
	ret = lotus_fmc100_xnand_probe(pdev, host, LOTUS_FMC_IFMODE_NAND);
	if (ret)
		pr_info("no found nand device.\n");
#endif

#ifdef CONFIG_LOTUS_FMC100_SPINAND
	if (!host->spinor) {
		ret = lotus_fmc100_xnand_probe(pdev, host, LOTUS_FMC_IFMODE_SPINAND);
		if (ret)
			pr_info("no found spi-nand device.\n");
	}
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

static void lotus_fmc100_driver_shutdown(struct platform_device *pdev)
{
	struct lotus_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return;

	DEV_FUN(host, spinor, shutdown);
	DEV_FUN(host, nand, shutdown);
	DEV_FUN(host, spinand, shutdown);
}

#ifdef CONFIG_PM
static int lotus_fmc100_driver_suspend(struct platform_device *pdev,
				   pm_message_t state)
{
	struct lotus_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return -EINVAL;

	DEV_FUN(host, spinor, suspend);
	DEV_FUN(host, nand, suspend);
	DEV_FUN(host, spinand, suspend);

	return 0;
}

static int lotus_fmc100_driver_resume(struct platform_device *pdev)
{
	struct lotus_fmc_host *host = platform_get_drvdata(pdev);
	if (!host)
		return -EINVAL;

	lotus_fmc100_controller_init(host);

	DEV_FUN(host, nand, resume);
	DEV_FUN(host, spinand, resume);
	DEV_FUN(host, spinor, resume);

	return 0;
}
#else
#  define lotus_fmc100_driver_suspend NULL
#  define lotus_fmc100_driver_resume  NULL
#endif /* CONFIG_PM */

static const struct of_device_id
lotus_fmc100_match[] __maybe_unused = {
	{ .compatible = "lotus,fmc-spi-nor", },
	{},
};
MODULE_DEVICE_TABLE(of, lotus_fmc100_match);

static struct platform_driver lotus_fmc100_pltm_driver = {
	.probe  = lotus_fmc100_driver_probe,
	.shutdown = lotus_fmc100_driver_shutdown,
	.suspend = lotus_fmc100_driver_suspend,
	.resume = lotus_fmc100_driver_resume,
	.driver = {
		.name = DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lotus_fmc100_match),
	},
};

static int __init lotus_fmc100_module_init(void)
{
	pr_info("registerd new interface driver lotus_fmc100.\n");
	return platform_driver_register(&lotus_fmc100_pltm_driver);
}
module_init(lotus_fmc100_module_init);

static void __exit lotus_fmc100_module_exit(void)
{
	platform_driver_unregister(&lotus_fmc100_pltm_driver);
}
module_exit(lotus_fmc100_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lotus");
MODULE_AUTHOR("Lotus");
MODULE_DESCRIPTION("Lotus Flash Controller V100 Device Driver");
