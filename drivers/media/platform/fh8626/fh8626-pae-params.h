/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 Difan Zhang
 */
#ifndef FH8626_PAE_PARAMS_H
#define FH8626_PAE_PARAMS_H

#include <linux/types.h>

#define FH8626_PAE_PARAM_BYTES 0x1bc
struct fh8626_pae_params {
	u8 bytes[FH8626_PAE_PARAM_BYTES];
};

struct fh8626_pae_registers {
	u32 dmu[0x64 / 4];
	u32 tae[0xa4 / 4];
};

void fh8626_pae_params_init(struct fh8626_pae_params *params);
void fh8626_pae_params_registers(const struct fh8626_pae_params *params,
				 struct fh8626_pae_registers *regs);

#endif
