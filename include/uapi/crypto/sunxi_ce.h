/*
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _UAPI_SUNXI_CE_H_
#define _UAPI_SUNXI_CE_H_

//#include <asm/ioctl.h>

typedef unsigned char		u8;
typedef unsigned int		u32;
typedef signed int			s32;

#define AES_MODE_ECB	0 /*ECB模式*/
#define AES_MODE_CBC	1 /*CBC模式*/

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32

#define AES_DIR_ENCRYPT	0 /*加密*/
#define AES_DIR_DECRYPT	1 /*解密*/

#define AES_IV_LENGTH	16

#define RSA_DIR_ENCRYPT 0
#define RSA_DIR_DECRYPT 1

/*define the ctx for aes requtest*/
typedef struct {
	u8 *src_buffer;
	u32 src_length;
	u8 *dst_buffer;
	u32 dst_length;
	u8 *key_buffer;
	u32 key_length;
	u8 *iv_buf;
	u32 iv_length;
	u32 aes_mode;
	u32 dir;
	u32 ion_flag;
	unsigned long src_phy;
	unsigned long dst_phy;
	unsigned long iv_phy;
	unsigned long key_phy;
	s32 channel_id;
} crypto_aes_req_ctx_t;

/*define the ctx for rsa requtest*/
typedef struct {
	u8 *sign_buffer;
	u32 sign_length;
	u8 *pkey_buffer;
	u32 pkey_length;
	u8 *dst_buffer;
	u32 dst_length;
	u32 dir;
	u32 rsa_width;
	u32 flag;
	s32 channel_id;
} crypto_rsa_req_ctx_t;

/*define the ctx for hash requtest*/
typedef struct {
	u8 *text_buffer;
	u32 text_length;
	u8 *key_buffer;  // hmac
	u32 key_length;
	u8 *dst_buffer;
	u32 dst_length;
	u8 *iv_buffer;
	u32 iv_length;
	u32 hash_mode;
	u32 ion_flag;
	unsigned long text_phy;
	unsigned long dst_phy;
	s32 channel_id;
} crypto_hash_req_ctx_t;


/*ioctl cmd*/
#define CE_IOC_MAGIC			'C'
#define CE_IOC_REQUEST			_IOR(CE_IOC_MAGIC, 0, int)
#define CE_IOC_FREE				_IOW(CE_IOC_MAGIC, 1, int)
#define CE_IOC_AES_CRYPTO		_IOW(CE_IOC_MAGIC, 2, crypto_aes_req_ctx_t)
#define CE_IOC_RSA_CRYPTO		_IOW(CE_IOC_MAGIC, 3, crypto_rsa_req_ctx_t)
#define CE_IOC_HASH_CRYPTO		_IOW(CE_IOC_MAGIC, 4, crypto_hash_req_ctx_t)

#endif /* end of _UAPI_SUNXI_CE_H_ */
