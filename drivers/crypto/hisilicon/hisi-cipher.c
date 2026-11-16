// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon multi-channel CIPHER driver
 *
 * Symmetric cipher engine (AES, DES, 3DES) found in many HiSilicon Hi35xx
 * SoCs. Channel 0 is a register-mode channel that is not used here; channels
 * 1-7 are linked-list DMA channels.
 *
 * A request takes a free channel, loads its key into the host key slot of the
 * same number, queues one input and one output node per scatterlist piece and
 * waits for the output queue interrupt. Up to seven requests run at the same
 * time. Requests the hardware cannot do (lengths that are not a multiple of
 * the cipher block, more nodes than the ring holds) go to a software fallback.
 *
 * Modes: ECB and CBC for AES, DES and 3DES (three keys, or two keys when K3
 * equals K1; keys with K1 equal to K2 or K2 equal to K3 are done in software,
 * the engine gives wrong results for them) and CTR for AES. The hardware only
 * works on whole cipher blocks.
 *
 * Queuing is done by crypto_engine in its retry mode: the engine thread hands
 * requests to do_one_request() one after the other, which sleeps while all
 * seven channels are busy, so the seven-way overlap is kept. Requests complete
 * from the threaded output interrupt handler (or the reset work) through the
 * engine.
 *
 * A request that does not finish in time resets the whole block, which cuts
 * every request on a channel. Requests still inside the deadline get to finish
 * first. A request that hit the deadline is run again in software, unless it
 * works in place (the engine may already have changed the input): that one
 * fails with -ETIMEDOUT.
 *
 * Nothing is assumed about the state the boot loader left behind: the clock
 * gate is enabled through the clock framework, the whole block is soft reset
 * through the reset controller at probe and resume, the interrupts are masked
 * before the handler is installed, all key slots are cleared and every
 * register the driver depends on is programmed explicitly.
 *
 * Queue memory (verified on hardware): both queues are arrays of 16-byte
 * nodes, same index in both queues. Every node but the last of a request
 * holds whole cipher blocks (a block split across nodes gave wrong results on
 * some chained modes):
 *   word0 data address
 *   word1 [22] last_lst, [21] iv_set, [15:0] buf_len
 *   word2 [15:0] data_len
 *   word3 IV address (input node, used with iv_set)
 */

#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/engine.h>
#include <crypto/internal/des.h>
#include <crypto/internal/skcipher.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/reset.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/unaligned.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define HC_KEY_BASE		0x0090		/* 8 slots of 8 words */
#define HC_KEY_STRIDE		0x20
#define HC_IVOUT_BASE		0x0020		/* channel n at +(n - 1) * 0x10 */
#define HC_INT_STATUS		0x1400
#define HC_INT_EN		0x1404
#define HC_INT_RAW		0x1408
#define  HC_INT_EN_ALL		BIT(31)
/*
 * chN input queue: bit N, output queue: bit 8 + N (verified on hardware; some
 * data sheets print the two halves swapped).
 */
#define  HC_INT_OBUF(ch)	BIT(8 + (ch))
#define HC_RST_STATUS		0x140c

/* Per-channel registers, channel n (1-7) at 0x1000 + n * 0x80 */
#define HC_CHAN(n)		(0x1000 + (n) * 0x80)
#define HC_IBUF_NUM		0x00
#define HC_IBUF_CNT		0x04
#define HC_IEMPTY_CNT		0x08
#define HC_INT_ICNTCFG		0x0c
#define HC_CTRL			0x10
#define  CTRL_DECRYPT		BIT(0)
#define  CTRL_MODE		GENMASK(3, 1)
#define  CTRL_ALG_SEL		GENMASK(5, 4)
#define  CTRL_IVIN_SEL		BIT(8)
#define  CTRL_KEY_LENGTH	GENMASK(10, 9)
#define  CTRL_KEY_ADDER		GENMASK(16, 14)
#define HC_SRC_LST_SADDR	0x14
#define HC_IAGE_TIMER		0x18
#define HC_OBUF_NUM		0x3c
#define HC_OBUF_CNT		0x40
#define HC_OFULL_CNT		0x44
#define HC_INT_OCNTCFG		0x48
#define HC_DEST_LST_SADDR	0x4c
#define HC_OAGE_TIMER		0x50

#define MODE_ECB		0
#define MODE_CBC		1
#define MODE_CTR		4
#define KEYLEN_2KEY		3	/* DES/3DES with two keys */
#define ALG_DES			0
#define ALG_3DES		1
#define ALG_AES			2

#define NODE_LAST		BIT(22)
#define NODE_IV_SET		BIT(21)
#define NODE_BUF_LEN		GENMASK(15, 0)
#define NODE_MAX_LEN		0xfff0

#define HC_RING			128		/* nodes per queue */
#define HC_NUM_CHAN		7		/* channels 1..7 */
/*
 * Request deadline: HC_TIMEOUT plus 1 ms per KiB (a 1 MB/s floor, far below
 * 3DES on the ARM926). A request is at most HC_RING nodes of NODE_MAX_LEN bytes.
 */
#define HC_TIMEOUT		(2 * HZ)
#define HC_MS_PER_KIB		1
#define HC_MAX_KIB		(HC_RING * NODE_MAX_LEN / 1024)
#define HC_TIMEOUT_MAX		(HC_TIMEOUT + msecs_to_jiffies(HC_MAX_KIB * HC_MS_PER_KIB))
#define HC_RST_SETTLE_US	50
#define HC_QUEUE_LEN		64
#define HC_MIN_HW_LEN		128		/* smaller requests are cheaper in software */

struct hc_node {
	__le32 addr;
	__le32 cas;
	__le32 len;
	__le32 iv_addr;
};

struct hc_chan_mem {
	struct hc_node in[HC_RING];
	struct hc_node out[HC_RING];
	u8 iv[16];
};

struct hc_dev;

struct hc_chan {
	struct hc_dev *hc;
	unsigned int id;			/* 1..7 */
	struct hc_chan_mem *mem;
	dma_addr_t mem_dma;
	struct skcipher_request *req;
	const void *key_ctx;			/* tfm whose key is in the slot */
	u32 key_gen;
	unsigned long deadline;			/* jiffies, timeout of the running request */
	unsigned int idx;			/* next ring slot (hardware pointer) */
	unsigned int nodes;			/* nodes of the running request */
	struct timer_list timer;
};

#define HC_NUM_ALGS		7

struct hc_alg {
	struct skcipher_engine_alg alg;
	struct hc_dev *hc;	/* owner, set in probe */
	u32 ctrl_alg;		/* ALG_* */
	u32 mode;		/* MODE_* */
	unsigned int blk;	/* cipher block size, node length granularity */
	atomic_t ref_held;	/* a reference on hc is held for this algorithm */
};

/*
 * Lifetime: one reference from probe (dropped by devres, after remove), one
 * per tfm and one per registered algorithm (dropped by hc_alg_destroy() once
 * the unregistered algorithm has no users left). Only the memory stays after
 * remove(); the registers and the queue memory are gone, which is why `dead`
 * is checked wherever hardware is used.
 */
struct hc_dev {
	struct kref ref;
	bool dead;				/* remove() ran, under lock */
	bool resetting;				/* an engine reset is under way, under lock */
	bool failed;				/* reset failed, fallback only, under lock */
	bool clk_on;				/* clock enabled by this driver (not in sleep) */
	bool suspended;				/* system sleep: fallback only, under lock */
	struct clk *clk;
	atomic_t key_gen;			/* changes with every setkey */
	struct hc_alg algs[HC_NUM_ALGS];
	struct device *dev;
	void __iomem *base;
	struct reset_control *rst;
	spinlock_t lock;			/* channels and backlog */
	struct crypto_engine *engine;
	wait_queue_head_t wq;			/* do_one_request() waits for a channel */
	struct hc_chan chan[HC_NUM_CHAN + 1];	/* [0] unused */
	unsigned int busy;			/* bit per channel in use */
	atomic_t done;				/* output interrupts seen */
	struct work_struct reset_work;
	int irq;
	struct dentry *dbg;
	/* request counters, shown in debugfs */
	atomic_t n_hw, n_fb_len, n_fb_small, n_fb_map, n_fb_nodes, n_fb_queue, n_timeout;
};

struct hc_ctx {
	struct hc_dev *hc;	/* reference held from init to exit */
	u8 key[AES_MAX_KEY_SIZE];
	unsigned int keylen;
	unsigned int hw_keylen;	/* bytes written to the key slot */
	u32 key_gen;		/* from hc_dev.key_gen, changes with every setkey */
	bool sw_only;		/* key the engine gets wrong: software only */
	u32 key_length;		/* CTRL.key_length encoding */
	const struct hc_alg *info;
	struct crypto_skcipher *fallback;
};

struct hc_reqctx {
	bool decrypt;
	bool bidir;		/* source and destination share memory */
	struct hc_chan *chan;
	struct scatterlist *src, *dst;
	int src_nents, dst_nents;
	/* must be last */
	struct skcipher_request fallback_req;
};

static void hc_release(struct kref *ref)
{
	kfree(container_of(ref, struct hc_dev, ref));
}

static void hc_put(struct hc_dev *hc)
{
	kref_put(&hc->ref, hc_release);
}

static void hc_put_action(void *data)
{
	hc_put(data);
}

/* The clock may be off (failed resume): disable it only if it is on */
static void hc_clk_off(void *data)
{
	struct hc_dev *hc = data;

	if (hc->clk_on)
		clk_disable_unprepare(hc->clk);
}

static const struct hc_alg *hc_to_info(struct crypto_skcipher *tfm)
{
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);

	return container_of(alg, struct hc_alg, alg.base);
}

static u32 hc_rd(struct hc_dev *hc, unsigned int off)
{
	return readl(hc->base + off);
}

static void hc_wr(struct hc_dev *hc, unsigned int off, u32 val)
{
	writel(val, hc->base + off);
}

static void hc_chan_wr(struct hc_chan *ch, unsigned int off, u32 val)
{
	writel(val, ch->hc->base + HC_CHAN(ch->id) + off);
}

static u32 hc_chan_rd(struct hc_chan *ch, unsigned int off)
{
	return readl(ch->hc->base + HC_CHAN(ch->id) + off);
}

static void hc_wipe_key(struct hc_dev *hc, unsigned int id)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		writel(0, hc->base + HC_KEY_BASE + id * HC_KEY_STRIDE + 4 * i);
}

/* Clear every key slot, including the one of channel 0 */
static void hc_wipe_all_keys(struct hc_dev *hc)
{
	unsigned int id;

	for (id = 0; id <= HC_NUM_CHAN; id++)
		hc_wipe_key(hc, id);
}

/* (Re)program the static queue registers of all channels */
static void hc_hw_init(struct hc_dev *hc)
{
	unsigned int i;

	hc_wr(hc, HC_INT_EN, 0);
	hc_wr(hc, HC_INT_RAW, 0xffff);
	hc_wipe_all_keys(hc);

	for (i = 1; i <= HC_NUM_CHAN; i++) {
		struct hc_chan *ch = &hc->chan[i];

		ch->idx = 0;
		ch->nodes = 0;
		ch->key_ctx = NULL;	/* a reset loses the keys */
		hc_chan_wr(ch, HC_SRC_LST_SADDR,
			   ch->mem_dma + offsetof(struct hc_chan_mem, in));
		hc_chan_wr(ch, HC_DEST_LST_SADDR,
			   ch->mem_dma + offsetof(struct hc_chan_mem, out));
		hc_chan_wr(ch, HC_IBUF_NUM, HC_RING);
		hc_chan_wr(ch, HC_OBUF_NUM, HC_RING);
		hc_chan_wr(ch, HC_IBUF_CNT, 0);
		hc_chan_wr(ch, HC_OBUF_CNT, HC_RING);
		hc_chan_wr(ch, HC_IAGE_TIMER, 0);
		hc_chan_wr(ch, HC_OAGE_TIMER, 0);
		hc_chan_wr(ch, HC_INT_ICNTCFG, 0);
		hc_chan_wr(ch, HC_INT_OCNTCFG, 0);
		/* no leftover mode or key slot selection from an earlier user */
		hc_chan_wr(ch, HC_CTRL, 0);
	}

	hc_wr(hc, HC_INT_EN, HC_INT_EN_ALL | GENMASK(8 + HC_NUM_CHAN, 9));
}

static int hc_hw_reset(struct hc_dev *hc)
{
	u32 val;
	int ret;

	/* No interrupt may be raised, or its handler run, while the block is in reset */
	hc_wr(hc, HC_INT_EN, 0);
	ret = reset_control_assert(hc->rst);
	if (ret)
		return ret;
	usleep_range(10, 20);
	ret = reset_control_deassert(hc->rst);
	if (ret)
		return ret;
	/*
	 * RST_STATUS reads 1 at reset on some SoCs, so polling it alone could pass
	 * before the reset is over: always give the block time to settle first.
	 */
	usleep_range(HC_RST_SETTLE_US, 2 * HC_RST_SETTLE_US);

	return readl_poll_timeout(hc->base + HC_RST_STATUS, val, val & 1, 10,
				  10000);
}

static void hc_get_block(void __iomem *reg, u8 *dst, unsigned int words)
{
	unsigned int i;

	for (i = 0; i < words; i++)
		put_unaligned_le32(readl(reg + 4 * i), dst + 4 * i);
}

/*
 * Walk the mapped source and destination lists together and cut them into
 * node pairs. With fill = false only count the nodes.
 */
static int hc_walk(struct hc_chan *ch, struct skcipher_request *req,
		   struct hc_reqctx *rctx, bool fill)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct scatterlist *s = rctx->src, *d = rctx->dst;
	unsigned int s_off = 0, d_off = 0, left = req->cryptlen;
	unsigned int n = 0, ns = rctx->src_nents, nd = rctx->dst_nents;
	bool iv = ctx->info->alg.base.ivsize;

	while (left) {
		unsigned int len;
		struct hc_node *in, *out;

		if (!ns || !nd)
			return -EINVAL;

		len = min3(left, sg_dma_len(s) - s_off, sg_dma_len(d) - d_off);
		len = min_t(unsigned int, len, NODE_MAX_LEN);
		if (len < left)
			len = round_down(len, ctx->info->blk);
		if (!len)
			return -EINVAL;

		if (fill) {
			unsigned int slot = (ch->idx + n) % HC_RING;
			bool last = len == left;

			in = &ch->mem->in[slot];
			out = &ch->mem->out[slot];
			in->addr = cpu_to_le32(sg_dma_address(s) + s_off);
			out->addr = cpu_to_le32(sg_dma_address(d) + d_off);
			in->len = cpu_to_le32(len);
			out->len = cpu_to_le32(len);
			in->cas = cpu_to_le32(FIELD_PREP(NODE_BUF_LEN, len) |
					      (last ? NODE_LAST : 0) |
					      (iv && !n ? NODE_IV_SET : 0));
			out->cas = cpu_to_le32(FIELD_PREP(NODE_BUF_LEN, len) |
					       (last ? NODE_LAST : 0));
			in->iv_addr = cpu_to_le32(ch->mem_dma +
				offsetof(struct hc_chan_mem, iv));
			out->iv_addr = 0;
		}

		n++;
		left -= len;
		s_off += len;
		d_off += len;
		if (s_off == sg_dma_len(s)) {
			s = sg_next(s);
			s_off = 0;
			ns--;
		}
		if (d_off == sg_dma_len(d)) {
			d = sg_next(d);
			d_off = 0;
			nd--;
		}
	}

	return n;
}

static bool hc_in_place(struct skcipher_request *req);

static void hc_unmap(struct hc_dev *hc, struct skcipher_request *req)
{
	struct hc_reqctx *rctx = skcipher_request_ctx(req);
	struct device *dev = hc->dev;
	int nents = sg_nents_for_len(req->src, req->cryptlen);

	if (req->src == req->dst) {
		if (rctx->src_nents)
			dma_unmap_sg(dev, req->src, nents, DMA_BIDIRECTIONAL);
	} else {
		if (rctx->src_nents)
			dma_unmap_sg(dev, req->src, nents,
				     DMA_TO_DEVICE);
		if (rctx->dst_nents)
			dma_unmap_sg(dev, req->dst,
				     sg_nents_for_len(req->dst, req->cryptlen),
				     rctx->bidir ? DMA_BIDIRECTIONAL : DMA_FROM_DEVICE);
	}
	rctx->chan = NULL;
}

static int hc_fallback(struct skcipher_request *req, bool decrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct hc_reqctx *rctx = skcipher_request_ctx(req);
	struct skcipher_request *sub = &rctx->fallback_req;

	skcipher_request_set_tfm(sub, ctx->fallback);
	skcipher_request_set_callback(sub, req->base.flags,
				      req->base.complete, req->base.data);
	skcipher_request_set_crypt(sub, req->src, req->dst, req->cryptlen,
				   req->iv);

	return decrypt ? crypto_skcipher_decrypt(sub) :
			 crypto_skcipher_encrypt(sub);
}

/* Start a request on a channel; -EAGAIN: use the fallback instead */
static int hc_start(struct hc_chan *ch, struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct hc_reqctx *rctx = skcipher_request_ctx(req);
	struct hc_dev *hc = ch->hc;
	unsigned int ivsize = ctx->info->alg.base.ivsize;
	int nents_s, nents_d, nodes, i;
	unsigned long flags;
	u32 ctrl;

	nents_s = sg_nents_for_len(req->src, req->cryptlen);
	nents_d = sg_nents_for_len(req->dst, req->cryptlen);
	if (nents_s < 0 || nents_d < 0)
		return -EINVAL;

	rctx->bidir = hc_in_place(req);
	if (req->src == req->dst) {
		rctx->src_nents = dma_map_sg(hc->dev, req->src, nents_s,
					     DMA_BIDIRECTIONAL);
		rctx->dst_nents = rctx->src_nents;
	} else {
		/*
		 * Overlapping lists: the engine writes memory that the source
		 * mapping covers. The destination is then mapped both ways (the
		 * lines are cleaned before and invalidated after); the source
		 * stays DMA_TO_DEVICE, which the DMA API debugging does not
		 * track, so the overlap is not reported as a double mapping.
		 */
		rctx->src_nents = dma_map_sg(hc->dev, req->src, nents_s,
					     DMA_TO_DEVICE);
		rctx->dst_nents = dma_map_sg(hc->dev, req->dst, nents_d,
					     rctx->bidir ? DMA_BIDIRECTIONAL : DMA_FROM_DEVICE);
	}
	rctx->src = req->src;
	rctx->dst = req->dst;
	rctx->chan = ch;
	if (!rctx->src_nents || !rctx->dst_nents) {
		atomic_inc(&hc->n_fb_map);
		hc_unmap(hc, req);
		return -EAGAIN;
	}

	nodes = hc_walk(ch, req, rctx, false);
	if (nodes <= 0 || nodes > HC_RING) {
		atomic_inc(&hc->n_fb_nodes);
		hc_unmap(hc, req);
		return -EAGAIN;
	}

	/*
	 * From here the hardware is touched. The ring position (ch->idx) is reset
	 * by the engine reset under the lock, so the nodes are filled under the
	 * lock too: remove() and the reset work either see ch->req and fail the
	 * request, or this sees the new state and backs out.
	 */
	spin_lock_irqsave(&hc->lock, flags);
	if (hc->dead || hc->failed || hc->resetting || hc->suspended) {
		spin_unlock_irqrestore(&hc->lock, flags);
		hc_unmap(hc, req);
		return -EAGAIN;
	}

	hc_walk(ch, req, rctx, true);
	if (ivsize)
		memcpy(ch->mem->iv, req->iv, ivsize);

	/* Key slot = channel number; reload only when another key was used */
	if (ch->key_ctx != ctx || ch->key_gen != ctx->key_gen) {
		for (i = 0; i < ctx->hw_keylen / 4; i++)
			writel(get_unaligned_le32(ctx->key + 4 * i),
			       hc->base + HC_KEY_BASE + ch->id * HC_KEY_STRIDE +
			       4 * i);
		ch->key_ctx = ctx;
		ch->key_gen = ctx->key_gen;
	}

	ctrl = FIELD_PREP(CTRL_ALG_SEL, ctx->info->ctrl_alg) |
	       FIELD_PREP(CTRL_MODE, ctx->info->mode) |
	       FIELD_PREP(CTRL_KEY_LENGTH, ctx->key_length) |
	       FIELD_PREP(CTRL_KEY_ADDER, ch->id);
	if (rctx->decrypt)
		ctrl |= CTRL_DECRYPT;
	if (ivsize)
		ctrl |= CTRL_IVIN_SEL;
	hc_chan_wr(ch, HC_CTRL, ctrl);

	atomic_inc(&hc->n_hw);
	ch->req = req;
	ch->nodes = nodes;
	/* The output interrupt fires at count >= threshold: once all are done */
	hc_chan_wr(ch, HC_INT_OCNTCFG, nodes);
	ch->deadline = jiffies + HC_TIMEOUT +
		       msecs_to_jiffies(req->cryptlen / 1024 * HC_MS_PER_KIB);
	mod_timer(&ch->timer, ch->deadline);

	/* writel() orders the node and key writes before this kick */
	hc_chan_wr(ch, HC_IBUF_CNT, nodes);
	spin_unlock_irqrestore(&hc->lock, flags);

	return -EINPROGRESS;
}

/* A request taken off its channel, finished once the lock is dropped */
struct hc_done {
	struct skcipher_request *req;
	int err;
	u8 iv[AES_BLOCK_SIZE];	/* output IV, valid when err is 0 */
};

/*
 * With hc->lock held: take the request off its channel and release the
 * channel. The buffers are unmapped later, without the lock, by
 * hc_unmap_done().
 */
static void hc_detach(struct hc_chan *ch, int err, struct hc_done *d)
{
	struct hc_dev *hc = ch->hc;
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(ch->req);
	unsigned int ivsize = crypto_skcipher_ivsize(tfm);
	unsigned int n = ch->nodes;

	if (!err) {
		/* Acknowledge: release the slots in both queues */
		hc_chan_wr(ch, HC_IEMPTY_CNT, n);
		hc_chan_wr(ch, HC_OFULL_CNT, n);
		hc_chan_wr(ch, HC_OBUF_CNT, n);
		ch->idx = (ch->idx + n) % HC_RING;
		/* Read now: the next request on this channel overwrites it */
		if (ivsize)
			hc_get_block(hc->base + HC_IVOUT_BASE + (ch->id - 1) * 0x10,
				     d->iv, ivsize / 4);
	} else {
		/* The engine is reset: the key slots are not trusted */
		hc_wipe_key(hc, ch->id);
		ch->key_ctx = NULL;
	}
	d->req = ch->req;
	d->err = err;
	ch->req = NULL;
	ch->nodes = 0;
	hc->busy &= ~BIT(ch->id);
}

static void hc_unmap_done(struct hc_dev *hc, struct hc_done *d)
{
	unsigned int ivsize = crypto_skcipher_ivsize(crypto_skcipher_reqtfm(d->req));

	hc_unmap(hc, d->req);
	/*
	 * The output IV is written only after the buffers are unmapped: req->iv can
	 * share a cache line with a mapped buffer (the generic gcm template puts its
	 * counter block and the tag buffer next to each other), and a CPU write to
	 * the line before the unmap makes the unmap write the stale cached bytes
	 * over the data the engine wrote.
	 */
	if (!d->err && ivsize)
		memcpy(d->req->iv, d->iv, ivsize);
}

/*
 * Complete a request through the engine. The engine wants softirq context, so
 * this is also used from the threaded interrupt handler, the reset work and
 * remove(), which run in process context.
 */
static void hc_complete(struct hc_dev *hc, struct skcipher_request *req, int err)
{
	local_bh_disable();
	crypto_finalize_skcipher_request(hc->engine, req, err);
	local_bh_enable();
}

static irqreturn_t hc_irq_thread(int irq, void *data)
{
	struct hc_dev *hc = data;
	unsigned int done = atomic_xchg(&hc->done, 0);
	unsigned int id;

	for (id = 1; id <= HC_NUM_CHAN; id++) {
		struct hc_chan *ch = &hc->chan[id];
		struct hc_done d;
		unsigned long flags;

		if (!(done & BIT(id)))
			continue;

		spin_lock_irqsave(&hc->lock, flags);
		/*
		 * An engine reset takes every request off its channel under this lock
		 * with the interrupts masked, so the registers of a block in reset are
		 * never read here.
		 */
		if (!ch->req || hc_chan_rd(ch, HC_OFULL_CNT) < ch->nodes) {
			spin_unlock_irqrestore(&hc->lock, flags);
			continue;
		}
		timer_delete(&ch->timer);
		hc_detach(ch, 0, &d);
		spin_unlock_irqrestore(&hc->lock, flags);

		wake_up(&hc->wq);
		hc_unmap_done(hc, &d);
		hc_complete(hc, d.req, 0);
	}

	return IRQ_HANDLED;
}

static irqreturn_t hc_irq(int irq, void *data)
{
	struct hc_dev *hc = data;
	u32 status = hc_rd(hc, HC_INT_STATUS);
	unsigned int id, done = 0;

	if (!status)
		return IRQ_NONE;

	hc_wr(hc, HC_INT_RAW, status);
	for (id = 1; id <= HC_NUM_CHAN; id++)
		if (status & HC_INT_OBUF(id))
			done |= BIT(id);
	if (!done)
		return IRQ_HANDLED;
	atomic_or(done, &hc->done);

	return IRQ_WAKE_THREAD;
}

/* Run a request in software and complete it (an asynchronous fallback completes itself) */
static void hc_fallback_complete(struct hc_dev *hc, struct skcipher_request *req)
{
	struct hc_reqctx *rctx = skcipher_request_ctx(req);
	int ret = hc_fallback(req, rctx->decrypt);

	if (ret == -EINPROGRESS || ret == -EBUSY)
		return;
	hc_complete(hc, req, ret);
}

/*
 * Wait until every request still on a channel is past its deadline, so that a
 * channel that is only slower than a hung one is not cut by the reset. False
 * when the device was removed meanwhile (remove() takes over); remove() itself
 * passes removing = true.
 */
static bool hc_wait_inflight(struct hc_dev *hc, bool removing)
{
	unsigned long flags;
	unsigned int id;
	bool wait;

	for (;;) {
		wait = false;
		spin_lock_irqsave(&hc->lock, flags);
		if (hc->dead && !removing) {
			spin_unlock_irqrestore(&hc->lock, flags);
			return false;
		}
		for (id = 1; id <= HC_NUM_CHAN; id++)
			if (hc->chan[id].req && time_before(jiffies, hc->chan[id].deadline))
				wait = true;
		spin_unlock_irqrestore(&hc->lock, flags);
		if (!wait)
			return true;
		usleep_range(500, 1000);
	}
}

/* Whether source and destination share memory: the engine may have changed the input */
static bool hc_in_place(struct skcipher_request *req)
{
	struct scatterlist *s, *d;
	unsigned int ls = req->cryptlen, ld;

	if (req->src == req->dst)
		return true;
	for (s = req->src; s && ls; s = sg_next(s)) {
		unsigned int n = min(ls, s->length);

		ls -= n;
		ld = req->cryptlen;
		for (d = req->dst; d && ld; d = sg_next(d)) {
			unsigned int m = min(ld, d->length);

			ld -= m;
			/* u64: a range ending at 4 GiB must not wrap in a 32-bit dma_addr_t */
			if ((u64)sg_phys(s) < (u64)sg_phys(d) + m &&
			    (u64)sg_phys(d) < (u64)sg_phys(s) + n)
				return true;
		}
	}

	return false;
}

/*
 * Reset the engine. Every request left on a channel is past its deadline. The
 * interrupts are masked and the requests are taken off the channels under the
 * lock, so no register is read while the block is in reset. A request the engine
 * finished meanwhile completes normally. For the others the buffers are
 * unmapped only after the reset stopped the DMA; then they run in software,
 * except a request that works in place: the engine may have changed its input,
 * so it fails with -ETIMEDOUT (-ESHUTDOWN on removal). If the reset itself
 * fails (including a reset controller error) the buffers stay mapped and those
 * requests are not completed: the DMA may still write to them, and a stuck
 * caller is better than a write into memory it has freed.
 */
static void hc_stop_engine(struct hc_dev *hc, bool shutdown)
{
	struct hc_done done[HC_NUM_CHAN + 1] = {};
	unsigned long flags;
	unsigned int id;
	int ret;

	spin_lock_irqsave(&hc->lock, flags);
	hc_wr(hc, HC_INT_EN, 0);
	hc->resetting = true;	/* new work backs out while the reset sleeps */
	for (id = 1; id <= HC_NUM_CHAN; id++) {
		struct hc_chan *ch = &hc->chan[id];
		int err = 0;

		if (!ch->req)
			continue;
		timer_delete(&ch->timer);
		if (hc_chan_rd(ch, HC_OFULL_CNT) < ch->nodes) {
			dev_err(hc->dev, "channel %u timed out, resetting the engine\n", id);
			atomic_inc(&hc->n_timeout);
			err = -ETIMEDOUT;
		}
		hc_detach(ch, err, &done[id]);
	}
	spin_unlock_irqrestore(&hc->lock, flags);

	synchronize_irq(hc->irq);
	ret = hc_hw_reset(hc);
	if (ret)
		dev_err(hc->dev, "engine does not leave reset (%d), using the software fallback\n",
			ret);

	spin_lock_irqsave(&hc->lock, flags);
	if (ret)
		hc->failed = true;
	else if (!shutdown)
		hc_hw_init(hc);
	if (shutdown)
		hc_wipe_all_keys(hc);
	hc->resetting = false;
	spin_unlock_irqrestore(&hc->lock, flags);
	wake_up(&hc->wq);

	for (id = 1; id <= HC_NUM_CHAN; id++) {
		struct hc_done *d = &done[id];

		if (!d->req)
			continue;
		if (!d->err) {
			hc_unmap_done(hc, d);
			hc_complete(hc, d->req, 0);
			continue;
		}
		if (ret) {
			dev_err(hc->dev, "channel %u: request left pending, its buffers stay mapped\n",
				id);
			continue;
		}
		hc_unmap(hc, d->req);
		if (hc_in_place(d->req))
			hc_complete(hc, d->req, shutdown ? -ESHUTDOWN : -ETIMEDOUT);
		else
			hc_fallback_complete(hc, d->req);
	}
}

/* A channel did not finish: reset the engine, the other requests get to finish first */
static void hc_reset_work(struct work_struct *work)
{
	struct hc_dev *hc = container_of(work, struct hc_dev, reset_work);
	unsigned long flags;
	unsigned int id;
	bool expired = false;

	spin_lock_irqsave(&hc->lock, flags);
	if (hc->dead || hc->resetting || hc->failed) {
		spin_unlock_irqrestore(&hc->lock, flags);
		return;
	}
	/* The timer of a request that finished meanwhile may still have queued this */
	for (id = 1; id <= HC_NUM_CHAN; id++) {
		struct hc_chan *ch = &hc->chan[id];

		if (ch->req && time_after_eq(jiffies, ch->deadline))
			expired = true;
	}
	if (!expired) {
		spin_unlock_irqrestore(&hc->lock, flags);
		return;
	}
	/* No new request starts until the reset is over */
	hc->resetting = true;
	spin_unlock_irqrestore(&hc->lock, flags);

	if (hc_wait_inflight(hc, false))
		hc_stop_engine(hc, false);
}

static void hc_timeout(struct timer_list *t)
{
	struct hc_chan *ch = timer_container_of(ch, t, timer);

	schedule_work(&ch->hc->reset_work);
}

static bool hc_can_claim(struct hc_dev *hc)
{
	return READ_ONCE(hc->dead) || READ_ONCE(hc->failed) || READ_ONCE(hc->suspended) ||
	       (!READ_ONCE(hc->resetting) &&
		READ_ONCE(hc->busy) != GENMASK(HC_NUM_CHAN, 1));
}

/*
 * Engine callback, in the engine thread (may sleep): take a free channel and
 * start the request on it. Returns 0 when the request was started or completed
 * (the completion comes from the interrupt or the fallback).
 */
static int hc_claim_chan(struct hc_dev *hc, unsigned int *idp)
{
	unsigned long flags;
	unsigned int id;

	for (;;) {
		wait_event(hc->wq, hc_can_claim(hc));
		spin_lock_irqsave(&hc->lock, flags);
		if (hc->dead || hc->failed || hc->suspended) {
			spin_unlock_irqrestore(&hc->lock, flags);
			return -EAGAIN;
		}
		if (hc->resetting || hc->busy == GENMASK(HC_NUM_CHAN, 1)) {
			spin_unlock_irqrestore(&hc->lock, flags);
			continue;
		}
		id = ffz(hc->busy | BIT(0));
		hc->busy |= BIT(id);
		spin_unlock_irqrestore(&hc->lock, flags);
		*idp = id;

		return 0;
	}
}

static void hc_release_chan(struct hc_dev *hc, unsigned int id)
{
	unsigned long flags;

	spin_lock_irqsave(&hc->lock, flags);
	hc->busy &= ~BIT(id);
	spin_unlock_irqrestore(&hc->lock, flags);
	wake_up(&hc->wq);	/* suspend waits for all channels to be free */
}

static int hc_do_one_request(struct crypto_engine *engine, void *areq)
{
	struct skcipher_request *req = skcipher_request_cast(areq);
	struct hc_ctx *ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct hc_dev *hc = ctx->hc;
	unsigned int id;
	int ret;

	ret = hc_claim_chan(hc, &id);
	if (!ret) {
		ret = hc_start(&hc->chan[id], req);
		if (ret == -EINPROGRESS)
			return 0;
		hc_release_chan(hc, id);
	}

	if (ret == -EAGAIN)
		hc_fallback_complete(hc, req);
	else
		hc_complete(hc, req, ret);

	return 0;
}

static int hc_crypt(struct skcipher_request *req, bool decrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct hc_reqctx *rctx = skcipher_request_ctx(req);
	struct hc_dev *hc = ctx->hc;
	unsigned long flags;
	int ret;

	rctx->decrypt = decrypt;

	if (!req->cryptlen)
		return 0;
	if (!IS_ALIGNED(req->cryptlen, ctx->info->blk)) {
		atomic_inc(&hc->n_fb_len);
		return hc_fallback(req, decrypt);
	}
	if (req->cryptlen < HC_MIN_HW_LEN || ctx->sw_only) {
		atomic_inc(&hc->n_fb_small);
		return hc_fallback(req, decrypt);
	}

	/*
	 * The engine is only used under the lock and while !dead: remove() sets
	 * dead before it stops the engine. Queuing does not sleep.
	 */
	spin_lock_irqsave(&hc->lock, flags);
	if (hc->dead || hc->failed || hc->suspended) {
		spin_unlock_irqrestore(&hc->lock, flags);
		return hc_fallback(req, decrypt);
	}
	ret = crypto_transfer_skcipher_request_to_engine(hc->engine, req);
	spin_unlock_irqrestore(&hc->lock, flags);
	if (ret == -ENOSPC) {
		/* the engine queue is full and the caller may not wait: do it in software */
		atomic_inc(&hc->n_fb_queue);
		return hc_fallback(req, decrypt);
	}

	return ret;
}

static int hc_encrypt(struct skcipher_request *req)
{
	return hc_crypt(req, false);
}

static int hc_decrypt(struct skcipher_request *req)
{
	return hc_crypt(req, true);
}

static int hc_setkey(struct crypto_skcipher *tfm, const u8 *key,
		     unsigned int keylen)
{
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	const struct hc_alg *info = ctx->info;
	unsigned int key_length = 0, hw_keylen = keylen;
	bool sw_only = false;
	int ret;

	/* Check the key first and change nothing in the context yet */
	switch (info->ctrl_alg) {
	case ALG_AES:
		ret = aes_check_keylen(keylen);
		if (ret)
			return ret;
		key_length = (keylen - 16) / 8;
		break;
	case ALG_DES:
		ret = verify_skcipher_des_key(tfm, key);
		if (ret)
			return ret;
		break;
	default:
		ret = verify_skcipher_des3_key(tfm, key);
		if (ret)
			return ret;
		/*
		 * K1 == K2 or K2 == K3 (the all-equal key included) makes EDE a
		 * single DES: the engine does not get these right, software does.
		 * K3 == K1 is two-key 3DES, which the engine has a mode for.
		 */
		sw_only = !memcmp(key, key + 8, 8) || !memcmp(key + 8, key + 16, 8);
		if (!memcmp(key, key + 16, 8)) {
			key_length = KEYLEN_2KEY;
			hw_keylen = 16;
		}
		break;
	}

	/*
	 * The fallback gets the key before the hardware state changes: if it
	 * rejects the key, the context keeps the key it had and both paths
	 * still agree.
	 */
	crypto_skcipher_clear_flags(ctx->fallback, CRYPTO_TFM_REQ_MASK);
	crypto_skcipher_set_flags(ctx->fallback,
				  crypto_skcipher_get_flags(tfm) &
				  CRYPTO_TFM_REQ_MASK);
	ret = crypto_skcipher_setkey(ctx->fallback, key, keylen);
	if (ret)
		return ret;

	ctx->sw_only = sw_only;
	ctx->key_length = key_length;
	ctx->hw_keylen = hw_keylen;
	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;
	ctx->key_gen = atomic_inc_return(&ctx->hc->key_gen);

	return 0;
}

static int hc_init_tfm(struct crypto_skcipher *tfm)
{
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	const char *name = crypto_tfm_alg_name(&tfm->base);

	ctx->info = hc_to_info(tfm);
	ctx->hc = ctx->info->hc;
	/* synchronous only: the engine completes the request itself */
	ctx->fallback = crypto_alloc_skcipher(name, 0,
					      CRYPTO_ALG_NEED_FALLBACK |
					      CRYPTO_ALG_ASYNC);
	if (IS_ERR(ctx->fallback))
		return PTR_ERR(ctx->fallback);
	kref_get(&ctx->hc->ref);

	crypto_skcipher_set_reqsize(tfm, sizeof(struct hc_reqctx) +
				    crypto_skcipher_reqsize(ctx->fallback));

	return 0;
}

static void hc_exit_tfm(struct crypto_skcipher *tfm)
{
	struct hc_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct hc_dev *hc = ctx->hc;
	unsigned long flags;
	unsigned int id;

	/*
	 * Do not leave the key of a freed tfm in the engine. The slots are only
	 * accessed while the block is up: not after remove(), while it is being
	 * reset, after a failed reset, or in system sleep (clock gated).
	 */
	spin_lock_irqsave(&hc->lock, flags);
	for (id = 1; id <= HC_NUM_CHAN; id++) {
		if (hc->chan[id].key_ctx != ctx || hc->chan[id].req)
			continue;
		if (!hc->dead && !hc->failed && !hc->resetting && !hc->suspended)
			hc_wipe_key(hc, id);
		hc->chan[id].key_ctx = NULL;
	}
	spin_unlock_irqrestore(&hc->lock, flags);
	memzero_explicit(ctx->key, sizeof(ctx->key));
	crypto_free_skcipher(ctx->fallback);
	hc_put(hc);
}

#define HC_FLAGS (CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK)

#define HC_ALGF(_flags, _name, _drv, _blk, _bs, _min, _max, _iv, _calg, _mode) \
	{								\
		.alg = {						\
			.base = {					\
				.base.cra_name = _name,			\
				.base.cra_driver_name = _drv,		\
				.base.cra_priority = 300,		\
				.base.cra_flags = _flags,		\
				.base.cra_blocksize = _bs,		\
				.base.cra_ctxsize = sizeof(struct hc_ctx), \
				.base.cra_module = THIS_MODULE,		\
				.min_keysize = _min,			\
				.max_keysize = _max,			\
				.ivsize = _iv,				\
				.chunksize = _blk,			\
				.setkey = hc_setkey,			\
				.encrypt = hc_encrypt,			\
				.decrypt = hc_decrypt,			\
				.init = hc_init_tfm,			\
				.exit = hc_exit_tfm,			\
			},						\
			.op.do_one_request = hc_do_one_request,		\
		},							\
		.ctrl_alg = _calg,					\
		.mode = _mode,						\
		.blk = _blk,						\
	}

static const struct hc_alg hc_alg_tmpl[HC_NUM_ALGS] = {
	HC_ALGF(HC_FLAGS, "ecb(aes)", "hisi-ecb-aes", 16, 16, 16, 32, 0, ALG_AES, MODE_ECB),
	HC_ALGF(HC_FLAGS, "cbc(aes)", "hisi-cbc-aes", 16, 16, 16, 32, 16, ALG_AES, MODE_CBC),
	HC_ALGF(HC_FLAGS, "ctr(aes)", "hisi-ctr-aes", 16, 1, 16, 32, 16, ALG_AES, MODE_CTR),
	HC_ALGF(HC_FLAGS, "ecb(des)", "hisi-ecb-des", 8, 8, 8, 8, 0, ALG_DES, MODE_ECB),
	HC_ALGF(HC_FLAGS, "cbc(des)", "hisi-cbc-des", 8, 8, 8, 8, 8, ALG_DES, MODE_CBC),
	HC_ALGF(HC_FLAGS, "ecb(des3_ede)", "hisi-ecb-des3", 8, 8, 24, 24, 0, ALG_3DES, MODE_ECB),
	HC_ALGF(HC_FLAGS, "cbc(des3_ede)", "hisi-cbc-des3", 8, 8, 24, 24, 8, ALG_3DES, MODE_CBC),
};

/*
 * The crypto core calls this when an unregistered algorithm loses its last
 * user. The algorithms live inside struct hc_dev, so each registered one holds
 * a reference on it: unbinding the device while a tfm (dm-crypt) still exists
 * is then safe, the memory stays until the last tfm is freed.
 */
static void hc_alg_put(struct hc_alg *alg)
{
	/* Whoever gets here first drops the reference, the other does nothing */
	if (atomic_xchg(&alg->ref_held, 0))
		hc_put(alg->hc);
}

static void hc_alg_destroy(struct crypto_alg *base)
{
	struct skcipher_alg *salg = container_of(base, struct skcipher_alg, base);
	struct skcipher_engine_alg *ealg = container_of(salg, struct skcipher_engine_alg, base);
	struct hc_alg *alg = container_of(ealg, struct hc_alg, alg);

	hc_alg_put(alg);
}

static int hc_register(struct hc_dev *hc)
{
	unsigned int i;
	int ret;

	for (i = 0; i < HC_NUM_ALGS; i++) {
		hc->algs[i] = hc_alg_tmpl[i];
		hc->algs[i].hc = hc;
		hc->algs[i].alg.base.base.cra_destroy = hc_alg_destroy;
		kref_get(&hc->ref);
		atomic_set(&hc->algs[i].ref_held, 1);	/* dropped by hc_alg_destroy() */
		ret = crypto_engine_register_skcipher(&hc->algs[i].alg);
		if (ret) {
			/*
			 * The core already called hc_alg_destroy() when it got
			 * as far as taking the algorithm; if it did not, the
			 * reference is still ours.
			 */
			hc_alg_put(&hc->algs[i]);
			while (i--)
				crypto_engine_unregister_skcipher(&hc->algs[i].alg);
			return ret;
		}
	}

	return 0;
}

static void hc_unregister(struct hc_dev *hc)
{
	unsigned int i;

	for (i = 0; i < HC_NUM_ALGS; i++)
		crypto_engine_unregister_skcipher(&hc->algs[i].alg);
}

static int hc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hc_dev *hc;
	struct hc_chan_mem *mem;
	dma_addr_t mem_dma;
	unsigned int i;
	int ret;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	hc = kzalloc_obj(*hc);
	if (!hc)
		return -ENOMEM;
	kref_init(&hc->ref);
	ret = devm_add_action_or_reset(dev, hc_put_action, hc);
	if (ret)
		return ret;
	hc->dev = dev;

	hc->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(hc->base))
		return PTR_ERR(hc->base);

	hc->irq = platform_get_irq(pdev, 0);
	if (hc->irq < 0)
		return hc->irq;

	hc->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(hc->clk))
		return dev_err_probe(dev, PTR_ERR(hc->clk), "cannot get the clock\n");
	ret = clk_prepare_enable(hc->clk);
	if (ret)
		return dev_err_probe(dev, ret, "cannot enable the clock\n");
	hc->clk_on = true;
	ret = devm_add_action_or_reset(dev, hc_clk_off, hc);
	if (ret)
		return ret;

	hc->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(hc->rst))
		return dev_err_probe(dev, PTR_ERR(hc->rst), "cannot get the reset\n");

	mem = dmam_alloc_coherent(dev, (HC_NUM_CHAN + 1) * sizeof(*mem),
				  &mem_dma, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	spin_lock_init(&hc->lock);
	init_waitqueue_head(&hc->wq);
	INIT_WORK(&hc->reset_work, hc_reset_work);
	for (i = 1; i <= HC_NUM_CHAN; i++) {
		struct hc_chan *ch = &hc->chan[i];

		ch->hc = hc;
		ch->id = i;
		ch->mem = mem + i;
		ch->mem_dma = mem_dma + i * sizeof(*mem);
		timer_setup(&ch->timer, hc_timeout, 0);
	}

	/* Whatever the boot loader enabled, nothing may interrupt before the reset */
	hc_wr(hc, HC_INT_EN, 0);
	ret = devm_request_threaded_irq(dev, hc->irq, hc_irq, hc_irq_thread, 0,
					dev_name(dev), hc);
	if (ret)
		return ret;

	ret = hc_hw_reset(hc);
	if (ret)
		return dev_err_probe(dev, ret, "engine does not leave reset\n");
	hc_hw_init(hc);

	platform_set_drvdata(pdev, hc);

	/* Retry mode: do_one_request() sleeps for a free channel, see its comment */
	hc->engine = crypto_engine_alloc_init_and_set(dev, true, false,
						      HC_QUEUE_LEN);
	if (!hc->engine) {
		ret = -ENOMEM;
		goto err_hw;
	}

	ret = crypto_engine_start(hc->engine);
	if (ret)
		goto err_engine;

	ret = hc_register(hc);
	if (ret) {
		dev_err_probe(dev, ret, "cannot register the algorithms\n");
		goto err_engine;
	}

	hc->dbg = debugfs_create_dir(dev_name(dev), NULL);
	debugfs_create_atomic_t("hw_requests", 0444, hc->dbg, &hc->n_hw);
	debugfs_create_atomic_t("fallback_length", 0444, hc->dbg, &hc->n_fb_len);
	debugfs_create_atomic_t("fallback_small", 0444, hc->dbg, &hc->n_fb_small);
	debugfs_create_atomic_t("fallback_dma_map", 0444, hc->dbg, &hc->n_fb_map);
	debugfs_create_atomic_t("fallback_nodes", 0444, hc->dbg, &hc->n_fb_nodes);
	debugfs_create_atomic_t("fallback_queue", 0444, hc->dbg, &hc->n_fb_queue);
	debugfs_create_atomic_t("timeouts", 0444, hc->dbg, &hc->n_timeout);

	return 0;

err_engine:
	crypto_engine_exit(hc->engine);
err_hw:
	hc_wr(hc, HC_INT_EN, 0);
	hc_wipe_all_keys(hc);

	return ret;
}

/*
 * No request may be left behind or run after this returns. The algorithms go
 * first, so no new tfm appears; requests on existing tfms use the software
 * fallback once `dead` is set. Running requests get to finish; one that is hung
 * is cut by an engine reset (software fallback, or -ESHUTDOWN when it works in
 * place); requests still in the engine queue run through the fallback while
 * the engine drains.
 */
static void hc_remove(struct platform_device *pdev)
{
	struct hc_dev *hc = platform_get_drvdata(pdev);
	unsigned long flags;
	unsigned int id, claimed;

	hc_unregister(hc);
	debugfs_remove_recursive(hc->dbg);

	/* From now on hc_crypt(), hc_do_one_request(), hc_start() and the reset work back out */
	spin_lock_irqsave(&hc->lock, flags);
	hc->dead = true;
	spin_unlock_irqrestore(&hc->lock, flags);
	wake_up(&hc->wq);

	/* A channel claimed before `dead` but not started yet: wait, it backs out or starts */
	for (;;) {
		claimed = 0;
		spin_lock_irqsave(&hc->lock, flags);
		for (id = 1; id <= HC_NUM_CHAN; id++)
			if ((hc->busy & BIT(id)) && !hc->chan[id].req)
				claimed |= BIT(id);
		spin_unlock_irqrestore(&hc->lock, flags);
		if (!claimed)
			break;
		usleep_range(100, 200);
	}

	for (id = 1; id <= HC_NUM_CHAN; id++)
		timer_delete_sync(&hc->chan[id].timer);
	cancel_work_sync(&hc->reset_work);

	/* A failed resume leaves the clock off; without it there is nothing to stop */
	if (!hc->clk_on && !clk_prepare_enable(hc->clk))
		hc->clk_on = true;
	if (hc->clk_on) {
		/* Nothing starts any more: stop the DMA before the buffers are unmapped */
		hc_wait_inflight(hc, true);
		hc_stop_engine(hc, true);
	}

	/* Drains the queue (through the fallback), then stops the thread */
	crypto_engine_exit(hc->engine);
}

/*
 * System sleep: new and queued requests go to the software fallback, running
 * ones finish, then the interrupts are masked and the clock gated. Resume
 * resets the engine (its state is not trusted after the clock was off) and
 * reloads the queue registers; keys are loaded again by the next request.
 */
static int hc_suspend(struct device *dev)
{
	struct hc_dev *hc = dev_get_drvdata(dev);
	unsigned long flags;
	unsigned int id;
	int ret;

	spin_lock_irqsave(&hc->lock, flags);
	hc->suspended = true;
	spin_unlock_irqrestore(&hc->lock, flags);
	wake_up(&hc->wq);

	ret = crypto_engine_stop(hc->engine);
	/*
	 * The reset work stays active: a hung request is cut by it, and the clock
	 * must not be gated while a reset is under way.
	 */
	if (!ret && !wait_event_timeout(hc->wq, !READ_ONCE(hc->busy) &&
					!READ_ONCE(hc->resetting), HC_TIMEOUT_MAX + HC_TIMEOUT))
		ret = -EBUSY;
	if (ret) {
		dev_warn(dev, "busy at suspend (%d), not suspending\n", ret);
		crypto_engine_start(hc->engine);
		spin_lock_irqsave(&hc->lock, flags);
		hc->suspended = false;
		spin_unlock_irqrestore(&hc->lock, flags);
		/* the timers fire once: look again for a request that ran past its deadline */
		schedule_work(&hc->reset_work);

		return ret;
	}

	hc_wr(hc, HC_INT_EN, 0);
	hc_wipe_all_keys(hc);
	spin_lock_irqsave(&hc->lock, flags);
	for (id = 1; id <= HC_NUM_CHAN; id++)
		hc->chan[id].key_ctx = NULL;
	spin_unlock_irqrestore(&hc->lock, flags);
	clk_disable_unprepare(hc->clk);
	hc->clk_on = false;

	return 0;
}

static int hc_resume(struct device *dev)
{
	struct hc_dev *hc = dev_get_drvdata(dev);
	unsigned long flags;
	int ret;

	/*
	 * If this fails the driver stays in the suspended state (software only) and
	 * clk_on stays false, so unbinding does not disable the clock a second time.
	 */
	ret = clk_prepare_enable(hc->clk);
	if (ret)
		return ret;
	hc->clk_on = true;

	ret = hc_hw_reset(hc);
	if (ret)
		dev_err(dev, "engine does not leave reset (%d), using the software fallback\n",
			ret);

	/* the reset of this resume decides: an earlier failure does not stick */
	spin_lock_irqsave(&hc->lock, flags);
	hc->failed = !!ret;
	if (!ret)
		hc_hw_init(hc);
	spin_unlock_irqrestore(&hc->lock, flags);

	crypto_engine_start(hc->engine);
	spin_lock_irqsave(&hc->lock, flags);
	hc->suspended = false;
	spin_unlock_irqrestore(&hc->lock, flags);
	wake_up(&hc->wq);

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(hc_pm_ops, hc_suspend, hc_resume);

/*
 * One generic compatible: the register map and the programming model are the
 * same on every SoC with this block. Clock gate, reset line and interrupt come
 * from the device tree.
 */
static const struct of_device_id hc_of_match[] = {
	{ .compatible = "hisilicon,hisi-cipher-mc" },
	{ }
};
MODULE_DEVICE_TABLE(of, hc_of_match);

static struct platform_driver hc_driver = {
	.probe = hc_probe,
	.remove = hc_remove,
	.driver = {
		.name = "hisi-cipher",
		.of_match_table = hc_of_match,
		.pm = pm_sleep_ptr(&hc_pm_ops),
	},
};
module_platform_driver(hc_driver);

MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_DESCRIPTION("HiSilicon multi-channel CIPHER driver");
MODULE_LICENSE("GPL");
