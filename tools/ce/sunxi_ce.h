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

#ifndef _SUNXI_CE_H_
#define _SUNXI_CE_H_

#include <asm/ioctl.h>

typedef unsigned char		u8;
typedef unsigned int		u32;
typedef signed int			s32;

#ifndef MIN
#define MIN(a, b) (a > b ? b : a)
#endif

#define AES_MODE_ECB	0 /*ECB模式*/
#define AES_MODE_CBC	1 /*CBC模式*/

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32

#define AES_DIR_ENCRYPT	0 /*加密*/
#define AES_DIR_DECRYPT	1 /*解密*/

#define AES_IV_LENGTH	16

/*define the ctx for aes requtest*/
typedef struct {
	u8 *src_buffer;
	u32 src_length;
	u8 *dst_buffer;
	u32 dst_length;
	u8 *key_buffer;
	u32 key_length;
	u8 *iv_buffer;
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

/*ioctl cmd*/
#define CE_IOC_MAGIC			'C'
#define CE_IOC_REQUEST			_IOR(CE_IOC_MAGIC, 0, int)
#define CE_IOC_FREE				_IOW(CE_IOC_MAGIC, 1, int)
#define CE_IOC_AES_CRYPTO		_IOW(CE_IOC_MAGIC, 2, crypto_aes_req_ctx_t)

/*IOMMU mode*/
#define ION_IOC_SUNXI_FLUSH_RANGE	5
#define ION_IOC_SUNXI_PHYS_ADDR		7

struct sunxi_phys_data {
	int handle;
	unsigned int phys_addr;
	unsigned int size;
};

struct sunxi_cache_range {
	unsigned long start;
	unsigned long end;
};

struct ion_buf_t {
	int ion_fd;
	struct sunxi_phys_data phys_data;
	unsigned char *vir_addr;
	unsigned int buf_len;
} ion_buf;

#define ION_BUF_LEN	(3 * 1024 * 1024)
#define DATA_LEN	(1024 * 1024)//(350 * 1024)//(350 * 1024 + 12)

#endif /* end of _SUNXI_CE_H_ */
