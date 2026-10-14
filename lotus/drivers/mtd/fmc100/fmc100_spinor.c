/*
 * Copyright (c) LOTUS. All rights reserved.
 */
#define DEVNAME "lotus_fmc100: "
#define pr_fmt(fmt) "spinor: " fmt

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/platform_device.h>

#include "fmc100_reg.h"
#include "fmc100_host.h"

static int lotus_fmc100_spinor_read_sr(struct flash_regop *regop)
{
	u8 status;
	struct flash_regop_info info = {0};

	info.priv = regop->priv;
	info.cmd = SPINOR_OP_RDSR;
	info.nr_cmd = 1;
	info.buf = &status;
	info.sz_buf = 1;
	regop->read_reg(&info);

	return status;
}

static int lotus_fmc100_spinor_wait_device_ready(struct flash_regop *regop)
{
	u32 regval;
	ulong deadline = jiffies + 4 * HZ;

	do {
		regval = regop->read_sr(regop);
		if (!(regval & SPINOR_STATUS_WIP))
			return 0;

		schedule_timeout_uninterruptible(1);

	} while (!time_after_eq(jiffies, deadline));

	pr_err("wait spi-nor chip ready timeout.\n");

	return 1;
}

static void lotus_fmc100_spinor_select_chip(struct lotus_fmc_spinor *spinor, int chipselect)
{
	struct lotus_fmc_host *lotus_fmc = spinor->host;
	lotus_fmc->chipselect = spinor->chipselect[chipselect];
}

static int lotus_fmc100_spinor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int nrchip = 0;
	u32 offset = (u32)instr->addr;
	loff_t len = instr->len;
	struct flash_regop_info info = {0};
	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)mtd->priv;
	struct lotus_fmc_host *lotus_fmc = spinor->host;
	struct flash_regop *regop = spinor->regop;
	struct lotus_fmc_reg *erase = &spinor->xfer.erase;

	if (offset & (spinor->erasesize - 1)) {
		pr_err("erase address(0x%08x) not block aligned.\n", offset);
		return -EINVAL;
	}

	if (len & (spinor->erasesize - 1)) {
		pr_err("erase len(0x%08x) not block aligned.\n", offset);
		return -EINVAL;
	}

	mutex_lock(&lotus_fmc->lock);
	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, erase->clock);

	nrchip = (int)(offset >> spinor->chipsize_shift);

	lotus_fmc100_spinor_select_chip(spinor, nrchip);

	offset = offset & spinor->chipsize_mask;

	info.priv = regop->priv;
	info.cmd = erase->cmd;
	info.nr_cmd = 1;
	info.addr_cycle = spinor->addr_cycle;

	while (len > 0) {
		regop->wait_device_ready(regop);
		spinor->drv->write_enable(regop);

		info.addrl = offset;
		regop->read_reg(&info);

		offset += spinor->erasesize;
		len -= spinor->erasesize;

		if (offset > spinor->chipsize) {
			offset -= spinor->chipsize;
			lotus_fmc100_spinor_select_chip(spinor, ++nrchip);
		}
	}
	mutex_unlock(&lotus_fmc->lock);

	return 0;
}

static void lotus_fmc100_spinor_dma_transfer(struct lotus_fmc_spinor *spinor,
					 struct lotus_fmc_reg *xfer, u32 offset,
					 int size)
{
	u32 regval;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	/* set dummy */
	regval = 0;
	if (xfer->dummy)
		regval |= LOTUS_FMC100_OP_DUMMY_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP);

	lotus_fmc_write(lotus_fmc, (u32)spinor->dma_paddr, LOTUS_FMC100_DMA_SADDR_D0);
	lotus_fmc_write(lotus_fmc, LOTUS_FMC100_DMA_LEN_MASK(size), LOTUS_FMC100_DMA_LEN);

	/* set 3/4 bytes address mode */
	regval = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_CFG);
	if (spinor->addr_cycle >= 4)
		regval |= LOTUS_FMC100_CFG_SPI_NOR_ADDR_MODE_4B;
	else
		regval &= ~LOTUS_FMC100_CFG_SPI_NOR_ADDR_MODE_4B;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_CFG);

	/* set address cycle and chip select */
	regval = xfer->op_cfg;
	if (lotus_fmc->chipselect)
		regval |= LOTUS_FMC100_OP_CFG_FM_CS;
	regval |= LOTUS_FMC100_OP_CFG_OEN_EN;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_OP_CFG);

	/* set address */
	lotus_fmc_write(lotus_fmc, 0, LOTUS_FMC100_ADDRH);
	lotus_fmc_write(lotus_fmc, offset, LOTUS_FMC100_ADDRL);

	lotus_fmc->irq_enable(lotus_fmc);

	lotus_fmc_write(lotus_fmc, xfer->op_ctrl, LOTUS_FMC100_OP_CTRL);

	lotus_fmc->wait_dma_finish(lotus_fmc);

	/* disable dma done interrupt */
	regval = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_INT_EN);
	regval &= ~LOTUS_FMC100_INT_EN_OP_DONE;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_INT_EN);
}

static int lotus_fmc100_spinor_dma_read(struct mtd_info *mtd, loff_t from, size_t len,
				    size_t *retlen, u_char *buf)
{
	int size;
	int nrchip = 0;
	u8 *ptr = (u8 *)buf;
	u32 offset = (u32)from;
	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)mtd->priv;
	struct lotus_fmc_host *lotus_fmc = spinor->host;
	struct lotus_fmc_reg *read = &spinor->xfer.read;

	mutex_lock(&lotus_fmc->lock);
	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, read->clock);

	nrchip = (int)(offset >> spinor->chipsize_shift);

	lotus_fmc100_spinor_select_chip(spinor, nrchip);
	spinor->regop->wait_device_ready(spinor->regop);

	offset = offset & spinor->chipsize_mask;

	while (len > 0) {
		if (offset & LOTUS_FMC100_SPINOR_DMA_SIZE_MASK) {
			size = LOTUS_FMC100_SPINOR_DMA_SIZE -
				(offset & LOTUS_FMC100_SPINOR_DMA_SIZE_MASK);
			if (size > len)
				size = len;
		} else {
			if (len > LOTUS_FMC100_SPINOR_DMA_SIZE)
				size = LOTUS_FMC100_SPINOR_DMA_SIZE;
			else
				size = len;
		}

		lotus_fmc100_spinor_dma_transfer(spinor, read, offset, size);

		memcpy(ptr, spinor->dma_vaddr, size);

		offset += size;
		ptr += size;
		len -= size;

		if (offset > spinor->chipsize) {
			offset -= spinor->chipsize;

			lotus_fmc100_spinor_select_chip(spinor, ++nrchip);
			spinor->regop->wait_device_ready(spinor->regop);
		}
	}

	*retlen = (size_t)(ptr - buf);

	mutex_unlock(&lotus_fmc->lock);

	return 0;
}

static int lotus_fmc100_spinor_dma_write(struct mtd_info *mtd, loff_t to, size_t len,
				     size_t *retlen, const u_char *buf)
{
	int size;
	int nrchip;
	u8 *ptr = (u8 *)buf;
	u32 offset = (u32)to;
	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)mtd->priv;
	struct lotus_fmc_host *lotus_fmc = spinor->host;
	struct lotus_fmc_reg *write = &spinor->xfer.write;

	mutex_lock(&lotus_fmc->lock);
	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, write->clock);

	nrchip = (int)(offset >> spinor->chipsize_shift);

	lotus_fmc100_spinor_select_chip(spinor, nrchip);
	spinor->regop->wait_device_ready(spinor->regop);
	spinor->drv->write_enable(spinor->regop);

	offset = offset & spinor->chipsize_mask;

	while (len > 0) {
		if (offset & LOTUS_FMC100_SPINOR_DMA_SIZE_MASK) {
			size = LOTUS_FMC100_SPINOR_DMA_SIZE -
				(offset & LOTUS_FMC100_SPINOR_DMA_SIZE_MASK);
			if (size > len)
				size = len;
		} else {
			if (len > LOTUS_FMC100_SPINOR_DMA_SIZE)
				size = LOTUS_FMC100_SPINOR_DMA_SIZE;
			else
				size = len;
		}

		memcpy(spinor->dma_vaddr, ptr, size);

		lotus_fmc100_spinor_dma_transfer(spinor, write, offset, size);

		offset += size;
		ptr += size;
		len -= size;

		if (offset > spinor->chipsize) {
			offset -= spinor->chipsize;

			lotus_fmc100_spinor_select_chip(spinor, ++nrchip);
			spinor->regop->wait_device_ready(spinor->regop);
			spinor->drv->write_enable(spinor->regop);
		}
	}

	*retlen = (size_t)(ptr - buf);

	mutex_unlock(&lotus_fmc->lock);

	return 0;
}

static u32 lotus_fmc100_spinor_op_iftype(u32 op)
{
	int ix;
	u32 op_iftype[] = {
		SPINOR_OP_READ,      LOTUS_FMC100_OP_CFG_MEM_IFTYPE_STD,
		SPINOR_OP_READ_FAST, LOTUS_FMC100_OP_CFG_MEM_IFTYPE_STD,
		SPINOR_OP_PP,        LOTUS_FMC100_OP_CFG_MEM_IFTYPE_STD,
		SPINOR_OP_PP_4B,     LOTUS_FMC100_OP_CFG_MEM_IFTYPE_STD,
		SPINOR_OP_READ_4B,   LOTUS_FMC100_OP_CFG_MEM_IFTYPE_STD,
		SPINOR_OP_READ_DUAL, LOTUS_FMC100_OP_CFG_MEM_IFTYPE_DUAL,
		SPINOR_OP_READ_DUAL_4B, LOTUS_FMC100_OP_CFG_MEM_IFTYPE_DUAL,
		SPINOR_OP_READ_QUAD, LOTUS_FMC100_OP_CFG_MEM_IFTYPE_QUAD,
		SPINOR_OP_PP_QUAD,   LOTUS_FMC100_OP_CFG_MEM_IFTYPE_QUAD,
		SPINOR_OP_READ_QUAD_4B,   LOTUS_FMC100_OP_CFG_MEM_IFTYPE_QUAD,
		SPINOR_OP_PP_QUAD_4B,     LOTUS_FMC100_OP_CFG_MEM_IFTYPE_QUAD,
		SPINOR_OP_READ_QUAD_ADDR, LOTUS_FMC100_OP_CFG_MEM_IFTYPE_QUAD_ADDR,
	};

	for (ix = 0; ix < ARRAY_SIZE(op_iftype); ix += 2) {
		if (op_iftype[ix] == op)
			return op_iftype[ix+1];
	}

	panic(DEVNAME "invalid spi-nor op cmd: 0x%02x\n", op);

	return 0;
}

static void lotus_fmc100_spinor_setup(struct lotus_fmc_spinor *spinor,
				  struct spinor_chip_xfer *chip_xfer)
{
	struct lotus_fmc_reg *reg;
	struct spiflash_xfer *xfer;

	/* setup erase config */
	xfer = chip_xfer->erase;
	reg = &spinor->xfer.erase;

	spinor->erasesize = xfer->size;
	reg->cmd = xfer->cmd;
	reg->clock = xfer->clock * 2000000;
	reg->dummy = xfer->dummy;

	/* setup read config */
	xfer = chip_xfer->read;
	reg = &spinor->xfer.read;

	reg->clock = xfer->clock * 2000000;
	reg->cmd = xfer->cmd;
	reg->dummy = xfer->dummy;

	reg->op_cfg = LOTUS_FMC100_OP_CFG_ADDR_NUM(spinor->addr_cycle) |
		LOTUS_FMC100_OP_CFG_DUMMY_NUM(reg->dummy) |
		lotus_fmc100_spinor_op_iftype(xfer->cmd);

	reg->op_ctrl = LOTUS_FMC100_OP_CTRL_DMA_OP_READY |
		LOTUS_FMC100_OP_CTRL_RD_OPCODE(xfer->cmd);

	/* setup write config */
	xfer = chip_xfer->write;
	reg = &spinor->xfer.write;

	reg->clock = xfer->clock * 2000000;
	reg->cmd = xfer->cmd;
	reg->dummy = xfer->dummy;

	reg->op_cfg = LOTUS_FMC100_OP_CFG_ADDR_NUM(spinor->addr_cycle) |
		LOTUS_FMC100_OP_CFG_DUMMY_NUM(reg->dummy) |
		lotus_fmc100_spinor_op_iftype(xfer->cmd);

	reg->op_ctrl = LOTUS_FMC100_OP_CTRL_DMA_OP_READY |
		LOTUS_FMC100_OP_CTRL_RW_OP_WRITE |
		LOTUS_FMC100_OP_CTRL_WR_OPCODE(xfer->cmd);
}

static int lotus_fmc100_spinor_probe_device(struct lotus_fmc_spinor *spinor)
{
	int nrchip;
	u8 id[FLASH_ID_LEN];
	struct flash_regop_info info;
	struct spinor_chip_xfer *chip_xfer = NULL;
	struct spinor_flash_dev *flash_dev = NULL;

	spinor->chipsize = 0;
	spinor->nr_chips = 0;
	spinor->erasesize = 0;

	clk_set_rate(spinor->host->clk, 12000000/* 12MHz */);

	for (nrchip = 0; nrchip < LOTUS_FMC100_OP_CFG_NUM_CS; nrchip++) {
		memset(&info, 0, sizeof(info));

		spinor->host->chipselect = nrchip;

		info.priv = spinor->host;
		info.cmd = SPINOR_OP_RDID;
		info.nr_cmd = 1;
		info.buf = id;
		info.sz_buf = sizeof(id);
		spinor->regop->read_reg(&info);

		if (!id[0])
			continue;

		pr_info("id: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
			id[0], id[1], id[2], id[3], id[4], id[5]);

		spinor->chipselect[spinor->nr_chips] = nrchip;

		if (!spinor->nr_chips) {
			flash_dev = spinor_match_id(id, sizeof(id));
			if (flash_dev == NULL) {
				pr_info("Can't recognize this chip(cs%d).\n", nrchip);
				return -EINVAL;
			}

			spinor->nr_chips = 1;
			spinor->chipsize = flash_dev->chipsize;
			spinor->chipsize_shift = ffs(spinor->chipsize) - 1;
			spinor->chipsize_mask = spinor->chipsize - 1;

			spinor->drv = spinor_get_driver(id, sizeof(id));

			if (spinor->chipsize >= SZ_32M)
				spinor->addr_cycle = 4;
			else
				spinor->addr_cycle = 3;

			continue;
		}

		if (flash_dev != NULL && memcmp(id, flash_dev->id, flash_dev->id_len))
			break;

		spinor->nr_chips++;
	}

	if (flash_dev == NULL)
		return -EINVAL;

	/* anytime, there is a default value return. */
	chip_xfer = spinor_get_xfer(lotus_fmc100_spinor_chip_xfer,
				    (char *)flash_dev->id, flash_dev->id_len);

	lotus_fmc100_spinor_setup(spinor, chip_xfer);

	pr_info("Block:%dB Chip:%dB Name:\"%s\"\n", spinor->erasesize, spinor->chipsize,
		flash_dev->name);

	return 0;
}

static void lotus_fmc100_spinor_chip_init(struct lotus_fmc_spinor *spinor)
{
	int nrchip;

	for (nrchip = 0; nrchip < spinor->nr_chips; nrchip++) {
		lotus_fmc100_spinor_select_chip(spinor, nrchip);

		if (spinor->drv->quad_enable(spinor->regop))
			pr_warn("quad enable fail!\n");

		if (spinor->addr_cycle >= 4)
			spinor->drv->entry_4addr(spinor->regop);
	}
}

static int lotus_fmc100_spinor_regop_open(void *priv)
{
	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)priv;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	mutex_lock(&lotus_fmc->lock);
	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, 0);
	lotus_fmc100_spinor_select_chip(spinor, 0);

	return 0;
}

static int lotus_fmc100_spinor_regop_release(void *priv)
{
	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)priv;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	mutex_unlock(&lotus_fmc->lock);

	return 0;
}

static void lotus_fmc100_spinor_shutdown(struct lotus_fmc_spinor *spinor)
{
	int nrchip;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	mutex_lock(&lotus_fmc->lock);

	lotus_fmc->fmc_crg_value = readl(lotus_fmc->fmc_crg_addr);

	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, 0);

	for (nrchip = 0; nrchip < spinor->nr_chips; nrchip++) {
		lotus_fmc100_spinor_select_chip(spinor, nrchip);

		//spinor->drv->quad_disable(spinor->regop);
		if (spinor->addr_cycle >= 4)
			spinor->drv->exit_4addr(spinor->regop);
	}

	mutex_unlock(&lotus_fmc->lock);
}

static void lotus_fmc100_spinor_suspend(struct lotus_fmc_spinor *spinor)
{
	lotus_fmc100_spinor_shutdown(spinor);
}

static void lotus_fmc100_spinor_resume(struct lotus_fmc_spinor *spinor)
{
	u32 regval;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	mutex_lock(&lotus_fmc->lock);

	regval = lotus_fmc_read(lotus_fmc, LOTUS_FMC100_CFG);
	regval |= LOTUS_FMC100_CFG_OP_MODE_NORMAL;
	lotus_fmc_write(lotus_fmc, regval, LOTUS_FMC100_CFG);

	lotus_fmc_write(lotus_fmc, LOTUS_FMC100_TIMING_SPI_CFG_DEF, LOTUS_FMC100_TIMING_SPI_CFG);

	lotus_fmc->set_ifmode(lotus_fmc, LOTUS_FMC_IFMODE_SPINOR);
	clk_set_rate(lotus_fmc->clk, 0);

	lotus_fmc100_spinor_chip_init(spinor);

	writel(lotus_fmc->fmc_crg_value, lotus_fmc->fmc_crg_addr);

	mutex_unlock(&lotus_fmc->lock);
}

int lotus_fmc100_spinor_probe(struct platform_device *pdev, struct lotus_fmc_host *host)
{
	int ret;
	struct mtd_info *mtd = NULL;
	struct lotus_fmc_spinor *spinor = NULL;
	struct flash_regop_intf *regop_intf = NULL;

	spinor = devm_kzalloc(&pdev->dev, sizeof(struct lotus_fmc_spinor), GFP_KERNEL);
	if (spinor == NULL) {
		pr_err("failed to allocate lotus_fmc_spinor structure.\n");
		return -ENOMEM;
	}

	spinor->name = "spinor";
	spinor->host = host;
	spinor->dev = &pdev->dev;
	spinor->regop->priv = host;
	spinor->regop->read_reg = lotus_fmc100_read_reg;
	spinor->regop->write_reg = lotus_fmc100_write_reg;
	spinor->regop->read_sr = lotus_fmc100_spinor_read_sr;
	spinor->regop->wait_device_ready = lotus_fmc100_spinor_wait_device_ready;

	spinor->shutdown = lotus_fmc100_spinor_shutdown;
	spinor->resume = lotus_fmc100_spinor_resume;
	spinor->suspend = lotus_fmc100_spinor_suspend;

	host->spinor = spinor;

	mtd = spinor->mtd;
	mtd->name = "sfc";
	mtd->type = MTD_NORFLASH;
	mtd->writesize = 1;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->owner = THIS_MODULE;
	mtd->priv = spinor;

	mtd->_erase = lotus_fmc100_spinor_erase;
	mtd->_write = lotus_fmc100_spinor_dma_write;
	mtd->_read  = lotus_fmc100_spinor_dma_read;

	host->set_ifmode(host, LOTUS_FMC_IFMODE_SPINOR);
	lotus_fmc_write(host, LOTUS_FMC100_TIMING_SPI_CFG_DEF, LOTUS_FMC100_TIMING_SPI_CFG);

	ret = lotus_fmc100_spinor_probe_device(spinor);
	if (ret)
		goto fail;

	lotus_fmc100_spinor_chip_init(spinor);

	mtd->size = spinor->chipsize * spinor->nr_chips;
	mtd->erasesize = spinor->erasesize;

	spinor->dma_vaddr = dma_alloc_coherent(spinor->dev,
		LOTUS_FMC100_SPINOR_DMA_SIZE, &spinor->dma_paddr, GFP_KERNEL);
	if (spinor->dma_vaddr == NULL) {
		pr_err("failed to allocate dma memory.\n");
		ret = -ENOMEM;
		goto fail;
	}

	ret = mtd_device_parse_register(mtd, NULL, NULL, NULL, 0);
	if (ret) {
		dma_free_coherent(spinor->dev, LOTUS_FMC100_SPINOR_DMA_SIZE,
			spinor->dma_vaddr, spinor->dma_paddr);
		goto fail;
	}

	regop_intf = &spinor->regop_intf;

	strncpy(regop_intf->name, spinor->name, sizeof(regop_intf->name) - 1);

	regop_intf->sz_buf = host->sz_iobase;
	regop_intf->args = spinor;
	regop_intf->regop = spinor->regop;
	regop_intf->open = lotus_fmc100_spinor_regop_open;
	regop_intf->release = lotus_fmc100_spinor_regop_release;

	regop_intf_create(regop_intf);

	return 0;
fail:
	host->spinor = NULL;
	devm_kfree(spinor->dev, spinor);

	return ret;
}

int lotus_fmc100_spinor_destory(struct lotus_fmc_spinor *spinor)
{
	regop_intf_destory(&spinor->regop_intf);

	mtd_device_unregister(spinor->mtd);

	dma_free_coherent(spinor->dev, LOTUS_FMC100_SPINOR_DMA_SIZE,
		spinor->dma_vaddr, spinor->dma_paddr);

	devm_kfree(spinor->dev, spinor);

	return 0;
}

int lotus_fmc100_spinor_cmd_opt(struct mtd_info *mtd, struct mtd_cmd_opt *cmd_opt)
{
	struct flash_regop_info info = {0};

	struct lotus_fmc_spinor *spinor = (struct lotus_fmc_spinor *)mtd->priv;
	struct flash_regop *regop = spinor->regop;
	struct lotus_fmc_host *lotus_fmc = spinor->host;

	mutex_lock(&lotus_fmc->lock);

	info.priv = regop->priv;
	info.cmd = cmd_opt->cmd;
	info.nr_cmd = 1;
	info.addrl = cmd_opt->addr;
	info.addr_cycle = cmd_opt->addr_cycle;
	info.dummy = cmd_opt->dummy;
	info.buf = cmd_opt->data;
	info.sz_buf = cmd_opt->data_len;

	if (cmd_opt->op)
		regop->write_reg(&info);
	else
		regop->read_reg(&info);

	mutex_unlock(&lotus_fmc->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(lotus_fmc100_spinor_cmd_opt);

