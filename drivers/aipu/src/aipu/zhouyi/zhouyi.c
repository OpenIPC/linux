// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file zhouyi.c
 * Implementations of the zhouyi AIPU hardware control and interrupt handle operations
 */

#include <linux/platform_device.h>
#include "zhouyi.h"

int zhouyi_read_status_reg(struct io_region *io)
{
	return aipu_read32(io, ZHOUYI_STAT_REG_OFFSET);
}

void zhouyi_clear_qempty_interrupt(struct io_region *io)
{
	aipu_write32(io, ZHOUYI_STAT_REG_OFFSET, ZHOUYI_IRQ_QEMPTY);
}

void zhouyi_clear_done_interrupt(struct io_region *io)
{
	aipu_write32(io, ZHOUYI_STAT_REG_OFFSET, ZHOUYI_IRQ_DONE);
}

void zhouyi_clear_excep_interrupt(struct io_region *io)
{
	aipu_write32(io, ZHOUYI_STAT_REG_OFFSET, ZHOUYI_IRQ_EXCEP);
}

void zhouyi_io_rw(struct io_region *io, struct aipu_io_req *io_req)
{
	if (unlikely((!io) || (!io_req))) {
		pr_err("invalid input args io/io_req to be NULL!");
		return;
	}

	/* TBD: offset r/w permission should be checked */

	if (io_req->rw == AIPU_IO_READ)
		io_req->value = aipu_read32(io, io_req->offset);
	else if (io_req->rw == AIPU_IO_WRITE)
		aipu_write32(io, io_req->offset, io_req->value);
}

int zhouyi_detect_aipu_version(struct platform_device *p_dev, int *version, int *config)
{
	struct resource *res = NULL;
	struct io_region io;

	if ((!p_dev) || (!version) || (!config))
		return -EINVAL;

	res = platform_get_resource(p_dev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	if (init_aipu_ioregion(&p_dev->dev, &io, res->start, res->end - res->start + 1))
		return -EINVAL;

	*version = zhouyi_get_hw_version_number(&io);
	*config = zhouyi_get_hw_config_number(&io);
	deinit_aipu_ioregion(&io);
	return 0;
}

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
int zhouyi_print_reg_info(struct io_region *io, char *buf, const char *name, int offset)
{
	if (unlikely((!io) || (!buf) || (!name)))
		return -EINVAL;

	return snprintf(buf, 1024, "0x%-*x%-*s0x%08x\n", 6, offset, 22, name,
	    aipu_read32(io, offset));
}
#endif

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
int zhouyi_sysfs_show(struct io_region *io, char *buf)
{
	int ret = 0;
	char tmp[1024];

	if (unlikely((!io) || (!buf)))
		return -EINVAL;

	ret += zhouyi_print_reg_info(io, tmp, "Ctrl Reg", ZHOUYI_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "Status Reg", ZHOUYI_STAT_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "Start PC Reg", ZHOUYI_START_PC_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "Intr PC Reg", ZHOUYI_INTR_PC_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "IPI Ctrl Reg", ZHOUYI_IPI_CTRL_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "Data Addr 0 Reg", ZHOUYI_DATA_ADDR_0_REG_OFFSET);
	strcat(buf, tmp);
	ret += zhouyi_print_reg_info(io, tmp, "Data Addr 1 Reg", ZHOUYI_DATA_ADDR_1_REG_OFFSET);
	strcat(buf, tmp);
	return ret;
}
#endif

int zhouyi_get_hw_version_number(struct io_region *io)
{
	int isa_version = 0;

	if (!io)
		return 0;

	isa_version = aipu_read32(io, ZHOUYI_ISA_VERSION_REG_OFFSET);
	if (isa_version == 0)
		return AIPU_ISA_VERSION_ZHOUYI_V1;
	else if (isa_version == 1)
		return AIPU_ISA_VERSION_ZHOUYI_V2;
	else
		return 0;
}

int zhouyi_get_hw_config_number(struct io_region *io)
{
	int high = 0;
	int low = 0;
	int isa_version = 0;
	int aiff_feature = 0;
	int tpc_feature = 0;

	if (!io)
		return 0;

	isa_version = aipu_read32(io, ZHOUYI_ISA_VERSION_REG_OFFSET);
	aiff_feature = aipu_read32(io, ZHOUYI_HWA_FEATURE_REG_OFFSET);
	tpc_feature = aipu_read32(io, ZHOUYI_TPC_FEATURE_REG_OFFSET);

	if (isa_version == 0)
		high = (aiff_feature & 0xF) + 6;
	else if (isa_version == 1)
		high = (aiff_feature & 0xF) + 8;

	low = (tpc_feature) & 0x1F;

	return high * 100 + low;
}
