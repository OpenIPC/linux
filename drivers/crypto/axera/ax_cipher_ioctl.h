/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_CIPHER_IOCTL_H_
#define _AX_CIPHER_IOCTL_H_

#define REE_CIPHER_IOC_NA	   0U
#define REE_CIPHER_IOC_W		1U
#define REE_CIPHER_IOC_R		2U
#define REE_CIPHER_IOC_RW	   3U

#define REE_CIPHER_IOC(dir,type,nr,size) (((dir) << 30)|((size) << 16)|((type) << 8)|((nr) << 0))

#define REE_CIPHER_IOR(type,nr,size)				REE_CIPHER_IOC(REE_CIPHER_IOC_R,(type),(nr), sizeof(size))
#define REE_CIPHER_IOW(type,nr,size)				REE_CIPHER_IOC(REE_CIPHER_IOC_W,(type),(nr), sizeof(size))
#define REE_CIPHER_IOWR(type,nr,size)			REE_CIPHER_IOC(REE_CIPHER_IOC_RW,(type),(nr),sizeof(size))

#define REE_CIPHER_IOC_DIR(nr)		(((nr) >> 30) & 0x03)
#define REE_CIPHER_IOC_TYPE(nr)		(((nr) >> 8) & 0xFF)
#define REE_CIPHER_IOC_NR(nr)		(((nr) >> 0) & 0xFF)
#define REE_CIPHER_IOC_SIZE(nr)		(((nr) >> 16) & 0x3FFF)
#define   AX_ID_CIPHER  0x100

typedef struct {
	ax_cipher_handle handle;
	AX_CIPHER_ALGO_E alg;
	AX_CIPHER_MODE_E work_mode;
	uint32_t key_size;
	uint8_t p_key[32];
	uint8_t p_iv[32];
} cipher_ctl_data_s;

typedef struct {
	ax_cipher_handle handle;
	uint64_t src_phyaddr;
	uint64_t dst_phyaddr;
	uint32_t u32_data_length;
} cipher_data_s;

typedef struct {
	uint32_t hash_type;
	uint32_t b_isfirst;
	uint32_t b_ismac;
	uint32_t u32_data_len;
	uint32_t u32_mac_key[16];
	uint32_t u32_sha_val[16];
	uint64_t u64_total_data_len;
	uint64_t u64_data_phy;
	uint32_t u32_key_len;
} cipher_hash_data_s;

typedef struct {
	uint64_t u64_data_phy;
	uint32_t u32_size;
} cipher_rng_s;

typedef struct {
	uint64_t u64_key_phyaddr;
	uint64_t u64_policy;
	uint64_t u64_sig_phyaddr;
	uint32_t u32_asset_size;
	uint32_t u32_modulus_bits;
	uint32_t u32_public_exponent_bytes;
	uint32_t u32_hash_size;
	uint32_t u32_salt_size;
	uint32_t u32_method;
	uint32_t hash_result[16];
} cipher_rsa_verify_s;

typedef struct {
	uint64_t u64_key_phyaddr;
	uint64_t u64_policy;
	uint64_t u64_sig_phyaddr;
	uint32_t u32_asset_size;
	uint32_t u32_modulus_bits;
	uint32_t u32_private_exponent_bytes;
	uint32_t u32_hash_size;
	uint32_t u32_salt_size;
	uint32_t u32_method;
	uint32_t hash_result[16];
} cipher_rsa_sign_s;

typedef struct {
	uint64_t u64_modulus_data_phyaddr;
	uint64_t u64_in_phyaddr;
	uint64_t u64_out_phyaddr;
	uint32_t u32_modulus_bits;
	uint32_t u32_public_exponent;
	uint32_t u32_input_bytes;
	uint32_t u32_output_bytes;
} cipher_pk_modexpe_s;

typedef struct {
	uint64_t u64_modulus_data_phyaddr;
	uint64_t u64_exponent_data_phyaddr;
	uint64_t u64_in_phyaddr;
	uint64_t u64_out_phyaddr;
	uint32_t u32_modulus_bits;
	uint32_t u32_public_exponent_bytes;
	uint32_t u32_input_bytes;
	uint32_t u32_output_bytes;
} cipher_pk_modexpd_s;

#define	AX_CIPHER_CMD_CREATEHANDLE		   REE_CIPHER_IOWR(AX_ID_CIPHER,  0x1,  cipher_ctl_data_s)
#define	AX_CIPHER_CMD_DESTROYHANDLE		  REE_CIPHER_IOW(AX_ID_CIPHER,  0x2,  uint64_t)
#define	AX_CIPHER_CMD_ENCRYPT				REE_CIPHER_IOW(AX_ID_CIPHER,  0x4,  cipher_data_s)
#define	AX_CIPHER_CMD_DECRYPT				REE_CIPHER_IOW(AX_ID_CIPHER,  0x5,  cipher_data_s)
#define	AX_CIPHER_CMD_HASHINIT				REE_CIPHER_IOWR(AX_ID_CIPHER,  0x6,  cipher_hash_data_s)
#define	AX_CIPHER_CMD_HASHUPDATE		 	REE_CIPHER_IOWR(AX_ID_CIPHER,  0x7,  cipher_hash_data_s)
#define	AX_CIPHER_CMD_HASHFINAL		  	REE_CIPHER_IOWR(AX_ID_CIPHER, 0x8,  cipher_hash_data_s)
#define	AX_CIPHER_CMD_GETRANDOMNUMBER		REE_CIPHER_IOWR(AX_ID_CIPHER,  0x9,  cipher_rng_s)
#define	AX_CIPHER_CMD_RSAVERIFY				REE_CIPHER_IOWR(AX_ID_CIPHER,  0xA,  cipher_rsa_verify_s)
#define	AX_CIPHER_CMD_RSASIGN				REE_CIPHER_IOWR(AX_ID_CIPHER,  0xB,  cipher_rsa_sign_s)
#define	AX_CIPHER_CMD_PKMODEXPE				REE_CIPHER_IOWR(AX_ID_CIPHER,  0xC,  cipher_pk_modexpe_s)
#define	AX_CIPHER_CMD_PKMODEXPD				REE_CIPHER_IOWR(AX_ID_CIPHER,  0xD,  cipher_pk_modexpd_s)

int cipher_ioctl(uint32_t cmd, void * argp);
int drv_cipher_init(void);
int drv_cipher_deinit(void);
#endif
