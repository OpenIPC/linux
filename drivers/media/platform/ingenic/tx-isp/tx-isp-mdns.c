// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/string.h>

#include "tx-isp-mdns.h"
#include "tx-isp-regs.h"

struct tx_isp_mdns_reg {
	u16 offset;
	u32 value;
};

/* SC1A4T linear day profile at gain-table index 1. */
static const struct tx_isp_mdns_reg mdns_sc1a4t_base[] = {
	{ 0x7870, 0x00000000 },
	{ 0x7888, 0x00000002 },
	{ 0x78a0, 0x00000001 },
	{ 0x78b8, 0x00000003 },
	{ 0x7874, 0x502d1010 },
	{ 0x788c, 0x502d1010 },
	{ 0x78a4, 0x502d1010 },
	{ 0x78bc, 0x502d1010 },
	{ 0x7880, 0x00080000 },
	{ 0x7898, 0x00000008 },
	{ 0x78b0, 0x00080008 },
	{ 0x7884, 0x02c80500 },
	{ 0x789c, 0x02d004f8 },
	{ 0x78b4, 0x02c804f8 },
	{ 0x7878, 0x000014f1 },
	{ 0x7890, 0x000014f1 },
	{ 0x78a8, 0x000014f1 },
	{ 0x78c0, 0x000014f1 },
	{ 0x787c, 0xff190114 },
	{ 0x7894, 0xff190114 },
	{ 0x78ac, 0xff190114 },
	{ 0x78c4, 0xff190114 },
	{ 0x7944, 0xff0c3064 },
	{ 0x7948, 0xc8a07850 },
	{ 0x7958, 0xc8a07850 },
	{ 0x7968, 0xc8a07850 },
	{ 0x7978, 0xc8a07850 },
	{ 0x794c, 0xd7d5d4d3 },
	{ 0x795c, 0xd7d5d4d3 },
	{ 0x796c, 0xd7d5d4d3 },
	{ 0x797c, 0xd7d5d4d3 },
	{ 0x7950, 0xfaf0e6dc },
	{ 0x7960, 0xfaf0e6dc },
	{ 0x7970, 0xfaf0e6dc },
	{ 0x7980, 0xfaf0e6dc },
	{ 0x7954, 0xfafafafa },
	{ 0x7964, 0xfafafafa },
	{ 0x7974, 0xfafafafa },
	{ 0x7984, 0xfafafafa },
	{ 0x7988, 0xc8a07850 },
	{ 0x798c, 0xd7d5d4d3 },
	{ 0x7990, 0xfaf0e6dc },
	{ 0x7994, 0xfafafafa },
	{ 0x7998, 0x00141402 },
	{ 0x799c, 0x00dc14ff },
	{ 0x79a0, 0x000000ff },
	{ 0x78c8, 0x00000000 },
	{ 0x78d4, 0x00070006 },
	{ 0x78d8, 0x00090008 },
	{ 0x78cc, 0x001c0018 },
	{ 0x78d0, 0x00240020 },
	{ 0x78e0, 0x0a0a0a0a },
	{ 0x78e4, 0x00008080 },
	{ 0x78e8, 0x00000100 },
	{ 0x78ec, 0x80808080 },
	{ 0x78f0, 0x80808080 },
	{ 0x78f4, 0x80808080 },
	{ 0x78f8, 0x80808080 },
	{ 0x78fc, 0x00000104 },
	{ 0x7900, 0x80808080 },
	{ 0x7904, 0x80808080 },
	{ 0x7908, 0x80808080 },
	{ 0x790c, 0x80808080 },
	{ 0x7910, 0x00000106 },
	{ 0x7914, 0x80808080 },
	{ 0x7918, 0x80808080 },
	{ 0x791c, 0x80808080 },
	{ 0x7920, 0x80808080 },
	{ 0x7924, 0x00000140 },
	{ 0x7928, 0x80808080 },
	{ 0x792c, 0x80808080 },
	{ 0x7930, 0x80808080 },
	{ 0x7934, 0x80808080 },
	{ 0x7938, 0x00000001 },
	{ 0x793c, 0x80808080 },
	{ 0x7940, 0x80808080 },
	{ 0x79a4, 0x0000000a },
	{ 0x79a8, 0x00000000 },
	{ 0x79ac, 0x0000000a },
	{ 0x79b0, 0x00000000 },
	{ 0x79b4, 0x00000000 },
	{ 0x79b8, 0x64646464 },
	{ 0x79bc, 0x64646464 },
	{ 0x79c0, 0x78646464 },
	{ 0x79c4, 0x000a1e46 },
	{ 0x79c8, 0x64646464 },
	{ 0x79cc, 0x64646464 },
	{ 0x79d0, 0x78646464 },
	{ 0x79d4, 0x000a1e46 },
	{ 0x79d8, 0x0064320a },
	{ 0x79dc, 0x000000dc },
	{ 0x79e0, 0x00000000 },
	{ 0x79e4, 0x0064320a },
	{ 0x79e8, 0x000000dc },
	{ 0x79ec, 0x00000000 },
	{ 0x79f0, 0x82828282 },
	{ 0x79f4, 0x82828282 },
	{ 0x79f8, 0x28324646 },
	{ 0x79fc, 0x00000a14 },
	{ 0x7a00, 0x00004000 },
	{ 0x7a64, 0xffff3141 },
	{ 0x7a68, 0x00141400 },
	{ 0x7a6c, 0x00c80aff },
	{ 0x7a70, 0x000000ff },
	{ 0x7a04, 0x00008080 },
	{ 0x7a08, 0x00000000 },
	{ 0x7a0c, 0x80808080 },
	{ 0x7a10, 0x80808080 },
	{ 0x7a14, 0x80808080 },
	{ 0x7a18, 0x80808080 },
	{ 0x7a1c, 0x00000000 },
	{ 0x7a20, 0x80808080 },
	{ 0x7a24, 0x80808080 },
	{ 0x7a28, 0x80808080 },
	{ 0x7a2c, 0x80808080 },
	{ 0x7a30, 0x00000000 },
	{ 0x7a34, 0x80808080 },
	{ 0x7a38, 0x80808080 },
	{ 0x7a3c, 0x80808080 },
	{ 0x7a40, 0x80808080 },
	{ 0x7a44, 0x00000000 },
	{ 0x7a48, 0x80808080 },
	{ 0x7a4c, 0x80808080 },
	{ 0x7a50, 0x80808080 },
	{ 0x7a54, 0x80808080 },
	{ 0x7a58, 0x00000001 },
	{ 0x7a5c, 0x00808080 },
	{ 0x7a60, 0x00000000 },
	{ 0x7a74, 0x09091480 },
	{ 0x7a78, 0x00000000 },
	{ 0x7a7c, 0x00000000 },
	{ 0x7a80, 0x0017ffa1 },
	{ 0x7a84, 0x00040000 },
	{ 0x7a88, 0x01600280 },
	{ 0x7a8c, 0x00000000 },
	{ 0x7a90, 0xffffffff },
	{ 0x7a94, 0xffffffff },
	{ 0x7a98, 0xffffffff },
	{ 0x7a9c, 0xffffffff },
	{ 0x7aa0, 0xffffffff },
	{ 0x7aa4, 0xffffffff },
	{ 0x7aa8, 0xffffffff },
	{ 0x7aac, 0xffffffff },
	{ 0x7ab0, 0x001e140a },
	{ 0x7ab4, 0x000000c8 },
	{ 0x7ab8, 0x00008080 },
	{ 0x7abc, 0x001e140a },
	{ 0x7ac0, 0x000000c8 },
	{ 0x7ac4, 0x00000001 },
	{ 0x7ac8, 0xffffffff },
	{ 0x7acc, 0xffffffff },
	{ 0x7ad0, 0xffffffff },
	{ 0x7ad4, 0x64ffffff },
	{ 0x7ad8, 0x1e140a80 },
	{ 0x7adc, 0x00000000 },
	{ 0x7ae0, 0x00000000 },
	{ 0x7ae4, 0x00000000 },
	{ 0x7ae8, 0x00000000 },
	{ 0x7aec, 0x00200040 },
};

enum tx_isp_mdns_fields {
	MDNS_BYTES,
	MDNS_MOTION,
	MDNS_STATIC,
	MDNS_SAD,
	MDNS_HISTOGRAM,
};

struct tx_isp_mdns_gain_reg {
	u16 offset;
	u8 fields;
	u32 value[9];
};

/* Entries correspond to log2(total gain) = 0 through 8. */
static const struct tx_isp_mdns_gain_reg mdns_sc1a4t_gain[] = {
	{ 0x7878, MDNS_MOTION, {
		0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1,
		0x000028f1, 0x000028f1, 0x000028f1, 0x000028f1 } },
	{ 0x7890, MDNS_MOTION, {
		0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1,
		0x000028f1, 0x000028f1, 0x000028f1, 0x000028f1 } },
	{ 0x78a8, MDNS_MOTION, {
		0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1,
		0x000028f1, 0x000028f1, 0x000028f1, 0x000028f1 } },
	{ 0x78c0, MDNS_MOTION, {
		0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1, 0x000014f1,
		0x000028f1, 0x000028f1, 0x000028f1, 0x000028f1 } },
	{ 0x787c, MDNS_STATIC, {
		0xff1900f4, 0xff190114, 0xff190134, 0xff190154, 0xff190174,
		0xff190224, 0xff190244, 0xff190244, 0xff190244 } },
	{ 0x7894, MDNS_STATIC, {
		0xff1900f4, 0xff190114, 0xff190134, 0xff190154, 0xff190174,
		0xff190224, 0xff190244, 0xff190244, 0xff190244 } },
	{ 0x78ac, MDNS_STATIC, {
		0xff1900f4, 0xff190114, 0xff190134, 0xff190154, 0xff190174,
		0xff190224, 0xff190244, 0xff190244, 0xff190244 } },
	{ 0x78c4, MDNS_STATIC, {
		0xff1900f4, 0xff190114, 0xff190134, 0xff190154, 0xff190174,
		0xff190224, 0xff190244, 0xff190244, 0xff190244 } },
	{ 0x7944, MDNS_SAD, {
		0xff0a3054, 0xff0c3064, 0xff0e3074, 0xff103084, 0xff123094,
		0xff143124, 0xff143124, 0xff143124, 0xff143124 } },
	{ 0x799c, MDNS_BYTES, {
		0x00dc14ff, 0x00dc14ff, 0x00dc14ff, 0x00e614ff, 0x00e614ff,
		0x00e614ff, 0x00e614ff, 0x00f014ff, 0x00f014ff } },
	{ 0x78c8, MDNS_HISTOGRAM, {
		0x0000001f, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000 } },
	{ 0x78e4, MDNS_BYTES, {
		0x00008080, 0x00008080, 0x00008c8c, 0x00008c8c, 0x00009696,
		0x00009696, 0x0000a0a0, 0x0000a0a0, 0x0000aaaa } },
	{ 0x7940, MDNS_BYTES, {
		0x80808080, 0x80808080, 0x80808080, 0x80808080, 0x80808080,
		0x80808080, 0x80808080, 0x64646464, 0x32323232 } },
	{ 0x79a4, MDNS_BYTES, {
		0x00000008, 0x0000000a, 0x0000000e, 0x00000010, 0x00000012,
		0x00000014, 0x00000016, 0x00000018, 0x0000001a } },
	{ 0x79ac, MDNS_BYTES, {
		0x00000008, 0x0000000a, 0x0000000e, 0x00000010, 0x00000012,
		0x00000014, 0x00000016, 0x00000018, 0x0000001a } },
	{ 0x79b4, MDNS_BYTES, {
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000001, 0x00000001 } },
	{ 0x79b8, MDNS_BYTES, {
		0x64646464, 0x64646464, 0xb4b4b4b4, 0xb4b4b4b4, 0xc8c8c8c8,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } },
	{ 0x79bc, MDNS_BYTES, {
		0x64646464, 0x64646464, 0xb4b4b4b4, 0xb4b4b4b4, 0xc8c8c8c8,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } },
	{ 0x79c0, MDNS_BYTES, {
		0x78646464, 0x78646464, 0x78b4b4b4, 0x78b4b4b4, 0x96c8c8c8,
		0xaaffffff, 0xb4ffffff, 0xc8ffffff, 0xf0ffffff } },
	{ 0x79c4, MDNS_BYTES, {
		0x000a1e46, 0x000a1e46, 0x000a1e46, 0x000a1e46, 0x000a3264,
		0x00285078, 0x00285082, 0x00326496, 0x003264e6 } },
	{ 0x79c8, MDNS_BYTES, {
		0x64646464, 0x64646464, 0xb4b4b4b4, 0xb4b4b4b4, 0xc8c8c8c8,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } },
	{ 0x79cc, MDNS_BYTES, {
		0x64646464, 0x64646464, 0xb4b4b4b4, 0xb4b4b4b4, 0xc8c8c8c8,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } },
	{ 0x79d0, MDNS_BYTES, {
		0x78646464, 0x78646464, 0x78b4b4b4, 0x78b4b4b4, 0x96c8c8c8,
		0xaaffffff, 0xb4ffffff, 0xc8ffffff, 0xf0ffffff } },
	{ 0x79d4, MDNS_BYTES, {
		0x000a1e46, 0x000a1e46, 0x000a1e46, 0x000a1e46, 0x000a3264,
		0x00285078, 0x00285082, 0x00326496, 0x323264e6 } },
	{ 0x79d8, MDNS_BYTES, {
		0x0064320a, 0x0064320a, 0x0064320a, 0x0064320a, 0x0064320a,
		0x0064320a, 0x0064320a, 0x0064320a, 0x00643200 } },
	{ 0x79e4, MDNS_BYTES, {
		0x0064320a, 0x0064320a, 0x0064320a, 0x0064320a, 0x0064320a,
		0x0064320a, 0x0064320a, 0x0064320a, 0x00643200 } },
	{ 0x79f0, MDNS_BYTES, {
		0x82828282, 0x82828282, 0x82828282, 0x82828282, 0x82828282,
		0x82828282, 0xaaaaaaaa, 0xc8c8c8c8, 0xf0f0f0f0 } },
	{ 0x79f4, MDNS_BYTES, {
		0x82828282, 0x82828282, 0x82828282, 0x82828282, 0x82828282,
		0x82828282, 0xaaaaaaaa, 0xc8c8c8c8, 0xf0f0f0f0 } },
	{ 0x79f8, MDNS_BYTES, {
		0x28324646, 0x28324646, 0x28324646, 0x32324646, 0x46464646,
		0x64828c82, 0x78aaaaaa, 0x96c8c8c8, 0xb4dcebe6 } },
	{ 0x79fc, MDNS_BYTES, {
		0x00000a14, 0x00000a14, 0x00000a14, 0x00000a32, 0x00002846,
		0x0000283c, 0x0a0a285a, 0x0a0a3c78, 0x0a0a6496 } },
	{ 0x7a6c, MDNS_BYTES, {
		0x00c80aff, 0x00c80aff, 0x00c80aff, 0x00d20aff, 0x00d20aff,
		0x00d20aff, 0x00dc0aff, 0x00dc0aff, 0x00dc0aff } },
	{ 0x7ab0, MDNS_BYTES, {
		0x001e140a, 0x001e140a, 0x001e140a, 0x001e140a, 0x001e140a,
		0x001e140a, 0x00966450, 0x00966450, 0x00966450 } },
	{ 0x7ab8, MDNS_BYTES, {
		0x00008080, 0x00008080, 0x00008080, 0x00008080, 0x00008080,
		0x00008080, 0x00000404, 0x00000404, 0x00000404 } },
	{ 0x7abc, MDNS_BYTES, {
		0x001e140a, 0x001e140a, 0x001e140a, 0x001e140a, 0x001e140a,
		0x001e140a, 0x00966450, 0x00966450, 0x00966450 } },
	{ 0x7ad4, MDNS_BYTES, {
		0x64ffffff, 0x64ffffff, 0x64ffffff, 0x64ffffff, 0x64ffffff,
		0x64ffffff, 0xffffffff, 0xffffffff, 0xffffffff } },
	{ 0x7ad8, MDNS_BYTES, {
		0x1e140a80, 0x1e140a80, 0x1e140a80, 0x1e140a80, 0x1e140a80,
		0x1e140a80, 0x96645080, 0x96645080, 0x96645080 } },
};

const u16 tx_isp_mdns_address_regs[TX_ISP_MDNS_PLANES] = {
	0x7820, 0x7828, 0x7830, 0x7840, 0x7848,
	0x7850, 0x7858, 0x7860, 0x7868,
};

int tx_isp_mdns_layout(u32 width, u32 height,
		       struct tx_isp_mdns_layout *layout)
{
	struct tx_isp_mdns_layout result = { };
	u32 stride, motion_stride;
	u64 bytes[3], total = 0;
	unsigned int i;

	if (!layout || width < 32 || height < 16 ||
	    width > U16_MAX || height > U16_MAX || ((width | height) & 1))
		return -EINVAL;

	stride = ALIGN(width, 8);
	motion_stride = ALIGN(DIV_ROUND_UP(width, 32), 8);

	bytes[0] = (u64)stride * height;
	bytes[1] = bytes[0] / 2;
	bytes[2] = (u64)(DIV_ROUND_UP(height, 16) + 1) * motion_stride;
	for (i = 0; i < ARRAY_SIZE(bytes); i++) {
		if (bytes[i] > U32_MAX - total)
			return -EOVERFLOW;
		result.plane[i].offset = total;
		result.plane[i].size = bytes[i];
		total += bytes[i];
	}
	result.plane[0].stride = stride;
	result.plane[1].stride = stride;
	result.plane[2].stride = motion_stride;
	/* The SC1A4T profile disables these history planes with zero strides. */
	for (i = 3; i < 6; i++) {
		result.plane[i] = result.plane[2];
		result.plane[i].stride = 0;
	}
	for (i = 6; i < TX_ISP_MDNS_PLANES; i++) {
		result.plane[i] = result.plane[0];
		result.plane[i].stride = 0;
	}
	if (total > U32_MAX - (PAGE_SIZE - 1))
		return -EOVERFLOW;
	result.size = ALIGN(total, PAGE_SIZE);
	*layout = result;
	return 0;
}

int tx_isp_mdns_alloc(struct device *dev, struct tx_isp_mdns_history *history,
		     u32 width, u32 height)
{
	struct tx_isp_mdns_layout layout;
	dma_addr_t dma;
	void *cpu;
	int ret;

	if (!dev || !history)
		return -EINVAL;
	if (history->cpu || history->active)
		return -EBUSY;
	ret = tx_isp_mdns_layout(width, height, &layout);
	if (ret)
		return ret;
	cpu = dma_alloc_coherent(dev, layout.size, &dma, GFP_KERNEL);
	if (!cpu)
		return -ENOMEM;
	if ((u64)dma + layout.size - 1 > U32_MAX || (dma & 7)) {
		dma_free_coherent(dev, layout.size, cpu, dma);
		return -ERANGE;
	}
	memset(cpu, 0, layout.size);
	history->layout = layout;
	history->dev = dev;
	history->dma = dma;
	history->cpu = cpu;
	history->width = width;
	history->height = height;
	/* Cleared storage is not a completed reference frame. */
	history->valid = false;
	return 0;
}

int tx_isp_mdns_free(struct tx_isp_mdns_history *history)
{
	if (!history)
		return -EINVAL;
	if (history->active)
		return -EBUSY;
	if (history->cpu)
		dma_free_coherent(history->dev, history->layout.size,
				  history->cpu, history->dma);
	memset(history, 0, sizeof(*history));
	return 0;
}

int tx_isp_mdns_invalidate(struct tx_isp_mdns_history *history)
{
	if (!history || !history->cpu)
		return -EINVAL;
	if (history->active)
		return -EBUSY;
	history->valid = false;
	memset(history->cpu, 0, history->layout.size);
	return 0;
}

static u32 tx_isp_mdns_interpolate(u32 a, u32 b, u32 fraction)
{
	u32 delta = (a > b ? a - b : b - a) * fraction;

	delta = (delta >> 16) + !!(delta & BIT(15));
	return a > b ? a - delta : a + delta;
}

static u32 tx_isp_mdns_field(u32 a, u32 b, u32 fraction, u32 mask, u32 shift)
{
	return tx_isp_mdns_interpolate((a >> shift) & mask,
				      (b >> shift) & mask, fraction) << shift;
}

static u32 tx_isp_mdns_gain(u16 reg, u32 value, u32 gain_log2_q16)
{
	const struct tx_isp_mdns_gain_reg *entry;
	u32 index = min(gain_log2_q16 >> 16, 8U);
	u32 fraction = gain_log2_q16 & 0xffff;
	u32 a, b, mask, result;
	unsigned int i, j, shift, fields;

	for (i = 0; i < ARRAY_SIZE(mdns_sc1a4t_gain); i++) {
		entry = &mdns_sc1a4t_gain[i];
		if (entry->offset != reg)
			continue;
		a = entry->value[index];
		if (index == 8)
			return a;
		b = entry->value[index + 1];
		if (entry->fields == MDNS_MOTION)
			return (a & ~0xff00) |
			       tx_isp_mdns_field(a, b, fraction, 0xff, 8);
		if (entry->fields == MDNS_STATIC)
			return (a & ~0xfff0) |
			       tx_isp_mdns_field(a, b, fraction, 0xfff, 4);
		if (entry->fields == MDNS_SAD)
			return (a & ~0x00ff0ff8) |
			       tx_isp_mdns_field(a, b, fraction, 0x1ff, 3) |
			       tx_isp_mdns_field(a, b, fraction, 0xff, 16);
		fields = entry->fields == MDNS_HISTOGRAM ? 5 : 4;
		result = 0;
		for (j = 0; j < fields; j++) {
			shift = entry->fields == MDNS_HISTOGRAM ? j : j * 8;
			mask = entry->fields == MDNS_HISTOGRAM ? 1 : 0xff;
			result |= tx_isp_mdns_field(a, b, fraction,
						   mask, shift);
		}
		return result;
	}
	return value;
}

static u32 tx_isp_mdns_block_size(u32 gain_log2_q16)
{
	static const u8 block_size[] = { 16, 16, 16, 16, 30, 30, 30, 30, 30 };
	u32 index = min(gain_log2_q16 >> 16, 8U);
	u32 block = block_size[index];

	if (index < 8)
		block = tx_isp_mdns_interpolate(block, block_size[index + 1],
						gain_log2_q16 & 0xffff);
	return ALIGN(block, 4);
}

static u32 tx_isp_mdns_geometry(u16 reg, u32 value, u32 width, u32 height,
			       u32 block)
{
	u32 x = 0, y = 0;
	u32 cw = width / 2, ch = height / 2;
	u32 half = block / 2;

	switch (reg) {
	case 0x7870:
	case 0x7888:
	case 0x78a0:
	case 0x78b8:
		if (reg == 0x78a0 || reg == 0x78b8)
			x = half;
		if (reg == 0x7888 || reg == 0x78b8)
			y = half;
		return (!!((width - x) % block)) |
		       (!!((height - y) % block) << 1);
	case 0x7874:
	case 0x788c:
	case 0x78a4:
	case 0x78bc:
		if (reg == 0x78a4 || reg == 0x78bc)
			x = half;
		if (reg == 0x788c || reg == 0x78bc)
			y = half;
		return (DIV_ROUND_UP(width - x, block) << 24) |
		       (DIV_ROUND_UP(height - y, block) << 16) |
		       (block << 8) | block;
	case 0x7880:
		return half << 16;
	case 0x7898:
		return half;
	case 0x78b0:
		return (half << 16) | half;
	case 0x7884:
		return ((height - half) << 16) | width;
	case 0x789c:
		return (height << 16) | (width - half);
	case 0x78b4:
		return ((height - half) << 16) | (width - half);
	case 0x7a84:
		return ((ch % 16 / 2) << 16) | (cw % 16 / 2);
	case 0x7a88:
		return ((ch - ch % 16 / 2 * 2) << 16) |
		       (cw - cw % 16 / 2 * 2);
	default:
		return value;
	}
}

static void tx_isp_mdns_write_params(void __iomem *base, u32 width, u32 height,
				     u32 gain_log2_q16)
{
	const struct tx_isp_mdns_reg *reg;
	u32 block = tx_isp_mdns_block_size(gain_log2_q16);
	u32 value;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(mdns_sc1a4t_base); i++) {
		reg = &mdns_sc1a4t_base[i];
		value = tx_isp_mdns_gain(reg->offset, reg->value,
					 gain_log2_q16);
		value = tx_isp_mdns_geometry(reg->offset, value, width,
					     height, block);
		writel(value, base + reg->offset);
	}
}

/* Registers carried in struct t23_isp_mdns_config::val, ascending. */
static const u16 mdns_config_regs[T23_ISP_MDNS_WORDS] = {
	0x78c8, 0x78cc, 0x78d0, 0x78d4, 0x78d8, 0x78e0, 0x78e4, 0x78e8,
	0x78ec, 0x78f0, 0x78f4, 0x78f8, 0x78fc, 0x7900, 0x7904, 0x7908,
	0x790c, 0x7910, 0x7914, 0x7918, 0x791c, 0x7920, 0x7924, 0x7928,
	0x792c, 0x7930, 0x7934, 0x7938, 0x793c, 0x7940, 0x7944, 0x7948,
	0x794c, 0x7950, 0x7954, 0x7958, 0x795c, 0x7960, 0x7964, 0x7968,
	0x796c, 0x7970, 0x7974, 0x7978, 0x797c, 0x7980, 0x7984, 0x7988,
	0x798c, 0x7990, 0x7994, 0x7998, 0x799c, 0x79a0, 0x79a4, 0x79a8,
	0x79ac, 0x79b0, 0x79b4, 0x79b8, 0x79bc, 0x79c0, 0x79c4, 0x79c8,
	0x79cc, 0x79d0, 0x79d4, 0x79d8, 0x79dc, 0x79e0, 0x79e4, 0x79e8,
	0x79ec, 0x79f0, 0x79f4, 0x79f8, 0x79fc, 0x7a00, 0x7a04, 0x7a08,
	0x7a0c, 0x7a10, 0x7a14, 0x7a18, 0x7a1c, 0x7a20, 0x7a24, 0x7a28,
	0x7a2c, 0x7a30, 0x7a34, 0x7a38, 0x7a3c, 0x7a40, 0x7a44, 0x7a48,
	0x7a4c, 0x7a50, 0x7a54, 0x7a58, 0x7a5c, 0x7a60, 0x7a64, 0x7a68,
	0x7a6c, 0x7a70, 0x7a74, 0x7a78, 0x7a7c, 0x7a80, 0x7a8c, 0x7a90,
	0x7a94, 0x7a98, 0x7a9c, 0x7aa0, 0x7aa4, 0x7aa8, 0x7aac, 0x7ab0,
	0x7ab4, 0x7ab8, 0x7abc, 0x7ac0, 0x7ac4, 0x7ac8, 0x7acc, 0x7ad0,
	0x7ad4, 0x7ad8, 0x7adc, 0x7ae0, 0x7ae4, 0x7ae8, 0x7aec,
};

static int tx_isp_mdns_config_index(u16 offset)
{
	unsigned int lo = 0, hi = ARRAY_SIZE(mdns_config_regs), mid;

	while (lo < hi) {
		mid = (lo + hi) / 2;
		if (mdns_config_regs[mid] == offset)
			return mid;
		if (mdns_config_regs[mid] < offset)
			lo = mid + 1;
		else
			hi = mid;
	}
	return -1;
}

/*
 * The statistics control and threshold words are replicated in each of the
 * four grid phases; the grid itself follows the frame size.
 */
static void tx_isp_mdns_write_config(void __iomem *base, u32 width, u32 height,
				     const struct t23_isp_mdns_config *cfg)
{
	unsigned int i;
	u16 offset;
	u32 value;
	int n;

	for (i = 0; i < ARRAY_SIZE(mdns_sc1a4t_base); i++) {
		offset = mdns_sc1a4t_base[i].offset;
		n = tx_isp_mdns_config_index(offset);
		if (n >= 0)
			value = cfg->val[n];
		else if (offset < 0x78c8 && (offset - 0x7878) % 0x18 == 0)
			value = cfg->sta_cfg;
		else if (offset < 0x78c8 && (offset - 0x787c) % 0x18 == 0)
			value = cfg->sta_thres;
		else
			value = tx_isp_mdns_geometry(offset, 0, width, height,
						     cfg->block_size);
		writel(value, base + offset);
	}
}

int tx_isp_mdns_check_config(const struct t23_isp_mdns_config *cfg)
{
	if ((cfg->top[0] & ~ISP_MDNS_FUNC_CTRL_MASK) ||
	    (cfg->top[1] & ~ISP_MDNS_FUNC_GROUP_MASK) || cfg->top[2])
		return -EINVAL;
	if (cfg->block_size < 16 || cfg->block_size > 255 ||
	    cfg->block_size % 4)
		return -EINVAL;
	return 0;
}

static int tx_isp_mdns_validate(u32 width, u32 height,
			       enum tx_isp_mdns_profile profile,
			       u32 gain_log2_q16)
{
	if (width < 32 || height < 16 || ((width | height) & 1))
		return -EINVAL;
	if (profile != TX_ISP_MDNS_SC1A4T_LINEAR_DAY)
		return -EOPNOTSUPP;
	if (gain_log2_q16 & BIT(31))
		return -EINVAL;
	/* Motion-grid row and column counts occupy eight bits each. */
	if (width > 4080 || height > 4080)
		return -EOPNOTSUPP;
	return 0;
}

int tx_isp_mdns_configure_bypassed(void __iomem *base, u32 width, u32 height,
				 enum tx_isp_mdns_profile profile,
				 u32 gain_log2_q16)
{
	int ret;

	if (!base)
		return -EINVAL;
	ret = tx_isp_mdns_validate(width, height, profile, gain_log2_q16);
	if (ret)
		return ret;
	writel(ISP_MDNS_FUNC_CTRL_BYPASS, base + ISP_MDNS_FUNC_CONTROL);
	writel(ISP_MDNS_FUNC_GROUP_DEFAULT, base + ISP_MDNS_FUNC_GROUP);
	writel(0, base + ISP_MDNS_DEBUG_CONTROL);
	writel(ISP_MDNS_TRIGGER_STOP, base + ISP_MDNS_REG_TRIGGER);
	tx_isp_mdns_write_params(base, width, height, gain_log2_q16);
	writel(ISP_MDNS_REFRESH_VAL, base + ISP_MDNS_FUNC_REFRESH);
	writel(ISP_MDNS_TRIGGER_START, base + ISP_MDNS_REG_TRIGGER);
	return 0;
}

int tx_isp_mdns_start(struct tx_isp_mdns_history *history, void __iomem *base,
		      enum tx_isp_mdns_profile profile, u32 gain_log2_q16,
		      const struct t23_isp_mdns_config *cfg)
{
	const struct tx_isp_mdns_plane *plane;
	unsigned int i;
	int ret;

	if (!history || !history->cpu || !base)
		return -EINVAL;
	if (history->active)
		return -EBUSY;
	ret = tx_isp_mdns_validate(history->width, history->height, profile,
				   gain_log2_q16);
	if (ret)
		return ret;

	for (i = 0; i < TX_ISP_MDNS_PLANES; i++) {
		plane = &history->layout.plane[i];
		writel((u32)history->dma + plane->offset,
		       base + tx_isp_mdns_address_regs[i]);
		writel(plane->stride, base + tx_isp_mdns_address_regs[i] + 4);
	}
	writel(ISP_MDNS_DMA_CTRL0_DEFAULT, base + ISP_MDNS_DMA_CTRL0);
	writel(ISP_MDNS_DMA_CTRL1_DEFAULT, base + ISP_MDNS_DMA_CTRL1);
	writel(ISP_MDNS_TRIGGER_STOP, base + ISP_MDNS_REG_TRIGGER);
	if (cfg) {
		writel(cfg->top[0] & ISP_MDNS_FUNC_CTRL_MASK,
		       base + ISP_MDNS_FUNC_CONTROL);
		writel(cfg->top[1] & ISP_MDNS_FUNC_GROUP_MASK,
		       base + ISP_MDNS_FUNC_GROUP);
		writel(0, base + ISP_MDNS_DEBUG_CONTROL);
		tx_isp_mdns_write_config(base, history->width, history->height,
					 cfg);
	} else {
		writel(ISP_MDNS_FUNC_CTRL_ENABLE, base + ISP_MDNS_FUNC_CONTROL);
		writel(ISP_MDNS_FUNC_GROUP_DEFAULT, base + ISP_MDNS_FUNC_GROUP);
		writel(0, base + ISP_MDNS_DEBUG_CONTROL);
		tx_isp_mdns_write_params(base, history->width, history->height,
					 gain_log2_q16);
	}
	if (!history->valid)
		writel(ISP_MDNS_REFRESH_VAL, base + ISP_MDNS_FUNC_REFRESH);
	/* Keep the allocation pinned until the whole ISP pipeline is idle. */
	history->active = true;
	/* Publish initialized history before starting DMA. */
	wmb();
	writel(ISP_MDNS_TRIGGER_START, base + ISP_MDNS_REG_TRIGGER);
	return 0;
}

/* Called only after ISP all-done, or after a completed hardware reset. */
void tx_isp_mdns_frame_done(struct tx_isp_mdns_history *history, bool error)
{
	if (!history || !history->active)
		return;
	history->active = false;
	history->valid = !error;
}

int tx_isp_mdns_bypass(struct tx_isp_mdns_history *history, void __iomem *base)
{
	if (!history || !base)
		return -EINVAL;
	if (history->active)
		return -EBUSY;
	writel(ISP_MDNS_FUNC_CTRL_BYPASS, base + ISP_MDNS_FUNC_CONTROL);
	writel(ISP_MDNS_FUNC_GROUP_DEFAULT, base + ISP_MDNS_FUNC_GROUP);
	writel(0, base + ISP_MDNS_DEBUG_CONTROL);
	writel(ISP_MDNS_REFRESH_VAL, base + ISP_MDNS_FUNC_REFRESH);
	writel(ISP_MDNS_TRIGGER_START, base + ISP_MDNS_REG_TRIGGER);
	history->valid = false;
	return 0;
}
