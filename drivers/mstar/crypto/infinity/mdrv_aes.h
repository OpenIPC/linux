/* Copyright (C) 2003-2006, Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _MSTAR_AES_H_
#define _MSTAR_AES_H_

/* driver logic flags */
#define AES_MODE_ECB 0
#define AES_MODE_CBC 1
#define AES_MODE_CTR 2
#define AES_DIR_DECRYPT 0
#define AES_DIR_ENCRYPT 1
#define AESDMA_ALLOC_MEMSIZE (16) //1MB  1024*1024
#define AESDMA_ALLOC_MEMSIZE_TEMP (16) //64byte for SHA

struct infinity_aes_op
{
	void *src;
	void *dst;//16
	u32 mode;
	u32 dir;
	u32 flags;
    u32 keylen;//24
	int len;
    u8 *iv;//8
	u8 key[AES_KEYSIZE_128];//16
	union {
		struct crypto_blkcipher *blk;
		struct crypto_cipher *cip;
	} fallback;
    u16 engine;

};


struct aesdma_alloc_dmem
{
    dma_addr_t  aesdma_phy_addr ;
    dma_addr_t  aesdma_phy_SHABuf_addr;
    const char* DMEM_AES_ENG_INPUT;
    const char* DMEM_AES_ENG_SHABUF;
    u8 *aesdma_vir_addr;
    u8 *aesdma_vir_SHABuf_addr;
}ALLOC_DMEM = {0, 0, "AESDMA_ENG", "AESDMA_ENG1", 0, 0};

struct platform_device *psg_mdrv_aesdma;

typedef enum
{
    E_MSOS_PRIORITY,            ///< Priority-order suspension
    E_MSOS_FIFO,                ///< FIFO-order suspension
} MsOSAttribute;



#endif
