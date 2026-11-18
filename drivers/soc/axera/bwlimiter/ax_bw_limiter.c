/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/types.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include "../../../../fs/proc/internal.h"
#include <linux/soc/axera/ax_bw_limiter.h>

#define DRIVER_NAME         "ax_bw_limiter"
#define NO_EXIST            -1
#define BIT_WIDTH_32        32
#define BIT_WIDTH_64        64
#define BIT_WIDTH_128       128
#define BIT_WIDTH_256       256

#define PROC_NODE_ROOT_NAME           "ax_proc/bw_limit"
#define PROC_BW_LIMITER_VAL_RD        "limiter_val_rd"
#define PROC_BW_LIMITER_VAL_WR        "limiter_val_wr"
#define PROC_BW_LIMITER_VAL_SUM_RDWR  "limiter_val_sum_rdwr"
#define PROC_BW_LIMITER_REGISTER      "register"
#define CHECK_SYS_ENABLE

// #define BW_DEBUG_LOG_EN
#ifdef BW_DEBUG_LOG_EN
#define BW_LOG_DBG(fmt, arg...) printk("[%s : %d] " fmt, __func__, __LINE__, ##arg)
#else
#define BW_LOG_DBG(fmt, arg...)
#endif
#define BW_LOG_NOTICE(fmt, arg...) printk(KERN_NOTICE "notice: [%s : %d] " fmt, __func__, __LINE__, ##arg)
#define BW_LOG_ERR(fmt, arg...) printk(KERN_ERR "err: [%s : %d] " fmt, __func__, __LINE__, ##arg)

int ax_bw_limiter_register_with_clk(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk);
int ax_bw_limiter_register_with_val(SUB_SYS_BW_LIMITERS sub_sys_bw, u32 work_clk);
int ax_bw_limiter_unregister(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk);
int ax_bw_limiter_refresh_limiter(SUB_SYS_BW_LIMITERS sub_sys_bw);

static struct proc_dir_entry *bw_limiter_root;
static DEFINE_MUTEX(ax_bw_limiter_mutex);

typedef enum {
	BW_PORT_RD,
	BW_PORT_WR,
	BW_PORT_END
} BW_OPS_E;

typedef struct {
	u32 limiter_enable:    1;
	u32 auto_limiter_wr:   1;
	u32 auto_limiter_rd:   1;
	u32 interl_value:      5;
} bw_limiter_ctrl_t;

typedef struct {
	u32 limiter_value_rd: 16;
	u32 limiter_value_wr: 16;
} bw_limiter_value_t;

typedef struct {
	u64 limiter_clk_en_shift    : 5;
	u64 limiter_clk_eb_shift    : 5;
	u64 limiter_rst_shift       : 5;
	u64 limiter_update_shift    : 5;
	u64 limiter_enable_shift    : 5;
	u64 auto_limiter_wr_shift   : 5;
	u64 auto_limiter_rd_shift   : 5;
	u64 interl_value_shift      : 5;
	u64 limiter_value_h_shift   : 5;
	u64 limiter_value_h_is_rd   : 1;
	u64 limiter_clk_eb_shared   : 1;
	u64 limiter_sys_id          : 5;
} bw_config_info_t;

/*  define base type to control bandwidth   */
typedef struct {
	u32 limiter_base_paddr;
	s32 limiter_clk_en_set_offset;
	s32 limiter_clk_en_clr_offset;
	s32 limiter_clk_eb_set_offset;
	s32 limiter_clk_eb_clr_offset;
	s32 limiter_rst_set_offset;
	s32 limiter_rst_clr_offset;
	s32 limiter_update_paddr_offset;
	s32 limiter_ctrl_paddr_offset;
	s32 limiter_value_paddr_offset;
	u32 bus_bitwidth;
	bw_config_info_t config_info;
} bw_limit_t;

typedef struct {
	u32 index;
	char proc_name[32];
	bw_limit_t *bw_limiter;
	struct proc_dir_entry *root_proc_file;
	struct proc_dir_entry *proc_rfile;
	struct proc_dir_entry *proc_wfile;
	u32 bwLimiter_value_rd;
	u32 bwLimiter_value_wr;
	u32 clk;
	bool sub_sys_enabled;
	struct clk *pclk;
	struct notifier_block nb;
} proc_node_info_t;

bw_limit_t bl_vpu_vdec = {
	.limiter_base_paddr             = 0x4030000,
	.limiter_clk_en_set_offset      = 0x10c,
	.limiter_clk_en_clr_offset      = 0x110,
	.limiter_clk_eb_set_offset      = 0x114,
	.limiter_clk_eb_clr_offset      = 0x118,
	.limiter_rst_set_offset         = 0x11c,
	.limiter_rst_clr_offset         = 0x120,
	.limiter_update_paddr_offset    = 0x88,
	.limiter_ctrl_paddr_offset      = 0x88,
	.limiter_value_paddr_offset     = 0x8c,
	.bus_bitwidth                   = BIT_WIDTH_64,
	.config_info = {
		.limiter_clk_en_shift    = 0,
		.limiter_clk_eb_shift    = 2,
		.limiter_rst_shift       = 2,
		.limiter_update_shift    = 9,
		.limiter_enable_shift    = 8,
		.auto_limiter_wr_shift   = 7,
		.auto_limiter_rd_shift   = 6,
		.interl_value_shift      = 0,
		.limiter_value_h_shift   = 14,
		.limiter_value_h_is_rd   = 1,
		.limiter_clk_eb_shared   = 0,
		.limiter_sys_id          = BL_VPU_SYS,
	}
};

static u32 bl_sys_base_addr[BL_SYS_ID_MAX] = {
	[BL_VPU_SYS]   = 0x4030000,
	[BL_MM_SYS]    = 0x4430000,
	[BL_ISP_SYS]   = 0x2500000,
	[BL_COMM_SYS]  = 0x2340000,
	[BL_FLASH_SYS] = 0x10030000,
};

bw_limit_t bl_vpu_venc          = {0x04030000, 0x10c, 0x110, 0x114, 0x118, 0x11c, 0x120, 0x078, 0x078, 0x07c, BIT_WIDTH_128,
                   .config_info = {0, 3, 3,  9,  8,  7,  6,  0, 14, 1, 0, BL_VPU_SYS}};
bw_limit_t bl_vpu_jenc          = {0x04030000, 0x10c, 0x110, 0x114, 0x118, 0x11c, 0x120, 0x080, 0x080, 0x084, BIT_WIDTH_128,
                   .config_info = {0, 1, 1,  9,  8,  7,  6,  0, 14, 1, 0, BL_VPU_SYS}};
bw_limit_t bl_mm_vpp            = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x01c, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_mm_gdc            = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x020, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_mm_tdp            = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x024, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_mm_ive            = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x028, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_mm_dpu            = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x02c, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_mm_dpu_lite       = {0x04401000, 0x0ac, 0x0b0, 0x0b4, 0x0b8, 0x0c4, 0x0c8, 0x010, 0x014, 0x030, BIT_WIDTH_64,
                   .config_info = {3, 2, 3,  0,  8,  7,  6,  0, 14, 0, 1, BL_MM_SYS}};
bw_limit_t bl_isp_ife           = {0x02400000, 0x0d0, 0x0d4, 0x0d8, 0x0dc, 0x0e0, 0x0e4, 0x18c, 0x190, 0x194, BIT_WIDTH_64,
                   .config_info = {3, 1, 1,  0,  8,  7,  6,  0, 16, 0, 1, BL_ISP_SYS}};
bw_limit_t bl_isp_itp           = {0x02480000, 0x0d0, 0x0d4, 0x0d8, 0x0dc, 0x0e0, 0x0e4, 0x14c, 0x150, 0x154, BIT_WIDTH_64,
                   .config_info = {3, 1, 1,  0,  8,  7,  6,  0, 16, 0, 1, BL_ISP_SYS}};
bw_limit_t bl_isp_yuv           = {0x024c0000, 0x0d0, 0x0d4, 0x0d8, 0x0dc, 0x0e0, 0x0e4, 0x150, 0x154, 0x158, BIT_WIDTH_64,
                   .config_info = {3, 1, 1,  0,  8,  7,  6,  0, 16, 0, 1, BL_ISP_SYS}};
bw_limit_t bl_cpu2ddr           = {0x02370000,    -1,    -1, 0x034, 0x038,    -1,    -1, 0x044, 0x048, 0x04c, BIT_WIDTH_64,
                   .config_info = {0, 3, 4,  0,  8,  7,  6,  0, 14, 0, 0, BL_COMM_SYS}};
bw_limit_t bl_flash_axdma       = {0x10030000,    -1,    -1, 0x4008, 0x8008, 0x4014, 0x8014, 0x16c, 0x16c, 0x1a8, BIT_WIDTH_64,
                   .config_info = {0, 7, 5, 29, 18, 19, 20, 21, 14, 1, 0, BL_FLASH_SYS}};
bw_limit_t bl_npu               = {0x03880000,    -1,    -1,     -1,     -1,     -1,     -1, 0x028, 0x02c, 0x02c, BIT_WIDTH_64,
                   .config_info = {0, 0, 0,  0, 25, 24, 24, 16,  0, 0, 0, 0}};

static proc_node_info_t sys_proc_node_array[BL_NUM_MAX] = {
	{BL_VPU_VDEC, "vpu_vdec", &bl_vpu_vdec, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_VPU_VENC, "vpu_venc", &bl_vpu_venc, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_VPU_JENC, "vpu_jenc", &bl_vpu_jenc, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_VPP, "mm_vpp", &bl_mm_vpp, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_GDC, "mm_gdc", &bl_mm_gdc, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_TDP, "mm_tdp", &bl_mm_tdp, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_IVE, "mm_ive", &bl_mm_ive, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_DPU, "mm_dpu", &bl_mm_dpu, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_MM_DPU_LITE, "mm_dpu_lite", &bl_mm_dpu_lite, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_ISP_IFE, "isp_ife", &bl_isp_ife, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_ISP_ITP, "isp_itp", &bl_isp_itp, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_ISP_YUV, "isp_yuv", &bl_isp_yuv, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_CPU2DDR, "cpu2ddr", &bl_cpu2ddr, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_FLASH_AXDMA, "flash_axdma", &bl_flash_axdma, NULL, NULL, NULL, 0, 0, 0, false},
	{BL_NPU, "npu", &bl_npu, NULL, NULL, NULL, 0, 0, 0, false}
};

static u32 bw_swap_wr_rd(u32 value, u32 pos)
{
	u32 temp = 0;
	u32 mask_bits = (1 << pos) - 1;
	if (!value) {
		return 0;
	}

	temp = value & mask_bits;
	temp = temp << pos;
	temp |= (value >> pos) & mask_bits;
	return temp;
}

static void bw_limiter_hw_enable(u32 id, char en)
{
	void *temp_vaddr;
	static u32 status[BL_SYS_ID_MAX] = {0};
	bw_limit_t *limiter = sys_proc_node_array[id].bw_limiter;
	u32 limiter_sys_id = limiter->config_info.limiter_sys_id;

	if (limiter_sys_id == 0) {
		//disable limiter
		if (!en) {
			temp_vaddr = ioremap(limiter->limiter_base_paddr +
						limiter->limiter_ctrl_paddr_offset, 0x4);
			if (temp_vaddr) {
				writel(0, temp_vaddr);
				iounmap(temp_vaddr);
			}
		}
		return ;
	}

	if (en) {
		if (limiter->limiter_clk_en_set_offset > 0 && !status[limiter_sys_id]) {
			temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
					limiter->limiter_clk_en_set_offset, 0x4);
			if (temp_vaddr) {
				writel(BIT(limiter->config_info.limiter_clk_en_shift), temp_vaddr);
				iounmap(temp_vaddr);
			}
		}

		if (limiter->limiter_clk_eb_set_offset > 0) {
			temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
						limiter->limiter_clk_eb_set_offset, 0x4);
			if (temp_vaddr) {
				writel(BIT(limiter->config_info.limiter_clk_eb_shift), temp_vaddr);
				iounmap(temp_vaddr);
			}
		}

		if (limiter->limiter_rst_clr_offset > 0) {
			temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
						limiter->limiter_rst_clr_offset, 0x4);
			if (temp_vaddr) {
				writel(BIT(limiter->config_info.limiter_rst_shift), temp_vaddr);
				iounmap(temp_vaddr);
			}
		}
		status[limiter_sys_id] |= BIT(id);
	} else {
		status[limiter_sys_id] &= ~BIT(id);
		if (!status[limiter_sys_id] || !limiter->config_info.limiter_clk_eb_shared) {
			//disable limiter
			temp_vaddr = ioremap(limiter->limiter_base_paddr +
						limiter->limiter_ctrl_paddr_offset, 0x4);
			if (temp_vaddr) {
				writel(0, temp_vaddr);
				iounmap(temp_vaddr);
			}
			//clk & reset
			if (limiter->limiter_clk_eb_clr_offset > 0) {
				temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
							limiter->limiter_clk_eb_clr_offset, 0x4);
				if (temp_vaddr) {
					writel(BIT(limiter->config_info.limiter_clk_eb_shift), temp_vaddr);
					iounmap(temp_vaddr);
				}
			}

			if (limiter->limiter_rst_set_offset > 0) {
				temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
							limiter->limiter_rst_set_offset, 0x4);
				if (temp_vaddr) {
					writel(BIT(limiter->config_info.limiter_rst_shift), temp_vaddr);
					iounmap(temp_vaddr);
				}
			}

			if (limiter->limiter_clk_en_clr_offset > 0 && !status[limiter_sys_id]) {
				if (limiter->limiter_clk_en_clr_offset) {
					temp_vaddr = ioremap(bl_sys_base_addr[limiter_sys_id] +
							limiter->limiter_clk_en_clr_offset, 0x4);
					if (temp_vaddr) {
						writel(BIT(limiter->config_info.limiter_clk_en_shift), temp_vaddr);
						iounmap(temp_vaddr);
					}
				}
			}
		}
	}

	return ;
};

static int ddr_bw_limiter_ctrl(u32 id, bw_limiter_ctrl_t *ctrl, bw_limiter_value_t *value,
			       BW_OPS_E bw_rw)
{
	u32 tmp_val;
	u32 ctrl_val;
	u32 limiter_val;
	u32 limiter_update;
	void *limiter_base_vaddr, *temp_vaddr;
	bw_limit_t *limiter = sys_proc_node_array[id].bw_limiter;
	u32 limiter_val_shift = limiter->config_info.limiter_value_h_shift;
	u32 limiter_val_mask = limiter_val_shift ? (1 << limiter_val_shift) - 1 : 0x3FFF;
	BW_LOG_DBG("start\n");

	if (ctrl == NULL || limiter == NULL) {
		BW_LOG_ERR("one of parameters is NULL !!!\n");
		return -1;
	}

	if (limiter->limiter_base_paddr == NO_EXIST) {
		BW_LOG_ERR("this BW Limiter doesn't exist\n");
		return 0;
	}

	BW_LOG_DBG("limiter_base_paddr %x\n", limiter->limiter_base_paddr);
	limiter_base_vaddr = ioremap(limiter->limiter_base_paddr, 0x1000);
	if (!limiter_base_vaddr) {
		BW_LOG_ERR("ioremap failed !!!\n");
		return -1;
	}

	limiter_val = readl(limiter_base_vaddr + limiter->limiter_value_paddr_offset);
	limiter_val &= (limiter_val_mask | (limiter_val_mask << limiter_val_shift));
	if (limiter->config_info.limiter_value_h_is_rd)
		limiter_val = bw_swap_wr_rd(limiter_val, limiter_val_shift);

	BW_LOG_DBG("limiter_value_wr = 0x%x, value_rd = 0x%x \n", value->limiter_value_wr,
		   value->limiter_value_rd);

	if (bw_rw == BW_PORT_RD) {
		limiter_val &= ~limiter_val_mask;
		limiter_val |= (value->limiter_value_rd & limiter_val_mask);
	} else if (bw_rw == BW_PORT_WR) {
		limiter_val &= ~(limiter_val_mask << limiter_val_shift);
		limiter_val |= (value->limiter_value_wr & limiter_val_mask) << limiter_val_shift;
	} else {
		BW_LOG_ERR("don't support this Mode : %d\n", bw_rw);
	}
	bw_limiter_hw_enable(id, 1);
	if (limiter_val) {
		ctrl_val = (ctrl->limiter_enable & 0x1) << limiter->config_info.limiter_enable_shift |
				(ctrl->auto_limiter_wr & 0x1) << limiter->config_info.auto_limiter_wr_shift |
				(ctrl->auto_limiter_rd & 0x1) << limiter->config_info.auto_limiter_rd_shift |
				(ctrl->interl_value & 0x3F) << limiter->config_info.interl_value_shift;

		writel(ctrl_val, limiter_base_vaddr + limiter->limiter_ctrl_paddr_offset);
	}

	BW_LOG_DBG("bw limiter ctrl_val = 0x%x, new limiter_val = 0x%x\n", ctrl_val, limiter_val);
	temp_vaddr = limiter_base_vaddr + limiter->limiter_value_paddr_offset;
	tmp_val = readl(temp_vaddr);
	tmp_val &= ~(limiter_val_mask | (limiter_val_mask << limiter_val_shift));
	if (limiter->config_info.limiter_value_h_is_rd) {
		writel(tmp_val | bw_swap_wr_rd(limiter_val, limiter_val_shift), temp_vaddr);
	} else {
		writel(tmp_val | limiter_val, temp_vaddr);
	}

	temp_vaddr = limiter_base_vaddr + limiter->limiter_update_paddr_offset;
	limiter_update = readl(temp_vaddr);
	writel(limiter_update | (1 << limiter->config_info.limiter_update_shift), temp_vaddr);
	mdelay(1);
	writel(limiter_update & ~(1 << limiter->config_info.limiter_update_shift), temp_vaddr);

	if (limiter_base_vaddr) {
		iounmap(limiter_base_vaddr);
	}

	if (!limiter_val) {
		ctrl->limiter_enable = 0;
		bw_limiter_hw_enable(id, 0);
	}

	BW_LOG_DBG("end\n");
	return 0;
}

int bw_limiter_set(u32 id, u32 bw_val, BW_OPS_E bw_rw)
{
	int ret = 0;
	u32 value;
	bw_limiter_ctrl_t limiter_ctrl;
	bw_limiter_value_t limiter_value;
	bw_limit_t *limiter = sys_proc_node_array[id].bw_limiter;
	#define WINDOW_INTERVAL_US 2

	BW_LOG_DBG("this limiter 's bw_val = %d MB, bw_ops = %s\n", bw_val,
		   bw_rw == BW_PORT_WR ? "wr" : "rd");
	limiter_ctrl.limiter_enable = 1;
	limiter_ctrl.auto_limiter_wr = 1;
	limiter_ctrl.auto_limiter_rd = 1;

	/* 1. computer interl_value */
	limiter_ctrl.interl_value = WINDOW_INTERVAL_US * 3 / 2;
	/* 2. computer value on base of bw ,like 1000 = 1GB */
	/*    the unit of bw_val is MB */
	value = bw_val * WINDOW_INTERVAL_US / (limiter->bus_bitwidth >> 3);

	BW_LOG_DBG("value 1= %d, 2= %d\n", (bw_val * WINDOW_INTERVAL_US), (limiter->bus_bitwidth >> 3));

	if (bw_rw == BW_PORT_WR) {
		limiter_value.limiter_value_wr = value;
		if (!limiter->config_info.limiter_value_h_shift)
			limiter_value.limiter_value_rd = value;
	} else if (bw_rw == BW_PORT_RD) {
		limiter_value.limiter_value_rd = value;
		if (!limiter->config_info.limiter_value_h_shift)
			limiter_value.limiter_value_wr = value;
	}

	ret = ddr_bw_limiter_ctrl(id, &limiter_ctrl, &limiter_value, bw_rw);
	if (ret) {
		BW_LOG_DBG("ddr limiter set failed\n");
		return ret;
	}

	return ret;
}

static int bw_find_id_by_proc_name(const char *name)
{
	u32 i;
	if (!name) {
		return -1;
	}

	for (i = 0; i < BL_NUM_MAX; i++) {
		if (!strcmp(sys_proc_node_array[i].proc_name, name)) {
			break;
		}
	}
	return i;
}

/*
	convert inputed string to integer
*/
static int bw_limiter_str2int(const char __user *buffer, u32 count, u32 *data)
{
	char kbuf[8];
	u32 i, temp;
	u32 input_flag = 0;
	u32 value = 0;

	if (count > 8) {
		return -1;
	}

	if (copy_from_user(kbuf, buffer, count)) {
		return -EFAULT;
	}

	for (i = 0; i < count; i++) {
		if ((kbuf[i] >= '0') && (kbuf[i] <= '9')) {
			input_flag = 1;
			temp = kbuf[i] - '0';
			value = value * 10 + temp;
		} else {
			if (input_flag) {
				break;
			} else {
				return -1;
			}
		}
	}

	if (data != NULL) {
		*data = value;
	} else {
		BW_LOG_ERR(" data is NULL\n");
		return -1;
	}

	return 0;
}

static int bw_find_id_by_proc_entry(struct proc_dir_entry *entry, BW_OPS_E ops_rw)
{
	u32 i;
	if (!entry) {
		return -1;
	}
	BW_LOG_DBG("bw_find_id_by_proc_entry entry= %px\n", entry);
	for (i = 0; i < BL_NUM_MAX; i++) {
		if ((ops_rw == BW_PORT_RD && sys_proc_node_array[i].proc_rfile == entry)
		    || (ops_rw == BW_PORT_WR && (sys_proc_node_array[i].proc_wfile == entry))) {
			BW_LOG_DBG("ops_rw = %d, [%d].proc_rfile = %px, proc_wfile = %px\n", ops_rw,
				   i, sys_proc_node_array[i].proc_rfile,
				   sys_proc_node_array[i].proc_wfile);
			break;
		}
	}
	return i;
}

static ssize_t ax_bw_limiter_rd_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *ppos)
{
	u32 id;
	u32 *bw_value;
	struct proc_dir_entry *entry;

	mutex_lock(&ax_bw_limiter_mutex);
	entry = PDE_DATA(file_inode(file));
	id = bw_find_id_by_proc_entry(entry, BW_PORT_RD);
	if (id >= BL_NUM_MAX) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to find the right id of sub sys\n");
		return -1;
	}

	bw_value = &sys_proc_node_array[id].bwLimiter_value_rd;
	if (bw_limiter_str2int(buffer, count, bw_value)) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to convert integer value \n");
		return -1;
	}
#ifdef CHECK_SYS_ENABLE
	if (sys_proc_node_array[id].sub_sys_enabled == false) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR("sub sys[%s] is not ready, please first power it!\n",
			   sys_proc_node_array[id].proc_name);
		return -1;
	}
#endif
	bw_limiter_set(id, *bw_value, BW_PORT_RD);
	mutex_unlock(&ax_bw_limiter_mutex);

	return count;
}

static int ax_bw_limiter_rd_proc_show(struct seq_file *m, void *v)
{
	u32 id;
	struct proc_dir_entry *local_entry;

	mutex_lock(&ax_bw_limiter_mutex);
	local_entry = PDE_DATA(file_inode(m->file));
	BW_LOG_DBG("ax_bw_limiter_rd_proc_read = %px\n", local_entry);
	id = bw_find_id_by_proc_entry(local_entry, BW_PORT_RD);
	if (id >= BL_NUM_MAX) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to find the right id[%d] of sub sys\n", id);
		return -1;
	}

	seq_printf(m, "%d MB\n", sys_proc_node_array[id].bwLimiter_value_rd);
	mutex_unlock(&ax_bw_limiter_mutex);
	return 0;
}

static int ax_bw_limiter_rd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_bw_limiter_rd_proc_show, NULL);
}

static struct file_operations ax_bw_limiter_rd_fops = {
	.open = ax_bw_limiter_rd_proc_open,
	.read = seq_read,
	.write = ax_bw_limiter_rd_proc_write,
	.release = single_release,
};

static ssize_t ax_bw_limiter_create_write(struct file *file, const char __user *buffer,
					  size_t count, loff_t *ppos)
{
	char kbuf[32] = { 0 };
	u32 en;
	SUB_SYS_BW_LIMITERS sub_sys_bw;

	if (count > 32) {
		BW_LOG_ERR("invalid parameter\n");
		return -1;
	}

	if (copy_from_user(kbuf, buffer, count)) {
		return -EFAULT;
	}

	if (sscanf(kbuf, "%d %u", (int *)&sub_sys_bw, &en) != 2) {
		BW_LOG_ERR("invalid parameter\n");
		return -1;
	}

	if (sub_sys_bw < 0 || sub_sys_bw >= BL_NUM_MAX) {
		BW_LOG_ERR("invalid sub_sys_bw value\n");
		return -1;
	}

	if (en) {
		ax_bw_limiter_register_with_val(sub_sys_bw, en);
	} else {
		ax_bw_limiter_unregister(sub_sys_bw, NULL);
	}

	return count;
}

static int ax_bw_limiter_create_show(struct seq_file *m, void *v)
{
	int i;

	seq_printf(m, "%-20s   ID\n", "MODULE");
	for (i = 0; i < BL_NUM_MAX; i++)
		seq_printf(m, "%-20s : %d\n", sys_proc_node_array[i].proc_name,
			   sys_proc_node_array[i].index);
	return 0;
}

static int ax_bw_limiter_create_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_bw_limiter_create_show, NULL);
}

static struct file_operations ax_bw_limiter_create_fsops = {
	.open = ax_bw_limiter_create_open,
	.read = seq_read,
	.write = ax_bw_limiter_create_write,
	.release = single_release,
};

static ssize_t ax_bw_limiter_wr_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *ppos)
{
	u32 id;
	u32 *bw_value;
	struct proc_dir_entry *entry;

	mutex_lock(&ax_bw_limiter_mutex);
	entry = PDE_DATA(file_inode(file));
	id = bw_find_id_by_proc_entry(entry, BW_PORT_WR);
	if (id >= BL_NUM_MAX) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to find the right id of sub sys\n");
		return -1;
	}
	bw_value = &sys_proc_node_array[id].bwLimiter_value_wr;
	if (bw_limiter_str2int(buffer, count, bw_value)) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to convert integer value \n");
		return -1;
	}
#ifdef CHECK_SYS_ENABLE
	if (sys_proc_node_array[id].sub_sys_enabled == false) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR("sub sys[%s] is not ready, please first power it!\n",
			   sys_proc_node_array[id].proc_name);
		return -1;
	}
#endif
	bw_limiter_set(id, *bw_value, BW_PORT_WR);
	mutex_unlock(&ax_bw_limiter_mutex);
	return count;
}

static int ax_bw_limiter_wr_proc_show(struct seq_file *m, void *v)
{
	u32 id;
	struct proc_dir_entry *local_entry;

	mutex_lock(&ax_bw_limiter_mutex);
	local_entry = PDE_DATA(file_inode(m->file));
	BW_LOG_DBG("ax_bw_limiter_wr_proc_read = %px\n", local_entry);
	id = bw_find_id_by_proc_entry(local_entry, BW_PORT_WR);
	if (id >= BL_NUM_MAX) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR(" fail to find the right id[%d] of sub sys\n", id);
		return -1;
	}

	seq_printf(m, "%d MB\n", sys_proc_node_array[id].bwLimiter_value_wr);
	mutex_unlock(&ax_bw_limiter_mutex);
	return 0;
}

static int ax_bw_limiter_wr_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_bw_limiter_wr_proc_show, NULL);
}

static struct file_operations ax_bw_limiter_wr_fsops = {
	.open = ax_bw_limiter_wr_proc_open,
	.read = seq_read,
	.write = ax_bw_limiter_wr_proc_write,
	.release = single_release,
};

static int ax_bw_limiter_proc_create_file(const char *proc_limiter)
{
	u32 id;
	char *tmp_name;
	bw_limit_t *limiter;
	struct proc_dir_entry *bw_sys_limiter_root;
	struct proc_dir_entry *bw_limiter_val_rfile = NULL;
	struct proc_dir_entry *bw_limiter_val_wfile= NULL;
	char proc_name_wr[64] = {PROC_BW_LIMITER_VAL_WR};
	char proc_name_sum_rdwr[64] = {PROC_BW_LIMITER_VAL_SUM_RDWR};

	if (!proc_limiter) {
		return -1;
	}

	BW_LOG_DBG("generated proc name : %s\n", proc_limiter);

	id = bw_find_id_by_proc_name(proc_limiter);
	if (id >= BL_NUM_MAX) {
		BW_LOG_ERR("cann't find the right id to index sub sys\n");
		return -1;
	}

	bw_sys_limiter_root = proc_mkdir(proc_limiter, bw_limiter_root);
	limiter = sys_proc_node_array[id].bw_limiter;
	if (limiter->config_info.limiter_value_h_shift) {
		bw_limiter_val_rfile =
		proc_create_data(PROC_BW_LIMITER_VAL_RD, 0644, bw_sys_limiter_root,
				&ax_bw_limiter_rd_fops, NULL);
		if (!bw_limiter_val_rfile) {
			BW_LOG_ERR("create proc bw_limiter_rd failen\n");
			return -1;
		}
		bw_limiter_val_rfile->data = bw_limiter_val_rfile;
	}
	tmp_name = limiter->config_info.limiter_value_h_shift ? proc_name_wr : proc_name_sum_rdwr;
	bw_limiter_val_wfile =
	proc_create_data(tmp_name, 0644, bw_sys_limiter_root,
			&ax_bw_limiter_wr_fsops, NULL);
	if (!bw_limiter_val_wfile) {
		BW_LOG_ERR("create proc bw_limiter_wr failen\n");
		return -1;
	}
	bw_limiter_val_wfile->data = bw_limiter_val_wfile;

	sys_proc_node_array[id].root_proc_file = bw_sys_limiter_root;
	sys_proc_node_array[id].proc_rfile = bw_limiter_val_rfile;
	sys_proc_node_array[id].proc_wfile = bw_limiter_val_wfile;
	return 0;
}

static int ax_bw_limiter_probe(struct platform_device *pdev)
{
	bw_limiter_root = proc_mkdir(PROC_NODE_ROOT_NAME, NULL);
	if (bw_limiter_root == NULL) {
		BW_LOG_ERR("create bw limiter root proc node failed!\n");
		return -1;
	}
	ax_bw_limiter_register_with_val(BL_CPU2DDR, 0);
	ax_bw_limiter_register_with_val(BL_FLASH_AXDMA, 0);
	proc_create_data(PROC_BW_LIMITER_REGISTER, 0644, bw_limiter_root,
			 &ax_bw_limiter_create_fsops, NULL);
	BW_LOG_DBG("ax_bw_limiter_probe\r\n");
	return 0;
}

static int ax_bw_limiter_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < BL_NUM_MAX; i++)
		if (sys_proc_node_array[i].sub_sys_enabled == true)
			ax_bw_limiter_unregister(i, NULL);
	remove_proc_entry(PROC_BW_LIMITER_REGISTER, bw_limiter_root);
	remove_proc_entry(PROC_NODE_ROOT_NAME, NULL);
	return 0;
}

int ax_bw_limiter_refresh_limiter(SUB_SYS_BW_LIMITERS sub_sys_bw)
{
	int ret = 0;
	u32 bw_val;
	void *limiter_base_vaddr;
	bw_limit_t *limiter = sys_proc_node_array[sub_sys_bw].bw_limiter;
	if (sub_sys_bw < 0 || sub_sys_bw >= BL_NUM_MAX) {
		BW_LOG_ERR("invalid sub_sys_bw value\n");
		return -EINVAL;
	}

	mutex_lock(&ax_bw_limiter_mutex);
	if (sys_proc_node_array[sub_sys_bw].sub_sys_enabled == false) {
		BW_LOG_ERR("not register before\n");
		ret = -EPERM;
		goto end;
	}

	limiter_base_vaddr = ioremap(limiter->limiter_base_paddr, 0x1000);
	if (!limiter_base_vaddr) {
		BW_LOG_ERR("ioremap failed !!!\n");
		ret = -ENOMEM;
		goto end;
	}

	bw_val = readl(limiter_base_vaddr + limiter->limiter_value_paddr_offset);
	iounmap(limiter_base_vaddr);
	if (bw_val) {
		BW_LOG_DBG("no need to refresh\n");
		ret = 0;
		goto end;
	}

	if (limiter->config_info.limiter_value_h_shift) {
		bw_val = sys_proc_node_array[sub_sys_bw].bwLimiter_value_rd;
		bw_limiter_set(sub_sys_bw, bw_val, BW_PORT_RD);
	}
	bw_val = sys_proc_node_array[sub_sys_bw].bwLimiter_value_wr;
	bw_limiter_set(sub_sys_bw, bw_val, BW_PORT_WR);

end:
	mutex_unlock(&ax_bw_limiter_mutex);
	return ret;
}
EXPORT_SYMBOL(ax_bw_limiter_refresh_limiter);

int ax_bw_limiter_register_with_clk(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk)
{
	int ret;
	if (sub_sys_bw < 0 || sub_sys_bw >= BL_NUM_MAX) {
		BW_LOG_ERR("invalid sub_sys_bw value\n");
		return -1;
	}

	mutex_lock(&ax_bw_limiter_mutex);
	if (sys_proc_node_array[sub_sys_bw].sub_sys_enabled == true) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_NOTICE("attempt to register the same bwlimiter once more\n");
		return -1;
	}
	sys_proc_node_array[sub_sys_bw].sub_sys_enabled = true;

	ret = ax_bw_limiter_proc_create_file(sys_proc_node_array[sub_sys_bw].proc_name);
	if (ret) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR("create proc node[%s] failed!\n",
			   sys_proc_node_array[sub_sys_bw].proc_name);
		return -1;
	}

	mutex_unlock(&ax_bw_limiter_mutex);

	return 0;
}
EXPORT_SYMBOL(ax_bw_limiter_register_with_clk);

int ax_bw_limiter_register_with_val(SUB_SYS_BW_LIMITERS sub_sys_bw, u32 work_clk)
{
	int ret;
	if (sub_sys_bw < 0 || sub_sys_bw >= BL_NUM_MAX) {
		BW_LOG_ERR("invalid sub_sys_bw value\n");
		return -1;
	}

	mutex_lock(&ax_bw_limiter_mutex);
	if (sys_proc_node_array[sub_sys_bw].sub_sys_enabled == true) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_NOTICE("attempt to register the same bwlimiter once more\n");
		return -1;
	} else {
		sys_proc_node_array[sub_sys_bw].sub_sys_enabled = true;
	}

	ret = ax_bw_limiter_proc_create_file(sys_proc_node_array[sub_sys_bw].proc_name);
	if (ret) {
		mutex_unlock(&ax_bw_limiter_mutex);
		BW_LOG_ERR("create proc node[%s] failed!\n",
			   sys_proc_node_array[sub_sys_bw].proc_name);
		return -1;
	}
	mutex_unlock(&ax_bw_limiter_mutex);

	return 0;
}
EXPORT_SYMBOL(ax_bw_limiter_register_with_val);

int ax_bw_limiter_unregister(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk)
{
	char *tmp_name;
	char proc_name_wr[64] = {PROC_BW_LIMITER_VAL_WR};
	char proc_name_sum_rdwr[64] = {PROC_BW_LIMITER_VAL_SUM_RDWR};
	bw_limit_t *limiter = sys_proc_node_array[sub_sys_bw].bw_limiter;

	if (sub_sys_bw < 0 || sub_sys_bw >= BL_NUM_MAX) {
		BW_LOG_ERR("invalid sub_sys_bw value\n");
		return -1;
	}

	mutex_lock(&ax_bw_limiter_mutex);
	if (sys_proc_node_array[sub_sys_bw].sub_sys_enabled == false) {
		BW_LOG_NOTICE("sub sys[%s] wasn't enabled before!!!\n",
			   sys_proc_node_array[sub_sys_bw].proc_name);
		mutex_unlock(&ax_bw_limiter_mutex);
		return -1;
	}

	if (limiter->config_info.limiter_value_h_shift) {
		remove_proc_entry(PROC_BW_LIMITER_VAL_RD, sys_proc_node_array[sub_sys_bw].root_proc_file);
	}
	tmp_name = limiter->config_info.limiter_value_h_shift ? proc_name_wr : proc_name_sum_rdwr;
	remove_proc_entry(tmp_name, sys_proc_node_array[sub_sys_bw].root_proc_file);
	remove_proc_entry(sys_proc_node_array[sub_sys_bw].proc_name, bw_limiter_root);
	sys_proc_node_array[sub_sys_bw].sub_sys_enabled = false;
	sys_proc_node_array[sub_sys_bw].proc_rfile = NULL;
	sys_proc_node_array[sub_sys_bw].proc_wfile = NULL;
	mutex_unlock(&ax_bw_limiter_mutex);

	return 0;
}
EXPORT_SYMBOL(ax_bw_limiter_unregister);

static const struct of_device_id ax_bw_limiter_of_match[] = {
	{.compatible = "axera,bw_limiter"},
	{},
};

static struct platform_driver ax_bw_limiter_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = ax_bw_limiter_of_match,
	},
	.probe = ax_bw_limiter_probe,
	.remove = ax_bw_limiter_remove,
};

static int __init ax_bw_limiter_init(void)
{
	platform_driver_register(&ax_bw_limiter_driver);
	return 0;
}

static void __exit ax_bw_limiter_exit(void)
{
	return platform_driver_unregister(&ax_bw_limiter_driver);
}

module_init(ax_bw_limiter_init);
module_exit(ax_bw_limiter_exit);

MODULE_AUTHOR("axera");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
MODULE_INFO(intree, "Y");
