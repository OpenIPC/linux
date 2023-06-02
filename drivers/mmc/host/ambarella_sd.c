/*
 * drivers/mmc/host/ambarella_sd.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/debugfs.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <plat/fio.h>
#include <plat/sd.h>
#include <plat/event.h>
#include <plat/rct.h>

static struct mmc_host *G_mmc[SD_INSTANCES * AMBA_SD_MAX_SLOT_NUM];

/* ==========================================================================*/
#define CONFIG_SD_AMBARELLA_TIMEOUT_VAL		(0xe)
#define CONFIG_SD_AMBARELLA_WAIT_TIMEOUT	(HZ / 100)
#define CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT	(100000)
#define CONFIG_SD_AMBARELLA_MAX_TIMEOUT		(10 * HZ)
#define CONFIG_SD_AMBARELLA_VSW_PRE_SPEC	(5)
#define CONFIG_SD_AMBARELLA_VSW_WAIT_LIMIT	(1000)

#undef CONFIG_SD_AMBARELLA_DEBUG
#undef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
#undef CONFIG_SD_AMBARELLA_TUNING_DEBUG

#define ambsd_printk(level, phcinfo, format, arg...)	\
	printk(level "%s.%u: " format, dev_name(phcinfo->pinfo->dev), \
	phcinfo->slot_id, ## arg)

#define ambsd_err(phcinfo, format, arg...)		\
	ambsd_printk(KERN_ERR, phcinfo, format, ## arg)
#define ambsd_warn(phcinfo, format, arg...)		\
	ambsd_printk(KERN_WARNING, phcinfo, format, ## arg)
#define ambsd_info(phcinfo, format, arg...)		\
	ambsd_printk(KERN_INFO, phcinfo, format, ## arg)
#define ambsd_rtdbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)

#ifdef CONFIG_SD_AMBARELLA_DEBUG
#define ambsd_dbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)
#else
#define ambsd_dbg(phcinfo, format, arg...)		\
	({ if (0) ambsd_printk(KERN_DEBUG, phcinfo, format, ##arg); 0; })
#endif

#ifdef CONFIG_SD_AMBARELLA_TUNING_DEBUG
#define ambsd_tuning_dbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)
#else
#define ambsd_tuning_dbg(phcinfo, format, arg...)		\
	({ if (0) ambsd_printk(KERN_DEBUG, phcinfo, format, ##arg); 0; })
#endif


/* ==========================================================================*/
enum ambarella_sd_state {
	AMBA_SD_STATE_IDLE,
	AMBA_SD_STATE_CMD,
	AMBA_SD_STATE_DATA,
	AMBA_SD_STATE_RESET,
	AMBA_SD_STATE_ERR
};

struct ambarella_sd_phy_timing {
	u32				mode;
	u32				val0;
	u32				val1;
};

struct ambarella_sd_mmc_info {
	struct mmc_host			*mmc;
	struct mmc_request		*mrq;

	wait_queue_head_t		wait;

	enum ambarella_sd_state		state;

	struct scatterlist		*sg;
	u32				sg_len;
	u32				wait_tmo;
	u16				blk_sz;
	u16				blk_cnt;
	u32				arg_reg;
	u16				xfr_reg;
	u16				cmd_reg;
	u16				sta_reg;
	u8				tmo;
	u8				use_adma;
	u32				sta_counter;

	char				*buf_vaddress;
	dma_addr_t			buf_paddress;
	u32				dma_address;
	u32				dma_size;

	void				(*pre_dma)(void *data);
	void				(*post_dma)(void *data);

	u32				slot_id;
	struct ambarella_sd_controller_info *pinfo;
	u32				valid;
	int				fixed_cd;
	int				fixed_wp;

	int				pwr_gpio;
	u8				pwr_gpio_active;
	int				v18_gpio;
	u8				v18_gpio_active;
	u32				no_1_8_v : 1,
					caps_ddr : 1,
					caps_adma : 1,
					force_gpio : 1;

	struct pinctrl			*pinctrl;
	struct pinctrl_state		*state_work;
	struct pinctrl_state		*state_idle;

	struct notifier_block		system_event;
	struct semaphore		system_event_sem;

	struct dentry			*debugfs;

#ifdef CONFIG_PM
	u32				sd_nisen;
	u32				sd_eisen;
	u32				sd_nixen;
	u32				sd_eixen;
#endif
};

struct ambarella_sd_controller_info {
	unsigned char __iomem 		*regbase;
	unsigned char __iomem 		*fio_reg;
	unsigned char __iomem 		*timing_reg;
	unsigned char __iomem		*sbc_reg;
	struct device			*dev;
	unsigned int			irq;
	u32				dma_fix;
	u32				reset_error;

	u32				max_blk_sz;
	struct kmem_cache		*buf_cache;

	struct clk			*clk;
	u32				default_wait_tmo;
	u32				switch_voltage_tmo;
	u8				slot_num;
	u32				phy_type;
	struct ambarella_sd_phy_timing	*phy_timing;
	u32				phy_timing_num;

	struct ambarella_sd_mmc_info	*pslotinfo[AMBA_SD_MAX_SLOT_NUM];
	struct mmc_ios			controller_ios;
	bool	auto_tuning;
};

static const u8 tuning_blk_pattern_4bit[] = {
        0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
        0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
        0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
        0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
        0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
        0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
        0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
        0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const u8 tuning_blk_pattern_8bit[] = {
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
        0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
        0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
        0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
        0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
        0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
        0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
        0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
        0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
        0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
        0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
        0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
        0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
        0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
        0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

/* ==========================================================================*/
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
static void ambarella_sd_show_info(struct ambarella_sd_mmc_info *pslotinfo)
{
	ambsd_dbg(pslotinfo, "Enter %s\n", __func__);
	ambsd_dbg(pslotinfo, "sg = 0x%x.\n", (u32)pslotinfo->sg);
	ambsd_dbg(pslotinfo, "sg_len = 0x%x.\n", pslotinfo->sg_len);
	ambsd_dbg(pslotinfo, "tmo = 0x%x.\n", pslotinfo->tmo);
	ambsd_dbg(pslotinfo, "blk_sz = 0x%x.\n", pslotinfo->blk_sz);
	ambsd_dbg(pslotinfo, "blk_cnt = 0x%x.\n", pslotinfo->blk_cnt);
	ambsd_dbg(pslotinfo, "arg_reg = 0x%x.\n", pslotinfo->arg_reg);
	ambsd_dbg(pslotinfo, "xfr_reg = 0x%x.\n", pslotinfo->xfr_reg);
	ambsd_dbg(pslotinfo, "cmd_reg = 0x%x.\n", pslotinfo->cmd_reg);
	ambsd_dbg(pslotinfo, "buf_vaddress = 0x%x.\n",
		(u32)pslotinfo->buf_vaddress);
	ambsd_dbg(pslotinfo, "buf_paddress = 0x%x.\n", pslotinfo->buf_paddress);
	ambsd_dbg(pslotinfo, "dma_address = 0x%x.\n", pslotinfo->dma_address);
	ambsd_dbg(pslotinfo, "dma_size = 0x%x.\n", pslotinfo->dma_size);
	ambsd_dbg(pslotinfo, "pre_dma = 0x%x.\n", (u32)pslotinfo->pre_dma);
	ambsd_dbg(pslotinfo, "post_dma = 0x%x.\n", (u32)pslotinfo->post_dma);
	ambsd_dbg(pslotinfo, "SD: state = 0x%x.\n", pslotinfo->state);
	ambsd_dbg(pslotinfo, "Exit %s\n", __func__);
}
#endif

void ambarella_detect_sd_slot(int slotid, int fixed_cd)
{
	struct mmc_host *mmc;
	struct ambarella_sd_mmc_info *pslotinfo;

	if (slotid >= SD_INSTANCES * AMBA_SD_MAX_SLOT_NUM) {
		pr_err("%s: Invalid slotid: %d\n", __func__, slotid);
		return;
	}

	mmc = G_mmc[slotid];
	pslotinfo = mmc_priv(mmc);
	pslotinfo->fixed_cd = fixed_cd;

	mmc_detect_change(mmc, 0);
}
EXPORT_SYMBOL(ambarella_detect_sd_slot);

static ssize_t fixed_cd_get(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct ambarella_sd_mmc_info *pslotinfo = file->private_data;
	char tmp[4];

	snprintf(tmp, sizeof(tmp), "%d\n", pslotinfo->fixed_cd);
	tmp[3] = '\n';

	return simple_read_from_buffer(buf, count, ppos, tmp, sizeof(tmp));
}

static ssize_t fixed_cd_set(struct file *file, const char __user *buf,
		size_t size, loff_t *ppos)
{
	struct ambarella_sd_mmc_info *pslotinfo = file->private_data;
	char tmp[20];
	ssize_t len;

	len = simple_write_to_buffer(tmp, sizeof(tmp) - 1, ppos, buf, size);
	if (len >= 0) {
		tmp[len] = '\0';
		pslotinfo->fixed_cd = !!simple_strtoul(tmp, NULL, 0);
	}

	mmc_detect_change(pslotinfo->mmc, 0);

	return len;
}

static const struct file_operations fixed_cd_fops = {
	.read	= fixed_cd_get,
	.write	= fixed_cd_set,
	.open	= simple_open,
	.llseek	= default_llseek,
};

static void ambarella_sd_add_debugfs(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct mmc_host *mmc = pslotinfo->mmc;
	struct dentry *root, *fixed_cd;

	if (!mmc->debugfs_root)
		return;

	root = debugfs_create_dir("ambhost", mmc->debugfs_root);
	if (IS_ERR_OR_NULL(root))
		goto err;

	pslotinfo->debugfs = root;

	fixed_cd = debugfs_create_file("fixed_cd", S_IWUSR | S_IRUGO,
			pslotinfo->debugfs, pslotinfo, &fixed_cd_fops);
	if (IS_ERR_OR_NULL(fixed_cd))
		goto err;

	return;

err:
	debugfs_remove_recursive(root);
	pslotinfo->debugfs = NULL;
	dev_err(pslotinfo->pinfo->dev, "failed to add debugfs\n");
}

static void ambarella_sd_remove_debugfs(struct ambarella_sd_mmc_info *pslotinfo)
{
	debugfs_remove_recursive(pslotinfo->debugfs);
}

static void ambarella_sd_check_dma_boundary(u32 address, u32 size, u32 max_size)
{
	u32 start_512kb, end_512kb;

	start_512kb = (address) & (~(max_size - 1));
	end_512kb = (address + size - 1) & (~(max_size - 1));
	BUG_ON(start_512kb != end_512kb);
}

static void ambarella_sd_pre_sg_to_dma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;
	u32 i, offset;

	for (i = 0, offset = 0; i < pslotinfo->sg_len; i++) {
		memcpy(pslotinfo->buf_vaddress + offset,
			sg_virt(&pslotinfo->sg[i]),
			pslotinfo->sg[i].length);
		offset += pslotinfo->sg[i].length;
	}
	BUG_ON(offset != pslotinfo->dma_size);
	dma_sync_single_for_device(pslotinfo->pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_pre_sg_to_adma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;
	int i;
	u32 offset;
	u32 dma_len;
	u32 remain_size;
	u32 current_addr;
	u32 word_num, byte_num;

	dma_len = dma_map_sg(pslotinfo->pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_TO_DEVICE);
	for (i = 0, offset = 0; i < dma_len; i++) {
		remain_size = sg_dma_len(&pslotinfo->sg[i]);
		current_addr = sg_dma_address(&pslotinfo->sg[i]);
		current_addr |= pslotinfo->pinfo->dma_fix;
		if (sd_addr_is_unlign(current_addr)) {
			ambsd_err(pslotinfo, "Please disable ADMA\n");
			BUG();
		}

		while (unlikely(remain_size > SD_ADMA_TBL_LINE_MAX_LEN)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			offset += SD_ADMA_TBL_LINE_SIZE;
			current_addr += SD_ADMA_TBL_LINE_MAX_LEN;
			remain_size -= SD_ADMA_TBL_LINE_MAX_LEN;
		}
		word_num = remain_size >> 2;
		byte_num = remain_size - (word_num << 2);
		if (word_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(word_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			current_addr += (word_num << 2);
			if (byte_num) {
				offset += SD_ADMA_TBL_LINE_SIZE;
			}
		}
		if (byte_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				byte_num << 16;
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
		}
		if (unlikely(i == dma_len - 1)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				SD_ADMA_TBL_ATTR_END;
		}
		offset += SD_ADMA_TBL_LINE_SIZE;
	}
	dma_sync_single_for_device(pslotinfo->pinfo->dev, pslotinfo->buf_paddress,
		offset, DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_post_sg_to_dma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;

	dma_sync_single_for_cpu(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress,
			pslotinfo->dma_size,
			DMA_TO_DEVICE);
}

static void ambarella_sd_post_sg_to_adma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;

	dma_sync_single_for_cpu(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress,
			pslotinfo->dma_size,
			DMA_FROM_DEVICE);

	dma_unmap_sg(pslotinfo->pinfo->dev,
			pslotinfo->sg,
			pslotinfo->sg_len,
			DMA_TO_DEVICE);
}

static void ambarella_sd_pre_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;

	dma_sync_single_for_device(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress,
			pslotinfo->dma_size,
			DMA_FROM_DEVICE);

	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_pre_adma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;
	int i;
	u32 dma_len;
	u32 offset;
	u32 remain_size;
	u32 current_addr;
	u32 word_num, byte_num;

	dma_len = dma_map_sg(pslotinfo->pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_FROM_DEVICE);
	for (i = 0, offset = 0; i < dma_len; i++) {
		remain_size = sg_dma_len(&pslotinfo->sg[i]);
		current_addr = sg_dma_address(&pslotinfo->sg[i]);
		current_addr |= pslotinfo->pinfo->dma_fix;
		if (sd_addr_is_unlign(current_addr)) {
			ambsd_err(pslotinfo, "Please disable ADMA\n");
			BUG();
		}

		while (unlikely(remain_size > SD_ADMA_TBL_LINE_MAX_LEN)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			offset += SD_ADMA_TBL_LINE_SIZE;
			current_addr += SD_ADMA_TBL_LINE_MAX_LEN;
			remain_size -= SD_ADMA_TBL_LINE_MAX_LEN;
		}
		word_num = remain_size >> 2;
		byte_num = remain_size - (word_num << 2);
		if (word_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(word_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			current_addr += (word_num << 2);
			if (byte_num) {
				offset += SD_ADMA_TBL_LINE_SIZE;
			}
		}
		if (byte_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(byte_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
		}
		if (unlikely(i == dma_len - 1)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				SD_ADMA_TBL_ATTR_END;
		}
		offset += SD_ADMA_TBL_LINE_SIZE;
	}
	dma_sync_single_for_device(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress, offset,
			DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_post_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;
	u32 i, offset;

	dma_sync_single_for_cpu(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress,
			pslotinfo->dma_size,
			DMA_FROM_DEVICE);

	for (i = 0, offset = 0; i < pslotinfo->sg_len; i++) {
		memcpy(sg_virt(&pslotinfo->sg[i]),
			pslotinfo->buf_vaddress + offset,
			pslotinfo->sg[i].length);
		offset += pslotinfo->sg[i].length;
	}
	BUG_ON(offset != pslotinfo->dma_size);
}

static void ambarella_sd_post_adma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo = data;

	dma_sync_single_for_cpu(pslotinfo->pinfo->dev,
			pslotinfo->buf_paddress,
			pslotinfo->dma_size,
			DMA_FROM_DEVICE);

	dma_unmap_sg(pslotinfo->pinfo->dev,
			pslotinfo->sg,
			pslotinfo->sg_len,
			DMA_FROM_DEVICE);
}

static void ambarella_sd_request_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	down(&pslotinfo->system_event_sem);

	if (pslotinfo->slot_id == 0) {
		fio_select_lock(SELECT_FIO_SD);
	} else {
		fio_select_lock(SELECT_FIO_SDIO);
		if (pslotinfo->force_gpio)
			pinctrl_select_state(pslotinfo->pinctrl, pslotinfo->state_work);
	}

}

static void ambarella_sd_release_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	if (pslotinfo->slot_id == 0) {
		fio_unlock(SELECT_FIO_SD);
	} else {
		if (pslotinfo->force_gpio)
			pinctrl_select_state(pslotinfo->pinctrl, pslotinfo->state_idle);
		fio_unlock(SELECT_FIO_SDIO);
	}

	up(&pslotinfo->system_event_sem);
}

static void ambarella_sd_enable_int(struct mmc_host *mmc, u32 mask)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	if (pinfo->slot_num > 1) {
		if (pslotinfo->slot_id == 0)
			fio_amb_sd0_set_int(mask, 1);
		else
			fio_amb_sdio0_set_int(mask, 1);
	} else {
		amba_setbitsl(pinfo->regbase + SD_NISEN_OFFSET, mask);
		amba_setbitsl(pinfo->regbase + SD_NIXEN_OFFSET, mask);
	}

}

static void ambarella_sd_disable_int(struct mmc_host *mmc, u32 mask)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	if (pinfo->slot_num > 1) {
		if (pslotinfo->slot_id == 0)
			fio_amb_sd0_set_int(mask, 0);
		else
			fio_amb_sdio0_set_int(mask, 0);
	} else {
		amba_clrbitsl(pinfo->regbase + SD_NISEN_OFFSET, mask);
		amba_clrbitsl(pinfo->regbase + SD_NIXEN_OFFSET, mask);
	}
}

static void ambarella_sd_set_iclk(struct mmc_host *mmc, u16 clk_div)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	clk_div <<= 8;
	clk_div |= SD_CLK_ICLK_EN;
	amba_writew(pinfo->regbase + SD_CLK_OFFSET, clk_div);
	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_ICLK_STABLE)
			break;
		if ((clkreg & ~SD_CLK_ICLK_STABLE) != clk_div) {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET, clk_div);
			udelay(1);
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo,
				"Wait SD_CLK_ICLK_STABLE = %d @ 0x%x\n",
				counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_clear_clken(struct mmc_host *mmc)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_EN) {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET,
				(clkreg & ~SD_CLK_EN));
			udelay(1);
		} else {
			break;
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "%s(%d @ 0x%x)\n",
				__func__, counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_set_clken(struct mmc_host *mmc)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_EN) {
			break;
		} else {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET,
				(clkreg | SD_CLK_EN));
			udelay(1);
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "%s(%d @ 0x%x)\n",
				__func__, counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_reset_all(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 nis_flag = 0;
	u32 eis_flag = 0;
	u32 counter = 0;
	u8 reset_reg;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	ambarella_sd_disable_int(mmc, 0xFFFFFFFF);
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, 0xFFFF, 0xFFFF);

	/*reset sd timing register*/
	if(pinfo->phy_type == 0 && pinfo->timing_reg) {
		amba_writel(pinfo->sbc_reg, 0x0);
		amba_writel(pinfo->timing_reg + 4, 0x0);
		amba_writel(pinfo->timing_reg, 0x04070000);
	} else if(pinfo->phy_type == 1) {
		amba_writel(pinfo->regbase + SD_DELAY_SEL_L, 0x0);
		amba_writel(pinfo->regbase + SD_DELAY_SEL_H, 0x0);
	} else if(pinfo->phy_type == 2 && pinfo->timing_reg) {
		amba_writel(pinfo->timing_reg, amba_rct_readl(pinfo->timing_reg)
			& 0x1c1f1c1f);
	}

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_ALL);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_ALL))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_ALL....\n");
			break;
		}
	}

	ambarella_sd_set_iclk(mmc, 0x0000);
	amba_writeb(pinfo->regbase + SD_TMO_OFFSET,
		CONFIG_SD_AMBARELLA_TIMEOUT_VAL);

	nis_flag = SD_NISEN_REMOVAL	|
		SD_NISEN_INSERT		|
		SD_NISEN_DMA		|
		SD_NISEN_BLOCK_GAP	|
		SD_NISEN_XFR_DONE	|
		SD_NISEN_CMD_DONE;
	eis_flag = SD_EISEN_ACMD12_ERR	|
		SD_EISEN_CURRENT_ERR	|
		SD_EISEN_DATA_BIT_ERR	|
		SD_EISEN_DATA_CRC_ERR	|
		SD_EISEN_DATA_TMOUT_ERR	|
		SD_EISEN_CMD_IDX_ERR	|
		SD_EISEN_CMD_BIT_ERR	|
		SD_EISEN_CMD_CRC_ERR	|
		SD_EISEN_CMD_TMOUT_ERR;

	if (pslotinfo->use_adma == 1)
		eis_flag |= SD_EISEN_ADMA_ERR;
	else
		eis_flag &= ~SD_EISEN_ADMA_ERR;

	ambarella_sd_enable_int(mmc, (eis_flag << 16) | nis_flag);

	pslotinfo->state = AMBA_SD_STATE_RESET;
	pinfo->reset_error = 0;

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static void ambarella_sd_reset_cmd_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 counter = 0;
	u8 reset_reg;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_CMD);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_CMD))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_CMD...\n");
			pinfo->reset_error = 1;
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static void ambarella_sd_reset_data_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 counter = 0;
	u8 reset_reg;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_DAT);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_DAT))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_DAT...\n");
			pinfo->reset_error = 1;
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static inline void ambarella_sd_data_done(
	struct ambarella_sd_mmc_info *pslotinfo, u16 nis, u16 eis)
{
	struct mmc_data *data;

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) == SD_CMD_RSP_48BUSY)) {
		if (eis) {
			pslotinfo->state = AMBA_SD_STATE_ERR;
		} else {
			pslotinfo->state = AMBA_SD_STATE_IDLE;
		}
		wake_up(&pslotinfo->wait);
		return;
	}
	if (pslotinfo->mrq == NULL) {
		ambsd_dbg(pslotinfo, "%s: mrq is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}
	if (pslotinfo->mrq->data == NULL) {
		ambsd_dbg(pslotinfo, "%s: data is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}

	data = pslotinfo->mrq->data;
	if (eis) {
		if (eis & SD_EIS_DATA_BIT_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_CRC_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_ADMA_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_TMOUT_ERR) {
			data->error = -ETIMEDOUT;
		} else {
			data->error = -EIO;
		}
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		ambsd_err(pslotinfo, "%s: CMD[%u] get eis[0x%x]\n", __func__,
			pslotinfo->mrq->cmd->opcode, eis);
#endif
		pslotinfo->state = AMBA_SD_STATE_ERR;
		wake_up(&pslotinfo->wait);
		return;
	} else {
		data->bytes_xfered = pslotinfo->dma_size;
	}

	pslotinfo->state = AMBA_SD_STATE_IDLE;
	wake_up(&pslotinfo->wait);
}

static inline void ambarella_sd_cmd_done(
	struct ambarella_sd_mmc_info *pslotinfo, u16 nis, u16 eis)
{
	struct mmc_command *cmd;
	u32 rsp0, rsp1, rsp2, rsp3;
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u16 ac12es;

	if (pslotinfo->mrq == NULL) {
		ambsd_dbg(pslotinfo, "%s: mrq is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}
	if (pslotinfo->mrq->cmd == NULL) {
		ambsd_dbg(pslotinfo, "%s: cmd is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}

	cmd = pslotinfo->mrq->cmd;
	if (eis) {
		if (eis & SD_EIS_CMD_BIT_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_CRC_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_TMOUT_ERR) {
			cmd->error = -ETIMEDOUT;
		} else if (eis & SD_EIS_ACMD12_ERR) {
			ac12es = amba_readl(pinfo->regbase + SD_AC12ES_OFFSET);
			if (ac12es & SD_AC12ES_TMOUT_ERROR) {
				cmd->error = -ETIMEDOUT;
			} else if (eis & SD_AC12ES_CRC_ERROR) {
				cmd->error = -EILSEQ;
			} else {
				cmd->error = -EIO;
			}

			if (pslotinfo->mrq->stop) {
				pslotinfo->mrq->stop->error = cmd->error;
			} else {
				ambsd_err(pslotinfo, "%s NULL stop 0x%x %u\n",
					__func__, ac12es, cmd->error);
			}
		} else {
			cmd->error = -EIO;
		}
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		ambsd_err(pslotinfo, "%s: CMD[%u] get eis[0x%x]\n", __func__,
			pslotinfo->mrq->cmd->opcode, eis);
#endif
		pslotinfo->state = AMBA_SD_STATE_ERR;
		wake_up(&pslotinfo->wait);
		return;
	}

	if (cmd->flags & MMC_RSP_136) {
		rsp0 = amba_readl(pinfo->regbase + SD_RSP0_OFFSET);
		rsp1 = amba_readl(pinfo->regbase + SD_RSP1_OFFSET);
		rsp2 = amba_readl(pinfo->regbase + SD_RSP2_OFFSET);
		rsp3 = amba_readl(pinfo->regbase + SD_RSP3_OFFSET);
		cmd->resp[0] = ((rsp3 << 8) | (rsp2 >> 24));
		cmd->resp[1] = ((rsp2 << 8) | (rsp1 >> 24));
		cmd->resp[2] = ((rsp1 << 8) | (rsp0 >> 24));
		cmd->resp[3] = (rsp0 << 8);
	} else {
		cmd->resp[0] = amba_readl(pinfo->regbase + SD_RSP0_OFFSET);
	}

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) != SD_CMD_RSP_48BUSY)) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
		wake_up(&pslotinfo->wait);
	}
}

static void ambarella_sd_set_clk(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 sd_clk, desired_clk, actual_clk, bneed_div = 1;
	u16 clk_div = 0x0000;

	ambarella_sd_clear_clken(mmc);
	if (ios->clock != 0) {
		desired_clk = ios->clock;
		if (desired_clk > mmc->f_max)
			desired_clk = mmc->f_max;

		if (desired_clk < 10000000) {
			/* Below 10Mhz, divide by sd controller */
			clk_set_rate(pinfo->clk, mmc->f_max);
		} else {
			clk_set_rate(pinfo->clk, desired_clk);
			actual_clk = clk_get_rate(pinfo->clk);
			bneed_div = 0;
		}

		if (bneed_div) {
			sd_clk = clk_get_rate(pinfo->clk);
			for (clk_div = 0x0; clk_div <= 0x80;) {
				if (clk_div == 0)
					actual_clk = sd_clk;
				else
					actual_clk = sd_clk / (clk_div << 1);

				if (actual_clk <= desired_clk)
					break;

				if (clk_div >= 0x80)
					break;

				if (clk_div == 0x0)
					clk_div = 0x1;
				else
					clk_div <<= 1;
			}
		}
		ambsd_dbg(pslotinfo, "sd_pll = %lu.\n", clk_get_rate(pinfo->clk));
		ambsd_dbg(pslotinfo, "desired_clk = %u.\n", desired_clk);
		ambsd_dbg(pslotinfo, "actual_clk = %u.\n", actual_clk);
		ambsd_dbg(pslotinfo, "clk_div = %u.\n", clk_div);
		ambarella_sd_set_iclk(mmc, clk_div);
		ambarella_sd_set_clken(mmc);
	}
}

static void ambarella_sd_set_pwr(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	if (ios->power_mode == MMC_POWER_OFF) {
		ambarella_sd_reset_all(pslotinfo->mmc);
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET, SD_PWR_OFF);

		if (gpio_is_valid(pslotinfo->pwr_gpio)) {
			gpio_set_value_cansleep(pslotinfo->pwr_gpio,
						!pslotinfo->pwr_gpio_active);
			msleep(300);
		}

		if (gpio_is_valid(pslotinfo->v18_gpio)) {
			gpio_set_value_cansleep(pslotinfo->v18_gpio,
						!pslotinfo->v18_gpio_active);
			msleep(10);
		}
	} else if (ios->power_mode == MMC_POWER_UP) {
		if (gpio_is_valid(pslotinfo->v18_gpio)) {
			gpio_set_value_cansleep(pslotinfo->v18_gpio,
						!pslotinfo->v18_gpio_active);
			msleep(10);
		}

		if (gpio_is_valid(pslotinfo->pwr_gpio)) {
			gpio_set_value_cansleep(pslotinfo->pwr_gpio,
						pslotinfo->pwr_gpio_active);
			msleep(300);
		}

		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_3_3V));
	} else if (ios->power_mode == MMC_POWER_ON) {
		switch (1 << ios->vdd) {
		case MMC_VDD_165_195:
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			break;
		default:
			ambsd_err(pslotinfo, "%s Wrong voltage[%u]!\n",
				__func__, ios->vdd);
			break;
		}
	}
	msleep(1);
	ambsd_dbg(pslotinfo, "pwr = 0x%x.\n",
		amba_readb(pinfo->regbase + SD_PWR_OFFSET));
}

static void ambarella_sd_set_phy_timing(
		struct ambarella_sd_controller_info *pinfo, int mode)
{
	u32 i, val0, val1;

	if (pinfo->phy_type < 0 || !pinfo->phy_timing || pinfo->phy_timing_num == 0)
		return;

	for (i = 0; i < pinfo->phy_timing_num; i++) {
		if (pinfo->phy_timing[i].mode & (0x1 << mode))
			break;
	}

	/* phy setting is not defined in DTS for this mode, so we use the
	 * default phy setting defined for DS mode. */
	if (i >= pinfo->phy_timing_num)
		i = 0;

	val0 = pinfo->phy_timing[i].val0;
	val1 = pinfo->phy_timing[i].val1;

	if (pinfo->phy_type == 0) {
		/*using rct sd phy*/
		amba_writel(pinfo->timing_reg, val0 | 0x02000000);
		amba_writel(pinfo->timing_reg, val0);
		amba_writel(pinfo->regbase + SD_LAT_CTRL_OFFSET, val1);
	} else if(pinfo->phy_type == 1) {
		/*using sd controller as sd phy*/
		amba_writel(pinfo->regbase + SD_DELAY_SEL_L, val0);
		amba_writel(pinfo->regbase + SD_DELAY_SEL_H, val1);
	} else {
		u32 ms_delay = amba_rct_readl(pinfo->timing_reg);
		ms_delay &= val0;
		ms_delay |= val1;
		amba_rct_writel(pinfo->timing_reg, ms_delay);
	}
}

static void ambarella_sd_set_bus(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u8 hostr = 0;

	hostr = amba_readb(pinfo->regbase + SD_HOST_OFFSET);
	if (ios->bus_width == MMC_BUS_WIDTH_8) {
		hostr |= SD_HOST_8BIT;
		hostr &= ~(SD_HOST_4BIT);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		hostr &= ~(SD_HOST_8BIT);
		hostr |= SD_HOST_4BIT;
	} else if (ios->bus_width == MMC_BUS_WIDTH_1) {
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	} else {
		ambsd_err(pslotinfo, "Unknown bus_width[%u], assume 1bit.\n",
			ios->bus_width);
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	}

	hostr &= ~SD_HOST_HIGH_SPEED;
	switch (ios->timing) {
	case MMC_TIMING_LEGACY:
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_MMC_HS200:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_SDR12:
	case MMC_TIMING_UHS_SDR25:
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_SDR104:
		amba_clrbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		amba_clrbitsw(pinfo->regbase + SD_HOST2_OFFSET, 0x0004);
		break;
	case MMC_TIMING_UHS_DDR50:
		hostr |= SD_HOST_HIGH_SPEED;
		amba_setbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		amba_setbitsw(pinfo->regbase + SD_HOST2_OFFSET, 0x0004);
		break;
	default:
		amba_clrbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		amba_clrbitsw(pinfo->regbase + SD_HOST2_OFFSET, 0x0004);
		ambsd_err(pslotinfo, "Unknown timing[%d], assume legacy.\n",
			ios->timing);
		break;
	}

	amba_writeb(pinfo->regbase + SD_HOST_OFFSET, hostr);
	ambsd_dbg(pslotinfo, "hostr = 0x%x.\n", hostr);

	if(!pinfo->auto_tuning || ios->timing == MMC_TIMING_LEGACY
		|| ios->timing == MMC_TIMING_MMC_HS || ios->timing == MMC_TIMING_SD_HS)
		ambarella_sd_set_phy_timing(pinfo, ios->timing);

}

static void ambarella_sd_check_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	if ((pinfo->controller_ios.power_mode != ios->power_mode) ||
		(pinfo->controller_ios.vdd != ios->vdd) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_pwr(mmc, ios);
		pinfo->controller_ios.power_mode = ios->power_mode;
		pinfo->controller_ios.vdd = ios->vdd;
	}

	if ((pinfo->controller_ios.clock != ios->clock) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_clk(mmc, ios);
		pinfo->controller_ios.clock = ios->clock;
	}

	if ((pinfo->controller_ios.bus_width != ios->bus_width) ||
		(pinfo->controller_ios.timing != ios->timing) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_bus(mmc, ios);
		pinfo->controller_ios.bus_width = ios->bus_width;
		pinfo->controller_ios.timing = ios->timing;
	}

	if (pslotinfo->state == AMBA_SD_STATE_RESET) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
	}
}

static void ambarella_sd_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	ambarella_sd_request_bus(mmc);
	ambarella_sd_check_ios(mmc, ios);
	ambarella_sd_release_bus(mmc);
}

static void ambarella_sd_recovery(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 latency = 0, sbc_reg = 0, timing_reg0 = 0, timing_reg1 = 0;
	u32 sd_delay_sel0 = 0, sd_delay_sel1 = 0, divisor = 0;

	/*save the sd timing register*/
	if(pinfo->phy_type == 0 && pinfo->timing_reg) {
		latency = amba_readl(pinfo->regbase + SD_LAT_CTRL_OFFSET);
		timing_reg0 = amba_readl(pinfo->timing_reg);
		timing_reg1 = amba_readl(pinfo->timing_reg + 4);
		sbc_reg = amba_readl(pinfo->sbc_reg);
	} else if(pinfo->phy_type == 1) {
		sd_delay_sel0 = amba_readl(pinfo->regbase + SD_DELAY_SEL_L);
		sd_delay_sel1 = amba_readl(pinfo->regbase + SD_DELAY_SEL_H);
	} else if(pinfo->phy_type == 2 && pinfo->timing_reg) {
		timing_reg0 = amba_readl(pinfo->timing_reg);
	}

	divisor = (amba_readw(pinfo->regbase + SD_CLK_OFFSET) & 0xff00) >> 8;

	ambarella_sd_reset_all(mmc);

	/*restore clk*/
	ambarella_sd_clear_clken(mmc);
	ambarella_sd_set_iclk(mmc, divisor);
	ambarella_sd_set_clken(mmc);
	ambarella_sd_set_bus(mmc, &pinfo->controller_ios);

	/*restore the sd timing register*/
	if(pinfo->phy_type == 0 && pinfo->timing_reg) {
		amba_writel(pinfo->regbase + SD_LAT_CTRL_OFFSET, latency);
		amba_writel(pinfo->timing_reg, timing_reg0);
		amba_writel(pinfo->timing_reg + 4, timing_reg1);
		amba_writel(pinfo->sbc_reg, sbc_reg);
	} else if(pinfo->phy_type == 1) {
		amba_writel(pinfo->regbase + SD_DELAY_SEL_L, sd_delay_sel0);
		amba_writel(pinfo->regbase + SD_DELAY_SEL_H, sd_delay_sel1);
	} else if(pinfo->phy_type == 2 && pinfo->timing_reg) {
		amba_writel(pinfo->timing_reg, timing_reg0);
	}

}

static u32 ambarella_sd_check_cd(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	int cdpin;

	if (pslotinfo->fixed_cd != -1) {
		cdpin = !!pslotinfo->fixed_cd;
	} else {
		cdpin = mmc_gpio_get_cd(mmc);
		if (cdpin < 0) {
			cdpin = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			cdpin &= SD_STA_CARD_INSERTED;
		}
	}

	return !!cdpin;
}

static inline void ambarella_sd_prepare_tmo(
	struct ambarella_sd_mmc_info *pslotinfo,
	struct mmc_data *pmmcdata)
{
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
	pslotinfo->wait_tmo = min_t(u32, pinfo->default_wait_tmo, CONFIG_SD_AMBARELLA_MAX_TIMEOUT);
	pslotinfo->sta_counter = pinfo->default_wait_tmo / CONFIG_SD_AMBARELLA_MAX_TIMEOUT + 1;

	ambsd_dbg(pslotinfo, "timeout_ns = %u, timeout_clks = %u, "
		"wait_tmo = %u, tmo = %u, sta_counter = %u.\n",
		pmmcdata->timeout_ns, pmmcdata->timeout_clks,
		pslotinfo->wait_tmo, pslotinfo->tmo, pslotinfo->sta_counter);
}

static inline void ambarella_sd_pre_cmd(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	pslotinfo->state = AMBA_SD_STATE_CMD;
	pslotinfo->sg_len = 0;
	pslotinfo->sg = NULL;
	pslotinfo->blk_sz = 0;
	pslotinfo->blk_cnt = 0;
	pslotinfo->arg_reg = 0;
	pslotinfo->cmd_reg = 0;
	pslotinfo->sta_reg = 0;
	pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
	pslotinfo->wait_tmo = (1 * HZ);
	pslotinfo->xfr_reg = 0;
	pslotinfo->dma_address = 0;
	pslotinfo->dma_size = 0;

	if (pslotinfo->mrq->stop) {
		if (likely(pslotinfo->mrq->stop->opcode ==
			MMC_STOP_TRANSMISSION)) {
			pslotinfo->xfr_reg = SD_XFR_AC12_EN;
		} else {
			ambsd_err(pslotinfo, "%s strange stop cmd%u\n",
				__func__, pslotinfo->mrq->stop->opcode);
		}
	}

	if (!(pslotinfo->mrq->cmd->flags & MMC_RSP_PRESENT))
		pslotinfo->cmd_reg = SD_CMD_RSP_NONE;
	else if (pslotinfo->mrq->cmd->flags & MMC_RSP_136)
		pslotinfo->cmd_reg = SD_CMD_RSP_136;
	else if (pslotinfo->mrq->cmd->flags & MMC_RSP_BUSY)
		pslotinfo->cmd_reg = SD_CMD_RSP_48BUSY;
	else
		pslotinfo->cmd_reg = SD_CMD_RSP_48;
	if (pslotinfo->mrq->cmd->flags & MMC_RSP_CRC)
		pslotinfo->cmd_reg |= SD_CMD_CHKCRC;
	if (pslotinfo->mrq->cmd->flags & MMC_RSP_OPCODE)
		pslotinfo->cmd_reg |= SD_CMD_CHKIDX;
	pslotinfo->cmd_reg |= SD_CMD_IDX(pslotinfo->mrq->cmd->opcode);
	pslotinfo->arg_reg = pslotinfo->mrq->cmd->arg;

	if (pslotinfo->mrq->data) {
		pslotinfo->state = AMBA_SD_STATE_DATA;
		ambarella_sd_prepare_tmo(pslotinfo, pslotinfo->mrq->data);
		pslotinfo->blk_sz = (pslotinfo->mrq->data->blksz & 0xFFF);
		pslotinfo->dma_size = pslotinfo->mrq->data->blksz *
			pslotinfo->mrq->data->blocks;
		pslotinfo->sg_len = pslotinfo->mrq->data->sg_len;
		pslotinfo->sg = pslotinfo->mrq->data->sg;
		pslotinfo->xfr_reg |= SD_XFR_DMA_EN;
		pslotinfo->cmd_reg |= SD_CMD_DATA;
		pslotinfo->blk_cnt = pslotinfo->mrq->data->blocks;
		if (pslotinfo->blk_cnt > 1) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg |= SD_XFR_BLKCNT_EN;
		}
		if (pslotinfo->mrq->data->flags & MMC_DATA_STREAM) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg &= ~SD_XFR_BLKCNT_EN;
		}
		if (pslotinfo->mrq->data->flags & MMC_DATA_WRITE) {
			pslotinfo->xfr_reg &= ~SD_XFR_CTH_SEL;
			pslotinfo->sta_reg = (SD_STA_WRITE_XFR_ACTIVE |
				SD_STA_DAT_ACTIVE);
			if (pslotinfo->use_adma == 1) {
				pslotinfo->pre_dma = &ambarella_sd_pre_sg_to_adma;
				pslotinfo->post_dma = &ambarella_sd_post_sg_to_adma;
			} else {
				pslotinfo->pre_dma = &ambarella_sd_pre_sg_to_dma;
				pslotinfo->post_dma = &ambarella_sd_post_sg_to_dma;
			}
		} else {
			pslotinfo->xfr_reg |= SD_XFR_CTH_SEL;
			pslotinfo->sta_reg = (SD_STA_READ_XFR_ACTIVE |
				SD_STA_DAT_ACTIVE);
			if (pslotinfo->use_adma == 1) {
				pslotinfo->pre_dma = &ambarella_sd_pre_adma_to_sg;
				pslotinfo->post_dma = &ambarella_sd_post_adma_to_sg;
			} else {
				pslotinfo->pre_dma = &ambarella_sd_pre_dma_to_sg;
				pslotinfo->post_dma = &ambarella_sd_post_dma_to_sg;
			}
		}
		pslotinfo->pre_dma(pslotinfo);
		if (pinfo->dma_fix) {
			pslotinfo->dma_address |= pinfo->dma_fix;
		}
	}
}

static inline void ambarella_sd_send_cmd(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 valid_request, sta_reg, tmpreg, counter = 0;
	long timeout;

	ambarella_sd_request_bus(pslotinfo->mmc);

	valid_request = ambarella_sd_check_cd(pslotinfo->mmc);
	ambsd_dbg(pslotinfo, "cmd = %u valid_request = %u.\n",
		pslotinfo->mrq->cmd->opcode, valid_request);
	if (!valid_request) {
		pslotinfo->mrq->cmd->error = -ENOMEDIUM;
		pslotinfo->state = AMBA_SD_STATE_ERR;
		goto ambarella_sd_send_cmd_exit;
	}

	ambarella_sd_check_ios(pslotinfo->mmc, &pslotinfo->mmc->ios);
	if (pslotinfo->mrq->data) {
		while (1) {
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((sta_reg & SD_STA_CMD_INHIBIT_DAT) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				if(pslotinfo->mrq->cmd->opcode != MMC_SEND_TUNING_BLOCK)
					ambsd_warn(pslotinfo,
						"Wait SD_STA_CMD_INHIBIT_DAT...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				pslotinfo->mrq->data->error = -EILSEQ;
				pinfo->reset_error = 1;
				goto ambarella_sd_send_cmd_exit;
			}
		}

		amba_writeb(pinfo->regbase + SD_TMO_OFFSET, pslotinfo->tmo);
		if (pslotinfo->use_adma == 1) {
			amba_setbitsb(pinfo->regbase + SD_HOST_OFFSET,
				SD_HOST_ADMA);
			amba_writel(pinfo->regbase + SD_ADMA_ADDR_OFFSET,
				pslotinfo->dma_address);
		} else {
			amba_clrbitsb(pinfo->regbase + SD_HOST_OFFSET,
				SD_HOST_ADMA);
			amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
				pslotinfo->dma_address);
		}
		amba_write2w(pinfo->regbase + SD_BLK_SZ_OFFSET,
			pslotinfo->blk_sz, pslotinfo->blk_cnt);
		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			pslotinfo->xfr_reg, pslotinfo->cmd_reg);
	} else {
		while (1) {
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((sta_reg & SD_STA_CMD_INHIBIT_CMD) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_STA_CMD_INHIBIT_CMD...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				pslotinfo->mrq->cmd->error = -EILSEQ;
				pinfo->reset_error = 1;
				goto ambarella_sd_send_cmd_exit;
			}
		}

		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			0x00, pslotinfo->cmd_reg);
	}

ambarella_sd_send_cmd_exit:
	if (pslotinfo->state == AMBA_SD_STATE_CMD) {
		timeout = wait_event_timeout(pslotinfo->wait,
			(pslotinfo->state != AMBA_SD_STATE_CMD),
			pslotinfo->wait_tmo);
		if (pslotinfo->state == AMBA_SD_STATE_CMD) {
			ambsd_err(pslotinfo,
				"cmd%u %u@[%ld:%u], sta=0x%04x\n",
				pslotinfo->mrq->cmd->opcode,
				pslotinfo->state, timeout, pslotinfo->wait_tmo,
				amba_readl(pinfo->regbase + SD_STA_OFFSET));
			pslotinfo->mrq->cmd->error = -ETIMEDOUT;
		}
	} else if (pslotinfo->state == AMBA_SD_STATE_DATA) {
		do {
			timeout = wait_event_timeout(pslotinfo->wait,
				(pslotinfo->state != AMBA_SD_STATE_DATA),
				pslotinfo->wait_tmo);
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((pslotinfo->state == AMBA_SD_STATE_DATA) &&
				(sta_reg & pslotinfo->sta_reg)) {
				ambsd_rtdbg(pslotinfo, "data%u %u@"
					"[%ld:%u:%u:%u], sta=0x%04x:0x%04x\n",
					pslotinfo->mrq->cmd->opcode,
					pslotinfo->state, timeout,
					pslotinfo->wait_tmo,
					pslotinfo->mrq->data->timeout_ns,
					pslotinfo->mrq->data->timeout_clks,
					sta_reg, pslotinfo->sta_reg);
				ambsd_rtdbg(pslotinfo,
					"DMA %u in %u sg [0x%08x:0x%08x]\n",
					pslotinfo->dma_size,
					pslotinfo->sg_len,
					pslotinfo->dma_address,
					pslotinfo->dma_size);
				tmpreg = amba_readw(pinfo->regbase +
					SD_BLK_CNT_OFFSET);
				if (tmpreg) {
					ambsd_rtdbg(pslotinfo,
						"SD_DMA_ADDR_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_DMA_ADDR_OFFSET));
					amba_writel((pinfo->regbase +
						SD_DMA_ADDR_OFFSET),
						amba_readl(pinfo->regbase +
						SD_DMA_ADDR_OFFSET));
				} else {
					ambsd_rtdbg(pslotinfo,
						"SD_DATA_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_DATA_OFFSET));
					ambsd_rtdbg(pslotinfo,
						"SD_STA_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_STA_OFFSET));
				}
			} else {
				break;
			}
		} while (pslotinfo->sta_counter--);
		if (pslotinfo->state == AMBA_SD_STATE_DATA) {
			ambsd_err(pslotinfo,
				"data%u %u@%u, sta=0x%04x:0x%04x\n",
				pslotinfo->mrq->cmd->opcode,
				pslotinfo->state,
				pslotinfo->wait_tmo,
				sta_reg, pslotinfo->sta_reg);
			pslotinfo->mrq->data->error = -ETIMEDOUT;
		}
	}
}

static inline void ambarella_sd_post_cmd(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	if (pslotinfo->state == AMBA_SD_STATE_IDLE) {
		ambarella_sd_release_bus(pslotinfo->mmc);
		if (pslotinfo->mrq->data) {
			pslotinfo->post_dma(pslotinfo);
		}
	} else {
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		u32 counter = 0;

		ambsd_err(pslotinfo, "CMD%u retries[%u] state[%u].\n",
			pslotinfo->mrq->cmd->opcode,
			pslotinfo->mrq->cmd->retries,
			pslotinfo->state);
		for (counter = 0; counter < 0x100; counter += 4) {
			ambsd_err(pslotinfo, "0x%04x: 0x%08x\n",
			counter, amba_readl(pinfo->regbase + counter));
		}
		ambarella_sd_show_info(pslotinfo);
#endif
		if (pinfo->reset_error) {
			ambarella_sd_recovery(pslotinfo->mmc);
			//ambarella_sd_reset_all(pslotinfo->mmc);
		}
		ambarella_sd_release_bus(pslotinfo->mmc);
	}
}

static void ambarella_sd_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	pslotinfo->mrq = mrq;
	ambarella_sd_pre_cmd(pslotinfo);
	ambarella_sd_send_cmd(pslotinfo);
	ambarella_sd_post_cmd(pslotinfo);
	pslotinfo->mrq = NULL;
	mmc_request_done(mmc, mrq);
}

static int ambarella_sd_get_ro(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	int wpspl;

	ambarella_sd_request_bus(mmc);

	if (pslotinfo->fixed_wp != -1) {
		wpspl = !!pslotinfo->fixed_wp;
	} else {
		wpspl = mmc_gpio_get_ro(mmc);
		if (wpspl < 0) {
			wpspl = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			wpspl &= SD_STA_WPS_PL;
		}
	}

	ambarella_sd_release_bus(mmc);

	ambsd_dbg(pslotinfo, "RO[%u].\n", wpspl);
	return !!wpspl;
}

static int ambarella_sd_get_cd(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	u32 cdpin;

	ambarella_sd_request_bus(mmc);
	cdpin = ambarella_sd_check_cd(mmc);
	ambarella_sd_release_bus(mmc);

	ambsd_dbg(pslotinfo, "CD[%u].\n", cdpin);
	return cdpin ? 1 : 0;
}

static void ambarella_sd_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	if (enable)
		ambarella_sd_enable_int(mmc, SD_NISEN_CARD);
	else
		ambarella_sd_disable_int(mmc, SD_NISEN_CARD);
}

static int ambarella_sd_ssvs(struct mmc_host *mmc, struct mmc_ios *ios)
{
	int retval = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);
	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_3_3V));

		if (gpio_is_valid(pslotinfo->v18_gpio)) {
			gpio_set_value_cansleep(pslotinfo->v18_gpio,
						!pslotinfo->v18_gpio_active);
			msleep(10);
		}
	} else if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {
		ambarella_sd_clear_clken(mmc);
		msleep(CONFIG_SD_AMBARELLA_VSW_PRE_SPEC);
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_1_8V));

		if (gpio_is_valid(pslotinfo->v18_gpio)) {
			gpio_set_value_cansleep(pslotinfo->v18_gpio,
						pslotinfo->v18_gpio_active);
			msleep(pinfo->switch_voltage_tmo);
		}
		ambarella_sd_set_clken(mmc);
	}
	ambarella_sd_release_bus(mmc);

	return retval;
}

static int ambarella_sd_card_busy(struct mmc_host *mmc)
{
	int retval = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 sta_reg;

	ambarella_sd_request_bus(mmc);
	sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
	ambsd_dbg(pslotinfo, "SD_STA_OFFSET = 0x%08X.\n", sta_reg);
	retval = !(sta_reg & 0x1F00000);
	ambarella_sd_release_bus(mmc);

	return retval;
}

static int ambarella_send_tuning_cmd(struct mmc_host *host, u32 opcode, int *cmd_error)
{
	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct scatterlist sg;
	struct mmc_ios *ios = &host->ios;
	const u8 *tuning_block_pattern;
	int size, err = 0;
	u8 *data_buf;

	if (ios->bus_width == MMC_BUS_WIDTH_8) {
			tuning_block_pattern = tuning_blk_pattern_8bit;
			size = sizeof(tuning_blk_pattern_8bit);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
			tuning_block_pattern = tuning_blk_pattern_4bit;
			size = sizeof(tuning_blk_pattern_4bit);
	} else
			return -EINVAL;

	data_buf = kzalloc(size, GFP_KERNEL);
	if (!data_buf)
			return -ENOMEM;

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = opcode;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = size;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	/*
	 * According to the tuning specs, Tuning process
	 * is normally shorter 40 executions of CMD19,
	 * and timeout value should be shorter than 150 ms
	 */
	data.timeout_ns = 150 * NSEC_PER_MSEC;

	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, data_buf, size);

	mmc_wait_for_req(host, &mrq);

	if (cmd_error)
		*cmd_error = cmd.error;

	if (cmd.error) {
		err = cmd.error;
		goto out;
	}

	if (data.error) {
		err = data.error;
		goto out;
	}

	if (memcmp(data_buf, tuning_block_pattern, size))
		err = -EIO;

out:
	kfree(data_buf);
	return err;

}

static int ambarella_sd_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	u32 tmp, misc, lat, s = -1, e = 0, middle;
	u32 best_misc = 0, best_s = -1, best_e = 0, doing_retune = 0;
	int dly, longest_range = 0, range = 0;
	u32 clock = pinfo->controller_ios.clock;

	if(!pinfo->auto_tuning || !pinfo->timing_reg)
		return 0;

retry:
	if (doing_retune) {
		clock -= 10000000;
		if (clock <= 50000000) {
			ambsd_tuning_dbg(pslotinfo,
				"tuning: can't find any valid timing\n");
			return -ECANCELED;
		}

		pinfo->controller_ios.clock = clock;
		ambarella_sd_set_clk(mmc, &pinfo->controller_ios);
	}

	if(pinfo->phy_type == 0)
		goto phy_type_0;
	else if (pinfo->phy_type == 1)
		goto phy_type_1;
	else if (pinfo->phy_type == 2)
		goto phy_type_2;

phy_type_0:
	for (misc = 0; misc < 4; misc++) {
		writel_relaxed(0x8001, pinfo->sbc_reg);

		tmp = readl_relaxed(pinfo->timing_reg);
		tmp &= 0x0000ffff;
		tmp |= ((misc >> 1) & 0x1) << 19;
		writel_relaxed(tmp | (1 << 25), pinfo->timing_reg);
		usleep_range(5, 10);
		writel_relaxed(tmp, pinfo->timing_reg);
		usleep_range(5, 10);
		lat = ((misc >> 0) & 0x1) + 1;
		tmp = (lat << 12) | (lat << 8) | (lat << 4) | (lat << 0);
		writel_relaxed(tmp, pinfo->regbase + SD_LAT_CTRL_OFFSET);

		for (dly = 0; dly < 4; dly++) {
			tmp = readl_relaxed(pinfo->sbc_reg);
			tmp &= 0xfffffff9;
//			tmp |= (((dly >> 6) & 0x3) << 1);
			tmp |= dly;
			writel_relaxed(tmp, pinfo->sbc_reg);
/*
			sel = dly % 64;
			if (sel < 0x20)
				sel = 63 - sel;
			else
				sel = sel - 32;

			tmp = (sel << 16) | (sel << 8) | (sel << 0);
			writel_relaxed(tmp, pinfo->timing_reg + 4);
*/
			if (ambarella_send_tuning_cmd(mmc, opcode, NULL) == 0) {
				/* Tuning is successful at this tuning point */
				if (s == -1)
					s = dly;
				e = dly;
				range++;
			} else {
				if (range > 0) {
					ambsd_tuning_dbg(pslotinfo,
						"tuning: misc[0x%x], count[%d](%d - %d)\n",
						misc, e - s + 1, s, e);
				}

				if (range > longest_range) {
					best_misc = misc;
					best_s = s;
					best_e = e;
					longest_range = range;
				}
				s = -1;
				e = range = 0;
			}
		}

		/* in case the last timings are all working */
		if (range > longest_range) {
			if (range > 0) {
				ambsd_tuning_dbg(pslotinfo,
					"tuning last: misc[0x%x], count[%d](%d - %d)\n",
					misc, e - s + 1, s, e);
			}
			best_misc = misc;
			best_s = s;
			best_e = e;
			longest_range = range;
		}
		s = -1;
		e = range = 0;
	}

	if (longest_range == 0) {
		if (clock > 50000000) {
			doing_retune = 1;
			goto retry;
		}

		return -EIO;
	}

	middle = (best_s + best_e) / 2;

//	tmp = (((middle >> 6) & 0x3) << 1) | 0x8001;
	tmp = (middle << 1) | 0x8001;
	writel_relaxed(tmp, pinfo->sbc_reg);
/*
	sel = middle % 64;
	if (sel < 0x20)
			sel = 63 - sel;
	else
			sel = sel - 32;

	tmp = (sel << 16) | (sel << 8) | (sel << 0);
	writel_relaxed(tmp, pinfo->timing_reg + 4);
*/
	tmp = readl_relaxed(pinfo->timing_reg);
	tmp &= 0x0000ffff;
	tmp |= ((best_misc >> 1) & 0x1) << 19;
	writel_relaxed(tmp | (1 << 25), pinfo->timing_reg);
	usleep_range(5, 10);
	writel_relaxed(tmp, pinfo->timing_reg);
	usleep_range(5, 10);

	lat = ((best_misc >> 0) & 0x1) + 1;
	tmp = (lat << 12) | (lat << 8) | (lat << 4) | (lat << 0);
	writel_relaxed(tmp, pinfo->regbase + SD_LAT_CTRL_OFFSET);

	ambarella_sd_recovery(mmc);

	return 0;

phy_type_1:

	/* misc: 3 bits data out delay
	 * dly: 2 bits clk mode and 3 bits clk out delay, total 5 bits;
	 */

	for(misc = 0; misc < 8; misc++) {
		for(dly = 0; dly < 32; dly++) {
			tmp = (misc << 9) | (misc << 15) | (misc << 21) | (misc << 27);
			amba_writel(pinfo->regbase + SD_DELAY_SEL_L, tmp);
			tmp = ((dly >> 3) << 25) | ((dly & 0x07) << 22) | (misc << 1)
				| (misc << 7) | (misc << 13) | (misc << 19);
			amba_writel(pinfo->regbase + SD_DELAY_SEL_H, tmp);

			if (ambarella_send_tuning_cmd(mmc, opcode, NULL) == 0) {
				/* Tuning is successful at this tuning point */
				if (s == -1)
					s = dly;
				e = dly;
				range++;
			} else {
				if (range > 0) {
					ambsd_tuning_dbg(pslotinfo,
						"tuning last: misc[0x%x], count[%d](%d - %d)\n",
						misc, e - s + 1, s, e);
				}

				if (range > longest_range) {
					best_misc = misc;
					best_s = s;
					best_e = e;
					longest_range = range;
				}
				s = -1;
				e = range = 0;
			}
		}

		/* in case the last timings are all working */
		if (range > longest_range) {
			if (range > 0) {
				ambsd_tuning_dbg(pslotinfo,
					"tuning last: misc[0x%x], count[%d](%d - %d)\n",
					misc, e - s + 1, s, e);
			}
			best_misc = misc;
			best_s = s;
			best_e = e;
			longest_range = range;
		}
		s = -1;
		e = range = 0;
	}

	if (longest_range == 0) {
		if (clock > 50000000) {
			doing_retune = 1;
			goto retry;
		}

		return -EIO;
	}

	middle = (best_s + best_e) / 2;

	tmp = (best_misc << 9) | (best_misc << 15) | (best_misc << 21) | (best_misc << 27);
	amba_writel(pinfo->regbase + SD_DELAY_SEL_L, tmp);
	tmp = ((middle >> 3) << 25) | ((middle & 0x07) << 22) | (best_misc << 1)
		| (best_misc << 7) | (best_misc << 13) | (best_misc << 19);
	amba_writel(pinfo->regbase + SD_DELAY_SEL_H, tmp);

	ambarella_sd_recovery(mmc);

	return 0;

phy_type_2:

	/* misc: 5 bits out clock delay
	 * dly: 2 bits out data delay and 3 bits input data delay, total 5 bits;
	 */

	/*This is only used for s2e sdio slot*/
	lat = amba_readl(pinfo->timing_reg);
	lat &= ~0xC3E0E000;

	for(misc = 0; misc < 32; misc++) {
		for(dly = 0; dly < 32; dly++) {
			tmp = lat | ((dly >> 3) << 30) | ((dly & 0x07) << 13) | (misc << 21);
			amba_writel(pinfo->timing_reg, tmp);
			if (ambarella_send_tuning_cmd(mmc, opcode, NULL) == 0) {
				/* Tuning is successful at this tuning point */
				if (s == -1)
					s = dly;
				e = dly;
				range++;
			} else {
				if (range > 0) {
					ambsd_tuning_dbg(pslotinfo,
						"tuning last: misc[0x%x], count[%d](%d - %d)\n",
						misc, e - s + 1, s, e);
				}

				if (range > longest_range) {
					best_misc = misc;
					best_s = s;
					best_e = e;
					longest_range = range;
				}
				s = -1;
				e = range = 0;
			}
		}

		/* in case the last timings are all working */
		if (range > longest_range) {
			if (range > 0) {
				ambsd_tuning_dbg(pslotinfo,
					"tuning last: misc[0x%x], count[%d](%d - %d)\n",
					misc, e - s + 1, s, e);
			}
			best_misc = misc;
			best_s = s;
			best_e = e;
			longest_range = range;
		}
		s = -1;
		e = range = 0;
	}

	if (longest_range == 0) {
		if (clock > 50000000) {
			doing_retune = 1;
			goto retry;
		}

		return -EIO;
	}

	middle = (best_s + best_e) / 2;
	tmp = lat | ((middle >> 3) << 30) | ((middle & 0x07) << 13) | (best_misc << 21);
	amba_writel(pinfo->timing_reg, tmp);

	ambarella_sd_recovery(mmc);

	return 0;
}

static const struct mmc_host_ops ambarella_sd_host_ops = {
	.request = ambarella_sd_request,
	.set_ios = ambarella_sd_ios,
	.get_ro = ambarella_sd_get_ro,
	.get_cd = ambarella_sd_get_cd,
	.enable_sdio_irq = ambarella_sd_enable_sdio_irq,
	.start_signal_voltage_switch = ambarella_sd_ssvs,
	.card_busy = ambarella_sd_card_busy,
	.execute_tuning =  ambarella_sd_execute_tuning,
};

static int ambarella_sd_system_event(struct notifier_block *nb,
	unsigned long val, void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = container_of(nb, struct ambarella_sd_mmc_info, system_event);
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	switch (val) {
	case AMBA_EVENT_PRE_CPUFREQ:
		pr_debug("%s[%u]: Pre Change\n", __func__, pslotinfo->slot_id);
		down(&pslotinfo->system_event_sem);
		break;

	case AMBA_EVENT_POST_CPUFREQ:
		pr_debug("%s[%u]: Post Change\n", __func__, pslotinfo->slot_id);
		ambarella_sd_set_clk(pslotinfo->mmc, &pinfo->controller_ios);
		up(&pslotinfo->system_event_sem);
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static irqreturn_t ambarella_sd_irq(int irq, void *devid)
{
	struct ambarella_sd_controller_info *pinfo = devid;
	struct ambarella_sd_mmc_info *pslotinfo = NULL;
	u16 nis, eis, slot_id;

	/* Read and clear the interrupt registers */
	amba_read2w(pinfo->regbase + SD_NIS_OFFSET, &nis, &eis);
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, nis, eis);

	if (pinfo->slot_num > 1) {
		u32 fio_ctrl = amba_readl(pinfo->fio_reg + FIO_CTR_OFFSET);
		slot_id = (fio_ctrl & FIO_CTR_XD) ? 1 : 0;
	} else
		slot_id = 0;

	pslotinfo = pinfo->pslotinfo[slot_id];

	ambsd_dbg(pslotinfo, "%s nis = 0x%x, eis = 0x%x & %u\n",
				__func__, nis, eis, pslotinfo->state);

	if (nis & SD_NIS_CARD) {
		ambsd_dbg(pslotinfo, "SD_NIS_CARD\n");
		mmc_signal_sdio_irq(pslotinfo->mmc);
	}

	if(pslotinfo->fixed_cd == -1) {
		if (nis & SD_NIS_REMOVAL) {
			ambsd_dbg(pslotinfo, "SD_NIS_REMOVAL\n");
			mmc_detect_change(pslotinfo->mmc, msecs_to_jiffies(1000));
		} else if (nis & SD_NIS_INSERT) {
			ambsd_dbg(pslotinfo, "SD_NIS_INSERT\n");
			mmc_detect_change(pslotinfo->mmc, msecs_to_jiffies(1000));
		}
	}

	if (eis) {
		if (eis & (SD_EIS_CMD_TMOUT_ERR | SD_EIS_CMD_CRC_ERR |
			SD_EIS_CMD_BIT_ERR | SD_EIS_CMD_IDX_ERR |
			SD_EIS_ACMD12_ERR)) {
			ambarella_sd_reset_cmd_line(pslotinfo->mmc);
		}
		if (eis & (SD_EIS_DATA_TMOUT_ERR | SD_EIS_DATA_CRC_ERR)) {
			ambarella_sd_reset_data_line(pslotinfo->mmc);
		}
		if (eis & (SD_EIS_DATA_BIT_ERR | SD_EIS_CURRENT_ERR)) {
			//ambarella_sd_reset_all(pslotinfo->mmc);
			ambarella_sd_recovery(pslotinfo->mmc);
		}
		if (pslotinfo->state == AMBA_SD_STATE_CMD) {
			ambarella_sd_cmd_done(pslotinfo, nis, eis);
		} else if (pslotinfo->state == AMBA_SD_STATE_DATA) {
			ambarella_sd_data_done(pslotinfo, nis, eis);
		}
	} else {
		if (nis & SD_NIS_CMD_DONE) {
			ambarella_sd_cmd_done(pslotinfo, nis, eis);
		}
		if (nis & SD_NIS_XFR_DONE) {
			ambarella_sd_data_done(pslotinfo, nis, eis);
		}
#if 0
		if (nis & SD_NIS_DMA) {
			amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
			amba_readl(pinfo->regbase + SD_DMA_ADDR_OFFSET));
		}
#endif
	}

	return IRQ_HANDLED;
}


/* ==========================================================================*/

static int ambarella_sd_init_slot(struct device_node *np, int id,
			struct ambarella_sd_controller_info *pinfo)
{
	struct device_node *save_np;
	struct ambarella_sd_mmc_info *pslotinfo = NULL;
	struct mmc_host *mmc;
	enum of_gpio_flags flags;
	u32 gpio_init_flag, hc_cap, hc_timeout_clk;
	int global_id, retval = 0;

	mmc = mmc_alloc_host(sizeof(*pslotinfo), pinfo->dev);
	if (!mmc) {
		dev_err(pinfo->dev, "Failed to alloc mmc host %u!\n", id);
		return -ENOMEM;
	}
	pslotinfo = mmc_priv(mmc);
	pslotinfo->mmc = mmc;

	/* in order to use mmc_of_parse(), we reassign the parent
	 * of_node, this should be a workaroud. */
	save_np = mmc->parent->of_node;
	mmc->parent->of_node = np;
	mmc_of_parse(mmc);
	mmc->parent->of_node = save_np;

	/* our own extra property */
	if (of_property_read_u32(np, "amb,fixed-wp", &pslotinfo->fixed_wp) < 0)
		pslotinfo->fixed_wp = -1;
	if (of_property_read_u32(np, "amb,fixed-cd", &pslotinfo->fixed_cd) < 0)
		pslotinfo->fixed_cd = -1;
	pslotinfo->no_1_8_v = !!of_find_property(np, "no-1-8-v", NULL);
	pslotinfo->caps_ddr = !!of_find_property(np, "amb,caps-ddr", NULL);
	pslotinfo->caps_adma = !!of_find_property(np, "amb,caps-adma", NULL);
	pslotinfo->force_gpio = !!of_find_property(np, "amb,force-gpio", NULL);
	if (pslotinfo->force_gpio) {
		pslotinfo->pinctrl = devm_pinctrl_get(pinfo->dev);
		if (IS_ERR(pslotinfo->pinctrl)) {
			retval = PTR_ERR(pslotinfo->pinctrl);
			dev_err(pinfo->dev, "Can't get pinctrl: %d\n", retval);
			goto init_slot_err1;
		}

		pslotinfo->state_work =
			pinctrl_lookup_state(pslotinfo->pinctrl, "work");
		if (IS_ERR(pslotinfo->state_work)) {
			retval = PTR_ERR(pslotinfo->state_work);
			dev_err(pinfo->dev, "Can't get pinctrl state: work\n");
			goto init_slot_err1;
		}

		pslotinfo->state_idle =
			pinctrl_lookup_state(pslotinfo->pinctrl, "idle");
		if (IS_ERR(pslotinfo->state_idle)) {
			retval = PTR_ERR(pslotinfo->state_idle);
			dev_err(pinfo->dev, "Can't get pinctrl state: idle\n");
			goto init_slot_err1;
		}
	}

	/* request gpio for external power control */
	pslotinfo->pwr_gpio = of_get_named_gpio_flags(np, "pwr-gpios", 0, &flags);
	pslotinfo->pwr_gpio_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	if (gpio_is_valid(pslotinfo->pwr_gpio)) {
		if (pslotinfo->pwr_gpio_active)
			gpio_init_flag = GPIOF_OUT_INIT_LOW;
		else
			gpio_init_flag = GPIOF_OUT_INIT_HIGH;

		retval = devm_gpio_request_one(pinfo->dev, pslotinfo->pwr_gpio,
					gpio_init_flag, "sd ext power");
		if (retval < 0) {
			dev_err(pinfo->dev, "Failed to request pwr-gpios!\n");
			goto init_slot_err1;
		}
	}

	/* request gpio for 3.3v/1.8v switch */
	pslotinfo->v18_gpio = of_get_named_gpio_flags(np, "v18-gpios", 0, &flags);
	pslotinfo->v18_gpio_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	if (gpio_is_valid(pslotinfo->v18_gpio)) {
		if (pslotinfo->v18_gpio_active)
			gpio_init_flag = GPIOF_OUT_INIT_LOW;
		else
			gpio_init_flag = GPIOF_OUT_INIT_HIGH;

		retval = devm_gpio_request_one(pinfo->dev, pslotinfo->v18_gpio,
				gpio_init_flag, "sd ext power");
		if (retval < 0) {
			dev_err(pinfo->dev, "Failed to request v18-gpios!\n");
			goto init_slot_err1;
		}
	}

	mmc->f_min = mmc->f_max >> 8;
	mmc->ops = &ambarella_sd_host_ops;

	init_waitqueue_head(&pslotinfo->wait);
	pslotinfo->state = AMBA_SD_STATE_ERR;
	pslotinfo->slot_id = id;
	pslotinfo->pinfo = pinfo;
	pinfo->pslotinfo[id] = pslotinfo;
	sema_init(&pslotinfo->system_event_sem, 1);

	ambarella_sd_request_bus(mmc);

	ambarella_sd_reset_all(mmc);

	hc_cap = amba_readl(pinfo->regbase + SD_CAP_OFFSET);

	dev_dbg(pinfo->dev,
		"SD Clock: base[%uMHz], min[%uHz], max[%uHz].\n",
		SD_CAP_BASE_FREQ(hc_cap), mmc->f_min, mmc->f_max);

	hc_timeout_clk = mmc->f_max / 1000;
	if (hc_timeout_clk == 0)
		hc_timeout_clk = 24000;

	mmc->max_discard_to = (1 << 27) / hc_timeout_clk;

	if (hc_cap & SD_CAP_MAX_2KB_BLK)
		mmc->max_blk_size = 2048;
	else if (hc_cap & SD_CAP_MAX_1KB_BLK)
		mmc->max_blk_size = 1024;
	else if (hc_cap & SD_CAP_MAX_512B_BLK)
		mmc->max_blk_size = 512;

	dev_dbg(pinfo->dev, "SD max_blk_size: %u.\n", mmc->max_blk_size);

	mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ |
			MMC_CAP_ERASE | MMC_CAP_BUS_WIDTH_TEST;

	mmc->caps2 |= MMC_CAP2_HS200 | MMC_CAP2_HS200_1_8V_SDR;

	if (mmc->f_max > 25000000) {
		mmc->caps |= MMC_CAP_SD_HIGHSPEED;
		mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
	}

	if (!(hc_cap & SD_CAP_DMA)) {
		ambsd_err(pslotinfo, "HW do not support DMA!\n");
		retval = -ENODEV;
		goto init_slot_err1;
	}

	if (hc_cap & SD_CAP_ADMA_SUPPORT) {
		dev_dbg(pinfo->dev, "HW support ADMA!\n");
		pslotinfo->use_adma = pslotinfo->caps_adma ? 1 : 0;
	}

	dev_dbg(pinfo->dev, "HW%s support Suspend/Resume!\n",
			(hc_cap & SD_CAP_SUS_RES) ? "" : " do not");

	mmc->ocr_avail = 0;
	if (hc_cap & SD_CAP_VOL_3_3V)
		mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;

	if ((hc_cap & SD_CAP_VOL_1_8V) && !pslotinfo->no_1_8_v) {
		mmc->ocr_avail |= MMC_VDD_165_195;
		if (hc_cap & SD_CAP_HIGH_SPEED)
			mmc->caps |= MMC_CAP_1_8V_DDR;

		mmc->caps |= MMC_CAP_UHS_SDR12;
		if (mmc->f_max > 25000000) {
			mmc->caps |= MMC_CAP_UHS_SDR25;
			if (pslotinfo->caps_ddr)
				mmc->caps |= MMC_CAP_UHS_DDR50;
		}

		if (mmc->f_max > 50000000)
			mmc->caps |= MMC_CAP_UHS_SDR50;

		if (mmc->f_max > 100000000)
			mmc->caps |= MMC_CAP_UHS_SDR104;
	}

	if (mmc->ocr_avail == 0) {
		ambsd_err(pslotinfo, "HW report wrong voltages[0x%x]!\n", hc_cap);
		retval = -ENODEV;
		goto init_slot_err1;
	}

	if (!(hc_cap & SD_CAP_INTMODE)) {
		ambsd_err(pslotinfo, "HW do not support Interrupt mode!\n");
		retval = -ENODEV;
		goto init_slot_err1;
	}

	dev_dbg(pinfo->dev, "SD caps: 0x%x.\n", mmc->caps);
	dev_dbg(pinfo->dev, "SD ocr: 0x%x.\n", mmc->ocr_avail);

	mmc->max_blk_count = 0xFFFF;
	mmc->max_seg_size = pinfo->max_blk_sz;
	mmc->max_segs = mmc->max_seg_size / PAGE_SIZE;
	mmc->max_req_size =
		min(mmc->max_seg_size, mmc->max_blk_size * mmc->max_blk_count);

	pslotinfo->buf_vaddress = kmem_cache_alloc(pinfo->buf_cache, GFP_KERNEL);
	if (!pslotinfo->buf_vaddress) {
		ambsd_err(pslotinfo, "Can't alloc DMA memory");
		retval = -ENOMEM;
		goto init_slot_err1;
	}
	pslotinfo->buf_paddress = dma_map_single(pinfo->dev,
					pslotinfo->buf_vaddress,
					mmc->max_req_size,
					DMA_BIDIRECTIONAL);
	ambarella_sd_check_dma_boundary(pslotinfo->buf_paddress,
				mmc->max_req_size, pinfo->max_blk_sz);

	dev_notice(pinfo->dev, "Slot%u use bounce buffer[0x%p<->0x%08x]\n",
		pslotinfo->slot_id, pslotinfo->buf_vaddress,
		pslotinfo->buf_paddress);

	dev_notice(pinfo->dev, "Slot%u req_size=0x%08X, "
		"segs=%u, seg_size=0x%08X\n",
		pslotinfo->slot_id, mmc->max_req_size,
		mmc->max_segs, mmc->max_seg_size);

	dev_notice(pinfo->dev, "Slot%u use %sDMA\n", pslotinfo->slot_id,
		pslotinfo->use_adma ? "A" :"");

	ambarella_sd_release_bus(mmc);

	retval = mmc_add_host(pslotinfo->mmc);
	if (retval < 0) {
		ambsd_err(pslotinfo, "Can't add mmc host!\n");
		goto init_slot_err2;
	}

	ambarella_sd_add_debugfs(pslotinfo);

	pslotinfo->system_event.notifier_call = ambarella_sd_system_event;
	ambarella_register_event_notifier(&pslotinfo->system_event);

	retval = of_property_read_u32(np, "global-id", &global_id);
	if (retval < 0 || global_id >= SD_INSTANCES * AMBA_SD_MAX_SLOT_NUM)
		global_id = 0;
	G_mmc[global_id] = mmc;

	return 0;

init_slot_err2:
	dma_unmap_single(pinfo->dev, pslotinfo->buf_paddress,
			pslotinfo->mmc->max_req_size, DMA_BIDIRECTIONAL);
	kmem_cache_free(pinfo->buf_cache, pslotinfo->buf_vaddress);

init_slot_err1:
	mmc_free_host(mmc);
	return retval;
}

static int ambarella_sd_free_slot(struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo = pslotinfo->pinfo;
	int retval = 0;

	ambarella_unregister_event_notifier(&pslotinfo->system_event);

	ambarella_sd_remove_debugfs(pslotinfo);

	mmc_remove_host(pslotinfo->mmc);

	if (pslotinfo->buf_paddress) {
		dma_unmap_single(pinfo->dev, pslotinfo->buf_paddress,
			pslotinfo->mmc->max_req_size, DMA_BIDIRECTIONAL);
		pslotinfo->buf_paddress = (dma_addr_t)NULL;
	}

	if (pslotinfo->buf_vaddress) {
		kmem_cache_free(pinfo->buf_cache, pslotinfo->buf_vaddress);
		pslotinfo->buf_vaddress = NULL;
	}

	mmc_free_host(pslotinfo->mmc);

	return retval;
}

static int ambarella_sd_of_parse(struct ambarella_sd_controller_info *pinfo)
{
	struct device_node *np = pinfo->dev->of_node;
	const __be32 *prop;
	const char *clk_name;
	int psize, tmo, switch_vol_tmo, retval = 0;

	retval = of_property_read_string(np, "amb,clk-name", &clk_name);
	if (retval < 0) {
		dev_err(pinfo->dev, "Get pll-name failed! %d\n", retval);
		goto pasre_err;
	}

	pinfo->clk = clk_get(NULL, clk_name);
	if (IS_ERR(pinfo->clk)) {
		dev_err(pinfo->dev, "Get PLL failed!\n");
		retval = PTR_ERR(pinfo->clk);
		goto pasre_err;
	}

	if (of_find_property(np, "amb,dma-addr-fix", NULL))
		pinfo->dma_fix = 0xc0000000;

	retval = of_property_read_u32(np, "amb,wait-tmo", &tmo);
	if (retval < 0)
		tmo = 0x10000;

	pinfo->default_wait_tmo = msecs_to_jiffies(tmo);

	if (pinfo->default_wait_tmo < CONFIG_SD_AMBARELLA_WAIT_TIMEOUT)
		pinfo->default_wait_tmo = CONFIG_SD_AMBARELLA_WAIT_TIMEOUT;

	retval = of_property_read_u32(np, "amb,switch-vol-tmo", &switch_vol_tmo);
	if (retval < 0)
		switch_vol_tmo = 100;

	pinfo->switch_voltage_tmo = switch_vol_tmo;

	retval = of_property_read_u32(np, "amb,max-blk-size", &pinfo->max_blk_sz);
	if (retval < 0)
		pinfo->max_blk_sz = 0x20000;

	pinfo->auto_tuning = !!of_find_property(np, "amb,auto-tuning", NULL);
	retval = of_property_read_u32(np, "amb,phy-type", &pinfo->phy_type);
	if(retval) {
		/*this controller don't support sd phy*/
		pinfo->phy_type = -1;
		retval = 0;
	} else {
		/* amb,phy-timing must be provided when timing_reg is given */
		prop = of_get_property(np, "amb,phy-timing", &psize);
		if (!prop) {
			retval = -EINVAL;
			goto pasre_err;
		}

		psize /= sizeof(u32);
		BUG_ON(psize % 3);
		pinfo->phy_timing_num = psize / 3;
		pinfo->phy_timing = devm_kzalloc(pinfo->dev, psize, GFP_KERNEL);
		if (pinfo->phy_timing == NULL) {
			retval = -ENOMEM;
			goto pasre_err;
		}

		retval = of_property_read_u32_array(np, "amb,phy-timing",
				(u32 *)pinfo->phy_timing, psize);

		/* bit0 of mode must be set, and the phy setting in first row with
		 * bit0 set is the default one. */
		BUG_ON(!(pinfo->phy_timing[0].mode & 0x1));
	}

pasre_err:
	return retval;
}

static int ambarella_sd_get_resource(struct platform_device *pdev,
			struct ambarella_sd_controller_info *pinfo)
{
	struct resource *mem;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get SD/MMC mem resource failed!\n");
		return -ENXIO;
	}

	pinfo->regbase = devm_ioremap(&pdev->dev,
					mem->start, resource_size(mem));
	if (pinfo->regbase == NULL) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get FIO mem resource failed!\n");
		return -ENXIO;
	}

	pinfo->fio_reg = devm_ioremap(&pdev->dev,
					mem->start, resource_size(mem));
	if (pinfo->fio_reg == NULL) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (mem == NULL) {
		pinfo->timing_reg = NULL;
		pinfo->auto_tuning = false;
	} else {
		pinfo->timing_reg = devm_ioremap(&pdev->dev,
					mem->start, resource_size(mem));
		if (pinfo->timing_reg == NULL) {
			dev_err(&pdev->dev, "devm_ioremap() failed\n");
			return -ENOMEM;
		}
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (mem == NULL) {
			pinfo->sbc_reg = pinfo->timing_reg;
	} else {
			pinfo->sbc_reg = devm_ioremap(&pdev->dev,
									mem->start, resource_size(mem));
			if (pinfo->sbc_reg == NULL) {
					dev_err(&pdev->dev, "devm_ioremap() failed for sbc_reg\n");
					return -ENOMEM;
			}
	}

	pinfo->irq = platform_get_irq(pdev, 0);
	if (pinfo->irq < 0) {
		dev_err(&pdev->dev, "Get SD/MMC irq resource failed!\n");
		return -ENXIO;
	}

	return 0;
}

static int ambarella_sd_probe(struct platform_device *pdev)
{
	struct ambarella_sd_controller_info *pinfo;
	struct device_node *slot_np;
	int retval, i, slot_id = -1;

	pinfo = devm_kzalloc(&pdev->dev, sizeof(*pinfo), GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		return -ENOMEM;
	}
	pinfo->dev = &pdev->dev;

	retval = ambarella_sd_get_resource(pdev, pinfo);
	if (retval < 0)
		return retval;

	retval = ambarella_sd_of_parse(pinfo);
	if (retval < 0)
		return retval;

	pinfo->buf_cache = kmem_cache_create(dev_name(&pdev->dev),
				pinfo->max_blk_sz, pinfo->max_blk_sz, 0, NULL);
	if (!pinfo->buf_cache) {
		dev_err(&pdev->dev, "Can't alloc DMA Cache");
		return -ENOMEM;
	}

	pinfo->slot_num = 0;
	for_each_child_of_node(pdev->dev.of_node, slot_np) {
		if (!slot_np->name || of_node_cmp(slot_np->name, "slot"))
			continue;

		retval = of_property_read_u32(slot_np, "reg", &slot_id);
		if (retval < 0 || slot_id >= AMBA_SD_MAX_SLOT_NUM)
			goto ambarella_sd_probe_free_host;

		retval = ambarella_sd_init_slot(slot_np, slot_id, pinfo);
		if (retval < 0)
			goto ambarella_sd_probe_free_host;

		pinfo->slot_num++;
	}

	platform_set_drvdata(pdev, pinfo);

	retval = devm_request_irq(&pdev->dev, pinfo->irq, ambarella_sd_irq,
				IRQF_SHARED | IRQF_TRIGGER_HIGH,
				dev_name(&pdev->dev), pinfo);
	if (retval < 0) {
		dev_err(&pdev->dev, "Can't Request IRQ%u!\n", pinfo->irq);
		goto ambarella_sd_probe_free_host;
	}

	dev_info(&pdev->dev, "%u slots @ %luHz\n",
			pinfo->slot_num, clk_get_rate(pinfo->clk));

	return 0;

ambarella_sd_probe_free_host:
	for (i = pinfo->slot_num - 1; i >= 0; i--)
		ambarella_sd_free_slot(pinfo->pslotinfo[i]);

	if (pinfo->buf_cache) {
		kmem_cache_destroy(pinfo->buf_cache);
		pinfo->buf_cache = NULL;
	}

	return retval;
}

static int ambarella_sd_remove(struct platform_device *pdev)
{
	struct ambarella_sd_controller_info *pinfo;
	int retval = 0, i;

	pinfo = platform_get_drvdata(pdev);

	for (i = 0; i < pinfo->slot_num; i++)
		ambarella_sd_free_slot(pinfo->pslotinfo[i]);

	if (pinfo->buf_cache)
		kmem_cache_destroy(pinfo->buf_cache);

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor SD/MMC Host Controller.\n");

	return retval;
}

#ifdef CONFIG_PM
static int ambarella_sd_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;

	int retval = 0, i;

	pinfo = platform_get_drvdata(pdev);

	for (i = 0; i < pinfo->slot_num; i++) {
		pslotinfo = pinfo->pslotinfo[i];

		if (pslotinfo->mmc) {
			if (pslotinfo->mmc->pm_caps & MMC_PM_KEEP_POWER) {
				pslotinfo->mmc->pm_flags = MMC_PM_KEEP_POWER;
			}
			retval = mmc_suspend_host(pslotinfo->mmc);
			if (retval) {
				ambsd_err(pslotinfo,
					"mmc_suspend_host[%d] failed[%d]!\n", i, retval);
				return retval;
			}
		}
		if (pslotinfo->mmc->pm_caps & MMC_PM_KEEP_POWER) {
			ambarella_sd_disable_int(pslotinfo->mmc, SD_NISEN_CARD);
			pslotinfo->sd_nisen = amba_readw(pinfo->regbase + SD_NISEN_OFFSET);
			pslotinfo->sd_eisen = amba_readw(pinfo->regbase + SD_EISEN_OFFSET);
			pslotinfo->sd_nixen = amba_readw(pinfo->regbase + SD_NIXEN_OFFSET);
			pslotinfo->sd_eixen = amba_readw(pinfo->regbase + SD_EIXEN_OFFSET);
		}
	}

	disable_irq(pinfo->irq);
	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n", __func__,
				retval, state.event);

	return retval;

}

static int ambarella_sd_resume(struct platform_device *pdev)
{
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;
	int retval = 0, i;

	pinfo = platform_get_drvdata(pdev);

	enable_irq(pinfo->irq);

	for (i = 0; i < pinfo->slot_num; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (gpio_is_valid(pslotinfo->pwr_gpio)){
			gpio_direction_output(pslotinfo->pwr_gpio, pslotinfo->pwr_gpio_active);
		}
		if (pslotinfo->mmc->pm_caps & MMC_PM_KEEP_POWER) {
			amba_writew(pinfo->regbase + SD_NISEN_OFFSET, pslotinfo->sd_nisen);
			amba_writew(pinfo->regbase + SD_EISEN_OFFSET, pslotinfo->sd_eisen);
			amba_writew(pinfo->regbase + SD_NIXEN_OFFSET, pslotinfo->sd_nixen);
			amba_writew(pinfo->regbase + SD_EIXEN_OFFSET, pslotinfo->sd_eixen);
			pslotinfo->mmc->caps |= MMC_CAP_NONREMOVABLE;
			mdelay(10);
			ambarella_sd_set_clk(pslotinfo->mmc, &pinfo->controller_ios);
			ambarella_sd_set_bus(pslotinfo->mmc, &pinfo->controller_ios);
			mdelay(10);
			ambarella_sd_enable_int(pslotinfo->mmc, SD_NISEN_CARD);
		} else {
			clk_set_rate(pinfo->clk, pslotinfo->mmc->f_max);
			ambarella_sd_reset_all(pslotinfo->mmc);
		}
	}

	for (i = 0; i < pinfo->slot_num; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			retval = mmc_resume_host(pslotinfo->mmc);
			if (retval) {
				ambsd_err(pslotinfo,
				"mmc_resume_host[%d] failed[%d]!\n", i, retval);
			}
		}
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, retval);

	return retval;
}
#endif

#ifdef CONFIG_AMBARELLA_EMMC_BOOT
void ambarella_sd_shutdown (struct platform_device *pdev)
{
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;
	struct mmc_host *host;
	struct mmc_command cmd = {0};
	int i;

	pinfo = platform_get_drvdata(pdev);

	for (i = 0; i < pinfo->slot_num; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		host = pslotinfo->mmc;
		if((host == G_mmc[0]) && ((system_state == SYSTEM_RESTART) ||
			(system_state == SYSTEM_HALT))) {
			if (mmc_try_claim_host(pslotinfo->mmc)) {
				cmd.opcode = 0;
				cmd.arg = 0xf0f0f0f0;
				cmd.flags = MMC_RSP_NONE;

				mmc_wait_for_cmd(pslotinfo->mmc, &cmd, 0);
			} else {
				dev_err(&pdev->dev, "Unable to claim host!\n");
			}
		}
	}

}
#endif

static const struct of_device_id ambarella_mmc_dt_ids[] = {
	{ .compatible = "ambarella,sdmmc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_mmc_dt_ids);

static struct platform_driver ambarella_sd_driver = {
	.probe		= ambarella_sd_probe,
	.remove		= ambarella_sd_remove,
#ifdef CONFIG_PM
	.suspend	= ambarella_sd_suspend,
	.resume		= ambarella_sd_resume,
#endif

#ifdef CONFIG_AMBARELLA_EMMC_BOOT
	.shutdown	= ambarella_sd_shutdown,
#endif
	.driver		= {
		.name	= "ambarella-sd",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_mmc_dt_ids,
	},
};

static int __init ambarella_sd_init(void)
{
	int retval = 0;

	retval = platform_driver_register(&ambarella_sd_driver);
	if (retval) {
		printk(KERN_ERR "%s: Register failed %d!\n",
			__func__, retval);
	}

	return retval;
}

static void __exit ambarella_sd_exit(void)
{
	platform_driver_unregister(&ambarella_sd_driver);
}

fs_initcall(ambarella_sd_init);
module_exit(ambarella_sd_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SD/MMC Host Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

