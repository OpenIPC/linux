// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Ingenic T23 SoC Hardware AES Cryptographic Accelerator Driver
 */

#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/internal/skcipher.h>
#include <crypto/scatterwalk.h>
#include <crypto/skcipher.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/unaligned.h>

/* AES Register Offsets (matching vendor jz-aes.h) */
#define AES_ASCR		0x00	/* AES Control Register */
#define AES_ASSR		0x04	/* AES Status Register */
#define AES_ASINTM		0x08	/* AES Interrupt Mask Register */
#define AES_ASSA		0x0C	/* AES DMA Source Address */
#define AES_ASDA		0x10	/* AES DMA Destination Address */
#define AES_ASTC		0x14	/* AES DMA Transfer Count (blocks) */
#define AES_ASDI		0x18	/* AES Data Input */
#define AES_ASDO		0x1C	/* AES Data Output */
#define AES_ASKY		0x20	/* AES Key Register */
#define AES_ASIV		0x24	/* AES IV Register */

/* AES_ASCR Bitfields */
#define AES_ASCR_CLR		BIT(10)
#define AES_ASCR_DMAS		BIT(9)
#define AES_ASCR_DMAE		BIT(8)
#define AES_ASCR_KEYL_128B	(0 << 6)
#define AES_ASCR_KEYL_192B	(1 << 6)
#define AES_ASCR_KEYL_256B	(2 << 6)
#define AES_ASCR_KEYL_MASK	GENMASK(7, 6)
#define AES_ASCR_ECB_MODE	(0 << 5)
#define AES_ASCR_CBC_MODE	(1 << 5)
#define AES_ASCR_MODE_MASK	BIT(5)
#define AES_ASCR_INEN_DAT	(0 << 4)
#define AES_ASCR_INDE_DAT	(1 << 4)
#define AES_ASCR_AESS		BIT(3)
#define AES_ASCR_KEYS		BIT(2)
#define AES_ASCR_INIT_IV	BIT(1)
#define AES_ASCR_AES_EN		BIT(0)

/* AES_ASSR Bitfields; writing 1 toggles a bit, so clear by writing back what is set */
#define AES_ASSR_DMAD		BIT(2)
#define AES_ASSR_AESD		BIT(1)
#define AES_ASSR_KEYD		BIT(0)

/* AES_ASINTM Bitfields (matching vendor jz-aes.h) */
#define AES_ASINTM_DMA_INIT_M	BIT(2)	/* DMA interrupt mask */
#define AES_ASINTM_AES_INIT_M	BIT(1)	/* AES done interrupt mask */
#define AES_ASINTM_KEY_INIT_M	BIT(0)	/* Key done interrupt mask */

#define INGENIC_AES_SLOT_SIZE	(16 * 1024)

static unsigned int chunk_size = 8192;
module_param(chunk_size, uint, 0644);
MODULE_PARM_DESC(chunk_size, "bytes per DMA transfer (16..16384, multiple of 16)");

/* One DMA transfer: big-endian word images of its input and output. */
struct ingenic_aes_slot {
	u8 *in;
	u8 *out;
	dma_addr_t src;
	dma_addr_t dst;
	size_t len;
};

struct ingenic_aes_dev {
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	struct completion dma_done;
	void *bounce_buf;
	struct ingenic_aes_slot slot[2];
	size_t used;		/* bytes of each slot buffer to scrub */
	bool dead;		/* DMA timed out; the engine may still own the buffer */
};

struct ingenic_aes_ctx {
	u32 key[8];
	unsigned int keylen;
	struct crypto_sync_skcipher *fallback_tfm;
	bool fallback_is_ecb;
};

static struct ingenic_aes_dev *g_aes_dev;
static DEFINE_MUTEX(ingenic_aes_lock);

static void ingenic_aes_clear_status(struct ingenic_aes_dev *dev, u32 mask)
{
	u32 status = readl(dev->base + AES_ASSR) & mask;

	if (status)
		writel(status, dev->base + AES_ASSR);
}

static int ingenic_aes_set_key_hw(struct ingenic_aes_dev *dev, u32 ascr,
				  const u32 *key, u32 keylen)
{
	u32 status;
	int i, ret;

	ingenic_aes_clear_status(dev, AES_ASSR_KEYD);
	for (i = 0; i < (keylen / 4); i++)
		writel(get_unaligned_be32((const u8 *)key + 4 * i),
		       dev->base + AES_ASKY);
	writel(ascr | AES_ASCR_KEYS, dev->base + AES_ASCR);
	ret = readl_poll_timeout(dev->base + AES_ASSR, status,
				 status & AES_ASSR_KEYD, 2, 50000);
	ingenic_aes_clear_status(dev, AES_ASSR_KEYD);
	return ret;
}

static void ingenic_aes_set_iv_hw(struct ingenic_aes_dev *dev, u32 ascr,
				  const u8 *iv)
{
	unsigned int i;

	for (i = 0; i < AES_BLOCK_SIZE; i += 4)
		writel(get_unaligned_be32(iv + i), dev->base + AES_ASIV);
	writel(ascr | AES_ASCR_INIT_IV, dev->base + AES_ASCR);
}

static void ingenic_aes_reset(struct ingenic_aes_dev *dev)
{
	writel(0, dev->base + AES_ASINTM);
	ingenic_aes_clear_status(dev, AES_ASSR_DMAD | AES_ASSR_AESD |
				 AES_ASSR_KEYD);
	writel(AES_ASCR_CLR, dev->base + AES_ASCR);
	writel(0, dev->base + AES_ASCR);
	readl(dev->base + AES_ASCR);
}

/*
 * DMA moves big-endian 32-bit words, so byte i of a stream sits at i ^ 3
 * in a slot buffer. The copy below swaps while copying.
 */
static void ingenic_aes_copy_swab(u8 *dst, const u8 *src, size_t pos,
				  size_t n, bool to_slot)
{
	size_t i = 0;

	if (!((pos | (uintptr_t)(to_slot ? src : dst)) & 3)) {
		u32 *d = (u32 *)(to_slot ? dst + pos : dst);
		const u32 *s = (const u32 *)(to_slot ? src : src + pos);

		for (; i + 4 <= n; i += 4)
			*d++ = swab32(*s++);
	}
	for (; i < n; i++) {
		if (to_slot)
			dst[(pos + i) ^ 3] = src[i];
		else
			dst[i] = src[(pos + i) ^ 3];
	}
}

static void ingenic_aes_load(u8 *slot, struct scatterlist *sg, size_t skip,
			     size_t len)
{
	struct sg_mapping_iter m;
	size_t pos = 0;

	sg_miter_start(&m, sg, sg_nents(sg), SG_MITER_FROM_SG);
	sg_miter_skip(&m, skip);
	while (pos < len && sg_miter_next(&m)) {
		size_t n = min(m.length, len - pos);

		ingenic_aes_copy_swab(slot, m.addr, pos, n, true);
		m.consumed = n;
		pos += n;
	}
	sg_miter_stop(&m);
}

static void ingenic_aes_store(struct scatterlist *sg, size_t skip,
			      const u8 *slot, size_t len)
{
	struct sg_mapping_iter m;
	size_t pos = 0;

	sg_miter_start(&m, sg, sg_nents(sg), SG_MITER_TO_SG);
	sg_miter_skip(&m, skip);
	while (pos < len && sg_miter_next(&m)) {
		size_t n = min(m.length, len - pos);

		ingenic_aes_copy_swab(m.addr, slot, pos, n, false);
		m.consumed = n;
		pos += n;
	}
	sg_miter_stop(&m);
}

/* dst = src ^ keystream in one pass; src and dst may be the same list. */
static void ingenic_aes_xor_store(struct scatterlist *dsg,
				  struct scatterlist *ssg, size_t skip,
				  const u8 *ks, size_t len)
{
	struct sg_mapping_iter d, m;
	size_t pos = 0;

	sg_miter_start(&m, ssg, sg_nents(ssg), SG_MITER_FROM_SG);
	sg_miter_start(&d, dsg, sg_nents(dsg), SG_MITER_TO_SG);
	sg_miter_skip(&m, skip);
	sg_miter_skip(&d, skip);
	while (pos < len && sg_miter_next(&m) && sg_miter_next(&d)) {
		size_t n = min3(m.length, d.length, len - pos);
		const u8 *in = m.addr;
		u8 *out = d.addr;
		size_t i = 0;

		if (!((pos | (uintptr_t)in | (uintptr_t)out) & 3)) {
			const u32 *k = (const u32 *)(ks + pos);

			for (; i + 4 <= n; i += 4)
				*(u32 *)(out + i) = *(const u32 *)(in + i) ^
						     swab32(*k++);
		}
		for (; i < n; i++)
			out[i] = in[i] ^ ks[(pos + i) ^ 3];
		m.consumed = n;
		d.consumed = n;
		pos += n;
	}
	sg_miter_stop(&d);
	sg_miter_stop(&m);
}

/* Counter blocks for one CTR transfer, written as big-endian words. */
static void ingenic_aes_ctr_fill(u8 *slot, u8 *ctr, size_t len)
{
	size_t pos;

	for (pos = 0; pos < len; pos += AES_BLOCK_SIZE) {
		ingenic_aes_copy_swab(slot, ctr, pos, AES_BLOCK_SIZE, true);
		crypto_inc(ctr, AES_BLOCK_SIZE);
	}
}

/* Program and start one transfer; ingenic_aes_finish() waits for it. */
static int ingenic_aes_start(struct ingenic_aes_dev *dev,
			     struct ingenic_aes_ctx *ctx,
			     struct ingenic_aes_slot *sl, size_t len,
			     const u8 *iv, bool is_cbc, bool encrypt)
{
	u32 ascr, status;
	int ret;

	sl->len = len;
	sl->src = dma_map_single(dev->dev, sl->in, len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev->dev, sl->src))
		return -ENOMEM;
	sl->dst = dma_map_single(dev->dev, sl->out, len, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev->dev, sl->dst)) {
		dma_unmap_single(dev->dev, sl->src, len, DMA_TO_DEVICE);
		return -ENOMEM;
	}

	ascr = AES_ASCR_AES_EN;
	if (is_cbc)
		ascr |= AES_ASCR_CBC_MODE;
	if (!encrypt)
		ascr |= AES_ASCR_INDE_DAT;
	if (ctx->keylen == AES_KEYSIZE_192)
		ascr |= AES_ASCR_KEYL_192B;
	else if (ctx->keylen == AES_KEYSIZE_256)
		ascr |= AES_ASCR_KEYL_256B;

	ingenic_aes_reset(dev);
	writel(ascr, dev->base + AES_ASCR);
	writel(ascr | AES_ASCR_CLR, dev->base + AES_ASCR);
	ret = readl_poll_timeout(dev->base + AES_ASCR, status,
				 !!(status & AES_ASCR_CBC_MODE) == is_cbc,
				 2, 50000);
	if (ret)
		goto fail;
	ascr |= AES_ASCR_DMAE;
	writel(ascr, dev->base + AES_ASCR);

	ret = ingenic_aes_set_key_hw(dev, ascr, ctx->key, ctx->keylen);
	if (ret)
		goto fail;
	if (is_cbc)
		ingenic_aes_set_iv_hw(dev, ascr, iv);

	writel(len / AES_BLOCK_SIZE, dev->base + AES_ASTC);
	writel(sl->src, dev->base + AES_ASSA);
	writel(sl->dst, dev->base + AES_ASDA);
	ingenic_aes_clear_status(dev, AES_ASSR_DMAD | AES_ASSR_AESD);
	reinit_completion(&dev->dma_done);
	writel(AES_ASINTM_DMA_INIT_M, dev->base + AES_ASINTM);
	writel(ascr | AES_ASCR_DMAS, dev->base + AES_ASCR);
	return 0;

fail:
	ingenic_aes_reset(dev);
	dma_unmap_single(dev->dev, sl->dst, len, DMA_FROM_DEVICE);
	dma_unmap_single(dev->dev, sl->src, len, DMA_TO_DEVICE);
	return ret;
}

static int ingenic_aes_finish(struct ingenic_aes_dev *dev,
			      struct ingenic_aes_slot *sl)
{
	int ret = 0;

	if (!wait_for_completion_timeout(&dev->dma_done, msecs_to_jiffies(500))) {
		dev_err(dev->dev, "DMA timeout: ASCR %08x ASSR %08x\n",
			readl(dev->base + AES_ASCR), readl(dev->base + AES_ASSR));
		/* An engine that did not finish might still write; never reuse it. */
		dev->dead = true;
		ret = -ETIMEDOUT;
	}
	ingenic_aes_reset(dev);
	dma_unmap_single(dev->dev, sl->dst, sl->len, DMA_FROM_DEVICE);
	dma_unmap_single(dev->dev, sl->src, sl->len, DMA_TO_DEVICE);
	return ret;
}

static irqreturn_t ingenic_aes_irq(int irq, void *data)
{
	struct ingenic_aes_dev *dev = data;

	if (!(readl(dev->base + AES_ASSR) & AES_ASSR_DMAD))
		return IRQ_NONE;
	writel(0, dev->base + AES_ASINTM);
	ingenic_aes_clear_status(dev, AES_ASSR_DMAD);
	complete(&dev->dma_done);
	return IRQ_HANDLED;
}

static inline size_t ingenic_aes_len_at(size_t off, size_t total, size_t step)
{
	return min(step, total - off);
}

static inline size_t ingenic_aes_dma_len(size_t off, size_t total, size_t step,
					 bool is_ctr)
{
	size_t len = ingenic_aes_len_at(off, total, step);

	return is_ctr ? round_up(len, AES_BLOCK_SIZE) : len;
}

/*
 * Transfers alternate between two slots: while the engine runs one, the
 * CPU loads the next and stores the previous. CBC chaining values come
 * from the slot buffers, so in-place requests are safe.
 */
static int ingenic_aes_crypt_dma(struct ingenic_aes_dev *dev,
				 struct ingenic_aes_ctx *ctx,
				 struct skcipher_request *req,
				 bool is_cbc, bool is_ctr, bool encrypt)
{
	size_t total = req->cryptlen, step, off, next;
	u8 iv[AES_BLOCK_SIZE], ctr[AES_BLOCK_SIZE];
	unsigned int k = 0;
	int ret;

	step = clamp_t(size_t, round_down(READ_ONCE(chunk_size), AES_BLOCK_SIZE),
		       AES_BLOCK_SIZE, INGENIC_AES_SLOT_SIZE);
	dev->used = round_up(min(total, step), AES_BLOCK_SIZE);
	if (is_ctr)
		memcpy(ctr, req->iv, AES_BLOCK_SIZE);
	else if (is_cbc)
		memcpy(iv, req->iv, AES_BLOCK_SIZE);

	if (is_ctr)
		ingenic_aes_ctr_fill(dev->slot[0].in, ctr,
				     ingenic_aes_dma_len(0, total, step, is_ctr));
	else
		ingenic_aes_load(dev->slot[0].in, req->src, 0,
				 ingenic_aes_len_at(0, total, step));
	ret = ingenic_aes_start(dev, ctx, &dev->slot[0],
				ingenic_aes_dma_len(0, total, step, is_ctr),
				iv, is_cbc, encrypt);
	if (ret)
		return ret;

	for (off = 0; off < total; off = next, k ^= 1) {
		struct ingenic_aes_slot *cur = &dev->slot[k];
		struct ingenic_aes_slot *nxt = &dev->slot[k ^ 1];
		size_t len = ingenic_aes_len_at(off, total, step);

		next = off + len;
		if (next < total) {
			if (is_ctr)
				ingenic_aes_ctr_fill(nxt->in, ctr,
						     ingenic_aes_dma_len(next, total, step, is_ctr));
			else
				ingenic_aes_load(nxt->in, req->src, next,
						 ingenic_aes_len_at(next, total, step));
		}
		ret = ingenic_aes_finish(dev, cur);
		if (ret)
			return ret;
		/* The next CBC transfer chains from this one's last ciphertext block. */
		if (is_cbc)
			ingenic_aes_copy_swab(iv, encrypt ? cur->out : cur->in,
					      len - AES_BLOCK_SIZE, AES_BLOCK_SIZE, false);
		if (next < total) {
			ret = ingenic_aes_start(dev, ctx, nxt,
						ingenic_aes_dma_len(next, total, step, is_ctr),
						iv, is_cbc, encrypt);
			if (ret)
				return ret;
		}
		if (is_ctr)
			ingenic_aes_xor_store(req->dst, req->src, off, cur->out, len);
		else
			ingenic_aes_store(req->dst, off, cur->out, len);
	}

	if (is_ctr)
		memcpy(req->iv, ctr, AES_BLOCK_SIZE);
	else if (is_cbc)
		memcpy(req->iv, iv, AES_BLOCK_SIZE);
	return 0;
}

static int ingenic_aes_fallback(struct ingenic_aes_ctx *ctx,
				struct skcipher_request *req, bool encrypt)
{
	SYNC_SKCIPHER_REQUEST_ON_STACK(subreq, ctx->fallback_tfm);
	int ret;

	skcipher_request_set_sync_tfm(subreq, ctx->fallback_tfm);
	skcipher_request_set_callback(subreq, req->base.flags, NULL, NULL);
	skcipher_request_set_crypt(subreq, req->src, req->dst,
				   req->cryptlen, req->iv);
	if (encrypt)
		ret = crypto_skcipher_encrypt(subreq);
	else
		ret = crypto_skcipher_decrypt(subreq);
	skcipher_request_zero(subreq);
	return ret;
}

/* Request data does not outlive the request in the shared slot buffers. */
static void ingenic_aes_scrub(struct ingenic_aes_dev *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dev->slot); i++) {
		memzero_explicit(dev->slot[i].in, dev->used);
		memzero_explicit(dev->slot[i].out, dev->used);
	}
	dev->used = 0;
}

static int ingenic_aes_crypt(struct skcipher_request *req, bool is_cbc, bool encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct ingenic_aes_dev *dev;
	int ret;

	if (!req->cryptlen)
		return 0;

	if (req->cryptlen % AES_BLOCK_SIZE)
		return -EINVAL;

	if (sg_nents_for_len(req->src, req->cryptlen) < 0 ||
	    sg_nents_for_len(req->dst, req->cryptlen) < 0)
		return -EINVAL;

	/* Atomic callers cannot wait for hardware completion. */
	if (!(req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP))
		return ingenic_aes_fallback(ctx, req, encrypt);

	mutex_lock(&ingenic_aes_lock);
	dev = g_aes_dev;
	if (dev && !dev->dead) {
		ret = ingenic_aes_crypt_dma(dev, ctx, req, is_cbc, false, encrypt);
		ingenic_aes_scrub(dev);
	} else if (dev) {
		ret = ingenic_aes_fallback(ctx, req, encrypt);
	} else {
		ret = -ENODEV;
	}
	mutex_unlock(&ingenic_aes_lock);

	return ret;
}

static int ingenic_aes_ctr_fallback(struct ingenic_aes_ctx *ctx,
				    struct skcipher_request *req)
{
	if (!ctx->fallback_is_ecb) {
		SYNC_SKCIPHER_REQUEST_ON_STACK(subreq, ctx->fallback_tfm);
		int ret;

		skcipher_request_set_sync_tfm(subreq, ctx->fallback_tfm);
		skcipher_request_set_callback(subreq, req->base.flags, NULL, NULL);
		skcipher_request_set_crypt(subreq, req->src, req->dst,
					   req->cryptlen, req->iv);
		ret = crypto_skcipher_encrypt(subreq);
		skcipher_request_zero(subreq);
		return ret;
	} else {
		size_t remaining = req->cryptlen;
		size_t offset = 0;
		u8 iv[AES_BLOCK_SIZE];
		u8 ks[AES_BLOCK_SIZE];
		u8 in_buf[AES_BLOCK_SIZE];
		struct scatterlist sg_in, sg_out;
		SYNC_SKCIPHER_REQUEST_ON_STACK(subreq, ctx->fallback_tfm);
		int ret = 0;

		memcpy(iv, req->iv, AES_BLOCK_SIZE);

		skcipher_request_set_sync_tfm(subreq, ctx->fallback_tfm);
		skcipher_request_set_callback(subreq, req->base.flags, NULL, NULL);

		while (remaining > 0) {
			size_t chunk = min_t(size_t, remaining, AES_BLOCK_SIZE);

			memcpy(in_buf, iv, AES_BLOCK_SIZE);
			crypto_inc(iv, AES_BLOCK_SIZE);

			sg_init_one(&sg_in, in_buf, AES_BLOCK_SIZE);
			sg_init_one(&sg_out, ks, AES_BLOCK_SIZE);
			skcipher_request_set_crypt(subreq, &sg_in, &sg_out,
						   AES_BLOCK_SIZE, NULL);
			ret = crypto_skcipher_encrypt(subreq);
			if (ret)
				break;

			scatterwalk_map_and_copy(in_buf, req->src, offset, chunk, 0);
			crypto_xor(in_buf, ks, chunk);
			scatterwalk_map_and_copy(in_buf, req->dst, offset, chunk, 1);

			offset += chunk;
			remaining -= chunk;
		}

		skcipher_request_zero(subreq);
		if (!ret)
			memcpy(req->iv, iv, AES_BLOCK_SIZE);
		return ret;
	}
}

static int ingenic_aes_ctr_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct ingenic_aes_dev *dev;
	int ret;

	if (!req->cryptlen)
		return 0;

	if (sg_nents_for_len(req->src, req->cryptlen) < 0 ||
	    sg_nents_for_len(req->dst, req->cryptlen) < 0)
		return -EINVAL;

	/* Atomic callers cannot wait for hardware completion. */
	if (!(req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP))
		return ingenic_aes_ctr_fallback(ctx, req);

	mutex_lock(&ingenic_aes_lock);
	dev = g_aes_dev;
	if (dev && !dev->dead) {
		ret = ingenic_aes_crypt_dma(dev, ctx, req, false, true, true);
		ingenic_aes_scrub(dev);
	} else if (dev) {
		ret = ingenic_aes_ctr_fallback(ctx, req);
	} else {
		ret = -ENODEV;
	}
	mutex_unlock(&ingenic_aes_lock);

	return ret;
}

static int ingenic_aes_ecb_encrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, false, true);
}

static int ingenic_aes_ecb_decrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, false, false);
}

static int ingenic_aes_cbc_encrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, true, true);
}

static int ingenic_aes_cbc_decrypt(struct skcipher_request *req)
{
	return ingenic_aes_crypt(req, true, false);
}

static int ingenic_aes_setkey(struct crypto_skcipher *tfm, const u8 *key,
			      unsigned int keylen)
{
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int ret;

	if (keylen != AES_KEYSIZE_128 &&
	    keylen != AES_KEYSIZE_192 &&
	    keylen != AES_KEYSIZE_256)
		return -EINVAL;

	crypto_sync_skcipher_clear_flags(ctx->fallback_tfm, CRYPTO_TFM_REQ_MASK);
	crypto_sync_skcipher_set_flags(ctx->fallback_tfm,
				       crypto_skcipher_get_flags(tfm) &
				       CRYPTO_TFM_REQ_MASK);
	ret = crypto_sync_skcipher_setkey(ctx->fallback_tfm, key, keylen);
	if (ret)
		return ret;
	memzero_explicit(ctx->key, sizeof(ctx->key));
	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;
	return 0;
}

static int ingenic_aes_init_tfm(struct crypto_skcipher *tfm)
{
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	const char *alg_name = crypto_tfm_alg_name(crypto_skcipher_tfm(tfm));

	ctx->fallback_tfm = crypto_alloc_sync_skcipher(alg_name, 0,
						       CRYPTO_ALG_NEED_FALLBACK);
	if (IS_ERR(ctx->fallback_tfm) && !strcmp(alg_name, "ctr(aes)")) {
		ctx->fallback_tfm = crypto_alloc_sync_skcipher("ecb(aes)", 0,
							       CRYPTO_ALG_NEED_FALLBACK);
		if (!IS_ERR(ctx->fallback_tfm))
			ctx->fallback_is_ecb = true;
	}
	if (IS_ERR(ctx->fallback_tfm)) {
		if (g_aes_dev && g_aes_dev->dev)
			dev_err(g_aes_dev->dev, "failed to allocate fallback for %s: %ld\n",
				alg_name, PTR_ERR(ctx->fallback_tfm));
		else
			pr_err("ingenic-aes: failed to allocate fallback for %s: %ld\n",
			       alg_name, PTR_ERR(ctx->fallback_tfm));
		return PTR_ERR(ctx->fallback_tfm);
	}

	return 0;
}

static void ingenic_aes_exit_tfm(struct crypto_skcipher *tfm)
{
	struct ingenic_aes_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (ctx->fallback_tfm)
		crypto_free_sync_skcipher(ctx->fallback_tfm);
	memzero_explicit(ctx->key, sizeof(ctx->key));
}

static struct skcipher_alg ingenic_aes_algs[] = {
	{
		.base.cra_name		= "ecb(aes)",
		.base.cra_driver_name	= "ecb-aes-ingenic",
		.base.cra_priority	= 300,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
					  CRYPTO_ALG_KERN_DRIVER_ONLY |
					  CRYPTO_ALG_NEED_FALLBACK,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct ingenic_aes_ctx),
		.base.cra_alignmask	= 0,
		.base.cra_module	= THIS_MODULE,
		.init			= ingenic_aes_init_tfm,
		.exit			= ingenic_aes_exit_tfm,
		.setkey			= ingenic_aes_setkey,
		.encrypt		= ingenic_aes_ecb_encrypt,
		.decrypt		= ingenic_aes_ecb_decrypt,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.chunksize		= AES_BLOCK_SIZE,
		.walksize		= AES_BLOCK_SIZE,
	},
	{
		.base.cra_name		= "cbc(aes)",
		.base.cra_driver_name	= "cbc-aes-ingenic",
		.base.cra_priority	= 300,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
					  CRYPTO_ALG_KERN_DRIVER_ONLY |
					  CRYPTO_ALG_NEED_FALLBACK,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct ingenic_aes_ctx),
		.base.cra_alignmask	= 0,
		.base.cra_module	= THIS_MODULE,
		.init			= ingenic_aes_init_tfm,
		.exit			= ingenic_aes_exit_tfm,
		.setkey			= ingenic_aes_setkey,
		.encrypt		= ingenic_aes_cbc_encrypt,
		.decrypt		= ingenic_aes_cbc_decrypt,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.chunksize		= AES_BLOCK_SIZE,
		.walksize		= AES_BLOCK_SIZE,
	},
	{
		.base.cra_name		= "ctr(aes)",
		.base.cra_driver_name	= "ctr-aes-ingenic",
		.base.cra_priority	= 300,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
					  CRYPTO_ALG_KERN_DRIVER_ONLY |
					  CRYPTO_ALG_NEED_FALLBACK,
		.base.cra_blocksize	= 1,
		.base.cra_ctxsize	= sizeof(struct ingenic_aes_ctx),
		.base.cra_alignmask	= 0,
		.base.cra_module	= THIS_MODULE,
		.init			= ingenic_aes_init_tfm,
		.exit			= ingenic_aes_exit_tfm,
		.setkey			= ingenic_aes_setkey,
		.encrypt		= ingenic_aes_ctr_crypt,
		.decrypt		= ingenic_aes_ctr_crypt,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.chunksize		= AES_BLOCK_SIZE,
		.walksize		= AES_BLOCK_SIZE,
	},
};

static int ingenic_aes_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ingenic_aes_dev *aes;
	struct resource *res;
	int ret, irq, i;

	aes = devm_kzalloc(dev, sizeof(*aes), GFP_KERNEL);
	if (!aes)
		return -ENOMEM;

	aes->dev = dev;
	init_completion(&aes->dma_done);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	aes->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(aes->base))
		return PTR_ERR(aes->base);

	aes->clk = devm_clk_get(dev, "aes");
	if (IS_ERR(aes->clk)) {
		dev_err(dev, "failed to get aes clock: %ld\n", PTR_ERR(aes->clk));
		return PTR_ERR(aes->clk);
	}

	ret = clk_prepare_enable(aes->clk);
	if (ret) {
		dev_err(dev, "failed to enable aes clock: %d\n", ret);
		return ret;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto err_clk;
	}
	ingenic_aes_reset(aes);
	ret = devm_request_irq(dev, irq, ingenic_aes_irq, 0, dev_name(dev), aes);
	if (ret)
		goto err_clk;

	aes->bounce_buf = kmalloc(4 * INGENIC_AES_SLOT_SIZE, GFP_KERNEL);
	if (!aes->bounce_buf) {
		ret = -ENOMEM;
		goto err_clk;
	}
	for (i = 0; i < ARRAY_SIZE(aes->slot); i++) {
		aes->slot[i].in = aes->bounce_buf + 2 * i * INGENIC_AES_SLOT_SIZE;
		aes->slot[i].out = aes->slot[i].in + INGENIC_AES_SLOT_SIZE;
	}

	ingenic_aes_reset(aes);
	mutex_lock(&ingenic_aes_lock);
	if (g_aes_dev) {
		mutex_unlock(&ingenic_aes_lock);
		ret = -EBUSY;
		goto err_free;
	}
	g_aes_dev = aes;
	mutex_unlock(&ingenic_aes_lock);

	platform_set_drvdata(pdev, aes);

	ret = crypto_register_skciphers(ingenic_aes_algs, ARRAY_SIZE(ingenic_aes_algs));
	if (ret) {
		dev_err(dev, "failed to register skciphers: %d\n", ret);
		goto err_bounce;
	}

	dev_dbg(dev, "AES accelerator registered (DMA)\n");
	return 0;

err_bounce:
	mutex_lock(&ingenic_aes_lock);
	g_aes_dev = NULL;
	mutex_unlock(&ingenic_aes_lock);
err_free:
	kfree(aes->bounce_buf);
err_clk:
	clk_disable_unprepare(aes->clk);
	return ret;
}

static void ingenic_aes_remove(struct platform_device *pdev)
{
	struct ingenic_aes_dev *aes = platform_get_drvdata(pdev);

	crypto_unregister_skciphers(ingenic_aes_algs, ARRAY_SIZE(ingenic_aes_algs));

	mutex_lock(&ingenic_aes_lock);
	g_aes_dev = NULL;
	mutex_unlock(&ingenic_aes_lock);

	ingenic_aes_reset(aes);
	clk_disable_unprepare(aes->clk);
	/* After a DMA timeout the buffer is leaked rather than handed back. */
	if (!aes->dead)
		kfree(aes->bounce_buf);
}

static const struct of_device_id ingenic_aes_of_match[] = {
	{ .compatible = "ingenic,t23-aes" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ingenic_aes_of_match);

static struct platform_driver ingenic_aes_driver = {
	.probe	= ingenic_aes_probe,
	.remove	= ingenic_aes_remove,
	.driver	= {
		.name		= "ingenic-aes",
		.of_match_table	= ingenic_aes_of_match,
	},
};
module_platform_driver(ingenic_aes_driver);

MODULE_DESCRIPTION("Ingenic T23 Hardware AES Accelerator Driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
