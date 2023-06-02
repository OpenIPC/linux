/*
 *#############################################################################
 *
 * Copyright (c) 2006-2011 MStar Semiconductor, Inc.
 * All rights reserved.
 *
 * Unless otherwise stipulated in writing, any and all information contained
 * herein regardless in any format shall remain the sole proprietary of
 * MStar Semiconductor Inc. and be kept in strict confidence
 * (!¡±MStar Confidential Information!¡L) by the recipient.
 * Any unauthorized act including without limitation unauthorized disclosure,
 * copying, use, reproduction, sale, distribution, modification, disassembling,
 * reverse engineering and compiling of the contents of MStar Confidential
 * Information is unlawful and strictly prohibited. MStar hereby reserves the
 * rights to any and all damages, losses, costs and expenses resulting therefrom.
 *
 *#############################################################################
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/hw_random.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>
#include <crypto/sha.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
//#include <asm/vio.h>
#include "ms_platform.h"
#include "ms_msys.h"
#include "halAESDMA.h"
#include <linux/mm.h>

#define LOOP_CNT                        100
#define INFINITY_SHA_BUFFER_SIZE        1024

#define SHA_DEBUG_FLAG (1)
#if (SHA_DEBUG_FLAG == 1)
#define SHA_DBG(fmt, arg...) printk(KERN_DEBUG fmt, ##arg)//KERN_DEBUG KERN_ALERT KERN_WARNING
#else
#define SHA_DBG(fmt, arg...)
#endif

//memory probe from aes.c
extern struct aesdma_alloc_dmem
{
    dma_addr_t  aesdma_phy_addr ;
    dma_addr_t  aesdma_phy_SHABuf_addr;
    const char* DMEM_AES_ENG_INPUT;
    const char* DMEM_AES_ENG_SHABUF;
    u8 *aesdma_vir_addr;
    u8 *aesdma_vir_SHABuf_addr;
}ALLOC_DMEM;

extern struct platform_device *psg_mdrv_aesdma;
extern void enableClock(void);
extern void disableClock(void);
extern void allocMem(U32 len);
extern void _ms_aes_mem_free(void);

extern void free_dmem(const char* name, unsigned int size, void *virt, dma_addr_t addr );
static  u8 gu8WorkMode=0;

static void* alloc_dmem(const char* name, unsigned int size, dma_addr_t *addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name,name,strlen(name)+1);
    dmem.length=size;
    if(0!=msys_request_dmem(&dmem)){
        return NULL;
    }
    *addr=dmem.phys;
    return (void *)((uintptr_t)dmem.kvirt);
}

void allocTempMem(U32 len)
{
    if (!(ALLOC_DMEM.aesdma_vir_SHABuf_addr = alloc_dmem(ALLOC_DMEM.DMEM_AES_ENG_SHABUF,
                                             len,
                                             &ALLOC_DMEM.aesdma_phy_SHABuf_addr))){
        printk("[input]unable to allocate aesdma memory\n");
    }
    memset(ALLOC_DMEM.aesdma_vir_SHABuf_addr, 0, len);
}

struct infinity_sha256_ctx
{
    u8 digest[SHA256_DIGEST_SIZE];
    u32 u32digest_len;
};

static void __BeforeStart(void)
{
    u32 u32Val=0;
    switch(gu8WorkMode)
    {
    case 0:
        SHA_DBG(" %s_info: case = %d\n",__FUNCTION__,gu8WorkMode);
        HAL_SHA_ManualMode(0);
        gu8WorkMode++;
        break;
    case 1:
        SHA_DBG(" %s_info: case = %d\n",__FUNCTION__,gu8WorkMode);
        HAL_SHA_ManualMode(1);
        HAL_SHA_Out((U32)ALLOC_DMEM.aesdma_vir_SHABuf_addr);
        HAL_SHA_Write_InitValue((U32)ALLOC_DMEM.aesdma_vir_SHABuf_addr);
        u32Val = HAL_SHA_ReadWordCnt();
        HAL_SHA_WriteWordCnt(u32Val);
        gu8WorkMode++;
        break;
    default:
        SHA_DBG(" %s_info: case = %d\n",__FUNCTION__,gu8WorkMode);
        HAL_SHA_Out((U32)ALLOC_DMEM.aesdma_vir_SHABuf_addr);
        HAL_SHA_Write_InitValue((U32)ALLOC_DMEM.aesdma_vir_SHABuf_addr);
        u32Val = HAL_SHA_ReadWordCnt();
        HAL_SHA_WriteWordCnt(u32Val);
        break;
    }
}

static int infinity_sha256_init(struct shash_desc *desc)
{
    struct infinity_sha256_ctx *infinity_ctx = crypto_tfm_ctx(&desc->tfm->base);
//	struct sha256_state *sctx = shash_desc_ctx(desc);


    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
	SHA_DBG(" %s %d \n","infinity_ctx->u32digest_len", infinity_ctx->u32digest_len);
//	infinity_ctx->u32digest_len = 3;
//    enableClock();
//	if(infinity_ctx->u32digest_len)
//	{
//		allocMem(4096);
//		allocMem(0x20);
//		allocTempMem(4096);
//	}
//	else
//	{
//		return -EINVAL;
//	}

    HAL_SHA_Reset();
    HAL_SHA_SetAddress(Chip_Phys_to_MIU(ALLOC_DMEM.aesdma_phy_addr));
    HAL_SHA_SelMode(1);
    gu8WorkMode = 0;
    memset(infinity_ctx, 0, sizeof(struct infinity_sha256_ctx));
    return 0;
}

static int infinity_sha256_update(struct shash_desc *desc, const u8 *data, unsigned int len)
{
    struct sha256_state *sctx = shash_desc_ctx(desc);
    struct infinity_sha256_ctx *infinity_ctx = crypto_tfm_ctx(&desc->tfm->base);
    u64 leftover= 0;
    u64 total= 0;
    u32 u32InputCopied = 0;
    u32 u32loopCnt;
    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);


    sctx->count = 0;
    total = sctx->count + len;
    if (total < SHA256_BLOCK_SIZE) {
        memcpy(sctx->buf + sctx->count, data, len);
        sctx->count += len;
        return 0;
    }

	SHA_DBG(" %s sctx->count %lld \n",__FUNCTION__,sctx->count);
    if(sctx->count)
    {
        memcpy(ALLOC_DMEM.aesdma_vir_addr, sctx->buf, sctx->count);
        Chip_Flush_MIU_Pipe();
    }

    leftover = total;
    if(leftover >= INFINITY_SHA_BUFFER_SIZE)
    {
    	SHA_DBG("INFINITY_SHA_BUFFER_SIZE-sctx->count %lld \n",INFINITY_SHA_BUFFER_SIZE-sctx->count);
        memcpy(ALLOC_DMEM.aesdma_vir_addr+sctx->count, data, INFINITY_SHA_BUFFER_SIZE-sctx->count);
        Chip_Flush_MIU_Pipe();
        u32InputCopied = INFINITY_SHA_BUFFER_SIZE-sctx->count;
        HAL_SHA_SetLength(INFINITY_SHA_BUFFER_SIZE);
        while(leftover >= INFINITY_SHA_BUFFER_SIZE)
        {
            __BeforeStart();
            HAL_SHA_Start();
            u32loopCnt = 0;
            while(((HAL_SHA_GetStatus() & SHARNG_CTRL_SHA_READY) != SHARNG_CTRL_SHA_READY) &&
                  (u32loopCnt<LOOP_CNT))
            {
                u32loopCnt++;
                mdelay(1);
            }
            leftover -= INFINITY_SHA_BUFFER_SIZE;
            if(leftover >= INFINITY_SHA_BUFFER_SIZE)
            {
				SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
                memcpy(ALLOC_DMEM.aesdma_vir_addr, data+u32InputCopied, INFINITY_SHA_BUFFER_SIZE);
                Chip_Flush_MIU_Pipe();
                u32InputCopied += INFINITY_SHA_BUFFER_SIZE;
            }
        }
    }
    else
    {
    	SHA_DBG(" %s line224 %d \n",__FUNCTION__,len);
        memcpy(ALLOC_DMEM.aesdma_vir_addr+sctx->count, data, len);
        Chip_Flush_MIU_Pipe();
    }

    if(leftover >= SHA256_BLOCK_SIZE)
    {
    	SHA_DBG(" %s line232 %d \n",__FUNCTION__,SHA256_BLOCK_SIZE);
        memcpy(ALLOC_DMEM.aesdma_vir_addr, data+u32InputCopied, SHA256_BLOCK_SIZE);
        Chip_Flush_MIU_Pipe();
        u32InputCopied += SHA256_BLOCK_SIZE;
        HAL_SHA_SetLength(total);
        while(leftover >= SHA256_BLOCK_SIZE)
        {
            __BeforeStart();
            HAL_SHA_Start();
            u32loopCnt = 0;
            while(((HAL_SHA_GetStatus() & SHARNG_CTRL_SHA_READY) != SHARNG_CTRL_SHA_READY) &&
                  (u32loopCnt<LOOP_CNT))
            {
                u32loopCnt++;
                mdelay(1);
            }
            leftover -= SHA256_BLOCK_SIZE;
            if(leftover >= SHA256_BLOCK_SIZE)
            {
               memcpy(ALLOC_DMEM.aesdma_vir_addr, data+u32InputCopied, SHA256_BLOCK_SIZE);
               Chip_Flush_MIU_Pipe();
               u32InputCopied += SHA256_BLOCK_SIZE;
            }
        }
    }

    if (leftover)
    {
        memcpy(sctx->buf, data+u32InputCopied, leftover);
    }
    sctx->count = leftover;
    HAL_SHA_Out((U32)infinity_ctx->digest);
    infinity_ctx->u32digest_len += HAL_SHA_ReadWordCnt();
    return 0;
}

static int infinity_sha256_final(struct shash_desc *desc, u8 *out)
{
    struct sha256_state *sctx = shash_desc_ctx(desc);
    struct infinity_sha256_ctx *infinity_ctx = crypto_tfm_ctx(&desc->tfm->base);
    u32 u32loopCnt;

    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
    if(sctx->count)
    {
        memset(ALLOC_DMEM.aesdma_vir_addr, 0, SHA256_BLOCK_SIZE);
        /*Need to check the value*/
        while (memcmp(ALLOC_DMEM.aesdma_vir_addr, sctx->buf,
			   sctx->count)!= 0 ){
            memcpy(ALLOC_DMEM.aesdma_vir_addr, sctx->buf, SHA256_BLOCK_SIZE);

        }
//        memcpy(ALLOC_DMEM.aesdma_vir_addr, sctx->buf, SHA256_BLOCK_SIZE);
//        Chip_Flush_MIU_Pipe();
        HAL_SHA_SetLength(sctx->count);
        __BeforeStart();
        HAL_SHA_Start();
        u32loopCnt = 0;
        while(((HAL_SHA_GetStatus() & SHARNG_CTRL_SHA_READY) != SHARNG_CTRL_SHA_READY) &&
              (u32loopCnt<LOOP_CNT))
        {
            u32loopCnt++;
            mdelay(1);
        }
        HAL_SHA_Out((U32)infinity_ctx->digest);
        infinity_ctx->u32digest_len += sctx->count;
        sctx->count = 0;
    }
    HAL_SHA_Reset();

    memcpy(out, infinity_ctx->digest, SHA256_DIGEST_SIZE);

//    _ms_aes_mem_free();
//    disableClock();
//    printk(KERN_ALERT " %s \n",__FUNCTION__);
    return 0;
}

static int infinity_sha256_export(struct shash_desc *desc, void *out)
{
    struct sha256_state *sctx = shash_desc_ctx(desc);
    struct infinity_sha256_ctx *infinity_ctx = crypto_tfm_ctx(&desc->tfm->base);
    struct sha256_state *octx = out;
    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
    octx->count = sctx->count + infinity_ctx->u32digest_len;
    memcpy(octx->buf, sctx->buf, sizeof(octx->buf));
    /* if no data has been processed yet, we need to export SHA256's
     * initial data, in case this context gets imported into a software
     * context */
    if(infinity_ctx->u32digest_len)
    {
        memcpy(octx->state, infinity_ctx->digest, SHA256_DIGEST_SIZE);
    }
    else
    {
        memset(octx->state, 0, SHA256_DIGEST_SIZE);
    }

    return 0;
}

static int infinity_sha256_import(struct shash_desc *desc, const void *in)
{
    struct sha256_state *sctx = shash_desc_ctx(desc);
    struct infinity_sha256_ctx *infinity_ctx = crypto_tfm_ctx(&desc->tfm->base);
    const struct sha256_state *ictx = in;
    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
    memcpy(sctx->buf, ictx->buf, sizeof(ictx->buf));
    sctx->count = ictx->count & 0x3f;
    infinity_ctx->u32digest_len = sctx->count;
    if (infinity_ctx->u32digest_len) {
        memcpy(infinity_ctx->digest, ictx->state, SHA256_DIGEST_SIZE);
    }
    return 0;
}

struct shash_alg infinity_shash_sha256_alg = {
	.digestsize = SHA256_DIGEST_SIZE,
	.init       = infinity_sha256_init,
	.update     = infinity_sha256_update,
	.final      = infinity_sha256_final,
	.export     = infinity_sha256_export,
	.import     = infinity_sha256_import,
	.descsize   = sizeof(struct sha256_state),
	.statesize  = sizeof(struct sha256_state),
	.base       = {
		.cra_name        = "sha256",
		.cra_driver_name = "sha256-infinity",
		.cra_priority    = 00,
		.cra_flags       = CRYPTO_ALG_TYPE_SHASH |
						CRYPTO_ALG_NEED_FALLBACK,
		.cra_blocksize   = SHA256_BLOCK_SIZE,
		.cra_module      = THIS_MODULE,
		.cra_ctxsize     = sizeof(struct infinity_sha256_ctx),
	}
};



int infinity_sha_create(void)
{
    int ret = -1;
    SHA_DBG(" %s %d \n",__FUNCTION__,__LINE__);
//	SHA_DBG(" %s %d \n",__FUNCTION__,SHA256_DIGEST_SIZE);
    ret = crypto_register_shash(&infinity_shash_sha256_alg);
//	allocMem(4096);
	allocTempMem(4096);

    return ret;
}

int infinity_sha_destroy(void)
{
    _ms_aes_mem_free();

//    crypto_unregister_alg(&infinity_shash_sha256_alg);
    return 0;
}
