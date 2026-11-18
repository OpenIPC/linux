/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifdef CONFIG_AX_CIPHER_MISC

#include "ax_cipher_adapt.h"
#include "eip130_drv.h"
#include "ax_cipher_ioctl.h"
#include "ax_cipher_token_crypto.h"
//#include "ax_printk.h"
#include "ax_cipher_proc.h"

#define HASH_RESULT_MAX_LEN_IN_WORD	  (16)
#define HASH_MAX_BLOCK_SIZE		 (0x100000)
#define CE_KEY_DATA_VHEADER	   (4)
#define AX_CIPHER_MAX_CHN 4
#define CE_DATA_WORD_B2WB(x)  (((x + 31) / 32))

typedef struct ax_cipher_chn_s {
	ax_bool_ce is_used;
	cipher_ctl_data_s cipher_ctrl;
} ax_cipher_chn_s;
static ax_cipher_chn_s   s_cipher_chn[AX_CIPHER_MAX_CHN];
extern struct mutex g_cipher_mutex;
extern uint32_t trng_configed;

static int cipher_param_check(AX_CIPHER_ALGO_E algorithm)
{
	switch (algorithm) {
	case AX_CIPHER_ALGO_CIPHER_AES:
		break;
	case AX_CIPHER_ALGO_CIPHER_DES:
		break;
	default:
		return AX_CIPHER_INVALID_PARAMETER;
	}
	return AX_CIPHER_SUCCESS;
}

static int cipher_handle_check(ax_cipher_handle handle)
{
	if (handle < AX_CIPHER_MAX_CHN)
		return AX_CIPHER_SUCCESS;
	else
		return AX_CIPHER_INVALID_PARAMETER;
}

static int cipher_create_handle(void *arg)
{
	int64_t i;
	int ret = AX_CIPHER_SUCCESS;
	cipher_ctl_data_s ctl;
	cipher_ctl_data_s *p_ctl;
	if (ax_copy_from_user(&ctl, (void *)arg, sizeof(cipher_ctl_data_s))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	if (cipher_param_check(ctl.alg) != AX_CIPHER_SUCCESS) {
		CE_LOG_PR(CE_ERR_LEVEL,"check fail, %d, %d\n", ctl.alg, ctl.work_mode);
		return AX_CIPHER_INVALID_PARAMETER;
	}
	ax_mutex_lock(&g_cipher_mutex);
	for (i = 0; i < AX_CIPHER_MAX_CHN; i++) {
		if (s_cipher_chn[i].is_used == AX_FALSE_CE) {
			break;
		}
	}
	memset(&s_cipher_chn[i], 0, sizeof(s_cipher_chn[i]));
	if (i < AX_CIPHER_MAX_CHN) {
		s_cipher_chn[i].is_used = AX_TRUE_CE;
		memcpy(&s_cipher_chn[i].cipher_ctrl, &ctl, sizeof(ctl));
		p_ctl = (cipher_ctl_data_s *)arg;
		ax_copy_to_user(&p_ctl->handle, &i, sizeof(ax_cipher_handle));
		ret = AX_CIPHER_SUCCESS;
	} else {
		ret = AX_CIPHER_FULL_ERROR;
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int cipher_destroy_handle(void *argp)
{
	ax_cipher_handle handle;
	int ret;
	if (ax_copy_from_user(&handle, (void *)argp, sizeof(ax_cipher_handle))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ax_mutex_lock(&g_cipher_mutex);
	if (handle < AX_CIPHER_MAX_CHN) {
		s_cipher_chn[handle].is_used = AX_FALSE_CE;
		ret = AX_CIPHER_SUCCESS;
	} else {
		ret = AX_CIPHER_INVALID_PARAMETER;
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int _cipher_encrypt(cipher_data_s *data, ax_bool_ce bIsEncrypt)
{
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	AX_CIPHER_TokenModeCipher mode;
	cipher_ctl_data_s *p_ctl;
	int u32_data_length;
	uint8_t algorithm;
	uint8_t algo;
	int proc_index;
	uint64_t start_us;
	p_ctl = &s_cipher_chn[data->handle].cipher_ctrl;
	mode = p_ctl->work_mode;
	u32_data_length = data->u32_data_length;
	algo = (uint8_t)p_ctl->alg;
	//AX_CIPHER_LOG_INFO("p_ctl:%x, data->handle: %d\n", p_ctl->alg, p_ctl->key_size);
	//AX_CIPHER_LOG_INFO("handle:%lld,alg:%d, key_size:%d,srcPhyAddr:%llx, dstPhyAddr:%llx, len: %x\n", data->handle,
	//				   p_ctl->alg, p_ctl->key_size, data->src_phyaddr, data->dst_phyaddr, u32_data_length);
	if (p_ctl->alg == AX_CIPHER_ALGO_CIPHER_AES) {
		algorithm = EIP130TOKEN_CRYPTO_ALGO_AES;
		if (u32_data_length % 16) {
			return AX_CIPHER_INVALID_PARAMETER;
		}
	} else if (p_ctl->alg == AX_CIPHER_ALGO_CIPHER_DES) {
		algorithm = EIP130TOKEN_CRYPTO_ALGO_DES;
		if (u32_data_length % 8) {
			return AX_CIPHER_INVALID_PARAMETER;
		}
		if (p_ctl->key_size == 24) {
			algorithm = EIP130TOKEN_CRYPTO_ALGO_3DES;
			algo = AX_CIPHER_ALGO_CIPHER_DES + 1;
		}
	}
	proc_index = ax_cipher_get_proc_class1_index(algo, (uint8_t)mode, (uint8_t)bIsEncrypt, (uint8_t)p_ctl->key_size);
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	Eip130Token_Command_Crypto_Operation(&command_token, algorithm, (uint8_t)mode, bIsEncrypt,
										 u32_data_length);
	Eip130Token_Command_Crypto_SetDataAddresses(&command_token, (uint64_t)data->src_phyaddr, u32_data_length,
			(uint64_t)data->dst_phyaddr, u32_data_length);
	Eip130Token_Command_Crypto_CopyKey(&command_token, p_ctl->p_key, p_ctl->key_size);
	Eip130Token_Command_Crypto_SetKeyLength(&command_token, p_ctl->key_size);
	if (mode != TOKEN_MODE_CIPHER_ECB) {
		// From token
		Eip130Token_Command_Crypto_CopyIV(&command_token, p_ctl->p_iv);
	}
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(u32_data_length, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	return ret;
}
static int cipher_encrypt(void *argp)
{
	cipher_data_s data;
	int ret;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_data_s))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ret = cipher_handle_check(data.handle);
	if (ret < 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"handle check fail\n");
		return AX_CIPHER_INVALID_PARAMETER;
	}
	ax_mutex_lock(&g_cipher_mutex);
	ret = _cipher_encrypt(&data, AX_TRUE_CE);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int cipher_decrypt(void *argp)
{
	cipher_data_s data;
	int ret;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_data_s))) {
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ret = cipher_handle_check(data.handle);
	if (ret < 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"handle check fail\n");
		return AX_CIPHER_INVALID_PARAMETER;
	}
	ax_mutex_lock(&g_cipher_mutex);
	ret = _cipher_encrypt(&data, AX_FALSE_CE);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int cipher_hash_update(void *argp)
{
	cipher_hash_data_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	ax_bool_ce fInit_with_default;
	cipher_hash_data_s *p_data = argp;
	int proc_index;
	uint64_t start_us;
	uint8_t key_len = 0;
	uint8_t algo;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_hash_data_s))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ax_mutex_lock(&g_cipher_mutex);
	if (data.b_isfirst) {
		fInit_with_default = AX_TRUE_CE;
	} else {
		fInit_with_default = AX_FALSE_CE;
	}
	memset(&command_token, 0, sizeof(command_token));
	if (data.b_ismac) {
		key_len = (uint8_t)data.u32_key_len;
		algo = (uint8_t)data.hash_type + AX_CIPHER_ALGO_MAC_HMAC_SHA1 - 1;
	} else {
		algo = (uint8_t)data.hash_type - 1;
	}
	proc_index = ax_cipher_get_proc_class1_index(algo, AX_CIPHER_MODE_CIPHER_MAX, 0, key_len);
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	if (data.b_ismac) {
		Eip130Token_Command_Mac(&command_token, (uint8_t)data.hash_type, fInit_with_default, AX_FALSE_CE, data.u64_data_phy,
								data.u32_data_len);
		Eip130Token_Command_Mac_CopyMAC(&command_token, (uint8_t *)data.u32_sha_val, sizeof(data.u32_sha_val));
		Eip130Token_Command_Mac_CopyKey(&command_token, (uint8_t *)data.u32_mac_key, data.u32_key_len);
	} else {
		Eip130Token_Command_Hash(&command_token, (uint8_t)data.hash_type, fInit_with_default, AX_FALSE_CE, data.u64_data_phy,
								 data.u32_data_len);
		Eip130Token_Command_Hash_CopyDigest(&command_token, (uint8_t *)data.u32_sha_val, sizeof(data.u32_sha_val));
	}
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(data.u32_data_len, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	if (ret == AX_CIPHER_SUCCESS) {
		Eip130Token_Result_Hash_CopyDigest(&result_token, HASH_RESULT_MAX_LEN_IN_WORD * 4,
										   (uint8_t *)data.u32_sha_val);
		ax_copy_to_user(p_data->u32_sha_val, data.u32_sha_val, sizeof(data.u32_sha_val));
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int cipher_hash_final(void *argp)
{
	cipher_hash_data_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	ax_bool_ce fInit_with_default;
	cipher_hash_data_s *p_data = argp;
	int proc_index;
	uint64_t start_us;
	uint8_t key_len = 0;
	uint8_t algo;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_hash_data_s))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ax_mutex_lock(&g_cipher_mutex);
	if (data.b_isfirst) {
		fInit_with_default = AX_TRUE_CE;
	} else {
		fInit_with_default = AX_FALSE_CE;
	}
	memset(&command_token, 0, sizeof(command_token));
	if (data.b_ismac) {
		key_len = (uint8_t)data.u32_key_len;
		algo = (uint8_t)data.hash_type + AX_CIPHER_ALGO_MAC_HMAC_SHA1 - 1;
	} else {
		algo = (uint8_t)data.hash_type - 1;
	}
	proc_index = ax_cipher_get_proc_class1_index(algo, AX_CIPHER_MODE_CIPHER_MAX, 0, key_len);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	if (data.b_ismac) {
		Eip130Token_Command_Mac(&command_token, (uint8_t)data.hash_type, fInit_with_default, AX_TRUE_CE, data.u64_data_phy,
								data.u32_data_len);
		Eip130Token_Command_Mac_CopyMAC(&command_token, (uint8_t *)data.u32_sha_val, sizeof(data.u32_sha_val));
		Eip130Token_Command_Mac_CopyKey(&command_token, (uint8_t *)data.u32_mac_key, data.u32_key_len);
		Eip130Token_Command_Mac_SetTotalMessageLength(&command_token, data.u64_total_data_len);
	} else {
		Eip130Token_Command_Hash(&command_token, (uint8_t)data.hash_type, fInit_with_default, AX_TRUE_CE, data.u64_data_phy,
								 data.u32_data_len);
		Eip130Token_Command_Hash_CopyDigest(&command_token, (uint8_t *)data.u32_sha_val, sizeof(data.u32_sha_val));
		Eip130Token_Command_Hash_SetTotalMessageLength(&command_token, data.u64_total_data_len);
	}
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(data.u32_data_len, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	if (ret == AX_CIPHER_SUCCESS) {
		Eip130Token_Result_Hash_CopyDigest(&result_token, sizeof(data.u32_sha_val), (uint8_t *)data.u32_sha_val);
		ax_copy_to_user(p_data->u32_sha_val, data.u32_sha_val, sizeof(data.u32_sha_val));
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static uint32_t ax_cipher_trng_config(
	uint8_t  auto_seed,
	uint16_t sample_cycles,
	uint8_t  sample_div,
	uint8_t  noise_blocks,
	ax_bool_ce   reseed)
{
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	// Configure
	Eip130Token_Command_TRNG_Configure(
		&command_token, auto_seed, sample_cycles,
		sample_div, noise_blocks);
	if (reseed) {
		// RRD = Reseed post-processor
		command_token.W[2] |= BIT_1;
	}
	command_token.W[0] |= 1;
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}
static int cipher_get_random_number(void *argp)
{
	cipher_rng_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	int proc_index;
	uint64_t start_us;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_rng_s))) {
		CE_LOG_PR(CE_ERR_LEVEL,"copy fail\n");
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
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
	memset(&command_token, 0, sizeof(command_token));
	proc_index = ax_cipher_get_proc_class1_index(AX_CIPHER_PROC_ALGO_TRNG_OFFSET, AX_CIPHER_MODE_CIPHER_MAX, 0, 0);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	Eip130Token_Command_RandomNumber_Generate(&command_token, data.u32_size, (uint64_t)data.u64_data_phy);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			uint32_t bw = ax_cipher_cal_bw(data.u32_size, period_us);
			ax_cipher_proc_update(proc_index, bw, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int ax_cipher_asset_alloc(uint64_t asset_policy, uint32_t asset_size, uint32_t *p_asset_id)
{
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	int ret;
	memset(&command_token, 0, sizeof(command_token));
	Eip130Token_Command_AssetCreate(&command_token, asset_policy, asset_size);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"alloc fail %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	*p_asset_id = result_token.W[1];
	return AX_CIPHER_SUCCESS;
}
static int ax_cipher_asset_free(uint32_t asset_id)
{
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	int ret;
	memset(&command_token, 0, sizeof(command_token));
	Eip130Token_Command_AssetDelete(&command_token, asset_id);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"free fail %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}
static int ax_cipher_asset_load(uint64_t phy_addr, uint32_t asset_id, uint32_t asset_size)
{
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	memset(&command_token, 0, sizeof(command_token));
	Eip130Token_Command_AssetLoad_Plaintext(&command_token, asset_id);
	Eip130Token_Command_AssetLoad_SetInput(&command_token, (uint64_t)phy_addr, asset_size);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"load fail %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}
static int cipher_rsa_verify(void *argp)
{
	cipher_rsa_verify_s data;
	int ret;
	int i;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t asset_id;
	int proc_index;
	uint64_t start_us;
	uint8_t sign_type = 0;
	uint8_t hash_type = 0;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_rsa_verify_s))) {
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ax_mutex_lock(&g_cipher_mutex);
	if (data.u64_policy & CE_POLICY_PK_RSA_PSS_SIGN) {
		sign_type = AX_CIPHER_PROC_RSA_PSS_SIGN;
	} else if (data.u64_policy & CE_POLICY_PK_RSA_PKCS1_SIGN) {
		sign_type = AX_CIPHER_PROC_RSA_PKCS1_SIGN;
	}
	if (data.u64_policy & CE_POLICY_SHA1) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA1;
	} else if (data.u64_policy & CE_POLICY_SHA224) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA224;
	} else if (data.u64_policy & CE_POLICY_SHA256) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA256;
	} else if (data.u64_policy & CE_POLICY_SHA384) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA384;
	} else if (data.u64_policy & CE_POLICY_SHA512) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA512;
	}
	proc_index = ax_cipher_get_proc_class2_index(AX_CIPHER_PROC_ALGO_RSA_SIGN_OFFSET + 1, sign_type, hash_type, data.u32_modulus_bits / 32);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	ret = ax_cipher_asset_alloc(data.u64_policy, data.u32_asset_size, &asset_id);
	if (ret < 0) {
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	ret = ax_cipher_asset_load(data.u64_key_phyaddr, asset_id, data.u32_asset_size);
	if (ret < 0) {
		ax_cipher_asset_free(asset_id);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	memset(&command_token, 0, sizeof(command_token));
	Eip130Token_Command_Pk_Asset_Command(&command_token, data.u32_method,
										 CE_DATA_WORD_B2WB(data.u32_modulus_bits), CE_DATA_WORD_B2WB(data.u32_public_exponent_bytes * 8),
										 (uint8_t)data.u32_salt_size, asset_id, 0,  0, 0, 0,
										 (uint64_t)data.u64_sig_phyaddr, (uint16_t)(data.u32_modulus_bits / 8 + 4));
	for (i = 0; i < (data.u32_hash_size / 4); i++) {
		command_token.W[12 + i] = data.hash_result[i];
	}
	command_token.W[3] |= data.u32_hash_size | (1 << 30);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			ax_cipher_proc_update(proc_index, (uint32_t)period_us, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	ax_cipher_asset_free(asset_id);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static int cipher_rsa_sign(void *argp)
{
	cipher_rsa_sign_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t asset_id;
	int proc_index;
	uint64_t start_us;
	uint8_t sign_type = 0;
	uint8_t hash_type = 0;
	int i;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_rsa_sign_s))) {
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	ax_mutex_lock(&g_cipher_mutex);
	if (data.u64_policy & CE_POLICY_PK_RSA_PSS_SIGN) {
		sign_type = AX_CIPHER_PROC_RSA_PSS_SIGN;
	} else if (data.u64_policy & CE_POLICY_PK_RSA_PKCS1_SIGN) {
		sign_type = AX_CIPHER_PROC_RSA_PKCS1_SIGN;
	}
	if (data.u64_policy & CE_POLICY_SHA1) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA1;
	} else if (data.u64_policy & CE_POLICY_SHA224) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA224;
	} else if (data.u64_policy & CE_POLICY_SHA256) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA256;
	} else if (data.u64_policy & CE_POLICY_SHA384) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA384;
	} else if (data.u64_policy & CE_POLICY_SHA512) {
		hash_type = AX_CIPHER_ALGO_HASH_SHA512;
	}
	proc_index = ax_cipher_get_proc_class2_index(AX_CIPHER_PROC_ALGO_RSA_SIGN_OFFSET, sign_type, hash_type, data.u32_modulus_bits / 32);
	start_us = 0;
	if (proc_index >= 0) {
		start_us = ax_cipher_proc_get_time_us();
		ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_START);
	}
	ret = ax_cipher_asset_alloc(data.u64_policy, data.u32_asset_size, &asset_id);
	if (ret < 0) {
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	ret = ax_cipher_asset_load(data.u64_key_phyaddr, asset_id, data.u32_asset_size);
	if (ret < 0) {
		ax_cipher_asset_free(asset_id);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	//AX_CIPHER_CacheInvalid(cmm.virAddr, CE_DATA_SIZE_VWB(key->modulusBits));
	memset(&command_token, 0, sizeof(command_token));
	Eip130Token_Command_Pk_Asset_Command(&command_token, data.u32_method,
										 CE_DATA_WORD_B2WB(data.u32_modulus_bits), CE_DATA_WORD_B2WB(data.u32_private_exponent_bytes * 8),
										 (uint8_t)data.u32_salt_size, asset_id, 0,  0, 0, 0,
										 (uint64_t)data.u64_sig_phyaddr, (uint16_t)data.u32_modulus_bits / 8 + 4);
	for (i = 0; i < (data.u32_hash_size / 4); i++) {
		command_token.W[12 + i] = data.hash_result[i];
	}
	command_token.W[3] |= data.u32_hash_size | (1 << 30);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if (proc_index >= 0) {
		if (ret == AX_CIPHER_SUCCESS) {
			uint64_t period_us = ax_cipher_proc_get_time_us() - start_us;
			ax_cipher_proc_update(proc_index, (uint32_t)period_us, AX_CIPHER_PROC_TASK_END);
		} else {
			ax_cipher_proc_update(proc_index, 0, AX_CIPHER_PROC_TASK_ERROR);
		}
	}
	ax_cipher_asset_free(asset_id);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static uint32_t ax_cipher_pka_claim(int key_bytes)
{
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t ret;
	/*val_AsymPkaClaim*/
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_Pk_Claim(&command_token, key_bytes / 4, 0, 0);
	Eip130Token_Command_SetTokenID(&command_token, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"pkaclaim fail %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}

static uint32_t ax_cipher_pka_load(uint64_t phy_addr, uint32_t key_bytes, uint32_t index)
{
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t ret;
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	Eip130Token_Command_Pk_NumLoad(&command_token, index, phy_addr, key_bytes);
	Eip130Token_Command_SetTokenID(&command_token, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"pkaload fail %x\n", result_token.W[0]);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	return AX_CIPHER_SUCCESS;
}
static uint32_t ax_cipher_pk_modexpe(void *argp)
{
	cipher_pk_modexpe_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t key_bytes;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_pk_modexpe_s))) {
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	ax_mutex_lock(&g_cipher_mutex);
	key_bytes = data.u32_modulus_bits / 8;
	ax_cipher_pka_claim(key_bytes);
	ret = ax_cipher_pka_load(data.u64_modulus_data_phyaddr, key_bytes, 0);
	if ((ret < 0)) {
		ax_cipher_pka_claim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}

	Eip130Token_Command_Pk_Operation(&command_token, (uint8_t)VEXTOKEN_PK_OP_MODEXPE, data.u32_public_exponent,
									 data.u64_in_phyaddr, key_bytes, data.u64_out_phyaddr, key_bytes + 4);
	Eip130Token_Command_SetTokenID(&command_token, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"PKModExpE fail %x\n", result_token.W[0]);
		ret = AX_CIPHER_INTERNAL_ERROR;
	}
	ax_cipher_pka_claim(0);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
static uint32_t ax_cipher_pk_modexpd(void *argp)
{
	cipher_pk_modexpd_s data;
	int ret;
	eip130_token_command_t command_token;
	eip130_token_result_t result_token;
	uint32_t key_bytes;
	if (ax_copy_from_user(&data, (void *)argp, sizeof(cipher_pk_modexpd_s))) {
		ret = AX_CIPHER_ACCESS_ERROR;
		return ret;
	}
	memset(&command_token, 0, sizeof(command_token));
	memset(&result_token, 0, sizeof(result_token));
	key_bytes = data.u32_modulus_bits / 8;
	ax_mutex_lock(&g_cipher_mutex);
	ax_cipher_pka_claim(key_bytes);
	ret = ax_cipher_pka_load(data.u64_modulus_data_phyaddr, key_bytes, 0);
	if ((ret < 0)) {
		ax_cipher_pka_claim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	ret = ax_cipher_pka_load(data.u64_exponent_data_phyaddr, key_bytes, 1);
	if ((ret < 0)) {
		ax_cipher_pka_claim(0);
		ax_mutex_unlock(&g_cipher_mutex);
		return AX_CIPHER_INTERNAL_ERROR;
	}

	Eip130Token_Command_Pk_Operation(&command_token, (uint8_t)VEXTOKEN_PK_OP_MODEXPD, 0,
									 data.u64_in_phyaddr, key_bytes, data.u64_out_phyaddr, key_bytes + 4);
	Eip130Token_Command_SetTokenID(&command_token, 0, AX_TRUE_CE);
	ret = eip130_physical_token_exchange((uint32_t *)&command_token, (uint32_t *)&result_token, OCCUPY_CE_MAILBOX_NR);
	if ((ret < 0) || (result_token.W[0] & (1 << 31))) {
		CE_LOG_PR(CE_ERR_LEVEL,"PKModExpD fail %x\n", result_token.W[0]);
		ret = AX_CIPHER_INTERNAL_ERROR;
	}
	ax_cipher_pka_claim(0);
	ax_mutex_unlock(&g_cipher_mutex);
	return ret;
}
int cipher_ioctl(uint32_t cmd, void *argp)
{
	int ret = AX_CIPHER_SUCCESS;
	if (argp == 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"Error, argp is NULL!\n");
		return AX_CIPHER_INVALID_PARAMETER;
	}
	switch (cmd) {
	case AX_CIPHER_CMD_CREATEHANDLE:
		ret = cipher_create_handle(argp);
		break;
	case AX_CIPHER_CMD_DESTROYHANDLE:
		ret = cipher_destroy_handle(argp);
		break;
	case AX_CIPHER_CMD_ENCRYPT:
		ret = cipher_encrypt(argp);
		break;
	case AX_CIPHER_CMD_DECRYPT:
		ret = cipher_decrypt(argp);
		break;
	case AX_CIPHER_CMD_HASHINIT:
		ret = AX_CIPHER_SUCCESS;
		break;
	case AX_CIPHER_CMD_HASHUPDATE:
		ret = cipher_hash_update(argp);
		break;
	case AX_CIPHER_CMD_HASHFINAL:
		ret = cipher_hash_final(argp);
		break;
	case AX_CIPHER_CMD_GETRANDOMNUMBER:
		ret = cipher_get_random_number(argp);
		break;
	case AX_CIPHER_CMD_RSAVERIFY:
		ret = cipher_rsa_verify(argp);
		break;
	case AX_CIPHER_CMD_RSASIGN:
		ret = cipher_rsa_sign(argp);
		break;
	case AX_CIPHER_CMD_PKMODEXPD:
		ret = ax_cipher_pk_modexpd(argp);
		break;
	case AX_CIPHER_CMD_PKMODEXPE:
		ret = ax_cipher_pk_modexpe(argp);
		break;
	default:
		return -1;
	}
	return ret;
}
int drv_cipher_init(void)
{
	memset(s_cipher_chn, 0, sizeof(s_cipher_chn));
	return 0;
}
int drv_cipher_deinit(void)
{
	return 0;
}
#endif
