/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_DRV_H_
#define _AX_GZIPD_DRV_H_

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include "ax_gzipd_api.h"
#include "ax_gzipd_log.h"

#define BYPASS_EN 0
#define MIN_TILE_SIZE (8 * 1024)

#define CMDQ_DEPTH           (16)
#define GZIP_COMPLETE_INTR   (0x1 << 0)
#define ALMOST_EMPTY_INTR    (0x1 << 1)

#define GZIPD_CLK_DISABLE	 (1)
#define GZIPD_CLK_ENABLE	 (0)
#define GZIPD_CLK_RATE		 (500000000)

typedef struct {
	uint64_t gzipd_base;
	uint64_t gzipd_flash_base;
	struct clk *gzipdClk;
	struct clk *gzipdCoreClk;
	struct reset_control *gzipdRst;
	struct reset_control *gzipdCoreRst;
	struct platform_device *pdev;
	struct resource *res;
	uint32_t irq;
} ax_gzipd_dev_t;

#ifdef CONFIG_PM_SLEEP
typedef struct {
	int32_t reg_ctl_cfg;
	int32_t reg_wdma_cfg0;
	int32_t reg_wdma_cfg1;
	int32_t reg_rdma_cfg0;
	int32_t reg_rdma_cfg1;
	int32_t reg_rdma_cfg2;
} ax_gzipd_reg_ctx_t;
#endif

void gzipd_dev_trigger_once(void);
void gzipd_dev_trigger_restore(void);
int gzipd_dev_tile_config(uint64_t outAddr, uint64_t outlen, uint32_t blknum, uint32_t tilenum);
int gzipd_dev_tile_cmdq(uint64_t startAddr, uint64_t tilelen);

#endif