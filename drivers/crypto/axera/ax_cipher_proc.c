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

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/clock.h>
#include <linux/sched.h>
#include "ax_cipher_proc.h"
#include "ax_cipher_adapt.h"

#define CIPHER_PROC_FILE_NAME "ax_proc/cipher"
#define CIPHER_PROC_ITEM_NUM (100)

static struct proc_dir_entry *g_cipher_proc_file;

typedef struct {
	uint8_t algo;
	uint8_t mode;
	uint8_t encrypt;
	uint8_t key_len;
} algo_class1_t;

typedef struct {
	uint8_t algo;
	uint8_t sign_type;
	uint8_t hash_type;
	uint8_t key_len;
} algo_class2_t;

union algo_class_u {
	algo_class1_t class1;
	algo_class2_t class2;
	uint32_t value;
};

typedef struct {
	union algo_class_u class;
	uint32_t min_bw;
	uint32_t max_bw;
	uint32_t avg_bw;
	uint32_t completed;
	uint32_t running;
	bool valid;
} cipher_info_t;

static cipher_info_t g_cipher_info[CIPHER_PROC_ITEM_NUM];
static struct mutex g_cipher_proc_lock;
static int g_loop_point;

static char *g_algo_name[] = {
	"SHA1           ",
	"SHA224         ",
	"SHA256         ",
	"SHA384         ",
	"SHA512         ",
	"HMAC_SHA1      ",
	"HMAC_SHA224    ",
	"HMAC_SHA256    ",
	"HMAC_SHA384    ",
	"HMAC_SHA512    ",
	"MAC_AES_CMAC   ",
	"MAC_AES_CBC_MAC",
	"AES            ",
	"DES            ",
	"3DES           ",
	"TRNG           ",
	"RSA_SIGN",
	"RSA_VERI"
};

static char *g_mode_name[] = {
	"ECB",
	"CBC",
	"CTR",
	"ICM",
	"F8 ",
	"CCM",
	"XTS",
	"GCM",
	"-  "
};

static char *g_ras_sign_mode_name[] = {
	"PKCS1_V15_SHA1  ",
	"PKCS1_V15_SHA224",
	"PKCS1_V15_SHA256",
	"PKCS1_V15_SHA384",
	"PKCS1_V15_SHA512",
	"PKCS1_PSS_SHA1  ",
	"PKCS1_PSS_SHA224",
	"PKCS1_PSS_SHA256",
	"PKCS1_PSS_SHA384",
	"PKCS1_PSS_SHA512"
};

static int ax_cipher_proc_show(struct seq_file *m, void *v)
{
	int i;
	seq_printf(m, "\t\t\t\t\t --------AES\\DES\\3DES\\HASH\\TRNG INFO-------- \n\n");
	seq_printf(m,
			"ALGO\t\tMODE\tENCRYPT\tKEY_LEN(B)\tMIN_BW(MB\\s)\tAVG_BW(MB\\s)\tMAX_BW(MB\\s)\tCOMPLETED\tRUNNING\n");
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (g_cipher_info[i].valid) {
			if (g_cipher_info[i].class.class1.algo <= AX_CIPHER_PROC_ALGO_TRNG_OFFSET) {
				seq_printf(m,
						"%s\t%s\t%d\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\n",
						g_algo_name[g_cipher_info[i].class.class1.algo],
						g_mode_name[g_cipher_info[i].class.class1.mode],
						g_cipher_info[i].class.class1.encrypt,
						g_cipher_info[i].class.class1.key_len,
						g_cipher_info[i].min_bw,
						g_cipher_info[i].avg_bw,
						g_cipher_info[i].max_bw,
						g_cipher_info[i].completed,
						g_cipher_info[i].running);
			}
		}
	}
	seq_printf(m, "\n\n");
	seq_printf(m, " ----------------------------------------------RSA INFO-----------------------------------------------\n");
	seq_printf(m,
			"ALGO\t\t\t\t\tKEY_LEN(B)\tMIN_TIME(us)\tAVG_TIME(us)\tMAX_TIME(us)\tCOMPLETED\tRUNNING\n");
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (g_cipher_info[i].valid) {
			if (g_cipher_info[i].class.class2.algo > AX_CIPHER_PROC_ALGO_TRNG_OFFSET) {
				int rsa_sign_mode = 0;
				if (g_cipher_info[i].class.class2.sign_type == AX_CIPHER_PROC_RSA_PSS_SIGN) {
					if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA1) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA1;
					} else if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA224) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA224;
					} else if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA256) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_PSS_SHA256;
					}
				} else {
					if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA1) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA1;
					} else if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA224) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA224;
					} else if (g_cipher_info[i].class.class2.hash_type == AX_CIPHER_ALGO_HASH_SHA256) {
						rsa_sign_mode = AX_CIPHER_RSA_SIGN_RSASSA_PKCS1_V15_SHA256;
					}
				}
				seq_printf(m,
						"%s_%s\t%10d\t%10d\t%10d\t%10d\t%10d\t%10d\t\n",
						g_algo_name[g_cipher_info[i].class.class2.algo],
						g_ras_sign_mode_name[rsa_sign_mode],
						(uint32_t)g_cipher_info[i].class.class2.key_len * 4,
						g_cipher_info[i].min_bw,
						g_cipher_info[i].avg_bw,
						g_cipher_info[i].max_bw,
						g_cipher_info[i].completed,
						g_cipher_info[i].running);
			}
		}
	}
	return 0;
}

static int ax_cipher_proc_open(struct inode *node, struct file *file)
{
	return single_open(file, ax_cipher_proc_show, NULL);
}

static struct file_operations ax_cipher_proc_fsops = {
	.open = ax_cipher_proc_open,
	.read = seq_read,
	.release = single_release,
	.llseek = seq_lseek,
};

static int ax_cipher_create_proc_file(void)
{
	g_cipher_proc_file = proc_create_data(CIPHER_PROC_FILE_NAME, 0644, NULL, &ax_cipher_proc_fsops, NULL);
	if (g_cipher_proc_file == NULL) {
		CE_LOG_PR(CE_ERR_LEVEL, "create proc entry %s failed\n", CIPHER_PROC_FILE_NAME);
		return -1;
	}
	return 0;
}

static void ax_cipher_remove_proc_entry(void)
{
	remove_proc_entry(CIPHER_PROC_FILE_NAME, NULL);
}

int ax_cipher_get_proc_class1_index(uint8_t algo, uint8_t mode, uint8_t encrypt, uint8_t key_len)
{
	union algo_class_u class;
	int i;
	class.class1.algo = algo;
	class.class1.mode = mode;
	class.class1.encrypt = encrypt;
	class.class1.key_len = key_len;
	ax_mutex_lock(&g_cipher_proc_lock);
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (g_cipher_info[i].valid) {
			if (g_cipher_info[i].class.value == class.value) {
				ax_mutex_unlock(&g_cipher_proc_lock);
				return i;
			}
		}
	}
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (!g_cipher_info[i].valid) {
			memset(&g_cipher_info[i], 0, sizeof(cipher_info_t));
			g_cipher_info[i].class.class1.algo = algo;
			g_cipher_info[i].class.class1.mode = mode;
			g_cipher_info[i].class.class1.encrypt = encrypt;
			g_cipher_info[i].class.class1.key_len = key_len;
			g_cipher_info[i].valid = true;
			ax_mutex_unlock(&g_cipher_proc_lock);
			return i;
		}
	}
	if (i == CIPHER_PROC_ITEM_NUM) {
		int index = g_loop_point;
		memset(&g_cipher_info[index], 0, sizeof(cipher_info_t));
		g_cipher_info[index].class.class1.algo = algo;
		g_cipher_info[index].class.class1.mode = mode;
		g_cipher_info[index].class.class1.encrypt = encrypt;
		g_cipher_info[index].class.class1.key_len = key_len;
		g_cipher_info[index].valid = true;
		g_loop_point++;
		if (g_loop_point == CIPHER_PROC_ITEM_NUM) {
			g_loop_point = 0;
		}
		ax_mutex_unlock(&g_cipher_proc_lock);
		return index;
	}
	ax_mutex_unlock(&g_cipher_proc_lock);
	return -1;
}

int ax_cipher_get_proc_class2_index(uint8_t algo, uint8_t sign_type, uint8_t hash_type, uint8_t key_len)
{
	union algo_class_u class;
	int i;
	class.class2.algo = algo;
	class.class2.sign_type = sign_type;
	class.class2.hash_type = hash_type;
	class.class2.key_len = key_len;
	ax_mutex_lock(&g_cipher_proc_lock);
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (g_cipher_info[i].valid) {
			if (g_cipher_info[i].class.value == class.value) {
				ax_mutex_unlock(&g_cipher_proc_lock);
				return i;
			}
		}
	}
	for (i = 0; i < CIPHER_PROC_ITEM_NUM; i++) {
		if (!g_cipher_info[i].valid) {
			memset(&g_cipher_info[i], 0, sizeof(cipher_info_t));
			g_cipher_info[i].class.class2.algo = algo;
			g_cipher_info[i].class.class2.sign_type = sign_type;
			g_cipher_info[i].class.class2.hash_type = hash_type;
			g_cipher_info[i].class.class2.key_len = key_len;
			g_cipher_info[i].valid = true;
			ax_mutex_unlock(&g_cipher_proc_lock);
			return i;
		}
	}
	if (i == CIPHER_PROC_ITEM_NUM) {
		int index = g_loop_point;
		memset(&g_cipher_info[index], 0, sizeof(cipher_info_t));
		g_cipher_info[index].class.class2.algo = algo;
		g_cipher_info[index].class.class2.sign_type = sign_type;
		g_cipher_info[index].class.class2.hash_type = hash_type;
		g_cipher_info[index].class.class2.key_len = key_len;
		g_cipher_info[index].valid = true;
		g_loop_point++;
		if (g_loop_point == CIPHER_PROC_ITEM_NUM) {
			g_loop_point = 0;
		}
		ax_mutex_unlock(&g_cipher_proc_lock);
		return index;
	}
	ax_mutex_unlock(&g_cipher_proc_lock);
	return -1;
}

uint32_t ax_cipher_cal_bw(uint32_t data_len, uint64_t period_us)
{
	unsigned long len = data_len;
	uint32_t bw = (uint32_t)(len / (unsigned long)period_us);
	return bw;
}

int ax_cipher_proc_update(int index, uint32_t bw, AX_CIPHER_PROC_TASK_STATUS_E status)
{
	cipher_info_t *cipher_info;
	if (index >= CIPHER_PROC_ITEM_NUM || index < 0) {
		CE_LOG_PR(CE_ERR_LEVEL, "index(%d) error\n", index);
		return -1;
	}
	ax_mutex_lock(&g_cipher_proc_lock);
	if (!g_cipher_info[index].valid) {
		ax_mutex_unlock(&g_cipher_proc_lock);
		CE_LOG_PR(CE_ERR_LEVEL, "index(%d) error\n", index);
		return -1;
	}
	cipher_info = &g_cipher_info[index];
	if (status == AX_CIPHER_PROC_TASK_START) {
		cipher_info->running++;
	} else if (status == AX_CIPHER_PROC_TASK_END) {
		if (cipher_info->completed == 0) {
			cipher_info->min_bw = bw;
			cipher_info->avg_bw = bw;
			cipher_info->max_bw = bw;
		} else {
			cipher_info->avg_bw = (cipher_info->avg_bw * cipher_info->completed + bw) / (cipher_info->completed + 1);
			if (bw < cipher_info->min_bw) {
				cipher_info->min_bw = bw;
			}
			if (bw > cipher_info->max_bw) {
				cipher_info->max_bw = bw;
			}
		}
		cipher_info->completed++;
		cipher_info->running--;
	} else {
		cipher_info->running--;
	}
	ax_mutex_unlock(&g_cipher_proc_lock);
	return 0;
}

uint64_t ax_cipher_proc_get_time_us(void)
{
	return div_u64(sched_clock(), 1000);
}

int ax_cipher_proc_int(void)
{
	int ret;
	memset(g_cipher_info, 0, sizeof(g_cipher_info));
	g_loop_point = 0;
	ret = ax_mutex_init(&g_cipher_proc_lock);
	if (ret != 0) {
		CE_LOG_PR(CE_ERR_LEVEL, "mutex_init failed\n");
		return -1;
	}
	return ax_cipher_create_proc_file();
}

void ax_cipher_proc_deinit(void)
{
	ax_mutex_deinit(&g_cipher_proc_lock);
	ax_cipher_remove_proc_entry();
}
#endif
