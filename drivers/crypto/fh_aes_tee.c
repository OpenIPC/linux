#define pr_fmt(fmt) "%s %d: " fmt, __func__, __LINE__

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
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <linux/crc32.h>
#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <crypto/rng.h>
#include "fh_aes.h"
#include <crypto/skcipher.h>
#include <mach/io.h>
#include <tee_client_api.h>

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

#define CRYPTO_QUEUE_LEN    (1000)
#define CRYPTION_POS		(0)
#define METHOD_POS			(1)
#define EMODE_POS			(4)


#define FH_AES_ALLIGN_SIZE			64
#define FH_AES_MALLOC_SIZE			2048
#define FH_AES_CTL_MAX_PROCESS_SIZE		(FH_AES_MALLOC_SIZE)

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

/* optee TA UUID and cmds defines */
static TEEC_UUID TA_FH_AES_UUID = \
    { 0xdd00d7bd, 0xe9e3, 0x4dcb,\
    { 0xad, 0xbc, 0xda, 0x54, 0xa7, 0x1b, 0x9b, 0x65} };

#define TA_INVOKE_AES_PREPARE         1
#define TA_INVOKE_AES_SET_KEY         2
#define TA_INVOKE_AES_SET_IV          3
#define TA_INVOKE_AES_ENCRYPT         4
#define TA_INVOKE_AES_DECRYPT         5
#define TA_INVOKE_AES_START_TRANSFER  6

static TEEC_Context ctx;
static TEEC_Session sess;

static int tee_fh_aes_set_key(void *key, struct ex_key_map_para *key_map, size_t size)
{
	uint32_t paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];

	memset(params, 0, sizeof(params));
	params[0].tmpref.buffer = key;
	params[0].tmpref.size = size;

	if (key_map) {
		paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT,
						 TEEC_NONE, TEEC_NONE);
		params[1].tmpref.buffer = key_map;
		params[1].tmpref.size = sizeof(*key_map);
	}

	return tee_invoke_cmd(&sess, TA_INVOKE_AES_SET_KEY, paramTypes, params);
}

static int tee_fh_aes_set_iv(void *iv, size_t size)
{
	uint32_t paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];

	memset(params, 0, sizeof(params));

	params[0].tmpref.buffer = iv;
	params[0].tmpref.size = size;

	return tee_invoke_cmd(&sess, TA_INVOKE_AES_SET_IV, paramTypes, params);
}

static int tee_fh_aes_prepare(int type, int mode, int flag)
{
	uint32_t paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
					 TEEC_VALUE_INPUT, TEEC_NONE);
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];

	memset(params, 0, sizeof(params));

	params[0].value.a = type;
	params[1].value.a = mode;
	params[2].value.a = flag;

	pr_debug("%x %x %x\n", type, mode, flag);

	return tee_invoke_cmd(&sess, TA_INVOKE_AES_PREPARE, paramTypes, params);
}

static int tee_fh_aes_start_transfer(void)
{
	uint32_t paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];

	memset(params, 0, sizeof(params));


	pr_debug("aes start transfer\n");

	return tee_invoke_cmd(&sess, TA_INVOKE_AES_START_TRANSFER, paramTypes, params);
}

static int tee_fh_aes_crypto_action(void *src, void *dst, size_t size, int action)
{
	uint32_t paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	TEEC_Parameter params[TEEC_CONFIG_PAYLOAD_REF_COUNT];

	memset(params, 0, sizeof(params));
	params[0].tmpref.buffer = src;
	params[0].tmpref.size = size;
	params[1].tmpref.buffer = dst;
	params[1].tmpref.size = size;

	return tee_invoke_cmd(&sess, action, paramTypes, params);
}

static int tee_fh_aes_encrypt(void *src, void *dst, size_t size)
{
	return tee_fh_aes_crypto_action(src, dst, size, TA_INVOKE_AES_ENCRYPT);
}

static int tee_fh_aes_decrypt(void *src, void *dst, size_t size)
{
	return tee_fh_aes_crypto_action(src, dst, size, TA_INVOKE_AES_DECRYPT);
}


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
static int fh_set_indata(struct fh_aes_dev *dev, struct scatterlist *sg);
static int fh_set_outdata(struct fh_aes_dev *dev, struct scatterlist *sg);
static void fh_set_aes_key_reg(struct fh_aes_dev *dev, uint8_t *key,
			       uint8_t *iv, unsigned int keylen);
static void fh_unset_indata(struct fh_aes_dev *dev);
static void fh_unset_outdata(struct fh_aes_dev *dev);
static void fh_aes_complete(struct fh_aes_dev *dev, int err);
static void fh_aes_crypt_start(struct fh_aes_dev *dev, unsigned long mode);
static void fh_aes_work_cb(struct work_struct *w);
static void fh_aes_crypt_with_sg(struct fh_aes_dev *dev,
unsigned long mode, struct crypto_tfm *tfm);
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
static int fh_aes_test_func(void *arg)
{
	pr_info("%s\n", __func__);

	msleep(10000);

	fh_aes_self_test_all();
	return 0;
}
#endif

static unsigned int get_tfm_block_size(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_blocksize;
}
/* function body */
static int fh_aes_handle_req(struct fh_aes_dev *dev,
			     struct ablkcipher_request *req)
{
	unsigned long flags;
	int err;
	//*(int *)0 = 1;
	//BUG();
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

	AES_DBG("%s\n", __func__);
	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192
	    && keylen != AES_KEYSIZE_256 && keylen != DES_KEY_SIZE
	    && keylen != DES3_EDE_KEY_SIZE)
		return -EINVAL;

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
	/*cpy dri local buf to core sg buf*/
	unsigned char *dst_xbuf;
	u32 size = dev->ot_size;
	u32 cpy_size = 0;
	u32 i = 0;
	/* unmap driver buf first.*/
	fh_unset_outdata(dev);
	dst_xbuf = &dev->ctl_dst_xbuf[0];
	do {
		/*get the sg list offset left
		and the xfer size lower one.*/
		cpy_size = min_t(u32,
		sg_dma_len(dev->sg_dst) - dev->dst_sg_offset, size);
		sg_pcopy_from_buffer(dev->sg_dst, 1,
		&dst_xbuf[i], cpy_size, dev->dst_sg_offset);
		i += cpy_size;
		dev->dst_sg_offset += cpy_size;
		size -= cpy_size;
		if (dev->dst_sg_offset == sg_dma_len(dev->sg_dst)) {
			/*if sg offset get to the end of sg list.
			then find the next..*/
			dev->dst_sg_offset = 0;
			dev->sg_dst = sg_next(dev->sg_dst);
		}
	} while (size);

}

static void fh_aes_rx(struct fh_aes_dev *dev)
{
	fh_unset_indata(dev);
}

static int fh_set_indata(struct fh_aes_dev *dev, struct scatterlist *sg)
{
	/*cpy usr buf data to dri local buf*/
	unsigned char *src_xbuf;
	int err;
	u32 cpy_size = 0;
	u32 i = 0;
	u32 size = dev->ot_size;

	src_xbuf = &dev->ctl_src_xbuf[0];

	do {
		/*get the sg list offset left and the xfer size lower one.*/
		cpy_size = min_t(u32,
		sg_dma_len(dev->sg_src) - dev->src_sg_offset, size);
		sg_pcopy_to_buffer(dev->sg_src, 1,
		&src_xbuf[i], cpy_size, dev->src_sg_offset);
		i += cpy_size;
		dev->src_sg_offset += cpy_size;
		size -= cpy_size;
		if (dev->src_sg_offset == sg_dma_len(dev->sg_src)) {
			/*if sg offset get to the end of sg list.
			then find the next..*/
			dev->src_sg_offset = 0;
			dev->sg_src = sg_next(dev->sg_src);
		}
	} while (size);
	/* cpy core date to driver buf, map the driver sg*/
	sg_init_one(&dev->src_sg_array[0], &src_xbuf[0], dev->ot_size);
	err = dma_map_sg(dev->dev, &dev->src_sg_array[0], 1, DMA_TO_DEVICE);
	if (!err)
		BUG();
	return err;
}

static int fh_set_outdata(struct fh_aes_dev *dev, struct scatterlist *sg)
{
	int err;
	/*out data to driver buf, then cpy to core buf*/
	sg_init_one(&dev->dst_sg_array[0],
	&dev->ctl_dst_xbuf[0], dev->ot_size);
	err = dma_map_sg(dev->dev, &dev->dst_sg_array[0], 1, DMA_FROM_DEVICE);
	if (!err)
		BUG();
	return err;
}


static void fh_set_aes_key_reg(struct fh_aes_dev *dev, uint8_t *key,
			       uint8_t *iv, unsigned int keylen)
{
	u32 method;
	u32 key_size = 0;
	u32 iv_size = 0;
	void *key_map = NULL;

	if (dev->iv_flag == true) {
		/*set iv*/
		/*if aes mode ....set 128 bit iv,  des set 64bit iv..*/
		AES_DBG("set iv reg\n");
		if ((dev->control_reg & AES_128_METHOD)
		    || ((dev->control_reg & AES_192_METHOD))
		    || (dev->control_reg & AES_256_METHOD)) {
			AES_DBG("aes iv mode...\n");
			iv_size = 16;
		} else {
			AES_DBG("des iv mode...\n");
			iv_size = 8;
		}
		tee_fh_aes_set_iv(iv, iv_size);
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

	if (dev->p_usr_def->mode & CRYPTO_EX_MEM_SWITCH_KEY)
		key_map = &dev->p_usr_def->adv.ex_key_para;
	else
		key_map = NULL;
	/* printk("usr def data is %x\n",dev->p_usr_def->mode); */
	tee_fh_aes_set_key(key, key_map, key_size);
}

static void fh_unset_indata(struct fh_aes_dev *dev)
{
	dma_unmap_sg(dev->dev, &dev->src_sg_array[0], 1, DMA_TO_DEVICE);
}

static void fh_unset_outdata(struct fh_aes_dev *dev)
{
	dma_unmap_sg(dev->dev, &dev->dst_sg_array[0], 1, DMA_FROM_DEVICE);
}

static void fh_aes_complete(struct fh_aes_dev *dev, int err)
{
	if (dev->req->base.complete)
		dev->req->base.complete(&dev->req->base, err);
}

enum {
	CRYPT_TYPE_AES,
	CRYPT_TYPE_DES,
	CRYPT_TYPE_DES3,
};

static void fh_aes_crypt_start(struct fh_aes_dev *dev,
unsigned long mode)
{
	struct crypto_tfm *tfm;
	struct crypto_ablkcipher *p_tfm;
	struct ablkcipher_request *req;

	req = dev->req;
	p_tfm = crypto_ablkcipher_reqtfm(dev->req);
	tfm = crypto_ablkcipher_tfm(p_tfm);
	fh_aes_crypt_with_sg(dev, mode, tfm);

	return;

}

static u32 fh_aes_update_request_data_size(u32 max_xfer_size,
u32 *total_size, u32 *first)
{
	u32 step_size = 0;

	step_size = min_t(u32, *total_size, max_xfer_size);
	if (*first == 0)
		*first = 1;

	*total_size -= step_size;
	return step_size;
}

static void fh_aes_crypt_with_sg(struct fh_aes_dev *dev,
		unsigned long mode, struct crypto_tfm *tfm)
{
	struct ablkcipher_request *req = dev->req;
	u32 control_reg = 0;
	int err;
	int crypto_type, crypto_mode;

	u32 total_size;
	u32 block_size_limit;
	u32 ot_len;
	u32 first_in = 0;

	total_size = req->nbytes;
	block_size_limit = get_tfm_block_size(tfm);
	dev->sg_src = req->src;
	dev->sg_dst = req->dst;
	dev->src_sg_offset = 0;
	dev->dst_sg_offset = 0;

	err = tee_open_session(&ctx, &sess, &TA_FH_AES_UUID);
	if (err) {
		pr_err("open_session failed: %d\n", err);
		return;
	}

	while (total_size) {
		if (((mode & CBC_MODE) || (mode & CTR_MODE) || (mode & CFB_MODE)
		|| (mode & OFB_MODE)) && (first_in == 0)) {
			control_reg |= 1 << 7;
			dev->iv_flag = true;
		} else {
			control_reg &= ~(1 << 7);
			dev->iv_flag = false;
		}

		/*emode & method*/
		control_reg |= (unsigned int) mode;
		dev->control_reg = control_reg;

		crypto_mode = (mode & 0x7 << EMODE_POS);
		crypto_type = mode & 0x0e;
		if (crypto_type == DES_METHOD)
			crypto_type = CRYPT_TYPE_DES;
		else if (crypto_type == TRIPLE_DES_METHOD)
			crypto_type = CRYPT_TYPE_DES3;
		else
			crypto_type = CRYPT_TYPE_AES;

		if (first_in == 0) {
			tee_fh_aes_prepare(crypto_type, crypto_mode, dev->p_usr_def->mode);
			fh_set_aes_key_reg(dev, dev->ctx->aes_key, req->info, dev->ctx->keylen);
			tee_fh_aes_start_transfer();
		}

		ot_len = fh_aes_update_request_data_size(
			FH_AES_CTL_MAX_PROCESS_SIZE, &total_size, &first_in);
		if (ot_len % block_size_limit)
			BUG();
		dev->ot_size = ot_len;

		fh_set_indata(dev, &dev->src_sg_array[0]);
		fh_set_outdata(dev, &dev->dst_sg_array[0]);

		if ((mode & 0x1) == 0)
			err = tee_fh_aes_encrypt(dev->ctl_src_xbuf, dev->ctl_dst_xbuf, ot_len);
		else
			err = tee_fh_aes_decrypt(dev->ctl_src_xbuf, dev->ctl_dst_xbuf, ot_len);
		if (err)
			goto error;

		/*update dst sg...*/
		fh_aes_rx(dev);
		fh_aes_tx(dev);
	}

error:
	tee_close_session(&ctx, &sess);
}

static void fh_aes_work_cb(struct work_struct *w)
{
	struct fh_aes_dev *dev = container_of(w, struct fh_aes_dev, work);
	struct crypto_async_request *async_req, *backlog;
	struct fh_aes_reqctx *reqctx;
	struct crypto_ablkcipher *p_tfm;
	struct af_alg_usr_def *p_usr_def;
	unsigned long flags;
	struct ablkcipher_request *p_ablk_req;

	AES_DBG("%s\n", __func__);
	/*get the req need to handle*/
	spin_lock_irqsave(&dev->lock, flags);
	async_req = crypto_dequeue_request(&dev->queue);
	backlog = crypto_get_backlog(&dev->queue);
	spin_unlock_irqrestore(&dev->lock, flags);
	if (!async_req)
		return;

	dev->req = ablkcipher_request_cast(async_req);
	p_tfm = crypto_ablkcipher_reqtfm(dev->req);
	p_ablk_req = dev->req;
	p_usr_def = p_ablk_req->usr_def;
	//p_usr_def = crypto_ablkcipher_usr_def(p_tfm);
	dev->p_usr_def = p_usr_def;
	dev->ctx = crypto_tfm_ctx(dev->req->base.tfm);
	reqctx = ablkcipher_request_ctx(dev->req);
	fh_aes_crypt_start(dev, reqctx->mode);
	fh_aes_complete(dev, 0);
	if (backlog) {
		if (backlog->complete)
			backlog->complete(backlog, -EINPROGRESS);
	}
	/*call the queue work until empty.*/
	if (dev->queue.qlen != 0)
		queue_work(dev->workqueue, &dev->work);
}


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

static int fh_aes_probe(struct platform_device *pdev)
{
	int i, j, err = -ENODEV;
	struct fh_aes_dev *pdata;
	struct device *dev = &pdev->dev;
	u32 uuid_array[11];
	TEEC_UUID uuid;
#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;

	err = of_property_read_u32_array(np, "optee-uuid", uuid_array, 11);
	if (err) {
		dev_err(&pdev->dev, "error when parse optee-uuid %s", np->full_name);
		return err;
	} else {
#else
	void *plat_data = dev_get_platdata(&pdev->dev);

	if (!plat_data) {
		dev_err(&pdev->dev, "cannot get optee-uuid from plat_data");
	} else {
		memcpy(uuid_array, plat_data, sizeof(uuid_array));
#endif

		uuid = get_uuid_from_array(uuid_array);
		dev_info(&pdev->dev,
			"aes-uuid: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			uuid.timeLow, uuid.timeMid, uuid.timeHiAndVersion,
			uuid.clockSeqAndNode[0], uuid.clockSeqAndNode[1],
			uuid.clockSeqAndNode[2], uuid.clockSeqAndNode[3],
			uuid.clockSeqAndNode[4], uuid.clockSeqAndNode[5],
			uuid.clockSeqAndNode[6], uuid.clockSeqAndNode[7]);
	}

	TA_FH_AES_UUID = uuid;

	pdata = kzalloc(sizeof(struct fh_aes_dev), GFP_KERNEL);
	if (!pdata) {
		err = -ENOMEM;
		goto err_malloc;
	}
	spin_lock_init(&pdata->lock);

	pdata->regs = NULL;

	pdata->en_isr = 1 << 0;

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
	dev_info(&pdev->dev, "aes driver registered\n");


#ifdef CONFIG_FH_AES_SELF_TEST
#include <linux/kthread.h>
	kthread_run(fh_aes_test_func, NULL, "aes_self_test");
#endif

	return 0;
err_algs:
	for (j = 0; j < i; j++)
		crypto_unregister_alg(&algs[j]);
	destroy_workqueue(pdata->workqueue);
	platform_set_drvdata(pdev, NULL);
	pobj_aes_dev = NULL;

err_irq:
	kfree(pdata);
err_malloc:
	pr_info("%s %d\n", __func__, __LINE__);

	return err;
}


static int fh_aes_remove(struct platform_device *pdev)
{
	int i;
	struct fh_aes_dev *pdata = platform_get_drvdata(pdev);

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

	return 0;
}

static const struct of_device_id fh_aes_of_match[] = {
	{.compatible = "fh,fh-aes",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_aes_of_match);

static struct platform_driver fh_aes_driver = {
	.driver = {
		.name = "fh_aes",
		.of_match_table = fh_aes_of_match,
	},
	.probe = fh_aes_probe,
	.remove = fh_aes_remove,
};

module_platform_driver(fh_aes_driver);
MODULE_AUTHOR("FULLHAN");
MODULE_DESCRIPTION("fullhan AES device driver for tee");
MODULE_LICENSE("GPL");
