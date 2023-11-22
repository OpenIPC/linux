/*  hashcrypt_speed - simple SHA+AES benchmark tool for cryptodev
 *
 *    Copyright (C) 2011 by Phil Sutter <phil.sutter@viprinet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include "cryptodev.h"
#include <unistd.h>

#define SHA256_RESULT_LEN 32

static int debug = 0;

typedef struct
{
    char *pu8InputData;
    int   dataSize;

    char *pu8Key;
    int   keyLen;

    int alg; // cryptodev_crypto_op_t
} CIPHER_SHA256_CASE_S;

static int test_cipher_hash256_case(int fdc, CIPHER_SHA256_CASE_S *pstCase)
{
    int   i = 0;
    int   j = 0;
    char *plaintext;

    uint8_t           mac1[SHA256_RESULT_LEN];
    uint8_t           mac2[SHA256_RESULT_LEN];
    struct session_op sess;
#ifdef CIOCGSESSINFO
    struct session_info_op siop;
#endif
    struct crypt_op cop;

    for (i = 0; i < 10; i++)
    {
        // get hash data1
        memset(pstCase->pu8Key, 0x42, pstCase->keyLen);

        memset(&sess, 0, sizeof(sess));
        sess.cipher = pstCase->alg;
        sess.keylen = pstCase->keyLen;
        sess.key    = (unsigned char *)pstCase->pu8Key;
        sess.mac    = CRYPTO_SHA2_256;
        if (ioctl(fdc, CIOCGSESSION, &sess))
        {
            perror("ioctl(CIOCGSESSION)");
            return 1;
        }
#ifdef CIOCGSESSINFO
        siop.ses = sess.ses;
        if (ioctl(fdc, CIOCGSESSINFO, &siop))
        {
            perror("ioctl(CIOCGSESSINFO)");
            return 1;
        }

        if (debug)
            printf("requested hash CRYPTO_SHA2_256, got %s with driver %s\n", siop.hash_info.cra_name,
                   siop.hash_info.cra_driver_name);

        if (siop.alignmask)
        {
            plaintext = (char *)(((unsigned long)pstCase->pu8InputData + siop.alignmask) & ~siop.alignmask);
        }
        else
        {
            plaintext = pstCase->pu8InputData;
        }
#endif
        plaintext = pstCase->pu8InputData;
        memset(plaintext, 0x15, pstCase->dataSize);

        memset(&cop, 0, sizeof(cop));
        cop.ses = sess.ses;
        cop.len = pstCase->dataSize;
        cop.op  = COP_ENCRYPT;
        cop.src = cop.dst = plaintext;
        cop.mac           = mac1;

        if (ioctl(fdc, CIOCCRYPT, &cop))
        {
            perror("ioctl(CIOCCRYPT)");
            return 1;
        }

        /* Finish crypto session */
        if (ioctl(fdc, CIOCFSESSION, &sess.ses))
        {
            perror("ioctl(CIOCFSESSION)");
            return 1;
        }

        // get hash data2
        memset(pstCase->pu8Key, 0x42, pstCase->keyLen);

        memset(&sess, 0, sizeof(sess));
        sess.cipher = pstCase->alg;
        sess.keylen = pstCase->keyLen;
        sess.key    = (unsigned char *)pstCase->pu8Key;
        sess.mac    = CRYPTO_SHA2_256;
        if (ioctl(fdc, CIOCGSESSION, &sess))
        {
            perror("ioctl(CIOCGSESSION)");
            return 1;
        }
#ifdef CIOCGSESSINFO
        siop.ses = sess.ses;
        if (ioctl(fdc, CIOCGSESSINFO, &siop))
        {
            perror("ioctl(CIOCGSESSINFO)");
            return 1;
        }

        if (debug)
            printf("requested hash CRYPTO_SHA2_256, got %s with driver %s\n", siop.hash_info.cra_name,
                   siop.hash_info.cra_driver_name);

        if (siop.alignmask)
        {
            plaintext = (char *)(((unsigned long)pstCase->pu8InputData + siop.alignmask) & ~siop.alignmask);
        }
        else
        {
            plaintext = pstCase->pu8InputData;
        }
#endif
        plaintext = pstCase->pu8InputData;
        memset(plaintext, 0x15, pstCase->dataSize);

        memset(&cop, 0, sizeof(cop));
        cop.ses = sess.ses;
        cop.len = pstCase->dataSize;
        cop.op  = COP_ENCRYPT;
        cop.src = cop.dst = plaintext;
        cop.mac           = mac2;

        if (ioctl(fdc, CIOCCRYPT, &cop))
        {
            perror("ioctl(CIOCCRYPT)");
            return 1;
        }

        /* Finish crypto session */
        if (ioctl(fdc, CIOCFSESSION, &sess.ses))
        {
            perror("ioctl(CIOCFSESSION)");
            return 1;
        }

        if (memcmp(mac1, mac2, SHA256_RESULT_LEN) != 0)
        {
            printf("hash1 and hash2 not match, i = %d\n", i);

            printf("sha1:");
            for (j = 0; j < SHA256_RESULT_LEN; j++)
            {
                if ((j % 16) == 0)
                    printf("\n");
                printf("%02x ", mac1[j]);
            }
            printf("\nsha2:");
            for (j = 0; j < SHA256_RESULT_LEN; j++)
            {
                if ((j % 16) == 0)
                    printf("\n");
                printf("%02x ", mac2[j]);
            }
            printf("\n");

            return 1;
        }
    }

    return 0;
}

int main(void)
{
    int  fd, fdc = -1;
    char keybuf[32];
    int  ret = 0;

    if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
        perror("open()");
        return 1;
    }

    if (ioctl(fd, CRIOGET, &fdc))
    {
        perror("ioctl(CRIOGET)");
        return 1;
    }

    char *               plaintext_raw = NULL;
    char                 szDesc[64];
    CIPHER_SHA256_CASE_S stCryptoCase;

    int dataSize[] = {1 * 1024, 2 * 1024, 3 * 1024, 4 * 1024, 5 * 1024, 6 * 1024, 7 * 1024, 8 * 1024};

    int alg[] = {CRYPTO_AES_CBC, CRYPTO_AES_ECB, CRYPTO_AES_CTR};

    int i, j;

    for (i = 0; i < sizeof(alg) / sizeof(alg[0]); i++)
    {
        for (j = 0; j < sizeof(dataSize) / sizeof(dataSize[0]); j++)
        {
            plaintext_raw = malloc(dataSize[j] + 63);

            memset(&stCryptoCase, 0, sizeof(CIPHER_SHA256_CASE_S));
            stCryptoCase.pu8InputData = plaintext_raw;
            stCryptoCase.dataSize     = dataSize[j];
            stCryptoCase.pu8Key       = keybuf;
            stCryptoCase.keyLen       = 32;
            stCryptoCase.alg          = alg[i];

            snprintf(szDesc, sizeof(szDesc) - 1, "CRYPTO_%s_SHA256-KEY-%d-DATA-%d",
                     (alg[i] == CRYPTO_AES_CBC)   ? "CBC"
                     : (alg[i] == CRYPTO_AES_ECB) ? "ECB"
                     : (alg[i] == CRYPTO_AES_CTR) ? "CTR"
                                                  : "unknow",
                     32, dataSize[j]);

            if (0 == test_cipher_hash256_case(fdc, &stCryptoCase))
            {
                printf("\033[0;32mTest %s passed \033[0m\n", szDesc);
            }
            else
            {
                printf("\033[0;31mTest %s fail \033[0m\n", szDesc);
                ret = 1;
            }

            free(plaintext_raw);
        }
    }

    close(fdc);
    close(fd);
    printf("exit ret=%d\n", ret);
    return ret;
}
