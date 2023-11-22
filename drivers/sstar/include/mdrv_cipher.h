/*
 * mdrv_cipher.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _SS_CIPHER_H_
#define _SS_CIPHER_H_

#include <linux/acpi.h>

#define AES_MODE_ECB 0
#define AES_MODE_CBC 1
#define AES_MODE_CTR 2

#if defined(CONFIG_ARCH_MERCURY6P)
#define RSA_LEN (64 * 2)
#else
#define RSA_LEN 64
#endif
/*
 * AES
 */

// algorithm mode
typedef enum
{
    E_AES_ALGO_ECB = 0,
    E_AES_ALGO_CBC,
    E_AES_ALGO_CTR,
} MDRV_AES_ALGO_MODE;

typedef enum
{
    E_AES_KEY_SRC_INT_UNI  = 0,
    E_AES_KEY_SRC_INT_OTP1 = 1,
    E_AES_KEY_SRC_INT_OTP2 = 2,
    E_AES_KEY_SRC_INT_OTP3 = 3,
    E_AES_KEY_SRC_INT_OTP4 = 4,
    E_AES_KEY_SRC_INT_OTP5 = 5,
    E_AES_KEY_SRC_INT_OTP6 = 6,
    E_AES_KEY_SRC_INT_OTP7 = 7,
    E_AES_KEY_SRC_INT_OTP8 = 8
} MDRV_AES_KEY_SOURCE;

// AES handle
typedef struct
{
    u8 *               in;      // input vitr addr
    u8 *               out;     // output vitr addr
    u32                len;     // length of input
    u8                 key[16]; // key
    u8                 iv[16];  // initial vector
    u32                keylen;  // length of key
    MDRV_AES_ALGO_MODE mode;
    dma_addr_t         in_pa;  // input phy addr
    dma_addr_t         out_pa; // output phy addr
} MDRV_AES_HANDLE;

/*
 * RSA
 */

// RSA handle
typedef struct
{
    u32 exp[RSA_LEN];     // exponent, 256 bit in max for RSA-2048
    u32 modulus[RSA_LEN]; // public / private modulus, 256 bit in max for RSA-2048
    u32 exp_len;          // length of exponent
    u32 mod_len;          // lenght of modulus
    u32 in[RSA_LEN];      // input, 256 bit in max for RSA-2048
    u32 out[RSA_LEN];     // output, 256 bit in max for RSA-2048
    u8  len;              // length of input, must be less than modulus length
    u8  pub_ekey;         // privateExponent(0) or publicExponent(1),
} MDRV_RSA_HANDLE;

/*
 * SHA
 */
typedef enum
{
    E_SHA_1        = 0,
    E_SHA_256      = 1,
    E_SHA_1_ONCE   = 2,
    E_SHA_256_ONCE = 3,
    E_SHA_MODE_NUM = 4,
} MDRV_SHA_MODE;

typedef struct
{
    u32 u32ShaVal[8]; // SHA256_DIGEST_SIZE, final hash value
    u64 u32DataPhy;   // data physical address
    u32 u32DataLen;
    struct
    {
        u32 state[8]; // SHA256_DIGEST_SIZE, init hash value
        u32 count;    // bytes processed
    } ctx;
    MDRV_SHA_MODE mode;
} MDRV_SHA_HANDLE;

// Public
int cipher_aes_init(void);
int cipher_aes_uninit(void);
int cipher_aes_encrypt(MDRV_AES_HANDLE *handle);
int cipher_aes_decrypt(MDRV_AES_HANDLE *handle);
int cipher_rsa_crypto(MDRV_RSA_HANDLE *handle);
int cipher_sha_init(MDRV_SHA_HANDLE *handle);
int cipher_sha_update(MDRV_SHA_HANDLE *handle);
int cipher_sha_final(MDRV_SHA_HANDLE *handle);
u32 cipher_random_num(void);

#endif //_SS_CIPHER_H_
