/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_CIPHER_PROC_H_
#define _AX_CIPHER_PROC_H_

typedef enum {
	AX_CIPHER_PROC_TASK_START = 0,
	AX_CIPHER_PROC_TASK_END = 1,
	AX_CIPHER_PROC_TASK_ERROR = 2
} AX_CIPHER_PROC_TASK_STATUS_E;

#define AX_CIPHER_PROC_RSA_PSS_SIGN (0)
#define AX_CIPHER_PROC_RSA_PKCS1_SIGN (1)

#define AX_CIPHER_PROC_ALGO_3DES_OFFSET (14)
#define AX_CIPHER_PROC_ALGO_TRNG_OFFSET (15)
#define AX_CIPHER_PROC_ALGO_RSA_SIGN_OFFSET (16)

int ax_cipher_proc_int(void);
void ax_cipher_proc_deinit(void);
int ax_cipher_get_proc_class1_index(uint8_t algo, uint8_t mode, uint8_t encrypt, uint8_t key_len);
int ax_cipher_get_proc_class2_index(uint8_t algo, uint8_t sign_type, uint8_t hash_type, uint8_t key_len);
int ax_cipher_proc_update(int index, uint32_t bw, AX_CIPHER_PROC_TASK_STATUS_E status);
uint64_t ax_cipher_proc_get_time_us(void);
uint32_t ax_cipher_cal_bw(uint32_t data_len, uint64_t period_us);

#endif
