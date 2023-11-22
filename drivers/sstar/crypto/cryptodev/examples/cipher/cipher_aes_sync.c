/*
 * Demo on how to use /dev/crypto device for ciphering.
 *
 * Placed under public domain.
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include "cryptodev.h"

static int debug = 1;

#define BLOCK_SIZE 16
#define KEY_SIZE   16

typedef struct
{
    char *pu8Src;
    int   dataSize;

    char *pu8Key;
    int   keyLen;

    char *pu8Dst;

    int alg; // cryptodev_crypto_op_t
} CIPHER_AES_CASE_S;

#define my_perror(x)                                     \
    {                                                    \
        fprintf(stderr, "%s: %d\n", __func__, __LINE__); \
        perror(x);                                       \
    }

static int test_cipher_aes_case(int cfd, CIPHER_AES_CASE_S *pstCase)
{
    char  iv[BLOCK_SIZE];
    char *plaintext;
    char *ciphertext;
    char  szDesc[64] = {
        0,
    };

    struct session_op sess;
#ifdef CIOCGSESSINFO
    struct session_info_op siop;
#endif
    struct crypt_op cryp;

    memset(&sess, 0, sizeof(sess));
    memset(&cryp, 0, sizeof(cryp));

    memset(pstCase->pu8Key, 0x33, pstCase->keyLen);
    memset(iv, 0x03, BLOCK_SIZE);

    /* Get crypto session for AES128 */
    sess.cipher = pstCase->alg;
    sess.keylen = pstCase->keyLen;
    sess.key    = pstCase->pu8Key;
    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        my_perror("ioctl(CIOCGSESSION)");
        return 1;
    }

#ifdef CIOCGSESSINFO
    siop.ses = sess.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop))
    {
        my_perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    if (debug)
        printf("requested cipher %s, got %s with driver %s\n",
               (pstCase->alg == CRYPTO_AES_CBC)   ? "CRYPTO_AES_CBC"
               : (pstCase->alg == CRYPTO_AES_ECB) ? "CRYPTO_AES_ECB"
               : (pstCase->alg == CRYPTO_AES_CTR) ? "CRYPTO_AES_CTR"
                                                  : "unkonw",
               siop.cipher_info.cra_name, siop.cipher_info.cra_driver_name);

    if (siop.alignmask)
    {
        plaintext  = (char *)(((unsigned long)pstCase->pu8Src + siop.alignmask) & ~siop.alignmask);
        ciphertext = (char *)(((unsigned long)pstCase->pu8Dst + siop.alignmask) & ~siop.alignmask);
    }
    else
    {
        plaintext  = pstCase->pu8Src;
        ciphertext = pstCase->pu8Dst;
    }
#else
    plaintext  = pstCase->pu8Src;
    ciphertext = pstCase->pu8dst;
#endif

    memset(plaintext, 0x77, pstCase->dataSize);
    memset(ciphertext, 0x00, pstCase->dataSize);

    /* Encrypt data.in to data.encrypted */
    cryp.ses = sess.ses;
    cryp.len = pstCase->dataSize;
    cryp.src = plaintext;
    cryp.dst = ciphertext;
    cryp.iv  = iv;
    cryp.op  = COP_ENCRYPT;
    if (ioctl(cfd, CIOCCRYPT, &cryp))
    {
        my_perror("ioctl(CIOCCRYPT)");
        return 1;
    }

    if (ioctl(cfd, CIOCFSESSION, &sess.ses))
    {
        my_perror("ioctl(CIOCFSESSION)");
        return 1;
    }

    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        my_perror("ioctl(CIOCGSESSION)");
        return 1;
    }

#ifdef CIOCGSESSINFO
    siop.ses = sess.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop))
    {
        my_perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    if (debug)
        printf("requested cipher %s, got %s with driver %s\n",
               (pstCase->alg == CRYPTO_AES_CBC)   ? "CRYPTO_AES_CBC"
               : (pstCase->alg == CRYPTO_AES_ECB) ? "CRYPTO_AES_ECB"
               : (pstCase->alg == CRYPTO_AES_CTR) ? "CRYPTO_AES_CTR"
                                                  : "unkonw",
               siop.cipher_info.cra_name, siop.cipher_info.cra_driver_name);
#endif

    /* Decrypt data.encrypted to data.decrypted */
    cryp.ses = sess.ses;
    cryp.len = pstCase->dataSize;
    cryp.src = ciphertext;
    cryp.dst = ciphertext;
    cryp.iv  = iv;
    cryp.op  = COP_DECRYPT;
    if (ioctl(cfd, CIOCCRYPT, &cryp))
    {
        my_perror("ioctl(CIOCCRYPT)");
        return 1;
    }

    snprintf(szDesc, sizeof(szDesc) - 1, "%s-KEY-%d-DATA-%d",
             (pstCase->alg == CRYPTO_AES_CBC)   ? "CRYPTO_AES_CBC"
             : (pstCase->alg == CRYPTO_AES_ECB) ? "CRYPTO_AES_ECB"
             : (pstCase->alg == CRYPTO_AES_CTR) ? "CRYPTO_AES_CTR"
                                                : "unkonw",
             pstCase->keyLen, pstCase->dataSize);

    /* Verify the result */
    if (memcmp(plaintext, ciphertext, pstCase->dataSize) != 0)
    {
        int i;

        if (debug)
        {
            fprintf(stderr, "FAIL: Decrypted data are different from the input data, case %s\n", szDesc);

            printf("ciphertext:\n0000: ");
            for (i = 0; i < pstCase->dataSize; i++)
            {
                printf("%02x ", ciphertext[i]);
                if ((i % 32) == 31)
                    printf("\n%04d: ", i);
            }
            printf("\n");
        }

        return 1;
    }
    else if (debug)
        printf("Test %s passed\n", szDesc);

    /* Finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses))
    {
        my_perror("ioctl(CIOCFSESSION)");
        return 1;
    }

    return 0;
}

static int test_crypto(int cfd)
{
    char *            plaintext_raw  = NULL;
    char *            ciphertext_raw = NULL;
    CIPHER_AES_CASE_S stCryptoCase;
    char              key[KEY_SIZE];
    char              szDesc[64];

    int dataSize[] = {1 * 1024, 2 * 1024, 3 * 1024, 4 * 1024, 5 * 1024, 6 * 1024, 7 * 1024, 8 * 1024};

    int alg[] = {CRYPTO_AES_ECB, CRYPTO_AES_CBC, CRYPTO_AES_CTR};
    int i, j;

    char *other;
    for (i = 0; i < sizeof(alg) / sizeof(alg[0]); i++)
    {
        for (j = 0; j < sizeof(dataSize) / sizeof(dataSize[0]); j++)
        {
            plaintext_raw  = malloc(dataSize[j] + 63);
            ciphertext_raw = malloc(dataSize[j] + 63);
            // other = malloc(dataSize[j] + 63);
            memset(&stCryptoCase, 0, sizeof(CIPHER_AES_CASE_S));
            stCryptoCase.pu8Src   = plaintext_raw;
            stCryptoCase.dataSize = dataSize[j];
            stCryptoCase.pu8Key   = key;
            stCryptoCase.keyLen   = KEY_SIZE;
            stCryptoCase.pu8Dst   = ciphertext_raw;
            stCryptoCase.alg      = alg[i];
            // stCryptoCase.pu8other = other;

            snprintf(szDesc, sizeof(szDesc) - 1, "%s-KEY-%d-DATA-%d",
                     (alg[i] == CRYPTO_AES_CBC)   ? "CRYPTO_AES_CBC"
                     : (alg[i] == CRYPTO_AES_ECB) ? "CRYPTO_AES_ECB"
                     : (alg[i] == CRYPTO_AES_CTR) ? "CRYPTO_AES_CTR"
                                                  : "unkonw",
                     KEY_SIZE, dataSize[j]);

            if (0 == test_cipher_aes_case(cfd, &stCryptoCase))
            {
                printf("\033[0;32mTest %s passed \033[0m\n", szDesc);
            }
            else
            {
                printf("\033[0;31mTest %s fail \033[0m\n", szDesc);
            }

            free(plaintext_raw);
            free(ciphertext_raw);
        }
    }

    return 0;
}

static int test_aes(int cfd)
{
    char plaintext1_raw[BLOCK_SIZE + 63], *plaintext1;
    char ciphertext1[BLOCK_SIZE] = {0xdf, 0x55, 0x6a, 0x33, 0x43, 0x8d, 0xb8, 0x7b,
                                    0xc4, 0x1b, 0x17, 0x52, 0xc5, 0x5e, 0x5e, 0x49};
    char iv1[BLOCK_SIZE];
    char key1[KEY_SIZE]              = {0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    char plaintext2_data[BLOCK_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00};
    char plaintext2_raw[BLOCK_SIZE + 63], *plaintext2;
    char ciphertext2[BLOCK_SIZE] = {0xb7, 0x97, 0x2b, 0x39, 0x41, 0xc4, 0x4b, 0x90,
                                    0xaf, 0xa7, 0xb2, 0x64, 0xbf, 0xba, 0x73, 0x87};
    char iv2[BLOCK_SIZE];
    char key2[KEY_SIZE];

    struct session_op sess;
#ifdef CIOCGSESSINFO
    struct session_info_op siop;
#endif
    struct crypt_op cryp;

    memset(&sess, 0, sizeof(sess));
    memset(&cryp, 0, sizeof(cryp));

    /* Get crypto session for AES128 */
    sess.cipher = CRYPTO_AES_CBC;
    sess.keylen = KEY_SIZE;
    sess.key    = key1;
    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }
#ifdef CIOCGSESSINFO
    siop.ses = sess.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop))
    {
        perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    plaintext1 = (char *)(((unsigned long)plaintext1_raw + siop.alignmask) & ~siop.alignmask);
#else
    plaintext1 = plaintext1_raw;
#endif
    memset(plaintext1, 0x0, BLOCK_SIZE);
    memset(iv1, 0x0, sizeof(iv1));

    /* Encrypt data.in to data.encrypted */
    cryp.ses = sess.ses;
    cryp.len = BLOCK_SIZE;
    cryp.src = plaintext1;
    cryp.dst = plaintext1;
    cryp.iv  = iv1;
    cryp.op  = COP_ENCRYPT;
    if (ioctl(cfd, CIOCCRYPT, &cryp))
    {
        perror("ioctl(CIOCCRYPT)");
        return 1;
    }

    /* Verify the result */
    if (memcmp(plaintext1, ciphertext1, BLOCK_SIZE) != 0)
    {
        fprintf(stderr, "FAIL: Decrypted data are different from the input data.\n");
        return 1;
    }

    /* Test 2 */

    memset(key2, 0x0, sizeof(key2));
    memset(iv2, 0x0, sizeof(iv2));

    /* Get crypto session for AES128 */
    sess.cipher = CRYPTO_AES_CBC;
    sess.keylen = KEY_SIZE;
    sess.key    = key2;
    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }

#ifdef CIOCGSESSINFO
    siop.ses = sess.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop))
    {
        perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    if (debug)
        printf("requested cipher CRYPTO_AES_CBC, got %s with driver %s\n", siop.cipher_info.cra_name,
               siop.cipher_info.cra_driver_name);

    plaintext2 = (char *)(((unsigned long)plaintext2_raw + siop.alignmask) & ~siop.alignmask);
#else
    plaintext2 = plaintext2_raw;
#endif
    memcpy(plaintext2, plaintext2_data, BLOCK_SIZE);

    /* Encrypt data.in to data.encrypted */
    cryp.ses = sess.ses;
    cryp.len = BLOCK_SIZE;
    cryp.src = plaintext2;
    cryp.dst = plaintext2;
    cryp.iv  = iv2;
    cryp.op  = COP_ENCRYPT;
    if (ioctl(cfd, CIOCCRYPT, &cryp))
    {
        perror("ioctl(CIOCCRYPT)");
        return 1;
    }

    /* Verify the result */
    if (memcmp(plaintext2, ciphertext2, BLOCK_SIZE) != 0)
    {
        int i;
        fprintf(stderr, "FAIL: Decrypted data are different from the input data.\n");
    }

    if (debug)
        printf("AES Test passed\n");

    /* Finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses))
    {
        perror("ioctl(CIOCFSESSION)");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int fd = -1, cfd = -1;

    if (argc > 1)
        debug = 1;

    /* Open the crypto device */
    fd = open("/dev/crypto", O_RDWR, 0);
    if (fd < 0)
    {
        perror("open(/dev/crypto)");
        return 1;
    }

    /* Clone file descriptor */
    if (ioctl(fd, CRIOGET, &cfd))
    {
        perror("ioctl(CRIOGET)");
        return 1;
    }

    /* Set close-on-exec (not really neede here) */
    if (fcntl(cfd, F_SETFD, 1) == -1)
    {
        perror("fcntl(F_SETFD)");
        return 1;
    }

    /* Run the test itself */
    if (test_aes(cfd))
        return 1;

    if (test_crypto(cfd))
        return 1;

    /* Close cloned descriptor */
    if (close(cfd))
    {
        perror("close(cfd)");
        return 1;
    }

    /* Close the original descriptor */
    if (close(fd))
    {
        perror("close(fd)");
        return 1;
    }

    printf("exit ret=0\n");
    return 0;
}
