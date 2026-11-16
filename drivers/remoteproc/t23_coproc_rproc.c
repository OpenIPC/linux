// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Low-Power RISC-V Coprocessor Remoteproc Operations
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/remoteproc.h>
#include <linux/elf.h>
#include <linux/firmware.h>
#include "t23_coproc.h"

static int t23_coproc_rproc_start(struct rproc *rproc)
{
	struct t23_coproc_priv *priv = rproc->priv;
	struct t23_mbox_header hdr;
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(priv->dev, "failed to enable CLK_RISCV: %d\n", ret);
		return ret;
	}

	/* Initialize TCSM Mailbox Header */
	memset(&hdr, 0, sizeof(hdr));
	hdr.magic = T23_MBOX_MAGIC;
	hdr.version = T23_MBOX_VERSION;
	hdr.tx_buf_offset = T23_TCSM_TX_OFFSET;
	hdr.tx_buf_size = sizeof(struct t23_ipc_packet) * T23_RING_NUM_SLOTS;
	hdr.rx_buf_offset = T23_TCSM_RX_OFFSET;
	hdr.rx_buf_size = sizeof(struct t23_ipc_packet) * T23_RING_NUM_SLOTS;

	memcpy_toio(priv->tcsm_base + T23_TCSM_MBOX_OFFSET, &hdr, sizeof(hdr));
	wmb();

	/* Deassert CPM global reset (bit 26 in CPM_SRBC) */
	if (priv->cpm_map)
		regmap_update_bits(priv->cpm_map, CPM_SRBC_OFFSET,
				   SRBC_RISCV_SR, 0);

	/* Deassert coprocessor local soft reset (bit 0 in REG_COPROC_CTRL) */
	writel(readl(priv->aux_base + REG_COPROC_CTRL) & ~COPROC_CTRL_SW_RESET,
	       priv->aux_base + REG_COPROC_CTRL);

	/* Enable host doorbell message interrupt (bit 3 in REG_COPROC_CTRL) */
	writel(readl(priv->aux_base + REG_COPROC_CTRL) | COPROC_CTRL_MSG_IRQ_EN,
	       priv->aux_base + REG_COPROC_CTRL);

	enable_irq(priv->irq);
	priv->running = true;

	/*
	 * Reset is released after the ELF load. The dump has no coprocessor
	 * firmware and no handshake, so this does not mean the core answered.
	 */
	dev_dbg(priv->dev, "coprocessor reset released at 0x%llx\n",
		(unsigned long long)rproc->bootaddr);
	return 0;
}

static int t23_coproc_rproc_stop(struct rproc *rproc)
{
	struct t23_coproc_priv *priv = rproc->priv;

	priv->running = false;
	disable_irq(priv->irq);

	/* Mask host doorbell interrupt and assert local soft reset */
	writel((readl(priv->aux_base + REG_COPROC_CTRL) & ~COPROC_CTRL_MSG_IRQ_EN) |
	       COPROC_CTRL_SW_RESET, priv->aux_base + REG_COPROC_CTRL);

	/* Assert CPM global reset */
	if (priv->cpm_map)
		regmap_update_bits(priv->cpm_map, CPM_SRBC_OFFSET,
				   SRBC_RISCV_SR, SRBC_RISCV_SR);

	clk_disable_unprepare(priv->clk);
	dev_dbg(priv->dev, "Coprocessor stopped\n");
	return 0;
}

static void t23_coproc_rproc_kick(struct rproc *rproc, int vqid)
{
	struct t23_coproc_priv *priv = rproc->priv;

	/* Pulse coprocessor doorbell to generate interrupt to RISC-V core */
	writel(COPROC_DOORBELL_TRIGGER, priv->aux_base + REG_COPROC_DOORBELL);
}

static void *t23_coproc_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len,
				       bool *is_iomem)
{
	struct t23_coproc_priv *priv = rproc->priv;

	if (!len)
		return NULL;
	if (da >= TCSM_IOBASE)
		da -= TCSM_IOBASE;
	if (da < TCSM_IOSIZE && len <= TCSM_IOSIZE - da) {
		if (is_iomem)
			*is_iomem = true;
		return (void __force *)(priv->tcsm_base + da);
	}

	return NULL;
}

const struct rproc_ops t23_coproc_rproc_ops = {
	.start		= t23_coproc_rproc_start,
	.stop		= t23_coproc_rproc_stop,
	.kick		= t23_coproc_rproc_kick,
	.da_to_va	= t23_coproc_rproc_da_to_va,
};
