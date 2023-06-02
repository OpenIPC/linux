/*
 * Cryptographic API.
 *
 * Support for Novatek NA51089 Hash Hardware acceleration.
 *
 * Copyright (c) 2020 Novatek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/crypto.h>

#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>
#include <crypto/sha.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>

#include <plat/top.h>
#include "na51089_hash.h"

#define DRV_VERSION                   "1.00.00"

#define ALIGN_UP(x, align_to)         (((x) + ((align_to)-1)) & ~((align_to)-1))
#define ALIGN_DN(x, align_to)         ((x) & ~((align_to)-1))
#define NA51089_HASH_QUEUE_LENGTH     16
#define NA51089_HASH_ALG_PRIORITY     1000
#define NA51089_HASH_DEFAULT_TIMEOUT  3000          ///< 3 sec
#define NA51089_HASH_BUFFER_LEN       ((PAGE_SIZE/16)+128)

#define NA51089_HASH_DMA_ADDR_MASK    0xFFFFFFFF    ///< DMA support address bit[31..0], Max to 4GB size

#define NA51089_IV_BYTE_REVERSE(x)    ((((x)&0xff)<<24) | ((((x)>>8)&0xff)<<16) | ((((x)>>16)&0xff)<<8) | (((x)>>24)&0xff))

typedef enum {
    NA51089_HASH_TBUF_ALLOC_NONE = 0,     ///< disable
    NA51089_HASH_TBUF_ALLOC_BUDDY,        ///< from kernel buddy system
    NA51089_HASH_TBUF_ALLOC_MAX
} NA51089_HASH_TBUF_ALLOC_T;

static int tbuf_alloc = NA51089_HASH_TBUF_ALLOC_NONE;
module_param(tbuf_alloc, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(tbuf_alloc, "Hash temporarily buffer allocator => 0:None 1:Buddy");

static int tbuf_size = PAGE_SIZE;
module_param(tbuf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(tbuf_size, "Hash temporarily buffer size => Bytes");

static int mclk = -1;
module_param(mclk, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mclk, "Hash master clock => 0:240MHz 1:320MHz 2:Reserved 3:PLL9, -1 means from device tree");

static u32 plat_chip_id = CHIP_NA51089;

struct na51089_hash_ctx {
	struct na51089_hash_dev          *dev;
	u8                               key[SHA256_BLOCK_SIZE] __attribute__((aligned(sizeof(u32))));
};

struct na51089_hash_reqctx {
	NA51089_HASH_MODE_T              mode;

	bool                             first_blk;
	bool                             last_blk;

	u32                              block_size;
	u32                              digest_size;
	u32                              total;
	u32                              offset;

	u8                               digest[SHA256_DIGEST_SIZE] __attribute__((aligned(sizeof(u32))));

	u32                              bufcnt;
	u32                              buflen;
	u8                               buffer[NA51089_HASH_BUFFER_LEN] __attribute__((aligned(sizeof(u32))));
};

struct na51089_hash_dev {
	struct device                    *dev;
	struct clk                       *clk;
	void __iomem                     *iobase;
	int                              irq;
	spinlock_t                       lock;
	bool                             busy;

	struct timer_list                timer;

	struct crypto_queue	             queue;
	struct tasklet_struct            queue_tasklet;
	struct tasklet_struct            done_tasklet;

	struct ahash_request	         *req;
	struct scatterlist               sg_ctx;

	void                             *tbuf_ctx;         ///< DMA temporarily buffer
};

struct na51089_hash_result {
	struct completion                completion;
	int                              error;
};

static struct na51089_hash_dev *na51089_hdev = NULL;

static inline u32 na51089_hash_read(struct na51089_hash_dev *dd, u32 offset)
{
	return readl(dd->iobase + offset);
}

static inline void na51089_hash_write(struct na51089_hash_dev *dd, u32 offset, u32 value)
{
	writel(value, dd->iobase + offset);
}

static void na51089_hash_digest_complete(struct crypto_async_request *req, int error)
{
	struct na51089_hash_result *result = req->data;

	if (error == -EINPROGRESS)
		return;

	result->error = error;
	complete(&result->completion);
}

static void na51089_hash_complete(struct na51089_hash_dev *dev, int err)
{
	struct ahash_request       *req    = dev->req;
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	if ((err == 0) && reqctx->last_blk && req->result) {
		memcpy(req->result, reqctx->digest, reqctx->digest_size);
	}

	if (reqctx->last_blk) {
		reqctx->last_blk  = false;
		reqctx->first_blk = false;
	}

	req->base.complete(&req->base, err);
	dev->busy = false;
	dev->req  = NULL;
}

static void na51089_hash_reset(struct na51089_hash_dev *dd)
{
	u32 value = 0;
	u32 cnt   = 0;

	/* disable hash */
	na51089_hash_write(dd, NA51089_HASH_CFG_REG, 0);

	/* set reset, hardware will auto clear */
	na51089_hash_write(dd, NA51089_HASH_CFG_REG, 0x01);

	/* check reset done */
	while ((value = na51089_hash_read(dd, NA51089_HASH_CFG_REG)) & 0x1) {
		if(cnt++ >= 100)
			break;
		udelay(1);
	}

	/* clear all status */
	na51089_hash_write(dd, NA51089_HASH_INT_STS_REG, 0x3);

	if (value & 0x1)
		dev_err(dd->dev, "hash hardware reset failed!!\n");
}

static void na51089_hash_timeout_handler(struct timer_list *t)
{
	unsigned long flags;
	struct na51089_hash_dev *dev = from_timer(dev, t, timer);

	spin_lock_irqsave(&dev->lock, flags);

	if (dev->req) {
		struct na51089_hash_reqctx *reqctx = ahash_request_ctx(dev->req);

		dev_err(dev->dev, "hash hardware dma timeout!\n");

		if (dev->sg_ctx.length) {
			dma_unmap_sg(dev->dev, &dev->sg_ctx, 1, DMA_TO_DEVICE);    ///< cache nothing to do
		}

		/* clear context buffer count and reset sg buffer */
		reqctx->bufcnt = 0;
		sg_init_one(&dev->sg_ctx, reqctx->buffer, reqctx->bufcnt);

		na51089_hash_complete(dev, -EINVAL);
		dev->busy = true;

		/* reset device */
		na51089_hash_reset(dev);

		/* trigger to do next hash request in queue */
		tasklet_schedule(&dev->queue_tasklet);
	}
	else {
		dev_err(dev->dev, "hash timer timeout!\n");
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

static void na51089_hash_dma_start(struct na51089_hash_dev *dev)
{
	struct ahash_request       *req    = dev->req;
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	int                        dma_map = 0;
	int                        err;
	unsigned long              flags;
	u32                        reg_value;

	spin_lock_irqsave(&dev->lock, flags);

	/* check request size and prepare buffer */
	if (req->src && req->nbytes) {
		if ((reqctx->bufcnt + req->nbytes) <= reqctx->buflen) {
			scatterwalk_map_and_copy((reqctx->buffer+reqctx->bufcnt), req->src, 0, req->nbytes, 0);
			reqctx->offset  = req->nbytes;
			reqctx->bufcnt += req->nbytes;
		}
		else {
			scatterwalk_map_and_copy((reqctx->buffer+reqctx->bufcnt), req->src, 0, (reqctx->buflen - reqctx->bufcnt), 0);
			reqctx->offset = reqctx->buflen - reqctx->bufcnt;
			reqctx->bufcnt = reqctx->buflen;
		}
	}
	else {
		reqctx->offset = req->nbytes;
	}
	sg_init_one(&dev->sg_ctx, reqctx->buffer, reqctx->bufcnt);

	/* remain data will keep in buffer for next request */
	if ((reqctx->offset == req->nbytes) && !reqctx->last_blk) {
		na51089_hash_complete(dev, 0);
		goto exit;
	}

	/* check request ctx buffer physical address */
	if ((((u32)virt_to_phys(reqctx->buffer) + reqctx->bufcnt) & (~NA51089_HASH_DMA_ADDR_MASK))) {
		if (dev->tbuf_ctx && tbuf_size) {
			if (reqctx->bufcnt > tbuf_size) {
				dev_err(dev->dev, "temp ctx buffer size is small than request ctx size\n");
				err = -ENOMEM;
				goto error;
			}
			if (reqctx->bufcnt) {
				dev_dbg(dev->dev, "copy %d bytes from request ctx to temp ctx buffer(start)\n", reqctx->bufcnt);
				memcpy(dev->tbuf_ctx, reqctx->buffer, reqctx->bufcnt);
			}
			sg_init_one(&dev->sg_ctx, dev->tbuf_ctx, reqctx->bufcnt);
		}
		else {
			dev_err(dev->dev, "request ctx buffer paddr:0x%08x not support\n", (u32)virt_to_phys(reqctx->buffer));
			err = -ENOMEM;
			goto error;
		}
	}

	/* source dma mapping and cache clean */
	if (dev->sg_ctx.length) {
		err = dma_map_sg(dev->dev, &dev->sg_ctx, 1, DMA_TO_DEVICE);	    ///< direction => memory to device, cache clean, DMA input
		if (!err) {
			dev_err(dev->dev, "source scatterlist dma map error\n");
			err = -ENOMEM;
			goto error;
		}
		dma_map = 1;
	}

	dev_dbg(dev->dev, "Hash => mode:%d block_size:%d digest_size:%d\n", reqctx->mode, reqctx->block_size, reqctx->digest_size);
	dev_dbg(dev->dev, "Hash => first:%d last:%d total:%d\n", (reqctx->first_blk ? 1 : 0), (reqctx->last_blk ? 1 : 0), reqctx->total);
	dev_dbg(dev->dev, "Req  => req_nents:%d req_nbytes:%d\n", (req->src ? sg_nents(req->src) : 0), req->nbytes);
	dev_dbg(dev->dev, "Sg   => sg_nents :%d sg_nbytes:%d\n", sg_nents(&dev->sg_ctx), dev->sg_ctx.length);
	dev_dbg(dev->dev, "Src  => pa:0x%08x va:0x%08x size:%d\n", (u32)sg_dma_address(&dev->sg_ctx), (u32)sg_virt(&dev->sg_ctx), dev->sg_ctx.length);

	/* set iv */
	switch (reqctx->mode) {
	case NA51089_HASH_MODE_SHA1:
	case NA51089_HASH_MODE_HMAC_SHA1:
		if (!reqctx->first_blk) {
		    if (plat_chip_id == CHIP_NA51055) {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[0]));
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[1]));
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[2]));
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[3]));
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[4]));
			}
			else {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, ((u32 *)reqctx->digest)[0]);
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, ((u32 *)reqctx->digest)[1]);
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, ((u32 *)reqctx->digest)[2]);
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, ((u32 *)reqctx->digest)[3]);
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, ((u32 *)reqctx->digest)[4]);
			}
			na51089_hash_write(dev, NA51089_HASH_IV5_REG, 0);
			na51089_hash_write(dev, NA51089_HASH_IV6_REG, 0);
			na51089_hash_write(dev, NA51089_HASH_IV7_REG, 0);

			dev_dbg(dev->dev, "IV   => %08x %08x %08x %08x %08x\n", ((u32 *)reqctx->digest)[0], ((u32 *)reqctx->digest)[1],
			                                                        ((u32 *)reqctx->digest)[2], ((u32 *)reqctx->digest)[3],
			                                                        ((u32 *)reqctx->digest)[4]);
		}
		break;
	case NA51089_HASH_MODE_SHA256:
	case NA51089_HASH_MODE_HMAC_SHA256:
		if (!reqctx->first_blk) {
		    if (plat_chip_id == CHIP_NA51055) {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[0]));
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[1]));
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[2]));
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[3]));
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[4]));
			    na51089_hash_write(dev, NA51089_HASH_IV5_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[5]));
			    na51089_hash_write(dev, NA51089_HASH_IV6_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[6]));
			    na51089_hash_write(dev, NA51089_HASH_IV7_REG, NA51089_IV_BYTE_REVERSE(((u32 *)reqctx->digest)[7]));
			}
			else {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, ((u32 *)reqctx->digest)[0]);
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, ((u32 *)reqctx->digest)[1]);
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, ((u32 *)reqctx->digest)[2]);
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, ((u32 *)reqctx->digest)[3]);
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, ((u32 *)reqctx->digest)[4]);
			    na51089_hash_write(dev, NA51089_HASH_IV5_REG, ((u32 *)reqctx->digest)[5]);
			    na51089_hash_write(dev, NA51089_HASH_IV6_REG, ((u32 *)reqctx->digest)[6]);
			    na51089_hash_write(dev, NA51089_HASH_IV7_REG, ((u32 *)reqctx->digest)[7]);
			}

			dev_dbg(dev->dev, "IV   => %08x %08x %08x %08x %08x %08x %08x %08x\n", ((u32 *)reqctx->digest)[0], ((u32 *)reqctx->digest)[1],
			                                                                       ((u32 *)reqctx->digest)[2], ((u32 *)reqctx->digest)[3],
			                                                                       ((u32 *)reqctx->digest)[4], ((u32 *)reqctx->digest)[5],
			                                                                       ((u32 *)reqctx->digest)[6], ((u32 *)reqctx->digest)[7]);
		}
		break;
	default:
		dev_err(dev->dev, "unknown hash mode(%d)!\n", reqctx->mode);
		err = -EINVAL;
		goto error;
	}

	/* set key */
	if (reqctx->first_blk && (reqctx->mode == NA51089_HASH_MODE_HMAC_SHA1 || reqctx->mode == NA51089_HASH_MODE_HMAC_SHA256)) {
		na51089_hash_write(dev, NA51089_HASH_KEY0_REG,  ((u32 *)ctx->key)[0]);
		na51089_hash_write(dev, NA51089_HASH_KEY1_REG,  ((u32 *)ctx->key)[1]);
		na51089_hash_write(dev, NA51089_HASH_KEY2_REG,  ((u32 *)ctx->key)[2]);
		na51089_hash_write(dev, NA51089_HASH_KEY3_REG,  ((u32 *)ctx->key)[3]);
		na51089_hash_write(dev, NA51089_HASH_KEY4_REG,  ((u32 *)ctx->key)[4]);
		na51089_hash_write(dev, NA51089_HASH_KEY5_REG,  ((u32 *)ctx->key)[5]);
		na51089_hash_write(dev, NA51089_HASH_KEY6_REG,  ((u32 *)ctx->key)[6]);
		na51089_hash_write(dev, NA51089_HASH_KEY7_REG,  ((u32 *)ctx->key)[7]);
		na51089_hash_write(dev, NA51089_HASH_KEY8_REG,  ((u32 *)ctx->key)[8]);
		na51089_hash_write(dev, NA51089_HASH_KEY9_REG,  ((u32 *)ctx->key)[9]);
		na51089_hash_write(dev, NA51089_HASH_KEY10_REG, ((u32 *)ctx->key)[10]);
		na51089_hash_write(dev, NA51089_HASH_KEY11_REG, ((u32 *)ctx->key)[11]);
		na51089_hash_write(dev, NA51089_HASH_KEY12_REG, ((u32 *)ctx->key)[12]);
		na51089_hash_write(dev, NA51089_HASH_KEY13_REG, ((u32 *)ctx->key)[13]);
		na51089_hash_write(dev, NA51089_HASH_KEY14_REG, ((u32 *)ctx->key)[14]);
		na51089_hash_write(dev, NA51089_HASH_KEY15_REG, ((u32 *)ctx->key)[15]);

		dev_dbg(dev->dev, "KEY0 => %08x %08x %08x %08x\n", ((u32 *)ctx->key)[0],  ((u32 *)ctx->key)[1],
		                                                   ((u32 *)ctx->key)[2],  ((u32 *)ctx->key)[3]);
		dev_dbg(dev->dev, "KEY4 => %08x %08x %08x %08x\n", ((u32 *)ctx->key)[4],  ((u32 *)ctx->key)[5],
		                                                   ((u32 *)ctx->key)[6],  ((u32 *)ctx->key)[7]);
		dev_dbg(dev->dev, "KEY8 => %08x %08x %08x %08x\n", ((u32 *)ctx->key)[8],  ((u32 *)ctx->key)[9],
		                                                   ((u32 *)ctx->key)[10], ((u32 *)ctx->key)[11]);
		dev_dbg(dev->dev, "KEY12=> %08x %08x %08x %08x\n", ((u32 *)ctx->key)[12], ((u32 *)ctx->key)[13],
		                                                   ((u32 *)ctx->key)[14], ((u32 *)ctx->key)[15]);
	}

	/* set total hash length for hardware auto padding on last block */
	if (reqctx->last_blk && (reqctx->offset == req->nbytes))
		na51089_hash_write(dev, NA51089_HASH_PADLEN_REG, reqctx->total);
	else
		na51089_hash_write(dev, NA51089_HASH_PADLEN_REG, 0);

	/* set DMA address and length */
	na51089_hash_write(dev, NA51089_HASH_DMA_SRC_REG,     ALIGN_DN(sg_dma_address(&dev->sg_ctx), 4));
	na51089_hash_write(dev, NA51089_HASH_DMA_TX_SIZE_REG, dev->sg_ctx.length);

	/* clear DMA interrupt status */
	na51089_hash_write(dev, NA51089_HASH_INT_STS_REG, 0x1);

	/* enable DMA interrupt mask */
	na51089_hash_write(dev, NA51089_HASH_INT_ENB_REG, 0x1);

	/* set config and trigger DMA */
	reg_value = 0x2 | (reqctx->mode<<4)	| (0x1<<12);
	if (!reqctx->first_blk)
		reg_value |= (0x1<<8);		///< using HASH_IV_0 to HASH_IV_7 as IV
	if (reqctx->last_blk && (reqctx->offset == req->nbytes))
		reg_value |= (0x1<<13);		///< enable hardware auto padding for last block
	if (reqctx->mode == NA51089_HASH_MODE_HMAC_SHA1 || reqctx->mode == NA51089_HASH_MODE_HMAC_SHA256) {
		if (reqctx->first_blk)
			reg_value |= (0x1<<21);	///< enable hardware internal padding
		if (reqctx->last_blk && (reqctx->offset == req->nbytes))
			reg_value |= (0x1<<22);	///< enable hardware external padding
	}
	na51089_hash_write(dev, NA51089_HASH_CFG_REG, reg_value);

	reqctx->first_blk = false;

	/* start timeout timer */
	mod_timer(&dev->timer, jiffies+msecs_to_jiffies(NA51089_HASH_DEFAULT_TIMEOUT));

exit:
	spin_unlock_irqrestore(&dev->lock, flags);

	return;

error:
	if (dma_map) {
		dma_unmap_sg(dev->dev, &dev->sg_ctx, 1, DMA_TO_DEVICE);   ///< cache nothing to do
	}
	na51089_hash_complete(dev, err);

	spin_unlock_irqrestore(&dev->lock, flags);
}

static void na51089_hash_done_tasklet(unsigned long data)
{
	struct na51089_hash_dev    *dev = (struct na51089_hash_dev *)data;
	struct ahash_request       *req = dev->req;
	struct na51089_hash_reqctx *reqctx;
	u32                        *p_digest;
	u32                        reg_value;
	unsigned long              flags;
	int                        err = 0;

	spin_lock_irqsave(&dev->lock, flags);

	/* check request */
	if (!req) {
		dev_err(dev->dev, "hash request is empty!!\n");
		goto exit;
	}

	reqctx   = ahash_request_ctx(req);
	p_digest = (u32 *)reqctx->digest;

	/* store output digest data */
	p_digest[0] = na51089_hash_read(dev, NA51089_HASH_OUT0_REG);
	p_digest[1] = na51089_hash_read(dev, NA51089_HASH_OUT1_REG);
	p_digest[2] = na51089_hash_read(dev, NA51089_HASH_OUT2_REG);
	p_digest[3] = na51089_hash_read(dev, NA51089_HASH_OUT3_REG);
	p_digest[4] = na51089_hash_read(dev, NA51089_HASH_OUT4_REG);
	p_digest[5] = na51089_hash_read(dev, NA51089_HASH_OUT5_REG);
	p_digest[6] = na51089_hash_read(dev, NA51089_HASH_OUT6_REG);
	p_digest[7] = na51089_hash_read(dev, NA51089_HASH_OUT7_REG);

	dev_dbg(dev->dev, "OUT  => %08x %08x %08x %08x %08x %08x %08x %08x\n",
			p_digest[0], p_digest[1], p_digest[2], p_digest[3],
			p_digest[4], p_digest[5], p_digest[6], p_digest[7]);

	/* umap sg context buffer */
	if (dev->sg_ctx.length) {
		dma_unmap_sg(dev->dev, &dev->sg_ctx, 1,  DMA_TO_DEVICE);       ///< cache nothing to do
	}

	/* clear context buffer count and reset sg buffer */
	reqctx->bufcnt = 0;
	sg_init_one(&dev->sg_ctx, reqctx->buffer, reqctx->bufcnt);

	/* check request complete */
	if (reqctx->offset < req->nbytes) {
		if ((req->nbytes - reqctx->offset) <= reqctx->buflen) {
			scatterwalk_map_and_copy(reqctx->buffer, req->src, reqctx->offset, req->nbytes - reqctx->offset, 0);
			reqctx->bufcnt += (req->nbytes - reqctx->offset);
			reqctx->offset  = req->nbytes;
		}
		else {
			scatterwalk_map_and_copy(reqctx->buffer, req->src, reqctx->offset, reqctx->buflen, 0);
			reqctx->bufcnt  = reqctx->buflen;
			reqctx->offset += reqctx->buflen;
		}
		sg_init_one(&dev->sg_ctx, reqctx->buffer, reqctx->bufcnt);

		if ((reqctx->offset == req->nbytes) && !reqctx->last_blk)
			goto complete;	///< remain data will keep in buffer for next request

		/* check request ctx buffer physical address */
		if ((((u32)virt_to_phys(reqctx->buffer) + reqctx->bufcnt) & (~NA51089_HASH_DMA_ADDR_MASK))) {
			if (dev->tbuf_ctx && tbuf_size) {
				if (reqctx->bufcnt > tbuf_size) {
					dev_err(dev->dev, "temp ctx buffer size is small than request ctx size\n");
					err = -ENOMEM;
					goto complete;
				}
				if (reqctx->bufcnt) {
					dev_dbg(dev->dev, "copy %d bytes from request ctx to temp ctx buffer(done)\n", reqctx->bufcnt);
					memcpy(dev->tbuf_ctx, reqctx->buffer, reqctx->bufcnt);
				}
				sg_init_one(&dev->sg_ctx, dev->tbuf_ctx, reqctx->bufcnt);
			}
			else {
				dev_err(dev->dev, "request ctx buffer paddr:0x%08x not support\n", (u32)virt_to_phys(reqctx->buffer));
				err = -ENOMEM;
				goto complete;
			}
		}

		/* source dma mapping and cache clean */
		if (dev->sg_ctx.length) {
			err = dma_map_sg(dev->dev, &dev->sg_ctx, 1, DMA_TO_DEVICE);	    ///< direction => memory to device, cache clean, DMA input
			if (!err) {
				dev_err(dev->dev, "source scatterlist dma map error\n");
				err = -ENOMEM;
				goto complete;
			}
		}

		/* update IV */
		switch (reqctx->mode) {
		case NA51089_HASH_MODE_SHA1:
		case NA51089_HASH_MODE_HMAC_SHA1:
		    if (plat_chip_id == CHIP_NA51055) {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, NA51089_IV_BYTE_REVERSE(p_digest[0]));
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, NA51089_IV_BYTE_REVERSE(p_digest[1]));
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, NA51089_IV_BYTE_REVERSE(p_digest[2]));
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, NA51089_IV_BYTE_REVERSE(p_digest[3]));
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, NA51089_IV_BYTE_REVERSE(p_digest[4]));
			}
			else {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, p_digest[0]);
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, p_digest[1]);
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, p_digest[2]);
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, p_digest[3]);
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, p_digest[4]);
			}
			dev_dbg(dev->dev, "NIV  => %08x %08x %08x %08x %08x\n", p_digest[0], p_digest[1], p_digest[2], p_digest[3], p_digest[4]);
			break;
		case NA51089_HASH_MODE_SHA256:
		case NA51089_HASH_MODE_HMAC_SHA256:
		    if (plat_chip_id == CHIP_NA51055) {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, NA51089_IV_BYTE_REVERSE(p_digest[0]));
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, NA51089_IV_BYTE_REVERSE(p_digest[1]));
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, NA51089_IV_BYTE_REVERSE(p_digest[2]));
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, NA51089_IV_BYTE_REVERSE(p_digest[3]));
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, NA51089_IV_BYTE_REVERSE(p_digest[4]));
			    na51089_hash_write(dev, NA51089_HASH_IV5_REG, NA51089_IV_BYTE_REVERSE(p_digest[5]));
			    na51089_hash_write(dev, NA51089_HASH_IV6_REG, NA51089_IV_BYTE_REVERSE(p_digest[6]));
			    na51089_hash_write(dev, NA51089_HASH_IV7_REG, NA51089_IV_BYTE_REVERSE(p_digest[7]));
			}
			else {
			    na51089_hash_write(dev, NA51089_HASH_IV0_REG, p_digest[0]);
			    na51089_hash_write(dev, NA51089_HASH_IV1_REG, p_digest[1]);
			    na51089_hash_write(dev, NA51089_HASH_IV2_REG, p_digest[2]);
			    na51089_hash_write(dev, NA51089_HASH_IV3_REG, p_digest[3]);
			    na51089_hash_write(dev, NA51089_HASH_IV4_REG, p_digest[4]);
			    na51089_hash_write(dev, NA51089_HASH_IV5_REG, p_digest[5]);
			    na51089_hash_write(dev, NA51089_HASH_IV6_REG, p_digest[6]);
			    na51089_hash_write(dev, NA51089_HASH_IV7_REG, p_digest[7]);
			}
			dev_dbg(dev->dev, "NIV  => %08x %08x %08x %08x %08x %08x %08x %08x\n", p_digest[0], p_digest[1], p_digest[2], p_digest[3], p_digest[4], p_digest[5], p_digest[6], p_digest[7]);
			break;
		default:
			break;
		}

		/* set total hash length for hardware auto padding on last block */
		if (reqctx->last_blk && (reqctx->offset == req->nbytes))
			na51089_hash_write(dev, NA51089_HASH_PADLEN_REG, reqctx->total);
		else
			na51089_hash_write(dev, NA51089_HASH_PADLEN_REG, 0);

		/* set DMA address and length */
		na51089_hash_write(dev, NA51089_HASH_DMA_SRC_REG,     ALIGN_DN(sg_dma_address(&dev->sg_ctx), 4));
		na51089_hash_write(dev, NA51089_HASH_DMA_TX_SIZE_REG, dev->sg_ctx.length);

		/* clear DMA interrupt status */
		na51089_hash_write(dev, NA51089_HASH_INT_STS_REG, 0x1);

		/* set config and trigger DMA */
		reg_value = 0x2 | (reqctx->mode<<4)	| (0x1<<8) | (0x1<<12);
		if (reqctx->last_blk && (reqctx->offset == req->nbytes)) {
			reg_value |= (0x1<<13);		///< enable hardware auto padding for last block
			if (reqctx->mode == NA51089_HASH_MODE_HMAC_SHA1 || reqctx->mode == NA51089_HASH_MODE_HMAC_SHA256) {
				reg_value |= (0x1<<22);	///< enable hardware external padding
			}
		}
		na51089_hash_write(dev, NA51089_HASH_CFG_REG, reg_value);

		/* start timeout timer */
		mod_timer(&dev->timer, jiffies+msecs_to_jiffies(NA51089_HASH_DEFAULT_TIMEOUT));

		dev_dbg(dev->dev, "Src  => pa:0x%08x va:0x%08x size:%d\n", (u32)sg_dma_address(&dev->sg_ctx), (u32)sg_virt(&dev->sg_ctx), dev->sg_ctx.length);
		dev_dbg(dev->dev, "trigger next DMA transfer!\n");
	}
	else {
		err = (reqctx->offset == req->nbytes) ? 0 : -EINVAL;
complete:
		na51089_hash_complete(dev, err);
		dev->busy = true;
		tasklet_schedule(&dev->queue_tasklet);  ///< trigger to do next hash request in queue
	}

exit:
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void na51089_hash_queue_tasklet(unsigned long data)
{
	struct na51089_hash_dev     *dev = (struct na51089_hash_dev *)data;
	struct crypto_async_request *async_req, *backlog;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);

	backlog   = crypto_get_backlog(&dev->queue);
	async_req = crypto_dequeue_request(&dev->queue);

	if (!async_req) {
		dev->busy = false;
		spin_unlock_irqrestore(&dev->lock, flags);
		return;
	}

	if (backlog) {
		backlog->complete(backlog, -EINPROGRESS);
	}

	/* assign new request to device */
	dev->req = ahash_request_cast(async_req);

	spin_unlock_irqrestore(&dev->lock, flags);

	na51089_hash_dma_start(dev);
}

static irqreturn_t na51089_hash_irq(int irq, void *dev_id)
{
	struct na51089_hash_dev *dev = (struct na51089_hash_dev *)dev_id;
	u32                     status;

	spin_lock(&dev->lock);

	/* read DMA status */
	status = na51089_hash_read(dev, NA51089_HASH_INT_STS_REG);

	/* clear DMA status */
	na51089_hash_write(dev, NA51089_HASH_INT_STS_REG, status);

	/* DMA transfer done */
	if (status & 0x1) {
		del_timer(&dev->timer);
		tasklet_schedule(&dev->done_tasklet);
	}

	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

static int na51089_hash_handle_req(struct na51089_hash_dev *dev, struct ahash_request *req)
{
	unsigned long flags;
	int err;

	spin_lock_irqsave(&dev->lock, flags);
	err = ahash_enqueue_request(&dev->queue, req);
	if (dev->busy) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return err;
	}
	dev->busy = true;
	spin_unlock_irqrestore(&dev->lock, flags);

	tasklet_schedule(&dev->queue_tasklet);

	return err;
}

static int na51089_hash_cra_init(struct crypto_tfm *tfm)
{
	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm), sizeof(struct na51089_hash_reqctx));
	return 0;
}

static void na51089_hash_cra_exit(struct crypto_tfm *tfm)
{
	return;
}

static int na51089_hash_setkey(struct crypto_ahash *tfm, const u8 *key, unsigned int keylen)
{
	struct na51089_hash_ctx    *ctx       = crypto_tfm_ctx(&tfm->base);
	unsigned int               blocksize  = crypto_tfm_alg_blocksize(crypto_ahash_tfm(tfm));
	unsigned int               digestsize = crypto_ahash_digestsize(tfm);
	const char                 *alg_name;
	struct crypto_ahash        *ahash_tfm;
	struct ahash_request       *req;
	struct na51089_hash_result result;
	struct scatterlist         sg;
	u8                         *buf;
	int                        ret;

	/* setup device */
	if (!ctx->dev) {
		ctx->dev = na51089_hdev;
	}

	/* clear key */
	memset(ctx->key, 0, sizeof(ctx->key));

	dev_dbg(ctx->dev->dev, "Hash => keylen:%d block_size:%d digest_size:%d\n", keylen, blocksize, digestsize);

	if (keylen <= blocksize) {
		memcpy(ctx->key, key, keylen);
		return 0;
	}

	if (digestsize == SHA1_DIGEST_SIZE)
		alg_name = "na51089-sha1";
	else if (digestsize == SHA256_DIGEST_SIZE)
		alg_name = "na51089-sha256";
	else
		return -EINVAL;

	ahash_tfm = crypto_alloc_ahash(alg_name, CRYPTO_ALG_TYPE_AHASH, CRYPTO_ALG_TYPE_AHASH_MASK);
	if (IS_ERR(ahash_tfm))
		return PTR_ERR(ahash_tfm);

	req = ahash_request_alloc(ahash_tfm, GFP_KERNEL);
	if (!req) {
		ret = -ENOMEM;
		goto err_free_ahash;
	}

	init_completion(&result.completion);
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG, na51089_hash_digest_complete, &result);
	crypto_ahash_clear_flags(ahash_tfm, ~0);

	buf = kzalloc(keylen, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto err_free_req;
	}

	memcpy(buf, key, keylen);
	sg_init_one(&sg, buf, keylen);
	ahash_request_set_crypt(req, &sg, ctx->key, keylen);

	ret = crypto_ahash_digest(req);
	if (ret == -EINPROGRESS || ret == -EBUSY) {
		ret = wait_for_completion_interruptible(&result.completion);
		if (!ret)
			ret = result.error;
	}

	if (ret)
		crypto_ahash_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);

	kfree(buf);

err_free_req:
	ahash_request_free(req);
err_free_ahash:
	crypto_free_ahash(ahash_tfm);

	return ret;
}

static int na51089_hash_init(struct ahash_request *req)
{
	struct crypto_tfm          *tfm    = crypto_ahash_tfm(crypto_ahash_reqtfm(req));
	struct crypto_alg          *alg    = tfm->__crt_alg;
	struct crypto_ahash        *ahash  = crypto_ahash_reqtfm(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(ahash);
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	/* setup device */
	if (!ctx->dev) {
		ctx->dev = na51089_hdev;
	}

	/* setup operation */
	switch (crypto_ahash_digestsize(ahash)) {
	case SHA1_DIGEST_SIZE:
		if (strcmp(alg->cra_name, "hmac(sha1)") == 0)
			reqctx->mode = NA51089_HASH_MODE_HMAC_SHA1;
		else
			reqctx->mode = NA51089_HASH_MODE_SHA1;
		reqctx->block_size  = SHA1_BLOCK_SIZE;
		reqctx->digest_size = SHA1_DIGEST_SIZE;
		break;
	case SHA256_DIGEST_SIZE:
		if (strcmp(alg->cra_name, "hmac(sha256)") == 0)
			reqctx->mode = NA51089_HASH_MODE_HMAC_SHA256;
		else
			reqctx->mode = NA51089_HASH_MODE_SHA256;
		reqctx->block_size  = SHA256_BLOCK_SIZE;
		reqctx->digest_size = SHA256_DIGEST_SIZE;
		break;
	default:
		return -EINVAL;
	}

	reqctx->total     = 0;
	reqctx->first_blk = true;
	reqctx->last_blk  = false;
	reqctx->bufcnt    = 0;
	reqctx->buflen    = ALIGN_DN(NA51089_HASH_BUFFER_LEN, reqctx->block_size);

	if (!reqctx->buflen)
		return -EINVAL;

	return 0;
}

static int na51089_hash_update(struct ahash_request *req)
{
	struct crypto_ahash        *ahash  = crypto_ahash_reqtfm(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(ahash);
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	if (!req->nbytes)
		return 0;

	reqctx->total += req->nbytes;

	return na51089_hash_handle_req(ctx->dev, req);
}

static int na51089_hash_final(struct ahash_request *req)
{
	struct crypto_ahash        *ahash  = crypto_ahash_reqtfm(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(ahash);
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	req->nbytes      = 0;
	reqctx->last_blk = true;

	return na51089_hash_handle_req(ctx->dev, req);
}

static int na51089_hash_finup(struct ahash_request *req)
{
	struct crypto_ahash        *ahash  = crypto_ahash_reqtfm(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(ahash);
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	reqctx->last_blk = true;
	reqctx->total   += req->nbytes;

	return na51089_hash_handle_req(ctx->dev, req);
}

static int na51089_hash_digest(struct ahash_request *req)
{
	int ret;
	struct crypto_ahash        *ahash  = crypto_ahash_reqtfm(req);
	struct na51089_hash_ctx    *ctx    = crypto_ahash_ctx(ahash);
	struct na51089_hash_reqctx *reqctx = ahash_request_ctx(req);

	ret = na51089_hash_init(req);
	if (ret)
		return ret;

	reqctx->first_blk = true;
	reqctx->last_blk  = true;
	reqctx->total    += req->nbytes;

	return na51089_hash_handle_req(ctx->dev, req);
}

static int na51089_hash_export(struct ahash_request *req, void *out)
{
	const struct na51089_hash_reqctx *ctx = ahash_request_ctx(req);

	memcpy(out, ctx, sizeof(*ctx));
	return 0;
}

static int na51089_hash_import(struct ahash_request *req, const void *in)
{
	struct na51089_hash_reqctx *ctx = ahash_request_ctx(req);

	memcpy(ctx, in, sizeof(*ctx));
	return 0;
}

static struct ahash_alg na51089_hash_algs[] = {
	{
		.init       = na51089_hash_init,
		.update     = na51089_hash_update,
		.final      = na51089_hash_final,
		.finup      = na51089_hash_finup,
		.digest     = na51089_hash_digest,
		.export		= na51089_hash_export,
		.import		= na51089_hash_import,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize  = sizeof(struct na51089_hash_reqctx),
			.base   = {
				.cra_name        = "sha1",
				.cra_driver_name = "na51089-sha1",
				.cra_priority    = NA51089_HASH_ALG_PRIORITY,
				.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize   = SHA1_BLOCK_SIZE,
				.cra_ctxsize     = sizeof(struct na51089_hash_ctx),
				.cra_alignmask   = 0x3,
				.cra_type        = &crypto_ahash_type,
				.cra_module      = THIS_MODULE,
				.cra_init        = na51089_hash_cra_init,
				.cra_exit        = na51089_hash_cra_exit,
			}
		}
	},
	{
		.init       = na51089_hash_init,
		.update     = na51089_hash_update,
		.final      = na51089_hash_final,
		.finup      = na51089_hash_finup,
		.digest     = na51089_hash_digest,
		.export		= na51089_hash_export,
		.import		= na51089_hash_import,
		.halg = {
			.digestsize = SHA256_DIGEST_SIZE,
			.statesize  = sizeof(struct na51089_hash_reqctx),
			.base   = {
				.cra_name        = "sha256",
				.cra_driver_name = "na51089-sha256",
				.cra_priority    = NA51089_HASH_ALG_PRIORITY,
				.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize   = SHA256_BLOCK_SIZE,
				.cra_ctxsize     = sizeof(struct na51089_hash_ctx),
				.cra_alignmask   = 0x3,
				.cra_type        = &crypto_ahash_type,
				.cra_module      = THIS_MODULE,
				.cra_init        = na51089_hash_cra_init,
				.cra_exit        = na51089_hash_cra_exit,
			}
		}
	},
	{
		.init		= na51089_hash_init,
		.update		= na51089_hash_update,
		.final		= na51089_hash_final,
		.finup		= na51089_hash_finup,
		.digest		= na51089_hash_digest,
		.export		= na51089_hash_export,
		.import		= na51089_hash_import,
		.setkey		= na51089_hash_setkey,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize  = sizeof(struct na51089_hash_reqctx),	///< size => 1 ~ (PAGE_SIZE/8)
			.base   = {
				.cra_name        = "hmac(sha1)",
				.cra_driver_name = "na51089-hmac-sha1",
				.cra_priority    = NA51089_HASH_ALG_PRIORITY,
				.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize   = SHA1_BLOCK_SIZE,
				.cra_ctxsize     = sizeof(struct na51089_hash_ctx),
				.cra_alignmask   = 0x3,
				.cra_type        = &crypto_ahash_type,
				.cra_module      = THIS_MODULE,
				.cra_init        = na51089_hash_cra_init,
				.cra_exit        = na51089_hash_cra_exit,
			}
		}
	},
	{
		.init		= na51089_hash_init,
		.update		= na51089_hash_update,
		.final		= na51089_hash_final,
		.finup		= na51089_hash_finup,
		.digest		= na51089_hash_digest,
		.export		= na51089_hash_export,
		.import		= na51089_hash_import,
		.setkey		= na51089_hash_setkey,
		.halg = {
			.digestsize = SHA256_DIGEST_SIZE,
			.statesize  = sizeof(struct na51089_hash_reqctx),
			.base   = {
				.cra_name        = "hmac(sha256)",
				.cra_driver_name = "na51089-hmac-sha256",
				.cra_priority    = NA51089_HASH_ALG_PRIORITY,
				.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
				.cra_blocksize   = SHA256_BLOCK_SIZE,
				.cra_ctxsize     = sizeof(struct na51089_hash_ctx),
				.cra_alignmask   = 0x3,
				.cra_type        = &crypto_ahash_type,
				.cra_module      = THIS_MODULE,
				.cra_init        = na51089_hash_cra_init,
				.cra_exit        = na51089_hash_cra_exit,
			}
		}
	}
};

static int na51089_hash_register_algs(struct na51089_hash_dev *dev)
{
	int i, err;

	for (i=0; i<ARRAY_SIZE(na51089_hash_algs); i++) {
		err = crypto_register_ahash(&na51089_hash_algs[i]);
		if (err)
			goto err_reg;
	}
	return 0;

err_reg:
	for (; i--; )
		crypto_unregister_ahash(&na51089_hash_algs[i]);

	return err;
}

static int na51089_hash_unregister_algs(struct na51089_hash_dev *dev)
{
	int i;

	for (i=0; i<ARRAY_SIZE(na51089_hash_algs); i++)
		crypto_unregister_ahash(&na51089_hash_algs[i]);

	return 0;
}

static int na51089_hash_probe(struct platform_device *pdev)
{
	int    ret;
	struct resource         *res;
	struct na51089_hash_dev *dd;
	struct clk              *parent_clk;
	char                    *parent_clk_name;

	/* Platform CHIP ID */
	plat_chip_id = (u32)nvt_get_chip_id();

	/* check tbuf allocator */
	if ((tbuf_alloc < 0) || (tbuf_alloc >= NA51089_HASH_TBUF_ALLOC_MAX))
		tbuf_alloc = NA51089_HASH_TBUF_ALLOC_NONE;

	/* check tbuf size */
	if (tbuf_size < NA51089_HASH_BUFFER_LEN)
	    tbuf_size = NA51089_HASH_BUFFER_LEN;

	dd = na51089_hdev = devm_kzalloc(&pdev->dev, sizeof(struct na51089_hash_dev), GFP_KERNEL);
	if (!dd) {
		dev_err(&pdev->dev, "unable to alloc device data struct\n");
		return -ENOMEM;
	}

	dd->dev  = &pdev->dev;
	dd->irq  = -1;
	dd->busy = false;

	platform_set_drvdata(pdev, dd);

	spin_lock_init(&dd->lock);

	crypto_init_queue(&dd->queue, NA51089_HASH_QUEUE_LENGTH);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no MEM resource info\n");
		return -ENODEV;
	}

	dd->irq = platform_get_irq(pdev,  0);
	if (dd->irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource info\n");
		return dd->irq;
	}

	dd->iobase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dd->iobase)) {
		dev_err(&pdev->dev, "can't ioremap\n");
		return PTR_ERR(dd->iobase);
	}

	ret = devm_request_irq(&pdev->dev, dd->irq, na51089_hash_irq, 0, dev_name(&pdev->dev), dd);
	if (ret) {
		dev_err(&pdev->dev, "unable to request IRQ\n");
		return ret;
	}

    /* get hash clock node */
	dd->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(dd->clk)) {
		dev_err(&pdev->dev, "failed to find hash hardware clock\n");
		return PTR_ERR(dd->clk);
	}

	/* get hash master clock selection from device tree */
	if (mclk < 0) {
        if (of_property_read_u32(pdev->dev.of_node, "mclk", &mclk)) {
            mclk = NA51089_HASH_MCLK_320MHZ;
        }
	}

    /* get parent clock node of hash master clock */
	switch (mclk) {
	    case NA51089_HASH_MCLK_240MHZ:
	        parent_clk_name = "fix240m";
	        break;
	    case NA51089_HASH_MCLK_PLL9:
	        parent_clk_name = "pll9";
	        break;
	    case NA51089_HASH_MCLK_320MHZ:
	    default:
	        parent_clk_name = "pllf320";
	        break;
	}
	parent_clk = clk_get(NULL, parent_clk_name);
	if (IS_ERR(parent_clk)) {
		dev_err(&pdev->dev, "failed to find hash parent clock\n");
		parent_clk = NULL;
	}

	/* set hash clock to specify parent */
	if (parent_clk) {
	    ret = clk_set_parent(dd->clk, parent_clk);
	    if (ret < 0)
	        dev_err(&pdev->dev, "fail to switch hash hardware clock\n");

	    /* release parent clock */
	    clk_put(parent_clk);
	}

	/* allocate tbuf */
	if (tbuf_size) {
		switch (tbuf_alloc) {
			case NA51089_HASH_TBUF_ALLOC_BUDDY:
				/* source tbuf */
				dd->tbuf_ctx = (void *)__get_free_pages(GFP_KERNEL, get_order(tbuf_size));
				if (!dd->tbuf_ctx) {
					dev_dbg(&pdev->dev, "unable to alloc ctx temp buffer from buddy\n");
				}
				else if ((((u32)page_to_phys(virt_to_page(dd->tbuf_ctx)) + tbuf_size) & (~NA51089_HASH_DMA_ADDR_MASK))) {
					dev_dbg(&pdev->dev, "alloc ctx temp buffer from buddy but dma not support buffer physical address\n");
					free_pages((unsigned long)dd->tbuf_ctx, get_order(tbuf_size));
					dd->tbuf_ctx = NULL;
				}
				break;
			case NA51089_HASH_TBUF_ALLOC_NONE:
			default:
				break;
		}
	}

	/* init tasklet for device request */
	tasklet_init(&dd->queue_tasklet, na51089_hash_queue_tasklet, (unsigned long)dd);
	tasklet_init(&dd->done_tasklet,  na51089_hash_done_tasklet,  (unsigned long)dd);

	/* init timer for device timeout */
	timer_setup(&dd->timer, na51089_hash_timeout_handler, 0);

	/* enable crypto hardware clock */
	clk_prepare_enable(dd->clk);

	/* software reset crypto hardware */
	na51089_hash_reset(dd);

	/* register hash algorithm */
	ret = na51089_hash_register_algs(dd);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hash algorithm\n");
		goto error;
	}

	pr_info("nvt-hash driver registered Version: %s (CLK: %luHz)\n", DRV_VERSION, clk_get_rate(dd->clk));

	return 0;

error:
	del_timer(&dd->timer);

	/* free tbuf */
	if (tbuf_size) {
		switch (tbuf_alloc) {
			case NA51089_HASH_TBUF_ALLOC_BUDDY:
				if (dd->tbuf_ctx) {
					free_pages((unsigned long)dd->tbuf_ctx, get_order(tbuf_size));
					dd->tbuf_ctx = NULL;
				}
				break;
			case NA51089_HASH_TBUF_ALLOC_NONE:
			default:
				break;
		}
	}

	tasklet_kill(&dd->queue_tasklet);
	tasklet_kill(&dd->done_tasklet);
	clk_disable_unprepare(dd->clk);

	return ret;
}

static int na51089_hash_remove(struct platform_device *pdev)
{
	struct na51089_hash_dev *dev = platform_get_drvdata(pdev);

	if (!dev)
		return -ENODEV;

	del_timer_sync(&dev->timer);

	na51089_hash_unregister_algs(dev);

	tasklet_kill(&dev->queue_tasklet);
	tasklet_kill(&dev->done_tasklet);

	/* free tbuf */
	if (tbuf_size) {
		switch (tbuf_alloc) {
			case NA51089_HASH_TBUF_ALLOC_BUDDY:
				if (dev->tbuf_ctx) {
					free_pages((unsigned long)dev->tbuf_ctx, get_order(tbuf_size));
					dev->tbuf_ctx = NULL;
				}
				break;
			case NA51089_HASH_TBUF_ALLOC_NONE:
			default:
				break;
		}
	}

	clk_disable_unprepare(dev->clk);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id na51089_hash_of_match[] = {
	{ .compatible = "nvt,nvt_hash" },
	{},
};
MODULE_DEVICE_TABLE(of, na51089_hash_of_match);
#else
#define na51089_hash_of_match     NULL
#endif

static struct platform_driver na51089_hash_driver = {
	.probe	= na51089_hash_probe,
	.remove	= na51089_hash_remove,
	.driver	= {
		.name	= "nvt_hash",
		.of_match_table	= na51089_hash_of_match,
	},
};

module_platform_driver(na51089_hash_driver);

MODULE_DESCRIPTION("Novatek hash hardware acceleration support.");
MODULE_AUTHOR("Novatek");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
