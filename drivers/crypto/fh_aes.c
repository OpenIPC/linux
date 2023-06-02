/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/in.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/irqreturn.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <crypto/rng.h>
#include "fh_aes.h"

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

#define CRYPTO_QUEUE_LEN    (1000)
#define CRYPTION_POS		(0)
#define METHOD_POS			(1)
#define EMODE_POS			(4)

#define aes_readl(aes, name) \
	__raw_readl(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writel(aes, name, val) \
	__raw_writel((val), &(((struct fh_aes_reg *)aes->regs)->name))

#define aes_readw(aes, name) \
	__raw_readw(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writew(aes, name, val) \
	__raw_writew((val), &(((struct fh_aes_reg *)aes->regs)->name))

#define aes_readb(aes, name) \
	__raw_readb(&(((struct fh_aes_reg *)aes->regs)->name))

#define aes_writeb(aes, name, val) \
	__raw_writeb((val), &(((struct fh_aes_reg *)aes->regs)->name))


#ifdef CONFIG_FH_EFUSE
#define FH_AESV2
#else
#undef FH_AESV2
#endif

#define FH_AES_ALLIGN_SIZE			64
#define FH_AES_MALLOC_SIZE			4096
#define FH_AES_CTL_MAX_PROCESS_SIZE		(FH_AES_MALLOC_SIZE - 1)

#ifdef FH_AESV2
#include   <../drivers/misc/fh_efuse.h>
extern struct wrap_efuse_obj s_efuse_obj;
#endif
/****************************************************************************
 * ADT section
 * add definition of user defined Data Type that only be used in this file  here
 ***************************************************************************/
enum {
	ENCRYPT = 0 << CRYPTION_POS,
	DECRYPT = 1 << CRYPTION_POS,
};

enum {
	ECB_MODE = 0 << EMODE_POS,
	CBC_MODE = 1 << EMODE_POS,
	CTR_MODE = 2 << EMODE_POS,
	CFB_MODE = 4 << EMODE_POS,
	OFB_MODE = 5 << EMODE_POS,
};

enum {
	DES_METHOD = 0 << METHOD_POS,
	TRIPLE_DES_METHOD = 1 << METHOD_POS,
	AES_128_METHOD = 4 << METHOD_POS,
	AES_192_METHOD = 5 << METHOD_POS,
	AES_256_METHOD = 6 << METHOD_POS,
};

/*****************************************************************************

 *  static fun;
 *****************************************************************************/

static int fh_aes_handle_req(struct fh_aes_dev *dev,
			     struct ablkcipher_request *req);
/*aes*/
static int fh_aes_crypt(struct ablkcipher_request *req, unsigned long mode);
static int fh_aes_ecb_encrypt(struct ablkcipher_request *req);
static int fh_aes_ecb_decrypt(struct ablkcipher_request *req);
static int fh_aes_cbc_encrypt(struct ablkcipher_request *req);
static int fh_aes_cbc_decrypt(struct ablkcipher_request *req);
static int fh_aes_ctr_encrypt(struct ablkcipher_request *req);
static int fh_aes_ctr_decrypt(struct ablkcipher_request *req);
static int fh_aes_ofb_encrypt(struct ablkcipher_request *req);
static int fh_aes_ofb_decrypt(struct ablkcipher_request *req);
static int fh_aes_cfb_encrypt(struct ablkcipher_request *req);
static int fh_aes_cfb_decrypt(struct ablkcipher_request *req);

/*des*/
static int fh_des_ecb_encrypt(struct ablkcipher_request *req);
static int fh_des_ecb_decrypt(struct ablkcipher_request *req);
static int fh_des_cbc_encrypt(struct ablkcipher_request *req);
static int fh_des_cbc_decrypt(struct ablkcipher_request *req);
static int fh_des_ofb_encrypt(struct ablkcipher_request *req);
static int fh_des_ofb_decrypt(struct ablkcipher_request *req);
static int fh_des_cfb_encrypt(struct ablkcipher_request *req);
static int fh_des_cfb_decrypt(struct ablkcipher_request *req);

/*tri des*/
static int fh_des_tri_ecb_encrypt(struct ablkcipher_request *req);
static int fh_des_tri_ecb_decrypt(struct ablkcipher_request *req);
static int fh_des_tri_cbc_encrypt(struct ablkcipher_request *req);
static int fh_des_tri_cbc_decrypt(struct ablkcipher_request *req);
static int fh_des_tri_ofb_encrypt(struct ablkcipher_request *req);
static int fh_des_tri_ofb_decrypt(struct ablkcipher_request *req);
static int fh_des_tri_cfb_encrypt(struct ablkcipher_request *req);
static int fh_des_tri_cfb_decrypt(struct ablkcipher_request *req);
static int fh_aes_setkey(struct crypto_ablkcipher *cipher, const uint8_t *key,
			 unsigned int keylen);
static int fh_aes_cra_init(struct crypto_tfm *tfm);
static void fh_aes_tx(struct fh_aes_dev *dev);
static void fh_aes_rx(struct fh_aes_dev *dev);
static irqreturn_t fh_aes_interrupt(int irq, void *dev_id);
static void aes_biglittle_swap(u8 *buf);
static int fh_set_indata(struct fh_aes_dev *dev, struct scatterlist *sg);
static int fh_set_outdata(struct fh_aes_dev *dev, struct scatterlist *sg);
static void fh_set_aes_key_reg(struct fh_aes_dev *dev, uint8_t *key,
			       uint8_t *iv, unsigned int keylen);
static void fh_set_dma_indata(struct fh_aes_dev *dev,
			      struct scatterlist *sg);
static void fh_set_dma_outdata(struct fh_aes_dev *dev,
			       struct scatterlist *sg);
static void fh_unset_indata(struct fh_aes_dev *dev);
static void fh_unset_outdata(struct fh_aes_dev *dev);
static void fh_aes_complete(struct fh_aes_dev *dev, int err);
static void fh_aes_crypt_start(struct fh_aes_dev *dev, unsigned long mode);
static void fh_aes_work_cb(struct work_struct *w);

#define fh_des_setkey  fh_aes_setkey
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
struct fh_aes_dev  *pobj_aes_dev = NULL;
static struct crypto_alg algs[] = {
	{
		.cra_name		= "ecb(aes)",
		.cra_driver_name	= "ecb-aes-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ecb_encrypt,
			.decrypt	= fh_aes_ecb_decrypt,
		}
	},
	{
		.cra_name		= "cbc(aes)",
		.cra_driver_name	= "cbc-aes-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,

		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_cbc_encrypt,
			.decrypt	= fh_aes_cbc_decrypt,
		}
	},
	{
		.cra_name		= "ctr(aes)",
		.cra_driver_name	= "ctr-aes-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ctr_encrypt,
			.decrypt	= fh_aes_ctr_decrypt,
		}
	},
	{
		.cra_name		= "ofb(aes)",
		.cra_driver_name	= "ofb-aes-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_ofb_encrypt,
			.decrypt	= fh_aes_ofb_decrypt,
		}
	},
	{
		.cra_name		= "cfb(aes)",
		.cra_driver_name	= "cfb-aes-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= fh_aes_setkey,
			.encrypt	= fh_aes_cfb_encrypt,
			.decrypt	= fh_aes_cfb_decrypt,
		}
	},
	{
		.cra_name		= "ecb(des)",
		.cra_driver_name	= "ecb-des-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,

		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_ecb_encrypt,
			.decrypt	= fh_des_ecb_decrypt,
		}
	},
	{
		.cra_name		= "cbc(des)",
		.cra_driver_name	= "cbc-des-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_cbc_encrypt,
			.decrypt	= fh_des_cbc_decrypt,
		}
	},
	{
		.cra_name		= "ofb(des)",
		.cra_driver_name	= "ofb-des-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_ofb_encrypt,
			.decrypt	= fh_des_ofb_decrypt,
		}
	},
	{
		.cra_name		= "cfb(des)",
		.cra_driver_name	= "cfb-des-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES_KEY_SIZE,
			.max_keysize	= DES_KEY_SIZE,
			.ivsize		= DES_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_cfb_encrypt,
			.decrypt	= fh_des_cfb_decrypt,
		}
	},
	{
		.cra_name			= "ecb(des3)",
		.cra_driver_name	= "ecb-des3-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,

		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_tri_ecb_encrypt,
			.decrypt	= fh_des_tri_ecb_decrypt,
		}
	},
	{
		.cra_name			= "cbc(des3)",
		.cra_driver_name	= "cbc-des3-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES3_EDE_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_tri_cbc_encrypt,
			.decrypt	= fh_des_tri_cbc_decrypt,
		}
	},
	{
		.cra_name			= "ofb(des3)",
		.cra_driver_name	= "ofb-des3-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES3_EDE_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_tri_ofb_encrypt,
			.decrypt	= fh_des_tri_ofb_decrypt,
		}
	},
	{
		.cra_name			= "cfb(des3)",
		.cra_driver_name	= "cfb-des3-fh",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC,
		.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct fh_aes_ctx),
		.cra_alignmask		= 0x0f,
		.cra_type		= &crypto_ablkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= fh_aes_cra_init,
		.cra_u.ablkcipher = {
			.min_keysize	= DES3_EDE_KEY_SIZE,
			.max_keysize	= DES3_EDE_KEY_SIZE,
			.ivsize		= DES3_EDE_BLOCK_SIZE,
			.setkey		= fh_des_setkey,
			.encrypt	= fh_des_tri_cfb_encrypt,
			.decrypt	= fh_des_tri_cfb_decrypt,
		}
	},
};

#ifdef CONFIG_FH_AES_SELF_TEST
extern void fh_aes_self_test_all(void);
#endif

/* function body */
static int fh_aes_handle_req(struct fh_aes_dev *dev,
			     struct ablkcipher_request *req)
{
	unsigned long flags;
	int err;
	spin_lock_irqsave(&dev->lock, flags);
	err = ablkcipher_enqueue_request(&dev->queue, req);
	spin_unlock_irqrestore(&dev->lock, flags);
	queue_work(dev->workqueue, &dev->work);
	return err;
}

static int fh_aes_crypt(struct ablkcipher_request *req, unsigned long mode)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct fh_aes_reqctx *reqctx = ablkcipher_request_ctx(req);
	struct fh_aes_dev *dev = ctx->dev;
	AES_DBG("%s\n", __func__);
	dev->reqctx = reqctx;
	/*if (!(mode & CFB_MODE)) {*/
		if ((!IS_ALIGNED(req->nbytes, AES_BLOCK_SIZE))
		    && (!IS_ALIGNED(req->nbytes, DES_BLOCK_SIZE))) {
			pr_err("request size is not exact amount of AES blocks\n");
			return -EINVAL;
		}
	/*}*/
	AES_DBG("reqctx->mode value: %x\n", (unsigned int)mode);
	reqctx->mode = mode;
	return fh_aes_handle_req(dev, req);
}

static int fh_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	u32 method = 0;
	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | ECB_MODE | ENCRYPT);
}


static int fh_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	u32 method = 0;

	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | ECB_MODE | DECRYPT);
}

static int fh_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	struct fh_aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	u32 method = 0;

	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | CBC_MODE | ENCRYPT);
}

static int fh_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;
	AES_DBG("%s\n", __func__);
	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}

	return fh_aes_crypt(req, method | CBC_MODE | DECRYPT);
}

static int fh_aes_ctr_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;
	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;

	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}

	return fh_aes_crypt(req, method | CTR_MODE | ENCRYPT);
}

static int fh_aes_ctr_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;
	AES_DBG("%s\n", __func__);
	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | CTR_MODE | DECRYPT);
}

static int fh_aes_ofb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;

	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | OFB_MODE | ENCRYPT);
}

static int fh_aes_ofb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;

	AES_DBG("%s\n", __func__);
	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}

	return fh_aes_crypt(req, method | OFB_MODE | DECRYPT);
}

static int fh_aes_cfb_encrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;

	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}
	return fh_aes_crypt(req, method | CFB_MODE | ENCRYPT);
}

static int fh_aes_cfb_decrypt(struct ablkcipher_request *req)
{
	struct crypto_ablkcipher *tfm;
	struct fh_aes_ctx *ctx;
	u32 method;

	tfm = crypto_ablkcipher_reqtfm(req);
	ctx = crypto_ablkcipher_ctx(tfm);
	method = 0;

	AES_DBG("%s\n", __func__);
	switch (ctx->keylen) {
	case AES_KEYSIZE_128:
		method = AES_128_METHOD;
		break;
	case AES_KEYSIZE_192:
		method = AES_192_METHOD;
		break;
	case AES_KEYSIZE_256:
		method = AES_256_METHOD;
		break;
	default:
		break;
	}

	return fh_aes_crypt(req, method | CFB_MODE | DECRYPT);
}
static int fh_des_ecb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | ECB_MODE | ENCRYPT);
}

static int fh_des_ecb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | ECB_MODE | DECRYPT);
}

static int fh_des_cbc_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | CBC_MODE | ENCRYPT);
}

static int fh_des_cbc_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | CBC_MODE | DECRYPT);
}

static int fh_des_ofb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | OFB_MODE | ENCRYPT);
}

static int fh_des_ofb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | OFB_MODE | DECRYPT);
}

static int fh_des_cfb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | CFB_MODE | ENCRYPT);
}

static int fh_des_cfb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = DES_METHOD;

	return fh_aes_crypt(req, method | CFB_MODE | DECRYPT);
}
static int fh_des_tri_ecb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | ECB_MODE | ENCRYPT);
}

static int fh_des_tri_ecb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | ECB_MODE | DECRYPT);
}

static int fh_des_tri_cbc_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | CBC_MODE | ENCRYPT);
}

static int fh_des_tri_cbc_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | CBC_MODE | DECRYPT);
}

static int fh_des_tri_ofb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | OFB_MODE | ENCRYPT);
}

static int fh_des_tri_ofb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | OFB_MODE | DECRYPT);
}

static int fh_des_tri_cfb_encrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | CFB_MODE | ENCRYPT);
}

static int fh_des_tri_cfb_decrypt(struct ablkcipher_request *req)
{
	u32 method;
	method = 0;
	method = TRIPLE_DES_METHOD;
	return fh_aes_crypt(req, method | CFB_MODE | DECRYPT);
}
static int fh_aes_setkey(struct crypto_ablkcipher *cipher, const uint8_t *key,
			 unsigned int keylen)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct fh_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	int i = 0;
	AES_DBG("%s\n", __func__);
	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192
	    && keylen != AES_KEYSIZE_256 && keylen != DES_KEY_SIZE
	    && keylen != DES3_EDE_KEY_SIZE)
		return -EINVAL;

	for (; i < keylen; i++)
		AES_DBG("%x", key[i]);
	AES_DBG("\n");

	memcpy(ctx->aes_key, key, keylen);
	ctx->keylen = keylen;
	return 0;
}

static int fh_aes_cra_init(struct crypto_tfm *tfm)
{
	struct fh_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	ctx->dev = pobj_aes_dev;
	tfm->crt_ablkcipher.reqsize = sizeof(struct fh_aes_reqctx);
	AES_DBG("%s\n", __func__);
	return 0;
}

static void fh_aes_tx(struct fh_aes_dev *dev)
{
	/*int err = 0;*/
	unsigned int i = 0;
	struct ablkcipher_request *req = dev->req;
	struct scatterlist *temp_sg = req->dst;
	int len = 0;
	fh_unset_outdata(dev);
	do {
		sg_copy_from_buffer(temp_sg, 1, &dev->ctl_dst_xbuf[i],
		sg_dma_len(temp_sg));
		len += sg_dma_len(temp_sg);
		i += sg_dma_len(temp_sg);
		temp_sg = sg_next(temp_sg);
	} while (temp_sg != NULL);
	/*fh_aes_complete(dev, err);*/
}

static void fh_aes_rx(struct fh_aes_dev *dev)
{
	fh_unset_indata(dev);
}

static irqreturn_t fh_aes_interrupt(int irq, void *dev_id)
{

	u32 isr_status;
	/*unsigned long flags;*/
	struct platform_device *pdev = (struct platform_device *) dev_id;
	struct fh_aes_dev *dev = platform_get_drvdata(pdev);
	/*u32 isr = dev->en_isr;*/
	AES_DBG("%s\n", __func__);
	/*spin_lock_irqsave(&dev->lock, flags);*/
	aes_writel(dev, dma_control, 0);
	isr_status = aes_readl(dev, intr_src);
	aes_writel(dev, intr_clear_status, 0x07);
	aes_writel(dev, intr_enable, 0);
	if (isr_status & 0x02)
		printk("dma rev hreap error...\n");
	if (isr_status & 0x04)
		printk("dma stop src ..\n");
	if (isr_status & 0x01) {
		AES_DBG("dma done..\n");
		complete(&(dev->done));
	}
	/*spin_unlock_irqrestore(&dev->lock, flags);*/
	return IRQ_HANDLED;
}

static void aes_biglittle_swap(u8 *buf)
{
	u8 tmp, tmp1;
	tmp = buf[0];
	tmp1 = buf[1];
	buf[0] = buf[3];
	buf[1] = buf[2];
	buf[2] = tmp1;
	buf[3] = tmp;
}

static int fh_set_indata(struct fh_aes_dev *dev, struct scatterlist *sg)
{
	int err;
	unsigned int i = 0;
	unsigned int len = 0;
	struct scatterlist *temp_sg = sg;
	unsigned char *src_xbuf;
	src_xbuf = &dev->ctl_src_xbuf[0];
	do {
		if (len + sg_dma_len(temp_sg) > FH_AES_CTL_MAX_PROCESS_SIZE) {
			printk("%s: total size > driver size 0x%x\n", __func__, FH_AES_CTL_MAX_PROCESS_SIZE);
			err = -ENOMEM;
			goto exit;
		}
		sg_copy_to_buffer(temp_sg, 1, &src_xbuf[i], sg_dma_len(temp_sg));
		len += sg_dma_len(temp_sg);
		i += sg_dma_len(temp_sg);
		temp_sg = sg_next(temp_sg);
	} while (temp_sg != NULL);

	sg_init_one(&dev->src_sg[0], &src_xbuf[0], len);
	err = dma_map_sg(dev->dev, &dev->src_sg[0], 1, DMA_TO_DEVICE);
	if (!err) {
		err = -ENOMEM;
		goto exit;
	}
	dev->sg_src = &dev->src_sg[0];
	err = 0;
exit:
	return err;
}

static int fh_set_outdata(struct fh_aes_dev *dev, struct scatterlist *sg)
{
	int err;
	sg_init_one(&dev->dst_sg[0],
	&dev->ctl_dst_xbuf[0], FH_AES_CTL_MAX_PROCESS_SIZE);
	err = dma_map_sg(dev->dev, &dev->dst_sg[0], 1, DMA_FROM_DEVICE);
	if (!err) {
		err = -ENOMEM;
		goto exit;
	}
	dev->sg_dst = &dev->dst_sg[0];
	err = 0;
exit:
	return err;
}

static void fh_set_aes_key_reg(struct fh_aes_dev *dev, uint8_t *key,
			       uint8_t *iv, unsigned int keylen)
{

	int i;
	u32 method;
	u32 temp_key_buf[32];
	u32 temp_iv_buf[32];
	u32 *p_dst = NULL;
	u32 key_size = 0;
	if (dev->iv_flag == true) {
		/*set iv*/
		/*if aes mode ....set 128 bit iv,  des set 64bit iv..*/
		AES_DBG("set iv reg\n");
		if ((dev->control_reg & AES_128_METHOD)
		    || ((dev->control_reg & AES_192_METHOD))
		    || (dev->control_reg & AES_256_METHOD)) {
			AES_DBG("aes iv mode...\n");

			memcpy((u8 *)&temp_iv_buf[0], iv, 16);
			p_dst = &temp_iv_buf[0];
			for (i = 0; i < 16 / sizeof(u32); i++)
				aes_biglittle_swap((u8 *)(p_dst + i));
			memcpy((u8 *)&((struct fh_aes_reg *) dev->regs)->initial_vector0,
			       temp_iv_buf, 16);
		} else {
			AES_DBG("des iv mode...\n");

			memcpy((u8 *)&temp_iv_buf[0], iv, 8);
			p_dst = &temp_iv_buf[0];
			for (i = 0; i < 8 / sizeof(u32); i++)
				aes_biglittle_swap((u8 *)(p_dst + i));

			memcpy((u8 *)&((struct fh_aes_reg *) dev->regs)->initial_vector0,
			       temp_iv_buf, 8);

		}
	}
	/*set key...*/
	method = dev->control_reg & 0x0e;
	AES_DBG("set key reg\n");

	switch (method) {
	case AES_128_METHOD:
		AES_DBG("set key aes 128 mode..\n");
		key_size = 16;

		break;
	case AES_192_METHOD:
		AES_DBG("set key aes 192 mode..\n");
		key_size = 24;
		break;

	case AES_256_METHOD:
		AES_DBG("set key aes 256 mode..\n");
		key_size = 32;
		break;

	case DES_METHOD:
		AES_DBG("set key des normal mode..\n");
		key_size = 8;
		break;

	case TRIPLE_DES_METHOD:
		AES_DBG("set key des triple mode..\n");
		key_size = 24;
		break;

	default:
		AES_DBG("error method!!\n");
		break;
	}
#ifdef FH_AESV2
	if (dev->p_usr_def->mode & CRYPTO_EX_MEM_SET_KEY) {
		s_efuse_obj.trans_key_start_no = 0;
		s_efuse_obj.trans_key_size = key_size / 4;
		efuse_trans_key(&s_efuse_obj,
				s_efuse_obj.trans_key_start_no,
				s_efuse_obj.trans_key_size, dev->p_usr_def);
	} else {
		s_efuse_obj.old_usr_def.mode &= ~CRYPTO_EX_MEM_SET_KEY;
		s_efuse_obj.old_usr_def.mode |= CRYPTO_CPU_SET_KEY;
		memcpy((u8 *)&temp_key_buf[0], key, key_size);
		p_dst = &temp_key_buf[0];
		for (i = 0; i < key_size / sizeof(u32); i++)
			aes_biglittle_swap((u8 *)(p_dst + i));
		memcpy((u8 *)&((struct fh_aes_reg *) dev->regs)->security_key0,
		       (u8 *)&temp_key_buf[0],
		       key_size);
	}

#else
	memcpy((u8 *)&temp_key_buf[0], key, key_size);
	p_dst = &temp_key_buf[0];
	for (i = 0; i < key_size / sizeof(u32); i++)
		aes_biglittle_swap((u8 *)(p_dst + i));

	memcpy((u8 *)&((struct fh_aes_reg *) dev->regs)->security_key0,
	       (u8 *)&temp_key_buf[0],
	       key_size);

#endif

}

static void fh_set_dma_indata(struct fh_aes_dev *dev,
			      struct scatterlist *sg)
{
	aes_writel(dev, dma_src_add, sg_dma_address(sg));
	AES_DBG("%s :dma trans size is :%x,add is:%x\n",
	__func__, sg_dma_len(sg), sg_dma_address(sg));
	aes_writel(dev, dma_trans_size, sg_dma_len(sg));
}

static void fh_set_dma_outdata(struct fh_aes_dev *dev,
			       struct scatterlist *sg)
{
	aes_writel(dev, dma_dst_add, sg_dma_address(sg));
}

static void fh_unset_indata(struct fh_aes_dev *dev)
{
	dma_unmap_sg(dev->dev, dev->sg_src, 1, DMA_TO_DEVICE);
}

static void fh_unset_outdata(struct fh_aes_dev *dev)
{
	dma_unmap_sg(dev->dev, dev->sg_dst, 1, DMA_FROM_DEVICE);
}

static void fh_aes_complete(struct fh_aes_dev *dev, int err)
{
	if (dev->req->base.complete)
		dev->req->base.complete(&dev->req->base, err);
}

static void fh_aes_crypt_start(struct fh_aes_dev *dev, unsigned long mode)
{

	struct ablkcipher_request *req = dev->req;
	u32 control_reg;
	u32 outfifo_thold = 0;
	u32 infifo_thold = 0;
	u32 isr;
	int err;
	unsigned long flags;
	control_reg = 0;
	spin_lock_irqsave(&dev->lock, flags);
	if ((mode & CBC_MODE) || (mode & CTR_MODE) || (mode & CFB_MODE)
	    || (mode & OFB_MODE)) {
		control_reg |= 1 << 7;
		dev->iv_flag = true;
	} else
		dev->iv_flag = false;

	/*emode & method*/
	control_reg |= (unsigned int) mode;
	dev->control_reg = control_reg;
	outfifo_thold = 0;
	infifo_thold = 8;
	isr = dev->en_isr;

	AES_DBG("control_reg:0x%x\n", control_reg);
	aes_writel(dev, encrypt_control, control_reg);
	/*set key...*/
	fh_set_aes_key_reg(dev, dev->ctx->aes_key, req->info, dev->ctx->keylen);
	err = fh_set_indata(dev, req->src);
	if (err)
		goto indata_error;

	err = fh_set_outdata(dev, req->dst);
	if (err)
		goto outdata_error;

	fh_set_dma_indata(dev, dev->sg_src);
	fh_set_dma_outdata(dev, dev->sg_dst);

	/*set fifo..*/
	AES_DBG("outfifo thold:%x\n", outfifo_thold);
	AES_DBG("infifo thold:%x\n", infifo_thold);
	aes_writel(dev, fifo_threshold, outfifo_thold << 8 | infifo_thold);
	/*set isr..*/
	AES_DBG("intr enable:%x\n", isr);
	aes_writel(dev, intr_enable, isr);
	/*enable dma go..*/
	aes_writel(dev, dma_control, 1);
	spin_unlock_irqrestore(&dev->lock, flags);

	return;

outdata_error:
	AES_DBG("outdata_error ..\n");
	fh_unset_indata(dev);

indata_error:
	AES_DBG("indata_error ..\n");
	fh_aes_complete(dev, err);
	spin_unlock_irqrestore(&dev->lock, flags);

}

static void fh_aes_work_cb(struct work_struct *w)
{
	struct fh_aes_dev *dev = container_of(w, struct fh_aes_dev, work);
	struct crypto_async_request *async_req, *backlog;
	struct fh_aes_reqctx *reqctx;
	struct crypto_ablkcipher *p_tfm;
	struct af_alg_usr_def *p_usr_def;
	unsigned long flags;
	AES_DBG("%s\n", __func__);
	/*get the req need to handle*/
	spin_lock_irqsave(&dev->lock, flags);
	async_req = crypto_dequeue_request(&dev->queue);
	backlog = crypto_get_backlog(&dev->queue);
	spin_unlock_irqrestore(&dev->lock, flags);
	if (!async_req)
		return;
	init_completion(&dev->done);
	dev->req = ablkcipher_request_cast(async_req);
	p_tfm = crypto_ablkcipher_reqtfm(dev->req);
	p_usr_def = crypto_ablkcipher_usr_def(p_tfm);
	dev->p_usr_def = p_usr_def;
	dev->ctx = crypto_tfm_ctx(dev->req->base.tfm);
	reqctx = ablkcipher_request_ctx(dev->req);
	fh_aes_crypt_start(dev, reqctx->mode);
	wait_for_completion(&dev->done);
	fh_aes_rx(dev);
	fh_aes_tx(dev);
	fh_aes_complete(dev, 0);
	if (backlog) {
		if (backlog->complete)
			backlog->complete(backlog, -EINPROGRESS);
	}
	/*call the queue work until empty.*/
	if (dev->queue.qlen != 0)
		queue_work(dev->workqueue, &dev->work);
}

/*add chenjn dsp use...*/
typedef struct {
	unsigned int base;
	void *vbase;
	unsigned int size;
} MEM_INFO;
typedef struct {
	MEM_INFO mem;
	unsigned char *remap_base; /**<已用大小*/
} RW_MEM_INFO;

struct tcrypt_result {
	struct completion completion;
	int err;
};

int aes_128_ecb_encrypt(char *key_128, RW_MEM_INFO in,
RW_MEM_INFO out, unsigned int data_len_align16);

int fh_aes_ctl_mem_init(struct fh_aes_dev *pdata)
{
	unsigned int t1;
	unsigned int t2;
	unsigned int t3;
	unsigned int t4;

	t1 = (unsigned int)kmalloc(FH_AES_MALLOC_SIZE
	+ FH_AES_ALLIGN_SIZE, GFP_KERNEL);
	if (!t1)
		goto err1;

	t2 = (unsigned int)kmalloc(FH_AES_MALLOC_SIZE
	+ FH_AES_ALLIGN_SIZE, GFP_KERNEL);
	if (!t2)
		goto err2;


	t3 = ((t1 + FH_AES_ALLIGN_SIZE - 1) & (~(FH_AES_ALLIGN_SIZE - 1)));
	t4 = ((t2 + FH_AES_ALLIGN_SIZE - 1) & (~(FH_AES_ALLIGN_SIZE - 1)));

	pdata->ctl_raw_src_xbuf = (unsigned char *)t1;
	pdata->ctl_raw_dst_xbuf = (unsigned char *)t2;
	pdata->ctl_src_xbuf = (unsigned char *)t3;
	pdata->ctl_dst_xbuf = (unsigned char *)t4;
	return 0;
err2:
	kfree((void *)t1);
err1:
	return -1;

}

static int __devinit fh_aes_probe(struct platform_device *pdev)
{

	int i, j, err = -ENODEV;
	struct fh_aes_dev *pdata;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct resource *ioarea;

	AES_DBG("aes probe get in..\n");
	if (pobj_aes_dev) {
		dev_err(&pdev->dev, "second crypto dev..\n");
		return -EEXIST;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "get platform source error..\n");
		return -ENODEV;
	}

	ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "aes region already claimed\n");
		/*BUG_ON(ioarea);*/
		return -EBUSY;
	}

	pdata = kzalloc(sizeof(struct fh_aes_dev), GFP_KERNEL);
	if (!pdata) {
		err = -ENOMEM;
		goto err_malloc;
	}

	spin_lock_init(&pdata->lock);
	pdata->regs = ioremap(res->start, resource_size(res));

	if (!pdata->regs) {
		dev_err(&pdev->dev, "aes region already mapped\n");
		err = -EINVAL;
		goto err_iomap;
	}
	pdata->irq_no = platform_get_irq(pdev, 0);
	if (pdata->irq_no < 0) {
		err = pdata->irq_no;
		dev_warn(dev, "aes interrupt is not available.\n");
		goto err_irq;
	}
	/*only enable dma done isr..*/
	pdata->en_isr = 1 << 0;
	err = request_irq(pdata->irq_no, fh_aes_interrupt, 0,
			  dev_name(&pdev->dev), pdev);

	if (err) {
		dev_dbg(&pdev->dev, "request_irq failed, %d\n", err);
		goto err_irq;
	}
	/*bind to plat dev..*/
	pdata->dev = dev;
	/*bing to static para..only one aes controller in fh..*/
	pobj_aes_dev = pdata;
	platform_set_drvdata(pdev, pdata);

	pdata->workqueue = create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!pdata->workqueue) {
		dev_err(&pdev->dev, "aes workqueue init error.\n");
		goto err_irq;
	}
	INIT_WORK(&pdata->work, fh_aes_work_cb);


	crypto_init_queue(&pdata->queue, CRYPTO_QUEUE_LEN);
	for (i = 0; i < ARRAY_SIZE(algs); i++) {
		INIT_LIST_HEAD(&algs[i].cra_list);
		err = crypto_register_alg(&algs[i]);

		if (err) {
			dev_warn(dev, "register alg error...\n");
			goto err_algs;
		}
	}

	err = fh_aes_ctl_mem_init(pdata);
	if (err) {
		dev_err(&pdev->dev, "aes malloc mem error..\n");
		goto err_algs;
	}
	pr_info("aes driver registered\n");

#ifdef CONFIG_FH_AES_SELF_TEST

	fh_aes_self_test_all();
#endif

	return 0;
err_algs:
	for (j = 0; j < i; j++)
		crypto_unregister_alg(&algs[j]);
	destroy_workqueue(pdata->workqueue);
	platform_set_drvdata(pdev, NULL);
	pobj_aes_dev = NULL;
	free_irq(pdata->irq_no, pdata);

err_irq:
	iounmap(pdata->regs);

err_iomap:
	kfree(pdata);

err_malloc:
	release_mem_region(res->start, resource_size(res));
	return err;
}

static int __devexit fh_aes_remove(struct platform_device *pdev)
{

	int i;
	struct fh_aes_dev *pdata = platform_get_drvdata(pdev);
	struct resource *res;

	for (i = 0; i < ARRAY_SIZE(algs); i++)
		crypto_unregister_alg(&algs[i]);

	destroy_workqueue(pdata->workqueue);
	platform_set_drvdata(pdev, NULL);
	pobj_aes_dev = NULL;
	free_irq(pdata->irq_no, pdata);
	iounmap(pdata->regs);
	kfree(pdata->ctl_raw_src_xbuf);
	kfree(pdata->ctl_raw_dst_xbuf);
	pdata->ctl_raw_src_xbuf = NULL;
	pdata->ctl_raw_dst_xbuf = NULL;
	pdata->ctl_src_xbuf = NULL;
	pdata->ctl_dst_xbuf = NULL;
	kfree(pdata);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));

	return 0;
}

static struct platform_driver fh_aes_driver = {
	.driver = {
		.name = "fh_aes",
		.owner = THIS_MODULE,
	},
	.probe = fh_aes_probe,
	.remove = __devexit_p(fh_aes_remove),
};

static int __init fh_aes_init(void)
{
	return platform_driver_register(&fh_aes_driver);
}
late_initcall(fh_aes_init);

static void __exit fh_aes_exit(void)
{
	platform_driver_unregister(&fh_aes_driver);
}
module_exit(fh_aes_exit);

static void tcrypt_complete(struct crypto_async_request *req, int err)
{
	struct tcrypt_result *res = req->data;
	if (err == -EINPROGRESS)
		return;
	complete(&res->completion);
}

int aes_128_ecb_encrypt(char *key_128, RW_MEM_INFO in,
RW_MEM_INFO out, unsigned int data_len_align16)
{
	static char *xbuf;
	static char *dst_xbuf;
	static struct crypto_ablkcipher *tfm;
	static struct ablkcipher_request *req;
	static int malloc_flag;
	/*const char *algo = NULL;*/
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	void *data;
	void *dst_data;
	struct tcrypt_result wait_result;

	/*malloc buf...*/
	if (malloc_flag != 0)
		goto work_go;
	malloc_flag = 1;
	xbuf = (void *)__get_free_page(GFP_KERNEL);
	if (!xbuf) {
		printk("no pages.\n");
		return -1;
	}

	dst_xbuf = (void *)__get_free_page(GFP_KERNEL);
	if (!dst_xbuf) {
		free_page((unsigned long)xbuf);
		printk("no pages.\n");
		return -1;
	}

	tfm = crypto_alloc_ablkcipher("ecb-aes-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(tfm)) {
		printk("aes_test: failed to alloc cipher!\n");
		free_page((unsigned long)xbuf);
		free_page((unsigned long)dst_xbuf);
		return -1;
	}
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		printk(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for\n");
		return -1;
	}

work_go:
	init_completion(&wait_result.completion);
	crypto_ablkcipher_setkey(tfm, (u8 *)key_128, 16);
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &wait_result);
	data = xbuf;
	dst_data = dst_xbuf;
	/*encrypt*/
	memcpy(data, in.remap_base, data_len_align16);
	sg_init_one(&sg[0], data, data_len_align16);
	sg_init_one(&dst_sg[0], dst_data, data_len_align16);
	ablkcipher_request_set_crypt(req, sg, dst_sg, data_len_align16, NULL);
	crypto_ablkcipher_encrypt(req);
	wait_for_completion(&wait_result.completion);
	memcpy(out.remap_base, dst_data, data_len_align16);

	return 0;

}
EXPORT_SYMBOL(aes_128_ecb_encrypt);


MODULE_AUTHOR("yu.zhang <zhangy@fullhan.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fullhan AES driver support");
