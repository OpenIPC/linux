/*
 * Cryptographic API.
 *
 * Support for Novatek NA51055 RSA Hardware acceleration.
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

#include <crypto/algapi.h>
#include <crypto/akcipher.h>
#include <crypto/scatterwalk.h>
#include <crypto/internal/akcipher.h>
#include <crypto/internal/rsa.h>

#include <plat/nvt-sramctl.h>
#include <plat/top.h>
#include "na51055_rsa.h"

#define DRV_VERSION  "1.01.00"

#define NA51055_RSA_QUEUE_LENGTH               16
#define NA51055_RSA_ALG_PRIORITY               1000
#define NA51055_RSA_DEFAULT_TIMEOUT            5000    ///<  5 sec

static int mclk = -1;
module_param(mclk, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mclk, "RSA master clock => 0:240MHz 1:320MHz 2:Reserved 3:PLL9, -1 means from device tree");

static u32 plat_chip_id = CHIP_NA51055;

struct na51055_rsa_key {
	u8     *n;
	u8     *e;
	u8     *d;
	size_t n_sz;
	size_t e_sz;
	size_t d_sz;
};

struct na51055_rsa_ctx {
	struct na51055_rsa_dev   *dev;
	struct na51055_rsa_key   key;               ///< key information
	int                      encrypt;           ///< 0:decrypt 1:encrypt
	NA51055_RSA_MODE_T       mode;              ///< operation mode
	NA51055_RSA_KEY_T        key_w;             ///< operation key width

	struct crypto_akcipher   *fallback_tfm;     ///< for support RSA-4096 to use software RSA
};

struct na51055_rsa_dev {
	struct device            *dev;
	struct clk               *clk;
	void __iomem             *iobase;
	int                      irq;
	spinlock_t               lock;
	bool                     busy;

	struct timer_list        timer;

	struct crypto_queue      queue;
	struct tasklet_struct    queue_tasklet;
	struct tasklet_struct    done_tasklet;

	struct akcipher_request  *req;
	size_t                   src_nbytes;
	size_t                   src_offset;
	size_t                   dst_nbytes;
	size_t                   dst_offset;

	u8                       buffer[NA51055_RSA_BUFFER_LEN] __attribute__((aligned(sizeof(u32))));
};

static struct na51055_rsa_dev *na51055_rdev = NULL;

static inline u32 na51055_rsa_read(struct na51055_rsa_dev *dd, u32 offset)
{
	return readl(dd->iobase + offset);
}

static inline void na51055_rsa_write(struct na51055_rsa_dev *dd, u32 offset, u32 value)
{
	writel(value, dd->iobase + offset);
}

static inline struct akcipher_request *na51055_rsa_akcipher_request_cast(struct crypto_async_request *req)
{
	return container_of(req, struct akcipher_request, base);
}

static inline void na51055_rsa_data_reverse(u8 *buf, int size)
{
	int i;
	for(i=0; i<(size>>1); i++) {
		buf[i]        ^= buf[size-i-1];
		buf[size-i-1] ^= buf[i];
		buf[i]        ^= buf[size-i-1];
	}
}

static void na51055_rsa_reset(struct na51055_rsa_dev *dd)
{
	u32 value = 0;
	u32 cnt   = 0;

	/* disable rsa */
	na51055_rsa_write(dd, NA51055_RSA_CTRL_REG, 0);
	na51055_rsa_write(dd, NA51055_RSA_CFG_REG,  0);

	/* set reset, hardware will auto clear */
	na51055_rsa_write(dd, NA51055_RSA_CFG_REG, 0x01);

	/* check reset done */
	while ((value = na51055_rsa_read(dd, NA51055_RSA_CFG_REG)) & 0x1) {
		if(cnt++ >= 100)
			break;
		udelay(1);
	}

	/* clear status */
	na51055_rsa_write(dd, NA51055_RSA_INT_STS_REG, 0x1);

	if (value & 0x1)
		dev_err(dd->dev, "rsa hardware reset failed!!\n");
}

static void na51055_rsa_complete(struct na51055_rsa_dev *dev, int err)
{
	akcipher_request_complete(dev->req, err);
	dev->busy = false;
	dev->req  = NULL;
}

static void na51055_rsa_timeout_handler(struct timer_list *t)
{
	unsigned long flags;
	struct na51055_rsa_dev *dev = from_timer(dev, t, timer);

	spin_lock_irqsave(&dev->lock, flags);

	if (dev->req) {
		dev_err(dev->dev, "rsa hardware dma timeout!\n");

		na51055_rsa_complete(dev, -EINVAL);

		dev->busy = true;

		/* reset device */
		na51055_rsa_reset(dev);

		/* trigger to do next rsa request in queue */
		tasklet_schedule(&dev->queue_tasklet);
	}
	else {
		dev_err(dev->dev, "rsa timer timeout!\n");
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

static void na51055_rsa_pio_start(struct na51055_rsa_dev *dev)
{
	struct akcipher_request *req = dev->req;
	struct na51055_rsa_ctx  *ctx = akcipher_tfm_ctx(crypto_akcipher_reqtfm(req));
	struct na51055_rsa_key  *key = &ctx->key;
	unsigned long           flags;
	int                     i;
	int                     err;
	int                     buf_len = (plat_chip_id == CHIP_NA51055) ? (NA51055_RSA_BUFFER_LEN>>1) : NA51055_RSA_BUFFER_LEN;

	spin_lock_irqsave(&dev->lock, flags);

	dev_dbg(dev->dev, "RSA   => encrypt:%d mode:%d key_w:%d\n", ctx->encrypt, ctx->mode, ctx->key_w);
	dev_dbg(dev->dev, "Key_N => va:0x%08x size:%d\n", (u32)key->n, key->n_sz);
	dev_dbg(dev->dev, "Key_E => va:0x%08x size:%d\n", (u32)key->e, key->e_sz);
	dev_dbg(dev->dev, "Key_D => va:0x%08x size:%d\n", (u32)key->d, key->d_sz);
	dev_dbg(dev->dev, "Src   => va:0x%08x size:%d nents:%d\n", (req->src ? (u32)sg_virt(req->src) : 0), req->src_len,  (req->src ? sg_nents(req->src) : 0));
	dev_dbg(dev->dev, "Dst   => va:0x%08x size:%d nents:%d\n", (req->dst ? (u32)sg_virt(req->dst) : 0), req->dst_len,  (req->dst ? sg_nents(req->dst) : 0));

	/* check request parameter */
	if (!req->src || !req->dst || !req->src_len || !req->dst_len) {
		err = -EINVAL;
		goto error;
	}
	if (!key->n || !key->n_sz || key->n_sz > buf_len) {
		err = -EINVAL;
		goto error;
	}
	if (ctx->encrypt && (!key->e || !key->e_sz || key->e_sz > buf_len)) {
		err = -EINVAL;
		goto error;
	}
	if (!ctx->encrypt && (!key->d || !key->d_sz || key->d_sz > buf_len)) {
		err = -EINVAL;
		goto error;
	}

	/* check pio busy */
	if (na51055_rsa_read(dev, NA51055_RSA_INT_STS_REG)&0x2) {
		dev_err(dev->dev, "rsa PIO mode busy!!\n");
		err = -EINVAL;
		goto error;
	}

	/*
	 *  RSA Input data byte order => big endian
	 *  [0000] 0F 0E 0D 0C 0B 0A 09 08 07 06 05 04 03 02 01 00
	 *
	 *  RSA SRAM data byte order => little endian
	 *  [0000] 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
	 */

	/* set data */
	dev->dst_offset = dev->dst_nbytes = 0;
	dev->src_offset = dev->src_nbytes = (req->src_len >= buf_len) ? buf_len : req->src_len;
	scatterwalk_map_and_copy(dev->buffer, req->src, 0, dev->src_nbytes, 0);
	if (dev->src_nbytes < buf_len) {
		memset(&dev->buffer[dev->src_nbytes], 0, (buf_len-dev->src_nbytes));
	}
	na51055_rsa_data_reverse(dev->buffer, dev->src_nbytes);
	na51055_rsa_write(dev, NA51055_RSA_DATA_ADDR_REG, 0);
	for (i=0; i<(buf_len/4); i++) {
		na51055_rsa_write(dev, NA51055_RSA_DATA_REG, ((u32 *)dev->buffer)[i]);
	}

	/* set key_n */
	memcpy(dev->buffer, key->n, key->n_sz);
	if (key->n_sz < buf_len) {
		memset(&dev->buffer[key->n_sz], 0, (buf_len-key->n_sz));
	}
	na51055_rsa_data_reverse(dev->buffer, key->n_sz);
	na51055_rsa_write(dev, NA51055_RSA_KEY_N_ADDR_REG, 0);
	for (i=0; i<(buf_len/4); i++) {
		na51055_rsa_write(dev, NA51055_RSA_KEY_N_REG, ((u32 *)dev->buffer)[i]);
	}

	/* set key_ed */
	if (ctx->encrypt) {
		memcpy(dev->buffer, key->e, key->e_sz);
		if (key->e_sz < buf_len) {
			memset(&dev->buffer[key->e_sz], 0, (buf_len-key->e_sz));
		}
		na51055_rsa_data_reverse(dev->buffer, key->e_sz);
	}
	else {
		memcpy(dev->buffer, key->d, key->d_sz);
		if (key->d_sz < buf_len) {
			memset(&dev->buffer[key->d_sz], 0, (buf_len-key->d_sz));
		}
		na51055_rsa_data_reverse(dev->buffer, key->d_sz);
	}
	na51055_rsa_write(dev, NA51055_RSA_KEY_ED_ADDR_REG, 0);
	for (i=0; i<(buf_len/4); i++) {
		na51055_rsa_write(dev, NA51055_RSA_KEY_ED_REG, ((u32 *)dev->buffer)[i]);
	}

	/* set key width and mode */
    na51055_rsa_write(dev, NA51055_RSA_CFG_REG, (((ctx->mode&0x3)<<4) | ((ctx->key_w&0x7)<<1)));

	/* clear interrupt status */
	na51055_rsa_write(dev, NA51055_RSA_INT_STS_REG, 0x1);

	/* enable interrupt mask */
	na51055_rsa_write(dev, NA51055_RSA_INT_ENB_REG, 0x1);

	/* trigger transfer */
	na51055_rsa_write(dev, NA51055_RSA_CTRL_REG, 0x1);

	/* start timeout timer */
	mod_timer(&dev->timer, jiffies+msecs_to_jiffies(NA51055_RSA_DEFAULT_TIMEOUT));

	spin_unlock_irqrestore(&dev->lock, flags);

	return;

error:
	na51055_rsa_complete(dev, err);

	spin_unlock_irqrestore(&dev->lock, flags);
}

static void na51055_rsa_done_tasklet(unsigned long data)
{
	struct na51055_rsa_dev  *dev = (struct na51055_rsa_dev *)data;
	struct akcipher_request *req = dev->req;
	struct na51055_rsa_ctx  *ctx;
	unsigned long           flags;
	int                     i;
	int                     err;
	int                     buf_len = (plat_chip_id == CHIP_NA51055) ? (NA51055_RSA_BUFFER_LEN>>1) : NA51055_RSA_BUFFER_LEN;

	spin_lock_irqsave(&dev->lock, flags);

	/* check request */
	if (!req) {
		dev_err(dev->dev, "rsa request is empty!!\n");
		goto exit;
	}
	ctx = akcipher_tfm_ctx(crypto_akcipher_reqtfm(req));

	/* set output data length base on key width */
	dev->dst_nbytes = (32<<ctx->key_w);

	/* copy output data to device buffer */
	na51055_rsa_write(dev, NA51055_RSA_DATA_ADDR_REG, 0);
	for (i=0; i<(dev->dst_nbytes/4); i++) {
		((u32 *)dev->buffer)[i] = na51055_rsa_read(dev, NA51055_RSA_DATA_REG);
	}
	na51055_rsa_data_reverse(dev->buffer, dev->dst_nbytes);

	/* copy data to request destination buffer */
	scatterwalk_map_and_copy(dev->buffer, req->dst, dev->dst_offset, dev->dst_nbytes, 1);
	dev->dst_offset += dev->dst_nbytes;

	dev_dbg(dev->dev, "Transfer done!!\n");
	dev_dbg(dev->dev, "Src   => offset:%d nbytes:%d\n", dev->src_offset, dev->src_nbytes);
	dev_dbg(dev->dev, "Dst   => offset:%d nbytes:%d\n", dev->dst_offset, dev->dst_nbytes);

	/* check request complete */
	if (dev->src_offset < req->src_len) {
		/* set data */
		dev->src_nbytes = ((req->src_len - dev->src_offset) >= buf_len) ? buf_len : (req->src_len - dev->src_offset);
		scatterwalk_map_and_copy(dev->buffer, req->src, dev->src_offset, dev->src_nbytes, 0);
		if (dev->src_nbytes < buf_len) {
			memset(&dev->buffer[dev->src_nbytes], 0, (buf_len-dev->src_nbytes));
		}
		na51055_rsa_data_reverse(dev->buffer, dev->src_nbytes);
		na51055_rsa_write(dev, NA51055_RSA_DATA_ADDR_REG, 0);
		for (i=0; i<(buf_len/4); i++) {
			na51055_rsa_write(dev, NA51055_RSA_DATA_REG, ((u32 *)dev->buffer)[i]);
		}

		/* clear interrupt status */
		na51055_rsa_write(dev, NA51055_RSA_INT_STS_REG, 0x1);

		/* trigger transfer */
		na51055_rsa_write(dev, NA51055_RSA_CTRL_REG, 0x1);

		/* start timeout timer */
		mod_timer(&dev->timer, jiffies+msecs_to_jiffies(NA51055_RSA_DEFAULT_TIMEOUT));

		dev_dbg(dev->dev, "trigger next transfer!\n");
	}
	else {
		err = (dev->src_offset == req->src_len) ? 0 : -EINVAL;
		na51055_rsa_complete(dev, err);
		dev->busy = true;
		tasklet_schedule(&dev->queue_tasklet);  ///< trigger to do next rsa request in queue
	}

exit:
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void na51055_rsa_queue_tasklet(unsigned long data)
{
	struct na51055_rsa_dev      *dev = (struct na51055_rsa_dev *)data;
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
	dev->req = na51055_rsa_akcipher_request_cast(async_req);

	spin_unlock_irqrestore(&dev->lock, flags);

	na51055_rsa_pio_start(dev);
}

static int na51055_rsa_handle_req(struct na51055_rsa_dev *dev, struct crypto_async_request *req)
{
	unsigned long flags;
	int err;

	spin_lock_irqsave(&dev->lock, flags);
	err = crypto_enqueue_request(&dev->queue, req);
	if (dev->busy) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return err;
	}
	dev->busy = true;
	spin_unlock_irqrestore(&dev->lock, flags);

	tasklet_schedule(&dev->queue_tasklet);

	return err;
}

static irqreturn_t na51055_rsa_irq(int irq, void *dev_id)
{
	struct na51055_rsa_dev *dev = (struct na51055_rsa_dev *)dev_id;
	u32                     status;

	spin_lock(&dev->lock);

	/* read status */
	status = na51055_rsa_read(dev, NA51055_RSA_INT_STS_REG);

	/* clear status */
	na51055_rsa_write(dev, NA51055_RSA_INT_STS_REG, (status & 0x1));

	/* transfer done */
	if (status & 0x1) {
		del_timer(&dev->timer);
		tasklet_schedule(&dev->done_tasklet);
	}

	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

static void na51055_rsa_free_key(struct na51055_rsa_key *key)
{
	if (key->n) {
		kfree(key->n);
		key->n = NULL;
	}
	key->n_sz = 0;

	if (key->e) {
		kfree(key->e);
		key->e = NULL;
	}
    key->e_sz = 0;

	if (key->d) {
		kfree(key->d);
		key->d = NULL;
	}
	key->d_sz = 0;
}

static int na51055_rsa_check_key_length(unsigned int len)
{
	switch (len) {	///< bit length
	case 256:
	case 512:
	case 1024:
	case 2048:
		return 0;
    case 4096:
        return ((plat_chip_id == CHIP_NA51055) ? -EINVAL : 0);
	}

	return -EINVAL;
}

static int na51055_rsa_do_fallback(struct akcipher_request *req, bool is_encrypt)
{
	struct crypto_akcipher  *tfm = crypto_akcipher_reqtfm(req);
	struct na51055_rsa_ctx  *ctx = akcipher_tfm_ctx(tfm);
	struct akcipher_request *subreq;
	int err;

	if (!ctx->fallback_tfm) {
		return -EINVAL;
	}
	else {
		subreq = akcipher_request_alloc(ctx->fallback_tfm, GFP_KERNEL);
		if (!subreq)
		    return -ENOMEM;

		/*
		 * Change the request to use the software fallback transform, and once
		 * the ciphering has completed, put the old transform back into the
		 * request.
		 */
		akcipher_request_set_callback(subreq, req->base.flags, NULL, NULL);
		akcipher_request_set_crypt(subreq, req->src, req->dst, req->src_len, req->dst_len);
		err = is_encrypt ? crypto_akcipher_encrypt(subreq) : crypto_akcipher_decrypt(subreq);
		akcipher_request_free(subreq);

		return err;
	}
}

static int na51055_rsa_enc(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct na51055_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *key = &ctx->key;

	if (na51055_rsa_check_key_length(key->n_sz<<3)) {
	    return na51055_rsa_do_fallback(req, 1);
	}

	if (unlikely(!key->n || !key->e))
		return -EINVAL;

	if (req->dst_len < key->n_sz) {
		req->dst_len = key->n_sz;
		dev_err(na51055_rdev->dev, "Output buffer length less than parameter n\n");
		return -EOVERFLOW;
	}

	/* setup device */
	ctx->dev = na51055_rdev;

	/* setup config */
	ctx->encrypt = 1;
	ctx->mode    = NA51055_RSA_MODE_NORMAL;
	switch(key->n_sz<<3) {
	case 256:
		ctx->key_w = NA51055_RSA_KEY_256;
		break;
	case 512:
		ctx->key_w = NA51055_RSA_KEY_512;
		break;
	case 1024:
		ctx->key_w = NA51055_RSA_KEY_1024;
		break;
	case 2048:
		ctx->key_w = NA51055_RSA_KEY_2048;
		break;
    case 4096:
        ctx->key_w = NA51055_RSA_KEY_4096;
        break;
	default:
		return -EINVAL;
	}

	return na51055_rsa_handle_req(ctx->dev, &req->base);
}

static int na51055_rsa_dec(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct na51055_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *key = &ctx->key;

	if (na51055_rsa_check_key_length(key->n_sz<<3)) {
	    return na51055_rsa_do_fallback(req, 0);
	}

	if (unlikely(!key->n || !key->d))
		return -EINVAL;

	if (req->dst_len < key->n_sz) {
		req->dst_len = key->n_sz;
		dev_err(na51055_rdev->dev, "Output buffer length less than parameter n\n");
		return -EOVERFLOW;
	}

	/* setup device */
	ctx->dev = na51055_rdev;

	/* setup config */
	ctx->encrypt = 0;
	ctx->mode    = NA51055_RSA_MODE_NORMAL;
	switch(key->n_sz<<3) {
	case 256:
		ctx->key_w = NA51055_RSA_KEY_256;
		break;
	case 512:
		ctx->key_w = NA51055_RSA_KEY_512;
		break;
	case 1024:
		ctx->key_w = NA51055_RSA_KEY_1024;
		break;
	case 2048:
		ctx->key_w = NA51055_RSA_KEY_2048;
		break;
	case 4096:
		ctx->key_w = NA51055_RSA_KEY_4096;
		break;
	default:
		return -EINVAL;
	}

	return na51055_rsa_handle_req(ctx->dev, &req->base);
}

static int na51055_rsa_set_pub_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen)
{
	int ret;
	struct na51055_rsa_ctx *ctx     = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *rsa_key = &ctx->key;
	struct rsa_key          raw_key = {0};

	/* free the old RSA key if any */
	na51055_rsa_free_key(rsa_key);

	/* parse public raw key */
	ret = rsa_parse_pub_key(&raw_key, key, keylen);
	if (ret)
		return ret;

	/*
	 * Skip leading zeros and copy the positive integer to a buffer.
	 * The decryption descriptor expects a positive integer for the
	 * RSA modulus and uses its length as decryption output length.
	 */
	while (!*raw_key.n && raw_key.n_sz) {
		raw_key.n++;
		raw_key.n_sz--;
	}
	if (na51055_rsa_check_key_length(raw_key.n_sz<<3)) {
	    /* The requested key size is not supported by HW, do a fallback */
	    if (ctx->fallback_tfm) {
	        rsa_key->n_sz = raw_key.n_sz;
	        return crypto_akcipher_set_pub_key(ctx->fallback_tfm, key, keylen);
	    }
	    else {
		    return -EINVAL;
		}
	}

	/* allocate Key_N buffer */
	rsa_key->n = kzalloc(raw_key.n_sz, GFP_KERNEL);
	if (!rsa_key->n) {
		ret = -ENOMEM;
		goto err;
	}
	rsa_key->n_sz = raw_key.n_sz;

	/* allocate Key_E buffer */
	rsa_key->e = kzalloc(raw_key.e_sz, GFP_KERNEL);
	if (!rsa_key->e) {
		ret = -ENOMEM;
		goto err;
	}
	rsa_key->e_sz = raw_key.e_sz;

	/* Copy Key_N */
	memcpy(rsa_key->n, raw_key.n, raw_key.n_sz);

	/* Copy Key_E */
	memcpy(rsa_key->e, raw_key.e, raw_key.e_sz);

	return 0;

err:
	na51055_rsa_free_key(rsa_key);
	return ret;
}

static int na51055_rsa_set_priv_key(struct crypto_akcipher *tfm, const void *key, unsigned int keylen)
{
	int ret;
	struct na51055_rsa_ctx *ctx     = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *rsa_key = &ctx->key;
	struct rsa_key          raw_key = {0};

	/* free the old RSA key if any */
	na51055_rsa_free_key(rsa_key);

	/* parse private raw key */
	ret = rsa_parse_priv_key(&raw_key, key, keylen);
	if (ret)
		return ret;

	/*
	 * Skip leading zeros and copy the positive integer to a buffer.
	 * The decryption descriptor expects a positive integer for the
	 * RSA modulus and uses its length as decryption output length.
	 */
	while (!*raw_key.n && raw_key.n_sz) {
		raw_key.n++;
		raw_key.n_sz--;
	}
	if (na51055_rsa_check_key_length(raw_key.n_sz<<3)) {
	    /* The requested key size is not supported by HW, do a fallback */
	    if (ctx->fallback_tfm) {
	        rsa_key->n_sz = raw_key.n_sz;
	        return crypto_akcipher_set_priv_key(ctx->fallback_tfm, key, keylen);
	    }
	    else {
		    return -EINVAL;
	    }
	}

	/* allocate Key_N buffer */
	rsa_key->n = kzalloc(raw_key.n_sz, GFP_KERNEL);
	if (!rsa_key->n) {
		ret = -ENOMEM;
		goto err;
	}
	rsa_key->n_sz = raw_key.n_sz;

	/* allocate Key_E buffer */
	rsa_key->e = kzalloc(raw_key.e_sz, GFP_KERNEL);
	if (!rsa_key->e) {
		ret = -ENOMEM;
		goto err;
	}
	rsa_key->e_sz = raw_key.e_sz;

	/* allocate Key_D buffer */
	rsa_key->d = kzalloc(raw_key.d_sz, GFP_KERNEL);
	if (!rsa_key->d) {
		ret = -ENOMEM;
		goto err;
	}
	rsa_key->d_sz = raw_key.d_sz;

	/* Copy Key_N */
	memcpy(rsa_key->n, raw_key.n, raw_key.n_sz);

	/* Copy Key_E */
	memcpy(rsa_key->e, raw_key.e, raw_key.e_sz);

	/* Copy Key_D */
	memcpy(rsa_key->d, raw_key.d, raw_key.d_sz);

	return 0;

err:
	na51055_rsa_free_key(rsa_key);
	return ret;
}

static unsigned int na51055_rsa_max_size(struct crypto_akcipher *tfm)
{
	struct na51055_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *key = &ctx->key;

    return (key->n_sz) ? key->n_sz : -EINVAL;
}

static int na51055_rsa_init_tfm(struct crypto_akcipher *tfm)
{
	struct na51055_rsa_ctx *ctx  = akcipher_tfm_ctx(tfm);
	struct crypto_tfm      *ctfm = crypto_akcipher_tfm(tfm);
	struct crypto_alg      *alg  = ctfm->__crt_alg;

	if (alg->cra_flags & CRYPTO_ALG_NEED_FALLBACK) {
		ctx->fallback_tfm = crypto_alloc_akcipher("rsa-generic", 0, 0);
		if (IS_ERR(ctx->fallback_tfm)) {
			pr_err( "failed to allocate akcipher fallback for %s\n", alg->cra_name);
			ctx->fallback_tfm = NULL;
		}
	}

	return 0;
}

static void na51055_rsa_exit_tfm(struct crypto_akcipher *tfm)
{
	struct na51055_rsa_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct na51055_rsa_key *key = &ctx->key;

	na51055_rsa_free_key(key);

	if (ctx->fallback_tfm) {
		crypto_free_akcipher(ctx->fallback_tfm);
		ctx->fallback_tfm = NULL;
	}

	return;
}

static struct akcipher_alg na51055_rsa_alg = {
	.encrypt      = na51055_rsa_enc,
	.decrypt      = na51055_rsa_dec,
	.sign         = na51055_rsa_dec,
	.verify       = na51055_rsa_enc,
	.set_pub_key  = na51055_rsa_set_pub_key,
	.set_priv_key = na51055_rsa_set_priv_key,
	.max_size     = na51055_rsa_max_size,
	.init         = na51055_rsa_init_tfm,
	.exit         = na51055_rsa_exit_tfm,
	.base = {
		.cra_name        = "rsa",
		.cra_driver_name = "rsa-na51055",
		.cra_priority    = NA51055_RSA_ALG_PRIORITY,
		.cra_flags       = CRYPTO_ALG_TYPE_AKCIPHER | CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK,
		.cra_module      = THIS_MODULE,
		.cra_ctxsize     = sizeof(struct na51055_rsa_ctx),
	}
};

static int na51055_rsa_probe(struct platform_device *pdev)
{
	int    ret;
	struct resource        *res;
	struct na51055_rsa_dev *dd;
	struct clk             *parent_clk;
	char                   *parent_clk_name;

	/* Platform CHIP ID */
	plat_chip_id = (u32)nvt_get_chip_id();

	dd = na51055_rdev = devm_kzalloc(&pdev->dev, sizeof(struct na51055_rsa_dev), GFP_KERNEL);
	if (!dd) {
		dev_err(&pdev->dev, "unable to alloc device data struct\n");
		return -ENOMEM;
	}

	dd->dev  = &pdev->dev;
	dd->irq  = -1;
	dd->busy = false;

	platform_set_drvdata(pdev, dd);

	spin_lock_init(&dd->lock);

	crypto_init_queue(&dd->queue, NA51055_RSA_QUEUE_LENGTH);

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

	ret = devm_request_irq(&pdev->dev, dd->irq, na51055_rsa_irq, 0, dev_name(&pdev->dev), dd);
	if (ret) {
		dev_err(&pdev->dev, "unable to request IRQ\n");
		return ret;
	}

    /* get rsa clock node */
	dd->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(dd->clk)) {
		dev_err(&pdev->dev, "failed to find rsa hardware clock\n");
		return PTR_ERR(dd->clk);
	}

	/* get rsa master clock selection from device tree */
	if (mclk < 0) {
        if (of_property_read_u32(pdev->dev.of_node, "mclk", &mclk)) {
            mclk = NA51055_RSA_MCLK_320MHZ;
        }
	}

    /* get parent clock node of rsa master clock */
	switch (mclk) {
	    case NA51055_RSA_MCLK_240MHZ:
	        parent_clk_name = "fix240m";
	        break;
	    case NA51055_RSA_MCLK_PLL9:
	        parent_clk_name = "pll9";
	        break;
	    case NA51055_RSA_MCLK_320MHZ:
	    default:
	        parent_clk_name = "pllf320";
	        break;
	}
	parent_clk = clk_get(NULL, parent_clk_name);
	if (IS_ERR(parent_clk)) {
		dev_err(&pdev->dev, "failed to find rsa parent clock\n");
		parent_clk = NULL;
	}

	/* set rsa clock to specify parent */
	if (parent_clk) {
	    ret = clk_set_parent(dd->clk, parent_clk);
	    if (ret < 0)
	        dev_err(&pdev->dev, "fail to switch rsa hardware clock\n");

	    /* release parent clock */
	    clk_put(parent_clk);
	}

	/* init tasklet for device request */
	tasklet_init(&dd->queue_tasklet, na51055_rsa_queue_tasklet, (unsigned long)dd);
	tasklet_init(&dd->done_tasklet,  na51055_rsa_done_tasklet,  (unsigned long)dd);

	/* init timer for device timeout */
	timer_setup(&dd->timer, na51055_rsa_timeout_handler, 0);

	/* enable crypto hardware clock */
	clk_prepare_enable(dd->clk);

	/* disable rsa sram shut-down */
	nvt_disable_sram_shutdown(RSA_SD);

	/* software reset rsa hardware */
	na51055_rsa_reset(dd);

	/* register rsa algorithm */
	ret = crypto_register_akcipher(&na51055_rsa_alg);
	if (ret) {
		dev_err(&pdev->dev, "failed to register rsa algorithm\n");
		goto error;
	}

	pr_info("nvt-rsa driver registered Version: %s (CLK: %luHz)\n", DRV_VERSION, clk_get_rate(dd->clk));

	return 0;

error:
	del_timer(&dd->timer);
	tasklet_kill(&dd->queue_tasklet);
	tasklet_kill(&dd->done_tasklet);
	nvt_enable_sram_shutdown(RSA_SD);
	clk_disable_unprepare(dd->clk);


	return ret;
}

static int na51055_rsa_remove(struct platform_device *pdev)
{
	struct na51055_rsa_dev *dev = platform_get_drvdata(pdev);

	if (!dev)
		return -ENODEV;

	del_timer_sync(&dev->timer);

	crypto_unregister_akcipher(&na51055_rsa_alg);

	tasklet_kill(&dev->queue_tasklet);
	tasklet_kill(&dev->done_tasklet);
	nvt_enable_sram_shutdown(RSA_SD);
	clk_disable_unprepare(dev->clk);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id na51055_rsa_of_match[] = {
	{ .compatible = "nvt,nvt_rsa" },
	{},
};
MODULE_DEVICE_TABLE(of, na51055_rsa_of_match);
#else
#define na51055_rsa_of_match     NULL
#endif

static struct platform_driver na51055_rsa_driver = {
	.probe	= na51055_rsa_probe,
	.remove	= na51055_rsa_remove,
	.driver	= {
		.name	= "nvt_rsa",
		.of_match_table	= na51055_rsa_of_match,
	},
};

module_platform_driver(na51055_rsa_driver);

MODULE_DESCRIPTION("Novatek rsa hardware acceleration support.");
MODULE_AUTHOR("Novatek");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
