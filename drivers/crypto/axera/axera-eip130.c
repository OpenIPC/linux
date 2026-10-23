// SPDX-License-Identifier: GPL-2.0-only
/* AX620E/AX630C EIP-130. Token protocol: Axera SDK 4.19 cipher driver.
 *
 * One crypto_engine serializes mailbox 2 and two persistent DMA buffers.
 * All caller memory is copied before submission. A timed-out engine is
 * reset and poisoned: no buffer can be reused by a later request while
 * an old DMA might still be outstanding. A root-only sysfs command can
 * reinitialize a poisoned engine while holding the same worker mutex.
 */
#include <crypto/aes.h>
#include <crypto/engine.h>
#include <crypto/internal/skcipher.h>
#include <crypto/scatterwalk.h>
#include <linux/crypto.h>
#include <linux/axera_eip130.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/compat.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/sizes.h>
#include <linux/unaligned.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define FW_NAME "axera/eip130.bin"
#define TOKEN_WORDS 64
#define BUFFER_SIZE SZ_64K
#define MAILBOX 0x400 /* mailbox 2 */
#define MBOX_CTRL 0x3f00
#define MBOX_LINK BIT(6)
#define MBOX_INPUT BIT(4)
#define MBOX_OUTPUT BIT(5)
#define IRQ_DONE BIT(3)
#define AIC_POL 0x3e00
#define AIC_TYPE 0x3e04
#define AIC_ENABLE 0x3e08
#define AIC_RAW 0x3e0c
#define AIC_ACK 0x3e10
#define MODULE_STATUS 0x3fe0
#define FW_RAM 0x4000
#define FW_BUSY BIT(8)
#define FW_CRC_OK BIT(9)
#define FW_WRITTEN BIT(20)
#define FW_CHECKED BIT(22)
#define FW_ACCEPTED BIT(23)
#define FATAL_ERROR BIT(31)

struct ax_dev {
	struct device *dev;
	void __iomem *regs;
	struct clk_bulk_data clocks[3];
	struct reset_control *resets;
	struct crypto_engine *engine;
	struct miscdevice misc;
	struct completion done;
	struct mutex lock;
	int irq;
	u16 sequence;
	bool poisoned;
	bool stopping;
	u32 result_status;
	u8 *input, *output;
	dma_addr_t input_dma, output_dma;
	atomic64_t requests, bytes, errors, timeouts;
	atomic64_t submissions, completions, unexpected_irqs, recoveries;
};

struct ax_ctx {
	struct ax_dev *dev;
	u8 key[AES_MAX_KEY_SIZE];
	u32 keylen;
	u32 mode;
};

struct ax_req {
	bool encrypt;
};

struct ax_alg {
	struct skcipher_engine_alg alg;
	struct ax_dev *dev;
	u32 mode;
};

/* Root-only fault injection for testing timeout quiescence. */
static unsigned int fault_timeout_once;
module_param(fault_timeout_once, uint, 0600);
MODULE_PARM_DESC(fault_timeout_once, "Drop one completion and poison/reset engine");

static void ax_write_words(struct ax_dev *ce, u32 offset, const u32 *words,
			   size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
		writel_relaxed(words[i], ce->regs + offset + i * sizeof(u32));
	/* The following non-relaxed doorbell orders this whole token/firmware
	 * payload. A barrier per word needlessly stalls the CPU on the APB bus.
	 */
}

static irqreturn_t ax_irq(int irq, void *data)
{
	struct ax_dev *ce = data;

	if (!(readl(ce->regs + AIC_RAW) & IRQ_DONE))
		return IRQ_NONE;
	writel(IRQ_DONE, ce->regs + AIC_ACK);
	if (!(readl(ce->regs + MBOX_CTRL) & MBOX_OUTPUT)) {
		atomic64_inc(&ce->unexpected_irqs);
		return IRQ_HANDLED;
	}
	/* AES ECB/CBC/CTR return data through DMA, not through the token.
	 * Mailbox ownership is released by MBOX_OUTPUT, independently of how
	 * many words were read. Only word 0 has the status and sequence ID.
	 */
	ce->result_status = readl_relaxed(ce->regs + MAILBOX);
	writel(MBOX_OUTPUT, ce->regs + MBOX_CTRL);
	atomic64_inc(&ce->completions);
	complete(&ce->done);
	return IRQ_HANDLED;
}

static void ax_quiesce(struct ax_dev *ce)
{
	writel(0, ce->regs + AIC_ENABLE);
	readl(ce->regs + AIC_ENABLE);
	synchronize_irq(ce->irq);
	reset_control_assert(ce->resets);
	/* Assert core AND AXI master reset before releasing DMA memory. */
	udelay(10);
}

static int ax_exchange(struct ax_dev *ce, u32 token[TOKEN_WORDS])
{
	bool inject;
	u32 status;
	u16 seq;
	int err = 0;
	const char *reason = "mailbox busy before submit";

	if (ce->poisoned)
		return -EIO;
	if (readl(ce->regs + MBOX_CTRL) & (MBOX_INPUT | MBOX_OUTPUT))
		goto poison;
	reinit_completion(&ce->done);
	seq = ++ce->sequence;
	token[0] |= seq;
	token[1] = 0x4f5a3647; /* public crypto officer identity */
	inject = xchg(&fault_timeout_once, 0);
	if (inject)
		writel(0, ce->regs + AIC_ENABLE);
	/* Coherent buffers still require ordering before the MMIO doorbell. */
	dma_wmb();
	ax_write_words(ce, MAILBOX, token, TOKEN_WORDS);
	atomic64_inc(&ce->submissions);
	writel(MBOX_INPUT, ce->regs + MBOX_CTRL);
	if (!wait_for_completion_timeout(&ce->done, msecs_to_jiffies(100))) {
		atomic64_inc(&ce->timeouts);
		err = -ETIMEDOUT;
		reason = "completion timeout";
		goto poison;
	}
	dma_rmb();
	status = ce->result_status;
	ce->result_status = 0;
	if ((status & 0xffff) != seq) {
		reason = "response sequence mismatch";
		goto poison;
	}
	if (status & FATAL_ERROR) {
		atomic64_inc(&ce->errors);
		dev_err_ratelimited(ce->dev, "token rejected (status %08x)\n",
				    status & ~0xffff);
		return -EIO;
	}
	return 0;
poison:
	WRITE_ONCE(ce->poisoned, true);
	atomic64_inc(&ce->errors);
	/* Record protocol state before reset; never log tokens containing keys. */
	dev_err(ce->dev, "%s: mailbox=%08x irq=%08x module=%08x seq=%u\n",
		reason, readl(ce->regs + MBOX_CTRL), readl(ce->regs + AIC_RAW),
		readl(ce->regs + MODULE_STATUS), ce->sequence);
	ax_quiesce(ce);
	ce->result_status = 0;
	dev_err(ce->dev, "engine reset after failed exchange; recovery required\n");
	return err ?: -EIO;
}

static int ax_firmware(struct ax_dev *ce)
{
	const struct firmware *fw;
	u32 status, *words;
	size_t i;
	int err;

	err = readl_poll_timeout(ce->regs + MODULE_STATUS, status,
				 !(status & FW_BUSY), 10, 100000);
	if (err)
		return err;
	if (!(status & FW_CRC_OK) || (status & FATAL_ERROR))
		return -EIO;
	if ((status & (FW_CHECKED | FW_ACCEPTED)) == (FW_CHECKED | FW_ACCEPTED))
		goto link;
	err = request_firmware(&fw, FW_NAME, ce->dev);
	if (err)
		return err;
	if (fw->size < 256 || fw->size > 256 + 0x1c000 || fw->size % 4) {
		err = -EINVAL;
		goto release;
	}
	words = kmalloc(fw->size, GFP_KERNEL);
	if (!words) {
		err = -ENOMEM;
		goto release;
	}
	for (i = 0; i < fw->size / 4; i++)
		words[i] = get_unaligned_le32(fw->data + i * 4);
	/* Firmware bootstrap uses mailbox 1; operations use mailbox 2. */
	writel(BIT(2), ce->regs + MBOX_CTRL);
	if (!(readl(ce->regs + MBOX_CTRL) & BIT(2))) {
		err = -EIO;
		goto free;
	}
	ax_write_words(ce, 0, words, TOKEN_WORDS);
	writel(BIT(0), ce->regs + MBOX_CTRL);
	writel(BIT(3), ce->regs + MBOX_CTRL);
	ax_write_words(ce, FW_RAM, words + TOKEN_WORDS,
		       fw->size / 4 - TOKEN_WORDS);
	writel(FW_WRITTEN, ce->regs + MODULE_STATUS);
	err = readl_poll_timeout(ce->regs + MODULE_STATUS, status,
				 status & (FW_CHECKED | FATAL_ERROR), 100, 1000000);
	if (!err && ((status & (FW_CRC_OK | FW_ACCEPTED | FATAL_ERROR)) !=
		     (FW_CRC_OK | FW_ACCEPTED)))
		err = -EIO;
free:
	kfree(words);
release:
	release_firmware(fw);
	if (err)
		return err;
link:
	writel(MBOX_LINK, ce->regs + MBOX_CTRL);
	if (!(readl(ce->regs + MBOX_CTRL) & MBOX_LINK))
		return -EIO;
	/* Discard an unconsumed response after timeout/reset recovery. */
	if (readl(ce->regs + MBOX_CTRL) & MBOX_OUTPUT)
		writel(MBOX_OUTPUT, ce->regs + MBOX_CTRL);
	writel(0, ce->regs + 0x3f10); /* no mailbox lockout */
	writel(0x1ff, ce->regs + AIC_POL);
	writel(0, ce->regs + AIC_TYPE);
	writel(IRQ_DONE, ce->regs + AIC_ACK);
	writel(IRQ_DONE, ce->regs + AIC_ENABLE);
	return 0;
}

static void ax_add_counter(u8 iv[AES_BLOCK_SIZE], u32 blocks)
{
	u64 sum;
	int i;

	for (i = 12; i >= 0; i -= 4) {
		sum = (u64)get_unaligned_be32(iv + i) + blocks;
		put_unaligned_be32((u32)sum, iv + i);
		blocks = sum >> 32;
	}
}

static int ax_do_cipher(struct crypto_engine *engine, void *areq)
{
	struct skcipher_request *req = skcipher_request_cast(areq);
	struct ax_ctx *ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct ax_req *rctx = skcipher_request_ctx(req);
	struct ax_dev *ce = ctx->dev;
	u32 token[TOKEN_WORDS] = {};
	u8 iv[AES_BLOCK_SIZE], next_iv[AES_BLOCK_SIZE];
	unsigned int offset = 0, length, padded, dirty = 0, i;
	int err = 0;

	mutex_lock(&ce->lock);
	if (ce->stopping || ce->poisoned) {
		err = -EIO;
		goto finish;
	}
	if (ctx->mode)
		memcpy(iv, req->iv, sizeof(iv));
	while (offset < req->cryptlen) {
		length = min_t(unsigned int, req->cryptlen - offset, BUFFER_SIZE);
		/* The hardware increments only the low 32 counter bits. */
		if (ctx->mode == 2) {
			u64 blocks = (1ULL << 32) - get_unaligned_be32(iv + 12);

			length = min_t(u64, length, blocks * AES_BLOCK_SIZE);
		}
		padded = ALIGN(length, AES_BLOCK_SIZE);
		dirty = max(dirty, padded);
		memset(ce->input + length, 0, padded - length);
		scatterwalk_map_and_copy(ce->input, req->src, offset, length, 0);
		if (ctx->mode == 1 && !rctx->encrypt)
			memcpy(next_iv, ce->input + length - AES_BLOCK_SIZE,
			       AES_BLOCK_SIZE);
		memset(token, 0, sizeof(token));
		token[0] = 1U << 24;
		token[2] = padded;
		token[3] = lower_32_bits(ce->input_dma);
		token[4] = upper_32_bits(ce->input_dma);
		token[5] = padded;
		token[6] = lower_32_bits(ce->output_dma);
		token[7] = upper_32_bits(ce->output_dma);
		token[8] = padded;
		token[11] = (ctx->mode << 4) | ((ctx->keylen / 8 - 1) << 16);
		if (rctx->encrypt || ctx->mode == 2)
			token[11] |= BIT(15);
		for (i = 0; i < ctx->keylen / 4; i++)
			token[17 + i] = get_unaligned_le32(ctx->key + i * 4);
		if (ctx->mode)
			for (i = 0; i < AES_BLOCK_SIZE / 4; i++)
				token[13 + i] = get_unaligned_le32(iv + i * 4);
		err = ax_exchange(ce, token);
		if (err)
			break;
		scatterwalk_map_and_copy(ce->output, req->dst, offset, length, 1);
		if (ctx->mode == 2)
			ax_add_counter(iv, padded / AES_BLOCK_SIZE);
		else if (ctx->mode == 1)
			memcpy(iv, rctx->encrypt ? ce->output + length - AES_BLOCK_SIZE :
			       next_iv, AES_BLOCK_SIZE);
		offset += length;
	}
	if (!err) {
		if (ctx->mode)
			memcpy(req->iv, iv, AES_BLOCK_SIZE);
		atomic64_inc(&ce->requests);
		atomic64_add(req->cryptlen, &ce->bytes);
	}
finish:
	memzero_explicit(token, sizeof(token));
	memzero_explicit(iv, sizeof(iv));
	memzero_explicit(next_iv, sizeof(next_iv));
	/* Wipe staging memory on both success and failure, after DMA is stopped. */
	memzero_explicit(ce->input, dirty);
	memzero_explicit(ce->output, dirty);
	mutex_unlock(&ce->lock);
	local_bh_disable();
	crypto_finalize_skcipher_request(engine, req, err);
	local_bh_enable();
	return 0;
}

static int ax_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned int len)
{
	struct ax_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (len != AES_KEYSIZE_128 && len != AES_KEYSIZE_192 && len != AES_KEYSIZE_256)
		return -EINVAL;
	memzero_explicit(ctx->key, sizeof(ctx->key));
	memcpy(ctx->key, key, len);
	ctx->keylen = len;
	return 0;
}

static int ax_cipher_init(struct crypto_skcipher *tfm)
{
	struct ax_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct ax_alg *alg = container_of(crypto_skcipher_alg(tfm), struct ax_alg,
					alg.base);

	ctx->dev = alg->dev;
	ctx->mode = alg->mode;
	crypto_skcipher_set_reqsize(tfm, sizeof(struct ax_req));
	return 0;
}

static void ax_cipher_exit(struct crypto_skcipher *tfm)
{
	memzero_explicit(crypto_skcipher_ctx(tfm), sizeof(struct ax_ctx));
}

static int ax_enqueue(struct skcipher_request *req, bool encrypt)
{
	struct ax_ctx *ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct ax_req *rctx = skcipher_request_ctx(req);

	if (!ctx->keylen)
		return -ENOKEY;
	if (READ_ONCE(ctx->dev->stopping) || READ_ONCE(ctx->dev->poisoned))
		return -EIO;
	if (ctx->mode != 2 && req->cryptlen % AES_BLOCK_SIZE)
		return -EINVAL;
	if (!req->cryptlen)
		return 0;
	rctx->encrypt = encrypt;
	return crypto_transfer_skcipher_request_to_engine(ctx->dev->engine, req);
}

static int ax_encrypt(struct skcipher_request *req)
{
	return ax_enqueue(req, true);
}

static int ax_decrypt(struct skcipher_request *req)
{
	return ax_enqueue(req, false);
}

#define AX_ALG(_mode, _name, _block, _iv) { \
	.mode = _mode, .alg = { .base = { \
		.base = { .cra_name = _name "(aes)", \
			.cra_driver_name = "axera-eip130-" _name, \
			.cra_priority = 200, .cra_flags = CRYPTO_ALG_ASYNC, \
			.cra_blocksize = _block, .cra_ctxsize = sizeof(struct ax_ctx), \
			.cra_module = THIS_MODULE }, \
		.min_keysize = AES_MIN_KEY_SIZE, .max_keysize = AES_MAX_KEY_SIZE, \
		.ivsize = _iv, .chunksize = AES_BLOCK_SIZE, \
		.setkey = ax_setkey, .encrypt = ax_encrypt, .decrypt = ax_decrypt, \
		.init = ax_cipher_init, .exit = ax_cipher_exit }, \
		.op.do_one_request = ax_do_cipher } }

static struct ax_alg ax_algs[] = {
	AX_ALG(0, "ecb", AES_BLOCK_SIZE, 0),
	AX_ALG(1, "cbc", AES_BLOCK_SIZE, AES_BLOCK_SIZE),
	AX_ALG(2, "ctr", 1, AES_BLOCK_SIZE),
};

/* Private, versioned application transport; no sockets, physical addresses or
 * raw tokens. The Crypto API below still owns queueing and hardware lifetime.
 */
#define AX_NATIVE_SG_MAX (DIV_ROUND_UP(AXERA_EIP130_MAX_INPUT, PAGE_SIZE) + 1)
struct ax_file {
	struct ax_dev *dev;
	struct mutex lock;
	struct crypto_skcipher *tfm;
	struct skcipher_request *request;
	u32 algorithm;
	u8 *input, *output;
	struct scatterlist src[AX_NATIVE_SG_MAX], dst[AX_NATIVE_SG_MAX];
};

static void ax_native_sg(struct scatterlist *sg, u8 *buffer)
{
	size_t offset = 0, length;
	unsigned int i = 0;
	unsigned int entries = DIV_ROUND_UP(offset_in_page(buffer) +
					    AXERA_EIP130_MAX_INPUT, PAGE_SIZE);

	sg_init_table(sg, entries);
	while (offset < AXERA_EIP130_MAX_INPUT) {
		u8 *address = buffer + offset;
		struct page *page = is_vmalloc_addr(address) ?
			vmalloc_to_page(address) : virt_to_page(address);

		length = min_t(size_t, PAGE_SIZE - offset_in_page(address),
			       AXERA_EIP130_MAX_INPUT - offset);
		sg_set_page(&sg[i++], page, length, offset_in_page(address));
		offset += length;
	}
}

static int ax_native_open(struct inode *inode, struct file *file)
{
	struct ax_dev *ce = container_of(file->private_data, struct ax_dev, misc);
	struct ax_file *ctx;

	if (READ_ONCE(ce->stopping))
		return -ESHUTDOWN;
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->dev = ce;
	mutex_init(&ctx->lock);
	file->private_data = ctx;
	return nonseekable_open(inode, file);
}

static int ax_native_release(struct inode *inode, struct file *file)
{
	struct ax_file *ctx = file->private_data;

	if (ctx->request)
		skcipher_request_free(ctx->request);
	if (ctx->tfm)
		crypto_free_skcipher(ctx->tfm);
	kvfree_sensitive(ctx->input, AXERA_EIP130_MAX_INPUT);
	kvfree_sensitive(ctx->output, AXERA_EIP130_MAX_INPUT);
	kfree_sensitive(ctx);
	return 0;
}

static int ax_native_key(struct ax_file *ctx, void __user *arg)
{
	struct axera_eip130_key key;
	const char *driver;
	int err;

	if (ctx->tfm)
		return -EBUSY;
	if (copy_from_user(&key, arg, sizeof(key))) {
		err = -EFAULT;
		goto out;
	}
	if (key.version != AXERA_EIP130_ABI || key.reserved ||
	    (key.key_size != 16 && key.key_size != 24 && key.key_size != 32)) {
		err = -EINVAL;
		goto out;
	}
	if (key.algorithm == AXERA_EIP130_ALG_ECB)
		driver = "axera-eip130-ecb";
	else if (key.algorithm == AXERA_EIP130_ALG_CTR)
		driver = "axera-eip130-ctr";
	else {
		err = -EOPNOTSUPP;
		goto out;
	}
	ctx->tfm = crypto_alloc_skcipher(driver, 0, 0);
	if (IS_ERR(ctx->tfm)) {
		err = PTR_ERR(ctx->tfm);
		ctx->tfm = NULL;
		goto out;
	}
	err = crypto_skcipher_setkey(ctx->tfm, key.key, key.key_size);
	if (err)
		goto cleanup;
	ctx->input = kvzalloc(AXERA_EIP130_MAX_INPUT, GFP_KERNEL);
	ctx->output = kvzalloc(AXERA_EIP130_MAX_INPUT, GFP_KERNEL);
	ctx->request = skcipher_request_alloc(ctx->tfm, GFP_KERNEL);
	if (!ctx->input || !ctx->output || !ctx->request) {
		err = -ENOMEM;
		goto cleanup;
	}
	ax_native_sg(ctx->src, ctx->input);
	ax_native_sg(ctx->dst, ctx->output);
	ctx->algorithm = key.algorithm;
	goto out;
cleanup:
	if (ctx->request)
		skcipher_request_free(ctx->request);
	crypto_free_skcipher(ctx->tfm);
	kvfree_sensitive(ctx->input, AXERA_EIP130_MAX_INPUT);
	kvfree_sensitive(ctx->output, AXERA_EIP130_MAX_INPUT);
	ctx->request = NULL;
	ctx->tfm = NULL;
	ctx->input = ctx->output = NULL;
out:
	memzero_explicit(&key, sizeof(key));
	return err;
}

static int ax_native_run(struct ax_file *ctx, void __user *arg)
{
	struct axera_eip130_request request;
	u8 iv[AES_BLOCK_SIZE];
	DECLARE_CRYPTO_WAIT(wait);
	int err = 0;

	if (copy_from_user(&request, arg, sizeof(request)))
		return -EFAULT;
	request.output_size = 0;
	if (!ctx->tfm) {
		err = -ENOKEY;
		goto out;
	}
	if (request.version != AXERA_EIP130_ABI || request.flags || request.reserved ||
	    request.size > AXERA_EIP130_MAX_INPUT ||
	    (request.size && (!request.src || !request.dst)) ||
	    (ctx->algorithm == AXERA_EIP130_ALG_ECB &&
	     (request.iv_size || request.size % AES_BLOCK_SIZE)) ||
	    (ctx->algorithm == AXERA_EIP130_ALG_CTR && request.iv_size != AES_BLOCK_SIZE)) {
		err = -EINVAL;
		goto out;
	}
	if (copy_from_user(ctx->input, u64_to_user_ptr(request.src), request.size)) {
		err = -EFAULT;
		goto wipe;
	}
	skcipher_request_set_callback(ctx->request, CRYPTO_TFM_REQ_MAY_BACKLOG |
				       CRYPTO_TFM_REQ_MAY_SLEEP, crypto_req_done, &wait);
	memcpy(iv, request.iv, sizeof(iv));
	skcipher_request_set_crypt(ctx->request, ctx->src, ctx->dst, request.size,
				   request.iv_size ? iv : NULL);
	err = crypto_wait_req(crypto_skcipher_encrypt(ctx->request), &wait);
	if (!err) {
		if (copy_to_user(u64_to_user_ptr(request.dst), ctx->output, request.size))
			err = -EFAULT;
		else
			request.output_size = request.size;
	}
wipe:
	memzero_explicit(ctx->input, request.size);
	memzero_explicit(ctx->output, request.size);
out:
	/* Preserve the caller's IV; only output_size is an output field. */
	memzero_explicit(iv, sizeof(iv));
	if (copy_to_user(arg, &request, sizeof(request)))
		err = -EFAULT;
	return err;
}

static long ax_native_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	struct ax_file *ctx = file->private_data;
	int err;

	mutex_lock(&ctx->lock);
	if (READ_ONCE(ctx->dev->stopping))
		err = -ESHUTDOWN;
	else if (command == AXERA_EIP130_SET_KEY)
		err = ax_native_key(ctx, (void __user *)arg);
	else if (command == AXERA_EIP130_RUN)
		err = ax_native_run(ctx, (void __user *)arg);
	else
		err = -ENOTTY;
	mutex_unlock(&ctx->lock);
	return err;
}

static const struct file_operations ax_native_fops = {
	.owner = THIS_MODULE,
	.open = ax_native_open,
	.release = ax_native_release,
	.unlocked_ioctl = ax_native_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
};

static ssize_t stats_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ax_dev *ce = dev_get_drvdata(dev);

	return sysfs_emit(buf, "requests %lld\nbytes %lld\nerrors %lld\ntimeouts %lld\npoisoned %u\nsubmissions %lld\ncompletions %lld\nunexpected_irqs %lld\nrecoveries %lld\n",
		atomic64_read(&ce->requests), atomic64_read(&ce->bytes),
		atomic64_read(&ce->errors), atomic64_read(&ce->timeouts),
		READ_ONCE(ce->poisoned), atomic64_read(&ce->submissions),
		atomic64_read(&ce->completions), atomic64_read(&ce->unexpected_irqs),
		atomic64_read(&ce->recoveries));
}
static DEVICE_ATTR_RO(stats);

static ssize_t recover_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct ax_dev *ce = dev_get_drvdata(dev);
	int err = 0;

	if (!sysfs_streq(buf, "1"))
		return -EINVAL;
	mutex_lock(&ce->lock);
	if (ce->stopping) {
		err = -ESHUTDOWN;
		goto out;
	}
	if (!ce->poisoned)
		goto out;
	/* ax_exchange has already reset the DMA master and synchronized IRQ.
	 * Keeping the worker mutex prevents queued requests from touching the
	 * staging buffers or mailbox until firmware initialization completes.
	 */
	err = reset_control_deassert(ce->resets);
	if (err)
		goto out;
	udelay(10);
	reinit_completion(&ce->done);
	err = ax_firmware(ce);
	if (err)
		ax_quiesce(ce);
	else {
		atomic64_inc(&ce->recoveries);
		WRITE_ONCE(ce->poisoned, false);
	}
out:
	mutex_unlock(&ce->lock);
	return err ?: count;
}
static DEVICE_ATTR_WO(recover);

static int ax_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ax_dev *ce;
	unsigned int i;
	int err;

	/* AX630C has one public engine; algorithm descriptors are singleton. */
	if (ax_algs[0].dev)
		return -EBUSY;
	ce = devm_kzalloc(dev, sizeof(*ce), GFP_KERNEL);
	if (!ce)
		return -ENOMEM;
	ce->dev = dev;
	ce->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ce->regs))
		return PTR_ERR(ce->regs);
	ce->clocks[0].id = "pclk";
	ce->clocks[1].id = "core";
	ce->clocks[2].id = "cnt";
	err = devm_clk_bulk_get(dev, ARRAY_SIZE(ce->clocks), ce->clocks);
	if (err)
		return dev_err_probe(dev, err, "clocks\n");
	ce->resets = devm_reset_control_array_get_exclusive(dev);
	if (IS_ERR(ce->resets))
		return dev_err_probe(dev, PTR_ERR(ce->resets), "resets\n");
	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (err)
		return err;
	ce->input = dmam_alloc_coherent(dev, BUFFER_SIZE, &ce->input_dma, GFP_KERNEL);
	ce->output = dmam_alloc_coherent(dev, BUFFER_SIZE, &ce->output_dma, GFP_KERNEL);
	if (!ce->input || !ce->output)
		return -ENOMEM;
	init_completion(&ce->done);
	mutex_init(&ce->lock);
	ce->irq = platform_get_irq(pdev, 0);
	if (ce->irq < 0)
		return ce->irq;
	err = clk_bulk_prepare_enable(ARRAY_SIZE(ce->clocks), ce->clocks);
	if (err)
		return err;
	err = reset_control_assert(ce->resets);
	if (err)
		goto clocks;
	udelay(10);
	err = reset_control_deassert(ce->resets);
	if (err)
		goto clocks;
	udelay(10);
	writel(0, ce->regs + AIC_ENABLE);
	err = devm_request_irq(dev, ce->irq, ax_irq, 0, dev_name(dev), ce);
	if (err)
		goto reset;
	err = ax_firmware(ce);
	if (err)
		goto quiesce;
	ce->engine = crypto_engine_alloc_init(dev, false);
	if (!ce->engine) {
		err = -ENOMEM;
		goto quiesce;
	}
	err = crypto_engine_start(ce->engine);
	if (err)
		goto engine;
	platform_set_drvdata(pdev, ce);
	for (i = 0; i < ARRAY_SIZE(ax_algs); i++) {
		ax_algs[i].dev = ce;
		err = crypto_engine_register_skcipher(&ax_algs[i].alg);
		if (err)
			goto unregister;
	}
	err = device_create_file(dev, &dev_attr_stats);
	if (err)
		goto unregister;
	err = device_create_file(dev, &dev_attr_recover);
	if (err) {
		device_remove_file(dev, &dev_attr_stats);
		goto unregister;
	}
	ce->misc = (struct miscdevice) {
		.minor = MISC_DYNAMIC_MINOR, .name = "axera-eip130", .mode = 0600,
		.fops = &ax_native_fops, .parent = dev,
	};
	err = misc_register(&ce->misc);
	if (err) {
		device_remove_file(dev, &dev_attr_recover);
		device_remove_file(dev, &dev_attr_stats);
		goto unregister;
	}
	dev_info(dev, "EIP-130 AES ECB/CBC/CTR, mailbox 2, IRQ %d\n", ce->irq);
	return 0;
unregister:
	while (i--)
		crypto_engine_unregister_skcipher(&ax_algs[i].alg);
	for (i = 0; i < ARRAY_SIZE(ax_algs); i++)
		ax_algs[i].dev = NULL;
	crypto_engine_stop(ce->engine);
engine:
	crypto_engine_exit(ce->engine);
quiesce:
	ax_quiesce(ce);
reset:
	reset_control_assert(ce->resets);
clocks:
	clk_bulk_disable_unprepare(ARRAY_SIZE(ce->clocks), ce->clocks);
	return dev_err_probe(dev, err, "EIP-130 initialization\n");
}

static void ax_remove(struct platform_device *pdev)
{
	struct ax_dev *ce = platform_get_drvdata(pdev);
	unsigned int i;

	misc_deregister(&ce->misc);
	device_remove_file(&pdev->dev, &dev_attr_stats);
	device_remove_file(&pdev->dev, &dev_attr_recover);
	for (i = 0; i < ARRAY_SIZE(ax_algs); i++)
		crypto_engine_unregister_skcipher(&ax_algs[i].alg);
	crypto_engine_stop(ce->engine);
	crypto_engine_exit(ce->engine);
	ax_quiesce(ce);
	clk_bulk_disable_unprepare(ARRAY_SIZE(ce->clocks), ce->clocks);
	for (i = 0; i < ARRAY_SIZE(ax_algs); i++)
		ax_algs[i].dev = NULL;
}

static const struct of_device_id ax_of_match[] = {
	{ .compatible = "axera,ax630c-eip130" },
	{}
};
MODULE_DEVICE_TABLE(of, ax_of_match);

static void ax_shutdown(struct platform_device *pdev)
{
	struct ax_dev *ce = platform_get_drvdata(pdev);

	/* crypto_engine_stop() can time out while clients are still submitting.
	 * Close admission first, drain queued requests without touching hardware,
	 * then serialize reset/clock-off with both the worker and sysfs recovery.
	 */
	mutex_lock(&ce->lock);
	WRITE_ONCE(ce->stopping, true);
	mutex_unlock(&ce->lock);
	crypto_engine_stop(ce->engine);
	mutex_lock(&ce->lock);
	ax_quiesce(ce);
	clk_bulk_disable_unprepare(ARRAY_SIZE(ce->clocks), ce->clocks);
	mutex_unlock(&ce->lock);
}

static struct platform_driver ax_driver = {
	.probe = ax_probe,
	.remove = ax_remove,
	.shutdown = ax_shutdown,
	.driver = {
		.name = "axera-eip130",
		.of_match_table = ax_of_match,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(ax_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Axera EIP-130 interrupt driven AES offload");
MODULE_FIRMWARE(FW_NAME);
