// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626 AES/DES/3DES engine
 *
 * Copyright (C) 2026 Difan Zhang
 *
 * The engine reads and writes memory through its own DMA. Requests are
 * processed one chunk at a time through a pair of coherent bounce buffers.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/crypto.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/des.h>
#include <crypto/internal/skcipher.h>
#include <crypto/scatterwalk.h>
#include <asm/unaligned.h>

#define FH_AES_REG_CTRL			0x00
#define FH_AES_REG_KEY(n)		(0x10 + ((n) * 4))
#define FH_AES_REG_IV(n)		(0x30 + ((n) * 4))
#define FH_AES_REG_DMA_SRC_ADDR		0x48
#define FH_AES_REG_DMA_DST_ADDR		0x4c
#define FH_AES_REG_DMA_LEN		0x50
#define FH_AES_REG_START		0x54
#define FH_AES_REG_BURST_SIZE		0x58
#define FH_AES_REG_INT_EN		0x5c
#define FH_AES_REG_INT_STATUS		0x60
#define FH_AES_REG_INT_CLEAR		0x68

#define FH_AES_CTRL_DIR_MASK		BIT(0)
#define FH_AES_CTRL_ENC			0
#define FH_AES_CTRL_DEC			BIT(0)

#define FH_AES_CTRL_ALG_MASK		GENMASK(3, 1)
#define FH_AES_CTRL_ALG_DES		(0 << 1)
#define FH_AES_CTRL_ALG_3DES		(1 << 1)
#define FH_AES_CTRL_ALG_AES128		(4 << 1)
#define FH_AES_CTRL_ALG_AES192		(5 << 1)
#define FH_AES_CTRL_ALG_AES256		(6 << 1)

#define FH_AES_CTRL_MODE_MASK		GENMASK(6, 4)
#define FH_AES_CTRL_MODE_ECB		(0 << 4)
#define FH_AES_CTRL_MODE_CBC		(1 << 4)
#define FH_AES_CTRL_MODE_CTR		(2 << 4)
#define FH_AES_CTRL_MODE_CFB		(4 << 4)
#define FH_AES_CTRL_MODE_OFB		(5 << 4)

#define FH_AES_CTRL_LOAD_IV		BIT(7)

#define FH_AES_START_RUN		1
#define FH_AES_START_STOP		0

#define FH_AES_BURST_DEFAULT		8

#define FH_AES_INT_DISABLE		0
#define FH_AES_INT_ENABLE		1

#define FH_AES_INT_DONE			BIT(0)
#define FH_AES_INT_SRC_ERR		BIT(1)
#define FH_AES_INT_DST_ERR		BIT(2)
#define FH_AES_INT_ALL			(FH_AES_INT_DONE | \
					 FH_AES_INT_SRC_ERR | \
					 FH_AES_INT_DST_ERR)

#define FH_AES_CHUNK_MAX		2048
#define FH_AES_BUF_SIZE			2048
#define FH_AES_QUEUE_LEN		1000

struct fh_aes_dev {
	struct list_head	list;
	struct device		*dev;
	void __iomem		*base;
	struct clk		*clk;
	int			irq;

	void			*in_buf;
	dma_addr_t		in_dma;
	void			*out_buf;
	dma_addr_t		out_dma;

	struct crypto_queue	queue;
	spinlock_t		lock;
	struct workqueue_struct	*workqueue;
	struct work_struct	work;
	struct completion	done;
	u32			status;
	bool			faulted;
};

struct fh_aes_ctx {
	unsigned int		keylen;
	u32			key_words[8];
};

struct fh_aes_reqctx {
	u32			mode;
};

static DEFINE_SPINLOCK(fh_aes_dev_lock);
static LIST_HEAD(fh_aes_dev_list);
static bool fh_aes_broken;

static void fh_aes_fault(struct fh_aes_dev *dev)
{
	unsigned long flags;

	/* Keep the clock, DMA storage, device and module until a
	 * hardware reset.
	 */
	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	if (!dev->faulted) {
		WRITE_ONCE(dev->faulted, true);
		fh_aes_broken = true;
		get_device(dev->dev);
		__module_get(THIS_MODULE);
	}
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);
}

static irqreturn_t fh_aes_interrupt(int irq, void *data)
{
	struct fh_aes_dev *dev = data;
	u32 status;

	status = readl(dev->base + FH_AES_REG_INT_STATUS);
	if (!(status & FH_AES_INT_ALL))
		return IRQ_NONE;
	writel(FH_AES_START_STOP, dev->base + FH_AES_REG_START);

	writel(FH_AES_INT_ALL, dev->base + FH_AES_REG_INT_CLEAR);
	writel(FH_AES_INT_DISABLE, dev->base + FH_AES_REG_INT_EN);

	dev->status = status;

	if (status & FH_AES_INT_SRC_ERR)
		dev_err_ratelimited(dev->dev, "DMA source error\n");
	if (status & FH_AES_INT_DST_ERR)
		dev_err_ratelimited(dev->dev, "DMA destination error\n");

	complete(&dev->done);

	return IRQ_HANDLED;
}

static void fh_aes_stop(struct fh_aes_dev *dev)
{
	writel(FH_AES_START_STOP, dev->base + FH_AES_REG_START);
	writel(FH_AES_INT_DISABLE, dev->base + FH_AES_REG_INT_EN);
	writel(FH_AES_INT_ALL, dev->base + FH_AES_REG_INT_CLEAR);
	readl(dev->base + FH_AES_REG_INT_EN);
	synchronize_irq(dev->irq);
}

static void fh_aes_work(struct work_struct *work)
{
	struct fh_aes_dev *dev = container_of(work, struct fh_aes_dev, work);
	struct crypto_async_request *async_req, *backlog;
	struct ablkcipher_request *req;
	struct fh_aes_ctx *ctx;
	struct fh_aes_reqctx *rctx;
	unsigned long flags;
	unsigned int nbytes;
	unsigned int offset;
	bool is_first_chunk;
	bool has_iv;
	unsigned int block;
	unsigned int iv_bytes;
	unsigned int hw_iv_bytes;
	u8 iv[AES_BLOCK_SIZE];
	int src_nents, dst_nents;
	u32 ctrl;
	int err = 0;
	int i;

next_request:
	err = 0;
	spin_lock_irqsave(&dev->lock, flags);
	backlog = crypto_get_backlog(&dev->queue);
	async_req = crypto_dequeue_request(&dev->queue);
	spin_unlock_irqrestore(&dev->lock, flags);

	if (!async_req)
		return;

	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);

	req = ablkcipher_request_cast(async_req);
	ctx = crypto_ablkcipher_ctx(crypto_ablkcipher_reqtfm(req));
	rctx = ablkcipher_request_ctx(req);
	if (READ_ONCE(dev->faulted)) {
		err = -EIO;
		goto complete;
	}
	src_nents = sg_nents_for_len(req->src, req->nbytes);
	dst_nents = sg_nents_for_len(req->dst, req->nbytes);
	if (src_nents < 0 || dst_nents < 0) {
		err = -EINVAL;
		goto complete;
	}

	ctrl = rctx->mode;
	/*
	 * DES loads two IV words and every other algorithm loads four. 3DES
	 * has an 8-byte IV, so its upper two words are written as zero.
	 * Chaining follows the cipher block size.
	 */
	if ((ctrl & FH_AES_CTRL_ALG_MASK) == FH_AES_CTRL_ALG_DES ||
	    (ctrl & FH_AES_CTRL_ALG_MASK) == FH_AES_CTRL_ALG_3DES)
		block = DES_BLOCK_SIZE;
	else
		block = AES_BLOCK_SIZE;
	iv_bytes = block;
	hw_iv_bytes = (ctrl & FH_AES_CTRL_ALG_MASK) == FH_AES_CTRL_ALG_DES ?
		DES_BLOCK_SIZE : AES_BLOCK_SIZE;
	has_iv = (ctrl & FH_AES_CTRL_MODE_MASK) != FH_AES_CTRL_MODE_ECB;
	is_first_chunk = true;
	if (has_iv)
		memcpy(iv, req->info, iv_bytes);

	nbytes = req->nbytes;
	offset = 0;

	while (nbytes > 0) {
		unsigned int chunk_len = min_t(unsigned int, nbytes,
					       FH_AES_CHUNK_MAX);
		u32 cur_ctrl = ctrl;
		unsigned int dma_len = ALIGN(chunk_len, block);

		fh_aes_stop(dev);
		if (is_first_chunk && has_iv)
			cur_ctrl |= FH_AES_CTRL_LOAD_IV;
		/* Control must be written before loading the first key
		 * and IV.
		 */
		writel(cur_ctrl, dev->base + FH_AES_REG_CTRL);
		if (is_first_chunk) {
			for (i = 0; i < ctx->keylen / 4; i++)
				writel(ctx->key_words[i],
				       dev->base + FH_AES_REG_KEY(i));
		}
		if (is_first_chunk && has_iv) {
			for (i = 0; i < hw_iv_bytes / 4; i++) {
				u32 iv_word = 0;

				if (i < iv_bytes / 4)
					iv_word = get_unaligned_be32(req->info +
								     i * 4);
				writel(iv_word, dev->base + FH_AES_REG_IV(i));
			}
		}

		memset(dev->in_buf, 0, dma_len);
		if (sg_pcopy_to_buffer(req->src, src_nents, dev->in_buf,
				      chunk_len, offset) != chunk_len) {
			err = -EINVAL;
			break;
		}

		reinit_completion(&dev->done);
		dev->status = 0;

		writel(dev->in_dma, dev->base + FH_AES_REG_DMA_SRC_ADDR);
		writel(dev->out_dma, dev->base + FH_AES_REG_DMA_DST_ADDR);
		writel(dma_len, dev->base + FH_AES_REG_DMA_LEN);
		writel(FH_AES_BURST_DEFAULT, dev->base + FH_AES_REG_BURST_SIZE);
		writel(FH_AES_INT_ENABLE, dev->base + FH_AES_REG_INT_EN);
		writel(FH_AES_START_RUN, dev->base + FH_AES_REG_START);

		if (!wait_for_completion_timeout(&dev->done,
						 msecs_to_jiffies(1000))) {
			dev_err(dev->dev, "AES transfer timed out\n");
			fh_aes_stop(dev);
			/* No hardware reset handshake exists; retire the
			 * engine.
			 */
			fh_aes_fault(dev);
			err = -ETIMEDOUT;
			break;
		}

		if (dev->status & (FH_AES_INT_SRC_ERR | FH_AES_INT_DST_ERR)) {
			dev_err(dev->dev, "DMA transfer error: 0x%08x\n",
				dev->status);
			err = -EIO;
			fh_aes_stop(dev);
			fh_aes_fault(dev);
			break;
		}
		dma_rmb();

		/* Recover the chaining value without assuming readable
		 * IV state.
		 */
		switch (ctrl & FH_AES_CTRL_MODE_MASK) {
		case FH_AES_CTRL_MODE_CBC:
		case FH_AES_CTRL_MODE_CFB:
			memcpy(iv, ((ctrl & FH_AES_CTRL_DEC) ? dev->in_buf :
				    dev->out_buf) + dma_len - block, block);
			break;
		case FH_AES_CTRL_MODE_CTR:
			for (i = 0; i < dma_len / AES_BLOCK_SIZE; i++)
				crypto_inc(iv, AES_BLOCK_SIZE);
			break;
		case FH_AES_CTRL_MODE_OFB:
			for (i = 0; i < block; i++)
				iv[i] =
				    ((u8 *)dev->in_buf)[dma_len - block + i] ^
				    ((u8 *)dev->out_buf)[dma_len - block + i];
			break;
		}

		if (sg_pcopy_from_buffer(req->dst, dst_nents, dev->out_buf,
					chunk_len, offset) != chunk_len) {
			err = -EINVAL;
			break;
		}

		offset += chunk_len;
		nbytes -= chunk_len;
		is_first_chunk = false;
	}
	if (!err && has_iv)
		memcpy(req->info, iv, iv_bytes);
	memzero_explicit(iv, sizeof(iv));
	if (!READ_ONCE(dev->faulted)) {
		memzero_explicit(dev->in_buf, FH_AES_BUF_SIZE);
		memzero_explicit(dev->out_buf, FH_AES_BUF_SIZE);
		for (i = 0; i < 8; i++)
			writel(0, dev->base + FH_AES_REG_KEY(i));
	}

complete:
	req->base.complete(&req->base, err);
	goto next_request;
}

static int fh_aes_crypt(struct ablkcipher_request *req, u32 mode)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct fh_aes_reqctx *rctx = ablkcipher_request_ctx(req);
	struct fh_aes_dev *dev;
	unsigned long flags;
	int err;

	/*
	 * DES and 3DES wrappers pass the alg field. AES wrappers pass only
	 * the mode and direction; the key length selects AES-128/192/256.
	 */
	if (ctx->keylen == DES_KEY_SIZE &&
	    (mode & FH_AES_CTRL_ALG_MASK) == FH_AES_CTRL_ALG_DES) {
		if ((mode & FH_AES_CTRL_MODE_MASK) != FH_AES_CTRL_MODE_CTR &&
		    !IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE))
			return -EINVAL;
	} else if ((mode & FH_AES_CTRL_ALG_MASK) == FH_AES_CTRL_ALG_3DES) {
		if (ctx->keylen != DES3_EDE_KEY_SIZE)
			return -EINVAL;
		if ((mode & FH_AES_CTRL_MODE_MASK) != FH_AES_CTRL_MODE_CTR &&
		    !IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE))
			return -EINVAL;
	} else if (mode & FH_AES_CTRL_ALG_MASK) {
		return -EINVAL;
	}

	/*
	 * DES is also a zero algorithm field, distinguished by key length,
	 * so it must not fall into the AES key-length switch.
	 */
	if (!(mode & FH_AES_CTRL_ALG_MASK) && ctx->keylen != DES_KEY_SIZE) {
		switch (ctx->keylen) {
		case AES_KEYSIZE_128:
			mode |= FH_AES_CTRL_ALG_AES128;
			break;
		case AES_KEYSIZE_192:
			mode |= FH_AES_CTRL_ALG_AES192;
			break;
		case AES_KEYSIZE_256:
			mode |= FH_AES_CTRL_ALG_AES256;
			break;
		default:
			return -EINVAL;
		}
		if ((mode & FH_AES_CTRL_MODE_MASK) != FH_AES_CTRL_MODE_CTR &&
		    !IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE))
			return -EINVAL;
	}

	rctx->mode = mode;
	if (!req->nbytes)
		return 0;
	if (!req->src || !req->dst ||
	    ((mode & FH_AES_CTRL_MODE_MASK) && !req->info))
		return -EINVAL;

	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	if (list_empty(&fh_aes_dev_list)) {
		err = -ENODEV;
	} else {
		dev = list_first_entry(&fh_aes_dev_list,
				       struct fh_aes_dev, list);
		spin_lock(&dev->lock);
		err = READ_ONCE(dev->faulted) ? -EIO :
			crypto_enqueue_request(&dev->queue, &req->base);
		if (!READ_ONCE(dev->faulted))
			queue_work(dev->workqueue, &dev->work);
		spin_unlock(&dev->lock);
	}
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);

	return err;
}

static int fh_aes_store_key(struct crypto_ablkcipher *tfm, const u8 *key,
			    unsigned int keylen, unsigned int expect)
{
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	unsigned int i;

	if (keylen != expect) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	ctx->keylen = keylen;
	memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
	for (i = 0; i < keylen / 4; i++)
		ctx->key_words[i] = get_unaligned_be32(key + i * 4);
	return 0;
}

static int fh_aes_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			 unsigned int keylen)
{
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192 &&
	    keylen != AES_KEYSIZE_256) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	return fh_aes_store_key(tfm, key, keylen, keylen);
}

static int verify_ablkcipher_des_key(struct crypto_ablkcipher *tfm,
				     const u8 *key)
{
#if IS_REACHABLE(CONFIG_CRYPTO_DES)
	u32 tmp[DES_EXPKEY_WORDS];

	if (unlikely(!des_ekey(tmp, key)) &&
	    (crypto_ablkcipher_get_flags(tfm) & CRYPTO_TFM_REQ_WEAK_KEY)) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_WEAK_KEY);
		return -EINVAL;
	}
#else
	u8 c0 = ((key[4] & 0x0e) << 4) | (key[0] & 0x1e);
	u8 c1 = ((key[5] & 0x0e) << 4) | (key[1] & 0x1e);
	u8 c2 = ((key[6] & 0x0e) << 4) | (key[2] & 0x1e);
	u8 c3 = ((key[7] & 0x0e) << 4) | (key[3] & 0x1e);
	u8 d0 = ((key[0] & 0xe0) >> 4) | (key[4] & 0xf0);
	u8 d1 = ((key[1] & 0xe0) >> 4) | (key[5] & 0xf0);
	u8 d2 = ((key[2] & 0xe0) >> 4) | (key[6] & 0xf0);
	u8 d3 = ((key[3] & 0xe0) >> 4) | (key[7] & 0xf0);
	bool weak = (c0 == c2 && (c0 == 0x00 || c0 == 0xfe)) &&
		    (c1 == c3 && (c1 == 0x00 || c1 == 0xfe)) &&
		    (d0 == d2 && (d0 == 0x00 || d0 == 0xfe)) &&
		    (d1 == d3 && (d1 == 0x00 || d1 == 0xfe));

	if (unlikely(weak) &&
	    (crypto_ablkcipher_get_flags(tfm) & CRYPTO_TFM_REQ_WEAK_KEY)) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_WEAK_KEY);
		return -EINVAL;
	}
#endif
	return 0;
}

static int verify_ablkcipher_des3_key(struct crypto_ablkcipher *tfm,
				      const u8 *key)
{
	u32 flags = crypto_ablkcipher_get_flags(tfm);

#if IS_REACHABLE(CONFIG_CRYPTO_DES)
	u32 expkey[DES3_EDE_EXPKEY_WORDS];
	int err;

	err = __des3_ede_setkey(expkey, &flags, key, DES3_EDE_KEY_SIZE);
	memzero_explicit(expkey, sizeof(expkey));
	if (err) {
		crypto_ablkcipher_set_flags(tfm, flags);
		return err;
	}
#endif
	/*
	 * RFC2451: If the first two or last two independent 64-bit keys are
	 * equal (K1 == K2 or K2 == K3), DES3 degenerates to single DES and
	 * must be rejected. Also reject K1 == K3 when weak key checking is
	 * requested.
	 */
	if (unlikely(!memcmp(key, key + DES_KEY_SIZE, DES_KEY_SIZE) ||
		     !memcmp(key + DES_KEY_SIZE, key + 2 * DES_KEY_SIZE,
			     DES_KEY_SIZE))) {
		crypto_ablkcipher_set_flags(tfm,
					    (flags & CRYPTO_TFM_REQ_WEAK_KEY) ?
					    CRYPTO_TFM_RES_WEAK_KEY :
					    CRYPTO_TFM_RES_BAD_KEY_SCHED);
		return -EINVAL;
	}

	if (unlikely(!memcmp(key, key + 2 * DES_KEY_SIZE, DES_KEY_SIZE)) &&
	    (flags & CRYPTO_TFM_REQ_WEAK_KEY)) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_WEAK_KEY);
		return -EINVAL;
	}

	return 0;
}

static int fh_des_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			 unsigned int keylen)
{
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	int err;

	if (keylen != DES_KEY_SIZE) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	err = verify_ablkcipher_des_key(tfm, key);
	if (err) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		return err;
	}

	return fh_aes_store_key(tfm, key, keylen, DES_KEY_SIZE);
}

static int fh_des3_setkey(struct crypto_ablkcipher *tfm, const u8 *key,
			  unsigned int keylen)
{
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	int err;

	if (keylen != DES3_EDE_KEY_SIZE) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	err = verify_ablkcipher_des3_key(tfm, key);
	if (err) {
		ctx->keylen = 0;
		memzero_explicit(ctx->key_words, sizeof(ctx->key_words));
		return err;
	}

	return fh_aes_store_key(tfm, key, keylen, DES3_EDE_KEY_SIZE);
}

static int fh_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_ECB | FH_AES_CTRL_ENC);
}

static int fh_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_ECB | FH_AES_CTRL_DEC);
}

static int fh_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CBC | FH_AES_CTRL_ENC);
}

static int fh_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CBC | FH_AES_CTRL_DEC);
}

static int fh_aes_ctr_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CTR | FH_AES_CTRL_ENC);
}

static int fh_aes_ctr_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CTR | FH_AES_CTRL_DEC);
}

static int fh_aes_cfb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CFB | FH_AES_CTRL_ENC);
}

static int fh_aes_cfb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_CFB | FH_AES_CTRL_DEC);
}

static int fh_aes_ofb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_OFB | FH_AES_CTRL_ENC);
}

static int fh_aes_ofb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_MODE_OFB | FH_AES_CTRL_DEC);
}

static int fh_des_ecb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_ECB |
			    FH_AES_CTRL_ENC);
}

static int fh_des_ecb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_ECB |
			    FH_AES_CTRL_DEC);
}

static int fh_des_cbc_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_CBC |
			    FH_AES_CTRL_ENC);
}

static int fh_des_cbc_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_CBC |
			    FH_AES_CTRL_DEC);
}

static int fh_des_cfb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_CFB |
			    FH_AES_CTRL_ENC);
}

static int fh_des_cfb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_CFB |
			    FH_AES_CTRL_DEC);
}

static int fh_des_ofb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_OFB |
			    FH_AES_CTRL_ENC);
}

static int fh_des_ofb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_DES | FH_AES_CTRL_MODE_OFB |
			    FH_AES_CTRL_DEC);
}

static int fh_des3_ecb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_ECB |
			    FH_AES_CTRL_ENC);
}

static int fh_des3_ecb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_ECB |
			    FH_AES_CTRL_DEC);
}

static int fh_des3_cbc_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_CBC |
			    FH_AES_CTRL_ENC);
}

static int fh_des3_cbc_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_CBC |
			    FH_AES_CTRL_DEC);
}

static int fh_des3_cfb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_CFB |
			    FH_AES_CTRL_ENC);
}

static int fh_des3_cfb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_CFB |
			    FH_AES_CTRL_DEC);
}

static int fh_des3_ofb_encrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_OFB |
			    FH_AES_CTRL_ENC);
}

static int fh_des3_ofb_decrypt(struct ablkcipher_request *req)
{
	return fh_aes_crypt(req, FH_AES_CTRL_ALG_3DES | FH_AES_CTRL_MODE_OFB |
			    FH_AES_CTRL_DEC);
}

static int fh_aes_cra_init(struct crypto_tfm *tfm)
{
	tfm->crt_ablkcipher.reqsize = sizeof(struct fh_aes_reqctx);

	return 0;
}

static void fh_aes_cra_exit(struct crypto_tfm *tfm)
{
	memzero_explicit(crypto_tfm_ctx(tfm), sizeof(struct fh_aes_ctx));
}

static struct crypto_alg fh_aes_algs[] = {
	{
		.cra_name		= "ecb(aes)",
		.cra_driver_name	= "ecb-aes-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ecb_encrypt,
			.decrypt	= fh_aes_ecb_decrypt,
		},
	},
	{
		.cra_name		= "cbc(aes)",
		.cra_driver_name	= "cbc-aes-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_cbc_encrypt,
			.decrypt	= fh_aes_cbc_decrypt,
		},
	},
	{
		.cra_name		= "ctr(aes)",
		.cra_driver_name	= "ctr-aes-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= 1,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ctr_encrypt,
			.decrypt	= fh_aes_ctr_decrypt,
		},
	},
	{
		.cra_name		= "cfb(aes)",
		.cra_driver_name	= "cfb-aes-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_cfb_encrypt,
			.decrypt	= fh_aes_cfb_decrypt,
		},
	},
	{
		.cra_name		= "ofb(aes)",
		.cra_driver_name	= "ofb-aes-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ofb_encrypt,
			.decrypt	= fh_aes_ofb_decrypt,
		},
	},
	{
		.cra_name		= "ecb(des)",
		.cra_driver_name	= "ecb-des-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_ecb_encrypt,
			.decrypt	= fh_des_ecb_decrypt,
		},
	},
	{
		.cra_name		= "cbc(des)",
		.cra_driver_name	= "cbc-des-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_cbc_encrypt,
			.decrypt	= fh_des_cbc_decrypt,
		},
	},
	{
		.cra_name		= "cfb(des)",
		.cra_driver_name	= "cfb-des-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_cfb_encrypt,
			.decrypt	= fh_des_cfb_decrypt,
		},
	},
	{
		.cra_name		= "ofb(des)",
		.cra_driver_name	= "ofb-des-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_ofb_encrypt,
			.decrypt	= fh_des_ofb_decrypt,
		},
	},
	{
		.cra_name		= "ecb(des3_ede)",
		.cra_driver_name	= "ecb-des3-ede-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.setkey		= fh_des3_setkey,
			.encrypt	= fh_des3_ecb_encrypt,
			.decrypt	= fh_des3_ecb_decrypt,
		},
	},
	{
		.cra_name		= "cbc(des3_ede)",
		.cra_driver_name	= "cbc-des3-ede-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des3_setkey,
			.encrypt	= fh_des3_cbc_encrypt,
			.decrypt	= fh_des3_cbc_decrypt,
		},
	},
	{
		.cra_name		= "cfb(des3_ede)",
		.cra_driver_name	= "cfb-des3-ede-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des3_setkey,
			.encrypt	= fh_des3_cfb_encrypt,
			.decrypt	= fh_des3_cfb_decrypt,
		},
	},
	{
		.cra_name		= "ofb(des3_ede)",
		.cra_driver_name	= "ofb-des3-ede-fh",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					  CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_exit		= fh_aes_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des3_setkey,
			.encrypt	= fh_des3_ofb_encrypt,
			.decrypt	= fh_des3_ofb_decrypt,
		},
	},
};

static int fh_aes_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fh_aes_dev *aes_dev;
	struct resource *res;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	ret = fh_aes_broken ? -EIO : 0;
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);
	if (ret)
		return ret;

	aes_dev = devm_kzalloc(dev, sizeof(*aes_dev), GFP_KERNEL);
	if (!aes_dev)
		return -ENOMEM;

	aes_dev->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	aes_dev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(aes_dev->base))
		return PTR_ERR(aes_dev->base);

	aes_dev->irq = platform_get_irq(pdev, 0);
	if (aes_dev->irq < 0) {
		dev_err(dev, "failed to get IRQ: %d\n", aes_dev->irq);
		return aes_dev->irq;
	}

	/* A fault must retain this reference beyond devres teardown. */
	aes_dev->clk = clk_get(dev, "hclk");
	if (IS_ERR(aes_dev->clk))
		return PTR_ERR(aes_dev->clk);
	ret = clk_prepare_enable(aes_dev->clk);
	if (ret) {
		clk_put(aes_dev->clk);
		return ret;
	}

	writel(FH_AES_START_STOP, aes_dev->base + FH_AES_REG_START);
	writel(FH_AES_INT_DISABLE, aes_dev->base + FH_AES_REG_INT_EN);
	writel(FH_AES_INT_ALL, aes_dev->base + FH_AES_REG_INT_CLEAR);
	readl(aes_dev->base + FH_AES_REG_INT_EN);

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "failed to set DMA mask: %d\n", ret);
		goto err_clk;
	}

	aes_dev->in_buf = dma_alloc_coherent(dev, FH_AES_BUF_SIZE,
					     &aes_dev->in_dma, GFP_KERNEL);
	if (!aes_dev->in_buf) {
		ret = -ENOMEM;
		goto err_clk;
	}

	aes_dev->out_buf = dma_alloc_coherent(dev, FH_AES_BUF_SIZE,
					      &aes_dev->out_dma, GFP_KERNEL);
	if (!aes_dev->out_buf) {
		ret = -ENOMEM;
		goto err_free_in;
	}

	crypto_init_queue(&aes_dev->queue, FH_AES_QUEUE_LEN);
	spin_lock_init(&aes_dev->lock);
	init_completion(&aes_dev->done);
	INIT_WORK(&aes_dev->work, fh_aes_work);

	aes_dev->workqueue = alloc_ordered_workqueue("fh_aes", 0);
	if (!aes_dev->workqueue) {
		ret = -ENOMEM;
		goto err_free_out;
	}
	ret = devm_request_irq(dev, aes_dev->irq, fh_aes_interrupt, 0,
			       dev_name(dev), aes_dev);
	if (ret)
		goto err_workqueue;

	platform_set_drvdata(pdev, aes_dev);

	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	if (!list_empty(&fh_aes_dev_list)) {
		spin_unlock_irqrestore(&fh_aes_dev_lock, flags);
		ret = -EBUSY;
		goto err_irq;
	}
	list_add_tail(&aes_dev->list, &fh_aes_dev_list);
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);

	ret = crypto_register_algs(fh_aes_algs, ARRAY_SIZE(fh_aes_algs));
	if (ret) {
		dev_err(dev, "failed to register algorithms: %d\n", ret);
		goto err_del_list;
	}

	return 0;

err_del_list:
	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	list_del(&aes_dev->list);
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);
	flush_workqueue(aes_dev->workqueue);
err_irq:
	fh_aes_stop(aes_dev);
	devm_free_irq(dev, aes_dev->irq, aes_dev);
err_workqueue:
	destroy_workqueue(aes_dev->workqueue);
err_free_out:
	if (!aes_dev->faulted)
		dma_free_coherent(dev, FH_AES_BUF_SIZE, aes_dev->out_buf,
				  aes_dev->out_dma);
err_free_in:
	if (!aes_dev->faulted)
		dma_free_coherent(dev, FH_AES_BUF_SIZE, aes_dev->in_buf,
				  aes_dev->in_dma);
err_clk:
	if (!aes_dev->faulted) {
		clk_disable_unprepare(aes_dev->clk);
		clk_put(aes_dev->clk);
	}
	return ret;
}

static int fh_aes_remove(struct platform_device *pdev)
{
	struct fh_aes_dev *aes_dev = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	unsigned long flags;

	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	list_del_init(&aes_dev->list);
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);

	flush_workqueue(aes_dev->workqueue);
	crypto_unregister_algs(fh_aes_algs, ARRAY_SIZE(fh_aes_algs));
	destroy_workqueue(aes_dev->workqueue);
	fh_aes_stop(aes_dev);
	devm_free_irq(dev, aes_dev->irq, aes_dev);

	if (!aes_dev->faulted) {
		dma_free_coherent(dev, FH_AES_BUF_SIZE, aes_dev->out_buf,
				  aes_dev->out_dma);
		dma_free_coherent(dev, FH_AES_BUF_SIZE, aes_dev->in_buf,
				  aes_dev->in_dma);
	} else {
		dev_warn(dev, "retaining DMA buffers after hardware fault\n");
	}

	if (!aes_dev->faulted) {
		clk_disable_unprepare(aes_dev->clk);
		clk_put(aes_dev->clk);
	}

	return 0;
}

static void fh_aes_shutdown(struct platform_device *pdev)
{
	struct fh_aes_dev *dev = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&fh_aes_dev_lock, flags);
	list_del_init(&dev->list);
	spin_unlock_irqrestore(&fh_aes_dev_lock, flags);
	flush_workqueue(dev->workqueue);
	fh_aes_stop(dev);
}

static int __maybe_unused fh_aes_suspend(struct device *dev)
{
	/* No reset/idle or context-retention support exists. */
	return -EBUSY;
}

static const struct dev_pm_ops fh_aes_pm_ops = {
	.suspend = fh_aes_suspend,
	.freeze = fh_aes_suspend,
	.poweroff = fh_aes_suspend,
};

static const struct of_device_id fh_aes_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-aes" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_aes_of_match);

static struct platform_driver fh_aes_driver = {
	.probe		= fh_aes_probe,
	.remove		= fh_aes_remove,
	.shutdown	= fh_aes_shutdown,
	.driver		= {
		.name		= "fh-aes",
		.of_match_table	= fh_aes_of_match,
		.pm		= &fh_aes_pm_ops,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(fh_aes_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Fullhan FH8626 Hardware AES Accelerator Driver");
MODULE_LICENSE("GPL v2");
