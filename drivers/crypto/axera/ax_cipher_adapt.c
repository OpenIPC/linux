/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#if defined(CONFIG_AX_CIPHER_CRYPTO) || defined(CONFIG_AX_CIPHER_MISC)
#include <linux/uaccess.h>
#include <asm/io.h>
#include "ax_cipher_adapt.h"
#include "eip130_drv.h"

void *ax_cipher_reversememcpy(void *dest, void *src, uint32_t size)
{
	uint8_t *dp = dest;
	const uint8_t *sp = src;
	sp += (size - 1);
	while (size--) {
		*dp++ = *sp--;
	}
	return dest;
}
void ax_cipher_convert_endian(uint8_t *buf, int len)
{
	char tmp;
	int i;
	for (i = 0; i < (len / 2); i ++) {
		tmp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = tmp;
	}
}
uint32_t ax_cipher_regread(void * addr)
{
	uint32_t val;
	val = readl_relaxed(addr);
	return val;
}
void ax_cipher_regwrite(uint32_t val, void * addr)
{
	writel_relaxed(val, addr);
}
int ax_copy_to_user(void *dst, void *src, uint32_t size)
{
	if(copy_to_user(dst, src, size)) {
		return -EFAULT;
	}
	return 0;
}
int ax_copy_from_user(void *dst, void *src, uint32_t size)
{
	if(copy_from_user(dst, src, size)) {
		return -EFAULT;
	}
	return 0;
}
int ax_mutex_init(void *mutex)
{
	mutex_init(mutex);
	return 0;
}
int ax_mutex_deinit(void *mutex)
{
	return 0;
}
int ax_mutex_lock(void *mutex)
{
	mutex_lock(mutex);
	return 0;
}
int ax_mutex_unlock(void*mutex)
{
	mutex_unlock(mutex);
	return 0;
}
void ax_cipher_dump(uint32_t *token, int size)
{
	int i = 0;
	for (i = 0 ; i < size / 4; i++) {
		if ((i % 4) == 0) {
			CE_LOG_PR(CE_INFO_LEVEL,"\n");
		}
		CE_LOG_PR(CE_INFO_LEVEL,"%08x ", token[i]);
	}
}
void ax_cipher_dump_bytes(uint8_t *token, int size)
{
	int i = 0;
	CE_LOG_PR(CE_INFO_LEVEL,"dump data begin\n");
	for (i = 0 ; i < size; i++) {
		if ((i % 16) == 0) {
			 CE_LOG_PR(CE_INFO_LEVEL,"dump data end\n");
		}
		CE_LOG_PR(CE_INFO_LEVEL,"dump data begin\n");
	}
	CE_LOG_PR(CE_INFO_LEVEL,"dump data end\n");
}
void dump_token(int *token)
{
	int i = 0;
	CE_LOG_PR(CE_INFO_LEVEL,"dump token begin\n");
	for (i = 0 ; i < 64; i++) {
		if ((i % 4) == 0) {
			 CE_LOG_PR(CE_INFO_LEVEL,"\n");
		}
		CE_LOG_PR(CE_INFO_LEVEL,"%08x ", token[i]);
	}
	CE_LOG_PR(CE_INFO_LEVEL,"dump token end\n");
}
#endif

