/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mtd/nand.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/setup.h>

#include "../raw/nfc_gen.h"
#include "xmedia_fmc100.h"

static void xmedia_fmc100_switch_to_spi_nand(struct xmedia_fmc_host *host)
{
	u32 reg;

	reg = fmc_readl(host, FMC_CFG);
	reg &= ~FLASH_TYPE_SEL_MASK;
	reg |= FMC_CFG_FLASH_SEL(FLASH_TYPE_SPI_NAND);
	fmc_writel(host, FMC_CFG, reg);
}

static void xmedia_fmc100_set_str_mode(const struct xmedia_fmc_host *host)
{
	u32 reg;

	reg = fmc_readl(host, FMC_GLOBAL_CFG);
	reg &= (~FMC_GLOBAL_CFG_DTR_MODE);
	fmc_writel(host, FMC_GLOBAL_CFG, reg);
}

static void xmedia_fmc100_operation_config(struct xmedia_fmc_host *host, int op)
{
	int ret;
	unsigned long clkrate = 0;
	struct xmedia_fmc_spi *spi = host->spi;

	xmedia_fmc100_switch_to_spi_nand(host);
	clk_prepare_enable(host->clk);
	switch (op) {
	case OP_STYPE_WRITE:
		clkrate = min((u_long)host->clkrate,
			      (u_long)CLK_FMC_TO_CRG_MHZ(spi->write->clock));
		break;
	case OP_STYPE_READ:
		clkrate = min((u_long)host->clkrate,
			      (u_long)CLK_FMC_TO_CRG_MHZ(spi->read->clock));
		break;
	case OP_STYPE_ERASE:
		clkrate = min((u_long)host->clkrate,
			      (u_long)CLK_FMC_TO_CRG_MHZ(spi->erase->clock));
		break;
	default:
		break;
	}

	ret = clk_set_rate(host->clk, clkrate);
	if (WARN_ON(ret)) {
		pr_err("clk_set_rate failed: %d\n", ret);
	}
}

static void xmedia_fmc100_send_cmd_write(struct xmedia_fmc_host *host)
{
	unsigned char pages_per_block_shift;
	unsigned int reg, block_num, block_num_h, page_num;
	int ret;
	struct xmedia_fmc_spi *spi = host->spi;
	struct nand_chip *chip = host->chip;
#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_WRITE
	const char *op = "Reg";
#else
	const char *op = "Dma";
#endif

	if (WR_DBG) {
		pr_info("\n");
	}
	FMC_PR(WR_DBG, "*-Start send %s page write command\n", op);

	mutex_lock(host->lock);
	xmedia_fmc100_operation_config(host, OP_STYPE_WRITE);

	ret = spi->driver->wait_ready(spi);
	if (ret) {
		DB_MSG("Error: %s program wait ready failed! status: %#x\n",
		       op, ret);
		goto end;
	}

	ret = spi->driver->write_enable(spi);
	if (ret) {
		DB_MSG("Error: %s program write enable failed! ret: %#x\n",
		       op, ret);
		goto end;
	}

	reg = FMC_INT_CLR_ALL;
	fmc_writel(host, FMC_INT_CLR, reg);
	FMC_PR(WR_DBG, "|-Set INT_CLR[%#x]%#x\n", FMC_INT_CLR, reg);

	reg = OP_CFG_FM_CS(host->cmd_op.cs)
	      | OP_CFG_MEM_IF_TYPE(spi->write->iftype)
	      | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(WR_DBG, "|-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	pages_per_block_shift = chip->phys_erase_shift - chip->page_shift;
	block_num = host->addr_value[1] >> pages_per_block_shift;
	block_num_h = block_num >> REG_CNT_HIGH_BLOCK_NUM_SHIFT;
	reg = FMC_ADDRH_SET(block_num_h);
	fmc_writel(host, FMC_ADDRH, reg);
	FMC_PR(WR_DBG, "|-Set ADDRH[%#x]%#x\n", FMC_ADDRH, reg);

	page_num = host->addr_value[1] - (block_num << pages_per_block_shift);
	reg = ((block_num & REG_CNT_BLOCK_NUM_MASK) << REG_CNT_BLOCK_NUM_SHIFT)
	      | ((page_num & REG_CNT_PAGE_NUM_MASK) << REG_CNT_PAGE_NUM_SHIFT);
	fmc_writel(host, FMC_ADDRL, reg);
	FMC_PR(WR_DBG, "|-Set ADDRL[%#x]%#x\n", FMC_ADDRL, reg);

	*host->epm = 0x0000;

#ifndef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_WRITE
	reg = host->dma_buffer;
	fmc_writel(host, FMC_DMA_SADDR_D0, reg);
	FMC_PR(WR_DBG, "|-Set DMA_SADDR_D[0x40]%#x\n", reg);

#ifdef CONFIG_64BIT
	reg = (host->dma_buffer & FMC_DMA_SADDRH_MASK) >> 32;
	fmc_writel(host, FMC_DMA_SADDRH_D0, reg);
	FMC_PR(WR_DBG, "\t|-Set DMA_SADDRH_D0[%#x]%#x\n", FMC_DMA_SADDRH_D0, reg);
#endif

	reg = host->dma_oob;
	fmc_writel(host, FMC_DMA_SADDR_OOB, reg);
	FMC_PR(WR_DBG, "|-Set DMA_SADDR_OOB[%#x]%#x\n", FMC_DMA_SADDR_OOB, reg);
#ifdef CONFIG_64BIT
	reg = (host->dma_oob & FMC_DMA_SADDRH_MASK) >> 32;
	fmc_writel(host, FMC_DMA_SADDRH_OOB, reg);
	FMC_PR(WR_DBG, "\t|-Set DMA_SADDRH_OOB[%#x]%#x\n", FMC_DMA_SADDRH_OOB,
	       reg);
#endif
#endif

	reg = OP_CTRL_WR_OPCODE(spi->write->cmd)
#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_WRITE
	      | OP_CTRL_DMA_OP(OP_TYPE_REG)
#else
	      | OP_CTRL_DMA_OP(OP_TYPE_DMA)
#endif
	      | OP_CTRL_RW_OP(RW_OP_WRITE)
	      | OP_CTRL_DMA_OP_READY;
	fmc_writel(host, FMC_OP_CTRL, reg);
	FMC_PR(WR_DBG, "|-Set OP_CTRL[%#x]%#x\n", FMC_OP_CTRL, reg);

	FMC_DMA_WAIT_INT_FINISH(host);

end:
	mutex_unlock(host->lock);
	FMC_PR(WR_DBG, "*-End %s page program!\n", op);
}

static void xmedia_fmc100_send_cmd_status(struct xmedia_fmc_host *host)
{
	u_char status;
	int ret;
	unsigned char addr = STATUS_ADDR;
	struct xmedia_fmc_spi *spi = NULL;

	if(host == NULL || host->spi == NULL) {
		DB_MSG("Error: host or host->spi is NULL!\n");
		return;
	}
	spi = host->spi;
	if (host->cmd_op.l_cmd == NAND_CMD_GET_FEATURES) {
		addr = PROTECT_ADDR;
	}

	ret = spi_nand_feature_op(spi, GET_OP, addr, &status);
	if (ret)
		return;
	FMC_PR((ER_DBG || WR_DBG), "\t*-Get status[%#x]: %#x\n", addr, status);
}

static void xmedia_fmc100_send_cmd_read(struct xmedia_fmc_host *host)
{
	unsigned char pages_per_block_shift;
	unsigned int reg, block_num, block_num_h, page_num;
	struct xmedia_fmc_spi *spi = host->spi;
	struct nand_chip *chip = host->chip;
	int ret;
#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_READ
	char *op = "Reg";
#else
	char *op = "Dma";
#endif

	if (RD_DBG) {
		pr_info("\n");
	}
	FMC_PR(RD_DBG, "\t*-Start %s page read\n", op);

	if ((host->addr_value[0] == host->cache_addr_value[0])
			&& (host->addr_value[1] == host->cache_addr_value[1])) {
		FMC_PR(RD_DBG, "\t*-%s read cache hit, addr[%#x %#x]\n",
		       op, host->addr_value[1], host->addr_value[0]);
		return;
	}

	mutex_lock(host->lock);
	xmedia_fmc100_operation_config(host, OP_STYPE_READ);

	FMC_PR(RD_DBG, "\t|-Wait ready before %s page read\n", op);
	ret = spi->driver->wait_ready(spi);
	if (ret) {
		DB_MSG("Error: %s read wait ready fail! ret: %#x\n", op, ret);
		goto end;
	}

	reg = FMC_INT_CLR_ALL;
	fmc_writel(host, FMC_INT_CLR, reg);
	FMC_PR(RD_DBG, "\t|-Set INT_CLR[%#x]%#x\n", FMC_INT_CLR, reg);

	if (host->cmd_op.l_cmd == NAND_CMD_READOOB)
		host->cmd_op.op_cfg = OP_CTRL_RD_OP_SEL(RD_OP_READ_OOB);
	else
		host->cmd_op.op_cfg = OP_CTRL_RD_OP_SEL(RD_OP_READ_ALL_PAGE);

	reg = OP_CFG_FM_CS(host->cmd_op.cs)
	      | OP_CFG_MEM_IF_TYPE(spi->read->iftype)
	      | OP_CFG_DUMMY_NUM(spi->read->dummy)
	      | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(RD_DBG, "\t|-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	pages_per_block_shift = chip->phys_erase_shift - chip->page_shift;
	block_num = host->addr_value[1] >> pages_per_block_shift;
	block_num_h = block_num >> REG_CNT_HIGH_BLOCK_NUM_SHIFT;

	reg = FMC_ADDRH_SET(block_num_h);
	fmc_writel(host, FMC_ADDRH, reg);
	FMC_PR(RD_DBG, "\t|-Set ADDRH[%#x]%#x\n", FMC_ADDRH, reg);

	page_num = host->addr_value[1] - (block_num << pages_per_block_shift);
	reg = ((block_num & REG_CNT_BLOCK_NUM_MASK) << REG_CNT_BLOCK_NUM_SHIFT)
	      | ((page_num & REG_CNT_PAGE_NUM_MASK) << REG_CNT_PAGE_NUM_SHIFT);
	fmc_writel(host, FMC_ADDRL, reg);
	FMC_PR(RD_DBG, "\t|-Set ADDRL[%#x]%#x\n", FMC_ADDRL, reg);

#ifndef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_READ
	reg = host->dma_buffer;
	fmc_writel(host, FMC_DMA_SADDR_D0, reg);
	FMC_PR(RD_DBG, "\t|-Set DMA_SADDR_D0[%#x]%#x\n", FMC_DMA_SADDR_D0, reg);

#ifdef CONFIG_64BIT
	reg = (host->dma_buffer & FMC_DMA_SADDRH_MASK) >> 32;
	fmc_writel(host, FMC_DMA_SADDRH_D0, reg);
	FMC_PR(RD_DBG, "\t|-Set DMA_SADDRH_D0[%#x]%#x\n", FMC_DMA_SADDRH_D0, reg);
#endif

	reg = host->dma_oob;
	fmc_writel(host, FMC_DMA_SADDR_OOB, reg);
	FMC_PR(RD_DBG, "\t|-Set DMA_SADDR_OOB[%#x]%#x\n", FMC_DMA_SADDR_OOB,
	       reg);

#ifdef CONFIG_64BIT
	reg = (host->dma_oob & FMC_DMA_SADDRH_MASK) >> 32;
	fmc_writel(host, FMC_DMA_SADDRH_OOB, reg);
	FMC_PR(RD_DBG, "\t|-Set DMA_SADDRH_OOB[%#x]%#x\n", FMC_DMA_SADDRH_OOB,
	       reg);
#endif
#endif

	reg = OP_CTRL_RD_OPCODE(spi->read->cmd) | host->cmd_op.op_cfg
#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_READ
	      | OP_CTRL_DMA_OP(OP_TYPE_REG)
#else
	      | OP_CTRL_DMA_OP(OP_TYPE_DMA)
#endif
	      | OP_CTRL_RW_OP(RW_OP_READ) | OP_CTRL_DMA_OP_READY;
	fmc_writel(host, FMC_OP_CTRL, reg);
	FMC_PR(RD_DBG, "\t|-Set OP_CTRL[%#x]%#x\n", FMC_OP_CTRL, reg);

	FMC_DMA_WAIT_INT_FINISH(host);

	host->cache_addr_value[0] = host->addr_value[0];
	host->cache_addr_value[1] = host->addr_value[1];

end:
	mutex_unlock(host->lock);
	FMC_PR(RD_DBG, "\t*-End %s page read\n", op);
}

static void xmedia_fmc100_send_cmd_erase(struct xmedia_fmc_host *host)
{
	unsigned int reg;
	struct xmedia_fmc_spi *spi = host->spi;
	int ret;

	if (ER_DBG) {
		pr_info("\n");
	}
	FMC_PR(ER_DBG, "\t*-Start send cmd erase!\n");

	mutex_lock(host->lock);
	xmedia_fmc100_operation_config(host, OP_STYPE_ERASE);

	ret = spi->driver->wait_ready(spi);
	FMC_PR(ER_DBG, "\t|-Erase wait ready, ret: %#x\n", ret);
	if (ret) {
		DB_MSG("Error: Erase wait ready fail! status: %#x\n", ret);
		goto end;
	}

	ret = spi->driver->write_enable(spi);
	if (ret) {
		DB_MSG("Error: Erase write enable failed! ret: %#x\n", ret);
		goto end;
	}

	reg = FMC_INT_CLR_ALL;
	fmc_writel(host, FMC_INT_CLR, reg);
	FMC_PR(ER_DBG, "\t|-Set INT_CLR[%#x]%#x\n", FMC_INT_CLR, reg);

	reg = spi->erase->cmd;
	fmc_writel(host, FMC_CMD, FMC_CMD_CMD1(reg));
	FMC_PR(ER_DBG, "\t|-Set CMD[%#x]%#x\n", FMC_CMD, reg);

	reg = FMC_ADDRL_BLOCK_H_MASK(host->addr_value[1])
	      | FMC_ADDRL_BLOCK_L_MASK(host->addr_value[0]);
	fmc_writel(host, FMC_ADDRL, reg);
	FMC_PR(ER_DBG, "\t|-Set ADDRL[%#x]%#x\n", FMC_ADDRL, reg);

	reg = OP_CFG_FM_CS(host->cmd_op.cs)
	      | OP_CFG_MEM_IF_TYPE(spi->erase->iftype)
	      | OP_CFG_ADDR_NUM(STD_OP_ADDR_NUM)
	      | OP_CFG_DUMMY_NUM(spi->erase->dummy)
	      | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(ER_DBG, "\t|-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	reg = FMC_OP_CMD1_EN
	      | FMC_OP_ADDR_EN
	      | FMC_OP_REG_OP_START;
	fmc_writel(host, FMC_OP, reg);
	FMC_PR(ER_DBG, "\t|-Set OP[%#x]%#x\n", FMC_OP, reg);

	FMC_CMD_WAIT_CPU_FINISH(host);

end:
	mutex_unlock(host->lock);
	FMC_PR(ER_DBG, "\t*-End send cmd erase!\n");
}

void xmedia_fmc100_ecc0_switch(struct xmedia_fmc_host *host, unsigned char op)
{
	unsigned int config;
#if EC_DBG
	unsigned int cmp_cfg;

	if(host == NULL) {
		DB_MSG("Error: host is NULL!\n");
		return;
	}
	config = fmc_readl(host, FMC_CFG);
	FMC_PR(EC_DBG, "\t *-Get CFG[%#x]%#x\n", FMC_CFG, config);

	if (op) {
		cmp_cfg = host->fmc_cfg;
	} else {
		cmp_cfg = host->fmc_cfg_ecc0;
	}

	if (cmp_cfg != config)
		DB_MSG("Warning: FMC config[%#x] is different.\n",
		       cmp_cfg);
#endif
	if(host == NULL) {
		DB_MSG("Error: host is NULL!\n");
		return;
	}
	if (op == ENABLE) {
		config = host->fmc_cfg_ecc0;
	} else if (op == DISABLE) {
		config = host->fmc_cfg;
	} else {
		DB_MSG("Error: Invalid opcode: %d\n", op);
		return;
	}

	fmc_writel(host, FMC_CFG, config);
	FMC_PR(EC_DBG, "\t *-Set CFG[%#x]%#x\n", FMC_CFG, config);
}

static void xmedia_fmc100_send_cmd_readid(struct xmedia_fmc_host *host)
{
	unsigned int reg;

	FMC_PR(BT_DBG, "\t|*-Start send cmd read ID\n");

	xmedia_fmc100_ecc0_switch(host, ENABLE);

	reg = FMC_CMD_CMD1(SPI_CMD_RDID);
	fmc_writel(host, FMC_CMD, reg);
	FMC_PR(BT_DBG, "\t||-Set CMD[%#x]%#x\n", FMC_CMD, reg);

	reg = READ_ID_ADDR;
	fmc_writel(host, FMC_ADDRL, reg);
	FMC_PR(BT_DBG, "\t||-Set ADDRL[%#x]%#x\n", FMC_ADDRL, reg);

	reg = OP_CFG_FM_CS(host->cmd_op.cs)
	      | OP_CFG_ADDR_NUM(READ_ID_ADDR_NUM)
	      | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(BT_DBG, "\t||-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	reg = FMC_DATA_NUM_CNT(MAX_SPI_NAND_ID_LEN);
	fmc_writel(host, FMC_DATA_NUM, reg);
	FMC_PR(BT_DBG, "\t||-Set DATA_NUM[%#x]%#x\n", FMC_DATA_NUM, reg);

	reg = FMC_OP_CMD1_EN
	      | FMC_OP_ADDR_EN
	      | FMC_OP_READ_DATA_EN
	      | FMC_OP_REG_OP_START;
	fmc_writel(host, FMC_OP, reg);
	FMC_PR(BT_DBG, "\t||-Set OP[%#x]%#x\n", FMC_OP, reg);

	host->addr_cycle = 0x0;

	FMC_CMD_WAIT_CPU_FINISH(host);

	xmedia_fmc100_ecc0_switch(host, DISABLE);

	FMC_PR(BT_DBG, "\t|*-End read flash ID\n");
}

static void xmedia_fmc100_send_cmd_reset(struct xmedia_fmc_host *host)
{
	unsigned int reg;

	FMC_PR(BT_DBG, "\t|*-Start send cmd reset\n");

	reg = FMC_CMD_CMD1(SPI_CMD_RESET);
	fmc_writel(host, FMC_CMD, reg);
	FMC_PR(BT_DBG, "\t||-Set CMD[%#x]%#x\n", FMC_CMD, reg);

	reg = OP_CFG_FM_CS(host->cmd_op.cs) | OP_CFG_OEN_EN;
	fmc_writel(host, FMC_OP_CFG, reg);
	FMC_PR(BT_DBG, "\t||-Set OP_CFG[%#x]%#x\n", FMC_OP_CFG, reg);

	reg = FMC_OP_CMD1_EN | FMC_OP_REG_OP_START;
	fmc_writel(host, FMC_OP, reg);
	FMC_PR(BT_DBG, "\t||-Set OP[%#x]%#x\n", FMC_OP, reg);

	FMC_CMD_WAIT_CPU_FINISH(host);

	FMC_PR(BT_DBG, "\t|*-End send cmd reset\n");
}

static void xmedia_fmc100_host_init(struct xmedia_fmc_host *host)
{
	unsigned int reg;

	FMC_PR(BT_DBG, "\t||*-Start SPI Nand host init\n");

	reg = fmc_readl(host, FMC_CFG);
	if ((reg & FMC_CFG_OP_MODE_MASK) == FMC_CFG_OP_MODE_BOOT) {
		reg |= FMC_CFG_OP_MODE(FMC_CFG_OP_MODE_NORMAL);
		fmc_writel(host, FMC_CFG, reg);
		FMC_PR(BT_DBG, "\t|||-Set CFG[%#x]%#x\n", FMC_CFG, reg);
	}

	host->fmc_cfg = reg;
	host->fmc_cfg_ecc0 = (reg & ~ECC_TYPE_MASK) | ECC_TYPE_0BIT;

	reg = fmc_readl(host, FMC_GLOBAL_CFG);
	if (reg & FMC_GLOBAL_CFG_WP_ENABLE) {
		reg &= ~FMC_GLOBAL_CFG_WP_ENABLE;
		fmc_writel(host, FMC_GLOBAL_CFG, reg);
	}

	host->addr_cycle = 0;
	host->addr_value[0] = 0;
	host->addr_value[1] = 0;
	host->cache_addr_value[0] = ~0;
	host->cache_addr_value[1] = ~0;

	host->send_cmd_write = xmedia_fmc100_send_cmd_write;
	host->send_cmd_status = xmedia_fmc100_send_cmd_status;
	host->send_cmd_read = xmedia_fmc100_send_cmd_read;
	host->send_cmd_erase = xmedia_fmc100_send_cmd_erase;
	host->send_cmd_readid = xmedia_fmc100_send_cmd_readid;
	host->send_cmd_reset = xmedia_fmc100_send_cmd_reset;
#ifdef CONFIG_PM
	host->suspend = xmedia_fmc100_suspend;
	host->resume  = xmedia_fmc100_resume;
#endif

	reg = TIMING_CFG_TCSH(CS_HOLD_TIME)
	      | TIMING_CFG_TCSS(CS_SETUP_TIME)
	      | TIMING_CFG_TSHSL(CS_DESELECT_TIME);
	fmc_writel(host, FMC_SPI_TIMING_CFG, reg);

	reg = ALL_BURST_ENABLE;
	fmc_writel(host, FMC_DMA_AHB_CTRL, reg);

	FMC_PR(BT_DBG, "\t||*-End SPI Nand host init\n");
}

static unsigned char xmedia_fmc100_read_byte(struct nand_chip *chip)
{
	struct xmedia_fmc_host *host = chip->priv;
	unsigned char value;
	unsigned char ret_val = 0;

	if (host->cmd_op.l_cmd == NAND_CMD_READID) {
		value = fmc_readb(host->iobase + host->offset);
		host->offset++;
		if (host->cmd_op.data_no == host->offset) {
			host->cmd_op.l_cmd = 0;
		}
		return value;
	}

	if (host->cmd_op.cmd == NAND_CMD_STATUS) {
		value = fmc_readl(host, FMC_STATUS);
		if (host->cmd_op.l_cmd == NAND_CMD_GET_FEATURES) {
			FMC_PR((ER_DBG || WR_DBG), "\t\tRead BP status:%#x\n",
			       value);
			if (ANY_BP_ENABLE(value)) {
				ret_val |= NAND_STATUS_WP;
			}

			host->cmd_op.l_cmd = NAND_CMD_STATUS;
		}

		if (!(value & STATUS_OIP_MASK)) {
			ret_val |= NAND_STATUS_READY;
		}

		if (value & STATUS_E_FAIL_MASK) {
			FMC_PR(ER_DBG, "\t\tGet erase status: %#x\n", value);
			ret_val |= NAND_STATUS_FAIL;
		}

		if (value & STATUS_P_FAIL_MASK) {
			FMC_PR(WR_DBG, "\t\tGet write status: %#x\n", value);
			ret_val |= NAND_STATUS_FAIL;
		}

		return ret_val;
	}

	if (host->cmd_op.l_cmd == NAND_CMD_READOOB) {
		value  = fmc_readb(host->buffer + host->pagesize + host->offset);
		host->offset++;
		return value;
	}

	host->offset++;

	return fmc_readb(host->buffer + host->column + host->offset - 1);
}

static void xmedia_fmc100_write_buf(struct nand_chip *chip,
			       const u_char *buf, int len)
{
	struct xmedia_fmc_host *host = chip->priv;

#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_WRITE
	if (buf == chip->oob_poi) {
		memcpy((char *)host->iobase + host->pagesize, buf, len);
	} else {
		memcpy((char *)host->iobase, buf, len);
	}
#else
	if (buf == chip->oob_poi) {
		memcpy((char *)(host->buffer + host->pagesize), buf, len);
	} else {
		memcpy((char *)host->buffer, buf, len);
	}
#endif
	return;
}

static void xmedia_fmc100_read_buf(struct nand_chip *chip, u_char *buf, int len)
{
	struct xmedia_fmc_host *host = chip->priv;

#ifdef XMEDIA_FMC100_SPI_NAND_SUPPORT_REG_READ
	if (buf == chip->oob_poi) {
		memcpy(buf, (char *)host->iobase + host->pagesize, len);
	} else {
		memcpy(buf, (char *)host->iobase, len);
	}
#else
	if (buf == chip->oob_poi) {
		memcpy(buf, (char *)host->buffer + host->pagesize, len);
	} else {
		memcpy(buf, (char *)host->buffer, len);
	}
#endif

#ifdef CONFIG_XMEDIA_NAND_ECC_STATUS_REPORT
	if (buf != chip->oob_poi) {
		u_int reg;
		u_int ecc_step = host->pagesize >> 10;

		reg = fmc_readl(host, XMEDIA_FMC100_ECC_ERR_NUM0_BUF0);
		while (ecc_step) {
			u_char err_num;

			err_num = GET_ECC_ERR_NUM(--ecc_step, reg);
			if (err_num == 0xff) {
				mtd->ecc_stats.failed++;
			} else {
				mtd->ecc_stats.corrected += err_num;
			}
		}
	}
#endif

	return;
}

static void xmedia_fmc100_select_chip(struct nand_chip *chip, int chipselect)
{
	struct xmedia_fmc_host *host = chip->priv;
	struct mtd_info *mtd = nand_to_mtd(chip);

	if (chipselect < 0) {
		mutex_unlock(&fmc_switch_mutex);
		return;
	}

	mutex_lock(&fmc_switch_mutex);

	if (chipselect > CONFIG_SPI_NAND_MAX_CHIP_NUM) {
		DB_BUG("Error: Invalid chipselect: %d\n", chipselect);
	}

	if (host->mtd != mtd) {
		host->mtd = mtd;
		host->cmd_op.cs = chipselect;
	}
}

static void xmedia_fmc100_cmd_ctrl(struct nand_chip *chip, int dat, unsigned ctrl)
{
	unsigned char cmd;
	int is_cache_invalid = 1;
	struct xmedia_fmc_host *host = chip->priv;
	unsigned int udat = (unsigned int)dat;

	if (ctrl & NAND_ALE) {
		unsigned int addr_value = 0;
		unsigned int addr_offset = 0;

		if (ctrl & NAND_CTRL_CHANGE) {
			host->addr_cycle = 0x0;
			host->addr_value[0] = 0x0;
			host->addr_value[1] = 0x0;
		}
		addr_offset = host->addr_cycle << 3;

		if (host->addr_cycle >= XMEDIA_FMC100_ADDR_CYCLE_MASK) {
			addr_offset = (host->addr_cycle -
				       XMEDIA_FMC100_ADDR_CYCLE_MASK) << 3;
			addr_value = 1;
		}
		host->addr_value[addr_value] |=
			((udat & 0xff) << addr_offset);

		host->addr_cycle++;
	}

	if ((ctrl & NAND_CLE) && (ctrl & NAND_CTRL_CHANGE)) {
		cmd = udat & 0xff;
		host->cmd_op.cmd = cmd;
		switch (cmd) {
		case NAND_CMD_PAGEPROG:
			host->offset = 0;
			host->send_cmd_write(host);
			break;

		case NAND_CMD_READSTART:
			is_cache_invalid = 0;
			if (host->addr_value[0] == host->pagesize) {
				host->cmd_op.l_cmd = NAND_CMD_READOOB;
			}
			host->send_cmd_read(host);
			break;

		case NAND_CMD_ERASE2:
			host->send_cmd_erase(host);
			break;

		case NAND_CMD_READID:
			memset((u_char *)(host->iobase), 0,
			       MAX_SPI_NAND_ID_LEN);
			host->cmd_op.l_cmd = cmd;
			host->cmd_op.data_no = MAX_SPI_NAND_ID_LEN;
			host->send_cmd_readid(host);
			break;

		case NAND_CMD_STATUS:
			host->send_cmd_status(host);
			break;

		case NAND_CMD_READ0:
			host->cmd_op.l_cmd = cmd;
			break;

		case NAND_CMD_RESET:
			host->send_cmd_reset(host);
			break;

		case NAND_CMD_SEQIN:
		case NAND_CMD_ERASE1:
		default:
			break;
		}
	}

	if ((dat == NAND_CMD_NONE) && host->addr_cycle) {
		if (host->cmd_op.cmd == NAND_CMD_SEQIN
				|| host->cmd_op.cmd == NAND_CMD_READ0
				|| host->cmd_op.cmd == NAND_CMD_READID) {
			host->offset = 0x0;
			host->column = (host->addr_value[0] & 0xffff);
		}
	}

	if (is_cache_invalid) {
		host->cache_addr_value[0] = ~0;
		host->cache_addr_value[1] = ~0;
	}
}

static int xmedia_fmc100_dev_ready(struct nand_chip *chip)
{
	unsigned int reg;
	unsigned long deadline = jiffies + FMC_MAX_READY_WAIT_JIFFIES;
	struct xmedia_fmc_host *host = chip->priv;

	do {
		reg = OP_CFG_FM_CS(host->cmd_op.cs) | OP_CFG_OEN_EN;
		fmc_writel(host, FMC_OP_CFG, reg);

		reg = FMC_OP_READ_STATUS_EN | FMC_OP_REG_OP_START;
		fmc_writel(host, FMC_OP, reg);

		FMC_CMD_WAIT_CPU_FINISH(host);

		reg = fmc_readl(host, FMC_STATUS);

		if (!(reg & STATUS_OIP_MASK)) {
			return NAND_STATUS_READY;
		}

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	if (!(chip->options & NAND_SCAN_SILENT_NODEV)) {
		pr_warn("Wait SPI nand ready timeout, status: %#x\n", reg);
	}

	return 0;
}

/*
 * 'host->epm' only use the first oobfree[0] field, it looks very simple, But...
 */
/* Default OOB area layout */
static int xmedia_fmc_ooblayout_ecc_default(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 32;
	oobregion->offset = 32;

	return 0;
}

static int xmedia_fmc_ooblayout_free_default(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 30;
	oobregion->offset = 2;

	return 0;
}

static struct mtd_ooblayout_ops xmedia_fmc_ooblayout_default_ops = {
	.ecc = xmedia_fmc_ooblayout_ecc_default,
	.free = xmedia_fmc_ooblayout_free_default,
};

#ifdef CONFIG_XMEDIA_NAND_FS_MAY_NO_YAFFS2
static int xmedia_fmc_ooblayout_ecc_4k16bit(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 14;
	oobregion->offset = 14;

	return 0;
}

static int xmedia_fmc_ooblayout_free_4k16bit(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 14;
	oobregion->offset = 2;

	return 0;
}

static struct mtd_ooblayout_ops xmedia_fmc_ooblayout_4k16bit_ops = {
	.ecc = xmedia_fmc_ooblayout_ecc_4k16bit,
	.free = xmedia_fmc_ooblayout_free_4k16bit,
};

static int xmedia_fmc_ooblayout_ecc_2k16bit(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 6;
	oobregion->offset = 6;

	return 0;
}

static int xmedia_fmc_ooblayout_free_2k16bit(struct mtd_info *mtd, int section,
					struct mtd_oob_region *oobregion)
{
	if (section) {
		return -ERANGE;
	}

	oobregion->length = 6;
	oobregion->offset = 2;

	return 0;
}

static struct mtd_ooblayout_ops xmedia_fmc_ooblayout_2k16bit_ops = {
	.ecc = xmedia_fmc_ooblayout_ecc_2k16bit,
	.free = xmedia_fmc_ooblayout_free_2k16bit,
};
#endif

static struct nand_config_info xmedia_fmc_spi_nand_config_table[] = {
	{NAND_PAGE_4K,  NAND_ECC_24BIT, 24, 200,    &xmedia_fmc_ooblayout_default_ops},
#ifdef CONFIG_XMEDIA_NAND_FS_MAY_NO_YAFFS2
	{NAND_PAGE_4K,  NAND_ECC_16BIT, 16, 128,    &xmedia_fmc_ooblayout_4k16bit_ops},
#endif
	{NAND_PAGE_4K,  NAND_ECC_8BIT,  8, 128,     &xmedia_fmc_ooblayout_default_ops},
	{NAND_PAGE_4K,  NAND_ECC_0BIT,  0, 32,      &xmedia_fmc_ooblayout_default_ops},
	{NAND_PAGE_2K,  NAND_ECC_24BIT, 24, 128,    &xmedia_fmc_ooblayout_default_ops},
#ifdef CONFIG_XMEDIA_NAND_FS_MAY_NO_YAFFS2
	{NAND_PAGE_2K,  NAND_ECC_16BIT, 16, 64,     &xmedia_fmc_ooblayout_2k16bit_ops},
#endif
	{NAND_PAGE_2K,  NAND_ECC_8BIT,  8, 64,      &xmedia_fmc_ooblayout_default_ops},
	{NAND_PAGE_2K,  NAND_ECC_0BIT,  0, 32,      &xmedia_fmc_ooblayout_default_ops},
	{0, 0, 0, 0, NULL},
};

/*
 * Auto-sensed the page size and ecc type value. driver will try each of page
 * size and ecc type one by one till flash can be read and wrote accurately.
 * so the page size and ecc type is match adaptively without switch on the board
 */
static struct nand_config_info *xmedia_fmc100_get_config_type_info(
	struct mtd_info *mtd, struct nand_dev_t *nand_dev)
{
	struct nand_config_info *best = NULL;
	struct nand_config_info *info = xmedia_fmc_spi_nand_config_table;

	nand_dev->start_type = "Auto";

	for (; info->ooblayout_ops; info++) {
		if (match_page_type_to_size(info->pagetype) != mtd->writesize) {
			continue;
		}

		if (mtd->oobsize < info->oobsize) {
			continue;
		}

		if (!best || (best->ecctype < info->ecctype)) {
			best = info;
		}
	}

	return best;
}

static void xmedia_fmc100_chip_init(struct nand_chip *chip)
{
	chip->legacy.read_byte = xmedia_fmc100_read_byte;
	chip->legacy.write_buf = xmedia_fmc100_write_buf;
	chip->legacy.read_buf = xmedia_fmc100_read_buf;

	chip->legacy.select_chip = xmedia_fmc100_select_chip;

	chip->legacy.cmd_ctrl = xmedia_fmc100_cmd_ctrl;
	chip->legacy.dev_ready = xmedia_fmc100_dev_ready;

	chip->legacy.chip_delay = FMC_CHIP_DELAY;

	chip->options = NAND_SKIP_BBTSCAN | NAND_BROKEN_XD
			| NAND_SCAN_SILENT_NODEV;

	chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_NONE;
}

static void xmedia_fmc100_set_oob_info(struct mtd_info *mtd,
				  struct nand_config_info *info, struct nand_dev_t *nand_dev)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct xmedia_fmc_host *host = chip->priv;
	struct mtd_oob_region xmedia_fmc_oobregion = {0, 0};
	if (info == NULL || mtd == NULL || nand_dev == NULL) {
		DB_MSG("set oob info err!!!\n");
		return;
	}

	if (info->ecctype != NAND_ECC_0BIT) {
		mtd->oobsize = info->oobsize;
	}

	host->oobsize = mtd->oobsize;
	nand_dev->oobsize = host->oobsize;

	host->dma_oob = host->dma_buffer + host->pagesize;
	host->bbm = (u_char *)(host->buffer + host->pagesize
			       + XMEDIA_FMC_BAD_BLOCK_POS);
	if(info->ooblayout_ops == NULL) {
		DB_MSG("Error: info->ooblayout_ops or is NULL!\n");
		return;
	}
	info->ooblayout_ops->free(mtd, 0, &xmedia_fmc_oobregion);

	mtd_set_ooblayout(mtd, info->ooblayout_ops);

	/* EB bits locate in the bottom two of CTRL(30) */
	host->epm = (u_short *)(host->buffer + host->pagesize
				+ xmedia_fmc_oobregion.offset + 28);

#ifdef CONFIG_XMEDIA_NAND_FS_MAY_NO_YAFFS2
	if (best->ecctype == NAND_ECC_16BIT) {
		if (host->pagesize == _2K) {
			/* EB bits locate in the bottom two of CTRL(4) */
			host->epm = (u_short *)(host->buffer + host->pagesize
						+ xmedia_fmc_oobregion.offset + 4);
		} else if (host->pagesize == _4K) {
			/* EB bit locate in the bottom two of CTRL(14) */
			host->epm = (u_short *)(host->buffer + host->pagesize
						+ xmedia_fmc_oobregion.offset + 12);
		}
	}
#endif
}

static unsigned int xmedia_fmc100_get_ecc_reg(struct xmedia_fmc_host *host,
		const struct nand_config_info *info, struct nand_dev_t *nand_dev)
{
	if (info == NULL || host == NULL || nand_dev == NULL) {
		DB_MSG("get ecc reg err!!!\n");
		return 0;
	}
	host->ecctype = info->ecctype;
	nand_dev->ecctype = host->ecctype;

	return FMC_CFG_ECC_TYPE(match_ecc_type_to_reg(info->ecctype));
}

static unsigned int xmedia_fmc100_get_page_reg(struct xmedia_fmc_host *host,
		const struct nand_config_info *info)
{
	if (info == NULL || host == NULL) {
		DB_MSG("get page reg err!!!\n");
		return 0;
	}
	host->pagesize = match_page_type_to_size(info->pagetype);

	return FMC_CFG_PAGE_SIZE(match_page_type_to_reg(info->pagetype));
}

static unsigned int xmedia_fmc100_get_block_reg(struct xmedia_fmc_host *host,
		const struct nand_config_info *info)
{
	unsigned int block_reg = 0;
	unsigned int page_per_block = 0;
	struct mtd_info *mtd = NULL;

	if (info == NULL || host == NULL) {
		DB_MSG("get block reg err!!!\n");
		return 0;
	}

	mtd = host->mtd;
	if(mtd == NULL) {
		DB_MSG("err:mtd is NULL!!!\n");
		return 0;
	}
	host->block_page_mask = ((mtd->erasesize / mtd->writesize) - 1);
	page_per_block = mtd->erasesize / match_page_type_to_size(info->pagetype);
	switch (page_per_block) {
	case 64:
		block_reg = BLOCK_SIZE_64_PAGE;
		break;
	case 128:
		block_reg = BLOCK_SIZE_128_PAGE;
		break;
	case 256:
		block_reg = BLOCK_SIZE_256_PAGE;
		break;
	case 512:
		block_reg = BLOCK_SIZE_512_PAGE;
		break;
	default:
		DB_MSG("Can't support block %#x and page %#x size\n",
		       mtd->erasesize, mtd->writesize);
	}

	return FMC_CFG_BLOCK_SIZE(block_reg);
}

static void xmedia_fmc100_set_fmc_cfg_reg(struct xmedia_fmc_host *host,
	const struct nand_config_info *type_info, struct nand_dev_t *nand_dev)
{
	unsigned int page_reg, ecc_reg, block_reg, reg_fmc_cfg;

	ecc_reg = xmedia_fmc100_get_ecc_reg(host, type_info, nand_dev);
	page_reg = xmedia_fmc100_get_page_reg(host, type_info);
	block_reg = xmedia_fmc100_get_block_reg(host, type_info);

	reg_fmc_cfg = fmc_readl(host, FMC_CFG);
	reg_fmc_cfg &= ~(PAGE_SIZE_MASK | ECC_TYPE_MASK | BLOCK_SIZE_MASK);
	reg_fmc_cfg |= ecc_reg | page_reg | block_reg;
	fmc_writel(host, FMC_CFG, reg_fmc_cfg);

	/* Save value of FMC_CFG and FMC_CFG_ECC0 to turn on/off ECC */
	host->fmc_cfg = reg_fmc_cfg;
	host->fmc_cfg_ecc0 = (host->fmc_cfg & ~ECC_TYPE_MASK) | ECC_TYPE_0BIT;
	FMC_PR(BT_DBG, "\t|-Save FMC_CFG[%#x]: %#x and FMC_CFG_ECC0: %#x\n",
	       FMC_CFG, host->fmc_cfg, host->fmc_cfg_ecc0);
}

static int xmedia_fmc100_set_config_info(struct mtd_info *mtd,
				    struct nand_chip *chip, struct nand_dev_t *nand_dev)
{
	struct xmedia_fmc_host *host = chip->priv;
	struct nand_config_info *type_info = NULL;

	FMC_PR(BT_DBG, "\t*-Start config Block Page OOB and Ecc\n");

	type_info = xmedia_fmc100_get_config_type_info(mtd, nand_dev);
	WARN_ON(!type_info);
	if (type_info == NULL) {
		DB_MSG("set config info err!!!\n");
		return 0;
	}

	FMC_PR(BT_DBG, "\t|-%s Config, PageSize %s EccType %s OOBSize %d\n",
	       nand_dev->start_type, nand_page_name(type_info->pagetype),
	       nand_ecc_name(type_info->ecctype), type_info->oobsize);

	/* Set the page_size, ecc_type, block_size of FMC_CFG[0x0] register */
	xmedia_fmc100_set_fmc_cfg_reg(host, type_info, nand_dev);

	xmedia_fmc100_set_oob_info(mtd, type_info, nand_dev);

	FMC_PR(BT_DBG, "\t*-End config Block Page Oob and Ecc\n");

	return 0;
}

void xmedia_fmc100_spi_nand_init(struct nand_chip *chip)
{
	struct xmedia_fmc_host *host = NULL;

	if((chip == NULL) || (chip->priv == NULL)) {
		DB_MSG("Error: chip or chip->priv is NULL!\n");
		return;
	}
	host = chip->priv;
	FMC_PR(BT_DBG, "\t|*-Start xmedia_fmc100 SPI Nand init\n");

	/* Switch SPI type to SPI nand */
	xmedia_fmc100_switch_to_spi_nand(host);

	/* hold on STR mode */
	xmedia_fmc100_set_str_mode(host);

	/* Xmedia_fmc host init */
	xmedia_fmc100_host_init(host);
	host->chip = chip;

	/* Xmedia_fmc nand_chip struct init */
	xmedia_fmc100_chip_init(chip);

	xmedia_fmc_spi_nand_ids_register();
	nfc_param_adjust = xmedia_fmc100_set_config_info;

	FMC_PR(BT_DBG, "\t|*-End xmedia_fmc100 SPI Nand init\n");
}
#ifdef CONFIG_PM
int xmedia_fmc100_suspend(struct platform_device *pltdev, pm_message_t state)
{
	int ret;
	struct xmedia_fmc_host *host = platform_get_drvdata(pltdev);
	struct xmedia_fmc_spi *spi = host->spi;

	mutex_lock(host->lock);
	xmedia_fmc100_switch_to_spi_nand(host);

	ret = spi->driver->wait_ready(spi);
	if (ret) {
		DB_MSG("Error: wait ready failed!");
		clk_disable_unprepare(host->clk);
		mutex_unlock(host->lock);
		return 0;
	}

	clk_disable_unprepare(host->clk);
	mutex_unlock(host->lock);

	return 0;
}

int xmedia_fmc100_resume(struct platform_device *pltdev)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pltdev);

	mutex_lock(host->lock);
	xmedia_fmc100_switch_to_spi_nand(host);
	clk_prepare_enable(host->clk);

	host->send_cmd_reset(host);

	xmedia_fmc100_spi_nand_config(host);

	mutex_unlock(host->lock);
	return 0;
}
#endif
