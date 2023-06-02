/*
 * ambarella_crypto.c
 *
 * History:
 *	2009/09/07 - [Qiao Wang]
 *    2013/01/04 - [Johnson Diao]
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/cryptohash.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/sha.h>
#include <crypto/md5.h>
#include <crypto/internal/hash.h>
#include <mach/hardware.h>
#include <plat/crypto.h>

DEFINE_MUTEX(engine_lock);
#define AMBA_CIPHER_COPY (1<<4)

static DECLARE_COMPLETION(g_aes_irq_wait);
static DECLARE_COMPLETION(g_des_irq_wait);
static DECLARE_COMPLETION(g_md5_sha1_irq_wait);

static struct workqueue_struct *handle_queue;
static struct work_struct work;

static int config_polling_mode = 0;
module_param(config_polling_mode, int, S_IRUGO);

static const char *ambdev_name =
	"Ambarella Media Processor Cryptography Engine";

struct des_ctx {
	u32 expkey[DES_EXPKEY_WORDS];
};
#define AMBA_AES_BLOCK 16
static md5_digest_t g_md5_digest __attribute__((__aligned__(4)));
static md5_data_t g_md5_data __attribute__((__aligned__(4)));
static sha1_digest_t g_sha1_digest __attribute__((__aligned__(4)));
static sha1_data_t g_sha1_data __attribute__((__aligned__(4)));
/******************* Basic Function ***************************************/
static unsigned long long int __ambarella_crypto_aligned_read64(u32 src)
{
	long long unsigned int ret;
	__asm__ __volatile__ (
	"ldrd %0,[%1]\n"
	: "=&r"(ret)
	: "r"(src)
	 );
	return ret;
}

static void __ambarella_crypto_aligned_write64(u32 low, u32 high,u32 dst)
{
	__asm__ __volatile__ (
	"mov r4, %0 \n\t"
	"mov r5, %1 \n\t"
	"strd r4, [%2] \n\t"
	:
	: "r"(low) , "r"(high),"r"(dst)
	: "r4", "r5", "memory" );
}

static int _ambarella_crypto_aligned_read64(u32 * buf,u32 * addr, unsigned int len) {
	int errCode = -1;
	int i;
	if ( ( 0 == (((u32) addr) & 0x07) ) && (0 == (len%8))) {
		/* address and length should be 64 bit aligned */
		for (i = 0; i < len/8; i++) {
			*(unsigned long long int *)(buf + 2*i) = __ambarella_crypto_aligned_read64((u32)(addr + 2*i));
		}
		errCode = 0;
	};
	return errCode;

}

static int _ambarella_crypto_aligned_write64(u32 * addr,u32 * buf, unsigned int len ) {
	int errCode = -1;
	int i;
	if ( ( 0 == (((u32) addr) & 0x07) ) && (0 == (len%8))) {
		/* address and length should be 64 bit aligned */
		for (i = 0; i < len/8; i++) {
			__ambarella_crypto_aligned_write64(*(u32 *)(buf + 2*i), *(u32 *)(buf + 2*i+1),(u32)(addr + 2*i));
		}
		errCode = 0;
	};
	return errCode;

}

//merge temp buffer
#define MAX_MERGE_SIZE 8
__attribute__ ((aligned (4))) u32 merg[MAX_MERGE_SIZE]={};

struct ambarella_crypto_info {
	void __iomem *regbase;

	bool binary_mode;
	bool cap_md5_sha1;
	bool data_swap;
	bool reg_64bit;
};
struct aes_fun_t{
	void (*opcode)(u32);
	void (*wdata)(u32*,u32*,int);
	void (*rdata)(u32*,u32*,int);
	u32*  (*reg_enc)(void);
	u32* (*reg_dec)(void);
}aes_fun;
u32* aes_reg_enc_dec_32(void)
{
	 return((u32*)CRYPT_A_INPUT1);
}
u32* aes_reg_enc_64(void)
{
	return( (u32*)CRYPT_A_INPUT1);
}

u32* aes_reg_dec_64(void)
{
	// When run the a5s or other with 32-bit register,this defined is NULL ,
	//so make sure correct in ambarella_crypto_probe
	return( (u32*)CRYPT_A_INPUT2);
}

void swap_write(u32 *offset,u32 *src,int size)
{
	int point=0;
	int len=size;

	for(size=(size>>2)-1;size >= 0;size--,point++){
		*(merg+point) = ntohl(*(src+size));
	}
	//the iONE registers need to accessed by 64-bit boundaries
	_ambarella_crypto_aligned_write64(offset,merg,len);

}

void swap_read(u32 *offset,u32 *src,int size)
{
	int point=0,len=size;

	//the iONE registers need to accessed by 64-bit boundaries
	_ambarella_crypto_aligned_read64(merg,src,len);
	for(size=(size>>2)-1;size >= 0;size--,point++){
		*(offset+point) = ntohl(*(merg+size));
	}
}

void aes_opcode(u32 flag)
{
	// When run the a7 or higher version with 64-bit  register,this defined is NULL,
	//so make sure correct in ambarella_crypto_probe
	amba_writel(CRYPT_A_OPCODE, flag);
}

void null_fun(u32 flag) {}
/***************** AES Function **********************************/
static void aes_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	struct crypto_aes_ctx *ctx=crypto_tfm_ctx(tfm);
	const __le32 *src = (const __le32 *)in;
	__le32 *dst = (__le32 *)out;
	u32 ready;
	u32 *offset=NULL;
	do{}while(mutex_trylock(&engine_lock) == 0);
	switch (ctx->key_length){
	case 16:
		offset = (u32*)CRYPT_A_128_96_REG;
		aes_fun.wdata(offset,ctx->key_enc,16);
		break;
	case 24:
		offset = (u32*)CRYPT_A_192_160_REG;
		aes_fun.wdata(offset,ctx->key_enc,24);
		break;
	case 32:
		offset = (u32*)CRYPT_A_256_224_REG;
		aes_fun.wdata(offset,ctx->key_enc,32);
		break;
	}
	//enc or dec option mode
	aes_fun.opcode(AMBA_HW_ENCRYPT_CMD);

	//get the input offset
	offset = aes_fun.reg_enc();

	//input the src
	aes_fun.wdata(offset,(u32*)src,16);

	do{
		ready = amba_readl(CRYPT_A_OUTPUT_READY_REG);
	}while(ready != 1);

	//get the output
	offset = (u32*)CRYPT_A_OUTPUT_96_REG;
	aes_fun.rdata(dst,offset,16);
	mutex_unlock(&engine_lock);
}


static void aes_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	struct crypto_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	const __le32 *src = (const __le32 *)in;
	__le32 *dst = (__le32 *)out;
	u32 ready;
	u32 *offset=NULL;

	do{}while(mutex_trylock(&engine_lock) == 0);
	switch (ctx->key_length) {
	case 16:
		offset = (u32*)CRYPT_A_128_96_REG;
		aes_fun.wdata(offset,ctx->key_enc,16);
	        break;
	case 24:
		offset = (u32*)CRYPT_A_192_160_REG;
		aes_fun.wdata(offset,ctx->key_enc,24);
	        break;
	case 32:
		offset = (u32*)CRYPT_A_256_224_REG;
		aes_fun.wdata(offset,ctx->key_enc,32);
	        break;
	}
	//enc or dec option mode
	aes_fun.opcode(AMBA_HW_DECRYPT_CMD);

	//get the input offset
	offset = aes_fun.reg_dec();

	//input the src
	aes_fun.wdata(offset,(u32*)src,16);

	do{
		ready = amba_readl(CRYPT_A_OUTPUT_READY_REG);
	}while(ready != 1);


	//get the output
	offset = (u32*)CRYPT_A_OUTPUT_96_REG;
	aes_fun.rdata(dst,offset,16);
	mutex_unlock(&engine_lock);
}

static struct crypto_alg aes_alg = {
	.cra_name		=	"aes",
	.cra_driver_name	=	"aes-ambarella",
	.cra_priority		=	AMBARELLA_CRA_PRIORITY,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	AES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct crypto_aes_ctx),
	.cra_alignmask		=	AMBARELLA_CRYPTO_ALIGNMENT - 1,
	.cra_module		=	THIS_MODULE,
	.cra_list		=	LIST_HEAD_INIT(aes_alg.cra_list),
	.cra_u			=	{
		.cipher = {
			.cia_min_keysize	=	AES_MIN_KEY_SIZE,
			.cia_max_keysize	=	AES_MAX_KEY_SIZE,
			.cia_setkey	   		= 	crypto_aes_set_key,
			.cia_encrypt	 	=	aes_encrypt,
			.cia_decrypt	  	=	aes_decrypt,
		}
	}
};

/****************** DES Function **************************/
struct des_fun_t{
	void (*opcode)(u32);
	void (*wdata)(u32*,u32*,int);
	void (*rdata)(u32*,u32*,int);
	u32* (*reg_enc)(void);
	u32* (*reg_dec)(void);
}des_fun;

u32* des_reg_enc_dec_32(void)
{
	return((u32*)CRYPT_D_INPUT1);
}

u32* des_reg_enc_64(void)
{
	return((u32*)CRYPT_D_INPUT1);
}

u32* des_reg_dec_64(void)
{
	// When run the a5s or other with 32-bit register,this defined is NULL ,
	//so make sure correct in ambarella_crypto_probe
	return((u32*)CRYPT_D_INPUT2);
}

void des_opcode(u32 flag)
{
	// When run the a7 or higher version with 64-bit  register,this defined is NULL,
	//so make sure correct in ambarella_crypto_probe
	amba_writel(CRYPT_D_OPCODE, flag);
}

static int des_setkey(struct crypto_tfm *tfm, const u8 *key,
		      unsigned int keylen)
{
	struct des_ctx *dctx = crypto_tfm_ctx(tfm);
	u32 *flags = &tfm->crt_flags;
	u32 tmp[DES_EXPKEY_WORDS];
	int ret;


	/* Expand to tmp */
	ret = des_ekey(tmp, key);

	if (unlikely(ret == 0) && (*flags & CRYPTO_TFM_REQ_WEAK_KEY)) {
		*flags |= CRYPTO_TFM_RES_WEAK_KEY;
		return -EINVAL;
	}

	/* Copy to output */
	memcpy(dctx->expkey, key, keylen);

	return 0;
}

static void des_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	struct des_ctx *ctx = crypto_tfm_ctx(tfm);
	const __le32 *src = (const __le32 *)in;
	__le32 *dst = (__le32 *)out;
	u32 ready;
	u32 *offset=NULL;

	do{}while(mutex_trylock(&engine_lock) == 0);
	//set key
	des_fun.wdata((u32*)CRYPT_D_HI_REG,ctx->expkey,8);

	//enc or dec option mode
	des_fun.opcode(AMBA_HW_ENCRYPT_CMD);

	//get the input offset
	offset = des_fun.reg_enc();

	//input the src
	des_fun.wdata(offset,(u32*)src,8);

	do{
		ready = amba_readl(CRYPT_D_OUTPUT_READY_REG);
	}while(ready != 1);


	//get the output
	offset = (u32*)CRYPT_D_OUTPUT_HI_REG;
	des_fun.rdata(dst,offset,8);
	mutex_unlock(&engine_lock);
}

static void des_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	struct des_ctx *ctx = crypto_tfm_ctx(tfm);
	const __le32 *src = (const __le32 *)in;
	__le32 *dst = (__le32 *)out;
	u32 ready;
	u32 *offset=NULL;

	do{}while(mutex_trylock(&engine_lock) == 0);
	//set key
	des_fun.wdata((u32*)CRYPT_D_HI_REG,ctx->expkey,8);

	//enc or dec option mode
	des_fun.opcode(AMBA_HW_DECRYPT_CMD);

	//get the input offset
	offset = des_fun.reg_dec();

	//input the src
	des_fun.wdata(offset,(u32*)src,8);

	do{
		ready = amba_readl(CRYPT_D_OUTPUT_READY_REG);
	}while(ready != 1);

	//get the output
	offset = (u32*)CRYPT_D_OUTPUT_HI_REG;
	des_fun.rdata(dst,offset,8);
	mutex_unlock(&engine_lock);

}


static struct crypto_alg des_alg = {
	.cra_name		=	"des",
	.cra_driver_name	=	"des-ambarella",
	.cra_priority		=	AMBARELLA_CRA_PRIORITY,
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	DES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct des_ctx),
	.cra_module		=	THIS_MODULE,
	.cra_alignmask		=	AMBARELLA_CRYPTO_ALIGNMENT - 1,
	.cra_list		=	LIST_HEAD_INIT(des_alg.cra_list),
	.cra_u			=	{
		.cipher = {
			.cia_min_keysize	=	DES_KEY_SIZE,
			.cia_max_keysize	=	DES_KEY_SIZE,
			.cia_setkey		=	des_setkey,
			.cia_encrypt		=	des_encrypt,
			.cia_decrypt		=	des_decrypt
		}
	}
};
/******************** ECB(AES) Function ***************************************************/

#define MAX_BLOCK 8192
#define AES_MAX_KEYLENGTH	(15 * 16)
#define AES_MAX_KEYLENGTH_U32	(AES_MAX_KEYLENGTH / sizeof(u32))
struct amba_ecb_ctx {
	u32 enc_key[AES_MAX_KEYLENGTH_U32];
	int key_len;
};

struct request_ops{
	enum{
		enc,
		dec,
	}op;
	struct ablkcipher_walk walk;
};

static struct pri_queue {
	struct crypto_queue queue;
	spinlock_t lock;

	enum {
		IDLE,
		BUSY,
	}status;

}amba_queue;

static void  handle_aes_encrypt(struct amba_ecb_ctx *ctx, u8 *out, u8* in)
{
	u32 *offset=NULL;
	__le32 *src = (__le32 *)in;
	__le32 *dst = (__le32 *)out;
	int ready=0;

	switch (ctx->key_len){
	case 16:
		offset = (u32*)CRYPT_A_128_96_REG;
		aes_fun.wdata(offset,ctx->enc_key,16);
		break;
	case 24:
		offset = (u32*)CRYPT_A_192_160_REG;
		aes_fun.wdata(offset,ctx->enc_key,24);
		break;
	case 32:
		offset = (u32*)CRYPT_A_256_224_REG;
		aes_fun.wdata(offset,ctx->enc_key,32);
		break;
	}
	aes_fun.opcode(AMBA_HW_ENCRYPT_CMD);
	offset = aes_fun.reg_enc();
	aes_fun.wdata(offset,(u32*)src,16);

	do{
		ready = try_wait_for_completion(&g_aes_irq_wait);
	}while(ready != 1);

	offset = (u32*)CRYPT_A_OUTPUT_96_REG;
	aes_fun.rdata(dst,offset,16);

}
static void handle_aes_decrypt(struct amba_ecb_ctx *ctx, u8 *out, u8* in)
{
	u32 *offset=NULL;
	__le32 *src = (__le32 *)in;
	__le32 *dst = (__le32 *)out;
	int ready = 0;


	switch (ctx->key_len){
	case 16:
		offset = (u32*)CRYPT_A_128_96_REG;
		aes_fun.wdata(offset,ctx->enc_key,16);
		break;
	case 24:
		offset = (u32*)CRYPT_A_192_160_REG;
		aes_fun.wdata(offset,ctx->enc_key,24);
		break;
	case 32:
		offset = (u32*)CRYPT_A_256_224_REG;
		aes_fun.wdata(offset,ctx->enc_key,32);
		break;
	}

	aes_fun.opcode(AMBA_HW_DECRYPT_CMD);
	offset = aes_fun.reg_dec();
	aes_fun.wdata(offset,(u32*)src,16);

	do{
		ready = try_wait_for_completion(&g_aes_irq_wait);
	}while(ready != 1);

	offset = (u32*)CRYPT_A_OUTPUT_96_REG;
	aes_fun.rdata(dst,offset,16);

}

static int handle_ecb_aes_req(struct ablkcipher_request *req)
{
	int nbytes,err;
	struct amba_ecb_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
	struct crypto_async_request *base = &req->base;
	struct request_ops *req_ctx = ablkcipher_request_ctx(req);
	struct crypto_tfm *tfm = req->base.tfm;
	struct blkcipher_desc desc = {
		.tfm = __crypto_blkcipher_cast(tfm),
		.info = req->info,
		.flags = req->base.flags,
	};
	void (*fun)(struct amba_ecb_ctx *ctx, u8 *out, u8* in);
	struct blkcipher_walk walk;

	walk.flags |= AMBA_CIPHER_COPY;
	blkcipher_walk_init(&walk, req->dst, req->src, req->nbytes);


	if (req_ctx->op == enc){
		fun = handle_aes_encrypt;
	}else{
		fun = handle_aes_decrypt;
	}


	err = blkcipher_walk_virt(&desc, &walk);

	while ((nbytes = walk.nbytes)) {
			u8 *wsrc = walk.src.virt.addr;
			u8 *wdst = walk.dst.virt.addr;

			do {
				fun(ctx, wdst, wsrc);

				wsrc += AMBA_AES_BLOCK;
				wdst += AMBA_AES_BLOCK;
			} while ((nbytes -= AMBA_AES_BLOCK) >= AMBA_AES_BLOCK);

			err = blkcipher_walk_done(&desc, &walk, nbytes);
	}


	local_bh_disable();
	base->complete(base,err);
	local_bh_enable();
	return 0;
}

static void do_queue(void)
{
	struct crypto_async_request *async_req = NULL;
	struct crypto_async_request *backlog=NULL;
	spin_lock_irq(&amba_queue.lock);
	if (amba_queue.status == IDLE){
		backlog = crypto_get_backlog(&amba_queue.queue);
		async_req = crypto_dequeue_request(&amba_queue.queue);
		if(async_req){
			BUG_ON(amba_queue.status != IDLE);
			amba_queue.status = BUSY;
		}
	}
	spin_unlock_irq(&amba_queue.lock);
	if (backlog){
		backlog->complete(backlog,-EINPROGRESS);
		backlog = NULL;
	}
	if (async_req){
		struct ablkcipher_request *req =
				container_of(async_req,
						struct ablkcipher_request,
						base);
		// handle the request
		handle_ecb_aes_req(req);
		amba_queue.status = IDLE;
	}
	return;
}

static int ecb_aes_set_key(struct crypto_ablkcipher *cipher,const u8 *key,
			unsigned int len)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct amba_ecb_ctx *ctx = crypto_tfm_ctx(tfm);
	switch (len){
	case 16:
	case 24:
	case 32:
		break;
	default:
		crypto_ablkcipher_set_flags(cipher,CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}
	ctx->key_len = len;
	memcpy(ctx->enc_key,key,len);
	return 0;
}

static int amba_handle_req(struct ablkcipher_request *req)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&amba_queue.lock,flags);
	ret = crypto_enqueue_request(&amba_queue.queue,&req->base);
	spin_unlock_irqrestore(&amba_queue.lock,flags);

	queue_work(handle_queue,&work);

	return ret;
}

static int ecb_aes_encrypt(struct ablkcipher_request *req)
{
	struct request_ops *req_ctx = ablkcipher_request_ctx(req);
	req_ctx->op = enc;
	return amba_handle_req(req);
}

static int ecb_aes_decrypt(struct ablkcipher_request *req)
{
	struct request_ops *req_ctx = ablkcipher_request_ctx(req);
	req_ctx->op = dec;
	return amba_handle_req(req);
}

static int aes_cra_init(struct crypto_tfm *tfm)
{
	return 0;
}


static struct crypto_alg ecb_aes_alg = {
	.cra_name		=	"ecb(aes)",
	.cra_driver_name	=	"ecb-aes-ambarella",
	.cra_priority		=	AMBARELLA_COMPOSITE_PRIORITY,
	.cra_flags		=	CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
	.cra_blocksize		=	AES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct amba_ecb_ctx),
	.cra_alignmask		=	AMBARELLA_CRYPTO_ALIGNMENT - 1,
	.cra_type		=	&crypto_ablkcipher_type,
	.cra_module		=	THIS_MODULE,
	.cra_init		= aes_cra_init,
	.cra_u			=	{
		.ablkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.setkey	   		= 	ecb_aes_set_key,
			.encrypt		=	ecb_aes_encrypt,
			.decrypt		=	ecb_aes_decrypt,
		}
	}
};

static int cbc_aes_encrypt(struct blkcipher_desc *desc,
			   struct scatterlist *dst, struct scatterlist *src,
			   unsigned int nbytes)
{
	int err = 0;

	return err;
}

static int cbc_aes_decrypt(struct blkcipher_desc *desc,
			   struct scatterlist *dst, struct scatterlist *src,
			   unsigned int nbytes)
{
	int err = 0;

	return err;
}

static struct crypto_alg cbc_aes_alg = {
	.cra_name		=	"cbc(aes)",
	.cra_driver_name	=	"cbc-aes-ambarella",
	.cra_priority		=	AMBARELLA_COMPOSITE_PRIORITY,
	.cra_flags		=	CRYPTO_ALG_TYPE_BLKCIPHER,
	.cra_blocksize		=	AES_BLOCK_SIZE,
	.cra_ctxsize		=	sizeof(struct crypto_aes_ctx),
	.cra_alignmask		=	AMBARELLA_CRYPTO_ALIGNMENT - 1,
	.cra_type		=	&crypto_blkcipher_type,
	.cra_module		=	THIS_MODULE,
	.cra_list		=	LIST_HEAD_INIT(cbc_aes_alg.cra_list),
	.cra_u			=	{
		.blkcipher = {
			.min_keysize		=	AES_MIN_KEY_SIZE,
			.max_keysize		=	AES_MAX_KEY_SIZE,
			.ivsize			=	AES_BLOCK_SIZE,
			.setkey	   		= 	crypto_aes_set_key,
			.encrypt		=	cbc_aes_encrypt,
			.decrypt		=	cbc_aes_decrypt,
		}
	}
};
/************************************MD5 START**************************/

struct md5_sha1_fun_t{
	void (*wdata)(u32*,u32*,int);
	void (*rdata)(u32*,u32*,int);
}md5_sha1_fun;

static void ambarella_md5_transform(u32 *hash, u32 const *in)
{
	u32 ready;
	do{}while(mutex_trylock(&engine_lock) == 0);

	memcpy(&g_md5_digest.digest_0, hash, 16);

	do{
		ready= amba_readl(CRYPT_MD5_SHA1_READY_INPUT);
	}while(ready != 1);

	md5_sha1_fun.wdata((u32 *)CRYPT_MD5_INIT_31_0,&(g_md5_digest.digest_0),16);

	do{
		ready= amba_readl(CRYPT_MD5_SHA1_READY_INPUT);
	}while(ready != 1);

	memcpy(&g_md5_data.data[0], in, 64);

	md5_sha1_fun.wdata((u32 *)CRYPT_MD5_INPUT_31_0,&(g_md5_data.data[0]),64);

	if(unlikely(config_polling_mode == 0)) {
		do{
			ready = try_wait_for_completion(&g_md5_sha1_irq_wait);
		}while(ready != 1);
	}else{
		do{
			ready = amba_readl(CRYPT_MD5_OUTPUT_READY);
		}while(ready != 1);
	}

	md5_sha1_fun.rdata(&(g_md5_digest.digest_0),(u32 *)CRYPT_MD5_OUTPUT_31_0, 16);

	memcpy(hash, &g_md5_digest.digest_0, 16);

	mutex_unlock(&engine_lock);
}

static inline void le32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__le32_to_cpus(buf);
		buf++;
	}
}

static inline void cpu_to_le32_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__cpu_to_le32s(buf);
		buf++;
	}
}

static inline void ambarella_md5_transform_helper(struct md5_state *ctx)
{
	le32_to_cpu_array(ctx->block, sizeof(ctx->block)/sizeof(u32));
	ambarella_md5_transform(ctx->hash, ctx->block);
}

static int ambarella_md5_init(struct shash_desc *desc)
{
	struct md5_state *mctx = shash_desc_ctx(desc);
	mctx->hash[0] = 0x67452301;
	mctx->hash[1] = 0xefcdab89;
	mctx->hash[2] = 0x98badcfe;
	mctx->hash[3] = 0x10325476;
	mctx->byte_count = 0;
	return 0;
}

static int ambarella_md5_update(struct shash_desc *desc, const u8 *data, unsigned int len)
{
	struct md5_state *mctx = shash_desc_ctx(desc);
	const u32 avail = sizeof(mctx->block) - (mctx->byte_count & 0x3f);
	mctx->byte_count += len;

	if (avail > len) {
		memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
		       data, len);
		return 0;
	}

	memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
	       data, avail);

	ambarella_md5_transform_helper(mctx);
	data += avail;
	len -= avail;

	while (len >= sizeof(mctx->block)) {
		memcpy(mctx->block, data, sizeof(mctx->block));
		ambarella_md5_transform_helper(mctx);
		data += sizeof(mctx->block);
		len -= sizeof(mctx->block);
	}

	memcpy(mctx->block, data, len);

	return 0;
}

static int ambarella_md5_final(struct shash_desc *desc, u8 *out)
{
	struct md5_state *mctx = shash_desc_ctx(desc);
	const unsigned int offset = mctx->byte_count & 0x3f;
	char *p = (char *)mctx->block + offset;
	int padding = 56 - (offset + 1);
	*p++ = 0x80;
	if (padding < 0) {
		memset(p, 0x00, padding + sizeof (u64));
		ambarella_md5_transform_helper(mctx);
		p = (char *)mctx->block;
		padding = 56;
	}

	memset(p, 0, padding);
	mctx->block[14] = mctx->byte_count << 3;
	mctx->block[15] = mctx->byte_count >> 29;
	le32_to_cpu_array(mctx->block, (sizeof(mctx->block) -
	                  sizeof(u64)) / sizeof(u32));
	ambarella_md5_transform(mctx->hash, mctx->block);
	cpu_to_le32_array(mctx->hash, sizeof(mctx->hash) / sizeof(u32));
	memcpy(out, mctx->hash, sizeof(mctx->hash));
	memset(mctx, 0, sizeof(*mctx));
	return 0;
}

static int ambarella_md5_export(struct shash_desc *desc, void *out)
{
	struct md5_state *ctx = shash_desc_ctx(desc);
	memcpy(out, ctx, sizeof(*ctx));
	return 0;
}

static int ambarella_md5_import(struct shash_desc *desc, const void *in)
{
	struct md5_state *ctx = shash_desc_ctx(desc);
	memcpy(ctx, in, sizeof(*ctx));
	return 0;
}

static struct shash_alg md5_alg = {
	.digestsize	=	MD5_DIGEST_SIZE,
	.init		=	ambarella_md5_init,
	.update		=	ambarella_md5_update,
	.final		=	ambarella_md5_final,
	.export		=	ambarella_md5_export,
	.import		=	ambarella_md5_import,
	.descsize	=	sizeof(struct md5_state),
	.statesize	=	sizeof(struct md5_state),
	.base		=	{
		.cra_name	=	"md5",
		.cra_driver_name=	"md5-ambarella",
		.cra_flags	=	CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize	=	MD5_HMAC_BLOCK_SIZE,
		.cra_module	=	THIS_MODULE,
	}
};
/************************************MD5 END**************************/

/************************************SHA1 START**************************/
static inline void be32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__be32_to_cpus(buf);
		buf++;
	}
}

static inline void cpu_to_be32_array(u32 *buf, unsigned int words)
{
	while (words--) {
		__cpu_to_be32s(buf);
		buf++;
	}
}

static int ambarella_sha1_init(struct shash_desc *desc)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	*sctx = (struct sha1_state){
		.state = { SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3, SHA1_H4 },
	};
	return 0;
}

void md5_sha1_write64(u32 * addr,u32 * buf, unsigned int len )
{
	/* len%8 ,align at 64bit*/
	if ((len&0x7)){
		len += (len&0x7);
	}
	_ambarella_crypto_aligned_write64(addr,buf,len);
}

void md5_sha1_read64(u32 * addr,u32 * buf, unsigned int len )
{
	/* len%8 ,align at 64bit*/
	if ((len&0x7)){
		len += (len&0x7);
	}
	_ambarella_crypto_aligned_read64(addr,buf,len);
}

void ambarella_sha1_transform(__u32 *digest, const char *in, __u32 *W)
{
    u32 ready;

    do{}while(mutex_trylock(&engine_lock) == 0);

    cpu_to_be32_array(digest, 5);
    memcpy(&g_sha1_digest.digest_0, digest, 16);
    g_sha1_digest.digest_128 = digest[4];

    do{
        ready= amba_readl(CRYPT_MD5_SHA1_READY_INPUT);
    }while(ready != 1);

    md5_sha1_fun.wdata((u32 *)CRYPT_SHA1_INIT_31_0,&(g_sha1_digest.digest_0),20);
    do{
        ready= amba_readl(CRYPT_MD5_SHA1_READY_INPUT);
    }while(ready != 1);

    memcpy(&g_sha1_data.data[0], in, 64);

    md5_sha1_fun.wdata((u32 *)CRYPT_SHA1_INPUT_31_0,&(g_sha1_data.data[0]),64);

    if(likely(config_polling_mode == 0)) {
	do{
            ready = try_wait_for_completion(&g_md5_sha1_irq_wait);
        }while(ready != 1);

    }else{
        do{
            ready = amba_readl(CRYPT_SHA1_OUTPUT_READY);
        }while(ready != 1);
    }

    md5_sha1_fun.rdata(&(g_sha1_digest.digest_0),(u32 *)CRYPT_SHA1_OUTPUT_31_0,20);

    memcpy(digest, &g_sha1_digest.digest_0, 20);
    cpu_to_be32_array(digest, 5);

    mutex_unlock(&engine_lock);
}



static int ambarella_sha1_update(struct shash_desc *desc, const u8 *data,
			unsigned int len)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	unsigned int partial, done;
	const u8 *src;

	partial = sctx->count & 0x3f;
	sctx->count += len;
	done = 0;
	src = data;

	if ((partial + len) > 63) {
		u32 temp[SHA_WORKSPACE_WORDS];

		if (partial) {
			done = -partial;
			memcpy(sctx->buffer + partial, data, done + 64);
			src = sctx->buffer;
		}

		do {
			ambarella_sha1_transform(sctx->state, src, temp);
			done += 64;
			src = data + done;
		} while (done + 63 < len);

		memset(temp, 0, sizeof(temp));
		partial = 0;
	}
	memcpy(sctx->buffer + partial, src, len - done);

	return 0;
}

/* Add padding and return the message digest. */
static int ambarella_sha1_final(struct shash_desc *desc, u8 *out)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	__be32 *dst = (__be32 *)out;
	u32 i, index, padlen;
	__be64 bits;
	static const u8 padding[64] = { 0x80, };

	bits = cpu_to_be64(sctx->count << 3);

	/* Pad out to 56 mod 64 */
	index = sctx->count & 0x3f;
	padlen = (index < 56) ? (56 - index) : ((64+56) - index);
	ambarella_sha1_update(desc, padding, padlen);

	/* Append length */
	ambarella_sha1_update(desc, (const u8 *)&bits, sizeof(bits));

	/* Store state in digest */
	for (i = 0; i < 5; i++)
		dst[i] = cpu_to_be32(sctx->state[i]);

	/* Wipe context */
	memset(sctx, 0, sizeof *sctx);

	return 0;
}

static int ambarella_sha1_export(struct shash_desc *desc, void *out)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	memcpy(out, sctx, sizeof(*sctx));
	return 0;
}

static int ambarella_sha1_import(struct shash_desc *desc, const void *in)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	memcpy(sctx, in, sizeof(*sctx));
	return 0;
}


static struct shash_alg sha1_alg = {
	.digestsize	=	SHA1_DIGEST_SIZE,
	.init		=	ambarella_sha1_init,
	.update		=	ambarella_sha1_update,
	.final		=	ambarella_sha1_final,
	.export		=	ambarella_sha1_export,
	.import		=	ambarella_sha1_import,
	.descsize	=	sizeof(struct sha1_state),
	.statesize	=	sizeof(struct sha1_state),
	.base		=	{
		.cra_name	=	"sha1",
		.cra_driver_name=	"sha1-ambarella",
		.cra_flags	=	CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize	=	SHA1_BLOCK_SIZE,
		.cra_module	=	THIS_MODULE,
	}
};

/************************************SHA1 END****************************/

static irqreturn_t ambarella_aes_irq(int irqno, void *dev_id)
{
	complete(&g_aes_irq_wait);

	return IRQ_HANDLED;
}
static irqreturn_t ambarella_des_irq(int irqno, void *dev_id)
{
	complete(&g_des_irq_wait);

	return IRQ_HANDLED;
}

static irqreturn_t ambarella_md5_sha1_irq(int irqno, void *dev_id)
{
	complete(&g_md5_sha1_irq_wait);

	return IRQ_HANDLED;
}

static int ambarella_crypto_of_parse(struct platform_device *pdev,
			struct ambarella_crypto_info *pinfo)
{
	struct device_node *np = pdev->dev.of_node;

	pinfo->binary_mode = !!of_find_property(np, "amb,binary-mode", NULL);
	pinfo->cap_md5_sha1 = !!of_find_property(np, "amb,cap-md5-sha1", NULL);
	pinfo->data_swap = !!of_find_property(np, "amb,data-swap", NULL);
	pinfo->reg_64bit = !!of_find_property(np, "amb,reg-64bit", NULL);

	return 0;
}

static int ambarella_crypto_probe(struct platform_device *pdev)
{
	struct resource	*mem = 0;
	struct ambarella_crypto_info *pinfo = 0;
	int aes_irq, des_irq, md5_irq, sha1_irq;
	int errCode;

	pinfo = devm_kzalloc(&pdev->dev, sizeof(*pinfo), GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get crypto mem resource failed!\n");
		return -ENXIO;
	}

	pinfo->regbase = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!pinfo->regbase) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, pinfo);

	ambarella_crypto_of_parse(pdev, pinfo);

	if(likely(config_polling_mode == 0)) {
		aes_irq = platform_get_irq_byname(pdev,"aes-irq");
		if (aes_irq < 0) {
			dev_err(&pdev->dev, "Get crypto aes irq failed!\n");
			return aes_irq;
		}

		des_irq = platform_get_irq_byname(pdev,"des-irq");
		if (des_irq < 0) {
			dev_err(&pdev->dev, "Get crypto des irq failed!\n");
			return aes_irq;
		}

		errCode = devm_request_irq(&pdev->dev, aes_irq,
					ambarella_aes_irq, IRQF_TRIGGER_RISING,
					dev_name(&pdev->dev), pinfo);
		if (errCode < 0) {
			dev_err(&pdev->dev, "Request aes irq failed!\n");
			return errCode;
		}
		errCode = devm_request_irq(&pdev->dev, des_irq,
					ambarella_des_irq, IRQF_TRIGGER_RISING,
					dev_name(&pdev->dev), pinfo);
		if (errCode < 0) {
			dev_err(&pdev->dev, "Request des irq failed!\n");
			return errCode;
		}

		amba_writel(CRYPT_A_INT_EN_REG, 0x0001);
		amba_writel(CRYPT_D_INT_EN_REG, 0x0001);

		if (pinfo->cap_md5_sha1 == 1) {
			md5_irq = platform_get_irq_byname(pdev,"md5-irq");
			if (md5_irq < 0){
				dev_err(&pdev->dev, "Get crypto md5 irq failed!\n");
				return md5_irq;
			}

			sha1_irq = platform_get_irq_byname(pdev,"sha1-irq");
			if (sha1_irq < 0){
				dev_err(&pdev->dev, "Get crypto sha1 irq failed!\n");
				return sha1_irq;
			}

			/* md5 and sha1 opentions can't be interleaved, so we
			 * use the same return irq func */
			errCode = devm_request_irq(&pdev->dev, md5_irq,
						ambarella_md5_sha1_irq,
						IRQF_TRIGGER_RISING,
						dev_name(&pdev->dev), pinfo);
			if (errCode < 0) {
				dev_err(&pdev->dev, "Request md5 irq failed!\n");
				return errCode;
			}

			errCode = devm_request_irq(&pdev->dev, sha1_irq,
						ambarella_md5_sha1_irq,
						IRQF_TRIGGER_RISING,
						dev_name(&pdev->dev), pinfo);
			if (errCode < 0) {
				dev_err(&pdev->dev, "Request sha1 irq failed!\n");
				return errCode;
			}

			amba_writel(CRYPT_MD5_INT_EN, 0x0001);
			amba_writel(CRYPT_SHA1_INT_EN, 0x0001);
		}
		//init workqueue

		spin_lock_init(&amba_queue.lock);
		handle_queue = create_workqueue("Crypto Ablk Workqueue");
		INIT_WORK(&work,(void *)do_queue);

		//register ecb aes ,des
		errCode = crypto_register_alg(&ecb_aes_alg);
		if (errCode <0) {
			dev_err(&pdev->dev,"register ecb_aes_alg failed. \n");
			return errCode;
		}

		crypto_init_queue(&amba_queue.queue,50);
	} else {
		errCode = crypto_register_alg(&aes_alg);
		if (errCode <0) {
			dev_err(&pdev->dev, "reigster aes_alg  failed.\n");
			return errCode;
		}
	}

	if (pinfo->binary_mode){
		aes_fun.opcode = aes_opcode;
		des_fun.opcode = des_opcode;
	}else{
		aes_fun.opcode = null_fun;
		des_fun.opcode = null_fun;
	}

	if (pinfo->data_swap){
		aes_fun.wdata = swap_write;
		aes_fun.rdata = swap_read;
		des_fun.wdata = swap_write;
		des_fun.rdata = swap_read;
	}else{
		aes_fun.wdata = (void*)memcpy;
		aes_fun.rdata = (void*)memcpy;
		des_fun.wdata = (void*)memcpy;
		des_fun.rdata = (void*)memcpy;
	}

	if (pinfo->reg_64bit){
		aes_fun.reg_enc = aes_reg_enc_64;
		aes_fun.reg_dec = aes_reg_dec_64;
		des_fun.reg_enc = des_reg_enc_64;
		des_fun.reg_dec = des_reg_dec_64;
	}else{
		aes_fun.reg_enc = aes_reg_enc_dec_32;
		aes_fun.reg_dec = aes_reg_enc_dec_32;
		des_fun.reg_enc = des_reg_enc_dec_32;
		des_fun.reg_dec = des_reg_enc_dec_32;
	}

	if (pinfo->cap_md5_sha1) {
		md5_sha1_fun.wdata = (void*)md5_sha1_write64;
		md5_sha1_fun.rdata = (void*)md5_sha1_read64;

		if ((errCode = crypto_register_shash(&sha1_alg))) {
			dev_err(&pdev->dev, "reigster sha1_alg  failed.\n");
			goto crypto_errCode_free_md5_sha1_irq;
		}
		if ((errCode = crypto_register_shash(&md5_alg))) {
			dev_err(&pdev->dev, "reigster md5_alg  failed.\n");
			goto crypto_errCode_free_sha1;
		}
	}

	if ((errCode = crypto_register_alg(&des_alg))) {
		dev_err(&pdev->dev, "reigster des_alg failed.\n");
		goto crypto_errCode_free_md5;
	}

	dev_notice(&pdev->dev,"%s probed(%s mode).\n", ambdev_name,
		config_polling_mode ? "polling" : "interrupt");

	return 0;

crypto_errCode_free_md5:
	if (pinfo->cap_md5_sha1)
		crypto_unregister_shash(&md5_alg);

crypto_errCode_free_sha1:
	if (pinfo->cap_md5_sha1)
		crypto_unregister_shash(&sha1_alg);

crypto_errCode_free_md5_sha1_irq:
	if(unlikely(config_polling_mode == 0)){
		destroy_workqueue(handle_queue);
		crypto_unregister_alg(&ecb_aes_alg);
	}else{
		crypto_unregister_alg(&aes_alg);
	}

	return errCode;
}

static int ambarella_crypto_remove(struct platform_device *pdev)
{
	struct ambarella_crypto_info *pinfo;
	int errCode = 0;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo->cap_md5_sha1) {
		crypto_unregister_shash(&sha1_alg);
		crypto_unregister_shash(&md5_alg);
	}
	crypto_unregister_alg(&des_alg);

	if (config_polling_mode == 0) {
		destroy_workqueue(handle_queue);
		crypto_unregister_alg(&ecb_aes_alg);

		platform_set_drvdata(pdev, NULL);
	} else {
		crypto_unregister_alg(&aes_alg);
	}

	dev_notice(&pdev->dev, "%s removed.\n", ambdev_name);

	return errCode;
}

static const struct of_device_id ambarella_crypto_dt_ids[] = {
	{.compatible = "ambarella,crypto", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_crypto_dt_ids);

static struct platform_driver ambarella_crypto_driver = {
	.probe		= ambarella_crypto_probe,
	.remove		= ambarella_crypto_remove,
#ifdef CONFIG_PM
	.suspend	= NULL,
	.resume		= NULL,
#endif
	.driver		= {
		.name	= "ambarella-crypto",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_crypto_dt_ids,
	},
};

module_platform_driver(ambarella_crypto_driver);

MODULE_DESCRIPTION("Ambarella Cryptography Engine");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qiao Wang");
MODULE_ALIAS("crypo-all");

