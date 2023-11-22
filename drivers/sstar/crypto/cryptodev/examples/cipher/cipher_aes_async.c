/*
 * Demo on how to use /dev/crypto device for ciphering.
 *
 * Placed under public domain.
 *
 */
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include "cryptodev.h"

static int debug = 0;

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

/* poll until POLLOUT, then call CIOCASYNCCRYPT */
int do_async_crypt(int cfd, struct crypt_op *cryp)
{
    struct pollfd pfd;

    pfd.fd     = cfd;
    pfd.events = POLLOUT;

    if (poll(&pfd, 1, -1) < 1)
    {
        perror("poll()");
        return 1;
    }

    if (ioctl(cfd, CIOCASYNCCRYPT, cryp))
    {
        perror("ioctl(CIOCASYNCCRYPT)");
        return 1;
    }
    return 0;
}

/* poll until POLLIN, then call CIOCASYNCFETCH */
int do_async_fetch(int cfd, struct crypt_op *cryp)
{
    struct pollfd pfd;

    pfd.fd     = cfd;
    pfd.events = POLLIN;

    if (poll(&pfd, 1, -1) < 1)
    {
        perror("poll()");
        return 1;
    }

    if (ioctl(cfd, CIOCASYNCFETCH, cryp))
    {
        perror("ioctl(CIOCCRYPT)");
        return 1;
    }
    return 0;
}

/* Check return value of stmt for identity with goodval. If they
 * don't match, call return with the value of stmt. */
#define DO_OR_DIE(stmt, goodval)                         \
    {                                                    \
        int __rc_val;                                    \
        if ((__rc_val = stmt) != goodval)                \
        {                                                \
            perror("DO_OR_DIE(" #stmt "," #goodval ")"); \
            return __rc_val;                             \
        }                                                \
    }

static int test_cipher_aes_case(int cfd, CIPHER_AES_CASE_S *pstCase)
{
    char *plaintext;
    char *ciphertext;
    char  iv[BLOCK_SIZE];

    struct session_op sess;
#ifdef CIOCGSESSINFO
    struct session_info_op siop;
#endif
    struct crypt_op cryp;

    if (debug)
        printf("running %s\n", __func__);

    memset(&sess, 0, sizeof(sess));
    memset(&cryp, 0, sizeof(cryp));

    memset(pstCase->pu8Key, 0x33, pstCase->keyLen);
    memset(iv, 0x03, sizeof(iv));

    /* Get crypto session for AES128 */
    sess.cipher = pstCase->alg;
    sess.keylen = pstCase->keyLen;
    sess.key    = pstCase->pu8Key;
    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }

    if (debug)
        printf("%s: got the session\n", __func__);

#ifdef CIOCGSESSINFO
    siop.ses = sess.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop))
    {
        perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }

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
    ciphertext = pstCase->pu8Dst;
#endif

    memset(plaintext, 0x15, pstCase->dataSize);

    /* Encrypt data.in to data.encrypted */
    cryp.ses = sess.ses;
    cryp.len = pstCase->dataSize;
    cryp.src = plaintext;
    cryp.dst = ciphertext;
    cryp.iv  = iv;
    cryp.op  = COP_ENCRYPT;

    DO_OR_DIE(do_async_crypt(cfd, &cryp), 0);
    DO_OR_DIE(do_async_fetch(cfd, &cryp), 0);

    if (debug)
        printf("%s: data encrypted\n", __func__);

    if (ioctl(cfd, CIOCFSESSION, &sess.ses))
    {
        perror("ioctl(CIOCFSESSION)");
        return 1;
    }
    if (debug)
        printf("%s: session finished\n", __func__);

    if (ioctl(cfd, CIOCGSESSION, &sess))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }
    if (debug)
        printf("%s: got new session\n", __func__);

    /* Decrypt data.encrypted to data.decrypted */
    cryp.ses = sess.ses;
    cryp.len = pstCase->dataSize;
    cryp.src = ciphertext;
    cryp.dst = ciphertext;
    cryp.iv  = iv;
    cryp.op  = COP_DECRYPT;

    DO_OR_DIE(do_async_crypt(cfd, &cryp), 0);
    DO_OR_DIE(do_async_fetch(cfd, &cryp), 0);

    if (debug)
        printf("%s: data encrypted\n", __func__);

    /* Verify the result */
    if (memcmp(plaintext, ciphertext, pstCase->dataSize) != 0)
    {
        if (debug)
            fprintf(stderr, "FAIL: Decrypted data are different from the input data.\n");

        return 1;
    }
    else if (debug)
        printf("Test passed\n");

    /* Finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess.ses))
    {
        perror("ioctl(CIOCFSESSION)");
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

    int alg[] = {CRYPTO_AES_CBC, CRYPTO_AES_ECB, CRYPTO_AES_CTR};
    int i, j;

    for (i = 0; i < sizeof(alg) / sizeof(alg[0]); i++)
    {
        for (j = 0; j < sizeof(dataSize) / sizeof(dataSize[0]); j++)
        {
            plaintext_raw  = malloc(dataSize[j] + 63);
            ciphertext_raw = malloc(dataSize[j] + 63);

            memset(&stCryptoCase, 0, sizeof(CIPHER_AES_CASE_S));
            stCryptoCase.pu8Src   = plaintext_raw;
            stCryptoCase.dataSize = dataSize[j];
            stCryptoCase.pu8Key   = key;
            stCryptoCase.keyLen   = KEY_SIZE;
            stCryptoCase.pu8Dst   = ciphertext_raw;
            stCryptoCase.alg      = alg[i];

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

    struct session_op sess1, sess2;
#ifdef CIOCGSESSINFO
    struct session_info_op siop1, siop2;
#endif
    struct crypt_op cryp1, cryp2;

    memset(&sess1, 0, sizeof(sess1));
    memset(&sess2, 0, sizeof(sess2));
    memset(&cryp1, 0, sizeof(cryp1));
    memset(&cryp2, 0, sizeof(cryp2));

    /* Get crypto session for AES128 */
    sess1.cipher = CRYPTO_AES_CBC;
    sess1.keylen = KEY_SIZE;
    sess1.key    = key1;
    if (ioctl(cfd, CIOCGSESSION, &sess1))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }
#ifdef CIOCGSESSINFO
    siop1.ses = sess1.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop1))
    {
        perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    plaintext1 = (char *)(((unsigned long)plaintext1_raw + siop1.alignmask) & ~siop1.alignmask);
#else
    plaintext1 = plaintext1_raw;
#endif
    memset(plaintext1, 0x0, BLOCK_SIZE);

    memset(iv1, 0x0, sizeof(iv1));
    memset(key2, 0x0, sizeof(key2));

    /* Get second crypto session for AES128 */
    sess2.cipher = CRYPTO_AES_CBC;
    sess2.keylen = KEY_SIZE;
    sess2.key    = key2;
    if (ioctl(cfd, CIOCGSESSION, &sess2))
    {
        perror("ioctl(CIOCGSESSION)");
        return 1;
    }
#ifdef CIOCGSESSINFO
    siop2.ses = sess2.ses;
    if (ioctl(cfd, CIOCGSESSINFO, &siop2))
    {
        perror("ioctl(CIOCGSESSINFO)");
        return 1;
    }
    plaintext2 = (char *)(((unsigned long)plaintext2_raw + siop2.alignmask) & ~siop2.alignmask);
#else
    plaintext2 = plaintext2_raw;
#endif
    memcpy(plaintext2, plaintext2_data, BLOCK_SIZE);

    /* Encrypt data.in to data.encrypted */
    cryp1.ses = sess1.ses;
    cryp1.len = BLOCK_SIZE;
    cryp1.src = plaintext1;
    cryp1.dst = plaintext1;
    cryp1.iv  = iv1;
    cryp1.op  = COP_ENCRYPT;

    DO_OR_DIE(do_async_crypt(cfd, &cryp1), 0);
    if (debug)
        printf("cryp1 written out\n");

    memset(iv2, 0x0, sizeof(iv2));

    /* Encrypt data.in to data.encrypted */
    cryp2.ses = sess2.ses;
    cryp2.len = BLOCK_SIZE;
    cryp2.src = plaintext2;
    cryp2.dst = plaintext2;
    cryp2.iv  = iv2;
    cryp2.op  = COP_ENCRYPT;

    DO_OR_DIE(do_async_crypt(cfd, &cryp2), 0);
    if (debug)
        printf("cryp2 written out\n");

    DO_OR_DIE(do_async_fetch(cfd, &cryp1), 0);
    DO_OR_DIE(do_async_fetch(cfd, &cryp2), 0);
    if (debug)
        printf("cryp1 + cryp2 successfully read\n");

    /* Verify the result */
    if (memcmp(plaintext1, ciphertext1, BLOCK_SIZE) != 0)
    {
        int i;
        fprintf(stderr, "FAIL: Decrypted data are different from the input data.\n");
        printf("plaintext:");
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if ((i % 30) == 0)
                printf("\n");
            printf("%02x ", plaintext1[i]);
        }
        printf("ciphertext:");
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if ((i % 30) == 0)
                printf("\n");
            printf("%02x ", ciphertext1[i]);
        }
        printf("\n");
        return 1;
    }
    else
    {
        if (debug)
            printf("result 1 passed\n");
    }

    /* Test 2 */

    /* Verify the result */
    if (memcmp(plaintext2, ciphertext2, BLOCK_SIZE) != 0)
    {
        int i;
        fprintf(stderr, "FAIL: Decrypted data are different from the input data.\n");
        printf("plaintext:");
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if ((i % 30) == 0)
                printf("\n");
            printf("%02x ", plaintext2[i]);
        }
        printf("ciphertext:");
        for (i = 0; i < BLOCK_SIZE; i++)
        {
            if ((i % 30) == 0)
                printf("\n");
            printf("%02x ", ciphertext2[i]);
        }
        printf("\n");
        return 1;
    }
    else
    {
        if (debug)
            printf("result 2 passed\n");
    }

    if (debug)
        printf("AES Test passed\n");

    /* Finish crypto session */
    if (ioctl(cfd, CIOCFSESSION, &sess1.ses))
    {
        perror("ioctl(CIOCFSESSION)");
        return 1;
    }
    if (ioctl(cfd, CIOCFSESSION, &sess2.ses))
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
