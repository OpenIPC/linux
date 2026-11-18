/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/pm.h>
#include "ax_gzipd_reg.h"
#include "ax_gzipd_api.h"
#include "ax_gzipd_sys.h"
#include "ax_gzipd_adapter.h"
#include "ax_gzipd_log.h"
#include "ax_gzipd_hal_type.h"
#include "ax_gzipd_mng.h"
#include "ax_gzipd_drv.h"

// #define AX620E_KERNEL_FRAMEWORK

static ax_gzipd_dev_t *g_ax_gzipd_dev = NULL;
static atomic_t g_gzipd_dev_init_account = ATOMIC_INIT(0);
static int32_t gzipd_trigger_flag = AX_FALSE;
#ifdef CONFIG_PM_SLEEP
static ax_gzipd_reg_ctx_t ax_gzipd_pm_ctx;
#endif

static DEFINE_SPINLOCK(gzipd_dev_lock);
#ifdef AX620E_KERNEL_FRAMEWORK
static int gzipd_dev_clk_enable(ax_gzipd_dev_t *gzipd_dev);
static int gzipd_dev_clk_disable(ax_gzipd_dev_t *gzipd_dev);
static int32_t gzipd_dev_hw_reset(ax_gzipd_dev_t *gzipd_dev);
#endif
static irqreturn_t gzipd_interrupt_handler(int irq, void *data);

static int gzipd_dev_clk_auto_gate(bool close)
{
	if (close) {
		gzipd_flash_reg_write(FLASH_CLK_EN_0_CLR_REG, CLK_GZIPD_CORE_EB_BIT);
		gzipd_flash_reg_write(FLASH_CLK_EN_1_CLR_REG, CLK_GZIPD_EB_BIT);
	} else {
		gzipd_flash_reg_write(FLASH_CLK_EN_0_SET_REG, CLK_GZIPD_CORE_EB_BIT);
		gzipd_flash_reg_write(FLASH_CLK_EN_1_SET_REG, CLK_GZIPD_EB_BIT);
	}
	return gzipd_ok;
}

static void gzipd_dev_clear_interrupt(void)
{
	gzipd_dev_clk_auto_gate(GZIPD_CLK_ENABLE);
	gzipd_reg_write(GZIPD_INTR_CLR_REG, 1);
}

static void gzipd_dev_set_clk_reg(int workclk)
{
	gzipd_dev_clk_auto_gate(GZIPD_CLK_DISABLE);
	gzipd_udelay(100);

	gzipd_flash_reg_write(FLASH_CLK_MUX0_SET_REG, (workclk << 9 | 0x5 << 6));

	gzipd_dev_clk_auto_gate(GZIPD_CLK_ENABLE);
}

static void gzipd_dev_set_rst_reg(void)
{
	gzipd_dev_clk_auto_gate(GZIPD_CLK_DISABLE);

	gzipd_flash_reg_write(FLASH_SW_RST0_SET_REG, GZIPD_SW_RST | GZIPD_CORE_SW_RST);
	gzipd_udelay(1);
	gzipd_flash_reg_write(FLASH_SW_RST0_CLR_REG, GZIPD_SW_RST | GZIPD_CORE_SW_RST);

	gzipd_dev_clk_auto_gate(GZIPD_CLK_ENABLE);
}

static int gzipd_dev_init(void)
{
#ifdef AX620E_KERNEL_FRAMEWORK
	int ret;
#endif
	spin_lock(&gzipd_dev_lock);
	gzipd_dev_clk_auto_gate(GZIPD_CLK_ENABLE);

#ifdef AX620E_KERNEL_FRAMEWORK
	spin_unlock(&gzipd_dev_lock);
	if (g_ax_gzipd_dev) {
		ret = gzipd_dev_hw_reset(g_ax_gzipd_dev);
		if (ret) {
			return -1;
		}

		ret = gzipd_dev_clk_enable(g_ax_gzipd_dev);
		if (ret) {
			return -1;
		}

		gzipd_dev_clear_interrupt();
		AX_GZIP_DEV_LOG_DBG("clk mux val = 0x%x\n", gzipd_flash_reg_read(FLASH_CLK_MUX0_REG));
	}
#else
	gzipd_dev_set_rst_reg();
	gzipd_dev_set_clk_reg(EPLL_500M);

#ifdef GZIPD_BYPASS_EN
	gzipd_reg_write(GZIPD_WDMA_CFG1_REG, osize-1);
#endif
	spin_unlock(&gzipd_dev_lock);
#endif

	AX_GZIP_DEV_LOG_DBG("gzipd device init finish");
	return 0;
}

static int gzipd_init(void)
{
	if (atomic_inc_return(&g_gzipd_dev_init_account) > 1) {
		return AX_GZIPD_STATUS_OK;
	}

	AX_GZIP_DEV_LOG_DBG("enter");
	gzipd_dev_init();
	gzipd_instance_init_handle();
	gzipd_thread_init();
	AX_GZIP_DEV_LOG_DBG("exit");
	return AX_GZIPD_STATUS_OK;
}

static int gzipd_dev_deinit(void)
{
	spin_lock(&gzipd_dev_lock);
	/*  TODO  */
	spin_unlock(&gzipd_dev_lock);
#ifdef AX620E_KERNEL_FRAMEWORK
	gzipd_dev_clk_disable(g_ax_gzipd_dev);
#else
	gzipd_dev_clk_auto_gate(GZIPD_CLK_DISABLE);
#endif
	return AX_GZIPD_STATUS_OK;
}

static int gzipd_deinit(void)
{
	if (atomic_read(&g_gzipd_dev_init_account) == 0 ) {
		return AX_GZIPD_STATUS_OK;
	}

	if (atomic_dec_return(&g_gzipd_dev_init_account) > 0) {
		return AX_GZIPD_STATUS_OK;
	}

	AX_GZIP_DEV_LOG_DBG("enter");
	gzipd_thread_deinit();
	gzipd_dev_deinit();
	gzipd_trigger_flag = AX_FALSE;
	AX_GZIP_DEV_LOG_DBG("exit");
	return 0;
}

static bool gzipd_dev_tile_is_available(void)
{
	return ((gzipd_reg_read(GZIPD_STATUS0_REG) >> 16) & 0x1F) < CMDQ_DEPTH;
}

int gzipd_dev_tile_config(uint64_t outAddr, uint64_t outlen, uint32_t blknum, uint32_t tilenum)
{
	if ((outAddr % 8)) {
		AX_GZIP_DEV_LOG_ERR(" output addr[0x%llx] is not aligned to 8bytes or tilenum[%d] is 0", outAddr, tilenum);
		return gzipd_error;
	}

#ifdef GZIPD_BYPASS_EN
#define BYPASS_EN 1
#else
#define BYPASS_EN 0
#endif

	spin_lock(&gzipd_dev_lock);
	gzipd_reg_write(GZIPD_WDMA_CFG1_REG, outlen-1);
	gzipd_reg_write(GZIPD_WDMA_CFG0_REG, outAddr >> 3);
	if (tilenum == 0) {
		gzipd_reg_write(GZIPD_RDMA_CFG0_REG, (0x10 << 11) | (blknum << 16) | BYPASS_EN << 31);
	} else {
		gzipd_reg_write(GZIPD_RDMA_CFG0_REG, (tilenum) | (0x10 << 11) | (blknum << 16) | BYPASS_EN << 31);
	}
	spin_unlock(&gzipd_dev_lock);

	return gzipd_ok;
}
int gzipd_dev_tile_cmdq(uint64_t startAddr, uint64_t tilelen)
{
	if ((startAddr % 8)) {
		AX_GZIP_DEV_LOG_ERR(" tile addr is not aligned to 8bytes or tilelen less than 8KB");
		return gzipd_error;
	}

	if (!gzipd_dev_tile_is_available()) {
		return gzipd_error;
	}

	gzipd_reg_write(GZIPD_RDMA_CFG1_REG, startAddr >> 3);
	gzipd_reg_write(GZIPD_RDMA_CFG2_REG, tilelen - 1);
	return gzipd_ok;
}

void gzipd_dev_trigger_once(void)
{
	if (gzipd_trigger_flag == AX_FALSE) {
		gzipd_trigger_flag = AX_TRUE;
		gzipd_reg_write(GZIPD_CTRL_CFG_REG,0x1D01u); /* disable almost empty int */
		AX_GZIP_DEV_LOG_DBG(" start to run gzipd.......");
	}
}

void gzipd_dev_trigger_restore(void)
{
	if (gzipd_trigger_flag == AX_TRUE) {
		gzipd_trigger_flag = AX_FALSE;
		AX_GZIP_DEV_LOG_DBG(" restore trigger flag to initial status");
	}
}

int gzipd_dev_show_regvalue(void)
{
	int row;
	uint32_t val0; uint32_t val1;
	uint32_t val2; uint32_t val3;
	AX_GZIP_DEV_LOG_ERR(" >>>>>>>>>>>>>>>>>>>>>>>> gzip reg value >>>>>>>>>>>>>>>>>>>>>>>>");
	for (row = 0; row < 3; row++) {
		val0 = gzipd_reg_read(row * 0x10 + 0 * 4);
		val1 = gzipd_reg_read(row * 0x10 + 1 * 4);
		val2 = gzipd_reg_read(row * 0x10 + 2 * 4);
		val3 = gzipd_reg_read(row * 0x10 + 3 * 4);
		AX_GZIP_DEV_LOG_ERR("    offset-%d0 : [0x%08x, 0x%08x, 0x%08x, 0x%08x]  ", row, val0, val1, val2, val3);
	}
	AX_GZIP_DEV_LOG_ERR(" <<<<<<<<<<<<<<<<<<<<<<<< gzip reg value <<<<<<<<<<<<<<<<<<<<<<<<");
	return gzipd_ok;
}

int gzipd_dev_is_corrupt(void)
{
	uint32_t statusReg1;
	statusReg1 = gzipd_reg_read(GZIPD_STATUS1_REG);
	if (statusReg1 & (0x7 << 3)) {
		AX_GZIP_DEV_LOG_ERR("gzipd GZIPD_STATUS1_REG = 0x%08x", statusReg1);
		return gzipd_error;
	}
	return gzipd_ok;
}

int gzipd_dev_get_status(uint32_t *stat_1, uint32_t *stat_2)
{
	uint32_t status1;
	uint32_t status2;
	uint32_t idx;
	#define STATUS1_MASK_BITMAP	 (0x1FF)
	#define STATUS2_MASK_BITMAP	 (0x3F)
	status1 = gzipd_reg_read(GZIPD_STATUS1_REG) & STATUS1_MASK_BITMAP;
	*stat_1 = status1;
	status2 = gzipd_reg_read(GZIPD_STATUS2_REG) & STATUS2_MASK_BITMAP;
	*stat_2 = status1;

	for (idx = 0; idx < 9; idx++) {
		uint32_t bit = (status1 >> idx ) & 0x1;
		if (!bit) {
			continue;
		}
		switch (idx) {
		case 0:
			AX_GZIP_DEV_LOG_DBG("gzip complete ok");
			break;
		case 1:
			AX_GZIP_DEV_LOG_DBG("gzip almost complete ok");
			break;
		case 2:
			AX_GZIP_DEV_LOG_ERR("gzip iblock crc error");
			break;
		case 3:
			AX_GZIP_DEV_LOG_ERR("gzip oblock crc error");
			break;
		case 4:
			AX_GZIP_DEV_LOG_ERR("gzip uncompressed size is larger than osize_m1 error");
			break;
		default:
			AX_GZIP_DEV_LOG_ERR("gzip rresp or bresp error");
			break;
		}
	}

	for (idx = 0; idx < 6; idx++) {
		uint32_t bit = (status2 >> idx ) & 0x1;
		if (!bit) {
			continue;
		}
		switch (idx) {
		case 0:
			AX_GZIP_DEV_LOG_ERR("gzip rdma cmd busy");
			break;
		case 1:
			AX_GZIP_DEV_LOG_ERR("gzip rdma axi ost busy");
			break;
		case 2:
			AX_GZIP_DEV_LOG_ERR("gzip rdma data busty");
			break;
		case 3:
			AX_GZIP_DEV_LOG_ERR("gzip wdma cmd busy");
			break;
		case 4:
			AX_GZIP_DEV_LOG_ERR("gzip wdma axi ost busy");
			break;
		case 5:
			AX_GZIP_DEV_LOG_ERR("gzip wdma data busy");
			break;
		}
	}
	return gzipd_ok;
}

/***********************************************************************************
*
* 	use kernel native api to imple this driver for gzipd device
*
***********************************************************************************/
static irqreturn_t gzipd_interrupt_handler(int irq, void *data)
{
	uint32_t raw_data;
	(void)irq;
	(void)data;

	AX_GZIP_DEV_LOG_DBG("enter");
	if (atomic_read(&g_gzipd_dev_init_account) == 0) {
		gzipd_reg_write(GZIPD_INTR_CLR_REG, 1);
		AX_GZIP_DEV_LOG_DBG("status1_reg = 0x%08x", gzipd_reg_read(GZIPD_STATUS1_REG));
		return IRQ_HANDLED;
	}

	if (gzipd_dev_is_corrupt()) {
		gzipd_dev_show_regvalue();
	}

	raw_data =  gzipd_reg_read(GZIPD_STATUS1_REG) & STATUS1_MASK_BITMAP;
	gzipd_irq_save_status(raw_data);
	if (raw_data) {
		gzipd_reg_write(GZIPD_INTR_CLR_REG, 1);
	}

	gzipd_irq_cond_broadcast();
	AX_GZIP_DEV_LOG_DBG("exit ");
	return IRQ_HANDLED;
}

static atomic_t g_gzipd_cnt;
static int ax_gzipd_open(struct inode *inode, struct file *filp)
{
	int ret = AX_GZIPD_STATUS_OK;
	if (1 == atomic_inc_return(&g_gzipd_cnt)) {
		ret = gzipd_instance_group_init((GZIPD_INSTANCE_GROUP_T **)&filp->private_data);
		inode->i_private = filp->private_data;
	}
	filp->private_data = inode->i_private;

	AX_GZIP_DEV_LOG_DBG("GZIPD device opened, count = %d", atomic_read(&g_gzipd_cnt));
	return ret;
}

static int ax_gzipd_release(struct inode *inode, struct file *filp)
{
	int ret = AX_GZIPD_STATUS_OK;
	AX_GZIP_DEV_LOG_DBG("enter");

	if (0 == atomic_dec_return(&g_gzipd_cnt)) {
		AX_GZIP_DEV_LOG_DBG("GZIPD device release, private_data = 0x%px", filp->private_data);
		gzipd_instance_group_deinit(filp->private_data);
		gzipd_deinit();
		inode->i_private = NULL;
	}
	return ret;
}

static long ax_gzipd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = AX_GZIPD_STATUS_FAIL;
	void __user *argp = (void __user *)arg;

	switch(cmd) {
	case GZIPD_IOCTL_CMD_DEV_INIT:
		gzipd_init();
		ret = AX_GZIPD_STATUS_OK;
		break;

	case GZIPD_IOCTL_CMD_DEV_CREATE_HANDLE:
	{
		void *tmpdata = NULL;
		ax_gzip_handle_info_t __user *userinfo;
		ax_gzip_handle_info_t info;
		int32_t handle;
		AX_GZIPD_HEADER_INFO_T headerInfo;

		userinfo = (ax_gzip_handle_info_t *)argp;
		tmpdata = kmalloc(sizeof(AX_GZIPD_HEADER_INFO_T), GFP_KERNEL);
		if (!tmpdata) {
			AX_GZIP_DEV_LOG_ERR("no enough mem");
			return AX_GZIPD_STATUS_NO_MEM;
		}

		if (copy_from_user(&info, argp, sizeof(info))) {
			kfree(tmpdata);
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}
		if (copy_from_user(tmpdata, info.gzipdData, sizeof(AX_GZIPD_HEADER_INFO_T))) {
			kfree(tmpdata);
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}

		ret = gzipd_instance_create_handle(&handle, tmpdata, &headerInfo, filp->private_data);
		if (ret) {
			kfree(tmpdata);
			AX_GZIP_DEV_LOG_ERR("create gzipd handle fail");
			return AX_GZIPD_INVALID_PARAM;
		}
		if (tmpdata) {
			kfree(tmpdata);
		}

		if (copy_to_user((void __user *)info.handle, &handle, sizeof(handle))) {
			AX_GZIP_DEV_LOG_ERR("copy to user fail");
			return -EFAULT;
		}

		if (copy_to_user(&userinfo->headerInfo, &headerInfo, sizeof(headerInfo))) {
			AX_GZIP_DEV_LOG_ERR("copy to user fail");
			return -EFAULT;
		}
		break;
	}

	case GZIPD_IOCTL_CMD_DEV_CONFIG:
	{
		GZIPD_DEV_CONF_T *usercfg = (GZIPD_DEV_CONF_T *)argp;
		GZIPD_DEV_CONF_T kusercfg;
		uint32_t handle;
		AX_GZIPD_IO_PARAM_T in_param;
		AX_GZIPD_IO_PARAM_T *userinparam;
		uint64_t tilescount;
		uint64_t last_tile_size;
		if (copy_from_user(&kusercfg, usercfg, sizeof(kusercfg))) {
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}
		handle = kusercfg.handle;
		userinparam = kusercfg.pInputParam;
		if (copy_from_user(&in_param, kusercfg.pInputParam, sizeof(in_param))) {
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}
		ret = gzipd_instance_cfg(handle, &in_param, &tilescount, &last_tile_size);
		if (ret) {
			AX_GZIP_DEV_LOG_ERR("fail. TileSz = %d, outBuf=0x%llx, outSz = %d, inSz = %d",
								 in_param.u32TileSize, in_param.stOutBuf.pPhyAddr, in_param.headerInfo.u32OutSize, in_param.headerInfo.u32OutSize);
			return AX_GZIPD_INVALID_PARAM;
		}

		if (copy_to_user(&userinparam->tilesNum, &tilescount, sizeof(tilescount))) {
			AX_GZIP_DEV_LOG_ERR("copy to user fail");
			return -EFAULT;
		}

		if (copy_to_user(&userinparam->lastTileLen, &last_tile_size, sizeof(last_tile_size))) {
			AX_GZIP_DEV_LOG_ERR("copy to user fail");
			return -EFAULT;
		}

		break;
	}

	case GZIPD_IOCTL_CMD_DEV_TILES_RUN:
	{
		GZIPD_DEV_PARAM_INFO_T *userinfo = (GZIPD_DEV_PARAM_INFO_T *)argp;
		GZIPD_DEV_PARAM_INFO_T info;
		uint32_t completeLen;
		if (copy_from_user(&info, argp, sizeof(info))) {
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}

		ret = gzipd_instance_run(info.handle, info.u64PhyAddr, info.u64DataLen, &completeLen);
		if (ret == AX_GZIPD_PART_COMPLETE || ret == AX_GZIPD_STATUS_OK) {
			AX_GZIP_DEV_LOG_DBG("start tile_addr = 0x%llx, total len = 0x%llx", info.u64PhyAddr, info.u64DataLen);
		} else {
			return AX_GZIPD_INVALID_PARAM;
		}

		if (copy_to_user(&userinfo->u32CompleteLen, &completeLen, sizeof(completeLen))) {
			AX_GZIP_DEV_LOG_ERR("copy to user fail");
			return -EFAULT;
		}
		break;
	}

	case GZIPD_IOCTL_CMD_DEV_LASTTILE_RUN:
	{
		GZIPD_DEV_PARAM_INFO_T info;
		if (copy_from_user(&info, argp, sizeof(info))) {
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}
		ret = gzipd_instance_lasttile_run(info.handle, info.u64PhyAddr, info.u64DataLen);
		if (ret) {
			AX_GZIP_DEV_LOG_ERR("start tile_addr = 0x%llx, lasttile size = 0x%llx", info.u64PhyAddr, info.u64DataLen);
			return AX_GZIPD_INVALID_PARAM;
		}

		break;
	}

	case GZIPD_IOCTL_CMD_DEV_WAIT_FINISH:
		ret = gzipd_instance_query_result(argp);
		break;

	case GZIPD_IOCTL_CMD_DEV_DESTROY_HANDLE:
	{
		uint32_t handle;
		if (copy_from_user(&handle, argp, sizeof(uint32_t))) {
			AX_GZIP_DEV_LOG_ERR("copy from user fail");
			return -EFAULT;
		}

		ret = gzipd_instance_destroy(handle);
		break;
	}

	case GZIPD_IOCTL_CMD_DEV_DEINIT:
		ret = gzipd_deinit();
		break;

	default:
		AX_GZIP_DEV_LOG_ERR("invalid ioctl");
		return -EFAULT;
	}
	return ret;
}

#ifdef AX620E_KERNEL_FRAMEWORK
static int32_t gzipd_dev_hw_reset(ax_gzipd_dev_t *gzipd_dev)
{
	struct reset_control *gzipd_rst = gzipd_dev->gzipd_rst;
	struct reset_control *gzipd_core_rst = gzipd_dev->gzipdCoreRst;
	gzipd_rst = devm_reset_control_get_optional(&gzipd_dev->pdev->dev, "gzipd_rst");
	if (IS_ERR(gzipd_rst)) {
		AX_GZIP_DEV_LOG_ERR("Get gzipd reset failed!\n");
		return -1;
	}
	gzipd_core_rst = devm_reset_control_get_optional(&gzipd_dev->pdev->dev, "gzipd_core_rst");
	if (IS_ERR(gzipd_core_rst)) {
		AX_GZIP_DEV_LOG_ERR("Get gzipd core reset failed!\n");
		return -1;
	}

	reset_control_assert(gzipd_rst);
	reset_control_assert(gzipd_core_rst);
	gzipd_udelay(1);
	reset_control_deassert(gzipd_rst);
	reset_control_deassert(gzipd_core_rst);
	return 0;
}

static int gzipd_dev_clk_enable(ax_gzipd_dev_t *gzipd_dev)
{
	int32_t ret;
	AX_U64 freq;
	struct clk *gzipd_clk = gzipd_dev->gzipdClk;
	struct clk *gzipd_core_clk = gzipd_dev->gzipdCoreClk;
	gzipd_clk = devm_clk_get(&gzipd_dev->pdev->dev, "gzipd_clk");
	if (IS_ERR(gzipd_clk)) {
		ret = PTR_ERR(gzipd_clk);
		AX_GZIP_DEV_LOG_ERR("Get gzipd clk failed, ret = %d!\n", ret);
		return ret;
	}
	gzipd_core_clk = devm_clk_get(&gzipd_dev->pdev->dev, "gzipd_core_clk");
	if (IS_ERR(gzipd_core_clk)) {
		ret = PTR_ERR(gzipd_core_clk);
		AX_GZIP_DEV_LOG_ERR("Get gzipd core clk failed, ret = %d!\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(gzipd_clk);
	if (ret) {
		AX_GZIP_DEV_LOG_ERR("Enable gzipd clock failed!\n");
		return ret;
	}
	ret = clk_prepare_enable(gzipd_core_clk);
	if (ret) {
		AX_GZIP_DEV_LOG_ERR("Enable gzipd core failed!\n");
		return ret;
	}

	ret = clk_set_rate(gzipd_core_clk, GZIPD_CLK_RATE);
	if (ret) {
		AX_GZIP_DEV_LOG_ERR("Set gzipd core clock rate failed, ret = %d!\n", ret);
		return ret;
	}
	freq = clk_get_rate(gzipd_core_clk);
	AX_GZIP_DEV_LOG_DBG("gzipd core clock freq = %llu\n", freq);

	return 0;
}

static int gzipd_dev_clk_disable(ax_gzipd_dev_t *gzipd_dev)
{
	if (IS_ERR(gzipd_dev->gzipdClk) || IS_ERR(gzipd_dev->gzipdCoreClk)) {
		AX_GZIP_DEV_LOG_ERR("Disable gzipd Clk failed!\n");
		return -1;
	}

	clk_disable_unprepare(gzipd_dev->gzipdCoreClk);
	clk_disable_unprepare(gzipd_dev->gzipdClk);
	return 0;
}

#endif

static const struct file_operations ax_gzipd_fops = {
	.owner = THIS_MODULE,
	.open = ax_gzipd_open,
	.unlocked_ioctl = ax_gzipd_ioctl,
	.release = ax_gzipd_release,
};

static struct miscdevice ax_gzipd_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ax_gzipd",
	.fops = &ax_gzipd_fops
};

static int ax_gzipd_probe(struct platform_device *pdev)
{
	g_ax_gzipd_dev = kzalloc(sizeof(*g_ax_gzipd_dev), GFP_KERNEL);
	if (!g_ax_gzipd_dev) {
		AX_GZIP_DEV_LOG_ERR("alloc mem for gzipd dev fail");
		return -ENOMEM;
	}
	g_ax_gzipd_dev->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	g_ax_gzipd_dev->pdev = pdev;

	misc_register(&ax_gzipd_miscdev);

	g_ax_gzipd_dev->irq = platform_get_irq(pdev, 0);
	if (g_ax_gzipd_dev->irq < 0) {
		AX_GZIP_DEV_LOG_ERR("get gzipd device irq fail");
		return -1;
	}

	gzipd_dev_clear_interrupt();
	if (0 != request_threaded_irq(g_ax_gzipd_dev->irq, gzipd_interrupt_handler, NULL, IRQF_ONESHOT, "axgzipd", NULL)) {
		AX_GZIP_DEV_LOG_ERR("request thread for gzipd error");
		return -1;
	}

	atomic_set(&g_gzipd_cnt, 0);
	return 0;
}

static int ax_gzipd_remove(struct platform_device *pdev)
{
	free_irq(g_ax_gzipd_dev->irq, NULL);
	kfree(g_ax_gzipd_dev);
	misc_deregister(&ax_gzipd_miscdev);
	return 0;
}
static const struct of_device_id ax_gzipd_of_match[] = {
	{.compatible = "axera,ax_gzipd"},
	{}
};

#ifdef CONFIG_PM_SLEEP
static void ax_gzipd_dev_pm_save_ctx(void)
{
	ax_gzipd_pm_ctx.reg_ctl_cfg = gzipd_flash_reg_read(GZIPD_CTRL_CFG_REG);
	ax_gzipd_pm_ctx.reg_wdma_cfg0 = gzipd_flash_reg_read(GZIPD_WDMA_CFG0_REG);
	ax_gzipd_pm_ctx.reg_wdma_cfg1 = gzipd_flash_reg_read(GZIPD_WDMA_CFG1_REG);
	ax_gzipd_pm_ctx.reg_rdma_cfg0 = gzipd_flash_reg_read(GZIPD_RDMA_CFG0_REG);
	ax_gzipd_pm_ctx.reg_rdma_cfg1 = gzipd_flash_reg_read(GZIPD_RDMA_CFG1_REG);
	ax_gzipd_pm_ctx.reg_rdma_cfg2 = gzipd_flash_reg_read(GZIPD_RDMA_CFG2_REG);
	AX_GZIP_DEV_LOG_DBG("sleep, save gzipd ctx");
}

static void ax_gzipd_dev_pm_restore_ctx(void)
{
	gzipd_flash_reg_write(GZIPD_CTRL_CFG_REG, ax_gzipd_pm_ctx.reg_ctl_cfg);
	gzipd_flash_reg_write(GZIPD_WDMA_CFG0_REG, ax_gzipd_pm_ctx.reg_wdma_cfg0 | 0x1);
	gzipd_flash_reg_write(GZIPD_WDMA_CFG1_REG, ax_gzipd_pm_ctx.reg_wdma_cfg1);
	gzipd_flash_reg_write(GZIPD_RDMA_CFG0_REG, ax_gzipd_pm_ctx.reg_rdma_cfg0);
	gzipd_flash_reg_write(GZIPD_RDMA_CFG1_REG, ax_gzipd_pm_ctx.reg_rdma_cfg1);
	gzipd_flash_reg_write(GZIPD_RDMA_CFG2_REG, ax_gzipd_pm_ctx.reg_rdma_cfg2);
	AX_GZIP_DEV_LOG_DBG("wakeup, restore gzipd ctx");
}

static int ax_gzipd_pm_prepare(struct device *dev)
{
	if (atomic_read(&g_gzipd_dev_init_account)) {
		ax_gzipd_dev_pm_save_ctx();
	}
	return 0;
}

static int ax_gzipd_pm_resume(struct device *dev)
{
	if (atomic_read(&g_gzipd_dev_init_account)) {
		gzipd_dev_init();
		ax_gzipd_dev_pm_restore_ctx();
	}
	return 0;
}

static const struct dev_pm_ops gzipd_pm_ops = {
	.prepare	= ax_gzipd_pm_prepare,
	.resume		= ax_gzipd_pm_resume,
};
#endif

static struct platform_driver ax_gzipd_driver = {
	.probe = ax_gzipd_probe,
	.remove = ax_gzipd_remove,
	.driver = {
		.name = "ax_gzipd",
		.of_match_table = ax_gzipd_of_match,
#ifdef CONFIG_PM_SLEEP
		.pm = &gzipd_pm_ops,
#endif
	},
};

static int __init ax_gzipd_init(void)
{
	AX_GZIP_DEV_LOG_DBG("enter ax_gzipd_init");
	gzipd_iomem_map();
	gzipd_flash_iomem_map();
	platform_driver_register(&ax_gzipd_driver);
	atomic_set(&g_gzipd_dev_init_account, 0);
	return 0;
}

static void ax_gzipd_exit(void)
{
	atomic_dec_return(&g_gzipd_dev_init_account);
	platform_driver_unregister(&ax_gzipd_driver);
	gzipd_flash_iomem_unmap();
	gzipd_iomem_unmap();
}

module_init(ax_gzipd_init);
module_exit(ax_gzipd_exit);

MODULE_AUTHOR("Axera");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");