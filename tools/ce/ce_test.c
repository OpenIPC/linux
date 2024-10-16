/*
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>	/*ioctl*/
#include <fcntl.h>		/*open*/
#include <unistd.h>		/*close/read/write*/
#include "sunxi_ce.h"

//#define CONFIG_USE_ION
//#define AES_FUNCTION_TEST
#define AES_TEST_MAX

#include <sys/mman.h>	/*mmap*/
#include "ion.h"

#define DATA_SIZE	(128 * 1024)

#define HEXDUMP_LINE_CHR_CNT 16
static int sunxi_hexdump(const unsigned char *buf, int bytes)
{
    unsigned char line[HEXDUMP_LINE_CHR_CNT] = {0};
    int addr;

    for (addr = 0; addr < bytes; addr += HEXDUMP_LINE_CHR_CNT) {
		int len = MIN(bytes - addr, HEXDUMP_LINE_CHR_CNT), i;
		memcpy(line, buf + addr, len);
		memset(line + len, 0, HEXDUMP_LINE_CHR_CNT - len);

		/* print addr */
		printf("0x%.8X: ", addr);
		/* print hex */
		for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++) {
			if (i < len) {
				printf("%.2X ", line[i]);
			} else {
				printf("   ");
			}
		}
		/* print char */
		printf("|");
		for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++) {
			if (i < len) {
				if (line[i] >= 0x20 && line[i] <= 0x7E) {
					printf("%c", line[i]);
				} else {
					printf(".");
				}
			} else {
				printf(" ");
			}
		}
		printf("|\n");
	}
	return 0;
}

int ase_crypto_test(int fd)
{
	u8 *src_buffer = NULL;
	u8 *dst_buffer = NULL;
	u8 *tmp = NULL;
	u8 key[AES_MIN_KEY_SIZE] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
								0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	u8 iv[AES_IV_LENGTH] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
								0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78};
	crypto_aes_req_ctx_t *aes_ctx = NULL;
	int channel_id = -1;
	int ret = -1;

	printf("####aes encrypt start#######\n");
	printf("app key_addr = %p iv_addr = %p\n", key, iv);

	src_buffer = malloc(DATA_SIZE);
	if (src_buffer == NULL) {
		printf("malloc src_buffer fail\n");
		goto fail;
	}
	memset(src_buffer, 0x88, DATA_SIZE);

	dst_buffer = malloc(DATA_SIZE);
	if (dst_buffer == NULL) {
		printf("malloc dst_buffer fail\n");
		goto fail;
	}
	memset(dst_buffer, 0x0, DATA_SIZE);

	printf("app src_buffer = %p dst_buffer =%p\n", src_buffer, dst_buffer);
	aes_ctx = malloc(sizeof(crypto_aes_req_ctx_t));
	if (aes_ctx == NULL) {
		printf("malloc aes_ctx fail\n");
		goto fail;
	}
	memset(aes_ctx, 0x0, DATA_SIZE);

	aes_ctx->src_buffer = src_buffer;
	aes_ctx->src_length = DATA_SIZE;
	aes_ctx->dst_buffer = dst_buffer;
	aes_ctx->dst_length = DATA_SIZE;
	aes_ctx->key_buffer = key;
	aes_ctx->key_length = AES_MIN_KEY_SIZE;
	aes_ctx->iv_buffer = iv;
	aes_ctx->iv_length = AES_IV_LENGTH;
	aes_ctx->aes_mode = AES_MODE_ECB;
	aes_ctx->dir = AES_DIR_ENCRYPT;

	/*reques channel*/
	ret = ioctl(fd, CE_IOC_REQUEST, &channel_id);
	if (ret) {
		printf("CE_IOC_REQUEST fail\n");
		goto fail;
	}
	aes_ctx->channel_id = channel_id;

	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("CE_IOC_AES_CRYPTO fail\n");
		ret = ioctl(fd, CE_IOC_FREE, &channel_id);
		if (ret)
			printf("CE_IOC_FREE error\n");
		goto fail;
	}
	/*B3 CA 2B B8 34 10 CF 1F F2 4F 9E F4 BA B9 24 27*/
	printf("dst_buffer %p\n", aes_ctx->dst_buffer);
	sunxi_hexdump(aes_ctx->dst_buffer, 16);

	/*#####################################*/
	printf("####aes decrypt start#####\n");
	tmp = malloc(DATA_SIZE);
	if (tmp == NULL) {
		printf("malloc tmp fail\n");
		goto fail;
	}
	memset(tmp, 0x0, DATA_SIZE);
	memcpy(tmp, aes_ctx->src_buffer, aes_ctx->src_length);

	memcpy(aes_ctx->src_buffer, aes_ctx->dst_buffer, aes_ctx->dst_length);
	memset(aes_ctx->dst_buffer, 0x0, aes_ctx->dst_length);

	aes_ctx->src_length = aes_ctx->dst_length;
	aes_ctx->dst_buffer = dst_buffer;
	aes_ctx->dst_length = aes_ctx->src_length;
	aes_ctx->key_buffer = key;
	aes_ctx->key_length = AES_MIN_KEY_SIZE;
	aes_ctx->iv_buffer = iv;
	aes_ctx->iv_length = AES_IV_LENGTH;
	aes_ctx->aes_mode = AES_MODE_CBC;
	aes_ctx->dir = AES_DIR_DECRYPT;

	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("CE_IOC_AES_decrypt fail\n");
		ret = ioctl(fd, CE_IOC_FREE, &channel_id);
		if (ret)
			printf("CE_IOC_FREE error\n");
		goto fail;
	}
	printf("dst_buffer %p\n", aes_ctx->dst_buffer);
	sunxi_hexdump(aes_ctx->dst_buffer, 16);

	ret = memcmp(tmp, aes_ctx->dst_buffer, aes_ctx->dst_length);
	if (ret) {
		printf("AES encrypt and decrypt memcmp error\n");
		goto fail;
	}

	free(src_buffer);
	free(dst_buffer);
	free(aes_ctx);
	free(tmp);
	return 0;

fail:
	if (src_buffer)
		free(src_buffer);
	if (dst_buffer)
		free(dst_buffer);
	if (aes_ctx)
		free(aes_ctx);
	if (tmp)
		free(tmp);
	return -1;
}

int aes_test_max(int fd)
{
	int ret = -1;
	struct sunxi_cache_range cache_range;
	u32 data_size = DATA_LEN;
	u8 *data = NULL;
	u8 key[16] = {
	0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
	0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
	};
	u8 iv[16] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	crypto_aes_req_ctx_t *aes_ctx = NULL;
	u8 *tmp = NULL;
	int orign_len = 0;
	int channel_id = -1;
	u32 cmp_size, offset, i;

	printf ("data_size = 0x%x\n", data_size);
	aes_ctx = (crypto_aes_req_ctx_t *)malloc(sizeof(crypto_aes_req_ctx_t));
	if (aes_ctx == NULL) {
		printf (" malloc data buffer fail\n");
		return -1;
	}
	memset(aes_ctx, 0x0, sizeof(crypto_aes_req_ctx_t));
	aes_ctx->iv_buffer = iv;
	aes_ctx->iv_length = 16;
	aes_ctx->key_buffer = key;
	aes_ctx->key_length = sizeof(key);
	aes_ctx->aes_mode = AES_MODE_CBC;
	aes_ctx->dir = AES_DIR_ENCRYPT;

#ifdef CONFIG_USE_ION
	aes_ctx->ion_flag = 1;
	aes_ctx->src_phy = ion_buf.phys_data.phys_addr;
	aes_ctx->src_buffer = ion_buf.vir_addr;
	aes_ctx->src_length = data_size;
	aes_ctx->dst_phy = ion_buf.phys_data.phys_addr + data_size;
	aes_ctx->dst_buffer = ion_buf.vir_addr + data_size;
	aes_ctx->dst_length = data_size;
	printf("src_phy = 0x%lx dst_phy = 0x%lx\n", aes_ctx->src_phy, aes_ctx->dst_phy);
#else
	/*malloc dst buf*/
	data = (u8 *)malloc(data_size);
	if (data == NULL) {
		printf ("%s: malloc dest buffer fail\n", __func__);
		ret = -1;
		goto fail;
	}
	aes_ctx->dst_buffer = data;
	aes_ctx->dst_length = data_size;

	/*malloc src buf*/
	data = (u8 *)malloc(data_size);
	if (data == NULL) {
		printf (" malloc data buffer fail\n");
		ret = -1;
		goto fail;
	}
	memset(data, 0x78, data_size);
	aes_ctx->src_buffer = data;
	aes_ctx->src_length = data_size;
#endif

	/*backup src_data*/
	tmp = (u8 *)malloc(data_size);
	if (tmp == NULL) {
		printf (" malloc data buffer fail\n");
		ret = -1;
		goto fail;
	}
	memset(tmp, 0x0, data_size);
	memcpy(tmp, aes_ctx->src_buffer, data_size);
	orign_len = aes_ctx->src_length;

	/*reques channel*/
	ret = ioctl(fd, CE_IOC_REQUEST, &channel_id);
	if (ret) {
		printf("%s: CE_IOC_REQUEST fail\n", __func__);
		goto fail;
	}
	aes_ctx->channel_id = channel_id;

	/*encrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: encrypt fail\n", __func__);
		goto fail;
	}

	/*change dst to src*/
	memcpy(aes_ctx->src_buffer, aes_ctx->dst_buffer, aes_ctx->dst_length);
	memset(aes_ctx->dst_buffer, 0x0, aes_ctx->dst_length);

	if (aes_ctx->ion_flag) {
		/* flush cache */
		cache_range.start = (long)ion_buf.vir_addr;
		cache_range.end = (long)(ion_buf.vir_addr + ion_buf.buf_len - 1);
		ret = ioctl(ion_buf.ion_fd, ION_IOC_SUNXI_FLUSH_RANGE, &cache_range);
		if (ret) {
			printf("before aes decrypto,flush cache error\n");
		}
	}
	printf ("orign_len = 0x%x\n", aes_ctx->dst_length);
	aes_ctx->dir = AES_DIR_DECRYPT;
	aes_ctx->src_length = aes_ctx->dst_length;

	/*decrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: decrypt fail\n", __func__);
		goto fail;
	}

	cmp_size = 4096;
	for (i = 0x0; i < 256; i++) {
		offset = cmp_size * i;
		if (memcmp(tmp + offset, aes_ctx->dst_buffer + offset, cmp_size)) {
			printf (" aes cbc memcmp fail (%d)\n", i);
			printf("src: \n");
			sunxi_hexdump(tmp + offset, cmp_size);
			printf("dst: \n");
			sunxi_hexdump(aes_ctx->dst_buffer + offset, cmp_size);
			goto fail;
		} else {
			//printf ("############aes_cbc_test pass: %d#############\n\n\n", cmp_size);
		}
	}

	printf ("############aes_test_max end#############\n");
	ret = 0x0;

fail:
	if (ioctl(fd, CE_IOC_FREE, &channel_id) < 0)
		printf("CE_IOC_FREE error\n");
	if (aes_ctx->dst_buffer && (aes_ctx->ion_flag != 1)) {
		printf("dst free\n");
		free(aes_ctx->dst_buffer);
	}
	if (aes_ctx->src_buffer && (aes_ctx->ion_flag != 1)) {
		printf("src free\n");
		free(aes_ctx->src_buffer);
	}
	if (aes_ctx) {
		printf("aes_ctx free\n");
		free(aes_ctx);
	}
	if (tmp) {
		printf("tmp free\n");
		free(tmp);
	}
	return ret;

}

int aes_ecb_test(crypto_aes_req_ctx_t *aes_ctx, int fd)
{
	u8 *tmp = NULL;
	int ret = 0;
	u8 iv[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	int orign_len = 0;
	int channel_id = -1;

	aes_ctx->iv_buffer = iv;
	aes_ctx->aes_mode = AES_MODE_ECB;
	aes_ctx->dir = AES_DIR_ENCRYPT;
	if (aes_ctx->src_length & 0xF) {
		aes_ctx->dst_length = ((aes_ctx->src_length + 15) / 16) * 16;
	} else {
		aes_ctx->dst_length = aes_ctx->src_length;
	}

	printf ("#######aes_ecb_size test %d################\n", aes_ctx->src_length);
	tmp = (u8 *)malloc(256);
	if (tmp == NULL) {
		printf (" malloc data buffer fail\n");
		return -1;
	}
	memset(tmp, 0x0, 256);
	memcpy(tmp, aes_ctx->src_buffer, aes_ctx->src_length);
	orign_len = aes_ctx->src_length;

	/*reques channel*/
	ret = ioctl(fd, CE_IOC_REQUEST, &channel_id);
	if (ret) {
		printf("%s: CE_IOC_REQUEST fail\n", __func__);
		goto fail;
	}
	aes_ctx->channel_id = channel_id;

	/*start encrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: encrypt fail\n", __func__);
		goto fail;
	}

	memcpy(aes_ctx->src_buffer, aes_ctx->dst_buffer, aes_ctx->dst_length);
	memset(aes_ctx->dst_buffer, 0x0, aes_ctx->dst_length);
	aes_ctx->dir = AES_DIR_DECRYPT;
	aes_ctx->src_length = aes_ctx->dst_length;
	/*start decrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: decrypt fail\n", __func__);
		goto fail;
	}

	if (memcmp(tmp, aes_ctx->dst_buffer, orign_len)) {
		printf ("src:\n");
		sunxi_hexdump(tmp, 16);
		printf ("dec:\n");
		sunxi_hexdump(aes_ctx->dst_buffer, 16);
		ret = -1;
		goto fail;
	}
	printf ("############aes_ecb_test pass: %d#############\n\n\n", orign_len);

	ret = 0;

fail:
	if (ioctl(fd, CE_IOC_FREE, &channel_id) < 0)
		printf("CE_IOC_FREE error\n");
	if (tmp)
		free(tmp);
	return ret;
}

int aes_cbc_test(crypto_aes_req_ctx_t *aes_ctx, int fd)
{
	u8 *tmp = NULL;
	int ret = 0;
	u8 iv[16] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	int orign_len = 0;
	int channel_id = -1;

	aes_ctx->iv_buffer = iv;
	aes_ctx->iv_length = 16;
	aes_ctx->aes_mode = AES_MODE_CBC;
	aes_ctx->dir = AES_DIR_ENCRYPT;
	if (aes_ctx->src_length & 0xF) {
		aes_ctx->dst_length = ((aes_ctx->src_length + 15) / 16) * 16;
	} else {
		aes_ctx->dst_length = aes_ctx->src_length;
	}

	printf ("#######aes_cbc_size test %d################\n", aes_ctx->src_length);
	tmp = (u8 *)malloc(256);
	if (tmp == NULL) {
		printf (" malloc data buffer fail\n");
		return -1;
	}
	memset(tmp, 0x0, 256);
	memcpy(tmp, aes_ctx->src_buffer, aes_ctx->src_length);
	orign_len = aes_ctx->src_length;

	/*reques channel*/
	ret = ioctl(fd, CE_IOC_REQUEST, &channel_id);
	if (ret) {
		printf("%s: CE_IOC_REQUEST fail\n", __func__);
		goto fail;
	}
	aes_ctx->channel_id = channel_id;

	/*start encrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: encrypt fail\n", __func__);
		goto fail;
	}

	aes_ctx->iv_buffer = iv;
	memcpy(aes_ctx->src_buffer, aes_ctx->dst_buffer, aes_ctx->dst_length);
	memset(aes_ctx->dst_buffer, 0x0, aes_ctx->dst_length);
	aes_ctx->dir = AES_DIR_DECRYPT;
	aes_ctx->src_length = aes_ctx->dst_length;
	/*start decrypt*/
	ret = ioctl(fd, CE_IOC_AES_CRYPTO, (unsigned long)aes_ctx);
	if (ret) {
		printf("%s: decrypt fail\n", __func__);
		goto fail;
	}

	if (memcmp(tmp, aes_ctx->dst_buffer, orign_len)) {
		printf (" aes cbc memcmp fail\n");
		printf ("src:\n");
		sunxi_hexdump(tmp, 16);
		printf ("dec:\n");
		sunxi_hexdump(aes_ctx->dst_buffer, 16);
		ret = -1;
		goto fail;
	}

	printf ("############aes_cbc_test pass: %d#############\n\n\n", orign_len);

	ret = 0;

fail:
	if (ioctl(fd, CE_IOC_FREE, &channel_id) < 0)
		printf("CE_IOC_FREE error\n");
	if (tmp)
		free(tmp);
	return ret;
}

int aes_test(int fd)
{
	int ret = -1;
	int i = 0;
	u32 data_size = 256;
	u32 size_test[4] = {13, 16, 223, 256};
	//u32 size_test[2] = {13, 223};
	//u32 algin_size = 0;
	u8 *data = NULL;
	u8 key[16] = {
	0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
	0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
	};
	crypto_aes_req_ctx_t *aes_ctx = NULL;

	aes_ctx = (crypto_aes_req_ctx_t *)malloc(sizeof(crypto_aes_req_ctx_t));
	if (aes_ctx == NULL) {
		printf (" malloc data buffer fail\n");
		return -1;
	}
	memset(aes_ctx, 0x0, sizeof(crypto_aes_req_ctx_t));

	aes_ctx->ion_flag = 0;
	aes_ctx->key_buffer = key;
	aes_ctx->key_length = sizeof(key);

	/*malloc dest buf*/
	data = (u8 *)malloc(data_size);
	if (data == NULL) {
		printf (" malloc dest buffer fail\n");
		ret = -1;
		goto out;
	}
	aes_ctx->dst_buffer = data;
	aes_ctx->dst_length = data_size;

	/*malloc src buf*/
	data = (u8 *)malloc(data_size);
	if (data == NULL) {
		printf (" malloc data buffer fail\n");
		ret = -1;
		goto out;
	}
	memset(data, 0x78, data_size);
	aes_ctx->src_buffer = data;
	aes_ctx->src_length = data_size;

	for (i = 0; i < 4; i++) {
		aes_ctx->src_length = size_test[i];
		if (aes_ecb_test(aes_ctx, fd) < 0) {
			printf("aes_ecb_test fail\n");
			ret = -1;
			goto out;
		}
	}

	for (i = 0; i < 4; i++) {
		aes_ctx->src_length = size_test[i];
		if (aes_cbc_test(aes_ctx, fd) < 0) {
			printf("aes_cbc_test fail\n");
			ret = -1;
			goto out;
		}
	}
	ret = 0;

out:
	if (aes_ctx->dst_buffer) {
		free(aes_ctx->dst_buffer);
	}
	if (aes_ctx->src_buffer) {
		free(aes_ctx->src_buffer);
	}
	if (aes_ctx) {
		free(aes_ctx);
	}
	return ret;
}

int ce_test(void)
{
    int ce_fd, ret;

	/*get ce fd*/
	ce_fd = open("/dev/ce", O_RDWR);
    if (ce_fd < 0) {
		printf("open /dev/ce error");
		return -1;
    }
	printf("open successful ,ce_fd = %d\n", ce_fd);

#ifdef AES_FUNCTION_TEST
	/*进行AES算法测试*/
	ret = aes_test(ce_fd);
	if (ret < 0) {
		printf("aes_test fail\n");
		goto fail;
	}
#endif

#ifdef AES_TEST_MAX
	ret = aes_test_max(ce_fd);
	if (ret < 0) {
		printf("aes_test_max fail\n");
		goto fail;
	}
#endif

	close(ce_fd);
    return 0;

fail:
	close(ce_fd);
    return ret;
}

int ion_memery_mode_test(void)
{
	struct ion_allocation_data alloc_data;
	struct ion_handle_data handle_data;
	struct ion_fd_data fd_data;
	struct ion_custom_data custom;
	struct sunxi_cache_range cache_range;
	u32 buf_size = ION_BUF_LEN;
	int ion_fd;
	int ret = 0;
	int i = 0;

	/*创建ion_client*/
	ion_fd = open("/dev/ion", O_RDONLY);
	if (ion_fd < 0) {
		printf("open device error!\n");
		ret = -1;
		return ret;
	}
	memset((void *)(&alloc_data), 0x0, sizeof(struct ion_allocation_data));

	printf("buf_size = 0x%x\n", buf_size);
	/*申请ION内存*/
	alloc_data.len = buf_size;
	alloc_data.align = 0;
	alloc_data.heap_id_mask = ION_HEAP_TYPE_DMA_MASK;	//使用cma堆
	ret = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);	//分配ion内存
	if (ret) {
		printf("ion alloc error!\n");
		goto out1;
	}
	ion_buf.buf_len = alloc_data.len;
	ion_buf.ion_fd = ion_fd;

	/*get dmabuf fd*/
	fd_data.handle = alloc_data.handle;
	ret = ioctl(ion_fd, ION_IOC_MAP, &fd_data);
	if (ret) {
		printf("ion map error\n");
		goto out2;
	}

	/*mmap to user space*/
	ion_buf.vir_addr = (u8 *)mmap(NULL, alloc_data.len, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd_data.fd, 0);
	if (MAP_FAILED == ion_buf.vir_addr) {
		printf("mmap to user space error!\n");
		goto out3;
	}

	//buffer.len = alloc_data.len;

	/*get phy addr*/
	ion_buf.phys_data.handle = alloc_data.handle;
	custom.cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom.arg = (unsigned long)&(ion_buf.phys_data);
	ret = ioctl(ion_fd, ION_IOC_CUSTOM, &custom);
	if (ret) {
		printf("ion get phy addr error!\n");
		goto out3;
	}

	/* fill some random data to buffer */
	for (i = 0; i < DATA_LEN; i++) {
		ion_buf.vir_addr[i] = random() % 256;
	}
	memset((ion_buf.vir_addr + DATA_LEN), 0x0, (buf_size - DATA_LEN));

	/* flush cache */
	cache_range.start = (long)ion_buf.vir_addr;
	cache_range.end = (long)(ion_buf.vir_addr + ion_buf.buf_len - 1);

	ret = ioctl(ion_fd, ION_IOC_SUNXI_FLUSH_RANGE, &cache_range);
	if (ret) {
		printf("before dma_test(),flush cache error\n");
	}

	/*ce_test*/
	ce_test();
	printf("ce_test end\n");
	munmap(ion_buf.vir_addr, alloc_data.len);

out3:
	close(fd_data.fd);
out2:
	handle_data.handle = alloc_data.handle;
	ret = ioctl(ion_fd, ION_IOC_FREE, &handle_data);
	if (ret)
		printf("ion free buffer error!\n");
out1:
	close(ion_fd);
	return ret;
}

int main(int argc, const char *argv[])
{
	int ret;

#ifdef CONFIG_USE_ION
	ret = ion_memery_mode_test();
	if (ret) {
		printf("ion_memery_mode_test\n");
		return -1;
	}
#else
	ret = ce_test();
	if (ret) {
		printf("ce_test fail\n");
		return -1;
	}
#endif
	return 0;
}
