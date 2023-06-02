#ifndef _MDRV_AESDMA_IO_ST_H
#define _MDRV_AESDMA_IO_ST_H

#include <linux/types.h>
#ifndef __KERNEL__
#define __user
#endif

/* API extensions for linux */
#define CRYPTO_HMAC_MAX_KEY_LEN		512
#define CRYPTO_CIPHER_MAX_KEY_LEN	64

/* All the supported algorithms
 */

enum cryptodev_crypto_op_t {
    CRYPTO_RIJNDAEL128_CBC = 1,
	CRYPTO_AES_CBC = CRYPTO_RIJNDAEL128_CBC,
	CRYPTO_SHA1 = 2,
	CRYPTO_AES_CTR = 3,
	CRYPTO_AES_ECB = 4,
	CRYPTO_SHA2_256,
	CRYPTO_ALGORITHM_ALL, /* Keep updated - see below */
	/*CRYPTO_RIJNDAEL128_CBC = 1,
	CRYPTO_AES_CBC = CRYPTO_RIJNDAEL128_CBC,
	CRYPTO_AES_CTR = 2,
	CRYPTO_AES_ECB = 3,
	CRYPTO_DES_CBC = 4,
	CRYPTO_DES_CTR = 5,
	CRYPTO_DES_ECB = 6,
	CRYPTO_TDES_CBC = 7,
	CRYPTO_TDES_CTR = 8,
	CRYPTO_TDES_ECB = 9,
	CRYPTO_SHA1 = 10,
	CRYPTO_SHA2_256 = 11,
    CRYPTO_ARC4 = 12,
	CRYPTO_MD5 = 13,
	CRYPTO_CAMELLIA_CBC = 101,
	CRYPTO_BLF_CBC = 102,
	CRYPTO_ALGORITHM_ALL,*/ /* Keep updated - see below */
};
#define	CRYPTO_ALGORITHM_MAX	(CRYPTO_ALGORITHM_ALL - 1)

/* Values for ciphers */
#define DES_BLOCK_LEN		8
#define DES3_BLOCK_LEN		8
#define RIJNDAEL128_BLOCK_LEN	16
#define AES_BLOCK_LEN		RIJNDAEL128_BLOCK_LEN
#define CAMELLIA_BLOCK_LEN      16
#define BLOWFISH_BLOCK_LEN	8
#define SKIPJACK_BLOCK_LEN	8
#define CAST128_BLOCK_LEN	8

/* the maximum of the above */
#define EALG_MAX_BLOCK_LEN	16

/* Values for hashes/MAC */
#define AALG_MAX_RESULT_LEN		64

/* maximum length of verbose alg names (depends on CRYPTO_MAX_ALG_NAME) */
#define CRYPTODEV_MAX_ALG_NAME		64

#define HASH_MAX_LEN 64

/* input of CIOCGSESSION */
struct session_op {
	/* Specify either cipher or mac
	 */
	__u32	cipher;		/* cryptodev_crypto_op_t */
	__u32	mac;		/* cryptodev_crypto_op_t */

	__u32	keylen;
	__u8	__user *key;
	__u32	mackeylen;
	__u8	__user *mackey;

	__u32	ses;		/* session identifier */
};

struct session_info_op {
	__u32 ses;		/* session identifier */

	/* verbose names for the requested ciphers */
	struct alg_info {
		char cra_name[CRYPTODEV_MAX_ALG_NAME];
		char cra_driver_name[CRYPTODEV_MAX_ALG_NAME];
	} cipher_info, hash_info;

	__u16	alignmask;	/* alignment constraints */
	__u32   flags;          /* SIOP_FLAGS_* */
};

/* If this flag is set then this algorithm uses
 * a driver only available in kernel (software drivers,
 * or drivers based on instruction sets do not set this flag).
 *
 * If multiple algorithms are involved (as in AEAD case), then
 * if one of them is kernel-driver-only this flag will be set.
 */
#define SIOP_FLAG_KERNEL_DRIVER_ONLY 1
#define	COP_ENCRYPT	0
#define COP_DECRYPT	1
/* input of CIOCCRYPT */
struct crypt_op {
	__u32	ses;		/* session identifier */
	__u16	op;		/* COP_ENCRYPT or COP_DECRYPT */
	__u16	flags;		/* see COP_FLAG_* */
	__u32	len;		/* length of source data */
	__u8	__user *src;	/* source data */
	__u8	__user *dst;	/* pointer to output data */
	/* pointer to output data for hash/MAC operations */
	__u8	__user *mac;
	/* initialization vector for encryption operations */
	__u8	__user *iv;
};

/* struct crypt_op flags */
#define COP_FLAG_NONE		(0 << 0) /* totally no flag */
#define COP_FLAG_UPDATE		(1 << 0) /* multi-update hash mode */
#define COP_FLAG_FINAL		(1 << 1) /* multi-update final hash mode */
#define COP_FLAG_WRITE_IV	(1 << 2) /* update the IV during operation */
#define COP_FLAG_NO_ZC		(1 << 3) /* do not zero-copy */
#define COP_FLAG_AEAD_TLS_TYPE  (1 << 4) /* authenticate and encrypt using the
                                          * TLS protocol rules */
#define COP_FLAG_AEAD_SRTP_TYPE  (1 << 5) /* authenticate and encrypt using the
                                           * SRTP protocol rules */
#define COP_FLAG_RESET		(1 << 6) /* multi-update reset the state.
                                          * should be used in combination
                                          * with COP_FLAG_UPDATE */
#define CRK_MAXPARAM	8
#define CRK_ALGORITHM_MAX	(CRK_ALGORITHM_ALL-1)

#endif