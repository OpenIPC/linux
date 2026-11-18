/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_CIPHER_CRYPTO_H_
#define _AX_CIPHER_CRYPTO_H_

#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <crypto/akcipher.h>
#include <crypto/skcipher.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/akcipher.h>
#include <crypto/sha.h>
#ifdef AX_CE_KERNEL5
#include <crypto/internal/des.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>
#endif
#include <crypto/des.h>
#include <crypto/aes.h>
#include <crypto/hmac.h>
#include <crypto/md5.h>
#include <crypto/sha3.h>
#include <crypto/internal/rng.h>
#include <linux/hw_random.h>
#include <crypto/internal/rsa.h>
#include "ax_cipher_adapt.h"

#define AX_CIPHER_CRA_PRIORITY (300)

typedef enum {
	AX_CIPHER_ALG_TYPE_SKCIPHER,
	AX_CIPHER_ALG_TYPE_AKCIPHER,
	AX_CIPHER_ALG_TYPE_AHASH,
	AX_CIPHER_ALG_TYPE_RNG,
} ax_cipher_alg_type_e;

typedef struct {
	struct device *dev;
	struct workqueue_struct *workqueue;
	struct work_struct work;
	struct crypto_queue queue;
	spinlock_t queue_lock;
	bool busy;
} ax_cipher_crypto_priv_s;

typedef struct {
	uint8_t key[32];
	uint8_t iv[32];
	uint32_t key_len;
	bool encrypt;
} ax_cipher_skcipher_data_s;

typedef struct {
	uint8_t state[64];
	uint8_t key[128];
	uint32_t cache_size;
	uint32_t cache_offset;
	uint32_t total_len;
	uint32_t key_len;
	bool is_final;
	bool is_first;
	bool is_align;
} ax_cipher_ahash_data_s;

typedef struct {
	uint8_t state[64];
	uint8_t key[128];
	uint8_t cache[128];
	uint32_t cache_size;
	uint32_t cache_offset;
	uint32_t total_len;
	uint32_t key_len;
	bool is_first;
	bool is_align;
} ax_cipher_ahash_state_s;

typedef struct {
	uint32_t e;
	uint32_t n_len;
	uint32_t d_len;
	uint32_t e_len;
	bool encrypt;
} ax_cipher_akcipher_data_s;

typedef struct {
	struct rng_alg rng_alg;
	struct hwrng trng;
	struct device *dev;
} ax_cipher_rng_alg_s;

typedef struct {
	ax_cipher_crypto_priv_s *priv;
	ax_cipher_alg_type_e type;
	bool des3;
	union {
		AX_CIPHER_ALGO_E algo;
		AX_CIPHER_RSA_SIGN_SCHEME_E rsa_sign;
	} cipher_alg;
	AX_CIPHER_MODE_E mode;
	union {
		struct skcipher_alg skcipher;
		struct akcipher_alg akcipher;
		struct ahash_alg ahash;
		ax_cipher_rng_alg_s rng;
	} alg;
	uint32_t align_size;
} ax_cipher_alg_template_s;

typedef struct {
	ax_cipher_crypto_priv_s *priv;
	ax_cipher_alg_template_s *tmpl;
	union {
		ax_cipher_skcipher_data_s skcipher;
		ax_cipher_ahash_data_s ahash;
		ax_cipher_akcipher_data_s akcipher;
	} data;
	int (*handle)(struct crypto_async_request *req);
	void *addr_vir;
} ax_cipher_ctx_s;


int ax_cipher_crypto_init(ax_cipher_crypto_priv_s *priv);
void ax_cipher_crypto_deinit(ax_cipher_crypto_priv_s *priv);
#endif
