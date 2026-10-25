// SPDX-License-Identifier: GPL-2.0
/*
 * Engine parameter block and its conversion to DMU/TAE registers. Address
 * fields are in 1-KiB DDR units. Geometry, reference slots, DMA storage and
 * frame flags must be filled in by the caller before the block is used.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/unaligned.h>
#include "fh8626-pae-params.h"

void fh8626_pae_params_init(struct fh8626_pae_params *params)
{
	u8 *p = params->bytes;

	memset(params, 0, sizeof(*params));
	p[0] = 77; /* Main profile */
	p[1] = 32; /* Level 3.2 */
	p[2] = p[3] = p[9] = 1; /* 4:2:0, one reference, CABAC */
	p[0x0b] = 10;
	p[0x0c] = p[0x0d] = 28;
	memset(p + 0x3c, 0x40, 3);
	p[0x41] = 4;
	p[0x43] = 0xc0;
	p[0x44] = 0x3f;
	p[0x48] = 0x22;
	p[0x49] = p[0x4a] = p[0x4b] = 0x1b;
	p[0x4c] = 0xb0;
	put_unaligned_le16(10, p + 0x74);
	put_unaligned_le16(500, p + 0x76);
	put_unaligned_le16(16, p + 0x78);
	put_unaligned_le16(300, p + 0x7a);
	p[0x7c] = 1;
	p[0x7d] = 6;
	p[0x7e] = 0x11;
	p[0x7f] = 0x19;
	put_unaligned_le32(0x001d1ddb, p + 0x80);
	put_unaligned_le16(0x4be, p + 0x84);
	put_unaligned_le16(0xecd, p + 0x86);
	p[0x88] = p[0x8a] = 1;
	put_unaligned_le16(0xffff, p + 0x8c);
	p[0x94] = 4;
	p[0x95] = 0x16;
	p[0x96] = 0x12;
	put_unaligned_le16(0xc80, p + 0x9c);
	put_unaligned_le16(150, p + 0xa0);
	put_unaligned_le16(10, p + 0xa4);
	memset(p + 0xac, 0x40, 7);
	memset(p + 0xb4, 0x40, 7);
	memset(p + 0xbc, 0x40, 4);
	put_unaligned_le32(20, p + 0xc8);
	put_unaligned_le32(0xffff, p + 0xcc);
	memset(p + 0xd0, 1, 4);
	memset(p + 0xd4, 0x40, 24);
	p[0xec] = p[0xed] = 0x11;
	put_unaligned_le16(5, p + 0x128);
	p[0x150] = 10; /* stream chunk shift */
	put_unaligned_le16(64, p + 0x194);
	put_unaligned_le16(16, p + 0x196);
	put_unaligned_le16(64, p + 0x198);
}

static u32 word(const u8 *p, unsigned int offset)
{
	return get_unaligned_le32(p + offset);
}

static u32 half(const u8 *p, unsigned int offset)
{
	return get_unaligned_le16(p + offset);
}

void fh8626_pae_params_registers(const struct fh8626_pae_params *params,
				 struct fh8626_pae_registers *regs)
{
	const u8 *p = params->bytes;
	u32 *d = regs->dmu, *t = regs->tae;
	u32 value;
	unsigned int i;
	static const u16 dmu_address_fields[] = {
		0x134, 0x138, 0, 0x140, 0x144, 0x160, 0x164, 0x168,
		0x16c, 0x14c,
	};

	memset(regs, 0, sizeof(*regs));
	d[0] = (((half(p, 4) >> 4) - 1) & 0xfff) |
		((((half(p, 6) >> 4) - 1) & 0xfff) << 12) |
		((p[0x8a] & 1) << 24);
	for (i = 0; i < ARRAY_SIZE(dmu_address_fields); i++)
		if (dmu_address_fields[i])
			d[i + 1] = word(p, dmu_address_fields[i]) & 0x7ffff;
	d[0x0c / 4] = half(p, 4) >> 4;
	d[0x2c / 4] = 0x280000;
	value = word(p, 0x148);
	d[0x30 / 4] = ((2 * value + 1) & 0x3ff) |
		((((value >> 10) * 2) & 0x3ff) << 10) |
		((((value >> 20) * 2) & 0x3ff) << 20);
	for (i = 0; i < 4; i++)
		d[0x34 / 4 + i] = word(p, 0x19c + 4 * i) & 0x7ffff;
	value = word(p, 0x1ac);
	d[0x44 / 4] = ((2 * value + 1) & 0x3ff) |
		((((value >> 10) * 2) & 0x3ff) << 10) |
		((p[0x1af] & 7) << 24);
	d[0x48 / 4] = (word(p, 0x170) & 0x7ffff) | ((u32)(p[0x89] & 1) << 31);
	d[0x4c / 4] = half(p, 0x8c);
	d[0x54 / 4] = d[0x60 / 4] = 1;
	d[0x58 / 4] = word(p, 0x90);

	t[0x04 / 4] = (half(p, 4) - 1) | ((half(p, 6) - 1) << 16);
	value = (p[0x40] & 0xe1) | ((p[8] & 1) << 4) |
		((p[0x41] & 15) << 8);
	/*
	 * Loop filter: disable_deblocking_filter_idc and the alpha/C0 and beta
	 * offsets (div2, 4-bit two's complement). Without explicit control they
	 * stay zero, the H.264 default of filtering every edge with no offset.
	 */
	if (p[0x10] == 1)
		value |= ((p[0x11] & 3) << 13) | ((p[0x12] & 15) << 15) |
			 ((p[0x13] & 15) << 19);
	/* The packed selector values 1,2,3 map to hardware 3,1,2. */
	if (p[0x43] >> 6)
		value |= (u32)(((p[0x43] >> 6) + 1) % 3 + 1) << 30;
	t[0x08 / 4] = value;
	t[0x0c / 4] = word(p, 0x44);
	t[0x10 / 4] = word(p, 0x48) & 0x3f3f3f55;
	value = word(p, 0x4c);
	t[0x14 / 4] = (value & 15) | (((p[3] - 1) & 15) << 4) |
		((value & 0x7f0) << 4) | ((value & 0x1800) << 4) |
		((value & 0x1fe000) << 4) | ((value & 0x3e00000) << 4);
	t[0x18 / 4] = word(p, 0x94) & 0x7f7fffe7;
	t[0x1c / 4] = (word(p, 0x98) & 0x000ff1ff) |
		((p[0x9d] & 0x0e) << 8) | 0xcc000000;
	t[0x20 / 4] = (word(p, 0x9c) & 0x000ff1ff) |
		((p[0x9d] & 0x0e) << 8);
	for (i = 0; i < 3; i++)
		t[0x24 / 4 + i] = half(p, 0xa0 + 4 * i);
	t[0x30 / 4] = word(p, 0xac);
	t[0x34 / 4] = word(p, 0xb0) & 0x00ffffff;
	t[0x38 / 4] = word(p, 0xb4);
	t[0x3c / 4] = word(p, 0xb8) & 0x00ffffff;
	t[0x40 / 4] = word(p, 0xbc);
	t[0x44 / 4] = word(p, 0x3c) & 0x00ffffff;
	for (i = 0; i < 6; i++)
		t[0x4c / 4 + i] = word(p, 0xd4 + 4 * i);
	t[0x64 / 4] = half(p, 0xec);
	for (i = 0; i < 9; i++)
		t[0x68 / 4 + i] = word(p, 0x50 + 4 * i) & 0x003fffff;
	t[0x8c / 4] = word(p, 0x74) & 0x0fff0fff;
	t[0x90 / 4] = word(p, 0x78) & 0x0fff0fff;
	t[0x94 / 4] = word(p, 0x7c);
	t[0x98 / 4] = word(p, 0x80) & 0x001fffff;
	t[0x9c / 4] = word(p, 0x84) & 0x0fff0fff;
	t[0xa0 / 4] = ((half(p, 0x194) & 255) << 8) |
		((half(p, 0x198) & 255) << 16) |
		((half(p, 0x196) & 255) << 24);
}
