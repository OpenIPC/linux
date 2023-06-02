/******************************************************************************

  Copyright (C), 2011-2014, Fullhan Tech. Co., Ltd.

 ******************************************************************************
  File Name     :fh_drv_cipher.h
  Version       : Initial Draft
  Author        : Fullhan fhsecurity team
  Created       : 
  Last Modified :
  Description   : 
  Function List :
  History       :
******************************************************************************/

#ifndef __FH_DRV_CIPHER_H__
#define __FH_DRV_CIPHER_H__

#include "fh_type.h"
#include "fh_unf_cipher.h"
#include "config.h"

#include <linux/mutex.h>
#include <linux/completion.h>

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */



#define HASH_BLOCK_SIZE           (64)

#ifdef CIPHER_MULTICIPHER_SUPPORT
#endif

#ifdef CIPHER_HASH_SUPPORT
#endif

#ifdef CIPHER_RNG_SUPPORT
typedef struct {
    FH_U32 u32TimeOutUs;
	FH_U32 u32RNG;
}CIPHER_RNG_S;
#endif

#ifdef CIPHER_RSA_SUPPORT
typedef struct {
    FH_U8  *pu8Input;
    FH_U8  *pu8Output;
    FH_U32  u32DataLen;
    FH_U8  *pu8N;
	FH_U8  *pu8K;
    FH_U16 u16NLen;
    FH_U16 u16KLen;
    FH_UNF_CIPHER_KEY_SRC_E enKeySrc;
}CIPHER_RSA_DATA_S;

typedef struct {
    FH_UNF_CIPHER_RSA_PRI_KEY_S stPriKey;
    FH_U32 u32NumBits;
    FH_U32 u32Exponent;
}CIPHER_RSA_KEY_S;
#endif

#ifdef CIPHER_KLAD_SUPPORT
typedef struct {
    FH_UNF_CIPHER_KEY_SRC_E enRootKey;
    FH_UNF_CIPHER_KLAD_TARGET_E enTarget;
    FH_U32 u32CleanKey[4];
    FH_U32 u32EncryptKey[4];
}CIPHER_KLAD_KEY_S;
#endif

#ifdef CIPHER_MULTICIPHER_SUPPORT
FH_S32 FH_DRV_CIPHER_CreateHandle(FH_HANDLE *pHdlr, FH_VOID *p_priv);
FH_S32 FH_DRV_CIPHER_DestroyHandle(FH_HANDLE Hdlr);
FH_S32 FH_DRV_CIPHER_Config(FH_HANDLE Hdlr, FH_UNF_CIPHER_CTRL_S *pConfig);
FH_S32 FH_DRV_CIPHER_Encrypt(FH_HANDLE Hdlr, FH_U32 ScrPhyAddr, FH_U32 DestPhyAddr, FH_U32 u32DataLength);
FH_S32 FH_DRV_CIPHER_Decrypt(FH_HANDLE Hdlr, FH_U32 ScrPhyAddr, FH_U32 DestPhyAddr, FH_U32 u32DataLength);
FH_S32 FH_DRV_CIPHER_EncryptMulti(FH_HANDLE Hdlr, FH_UNF_CIPHER_DATA_S *DataPkg, FH_U32 DataPkgNum);
FH_S32 FH_DRV_CIPHER_DecryptMulti(FH_HANDLE Hdlr, FH_UNF_CIPHER_DATA_S *DataPkg, FH_U32 DataPkgNum);
FH_S32 FH_DRV_CIPHER_GetHandleConfig(FH_HANDLE Hdlr, FH_UNF_CIPHER_CTRL_S *pConfig);
#endif

#ifdef CIPHER_RNG_SUPPORT
FH_S32 FH_DRV_CIPHER_GetRandomNumber(CIPHER_RNG_S *pstRNG);
#endif

#ifdef CIPHER_HASH_SUPPORT
FH_S32 FH_DRV_CIPHER_CalcHashInit(FH_HANDLE *pHdlr, FH_VOID *p_priv, FH_UNF_CIPHER_HASH_TYPE_E alg, FH_U8 *hmackey);
FH_S32 FH_DRV_CIPHER_CalcHashUpdate(FH_HANDLE Hdlr, FH_U8 *msg, FH_U32 msglen);
FH_S32 FH_DRV_CIPHER_CalcHashFinal(FH_HANDLE Hdlr, FH_U8 *dgst, FH_U32 *dgstlen);
FH_S32 FH_DRV_CIPHER_CalcHash(FH_VOID *p_priv,
	                                FH_UNF_CIPHER_HASH_TYPE_E alg,
	                                FH_U32 MsgPhyAddr,
	                                FH_U32 MsgLen,
	                                FH_U8 *dgst,
	                                FH_U32 *dgstlen);
FH_S32 FH_DRV_CIPHER_GetHashInfo(FH_HANDLE Hdlr, FH_UNF_CIPHER_HASH_TYPE_E *alg, FH_U8 **hmackey);
#endif

#ifdef CIPHER_RSA_SUPPORT
FH_S32 FH_DRV_CIPHER_CalcRsa(CIPHER_RSA_DATA_S *pCipherRsaData);
#endif

#ifdef CIPHER_KLAD_SUPPORT
FH_S32 FH_DRV_CIPHER_KladEncryptKey(CIPHER_KLAD_KEY_S *pstKladKey);
#endif

FH_VOID FH_DRV_CIPHER_Suspend(FH_VOID);
FH_S32 FH_DRV_CIPHER_Resume(FH_VOID);
FH_S32 FH_DRV_CIPHER_SoftReset(FH_VOID);














typedef enum {
	RPU_ALGO_AES128,
	RPU_ALGO_DES,
	RPU_ALGO_TDES,
	RPU_ALGO_SM4,
	RPU_ALGO_SHA1,
	RPU_ALGO_SHA256,
	RPU_ALGO_SM3,
	RPU_ALGO_RSA2048_SIGN,
	RPU_ALGO_RSA2048_VERIFY,
	RPU_ALGO_SM2_SIGN,
	RPU_ALGO_SM2_VERIFY,
} DRV_RPU_ALGO_E;

typedef enum  _DRV_RPU_ST_E {
	RPU_ST_AES128_SH256_RSA2048_4_AES128 = 0x1,
	RPU_ST_AES128_SH256_RSA2048_4_SH256 = 0x2,
	RPU_ST_AES128_SH256_RSA2048_4_RSA2048 = 0x3,

	RPU_ST_SM2_SM3_SM4_4_SM4 = 0x04,
	RPU_ST_SM2_SM3_SM4_4_SM3 = 0x08,
	RPU_ST_SM2_SM3_SM4_4_SM2_VERIFY = 0x0C,
	RPU_ST_SM2_SM3_SM4_4_SM2_SIGN = 0x10,

	RPU_ST_DES = 0x20,
	RPU_ST_TDES = 0x40,
	RPU_ST_SH1 = 0x60,

	RPU_ST_BUTT = 0,
} DRV_RPU_ST_E;

typedef enum  _DRV_RPU_STAGE_E {
	RPU_STAGE_CREATE,
	RPU_STAGE_CONFIG,
	RPU_STAGE_DATA,
	RPU_STAGE_BUTT,
} DRV_RPU_STAGE_E;

typedef struct _DRV_CIPHER_HDLR_S {
	void *priv;
	void __iomem *regs;

	DRV_RPU_ST_E st;
	bool is_break;
	DRV_RPU_STAGE_E	stage;
	bool is_init;

	FH_U32 IVSet;
	FH_U32 iv[4];
	FH_U32 key[8];
	FH_U32 iv_len;
	FH_U32 key_len;
	FH_U32 work_mode;  
	DRV_RPU_ALGO_E algo;
} DRV_CIPHER_HDLR_S;

typedef enum  _DRV_HASH_STAGE_E {
	HASH_STAGE_INIT,
	HASH_STAGE_UPDATE,
	HASH_STAGE_FINAL,
	HASH_STAGE_BUTT,
} DRV_HASH_STAGE_E;

typedef struct _DRV_HASH_HDLR_S {
#if 0
	void *priv;
	void __iomem *regs;

	DRV_RPU_ST_E st;
	bool is_break;
	DRV_HASH_STAGE_E stage;
	bool is_init;

	FH_U32 state[8];
	DRV_RPU_ALGO_E algo;

	FH_U32 remain[HASH_BLOCK_SIZE/2];
	FH_U32 remain_len;
	FH_U32 total_len;

	FH_U8 hmackey[HASH_BLOCK_SIZE];
	bool is_hmac;
#else
	void *priv;
	void __iomem *regs;

	FH_U32 state[8];
	FH_U32 remain[HASH_BLOCK_SIZE/2];
	FH_U32 remain_len;
	FH_U32 total_len;

	DRV_RPU_ALGO_E algo;
	DRV_RPU_ST_E st;
	DRV_HASH_STAGE_E stage;
	bool is_break;
	bool is_init;
	bool is_hmac;
	FH_U8 hmackey[HASH_BLOCK_SIZE];
#endif
} DRV_HASH_HDLR_S;

typedef struct fhCESA_RES_S {
	struct device		*dev;
	void __iomem		*regs;
	int					irq;
	struct clk			*clk;
	struct completion	completion;
	struct mutex		lock;
	/* spinlock_t			lock; */

	DRV_RPU_ST_E		st;
	void				*curr_hdlr;
}CESA_RES_S;

DRV_RPU_ST_E rpu_st_switch(void __iomem *regbase, DRV_RPU_ST_E old, DRV_RPU_ST_E new);











#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* End of #ifndef __FH_DRV_CIPHER_H__*/

