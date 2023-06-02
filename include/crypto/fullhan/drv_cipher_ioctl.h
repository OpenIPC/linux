/******************************************************************************

  Copyright (C), 2011-2014, Fullhan Tech. Co., Ltd.

 ******************************************************************************
  File Name     : drv_cipher_ioctl.h
  Version       : Initial Draft
  Author        : Fullhan fhsecurity team
  Created       : 
  Last Modified :
  Description   : 
  Function List :
  History       :
******************************************************************************/

#ifndef __DRV_CIPHER_IOCTL_H__
#define __DRV_CIPHER_IOCTL_H__

#include "fh_type.h"
#include "fh_unf_cipher.h"
#include "fh_drv_cipher.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */



typedef struct _CMD_CIPHER_HANDLE_S {
	FH_HANDLE hdlr;
} CMD_CIPHER_CREATE_S, CMD_CIPHER_DESTROY_S, CMD_CIPHER_PROCESS_STOP_S;

typedef struct _CMD_CIPHER_CTRL_S {
	FH_HANDLE hdlr;
	FH_UNF_CIPHER_CTRL_S ctrl;
} CMD_CIPHER_CTRL_S, CMD_CIPHER_INFO_S;

typedef struct _CMD_CIPHER_PROCESS_S {
	FH_HANDLE hdlr;

	FH_UNF_CIPHER_DATA_S *pKernelDataPkg;
	FH_UNF_CIPHER_DATA_S *pDataPkg;
	FH_U32 DataPkgNum;
	FH_UNF_CIPHER_CTRL_S ctrl;
} CMD_CIPHER_PROCESS_S;

typedef struct _CMD_HASH_INIT_S {
	FH_HANDLE hdlr;

	FH_UNF_CIPHER_HASH_TYPE_E alg;
	FH_U8 *hmackey;
	FH_U8 *khmackey;
} CMD_HASH_INIT_S;

typedef struct _CMD_HASH_UPDATE_S {
	FH_HANDLE hdlr;

	FH_U8 *kmsg;
	FH_U8 *msg;
	FH_U32 msglen;
} CMD_HASH_UPDATE_S;

typedef struct _CMD_HASH_FINAL_S {
	FH_HANDLE hdlr;

	FH_U8 *output;
} CMD_HASH_FINAL_S;

typedef struct _CMD_HASH_DIGEST_S {
	FH_UNF_CIPHER_HASH_TYPE_E alg;
	FH_U32 u32MsgPhyAddr;
	FH_U32 msglen;
	FH_U8 *output;
} CMD_HASH_DIGEST_S;

typedef struct _CMD_GET_HASHINFO_S {
	FH_HANDLE hdlr;
	FH_UNF_CIPHER_HASH_TYPE_E *alg;
	FH_UNF_CIPHER_HASH_TYPE_E kalg;
	FH_U8 *hmackey;
	FH_U8 *khmackey;
} CMD_GET_HASHINFO_S;



/* Ioctl definitions */
#define FH_ID_CIPHER 100
#define FH_ID_HASH   101

#define    CMD_CIPHER_CREATEHANDLE           _IOWR(FH_ID_CIPHER, 0x1, CMD_CIPHER_CREATE_S)
#define    CMD_CIPHER_DESTROYHANDLE          _IOW(FH_ID_CIPHER, 0x2, CMD_CIPHER_DESTROY_S)
#define    CMD_CIPHER_CONFIGHANDLE           _IOW(FH_ID_CIPHER, 0x3, CMD_CIPHER_CTRL_S)
#define    CMD_CIPHER_ENCRYPT                _IOW(FH_ID_CIPHER, 0x4, CMD_CIPHER_PROCESS_S)
#define    CMD_CIPHER_DECRYPT                _IOW(FH_ID_CIPHER, 0x5, CMD_CIPHER_PROCESS_S)
#define    CMD_CIPHER_DECRYPTMULTI           _IOW(FH_ID_CIPHER, 0x6, CMD_CIPHER_PROCESS_S)
#define    CMD_CIPHER_ENCRYPTMULTI           _IOW(FH_ID_CIPHER, 0x7, CMD_CIPHER_PROCESS_S)
#define    CMD_CIPHER_GETHANDLECONFIG        _IOWR(FH_ID_CIPHER, 0x9, CMD_CIPHER_CTRL_S)
#define    CMD_CIPHER_DECRYPTMULTI_EX        _IOW(FH_ID_CIPHER, 0xe, CMD_CIPHER_PROCESS_S)
#define    CMD_CIPHER_ENCRYPTMULTI_EX        _IOW(FH_ID_CIPHER, 0xf, CMD_CIPHER_PROCESS_S)

#ifdef CIPHER_KLAD_SUPPORT
#define    CMD_CIPHER_KLAD_KEY               _IOWR(FH_ID_CIPHER, 0x12, CIPHER_KLAD_KEY_S)
#endif

#ifdef CIPHER_RNG_SUPPORT
#define    CMD_CIPHER_GETRANDOMNUMBER        _IOWR(FH_ID_CIPHER,  0x8, CIPHER_RNG_S)
#endif

#ifdef CIPHER_HASH_SUPPORT
#define    CMD_CIPHER_CALCHASHINIT           _IOWR(FH_ID_CIPHER, 0xa, CMD_HASH_INIT_S)
#define    CMD_CIPHER_CALCHASHUPDATE         _IOWR(FH_ID_CIPHER, 0xb, CMD_HASH_UPDATE_S)
#define    CMD_CIPHER_CALCHASHFINAL          _IOWR(FH_ID_CIPHER, 0xc, CMD_HASH_FINAL_S)
#define    CMD_CIPHER_CALCHASH               _IOWR(FH_ID_CIPHER, 0x10, CMD_HASH_DIGEST_S)
#define    CMD_CIPHER_GET_HASHINFO           _IOWR(FH_ID_CIPHER, 0x11, CMD_GET_HASHINFO_S)
#endif

#ifdef CIPHER_RSA_SUPPORT
#define    CMD_CIPHER_CALCRSA                _IOWR(FH_ID_CIPHER, 0xd, CIPHER_RSA_DATA_S)
#endif



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* End of #ifndef __DRV_CIPHER_IOCTL_H__*/
