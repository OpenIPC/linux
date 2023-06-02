/*
 * fh_aes_test.c
 *
 *  Created on: May 7, 2015
 *      Author: yu.zhang
 */
#ifdef CONFIG_FH_AES_SELF_TEST
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
#include <crypto/cryptd.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <crypto/rng.h>
#include "fh_aes.h"
//cbc aes 128
#define AES_IV0			0x00010203
#define AES_IV1			0x04050607
#define AES_IV2			0x08090a0b
#define AES_IV3			0x0c0d0e0f

#define AES_KEY0		0x2b7e1516
#define AES_KEY1		0x28aed2a6
#define AES_KEY2		0xabf71588
#define AES_KEY3		0x09cf4f3c


static const unsigned char aes_cbc_iv_buf[] = {
	0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
};

static const unsigned char aes_cbc_key_buf[] = {
	0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6, 0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
};


//ecb aes 256
#define AES_ECB_KEY0	0x603deb10
#define AES_ECB_KEY1	0x15ca71be
#define AES_ECB_KEY2	0x2b73aef0
#define AES_ECB_KEY3	0x857d7781
#define AES_ECB_KEY4	0x1f352c07
#define AES_ECB_KEY5	0x3b6108d7
#define AES_ECB_KEY6	0x2d9810a3
#define AES_ECB_KEY7	0x0914dff4


static const unsigned char aes_ecb_key_buf[] = {
	0x60,0x3d,0xeb,0x10, 0x15,0xca,0x71,0xbe, 0x2b,0x73,0xae,0xf0, 0x85,0x7d,0x77,0x81,
	0x1f,0x35,0x2c,0x07, 0x3b,0x61,0x08,0xd7, 0x2d,0x98,0x10,0xa3, 0x09,0x14,0xdf,0xf4,
};

//ctr aes 192
#define AES_CTR_KEY0 	0x8e73b0f7
#define AES_CTR_KEY1 	0xda0e6452
#define AES_CTR_KEY2 	0xc810f32b
#define AES_CTR_KEY3 	0x809079e5
#define AES_CTR_KEY4 	0x62f8ead2
#define AES_CTR_KEY5 	0x522c6b7b

#define AES_CTR_IV0		0xf0f1f2f3
#define AES_CTR_IV1		0xf4f5f6f7
#define AES_CTR_IV2		0xf8f9fafb
#define AES_CTR_IV3		0xfcfdfeff


static const unsigned char aes_ctr_iv_buf[] = {
	0xf0,0xf1,0xf2,0xf3, 0xf4,0xf5,0xf6,0xf7, 0xf8,0xf9,0xfa,0xfb, 0xfc,0xfd,0xfe,0xff,
};

static const unsigned char aes_ctr_key_buf[] = {
	0x8e,0x73,0xb0,0xf7, 0xda,0x0e,0x64,0x52, 0xc8,0x10,0xf3,0x2b, 0x80,0x90,0x79,0xe5,
	0x62,0xf8,0xea,0xd2, 0x52,0x2c,0x6b,0x7b,
};


//ofb aes 256
#define AES_OFB_256_KEY0 0x603deb10
#define AES_OFB_256_KEY1 0x15ca71be
#define AES_OFB_256_KEY2 0x2b73aef0
#define AES_OFB_256_KEY3 0x857d7781
#define AES_OFB_256_KEY4 0x1f352c07
#define AES_OFB_256_KEY5 0x3b6108d7
#define AES_OFB_256_KEY6 0x2d9810a3
#define AES_OFB_256_KEY7 0x0914dff4

#define AES_OFB_IV0		0x00010203
#define AES_OFB_IV1		0x04050607
#define AES_OFB_IV2		0x08090a0b
#define AES_OFB_IV3		0x0c0d0e0f

static const unsigned char aes_ofb_iv_buf[] = {
	0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
};

static const unsigned char aes_ofb_key_buf[] = {
	0x60,0x3d,0xeb,0x10, 0x15,0xca,0x71,0xbe, 0x2b,0x73,0xae,0xf0, 0x85,0x7d,0x77,0x81,
	0x1f,0x35,0x2c,0x07, 0x3b,0x61,0x08,0xd7, 0x2d,0x98,0x10,0xa3, 0x09,0x14,0xdf,0xf4,
};

//des ecb
#define DES_ECB_KEY0 	0x01010101
#define DES_ECB_KEY1 	0x01010101

static const unsigned char des_ecb_key_buf[] = {
	0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01,
};
//des cbc
#define DES_CBC_KEY0 	0x01234567
#define DES_CBC_KEY1 	0x89abcdef

#define DES_CBC_IV0		0x12345678
#define DES_CBC_IV1		0x90abcdef


static const unsigned char des_cbc_key_buf[] = {
	0x01,0x23,0x45,0x67, 0x89,0xab,0xcd,0xef,
};

static const unsigned char des_cbc_iv_buf[] = {
	0x12,0x34,0x56,0x78, 0x90,0xab,0xcd,0xef,
};


//ofb cbc
#define DES_OFB_KEY0 	0x01234567
#define DES_OFB_KEY1 	0x89abcdef

#define DES_OFB_IV0		0x12345678
#define DES_OFB_IV1		0x90abcdef

static const unsigned char des_ofb_key_buf[] = {
	0x01,0x23,0x45,0x67, 0x89,0xab,0xcd,0xef,
};

static const unsigned char des_ofb_iv_buf[] = {
	0x12,0x34,0x56,0x78, 0x90,0xab,0xcd,0xef,
};

//ecb tri-des
#define DES_TRI_ECB_KEY0	 0x01234567
#define DES_TRI_ECB_KEY1 	 0x89abcdef

#define DES_TRI_ECB_KEY2	 0x23456789
#define DES_TRI_ECB_KEY3	 0xabcdef01

#define DES_TRI_ECB_KEY4	 0x456789ab
#define DES_TRI_ECB_KEY5	 0xcdef0123

static const unsigned char des3_ecb_key_buf[] = {
	0x01,0x23,0x45,0x67, 0x89,0xab,0xcd,0xef, 0x23,0x45,0x67,0x89, 0xab,0xcd,0xef,0x01,
	0x45,0x67,0x89,0xab, 0xcd,0xef,0x01,0x23,
};

//cbc tri-des
#define DES_TRI_CBC_KEY0	 0x01234567
#define DES_TRI_CBC_KEY1 	 0x89abcdef

#define DES_TRI_CBC_KEY2	 0x23456789
#define DES_TRI_CBC_KEY3	 0xabcdef01

#define DES_TRI_CBC_KEY4	 0x456789ab
#define DES_TRI_CBC_KEY5	 0xcdef0123

#define DES_TRI_CBC_IV0		0x12345678
#define DES_TRI_CBC_IV1		0x90abcdef

static const unsigned char des3_cbc_key_buf[] = {
	0x01,0x23,0x45,0x67, 0x89,0xab,0xcd,0xef, 0x23,0x45,0x67,0x89, 0xab,0xcd,0xef,0x01,
	0x45,0x67,0x89,0xab, 0xcd,0xef,0x01,0x23,
};
static const unsigned char des3_cbc_iv_buf[] = {
	0x12,0x34,0x56,0x78, 0x90,0xab,0xcd,0xef,
};
#define XBUFSIZE	512

struct tcrypt_result {
	struct completion completion;
	int err;
};

static inline void hexdump(unsigned char *buf, unsigned int len);
static void tcrypt_complete(struct crypto_async_request *req, int err);
static int testmgr_alloc_buf(char *buf[XBUFSIZE]);


static struct tcrypt_result result;
static const unsigned char plain_text[]  = {
		0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96, 0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a,
		0xae,0x2d,0x8a,0x57, 0x1e,0x03,0xac,0x9c, 0x9e,0xb7,0x6f,0xac, 0x45,0xaf,0x8e,0x51,
		0x30,0xc8,0x1c,0x46, 0xa3,0x5c,0xe4,0x11, 0xe5,0xfb,0xc1,0x19, 0x1a,0x0a,0x52,0xef,
		0xf6,0x9f,0x24,0x45, 0xdf,0x4f,0x9b,0x17, 0xad,0x2b,0x41,0x7b, 0xe6,0x6c,0x37,0x10,
};

static const unsigned char cipher_text[] = {
		0x76,0x49,0xab,0xac, 0x81,0x19,0xb2,0x46, 0xce,0xe9,0x8e,0x9b, 0x12,0xe9,0x19,0x7d,
		0x50,0x86,0xcb,0x9b, 0x50,0x72,0x19,0xee, 0x95,0xdb,0x11,0x3a, 0x91,0x76,0x78,0xb2,
		0x73,0xbe,0xd6,0xb8, 0xe3,0xc1,0x74,0x3b, 0x71,0x16,0xe6,0x9e, 0x22,0x22,0x95,0x16,
		0x3f,0xf1,0xca,0xa1, 0x68,0x1f,0xac,0x09, 0x12,0x0e,0xca,0x30, 0x75,0x86,0xe1,0xa7,
};

static const unsigned char plain_ecb_256_text[] = {
		0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96, 0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a,
		0xae,0x2d,0x8a,0x57, 0x1e,0x03,0xac,0x9c, 0x9e,0xb7,0x6f,0xac, 0x45,0xaf,0x8e,0x51,
		0x30,0xc8,0x1c,0x46, 0xa3,0x5c,0xe4,0x11, 0xe5,0xfb,0xc1,0x19, 0x1a,0x0a,0x52,0xef,
		0xf6,0x9f,0x24,0x45, 0xdf,0x4f,0x9b,0x17, 0xad,0x2b,0x41,0x7b, 0xe6,0x6c,0x37,0x10,
};

static const unsigned char cipher_ecb_256_text[] = {
		0xf3,0xee,0xd1,0xbd, 0xb5,0xd2,0xa0,0x3c, 0x06,0x4b,0x5a,0x7e, 0x3d,0xb1,0x81,0xf8,
		0x59,0x1c,0xcb,0x10, 0xd4,0x10,0xed,0x26, 0xdc,0x5b,0xa7,0x4a, 0x31,0x36,0x28,0x70,
		0xb6,0xed,0x21,0xb9, 0x9c,0xa6,0xf4,0xf9, 0xf1,0x53,0xe7,0xb1, 0xbe,0xaf,0xed,0x1d,
		0x23,0x30,0x4b,0x7a, 0x39,0xf9,0xf3,0xff, 0x06,0x7d,0x8d,0x8f, 0x9e,0x24,0xec,0xc7,
};

static const unsigned char plain_ctr_192_text[] = {
		0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96, 0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a,
		0xae,0x2d,0x8a,0x57, 0x1e,0x03,0xac,0x9c, 0x9e,0xb7,0x6f,0xac, 0x45,0xaf,0x8e,0x51,
		0x30,0xc8,0x1c,0x46, 0xa3,0x5c,0xe4,0x11, 0xe5,0xfb,0xc1,0x19, 0x1a,0x0a,0x52,0xef,
		0xf6,0x9f,0x24,0x45, 0xdf,0x4f,0x9b,0x17, 0xad,0x2b,0x41,0x7b, 0xe6,0x6c,0x37,0x10,
};

static const unsigned char cipher_ctr_192_text[] = {
		0x1a,0xbc,0x93,0x24,	0x17,0x52,0x1c,0xa2,	0x4f,0x2b,0x04,0x59,	0xfe,0x7e,0x6e,0x0b,
		0x09,0x03,0x39,0xec,	0x0a,0xa6,0xfa,0xef,	0xd5,0xcc,0xc2,0xc6,	0xf4,0xce,0x8e,0x94,
		0x1e,0x36,0xb2,0x6b,	0xd1,0xeb,0xc6,0x70,	0xd1,0xbd,0x1d,0x66,	0x56,0x20,0xab,0xf7,
		0x4f,0x78,0xa7,0xf6,	0xd2,0x98,0x09,0x58,	0x5a,0x97,0xda,0xec,	0x58,0xc6,0xb0,0x50,
};

static const unsigned char plain_ofb_256_text[] = {
		0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96, 0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a,
		0xae,0x2d,0x8a,0x57, 0x1e,0x03,0xac,0x9c, 0x9e,0xb7,0x6f,0xac, 0x45,0xaf,0x8e,0x51,
		0x30,0xc8,0x1c,0x46, 0xa3,0x5c,0xe4,0x11, 0xe5,0xfb,0xc1,0x19, 0x1a,0x0a,0x52,0xef,
		0xf6,0x9f,0x24,0x45, 0xdf,0x4f,0x9b,0x17, 0xad,0x2b,0x41,0x7b, 0xe6,0x6c,0x37,0x10,
};

static const unsigned char cipher_ofb_256_text[] = {
		0xdc,0x7e,0x84,0xbf,0xda,0x79,0x16,0x4b,0x7e,0xcd,0x84,0x86,0x98,0x5d,0x38,0x60,
		0x4f,0xeb,0xdc,0x67,0x40,0xd2,0x0b,0x3a,0xc8,0x8f,0x6a,0xd8,0x2a,0x4f,0xb0,0x8d,
		0x71,0xab,0x47,0xa0,0x86,0xe8,0x6e,0xed,0xf3,0x9d,0x1c,0x5b,0xba,0x97,0xc4,0x08,
		0x01,0x26,0x14,0x1d,0x67,0xf3,0x7b,0xe8,0x53,0x8f,0x5a,0x8b,0xe7,0x40,0xe4,0x84,
};

static const unsigned char plain_des_ecb_text[] = {
		0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const unsigned char cipher_des_ecb_text[] = {
		0x95,0xF8,0xA5,0xE5,0xDD,0x31,0xD9,0x00,0xDD,0x7F,0x12,0x1C,0xA5,0x01,0x56,0x19,
		0x2E,0x86,0x53,0x10,0x4F,0x38,0x34,0xEA,0x4B,0xD3,0x88,0xFF,0x6C,0xD8,0x1D,0x4F,
		0x20,0xB9,0xE7,0x67,0xB2,0xFB,0x14,0x56,0x55,0x57,0x93,0x80,0xD7,0x71,0x38,0xEF,
		0x6C,0xC5,0xDE,0xFA,0xAF,0x04,0x51,0x2F,0x0D,0x9F,0x27,0x9B,0xA5,0xD8,0x72,0x60,
};

static const unsigned char plain_des_cbc_text[] = {
		0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
		0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20,
};

static const unsigned char cipher_des_cbc_text[] = {
		0xe5,0xc7,0xcd,0xde,0x87,0x2b,0xf2,0x7c,0x43,0xe9,0x34,0x00,0x8c,0x38,0x9c,0x0f,
		0x68,0x37,0x88,0x49,0x9a,0x7c,0x05,0xf6,
};

static const unsigned char  plain_des_ofb_text[] = {
		0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,0x43,0xe9,0x34,0x00,0x8c,0x38,0x9c,0x0f,
			0x68,0x37,0x88,0x49,0x9a,0x7c,0x05,0xf6,
};

static const unsigned char cipher_des_ofb_text[] = {
		0xf3,0x09,0x62,0x49,0xc7,0xf4,0x6e,0x51,0x1e,0x7e,0x5e,0x50,0xcb,0xbe,0xc4,0x10,
		0x33,0x35,0xa1,0x8a,0xde,0x4a,0x91,0x15,
};

static const unsigned char plain_des_tri_ecb_text[] = {
		0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,0x43,0xe9,0x34,0x00,0x8c,0x38,0x9c,0x0f,
		0x68,0x37,0x88,0x49,0x9a,0x7c,0x05,0xf6,
};

static const unsigned char cipher_des_tri_ecb_text[] = {
		0x31,0x4f,0x83,0x27,0xfa,0x7a,0x09,0xa8,0xd5,0x89,0x5f,0xad,0xe9,0x8f,0xae,0xdf,
		0x98,0xf4,0x70,0xeb,0x35,0x53,0xa5,0xda,
};

static const unsigned char plain_des_tri_cbc_text[] = {
		0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,0x43,0xe9,0x34,0x00,0x8c,0x38,0x9c,0x0f,
		0x68,0x37,0x88,0x49,0x9a,0x7c,0x05,0xf6,
};

static const unsigned char cipher_des_tri_cbc_text[] = {
		0xf3,0xc0,0xff,0x02,0x6c,0x02,0x30,0x89,0xc4,0x3a,0xdd,0x8f,0xd8,0xcd,0x5e,0x43,
		0x2b,0xfd,0x41,0xd3,0x13,0x0b,0xcf,0x40,
};

static inline void hexdump(unsigned char *buf, unsigned int len)
{
	while (len--)
		AES_DBG("%02x", *buf++);
	AES_DBG("\n");
}

static void tcrypt_complete(struct crypto_async_request *req, int err)
{
	struct tcrypt_result *res = req->data;
	if (err == -EINPROGRESS)
		return;

	AES_DBG("crypt all over....\n");
	complete(&res->completion);

}

static int testmgr_alloc_buf(char *buf[XBUFSIZE])
{
	int i;
	for (i = 0; i < XBUFSIZE; i++) {
		buf[i] = (void *)__get_free_page(GFP_KERNEL);
		if (!buf[i])
			goto err_free_buf;
	}

	return 0;
err_free_buf:
	while (i-- > 0)
		free_page((unsigned long)buf[i]);

	return -ENOMEM;
}

void fh_aes_set_crypto_key_source(struct af_alg_usr_def *p_def, u32 key_source){
	p_def->mode = key_source;
}

static int fh_aes_cbc128_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	const char *algo;

	void *data;
	void *dst_data;

	u32 key[4] = { AES_KEY0, AES_KEY1, AES_KEY2, AES_KEY3 };
	u32 iv[4] = { AES_IV0, AES_IV1, AES_IV2, AES_IV3 };

	memcpy(&key[0],&aes_cbc_key_buf[0],sizeof(aes_cbc_key_buf));
	memcpy(&iv[0],&aes_cbc_iv_buf[0],sizeof(aes_cbc_iv_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");
	t_crytfm = crypto_alloc_base("cbc-aes-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	pr_info("driver name %s\n",algo);
	init_completion(&result.completion);
	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 16);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_text, 64))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);
	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);
	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);
	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_text, 64))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

static int fh_aes_ecb256_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[8] = {
		AES_ECB_KEY0, AES_ECB_KEY1, AES_ECB_KEY2, AES_ECB_KEY3,
		AES_ECB_KEY4, AES_ECB_KEY5, AES_ECB_KEY6, AES_ECB_KEY7
	};
	void *data;
	void *dst_data;
	memcpy(&key[0],&aes_ecb_key_buf[0],sizeof(aes_ecb_key_buf));
	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");
	t_crytfm = crypto_alloc_base("ecb-aes-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}

	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 32);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_ecb_256_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);
	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, NULL);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_ecb_256_text, 64))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_ecb_256_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, NULL);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_ecb_256_text, 64))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

static int fh_aes_ofb256_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[8] = {
		AES_OFB_256_KEY0, AES_OFB_256_KEY1, AES_OFB_256_KEY2,
		AES_OFB_256_KEY3,
		AES_OFB_256_KEY4, AES_OFB_256_KEY5, AES_OFB_256_KEY6,
		AES_OFB_256_KEY7
	};
	u32 iv[4] =
	{ AES_OFB_IV0, AES_OFB_IV1, AES_OFB_IV2, AES_OFB_IV3 };

	void *data;
	void *dst_data;
	memcpy(&key[0],&aes_ofb_key_buf[0],sizeof(aes_ofb_key_buf));
	memcpy(&iv[0],&aes_ofb_iv_buf[0],sizeof(aes_ofb_iv_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");

	t_crytfm = crypto_alloc_base("ofb-aes-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}

	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 32);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;
	//encrypt
	memcpy(data, plain_ofb_256_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);
	wait_for_completion(&result.completion);
	if (memcmp(dst_data, cipher_ofb_256_text, 64))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");
	//decrypt
	memcpy(data, cipher_ofb_256_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);
	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);
	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_ofb_256_text, 64))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;
}

static int fh_des_ecb_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[2] = { DES_ECB_KEY0, DES_ECB_KEY1 };

	void *data;
	void *dst_data;

	memcpy(&key[0],&des_ecb_key_buf[0],sizeof(des_ecb_key_buf));
	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");

	t_crytfm = crypto_alloc_base("ecb-des-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}


	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 8);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_des_ecb_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, NULL);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_des_ecb_text, 64))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_des_ecb_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, NULL);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_des_ecb_text, 64))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

static int fh_des_cbc_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	struct crypto_tfm * t_crytfm;
	u32 key[2] = { DES_CBC_KEY0, DES_CBC_KEY1 };
	u32 iv[2] = { DES_CBC_IV0, DES_CBC_IV1 };
	void *data;
	void *dst_data;


	memcpy(&key[0],&des_cbc_key_buf[0],sizeof(des_cbc_key_buf));
	memcpy(&iv[0],&des_cbc_iv_buf[0],sizeof(des_cbc_iv_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");

	t_crytfm = crypto_alloc_base("cbc-des-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}


	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 8);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_des_cbc_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_des_cbc_text, 24))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_des_cbc_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_des_cbc_text, 24))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;
}

static int fh_des_ofb_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[2] = { DES_OFB_KEY0, DES_OFB_KEY1 };
	u32 iv[2] = { DES_OFB_IV0, DES_OFB_IV1 };
	void *data;
	void *dst_data;

	memcpy(&key[0],&des_ofb_key_buf[0],sizeof(des_ofb_key_buf));
	memcpy(&iv[0],&des_ofb_iv_buf[0],sizeof(des_ofb_iv_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");

	t_crytfm = crypto_alloc_base("ofb-des-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}

	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 8);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_des_ofb_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);
	wait_for_completion(&result.completion);
	if (memcmp(dst_data, cipher_des_ofb_text, 24))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_des_ofb_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);
	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);
	wait_for_completion(&result.completion);
	if (memcmp(dst_data, plain_des_ofb_text, 24))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

static int fh_des_tri_ecb_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[6] = {
		DES_TRI_ECB_KEY0, DES_TRI_ECB_KEY1, DES_TRI_ECB_KEY2,
		DES_TRI_ECB_KEY3, DES_TRI_ECB_KEY4, DES_TRI_ECB_KEY5
	};

	void *data;
	void *dst_data;

	memcpy(&key[0],&des3_ecb_key_buf[0],sizeof(des3_ecb_key_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");

	t_crytfm = crypto_alloc_base("ecb-des3-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}

	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 24);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_des_tri_ecb_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)NULL);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_des_tri_ecb_text, 24))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_des_tri_ecb_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)NULL);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_des_tri_ecb_text, 24))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

static int fh_des_tri_cbc_self_test(void * xbuf, void* dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	const char *algo;
	struct crypto_tfm * t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	u32 key[6] = {
		DES_TRI_CBC_KEY0, DES_TRI_CBC_KEY1, DES_TRI_CBC_KEY2,
		DES_TRI_CBC_KEY3, DES_TRI_CBC_KEY4, DES_TRI_CBC_KEY5
	};
	u32 iv[2] = { DES_TRI_CBC_IV0, DES_TRI_CBC_IV1 };

	void *data;
	void *dst_data;


	memcpy(&key[0],&des3_cbc_key_buf[0],sizeof(des3_cbc_key_buf));
	memcpy(&iv[0],&des3_cbc_iv_buf[0],sizeof(des3_cbc_iv_buf));

	AES_DBG("aes self test get in...\n");

	AES_DBG(" *_* step 1\n");


	t_crytfm = crypto_alloc_base("cbc-des3-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}


	AES_DBG(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&result.completion);

	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 24);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_des_tri_cbc_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);
	fh_aes_set_crypto_key_source(p_def, CRYPTO_CPU_SET_KEY);
	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_des_tri_cbc_text, 24))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_des_tri_cbc_text, 24);
	memset(dst_data, 0, 24);
	sg_init_one(&sg[0], data, 24);
	sg_init_one(&dst_sg[0], dst_data, 24);

	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 24, (void *)iv);

	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_des_tri_cbc_text, 24))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;

}

#if(0)

typedef struct
{
	unsigned int base;
	void * vbase;
	unsigned int size;
}MEM_INFO;
typedef struct {
	MEM_INFO mem;
	unsigned char *remap_base; /**<已用大小*/
} RW_MEM_INFO;


static  unsigned char aes_128_key_buf[] = {
	0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6, 0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
};
static  unsigned char plain_aes_128_text[]  = {
		0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96, 0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a,
		0xae,0x2d,0x8a,0x57, 0x1e,0x03,0xac,0x9c, 0x9e,0xb7,0x6f,0xac, 0x45,0xaf,0x8e,0x51,
		0x30,0xc8,0x1c,0x46, 0xa3,0x5c,0xe4,0x11, 0xe5,0xfb,0xc1,0x19, 0x1a,0x0a,0x52,0xef,
		0xf6,0x9f,0x24,0x45, 0xdf,0x4f,0x9b,0x17, 0xad,0x2b,0x41,0x7b, 0xe6,0x6c,0x37,0x10,
};

static  unsigned char cipher_aes_128_text[]  = {
		0x3A,0xD7,0x7B,0xB4, 0x0D,0x7A,0x36,0x60, 0xA8,0x9E,0xCA,0xF3, 0x24,0x66,0xEF,0x97,
		0xf5,0xd3,0xd5,0x85, 0x03,0xb9,0x69,0x9d, 0xe7,0x85,0x89,0x5a, 0x96,0xfd,0xba,0xaf,
		0x43,0xb1,0xcd,0x7f, 0x59,0x8e,0xce,0x23, 0x88,0x1b,0x00,0xe3, 0xed,0x03,0x06,0x88,
		0x7b,0x0c,0x78,0x5e, 0x27,0xe8,0xad,0x3f, 0x82,0x23,0x20,0x71, 0x04,0x72,0x5d,0xd4,
};

int aes_128_ecb_encrypt(char *key_128, RW_MEM_INFO in,
	RW_MEM_INFO out, unsigned int data_len_align16){

	static char *xbuf;
	static char *dst_xbuf;
	static struct crypto_ablkcipher *tfm;
	static struct ablkcipher_request *req;
	static malloc_flag = 0;
	const char *algo;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	void *data;
	void *dst_data;
	struct tcrypt_result wait_result;

//malloc buf...
	if(malloc_flag != 0){
		goto work_go;
	}
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

	tfm =
		cryptd_alloc_ablkcipher("ecb-aes-fh",
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
			"for %s\n", algo);
		return -1;
	}


work_go:
	printk("aes self test get in...\n");
	printk(" *_* step 1\n");

	printk(" *_* step 2\n");
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	init_completion(&wait_result.completion);

	printk(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *)key_128, 16);

	printk(" *_* step 4\n");


	printk(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &wait_result);

	printk(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, in.remap_base, data_len_align16);
	//memset(dst_data, 0, data_len_align16);
	sg_init_one(&sg[0], data, data_len_align16);
	sg_init_one(&dst_sg[0], dst_data, data_len_align16);

	printk(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, data_len_align16, NULL);

	printk(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&wait_result.completion);

	memcpy(out.remap_base, dst_data, data_len_align16);

	return 0;

}
#endif

#if (0)
/* keymap test with efuse, must set keys in efuse first */

static struct ex_key_map_para ex_key_para = {
	.map_size = sizeof(aes_cbc_key_buf) / sizeof(uint32_t),
	.map = { {0, 0}, {1, 4}, {2, 8}, {3, 12} },
};

static int fh_aes_cbc128_keymap_self_test(void *xbuf, void *dst_xbuf,
	struct af_alg_usr_def *p_def)
{
	struct crypto_ablkcipher *tfm;
	struct ablkcipher_request *req;
	struct crypto_tfm *t_crytfm;
	struct scatterlist sg[8];
	struct scatterlist dst_sg[8];
	const char *algo;

	void *data;
	void *dst_data;

	u32 key[4] = { AES_KEY0, AES_KEY1, AES_KEY2, AES_KEY3 };
	u32 iv[4] = { AES_IV0, AES_IV1, AES_IV2, AES_IV3 };

	memcpy(&key[0], &aes_cbc_key_buf[0], sizeof(aes_cbc_key_buf));
	memcpy(&iv[0], &aes_cbc_iv_buf[0], sizeof(aes_cbc_iv_buf));

	AES_DBG("aes self test get in...\n");
	AES_DBG(" *_* step 1\n");
	t_crytfm = crypto_alloc_base("cbc-aes-fh",
					CRYPTO_ALG_TYPE_ABLKCIPHER |
					CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(t_crytfm)) {
		AES_DBG("aes_test: failed to alloc t_crytfm!\n");
		return -1;
	}
	tfm = __crypto_ablkcipher_cast(t_crytfm);
	if (IS_ERR(tfm)) {
		AES_DBG("aes_test: failed to alloc cipher!\n");
		return -1;
	}
	algo = crypto_tfm_alg_driver_name(crypto_ablkcipher_tfm(tfm));
	pr_info("driver name %s\n", algo);
	init_completion(&result.completion);
	AES_DBG(" *_* step 3\n");
	crypto_ablkcipher_setkey(tfm, (u8 *) key, 16);

	AES_DBG(" *_* step 4\n");
	req = ablkcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) {
		AES_DBG(KERN_ERR "alg: skcipher: Failed to allocate request "
			"for %s\n", algo);
		return -1;
	}

	AES_DBG(" *_* step 5\n");
	ablkcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
					tcrypt_complete, &result);

	AES_DBG(" *_* step 6\n");
	data = xbuf;
	dst_data = dst_xbuf;

	//encrypt
	memcpy(data, plain_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);

	AES_DBG(" *_* step 7\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);

	ablkcipher_request_set_usrdef(req, p_def);
	AES_DBG(" *_* step 8\n");
	crypto_ablkcipher_encrypt(req);

	wait_for_completion(&result.completion);

	if (memcmp(dst_data, cipher_text, 64))
		AES_PRINT_RESULT(" encrypt error....\n");
	else
		AES_PRINT_RESULT(" encrypt ok....\n");

	//decrypt
	memcpy(data, cipher_text, 64);
	memset(dst_data, 0, 64);
	sg_init_one(&sg[0], data, 64);
	sg_init_one(&dst_sg[0], dst_data, 64);
	AES_DBG(" *_* step 8\n");
	ablkcipher_request_set_crypt(req, sg, dst_sg, 64, (void *)iv);
	AES_DBG(" *_* step 9\n");
	crypto_ablkcipher_decrypt(req);
	wait_for_completion(&result.completion);

	if (memcmp(dst_data, plain_text, 64))
		AES_PRINT_RESULT(" decrypt error....\n");
	else
		AES_PRINT_RESULT(" decrypt ok....\n");

	return 0;
}
#endif

int fh_aes_self_test_all(void)
{

	static char *xbuf;
	static char *dst_xbuf;
	struct af_alg_usr_def usr_def = {0};

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

	pr_info("aes cbc128 self test go...\n");
	fh_aes_cbc128_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("aes ecb256 self test go...\n");
	fh_aes_ecb256_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("aes ofb 256 self test go...\n");
	fh_aes_ofb256_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("des ecb self test go...\n");
	fh_des_ecb_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("des cbc self test go...\n");
	fh_des_cbc_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("des ofb self test go...\n");
	fh_des_ofb_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("des tri ecb self test go...\n");
	fh_des_tri_ecb_self_test(xbuf, dst_xbuf, &usr_def);
	pr_info("des tri cbc self test go...\n");
	fh_des_tri_cbc_self_test(xbuf, dst_xbuf, &usr_def);

#if (0)
	pr_info("########## fh_aes_cbc128_keymap_self_test ###########\n");
	usr_def.mode = CRYPTO_EX_MEM_SET_KEY;
	usr_def.adv.ex_key_para = ex_key_para;
	fh_aes_cbc128_keymap_self_test(xbuf, dst_xbuf, &usr_def);
	usr_def.mode = CRYPTO_EX_MEM_SWITCH_KEY | CRYPTO_EX_MEM_4_ENTRY_1_KEY
			| CRYPTO_EX_MEM_SET_KEY;
	fh_aes_cbc128_keymap_self_test(xbuf, dst_xbuf, &usr_def);
#endif

#if (0)
	RW_MEM_INFO in;
	RW_MEM_INFO out;
	unsigned char temp_buf[64] = {0};

	in.remap_base = &plain_aes_128_text[0];
	out.remap_base = &temp_buf[0];

	pr_info("chenjn self test go.....\n");

	aes_128_ecb_encrypt(&aes_128_key_buf[0], in,
			out, 64);
	for (i = 0; i < sizeof(temp_buf); i++)
	printk("cipher data[%d]:0x%x\n", i, temp_buf[i]);
#endif

	return 0;
}

#endif
