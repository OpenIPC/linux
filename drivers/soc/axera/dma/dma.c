/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include "dma.h"

#define AX_DMA_DEVICE_NAME "ax_dma"
#define AX_DMA_PROC_NAME "ax_proc/dma"

static DEFINE_IDR(xfer_idr);
static struct ax_dma_device *dma;
static s32 ax_dma_set_lli_msg(ax_dma_xfer_msg_t *dma_xfer_msg);
static void ax_dma_free_lli(ax_dma_xfer_msg_t *dma_xfer_msg);
static ax_dma_dbg_t g_dbg_buf;
static char *ax_dma_mode[6] = {
	"1D      ", "2D      ", "3D      ", "4D      ", "memset  ", "checksum"
};

static u64 do_div_quotient(u64 n, u32 base)
{
	u64 tmp;
	tmp = do_div(n, base);
	return n;
}

static void kfree_set_null(void *p)
{
	if (!IS_ERR_OR_NULL(p))
		kfree(p);
	p = NULL;
}

static u32 ax_dma_read(struct ax_dma_device *dma, u32 reg)
{
	return readl(dma->base + reg);
}

static void ax_dma_write(struct ax_dma_device *dma, u32 reg, u32 val)
{
	writel(val, dma->base + reg);
}

static void ax_dma_rmw(struct ax_dma_device *dma, u32 reg,
			  u32 set, u32 mask, u32 shift)
{
	u32 val;

	val = ax_dma_read(dma, reg);
	val &= ~(mask << shift);
	val |= (set << shift);
	ax_dma_write(dma, reg, val);
}

static void ax_dma_clk_enable(void)
{
	clk_prepare_enable(dma->pclk);
	clk_prepare_enable(dma->aclk);
}

static void ax_dma_clk_disable(void)
{
	clk_disable_unprepare(dma->aclk);
	clk_disable_unprepare(dma->pclk);
}

static void ax_dma_hw_reset(void)
{
	reset_control_assert(dma->arst);
	reset_control_assert(dma->prst);
	reset_control_deassert(dma->prst);
	reset_control_deassert(dma->arst);
}

static int __maybe_unused ax_dma_suspend(struct device *dev)
{
	int ret = 0;
	u32 val;

	ax_dma_rmw(dma, DMA_CTRL, 1, DMA_CTRL_SUSPEND_MASK,
		      DMA_CTRL_SUSPEND_SHIFT);
	ret = readl_poll_timeout_atomic(dma->base + DMA_STA, val,
				      (val & BIT(DMA_STA_SUSPEND_SHIFT)),
				      1000, 10000);
	if (ret == -ETIMEDOUT) {
		pr_warn("ax_dma failed to suspend\n");
	} else {
		dma->suspend = 1;
		ax_dma_clk_disable();
	}
	return 0;
}

static int __maybe_unused ax_dma_resume(struct device *dev)
{
	if (dma->suspend == 1) {
		dma->suspend = 0;
		ax_dma_clk_enable();
		ax_dma_rmw(dma, DMA_CTRL, 0, DMA_CTRL_SUSPEND_MASK,
						DMA_CTRL_SUSPEND_SHIFT);
	}
	return 0;
}

static void ax_dma_set_base_lli(u64 lli_base)
{
	ax_dma_rmw(dma, DMA_CTRL, 1, DMA_CTRL_TYPE_MASK,
		      DMA_CTRL_TYPE_SHIFT);
	ax_dma_write(dma, DMA_LLI_BASE_H,
			(u32) ((u64) lli_base >> 32));
	ax_dma_write(dma, DMA_LLI_BASE_L,
			(u32) ((u64) lli_base & 0xFFFFFFFF));
}

static void ax_dma_enable_irq(struct ax_dma_device *dma, bool en)
{
	u64 tmp;
	u64 hwch = 0;
	u64 irq_mask = 0x3D;

	tmp = ax_dma_read(dma, DMA_INT_GLB_MASK);
	if (en) {
		tmp |= BIT(hwch);
		writel(tmp, dma->base + DMA_INT_GLB_MASK);
		writel(irq_mask, dma->base + DMA_INT_MASK(hwch));
	} else {
		tmp &= ~BIT(hwch);
		writel(tmp, dma->base + DMA_INT_GLB_MASK);
		writel(0x0, dma->base + DMA_INT_MASK(hwch));
	}
}

static inline bool ax_chan_hw_is_enable(struct ax_dma_device *dma)
{
	return ! !ax_dma_read(dma, DMA_INT_GLB_MASK);
}

static void ax_dma_disable_all_irq(struct ax_dma_device *dma)
{
	u64 i;

	writel(0x0, dma->base + DMA_INT_GLB_MASK);
	for (i = 0; i < 16; i++) {
		writel(0x0, dma->base + DMA_INT_MASK(i));
	}
}

static void ax_dma_start(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	if (ax_chan_hw_is_enable(dma))
		return;
	dma_xfer_msg->ktime_run = ktime_get();
	ax_dma_set_base_lli(dma_xfer_msg->xfer.lli_base);
	ax_dma_enable_irq(dma, 1);
	mb();
	ax_dma_write(dma, DMA_START, 1);
	mb();
	ax_dma_write(dma, DMA_TRIG, 0);
	mb();
	return;
}

u32 ax_dma_get_checksum(struct ax_dma_device *dma)
{
	return ax_dma_read(dma, DMA_CHECKSUM);
}

static s32 ax_dma_stat_show(struct seq_file *m, void *v)
{
	s32 i, count;

	seq_printf(m, "The number of user: %d\n",
		   atomic_read(&g_dbg_buf.chan_use));
	seq_printf(m, "The number of all tasks so far: %lld\n",
		   (u64)atomic64_read(&g_dbg_buf.total));
	seq_printf(m,
		   "The number of tasks that still need to be processed: %lld\n",
		   (u64)atomic64_read(&g_dbg_buf.current_total));
	seq_printf(m, "\n");
	seq_printf(m, "type     \tmsg-id        \tsize           \t"
		   "start          \twait(ns)       \trun(ns)        \t\n");
	for (i = 0; i < 8; i++) {
		count = (g_dbg_buf.last + i + 1) & 0x7;
		if (!g_dbg_buf.info[count].size)
			continue;
		seq_printf(m,
			   "%s\t%-2d         \t%-15d\t%-15llu\t%-15llu\t%-15llu\t\n",
			   ax_dma_mode[g_dbg_buf.info[count].dma_mode -
					  AX_DMA_MODE_OFFSET],
			   g_dbg_buf.info[count].msg_id,
			   g_dbg_buf.info[count].size,
			   g_dbg_buf.info[count].start,
			   g_dbg_buf.info[count].wait,
			   g_dbg_buf.info[count].run);
	}
	return 0;
}

static s32 ax_dma_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_dma_stat_show, NULL);
}

static const struct file_operations ax_dma_stat_ops = {
	.open = ax_dma_stat_open,
	.read = seq_read,
	.release = seq_release,
};

static s32 ax_dma_proc_init(struct ax_dma_device *dma)
{
	struct proc_dir_entry *p = NULL;
	p = proc_create(AX_DMA_PROC_NAME, 0644, NULL, &ax_dma_stat_ops);
	if (unlikely(!p)) {
		pr_err("err: Create proc fail!\n");
		return -EFAULT;
	}
	return 0;
}

void ax_dma_work_handler(struct work_struct *work)
{
	int ret = 0;
	unsigned long flags;
	ax_dma_xfer_msg_t *__dma_xfer_msg = NULL;
	ax_dma_work_data_t *work_data = container_of(work, ax_dma_work_data_t, work);
	ax_dma_xfer_msg_t *dma_xfer_msg = work_data->dma_xfer_msg;

	dma_xfer_msg->dma_callback(dma_xfer_msg->arg,
					dma_xfer_msg->xfer_stat.stat);
	ret = ax_dma_ioctl(dma_xfer_msg->filp,
				DMA_FREE_MSG_BY_ID,
				dma_xfer_msg->xfer_stat.id);
	if (ret) {
		pr_err("DMA_FREE_MSG_BY_ID err\n");
	}
	ax_dma_release(work_data->inode, work_data->filp);
	kfree_set_null(work_data->filp);
	kfree_set_null(work_data->inode);

	spin_lock_irqsave(&dma->lock, flags);
	__dma_xfer_msg =
	    list_first_entry_or_null(&dma->dma_pending_list,
				     ax_dma_xfer_msg_t, xfer_node);
	ax_dma_enable_irq(dma, 0);
	if (__dma_xfer_msg) {
		ax_dma_start(__dma_xfer_msg);
	}
	spin_unlock_irqrestore(&dma->lock, flags);

	return ;
}

static irqreturn_t ax_dma_interrupt(s32 irq, void *dev_id)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL;
	ax_dma_xfer_msg_t *__dma_xfer_msg = NULL;
	ax_dma_chn_t *chn = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dma->lock, flags);
	dma->irq_stat = ax_dma_read(dma, DMA_INT_STA(0));
	pr_debug("raw star %d\n", ax_dma_read(dma, DMA_INT_RAW(0)));
	ax_dma_write(dma, DMA_INT_CLR(0), 0x3F);
	mb();
	ax_dma_enable_irq(dma, 0);

	dma_xfer_msg =
	    list_first_entry_or_null(&dma->dma_pending_list,
				     ax_dma_xfer_msg_t, xfer_node);
	if (!dma_xfer_msg) {
		spin_unlock_irqrestore(&dma->lock, flags);
		pr_err("err: raw star %d\n", dma->irq_stat);
		return IRQ_HANDLED;
	}

	if (dma->irq_stat & DMA_AXI_WE_ERR) {
		dma_xfer_msg->xfer_stat.stat = DMA_AXI_WE_ERR;
	} else if (dma->irq_stat & DMA_AXI_RD_ERR) {
		dma_xfer_msg->xfer_stat.stat = DMA_AXI_RD_ERR;
	} else if (dma->irq_stat & DMA_XFER_SUCCESS) {
		dma_xfer_msg->xfer_stat.stat = DMA_XFER_SUCCESS;
	}
	if (dma_xfer_msg->xfer.dma_mode == AX_DMA_CHECKSUM)
		dma_xfer_msg->xfer_stat.checksum =
		    ax_dma_get_checksum(dma);
	if (dma_xfer_msg->xfer.dma_mode != AX_DMA_CHECKSUM) {
		list_move_tail(&dma_xfer_msg->xfer_node,
			       &dma->dma_complite_list);
		chn = (ax_dma_chn_t *)dma_xfer_msg->filp->private_data;

		if (dma_xfer_msg->kuser) {
			list_move_tail(&dma_xfer_msg->xfer_node,
			       &dma->dma_free_list);
			if (!IS_ERR_OR_NULL(dma_xfer_msg->dma_callback)) {
				dma->work_data.inode = chn->inode;
				dma->work_data.filp = dma_xfer_msg->filp;
				dma->work_data.dma_xfer_msg = dma_xfer_msg;
				schedule_work(&dma->work_data.work);
				goto end;
			} else {
				complete(&dma->complete);
			}
		} else {
			wake_up_interruptible(&chn->event_wait);
		}
	} else {
		list_move_tail(&dma_xfer_msg->xfer_node,
			       &dma->dma_checksum_complite_list);
		__dma_xfer_msg =
		    (ax_dma_xfer_msg_t *)idr_find(&xfer_idr,
						      dma_xfer_msg->xfer_stat.id);
		chn = (ax_dma_chn_t *) __dma_xfer_msg-> filp->private_data;
		if (!(--__dma_xfer_msg->checksum_num)) {
			list_move_tail(&__dma_xfer_msg->xfer_node,
				       &dma->dma_complite_list);
			wake_up_interruptible(&chn->event_wait);
		}
	}
	__dma_xfer_msg =
	    list_first_entry_or_null(&dma->dma_pending_list,
				     ax_dma_xfer_msg_t, xfer_node);
	if (__dma_xfer_msg) {
		ax_dma_start(__dma_xfer_msg);
	}
end:
	spin_unlock_irqrestore(&dma->lock, flags);

	return IRQ_HANDLED;
}

static u32 ax_dma_get_xfer_width(size_t size)
{
	s32 max_width = 3;
	return __ffs(size | BIT(max_width));
}

static void
ax_dma_set_ctrl_data(unsigned long long data, unsigned long long *ctrl_data,
			unsigned int shift, unsigned long long mask)
{
	*ctrl_data = ((*ctrl_data) & (~(mask << shift))) | (data << shift);
}

static void ax_dma_set_lli_ctrl_data(ax_dma_xfer_msg_t *dma_xfer_msg,
					u64 *ctrl_data,
					ax_dma_lli_ctrl_t lli_ctrl)
{
	*ctrl_data = AX_DMA_DEFAULT_CTRL;
	if(dma_xfer_msg->kuser) {
		ax_dma_set_ctrl_data(0, ctrl_data, AWLEN_SHIFT, AWLEN_MASK);
		ax_dma_set_ctrl_data(0, ctrl_data, ARLEN_SHIFT, ARLEN_MASK);
	}
	ax_dma_set_ctrl_data(lli_ctrl.lli_dst_tr_width, ctrl_data,
				DST_TR_WIDTH_SHIFT, DST_TR_WIDTH_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_src_tr_width, ctrl_data,
				SRC_TR_WIDTH_SHIFT, SRC_TR_WIDTH_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_wb, ctrl_data, WB_SHIFT, WB_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_endian, ctrl_data,
				ENDIAN_SHIFT, ENDIAN_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_chksum, ctrl_data,
				CHKSUM_SHIFT, CHKSUM_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_xor, ctrl_data,
				XOR_NUM_SHIFT, XOR_NUM_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_type, ctrl_data,
				TYPE_SHIFT, TYPE_MASK);
	ax_dma_set_ctrl_data(lli_ctrl.lli_last, ctrl_data,
				LAST_SHIFT, LAST_MASK);
}

static u32 ax_dma_get_dim_size(ax_dma_xfer_msg_t *dma_xfer_msg,
			       ax_dma_lli_t *lli_list, u32 dma_mode)
{
	u32 size;

	size = lli_list->lli_addr_vir->src_imgw <<
	    lli_list->lli_ctrl.lli_src_tr_width;
	size *= lli_list->lli_addr_vir->src_ntile1;
	if (dma_mode > AX_DMA_2D)
		size *= lli_list->lli_addr_vir->src_ntile2;
	if (dma_mode > AX_DMA_3D)
		size *= lli_list->lli_addr_vir->src_ntile3;

	return size;
}

static s32 ax_dma_lli_write(ax_dma_xfer_msg_t *dma_xfer_msg,
			       ax_dma_lli_t *lli_list,
			       u8 *p_msg_lli, ax_dma_xfer_t *xfer)
{
	u32 src_imgw, dst_imgw;
	u64 size, size_width;
	u32 dma_mode = xfer->dma_mode;
	ax_dma_desc_t *p_msg_lli_data = (ax_dma_desc_t *)p_msg_lli;
	ax_dma_xd_desc_t *p_dim_msg_lli_data =
	    (ax_dma_xd_desc_t *) p_msg_lli;
	ax_dma_lli_pool_t *lli_pool = NULL;
	unsigned long flags;

	lli_pool = (void *)kzalloc(sizeof(ax_dma_lli_pool_t), GFP_KERNEL);
	lli_pool->lli_addr = lli_list->lli_addr;
	lli_pool->lli_addr_vir = lli_list->lli_addr_vir;
	spin_lock_irqsave(&dma->lock, flags);
	list_add_tail(&lli_pool->lli_node, &dma_xfer_msg->dma_pool_list);
	spin_unlock_irqrestore(&dma->lock, flags);
	lli_list->lli_addr_vir->llp = lli_list->lli_addr_next;
	lli_list->lli_ctrl.lli_type =
	    (dma_mode == AX_DMA_CHECKSUM) ? AX_DMA_1D : dma_mode;
	lli_list->lli_ctrl.lli_endian = xfer->endian;
	if (lli_list->lli_addr_vir_next == NULL) {
		lli_list->lli_ctrl.lli_last = 1;
	} else {
		lli_list->lli_ctrl.lli_last = 0;
	}
	lli_list->lli_ctrl.lli_wb = 1;
	lli_list->lli_ctrl.lli_chksum = 0;

	if (dma_mode > AX_DMA_1D && dma_mode < AX_DMA_MEMORY_INIT) {
		src_imgw = p_dim_msg_lli_data->src_info.imgw;
		dst_imgw = p_dim_msg_lli_data->dst_info.imgw;

		lli_list->lli_addr_vir->src =
		    p_dim_msg_lli_data->src_info.paddr;
		lli_list->lli_addr_vir->dst =
		    p_dim_msg_lli_data->dst_info.paddr;

		lli_list->lli_addr_vir->src_stride1 =
		    (u32) (p_dim_msg_lli_data->src_info.stride[0]);
		lli_list->lli_addr_vir->dst_stride1 =
		    (u32) (p_dim_msg_lli_data->dst_info.stride[0]);
		lli_list->lli_addr_vir->src_stride2 =
		    (u32) (p_dim_msg_lli_data->src_info.stride[1]);
		lli_list->lli_addr_vir->dst_stride2 =
		    (u32) (p_dim_msg_lli_data->dst_info.stride[1]);
		lli_list->lli_addr_vir->src_stride3 =
		    (u32) (p_dim_msg_lli_data->src_info.stride[2]);
		lli_list->lli_addr_vir->dst_stride3 =
		    (u32) (p_dim_msg_lli_data->dst_info.stride[2]);
		lli_list->lli_addr_vir->src_ntile1 =
		    (u16) (p_dim_msg_lli_data->ntiles[0]);
		lli_list->lli_addr_vir->dst_ntile1 = src_imgw *
		    lli_list->lli_addr_vir->src_ntile1 / dst_imgw;
		lli_list->lli_addr_vir->src_ntile2 =
		    lli_list->lli_addr_vir->dst_ntile2 =
		    (u16) (p_dim_msg_lli_data->ntiles[1]);
		lli_list->lli_addr_vir->src_ntile3 =
		    lli_list->lli_addr_vir->dst_ntile3 =
		    (u16) (p_dim_msg_lli_data->ntiles[2]);

		lli_list->lli_ctrl.lli_src_tr_width =
		    ax_dma_get_xfer_width(src_imgw);

		lli_list->lli_ctrl.lli_dst_tr_width =
		    ax_dma_get_xfer_width(dst_imgw);

		lli_list->lli_addr_vir->src_imgw =
		    (u16) (p_dim_msg_lli_data->src_info.imgw >>
			   lli_list->lli_ctrl.lli_src_tr_width);

		lli_list->lli_addr_vir->dst_imgw =
		    (u16) (p_dim_msg_lli_data->dst_info.imgw >>
			   lli_list->lli_ctrl.lli_dst_tr_width);
		if(!lli_list->lli_addr_vir->src_imgw || !lli_list->lli_addr_vir->dst_imgw) {
			pr_err("err\n");
			return -EINVAL;
		}
		dma_xfer_msg->size = ax_dma_get_dim_size(dma_xfer_msg,
							       lli_list,
							       xfer->dma_mode);
	} else {
		if (dma_mode == AX_DMA_CHECKSUM) {
			dma_xfer_msg->size = p_msg_lli_data->size;
			//chacksum src addr align 32
			if (p_msg_lli_data->src & 0x3) {
				pr_err("err\n");
				return -EINVAL;
			}
			lli_list->lli_ctrl.lli_wb = 0;
			lli_list->lli_ctrl.lli_chksum = 1;
		}
		if (dma_mode == AX_DMA_MEMORY_INIT &&
		    (!p_msg_lli_data->size || p_msg_lli_data->size & 0x7)) {
			pr_err("err\n");
			return -EINVAL;
		}
		lli_list->lli_addr_vir->src = p_msg_lli_data->src;
		lli_list->lli_addr_vir->dst = p_msg_lli_data->dst;
		size = p_msg_lli_data->size;
		lli_list->lli_ctrl.lli_src_tr_width =
		    lli_list->lli_ctrl.lli_dst_tr_width =
		    ax_dma_get_xfer_width(size);
		lli_list->lli_addr_vir->size =
		    size >> lli_list->lli_ctrl.lli_src_tr_width;
	}
	//detect endian
	if (dma_mode < AX_DMA_MEMORY_INIT) {
		if (dma_mode != AX_DMA_1D)
			size = src_imgw * lli_list->lli_addr_vir->src_ntile1;
		size_width = ax_dma_get_xfer_width(size);
		if (lli_list->lli_ctrl.lli_endian == DMA_ENDIAN_16B) {
			if (size_width < DMA_TR_2B) {
				pr_err("err: detect endian fail\n");
				return -EINVAL;
			}
		} else if (lli_list->lli_ctrl.lli_endian == DMA_ENDIAN_32B) {
			if (size_width < DMA_TR_4B) {
				pr_err("err: detect endian fail\n");
				return -EINVAL;
			}
		}
	}
	ax_dma_set_lli_ctrl_data(dma_xfer_msg,
				    &lli_list->lli_addr_vir->ctrl,
				    lli_list->lli_ctrl);
	return 0;
}

static s32 ax_dma_set_lli_ctrl(ax_dma_xfer_msg_t *dma_xfer_msg,
				  ax_dma_lli_t *lli_list, u8 *p_msg_lli,
				  ax_dma_xfer_t *xfer)
{
	s32 ret;
	u32 step_size = 0;
	u32 tmp_size = 0;
	u32 dma_mode = xfer->dma_mode;
	ax_dma_lli_t tmp_lli_list;
	ax_dma_desc_t *p_msg_lli_data = (ax_dma_desc_t *) p_msg_lli;
	if (dma_mode == DMA_1D || dma_mode == DMA_MEMORY_INIT) {
		memcpy(&tmp_lli_list, lli_list, sizeof(ax_dma_lli_t));
		dma_xfer_msg->size += p_msg_lli_data->size;
		while (1) {
			tmp_size = p_msg_lli_data->size;
			if (p_msg_lli_data->size >= DMA_TR_4B_MAXSIZE) {
				step_size = DMA_TR_4B_MAXSIZE;
			} else if (p_msg_lli_data->size >= DMA_TR_2B_MAXSIZE) {
				step_size = DMA_TR_2B_MAXSIZE;
			} else if (p_msg_lli_data->size >= DMA_TR_1B_MAXSIZE) {
				step_size = DMA_TR_1B_MAXSIZE;
			} else {
				step_size = p_msg_lli_data->size;
			}
			if (p_msg_lli_data->size == step_size) {
				return ax_dma_lli_write(dma_xfer_msg,
							   lli_list, p_msg_lli,
							   xfer);
			}
			p_msg_lli_data->size = step_size;
			if (p_msg_lli_data->size) {
				lli_list->lli_addr_vir_next =
				    dma_pool_zalloc(dma->dma_pool,
						    GFP_KERNEL,
						    (dma_addr_t *)&lli_list->lli_addr_next);
				if (unlikely(!lli_list->lli_addr_vir_next)) {
					ax_dma_free_lli(dma_xfer_msg);
					pr_err("err\n");
					return -ENOMEM;
				}

				ret =
				    ax_dma_lli_write(dma_xfer_msg,
							lli_list, p_msg_lli,
							xfer);
				if (ret) {
					pr_err("err\n");
					return -EINVAL;
				}

				lli_list->lli_addr = lli_list->lli_addr_next;
				lli_list->lli_addr_vir =
				    lli_list->lli_addr_vir_next;
				lli_list->lli_addr_next =
				    tmp_lli_list.lli_addr_next;
				lli_list->lli_addr_vir_next =
				    tmp_lli_list.lli_addr_vir_next;
			}

			p_msg_lli_data->size = tmp_size - step_size;
			p_msg_lli_data->dst += step_size;
			if (dma_mode != DMA_MEMORY_INIT)
				p_msg_lli_data->src += step_size;
		}
		return 0;
	} else {
		return ax_dma_lli_write(dma_xfer_msg, lli_list, p_msg_lli,
					   xfer);
	}
}

static s32 ax_dma_get_id(struct ax_dma_device *dma, void *arg)
{
	s32 id;
	mutex_lock(&dma->mutex_idr);
	id = idr_alloc_cyclic(&xfer_idr, arg, 1, 1024, GFP_KERNEL);
	mutex_unlock(&dma->mutex_idr);
	return id;
}

static void ax_dma_free_id(struct ax_dma_device *dma, s32 id)
{
	mutex_lock(&dma->mutex_idr);
	idr_remove(&xfer_idr, id);
	mutex_unlock(&dma->mutex_idr);
}

static void ax_dma_free_xfer_buf(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	kfree_set_null(dma_xfer_msg->dma_msg.msg_lli);
	kfree_set_null(dma_xfer_msg);
}

static void ax_dma_free_xfer_msg(ax_dma_xfer_msg_t *dma_xfer_msg,
				    bool free_id)
{
	ax_dma_free_lli(dma_xfer_msg);
	if (free_id)
		ax_dma_free_id(dma, dma_xfer_msg->xfer_stat.id);
	ax_dma_free_xfer_buf(dma_xfer_msg);
}

static s32 ax_dma_check_args(ax_dma_msg_t *dma_msg)
{
	s32 i;
	ax_dma_desc_t *msg_data = (ax_dma_desc_t *) dma_msg->msg_lli;
	ax_dma_xd_desc_t *msg_dim_data =
	    (ax_dma_xd_desc_t *) dma_msg->msg_lli;

	if (dma_msg->lli_num < 1) {
		return -EINVAL;
	}
	switch (dma_msg->dma_mode) {
	case AX_DMA_4D:
	case AX_DMA_3D:
	case AX_DMA_2D:
		for (i = 0; i < dma_msg->lli_num; i++) {
			if (!msg_dim_data->src_info.paddr ||
			    !msg_dim_data->dst_info.paddr ||
			    !msg_dim_data->src_info.imgw ||
			    !msg_dim_data->dst_info.imgw ||
			    msg_dim_data->src_info.imgw >
			    msg_dim_data->src_info.stride[0] ||
			    msg_dim_data->dst_info.imgw >
			    msg_dim_data->dst_info.stride[0]) {
				pr_err("err\n");
				return -EINVAL;
			}
			msg_dim_data++;
		}
		break;
	case AX_DMA_1D:
		for (i = 0; i < dma_msg->lli_num; i++) {
			if (!msg_data->size || !msg_data->src || !msg_data->dst) {
				pr_err("err\n");
				return -EINVAL;
			}
			msg_data++;
		}
		break;
	case AX_DMA_CHECKSUM:
		for (i = 0; i < dma_msg->lli_num; i++) {
			if (!msg_data->size || !msg_data->src) {
				pr_err("err\n");
				return -EINVAL;
			}
			msg_data++;
		}
		break;
	case AX_DMA_MEMORY_INIT:
		for (i = 0; i < dma_msg->lli_num; i++) {
			if (!msg_data->size || !msg_data->dst) {
				pr_err("err\n");
				return -EINVAL;
			}
			msg_data++;
		}
		break;
	}
	return 0;
}

static void ax_dma_free_lli(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	ax_dma_xfer_t *xfer = &dma_xfer_msg->xfer;
	ax_dma_lli_pool_t *lli_pool = NULL, *_lli_pool = NULL;

	if(list_empty(&dma_xfer_msg->dma_pool_list))
		return;
	list_for_each_entry_safe(lli_pool, _lli_pool,
				 &dma_xfer_msg->dma_pool_list, lli_node) {
		if (lli_pool) {
			dma_pool_free(dma->dma_pool,
				      lli_pool->lli_addr_vir,
				      lli_pool->lli_addr);
			list_del(&lli_pool->lli_node);
			kfree_set_null(lli_pool);
		}
	}
	kfree_set_null(xfer->lli_list);
	return;
}

static s32 ax_dma_set_lli_msg(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	s32 i, ret = 0;
	ax_dma_msg_t *dma_msg = &dma_xfer_msg->dma_msg;
	ax_dma_xfer_t *xfer = &dma_xfer_msg->xfer;
	ax_dma_lli_t *lli_list = NULL;
	u8 *p_msg_lli = (u8 *) dma_msg->msg_lli;
	xfer->endian = dma_msg->endian;
	xfer->dma_mode = dma_msg->dma_mode;
	xfer->ch = 0;
	xfer->lli_list =
	    (ax_dma_lli_t *) kzalloc(sizeof(ax_dma_lli_t) *
					dma_msg->lli_num, GFP_KERNEL);
	if (unlikely(!xfer->lli_list)) {
		pr_err("err\n");
		return -ENOMEM;
	}
	lli_list = xfer->lli_list;
	lli_list->lli_addr_vir =
	    (ax_dma_lli_reg_t *) dma_pool_zalloc(dma->dma_pool,
						    GFP_KERNEL,
						    (dma_addr_t *)&lli_list->lli_addr);
	if (unlikely(!lli_list->lli_addr_vir)) {
		pr_err("err\n");
		goto err_lli_list;
	}

	xfer->lli_base = lli_list->lli_addr - 128 * (xfer->ch);
	xfer->lli_base_vir = (dma_addr_t) (lli_list->lli_addr_vir) - 128 * (xfer->ch);
	for (i = 0; i < dma_msg->lli_num; i++) {
		if (dma_msg->lli_num - i == 1) {
			lli_list->lli_addr_vir_next = NULL;
			lli_list->lli_addr_next = 0;
		} else {
			lli_list->lli_addr_vir_next =
			    dma_pool_zalloc(dma->dma_pool, GFP_KERNEL,
					    (dma_addr_t *)&lli_list->lli_addr_next);
			if (unlikely(!lli_list->lli_addr_vir_next)) {
				pr_err("err\n");
				ret = -EINVAL;
				goto err_lli_addr;
			}
		}
		ret =
		    ax_dma_set_lli_ctrl(dma_xfer_msg, lli_list, p_msg_lli,
					   xfer);
		if (ret) {
			pr_err("err\n");
			ret = -EINVAL;
			goto err_lli_addr;
		}

		if (xfer->dma_mode > AX_DMA_1D
		    && xfer->dma_mode < AX_DMA_MEMORY_INIT) {
			p_msg_lli += sizeof(ax_dma_xd_desc_t) / sizeof(u8);
			pr_debug("XD");
		} else {
			p_msg_lli += sizeof(ax_dma_desc_t) / sizeof(u8);
			pr_debug("MEM");
		}
		if (dma_msg->lli_num - i > 1) {
			lli_list++;
			lli_list->lli_addr = (lli_list - 1)->lli_addr_next;
			lli_list->lli_addr_vir =
			    (lli_list - 1)->lli_addr_vir_next;
		}
	}
	return 0;
err_lli_addr:
	ax_dma_free_lli(dma_xfer_msg);
err_lli_list:
	kfree_set_null(xfer->lli_list);
	return ret;
}

static void ax_dma_free_list(struct file *filp, struct list_head *list,
				bool free_id)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL, *__dma_xfer_msg = NULL;

	if(list_empty(list))
		return;

	list_for_each_entry_safe(dma_xfer_msg, __dma_xfer_msg,
				 list, xfer_node) {
		if (dma_xfer_msg && dma_xfer_msg->filp == filp) {
			ax_dma_free_xfer_msg(dma_xfer_msg, free_id);
		}
	}
}

static s32 ax_dma_open(struct inode *inode, struct file *filp)
{
	ax_dma_chn_t *chn = NULL;
	atomic_inc(&g_dbg_buf.chan_use);
	chn = (ax_dma_chn_t *) kzalloc(sizeof(ax_dma_chn_t), GFP_KERNEL);
	if ((long)filp->private_data == 1) {
		chn->kuser = 1;
	} else {
		chn->kuser = 0;
		init_waitqueue_head(&chn->event_wait);
	}
	chn->inode = inode;
	filp->private_data = (void *)chn;
	mutex_init(&chn->mutex_sync);
	return 0;
}

static s32 ax_dma_release(struct inode *inode, struct file *filp)
{
	ax_dma_chn_t *chn = (ax_dma_chn_t *)filp->private_data;
	unsigned long flags;

	spin_lock_irqsave(&dma->lock, flags);
	ax_dma_free_list(filp, &dma->dma_pending_list, 1);
	ax_dma_free_list(filp, &dma->dma_complite_list, 1);
	ax_dma_free_list(filp, &dma->dma_free_list, 1);
	ax_dma_free_list(filp, &dma->dma_checksum_cfg_list, 0);
	ax_dma_free_list(filp, &dma->dma_checksum_complite_list, 0);
	kfree_set_null(chn);
	spin_unlock_irqrestore(&dma->lock, flags);
	atomic_dec(&g_dbg_buf.chan_use);
	return 0;
}

static ssize_t ax_dma_setsync(struct file *filp, const char __user *buffer,
			   size_t count, loff_t *ppos)
{
	ax_dma_chn_t *chn = (ax_dma_chn_t *)filp->private_data;

	if (count == 0)
		return 0;

	if (copy_from_user
	    ((void *)&chn->file_msg, buffer, sizeof(ax_dma_file_msg_t)))
		return -EFAULT;

	return count;
}

static ssize_t ax_dma_getsync(struct file *filp, char __user *buffer,
			   size_t count, loff_t *ppos)
{
	ax_dma_chn_t *chn = (ax_dma_chn_t *)filp->private_data;

	if (count == 0)
		return 0;

	return copy_to_user(buffer, (void *)&chn->file_msg,
			    sizeof(ax_dma_file_msg_t));
}

static ax_dma_xfer_msg_t *ax_dma_find_entry_by_filp(struct list_head
							  *list,
							  struct file *filp)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL;
	list_for_each_entry(dma_xfer_msg, list, xfer_node) {
		if (dma_xfer_msg->filp == filp) {
			return dma_xfer_msg;
		}
	}
	return NULL;
}

static ax_dma_xfer_msg_t *ax_dma_find_entry_by_id(struct list_head
							  *list, s32 id)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL;
	list_for_each_entry(dma_xfer_msg, list, xfer_node) {
		if (dma_xfer_msg->xfer_stat.id == id) {
			return dma_xfer_msg;
		}
	}
	return NULL;
}

static __poll_t ax_dma_poll(struct file *filp,
			       struct poll_table_struct *wait)
{
	ax_dma_chn_t *chn = (ax_dma_chn_t *)filp->private_data;
	__poll_t mask = 0;
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL;
	unsigned long flags;

	poll_wait(filp, &chn->event_wait, wait);
	spin_lock_irqsave(&dma->lock, flags);
	dma_xfer_msg =
	    ax_dma_find_entry_by_filp(&dma->dma_complite_list, filp);
	if (!dma_xfer_msg) {
		spin_unlock_irqrestore(&dma->lock, flags);
		return mask;
	}
	if (dma_xfer_msg
	    && (dma_xfer_msg->xfer_stat.stat & DMA_XFER_DONE)) {
		list_move_tail(&dma_xfer_msg->xfer_node, &dma->dma_free_list);
		mask = POLLIN | POLLRDNORM;
	}
	spin_unlock_irqrestore(&dma->lock, flags);
	if (chn->file_msg.issync)
		mutex_unlock(&chn->mutex_sync);
	return mask;
}

static int ax_dma_create_lli(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	s32 ret = 0;
	ax_dma_msg_t *dma_msg = &dma_xfer_msg->dma_msg;
	if (ax_dma_check_args(dma_msg)) {
		pr_err("err\n");
		ret = -EINVAL;
		goto err;
	}
	ret = ax_dma_set_lli_msg(dma_xfer_msg);
	if (ret) {
		pr_err("err\n");
		ret = -EINVAL;
		goto err;
	}
	dma_xfer_msg->xfer_stat.stat = DMA_XFER_IDLE;
	return 0;
err:
	dma_xfer_msg->xfer_stat.stat = -EINVAL;
	return ret;
}

static void ax_dma_free_checksum_list_by_id(int id, struct list_head *list)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL, *__dma_xfer_msg = NULL;

	list_for_each_entry_safe(dma_xfer_msg, __dma_xfer_msg,
				 list, xfer_node) {
		if (dma_xfer_msg->xfer_stat.id == id) {
			ax_dma_free_xfer_msg(dma_xfer_msg, 0);
		}
	}
}

static int ax_dma_create_xfer_buf(struct file *filp,
				     ax_dma_xfer_msg_t **dma_xfer_msg,
				     ax_dma_msg_t msg,
				     ax_dma_desc_t *msg_lli)
{
	s32 msg_size, ret = 0;
	long msg_lli_user;
	ax_dma_msg_t *dma_msg;
	ax_dma_xfer_msg_t *p_dma_xfer_msg = NULL;
	ax_dma_chn_t *chn = (ax_dma_chn_t *)filp->private_data;
	*dma_xfer_msg =
	    (void *)kzalloc(sizeof(ax_dma_xfer_msg_t), GFP_KERNEL);
	p_dma_xfer_msg = *dma_xfer_msg;
	INIT_LIST_HEAD(&p_dma_xfer_msg->dma_pool_list);
	if (unlikely(!p_dma_xfer_msg)) {
		pr_err("err: ax_dma error\n");
		return -ENOMEM;
	}
	p_dma_xfer_msg->filp = filp;
	dma_msg = &p_dma_xfer_msg->dma_msg;
	if (unlikely(IS_ERR_OR_NULL(dma_msg))) {
		ret = -EINVAL;
		pr_err("err\n");
		goto xfer_msg;
	}
	memcpy(dma_msg, &msg, sizeof(ax_dma_msg_t));
	if (dma_msg->lli_num < 1) {
		ret = -EINVAL;
		pr_err("err\n");
		goto xfer_msg;
	}
	msg_lli_user = (long) (dma_msg->msg_lli);
	if (dma_msg->dma_mode > AX_DMA_1D &&
	    dma_msg->dma_mode < AX_DMA_MEMORY_INIT) {
		msg_size = sizeof(ax_dma_xd_desc_t);
	} else {
		msg_size = sizeof(ax_dma_desc_t);
	}
	dma_msg->msg_lli =
	    kzalloc(msg_size * (dma_msg->lli_num), GFP_KERNEL);
	if (unlikely(!dma_msg->msg_lli)) {
		pr_err("err\n");
		p_dma_xfer_msg->xfer_stat.stat = -EFAULT;
		ret = -ENOMEM;
		goto xfer_msg;
	}
	if (msg_lli) {
		memcpy(dma_msg->msg_lli, msg_lli,
			msg_size * dma_msg->lli_num);
	} else {
		if (chn->kuser) {
			memcpy((void *)(dma_msg->msg_lli),
			      (void *)msg_lli_user,
			      msg_size * dma_msg->lli_num);
		} else if(unlikely(copy_from_user
			     ((void *)(dma_msg->msg_lli),
			      (void *)msg_lli_user,
			      msg_size * dma_msg->lli_num))) {
			pr_err("err\n");
			ret = -EIO;
			p_dma_xfer_msg->xfer_stat.stat = -EFAULT;
			goto err_msg_lli;
		}
	}
	return 0;
err_msg_lli:
	kfree_set_null(dma_msg->msg_lli);
xfer_msg:
	kfree_set_null(p_dma_xfer_msg);
	return ret;
}

static int ax_dma_creat_checksum_msg(struct file *filp,
					ax_dma_xfer_msg_t *dma_xfer_msg)
{
	int ret = 0;
	unsigned long flags;
	s32 num = 0, __num = 0, tmp_size = 0, step = 0;
	ax_dma_xfer_msg_t *check_xfer_msg = NULL;
	ax_dma_msg_t *dma_msg = &dma_xfer_msg->dma_msg;
	ax_dma_desc_t *msg_data = (ax_dma_desc_t *) dma_msg->msg_lli;
	ax_dma_desc_t *__msg_data;

	num = msg_data->size >> 23;
	tmp_size = msg_data->size & 0x7FFFFF;
	num += tmp_size / DMA_TR_2B_MAXSIZE;
	tmp_size %= DMA_TR_2B_MAXSIZE;
	num += DIV_ROUND_UP(tmp_size, DMA_TR_1B_MAXSIZE);
	dma_xfer_msg->xfer.sub_num = num - 1;

	tmp_size = msg_data->size;
	while (__num < num - 1) {
		ret =
		    ax_dma_create_xfer_buf(filp, &check_xfer_msg,
					      *dma_msg, msg_data);
		if (ret) {
			pr_err("err\n");
			return ret;
		}
		__msg_data =
		    (ax_dma_desc_t *)check_xfer_msg->dma_msg.msg_lli;
		__msg_data->src = msg_data->src + step;
		__msg_data->dst = msg_data->dst + step;
		if (tmp_size > DMA_TR_4B_MAXSIZE) {
			__msg_data->size = DMA_TR_4B_MAXSIZE;
		} else if (tmp_size > DMA_TR_2B_MAXSIZE) {
			__msg_data->size = DMA_TR_2B_MAXSIZE;
		} else if (tmp_size > DMA_TR_1B_MAXSIZE) {
			__msg_data->size = DMA_TR_1B_MAXSIZE;
		} else {
			__msg_data->size = tmp_size;
		}
		check_xfer_msg->xfer_stat.id = dma_xfer_msg->xfer_stat.id;
		check_xfer_msg->filp = dma_xfer_msg->filp;
		ret = ax_dma_create_lli(check_xfer_msg);
		if (ret) {
			pr_err("err\n");
			goto err;
		}
		spin_lock_irqsave(&dma->lock, flags);
		list_add_tail(&check_xfer_msg->xfer_node,
			      &dma->dma_checksum_cfg_list);
		spin_unlock_irqrestore(&dma->lock, flags);
		step += __msg_data->size;
		tmp_size -= __msg_data->size;
		__num++;
	}
	msg_data->size = tmp_size;
	msg_data->src += step;
	msg_data->dst += step;
	return 0;
err:
	ax_dma_free_xfer_buf(check_xfer_msg);
	ax_dma_free_checksum_list_by_id(dma_xfer_msg->xfer_stat.id,
					   &dma->dma_checksum_cfg_list);
	return ret;
}

static void ax_dma_checksum_list_remove_reverse(int id, int count)
{
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL, *__dma_xfer_msg = NULL;

	if (count) {
		list_for_each_entry_safe_reverse(dma_xfer_msg,
						 __dma_xfer_msg,
						 &dma->dma_checksum_cfg_list,
						 xfer_node) {
			if (dma_xfer_msg->xfer_stat.id == id) {
				list_del(&dma_xfer_msg->xfer_node);
				ax_dma_free_xfer_msg(dma_xfer_msg, 0);
				if (--count == 0)
					break;
			}
		}
	}
}

static int ax_dma_checksum_list_add(int id, int count)
{
	int __count = count;
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL, *__dma_xfer_msg = NULL;
	if (__count)
		list_for_each_entry_safe(dma_xfer_msg, __dma_xfer_msg,
					 &dma->dma_checksum_cfg_list,
					 xfer_node) {
		if (dma_xfer_msg->xfer_stat.id == id) {
			list_move_tail(&dma_xfer_msg->xfer_node,
				       &dma->dma_pending_list);
			if (--__count == 0)
				break;
		}
		}
	if (__count)
		return count - __count;
	else
		return 0;
}

static u32 ax_dma_cali_checksum(ax_dma_xfer_msg_t *dma_xfer_msg)
{
	ax_dma_xfer_msg_t *checksum_xfer_msg = NULL, *__checksum_xfer_msg =
	    NULL;
	ax_dma_xfer_stat_t *xfer_stat = NULL;
	u32 checksum = dma_xfer_msg->xfer_stat.checksum;
	s32 count = dma_xfer_msg->xfer.sub_num;

	list_for_each_entry_safe(checksum_xfer_msg, __checksum_xfer_msg,
				 &dma->dma_checksum_complite_list,
				 xfer_node) {
		xfer_stat = &checksum_xfer_msg->xfer_stat;
		if (xfer_stat->id == dma_xfer_msg->xfer_stat.id) {
			checksum += xfer_stat->checksum;
			list_del(&checksum_xfer_msg->xfer_node);
			ax_dma_free_xfer_msg(checksum_xfer_msg, 0);
			if (--count == 0)
				break;
		}
	}
	return checksum;
}

static long ax_dma_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	s32 id, ret = 0;
	ax_dma_chn_t *chn = NULL;
	ax_dma_xfer_msg_t *dma_xfer_msg = NULL;
	ax_dma_msg_t *dma_msg = NULL;
	ax_dma_msg_t tmp_dma_msg;
	ax_dma_stat_t dma_stat;
	unsigned long flags;

	if (!IS_ERR_OR_NULL(filp)) {
		chn = (ax_dma_chn_t *)filp->private_data;
	}

	switch (cmd) {
	case DMA_CFG_MEM_CHN_CMD:
		if (chn->kuser) {
			memcpy((void *)&tmp_dma_msg, (void *)arg,
			       sizeof(ax_dma_msg_t));
		} else if (unlikely(copy_from_user
			     ((void *)&tmp_dma_msg, (void *)arg,
			      sizeof(ax_dma_msg_t)))) {
			pr_err("err: ax_dma error\n");
			return -EIO;
		}
		ret =
		    ax_dma_create_xfer_buf(filp, &dma_xfer_msg,
					      tmp_dma_msg, NULL);
		if (ret) {
			pr_err("err\n");
			return ret;
		}
		dma_xfer_msg->dma_callback =
		    (int (*)(void *, unsigned int))tmp_dma_msg.pFun;
		dma_xfer_msg->arg = tmp_dma_msg.arg;
		dma_xfer_msg->kuser = chn->kuser;
		dma_msg = &dma_xfer_msg->dma_msg;
		dma_xfer_msg->xfer_stat.id =
		    ax_dma_get_id(dma, dma_xfer_msg);
		if (dma_xfer_msg->xfer_stat.id < 0) {
			pr_err("err\n");
			ret = -EIO;
			goto err;
		}
		if (dma_xfer_msg->dma_msg.dma_mode == AX_DMA_CHECKSUM) {
			if (dma_msg->lli_num != 1) {
				ret = -EINVAL;
				pr_err("err\n");
				goto err_id;
			}
			ret = ax_dma_creat_checksum_msg(filp, dma_xfer_msg);
			if (ret) {
				pr_err("err\n");
				goto err_id;
			}
		}
		ret = ax_dma_create_lli(dma_xfer_msg);
		if (ret) {
			pr_err("err\n");
			goto err_checksum;
		}
		ret = dma_xfer_msg->xfer_stat.id;
		break;
err_checksum:
		if (dma_xfer_msg->dma_msg.dma_mode == AX_DMA_CHECKSUM) {
			ax_dma_free_checksum_list_by_id
			    (dma_xfer_msg->xfer_stat.id,
			     &dma->dma_checksum_cfg_list);
		}
err_id:
		ax_dma_free_id(dma, dma_xfer_msg->xfer_stat.id);
err:
		ax_dma_free_xfer_buf(dma_xfer_msg);
		pr_err("err\n");
		break;
	case DMA_START_CHN_CMD:
		if (chn->kuser) {
			id = arg;
		} else if (unlikely
		    (copy_from_user((void *)(&id), (void *)arg, sizeof(s32)))) {
			pr_err("err: ax_dma error\n");
			return -EFAULT;
		}
		mutex_lock(&dma->mutex_idr);
		dma_xfer_msg =
		    (ax_dma_xfer_msg_t *) idr_find(&xfer_idr, id);
		mutex_unlock(&dma->mutex_idr);
		if (dma_xfer_msg) {
			atomic64_inc(&g_dbg_buf.total);
			atomic64_inc(&g_dbg_buf.current_total);
			dma_xfer_msg->time_start = ktime_get_real();
			dma_xfer_msg->ktime_start = ktime_get();
			dma_xfer_msg->xfer_stat.stat = DMA_XFER_WAIT;
			dma->id = id;
			if (chn->file_msg.issync)
				mutex_lock(&chn->mutex_sync);
			spin_lock_irqsave(&dma->lock, flags);
			list_add_tail(&dma_xfer_msg->xfer_node,
				      &dma->dma_pending_list);
			if (dma_xfer_msg->xfer.dma_mode ==
			    AX_DMA_CHECKSUM) {
				ret =
				    ax_dma_checksum_list_add(id,
								dma_xfer_msg->
								xfer.sub_num);
				if (ret) {
					pr_err("add list count err %d\n", ret);
					ax_dma_checksum_list_remove_reverse
					    (id, ret);
					ax_dma_free_xfer_msg(dma_xfer_msg,
								1);
					spin_unlock_irqrestore(&dma->lock,
							       flags);
					if (chn->file_msg.issync)
						mutex_unlock(&chn->mutex_sync);
					ret = -EFAULT;
					break;
				}
			}
			dma_xfer_msg->checksum_num =
			    dma_xfer_msg->xfer.sub_num + 1;

			ax_dma_start(dma_xfer_msg);
			spin_unlock_irqrestore(&dma->lock, flags);
		} else {
			ret = -EFAULT;
		}
		break;
	case DMA_GET_RESULT:
		if (unlikely
		    (copy_from_user((void *)(&dma_stat), (void *)arg,
				    sizeof(ax_dma_stat_t)))) {
			pr_err("err: ax_dma error\n");
			return -EFAULT;
		}
		spin_lock_irqsave(&dma->lock, flags);
		if (dma_stat.xfer_stat.id)
			dma_xfer_msg =
				ax_dma_find_entry_by_id(
					&dma->dma_free_list,
					dma_stat.xfer_stat.id);
		else
			dma_xfer_msg =
				ax_dma_find_entry_by_filp(
					&dma->dma_free_list,
					filp);
		if (!dma_xfer_msg) {
			spin_unlock_irqrestore(&dma->lock, flags);
			ret = -EFAULT;
			break;
		}
		dma_msg = &dma_xfer_msg->dma_msg;
		if (dma_xfer_msg->xfer.dma_mode == AX_DMA_CHECKSUM)
			dma_stat.xfer_stat.checksum =
				ax_dma_cali_checksum(dma_xfer_msg);
		dma_stat.pFun = dma_msg->pFun;
		dma_stat.arg = dma_msg->arg;
		list_del(&dma_xfer_msg->xfer_node);
		atomic64_dec(&g_dbg_buf.current_total);
		g_dbg_buf.last = (g_dbg_buf.last + 1) & 0x7;
		g_dbg_buf.info[g_dbg_buf.last].start =
		    do_div_quotient(dma_xfer_msg->time_start, 1000000000);
		g_dbg_buf.info[g_dbg_buf.last].wait =
		    dma_xfer_msg->ktime_run - dma_xfer_msg->ktime_start;
		g_dbg_buf.info[g_dbg_buf.last].run =
		    ktime_sub(ktime_get(), dma_xfer_msg->ktime_run);
		g_dbg_buf.info[g_dbg_buf.last].msg_id =
		    dma_xfer_msg->xfer_stat.id;
		g_dbg_buf.info[g_dbg_buf.last].size = dma_xfer_msg->size;
		g_dbg_buf.info[g_dbg_buf.last].dma_mode =
		    dma_xfer_msg->xfer.dma_mode;
		spin_unlock_irqrestore(&dma->lock, flags);
		ret = copy_to_user((void __user *)arg, &dma_stat,
				   sizeof(ax_dma_stat_t));
		ax_dma_free_xfer_msg(dma_xfer_msg, 1);
		break;
	case DMA_FREE_MSG_BY_ID:
		id = arg;
		dma_xfer_msg = ax_dma_find_entry_by_id(
				    &dma->dma_free_list, id);
		list_del(&dma_xfer_msg->xfer_node);
		atomic64_dec(&g_dbg_buf.current_total);
		g_dbg_buf.last = (g_dbg_buf.last + 1) & 0x7;
		g_dbg_buf.info[g_dbg_buf.last].start =
		    do_div_quotient(dma_xfer_msg->time_start, 1000000000);
		g_dbg_buf.info[g_dbg_buf.last].wait =
		    dma_xfer_msg->ktime_run - dma_xfer_msg->ktime_start;
		g_dbg_buf.info[g_dbg_buf.last].run =
		    ktime_sub(ktime_get(), dma_xfer_msg->ktime_run);
		g_dbg_buf.info[g_dbg_buf.last].msg_id =
		    dma_xfer_msg->xfer_stat.id;
		g_dbg_buf.info[g_dbg_buf.last].size = dma_xfer_msg->size;
		g_dbg_buf.info[g_dbg_buf.last].dma_mode =
		    dma_xfer_msg->xfer.dma_mode;
		ax_dma_free_xfer_msg(dma_xfer_msg, 1);
		break;
	default:
		pr_info("cmd err\n");
		break;
	}
	return ret;
}

int ax_dma_xfer(u64 src_addr, u64 dest_addr, u32 size,
		   int (*dma_callback) (void *, unsigned int), void *arg)
{
	struct inode *inode = kzalloc(sizeof(struct inode), GFP_KERNEL);
	struct file *filp = kzalloc(sizeof(struct file), GFP_KERNEL);
	ax_dma_msg_t dma_msg;
	ax_dma_desc_t msg_lli;
	u32 ret = 0, id = 0;

	if (!dma_callback)
		mutex_lock(&dma->kmutex);
	msg_lli.src = src_addr;
	msg_lli.dst = dest_addr;
	msg_lli.size = size;
	dma_msg.lli_num = 1;
	dma_msg.endian = 0;
	dma_msg.dma_mode = DMA_1D;
	dma_msg.msg_lli = &msg_lli;
	dma_msg.pFun =
	    (void (*)(ax_dma_xfer_stat_t *, void *))dma_callback;
	dma_msg.arg = arg;

	filp->private_data = (void *)1;
	ax_dma_open(inode, filp);
	id = ax_dma_ioctl(filp, DMA_CFG_MEM_CHN_CMD,
			     (unsigned long)&dma_msg);
	if (id > 0) {
		ret = ax_dma_ioctl(filp, DMA_START_CHN_CMD, id);
		if (ret) {
			pr_err("DMA_START_CHN_CMD err\n");
			ret = -1;
			goto err;
		}
	} else {
		ret = -1;
		pr_err("DMA_CFG_MEM_CHN_CMD err\n");
		goto err;
	}
	if (!dma_callback) {
		if (!wait_for_completion_timeout(&dma->complete, msecs_to_jiffies(5000))) {
			pr_err("timed out\n");
			ret = -ETIMEDOUT;
		}
		{
			struct file *filp = NULL;
			ax_dma_xfer_msg_t *dma_xfer_msg =
				(ax_dma_xfer_msg_t *) idr_find(&xfer_idr, id);
			if (dma_xfer_msg) {
				filp = dma_xfer_msg->filp;
				ret = ax_dma_ioctl(dma_xfer_msg->filp,
						DMA_FREE_MSG_BY_ID, id);
				if (ret) {
					pr_err("DMA_FREE_MSG_BY_ID err\n");
				}
				ax_dma_release(inode, filp);
				kfree_set_null(filp);
				kfree_set_null(inode);
			}
		}
	}
err:
	if (!dma_callback) {
		mutex_unlock(&dma->kmutex);
	}
	return ret;
}
EXPORT_SYMBOL(ax_dma_xfer);

int ax_dma_xfer_sync(u64 src_addr, u64 dest_addr, u32 size)
{
	return ax_dma_xfer(src_addr, dest_addr, size, NULL, NULL);
}
EXPORT_SYMBOL(ax_dma_xfer_sync);

int ax_dma_xfer_crop(ax_dma_xd_desc_t *xd_desc, u32 lli_num,
		    ax_dma_xfer_mode_t dma_mode, ax_dma_endian_t endian,
		    int (*dma_callback) (void *, unsigned int), void *arg)
{
	struct inode *inode = kzalloc(sizeof(struct inode), GFP_KERNEL);
	struct file *filp = kzalloc(sizeof(struct file), GFP_KERNEL);
	ax_dma_msg_t dma_msg;
	u32 ret = 0, id = 0;

	if (!dma_callback)
		mutex_unlock(&dma->kmutex);
	dma_msg.lli_num = lli_num;
	dma_msg.endian = endian;
	dma_msg.dma_mode = dma_mode;
	dma_msg.msg_lli = xd_desc;
	dma_msg.pFun =
		(void (*)(ax_dma_xfer_stat_t *, void *))dma_callback;

	filp->private_data = (void *)1;
	ax_dma_open(inode, filp);
	id = ax_dma_ioctl(filp, DMA_CFG_MEM_CHN_CMD,
			     (unsigned long)&dma_msg);
	if (id > 0) {
		ret = ax_dma_ioctl(filp, DMA_START_CHN_CMD, id);
		if (ret) {
			pr_err("DMA_START_CHN_CMD err\n");
			ret = -1;
			goto err;
		}
	} else {
		ret = -1;
		pr_err("DMA_CFG_MEM_CHN_CMD err\n");
		goto err;
	}
	if (!dma_callback) {
		if (!wait_for_completion_timeout(&dma->complete, msecs_to_jiffies(5000))) {
			pr_err("timed out\n");
			ret = -ETIMEDOUT;
		}
		{
			struct file *filp = NULL;
			ax_dma_xfer_msg_t *dma_xfer_msg =
				(ax_dma_xfer_msg_t *) idr_find(&xfer_idr, id);
			if (dma_xfer_msg) {
				filp = dma_xfer_msg->filp;
				ret = ax_dma_ioctl(dma_xfer_msg->filp,
						DMA_FREE_MSG_BY_ID, id);
				if (ret) {
					pr_err("DMA_FREE_MSG_BY_ID err\n");
				}
				ax_dma_release(inode, filp);
				kfree_set_null(filp);
				kfree_set_null(inode);
			}
		}
	}
err:
	if (!dma_callback) {
		mutex_unlock(&dma->kmutex);
	}
	return ret;
}
EXPORT_SYMBOL(ax_dma_xfer_crop);

int ax_dma_xfer_crop_sync(ax_dma_xd_desc_t *xd_desc, u32 lli_num,
		    ax_dma_xfer_mode_t dma_mode, ax_dma_endian_t endian)
{
	return ax_dma_xfer_crop(xd_desc, lli_num, dma_mode, endian, NULL, NULL);
}
EXPORT_SYMBOL(ax_dma_xfer_crop_sync);

static const struct file_operations ax_dma_fops = {
	.owner = THIS_MODULE,
	.open = ax_dma_open,
	.read = ax_dma_getsync,
	.write = ax_dma_setsync,
	.release = ax_dma_release,
	.unlocked_ioctl = ax_dma_ioctl,
	.poll = ax_dma_poll,
};

static struct miscdevice ax_dma_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = AX_DMA_DEVICE_NAME,
	.fops = &ax_dma_fops,
};

static s32 ax_dma_probe(struct platform_device *pdev)
{
	s32 err;
	struct resource *res = NULL;

	dma =
	    devm_kzalloc(&pdev->dev, sizeof(struct ax_dma_device),
			 GFP_KERNEL);
	if (unlikely(!dma))
		return -ENOMEM;
	dma->pdev = pdev;
	dma->irq = platform_get_irq(pdev, 0);
	if (unlikely(dma->irq < 0)) {
		pr_err("err: failed to get IRQ number\n");
		return dma->irq;
	}
	INIT_WORK(&dma->work_data.work, ax_dma_work_handler);
	err = devm_request_irq(&pdev->dev, dma->irq,
			       ax_dma_interrupt, IRQF_SHARED,
			       dev_name(&pdev->dev), dma);
	if (unlikely(err)) {
		pr_err("err: failed to request_irq\n");
		return err;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!res)) {
		pr_err("err: platform_get_resource error\n");
		return -ENOMEM;
	}

	dma->base = devm_ioremap_resource(&pdev->dev, res);
	if (unlikely(IS_ERR_OR_NULL(dma->base))) {
		pr_err("err\n");
		err = PTR_ERR(dma->base);
		return err;
	}

	dma->arst = devm_reset_control_get_optional(&pdev->dev, "dma-arst");
	if (IS_ERR_OR_NULL(dma->arst)) {
		pr_err("err\n");
		return PTR_ERR(dma->arst);
	}

	dma->prst = devm_reset_control_get_optional(&pdev->dev, "dma-prst");
	if (IS_ERR_OR_NULL(dma->prst)) {
		pr_err("err\n");
		return PTR_ERR(dma->prst);
	}

	dma->aclk = devm_clk_get(&pdev->dev, "dma-aclk");
	if (IS_ERR_OR_NULL(dma->aclk)) {
		pr_err("err\n");
		return PTR_ERR(dma->aclk);
	}

	dma->pclk = devm_clk_get(&pdev->dev, "dma-pclk");
	if (IS_ERR_OR_NULL(dma->pclk)) {
		pr_err("err\n");
		return PTR_ERR(dma->pclk);
	}
	ax_dma_clk_enable();
	/* Set the dma mask bits */
	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	dma->dma_pool = dmam_pool_create(KBUILD_MODNAME, &pdev->dev,
					       128, 64, 0);
	if (unlikely(!dma->dma_pool)) {
		pr_err("err: No memory for descriptors dma pool\n");
		return -ENOMEM;
	}
	err = ax_dma_proc_init(dma);
	if (unlikely(err)) {
		pr_err("err\n");
		return err;
	}
	err = misc_register(&ax_dma_miscdev);
	if (unlikely(err)) {
		pr_err("err: misc_register fail\n");
		goto err_register;
	}
	ax_dma_hw_reset();
	init_completion(&dma->complete);
	mutex_init(&dma->kmutex);
	mutex_init(&dma->mutex_idr);
	INIT_LIST_HEAD(&dma->dma_pending_list);
	INIT_LIST_HEAD(&dma->dma_complite_list);
	INIT_LIST_HEAD(&dma->dma_free_list);
	INIT_LIST_HEAD(&dma->dma_checksum_cfg_list);
	INIT_LIST_HEAD(&dma->dma_checksum_complite_list);
	spin_lock_init(&dma->lock);
	platform_set_drvdata(pdev, dma);
	atomic64_set(&g_dbg_buf.total, 0);
	atomic64_set(&g_dbg_buf.current_total, 0);
	pr_info("done\n");
	return 0;
err_register:
	pr_err("Err !\n");
	remove_proc_entry(AX_DMA_PROC_NAME, NULL);
	return err;
}

static s32 ax_dma_remove(struct platform_device *pdev)
{
	if (unlikely(!dma)) {
		return -EINVAL;
	}
	/* Disable DMA interrupt */
	ax_dma_disable_all_irq(dma);
	ax_dma_clk_disable();
	synchronize_irq(dma->irq);
	/* Disable hardware */
	remove_proc_entry(AX_DMA_PROC_NAME, NULL);
	misc_deregister(&ax_dma_miscdev);
	dmam_pool_destroy(dma->dma_pool);
	kfree_set_null(dma);
	return 0;
}

static const struct dev_pm_ops ax_dma_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ax_dma_suspend, ax_dma_resume)
};

static const struct of_device_id ax_dma_of_dev_id[] = {
	{
	 .compatible = "axera,dma",
	 },
	{}
};

MODULE_DEVICE_TABLE(of, dma_of_dev_id);

static struct platform_driver ax_dma_driver = {
	.probe = ax_dma_probe,
	.remove = ax_dma_remove,
	.driver = {
		   .name = "ax-dma",
		   .of_match_table = ax_dma_of_dev_id,
		   .pm = &ax_dma_pm_ops,
		   },
};

static int __init ax_dma_init(void)
{
	return platform_driver_register(&ax_dma_driver);
}
subsys_initcall_sync(ax_dma_init);

static void __exit ax_dma_exit(void)
{
	platform_driver_unregister(&ax_dma_driver);
}
module_exit(ax_dma_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Axera DMA Controller driver");
MODULE_AUTHOR("Axera");
