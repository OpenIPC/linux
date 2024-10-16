// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file z2.c
 * Implementation of the zhouyi v1 AIPU hardware cntrol interfaces
 */

#include <linux/irqreturn.h>
#include <linux/bitops.h>
#include "aipu_priv.h"
#include "z2.h"
#include "aipu_io.h"
#if ((defined BUILD_PLATFORM_JUNO) && (BUILD_PLATFORM_JUNO == 1))
#include "junor2.h"
#elif ((defined BUILD_PLATFORM_6CG) && (BUILD_PLATFORM_6CG == 1))
#include "x6cg.h"
#endif
#include "config.h"

static int zhouyi_v2_get_hw_version_number(struct aipu_core *core)
{
	if (likely(core))
		return zhouyi_get_hw_version_number(&core->reg[0]);
	return 0;
}

static int zhouyi_v2_get_hw_config_number(struct aipu_core *core)
{
	if (likely(core))
		return zhouyi_get_hw_config_number(&core->reg[0]);
	return 0;
}

static void zhouyi_v2_enable_interrupt(struct aipu_core *core)
{
	if (likely(core))
		aipu_write32(&core->reg[0], ZHOUYI_CTRL_REG_OFFSET,
			ZHOUYIV2_IRQ_ENABLE_FLAG);
}

static void zhouyi_v2_disable_interrupt(struct aipu_core *core)
{
	if (likely(core))
		aipu_write32(&core->reg[0], ZHOUYI_CTRL_REG_OFFSET,
			ZHOUYIV2_IRQ_DISABLE_FLAG);
}

static void zhouyi_v2_clear_qempty_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_qempty_interrupt(&core->reg[0]);
}

static void zhouyi_v2_clear_done_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_done_interrupt(&core->reg[0]);
}

static void zhouyi_v2_clear_excep_interrupt(struct aipu_core *core)
{
	if (likely(core))
		zhouyi_clear_excep_interrupt(&core->reg[0]);
}

static void zhouyi_v2_clear_fault_interrupt(struct aipu_core *core)
{
	if (likely(core))
		aipu_write32(&core->reg[0], ZHOUYI_STAT_REG_OFFSET, ZHOUYI_IRQ_FAULT);
}

static void zhouyi_v2_trigger(struct aipu_core *core)
{
	int start_pc = 0;

	if (likely(core)) {
		start_pc = aipu_read32(&core->reg[0], ZHOUYI_START_PC_REG_OFFSET) & 0xFFFFFFF0;
		aipu_write32(&core->reg[0], ZHOUYI_START_PC_REG_OFFSET, start_pc | 0xD);
	}
}

static int zhouyi_v2_reserve(struct aipu_core *core, struct aipu_job_desc *udesc, int do_trigger)
{
	u32 start_pc = 0;
	u32 ase0_base_high = 0;

	if (unlikely((!core) || (!udesc)))
		return -EINVAL;

	start_pc = (u32)udesc->start_pc_addr;
	ase0_base_high = udesc->start_pc_addr >> 32;

	/* Load data addr 0 register */
	aipu_write32(&core->reg[0], ZHOUYI_DATA_ADDR_0_REG_OFFSET, (u32)udesc->data_0_addr);

	/* Load data addr 1 register */
	aipu_write32(&core->reg[0], ZHOUYI_DATA_ADDR_1_REG_OFFSET, (u32)udesc->data_1_addr);

	/* Load interrupt PC */
	aipu_write32(&core->reg[0], ZHOUYI_INTR_PC_REG_OFFSET, (u32)udesc->intr_handler_addr);

	/* Load ASE registers */
	/* ASE 0 */
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT0_CTRL_REG_OFFSET, ZHOUYI_V2_ASE_RW_ENABLE);
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT0_HIGH_BASE_REG_OFFSET, ase0_base_high);
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT0_LOW_BASE_REG_OFFSET, 0);
	dev_dbg(core->dev, "ASE 0 Ctrl 0x%x, ASE 0 PA 0x%llx",
		aipu_read32(&core->reg[0], AIPU_ADDR_EXT0_CTRL_REG_OFFSET),
		((u64)aipu_read32(&core->reg[0], AIPU_ADDR_EXT0_HIGH_BASE_REG_OFFSET) << 32) +
		aipu_read32(&core->reg[0], AIPU_ADDR_EXT0_LOW_BASE_REG_OFFSET));
	/* ASE 1 */
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT1_CTRL_REG_OFFSET, ZHOUYI_V2_ASE_READ_ENABLE);
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT1_HIGH_BASE_REG_OFFSET, ase0_base_high);
	aipu_write32(&core->reg[0], AIPU_ADDR_EXT1_LOW_BASE_REG_OFFSET, 0);
	dev_dbg(core->dev, "ASE 1 Ctrl 0x%x, ASE 1 PA 0x%llx",
		aipu_read32(&core->reg[0], AIPU_ADDR_EXT1_CTRL_REG_OFFSET),
		((u64)aipu_read32(&core->reg[0], AIPU_ADDR_EXT1_HIGH_BASE_REG_OFFSET) << 32) +
		aipu_read32(&core->reg[0], AIPU_ADDR_EXT1_LOW_BASE_REG_OFFSET));
	/* ASE 2: to be removed */
	if (!udesc->enable_asid) {
		aipu_write32(&core->reg[0], AIPU_ADDR_EXT2_CTRL_REG_OFFSET, ZHOUYI_V2_ASE_RW_ENABLE);
		aipu_write32(&core->reg[0], AIPU_ADDR_EXT2_HIGH_BASE_REG_OFFSET, ase0_base_high);
		aipu_write32(&core->reg[0], AIPU_ADDR_EXT2_LOW_BASE_REG_OFFSET, 0);
		dev_dbg(core->dev, "Default: ASE 2 Ctrl 0x%x, ASE 0 PA 0x%llx",
			aipu_read32(&core->reg[0], AIPU_ADDR_EXT2_CTRL_REG_OFFSET),
			((u64)aipu_read32(&core->reg[0], AIPU_ADDR_EXT2_HIGH_BASE_REG_OFFSET) << 32) +
			aipu_read32(&core->reg[0], AIPU_ADDR_EXT2_LOW_BASE_REG_OFFSET));
	}

	/* Load start PC register */
	if (do_trigger)
		start_pc |= 0xD;
	aipu_write32(&core->reg[0], ZHOUYI_START_PC_REG_OFFSET, start_pc);

	dev_dbg(core->dev, "[Job %d] trigger done: start pc = 0x%x, dreg0 = 0x%x, dreg1 = 0x%x\n",
		udesc->job_id, start_pc, (u32)udesc->data_0_addr, (u32)udesc->data_1_addr);

	return 0;
}

static bool zhouyi_v2_is_idle(struct aipu_core *core)
{
	unsigned long val = 0;

	if (unlikely(!core)) {
		pr_err("invalid input args core to be NULL!");
		return 0;
	}

	val = aipu_read32(&core->reg[0], ZHOUYI_STAT_REG_OFFSET);
	return test_bit(16, &val) && test_bit(17, &val) && test_bit(18, &val);
}

static int zhouyi_v2_read_status_reg(struct aipu_core *core)
{
	if (unlikely(!core))
		return 0;
	return zhouyi_read_status_reg(&core->reg[0]);
}

static void zhouyi_v2_print_hw_id_info(struct aipu_core *core)
{
	if (unlikely(!core)) {
		pr_err("invalid input args core to be NULL!");
		return;
	}

	dev_info(core->dev, "AIPU Initial Status: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_STAT_REG_OFFSET));

	dev_info(core->dev, "########## AIPU CORE %d: ZHOUYI V2 ##########", core->id);
	dev_info(core->dev, "# ISA Version Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_ISA_VERSION_REG_OFFSET));
	dev_info(core->dev, "# TPC Feature Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_TPC_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# SPU Feature Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_SPU_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# HWA Feature Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_HWA_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Revision ID Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_REVISION_ID_REG_OFFSET));
	dev_info(core->dev, "# Memory Hierarchy Feature Register: 0x%x", aipu_read32(&core->reg[0], ZHOUYI_MEM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Instruction RAM Feature Register:  0x%x", aipu_read32(&core->reg[0], ZHOUYI_INST_RAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# TEC Local SRAM Feature Register:   0x%x", aipu_read32(&core->reg[0], ZHOUYI_LOCAL_SRAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Global SRAM Feature Register:      0x%x", aipu_read32(&core->reg[0], ZHOUYI_GLOBAL_SRAM_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Instruction Cache Feature Register:0x%x", aipu_read32(&core->reg[0], ZHOUYI_INST_CACHE_FEATURE_REG_OFFSET));
	dev_info(core->dev, "# Data Cache Feature Register:       0x%x", aipu_read32(&core->reg[0], ZHOUYI_DATA_CACHE_FEATURE_REG_OFFSET));
	dev_info(core->dev, "############################################");
}

static int zhouyi_v2_io_rw(struct aipu_core *core, struct aipu_io_req *io_req)
{
	if (unlikely(!io_req))
		return -EINVAL;

	if (!core || (io_req->offset > ZHOUYI_V2_MAX_REG_OFFSET))
		return -EINVAL;

	zhouyi_io_rw(&core->reg[0], io_req);
	return 0;
}

static int zhouyi_v2_upper_half(void *data)
{
	int ret = 0;
	struct aipu_core *core = (struct aipu_core *)data;

	if (core->interrupts) {
#if ((defined BUILD_PLATFORM_JUNO) && (BUILD_PLATFORM_JUNO == 1))
		if (!JUNOR2_IS_CORE_IRQ(core->interrupts, core->id))
#elif ((defined BUILD_PLATFORM_6CG) && (BUILD_PLATFORM_6CG == 1))
		if (!X6CG_IS_CORE_IRQ(core->interrupts, core->id))
#else
		if (0)
#endif
			return IRQ_NONE;
	}

	zhouyi_v2_disable_interrupt(core);
	ret = zhouyi_v2_read_status_reg(core);
	if (ret & ZHOUYI_IRQ_QEMPTY)
		zhouyi_v2_clear_qempty_interrupt(core);

	if (ret & ZHOUYI_IRQ_DONE) {
		zhouyi_v2_clear_done_interrupt(core);
		aipu_job_manager_irq_upper_half(core, 0);
		aipu_irq_schedulework(core->irq_obj);
	}

	if (ret & ZHOUYI_IRQ_EXCEP) {
		zhouyi_v2_clear_excep_interrupt(core);
		aipu_job_manager_irq_upper_half(core, 1);
		aipu_irq_schedulework(core->irq_obj);
	}

	if (ret & ZHOUYI_IRQ_FAULT)
		zhouyi_v2_clear_fault_interrupt(core);
	zhouyi_v2_enable_interrupt(core);

	/* success */
	return IRQ_HANDLED;
}

static void zhouyi_v2_bottom_half(void *data)
{
	aipu_job_manager_irq_bottom_half(data);
}

static void zhouyi_v2_start_bw_profiling(struct aipu_core *core)
{
}

static void zhouyi_v2_stop_bw_profiling(struct aipu_core *core)
{
}

static void zhouyi_v2_read_profiling_reg(struct aipu_core *core, struct aipu_ext_profiling_data *pdata)
{
}

static bool zhouyi_v2_has_clk_ctrl(struct aipu_core *core)
{
	return 0;
}

static void zhouyi_v2_enable_clk_gating(struct aipu_core *core)
{
}

static void zhouyi_v2_disable_clk_gating(struct aipu_core *core)
{
}

static bool zhouyi_v2_is_clk_gated(struct aipu_core *core)
{
	return 0;
}

static bool zhouyi_v2_has_power_ctrl(struct aipu_core *core)
{
	return false;
}

static void zhouyi_v2_power_on(struct aipu_core *core)
{
}

static void zhouyi_v2_power_off(struct aipu_core *core)
{
}

static bool zhouyi_v2_is_power_on(struct aipu_core *core)
{
	return 1;
}

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
static int zhouyi_v2_sysfs_show(struct aipu_core *core, char *buf)
{
	int ret = 0;
	char tmp[1024];

	if (unlikely((!core) || (!buf)))
		return -EINVAL;

	ret += zhouyi_sysfs_show(&core->reg[0], buf);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "Data Addr 2 Reg",
	    AIPU_DATA_ADDR_2_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "Data Addr 3 Reg",
	    AIPU_DATA_ADDR_3_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE0 Ctrl Reg",
	    AIPU_ADDR_EXT0_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE0 High Base Reg",
	    AIPU_ADDR_EXT0_HIGH_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE0 Low Base Reg",
	    AIPU_ADDR_EXT0_LOW_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE1 Ctrl Reg",
	    AIPU_ADDR_EXT1_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE1 High Base Reg",
	    AIPU_ADDR_EXT1_HIGH_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE1 Low Base Reg",
	    AIPU_ADDR_EXT1_LOW_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE2 Ctrl Reg",
	    AIPU_ADDR_EXT2_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE2 High Base Reg",
	    AIPU_ADDR_EXT2_HIGH_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE2 Low Base Reg",
	    AIPU_ADDR_EXT2_LOW_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE3 Ctrl Reg",
	    AIPU_ADDR_EXT3_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE3 High Base Reg",
	    AIPU_ADDR_EXT3_HIGH_BASE_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(&core->reg[0], tmp, "ASE3 Low Base Reg",
	    AIPU_ADDR_EXT3_LOW_BASE_REG_OFFSET);
	strcat(buf, tmp);
	return ret;
}
#endif

struct aipu_core_operations zhouyi_v2_ops = {
	.get_version = zhouyi_v2_get_hw_version_number,
	.get_config = zhouyi_v2_get_hw_config_number,
	.enable_interrupt = zhouyi_v2_enable_interrupt,
	.disable_interrupt = zhouyi_v2_disable_interrupt,
	.trigger = zhouyi_v2_trigger,
	.reserve = zhouyi_v2_reserve,
	.is_idle = zhouyi_v2_is_idle,
	.read_status_reg = zhouyi_v2_read_status_reg,
	.print_hw_id_info = zhouyi_v2_print_hw_id_info,
	.io_rw = zhouyi_v2_io_rw,
	.upper_half = zhouyi_v2_upper_half,
	.bottom_half = zhouyi_v2_bottom_half,
	.start_bw_profiling = zhouyi_v2_start_bw_profiling,
	.stop_bw_profiling = zhouyi_v2_stop_bw_profiling,
	.read_profiling_reg = zhouyi_v2_read_profiling_reg,
	.has_clk_ctrl = zhouyi_v2_has_clk_ctrl,
	.enable_clk_gating = zhouyi_v2_enable_clk_gating,
	.disable_clk_gating = zhouyi_v2_disable_clk_gating,
	.is_clk_gated = zhouyi_v2_is_clk_gated,
	.has_power_ctrl = zhouyi_v2_has_power_ctrl,
	.power_on = zhouyi_v2_power_on,
	.power_off = zhouyi_v2_power_off,
	.is_power_on = zhouyi_v2_is_power_on,
#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
	.sysfs_show = zhouyi_v2_sysfs_show,
#endif
};
