/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifdef CONFIG_AX_CIPHER_CRYPTO

#include "ax_cipher_crypto.h"
#include "ax_cipher_proc.h"
#include "eip130_drv.h"
#include "ax_cipher_token_crypto.h"
#include "ax_cipher_proc.h"

typedef struct {
	ax_cipher_ctx_s *ctx;
	uint64_t src_addr;
	uint64_t dst_addr;
	int data_len;
} ax_skcipher_run_data_s;

typedef struct {
	ax_cipher_ctx_s *ctx;
	uint64_t src_addr;
	int data_len;
	bool is_final;
	bool key_cal;
	uint8_t *key;
} ax_ahash_run_data_s;

typedef struct {
	uint64_t dst_addr;
	int data_len;
} ax_rng_run_data_s;

typedef struct {
	ax_cipher_ctx_s *ctx;
	uint64_t src_addr;
	uint64_t dst_addr;
	uint64_t n_addr;
	uint64_t d_addr;
	int src_len;
	int dst_len;
} ax_akcipher_run_data_s;

extern struct mutex g_cipher_mutex;
extern uint32_t trng_configed;

static int check_alignment(struct scatterlist *sg_src,
		struct scatterlist *sg_dst,
		int align_mask)
{
	int in, out, align;

	in = IS_ALIGNED((uint32_t)sg_src->offset, 4) &&
		IS_ALIGNED((uint32_t)sg_src->length, align_mask);
	if (!sg_dst)
		return in;
	out = IS_ALIGNED((uint32_t)sg_dst->offset, 4) &&
		IS_ALIGNED((uint32_t)sg_dst->length, align_mask);
	align = in && out;

	return (align && (sg_src->length == sg_dst->length));
}

static int ax_skcipher_setkey(struct crypto_skcipher *ctfm, const u8 *key,
		unsigned int len)
{
	ax_cipher_ctx_s *ctx = crypto_skcipher_ctx(ctfm);
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
#ifdef AX_CE_KERNEL5
	int ret;
#endif

	if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_CIPHER_AES) {
		if (len != AES_KEYSIZE_128 && len != AES_KEYSIZE_192 &&
				len != AES_KEYSIZE_256) {
			//CE_LOG_PR(CE_ERR_LEVEL, "ken len = %u", len);
			return -EINVAL;
		}
#ifdef AX_CE_KERNEL5
	} else if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_CIPHER_DES) {
		if (tmpl->des3) {
			ret = verify_skcipher_des3_key(ctfm, key);
		} else {
			ret = verify_skcipher_des_key(ctfm, key);
		}
		if (ret) {
			return ret;
		}
#endif
	}
	memcpy(ctx->data.skcipher.key, key, len);
	ctx->data.skcipher.key_len = len;
	return 0;
}

static int ax_cipher_queue_req(struct crypto_async_request *base, bool encrypt_final)
{
	int ret;
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(base->tfm);
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	if (tmpl->type == AX_CIPHER_ALG_TYPE_SKCIPHER) {
		ctx->data.skcipher.encrypt = encrypt_final;
	} else if (tmpl->type == AX_CIPHER_ALG_TYPE_AHASH) {
		ctx->data.ahash.is_final = encrypt_final;
	} else if (tmpl->type == AX_CIPHER_ALG_TYPE_AKCIPHER) {
		ctx->data.akcipher.encrypt = encrypt_final;
	}
	spin_lock(&priv->queue_lock);
	ret = crypto_enqueue_request(&priv->queue, base);
	spin_unlock(&priv->queue_lock);
	queue_work(priv->workqueue, &priv->work);
	return ret;

}

static int ax_skcipher_encrypt(struct skcipher_request *req)
{
	return ax_cipher_queue_req(&req->base, true);
}

static int ax_skcipher_decrypt(struct skcipher_request *req)
{
	return ax_cipher_queue_req(&req->base, false);
}

static int ax_skcipher_run(ax_skcipher_run_data_s *run_data)
{
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	uint8_t algorithm;
	int proc_index;
	uint64_t start_us;
	int ret = 0;
	ax_cipher_ctx_s *ctx = run_data->ctx;
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	AX_CIPHER_TokenModeCipher mode = tmpl->mode;
	uint8_t algo = tmpl->cipher_alg.algo;
	uint32_t blocksize = tmpl->alg.skcipher.base.cra_blocksize;
	int first = 0;
	int second = 0;
	int data_len = run_data->data_len;

	if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_CIPHER_AES) {
		algorithm = EIP130TOKEN_CRYPTO_ALGO_AES;
		if (run_data->data_len % 16) {
			CE_LOG_PR(CE_ERR_LEVEL, "data len must 16byte align");
			return AX_CIPHER_INVALID_PARAMETER;
		}
	} else if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_CIPHER_DES) {
		algorithm = EIP130TOKEN_CRYPTO_ALGO_DES;
		if (run_data->data_len % 8) {
			CE_LOG_PR(CE_ERR_LEVEL, "data len must 8byte align");
			return AX_CIPHER_INVALID_PARAMETER;
		}
		if (ctx->data.skcipher.key_len == 24) {
			algorithm = EIP130TOKEN_CRYPTO_ALGO_3DES;
			algo = AX_CIPHER_ALGO_CIPHER_DES + 1;
		}
	}

	ax_mutex_lock(&g_cipher_mutex);
	if (mode == TOKEN_MODE_CIPHER_CTR) {
		uint64_t count = 0;
		uint8_t *p_count = (uint8_t *)&count;
		int block_cnt = data_len / blocksize;
		p_count[0] = ctx->data.skcipher.iv[blocksize - 1];
		p_count[1] = ctx->data.skcipher.iv[blocksize - 2];
		p_count[2] = ctx->data.skcipher.iv[blocksize - 3];
		p_count[3] = ctx->data.skcipher.iv[blocksize - 4];
		if (count + block_cnt + 1 > 0xffffffff) {
			second = (count + block_cnt) - 0xffffffff - 1;
		}
		if (second > 0) {
			first = block_cnt - second;
			data_len = first * blocksize;
		} else {
			first = block_cnt;
		}
	}
	proc_index = ax_cipher_get_proc_class1_index(algo, (uint8_t)mode, (uint8_t)ctx->data.skcipher.encrypt, (uint8_t)ctx->data.skcipher.key_len);
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
exec_token:
	Eip130Token_Command_Crypto_Operation(&ctoken, algorithm, (uint8_t)mode, ctx->data.skcipher.encrypt,
										 data_len);
	Eip130Token_Command_Crypto_SetDataAddresses(&ctoken, run_data->src_addr, data_len,
			run_data->dst_addr, data_len);
	Eip130Token_Command_Crypto_CopyKey(&ctoken, ctx->data.skcipher.key, ctx->data.skcipher.key_len);
	Eip130Token_Command_Crypto_SetKeyLength(&ctoken, ctx->data.skcipher.key_len);
	if (mode != TOKEN_MODE_CIPHER_ECB) {
		// From token
		Eip130Token_Command_Crypto_CopyIV(&ctoken, ctx->data.skcipher.iv);
	}
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if (second > 0) {
		uint32_t value = first;
		uint32_t carry = 0;
		uint8_t *p_iv = ctx->data.skcipher.iv;
		int i;
		run_data->src_addr += data_len;
		run_data->dst_addr += data_len;
		data_len = second * blocksize;
		for (i = 0; i < blocksize; i++) {
			value += carry + p_iv[blocksize - 1 - i];
			carry = value >> 8;
			p_iv[blocksize - 1 - i] = value & 0xff;
			value = 0;
		}
		second = 0;
		goto exec_token;
	}
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(run_data->data_len, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CTR) {
		Eip130Token_Result_ReadByteArray(&rtoken, 2, 16, ctx->data.skcipher.iv);
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}

static int ax_ahash_run(ax_ahash_run_data_s *run_data)
{
	ax_bool_ce init_with_default, finalize;
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int proc_index;
	uint32_t key_len = 0;
	bool is_mac = false;
	uint64_t start_us;
	int ret = 0;
	ax_cipher_ctx_s *ctx = run_data->ctx;
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	uint8_t algo;
	uint8_t hash_type;
	uint32_t digist_len = 0;
	switch (tmpl->cipher_alg.algo) {
	case AX_CIPHER_ALGO_HASH_SHA1:
		hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA1;
		digist_len = 20;
		break;
	case AX_CIPHER_ALGO_HASH_SHA224:
		hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA224;
		digist_len = 28;
		break;
	case AX_CIPHER_ALGO_HASH_SHA256:
		hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA256;
		digist_len = 32;
		break;
	case AX_CIPHER_ALGO_HASH_SHA384:
		hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA384;
		digist_len = 48;
		break;
	case AX_CIPHER_ALGO_HASH_SHA512:
		hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA512;
		digist_len = 64;
		break;
	case AX_CIPHER_ALGO_MAC_HMAC_SHA1:
		if (run_data->key_cal) {
			hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA1;
		} else {
			hash_type = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA1;
			is_mac = true;
		}
		digist_len = 20;
		break;
	case AX_CIPHER_ALGO_MAC_HMAC_SHA224:
		if (run_data->key_cal) {
			hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA224;
		} else {
			hash_type = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224;
			is_mac = true;
		}
		digist_len = 28;
		break;
	case AX_CIPHER_ALGO_MAC_HMAC_SHA256:
		if (run_data->key_cal) {
			hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA256;
		} else {
			hash_type = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256;
			is_mac = true;
		}
		digist_len = 32;
		break;
	case AX_CIPHER_ALGO_MAC_HMAC_SHA384:
		if (run_data->key_cal) {
			hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA384;
		} else {
			hash_type = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA384;
			is_mac = true;
		}
		digist_len = 48;
		break;
	case AX_CIPHER_ALGO_MAC_HMAC_SHA512:
		if (run_data->key_cal) {
			hash_type = EIP130TOKEN_HASH_ALGORITHM_SHA512;
		} else {
			hash_type = EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA512;
			is_mac = true;
		}
		digist_len = 64;
		break;
	default:
		return AX_CIPHER_INVALID_ALGORITHM;
	}
	if ((hash_type == EIP130TOKEN_HASH_ALGORITHM_SHA224
			|| hash_type == EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224)) {
		digist_len = 32;
	}
	if ((hash_type == EIP130TOKEN_HASH_ALGORITHM_SHA384
			|| hash_type == EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA384)) {
		digist_len = 64;
	}
	if (is_mac) {
		key_len = ctx->data.ahash.key_len;
	}
	ax_mutex_lock(&g_cipher_mutex);
	ctx->data.ahash.total_len += run_data->data_len;
	if (ctx->data.ahash.is_first) {
		init_with_default = AX_TRUE_CE;
	} else {
		init_with_default = AX_FALSE_CE;
	}
	if (run_data->key_cal) {
		init_with_default = AX_TRUE_CE;
	}
	if (run_data->is_final) {
		finalize = AX_TRUE_CE;
	} else {
		finalize = AX_FALSE_CE;
	}
	ctx->data.ahash.is_first = false;
	memset(&ctoken, 0, sizeof(ctoken));
	if (is_mac) {
		algo = (uint8_t)hash_type + AX_CIPHER_ALGO_MAC_HMAC_SHA1 - 1;
	} else {
		algo = (uint8_t)hash_type - 1;
	}
	proc_index = ax_cipher_get_proc_class1_index(algo, AX_CIPHER_MODE_CIPHER_MAX, 0, key_len);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	if (is_mac) {
		Eip130Token_Command_Mac(&ctoken, (uint8_t)hash_type, init_with_default, finalize, run_data->src_addr,
								run_data->data_len);
		Eip130Token_Command_Mac_CopyMAC(&ctoken, (uint8_t *)ctx->data.ahash.state, digist_len);
		Eip130Token_Command_Mac_CopyKey(&ctoken, (uint8_t *)ctx->data.ahash.key, key_len);
		if (finalize) {
			Eip130Token_Command_Mac_SetTotalMessageLength(&ctoken, ctx->data.ahash.total_len);
		}
	} else {
		Eip130Token_Command_Hash(&ctoken, (uint8_t)hash_type, init_with_default, finalize, run_data->src_addr,
								 run_data->data_len);
		if (!run_data->key_cal) {
			Eip130Token_Command_Hash_CopyDigest(&ctoken, (uint8_t *)ctx->data.ahash.state, digist_len);
		}
		if (finalize) {
			if (run_data->key_cal) {
				Eip130Token_Command_Hash_SetTotalMessageLength(&ctoken, run_data->data_len);
			} else {
				Eip130Token_Command_Hash_SetTotalMessageLength(&ctoken, ctx->data.ahash.total_len);
			}
		}
	}
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(run_data->data_len, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	if (ret == AX_CIPHER_SUCCESS) {
		if (run_data->key_cal) {
			Eip130Token_Result_Hash_CopyDigest(&rtoken, digist_len, (uint8_t *)run_data->key);
		} else {
			Eip130Token_Result_Hash_CopyDigest(&rtoken, digist_len, (uint8_t *)ctx->data.ahash.state);
		}
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}

static int ax_skcipher_handle(struct crypto_async_request *async)
{
	int ret = 0;
	int nr_src, nr_dst;
	int left_bytes, total_bytes;
	struct scatterlist *sg_src, *sg_dst, *first, *sg_unmap;
	struct scatterlist sg_tmp;
	bool align = true;
	int ivsize;
	uint32_t count;
	ax_skcipher_run_data_s run_data;

	struct skcipher_request *req = skcipher_request_cast(async);
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(req->base.tfm);
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	struct crypto_skcipher *skcipher = crypto_skcipher_reqtfm(req);
	ivsize = crypto_skcipher_ivsize(skcipher);
	memcpy(ctx->data.skcipher.iv, req->iv, ivsize);
	left_bytes = req->cryptlen;
	total_bytes = req->cryptlen;
	nr_src = sg_nents_for_len(req->src, req->cryptlen);
	nr_dst = sg_nents_for_len(req->dst, req->cryptlen);
	sg_src = req->src;
	first = req->src;
	sg_dst = req->dst;
	run_data.ctx= ctx;
	if (!IS_ALIGNED(total_bytes, tmpl->align_size)) {
		if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CTR) {
			align = false;
		} else {
			//CE_LOG_PR(CE_ERR_LEVEL, "align error, total_bytes = %d, align_size = %u",
			//		total_bytes, tmpl->align_size);
			return -EINVAL;
		}
	}
	if (total_bytes && tmpl->mode == AX_CIPHER_MODE_CIPHER_CBC && !ctx->data.skcipher.encrypt) {
		if (!sg_pcopy_to_buffer(first, nr_src, req->iv, ivsize, total_bytes - ivsize)) {
			CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
			return -EINVAL;
		}
	}
	while (left_bytes) {
		uint8_t new_iv[32];
		align = align ? check_alignment(sg_src, sg_dst, tmpl->align_size) : align;
		if (align) {
			count = (uint32_t)min(left_bytes, (int)sg_src->length);
			left_bytes -= count;
			if (!dma_map_sg(priv->dev, sg_src, 1, DMA_TO_DEVICE)) {
				CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(src)  error");
				return -EINVAL;
			}
			run_data.src_addr = sg_dma_address(sg_src);

			if (sg_dst) {
				if (!dma_map_sg(priv->dev, sg_dst, 1, DMA_FROM_DEVICE)) {
					CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(dst)  error");
					dma_unmap_sg(priv->dev, sg_src, 1, DMA_TO_DEVICE);
					return -EINVAL;
				}
				run_data.dst_addr = sg_dma_address(sg_dst);
			}
		} else {
			count = (left_bytes > PAGE_SIZE) ? PAGE_SIZE : left_bytes;

			if (!sg_pcopy_to_buffer(first, nr_src,
						ctx->addr_vir, count,
						total_bytes - left_bytes)) {
				CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
				return -EINVAL;
			}
			left_bytes -= count;
			sg_init_one(&sg_tmp, ctx->addr_vir, count);
			if (!dma_map_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE)) {
				CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
				return -ENOMEM;
			}
			run_data.src_addr = sg_dma_address(&sg_tmp);

			if (sg_dst) {
				if (!dma_map_sg(priv->dev, &sg_tmp, 1,
							DMA_FROM_DEVICE)) {
					CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
					dma_unmap_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE);
					return -ENOMEM;
				}
				run_data.dst_addr = sg_dma_address(&sg_tmp);
			}
		}
		if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CBC && !ctx->data.skcipher.encrypt) {
			if (!sg_pcopy_to_buffer(first, nr_src,
						new_iv, ivsize,
						total_bytes - (left_bytes + ivsize))) {
				CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
				ret = -EINVAL;
				goto finish;
			}
		}
		run_data.data_len = ALIGN(count, tmpl->align_size);
		ret = ax_skcipher_run(&run_data);
finish:
		sg_unmap = align ? sg_src: &sg_tmp;
		dma_unmap_sg(priv->dev, sg_unmap, 1, DMA_TO_DEVICE);
		if (sg_dst) {
			sg_unmap = align ? sg_dst: &sg_tmp;
			dma_unmap_sg(priv->dev, sg_unmap, 1, DMA_FROM_DEVICE);
		}
		if (ret) {
			return ret;
		}
		if (!align) {
			if (!sg_pcopy_from_buffer(req->dst, nr_dst,
						ctx->addr_vir, count,
						total_bytes - left_bytes - count)) {
				CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
				return -EINVAL;
			}
		}
		if (left_bytes) {
			if (ivsize) {
				if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CBC && ctx->data.skcipher.encrypt) {
					if (align) {
						memcpy(ctx->data.skcipher.iv, sg_virt(sg_dst) + sg_dst->length - ivsize, ivsize);
					} else {
						memcpy(ctx->data.skcipher.iv, ctx->addr_vir + count - ivsize, ivsize);
					}
				} else if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CBC && !ctx->data.skcipher.encrypt) {
					memcpy(ctx->data.skcipher.iv, new_iv, ivsize);
				}
			}
			if (align) {
				if (sg_is_last(sg_src)) {
					CE_LOG_PR(CE_ERR_LEVEL, "Lack of data");
					return -ENOMEM;
				}
				sg_src = sg_next(sg_src);
				sg_dst = sg_next(sg_dst);
			}
		} else {
			if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CBC && ctx->data.skcipher.encrypt) {
				if (align) {
					memcpy(req->iv, sg_virt(sg_dst) + sg_dst->length - ivsize, ivsize);
				} else {
					memcpy(req->iv, ctx->addr_vir + count - ivsize, ivsize);
				}
			} else if (tmpl->mode == AX_CIPHER_MODE_CIPHER_CTR) {
				memcpy(req->iv, ctx->data.skcipher.iv, ivsize);
			}
		}
	}
	return ret;
}

static int ax_ahash_handle(struct crypto_async_request *async)
{
	int ret = 0;
	int nr_src;
	int left_bytes, total_bytes;
	struct scatterlist *sg_src, *first, *sg_unmap;
	struct scatterlist sg_tmp;
	bool align;
	uint32_t count;
	ax_ahash_run_data_s run_data;
	uint32_t blk_size = 0;

	struct ahash_request *req = ahash_request_cast(async);
	ax_cipher_ctx_s *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	left_bytes = req->nbytes;
	total_bytes = req->nbytes;
	nr_src = sg_nents_for_len(req->src, req->nbytes);
	sg_src = req->src;
	first = req->src;
	run_data.ctx= ctx;
	run_data.key_cal = false;
	align = ctx->data.ahash.is_align;
	if (left_bytes == 0) {
		if (ctx->data.ahash.is_final
				&& !ctx->data.ahash.cache_size) {
			run_data.data_len = 0;
			run_data.src_addr = 0;
			run_data.is_final = true;
			ret = ax_ahash_run(&run_data);
		}
	}

	while (left_bytes || ctx->data.ahash.cache_size) {
		align = align ? check_alignment(sg_src, NULL, tmpl->align_size) : align;
		ctx->data.ahash.is_align = align;
		if (align) {
			count = (uint32_t)min(left_bytes, (int)sg_src->length);
			left_bytes -= count;
			if (!dma_map_sg(priv->dev, sg_src, 1, DMA_TO_DEVICE)) {
				CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(src)  error");
				return -EINVAL;
			}
			run_data.src_addr = sg_dma_address(sg_src);
			blk_size = count;
		} else {
			uint32_t cache_remain = PAGE_SIZE - ctx->data.ahash.cache_size;
			count = (left_bytes > cache_remain) ? cache_remain : left_bytes;

			if (ctx->data.ahash.cache_size && ctx->data.ahash.cache_offset) {
				memcpy(ctx->addr_vir,
						ctx->addr_vir + ctx->data.ahash.cache_offset,
						ctx->data.ahash.cache_size);
			}

			if (count) {
				if (!sg_pcopy_to_buffer(first, nr_src,
							ctx->addr_vir + ctx->data.ahash.cache_size, count,
							total_bytes - left_bytes)) {
					CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
					return -EINVAL;
				}
			}
			left_bytes -= count;
			if (ctx->data.ahash.is_final) {
				blk_size = (count + ctx->data.ahash.cache_size);
				ctx->data.ahash.cache_size = 0;
				ctx->data.ahash.cache_offset = 0;
			} else {
				blk_size = (count + ctx->data.ahash.cache_size) / tmpl->align_size * tmpl->align_size;
				ctx->data.ahash.cache_size = (count + ctx->data.ahash.cache_size) % tmpl->align_size;
				if (ctx->data.ahash.cache_size) {
					ctx->data.ahash.cache_offset = blk_size;
				} else {
					ctx->data.ahash.cache_offset = 0;
				}
			}

			if (blk_size) {
				sg_init_one(&sg_tmp, ctx->addr_vir, blk_size);
				if (!dma_map_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE)) {
					CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
					return -ENOMEM;
				}
				run_data.src_addr = sg_dma_address(&sg_tmp);
			}
		}

		if (blk_size) {
			if (ctx->data.ahash.is_final &&
					(left_bytes + ctx->data.ahash.cache_size) == 0) {
				run_data.is_final = true;
			} else {
				run_data.is_final = false;
			}
			run_data.data_len = blk_size;
			ret = ax_ahash_run(&run_data);
			sg_unmap = align ? sg_src: &sg_tmp;
			dma_unmap_sg(priv->dev, sg_unmap, 1, DMA_TO_DEVICE);
			if (ret) {
				return ret;
			}
		}
		if (left_bytes) {
			if (align) {
				if (sg_is_last(sg_src)) {
					CE_LOG_PR(CE_ERR_LEVEL,
							"Lack of data, left_bytes = %d sg_src->length = %d",
							left_bytes, sg_src->length);
					return -ENOMEM;
				}
				sg_src = sg_next(sg_src);
			}
		}
		if (!left_bytes) {
			break;
		}
	}

	req->nbytes = 0;

	if (!ret && ctx->data.ahash.is_final) {
		uint32_t digist_len = 0;
		switch (tmpl->cipher_alg.algo) {
			case AX_CIPHER_ALGO_HASH_SHA1:
				digist_len = 20;
				break;
			case AX_CIPHER_ALGO_HASH_SHA224:
				digist_len = 28;
				break;
			case AX_CIPHER_ALGO_HASH_SHA256:
				digist_len = 32;
				break;
			case AX_CIPHER_ALGO_HASH_SHA384:
				digist_len = 48;
				break;
			case AX_CIPHER_ALGO_HASH_SHA512:
				digist_len = 64;
				break;
			case AX_CIPHER_ALGO_MAC_HMAC_SHA1:
				digist_len = 20;
				break;
			case AX_CIPHER_ALGO_MAC_HMAC_SHA224:
				digist_len = 28;
				break;
			case AX_CIPHER_ALGO_MAC_HMAC_SHA256:
				digist_len = 32;
				break;
			case AX_CIPHER_ALGO_MAC_HMAC_SHA384:
				digist_len = 48;
				break;
			case AX_CIPHER_ALGO_MAC_HMAC_SHA512:
				digist_len = 64;
				break;
			default:
				break;
		}
		memcpy(req->result, ctx->data.ahash.state, digist_len);
	}

	return ret;
}

static int ax_cipher_skcipher_cra_init(struct crypto_tfm *tfm)
{
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(tfm);
	ax_cipher_alg_template_s *tmpl;

	tmpl = container_of(tfm->__crt_alg, ax_cipher_alg_template_s, alg.skcipher.base);
	ctx->priv = tmpl->priv;
	ctx->tmpl = tmpl;
	ctx->handle = ax_skcipher_handle;
	ctx->addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (ctx->addr_vir == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "create_singlethread_workqueue failed");
		return -ENOMEM;
	}
	return 0;
}

static int ax_cipher_ahash_cra_init(struct crypto_tfm *tfm)
{
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(tfm);
	ax_cipher_alg_template_s *tmpl;

	tmpl = container_of(tfm->__crt_alg, ax_cipher_alg_template_s, alg.ahash.halg.base);
	ctx->priv = tmpl->priv;
	ctx->tmpl = tmpl;
	ctx->handle = ax_ahash_handle;
	ctx->addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (ctx->addr_vir == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "create_singlethread_workqueue failed");
		return -ENOMEM;
	}
	return 0;
}

static void ax_cipher_cra_exit(struct crypto_tfm *tfm)
{
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(tfm);
	free_page((unsigned long)ctx->addr_vir);
}

ax_cipher_alg_template_s ax_cipher_alg_ecb_des = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_DES,
	.mode = AX_CIPHER_MODE_CIPHER_ECB,
	.des3 = false,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = DES_KEY_SIZE,
		.max_keysize = DES_KEY_SIZE,
		.base = {
			.cra_name = "ecb(des)",
			.cra_driver_name = "axcipher-ecb-des",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = DES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0x7,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 8,
};

ax_cipher_alg_template_s ax_cipher_alg_cbc_des = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_DES,
	.mode = AX_CIPHER_MODE_CIPHER_CBC,
	.des3 = false,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = DES_KEY_SIZE,
		.max_keysize = DES_KEY_SIZE,
		.ivsize = DES_BLOCK_SIZE,
		.base = {
			.cra_name = "cbc(des)",
			.cra_driver_name = "axcipher-cbc-des",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = DES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0x7,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 8,
};

ax_cipher_alg_template_s ax_cipher_alg_ecb_des3 = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_DES,
	.mode = AX_CIPHER_MODE_CIPHER_ECB,
	.des3 = true,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = DES3_EDE_KEY_SIZE,
		.max_keysize = DES3_EDE_KEY_SIZE,
		.base = {
			.cra_name = "ecb(des3_ede)",
			.cra_driver_name = "axcipher-ecb-des3-ede",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = DES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0x7,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 8,
};

ax_cipher_alg_template_s ax_cipher_alg_cbc_des3 = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_DES,
	.mode = AX_CIPHER_MODE_CIPHER_CBC,
	.des3 = true,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = DES3_EDE_KEY_SIZE,
		.max_keysize = DES3_EDE_KEY_SIZE,
		.ivsize = DES3_EDE_BLOCK_SIZE,
		.base = {
			.cra_name = "cbc(des3_ede)",
			.cra_driver_name = "axcipher-cbc-des3-ede",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = DES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0x7,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 8,
};

ax_cipher_alg_template_s ax_cipher_alg_ecb_aes = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_AES,
	.mode = AX_CIPHER_MODE_CIPHER_ECB,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.base = {
			.cra_name = "ecb(aes)",
			.cra_driver_name = "axcipher-ecb-aes",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = AES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0xf,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 16,
};

ax_cipher_alg_template_s ax_cipher_alg_cbc_aes = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_AES,
	.mode = AX_CIPHER_MODE_CIPHER_CBC,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.base = {
			.cra_name = "cbc(aes)",
			.cra_driver_name = "axcipher-cbc-aes",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = AES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0xf,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 16,
};

ax_cipher_alg_template_s ax_cipher_alg_ctr_aes = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_AES,
	.mode = AX_CIPHER_MODE_CIPHER_CTR,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.base = {
			.cra_name = "ctr(aes)",
			.cra_driver_name = "axcipher-ctr-aes",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = AES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0xf,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 16,
};

ax_cipher_alg_template_s ax_cipher_alg_icm_aes = {
	.type = AX_CIPHER_ALG_TYPE_SKCIPHER,
	.cipher_alg.algo = AX_CIPHER_ALGO_CIPHER_AES,
	.mode = AX_CIPHER_MODE_CIPHER_ICM,
	.alg.skcipher = {
		.setkey = ax_skcipher_setkey,
		.encrypt = ax_skcipher_encrypt,
		.decrypt = ax_skcipher_decrypt,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.base = {
			.cra_name = "icm(aes)",
			.cra_driver_name = "axcipher-icm-aes",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_blocksize = AES_BLOCK_SIZE,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_alignmask = 0xf,
			.cra_init = ax_cipher_skcipher_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
	.align_size = 16,
};

static int  ax_cipher_ahash_init(struct ahash_request *areq)
{
	ax_cipher_ctx_s *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	ctx->data.ahash.cache_size = 0;
	ctx->data.ahash.cache_offset = 0;
	ctx->data.ahash.total_len = 0;
	ctx->data.ahash.is_final = false;
	ctx->data.ahash.is_first = true;
	ctx->data.ahash.is_align = true;
	memset(ctx->data.ahash.state, 0, 64);
	return 0;
}

static int ax_cipher_ahash_update(struct ahash_request *areq)
{
	return ax_cipher_queue_req(&areq->base, false);
}

static int ax_cipher_ahash_final(struct ahash_request *areq)
{
	return ax_cipher_queue_req(&areq->base, true);
}

static int ax_cipher_ahash_finup(struct ahash_request *areq)
{
	return ax_cipher_ahash_final(areq);
}

static int ax_cipher_ahash_digest(struct ahash_request *areq)
{
	ax_cipher_ahash_init(areq);
	return ax_cipher_ahash_finup(areq);
}

static int ax_cipher_ahash_export(struct ahash_request *areq, void *out)
{
	ax_cipher_ctx_s *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_ahash_state_s *export = (ax_cipher_ahash_state_s *)out;
	export->cache_size = ctx->data.ahash.cache_size;
	export->cache_offset = ctx->data.ahash.cache_offset;
	export->is_first = ctx->data.ahash.is_first;
	export->total_len = ctx->data.ahash.total_len;
	export->is_align = ctx->data.ahash.is_align;
	export->key_len = ctx->data.ahash.key_len;
	if (export->cache_size) {
		memcpy(export->cache, ctx->addr_vir + export->cache_offset, export->cache_size);
	}
	memcpy(export->state, ctx->data.ahash.state, 64);
	if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA1
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA224
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA256
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA512) {
		memcpy(export->key, ctx->data.ahash.key, export->key_len);
	}
	return 0;
}

static int ax_cipher_ahash_import(struct ahash_request *areq, const void *in)
{
	ax_cipher_ctx_s *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(areq));
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_ahash_state_s *export = (ax_cipher_ahash_state_s *)in;
	ctx->data.ahash.cache_size = export->cache_size;
	ctx->data.ahash.cache_offset = export->cache_offset;
	ctx->data.ahash.is_first = export->is_first;
	ctx->data.ahash.total_len = export->total_len;
	ctx->data.ahash.is_align = export->is_align;
	ctx->data.ahash.key_len = export->key_len;
	if (export->cache_size) {
		memcpy(ctx->addr_vir + export->cache_offset, export->cache, export->cache_size);
	}
	memcpy(ctx->data.ahash.state, export->state, 64);
	if (tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA1
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA224
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA256
			|| tmpl->cipher_alg.algo == AX_CIPHER_ALGO_MAC_HMAC_SHA512) {
		memcpy(ctx->data.ahash.key, export->key, export->key_len);
	}
	return 0;
}

static int ax_cipher_ahash_setkey(struct crypto_ahash *tfm, const u8 *key, unsigned int keylen)
{
	ax_cipher_ctx_s *ctx = crypto_ahash_ctx(tfm);
	ax_cipher_alg_template_s *tmpl = ctx->tmpl;
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	if (keylen > tmpl->align_size) {
		uint8_t *keydup;
		struct scatterlist sg;
		ax_ahash_run_data_s run_data;
		int ret;
		keydup = kmemdup(key, keylen, GFP_KERNEL);
		if (!keydup) {
			CE_LOG_PR(CE_ERR_LEVEL, "kmemdup error");
		}
		sg_init_one(&sg, keydup, keylen);
		if (!dma_map_sg(priv->dev, &sg, 1, DMA_TO_DEVICE)) {
			CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg)  error");
			return -ENOMEM;
		}
		run_data.src_addr = sg_dma_address(&sg);
		run_data.ctx= ctx;
		run_data.data_len = keylen;
		run_data.key_cal = true;
		run_data.is_final = true;
		run_data.key = ctx->data.ahash.key;
		ctx->data.ahash.key_len = crypto_ahash_digestsize(tfm);
		ret = ax_ahash_run(&run_data);
		dma_unmap_sg(priv->dev, &sg, 1, DMA_TO_DEVICE);
		kfree(keydup);
	} else {
		memcpy(ctx->data.ahash.key, key, keylen);
		ctx->data.ahash.key_len = keylen;
	}
	return 0;
}

ax_cipher_alg_template_s ax_cipher_alg_sha1 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_HASH_SHA1,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "sha1",
				.cra_driver_name = "axcipher-sha1",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA1_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_sha224 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_HASH_SHA224,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.halg = {
			.digestsize = SHA224_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "sha224",
				.cra_driver_name = "axcipher-sha224",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA224_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_sha256 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_HASH_SHA256,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.halg = {
			.digestsize = SHA256_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "sha256",
				.cra_driver_name = "axcipher-sha256",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA256_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_sha512 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_HASH_SHA512,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.halg = {
			.digestsize = SHA512_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "sha512",
				.cra_driver_name = "axcipher-sha512",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA512_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 128,
};

ax_cipher_alg_template_s ax_cipher_alg_sha384 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_HASH_SHA384,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.halg = {
			.digestsize = SHA384_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "sha384",
				.cra_driver_name = "axcipher-sha384",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA384_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 128,
};

ax_cipher_alg_template_s ax_cipher_alg_hmac_sha1 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_MAC_HMAC_SHA1,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.setkey = ax_cipher_ahash_setkey,
		.halg = {
			.digestsize = SHA1_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "hmac(sha1)",
				.cra_driver_name = "axcipher-hmac-sha1",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA1_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_hmac_sha224 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_MAC_HMAC_SHA224,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.setkey = ax_cipher_ahash_setkey,
		.halg = {
			.digestsize = SHA224_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "hmac(sha224)",
				.cra_driver_name = "axcipher-hmac-sha224",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA224_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_hmac_sha256 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_MAC_HMAC_SHA256,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.setkey = ax_cipher_ahash_setkey,
		.halg = {
			.digestsize = SHA256_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "hmac(sha256)",
				.cra_driver_name = "axcipher-hmac-sha256",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA256_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 64,
};

ax_cipher_alg_template_s ax_cipher_alg_hmac_sha384 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_MAC_HMAC_SHA384,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.setkey = ax_cipher_ahash_setkey,
		.halg = {
			.digestsize = SHA384_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "hmac(sha384)",
				.cra_driver_name = "axcipher-hmac-sha384",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA384_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 128,
};

ax_cipher_alg_template_s ax_cipher_alg_hmac_sha512 = {
	.type = AX_CIPHER_ALG_TYPE_AHASH,
	.cipher_alg.algo = AX_CIPHER_ALGO_MAC_HMAC_SHA512,
	.alg.ahash = {
		.init = ax_cipher_ahash_init,
		.update = ax_cipher_ahash_update,
		.final = ax_cipher_ahash_final,
		.finup = ax_cipher_ahash_finup,
		.digest = ax_cipher_ahash_digest,
		.export = ax_cipher_ahash_export,
		.import = ax_cipher_ahash_import,
		.setkey = ax_cipher_ahash_setkey,
		.halg = {
			.digestsize = SHA512_DIGEST_SIZE,
			.statesize = sizeof(ax_cipher_ahash_state_s),
			.base = {
				.cra_name = "hmac(sha512)",
				.cra_driver_name = "axcipher-hmac-sha512",
				.cra_priority = AX_CIPHER_CRA_PRIORITY,
				.cra_flags = CRYPTO_ALG_ASYNC |
					CRYPTO_ALG_KERN_DRIVER_ONLY |
					CRYPTO_TFM_REQ_MAY_SLEEP,
				.cra_blocksize = SHA512_BLOCK_SIZE,
				.cra_ctxsize = sizeof(ax_cipher_ctx_s),
				.cra_init = ax_cipher_ahash_cra_init,
				.cra_exit = ax_cipher_cra_exit,
				.cra_module = THIS_MODULE,
			},
		},
	},
	.align_size = 128,
};

static int ax_cipher_trng_config(
	uint8_t auto_seed,
	uint16_t sample_cycles,
	uint8_t  sample_div,
	uint8_t  noise_blocks,
	bool   reseed)
{
	int ret;
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	memset(&ctoken, 0, sizeof(ctoken));
	memset(&rtoken, 0, sizeof(rtoken));
	// Configure
	Eip130Token_Command_TRNG_Configure(
		&ctoken, auto_seed, sample_cycles,
		sample_div, noise_blocks);
	if (reseed) {
		// RRD = Reseed post-processor
		ctoken.W[2] |= BIT_1;
	}
	ctoken.W[0] |= 1;
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (rtoken.W[0] & (1 << 31))) {
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}

static int ax_rng_run(ax_rng_run_data_s *run_data)
{
	int ret;
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int proc_index;
	uint64_t start_us;

	ax_mutex_lock(&g_cipher_mutex);
	if (!trng_configed) {
		ret = ax_cipher_trng_config(0, 1, 0, 8, 1);
		if (ret < 0) {
			CE_LOG_PR(CE_ERR_LEVEL,"config fail\n");
			ax_mutex_unlock(&g_cipher_mutex);
			return AX_CIPHER_INTERNAL_ERROR;
		}
		trng_configed = 1;
	}
	memset(&ctoken, 0, sizeof(ctoken));
	proc_index = ax_cipher_get_proc_class1_index(AX_CIPHER_PROC_ALGO_TRNG_OFFSET, AX_CIPHER_MODE_CIPHER_MAX, 0, 0);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	Eip130Token_Command_RandomNumber_Generate(&ctoken, run_data->data_len, run_data->dst_addr);
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(run_data->data_len, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}

static int ax_cipher_rng_cra_init(struct crypto_tfm *tfm)
{
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(tfm);
	ax_cipher_alg_template_s *tmpl;

	tmpl = container_of(tfm->__crt_alg, ax_cipher_alg_template_s, alg.rng.rng_alg.base);
	ctx->priv = tmpl->priv;
	ctx->tmpl = tmpl;
	ctx->addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (ctx->addr_vir == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "__get_free_page failed");
		return -ENOMEM;
	}
	return 0;
}

#ifdef SUPPORT_HW_RANDOM
static int ax_cipher_trng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	uint32_t left_bytes = max;
	ax_rng_run_data_s run_data;
	struct scatterlist sg_tmp;
	uint32_t count;
	uint32_t offset = 0;
	ax_cipher_rng_alg_s *rng_alg = container_of(rng, ax_cipher_rng_alg_s, trng);
	int ret;
	void *addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (addr_vir == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "__get_free_page failed");
		return -ENOMEM;
	}

	while (left_bytes) {
		count = (left_bytes > PAGE_SIZE) ? PAGE_SIZE : left_bytes;
		left_bytes -= count;
		sg_init_one(&sg_tmp, addr_vir, count);
		if (!dma_map_sg(rng_alg->dev, &sg_tmp, 1, DMA_FROM_DEVICE)) {
			free_page((unsigned long)addr_vir);
			CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
			return offset;
		}
		run_data.dst_addr = sg_dma_address(&sg_tmp);
		run_data.data_len = count;
		ret = ax_rng_run(&run_data);
		dma_unmap_sg(rng_alg->dev, &sg_tmp, 1, DMA_FROM_DEVICE);
		if (ret) {
			free_page((unsigned long)addr_vir);
			return offset;
		}
		memcpy(buf + offset, addr_vir, count);
		offset += count;
	}
	free_page((unsigned long)addr_vir);
	return offset;

}
#endif

static int ax_cipher_rng_generate(struct crypto_rng *tfm,
		const u8 *src, unsigned int slen,
		u8 *dst, unsigned int dlen )
{
	uint32_t left_bytes = slen;
	ax_rng_run_data_s run_data;
	struct scatterlist sg_tmp;
	uint32_t count;
	uint32_t offset = 0;
	int ret;
	ax_cipher_ctx_s *ctx = crypto_rng_ctx(tfm);
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	while (left_bytes) {
		count = (left_bytes > PAGE_SIZE) ? PAGE_SIZE : left_bytes;
		left_bytes -= count;
		sg_init_one(&sg_tmp, ctx->addr_vir, count);
		if (!dma_map_sg(priv->dev, &sg_tmp, 1, DMA_FROM_DEVICE)) {
			CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
			return -ENOMEM;
		}
		run_data.dst_addr = sg_dma_address(&sg_tmp);
		run_data.data_len = count;
		ret = ax_rng_run(&run_data);
		dma_unmap_sg(priv->dev, &sg_tmp, 1, DMA_FROM_DEVICE);
		if (ret) {
			return ret;
		}
		memcpy(dst + offset, ctx->addr_vir, count);
		offset += count;
	}
	return 0;
}

static int ax_cipher_rng_seed(struct crypto_rng *tfm, const u8 *seed,
		unsigned int slen)
{
	return 0;
}

ax_cipher_alg_template_s ax_cipher_alg_rng = {
	.type = AX_CIPHER_ALG_TYPE_RNG,
	.alg.rng.rng_alg = {
		.generate = ax_cipher_rng_generate,
		.seed = ax_cipher_rng_seed,
		.seedsize = 0,
		.base = {
			.cra_name = "stdrng",
			.cra_driver_name = "axcipher-rng",
			.cra_priority = AX_CIPHER_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_TYPE_RNG,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_init = ax_cipher_rng_cra_init,
			.cra_exit = ax_cipher_cra_exit,
			.cra_module = THIS_MODULE,
		},
	},
};

static int ax_cipher_crypto_pkclaim(int key_bytes)
{
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int ret;
	/*val_AsymPkaClaim*/
	memset(&ctoken, 0, sizeof(ctoken));
	memset(&rtoken, 0, sizeof(rtoken));
	Eip130Token_Command_Pk_Claim(&ctoken, key_bytes / 4, 0, 0);
	Eip130Token_Command_SetTokenID(&ctoken, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (rtoken.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"pkaclaim fail %x\n", rtoken.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}

static int ax_cipher_crypto_pkload(uint64_t phy_addr, int key_bytes, int index)
{
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int ret;
	memset(&ctoken, 0, sizeof(ctoken));
	memset(&rtoken, 0, sizeof(rtoken));
	Eip130Token_Command_Pk_NumLoad(&ctoken, index, phy_addr, key_bytes);
	Eip130Token_Command_SetTokenID(&ctoken, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (rtoken.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"pkaload fail %x\n", rtoken.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}

static int ax_cipher_pk_public(ax_akcipher_run_data_s *run_data)
{
	int ret;
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int key_bytes = run_data->ctx->data.akcipher.n_len;
	memset(&ctoken, 0, sizeof(ctoken));
	memset(&rtoken, 0, sizeof(rtoken));
	ax_mutex_lock(&g_cipher_mutex);
	ax_cipher_crypto_pkclaim(key_bytes);
	ret = ax_cipher_crypto_pkload(run_data->n_addr, key_bytes, 0);
	if ((ret < 0)) {
		ax_cipher_crypto_pkclaim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}

	Eip130Token_Command_Pk_Operation(&ctoken, (uint8_t)VEXTOKEN_PK_OP_MODEXPE,
			run_data->ctx->data.akcipher.e,
			run_data->src_addr,
			key_bytes,
			run_data->dst_addr,
			key_bytes + 4);
	Eip130Token_Command_SetTokenID(&ctoken, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (rtoken.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"PKModExpE fail %x\n", rtoken.W[0]);
		ret = AX_CIPHER_INTERNAL_ERROR;
	}
	ax_cipher_crypto_pkclaim(0);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}

static int ax_cipher_pk_private(ax_akcipher_run_data_s *run_data)
{
	int ret;
	eip130_token_command_t ctoken;
	eip130_token_result_t rtoken;
	int key_bytes = run_data->ctx->data.akcipher.n_len;
	memset(&ctoken, 0, sizeof(ctoken));
	memset(&rtoken, 0, sizeof(rtoken));
	ax_mutex_lock(&g_cipher_mutex);
	ax_cipher_crypto_pkclaim(key_bytes);
	ret = ax_cipher_crypto_pkload(run_data->n_addr, key_bytes, 0);
	if ((ret < 0)) {
		ax_cipher_crypto_pkclaim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	ret = ax_cipher_crypto_pkload(run_data->d_addr, key_bytes, 1);
	if ((ret < 0)) {
		ax_cipher_crypto_pkclaim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}

	Eip130Token_Command_Pk_Operation(&ctoken, (uint8_t)VEXTOKEN_PK_OP_MODEXPD, 0,
			run_data->src_addr,
			key_bytes,
			run_data->dst_addr,
			key_bytes + 4);
	Eip130Token_Command_SetTokenID(&ctoken, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&ctoken, (uint32_t *)&rtoken, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (rtoken.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"PKModExpD fail %x\n", rtoken.W[0]);
		ret = AX_CIPHER_INTERNAL_ERROR;
	}
	ax_cipher_crypto_pkclaim(0);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}

static int ax_akcipher_run(ax_akcipher_run_data_s *run_data)
{
	int ret = 0;
	if (!run_data->ctx->data.akcipher.encrypt) {
		ret = ax_cipher_pk_private(run_data);
	} else {
		ret = ax_cipher_pk_public(run_data);
	}
	return ret;
}

static int ax_akcipher_handle(struct crypto_async_request *async)
{
	int ret = 0;
	int nr_src, nr_dst;
	int src_len, dst_len;
	struct scatterlist *sg_src, *sg_dst;
	struct scatterlist sg_tmp;
	ax_akcipher_run_data_s run_data;

	struct akcipher_request *req = container_of(async, struct akcipher_request, base);
	ax_cipher_ctx_s *ctx = crypto_tfm_ctx(req->base.tfm);
	uint32_t n_len = ctx->data.akcipher.n_len;
	ax_cipher_crypto_priv_s *priv = ctx->priv;
	src_len = req->src_len;
	dst_len = req->dst_len;
	nr_src = sg_nents_for_len(req->src, req->src_len);
	nr_dst = sg_nents_for_len(req->dst, req->dst_len);
	sg_src = req->src;
	sg_dst = req->dst;
	run_data.ctx= ctx;
	memset(ctx->addr_vir + n_len * 2, 0x0, n_len);
	if (!sg_pcopy_to_buffer(sg_src, nr_src,
				ctx->addr_vir + n_len * 2 + n_len - src_len, src_len, 0)) {
		CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
		return -EINVAL;
	}
	ax_cipher_convert_endian(ctx->addr_vir + n_len * 2, n_len);
	sg_init_one(&sg_tmp, ctx->addr_vir, n_len * 3);
	if (!dma_map_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE)) {
		CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
		return -ENOMEM;
	}
	run_data.n_addr = sg_dma_address(&sg_tmp);
	run_data.d_addr = run_data.n_addr + n_len;
	run_data.src_addr = run_data.d_addr + n_len;
	run_data.src_len = src_len;

	if (sg_dst) {
		if (!dma_map_sg(priv->dev, &sg_tmp, 1,
					DMA_FROM_DEVICE)) {
			CE_LOG_PR(CE_ERR_LEVEL, "dma_map_sg(sg_tmp)  error");
			dma_unmap_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE);
			return -ENOMEM;
		}
		run_data.dst_addr = run_data.src_addr;
		run_data.dst_len = dst_len;
	}

	ret = ax_akcipher_run(&run_data);
	dma_unmap_sg(priv->dev, &sg_tmp, 1, DMA_TO_DEVICE);
	if (sg_dst) {
		dma_unmap_sg(priv->dev, &sg_tmp, 1, DMA_FROM_DEVICE);
	}
	if (ret) {
		return ret;
	}
	ax_cipher_convert_endian(ctx->addr_vir + n_len * 2, n_len);
	if (!sg_pcopy_from_buffer(req->dst, nr_dst,
				ctx->addr_vir + n_len * 2, dst_len, 0)) {
		CE_LOG_PR(CE_ERR_LEVEL, "pcopy  error");
		return -EINVAL;
	}
	return ret;
}

static int ax_cipher_akcipher_init(struct crypto_akcipher *tfm)
{
	ax_cipher_ctx_s *ctx = akcipher_tfm_ctx(tfm);
	ax_cipher_alg_template_s *tmpl;

	tmpl = container_of(tfm->base.__crt_alg, ax_cipher_alg_template_s, alg.akcipher.base);
	ctx->priv = tmpl->priv;
	ctx->tmpl = tmpl;
	ctx->handle = ax_akcipher_handle;
	ctx->data.akcipher.n_len = 0;
	ctx->data.akcipher.e_len = 0;
	ctx->data.akcipher.d_len = 0;
	ctx->addr_vir = (void *)__get_free_page(GFP_KERNEL);
	if (ctx->addr_vir == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "__get_free_page failed");
		return -ENOMEM;
	}
	return 0;
}

static void ax_cipher_akcipher_exit(struct crypto_akcipher *tfm)
{
	ax_cipher_ctx_s *ctx = akcipher_tfm_ctx(tfm);
	free_page((unsigned long)ctx->addr_vir);
}

static int ax_akcipher_encrypt(struct akcipher_request *req)
{
	return ax_cipher_queue_req(&req->base, true);
}

static int ax_akcipher_decrypt(struct akcipher_request *req)
{
	return ax_cipher_queue_req(&req->base, false);
}

static int ax_cipher_akcipher_setkey(struct crypto_akcipher *tfm,
		const void *key, unsigned int keylen, bool private)
{
	int ret;
	struct rsa_key raw_key;
	ax_cipher_ctx_s *ctx = akcipher_tfm_ctx(tfm);
	if (!private) {
		ret = rsa_parse_pub_key(&raw_key, key, keylen);
		if (ret) {
			CE_LOG_PR(CE_ERR_LEVEL, "rsa_parse_pub_key error");
			return ret;
		}
		if (raw_key.n_sz != 128
				&& raw_key.n_sz != 256
				&& raw_key.n_sz != 384
				&& raw_key.e_sz > 4) {
			CE_LOG_PR(CE_ERR_LEVEL, "keylen is not support, n_sz = %ld, d_sz = %ld",
					(long)raw_key.n_sz, (long)raw_key.e_sz);
			return -EINVAL;
		}

		memcpy(ctx->addr_vir, raw_key.n, raw_key.n_sz);
		ctx->data.akcipher.n_len = raw_key.n_sz;
		memcpy(&ctx->data.akcipher.e, raw_key.e, raw_key.e_sz);
		ctx->data.akcipher.e_len = raw_key.e_sz;
		ax_cipher_convert_endian(ctx->addr_vir, raw_key.n_sz);
	} else {
		ret = rsa_parse_priv_key(&raw_key, key, keylen);
		if (ret) {
			CE_LOG_PR(CE_ERR_LEVEL, "rsa_parse_prive_key error");
			return ret;
		}
		if (raw_key.n_sz != 128
				&& raw_key.n_sz != 256
				&& raw_key.n_sz != 384) {
			CE_LOG_PR(CE_ERR_LEVEL, "keylen is not support, n_sz = %ld",
					(long)raw_key.n_sz);
			return -EINVAL;
		}

		memcpy(ctx->addr_vir, raw_key.n, raw_key.n_sz);
		ctx->data.akcipher.n_len = raw_key.n_sz;
		memset(ctx->addr_vir + raw_key.n_sz, 0, raw_key.n_sz);
		memcpy(ctx->addr_vir + raw_key.n_sz, raw_key.d, raw_key.d_sz);
		ctx->data.akcipher.d_len = raw_key.d_sz;
		memcpy(&ctx->data.akcipher.e, raw_key.e, raw_key.e_sz);
		ctx->data.akcipher.e_len = raw_key.e_sz;
		ax_cipher_convert_endian(ctx->addr_vir, raw_key.n_sz);
		ax_cipher_convert_endian(ctx->addr_vir + raw_key.n_sz, raw_key.n_sz);
	}
	return 0;
}

static int ax_cipher_akcipher_setpubkey(struct crypto_akcipher *tfm, const void *key,
		unsigned int keylen)
{
	return ax_cipher_akcipher_setkey(tfm, key, keylen, false);
}

static int ax_cipher_akcipher_setprivkey(struct crypto_akcipher *tfm, const void *key,
		unsigned int keylen)
{
	return ax_cipher_akcipher_setkey(tfm, key, keylen, true);
}

static unsigned int ax_cipher_akcipher_max_size(struct crypto_akcipher *tfm)
{
	ax_cipher_ctx_s *ctx = akcipher_tfm_ctx(tfm);

	return ctx->data.akcipher.n_len;
}

ax_cipher_alg_template_s ax_cipher_alg_rsa = {
	.type = AX_CIPHER_ALG_TYPE_AKCIPHER,
	.alg.akcipher = {
		.sign = ax_akcipher_decrypt,
		.verify = ax_akcipher_encrypt,
		.encrypt = ax_akcipher_encrypt,
		.decrypt = ax_akcipher_decrypt,
		.set_pub_key = ax_cipher_akcipher_setpubkey,
		.set_priv_key = ax_cipher_akcipher_setprivkey,
		.max_size = ax_cipher_akcipher_max_size,
		.init = ax_cipher_akcipher_init,
		.exit = ax_cipher_akcipher_exit,
		.base = {
			.cra_name = "rsa",
			.cra_driver_name = "axcipher-rsa",
			.cra_priority = 0,
			.cra_flags = CRYPTO_ALG_ASYNC |
				CRYPTO_ALG_KERN_DRIVER_ONLY |
				CRYPTO_TFM_REQ_MAY_SLEEP,
			.cra_ctxsize = sizeof(ax_cipher_ctx_s),
			.cra_module = THIS_MODULE,
		},
	},
};

ax_cipher_alg_template_s *cipher_algs[] = {
	&ax_cipher_alg_ecb_des,
	&ax_cipher_alg_cbc_des,
	&ax_cipher_alg_ecb_des3,
	&ax_cipher_alg_cbc_des3,
	&ax_cipher_alg_ecb_aes,
	&ax_cipher_alg_cbc_aes,
	&ax_cipher_alg_ctr_aes,
	&ax_cipher_alg_sha1,
	&ax_cipher_alg_sha224,
	&ax_cipher_alg_sha256,
	&ax_cipher_alg_sha384,
	&ax_cipher_alg_sha512,
	&ax_cipher_alg_hmac_sha1,
	&ax_cipher_alg_hmac_sha224,
	&ax_cipher_alg_hmac_sha256,
	&ax_cipher_alg_hmac_sha384,
	&ax_cipher_alg_hmac_sha512,
	&ax_cipher_alg_rng,
	&ax_cipher_alg_rsa,
};

static int cipher_register_algorithms(ax_cipher_crypto_priv_s *priv)
{
	int i, j, ret = 0;

	for (i = 0; i < ARRAY_SIZE(cipher_algs); i++) {
		cipher_algs[i]->priv = priv;
		if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_SKCIPHER) {
			ret = crypto_register_skcipher(&cipher_algs[i]->alg.skcipher);
		} else if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_AKCIPHER) {
			ret = crypto_register_akcipher(&cipher_algs[i]->alg.akcipher);
		} else if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_RNG) {
			ret = crypto_register_rng(&cipher_algs[i]->alg.rng.rng_alg);
#ifdef SUPPORT_HW_RANDOM
			if (ret == 0) {
				cipher_algs[i]->alg.rng.trng.name = "ax_trng";
				cipher_algs[i]->alg.rng.trng.read = ax_cipher_trng_read;
				cipher_algs[i]->alg.rng.trng.quality = 32768;
				cipher_algs[i]->alg.rng.dev = priv->dev;
				ret = devm_hwrng_register(priv->dev,
						&cipher_algs[i]->alg.rng.trng);
			}
#endif
		} else {
			ret = crypto_register_ahash(&cipher_algs[i]->alg.ahash);
		}

		if (ret)
			goto fail;
	}

	return 0;

fail:
	for (j = 0; j < i; j++) {
		if (cipher_algs[j]->type == AX_CIPHER_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&cipher_algs[j]->alg.skcipher);
		else if (cipher_algs[j]->type == AX_CIPHER_ALG_TYPE_AKCIPHER)
			crypto_unregister_akcipher(&cipher_algs[j]->alg.akcipher);
		else if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_RNG)
			crypto_unregister_rng(&cipher_algs[i]->alg.rng.rng_alg);
		else
			crypto_unregister_ahash(&cipher_algs[j]->alg.ahash);
	}

	return ret;
}

static void cipher_unregister_algorithms(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cipher_algs); i++) {
		if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_SKCIPHER)
			crypto_unregister_skcipher(&cipher_algs[i]->alg.skcipher);
		else if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_AKCIPHER)
			crypto_unregister_akcipher(&cipher_algs[i]->alg.akcipher);
		else if (cipher_algs[i]->type == AX_CIPHER_ALG_TYPE_RNG)
			crypto_unregister_rng(&cipher_algs[i]->alg.rng.rng_alg);
		else
			crypto_unregister_ahash(&cipher_algs[i]->alg.ahash);
	}
}

static void ax_cipher_dequeue(ax_cipher_crypto_priv_s *priv)
{
	struct crypto_async_request *req, *backlog;
	ax_cipher_ctx_s *ctx;
	int ret;
	spin_lock(&priv->queue_lock);
	backlog = crypto_get_backlog(&priv->queue);
	req = crypto_dequeue_request(&priv->queue);
	spin_unlock(&priv->queue_lock);

	if (!req) {
		return;
	}
	ctx = crypto_tfm_ctx(req->tfm);
	if (backlog)
		backlog->complete(backlog, -EINPROGRESS);
	ret = ctx->handle(req);
	req->complete(req, ret);
	queue_work(priv->workqueue, &priv->work);
}

static void ax_cipher_dequeue_work(struct work_struct *work)
{
	ax_cipher_crypto_priv_s *priv = container_of(work, ax_cipher_crypto_priv_s, work);
	ax_cipher_dequeue(priv);
}

int ax_cipher_crypto_init(ax_cipher_crypto_priv_s *priv)
{
	int ret;
	INIT_WORK(&priv->work, ax_cipher_dequeue_work);
	priv->workqueue =
		create_singlethread_workqueue("ax_cipher_wq");
	if (!priv->workqueue) {
		CE_LOG_PR(CE_ERR_LEVEL, "create_singlethread_workqueue failed");
		return -ENOMEM;
	}
	priv->busy = false;
	crypto_init_queue(&priv->queue, 100);
	spin_lock_init(&priv->queue_lock);
	ret = cipher_register_algorithms(priv);
	if (ret) {
		destroy_workqueue(priv->workqueue);
		CE_LOG_PR(CE_ERR_LEVEL, "Failed to register algorithms (%d)", ret);
		return ret;
	}
	return 0;
}

void ax_cipher_crypto_deinit(ax_cipher_crypto_priv_s *priv)
{
	destroy_workqueue(priv->workqueue);
	cipher_unregister_algorithms();
}

#endif
