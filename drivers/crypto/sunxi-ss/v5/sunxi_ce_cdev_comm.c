/*
 * The driver of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2014 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/rng.h>
#include <crypto/internal/aead.h>
#include <crypto/hash.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/io.h>

#include "../sunxi_ce_cdev.h"
#include "sunxi_ss_reg.h"

#define NO_DMA_MAP		(0xE7)
#define SRC_DATA_DIR	(0)
#define DST_DATA_DIR	(0x1)

extern sunxi_ce_cdev_t	*ce_cdev;

void ce_print_task_desc(ce_task_desc_t *task)
{
	int i;
	u64 phy_addr;

#ifndef SUNXI_CE_DEBUG
	return;
#endif

	printk("---------------------task_info--------------------\n");
	printk("task->comm_ctl = 0x%x\n", task->comm_ctl);
	printk("task->sym_ctl = 0x%x\n", task->sym_ctl);
	printk("task->asym_ctl = 0x%x\n", task->asym_ctl);
	printk("task->key_addr = 0x%llx\n", (u64)ce_task_addr_get(task->key_addr));
	printk("task->iv_addr = 0x%llx\n", (u64)ce_task_addr_get(task->iv_addr));
	printk("task->ctr_addr = 0x%llx\n", (u64)ce_task_addr_get(task->ctr_addr));
	printk("task->data_len = 0x%x\n", task->data_len);


	for (i = 0; i < 8; i++) {
		phy_addr = (u64)ce_task_addr_get(task->ce_sg[i].src_addr);
		if (phy_addr) {
			printk("task->src[%d].addr = 0x%llx\n", i, phy_addr);
			printk("task->src[%d].len = 0x%x\n", i, task->ce_sg[i].src_len);
		}
	}

	for (i = 0; i < 8; i++) {
		phy_addr = (u64)ce_task_addr_get(task->ce_sg[i].dst_addr);
		if (phy_addr) {
			printk("task->dst[%d].addr = 0x%llx\n", i, phy_addr);
			printk("task->dst[%d].len = 0x%x\n", i, task->ce_sg[i].dst_len);
		}
	}
	printk("task->task_phy_addr = 0x%llx\n", (u64)task->task_phy_addr);
}


irqreturn_t sunxi_ce_irq_handler(int irq, void *dev_id)
{
	int i;
	int pending = 0;
	sunxi_ce_cdev_t *p_cdev = (sunxi_ce_cdev_t *)dev_id;
	unsigned long flags = 0;

	spin_lock_irqsave(&p_cdev->lock, flags);

	pending = ss_pending_get();
	SS_DBG("pending: %#x\n", pending);
	for (i = 0; i < SS_FLOW_NUM; i++) {
		if (pending & (CE_CHAN_PENDING << (2 * i))) {
			SS_DBG("Chan %d completed. pending: %#x\n", i, pending);
			ss_pending_clear(i);
			complete(&p_cdev->flows[i].done);
		}
	}

	spin_unlock_irqrestore(&p_cdev->lock, flags);
	return IRQ_HANDLED;
}

static int check_aes_ctx_vaild(crypto_aes_req_ctx_t *req)
{
	if (!req->src_buffer || !req->dst_buffer || !req->key_buffer) {
		SS_ERR("Invalid para: src = 0x%px, dst = 0x%px key = 0x%p\n",
				req->src_buffer, req->dst_buffer, req->key_buffer);
		return -EINVAL;
	}

	if (req->iv_length) {
		if (!req->iv_buf) {
			SS_ERR("Invalid para: iv_buf = 0x%px\n", req->iv_buf);
			return -EINVAL;
		}
	}

	SS_DBG("key_length = %d\n", req->key_length);
	if (req->key_length > AES_MAX_KEY_SIZE) {
		SS_ERR("Invalid para: key_length = %d\n", req->key_length);
		return -EINVAL;
	} else if (req->key_length < AES_MIN_KEY_SIZE) {
		SS_ERR("Invalid para: key_length = %d\n", req->key_length);
		return -EINVAL;
	}

	return 0;
}

static void ce_aes_config(crypto_aes_req_ctx_t *req, ce_task_desc_t *task)
{
	task->chan_id = req->channel_id;
	ss_method_set(req->dir, SS_METHOD_AES, task);
	ss_aes_mode_set(req->aes_mode, task);
}

static void task_iv_init(crypto_aes_req_ctx_t *req, ce_task_desc_t *task, int flag)
{
	if (req->iv_length) {
		if (flag == DMA_MEM_TO_DEV) {
			ss_iv_set(req->iv_buf, req->iv_length, task);
			req->iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buf,
									req->iv_length, DMA_MEM_TO_DEV);
			SS_DBG("iv = %px, iv_phy_addr = 0x%lx\n", req->iv_buf, req->iv_phy);
		} else if (flag == DMA_DEV_TO_MEM) {
			dma_unmap_single(ce_cdev->pdevice,
				req->iv_phy, req->iv_length, DMA_DEV_TO_MEM);
		} else if (flag == NO_DMA_MAP) {
			ce_iv_phyaddr_set(req->iv_phy, task);
			//task->iv_addr = (req->iv_phy >> WORD_ALGIN);
			SS_DBG("iv_phy_addr = 0x%lx\n", req->iv_phy);
		}
	}
	return;
}

static ce_task_desc_t *ce_alloc_task(void)
{
	dma_addr_t task_phy_addr;
	ce_task_desc_t *task;

	task = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &task_phy_addr);
	if (task == NULL) {
		SS_ERR("Failed to alloc for task\n");
		return NULL;
	} else {
		task->next_virt = NULL;
		task->task_phy_addr = task_phy_addr;
		SS_DBG("task = 0x%px task_phy = 0x%px\n", task, (void *)task_phy_addr);
	}

	return task;
}

static void ce_task_destroy(ce_task_desc_t *task)
{
	ce_task_desc_t *prev;

	while (task != NULL) {
		prev = task;
		task = task->next_virt;
		SS_DBG("prev = 0x%px, prev_phy = 0x%px\n", prev, (void *)prev->task_phy_addr);
		dma_pool_free(ce_cdev->task_pool, prev, prev->task_phy_addr);
	}
	return;
}

static int ce_task_data_init(crypto_aes_req_ctx_t *req, phys_addr_t src_phy,
						phys_addr_t dst_phy, u32 length, ce_task_desc_t *task)
{
	u32 block_size = TASK_MAX_DATA_SIZE;
	u32 block_num, alloc_flag = 0;
	u32 last_data_len, last_size;
	u32 data_len_offset = 0;
	u32 i = 0, n;
	dma_addr_t ptask_phy;
	dma_addr_t tmp_addr;
	dma_addr_t next_iv_phy = 0;
	ce_task_desc_t *ptask = task, *prev;

	block_num = length / block_size;
	last_size = length % block_size;
	ptask->data_len = 0;
	SS_DBG("total_len = 0x%x block_num =%d last_size =%d\n", length, block_num, last_size);
	while (length) {
		if (alloc_flag) {
			ptask = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &ptask_phy);
			if (ptask == NULL) {
				SS_ERR("Failed to alloc for ptask\n");
				return -ENOMEM;
			}
			ptask->chan_id  = prev->chan_id;
			ptask->comm_ctl = prev->comm_ctl;
			ptask->sym_ctl  = prev->sym_ctl;
			ptask->asym_ctl = prev->asym_ctl;
			ce_task_addr_set(NULL, ce_task_addr_get(prev->key_addr), ptask->key_addr);
			ce_task_addr_set(NULL, ce_task_addr_get(prev->iv_addr), ptask->iv_addr);
			ptask->data_len = 0;
			ce_task_addr_set(NULL, ce_task_addr_get(prev->next_task_addr), ptask->iv_addr);
			//prev->next = (ce_task_desc_t *)(ptask_phy >> WORD_ALGIN);
			prev->next_virt = ptask;
			ptask->task_phy_addr = ptask_phy;

			SS_DBG("ptask = 0x%px, ptask_phy = 0x%px\n", ptask, (void *)ptask_phy);

			if (SS_AES_MODE_CBC == req->aes_mode) {
				req->iv_phy = next_iv_phy;
				task_iv_init(req, ptask, NO_DMA_MAP);
			}
			i = 0;
		}

		if (block_num) {
			n = (block_num > 8) ? CE_SCATTERS_PER_TASK : block_num;
			for (i = 0; i < n; i++) {
				ce_task_addr_set(NULL, (src_phy + data_len_offset), ptask->ce_sg[i].src_addr);
				ptask->ce_sg[i].src_len = block_size;
				ce_task_addr_set(NULL, (dst_phy + data_len_offset), ptask->ce_sg[i].dst_addr);
				ptask->ce_sg[i].dst_len = block_size;
				ptask->data_len += block_size;
				data_len_offset += block_size;
			}
			block_num = block_num - n;
		}

		SS_DBG("block_num =%d i =%d\n", block_num, i);

		/*the last no engure block size*/
		if ((block_num == 0) && (last_size == 0)) {	/*block size aglin */
			ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
			alloc_flag = 0;
			//ptask->next = NULL;
			break;
		} else if ((block_num == 0) && (last_size != 0)) {
			SS_DBG("last_size =%d data_len_offset= %d\n", last_size, data_len_offset);
			/* not block size aglin */
			if ((i < CE_SCATTERS_PER_TASK) && (data_len_offset < length)) {
				last_data_len = length - data_len_offset;
				ce_task_addr_set(0, (src_phy + data_len_offset), ptask->ce_sg[i].src_addr);
				ptask->ce_sg[i].src_len = last_data_len;
				ce_task_addr_set(0, (dst_phy + data_len_offset), ptask->ce_sg[i].dst_addr);
				ptask->ce_sg[i].dst_len = last_data_len;
				ptask->data_len += last_data_len;
				ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
				//ptask->next = NULL;
				break;
			}
		}

		if (req->dir == SS_DIR_ENCRYPT) {
			SS_DBG("next_iv_phy = 0x%x\n", next_iv_phy);
			tmp_addr = ce_task_addr_get(ptask->ce_sg[7].dst_addr);
			tmp_addr = tmp_addr + ptask->ce_sg[i].dst_len - 16;
			ce_task_addr_set(0, tmp_addr, (u8 *)&next_iv_phy);
			SS_DBG("next_iv_phy = 0x%x\n", next_iv_phy);
		} else {
			tmp_addr = ce_task_addr_get(ptask->ce_sg[7].src_addr);
			tmp_addr = tmp_addr + ptask->ce_sg[7].src_len - 16;
			ce_task_addr_set(0, tmp_addr, (u8 *)&next_iv_phy);
		}
		alloc_flag = 1;
		prev = ptask;

	}
	return 0;
}

static int aes_crypto_start(crypto_aes_req_ctx_t *req, u8 *src_buffer,
							u32 src_length, u8 *dst_buffer)
{
	int ret = 0;
	int channel_id = req->channel_id;
	u32 padding_flag = ce_cdev->flows[req->channel_id].buf_pendding;
	phys_addr_t key_phy = 0;
	phys_addr_t src_phy = 0;
	phys_addr_t dst_phy = 0;
	ce_task_desc_t *task = NULL;

	task = ce_alloc_task();
	if (!task) {
		return -1;
	}

	/*task_mode_set*/
	ce_aes_config(req, task);

	/*task_key_set*/
	if (req->key_length) {
		key_phy = dma_map_single(ce_cdev->pdevice,
					req->key_buffer, req->key_length, DMA_MEM_TO_DEV);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer, (void *)key_phy);
		ss_key_set(req->key_buffer, req->key_length, task);
	}

	SS_DBG("ion_flag = %d padding_flag =%d", req->ion_flag, padding_flag);
	/*task_iv_set*/
	if (req->ion_flag && padding_flag) {
		task_iv_init(req, task, NO_DMA_MAP);
	} else {
		task_iv_init(req, task, DMA_MEM_TO_DEV);
	}

	/*task_data_set*/
	/*only the last src_buf is malloc*/
	if (req->ion_flag && (!padding_flag)) {
		src_phy = req->src_phy;
	} else {
		src_phy = dma_map_single(ce_cdev->pdevice, src_buffer, src_length, DMA_MEM_TO_DEV);
	}
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", src_buffer, (void *)src_phy);

	/*the dst_buf is from user*/
	if (req->ion_flag) {
		dst_phy = req->dst_phy;
	} else {
		dst_phy = dma_map_single(ce_cdev->pdevice, dst_buffer, src_length, DMA_MEM_TO_DEV);
	}
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", dst_buffer, (void *)dst_phy);

	ce_task_data_init(req, src_phy, dst_phy, src_length, task);
	ce_print_task_desc(task);

	/*start ce*/
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_start(task, SS_METHOD_AES, req->aes_mode);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_reg_print();
		ce_task_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
	}

	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	/*key*/
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice,
			key_phy, req->key_length, DMA_DEV_TO_MEM);
	}

	/*iv*/
	if (req->ion_flag && padding_flag) {
		;
	} else {
		task_iv_init(req, task, DMA_DEV_TO_MEM);
	}

	/*data*/
	if (req->ion_flag && (!padding_flag)) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				src_phy, src_length, DMA_DEV_TO_MEM);
	}

	if (req->ion_flag) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				dst_phy, src_length, DMA_DEV_TO_MEM);
	}

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		return -EINVAL;
	}

	return 0;
}

int do_aes_crypto(crypto_aes_req_ctx_t *req_ctx)
{
	u32 last_block_size = 0;
	u32 block_num = 0;
	u32 padding_size = 0;
	u32 first_encypt_size = 0;
	u8 data_block[AES_BLOCK_SIZE];
	int channel_id = req_ctx->channel_id;
	int ret;

	ret = check_aes_ctx_vaild(req_ctx);
	if (ret) {
		return -1;
	}

	memset(data_block, 0x0, AES_BLOCK_SIZE);
	ce_cdev->flows[channel_id].buf_pendding = 0;

	if (req_ctx->dir == SS_DIR_DECRYPT) {
		ret = aes_crypto_start(req_ctx, req_ctx->src_buffer,
								req_ctx->src_length, req_ctx->dst_buffer);
		if (ret) {
			SS_ERR("aes decrypt fail\n");
			return -2;
		}
		req_ctx->dst_length = req_ctx->src_length;
	} else {
		block_num = req_ctx->src_length / AES_BLOCK_SIZE;
		last_block_size = req_ctx->src_length % AES_BLOCK_SIZE;
		padding_size = AES_BLOCK_SIZE - last_block_size;

		if (block_num > 0) {
			SS_DBG("block_num = %d\n", block_num);
			first_encypt_size = block_num * AES_BLOCK_SIZE;
			SS_DBG("src_phy = 0x%lx, dst_phy = 0x%lx\n", req_ctx->src_phy, req_ctx->dst_phy);
			ret = aes_crypto_start(req_ctx, req_ctx->src_buffer,
						first_encypt_size,
						req_ctx->dst_buffer
						);
			if (ret) {
				SS_ERR("aes encrypt fail\n");
				return -2;
			}
			req_ctx->dst_length = block_num * AES_BLOCK_SIZE;
			/*not align 16byte*/
			if (last_block_size) {
				SS_DBG("last_block_size = %d\n", last_block_size);
				SS_DBG("padding_size = %d\n", padding_size);
				ce_cdev->flows[channel_id].buf_pendding = padding_size;
				if (req_ctx->ion_flag) {
					SS_ERR("ion memery must be 16 byte algin\n");
				} else {
					memcpy(data_block, req_ctx->src_buffer + first_encypt_size, last_block_size);
					memset(data_block + last_block_size, padding_size, padding_size);
				}
				if (SS_AES_MODE_CBC == req_ctx->aes_mode) {
					if (req_ctx->ion_flag) {
						req_ctx->iv_phy = req_ctx->dst_phy + first_encypt_size - AES_BLOCK_SIZE;
					} else {
						req_ctx->iv_buf = req_ctx->dst_buffer + first_encypt_size - AES_BLOCK_SIZE;
					}
				}

				ret = aes_crypto_start(req_ctx, data_block, AES_BLOCK_SIZE,
										req_ctx->dst_buffer + first_encypt_size
										);
				if (ret) {
					SS_ERR("aes encrypt fail\n");
					return -2;
				}
				req_ctx->dst_length = req_ctx->dst_length + AES_BLOCK_SIZE;
			}
		} else {
			SS_DBG("padding_size = %d\n", padding_size);
			ce_cdev->flows[channel_id].buf_pendding = padding_size;
			if (req_ctx->ion_flag) {
				SS_ERR("ion memery must be 16 byte algin\n");
			} else {
				memcpy(data_block, req_ctx->src_buffer, req_ctx->src_length);
				memset(data_block + last_block_size, padding_size, padding_size);
			}
			ret = aes_crypto_start(req_ctx, data_block, AES_BLOCK_SIZE,
									req_ctx->dst_buffer
									);
			if (ret) {
				SS_ERR("aes encrypt fail\n");
				return -2;
			}

			req_ctx->dst_length = (block_num + 1) * AES_BLOCK_SIZE;
		}
	}
	SS_DBG("do_aes_crypto sucess\n");
	return 0;
}

static int check_rsa_ctx_vaild(crypto_rsa_req_ctx_t *req)
{
	if (!req->sign_buffer || !req->pkey_buffer || !req->dst_buffer) {
		SS_ERR("Invalid para: src = 0x%px, dst = 0x%px key = 0x%p\n",
		       req->sign_buffer, req->pkey_buffer, req->dst_buffer);
		return -EINVAL;
	}

	if ((!req->sign_length) || (!req->pkey_length)) {
		SS_ERR("Invalid len: sign = 0x%x, pkey = 0x%x\n",
		       req->sign_length, req->pkey_length);
		return -EINVAL;
	}
	return 0;
}

static void ce_rsa_config(crypto_rsa_req_ctx_t *req, ce_task_desc_t *task)
{
	task->chan_id = req->channel_id;
	ss_method_set(req->dir, SS_METHOD_RSA, task);
	ss_rsa_width_set(req->rsa_width, task);
	task->comm_ctl |= 1 << 31;
}

static void ce_task_rsa_init(crypto_rsa_req_ctx_t *req, phys_addr_t pkey_phy,
			     phys_addr_t sign_phy, phys_addr_t dst_phy,
			     ce_task_desc_t *ptask)
{
	ulong data_len = 0;
	ce_task_addr_set(0, pkey_phy, ptask->ce_sg[0].src_addr);
	ptask->ce_sg[0].src_len = req->pkey_length;

	data_len += req->pkey_length;

	ce_task_addr_set(0, sign_phy, ptask->ce_sg[1].src_addr);
	ptask->ce_sg[1].src_len = req->sign_length;
	data_len += req->sign_length;

	ce_task_addr_set(0, dst_phy, ptask->ce_sg[0].dst_addr);
	ptask->ce_sg[0].dst_len = req->dst_length;
	ss_data_len_set(data_len, ptask);
}

void ce_rsa_task_print(crypto_rsa_req_ctx_t *rsa_ctx)
{
	crypto_rsa_req_ctx_t *rsa_req_ctx = rsa_ctx;
	pr_err("the rsa task");

	pr_err("[sign address] 0x%x\n", rsa_req_ctx->sign_buffer);
	pr_err("[sign length] %d\n", rsa_req_ctx->sign_length);

	pr_err("[pkey address] 0x%x\n", rsa_req_ctx->pkey_buffer);
	pr_err("[pkey length] %d\n", rsa_req_ctx->pkey_length);

	pr_err("[dst address] 0x%x\n", rsa_req_ctx->dst_buffer);
	pr_err("[dst length] %d\n", rsa_req_ctx->dst_length);

	pr_err("[dir] %d\n", rsa_req_ctx->dir);
	pr_err("[rsa width] %d\n", rsa_req_ctx->rsa_width);
	pr_err("[flag] %d\n", rsa_req_ctx->flag);
	pr_err("[channel_id] %d\n", rsa_req_ctx->channel_id);
}

void ce_rsa_task_description_print(ce_task_desc_t *task)
{
	int i;
	ulong addr;
	ce_task_desc_t *ptask = task;
	pr_err("the rsa task description");

	pr_err("[channel_id] 0x%x\n", ptask->chan_id);
	pr_err("[comm_ctl] 0x%x\n", ptask->comm_ctl);
	pr_err("[sym_ctl] 0x%x\n", ptask->sym_ctl);
	pr_err("[asym_ctl] 0x%x\n", ptask->asym_ctl);

	pr_err("[data_len] 0x%x\n", ptask->data_len);
	for (i = 0; i < 8; i++) {
		pr_err("[ce_sg%d src_addr] 0x%x\n", i,
		       *((ulong *)(ptask->ce_sg[i].src_addr)));
		pr_err("[ce_sg%d dst_addr] 0x%x\n", i,
		       *((ulong *)(ptask->ce_sg[i].dst_addr)));
		pr_err("[ce_sg%d src_len] 0x%x\n", i,
		       (ptask->ce_sg[i].src_len));
		pr_err("[ce_sg%d dst_len] 0x%x\n", i,
		       (ptask->ce_sg[i].dst_len));
	}
}

static int rsa_crypto_start(crypto_rsa_req_ctx_t *req)
{
	int ret = 0;
	int channel_id = req->channel_id;
	phys_addr_t pkey_phy = 0;
	phys_addr_t sign_phy = 0;
	phys_addr_t dst_phy = 0;
	ce_task_desc_t *task = NULL;

	task = ce_alloc_task();
	if (!task) {
		return -1;
	}

	/*task_set*/
	ce_rsa_config(req, task);

	pkey_phy = dma_map_single(ce_cdev->pdevice, req->pkey_buffer,
				  req->pkey_length, DMA_MEM_TO_DEV);
	SS_DBG("pkey = 0x%px, pkey_phy_addr = 0x%px\n", req->pkey_buffer,
	       (void *)pkey_phy);

	sign_phy = dma_map_single(ce_cdev->pdevice, req->sign_buffer,
				  req->sign_length, DMA_MEM_TO_DEV);
	SS_DBG("sign = 0x%px, sign_phy_addr = 0x%px\n", req->sign_buffer,
	       (void *)sign_phy);

	dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
				 req->dst_length, DMA_MEM_TO_DEV);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
	       (void *)dst_phy);

	ce_task_rsa_init(req, pkey_phy, sign_phy, dst_phy, task);
	/*start ce*/
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_start(task, SS_METHOD_RSA, 0);
	ss_pending_clear(channel_id);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_rsa_task_description_print(task);
		ce_rsa_task_print(req);
		ce_reg_print();
		ce_task_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
	}

	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	/*pkey*/
	dma_unmap_single(ce_cdev->pdevice, pkey_phy, req->pkey_length,
			 DMA_DEV_TO_MEM);
	/*data*/
	dma_unmap_single(ce_cdev->pdevice, sign_phy, req->sign_length,
			 DMA_DEV_TO_MEM);

	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
			 DMA_DEV_TO_MEM);

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n", ss_reg_rd(CE_REG_TSR),
	       ss_reg_rd(CE_REG_ERR));

	if (ss_flow_err(channel_id)) {
		ce_rsa_task_print(req);
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		return -EINVAL;
	}
	return 0;
}

int do_rsa_crypto(crypto_rsa_req_ctx_t *req_ctx)
{
	int ret;

	ret = check_rsa_ctx_vaild(req_ctx);
	if (ret) {
		return -1;
	}

	if (req_ctx->dir == SS_DIR_ENCRYPT) {
		SS_ERR("rsa decrypt fail\n");
		return -2;
	} else {
		ret = rsa_crypto_start(req_ctx);
		if (ret) {
			SS_ERR("rsa encrypt fail\n");
			return -2;
		}
	}
	SS_ERR("do_rsa_crypto sucess\n");
	return ret;
}

static int check_hash_ctx_vaild(crypto_hash_req_ctx_t *req)
{
	if (!req->text_buffer || !req->dst_buffer) {
		SS_ERR("Invalid para: text = 0x%px, dst = 0x%px\n",
		       req->text_buffer, req->dst_buffer);
		return -EINVAL;
	}

	if (!req->text_length) {
		SS_ERR("Invalid len: text = 0x%x\n", req->text_length);
		return -EINVAL;
	}
	if (req->key_length) {
		if (!req->key_buffer) {
			SS_ERR("Invalid para: key = 0x%px\n", req->key_buffer);
			return -EINVAL;
		}
	}
	if (req->iv_length) {
		if (!req->iv_buffer) {
			SS_ERR("Invalid para: key = 0x%px\n", req->iv_buffer);
			return -EINVAL;
		}
	}
	return 0;
}

static void ce_hash_config(crypto_hash_req_ctx_t *req,
			   ce_new_task_desc_t *task)
{
	ss_hash_method_set(req->hash_mode, task);
	ss_hash_cmd_set(req->channel_id, task);
}

static void ce_task_hash_init(crypto_hash_req_ctx_t *req, phys_addr_t key_phy,
			      phys_addr_t text_phy, phys_addr_t dst_phy,
			      phys_addr_t iv_phy, ce_new_task_desc_t *ptask)
{
	ulong total_bit_len = req->text_length * 8;
	ce_task_addr_set(0, text_phy, ptask->ce_sg[0].src_addr);
	ptask->ce_sg[0].src_len = req->text_length;

	ce_task_addr_set(0, dst_phy, ptask->ce_sg[0].dst_addr);
	ptask->ce_sg[0].dst_len = req->dst_length;

	memcpy(ptask->data_len, &total_bit_len, 4);

	if (req->key_length) {
		ce_task_addr_set(0, key_phy, ptask->key_addr);
	}
	if (req->key_length) {
		ce_task_addr_set(0, iv_phy, ptask->iv_addr);
	}
}

static ce_new_task_desc_t *ce_alloc_hash_task(void)
{
	dma_addr_t task_phy_addr;
	ce_new_task_desc_t *task;

	task = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &task_phy_addr);
	if (task == NULL) {
		SS_ERR("Failed to alloc for task\n");
		return NULL;
	} else {
		task->next_virt = NULL;
		task->task_phy_addr = task_phy_addr;
		SS_ERR("task = 0x%px task_phy = 0x%px\n", task,
		       (void *)task_phy_addr);
	}

	return task;
}

static void ce_task_hash_destroy(ce_new_task_desc_t *task)
{
	ce_new_task_desc_t *prev;

	while (task != NULL) {
		prev = task;
		task = task->next_virt;
		SS_DBG("prev = 0x%px, prev_phy = 0x%px\n", prev,
		       (void *)prev->task_phy_addr);
		dma_pool_free(ce_cdev->task_pool, prev, prev->task_phy_addr);
	}
	return;
}

void ce_hash_task_description_print(ce_new_task_desc_t *task)
{
	int i;
	ulong addr;
	ce_new_task_desc_t *ptask = task;
	pr_err("the hash task description");

	pr_err("[comm_ctl] 0x%lx\n", ((ulong)ptask->comm_ctl));
	pr_err("[main_cmd] 0x%lx\n", ((ulong)ptask->main_cmd));
	pr_err("[data_len] 0x%x\n", *((ulong *)ptask->data_len));

	pr_err("[data_len] 0x%x\n", ptask->data_len);
	for (i = 0; i < 8; i++) {
		pr_err("[ce_sg%d src_addr] 0x%x\n", i,
		       *((ulong *)(ptask->ce_sg[i].src_addr)));
		pr_err("[ce_sg%d dst_addr] 0x%x\n", i,
		       *((ulong *)(ptask->ce_sg[i].dst_addr)));
		pr_err("[ce_sg%d src_len] 0x%x\n", i,
		       (ptask->ce_sg[i].src_len));
		pr_err("[ce_sg%d dst_len] 0x%x\n", i,
		       (ptask->ce_sg[i].dst_len));
	}
}
int hash_crypto_start(crypto_hash_req_ctx_t *req)
{
	int ret = 0;
	phys_addr_t key_phy = 0;
	phys_addr_t text_phy = 0;
	phys_addr_t dst_phy = 0;
	phys_addr_t iv_phy = 0;
	ce_new_task_desc_t *task = NULL;
	int channel_id = req->channel_id;
	int i;
	task = ce_alloc_hash_task();
	if (!task) {
		return -1;
	}

	/*task_mode_set*/
	ce_hash_config(req, task);
	/*task_key_set*/
	if (req->key_length) {
		key_phy = dma_map_single(ce_cdev->pdevice, req->key_buffer,
					 req->key_length, DMA_MEM_TO_DEV);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer,
		       (void *)key_phy);
	}

	if (req->iv_length) {
		iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buffer,
					req->iv_length, DMA_MEM_TO_DEV);
		SS_DBG("iv = %px, iv_phy_addr = 0x%lx\n", req->iv_buffer,
		       iv_phy);
	}

	/*task_data_set*/
	/*only the last src_buf is malloc*/
	if (req->ion_flag) {
		text_phy = req->text_phy;
	} else {
		text_phy = dma_map_single(ce_cdev->pdevice, req->text_buffer,
					  req->text_length, DMA_MEM_TO_DEV);
	}
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", req->text_buffer,
	       (void *)text_phy);

	/*the dst_buf is from user*/
		dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
					 req->dst_length, DMA_MEM_TO_DEV);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
	       (void *)dst_phy);

	ce_task_hash_init(req, key_phy, text_phy, dst_phy, iv_phy, task);

	/*start ce*/
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_hash_start(task, req->hash_mode, 0);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_reg_print();
		ce_task_hash_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
		ce_hash_task_description_print(task);
		return ret;
	}

	ss_irq_disable(channel_id);
	ce_task_hash_destroy(task);

	/*key*/
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice, key_phy, req->key_length,
				 DMA_DEV_TO_MEM);
	}

	/*iv*/
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice, iv_phy, req->iv_length,
				 DMA_DEV_TO_MEM);
	}

	/*data*/
	if (req->ion_flag) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice, text_phy, req->text_length,
				 DMA_DEV_TO_MEM);
	}

	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
			 DMA_DEV_TO_MEM);

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		return -EINVAL;
	}
	return 0;
}


int do_hash_crypto(crypto_hash_req_ctx_t *req_ctx)
{
	int ret;
	ret = check_hash_ctx_vaild(req_ctx);
	if (ret) {
		return -1;
	}

	ret = hash_crypto_start(req_ctx);
	if (ret) {
		SS_ERR("hash encrypt fail\n");
		return -2;
	}

	SS_ERR("do_hash_crypto sucess\n");
	return 0;
}
