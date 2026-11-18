/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/interrupt.h>
#include <linux/mutex.h>

#ifndef __AX_CIPHER_ADAPT_H_
#define __AX_CIPHER_ADAPT_H_

#define CE_CMM_TOKEN 						"ax_ce"
#define DMA_ALLOC_ALIGN 					0x40
#define OCCUPY_CE_MAILBOX_NR				(2)

typedef uint64_t				  ax_cipher_handle;
#define AX_TRUE_CE 1
#define AX_FALSE_CE 0
//#define bool char
#define ax_bool_ce char

/** Cipher algorithm */
typedef enum {
	AX_CIPHER_ALGO_HASH_SHA1	= 0,		  // SHA-1
	AX_CIPHER_ALGO_HASH_SHA224  = 1,		  // SHA-224
	AX_CIPHER_ALGO_HASH_SHA256  = 2,		  // SHA-256
	AX_CIPHER_ALGO_HASH_SHA384  = 3,		  // SHA-384
	AX_CIPHER_ALGO_HASH_SHA512  = 4,		  // SHA-512
	AX_CIPHER_ALGO_MAC_HMAC_SHA1 = 5,		 // HMAC-SHA-1
	AX_CIPHER_ALGO_MAC_HMAC_SHA224 = 6,	   // HMAC-SHA-224
	AX_CIPHER_ALGO_MAC_HMAC_SHA256 = 7,	   // HMAC-SHA-256
	AX_CIPHER_ALGO_MAC_HMAC_SHA384 = 8,	   // HMAC-SHA-384
	AX_CIPHER_ALGO_MAC_HMAC_SHA512 = 9,	   // HMAC-SHA-512
	AX_CIPHER_ALGO_MAC_AES_CMAC = 10,		 // AES-CMAC
	AX_CIPHER_ALGO_MAC_AES_CBC_MAC = 11,	  // AES-CBC-MAC
	AX_CIPHER_ALGO_CIPHER_AES = 12,		   // AES
	AX_CIPHER_ALGO_CIPHER_DES = 13,		   // DES
	AX_CIPHER_ALG_INVALID = 0xffffffff,
} AX_CIPHER_ALGO_E;
typedef enum {
	// (Block)Cipher modes
	AX_CIPHER_MODE_CIPHER_ECB = 0,		// ECB
	AX_CIPHER_MODE_CIPHER_CBC,			// CBC
	AX_CIPHER_MODE_CIPHER_CTR,			// CTR
	AX_CIPHER_MODE_CIPHER_ICM,			// ICM
	AX_CIPHER_MODE_CIPHER_F8,			 // F8
	AX_CIPHER_MODE_CIPHER_CCM,			// CCM
	AX_CIPHER_MODE_CIPHER_XTS,			// XTS
	AX_CIPHER_MODE_CIPHER_GCM,			// GCM
	AX_CIPHER_MODE_CIPHER_MAX,			// must be last
} AX_CIPHER_MODE_E;

typedef enum AX_CIPHER_RSA_SIGN_SCHEME_E {
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA1 = 0x0,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA224,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA384,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA512,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA384,
	AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA512,
	AX_CIPHER_RSA_SIGN_SCHEME_INVALID  = 0xffffffff,
} AX_CIPHER_RSA_SIGN_SCHEME_E;

typedef enum {
	AX_CIPHER_SUCCESS = 0,					/** No error */
	AX_CIPHER_INVALID_TOKEN = 0x80020001,			 /** Invalid token */
	AX_CIPHER_INVALID_PARAMETER = 0x80020002,		 /** Invalid parameter */
	AX_CIPHER_INVALID_KEYSIZE = 0x80020003,		   /** Invalid key size */
	AX_CIPHER_INVALID_LENGTH = 0x80020004,			/** Invalid length */
	AX_CIPHER_INVALID_LOCATION = 0x80020005,		  /** Invalid location */
	AX_CIPHER_CLOCK_ERROR = 0x80020006,			   /** Clock error */
	AX_CIPHER_ACCESS_ERROR = 0x80020007,			  /** Access error */
	AX_CIPHER_UNWRAP_ERROR = 0x80020008,			 /** Unwrap error */
	AX_CIPHER_DATA_OVERRUN_ERROR = 0x80020009,	   /** Data overrun error */
	AX_CIPHER_ASSET_CHECKSUM_ERROR = 0x8002000A,	 /** Asset checksum error */
	AX_CIPHER_INVALID_ASSET = 0x8002000B,			/** Invalid Asset */
	AX_CIPHER_FULL_ERROR = 0x8002000C,			   /** Full/Overflow error */
	AX_CIPHER_INVALID_ADDRESS = 0x8002000D,		  /** Invalid address */
	AX_CIPHER_INVALID_MODULUS = 0x8002000E,		  /** Invalid Modulus */
	AX_CIPHER_VERIFY_ERROR = 0x8002000F,			 /** Verify error */
	AX_CIPHER_INVALID_STATE = 0x80020010,			/** Invalid state */
	AX_CIPHER_OTP_WRITE_ERROR = 0x80020011,		  /** OTP write error */
	AX_CIPHER_ASSET_EXPIRED = 0x80020012,			/** Asset expired error */
	AX_CIPHER_COPROCESSOR_ERROR = 0x80020013,		/** Coprocessor error */
	AX_CIPHER_PANIC_ERROR = 0x80020014,			  /** Panic error */
	AX_CIPHER_TRNG_SHUTDOWN_ERROR = 0x80020015,	  /** Too many FROs shutdown */
	AX_CIPHER_DRBG_STUCK_ERROR = 0x80020016,		 /** Stuck DRBG */
	AX_CIPHER_UNSUPPORTED = 0x80020017,			 /** Not supported */
	AX_CIPHER_NOT_INITIALIZED = 0x80020018,		 /** Not initialized yet */
	AX_CIPHER_BAD_ARGUMENT = 0x80020019,			/** Wrong use; not depending on configuration */
	AX_CIPHER_INVALID_ALGORITHM = 0x8002001A,	   /** Invalid algorithm code */
	AX_CIPHER_INVALID_MODE = 0x8002001B,			/** Invalid mode code */
	AX_CIPHER_BUFFER_TOO_SMALL = 0x8002001C,		/** Provided buffer too small for intended use */
	AX_CIPHER_NO_MEMORY = 0x8002001D,			   /** No memory */
	AX_CIPHER_OPERATION_FAILED = 0x8002001E,		/** Operation failed */
	AX_CIPHER_TIMEOUT_ERROR = 0x8002001F,		   /** Token or data timeout error */
	AX_CIPHER_INTERNAL_ERROR = 0x80020020,		  /** Internal error */
	AX_CIPHER_LOAD_KEY_ERROR = 0x80020021,		  /** load key error */
	AX_CIPHER_OPEN_ERROR = 0x80020022,		  /** load dev error */
} AX_CIPHER_STS;

#define  AX_CIPHER_LOG_LEVEL  (4)
#define  CE_FATAL_LEVEL  0
#define  CE_ERR_LEVEL  1
#define  CE_WARNING_LEVEL  2
#define  CE_INFO_LEVEL  3
#define  CE_DEBUG_LEVEL  4

#if 0
#define  CE_LOG_PR(log_level,format, ...) \
	do {  \
		switch(log_level) { \
		case CE_FATAL_LEVEL: \
			ax_pr_crit(AX_ID_CE, "CE", "[CE - 2][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_ERR_LEVEL: \
			ax_pr_err(AX_ID_CE, "CE", "[CE - 2][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_WARNING_LEVEL: \
			ax_pr_warn(AX_ID_CE, "CE", "[CE - 2][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_INFO_LEVEL: \
			ax_pr_info(AX_ID_CE, "CE", "[CE - 2][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_DEBUG_LEVEL: \
			ax_pr_debug(AX_ID_CE, "CE", "[CE - 2][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		default: \
			break; \
		} \
	} while (0)

#else
#define  CE_LOG_PR(log_level,format, ...) \
	do {  \
		switch(log_level) { \
		case CE_FATAL_LEVEL: \
			printk("[CE - E][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_ERR_LEVEL: \
			printk("[CE - E][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_WARNING_LEVEL: \
			printk("[CE - W][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_INFO_LEVEL: \
			printk("[CE - I][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		case CE_DEBUG_LEVEL: \
			printk("[CE - D][%s][%d]: " format "\n", __func__, __LINE__,  ##__VA_ARGS__); \
			break; \
		default: \
			break; \
		} \
	} while (0)
#endif

uint32_t ax_cipher_regread(void * addr);
void ax_cipher_regwrite(uint32_t val, void * addr);
void *ax_cipher_reversememcpy(void *dest, void *src, uint32_t size);
void ax_cipher_convert_endian(uint8_t *buf, int len);
int ax_copy_to_user(void *dst, void *src, uint32_t size);
int ax_copy_from_user(void *dst, void *src, uint32_t size);
int ax_mutex_init(void *mutex);
int ax_mutex_deinit(void *mutex);
int ax_mutex_lock(void *mutex);
int ax_mutex_unlock(void *mutex);
void ax_cipher_dump(uint32_t *token, int size);
void ax_cipher_dump_bytes(uint8_t *token, int size);
void dump_token(int *token);
#endif
