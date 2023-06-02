/*
 * Cryptographic API.
 *
 * Support for Novatek NA51055 Crypto Hardware acceleration.
 *
 * Copyright (c) 2019 Novatek Inc.
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

#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/hash.h>
#include <crypto/b128ops.h>
#include <crypto/scatterwalk.h>

#include <plat/efuse_protected.h>
#include "na51055_crypto.h"

#define DRV_VERSION                         "1.01.00"

//#define NA51055_CRYPTO_GCM_SOFTWARE_GHASH   1
//#define NA51055_CRYPTO_PIO_SUPPORT          1
#define NA51055_CRYPTO_DMA_TIMEOUT_DBG      1
#define NA51055_CRYPTO_DMA_DESC_CV_CHECK    1

#define ALIGN_UP(x, align_to)               (((x) + ((align_to)-1)) & ~((align_to)-1))
#define ALIGN_DN(x, align_to)               ((x) & ~((align_to)-1))

#define NA51055_CRYPTO_QUEUE_LENGTH         10
#define NA51055_CRYPTO_DEFAULT_TIMEOUT      3000        ///< 3 sec
#define NA51055_CRYPTO_ALG_PRIORITY         1000

#define NA51055_CRYPTO_DMA_ADDR_MASK        0xFFFFFFFF  ///< DMA support address bit[31..0], Max to 4GB size

#define NA51055_CRYPTO_EKEY_HDR_MAGIC       0x79656B65  ///< 'ekey'
#define NA51055_CRYPTO_EKEY_OFS_MAX         20          ///< 0 ~ 19 word offset

typedef enum {
    NA51055_CRYPTO_TBUF_ALLOC_NONE = 0,     ///< disable
    NA51055_CRYPTO_TBUF_ALLOC_BUDDY,        ///< from kernel buddy system
    NA51055_CRYPTO_TBUF_ALLOC_MAX
} NA51055_CRYPTO_TBUF_ALLOC_T;

static int tbuf_alloc = NA51055_CRYPTO_TBUF_ALLOC_NONE;
module_param(tbuf_alloc, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(tbuf_alloc, "Crypto temporarily buffer allocator => 0:None 1:Buddy");

static int tbuf_size = PAGE_SIZE;
module_param(tbuf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(tbuf_size, "Crypto temporarily buffer size => Bytes");

static int mclk = -1;
module_param(mclk, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mclk, "Crypto master clock => 0:240MHz 1:320MHz 2:Reserved 3:PLL9, -1 means from device tree");

struct na51055_ekey_hdr {
    u32     magic;              ///< must be 'ekey', to enable key load from efuse secure key section
    u8      offset;             ///< key offset from efuse secure key section, word unit
    u8      rsvd[3];            ///< must all zero
} __attribute__((packed));

struct na51055_crypto_dma_buf {
	void       *vaddr;
	dma_addr_t paddr;
	size_t     size;
};

struct na51055_crypto_dma_block {
	u32        src_addr;
	u32        dst_addr;
	u32        length;
	u32        block_cfg;
} __attribute__((packed, aligned(4)));

struct na51055_crypto_dma_desc {
	u32                              key[NA51055_CRYPTO_MAX_KEY_SIZE/sizeof(u32)];    ///< crypto input key value
	u32                              iv[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)];      ///< crypto input IV value
	u32                              counter[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)]; ///< crypto input counter value in the CTR
	u32                              header_cfg;                                      ///< DMA descriptor header configuration
	u32                              reserved[3];                                     ///< reserve bytes
	u32                              cv[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)];      ///< current IV
	u32                              s0[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)];      ///< E(K,Y0) or S0 in the GCM
	u32                              ghash[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)];   ///< GHASH output data in the GCM
	struct na51055_crypto_dma_block  block[NA51055_CRYPTO_MAX_DMA_BLOCK_NUM];         ///< DMA process blocks
} __attribute__((packed, aligned(4)));

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
struct na51055_crypto_ghash_ctx {
	unsigned int                    cryptlen;
	struct scatterlist              *src;
	void                            (*complete)(struct aead_request *req, int err);
};
#endif

struct na51055_crypto_ctx {
	struct na51055_crypto_dev        *dev;
	int                              keylen;
	u32                              key[AES_KEYSIZE_256/sizeof(u32)];
	u32                              block_size;

	union {
		struct crypto_skcipher       *skcipher;                                       ///< skcipher fallback handler
		struct crypto_aead           *aead;                                           ///< aead     fallback handler
	} fallback_u;

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	struct crypto_cipher             *cipher;                                         ///< gcm cipher handler
	struct crypto_ahash              *ghash;                                          ///< gcm ghash  handler
	struct ahash_request             *ghash_req;                                      ///< gcm ghash  request
#endif
};

struct na51055_crypto_reqctx {
	NA51055_CRYPTO_TYPE_T            type;
	NA51055_CRYPTO_MODE_T            mode;
	NA51055_CRYPTO_OPMODE_T          opmode;
	NA51055_CRYPTO_KEY_SRC_T         key_src;
	u32                              get_s0;
	NA51055_CRYPTO_CCM_TYPE_T        ccm_type;
};

struct na51055_crypto_gcm_reqctx {
	NA51055_CRYPTO_TYPE_T            type;
	NA51055_CRYPTO_MODE_T            mode;
	NA51055_CRYPTO_OPMODE_T          opmode;
	NA51055_CRYPTO_KEY_SRC_T         key_src;
	u32                              get_s0;
	NA51055_CRYPTO_CCM_TYPE_T        ccm_type;

	struct scatterlist               src[2];
	struct scatterlist               dst[2];
	struct scatterlist               *sg_src;
	struct scatterlist               *sg_dst;

	size_t                           cryptlen;

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	struct na51055_crypto_ghash_ctx  ghash_ctx;
	struct scatterlist               ghash_src;
#endif
	u32                              auth_tag[AES_BLOCK_SIZE/sizeof(u32)];
	u32                              iauth_tag[AES_BLOCK_SIZE/sizeof(u32)];
};

struct na51055_crypto_ccm_reqctx {
	NA51055_CRYPTO_TYPE_T            type;
	NA51055_CRYPTO_MODE_T            mode;
	NA51055_CRYPTO_OPMODE_T          opmode;
	NA51055_CRYPTO_KEY_SRC_T         key_src;
	u32                              get_s0;
	NA51055_CRYPTO_CCM_TYPE_T        ccm_type;

	struct scatterlist               src[2];
	struct scatterlist               dst[2];
	struct scatterlist               *sg_src;
	struct scatterlist               *sg_dst;

	size_t                           cryptlen;

	struct scatterlist               *sg_crypt;
	struct scatterlist               sg_payload;
	size_t                           payload_size;
	u32                              auth_tag[AES_BLOCK_SIZE/sizeof(u32)];
	u32                              iauth_tag[AES_BLOCK_SIZE/sizeof(u32)];
};

struct na51055_crypto_dma_ch {
	int                              idx;                                           ///< DMA channel index
	NA51055_CRYPTO_STATE_T           state;                                         ///< DMA channel state

	dma_addr_t                       desc_paddr;                                    ///< DMA channel descriptor physical address
	struct na51055_crypto_dma_desc   *desc;                                         ///< DMA channel descriptor

    void                             *tbuf_src;                                     ///< DMA source      temporarily buffer
    void                             *tbuf_dst;                                     ///< DMA destination temporarily buffer
    void                             *tbuf_ass;                                     ///< DMA associated  temporarily buffer

	struct timer_list                timer;                                         ///< DMA channel timeout timer

	u32                              iv[NA51055_CRYPTO_MAX_IV_SIZE/sizeof(u32)];    ///< for store current request IV

	struct crypto_async_request      *req;                                          ///< asynchronous request
	struct na51055_crypto_ctx        *ctx;
	struct scatterlist               *sg_ass;                                       ///< associated  data scatter list
	struct scatterlist               *sg_src;                                       ///< source      data scatter list
	struct scatterlist               *sg_dst;                                       ///< destination data scatter list

	struct scatterlist               *sg_ass_work;
	struct scatterlist               *sg_src_work;
	struct scatterlist               *sg_dst_work;

	int                              sg_ass_nents;
	int                              sg_src_nents;
	int                              sg_dst_nents;
	int                              sg_same;                                       ///< source and destination use same buffer

	int                              ass_copied;
	int                              src_copied;
	int                              dst_copied;
	struct scatterlist               sg_ass_cpy;
	struct scatterlist               sg_src_cpy;
	struct scatterlist               sg_dst_cpy;

	size_t                           sg_src_len;
	size_t                           sg_src_ofs;
	size_t                           sg_dst_len;
	size_t                           sg_dst_ofs;

	size_t                           ass_total;
	size_t                           src_total;
	size_t                           dst_total;

	size_t                           ass_nbytes;
	size_t                           req_nbytes;
};

struct na51055_crypto_dev {
	struct device                    *dev;
	struct clk                       *clk;
	void __iomem                     *iobase;
	int                              irq;
	spinlock_t                       lock;
	spinlock_t                       pio_lock;
	spinlock_t                       queue_lock;

	struct na51055_crypto_dma_buf    dma_buf;
	struct na51055_crypto_dma_ch     dma_ch[NA51055_CRYPTO_DMA_CH_MAX];

	struct crypto_queue              queue;
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	struct crypto_queue              hash_queue;
#endif
	struct crypto_queue              payload_queue;

	struct tasklet_struct            queue_tasklet;
	struct tasklet_struct            done_tasklet;
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	struct tasklet_struct            hash_tasklet;
#endif
	struct tasklet_struct            payload_tasklet;
};

static struct na51055_crypto_dev *na51055_cdev = NULL;

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
static const u8 na51055_crypto_aes_zeroes[AES_BLOCK_SIZE] __attribute__ ((aligned(4))) = {[0 ... (AES_BLOCK_SIZE-1)] = 0};
#endif

static inline u32 na51055_crypto_read(struct na51055_crypto_dev *dd, u32 offset)
{
	return readl(dd->iobase + offset);
}

static inline void na51055_crypto_write(struct na51055_crypto_dev *dd, u32 offset, u32 value)
{
	writel(value, dd->iobase + offset);
}

static void na51055_crypto_reset(struct na51055_crypto_dev *dd)
{
	u32 value = 0;
	u32 cnt   = 0;

	/* disable crypto */
	na51055_crypto_write(dd, NA51055_CRYPTO_CFG_REG, 0);

	/* set reset, hardware will auto clear */
	na51055_crypto_write(dd, NA51055_CRYPTO_CFG_REG, 0x01);

	/* check reset done */
	while ((value = na51055_crypto_read(dd, NA51055_CRYPTO_CFG_REG)) & 0x1) {
		if(cnt++ >= 100)
			break;
		udelay(1);
	}

	/* clear all status */
	na51055_crypto_write(dd, NA51055_CRYPTO_INT_STS_REG, 0xFF1);

	if (value & 0x1)
		dev_err(dd->dev, "crypto hardware reset failed!!\n");
}

static int na51055_crypto_handle_req(struct na51055_crypto_dev *dev, struct crypto_async_request *req, int q_id)
{
	unsigned long flags;
	int err;

	spin_lock_irqsave(&dev->queue_lock, flags);

	switch (q_id) {
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	case 1:		/* put to hash queue for gcm ghash*/
		err = crypto_enqueue_request(&dev->hash_queue, req);
		tasklet_schedule(&dev->hash_tasklet);
		break;
#endif
	case 2:		/* put to payload queue for ccm payload */
		err = crypto_enqueue_request(&dev->payload_queue, req);
		tasklet_schedule(&dev->payload_tasklet);
		break;
	default:	/* put to normal queue for general hardware operation */
		err = crypto_enqueue_request(&dev->queue, req);
		tasklet_schedule(&dev->queue_tasklet);
		break;
	}

	spin_unlock_irqrestore(&dev->queue_lock, flags);

	return err;
}

static void na51055_crypto_complete(struct na51055_crypto_dev *dev, NA51055_CRYPTO_DMA_CH_T ch, int err)
{
	if (ch >= NA51055_CRYPTO_DMA_CH_MAX)
		return;

	(dev->dma_ch[ch].req)->complete(dev->dma_ch[ch].req, err);
	dev->dma_ch[ch].req   = NULL;
	dev->dma_ch[ch].state = NA51055_CRYPTO_STATE_IDLE;
}

static bool na51055_crypto_is_sg_aligned(struct scatterlist *sg, int align_size, int align_last, int chk_size)
{
	int count = 0;

	while (sg && (count < chk_size)) {
		if (((sg_phys(sg) + sg->length) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
			return false;
		}
		else if ((count + sg->length) < chk_size) {
			if (!IS_ALIGNED(sg->length, align_size) || !IS_ALIGNED(sg_phys(sg), 4))	///< DMA address must word alignment
				return false;
		}
		else {
			if (align_last == 0) {
				if(!IS_ALIGNED(sg_phys(sg), 4))	///< last sg length not require to alignment for hardware
					return false;
			}
			else {
				if (!IS_ALIGNED((chk_size-count), align_size) || !IS_ALIGNED(sg_phys(sg), 4))	///< DMA address must word alignment
					return false;
			}
		}
		count += sg->length;

		sg = sg_next(sg);
	}

	return true;
}

static void na51055_crypto_timeout_handler(struct timer_list *t)
{
    struct na51055_crypto_dma_ch *pdma = from_timer(pdma, t, timer);
	struct na51055_crypto_dev    *dev  = container_of(pdma, struct na51055_crypto_dev, dma_ch[pdma->idx]);
	unsigned long flags;
	int i;
#ifdef NA51055_CRYPTO_DMA_TIMEOUT_DBG
	int j;
	volatile u32 *p_desc;
#endif

	spin_lock_irqsave(&dev->lock, flags);

	/* delete timeout timer */
	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++)
		del_timer(&dev->dma_ch[i].timer);

#ifdef NA51055_CRYPTO_DMA_TIMEOUT_DBG
	dev_err(dev->dev, "crypto timeout!\n");

	/* dump device register */
	for (i=0; i<=NA51055_CRYPTO_KEY_READ_REG; i+=16) {
	    dev_err(dev->dev, "%04x | %08x %08x %08x %08x\n",
	            i,
	            na51055_crypto_read(dev, i),
	            na51055_crypto_read(dev, i+4),
	            na51055_crypto_read(dev, i+8),
	            na51055_crypto_read(dev, i+12));
	}

	/* dump DMA descriptor */
	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		if (!dev->dma_ch[i].req)
			continue;
	    p_desc = (volatile u32 *)dev->dma_ch[i].desc;
	    dev_err(dev->dev, "[DMA#%d] paddr:0x%08x vaddr:0x%08x\n", i, (u32)dev->dma_ch[i].desc_paddr, (u32)dev->dma_ch[i].desc);
	    for (j=0; j<(sizeof(struct na51055_crypto_dma_desc)/4); j+=4) {
	        dev_err(dev->dev, "%04x | %08x %08x %08x %08x\n", (j*4), p_desc[j], p_desc[j+1], p_desc[j+2], p_desc[j+3]);
	    }
	}
#endif

	/* disable and reset device */
	na51055_crypto_reset(dev);

	/* complete pending request */
	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		if (!dev->dma_ch[i].req)
			continue;

		if (i == pdma->idx)
			dev_err(dev->dev, "crypto dma#%d timeout!\n", i);

		if (dev->dma_ch[i].sg_src && dev->dma_ch[i].sg_src_nents) {
			dma_unmap_sg(dev->dev, dev->dma_ch[i].sg_src, dev->dma_ch[i].sg_src_nents, DMA_TO_DEVICE);   ///< cache nothing to do
			dev->dma_ch[i].sg_src       = NULL;
			dev->dma_ch[i].sg_src_nents = 0;
		}
		if (dev->dma_ch[i].sg_dst && dev->dma_ch[i].sg_dst_nents) {
			dma_unmap_sg(dev->dev, dev->dma_ch[i].sg_dst, dev->dma_ch[i].sg_dst_nents, DMA_FROM_DEVICE); ///< cache invalidate
			dev->dma_ch[i].sg_dst       = NULL;
			dev->dma_ch[i].sg_dst_nents = 0;
		}
		if (dev->dma_ch[i].sg_ass && dev->dma_ch[i].sg_ass_nents) {
			dma_unmap_sg(dev->dev, dev->dma_ch[i].sg_ass, dev->dma_ch[i].sg_ass_nents, DMA_TO_DEVICE);   ///< cache nothing to do
			dev->dma_ch[i].sg_ass       = NULL;
			dev->dma_ch[i].sg_ass_nents = 0;
		}
		if (dev->dma_ch[i].dst_copied == 1) {
			free_pages((unsigned long)sg_virt(&dev->dma_ch[i].sg_dst_cpy), get_order(dev->dma_ch[i].sg_dst_cpy.length));
			dev->dma_ch[i].dst_copied = 0;
			if (dev->dma_ch[i].sg_same && dev->dma_ch[i].src_copied)
				dev->dma_ch[i].src_copied = 0;
		}
		else {
			dev->dma_ch[i].dst_copied = 0;
		}
		if (dev->dma_ch[i].src_copied == 1) {
			free_pages((unsigned long)sg_virt(&dev->dma_ch[i].sg_src_cpy), get_order(dev->dma_ch[i].sg_src_cpy.length));
			dev->dma_ch[i].src_copied = 0;
		}
		else {
			dev->dma_ch[i].src_copied = 0;
		}
		if (dev->dma_ch[i].ass_copied == 1) {
			free_pages((unsigned long)sg_virt(&dev->dma_ch[i].sg_ass_cpy), get_order(dev->dma_ch[i].sg_ass_cpy.length));
			dev->dma_ch[i].ass_copied = 0;
		}
		else {
			dev->dma_ch[i].ass_copied = 0;
		}
		if (crypto_tfm_alg_type((dev->dma_ch[i].req)->tfm) == CRYPTO_ALG_TYPE_AEAD) {
			struct na51055_crypto_ccm_reqctx *ccm_reqctx = aead_request_ctx(aead_request_cast(dev->dma_ch[i].req));
			if (ccm_reqctx->ccm_type && ccm_reqctx->payload_size) {
				free_pages((unsigned long)sg_virt(&ccm_reqctx->sg_payload), get_order(ccm_reqctx->payload_size));
				ccm_reqctx->payload_size = 0;
			}
		}
		na51055_crypto_complete(dev, i, -EINVAL);
	}

	/* trigger to do next crypto request in queue */
	tasklet_schedule(&dev->queue_tasklet);

	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

static int na51055_crypto_trigger_efuse_key(u8 key_ofs, u8 key_cnt)
{
	return trigger_efuse_key(EFUSE_KEY_MANAGER_CRYPTO, key_ofs, key_cnt);
}

static void na51055_crypto_start(struct na51055_crypto_dev *dev)
{
	struct na51055_ekey_hdr          *ekey_hdr;
	struct ablkcipher_request        *ablk_req;
	struct aead_request              *aead_req;
	struct na51055_crypto_ctx        *ctx;
	struct na51055_crypto_reqctx     *reqctx;
	struct na51055_crypto_ccm_reqctx *ccm_reqctx;
	struct na51055_crypto_gcm_reqctx *gcm_reqctx;
	volatile struct na51055_crypto_dma_desc *desc;
	struct na51055_crypto_dma_ch     *dma_ch;
	u8                               *iv;
	struct scatterlist               *ass_sg;
	struct scatterlist               *src_sg;
	struct scatterlist               *dst_sg;
	void                             *ass_pages;
	void                             *src_pages;
	void                             *dst_pages;
	int                              ass_dma_map;
	int                              src_dma_map;
	int                              dst_dma_map;
	u32                              ivsize;
	u32                              reg_value;
	u32                              dma_len;
	u32                              auth_size;
	int                              block_num;
	int                              align_last;
	int                              err, i, j;
	unsigned long                    flags;
	u8                               key_cnt;
	u32                              dma_enb = 0;

	spin_lock_irqsave(&dev->lock, flags);

	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		dma_ch = &dev->dma_ch[i];

		/* check dma channel have assign request to transfer */
		if (dma_ch->state != NA51055_CRYPTO_STATE_START)
			continue;

		/* check request ready */
		if (!dma_ch->req) {
			dev_err(dev->dev, "crypto dma#%d not assign any request!!\n", i);
			dma_ch->state = NA51055_CRYPTO_STATE_IDLE;
			continue;
		}

		/* check request algorithm type */
		ccm_reqctx = NULL;
		if (crypto_tfm_alg_type(dma_ch->req->tfm) == CRYPTO_ALG_TYPE_AEAD) {
			aead_req   = aead_request_cast(dma_ch->req);
			reqctx     = aead_request_ctx(aead_req);
			ctx        = dma_ch->ctx;
			iv         = aead_req->iv;
			ivsize     = crypto_aead_ivsize(crypto_aead_reqtfm(aead_req));
			if (reqctx->ccm_type) {
				ccm_reqctx = (struct na51055_crypto_ccm_reqctx *)reqctx;
				if (ccm_reqctx->opmode == NA51055_CRYPTO_OPMODE_CBC) {
					ivsize     = 0;						///< use zero IV
					ass_sg     = NULL;
					src_sg     = &ccm_reqctx->sg_payload;
					dst_sg     = NULL;
					auth_size  = crypto_aead_authsize(crypto_aead_reqtfm(aead_req));

					dma_ch->ass_nbytes = 0;
					dma_ch->req_nbytes = ccm_reqctx->payload_size;
				}
				else {
					ass_sg     = NULL;
					src_sg     = ccm_reqctx->sg_src;
					dst_sg     = ccm_reqctx->sg_dst;
					auth_size  = crypto_aead_authsize(crypto_aead_reqtfm(aead_req));

					dma_ch->ass_nbytes = 0;
					dma_ch->req_nbytes = ccm_reqctx->cryptlen;
				}
			}
			else {
				gcm_reqctx = (struct na51055_crypto_gcm_reqctx *)reqctx;
				ass_sg     = aead_req->src;
				src_sg     = gcm_reqctx->sg_src;
				dst_sg     = gcm_reqctx->sg_dst;
				auth_size  = crypto_aead_authsize(crypto_aead_reqtfm(aead_req));

				dma_ch->ass_nbytes = aead_req->assoclen;
				dma_ch->req_nbytes = gcm_reqctx->cryptlen;
			}
		}
		else {
			ablk_req   = ablkcipher_request_cast(dma_ch->req);
			reqctx     = ablkcipher_request_ctx(ablk_req);
			ctx        = dma_ch->ctx;
			iv         = ablk_req->info;
			ivsize     = crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(ablk_req));
			ass_sg     = NULL;
			src_sg     = ablk_req->src;
			dst_sg     = ablk_req->dst;
			auth_size  = 0;

			dma_ch->ass_nbytes = 0;
			dma_ch->req_nbytes = ablk_req->nbytes;
		}

		desc        = dma_ch->desc;
		ass_pages   = NULL;
		src_pages   = NULL;
		dst_pages   = NULL;
		ass_dma_map = 0;
		src_dma_map = 0;
		dst_dma_map = 0;
		err         = 0;
		align_last  = (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM || reqctx->opmode == NA51055_CRYPTO_OPMODE_CTR || reqctx->opmode == NA51055_CRYPTO_OPMODE_CFB) ? 0 : 1;

        dev_dbg(dev->dev, "DMA#%d Cryp => mode:%d opmode:%d type:%d block_size:%d keylen:%d ivsize:%d\n", i, reqctx->mode, reqctx->opmode, reqctx->type, ctx->block_size, ctx->keylen, ivsize);
		dev_dbg(dev->dev, "DMA#%d Sg   => orignal src_nents :%d dst_nents:%d req_nbytes:%d auth_size:%d\n", i, (src_sg ? sg_nents(src_sg) : 0), (dst_sg ? sg_nents(dst_sg) : 0), dma_ch->req_nbytes, auth_size);
		dev_dbg(dev->dev, "DMA#%d Src  => orignal pa:0x%08x va:0x%08x size:%d\n", i, (src_sg ? (u32)sg_phys(src_sg) : 0), (src_sg ? (u32)sg_virt(src_sg) : 0), (src_sg ? src_sg->length : 0));
		dev_dbg(dev->dev, "DMA#%d Dst  => orignal pa:0x%08x va:0x%08x size:%d\n", i, (dst_sg ? (u32)sg_phys(dst_sg) : 0), (dst_sg ? (u32)sg_virt(dst_sg) : 0), (dst_sg ? dst_sg->length : 0));

		/* source length alignment check */
		if (src_sg && dma_ch->req_nbytes) {
			if (sg_nents_for_len(src_sg, dma_ch->req_nbytes) <= 0) {
				dev_err(dev->dev, "crypto dma#%d source buffer size is small than request size\n", i);
				err = -EINVAL;
				goto error;
			}
			if (!na51055_crypto_is_sg_aligned(src_sg, ctx->block_size, align_last, dma_ch->req_nbytes)) {
				if (dma_ch->tbuf_src && tbuf_size) {
					if (dma_ch->req_nbytes > tbuf_size) {
						if (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM) {
							dev_err(dev->dev, "crypto dma#%d source temp buffer size is small than request size\n", i);
							err = -EINVAL;
							goto error;
						}
						dev_dbg(dev->dev, "crypto dma#%d copy %d bytes from original offset 0 to temp buffer\n", i, tbuf_size);
						scatterwalk_map_and_copy(dma_ch->tbuf_src, src_sg, 0, tbuf_size, 0);
						sg_init_one(&dma_ch->sg_src_cpy, dma_ch->tbuf_src, tbuf_size);
					}
					else {
					    dev_dbg(dev->dev, "crypto dma#%d copy %d bytes from original offset 0 to temp buffer\n", i, dma_ch->req_nbytes);
						scatterwalk_map_and_copy(dma_ch->tbuf_src, src_sg, 0, dma_ch->req_nbytes, 0);
						sg_init_one(&dma_ch->sg_src_cpy, dma_ch->tbuf_src, dma_ch->req_nbytes);
					}
					dma_ch->src_copied = 2;
					dma_ch->sg_src     = &dma_ch->sg_src_cpy;
				}
				else {
					src_pages = (void *)__get_free_pages(GFP_ATOMIC, get_order(dma_ch->req_nbytes));
					if (!src_pages) {
						dev_err(dev->dev, "crypto dma#%d no free memory to allocte source buffer\n", i);
						err = -ENOMEM;
						goto error;
					}
					if ((((u32)page_to_phys(virt_to_page(src_pages)) + dma_ch->req_nbytes) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
						dev_err(dev->dev, "crypto dma#%d allocated source paddr:0x%08x not support\n", i, page_to_phys(virt_to_page(src_pages)));
						err = -ENOMEM;
						goto error;
					}
					scatterwalk_map_and_copy(src_pages, src_sg, 0, dma_ch->req_nbytes, 0);
					sg_init_one(&dma_ch->sg_src_cpy, src_pages, dma_ch->req_nbytes);

					dma_ch->src_copied = 1;
					dma_ch->sg_src     = &dma_ch->sg_src_cpy;
				}
			}
			else {
				dma_ch->src_copied = 0;
				dma_ch->sg_src     = src_sg;
			}
		}
		else {
			dma_ch->src_copied = 0;
			dma_ch->sg_src     = src_sg;
		}

		/* destination length alignment check */
		if (dst_sg && dma_ch->req_nbytes) {
			if (sg_nents_for_len(dst_sg, dma_ch->req_nbytes) <= 0) {
				dev_err(dev->dev, "crypto dma#%d destination buffer size is small than request size\n", i);
				err = -EINVAL;
				goto error;
			}
			if (!na51055_crypto_is_sg_aligned(dst_sg, ctx->block_size, align_last, dma_ch->req_nbytes) || !IS_ALIGNED(dma_ch->req_nbytes, 4)) {
				if (dma_ch->tbuf_dst && tbuf_size) {
					if (dma_ch->req_nbytes > tbuf_size) {
						if (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM) {
						    dev_err(dev->dev, "crypto dma#%d destination temp buffer size is small than request size\n", i);
						    err = -EINVAL;
						    goto error;
						}
						sg_init_one(&dma_ch->sg_dst_cpy, dma_ch->tbuf_dst, tbuf_size);
					}
					else {
						sg_init_one(&dma_ch->sg_dst_cpy, dma_ch->tbuf_dst, dma_ch->req_nbytes);
					}
					dma_ch->dst_copied = 2;
					dma_ch->sg_dst     = &dma_ch->sg_dst_cpy;
				}
				else {
					if ((dma_ch->src_copied == 1) && IS_ALIGNED(dma_ch->req_nbytes, 4)) {
						sg_init_one(&dma_ch->sg_dst_cpy, src_pages, dma_ch->req_nbytes);  ///< source and destination use the same buffer to reduce memory usage
					}
					else {
						dst_pages = (void *)__get_free_pages(GFP_ATOMIC, get_order(ALIGN_UP(dma_ch->req_nbytes, 4)));
						if (!dst_pages) {
							dev_err(dev->dev, "crypto dma#%d no free memory to allocte destination buffer\n", i);
							err = -ENOMEM;
							goto error;
						}
						if ((((u32)page_to_phys(virt_to_page(dst_pages)) + ALIGN_UP(dma_ch->req_nbytes, 4)) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
							dev_err(dev->dev, "crypto dma#%d allocated destination paddr:0x%08x not support\n", i, page_to_phys(virt_to_page(dst_pages)));
							err = -ENOMEM;
							goto error;
						}
						sg_init_one(&dma_ch->sg_dst_cpy, dst_pages, ALIGN_UP(dma_ch->req_nbytes, 4));
					}
					dma_ch->dst_copied = 1;
					dma_ch->sg_dst     = &dma_ch->sg_dst_cpy;
				}
			}
			else {
				dma_ch->dst_copied = 0;
				dma_ch->sg_dst     = dst_sg;
			}
		}
		else {
			dma_ch->dst_copied = 0;
			dma_ch->sg_dst     = dst_sg;
		}

		/* associated data length alignment check */
		if (ass_sg && dma_ch->ass_nbytes) {
			if(sg_nents_for_len(ass_sg, dma_ch->ass_nbytes) <= 0) {
				dev_err(dev->dev, "crypto dma#%d associated buffer size is small than request size\n", i);
				err = -EINVAL;
				goto error;
			}
			if (!na51055_crypto_is_sg_aligned(ass_sg, ctx->block_size, align_last, dma_ch->ass_nbytes)) {
				if (dma_ch->tbuf_ass && tbuf_size) {
					if (dma_ch->ass_nbytes > tbuf_size) {
						dev_err(dev->dev, "crypto dma#%d associated temp buffer size is small than request size\n", i);
						err = -ENOMEM;
						goto error;
					}
					scatterwalk_map_and_copy(dma_ch->tbuf_ass, ass_sg, 0, dma_ch->ass_nbytes, 0);
					sg_init_one(&dma_ch->sg_ass_cpy, dma_ch->tbuf_ass, dma_ch->ass_nbytes);

					dma_ch->ass_copied = 2;
					dma_ch->sg_ass     = &dma_ch->sg_ass_cpy;
				}
				else {
					ass_pages = (void *)__get_free_pages(GFP_ATOMIC, get_order(dma_ch->ass_nbytes));
					if (!ass_pages) {
						dev_err(dev->dev, "crypto dma#%d no free memory to allocte associated data buffer\n", i);
						err = -ENOMEM;
						goto error;
					}
					if ((((u32)page_to_phys(virt_to_page(ass_pages)) + dma_ch->ass_nbytes) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
						dev_err(dev->dev, "crypto dma#%d allocated associated paddr:0x%08x not support\n", i, page_to_phys(virt_to_page(ass_pages)));
						err = -ENOMEM;
						goto error;
					}
					scatterwalk_map_and_copy(ass_pages, ass_sg, 0, dma_ch->ass_nbytes, 0);
					sg_init_one(&dma_ch->sg_ass_cpy, ass_pages, dma_ch->ass_nbytes);

					dma_ch->ass_copied = 1;
					dma_ch->sg_ass     = &dma_ch->sg_ass_cpy;
				}
			}
			else {
				dma_ch->ass_copied = 0;
				dma_ch->sg_ass     = ass_sg;
			}
		}
		else {
			dma_ch->ass_copied = 0;
			dma_ch->sg_ass     = ass_sg;
		}

		/* source dma mapping and cache clean */
		if (dma_ch->sg_src && dma_ch->req_nbytes) {
			dma_ch->sg_src_nents = dma_map_sg(dev->dev, dma_ch->sg_src, sg_nents(dma_ch->sg_src), DMA_TO_DEVICE);	  ///< direction => memory to device, cache clean, DMA input
			if (!dma_ch->sg_src_nents) {
				dev_err(dev->dev, "crypto dma#%d source scatterlist dma map error\n", i);
				err = -ENOMEM;
				goto error;
			}
			src_dma_map = 1;
		}
		else {
			dma_ch->sg_src_nents = 0;
		}

		/* destination dma mapping and cache invalidate */
		if (dma_ch->sg_dst && dma_ch->req_nbytes) {
			dma_ch->sg_dst_nents = dma_map_sg(dev->dev, dma_ch->sg_dst, sg_nents(dma_ch->sg_dst), DMA_FROM_DEVICE);    ///< direction => memory from device, cache invalidate, DMA output
			if (!dma_ch->sg_dst_nents) {
				dev_err(dev->dev, "crypto dma#%d destination scatterlist dma map error\n", i);
				err = -ENOMEM;
				goto error;
			}
			dst_dma_map = 1;
		}
		else {
			dma_ch->sg_dst_nents = 0;
		}

		/* associated dma mapping and cache clean */
		if (dma_ch->sg_ass && dma_ch->ass_nbytes) {
			dma_ch->sg_ass_nents = dma_map_sg(dev->dev, dma_ch->sg_ass, sg_nents_for_len(dma_ch->sg_ass, dma_ch->ass_nbytes), DMA_TO_DEVICE);	  ///< direction => memory to device, cache clean, DMA input
			if (!dma_ch->sg_ass_nents) {
				dev_err(dev->dev, "crypto dma#%d associated scatterlist dma map error\n", i);
				err = -ENOMEM;
				goto error;
			}
			ass_dma_map = 1;
		}
		else {
			dma_ch->sg_ass_nents = 0;
		}

		/* source and destination use same buffer */
		if (src_dma_map && dst_dma_map)
			dma_ch->sg_same = (sg_phys(dma_ch->sg_src) == sg_phys(dma_ch->sg_dst)) ? 1 : 0;
		else
			dma_ch->sg_same = 0;

		dev_dbg(dev->dev, "DMA#%d Sg   => src_nents :%d dst_nents:%d req_nbytes:%d auth_size:%d\n", i, (dma_ch->sg_src ? sg_nents(dma_ch->sg_src) : 0), (dma_ch->sg_dst ? sg_nents(dma_ch->sg_dst) : 0), dma_ch->req_nbytes, auth_size);
		dev_dbg(dev->dev, "DMA#%d Sg   => src_copied:%d dst_copied:%d same:%d\n", i, dma_ch->src_copied, dma_ch->dst_copied, dma_ch->sg_same);
		dev_dbg(dev->dev, "DMA#%d Src  => pa:0x%08x va:0x%08x size:%d\n", i, (dma_ch->sg_src ? (u32)sg_dma_address(dma_ch->sg_src) : 0), (dma_ch->sg_src ? (u32)sg_virt(dma_ch->sg_src) : 0), (dma_ch->sg_src ? dma_ch->sg_src->length : 0));
		dev_dbg(dev->dev, "DMA#%d Dst  => pa:0x%08x va:0x%08x size:%d\n", i, (dma_ch->sg_dst ? (u32)sg_dma_address(dma_ch->sg_dst) : 0), (dma_ch->sg_dst ? (u32)sg_virt(dma_ch->sg_dst) : 0), (dma_ch->sg_dst ? dma_ch->sg_dst->length : 0));
		dev_dbg(dev->dev, "DMA#%d Ass  => ass_nents:%d ass_nbytes:%d ass_copied:%d\n", i, (dma_ch->sg_ass ? sg_nents_for_len(dma_ch->sg_ass, dma_ch->ass_nbytes) : 0), dma_ch->ass_nbytes, dma_ch->ass_copied);
		dev_dbg(dev->dev, "DMA#%d Ass  => pa:0x%08x va:0x%08x size:%d\n", i, (dma_ch->sg_ass ? (u32)sg_dma_address(dma_ch->sg_ass) : 0), (dma_ch->sg_ass ? (u32)sg_virt(dma_ch->sg_ass) : 0), (dma_ch->sg_ass ? dma_ch->sg_ass->length : 0));
		dev_dbg(dev->dev, "DMA#%d Desc => pa:0x%08x va:0x%08x\n", i, (u32)dma_ch->desc_paddr, (u32)dma_ch->desc);

		/* clear old IV */
		memset(dma_ch->iv, 0,  sizeof(dma_ch->iv));

		/* store input IV */
		if (iv && ivsize)
			memcpy(dma_ch->iv, iv, ivsize);

		/* clear descriptor IV and Key and Counter */
		memset((void *)desc->iv,      0, sizeof(desc->iv));
		memset((void *)desc->key,     0, sizeof(desc->key));
		memset((void *)desc->counter, 0, sizeof(desc->counter));

		/* key header */
		ekey_hdr = (struct na51055_ekey_hdr *)ctx->key;

		if ((ekey_hdr->magic == NA51055_CRYPTO_EKEY_HDR_MAGIC) &&
			(ekey_hdr->offset < NA51055_CRYPTO_EKEY_OFS_MAX)   &&
			(ekey_hdr->rsvd[0] == 0)                           &&
			(ekey_hdr->rsvd[1] == 0)                           &&
			(ekey_hdr->rsvd[2] == 0)) {
			switch(reqctx->mode) {
			case NA51055_CRYPTO_MODE_DES:
				key_cnt = 2;
				break;
			case NA51055_CRYPTO_MODE_3DES:
				key_cnt = 6;
				break;
			case NA51055_CRYPTO_MODE_AES_128:
				key_cnt = 4;
				break;
			case NA51055_CRYPTO_MODE_AES_256:
				key_cnt = 8;
				break;
			default:
				dev_err(dev->dev, "DMA#%d unknown crypto mode(%d)!\n", i, reqctx->mode);
				err = -EINVAL;
				goto error;
			}

			if (na51055_crypto_trigger_efuse_key(ekey_hdr->offset, key_cnt)) {
				dev_err(dev->dev, "DMA#%d Key  => key invalid(offset:%d)!\n", i, ekey_hdr->offset);
				err = -EINVAL;
				goto error;
			}
			else {
				reqctx->key_src = NA51055_CRYPTO_KEY_SRC_MANAGAMENT;
				dev_dbg(dev->dev, "DMA#%d Key  => from efuse secure section offset:%d\n", i, ekey_hdr->offset);
			}
		}

		/* setup DMA descriptor */
		switch (reqctx->mode) {
		case NA51055_CRYPTO_MODE_DES:		/* key => 64 bits, IV => 64 bits, data => 64 bits */
			/* set IV */
			if (iv && ivsize) {
				desc->iv[0] = dma_ch->iv[0];
				desc->iv[1] = dma_ch->iv[1];
				dev_dbg(dev->dev, "DMA#%d IV   => %08x %08x\n", i, dma_ch->iv[0], dma_ch->iv[1]);
			}

			/* set key */
			if (reqctx->key_src == NA51055_CRYPTO_KEY_SRC_DESC0) {
				desc->key[0] = ctx->key[0];
				desc->key[1] = ctx->key[1];
				dev_dbg(dev->dev, "DMA#%d Key  => %08x %08x\n", i, ctx->key[0], ctx->key[1]);
			}
			break;
		case NA51055_CRYPTO_MODE_3DES:		/* key => 192 bit, IV => 64 bits, data => 64 bits, DES-EDE3 */
			/* set IV */
			if (iv && ivsize) {
				desc->iv[0] = dma_ch->iv[0];
				desc->iv[1] = dma_ch->iv[1];
				dev_dbg(dev->dev, "DMA#%d IV   => %08x %08x\n", i, dma_ch->iv[0], dma_ch->iv[1]);
			}

			/* set key */
			if (reqctx->key_src == NA51055_CRYPTO_KEY_SRC_DESC0) {
				desc->key[0] = ctx->key[0];
				desc->key[1] = ctx->key[1];
				desc->key[2] = ctx->key[2];
				desc->key[3] = ctx->key[3];
				desc->key[4] = ctx->key[4];
				desc->key[5] = ctx->key[5];
				dev_dbg(dev->dev, "DMA#%d Key  => %08x %08x %08x %08x %08x %08x\n", i, ctx->key[0], ctx->key[1], ctx->key[2], ctx->key[3], ctx->key[4], ctx->key[5]);
			}
			break;
		case NA51055_CRYPTO_MODE_AES_128:	/* key => 128 bits, IV => 128 bits, data => 128 bits*/
			/* set IV */
			if (iv && ivsize) {
				desc->iv[0] = dma_ch->iv[0];
				desc->iv[1] = dma_ch->iv[1];
				desc->iv[2] = dma_ch->iv[2];
				desc->iv[3] = dma_ch->iv[3];
				dev_dbg(dev->dev, "DMA#%d IV   => %08x %08x %08x %08x\n", i, dma_ch->iv[0], dma_ch->iv[1], dma_ch->iv[2], dma_ch->iv[3]);
			}

			/* set key */
			if (reqctx->key_src == NA51055_CRYPTO_KEY_SRC_DESC0) {
				desc->key[0] = ctx->key[0];
				desc->key[1] = ctx->key[1];
				desc->key[2] = ctx->key[2];
				desc->key[3] = ctx->key[3];
				dev_dbg(dev->dev, "DMA#%d Key  => %08x %08x %08x %08x\n", i, ctx->key[0], ctx->key[1], ctx->key[2], ctx->key[3]);
			}
			break;
		case NA51055_CRYPTO_MODE_AES_256:	/* key => 256 bits, IV => 128 bits, data => 128 bits*/
			/* set IV */
			if (iv && ivsize) {
				desc->iv[0] = dma_ch->iv[0];
				desc->iv[1] = dma_ch->iv[1];
				desc->iv[2] = dma_ch->iv[2];
				desc->iv[3] = dma_ch->iv[3];
				dev_dbg(dev->dev, "DMA#%d IV   => %08x %08x %08x %08x\n", i, dma_ch->iv[0], dma_ch->iv[1], dma_ch->iv[2], dma_ch->iv[3]);
			}

			/* set key */
			if (reqctx->key_src == NA51055_CRYPTO_KEY_SRC_DESC0) {
				desc->key[0] = ctx->key[0];
				desc->key[1] = ctx->key[1];
				desc->key[2] = ctx->key[2];
				desc->key[3] = ctx->key[3];
				desc->key[4] = ctx->key[4];
				desc->key[5] = ctx->key[5];
				desc->key[6] = ctx->key[6];
				desc->key[7] = ctx->key[7];
				dev_dbg(dev->dev, "DMA#%d Key  => %08x %08x %08x %08x %08x %08x %08x %08x\n", i, ctx->key[0], ctx->key[1], ctx->key[2], ctx->key[3], ctx->key[4], ctx->key[5], ctx->key[6], ctx->key[7]);
			}
			break;
		default:
			dev_err(dev->dev, "DMA#%d unknown crypto mode(%d)!\n", i, reqctx->mode);
			err = -EINVAL;
			goto error;
		}

		/* set conuter */
		if (reqctx->opmode == NA51055_CRYPTO_OPMODE_CTR) {
			if (reqctx->mode == NA51055_CRYPTO_MODE_DES || reqctx->mode == NA51055_CRYPTO_MODE_3DES)
				desc->counter[1] = 1;
			else
				desc->counter[3] = 1;
		}

		/* set header config */
		desc->header_cfg = (reqctx->type) | (reqctx->mode<<4) | (reqctx->opmode<<8) | (reqctx->key_src<<12) | (reqctx->get_s0<<16);

		/* set associated block config */
		block_num = 0;
		if (dma_ch->sg_ass && dma_ch->ass_nbytes) {
			dma_ch->sg_ass_work = dma_ch->sg_ass;
			dma_ch->ass_total   = 0;
			for (j=0; j<NA51055_CRYPTO_MAX_DMA_BLOCK_NUM; j++) {
				if (dma_ch->ass_total >= dma_ch->ass_nbytes)
					break;
				if (!dma_ch->sg_ass_work)
					break;

				dma_len = (dma_ch->sg_ass_work)->length;
				if ((dma_ch->ass_total + dma_len) > dma_ch->ass_nbytes) {
					dma_len = dma_ch->ass_nbytes - dma_ch->ass_total;
				}
				if(!dma_len) {
					dev_err(dev->dev, "crypto dma#%d associated invalid dma length(%u)!\n", i, dma_len);
					err = -EINVAL;
					goto error;
				}

				/* DMA block config setting */
				desc->block[j].src_addr  = ALIGN_DN(sg_dma_address(dma_ch->sg_ass_work), 4);
				desc->block[j].dst_addr  = 0;
				desc->block[j].length    = dma_len;
				desc->block[j].block_cfg = 0;
				block_num++;

				dma_ch->ass_total   += dma_len;
				dma_ch->sg_ass_work  = sg_next(dma_ch->sg_ass_work);
			}

			if (dma_ch->ass_total != dma_ch->ass_nbytes) {
				dev_err(dev->dev, "crypto dma#%d descriptor buffer not enough for associated data!(blk_num=%d)\n", i, block_num);
				err = -ENOMEM;
				goto error;
			}

			if (block_num) {
				desc->block[block_num-1].block_cfg |= 0x1;
			}
		}

		/* check GCM mode without any associated data */
		if ((block_num == 0) && (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM)) {
			desc->block[0].src_addr  = 0;
			desc->block[0].dst_addr  = 0;
			desc->block[0].length    = 0;
			desc->block[0].block_cfg = 1;
			block_num++;
		}

		/* set crypto block config */
		if (dma_ch->sg_src && dma_ch->sg_dst && dma_ch->req_nbytes) {
			dma_ch->sg_src_work = dma_ch->sg_src;
			dma_ch->sg_src_len  = (dma_ch->sg_src_work)->length;
			dma_ch->sg_src_ofs  = 0;
			dma_ch->src_total   = 0;
			dma_ch->sg_dst_work = dma_ch->sg_dst;
			dma_ch->sg_dst_len  = (dma_ch->sg_dst_work)->length;
			dma_ch->sg_dst_ofs  = 0;
			dma_ch->dst_total   = 0;
			for (j=block_num; j<NA51055_CRYPTO_MAX_DMA_BLOCK_NUM; j++) {
				if (dma_ch->src_total >= dma_ch->req_nbytes || dma_ch->dst_total >= dma_ch->req_nbytes)
					break;
				if (!dma_ch->sg_src_work || !dma_ch->sg_dst_work)
					break;

				/* caculate dma transfer length */
				dma_len = min(dma_ch->sg_src_len, dma_ch->sg_dst_len);
				if ((dma_ch->src_total + dma_len) > dma_ch->req_nbytes) {
					dma_len = dma_ch->req_nbytes - dma_ch->src_total;
				}
				if(!dma_len || (align_last && !IS_ALIGNED(dma_len, ctx->block_size))) {
					dev_err(dev->dev, "crypto dma#%d request invalid dma length(%u)!\n", i, dma_len);
					err = -EINVAL;
					goto error;
				}

				/* DMA block config setting */
				desc->block[j].src_addr  = ALIGN_DN((sg_dma_address(dma_ch->sg_src_work)+dma_ch->sg_src_ofs), 4);
				desc->block[j].dst_addr  = ALIGN_DN((sg_dma_address(dma_ch->sg_dst_work)+dma_ch->sg_dst_ofs), 4);
				desc->block[j].length    = dma_len;
				desc->block[j].block_cfg = 0;
				block_num++;

				dma_ch->src_total  += dma_len;
				dma_ch->sg_src_len -= dma_len;
				if (dma_ch->sg_src_len == 0) {
					dma_ch->sg_src_work = sg_next(dma_ch->sg_src_work);
					dma_ch->sg_src_len  = (!dma_ch->sg_src_work) ? 0 : (dma_ch->sg_src_work)->length;
					dma_ch->sg_src_ofs  = 0;
				}
				else {
					dma_ch->sg_src_ofs += dma_len;
				}

				dma_ch->dst_total  += dma_len;
				dma_ch->sg_dst_len -= dma_len;
				if (dma_ch->sg_dst_len == 0) {
					dma_ch->sg_dst_work = sg_next(dma_ch->sg_dst_work);
					dma_ch->sg_dst_len = (!dma_ch->sg_dst_work) ? 0 : (dma_ch->sg_dst_work)->length;
					dma_ch->sg_dst_ofs = 0;
				}
				else {
					dma_ch->sg_dst_ofs += dma_len;
				}
			}
		}
		else if (dma_ch->sg_src && !dma_ch->sg_dst && dma_ch->req_nbytes) {
			dma_ch->sg_src_work = dma_ch->sg_src;
			dma_ch->sg_src_len  = (dma_ch->sg_src_work)->length;
			dma_ch->sg_src_ofs  = 0;
			dma_ch->src_total   = 0;
			dma_ch->sg_dst_work = NULL;
			dma_ch->sg_dst_len  = 0;
			dma_ch->sg_dst_ofs  = 0;
			dma_ch->dst_total   = 0;
			for (j=block_num; j<NA51055_CRYPTO_MAX_DMA_BLOCK_NUM; j++) {
				if (dma_ch->src_total >= dma_ch->req_nbytes)
					break;
				if (!dma_ch->sg_src_work)
					break;

				/* caculate dma transfer length */
				dma_len = dma_ch->sg_src_len;
				if(!dma_len || (align_last && !IS_ALIGNED(dma_len, ctx->block_size))) {
					dev_err(dev->dev, "crypto dma#%d request invalid dma length(%u)!\n", i, dma_len);
					err = -EINVAL;
					goto error;
				}

				/* DMA block config setting */
				desc->block[j].src_addr  = ALIGN_DN((sg_dma_address(dma_ch->sg_src_work)+dma_ch->sg_src_ofs), 4);
				desc->block[j].dst_addr  = desc->block[j].src_addr;
				desc->block[j].length    = dma_len;
				desc->block[j].block_cfg = (1<<8);		///< turn on non_flush to block dma write out
				block_num++;

				dma_ch->src_total  += dma_len;
				dma_ch->dst_total  += dma_len;
				dma_ch->sg_src_len -= dma_len;
				if (dma_ch->sg_src_len == 0) {
					dma_ch->sg_src_work = sg_next(dma_ch->sg_src_work);
					dma_ch->sg_src_len  = (!dma_ch->sg_src_work) ? 0 : (dma_ch->sg_src_work)->length;
					dma_ch->sg_src_ofs  = 0;
				}
				else {
					dma_ch->sg_src_ofs += dma_len;
				}
			}
		}
		else {
			dma_ch->sg_src_work = NULL;
			dma_ch->sg_src_len  = 0;
			dma_ch->sg_src_ofs  = 0;
			dma_ch->src_total   = dma_ch->req_nbytes;
			dma_ch->sg_dst_work = NULL;
			dma_ch->sg_dst_len  = 0;
			dma_ch->sg_dst_ofs  = 0;
			dma_ch->dst_total   = dma_ch->req_nbytes;

			desc->block[block_num].src_addr  = 0;
			desc->block[block_num].dst_addr  = 0;
			desc->block[block_num].length    = 0;
			desc->block[block_num].block_cfg = 0;
			block_num++;
		}

		/* check GCM mode all request data ready for one DMA trigger */
		if ((reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM) && (dma_ch->src_total != dma_ch->req_nbytes)) {
			dev_err(dev->dev, "crypto dma#%d descriptor buffer not enough for crypto data!(blk_num=%d)\n", i, block_num);
			err = -ENOMEM;
			goto error;
		}

		if (block_num) {
			/* set the last block */
			desc->block[block_num-1].block_cfg |= 0x1;
			dma_enb |= (0x1<<i);

#ifdef NA51055_CRYPTO_DMA_DESC_CV_CHECK
			/* clear current cv */
			desc->cv[0] = 0;
			desc->cv[1] = 0;
			desc->cv[2] = 0;
			desc->cv[3] = 0;
#endif

			/* dummy read to ensure data in DDR */
			wmb();
			reg_value = desc->block[block_num-1].block_cfg;
			rmb();

			/* set dma channel state to busy */
			dma_ch->state = NA51055_CRYPTO_STATE_BUSY;
		}
		else {
			err = -EINVAL;
		}

error:
		if (err) {
			if (src_dma_map) {
				dma_unmap_sg(dev->dev, dma_ch->sg_src, dma_ch->sg_src_nents, DMA_TO_DEVICE);   ///< cache nothing to do
				dma_ch->sg_src       = NULL;
				dma_ch->sg_src_nents = 0;
			}
			if (dst_dma_map) {
				dma_unmap_sg(dev->dev, dma_ch->sg_dst, dma_ch->sg_dst_nents, DMA_FROM_DEVICE); ///< cache invalidate
				dma_ch->sg_dst       = NULL;
				dma_ch->sg_dst_nents = 0;
			}
			if (ass_dma_map) {
				dma_unmap_sg(dev->dev, dma_ch->sg_ass, dma_ch->sg_ass_nents, DMA_TO_DEVICE);   ///< cache nothing to do
				dma_ch->sg_ass       = NULL;
				dma_ch->sg_ass_nents = 0;
			}
			if (src_pages)
				free_pages((unsigned long)src_pages, get_order(dma_ch->req_nbytes));
			if (dst_pages)
				free_pages((unsigned long)dst_pages, get_order(ALIGN_UP(dma_ch->req_nbytes, 4)));
			if (ass_pages)
				free_pages((unsigned long)ass_pages, get_order(dma_ch->ass_nbytes));
			if (ccm_reqctx && ccm_reqctx->payload_size) {
				free_pages((unsigned long)sg_virt(&ccm_reqctx->sg_payload), get_order(ccm_reqctx->payload_size));
				ccm_reqctx->payload_size = 0;
			}

			na51055_crypto_complete(dev, i, err);
		}
	}

	/* trigger DMA channel transfer */
	if (dma_enb) {
		/* set crypto enable */
		reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_CFG_REG);
		na51055_crypto_write(dev, NA51055_CRYPTO_CFG_REG, (reg_value|0x2));

		/* set interrupt enable */
		reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_INT_ENB_REG);
		na51055_crypto_write(dev, NA51055_CRYPTO_INT_ENB_REG, (reg_value|(dma_enb<<4)));

		/* clear interrupt status */
		na51055_crypto_write(dev, NA51055_CRYPTO_INT_STS_REG, (dma_enb<<4));

		/* set DMA descriptor source address */
		for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
			if ((dma_enb & (0x1<<i)) == 0)
				continue;
			na51055_crypto_write(dev, NA51055_CRYPTO_DMA0_ADDR_REG+(i*4), dev->dma_ch[i].desc_paddr);
		}

		/* set DMA channel enable */
		reg_value  = na51055_crypto_read(dev, NA51055_CRYPTO_CTRL_REG);
		reg_value &= ~(0xf<<4);
		reg_value |= (dma_enb<<4);
		na51055_crypto_write(dev, NA51055_CRYPTO_CTRL_REG, reg_value);

		/* start timeout timer */
		for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
			if ((dma_enb & (0x1<<i)) == 0)
				continue;
			mod_timer(&dev->dma_ch[i].timer, jiffies+msecs_to_jiffies(NA51055_CRYPTO_DEFAULT_TIMEOUT));
		}
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

#ifdef NA51055_CRYPTO_PIO_SUPPORT
static int na51055_crypto_pio(struct na51055_crypto_dev *dev, NA51055_CRYPTO_MODE_T mode, NA51055_CRYPTO_TYPE_T type, const u32 *key, const u32 *src, u32 *dst)
{
#define NA51055_CRYPTO_PIO_TIMEOUT  3000
	int ret     = 0;
	u32 cnt     = 0;
	u32 reg_value;
	u8  key_cnt;
	struct na51055_ekey_hdr *ekey_hdr = (struct na51055_ekey_hdr *)key;

	if (mode >= NA51055_CRYPTO_MODE_MAX || type >= NA51055_CRYPTO_TYPE_MAX || !key || !dst)
		return -EINVAL;

	spin_lock(&dev->pio_lock);

	/* check pio mode busy or not */
	reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_CTRL_REG);
	while ((reg_value & 0x1) && (cnt++ < NA51055_CRYPTO_PIO_TIMEOUT)) {
		udelay(1);
		reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_CTRL_REG);
	}

	if (reg_value & 0x1) {
		dev_err(dev->dev, "crypto PIO mode busy!!\n");
		ret = -EINVAL;
		goto exit;
	}

	dev_dbg(dev->dev, "PIO        => mode:%d type:%d\n", mode, type);

	/* set key */
	if ((ekey_hdr->magic == NA51055_CRYPTO_EKEY_HDR_MAGIC) &&
		(ekey_hdr->offset < NA51055_CRYPTO_EKEY_OFS_MAX)   &&
		(ekey_hdr->rsvd[0] == 0)                           &&
		(ekey_hdr->rsvd[1] == 0)                           &&
		(ekey_hdr->rsvd[2] == 0)) {
		switch(mode) {
		case NA51055_CRYPTO_MODE_DES:
			key_cnt = 2;
			break;
		case NA51055_CRYPTO_MODE_3DES:
			key_cnt = 6;
			break;
		case NA51055_CRYPTO_MODE_AES_128:
			key_cnt = 4;
			break;
		case NA51055_CRYPTO_MODE_AES_256:
			key_cnt = 8;
			break;
		default:
			dev_err(dev->dev, "crypto PIO mode not support mode:%d!!\n", mode);
			ret = -EINVAL;
			goto exit;
		}

		if (na51055_crypto_trigger_efuse_key(ekey_hdr->offset, key_cnt)) {
			dev_err(dev->dev, "PIO Key    => key invalid(offset:%d)!\n", ekey_hdr->offset);
			ret = -EINVAL;
			goto exit;
		}
		else {
			dev_dbg(dev->dev, "PIO Key    => from efuse secure section offset:%d\n", ekey_hdr->offset);
		}
	}
	else {
		switch(mode) {
		case NA51055_CRYPTO_MODE_DES:
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY0_REG, key[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY1_REG, key[1]);
			dev_dbg(dev->dev, "PIO Key    => %08x %08x\n", key[0], key[1]);
			break;
		case NA51055_CRYPTO_MODE_3DES:
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY0_REG, key[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY1_REG, key[1]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY2_REG, key[2]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY3_REG, key[3]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY4_REG, key[4]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY5_REG, key[5]);
			dev_dbg(dev->dev, "PIO Key    => %08x %08x %08x %08x %08x %08x\n", key[0], key[1], key[2], key[3], key[4], key[5]);
			break;
		case NA51055_CRYPTO_MODE_AES_128:
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY0_REG, key[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY1_REG, key[1]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY2_REG, key[2]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY3_REG, key[3]);
			dev_dbg(dev->dev, "PIO Key    => %08x %08x %08x %08x\n", key[0], key[1], key[2], key[3]);
			break;
		case NA51055_CRYPTO_MODE_AES_256:
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY0_REG, key[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY1_REG, key[1]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY2_REG, key[2]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY3_REG, key[3]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY4_REG, key[4]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY5_REG, key[5]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY6_REG, key[6]);
			na51055_crypto_write(dev, NA51055_CRYPTO_KEY7_REG, key[7]);
			dev_dbg(dev->dev, "PIO Key    => %08x %08x %08x %08x %08x %08x %08x %08x\n", key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
			break;
		default:
			dev_err(dev->dev, "crypto PIO mode not support mode:%d!!\n", mode);
			ret = -EINVAL;
			goto exit;
		}
	}

	/* set input data */
	if (!src) {
		na51055_crypto_write(dev, NA51055_CRYPTO_IN0_REG, 0);
		na51055_crypto_write(dev, NA51055_CRYPTO_IN1_REG, 0);
		na51055_crypto_write(dev, NA51055_CRYPTO_IN2_REG, 0);
		na51055_crypto_write(dev, NA51055_CRYPTO_IN3_REG, 0);
		dev_dbg(dev->dev, "PIO In     => %08x %08x %08x %08x\n", 0, 0, 0, 0);
	}
	else {
		if (mode == NA51055_CRYPTO_MODE_DES || mode == NA51055_CRYPTO_MODE_3DES) {
			na51055_crypto_write(dev, NA51055_CRYPTO_IN0_REG, src[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN1_REG, src[1]);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN2_REG, 0);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN3_REG, 0);
			dev_dbg(dev->dev, "PIO In     => %08x %08x %08x %08x\n", src[0], src[1], 0, 0);
		}
		else {
			na51055_crypto_write(dev, NA51055_CRYPTO_IN0_REG, src[0]);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN1_REG, src[1]);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN2_REG, src[2]);
			na51055_crypto_write(dev, NA51055_CRYPTO_IN3_REG, src[3]);
			dev_dbg(dev->dev, "PIO In     => %08x %08x %08x %08x\n", src[0], src[1], src[2], src[3]);
		}
	}

	/* set config */
	reg_value = 0x2 | (mode<<4) | (type<<8);
	na51055_crypto_write(dev, NA51055_CRYPTO_CFG_REG, reg_value);

	/* clear PIO status */
	na51055_crypto_write(dev, NA51055_CRYPTO_INT_STS_REG, 0x1);

	/* trigger PIO mode */
	na51055_crypto_write(dev, NA51055_CRYPTO_CTRL_REG, 0x1);

	/* polling status */
	cnt = 0;
	reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_INT_STS_REG);
	while (((reg_value & 0x1) == 0) && (cnt++ < NA51055_CRYPTO_PIO_TIMEOUT)) {
		udelay(1);
		reg_value = na51055_crypto_read(dev, NA51055_CRYPTO_INT_STS_REG);
	}

	if (reg_value & 0x1) {
		/* clear status */
		na51055_crypto_write(dev, NA51055_CRYPTO_INT_STS_REG, 0x1);

		/* read output data */
		if (mode == NA51055_CRYPTO_MODE_DES || mode == NA51055_CRYPTO_MODE_3DES) {
			dst[0] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT0_REG);
			dst[1] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT1_REG);
			dev_dbg(dev->dev, "PIO Out    => %08x %08x\n", dst[0], dst[1]);
		}
		else {
			dst[0] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT0_REG);
			dst[1] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT1_REG);
			dst[2] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT2_REG);
			dst[3] = na51055_crypto_read(dev, NA51055_CRYPTO_OUT3_REG);
			dev_dbg(dev->dev, "PIO Out    => %08x %08x %08x %08x\n", dst[0], dst[1], dst[2], dst[3]);
		}
	}
	else {
		dev_err(dev->dev, "crypto PIO mode timeout!!\n");
		ret = -EINVAL;
	}

exit:
	spin_unlock(&dev->pio_lock);

	return ret;
}
#endif

static int na51055_crypto_cra_init(struct crypto_tfm *tfm)
{
	struct na51055_crypto_ctx *ctx = crypto_tfm_ctx(tfm);
	struct crypto_alg         *alg = tfm->__crt_alg;

	if (alg->cra_flags & CRYPTO_ALG_NEED_FALLBACK) {
		ctx->fallback_u.skcipher = crypto_alloc_skcipher(crypto_tfm_alg_name(tfm), CRYPTO_ALG_TYPE_SKCIPHER, (CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK));
		if (IS_ERR(ctx->fallback_u.skcipher)) {
			pr_err( "failed to allocate skcipher fallback for %s\n", alg->cra_name);
			ctx->fallback_u.skcipher = NULL;
		}
	}

	if (ctx->fallback_u.skcipher)
		tfm->crt_ablkcipher.reqsize = max(sizeof(struct na51055_crypto_reqctx), crypto_skcipher_reqsize(ctx->fallback_u.skcipher));
	else
		tfm->crt_ablkcipher.reqsize = sizeof(struct na51055_crypto_reqctx);

	return 0;
}

static void na51055_crypto_cra_exit(struct crypto_tfm *tfm)
{
	struct na51055_crypto_ctx *ctx = crypto_tfm_ctx(tfm);

	if (ctx->fallback_u.skcipher) {
		crypto_free_skcipher(ctx->fallback_u.skcipher);
		ctx->fallback_u.skcipher = NULL;
	}

	return;
}

static int na51055_crypto_cra_aes_gcm_init(struct crypto_aead *tfm)
{
	struct na51055_crypto_ctx *ctx     = crypto_aead_ctx(tfm);
	struct crypto_tfm         *org_tfm = crypto_aead_tfm(tfm);
	struct crypto_alg         *alg     = org_tfm->__crt_alg;

	if (alg->cra_flags & CRYPTO_ALG_NEED_FALLBACK) {
		ctx->fallback_u.aead = crypto_alloc_aead(crypto_tfm_alg_name(org_tfm), CRYPTO_ALG_TYPE_AEAD, (CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK));
		if (IS_ERR(ctx->fallback_u.aead)) {
			pr_err( "failed to allocate aead fallback for %s\n", alg->cra_name);
			ctx->fallback_u.aead = NULL;
		}
	}

	if (ctx->fallback_u.aead)
		crypto_aead_set_reqsize(tfm, max(sizeof(struct na51055_crypto_gcm_reqctx), (sizeof(struct aead_request)+crypto_aead_reqsize(ctx->fallback_u.aead))));
	else
		crypto_aead_set_reqsize(tfm, sizeof(struct na51055_crypto_gcm_reqctx));

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
#ifndef NA51055_CRYPTO_PIO_SUPPORT
	/* allocate cipher handler */
	ctx->cipher = crypto_alloc_cipher("aes", CRYPTO_ALG_TYPE_CIPHER, CRYPTO_ALG_NEED_FALLBACK);
	if (IS_ERR(ctx->cipher)) {
		pr_err( "failed to allocate cipher\n");
		ctx->cipher = NULL;
	}
#endif

	/* allocate ghash handler */
	ctx->ghash = crypto_alloc_ahash("ghash", CRYPTO_ALG_TYPE_AHASH, (CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK));
	if (IS_ERR(ctx->ghash)) {
		pr_err( "failed to allocate ghash\n");
		ctx->ghash = NULL;
	}

	/* allocate ghash request */
	if (ctx->ghash) {
		ctx->ghash_req = ahash_request_alloc(ctx->ghash, GFP_KERNEL);
		if (!ctx->ghash_req) {
			pr_err( "failed to allocate ghash request\n");
		}
	}
#endif

	return 0;
}

static void na51055_crypto_cra_aes_gcm_exit(struct crypto_aead *tfm)
{
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (ctx->fallback_u.aead) {
		crypto_free_aead(ctx->fallback_u.aead);
		ctx->fallback_u.aead = NULL;
	}

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	if (ctx->cipher) {
		crypto_free_cipher(ctx->cipher);
		ctx->cipher = NULL;
	}
	if (ctx->ghash_req) {
		ahash_request_free(ctx->ghash_req);
		ctx->ghash_req = NULL;
	}
	if (ctx->ghash) {
		crypto_free_ahash(ctx->ghash);
		ctx->ghash = NULL;
	}
#endif

	return;
}

static int na51055_crypto_cra_aes_ccm_init(struct crypto_aead *tfm)
{
	struct na51055_crypto_ctx *ctx     = crypto_aead_ctx(tfm);
	struct crypto_tfm         *org_tfm = crypto_aead_tfm(tfm);
	struct crypto_alg         *alg     = org_tfm->__crt_alg;

	if (alg->cra_flags & CRYPTO_ALG_NEED_FALLBACK) {
		ctx->fallback_u.aead = crypto_alloc_aead(crypto_tfm_alg_name(org_tfm), CRYPTO_ALG_TYPE_AEAD, (CRYPTO_ALG_ASYNC | CRYPTO_ALG_NEED_FALLBACK));
		if (IS_ERR(ctx->fallback_u.aead)) {
			pr_err( "failed to allocate aead fallback for %s\n", alg->cra_name);
			ctx->fallback_u.aead = NULL;
		}
	}

	if (ctx->fallback_u.aead)
		crypto_aead_set_reqsize(tfm, max(sizeof(struct na51055_crypto_ccm_reqctx), (sizeof(struct aead_request)+crypto_aead_reqsize(ctx->fallback_u.aead))));
	else
		crypto_aead_set_reqsize(tfm, sizeof(struct na51055_crypto_ccm_reqctx));

	return 0;
}

static void na51055_crypto_cra_aes_ccm_exit(struct crypto_aead *tfm)
{
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (ctx->fallback_u.aead) {
		crypto_free_aead(ctx->fallback_u.aead);
		ctx->fallback_u.aead = NULL;
	}

	return;
}

static int na51055_crypto_aes_do_fallback(struct ablkcipher_request *req, bool is_encrypt)
{
	struct crypto_tfm         *tfm = crypto_ablkcipher_tfm(crypto_ablkcipher_reqtfm(req));
	struct na51055_crypto_ctx *ctx = crypto_tfm_ctx(tfm);
	int err;

	if (!ctx->fallback_u.skcipher) {
		return -EINVAL;
	}
	else {
		SKCIPHER_REQUEST_ON_STACK(subreq, ctx->fallback_u.skcipher);

		/*
		 * Change the request to use the software fallback transform, and once
		 * the ciphering has completed, put the old transform back into the
		 * request.
		 */
		skcipher_request_set_tfm(subreq, ctx->fallback_u.skcipher);
		skcipher_request_set_callback(subreq, req->base.flags, NULL, NULL);
		skcipher_request_set_crypt(subreq, req->src, req->dst, req->nbytes, req->info);
		err = is_encrypt ? crypto_skcipher_encrypt(subreq) : crypto_skcipher_decrypt(subreq);
		skcipher_request_zero(subreq);

		return err;
	}
}

static int na51055_crypto_aes_aead_do_fallback(struct aead_request *req, bool is_encrypt)
{
	struct crypto_tfm         *tfm = crypto_aead_tfm(crypto_aead_reqtfm(req));
	struct na51055_crypto_ctx *ctx = crypto_tfm_ctx(tfm);

	if (!ctx->fallback_u.aead) {
		return -EINVAL;
	}
	else {
		struct aead_request *subreq = aead_request_ctx(req);

		aead_request_set_tfm(subreq, ctx->fallback_u.aead);
		aead_request_set_callback(subreq, req->base.flags, req->base.complete, req->base.data);
		aead_request_set_crypt(subreq, req->src, req->dst, req->cryptlen, req->iv);
		aead_request_set_ad(subreq, req->assoclen);

		return is_encrypt ? crypto_aead_encrypt(subreq) : crypto_aead_decrypt(subreq);
	}
}

static int na51055_crypto_aes_setkey(struct crypto_ablkcipher *tfm, const u8 *key, unsigned int keylen)
{
	int err;
	struct na51055_crypto_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (unlikely(keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_256)) {
		/* The requested key size is not supported by HW, do a fallback */
		if (ctx->fallback_u.skcipher) {
			crypto_skcipher_clear_flags(ctx->fallback_u.skcipher, CRYPTO_TFM_REQ_MASK);
			crypto_skcipher_set_flags(ctx->fallback_u.skcipher, crypto_ablkcipher_get_flags(tfm)&CRYPTO_TFM_REQ_MASK);
			err = crypto_skcipher_setkey(ctx->fallback_u.skcipher, key, keylen);
			if (err) {
				crypto_ablkcipher_clear_flags(tfm, CRYPTO_TFM_REQ_MASK);
				crypto_ablkcipher_set_flags(tfm, crypto_skcipher_get_flags(ctx->fallback_u.skcipher)&CRYPTO_TFM_REQ_MASK);
				return err;
			}
		}
		else {
			crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
			return -EINVAL;
		}
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static void na51055_crypto_aes_gcm_copy_hash(struct aead_request *req)
{
	struct crypto_aead               *tfm      = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx     = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx   = aead_request_ctx(req);
	u8                               *auth_tag = (u8 *)reqctx->auth_tag;

	scatterwalk_map_and_copy(auth_tag, req->dst, (req->assoclen + req->cryptlen), crypto_aead_authsize(tfm), 1);

	dev_dbg((pctx->dev)->dev, "AuthTag Out=> %08x %08x %08x %08x\n", reqctx->auth_tag[0], reqctx->auth_tag[1], reqctx->auth_tag[2], reqctx->auth_tag[3]);
}

static int na51055_crypto_aes_gcm_verify_hash(struct aead_request *req)
{
	struct crypto_aead               *tfm       = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx      = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx    = aead_request_ctx(req);
	u8                               *auth_tag  = (u8 *)reqctx->auth_tag;
	u8                               *iauth_tag = (u8 *)reqctx->iauth_tag;
	unsigned int                     authsize   = crypto_aead_authsize(tfm);

	crypto_xor(auth_tag, iauth_tag, authsize);
	scatterwalk_map_and_copy(iauth_tag, req->src, (req->assoclen + req->cryptlen - authsize), authsize, 0);

	dev_dbg((pctx->dev)->dev, "AuthTag Out=> %08x %08x %08x %08x\n", reqctx->auth_tag[0],  reqctx->auth_tag[1],  reqctx->auth_tag[2],  reqctx->auth_tag[3]);
	dev_dbg((pctx->dev)->dev, "AuthTag In => %08x %08x %08x %08x\n", reqctx->iauth_tag[0], reqctx->iauth_tag[1], reqctx->iauth_tag[2], reqctx->iauth_tag[3]);

	return crypto_memneq(iauth_tag, auth_tag, authsize) ? -EBADMSG : 0;
}

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
static inline unsigned int __gcm_remain(unsigned int len)
{
	len &= 0xfU;
	return len ? 16 - len : 0;
}

static void na51055_crypto_aes_gcm_encrypt_hash_done(struct aead_request *req, int err)
{
	if (!err)
		na51055_crypto_aes_gcm_copy_hash(req);

	aead_request_complete(req, err);
}

static void na51055_crypto_aes_gcm_decrypt_hash_done(struct aead_request *req, int err)
{
	struct crypto_aead        *tfm  = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx *pctx = crypto_aead_ctx(tfm);

	if (!err)
		na51055_crypto_handle_req(pctx->dev, &req->base, 0);	///< ghash complete then issue hardware to do gcm decryption
	else
		aead_request_complete(req, err);
}

static int na51055_crypto_aes_gcm_hash_update(struct aead_request *req, crypto_completion_t compl, struct scatterlist *src, unsigned int len)
{
	struct crypto_aead        *tfm   = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx *pctx  = crypto_aead_ctx(tfm);
	struct ahash_request      *ahreq = pctx->ghash_req;

	ahash_request_set_callback(ahreq, aead_request_flags(req), compl, req);
	ahash_request_set_crypt(ahreq, src, NULL, len);

	return crypto_ahash_update(ahreq);
}

static int na51055_crypto_aes_gcm_hash_remain(struct aead_request *req, unsigned int remain, crypto_completion_t compl)
{
	struct crypto_aead               *tfm    = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx   = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);
	struct ahash_request             *ahreq  = pctx->ghash_req;

	ahash_request_set_callback(ahreq, aead_request_flags(req), compl, req);
	sg_init_one(&reqctx->ghash_src, na51055_crypto_aes_zeroes, remain);
	ahash_request_set_crypt(ahreq, &reqctx->ghash_src, NULL, remain);

	return crypto_ahash_update(ahreq);
}

static void __gcm_hash_final_done(struct aead_request *req, int err)
{
	struct crypto_aead               *tfm       = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx      = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx    = aead_request_ctx(req);
	u8                               *auth_tag  = (u8 *)reqctx->auth_tag;
	u8                               *iauth_tag = (u8 *)reqctx->iauth_tag;

	if (!err) {
		crypto_xor(auth_tag, iauth_tag, crypto_aead_authsize(tfm));
		dev_dbg((pctx->dev)->dev, "GHASH Out  => %08x %08x %08x %08x\n", reqctx->iauth_tag[0], reqctx->iauth_tag[1], reqctx->iauth_tag[2], reqctx->iauth_tag[3]);
	}

	reqctx->ghash_ctx.complete(req, err);
}

static void na51055_crypto_aes_gcm_hash_final_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_final_done(req, err);
}

static int na51055_crypto_aes_gcm_hash_final(struct aead_request *req)
{
	struct crypto_aead               *tfm       = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx      = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx    = aead_request_ctx(req);
	struct ahash_request             *ahreq     = pctx->ghash_req;
	u8                               *iauth_tag = (u8 *)reqctx->iauth_tag;

	ahash_request_set_callback(ahreq, aead_request_flags(req), na51055_crypto_aes_gcm_hash_final_done, req);
	ahash_request_set_crypt(ahreq, NULL, iauth_tag, 0);

	return crypto_ahash_final(ahreq);
}

static void __gcm_hash_len_done(struct aead_request *req, int err)
{
	if (!err) {
		err = na51055_crypto_aes_gcm_hash_final(req);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	__gcm_hash_final_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_len_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_len_done(req, err);
}

static int na51055_crypto_aes_gcm_hash_len(struct aead_request *req)
{
	struct crypto_aead               *tfm    = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *pctx   = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);
	struct ahash_request             *ahreq  = pctx->ghash_req;
	struct na51055_crypto_ghash_ctx  *gctx   = &reqctx->ghash_ctx;
	u128                             lengths;

	lengths.a = cpu_to_be64(req->assoclen * 8);
	lengths.b = cpu_to_be64(gctx->cryptlen * 8);
	memcpy(reqctx->iauth_tag, &lengths, 16);
	sg_init_one(&reqctx->ghash_src, reqctx->iauth_tag, 16);
	ahash_request_set_callback(ahreq, aead_request_flags(req), na51055_crypto_aes_gcm_hash_len_done, req);
	ahash_request_set_crypt(ahreq, &reqctx->ghash_src, NULL, sizeof(lengths));

	return crypto_ahash_update(ahreq);
}

static void __gcm_hash_crypt_remain_done(struct aead_request *req, int err)
{
	if (!err) {
		err = na51055_crypto_aes_gcm_hash_len(req);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	__gcm_hash_len_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_crypt_remain_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_crypt_remain_done(req, err);
}

static void __gcm_hash_crypt_done(struct aead_request *req, int err)
{
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);
	struct na51055_crypto_ghash_ctx  *gctx   = &reqctx->ghash_ctx;
	unsigned int remain;

	if (!err) {
		remain = __gcm_remain(gctx->cryptlen);
		BUG_ON(!remain);
		err = na51055_crypto_aes_gcm_hash_remain(req, remain, na51055_crypto_aes_gcm_hash_crypt_remain_done);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	__gcm_hash_crypt_remain_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_crypt_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_crypt_done(req, err);
}

static void __gcm_hash_assoc_remain_done(struct aead_request *req, int err)
{
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);
	struct na51055_crypto_ghash_ctx  *gctx   = &reqctx->ghash_ctx;
	unsigned int                     remain  = 0;
	crypto_completion_t              compl;

	if (!err && gctx->cryptlen) {
		remain = __gcm_remain(gctx->cryptlen);
		compl = remain ? na51055_crypto_aes_gcm_hash_crypt_done : na51055_crypto_aes_gcm_hash_crypt_remain_done;
		err   = na51055_crypto_aes_gcm_hash_update(req, compl, gctx->src, gctx->cryptlen);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	if (remain)
		__gcm_hash_crypt_done(req, err);
	else
		__gcm_hash_crypt_remain_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_assoc_remain_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_assoc_remain_done(req, err);
}

static void __gcm_hash_assoc_done(struct aead_request *req, int err)
{
	unsigned int              remain;

	if (!err) {
		remain = __gcm_remain(req->assoclen);
		BUG_ON(!remain);
		err = na51055_crypto_aes_gcm_hash_remain(req, remain, na51055_crypto_aes_gcm_hash_assoc_remain_done);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	__gcm_hash_assoc_remain_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_assoc_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_assoc_done(req, err);
}

static void __gcm_hash_init_done(struct aead_request *req, int err)
{
	unsigned int        remain = 0;
	crypto_completion_t compl;

	if (!err && req->assoclen) {
		remain = __gcm_remain(req->assoclen);
		compl  = remain ? na51055_crypto_aes_gcm_hash_assoc_done : na51055_crypto_aes_gcm_hash_assoc_remain_done;
		err    = na51055_crypto_aes_gcm_hash_update(req, compl, req->src, req->assoclen);
		if (err == -EINPROGRESS || err == -EBUSY)
			return;
	}

	if (remain)
		__gcm_hash_assoc_done(req, err);
	else
		__gcm_hash_assoc_remain_done(req, err);
}

static void na51055_crypto_aes_gcm_hash_init_done(struct crypto_async_request *areq, int err)
{
	struct aead_request *req = (struct aead_request *)areq->data;

	__gcm_hash_init_done(req, err);
}

static int na51055_crypto_aes_gcm_hash(struct aead_request *req, struct na51055_crypto_ctx *pctx)
{
	struct ahash_request             *ahreq  = pctx->ghash_req;
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);
	struct na51055_crypto_ghash_ctx  *gctx   = &reqctx->ghash_ctx;
	unsigned int                     remain;
	crypto_completion_t              compl;
	int err;

	if (!pctx->ghash || !pctx->ghash_req)
		return -EBADMSG;

	/* hash init */
	ahash_request_set_callback(ahreq, aead_request_flags(req), na51055_crypto_aes_gcm_hash_init_done, req);
	err = crypto_ahash_init(ahreq);
	if (err)
		return err;

	/* hash assoc data */
	remain = __gcm_remain(req->assoclen);
	compl = remain ? na51055_crypto_aes_gcm_hash_assoc_done : na51055_crypto_aes_gcm_hash_assoc_remain_done;
	err = na51055_crypto_aes_gcm_hash_update(req, compl, req->src, req->assoclen);
	if (err)
		return err;
	if (remain) {
		err = na51055_crypto_aes_gcm_hash_remain(req, remain, na51055_crypto_aes_gcm_hash_assoc_remain_done);
		if (err)
			return err;
	}

	/* hash crypt data */
	remain = __gcm_remain(gctx->cryptlen);
	compl = remain ? na51055_crypto_aes_gcm_hash_crypt_done : na51055_crypto_aes_gcm_hash_crypt_remain_done;
	err = na51055_crypto_aes_gcm_hash_update(req, compl, gctx->src, gctx->cryptlen);
	if (err)
		return err;
	if (remain) {
		err = na51055_crypto_aes_gcm_hash_remain(req, remain, na51055_crypto_aes_gcm_hash_crypt_remain_done);
		if (err)
			return err;
	}

	/* hash length data => Len(A)||Len(C) */
	err = na51055_crypto_aes_gcm_hash_len(req);
	if (err)
		return err;

	/* hash final output */
	err = na51055_crypto_aes_gcm_hash_final(req);
	if (err)
		return err;

	dev_dbg((pctx->dev)->dev, "GHASH Out  => %08x %08x %08x %08x\n", reqctx->iauth_tag[0], reqctx->iauth_tag[1], reqctx->iauth_tag[2], reqctx->iauth_tag[3]);

	return 0;
}
#endif

static int na51055_crypto_aes_gcm_setkey(struct crypto_aead *tfm, const u8 *key, unsigned int keylen)
{
	int err;
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (unlikely(keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_256)) {
		/* The requested key size is not supported by HW, do a fallback */
		if (ctx->fallback_u.aead) {
			crypto_aead_clear_flags(ctx->fallback_u.aead, CRYPTO_TFM_REQ_MASK);
			crypto_aead_set_flags(ctx->fallback_u.aead, crypto_aead_get_flags(tfm)&CRYPTO_TFM_REQ_MASK);
			err = crypto_aead_setkey(ctx->fallback_u.aead, key, keylen);
			if (err) {
				crypto_aead_clear_flags(tfm, CRYPTO_TFM_REQ_MASK);
				crypto_aead_set_flags(tfm, crypto_aead_get_flags(ctx->fallback_u.aead)&CRYPTO_TFM_REQ_MASK);
				return err;
			}
		}
		else {
			crypto_aead_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
			return -EINVAL;
		}
	}

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
#ifndef NA51055_CRYPTO_PIO_SUPPORT
	/* use software cipher to create gcm ghash key with zero pattern */
	if (ctx->cipher) {
		crypto_cipher_clear_flags(ctx->cipher, CRYPTO_TFM_REQ_MASK);
		crypto_cipher_set_flags(ctx->cipher, crypto_aead_get_flags(tfm)&CRYPTO_TFM_REQ_MASK);
		err = crypto_cipher_setkey(ctx->cipher, key, keylen);
		if (err) {
			crypto_aead_clear_flags(tfm, CRYPTO_TFM_REQ_MASK);
			crypto_aead_set_flags(tfm, crypto_cipher_get_flags(ctx->cipher)&CRYPTO_TFM_REQ_MASK);
			return err;
		}
		crypto_cipher_encrypt_one(ctx->cipher, (u8 *)ctx->key, na51055_crypto_aes_zeroes);
	}
#else
	/* use PIO mode to create gcm ghash key with zero pattern */
	memcpy(ctx->key, key, keylen);
	err = na51055_crypto_pio(na51055_cdev, ((keylen == AES_KEYSIZE_128) ? NA51055_CRYPTO_MODE_AES_128 : NA51055_CRYPTO_MODE_AES_256), NA51055_CRYPTO_TYPE_ENCRYPT, ctx->key, NULL, ctx->key);
	if (err)
		return err;
#endif

	if (ctx->ghash) {
		crypto_ahash_clear_flags(ctx->ghash, CRYPTO_TFM_REQ_MASK);
		crypto_ahash_set_flags(ctx->ghash, crypto_aead_get_flags(tfm)&CRYPTO_TFM_REQ_MASK);
		err = crypto_ahash_setkey(ctx->ghash, (u8 *)ctx->key, AES_BLOCK_SIZE);
		if (err) {
			crypto_aead_clear_flags(tfm, CRYPTO_TFM_REQ_MASK);
			crypto_aead_set_flags(tfm, crypto_ahash_get_flags(ctx->ghash)&CRYPTO_TFM_REQ_MASK);
			return err;
		}
	}
#endif

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int na51055_crypto_aes_gcm_setauthsize(struct crypto_aead *tfm, unsigned int authsize)
{
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (authsize > crypto_aead_alg(tfm)->maxauthsize)
		return -EINVAL;

	if (ctx->fallback_u.aead) {
		crypto_aead_setauthsize(ctx->fallback_u.aead, authsize);
	}

	tfm->authsize = authsize;

	return 0;
}

static void na51055_crypto_aes_gcm_complete(struct na51055_crypto_dev *dev, NA51055_CRYPTO_DMA_CH_T ch, int err)
{
	struct aead_request              *aead_req;
	struct na51055_crypto_gcm_reqctx *reqctx;

	if (ch >= NA51055_CRYPTO_DMA_CH_MAX)
		return;

	aead_req = aead_request_cast(dev->dma_ch[ch].req);
	reqctx   = aead_request_ctx(aead_req);

	if (err) {
		aead_request_complete(aead_req, err);
	}
	else {
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
		/* copy hardware output E(K, Y0) value for auth tag */
		memcpy(reqctx->auth_tag, (u8 *)(dev->dma_ch[ch].desc)->s0, AES_BLOCK_SIZE);

		if (reqctx->type == NA51055_CRYPTO_TYPE_ENCRYPT) {
			unsigned long flags;

			/* gcm encryption, hardware do gcm encrypt first then issue software to do ghash request */
			spin_lock_irqsave(&dev->queue_lock, flags);
			crypto_enqueue_request(&dev->hash_queue, dev->dma_ch[ch].req);
			spin_unlock_irqrestore(&dev->queue_lock, flags);
			tasklet_schedule(&dev->hash_tasklet);
		}
		else {
			/* gcm dencryption, software do ghash first before hardware do gcm decrypt */
			err = na51055_crypto_aes_gcm_verify_hash(aead_req);
			aead_request_complete(aead_req, err);
		}
#else
		/* copy hardware output E(K, Y0) and GHash value for auth tag */
		memcpy(reqctx->auth_tag,  (u8 *)(dev->dma_ch[ch].desc)->s0,    AES_BLOCK_SIZE);
		memcpy(reqctx->iauth_tag, (u8 *)(dev->dma_ch[ch].desc)->ghash, AES_BLOCK_SIZE);

		if (reqctx->type == NA51055_CRYPTO_TYPE_ENCRYPT) {
			crypto_xor((u8 *)reqctx->auth_tag, (u8 *)reqctx->iauth_tag, crypto_aead_authsize(crypto_aead_reqtfm(aead_req)));
			na51055_crypto_aes_gcm_copy_hash(aead_req);
		}
		else {
			err = na51055_crypto_aes_gcm_verify_hash(aead_req);
		}
		aead_request_complete(aead_req, err);
#endif
	}
	dev->dma_ch[ch].req   = NULL;
	dev->dma_ch[ch].state = NA51055_CRYPTO_STATE_IDLE;
}

static inline int __ccm_check_iv(const u8 *iv)
{
	/* 2 <= L <= 8, so 1 <= L' <= 7. */
	if (1 > iv[0] || iv[0] > 7)
		return -EINVAL;

	return 0;
}

static int __ccm_set_msg_len(u8 *block, unsigned int msglen, int csize)
{
	__be32 data;

	memset(block, 0, csize);
	block += csize;

	if (csize >= 4)
		csize = 4;
	else if (msglen > (1 << (8 * csize)))
		return -EOVERFLOW;

	data = cpu_to_be32(msglen);
	memcpy(block - csize, (u8 *)&data + 4 - csize, csize);

	return 0;
}

static int __ccm_generate_b0(u8 *iv, unsigned int assoclen, unsigned int authsize, unsigned int cryptlen, u8 *b0)
{
	unsigned int l, lp, m = authsize;
	int rc;

	memcpy(b0, iv, 16);

	lp = b0[0];
	l  = lp + 1;

	/* set m, bits 3-5 */
	*b0 |= (8 * ((m - 2) / 2));

	/* set adata, bit 6, if associated data is used */
	if (assoclen)
		*b0 |= 64;

	rc = __ccm_set_msg_len(b0 + 16 - l, cryptlen, l);

	return rc;
}

static void na51055_crypto_aes_ccm_complete(struct na51055_crypto_dev *dev, NA51055_CRYPTO_DMA_CH_T ch, int err)
{
	struct crypto_aead               *tfm;
	struct aead_request              *aead_req;
	struct na51055_crypto_ccm_reqctx *reqctx;
	unsigned long flags;

	if (ch >= NA51055_CRYPTO_DMA_CH_MAX)
		return;

	aead_req = aead_request_cast(dev->dma_ch[ch].req);
	tfm      = crypto_aead_reqtfm(aead_req);
	reqctx   = aead_request_ctx(aead_req);

	/* free payload buffer */
	if (reqctx->payload_size) {
		free_pages((unsigned long)sg_virt(&reqctx->sg_payload), get_order(reqctx->sg_payload.length));
		reqctx->payload_size = 0;
	}

	if (err) {
		aead_request_complete(aead_req, err);
	}
	else {
		if (reqctx->ccm_type == NA51055_CRYPTO_CCM_TYPE_ENCRYPT) {
			if (reqctx->opmode == NA51055_CRYPTO_OPMODE_CBC) {
				/* copy tag */
				memcpy(reqctx->auth_tag, (u8 *)(dev->dma_ch[ch].desc)->cv, AES_BLOCK_SIZE);

				dev_dbg(dev->dev, "Tag Out    => %08x %08x %08x %08x\n", reqctx->auth_tag[0], reqctx->auth_tag[1], reqctx->auth_tag[2], reqctx->auth_tag[3]);

				/* ccm encryption, hardware do payload CBC encrypt first to create tag then issue CTR to create cipher */
				reqctx->type   = NA51055_CRYPTO_TYPE_ENCRYPT;
				reqctx->opmode = NA51055_CRYPTO_OPMODE_CTR;
				reqctx->get_s0 = 1;
				spin_lock_irqsave(&dev->queue_lock, flags);
				crypto_enqueue_request(&dev->queue, dev->dma_ch[ch].req);
				spin_unlock_irqrestore(&dev->queue_lock, flags);
				tasklet_schedule(&dev->queue_tasklet);
			}
			else {
				/* tag XOR s0 to create auth_tag */
				crypto_xor((u8 *)reqctx->auth_tag, (u8 *)(dev->dma_ch[ch].desc)->s0, AES_BLOCK_SIZE);

				dev_dbg(dev->dev, "AuthTag Out=> %08x %08x %08x %08x\n", reqctx->auth_tag[0], reqctx->auth_tag[1], reqctx->auth_tag[2], reqctx->auth_tag[3]);

				/* copy auth_tag to destination buffer */
				scatterwalk_map_and_copy(reqctx->auth_tag, aead_req->dst, (aead_req->assoclen + reqctx->cryptlen), crypto_aead_authsize(tfm), 1);

				/* request completed */
				aead_request_complete(aead_req, err);
			}
		}
		else if (reqctx->ccm_type == NA51055_CRYPTO_CCM_TYPE_DECRYPT) {
			if (reqctx->opmode == NA51055_CRYPTO_OPMODE_CTR) {
				/* copy S0 */
				memcpy(reqctx->auth_tag, (u8 *)(dev->dma_ch[ch].desc)->s0, AES_BLOCK_SIZE);

				/* ccm decryption, hardware do ctr decrypt first then issue cbc */
				reqctx->type   = NA51055_CRYPTO_TYPE_ENCRYPT;
				reqctx->opmode = NA51055_CRYPTO_OPMODE_CBC;
				reqctx->get_s0 = 0;
				spin_lock_irqsave(&dev->queue_lock, flags);
				crypto_enqueue_request(&dev->payload_queue, dev->dma_ch[ch].req);
				spin_unlock_irqrestore(&dev->queue_lock, flags);
				tasklet_schedule(&dev->payload_tasklet);
			}
			else {
				/* make auth_tag */
				crypto_xor((u8 *)reqctx->auth_tag, (u8 *)(dev->dma_ch[ch].desc)->cv, AES_BLOCK_SIZE);

				dev_dbg(dev->dev, "AuthTag Out=> %08x %08x %08x %08x\n", reqctx->auth_tag[0], reqctx->auth_tag[1], reqctx->auth_tag[2], reqctx->auth_tag[3]);

				/* copy source tag */
				scatterwalk_map_and_copy(reqctx->iauth_tag, aead_req->src, (aead_req->assoclen + reqctx->cryptlen), crypto_aead_authsize(tfm), 0);

				dev_dbg(dev->dev, "AuthTag In => %08x %08x %08x %08x\n", reqctx->iauth_tag[0], reqctx->iauth_tag[1], reqctx->iauth_tag[2], reqctx->iauth_tag[3]);

				/* verify tag */
				if (crypto_memneq(reqctx->auth_tag, reqctx->iauth_tag, crypto_aead_authsize(tfm)))
					aead_request_complete(aead_req, -EBADMSG);
				else
					aead_request_complete(aead_req, 0);
			}
		}
		else {
			dev_err(dev->dev, "crypto ccm type invalid!\n");
			aead_request_complete(aead_req, -EINVAL);
		}
	}
	dev->dma_ch[ch].req   = NULL;
	dev->dma_ch[ch].state = NA51055_CRYPTO_STATE_IDLE;
}

static int na51055_crypto_aes_ccm_setkey(struct crypto_aead *tfm, const u8 *key, unsigned int keylen)
{
	int err;
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (unlikely(keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_256)) {
		/* The requested key size is not supported by HW, do a fallback */
		if (ctx->fallback_u.aead) {
			crypto_aead_clear_flags(ctx->fallback_u.aead, CRYPTO_TFM_REQ_MASK);
			crypto_aead_set_flags(ctx->fallback_u.aead, crypto_aead_get_flags(tfm)&CRYPTO_TFM_REQ_MASK);
			err = crypto_aead_setkey(ctx->fallback_u.aead, key, keylen);
			if (err) {
				crypto_aead_clear_flags(tfm, CRYPTO_TFM_REQ_MASK);
				crypto_aead_set_flags(tfm, crypto_aead_get_flags(ctx->fallback_u.aead)&CRYPTO_TFM_REQ_MASK);
				return err;
			}
		}
		else {
			crypto_aead_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
			return -EINVAL;
		}
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int na51055_crypto_aes_ccm_setauthsize(struct crypto_aead *tfm, unsigned int authsize)
{
	struct na51055_crypto_ctx *ctx = crypto_aead_ctx(tfm);

	if (authsize > crypto_aead_alg(tfm)->maxauthsize)
		return -EINVAL;

	if (ctx->fallback_u.aead) {
		crypto_aead_setauthsize(ctx->fallback_u.aead, authsize);
	}

	tfm->authsize = authsize;

	return 0;
}

static void na51055_crypto_queue_tasklet(unsigned long data)
{
	struct na51055_crypto_dev   *dev = (struct na51055_crypto_dev *)data;
	struct crypto_async_request *async_req, *backlog;
	unsigned long flags;
	int i, req_cnt = 0;

	spin_lock_irqsave(&dev->lock, flags);

	backlog = crypto_get_backlog(&dev->queue);

	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		if (dev->dma_ch[i].state != NA51055_CRYPTO_STATE_IDLE)
			continue;

		spin_lock(&dev->queue_lock);
		async_req = crypto_dequeue_request(&dev->queue);
		spin_unlock(&dev->queue_lock);

		if (!async_req)
			goto exit;

		if (backlog)
			backlog->complete(backlog, -EINPROGRESS);

		/* assign new request to dma channel */
		dev->dma_ch[i].req   = async_req;
		dev->dma_ch[i].ctx   = crypto_tfm_ctx(async_req->tfm);
		dev->dma_ch[i].state = NA51055_CRYPTO_STATE_START;

		req_cnt++;
	}

exit:
	spin_unlock_irqrestore(&dev->lock, flags);

	if (req_cnt)
		na51055_crypto_start(dev);
}

static void na51055_crypto_done_tasklet(unsigned long data)
{
	struct na51055_crypto_dev        *dev = (struct na51055_crypto_dev *)data;
	struct ablkcipher_request        *ablk_req;
	struct aead_request              *aead_req;
	struct na51055_crypto_ctx        *ctx;
	struct na51055_crypto_reqctx     *reqctx;
	struct na51055_crypto_ccm_reqctx *ccm_reqctx;
	struct na51055_crypto_gcm_reqctx *gcm_reqctx;
	struct na51055_crypto_dma_ch     *dma_ch;
	volatile struct na51055_crypto_dma_desc *desc;
	struct scatterlist               *req_dst_sg;
	struct scatterlist               *req_src_sg;
	unsigned long                    flags;
	int                              err, i, j;
	int                              block_num;
	int                              align_chk;
	u8                               *iv;
	u32                              ivsize;
	u32                              reg_value;
	u32                              dma_len;
	u32                              dma_enb = 0;

	spin_lock_irqsave(&dev->lock, flags);

	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		dma_ch = &dev->dma_ch[i];

		if (dma_ch->state != NA51055_CRYPTO_STATE_DONE)
			continue;

		if (!dma_ch->req) {
			dev_err(dev->dev, "crypto dma#%d done but request is empty!!\n", i);
			dma_ch->state = NA51055_CRYPTO_STATE_IDLE;
			continue;
		}

		desc = dma_ch->desc;
		ctx  = dma_ch->ctx;

		if (crypto_tfm_alg_type(dma_ch->req->tfm) == CRYPTO_ALG_TYPE_AEAD) {
			aead_req   = aead_request_cast(dma_ch->req);
			reqctx     = aead_request_ctx(aead_req);
			ivsize     = crypto_aead_ivsize(crypto_aead_reqtfm(aead_req));
			iv         = aead_req->iv;
			if (reqctx->ccm_type) {
				ccm_reqctx = (struct na51055_crypto_ccm_reqctx *)reqctx;
				req_src_sg = ccm_reqctx->sg_src;
				req_dst_sg = ccm_reqctx->sg_dst;
			}
			else {
				gcm_reqctx = (struct na51055_crypto_gcm_reqctx *)reqctx;
				req_src_sg = gcm_reqctx->sg_src;
				req_dst_sg = gcm_reqctx->sg_dst;
			}
		}
		else {
			ablk_req   = ablkcipher_request_cast(dma_ch->req);
			reqctx     = ablkcipher_request_ctx(ablk_req);
			ivsize     = crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(ablk_req));
			iv         = ablk_req->info;
			req_src_sg = ablk_req->src;
			req_dst_sg = ablk_req->dst;
		}
		align_chk = (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM || reqctx->opmode == NA51055_CRYPTO_OPMODE_CTR || reqctx->opmode == NA51055_CRYPTO_OPMODE_CFB) ? 0 : 1;

		dev_dbg(dev->dev, "DMA#%d      => transfer done\n", i);
		dev_dbg(dev->dev, "DMA#%d CIV  => %08x %08x %08x %08x\n", i, desc->cv[0], desc->cv[1], desc->cv[2], desc->cv[3]);
		dev_dbg(dev->dev, "DMA#%d S0   => %08x %08x %08x %08x\n", i, desc->s0[0], desc->s0[1], desc->s0[2], desc->s0[3]);
		dev_dbg(dev->dev, "DMA#%d GHASH=> %08x %08x %08x %08x\n", i, desc->ghash[0], desc->ghash[1], desc->ghash[2], desc->ghash[3]);

		/* check processed source data length if use source temp buffer */
		if (!dma_ch->sg_src_work && (dma_ch->src_copied == 2)) {
			dma_unmap_sg(dev->dev, dma_ch->sg_src, dma_ch->sg_src_nents, DMA_TO_DEVICE);    ///< cache nothing to do
			dma_ch->sg_src       = NULL;
			dma_ch->sg_src_nents = 0;
			dma_ch->src_copied   = 0;

			/* copy remain data to temp buffer */
			if ((dma_ch->src_total < dma_ch->req_nbytes) && req_src_sg) {
				if (dma_ch->req_nbytes >= (dma_ch->src_total + tbuf_size)) {
				    dev_dbg(dev->dev, "crypto dma#%d copy %d bytes from original offset %d to temp buffer\n", i, tbuf_size, dma_ch->src_total);
					scatterwalk_map_and_copy(dma_ch->tbuf_src, req_src_sg, dma_ch->src_total, tbuf_size, 0);
					sg_init_one(&dma_ch->sg_src_cpy, dma_ch->tbuf_src, tbuf_size);
				}
				else {
				    dev_dbg(dev->dev, "crypto dma#%d copy %d bytes from original offset %d to temp buffer\n", i, (dma_ch->req_nbytes - dma_ch->src_total), dma_ch->src_total);
					scatterwalk_map_and_copy(dma_ch->tbuf_src, req_src_sg, dma_ch->src_total, (dma_ch->req_nbytes - dma_ch->src_total), 0);
					sg_init_one(&dma_ch->sg_src_cpy, dma_ch->tbuf_src, (dma_ch->req_nbytes - dma_ch->src_total));
				}

				/* source dma mapping and cache clean */
				dma_ch->sg_src_nents = dma_map_sg(dev->dev, &dma_ch->sg_src_cpy, sg_nents(&dma_ch->sg_src_cpy), DMA_TO_DEVICE);	  ///< direction => memory to device, cache clean, DMA input
				if (!dma_ch->sg_src_nents) {
					dev_err(dev->dev, "crypto dma#%d source scatterlist dma map error\n", i);
				}
				else {
					dma_ch->src_copied  = 2;
					dma_ch->sg_src      = &dma_ch->sg_src_cpy;
					dma_ch->sg_src_work = dma_ch->sg_src;
					dma_ch->sg_src_len  = (dma_ch->sg_src_work)->length;
					dma_ch->sg_src_ofs  = 0;
				}
			}
		}

		/* check process destination data length if use destination temp buffer */
		if (!dma_ch->sg_dst_work && (dma_ch->dst_copied == 2)) {
			dma_unmap_sg(dev->dev, dma_ch->sg_dst, dma_ch->sg_dst_nents, DMA_FROM_DEVICE);  ///< cache invalidate
			dma_ch->sg_dst       = NULL;
			dma_ch->sg_dst_nents = 0;
			dma_ch->dst_copied   = 0;

			/* copy output data to destination buffer */
			if ((dma_ch->dst_total >= dma_ch->sg_dst_cpy.length) && req_dst_sg) {
				dev_dbg(dev->dev, "crypto dma#%d copy %d bytes back to original buffer offset %d\n", i, dma_ch->sg_dst_cpy.length, (dma_ch->dst_total - dma_ch->sg_dst_cpy.length));
				scatterwalk_map_and_copy(sg_virt(&dma_ch->sg_dst_cpy), req_dst_sg, (dma_ch->dst_total - dma_ch->sg_dst_cpy.length), dma_ch->sg_dst_cpy.length, 1);
			}
			else {
				dev_err(dev->dev, "crypto dma#%d destination buffer config incorrect\n", i);
				goto completed;
			}

			/* prepare next temp destination buffer */
			if (dma_ch->dst_total < dma_ch->req_nbytes) {
				if (dma_ch->req_nbytes >= (dma_ch->dst_total + tbuf_size))
					sg_init_one(&dma_ch->sg_dst_cpy, dma_ch->tbuf_dst, tbuf_size);
				else
					sg_init_one(&dma_ch->sg_dst_cpy, dma_ch->tbuf_dst, (dma_ch->req_nbytes - dma_ch->dst_total));

				/* destination dma mapping and cache invalidate */
				dma_ch->sg_dst_nents = dma_map_sg(dev->dev, &dma_ch->sg_dst_cpy, sg_nents(&dma_ch->sg_dst_cpy), DMA_FROM_DEVICE);    ///< cache invalidate, DMA output
				if (!dma_ch->sg_dst_nents) {
					dev_err(dev->dev, "crypto dma#%d destination scatterlist dma map error\n", i);
					goto completed;
				}

				dma_ch->dst_copied  = 2;
				dma_ch->sg_dst      = &dma_ch->sg_dst_cpy;
				dma_ch->sg_dst_work = dma_ch->sg_dst;
				dma_ch->sg_dst_len  = (dma_ch->sg_dst_work)->length;
				dma_ch->sg_dst_ofs  = 0;
			}
		}

#ifdef NA51055_CRYPTO_DMA_DESC_CV_CHECK
		/* check engine dma update cv ready */
		if (reqctx->opmode != NA51055_CRYPTO_OPMODE_ECB) {
			switch (reqctx->mode) {
			case NA51055_CRYPTO_MODE_DES:
			case NA51055_CRYPTO_MODE_3DES:
			    for (j=0; j<100; j++) {
			        if (desc->cv[0] || desc->cv[1])
                        break;
			        udelay(1);
			    }
			    if (!desc->cv[0] && !desc->cv[1]) {
			        dev_err(dev->dev, "DMA#%d CV not ready => %08x %08x\n", i, desc->cv[0], desc->cv[1]);
			        goto completed;
			    }
				break;
			case NA51055_CRYPTO_MODE_AES_128:
			case NA51055_CRYPTO_MODE_AES_256:
			    for (j=0; j<100; j++) {
			        if (desc->cv[0] || desc->cv[1] || desc->cv[2] || desc->cv[3])
			            break;
			        udelay(1);
			    }
			    if (!desc->cv[0] && !desc->cv[1] && !desc->cv[2] && !desc->cv[3]) {
			        dev_err(dev->dev, "DMA#%d CV not ready => %08x %08x %08x %08x\n", i, desc->cv[0], desc->cv[1], desc->cv[2], desc->cv[3]);
			        goto completed;
			    }
				break;
			default:
				break;
			}
		}
#endif

		/* check request complete */
		if (dma_ch->sg_src_work && dma_ch->sg_dst_work) {
			if(reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM)
				goto completed;

			block_num = 0;
			for (j=0; j<NA51055_CRYPTO_MAX_DMA_BLOCK_NUM; j++) {
				if (dma_ch->src_total >= dma_ch->req_nbytes || dma_ch->dst_total >= dma_ch->req_nbytes)
					break;
				if (!dma_ch->sg_src_work || !dma_ch->sg_dst_work)
					break;

				/* caculate dma transfer length */
				dma_len = min(dma_ch->sg_src_len, dma_ch->sg_dst_len);
				if ((dma_ch->src_total + dma_len) > dma_ch->req_nbytes) {
					dma_len = dma_ch->req_nbytes - dma_ch->src_total;
				}
				if(!dma_len || (align_chk && !IS_ALIGNED(dma_len, ctx->block_size))) {
					dev_err(dev->dev, "crypto dma#%d request invalid dma length(%u)!\n", i, dma_len);
					goto completed;
				}

				/* DMA block config setting */
				desc->block[j].src_addr  = ALIGN_DN((sg_dma_address(dma_ch->sg_src_work)+dma_ch->sg_src_ofs), 4);
				desc->block[j].dst_addr  = ALIGN_DN((sg_dma_address(dma_ch->sg_dst_work)+dma_ch->sg_dst_ofs), 4);
				desc->block[j].length    = dma_len;
				desc->block[j].block_cfg = 0;
				block_num++;

				dma_ch->src_total  += dma_len;
				dma_ch->sg_src_len -= dma_len;
				if (dma_ch->sg_src_len == 0) {
					dma_ch->sg_src_work = sg_next(dma_ch->sg_src_work);
					dma_ch->sg_src_len  = (!dma_ch->sg_src_work) ? 0 : (dma_ch->sg_src_work)->length;
					dma_ch->sg_src_ofs  = 0;
				}
				else {
					dma_ch->sg_src_ofs += dma_len;
				}

				dma_ch->dst_total  += dma_len;
				dma_ch->sg_dst_len -= dma_len;
				if (dma_ch->sg_dst_len == 0) {
					dma_ch->sg_dst_work = sg_next(dma_ch->sg_dst_work);
					dma_ch->sg_dst_len = (!dma_ch->sg_dst_work) ? 0 : (dma_ch->sg_dst_work)->length;
					dma_ch->sg_dst_ofs = 0;
				}
				else {
					dma_ch->sg_dst_ofs += dma_len;
				}
			}

			if (block_num) {
				/* update IV */
				if (reqctx->opmode != NA51055_CRYPTO_OPMODE_ECB) {
					switch (reqctx->mode) {
					case NA51055_CRYPTO_MODE_DES:
					case NA51055_CRYPTO_MODE_3DES:
						desc->iv[0] = desc->cv[0];
						desc->iv[1] = desc->cv[1];
						dev_dbg(dev->dev, "DMA#%d NIV  => %08x %08x\n", i, desc->iv[0], desc->iv[1]);
						break;
					case NA51055_CRYPTO_MODE_AES_128:
					case NA51055_CRYPTO_MODE_AES_256:
						desc->iv[0] = desc->cv[0];
						desc->iv[1] = desc->cv[1];
						desc->iv[2] = desc->cv[2];
						desc->iv[3] = desc->cv[3];
						dev_dbg(dev->dev, "DMA#%d NIV  => %08x %08x %08x %08x\n", i, desc->iv[0], desc->iv[1], desc->iv[2], desc->iv[3]);
						break;
					default:
						break;
					}
				}

				/* set the last block */
				desc->block[block_num-1].block_cfg |= 0x1;
				dma_enb |= (0x1<<i);

#ifdef NA51055_CRYPTO_DMA_DESC_CV_CHECK
                /* clear current cv */
                desc->cv[0] = 0;
                desc->cv[1] = 0;
                desc->cv[2] = 0;
                desc->cv[3] = 0;
#endif

				/* dummy read to ensure data in DDR */
                wmb();
				reg_value = desc->block[block_num-1].block_cfg;
				rmb();

				/* set dma channel state to busy */
				dma_ch->state = NA51055_CRYPTO_STATE_BUSY;

				dev_dbg(dev->dev, "DMA#%d      => trigger next transfer!\n", i);
			}
			else {
				goto completed;
			}
		}
		else {
completed:
			if (dma_ch->sg_src && dma_ch->sg_src_nents) {
				dma_unmap_sg(dev->dev, dma_ch->sg_src, dma_ch->sg_src_nents, DMA_TO_DEVICE);    ///< cache nothing to do
				dma_ch->sg_src       = NULL;
				dma_ch->sg_src_nents = 0;
			}
			if (dma_ch->sg_dst && dma_ch->sg_dst_nents) {
				dma_unmap_sg(dev->dev, dma_ch->sg_dst, dma_ch->sg_dst_nents, DMA_FROM_DEVICE);  ///< cache invalidate
				dma_ch->sg_dst       = NULL;
				dma_ch->sg_dst_nents = 0;
			}
			if (dma_ch->sg_ass && dma_ch->sg_ass_nents) {
				dma_unmap_sg(dev->dev, dma_ch->sg_ass, dma_ch->sg_ass_nents, DMA_TO_DEVICE);    ///< cache nothing to do
				dma_ch->sg_ass       = NULL;
				dma_ch->sg_ass_nents = 0;
			}
			if (dma_ch->dst_copied == 1) {
				dev_dbg(dev->dev, "crypto dma#%d copy %d bytes back to original buffer\n", i, dma_ch->dst_total);
				scatterwalk_map_and_copy(sg_virt(&dma_ch->sg_dst_cpy), req_dst_sg, 0, dma_ch->dst_total, 1);
				free_pages((unsigned long)sg_virt(&dma_ch->sg_dst_cpy), get_order(dma_ch->sg_dst_cpy.length));
				dma_ch->dst_copied = 0;
				if (dma_ch->sg_same && dma_ch->src_copied)
					dma_ch->src_copied = 0;
			}
			else {
				dma_ch->dst_copied = 0;
			}
			if (dma_ch->src_copied == 1) {
				free_pages((unsigned long)sg_virt(&dma_ch->sg_src_cpy), get_order(dma_ch->sg_src_cpy.length));
				dma_ch->src_copied = 0;
			}
			else {
				dma_ch->src_copied = 0;
			}
			if (dma_ch->ass_copied == 1) {
				free_pages((unsigned long)sg_virt(&dma_ch->sg_ass_cpy), get_order(dma_ch->sg_ass_cpy.length));
				dma_ch->ass_copied = 0;
			}
			else {
				dma_ch->ass_copied = 0;
			}

			/* check all request data completed */
			err = (dma_ch->dst_total == dma_ch->req_nbytes) ? 0 : -EINVAL;

			/* request complete callback */
			if (reqctx->ccm_type)
				na51055_crypto_aes_ccm_complete(dev, i, err);
			else if (reqctx->opmode == NA51055_CRYPTO_OPMODE_GCM)
				na51055_crypto_aes_gcm_complete(dev, i, err);
			else {
				/* next IV copy back */
				if (iv && ivsize) {
					memcpy((void *)iv, (void *)desc->cv, ivsize);
				}
				na51055_crypto_complete(dev, i, err);
			}
		}
	}

	/* trigger DMA channel transfer */
	if (dma_enb) {
		/* clear interrupt status */
		na51055_crypto_write(dev, NA51055_CRYPTO_INT_STS_REG, (dma_enb<<4));

		/* set DMA channel enable */
		reg_value  = na51055_crypto_read(dev, NA51055_CRYPTO_CTRL_REG);
		reg_value &= ~(0xf<<4);
		reg_value |= (dma_enb<<4);
		na51055_crypto_write(dev, NA51055_CRYPTO_CTRL_REG, reg_value);

		/* start timeout timer */
		for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
			if ((dma_enb & (0x1<<i)) == 0)
				continue;
			mod_timer(&dev->dma_ch[i].timer, jiffies+msecs_to_jiffies(NA51055_CRYPTO_DEFAULT_TIMEOUT));
		}
	}

	/* trigger to do next crypto request in queue */
	tasklet_schedule(&dev->queue_tasklet);

	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
static void na51055_crypto_hash_tasklet(unsigned long data)
{
	struct na51055_crypto_dev        *dev = (struct na51055_crypto_dev *)data;
	struct crypto_aead               *tfm;
	struct crypto_async_request      *async_req;
	struct aead_request              *aead_req;
	struct na51055_crypto_ctx        *ctx;
	struct na51055_crypto_gcm_reqctx *reqctx;
	int                              err;
	unsigned long                    flags;

	spin_lock_irqsave(&dev->queue_lock, flags);

	async_req = crypto_dequeue_request(&dev->hash_queue);

	spin_unlock_irqrestore(&dev->queue_lock, flags);

	if (!async_req)
		return;

	aead_req = aead_request_cast(async_req);
	tfm      = crypto_aead_reqtfm(aead_req);
	ctx      = crypto_aead_ctx(tfm);
	reqctx   = aead_request_ctx(aead_req);

	err = na51055_crypto_aes_gcm_hash(aead_req, ctx);
	if (err == -EINPROGRESS || err == -EBUSY)
		return;

    /* ghash complete */
	if (!err) {
		if (reqctx->type == NA51055_CRYPTO_TYPE_ENCRYPT) {
			crypto_xor((u8 *)reqctx->auth_tag, (u8 *)reqctx->iauth_tag, crypto_aead_authsize(tfm));
			na51055_crypto_aes_gcm_copy_hash(aead_req);
			aead_request_complete(aead_req, err);
		}
		else {
			/* gcm decryption, do ghash first then issue hardware to do gcm decryption */
			na51055_crypto_handle_req(dev, async_req, 0);
		}
	}
	else {
		aead_request_complete(aead_req, err);
	}
}
#endif

static void na51055_crypto_payload_tasklet(unsigned long data)
{
	struct na51055_crypto_dev        *dev = (struct na51055_crypto_dev *)data;
	struct crypto_aead               *tfm;
	struct crypto_async_request      *async_req;
	struct aead_request              *aead_req;
	struct na51055_crypto_ctx        *ctx;
	struct na51055_crypto_ccm_reqctx *reqctx;
	u8                               *payload_pages;
	u32                              payload_size;
	u32		                         cryptlen;
	u32                              assoclen;
	u32                              authlen;
	u32                              offset;
	int                              err;
	unsigned long                    flags;

	spin_lock_irqsave(&dev->queue_lock, flags);

	async_req = crypto_dequeue_request(&dev->payload_queue);

	spin_unlock_irqrestore(&dev->queue_lock, flags);

	if (!async_req)
		return;

	aead_req = aead_request_cast(async_req);
	tfm      = crypto_aead_reqtfm(aead_req);
	ctx      = crypto_aead_ctx(tfm);
	reqctx   = aead_request_ctx(aead_req);
	cryptlen = reqctx->cryptlen;
	assoclen = aead_req->assoclen;
	authlen  = crypto_aead_authsize(tfm);

	/* allocate buffer for create ccm B payload */
	payload_size = 16;								///< first 16 bytes for payload B0
	if (assoclen)
		payload_size += ((assoclen <= 65280) ? ALIGN_UP(2+assoclen, 16) : ALIGN_UP(6+assoclen, 16));
	if (cryptlen)
		payload_size += ALIGN_UP(cryptlen, 16);
	payload_pages = (u8 *)__get_free_pages(GFP_ATOMIC, get_order(payload_size));
	if (!payload_pages) {
		dev_err(dev->dev, "crypto no free memory to allocte ccm payload buffer!\n");
		err = -ENOMEM;
		goto error;
	}
	sg_init_one(&reqctx->sg_payload, payload_pages, payload_size);
	reqctx->payload_size = payload_size;

	/* generate B0 */
	err = __ccm_generate_b0(aead_req->iv, assoclen, authlen, cryptlen, &payload_pages[0]);
	if (err) {
		dev_err(dev->dev, "crypto ccm generate B0 payload failed!\n");
		err = -EINVAL;
		goto error;
	}
	offset = 16;

	/* generate B1 ... Br */
	if (assoclen) {
		if (assoclen <= 65280) {
			*(__be16 *)&payload_pages[offset] = cpu_to_be16(assoclen);
			scatterwalk_map_and_copy(&payload_pages[offset+2], aead_req->src, 0, assoclen, 0);
			offset += (2 + assoclen);
		}
		else {
			*(__be16 *)&payload_pages[offset]   = cpu_to_be16(0xfffe);
			*(__be32 *)&payload_pages[offset+2] = cpu_to_be32(assoclen);
			scatterwalk_map_and_copy(&payload_pages[offset+6], aead_req->src, 0, assoclen, 0);
			offset += (6 + assoclen);
		}

		/* check and see if there's leftover data that wasn't
		 * enough to fill a block.
		 */
		if (!IS_ALIGNED(offset, 16)) {
			memset(&payload_pages[offset], 0, ALIGN_UP(offset, 16)-offset);
			offset = ALIGN_UP(offset, 16);
		}
	}

	if (cryptlen) {
		scatterwalk_map_and_copy(&payload_pages[offset], reqctx->sg_crypt, 0, cryptlen, 0);
		offset += cryptlen;

		/* check and see if there's leftover data that wasn't
		 * enough to fill a block.
		 */
		if (!IS_ALIGNED(offset, 16)) {
			memset(&payload_pages[offset], 0, ALIGN_UP(offset, 16)-offset);
			offset = ALIGN_UP(offset, 16);
		}
	}

	na51055_crypto_handle_req(dev, async_req, 0);

error:
	if (err) {
		if (payload_pages) {
			free_pages((unsigned long)payload_pages, get_order(payload_size));
			reqctx->payload_size = 0;
		}
		aead_request_complete(aead_req, err);
	}
}

static int na51055_crypto_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 1);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_ECB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 0);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_ECB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 1);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CBC;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 0);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CBC;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_cfb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 1);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_cfb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 0);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_ofb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 1);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_OFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_ofb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of AES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 0);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_OFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_ctr_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 1);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CTR;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_ctr_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_do_fallback(req, 0);
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CTR;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_gcm_encrypt(struct aead_request *req)
{
	struct crypto_aead               *tfm    = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *ctx    = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx = aead_request_ctx(req);

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_aead_do_fallback(req, 1);
	}

	sg_init_table(reqctx->src, 2);
	sg_init_table(reqctx->dst, 2);

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_GCM;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;
	reqctx->cryptlen = req->cryptlen;
	reqctx->sg_src   = scatterwalk_ffwd(reqctx->src, req->src, req->assoclen);
	reqctx->sg_dst   = (req->src == req->dst) ? reqctx->sg_src : scatterwalk_ffwd(reqctx->dst, req->dst, req->assoclen);

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	reqctx->ghash_ctx.src      = reqctx->sg_dst;
	reqctx->ghash_ctx.cryptlen = req->cryptlen;
	reqctx->ghash_ctx.complete = na51055_crypto_aes_gcm_encrypt_hash_done;
#endif

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_aes_gcm_decrypt(struct aead_request *req)
{
	struct crypto_aead               *tfm     = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *ctx     = crypto_aead_ctx(tfm);
	struct na51055_crypto_gcm_reqctx *reqctx  = aead_request_ctx(req);
	unsigned int                     authsize = crypto_aead_authsize(tfm);

	if (req->cryptlen < authsize)
		return -EINVAL;

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_aead_do_fallback(req, 0);
	}

	sg_init_table(reqctx->src, 2);
	sg_init_table(reqctx->dst, 2);

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_GCM;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;
	reqctx->cryptlen = req->cryptlen - authsize;
	reqctx->sg_src   = scatterwalk_ffwd(reqctx->src, req->src, req->assoclen);
	reqctx->sg_dst   = (req->src == req->dst) ? reqctx->sg_src : scatterwalk_ffwd(reqctx->dst, req->dst, req->assoclen);

#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	reqctx->ghash_ctx.src      = reqctx->sg_src;
	reqctx->ghash_ctx.cryptlen = req->cryptlen - authsize;
	reqctx->ghash_ctx.complete = na51055_crypto_aes_gcm_decrypt_hash_done;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 1);	///< 1 means to issue to do software ghash first
#else
	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
#endif
}

static int na51055_crypto_aes_ccm_encrypt(struct aead_request *req)
{
	struct crypto_aead               *tfm    = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *ctx    = crypto_aead_ctx(tfm);
	struct na51055_crypto_ccm_reqctx *reqctx = aead_request_ctx(req);
	int                              err;

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_aead_do_fallback(req, 1);
	}

	err = __ccm_check_iv(req->iv);
	if (err)
		return err;

	sg_init_table(reqctx->src, 2);
	sg_init_table(reqctx->dst, 2);

	/* Note: rfc 3610 and NIST 800-38C require counter of
	 * zero to encrypt auth tag.
	 */
	memset(req->iv + 15 - req->iv[0], 0, req->iv[0] + 1);

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type         = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode         = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode       = NA51055_CRYPTO_OPMODE_CBC;
	reqctx->key_src      = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0       = 0;
	reqctx->ccm_type     = NA51055_CRYPTO_CCM_TYPE_ENCRYPT;
	reqctx->sg_src       = scatterwalk_ffwd(reqctx->src, req->src, req->assoclen);
	reqctx->sg_dst       = (req->src == req->dst) ? reqctx->sg_src : scatterwalk_ffwd(reqctx->dst, req->dst, req->assoclen);
	reqctx->sg_crypt     = reqctx->sg_src;
	reqctx->cryptlen     = req->cryptlen;
	reqctx->payload_size = 0;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 2);
}

static int na51055_crypto_aes_ccm_decrypt(struct aead_request *req)
{
	struct crypto_aead               *tfm     = crypto_aead_reqtfm(req);
	struct na51055_crypto_ctx        *ctx     = crypto_aead_ctx(tfm);
	struct na51055_crypto_ccm_reqctx *reqctx  = aead_request_ctx(req);
	unsigned int                     authsize = crypto_aead_authsize(tfm);
	int                              err;

	if (unlikely(ctx->keylen != AES_KEYSIZE_128 && ctx->keylen != AES_KEYSIZE_256)) {
		return na51055_crypto_aes_aead_do_fallback(req, 0);
	}

	if (req->cryptlen < authsize)
		return -EINVAL;

	err = __ccm_check_iv(req->iv);
	if (err)
		return err;

	sg_init_table(reqctx->src, 2);
	sg_init_table(reqctx->dst, 2);

	/* Note: rfc 3610 and NIST 800-38C require counter of
	 * zero to encrypt auth tag.
	 */
	memset(req->iv + 15 - req->iv[0], 0, req->iv[0] + 1);

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = AES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type         = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode         = (ctx->keylen == AES_KEYSIZE_256) ? NA51055_CRYPTO_MODE_AES_256 : NA51055_CRYPTO_MODE_AES_128;
	reqctx->opmode       = NA51055_CRYPTO_OPMODE_CTR;
	reqctx->key_src      = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0       = 1;
	reqctx->ccm_type     = NA51055_CRYPTO_CCM_TYPE_DECRYPT;
	reqctx->sg_src       = scatterwalk_ffwd(reqctx->src, req->src, req->assoclen);
	reqctx->sg_dst       = (req->src == req->dst) ? reqctx->sg_src : scatterwalk_ffwd(reqctx->dst, req->dst, req->assoclen);
	reqctx->sg_crypt     = reqctx->sg_dst;
	reqctx->cryptlen     = req->cryptlen - authsize;
	reqctx->payload_size = 0;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_setkey(struct crypto_ablkcipher *tfm, const u8 *key, unsigned int keylen)
{
	struct na51055_crypto_ctx *ctx = crypto_ablkcipher_ctx(tfm);

	if (unlikely(keylen != DES_KEY_SIZE && keylen != DES3_EDE_KEY_SIZE)) {
		crypto_ablkcipher_set_flags(tfm, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	/* check for weak keys */
	if (crypto_ablkcipher_get_flags(tfm) & CRYPTO_TFM_REQ_WEAK_KEY)
		return -EINVAL;

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int na51055_crypto_des_ecb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_ECB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_ecb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_ECB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_cbc_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CBC;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_cbc_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CBC;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_cfb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_cfb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_ofb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_OFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_ofb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE)) {
		pr_err("request size is not exact amount of DES blocks(nbytes:%d)\n", req->nbytes);
		return -EINVAL;
	}

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_OFB;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_ctr_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_ENCRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CTR;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static int na51055_crypto_des_ctr_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher     *tfm    = crypto_ablkcipher_reqtfm(req);
	struct na51055_crypto_ctx    *ctx    = crypto_ablkcipher_ctx(tfm);
	struct na51055_crypto_reqctx *reqctx = ablkcipher_request_ctx(req);

	if (unlikely(ctx->keylen != DES_KEY_SIZE && ctx->keylen != DES3_EDE_KEY_SIZE)) {
		return -EINVAL;
	}

	/* setup device */
	ctx->dev        = na51055_cdev;
	ctx->block_size = DES_BLOCK_SIZE;

	/* setup operation */
	reqctx->type     = NA51055_CRYPTO_TYPE_DECRYPT;
	reqctx->mode     = (ctx->keylen == DES_KEY_SIZE) ? NA51055_CRYPTO_MODE_DES : NA51055_CRYPTO_MODE_3DES;
	reqctx->opmode   = NA51055_CRYPTO_OPMODE_CTR;
	reqctx->key_src  = NA51055_CRYPTO_KEY_SRC_DESC0;
	reqctx->get_s0   = 0;
	reqctx->ccm_type = NA51055_CRYPTO_CCM_TYPE_NONE;

	return na51055_crypto_handle_req(ctx->dev, &req->base, 0);
}

static irqreturn_t na51055_crypto_irq(int irq, void *dev_id)
{
	int i, dma_done = 0;
	struct na51055_crypto_dev *dev = (struct na51055_crypto_dev *)dev_id;
	u32                       status;

	spin_lock(&dev->lock);

	/* read DMA status */
	status = na51055_crypto_read(dev, NA51055_CRYPTO_INT_STS_REG);

	/* clear DMA status, not clear PIO status */
	na51055_crypto_write(dev, NA51055_CRYPTO_INT_STS_REG, status&(~0x1));

	/* DMA channel transfer done */
	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		if (status & (0x1<<(i+4))) {
			del_timer(&dev->dma_ch[i].timer);
			dev->dma_ch[i].state = NA51055_CRYPTO_STATE_DONE;
			dma_done++;
		}
	}

	if (dma_done) {
		tasklet_schedule(&dev->done_tasklet);
	}

	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

static struct crypto_alg na51055_crypto_algs[] = {
	{
		.cra_name			= "ecb(aes)",
		.cra_driver_name	= "na51055-ecb-aes",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.setkey			= na51055_crypto_aes_setkey,
			.encrypt		= na51055_crypto_aes_ecb_encrypt,
			.decrypt		= na51055_crypto_aes_ecb_decrypt,
		}
	},
	{
		.cra_name			= "cbc(aes)",
		.cra_driver_name	= "na51055-cbc-aes",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize			= AES_BLOCK_SIZE,
			.setkey			= na51055_crypto_aes_setkey,
			.encrypt		= na51055_crypto_aes_cbc_encrypt,
			.decrypt		= na51055_crypto_aes_cbc_decrypt,
		}
	},
	{
		.cra_name			= "cfb(aes)",
		.cra_driver_name	= "na51055-cfb-aes",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize			= AES_BLOCK_SIZE,
			.setkey			= na51055_crypto_aes_setkey,
			.encrypt		= na51055_crypto_aes_cfb_encrypt,
			.decrypt		= na51055_crypto_aes_cfb_decrypt,
		}
	},
	{
		.cra_name			= "ofb(aes)",
		.cra_driver_name	= "na51055-ofb-aes",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize			= AES_BLOCK_SIZE,
			.setkey			= na51055_crypto_aes_setkey,
			.encrypt		= na51055_crypto_aes_ofb_encrypt,
			.decrypt		= na51055_crypto_aes_ofb_decrypt,
		}
	},
	{
		.cra_name			= "ctr(aes)",
		.cra_driver_name	= "na51055-ctr-aes",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize			= AES_BLOCK_SIZE,
			.setkey			= na51055_crypto_aes_setkey,
			.encrypt		= na51055_crypto_aes_ctr_encrypt,
			.decrypt		= na51055_crypto_aes_ctr_decrypt,
		}
	},
	{
		.cra_name			= "ecb(des)",
		.cra_driver_name	= "na51055-ecb-des",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ecb_encrypt,
			.decrypt		= na51055_crypto_des_ecb_decrypt,
		}
	},
	{
		.cra_name			= "cbc(des)",
		.cra_driver_name	= "na51055-cbc-des",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_cbc_encrypt,
			.decrypt		= na51055_crypto_des_cbc_decrypt,
		}
	},
	{
		.cra_name			= "cfb(des)",
		.cra_driver_name	= "na51055-cfb-des",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_cfb_encrypt,
			.decrypt		= na51055_crypto_des_cfb_decrypt,
		}
	},
	{
		.cra_name			= "ofb(des)",
		.cra_driver_name	= "na51055-ofb-des",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ofb_encrypt,
			.decrypt		= na51055_crypto_des_ofb_decrypt,
		}
	},
	{
		.cra_name			= "ctr(des)",
		.cra_driver_name	= "na51055-ctr-des",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ctr_encrypt,
			.decrypt		= na51055_crypto_des_ctr_decrypt,
		}
	},
	{
		.cra_name			= "ecb(des3_ede)",
		.cra_driver_name	= "na51055-ecb-des3",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ecb_encrypt,
			.decrypt		= na51055_crypto_des_ecb_decrypt,
		}
	},
	{
		.cra_name			= "cbc(des3_ede)",
		.cra_driver_name	= "na51055-cbc-des3",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_cbc_encrypt,
			.decrypt		= na51055_crypto_des_cbc_decrypt,
		}
	},
	{
		.cra_name			= "cfb(des3_ede)",
		.cra_driver_name	= "na51055-cfb-des3",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_cfb_encrypt,
			.decrypt		= na51055_crypto_des_cfb_decrypt,
		}
	},
	{
		.cra_name			= "ofb(des3_ede)",
		.cra_driver_name	= "na51055-ofb-des3",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ofb_encrypt,
			.decrypt		= na51055_crypto_des_ofb_decrypt,
		}
	},
	{
		.cra_name			= "ctr(des3_ede)",
		.cra_driver_name	= "na51055-ctr-des3",
		.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
		.cra_flags			= CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
		.cra_alignmask		= 0x3,
		.cra_type			= &crypto_ablkcipher_type,
		.cra_module			= THIS_MODULE,
		.cra_init			= na51055_crypto_cra_init,
		.cra_exit			= na51055_crypto_cra_exit,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize			= DES_BLOCK_SIZE,
			.setkey			= na51055_crypto_des_setkey,
			.encrypt		= na51055_crypto_des_ctr_encrypt,
			.decrypt		= na51055_crypto_des_ctr_decrypt,
		}
	}
};

static struct aead_alg na51055_aead_algs[] = {
	{
		.base = {
			.cra_name			= "gcm(aes)",
			.cra_driver_name	= "na51055-gcm-aes",
			.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
			.cra_flags			= CRYPTO_ALG_TYPE_AEAD | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
			.cra_blocksize		= 1,
			.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
			.cra_alignmask		= 0x3,
			.cra_module			= THIS_MODULE,
		},
		.init        = na51055_crypto_cra_aes_gcm_init,
		.exit        = na51055_crypto_cra_aes_gcm_exit,
		.ivsize		 = 12,
		.maxauthsize = AES_BLOCK_SIZE,
		.setkey		 = na51055_crypto_aes_gcm_setkey,
		.setauthsize = na51055_crypto_aes_gcm_setauthsize,
		.encrypt	 = na51055_crypto_aes_gcm_encrypt,
		.decrypt	 = na51055_crypto_aes_gcm_decrypt,
	},
	{
		.base = {
			.cra_name			= "ccm(aes)",
			.cra_driver_name	= "na51055-ccm-aes",
			.cra_priority		= NA51055_CRYPTO_ALG_PRIORITY,
			.cra_flags			= CRYPTO_ALG_TYPE_AEAD | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
			.cra_blocksize		= 1,
			.cra_ctxsize		= sizeof(struct na51055_crypto_ctx),
			.cra_alignmask		= 0x3,
			.cra_module			= THIS_MODULE,
		},
		.init        = na51055_crypto_cra_aes_ccm_init,
		.exit        = na51055_crypto_cra_aes_ccm_exit,
		.ivsize		 = AES_BLOCK_SIZE,
		.maxauthsize = AES_BLOCK_SIZE,
		.setkey		 = na51055_crypto_aes_ccm_setkey,
		.setauthsize = na51055_crypto_aes_ccm_setauthsize,
		.encrypt	 = na51055_crypto_aes_ccm_encrypt,
		.decrypt	 = na51055_crypto_aes_ccm_decrypt,
	}
};

static int na51055_crypto_dma_buf_alloc(struct na51055_crypto_dev *dev)
{
	int ret = 0;

	if(dev->dma_buf.vaddr) {	///< buffer have allocated
		goto exit;
	}

    dev->dma_buf.vaddr = dma_alloc_coherent(NULL, sizeof(struct na51055_crypto_dma_desc)*NA51055_CRYPTO_DMA_CH_MAX, &dev->dma_buf.paddr, GFP_DMA | GFP_KERNEL);

	if (!dev->dma_buf.vaddr) {
		ret = -ENOMEM;
		goto exit;
	}
	dev->dma_buf.size = sizeof(struct na51055_crypto_dma_desc)*NA51055_CRYPTO_DMA_CH_MAX;

	/* check DMA descriptor buffer address */
	if (((dev->dma_buf.paddr + dev->dma_buf.size) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
		dev_err(dev->dev, "crypto allocated dma descriptor address:0x%08x not support\n", dev->dma_buf.paddr);

		dma_free_coherent(NULL, dev->dma_buf.size, dev->dma_buf.vaddr, dev->dma_buf.paddr);

		dev->dma_buf.vaddr = NULL;
		dev->dma_buf.paddr = 0;
		dev->dma_buf.size  = 0;

		ret = -ENOMEM;
		goto exit;
	}

exit:
	return ret;
}

static void na51055_crypto_dma_buf_free(struct na51055_crypto_dev *dev)
{
	if (dev->dma_buf.vaddr) {
		dma_free_coherent(NULL, dev->dma_buf.size, dev->dma_buf.vaddr, dev->dma_buf.paddr);

		dev->dma_buf.vaddr = NULL;
		dev->dma_buf.paddr = 0;
		dev->dma_buf.size  = 0;
	}
}

static int na51055_crypto_probe(struct platform_device *pdev)
{
	int    ret, i;
	struct resource           *res;
	struct na51055_crypto_dev *dd;
	struct clk                *parent_clk;
	char                      *parent_clk_name;

	/* check tbuf allocator */
	if ((tbuf_alloc < 0) || (tbuf_alloc >= NA51055_CRYPTO_TBUF_ALLOC_MAX))
		tbuf_alloc = NA51055_CRYPTO_TBUF_ALLOC_NONE;

	/* check tbuf size */
	if (tbuf_size < 0)
	    tbuf_size = PAGE_SIZE;

	dd = na51055_cdev = devm_kzalloc(&pdev->dev, sizeof(struct na51055_crypto_dev), GFP_KERNEL);
	if (!dd) {
		dev_err(&pdev->dev, "unable to alloc device data struct\n");
		return -ENOMEM;
	}

	dd->dev = &pdev->dev;
	dd->irq = -1;

	platform_set_drvdata(pdev, dd);

	spin_lock_init(&dd->lock);
	spin_lock_init(&dd->pio_lock);
	spin_lock_init(&dd->queue_lock);

	crypto_init_queue(&dd->queue,         NA51055_CRYPTO_QUEUE_LENGTH);
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	crypto_init_queue(&dd->hash_queue,    NA51055_CRYPTO_QUEUE_LENGTH);
#endif
	crypto_init_queue(&dd->payload_queue, NA51055_CRYPTO_QUEUE_LENGTH);

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

	ret = devm_request_irq(&pdev->dev, dd->irq, na51055_crypto_irq, 0, dev_name(&pdev->dev), dd);
	if (ret) {
		dev_err(&pdev->dev, "unable to request IRQ\n");
		return ret;
	}

    /* get crypto clock node */
	dd->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(dd->clk)) {
		dev_err(&pdev->dev, "failed to find crypto hardware clock\n");
		return PTR_ERR(dd->clk);
	}

	/* get crypto master clock selection from device tree */
	if (mclk < 0) {
        if (of_property_read_u32(pdev->dev.of_node, "mclk", &mclk)) {
            mclk = NA51055_CRYPTO_MCLK_320MHZ;
        }
	}

    /* get parent clock node of crypto master clock */
	switch (mclk) {
	    case NA51055_CRYPTO_MCLK_240MHZ:
	        parent_clk_name = "fix240m";
	        break;
	    case NA51055_CRYPTO_MCLK_PLL9:
	        parent_clk_name = "pll9";
	        break;
	    case NA51055_CRYPTO_MCLK_320MHZ:
	    default:
	        parent_clk_name = "pllf320";
	        break;
	}
	parent_clk = clk_get(NULL, parent_clk_name);
	if (IS_ERR(parent_clk)) {
		dev_err(&pdev->dev, "failed to find crypto parent clock\n");
		parent_clk = NULL;
	}

	/* set crypto clock to specify parent */
	if (parent_clk) {
	    ret = clk_set_parent(dd->clk, parent_clk);
	    if (ret < 0)
	        dev_err(&pdev->dev, "fail to switch crypto hardware clock\n");

	    /* release parent clock */
	    clk_put(parent_clk);
	}

	/* allocate DMA descriptor memory */
	ret = na51055_crypto_dma_buf_alloc(dd);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to alloc buffer for dma descriptor\n");
		return ret;
	}

	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		dd->dma_ch[i].idx        = i;
		dd->dma_ch[i].desc_paddr = ((u32)dd->dma_buf.paddr)+(sizeof(struct na51055_crypto_dma_desc)*i);
		dd->dma_ch[i].desc       = (struct na51055_crypto_dma_desc *)(((u32)dd->dma_buf.vaddr)+(sizeof(struct na51055_crypto_dma_desc)*i));
		dd->dma_ch[i].tbuf_src   = NULL;
		dd->dma_ch[i].tbuf_dst   = NULL;
		dd->dma_ch[i].tbuf_ass   = NULL;

		/* allocate tbuf */
		if (tbuf_size) {
			switch (tbuf_alloc) {
				case NA51055_CRYPTO_TBUF_ALLOC_BUDDY:
					/* source tbuf */
					dd->dma_ch[i].tbuf_src = (void *)__get_free_pages(GFP_KERNEL, get_order(tbuf_size));
					if (!dd->dma_ch[i].tbuf_src) {
						dev_dbg(&pdev->dev, "unable to alloc source temp buffer from buddy\n");
					}
					else if ((((u32)page_to_phys(virt_to_page(dd->dma_ch[i].tbuf_src)) + tbuf_size) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
						dev_dbg(&pdev->dev, "alloc source temp buffer from buddy but dma not support buffer physical address\n");
						free_pages((unsigned long)dd->dma_ch[i].tbuf_src, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_src = NULL;
					}

					/* destination tbuf */
					dd->dma_ch[i].tbuf_dst = (void *)__get_free_pages(GFP_KERNEL, get_order(tbuf_size));
					if (!dd->dma_ch[i].tbuf_dst) {
						dev_dbg(&pdev->dev, "unable to alloc destination temp buffer from buddy\n");
					}
					else if ((((u32)page_to_phys(virt_to_page(dd->dma_ch[i].tbuf_dst)) + tbuf_size) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
						dev_dbg(&pdev->dev, "alloc destination temp buffer from buddy but dma not support buffer physical address\n");
						free_pages((unsigned long)dd->dma_ch[i].tbuf_dst, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_dst = NULL;
					}

					/* associated tbuf */
					dd->dma_ch[i].tbuf_ass = (void *)__get_free_pages(GFP_KERNEL, get_order(tbuf_size));
					if (!dd->dma_ch[i].tbuf_ass) {
						dev_dbg(&pdev->dev, "unable to alloc associated temp buffer from buddy\n");
					}
					else if ((((u32)page_to_phys(virt_to_page(dd->dma_ch[i].tbuf_ass)) + tbuf_size) & (~NA51055_CRYPTO_DMA_ADDR_MASK))) {
						dev_dbg(&pdev->dev, "alloc associated temp buffer from buddy but dma not support buffer physical address\n");
						free_pages((unsigned long)dd->dma_ch[i].tbuf_ass, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_ass = NULL;
					}
					break;
				case NA51055_CRYPTO_TBUF_ALLOC_NONE:
				default:
					break;
			}
		}

   		/* init timer for DMA channel timeout */
		timer_setup(&dd->dma_ch[i].timer, na51055_crypto_timeout_handler, 0);
	}

	/* init tasklet for device request */
	tasklet_init(&dd->queue_tasklet,   na51055_crypto_queue_tasklet,   (unsigned long)dd);
	tasklet_init(&dd->done_tasklet,    na51055_crypto_done_tasklet,    (unsigned long)dd);
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	tasklet_init(&dd->hash_tasklet,    na51055_crypto_hash_tasklet,    (unsigned long)dd);
#endif
	tasklet_init(&dd->payload_tasklet, na51055_crypto_payload_tasklet, (unsigned long)dd);

	/* enable crypto hardware clock */
	clk_prepare_enable(dd->clk);

	/* software reset crypto hardware */
	na51055_crypto_reset(dd);

	/* register crypto algorithm */
	ret = crypto_register_algs(na51055_crypto_algs, ARRAY_SIZE(na51055_crypto_algs));
	if (ret) {
		dev_err(&pdev->dev, "failed to register crypto algorithm\n");
		goto err;
	}

	/* register crypto aead algorithm */
	ret = crypto_register_aeads(na51055_aead_algs, ARRAY_SIZE(na51055_aead_algs));
	if (ret) {
		dev_err(&pdev->dev, "failed to register aead algorithm\n");
		goto err;
	}

	pr_info("nvt-crypto driver registered Version: %s (CLK: %luHz)\n", DRV_VERSION, clk_get_rate(dd->clk));

	return 0;

err:
	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		/* free timer */
		del_timer(&dd->dma_ch[i].timer);

		/* free tbuf */
		if (tbuf_size) {
			switch (tbuf_alloc) {
				case NA51055_CRYPTO_TBUF_ALLOC_BUDDY:
					if (dd->dma_ch[i].tbuf_src) {
						free_pages((unsigned long)dd->dma_ch[i].tbuf_src, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_src = NULL;
					}
					if (dd->dma_ch[i].tbuf_dst) {
						free_pages((unsigned long)dd->dma_ch[i].tbuf_dst, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_dst = NULL;
					}
					if (dd->dma_ch[i].tbuf_ass) {
						free_pages((unsigned long)dd->dma_ch[i].tbuf_ass, get_order(tbuf_size));
						dd->dma_ch[i].tbuf_ass = NULL;
					}
					break;
				case NA51055_CRYPTO_TBUF_ALLOC_NONE:
				default:
					break;
			}
		}
	}
	tasklet_kill(&dd->queue_tasklet);
	tasklet_kill(&dd->done_tasklet);
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	tasklet_kill(&dd->hash_tasklet);
#endif
	tasklet_kill(&dd->payload_tasklet);
	clk_disable_unprepare(dd->clk);
	na51055_crypto_dma_buf_free(dd);
	return ret;
}

static int na51055_crypto_remove(struct platform_device *pdev)
{
	int i;
	struct na51055_crypto_dev *dev = platform_get_drvdata(pdev);

	if (!dev)
		return -ENODEV;

	crypto_unregister_algs(na51055_crypto_algs, ARRAY_SIZE(na51055_crypto_algs));
	crypto_unregister_aeads(na51055_aead_algs, ARRAY_SIZE(na51055_aead_algs));

	tasklet_kill(&dev->queue_tasklet);
	tasklet_kill(&dev->done_tasklet);
#ifdef NA51055_CRYPTO_GCM_SOFTWARE_GHASH
	tasklet_kill(&dev->hash_tasklet);
#endif
	tasklet_kill(&dev->payload_tasklet);

	for (i=0; i<NA51055_CRYPTO_DMA_CH_MAX; i++) {
		/* free timer */
		del_timer_sync(&dev->dma_ch[i].timer);

		/* free tbuf */
		if (tbuf_size) {
			switch (tbuf_alloc) {
				case NA51055_CRYPTO_TBUF_ALLOC_BUDDY:
					if (dev->dma_ch[i].tbuf_src) {
						free_pages((unsigned long)dev->dma_ch[i].tbuf_src, get_order(tbuf_size));
						dev->dma_ch[i].tbuf_src = NULL;
					}
					if (dev->dma_ch[i].tbuf_dst) {
						free_pages((unsigned long)dev->dma_ch[i].tbuf_dst, get_order(tbuf_size));
						dev->dma_ch[i].tbuf_dst = NULL;
					}
					if (dev->dma_ch[i].tbuf_ass) {
						free_pages((unsigned long)dev->dma_ch[i].tbuf_ass, get_order(tbuf_size));
						dev->dma_ch[i].tbuf_ass = NULL;
					}
					break;
				case NA51055_CRYPTO_TBUF_ALLOC_NONE:
				default:
					break;
			}
		}
	}

	clk_disable_unprepare(dev->clk);
	na51055_crypto_dma_buf_free(dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id na51055_crypto_of_match[] = {
	{ .compatible = "nvt,nvt_crypto" },
	{},
};
MODULE_DEVICE_TABLE(of, na51055_crypto_of_match);
#else
#define na51055_crypto_of_match     NULL
#endif

static struct platform_driver na51055_crypto_driver = {
	.probe	= na51055_crypto_probe,
	.remove	= na51055_crypto_remove,
	.driver	= {
		.name	= "nvt_crypto",
		.of_match_table	= na51055_crypto_of_match,
	},
};

module_platform_driver(na51055_crypto_driver);

MODULE_DESCRIPTION("Novatek crypto hardware acceleration support.");
MODULE_AUTHOR("Novatek");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
