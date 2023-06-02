/*
 * if_alg: User-space algorithm interface
 *
 * Copyright (c) 2010 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#ifndef _LINUX_IF_ALG_H
#define _LINUX_IF_ALG_H

#include <linux/types.h>

struct sockaddr_alg {
	__u16	salg_family;
	__u8	salg_type[14];
	__u32	salg_feat;
	__u32	salg_mask;
	__u8	salg_name[64];
};

struct af_alg_iv {
	__u32	ivlen;
	__u8	iv[0];
};

#define MAX_EX_KEY_MAP_SIZE              8
struct ex_key_map {
	__u32 crypto_key_no;
	__u32 ex_mem_entry;
};

struct ex_key_map_para {
	__u32 map_size;
	struct ex_key_map map[MAX_EX_KEY_MAP_SIZE];
};

struct crypto_adv_info {
	struct ex_key_map_para ex_key_para;
};

struct af_alg_usr_def {
#define CRYPTO_CPU_SET_KEY                      (1<<0)
#define CRYPTO_EX_MEM_SET_KEY                   (1<<1)
#define CRYPTO_EX_MEM_INDEP_POWER               (1<<2)
/*bit 8~ex mem bit field..*/
#define CRYPTO_EX_MEM_SWITCH_KEY                (1<<8)
/*if set ex mem set switch key..then parse below..*/
#define CRYPTO_EX_MEM_4_ENTRY_1_KEY             (1<<9)
	__u32	mode;
/*if key_flag set efuse...then parse the para below...*/
	struct crypto_adv_info adv;
};


/* Socket options */
#define ALG_SET_KEY			1
#define ALG_SET_IV			2
#define ALG_SET_OP			3
#define ALG_SET_AEAD_ASSOCLEN		4
#define ALG_SET_AEAD_AUTHSIZE		5
#define ALG_USR_DEF			8
/* Operations */
#define ALG_OP_DECRYPT			0
#define ALG_OP_ENCRYPT			1

#endif	/* _LINUX_IF_ALG_H */
