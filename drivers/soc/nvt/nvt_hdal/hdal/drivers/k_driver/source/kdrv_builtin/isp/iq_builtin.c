#if defined(__KERNEL__)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <mach/fmem.h>
#include <mach/nvt-io.h>
#include <linux/of.h>
#include <plat/top.h>

#include "kwrap/cpu.h"
#include "kwrap/debug.h"
#include "vdoenc_builtin.h"

#include "iq_reg.h"
#include "iq_builtin.h"
#include "iq_dtsi.h"
#include "iq_lib_nvt.h"

#if (NVT_FAST_ISP_FLOW)
//=============================================================================
// define
//=============================================================================
#define DPC_ENABLE    1
#define ECS_ENABLE    1
#define LDC_ENABLE    1
#define WDR_ENABLE    1
#define LV_BASE 1000000
#define IQ_BUILTIN_DBG_MSG 0
#define IQ_BUILTIN_REG_MSG 0

#define IQ_3DNR_STABLE_FRM_CNT 10
#define IQ_LCE_STABLE_CNT 10

//=============================================================================
// global
//=============================================================================
static ISOMAP iso_map_tab[IQ_GAIN_ID_MAX_NUM] = {
	{    100,      IQ_GAIN_1X},
	{    200,      IQ_GAIN_2X},
	{    400,      IQ_GAIN_4X},
	{    800,      IQ_GAIN_8X},
	{   1600,     IQ_GAIN_16X},
	{   3200,     IQ_GAIN_32X},
	{   6400,     IQ_GAIN_64X},
	{  12800,    IQ_GAIN_128X},
	{  25600,    IQ_GAIN_256X},
	{  51200,    IQ_GAIN_512X},
	{ 102400,   IQ_GAIN_1024X},
	{ 204800,   IQ_GAIN_2048X},
	{ 409600,   IQ_GAIN_4096X},
	{ 819200,   IQ_GAIN_8192X},
	{1638400,  IQ_GAIN_16384X},
	{3276800,  IQ_GAIN_32768X}
};

static UINT32 iq_sqrt_tab[41] = { // sqrt[x]*256, x = 0, 0.2, 0.4, ..., 8
	0, 114, 161, 198, 228, 256, 280, 302, 323, 343,
	362, 379, 396, 412, 428, 443, 457, 472, 485, 499,
	512, 524, 536, 549, 560, 572, 583, 594, 605, 616,
	627, 637, 647, 657, 667, 677, 686, 696, 705, 714,
	724
};

static UINT32 iq_bilateral_center[IQ_GAIN_ID_MAX_NUM][2] = {
	{  15,   17},
	{  22,   24},
	{  35,   40},
	{  77,   92},
	{  88,  128},
	{ 100,  143},
	{ 129,  154},
	{ 243,  281},
	{ 335,  402},
	{ 500,  600},
	{ 900, 1000},
	{1000, 1023},
	{1000, 1023},
	{1000, 1023},
	{1000, 1023},
	{1000, 1023},
};

static UINT32 *gamma_buffer[ISP_BUILTIN_ID_MAX_NUM] = {NULL};
static UINT32 *ycurve_buffer[ISP_BUILTIN_ID_MAX_NUM] = {NULL};

#if DPC_ENABLE
static UINT32 *dpc_buffer[ISP_BUILTIN_ID_MAX_NUM] = {NULL};
#endif

#if ECS_ENABLE
static UINT32 *ecs_buffer[ISP_BUILTIN_ID_MAX_NUM] = {NULL};
#endif

#if LDC_ENABLE
static UINT32 *lut2d_buffer[ISP_BUILTIN_ID_MAX_NUM] = {NULL};
#endif

IQ_PARAM_PTR *iq_param[ISP_BUILTIN_ID_MAX_NUM] = {NULL};
IQ_PARAM_PTR iq_param_memalloc_addr[ISP_BUILTIN_ID_MAX_NUM] = {0};
static BOOL iq_param_memalloc_valid[ISP_BUILTIN_ID_MAX_NUM] = {0};
static UINT32 iq_edge_ker_tab[17][2] = {
	{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8},
	{1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8}, {6, 8}, {7, 8}, {8, 8} };
static UINT8 iq_shdr_dbg_fcurve_idx_lut[IQ_SHDR_FCURVE_IDX_NUM] = {
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18,
	20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
	40, 42, 44, 46, 48, 50, 52, 54, 56, 58,
	60, 62};
static UINT8 iq_shdr_dbg_fcurve_split_lut[IQ_SHDR_FCURVE_SPLIT_NUM] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1};
static UINT16 iq_shdr_dbg_short_fcurve_val_lut[IQ_SHDR_FCURVE_VAL_NUM] = {
	0, 64, 128, 192, 256, 320, 384, 448, 512, 576,
	640, 704, 768, 832, 896, 960, 1024, 1088, 1152, 1216,
	1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856,
	1920, 1984, 2048, 2112, 2176, 2240, 2304, 2368, 2432, 2496,
	2560, 2624, 2688, 2752, 2816, 2880, 2944, 3008, 3072, 3136,
	3200, 3264, 3328, 3392, 3456, 3520, 3584, 3648, 3712, 3776,
	3840, 3904, 3968, 4032, 4095};
static UINT16 iq_shdr_dbg_long_fcurve_val_lut[IQ_SHDR_FCURVE_VAL_NUM] = {
	0, 1024, 2048, 3072, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
	4095, 4095, 4095, 4095, 4095};

static UINT32 iq_frm_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};

IQ_BUILTIN_FRONT_DTSI iq_front_dtsi = {
	IQ_3DNR_STABLE_FRM_CNT,
	{0, 0, 0, 10, 20, 40, 80, 100, 100, 100, 100, 100, 100, 100, 100, 100},
	150,
	150,
	25,
	60,
	60
};

IQ_BUILTIN_FRONT_FACTOR iq_front_factor = {
	IQ_3DNR_STABLE_FRM_CNT,
	150,
	150,
	25,
	60,
	60
};

#define SUB_NODE_LENGTH 64

//=============================================================================
// iq cal
//=============================================================================
INT32 iq_cal(INT32 ratio, INT32 int_value, INT32 min_value, INT32 max_value)
{
	if (ratio == 100) {
		return int_value;
	}
	return IQ_CLAMP(int_value * ratio / 100, min_value, max_value);
}

static INT32 iq_intpl(INT32 index, INT32 l_value, INT32 h_value, INT32 l_index, INT32 h_index)
{
	INT32 range = h_index - l_index;

	if (l_value == h_value) {
		return l_value;
	} else if (index <= l_index) {
		return l_value;
	} else if (index >= h_index) {
		return h_value;
	}
	if (h_value < l_value) {
		return l_value + ((h_value - l_value) * (index  - l_index) - (range >> 1)) / range;
	} else {
		return l_value + ((h_value - l_value) * (index  - l_index) + (range >> 1)) / range;
	}
}

INT32 iq_intpl_value(INT32 ratio, INT32 int_value, INT32 min_value, INT32 max_value)
{
	if (ratio == 100) {
		return int_value;
	} else if (ratio > 100) {
		return iq_intpl(ratio, int_value, max_value, 100, 200);
	} else {
		return iq_intpl(ratio, min_value, int_value, 0, 100);
	}
}

static void iq_intpl_tbl_uint32(UINT32 *l_value, UINT32 *h_value, INT32 size, UINT32 *target, INT32 index, INT32 l_index, INT32 h_index)
{
	INT32 i;

	for (i = 0; i < size; i++) {
		target[i] = iq_intpl(index, l_value[i], h_value[i], l_index, h_index);
	}
}

static void iq_intpl_tbl_gamma(UINT32 *l_value, UINT32 *h_value, INT32 l_index, INT32 h_index, INT32 index, UINT32 *target)
{
	INT32 i;

	for (i = 0; i < IQ_GAMMA_GAMMA_LEN; i++) {
		target[i] = iq_intpl(index, l_value[i], h_value[i], l_index, h_index);
	}
}

static void iq_intpl_tbl_cc(INT16 *l_value, INT16 *h_value, INT32 l_index, INT32 h_index, INT32 index, INT16 *target)
{
	INT32 i, E1, E2;

	for (i = 0; i < 9; i++) {
		E1 = l_value[i];
		if (E1 > 0x800) {
			E1 = E1 - 0x1000;
		}

		E2 = h_value[i];
		if (E2 > 0x800) {
			E2 = E2 - 0x1000;
		}
		target[i] = iq_intpl(index, E1, E2, l_index, h_index);
	}
}

//=============================================================================
// internal functions
//=============================================================================
static void iq_builtin_set_companding(IQ_PARAM_PTR *iq_param)
{
	T_RD0_DECOMP                        decomp_reg_0;
	T_RD4_DECOMP                        decomp_reg_1;
	T_RD8_DECOMP                        decomp_reg_2;
	T_RDC_DECOMP                        decomp_reg_3;
	T_RE0_DECOMP                        decomp_reg_4;
	T_RE4_DECOMP                        decomp_reg_5;

	T_RE8_DECOMP                        decomp_sub_pts_reg_0;
	T_REC_DECOMP                        decomp_sub_pts_reg_1;
	T_RF0_DECOMP                        decomp_sub_pts_reg_2;
	T_RF4_DECOMP                        decomp_sub_pts_reg_3;
	T_RF8_DECOMP                        decomp_sub_pts_reg_4;
	T_RFC_DECOMP                        decomp_sub_pts_reg_5;

	T_R100_DECOMP                       decomp_shift_pts_reg_0;
	T_R104_DECOMP                       decomp_shift_pts_reg_1;

	T_R108_COMP                         comp_reg_0;
	T_R10C_COMP                         comp_reg_1;
	T_R110_COMP                         comp_reg_2;

	T_R114_COMP                         comp_sub_pts_reg_0;
	T_R118_COMP                         comp_sub_pts_reg_1;
	T_R11C_COMP                         comp_sub_pts_reg_2;
	T_R120_COMP                         comp_sub_pts_reg_3;
	T_R124_COMP                         comp_sub_pts_reg_4;
	T_R128_COMP                         comp_sub_pts_reg_5;

	T_R12C_COMP                         comp_shift_pts_reg_0;
	T_R130_COMP                         comp_shift_pts_reg_1;

	IQ_COMPANDING_PARAM                 *companding;
	unsigned int                        reg_addr;

	reg_addr = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);

	companding = iq_param->companding;

	decomp_reg_0.bit.DECOMP_KP0 = companding->decomp_knee_pts[0];
	decomp_reg_0.bit.DECOMP_KP1 = companding->decomp_knee_pts[1];
	nvt_writel(decomp_reg_0.reg, reg_addr + RD0_DECOMP_OFS);

	decomp_reg_1.bit.DECOMP_KP2 = companding->decomp_knee_pts[2];
	decomp_reg_1.bit.DECOMP_KP3 = companding->decomp_knee_pts[3];
	nvt_writel(decomp_reg_1.reg, reg_addr + RD4_DECOMP_OFS);

	decomp_reg_2.bit.DECOMP_KP4 = companding->decomp_knee_pts[4];
	decomp_reg_2.bit.DECOMP_KP5 = companding->decomp_knee_pts[5];
	nvt_writel(decomp_reg_2.reg, reg_addr + RD8_DECOMP_OFS);

	decomp_reg_3.bit.DECOMP_KP6 = companding->decomp_knee_pts[6];
	decomp_reg_3.bit.DECOMP_KP7 = companding->decomp_knee_pts[7];
	nvt_writel(decomp_reg_3.reg, reg_addr + RDC_DECOMP_OFS);

	decomp_reg_4.bit.DECOMP_KP8 = companding->decomp_knee_pts[8];
	decomp_reg_4.bit.DECOMP_KP9 = companding->decomp_knee_pts[9];
	nvt_writel(decomp_reg_4.reg, reg_addr + RE0_DECOMP_OFS);

	decomp_reg_5.bit.DECOMP_KP10 = companding->decomp_knee_pts[10];
	nvt_writel(decomp_reg_5.reg, reg_addr + RE4_DECOMP_OFS);

	decomp_sub_pts_reg_0.bit.DECOMP_SP0 = companding->decomp_str_pts[0];
	decomp_sub_pts_reg_0.bit.DECOMP_SP1 = companding->decomp_str_pts[1];
	nvt_writel(decomp_sub_pts_reg_0.reg, reg_addr + RE8_DECOMP_OFS);

	decomp_sub_pts_reg_1.bit.DECOMP_SP2 = companding->decomp_str_pts[2];
	decomp_sub_pts_reg_1.bit.DECOMP_SP3 = companding->decomp_str_pts[3];
	nvt_writel(decomp_sub_pts_reg_1.reg, reg_addr + REC_DECOMP_OFS);

	decomp_sub_pts_reg_2.bit.DECOMP_SP4 = companding->decomp_str_pts[4];
	decomp_sub_pts_reg_2.bit.DECOMP_SP5 = companding->decomp_str_pts[5];
	nvt_writel(decomp_sub_pts_reg_2.reg, reg_addr + RF0_DECOMP_OFS);

	decomp_sub_pts_reg_3.bit.DECOMP_SP6 = companding->decomp_str_pts[6];
	decomp_sub_pts_reg_3.bit.DECOMP_SP7 = companding->decomp_str_pts[7];
	nvt_writel(decomp_sub_pts_reg_3.reg, reg_addr + RF4_DECOMP_OFS);

	decomp_sub_pts_reg_4.bit.DECOMP_SP8 = companding->decomp_str_pts[8];
	decomp_sub_pts_reg_4.bit.DECOMP_SP9 = companding->decomp_str_pts[9];
	nvt_writel(decomp_sub_pts_reg_4.reg, reg_addr + RF8_DECOMP_OFS);

	decomp_sub_pts_reg_5.bit.DECOMP_SP10 = companding->decomp_str_pts[10];
	decomp_sub_pts_reg_5.bit.DECOMP_SP11 = companding->decomp_str_pts[11];
	nvt_writel(decomp_sub_pts_reg_5.reg, reg_addr + RFC_DECOMP_OFS);

	decomp_shift_pts_reg_0.bit.DECOMP_SB0 = companding->decomp_shift_pts[0];
	decomp_shift_pts_reg_0.bit.DECOMP_SB1 = companding->decomp_shift_pts[1];
	decomp_shift_pts_reg_0.bit.DECOMP_SB2 = companding->decomp_shift_pts[2];
	decomp_shift_pts_reg_0.bit.DECOMP_SB3 = companding->decomp_shift_pts[3];
	decomp_shift_pts_reg_0.bit.DECOMP_SB4 = companding->decomp_shift_pts[4];
	decomp_shift_pts_reg_0.bit.DECOMP_SB5 = companding->decomp_shift_pts[5];
	decomp_shift_pts_reg_0.bit.DECOMP_SB6 = companding->decomp_shift_pts[6];
	decomp_shift_pts_reg_0.bit.DECOMP_SB7 = companding->decomp_shift_pts[7];
	nvt_writel(decomp_shift_pts_reg_0.reg, reg_addr + R100_DECOMP_OFS);

	decomp_shift_pts_reg_1.bit.DECOMP_SB8 = companding->decomp_shift_pts[8];
	decomp_shift_pts_reg_1.bit.DECOMP_SB9 = companding->decomp_shift_pts[9];
	decomp_shift_pts_reg_1.bit.DECOMP_SB10 = companding->decomp_shift_pts[10];
	decomp_shift_pts_reg_1.bit.DECOMP_SB11 = companding->decomp_shift_pts[11];
	nvt_writel(decomp_shift_pts_reg_1.reg, reg_addr + R104_DECOMP_OFS);

	comp_reg_0.bit.COMP_KP0 = companding->comp_knee_pts[0];
	comp_reg_0.bit.COMP_KP1 = companding->comp_knee_pts[1];
	comp_reg_0.bit.COMP_KP2 = companding->comp_knee_pts[2];
	comp_reg_0.bit.COMP_KP3 = companding->comp_knee_pts[3];
	nvt_writel(comp_reg_0.reg, reg_addr + R108_COMP_OFS);

	comp_reg_1.bit.COMP_KP4 = companding->comp_knee_pts[4];
	comp_reg_1.bit.COMP_KP5 = companding->comp_knee_pts[5];
	comp_reg_1.bit.COMP_KP6 = companding->comp_knee_pts[6];
	comp_reg_1.bit.COMP_KP7 = companding->comp_knee_pts[7];
	nvt_writel(comp_reg_1.reg, reg_addr + R10C_COMP_OFS);

	comp_reg_2.bit.COMP_KP8 = companding->comp_knee_pts[8];
	comp_reg_2.bit.COMP_KP9 = companding->comp_knee_pts[9];
	comp_reg_2.bit.COMP_KP10 = companding->comp_knee_pts[10];
	nvt_writel(comp_reg_2.reg, reg_addr + R110_COMP_OFS);

	comp_sub_pts_reg_0.bit.COMP_SP0 = companding->comp_str_pts[0];
	comp_sub_pts_reg_0.bit.COMP_SP1 = companding->comp_str_pts[1];
	nvt_writel(comp_sub_pts_reg_0.reg, reg_addr + R114_COMP_OFS);

	comp_sub_pts_reg_1.bit.COMP_SP2 = companding->comp_str_pts[2];
	comp_sub_pts_reg_1.bit.COMP_SP3 = companding->comp_str_pts[3];
	nvt_writel(comp_sub_pts_reg_1.reg, reg_addr + R118_COMP_OFS);

	comp_sub_pts_reg_2.bit.COMP_SP4 = companding->comp_str_pts[4];
	comp_sub_pts_reg_2.bit.COMP_SP5 = companding->comp_str_pts[5];
	nvt_writel(comp_sub_pts_reg_2.reg, reg_addr + R11C_COMP_OFS);

	comp_sub_pts_reg_3.bit.COMP_SP6 = companding->comp_str_pts[6];
	comp_sub_pts_reg_3.bit.COMP_SP7 = companding->comp_str_pts[7];
	nvt_writel(comp_sub_pts_reg_3.reg, reg_addr + R120_COMP_OFS);

	comp_sub_pts_reg_4.bit.COMP_SP8 = companding->comp_str_pts[8];
	comp_sub_pts_reg_4.bit.COMP_SP9 = companding->comp_str_pts[9];
	nvt_writel(comp_sub_pts_reg_4.reg, reg_addr + R124_COMP_OFS);

	comp_sub_pts_reg_5.bit.COMP_SP10 = companding->comp_str_pts[10];
	comp_sub_pts_reg_5.bit.COMP_SP11 = companding->comp_str_pts[11];
	nvt_writel(comp_sub_pts_reg_5.reg, reg_addr + R128_COMP_OFS);

	comp_shift_pts_reg_0.bit.COMP_SB0 = companding->comp_shift_pts[0];
	comp_shift_pts_reg_0.bit.COMP_SB1 = companding->comp_shift_pts[1];
	comp_shift_pts_reg_0.bit.COMP_SB2 = companding->comp_shift_pts[2];
	comp_shift_pts_reg_0.bit.COMP_SB3 = companding->comp_shift_pts[3];
	comp_shift_pts_reg_0.bit.COMP_SB4 = companding->comp_shift_pts[4];
	comp_shift_pts_reg_0.bit.COMP_SB5 = companding->comp_shift_pts[5];
	comp_shift_pts_reg_0.bit.COMP_SB6 = companding->comp_shift_pts[6];
	comp_shift_pts_reg_0.bit.COMP_SB7 = companding->comp_shift_pts[7];
	nvt_writel(comp_shift_pts_reg_0.reg, reg_addr + R12C_COMP_OFS);

	comp_shift_pts_reg_1.bit.COMP_SB8 = companding->comp_shift_pts[8];
	comp_shift_pts_reg_1.bit.COMP_SB9 = companding->comp_shift_pts[9];
	comp_shift_pts_reg_1.bit.COMP_SB10 = companding->comp_shift_pts[10];
	comp_shift_pts_reg_1.bit.COMP_SB11 = companding->comp_shift_pts[11];
	nvt_writel(comp_shift_pts_reg_1.reg, reg_addr + R130_COMP_OFS);
}

static void iq_builtin_set_sie_ob(UINT32 id, UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	T_RBC_BASIC_OB              sie_ob_ofs_reg;
	T_R2A8_STCS_OB              ca_la_ob_reg;
	T_R1E8_STCS                 ca_src_reg;

	unsigned int                reg_addr_m = 0, reg_addr_sub = 0;
	IQ_OB_TUNE_PARAM            *final_ob_tune;

	UINT32                      single_ob;
	UINT32                      iso_idx = 0;
	INT32                       i;

	switch (id) {
	default:
	case 0:
		reg_addr_m = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
		break;
	case 1:
		reg_addr_m = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
		break;
	case 2:
		reg_addr_m = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
		break;
	case 3:
		reg_addr_m = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
		break;
	case 4:
		reg_addr_m = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
		break;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		for (i = ISP_BUILTIN_ID_MAX_NUM - 1; i >= 0; i--) {
			if ((0x1 << i) & isp_builtin_get_shdr_id_mask(id)) {
				break;
			}
		}

		switch (i) {
		default:
		case 0:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
		}
	}

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->ob->mode == IQ_OP_TYPE_AUTO) {
		final_ob_tune = &iq_param->ob->auto_param[iso_idx];
	} else {
		final_ob_tune = &iq_param->ob->manual_param;
	}
	single_ob = IQ_CLAMP(((final_ob_tune->cofs[1] + final_ob_tune->cofs[2]) >> 1), 0, 4095);

	// set short exp frame SIE
	sie_ob_ofs_reg.reg = nvt_readl(reg_addr_m + RBC_BASIC_OB_OFS);
	sie_ob_ofs_reg.bit.OB_OFS = single_ob;
	nvt_writel(sie_ob_ofs_reg.reg, reg_addr_m + RBC_BASIC_OB_OFS);

	// set long exp frame SIE
	if (reg_addr_sub != 0) {
		sie_ob_ofs_reg.reg = nvt_readl(reg_addr_sub + RBC_BASIC_OB_OFS);
		sie_ob_ofs_reg.bit.OB_OFS = single_ob;
		nvt_writel(sie_ob_ofs_reg.reg, reg_addr_sub + RBC_BASIC_OB_OFS);
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		ca_la_ob_reg.bit.STCS_CA_OB_OFS = 0; // software operation at sie_getCAResultManual()
		ca_la_ob_reg.bit.STCS_LA_OB_OFS = single_ob;

		// set short exp frame SIE
		nvt_writel(ca_la_ob_reg.reg, reg_addr_m + R2A8_STCS_OB_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			nvt_writel(ca_la_ob_reg.reg, reg_addr_sub + R2A8_STCS_OB_OFS);
		}
	} else {
		// set short exp frame SIE
		ca_src_reg.reg = nvt_readl(reg_addr_m + R1E8_STCS_OFS);
		ca_src_reg.bit.STCS_CA_ACCM_SRC = 1; // 520 don't exist this register
		nvt_writel(ca_src_reg.reg, reg_addr_m + R1E8_STCS_OFS);
		ca_la_ob_reg.bit.STCS_CA_OB_OFS = single_ob;
		ca_la_ob_reg.bit.STCS_LA_OB_OFS = single_ob;
		nvt_writel(ca_la_ob_reg.reg, reg_addr_m + R2A8_STCS_OB_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			ca_src_reg.reg = nvt_readl(reg_addr_sub + R1E8_STCS_OFS);
			ca_src_reg.bit.STCS_CA_ACCM_SRC = 1; // 520 don't exist this register
			nvt_writel(ca_src_reg.reg, reg_addr_sub + R1E8_STCS_OFS);
			ca_la_ob_reg.bit.STCS_CA_OB_OFS = single_ob;
			ca_la_ob_reg.bit.STCS_LA_OB_OFS = single_ob;
			nvt_writel(ca_la_ob_reg.reg, reg_addr_sub + R2A8_STCS_OB_OFS);
		}
	}
}

static void iq_builtin_set_ife_ob(UINT32 id, UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	T_OB_REGISTER_0             ob_reg_0;
	T_OB_REGISTER_1             ob_reg_1;
	T_OB_REGISTER_2             ob_reg_2;
	T_COLOR_OFFSET_REGISTER_3   color_offset_reg_3;
	T_COLOR_OFFSET_REGISTER_4   color_offset_reg_4;
	T_COLOR_OFFSET_REGISTER_5   color_offset_reg_5;
	T_COLOR_OFFSET_REGISTER_6   color_offset_reg_6;
	T_COLOR_OFFSET_REGISTER_7   color_offset_reg_7;
	T_COLOR_OFFSET_REGISTER_8   color_offset_reg_8;

	unsigned int                reg_addr;
	IQ_OB_TUNE_PARAM            *final_ob_tune;

	UINT32                      iso_idx = 0;
	INT32                       i;

	reg_addr = (unsigned int)ioremap_nocache(IFE_BASE_ADDR, IFE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->ob->mode == IQ_OP_TYPE_AUTO) {
		final_ob_tune = &iq_param->ob->auto_param[iso_idx];
	} else {
		final_ob_tune = &iq_param->ob->manual_param;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		ob_reg_0.bit.ife_cofs_r  = 0;
		ob_reg_0.bit.ife_cofs_gr = 0;
		nvt_writel(ob_reg_0.reg, reg_addr + OB_OFS_0);

		ob_reg_1.bit.ife_cofs_gb = 0;
		ob_reg_1.bit.ife_cofs_b  = 0;
		nvt_writel(ob_reg_1.reg, reg_addr + OB_OFS_1);

		ob_reg_2.bit.ife_cofs_ir = 0;
		nvt_writel(ob_reg_2.reg, reg_addr + OB_OFS_2);
	} else {
		ob_reg_0.bit.ife_cofs_r  = final_ob_tune->cofs[0];
		ob_reg_0.bit.ife_cofs_gr = final_ob_tune->cofs[1];
		nvt_writel(ob_reg_0.reg, reg_addr + OB_OFS_0);

		ob_reg_1.bit.ife_cofs_gb = final_ob_tune->cofs[2];
		ob_reg_1.bit.ife_cofs_b  = final_ob_tune->cofs[3];
		nvt_writel(ob_reg_1.reg, reg_addr + OB_OFS_1);

		ob_reg_2.bit.ife_cofs_ir = final_ob_tune->cofs[4];
		nvt_writel(ob_reg_2.reg, reg_addr + OB_OFS_2);
	}

	if (isp_builtin_get_shdr_enable(id)) {
		color_offset_reg_3.bit.ife_f_p0_cofs_r = final_ob_tune->cofs[0];
		color_offset_reg_3.bit.ife_f_p0_cofs_gr = final_ob_tune->cofs[1];
		nvt_writel(color_offset_reg_3.reg, reg_addr + COLOR_OFFSET_REGISTER_3_OFS);

		color_offset_reg_4.bit.ife_f_p0_cofs_gb = final_ob_tune->cofs[2];
		color_offset_reg_4.bit.ife_f_p0_cofs_b = final_ob_tune->cofs[3];
		nvt_writel(color_offset_reg_4.reg, reg_addr + COLOR_OFFSET_REGISTER_4_OFS);

		color_offset_reg_5.bit.ife_f_p0_cofs_ir = final_ob_tune->cofs[4];
		nvt_writel(color_offset_reg_5.reg, reg_addr + COLOR_OFFSET_REGISTER_5_OFS);

		color_offset_reg_6.bit.ife_f_p1_cofs_r = final_ob_tune->cofs[0];
		color_offset_reg_6.bit.ife_f_p1_cofs_gr = final_ob_tune->cofs[1];
		nvt_writel(color_offset_reg_6.reg, reg_addr + COLOR_OFFSET_REGISTER_6_OFS);

		color_offset_reg_7.bit.ife_f_p1_cofs_gb = final_ob_tune->cofs[2];
		color_offset_reg_7.bit.ife_f_p1_cofs_b = final_ob_tune->cofs[3];
		nvt_writel(color_offset_reg_7.reg, reg_addr + COLOR_OFFSET_REGISTER_7_OFS);

		color_offset_reg_8.bit.ife_f_p1_cofs_ir = final_ob_tune->cofs[4];
		nvt_writel(color_offset_reg_8.reg, reg_addr + COLOR_OFFSET_REGISTER_8_OFS);
	} else {
		color_offset_reg_3.bit.ife_f_p0_cofs_r = 0;
		color_offset_reg_3.bit.ife_f_p0_cofs_gr = 0;
		nvt_writel(color_offset_reg_3.reg, reg_addr + COLOR_OFFSET_REGISTER_3_OFS);

		color_offset_reg_4.bit.ife_f_p0_cofs_gb = 0;
		color_offset_reg_4.bit.ife_f_p0_cofs_b = 0;
		nvt_writel(color_offset_reg_4.reg, reg_addr + COLOR_OFFSET_REGISTER_4_OFS);

		color_offset_reg_5.bit.ife_f_p0_cofs_ir = 0;
		nvt_writel(color_offset_reg_5.reg, reg_addr + COLOR_OFFSET_REGISTER_5_OFS);

		color_offset_reg_6.bit.ife_f_p1_cofs_r = 0;
		color_offset_reg_6.bit.ife_f_p1_cofs_gr = 0;
		nvt_writel(color_offset_reg_6.reg, reg_addr + COLOR_OFFSET_REGISTER_6_OFS);

		color_offset_reg_7.bit.ife_f_p1_cofs_gb = 0;
		color_offset_reg_7.bit.ife_f_p1_cofs_b = 0;
		nvt_writel(color_offset_reg_7.reg, reg_addr + COLOR_OFFSET_REGISTER_7_OFS);

		color_offset_reg_8.bit.ife_f_p1_cofs_ir = 0;
		nvt_writel(color_offset_reg_8.reg, reg_addr + COLOR_OFFSET_REGISTER_8_OFS);
	}
}

#if DPC_ENABLE
static void iq_builtin_set_dpc(UINT32 id, IQ_PARAM_PTR *iq_param)
{
	T_R48_ENGINE_DRAM                dpc_addr;
	T_R4_ENGINE_FUNCTION             dpc_en;

	unsigned int                     reg_addr_m = 0, reg_addr_sub = 0;
	IQ_DPC_PARAM                     *dpc;

	VOS_ADDR                         phy_adr;
	INT32                            i;

	switch (id) {
		default:
		case 0:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		for (i = ISP_BUILTIN_ID_MAX_NUM - 1; i >= 0; i--) {
			if ((0x1 << i) & isp_builtin_get_shdr_id_mask(id)) {
				break;
			}
		}

		switch (i) {
		default:
		case 0:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
		}
	}

	dpc = iq_param->dpc;

	// set short exp frame SIE
	dpc_en.reg = nvt_readl(reg_addr_m + R4_ENGINE_FUNCTION_OFS);
	dpc_en.bit.DPC_EN = dpc->enable;
	nvt_writel(dpc_en.reg, reg_addr_m + R4_ENGINE_FUNCTION_OFS);

	// set long exp frame SIE
	if (reg_addr_sub != 0) {
		dpc_en.reg = nvt_readl(reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
		dpc_en.bit.DPC_EN = dpc->enable;
		nvt_writel(dpc_en.reg, reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
	}

	if (dpc->enable) {
		int i;
		for (i = 0; i < DPC_TABLE_SIZE; i++) {
			dpc_buffer[id][i] = dpc->table[i];
		}

		vos_cpu_dcache_sync((VOS_ADDR)dpc_buffer[id], DPC_BUFFER_SIZE, VOS_DMA_TO_DEVICE);
		phy_adr = vos_cpu_get_phy_addr((unsigned int)dpc_buffer[id]);
		dpc_addr.bit.DRAM_IN1_SAI = phy_adr >> 2;

		// set short exp frame SIE
		nvt_writel(dpc_addr.reg, reg_addr_m + R48_ENGINE_DRAM_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			nvt_writel(dpc_addr.reg, reg_addr_sub + R48_ENGINE_DRAM_OFS);
		}
	}
}
#endif

#if ECS_ENABLE
static void iq_builtin_set_ecs(UINT32 id, UINT32 ct, IQ_PARAM_PTR *iq_param)
{
	T_R50_ENGINE_DRAM                ecs_addr;
	T_R4_ENGINE_FUNCTION             ecs_en;

	unsigned int                     reg_addr_m = 0, reg_addr_sub = 0;
	IQ_SHADING_PARAM                 *shading;
	IQ_SHADING_EXT_PARAM             *shading_ext;

	VOS_ADDR                         phy_adr;
	UINT32                           ecs_idx = 0;
	INT32                            i;

	switch (id) {
		default:
		case 0:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		for (i = ISP_BUILTIN_ID_MAX_NUM - 1; i >= 0; i--) {
			if ((0x1 << i) & isp_builtin_get_shdr_id_mask(id)) {
				break;
			}
		}

		switch (i) {
		default:
		case 0:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
		}
	}

	shading = iq_param->shading;
	shading_ext = iq_param->shading_ext;

	if (ct < (iq_param->shading_inter->ecs_smooth_l_m_ct_lower + iq_param->shading_inter->ecs_smooth_l_m_ct_upper) / 2) {
		ecs_idx = IQ_ECS_TEMPERATURE_L;
	} else if (ct < (iq_param->shading_inter->ecs_smooth_m_h_ct_lower + iq_param->shading_inter->ecs_smooth_m_h_ct_upper) / 2) {
		ecs_idx = IQ_ECS_TEMPERATURE_M;
	} else {
		ecs_idx = IQ_ECS_TEMPERATURE_H;
	}

	if (iq_param->shading_inter->mode == IQ_OP_TYPE_AUTO) {
		for (i = 0; i < ECS_TABLE_SIZE; i++) {
			ecs_buffer[id][i] = shading_ext->ecs_map_tbl[ecs_idx][i];
		}
	} else {
		for (i = 0; i < ECS_TABLE_SIZE; i++) {
			ecs_buffer[id][i] = shading->ecs_map_tbl[i];
		}
	}

	// set short exp frame SIE
	ecs_en.reg = nvt_readl(reg_addr_m + R4_ENGINE_FUNCTION_OFS);
	ecs_en.bit.ECS_EN = shading->ecs_enable;
	nvt_writel(ecs_en.reg, reg_addr_m + R4_ENGINE_FUNCTION_OFS);

	// set long exp frame SIE
	if (reg_addr_sub != 0) {
		ecs_en.reg = nvt_readl(reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
		ecs_en.bit.ECS_EN = shading->ecs_enable;
		nvt_writel(ecs_en.reg, reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
	}

	if (shading->ecs_enable) {
		vos_cpu_dcache_sync((VOS_ADDR)ecs_buffer[id], ECS_BUFFER_SIZE, VOS_DMA_TO_DEVICE);
		phy_adr = vos_cpu_get_phy_addr((unsigned int)ecs_buffer[id]);
		ecs_addr.bit.DRAM_IN2_SAI = phy_adr >> 2;

		// set short exp frame SIE
		nvt_writel(ecs_addr.reg, reg_addr_m + R50_ENGINE_DRAM_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			nvt_writel(ecs_addr.reg, reg_addr_sub + R50_ENGINE_DRAM_OFS);
		}
	}
}
#endif

static void iq_builtin_set_dgain(UINT32 id, UINT32 value)
{
	T_R4_ENGINE_FUNCTION        sie_control_reg;
	T_BASIC_CG_0                basic_cg_0;
	T_BASIC_CG_1                basic_cg_1;
	T_BASIC_CG_2                basic_cg_2;
	T_STCS_LA_CG_0              stcs_la_cg_0;
	T_STCS_LA_CG_1              stcs_la_cg_1;
	T_R15C_BASIC_DGAIN          basic_dgain;

	unsigned int                reg_addr_m = 0, reg_addr_sub = 0;
	UINT32                      dgain = 0;
	INT32 i;

	switch (id) {
		default:
		case 0:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_m = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		for (i = ISP_BUILTIN_ID_MAX_NUM - 1; i >= 0; i--) {
			if ((0x1 << i) & isp_builtin_get_shdr_id_mask(id)) {
				break;
			}
		}

		switch (i) {
		default:
		case 0:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr_sub = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
		}
	}

	dgain = value;

	if (nvt_get_chip_id() == CHIP_NA51055) { // 520 achieve at SIE cgain
		// set short exp frame SIE
		sie_control_reg.reg = nvt_readl(reg_addr_m + R4_ENGINE_FUNCTION_OFS);
		sie_control_reg.bit.CGAIN_EN = 1;
		nvt_writel(sie_control_reg.reg, reg_addr_m + R4_ENGINE_FUNCTION_OFS);

		basic_cg_0.bit.CGAIN_RGAIN = dgain;
		basic_cg_0.bit.CGAIN_GRGAIN = dgain;
		nvt_writel(basic_cg_0.reg, reg_addr_m + BASIC_CG_0_OFS);

		basic_cg_1.bit.CGAIN_GBGAIN = dgain;
		basic_cg_1.bit.CGAIN_BGAIN = dgain;
		nvt_writel(basic_cg_1.reg, reg_addr_m + BASIC_CG_1_OFS);

		basic_cg_2.bit.CGAIN_IRGAIN = dgain;
		basic_cg_2.bit.CGAIN_LEVEL_SEL = 1; // 3.7
		nvt_writel(basic_cg_2.reg, reg_addr_m + BASIC_CG_2_OFS);

		stcs_la_cg_0.bit.LA_CG_RGAIN = dgain;
		stcs_la_cg_0.bit.LA_CG_GGAIN = dgain;
		nvt_writel(stcs_la_cg_0.reg, reg_addr_m + STCS_LA_CG_0_OFS);

		stcs_la_cg_1.bit.LA_CG_BGAIN = dgain;
		nvt_writel(stcs_la_cg_1.reg, reg_addr_m + STCS_LA_CG_1_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			sie_control_reg.reg = nvt_readl(reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
			sie_control_reg.bit.CGAIN_EN = 1;
			nvt_writel(sie_control_reg.reg, reg_addr_sub + R4_ENGINE_FUNCTION_OFS);

			basic_cg_0.bit.CGAIN_RGAIN = dgain;
			basic_cg_0.bit.CGAIN_GRGAIN = dgain;
			nvt_writel(basic_cg_0.reg, reg_addr_sub + BASIC_CG_0_OFS);

			basic_cg_1.bit.CGAIN_GBGAIN = dgain;
			basic_cg_1.bit.CGAIN_BGAIN = dgain;
			nvt_writel(basic_cg_1.reg, reg_addr_sub + BASIC_CG_1_OFS);

			basic_cg_2.bit.CGAIN_IRGAIN = dgain;
			basic_cg_2.bit.CGAIN_LEVEL_SEL = 1; // 3.7
			nvt_writel(basic_cg_2.reg, reg_addr_sub + BASIC_CG_2_OFS);

			stcs_la_cg_0.bit.LA_CG_RGAIN = dgain;
			stcs_la_cg_0.bit.LA_CG_GGAIN = dgain;
			nvt_writel(stcs_la_cg_0.reg, reg_addr_sub + STCS_LA_CG_0_OFS);

			stcs_la_cg_1.bit.LA_CG_BGAIN = dgain;
			nvt_writel(stcs_la_cg_1.reg, reg_addr_sub + STCS_LA_CG_1_OFS);
		}
	}else{
		// set short exp frame SIE
		sie_control_reg.reg = nvt_readl(reg_addr_m + R4_ENGINE_FUNCTION_OFS);
		sie_control_reg.bit.CGAIN_EN = 0;
		sie_control_reg.bit.DGAIN_EN = 1;
		nvt_writel(sie_control_reg.reg, reg_addr_m + R4_ENGINE_FUNCTION_OFS);

		basic_dgain.bit.DGAIN_GAIN = dgain << 1; // 3.7 to 8.8
		nvt_writel(basic_dgain.reg, reg_addr_m + R15C_BASIC_DGAIN_OFS);

		// set long exp frame SIE
		if (reg_addr_sub != 0) {
			sie_control_reg.reg = nvt_readl(reg_addr_sub + R4_ENGINE_FUNCTION_OFS);
			sie_control_reg.bit.CGAIN_EN = 0;
			sie_control_reg.bit.DGAIN_EN = 1;
			nvt_writel(sie_control_reg.reg, reg_addr_sub + R4_ENGINE_FUNCTION_OFS);

			basic_dgain.bit.DGAIN_GAIN = dgain << 1; // 3.7 to 8.8
			nvt_writel(basic_dgain.reg, reg_addr_sub + R15C_BASIC_DGAIN_OFS);
		}
	}
}

static void iq_builtin_set_cgain(UINT32 id, ISP_BUILTIN_CGAIN *value, IQ_PARAM_PTR *iq_param, UINT32 gain, BOOL ui_night_mode)
{
	ISP_BUILTIN_CGAIN *cgain_temp = (ISP_BUILTIN_CGAIN *) value;
	T_IFE_CGAIN_CONTROL             ife_cgain_control;
	T_IFE_CGAIN_0                   ife_cgain_0;
	T_IFE_CGAIN_1                   ife_cgain_1;
	T_IFE_CGAIN_2                   ife_cgain_2;
	T_COLOR_GAIN_REGISTER_4         color_gain_reg_4;
	T_COLOR_GAIN_REGISTER_5         color_gain_reg_5;
	T_COLOR_GAIN_REGISTER_6         color_gain_reg_6;
	T_COLOR_GAIN_REGISTER_7         color_gain_reg_7;
	T_COLOR_GAIN_REGISTER_8         color_gain_reg_8;
	T_COLOR_GAIN_REGISTER_9         color_gain_reg_9;

	unsigned int                    reg_addr;
	IQ_OB_TUNE_PARAM                *final_ob_tune;

	UINT32                          cgain_r, cgain_g, cgain_b;
	UINT32                          sie_cgain[3] = {0};
	UINT32                          ob_ratio;
	UINT32                          iso_idx = 0;
	INT32                           i;

	reg_addr = (unsigned int)ioremap_nocache(IFE_BASE_ADDR, IFE_SIZE);

	cgain_r = cgain_temp->r;
	cgain_g = cgain_temp->g;
	cgain_b = cgain_temp->b;

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->ob->mode == IQ_OP_TYPE_AUTO) {
		final_ob_tune = &iq_param->ob->auto_param[iso_idx];
	} else {
		final_ob_tune = &iq_param->ob->manual_param;
	}

	ob_ratio = (4096 - ((final_ob_tune->cofs[1] + final_ob_tune->cofs[2]) >> 1));
	sie_cgain[0] = IQ_CLAMP(((cgain_r << 12) + (ob_ratio - 1)) / ob_ratio, 0, 2047);
	sie_cgain[1] = IQ_CLAMP(((cgain_g << 12) + (ob_ratio - 1)) / ob_ratio, 0, 2047);
	sie_cgain[2] = IQ_CLAMP(((cgain_b << 12) + (ob_ratio - 1)) / ob_ratio, 0, 2047);

	if (ui_night_mode == TRUE) {
		sie_cgain[0] = sie_cgain[1];
		sie_cgain[2] = sie_cgain[1];
	}

	if (isp_builtin_get_shdr_enable(id)) {
		ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
		ife_cgain_control.bit.cgain_range = 1; // 3.7
		nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

		ife_cgain_0.bit.ife_cgain_r = 128;
		ife_cgain_0.bit.ife_cgain_gr = 128;
		nvt_writel(ife_cgain_0.reg, reg_addr + IFE_CGAIN_0_OFS);

		ife_cgain_1.bit.ife_cgain_gb = 128;
		ife_cgain_1.bit.ife_cgain_b = 128;
		nvt_writel(ife_cgain_1.reg, reg_addr + IFE_CGAIN_1_OFS);

		ife_cgain_2.bit.ife_cgain_ir = 128;
		nvt_writel(ife_cgain_2.reg, reg_addr + IFE_CGAIN_2_OFS);
	} else {
		if (sie_cgain[0] > 1023 || sie_cgain[2] > 1023) {
			ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
			ife_cgain_control.bit.cgain_range = 1; // 3.7
			nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

			ife_cgain_0.bit.ife_cgain_r = sie_cgain[0] >> 1;
			ife_cgain_0.bit.ife_cgain_gr = sie_cgain[1] >> 1;
			nvt_writel(ife_cgain_0.reg, reg_addr + IFE_CGAIN_0_OFS);

			ife_cgain_1.bit.ife_cgain_gb = sie_cgain[1] >> 1;
			ife_cgain_1.bit.ife_cgain_b = sie_cgain[2] >> 1;
			nvt_writel(ife_cgain_1.reg, reg_addr + IFE_CGAIN_1_OFS);

			ife_cgain_2.bit.ife_cgain_ir = sie_cgain[1] >> 1;
			nvt_writel(ife_cgain_2.reg, reg_addr + IFE_CGAIN_2_OFS);
		} else {
			ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
			ife_cgain_control.bit.cgain_range = 0; // 2.8
			nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

			ife_cgain_0.bit.ife_cgain_r = sie_cgain[0];
			ife_cgain_0.bit.ife_cgain_gr = sie_cgain[1];
			nvt_writel(ife_cgain_0.reg, reg_addr + IFE_CGAIN_0_OFS);

			ife_cgain_1.bit.ife_cgain_gb = sie_cgain[1];
			ife_cgain_1.bit.ife_cgain_b = sie_cgain[2];
			nvt_writel(ife_cgain_1.reg, reg_addr + IFE_CGAIN_1_OFS);

			ife_cgain_2.bit.ife_cgain_ir = sie_cgain[1];
			nvt_writel(ife_cgain_2.reg, reg_addr + IFE_CGAIN_2_OFS);
		}
	}

	if (isp_builtin_get_shdr_enable(id)) {
		if (sie_cgain[0] > 1023 || sie_cgain[2] > 1023) {
			ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
			ife_cgain_control.bit.ife_f_cgain_range = 1; // 3.7
			nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

			color_gain_reg_4.bit.ife_f_p0_cgain_r = sie_cgain[0] >> 1;
			color_gain_reg_4.bit.ife_f_p0_cgain_gr = sie_cgain[1] >> 1;
			nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_4_OFS);

			color_gain_reg_5.bit.ife_f_p0_cgain_gb = sie_cgain[1] >> 1;
			color_gain_reg_5.bit.ife_f_p0_cgain_b = sie_cgain[2] >> 1;
			nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_5_OFS);

			color_gain_reg_6.bit.ife_f_p0_cgain_ir = sie_cgain[1] >> 1;
			nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_6_OFS);

			color_gain_reg_7.bit.ife_f_p1_cgain_r = sie_cgain[0] >> 1;
			color_gain_reg_7.bit.ife_f_p1_cgain_gr = sie_cgain[1] >> 1;
			nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_7_OFS);

			color_gain_reg_8.bit.ife_f_p1_cgain_gb = sie_cgain[1] >> 1;
			color_gain_reg_8.bit.ife_f_p1_cgain_b = sie_cgain[2] >> 1;
			nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_8_OFS);

			color_gain_reg_9.bit.ife_f_p1_cgain_ir = sie_cgain[1] >> 1;
			nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_9_OFS);
		} else {
			ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
			ife_cgain_control.bit.ife_f_cgain_range = 0; // 2.8
			nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

			color_gain_reg_4.bit.ife_f_p0_cgain_r = sie_cgain[0];
			color_gain_reg_4.bit.ife_f_p0_cgain_gr = sie_cgain[1];
			nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_4_OFS);

			color_gain_reg_5.bit.ife_f_p0_cgain_gb = sie_cgain[1];
			color_gain_reg_5.bit.ife_f_p0_cgain_b = sie_cgain[2];
			nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_5_OFS);

			color_gain_reg_6.bit.ife_f_p0_cgain_ir = sie_cgain[1];
			nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_6_OFS);

			color_gain_reg_7.bit.ife_f_p1_cgain_r = sie_cgain[0];
			color_gain_reg_7.bit.ife_f_p1_cgain_gr = sie_cgain[1];
			nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_7_OFS);

			color_gain_reg_8.bit.ife_f_p1_cgain_gb = sie_cgain[1];
			color_gain_reg_8.bit.ife_f_p1_cgain_b = sie_cgain[2];
			nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_8_OFS);

			color_gain_reg_9.bit.ife_f_p1_cgain_ir = sie_cgain[1];
			nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_9_OFS);
		}
	} else {
		ife_cgain_control.reg = nvt_readl(reg_addr + IFE_CGAIN_CONTROL_OFS);
		ife_cgain_control.bit.ife_f_cgain_range = 1; // 3.7
		nvt_writel(ife_cgain_control.reg, reg_addr + IFE_CGAIN_CONTROL_OFS);

		color_gain_reg_4.bit.ife_f_p0_cgain_r = 128;
		color_gain_reg_4.bit.ife_f_p0_cgain_gr = 128;
		nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_4_OFS);

		color_gain_reg_5.bit.ife_f_p0_cgain_gb = 128;
		color_gain_reg_5.bit.ife_f_p0_cgain_b = 128;
		nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_5_OFS);

		color_gain_reg_6.bit.ife_f_p0_cgain_ir = 128;
		nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_6_OFS);

		color_gain_reg_7.bit.ife_f_p1_cgain_r = 128;
		color_gain_reg_7.bit.ife_f_p1_cgain_gr = 128;
		nvt_writel(color_gain_reg_4.reg, reg_addr + COLOR_GAIN_REGISTER_7_OFS);

		color_gain_reg_8.bit.ife_f_p1_cgain_gb = 128;
		color_gain_reg_8.bit.ife_f_p1_cgain_b = 128;
		nvt_writel(color_gain_reg_5.reg, reg_addr + COLOR_GAIN_REGISTER_8_OFS);

		color_gain_reg_9.bit.ife_f_p1_cgain_ir = 128;
		nvt_writel(color_gain_reg_6.reg, reg_addr + COLOR_GAIN_REGISTER_9_OFS);
	}
}

static void iq_builtin_set_shdr(UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	T_NRS_REGISTER                                                 nrs_reg_control;
	T_RHE_NRS_S_REGISTER                                           nrs_s_reg;
	T_RHE_NRS_BILATERAL_LUT_OFFSET0                                nrs_b_lut_offset0;
	T_RHE_NRS_BILATERAL_LUT_OFFSET1                                nrs_b_lut_offset1;
	T_RHE_NRS_BILATERAL_LUT_RANGE2                                 nrs_b_lut_th1;
	T_RHE_NRS_GBILATERAL_LUT_OFFSET0                               nrs_g_lut_offset0;
	T_RHE_NRS_GBILATERAL_LUT_OFFSET1                               nrs_g_lut_offset1;
	T_RHE_NRS_GBILATERAL_LUT_RANGE2                                nrs_g_lut_th1;

	T_RHE_FUSION_REGISTER                                          fusion_reg;
	T_RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE                       long_exp_nor;
	T_RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE                      short_exp_nor;
	T_RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE                         long_exp_diff;
	T_RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE                        short_exp_diff;

	T_RHE_FUSION_MOTION_COMPENSATION                               fusion_mc_reg;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_0                         fusion_mc_pos_lut_reg0;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_1                         fusion_mc_pos_lut_reg1;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_2                         fusion_mc_pos_lut_reg2;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_3                         fusion_mc_pos_lut_reg3;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_4                         fusion_mc_pos_lut_reg4;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_5                         fusion_mc_pos_lut_reg5;

	T_RHE_FCURVE_CTRL                                              fcurve_control_reg;
	T_RHE_FCURVE_INDEX_REGISTER0                                   fcurve_idx_lut0;
	T_RHE_FCURVE_INDEX_REGISTER1                                   fcurve_idx_lut1;
	T_RHE_FCURVE_INDEX_REGISTER2                                   fcurve_idx_lut2;
	T_RHE_FCURVE_INDEX_REGISTER3                                   fcurve_idx_lut3;
	T_RHE_FCURVE_INDEX_REGISTER4                                   fcurve_idx_lut4;
	T_RHE_FCURVE_INDEX_REGISTER5                                   fcurve_idx_lut5;
	T_RHE_FCURVE_INDEX_REGISTER6                                   fcurve_idx_lut6;
	T_RHE_FCURVE_INDEX_REGISTER7                                   fcurve_idx_lut7;

	T_RHE_FCURVE_SPLIT_REGISTER0                                   fcurve_spilt_lut0;
	T_RHE_FCURVE_SPLIT_REGISTER1                                   fcurve_spilt_lut1;

	T_RHE_FCURVE_VALUE_REGISTER0                                   fcurve_val_lut0;
	T_RHE_FCURVE_VALUE_REGISTER1                                   fcurve_val_lut1;
	T_RHE_FCURVE_VALUE_REGISTER2                                   fcurve_val_lut2;
	T_RHE_FCURVE_VALUE_REGISTER3                                   fcurve_val_lut3;
	T_RHE_FCURVE_VALUE_REGISTER4                                   fcurve_val_lut4;
	T_RHE_FCURVE_VALUE_REGISTER5                                   fcurve_val_lut5;
	T_RHE_FCURVE_VALUE_REGISTER6                                   fcurve_val_lut6;
	T_RHE_FCURVE_VALUE_REGISTER7                                   fcurve_val_lut7;
	T_RHE_FCURVE_VALUE_REGISTER8                                   fcurve_val_lut8;
	T_RHE_FCURVE_VALUE_REGISTER9                                   fcurve_val_lut9;
	T_RHE_FCURVE_VALUE_REGISTER10                                  fcurve_val_lut10;
	T_RHE_FCURVE_VALUE_REGISTER11                                  fcurve_val_lut11;
	T_RHE_FCURVE_VALUE_REGISTER12                                  fcurve_val_lut12;
	T_RHE_FCURVE_VALUE_REGISTER13                                  fcurve_val_lut13;
	T_RHE_FCURVE_VALUE_REGISTER14                                  fcurve_val_lut14;
	T_RHE_FCURVE_VALUE_REGISTER15                                  fcurve_val_lut15;
	T_RHE_FCURVE_VALUE_REGISTER16                                  fcurve_val_lut16;
	T_RHE_FCURVE_VALUE_REGISTER17                                  fcurve_val_lut17;
	T_RHE_FCURVE_VALUE_REGISTER18                                  fcurve_val_lut18;
	T_RHE_FCURVE_VALUE_REGISTER19                                  fcurve_val_lut19;
	T_RHE_FCURVE_VALUE_REGISTER20                                  fcurve_val_lut20;
	T_RHE_FCURVE_VALUE_REGISTER21                                  fcurve_val_lut21;
	T_RHE_FCURVE_VALUE_REGISTER22                                  fcurve_val_lut22;
	T_RHE_FCURVE_VALUE_REGISTER23                                  fcurve_val_lut23;
	T_RHE_FCURVE_VALUE_REGISTER24                                  fcurve_val_lut24;
	T_RHE_FCURVE_VALUE_REGISTER25                                  fcurve_val_lut25;
	T_RHE_FCURVE_VALUE_REGISTER26                                  fcurve_val_lut26;
	T_RHE_FCURVE_VALUE_REGISTER27                                  fcurve_val_lut27;
	T_RHE_FCURVE_VALUE_REGISTER28                                  fcurve_val_lut28;
	T_RHE_FCURVE_VALUE_REGISTER29                                  fcurve_val_lut29;
	T_RHE_FCURVE_VALUE_REGISTER30                                  fcurve_val_lut30;
	T_RHE_FCURVE_VALUE_REGISTER31                                  fcurve_val_lut31;
	T_RHE_FCURVE_VALUE_REGISTER32                                  fcurve_val_lut32;

	unsigned int                                                   reg_addr;
	IQ_SHDR_PARAM                                                  *shdr;
	IQ_SHDR_TUNE_PARAM                                             *final_shdr_tune;

	UINT32                                                         iso_idx = 0;
	INT32                                                          i;

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	shdr = iq_param->shdr;
	if (shdr->mode == IQ_OP_TYPE_AUTO) {
		final_shdr_tune = &shdr->auto_param[iso_idx];
	} else {
		final_shdr_tune = &shdr->manual_param;
	}

	reg_addr = (unsigned int)ioremap_nocache(IFE_BASE_ADDR, IFE_SIZE);

	nrs_reg_control.reg = nvt_readl(reg_addr + NRS_REGISTER_OFS);
	nrs_reg_control.bit.ife_f_nrs_ord_en = shdr->nrs_enable;
	nrs_reg_control.bit.ife_f_nrs_bilat_en = shdr->nrs_enable;
	nrs_reg_control.bit.ife_f_nrs_gbilat_en = shdr->nrs_enable;
	nvt_writel(nrs_reg_control.reg, reg_addr + NRS_REGISTER_OFS);

	nrs_s_reg.reg = nvt_readl(reg_addr + RHE_NRS_S_REGISTER_OFS);
	nrs_s_reg.bit.ife_f_nrs_ord_diff_thr = final_shdr_tune->nrs_diff_th;
	nvt_writel(nrs_s_reg.reg, reg_addr + RHE_NRS_S_REGISTER_OFS);

	nrs_b_lut_offset0.reg = nvt_readl(reg_addr + RHE_NRS_BILATERAL_LUT_OFFSET0_OFS);
	nrs_b_lut_offset0.bit.ife_f_nrs_bilat_lut_offset0 = final_shdr_tune->nrs_ofs[0];
	nrs_b_lut_offset0.bit.ife_f_nrs_bilat_lut_offset1 = final_shdr_tune->nrs_ofs[1];
	nrs_b_lut_offset0.bit.ife_f_nrs_bilat_lut_offset2 = final_shdr_tune->nrs_ofs[2];
	nrs_b_lut_offset0.bit.ife_f_nrs_bilat_lut_offset3 = final_shdr_tune->nrs_ofs[3];
	nvt_writel(nrs_b_lut_offset0.reg, reg_addr + RHE_NRS_BILATERAL_LUT_OFFSET0_OFS);

	nrs_b_lut_offset1.reg = nvt_readl(reg_addr + RHE_NRS_BILATERAL_LUT_OFFSET1_OFS);
	nrs_b_lut_offset1.bit.ife_f_nrs_bilat_lut_offset4 = final_shdr_tune->nrs_ofs[4];
	nrs_b_lut_offset1.bit.ife_f_nrs_bilat_lut_offset5 = final_shdr_tune->nrs_ofs[5];
	nvt_writel(nrs_b_lut_offset1.reg, reg_addr + RHE_NRS_BILATERAL_LUT_OFFSET1_OFS);

	nrs_b_lut_th1.reg = nvt_readl(reg_addr + RHE_NRS_BILATERAL_LUT_RANGE2_OFS);
	nrs_b_lut_th1.bit.ife_f_nrs_bilat_lut_th1 = final_shdr_tune->nrs_th[0];
	nrs_b_lut_th1.bit.ife_f_nrs_bilat_lut_th2 = final_shdr_tune->nrs_th[1];
	nrs_b_lut_th1.bit.ife_f_nrs_bilat_lut_th3 = final_shdr_tune->nrs_th[2];
	nrs_b_lut_th1.bit.ife_f_nrs_bilat_lut_th4 = final_shdr_tune->nrs_th[3];
	nrs_b_lut_th1.bit.ife_f_nrs_bilat_lut_th5 = final_shdr_tune->nrs_th[4];
	nvt_writel(nrs_b_lut_th1.reg, reg_addr + RHE_NRS_BILATERAL_LUT_RANGE2_OFS);

	nrs_g_lut_offset0.reg = nvt_readl(reg_addr + RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS);
	nrs_g_lut_offset0.bit.ife_f_nrs_gbilat_lut_offset0 = final_shdr_tune->nrs_ofs[0];
	nrs_g_lut_offset0.bit.ife_f_nrs_gbilat_lut_offset1 = final_shdr_tune->nrs_ofs[1];
	nrs_g_lut_offset0.bit.ife_f_nrs_gbilat_lut_offset2 = final_shdr_tune->nrs_ofs[2];
	nrs_g_lut_offset0.bit.ife_f_nrs_gbilat_lut_offset3 = final_shdr_tune->nrs_ofs[3];
	nvt_writel(nrs_g_lut_offset0.reg, reg_addr + RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS);

	nrs_g_lut_offset1.reg = nvt_readl(reg_addr + RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS);
	nrs_g_lut_offset1.bit.ife_f_nrs_gbilat_lut_offset4 = final_shdr_tune->nrs_ofs[4];
	nrs_g_lut_offset1.bit.ife_f_nrs_gbilat_lut_offset5 = final_shdr_tune->nrs_ofs[5];
	nvt_writel(nrs_g_lut_offset1.reg, reg_addr + RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS);

	nrs_g_lut_th1.reg = nvt_readl(reg_addr + RHE_NRS_GBILATERAL_LUT_RANGE2_OFS);
	nrs_g_lut_th1.bit.ife_f_nrs_gbilat_lut_th1 = final_shdr_tune->nrs_th[0];
	nrs_g_lut_th1.bit.ife_f_nrs_gbilat_lut_th2 = final_shdr_tune->nrs_th[1];
	nrs_g_lut_th1.bit.ife_f_nrs_gbilat_lut_th3 = final_shdr_tune->nrs_th[2];
	nrs_g_lut_th1.bit.ife_f_nrs_gbilat_lut_th4 = final_shdr_tune->nrs_th[3];
	nrs_g_lut_th1.bit.ife_f_nrs_gbilat_lut_th5 = final_shdr_tune->nrs_th[4];
	nvt_writel(nrs_g_lut_th1.reg, reg_addr + RHE_NRS_GBILATERAL_LUT_RANGE2_OFS);

	fusion_reg.reg = nvt_readl(reg_addr + RHE_FUSION_REGISTER_OFS);
	fusion_reg.bit.ife_f_fusion_normal_blend_curve_sel = shdr->fusion_nor_sel;
	fusion_reg.bit.ife_f_fusion_diff_blend_curve_sel = shdr->fusion_dif_sel;
	fusion_reg.bit.ife_f_fusion_mode = 0; // 0:blend, 1:short, 3:long
	if (fusion_reg.bit.ife_f_fusion_mode == 1) {
		memcpy(shdr->fcurve_idx_lut, iq_shdr_dbg_fcurve_idx_lut, sizeof(UINT8) * IQ_SHDR_FCURVE_IDX_NUM);
		memcpy(shdr->fcurve_split_lut, iq_shdr_dbg_fcurve_split_lut, sizeof(UINT8) * IQ_SHDR_FCURVE_SPLIT_NUM);
		memcpy(shdr->fcurve_val_lut, iq_shdr_dbg_short_fcurve_val_lut, sizeof(UINT16) * IQ_SHDR_FCURVE_VAL_NUM);
	} else if (fusion_reg.bit.ife_f_fusion_mode == 3) {
		memcpy(shdr->fcurve_idx_lut, iq_shdr_dbg_fcurve_idx_lut, sizeof(UINT8) * IQ_SHDR_FCURVE_IDX_NUM);
		memcpy(shdr->fcurve_split_lut, iq_shdr_dbg_fcurve_split_lut, sizeof(UINT8) * IQ_SHDR_FCURVE_SPLIT_NUM);
		memcpy(shdr->fcurve_val_lut, iq_shdr_dbg_long_fcurve_val_lut, sizeof(UINT16) * IQ_SHDR_FCURVE_VAL_NUM);
	}
	nvt_writel(fusion_reg.reg, reg_addr + RHE_FUSION_REGISTER_OFS);

	fusion_reg.reg = nvt_readl(reg_addr + RHE_FUSION_REGISTER_OFS);
	fusion_reg.bit.ife_f_fusion_normal_blend_curve_sel = shdr->fusion_nor_sel;
	fusion_reg.bit.ife_f_fusion_diff_blend_curve_sel = shdr->fusion_dif_sel;
	nvt_writel(fusion_reg.reg, reg_addr + RHE_FUSION_REGISTER_OFS);

	long_exp_nor.reg = nvt_readl(reg_addr + RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS);
	long_exp_nor.bit.ife_f_fusion_long_exp_normal_blend_curve_knee_point0 = shdr->fusion_l_nor_knee;
	long_exp_nor.bit.ife_f_fusion_long_exp_normal_blend_curve_range = shdr->fusion_l_nor_range;
	nvt_writel(long_exp_nor.reg, reg_addr + RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS);

	short_exp_nor.reg = nvt_readl(reg_addr + RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS);
	short_exp_nor.bit.ife_f_fusion_short_exp_normal_blend_curve_knee_point0 = shdr->fusion_s_nor_knee;
	short_exp_nor.bit.ife_f_fusion_short_exp_normal_blend_curve_range = shdr->fusion_s_nor_range;
	nvt_writel(short_exp_nor.reg, reg_addr + RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS);

	long_exp_diff.reg = nvt_readl(reg_addr + RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS);
	long_exp_diff.bit.ife_f_fusion_long_exp_diff_blend_curve_knee_point0 = shdr->fusion_l_dif_knee;
	long_exp_diff.bit.ife_f_fusion_long_exp_diff_blend_curve_range = shdr->fusion_l_dif_range;
	nvt_writel(long_exp_diff.reg, reg_addr + RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS);

	short_exp_diff.reg = nvt_readl(reg_addr + RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS);
	short_exp_diff.bit.ife_f_fusion_short_exp_diff_blend_curve_knee_point0 = shdr->fusion_s_dif_knee;
	short_exp_diff.bit.ife_f_fusion_short_exp_diff_blend_curve_range = shdr->fusion_s_dif_range;
	nvt_writel(short_exp_diff.reg, reg_addr + RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS);

	fusion_mc_reg.reg = nvt_readl(reg_addr + RHE_FUSION_MOTION_COMPENSATION_OFS);
	fusion_mc_reg.bit.ife_f_fusion_mc_lumthr = shdr->fusion_lum_th;
	nvt_writel(fusion_mc_reg.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_OFS);

	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight0 = shdr->fusion_diff_w[0];
	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight1 = shdr->fusion_diff_w[1];
	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight2 = shdr->fusion_diff_w[2];
	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight3 = shdr->fusion_diff_w[3];
	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight4 = shdr->fusion_diff_w[4];
	fusion_mc_pos_lut_reg0.bit.ife_f_fusion_mc_lut_positive_diff_weight5 = shdr->fusion_diff_w[5];
	nvt_writel(fusion_mc_pos_lut_reg0.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS);

	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight6 = shdr->fusion_diff_w[6];
	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight7 = shdr->fusion_diff_w[7];
	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight8 = shdr->fusion_diff_w[8];
	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight9 = shdr->fusion_diff_w[9];
	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight10 = shdr->fusion_diff_w[10];
	fusion_mc_pos_lut_reg1.bit.ife_f_fusion_mc_lut_positive_diff_weight11 = shdr->fusion_diff_w[11];
	nvt_writel(fusion_mc_pos_lut_reg1.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS);

	fusion_mc_pos_lut_reg2.reg = nvt_readl(reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS);
	fusion_mc_pos_lut_reg2.bit.ife_f_fusion_mc_lut_positive_diff_weight12 = shdr->fusion_diff_w[12];
	fusion_mc_pos_lut_reg2.bit.ife_f_fusion_mc_lut_positive_diff_weight13 = shdr->fusion_diff_w[13];
	fusion_mc_pos_lut_reg2.bit.ife_f_fusion_mc_lut_positive_diff_weight14 = shdr->fusion_diff_w[14];
	fusion_mc_pos_lut_reg2.bit.ife_f_fusion_mc_lut_positive_diff_weight15 = shdr->fusion_diff_w[15];
	nvt_writel(fusion_mc_pos_lut_reg2.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS);

	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight0 = shdr->fusion_diff_w[0];
	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight1 = shdr->fusion_diff_w[1];
	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight2 = shdr->fusion_diff_w[2];
	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight3 = shdr->fusion_diff_w[3];
	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight4 = shdr->fusion_diff_w[4];
	fusion_mc_pos_lut_reg3.bit.ife_f_fusion_mc_lut_negative_diff_weight5 = shdr->fusion_diff_w[5];
	nvt_writel(fusion_mc_pos_lut_reg3.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS);

	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight6 = shdr->fusion_diff_w[6];
	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight7 = shdr->fusion_diff_w[7];
	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight8 = shdr->fusion_diff_w[8];
	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight9 = shdr->fusion_diff_w[9];
	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight10 = shdr->fusion_diff_w[10];
	fusion_mc_pos_lut_reg4.bit.ife_f_fusion_mc_lut_negative_diff_weight11 = shdr->fusion_diff_w[11];
	nvt_writel(fusion_mc_pos_lut_reg4.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS);

	fusion_mc_pos_lut_reg5.reg = nvt_readl(reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS);
	fusion_mc_pos_lut_reg5.bit.ife_f_fusion_mc_lut_negative_diff_weight12 = shdr->fusion_diff_w[12];
	fusion_mc_pos_lut_reg5.bit.ife_f_fusion_mc_lut_negative_diff_weight13 = shdr->fusion_diff_w[13];
	fusion_mc_pos_lut_reg5.bit.ife_f_fusion_mc_lut_negative_diff_weight14 = shdr->fusion_diff_w[14];
	fusion_mc_pos_lut_reg5.bit.ife_f_fusion_mc_lut_negative_diff_weight15 = shdr->fusion_diff_w[15];
	nvt_writel(fusion_mc_pos_lut_reg5.reg, reg_addr + RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS);

	fcurve_control_reg.bit.ife_f_fcurve_ymean_select = shdr->fcurve_y_mean_sel;
	fcurve_control_reg.bit.ife_f_fcurve_yvweight = shdr->fcurve_yv_w;
	nvt_writel(fcurve_control_reg.reg, reg_addr + RHE_FCURVE_CTRL_OFS);

	fcurve_idx_lut0.bit.ife_f_fcurve_index_lut0 = shdr->fcurve_idx_lut[0];
	fcurve_idx_lut0.bit.ife_f_fcurve_index_lut1 = shdr->fcurve_idx_lut[1];
	fcurve_idx_lut0.bit.ife_f_fcurve_index_lut2 = shdr->fcurve_idx_lut[2];
	fcurve_idx_lut0.bit.ife_f_fcurve_index_lut3 = shdr->fcurve_idx_lut[3];
	nvt_writel(fcurve_idx_lut0.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER0_OFS);

	fcurve_idx_lut1.bit.ife_f_fcurve_index_lut4 = shdr->fcurve_idx_lut[4];
	fcurve_idx_lut1.bit.ife_f_fcurve_index_lut5 = shdr->fcurve_idx_lut[5];
	fcurve_idx_lut1.bit.ife_f_fcurve_index_lut6 = shdr->fcurve_idx_lut[6];
	fcurve_idx_lut1.bit.ife_f_fcurve_index_lut7 = shdr->fcurve_idx_lut[7];
	nvt_writel(fcurve_idx_lut1.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER1_OFS);

	fcurve_idx_lut2.bit.ife_f_fcurve_index_lut8 = shdr->fcurve_idx_lut[8];
	fcurve_idx_lut2.bit.ife_f_fcurve_index_lut9 = shdr->fcurve_idx_lut[9];
	fcurve_idx_lut2.bit.ife_f_fcurve_index_lut10 = shdr->fcurve_idx_lut[10];
	fcurve_idx_lut2.bit.ife_f_fcurve_index_lut11 = shdr->fcurve_idx_lut[11];
	nvt_writel(fcurve_idx_lut2.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER2_OFS);

	fcurve_idx_lut3.bit.ife_f_fcurve_index_lut12 = shdr->fcurve_idx_lut[12];
	fcurve_idx_lut3.bit.ife_f_fcurve_index_lut13 = shdr->fcurve_idx_lut[13];
	fcurve_idx_lut3.bit.ife_f_fcurve_index_lut14 = shdr->fcurve_idx_lut[14];
	fcurve_idx_lut3.bit.ife_f_fcurve_index_lut15 = shdr->fcurve_idx_lut[15];
	nvt_writel(fcurve_idx_lut3.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER3_OFS);

	fcurve_idx_lut4.bit.ife_f_fcurve_index_lut16 = shdr->fcurve_idx_lut[16];
	fcurve_idx_lut4.bit.ife_f_fcurve_index_lut17 = shdr->fcurve_idx_lut[17];
	fcurve_idx_lut4.bit.ife_f_fcurve_index_lut18 = shdr->fcurve_idx_lut[18];
	fcurve_idx_lut4.bit.ife_f_fcurve_index_lut19 = shdr->fcurve_idx_lut[19];
	nvt_writel(fcurve_idx_lut4.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER4_OFS);

	fcurve_idx_lut5.bit.ife_f_fcurve_index_lut20 = shdr->fcurve_idx_lut[20];
	fcurve_idx_lut5.bit.ife_f_fcurve_index_lut21 = shdr->fcurve_idx_lut[21];
	fcurve_idx_lut5.bit.ife_f_fcurve_index_lut22 = shdr->fcurve_idx_lut[22];
	fcurve_idx_lut5.bit.ife_f_fcurve_index_lut23 = shdr->fcurve_idx_lut[23];
	nvt_writel(fcurve_idx_lut5.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER5_OFS);

	fcurve_idx_lut6.bit.ife_f_fcurve_index_lut24 = shdr->fcurve_idx_lut[24];
	fcurve_idx_lut6.bit.ife_f_fcurve_index_lut25 = shdr->fcurve_idx_lut[25];
	fcurve_idx_lut6.bit.ife_f_fcurve_index_lut26 = shdr->fcurve_idx_lut[26];
	fcurve_idx_lut6.bit.ife_f_fcurve_index_lut27 = shdr->fcurve_idx_lut[27];
	nvt_writel(fcurve_idx_lut6.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER6_OFS);

	fcurve_idx_lut7.bit.ife_f_fcurve_index_lut28 = shdr->fcurve_idx_lut[28];
	fcurve_idx_lut7.bit.ife_f_fcurve_index_lut29 = shdr->fcurve_idx_lut[29];
	fcurve_idx_lut7.bit.ife_f_fcurve_index_lut30 = shdr->fcurve_idx_lut[30];
	fcurve_idx_lut7.bit.ife_f_fcurve_index_lut31 = shdr->fcurve_idx_lut[31];
	nvt_writel(fcurve_idx_lut7.reg, reg_addr + RHE_FCURVE_INDEX_REGISTER7_OFS);

	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut0 = shdr->fcurve_split_lut[0];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut1 = shdr->fcurve_split_lut[1];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut2 = shdr->fcurve_split_lut[2];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut3 = shdr->fcurve_split_lut[3];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut4 = shdr->fcurve_split_lut[4];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut5 = shdr->fcurve_split_lut[5];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut6 = shdr->fcurve_split_lut[6];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut7 = shdr->fcurve_split_lut[7];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut8 = shdr->fcurve_split_lut[8];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut9 = shdr->fcurve_split_lut[9];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut10 = shdr->fcurve_split_lut[10];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut11 = shdr->fcurve_split_lut[11];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut12 = shdr->fcurve_split_lut[12];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut13 = shdr->fcurve_split_lut[13];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut14 = shdr->fcurve_split_lut[14];
	fcurve_spilt_lut0.bit.ife_f_fcurve_split_lut15 = shdr->fcurve_split_lut[15];
	nvt_writel(fcurve_spilt_lut0.reg, reg_addr + RHE_FCURVE_SPLIT_REGISTER0_OFS);

	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut16 = shdr->fcurve_split_lut[16];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut17 = shdr->fcurve_split_lut[17];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut18 = shdr->fcurve_split_lut[18];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut19 = shdr->fcurve_split_lut[19];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut20 = shdr->fcurve_split_lut[20];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut21 = shdr->fcurve_split_lut[21];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut22 = shdr->fcurve_split_lut[22];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut23 = shdr->fcurve_split_lut[23];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut24 = shdr->fcurve_split_lut[24];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut25 = shdr->fcurve_split_lut[25];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut26 = shdr->fcurve_split_lut[26];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut27 = shdr->fcurve_split_lut[27];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut28 = shdr->fcurve_split_lut[28];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut29 = shdr->fcurve_split_lut[29];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut30 = shdr->fcurve_split_lut[30];
	fcurve_spilt_lut1.bit.ife_f_fcurve_split_lut31 = shdr->fcurve_split_lut[31];
	nvt_writel(fcurve_spilt_lut1.reg, reg_addr + RHE_FCURVE_SPLIT_REGISTER1_OFS);

	fcurve_val_lut0.bit.ife_f_fcurve_val_lut0 = shdr->fcurve_val_lut[0];
	fcurve_val_lut0.bit.ife_f_fcurve_val_lut1 = shdr->fcurve_val_lut[1];
	nvt_writel(fcurve_val_lut0.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER0_OFS);

	fcurve_val_lut1.bit.ife_f_fcurve_val_lut2 = shdr->fcurve_val_lut[2];
	fcurve_val_lut1.bit.ife_f_fcurve_val_lut3 = shdr->fcurve_val_lut[3];
	nvt_writel(fcurve_val_lut1.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER1_OFS);

	fcurve_val_lut2.bit.ife_f_fcurve_val_lut4 = shdr->fcurve_val_lut[4];
	fcurve_val_lut2.bit.ife_f_fcurve_val_lut5 = shdr->fcurve_val_lut[5];
	nvt_writel(fcurve_val_lut2.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER2_OFS);

	fcurve_val_lut3.bit.ife_f_fcurve_val_lut6 = shdr->fcurve_val_lut[6];
	fcurve_val_lut3.bit.ife_f_fcurve_val_lut7 = shdr->fcurve_val_lut[7];
	nvt_writel(fcurve_val_lut3.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER3_OFS);

	fcurve_val_lut4.bit.ife_f_fcurve_val_lut8 = shdr->fcurve_val_lut[8];
	fcurve_val_lut4.bit.ife_f_fcurve_val_lut9 = shdr->fcurve_val_lut[9];
	nvt_writel(fcurve_val_lut4.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER4_OFS);

	fcurve_val_lut5.bit.ife_f_fcurve_val_lut10 = shdr->fcurve_val_lut[10];
	fcurve_val_lut5.bit.ife_f_fcurve_val_lut11 = shdr->fcurve_val_lut[11];
	nvt_writel(fcurve_val_lut5.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER5_OFS);

	fcurve_val_lut6.bit.ife_f_fcurve_val_lut12 = shdr->fcurve_val_lut[12];
	fcurve_val_lut6.bit.ife_f_fcurve_val_lut13 = shdr->fcurve_val_lut[13];
	nvt_writel(fcurve_val_lut6.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER6_OFS);

	fcurve_val_lut7.bit.ife_f_fcurve_val_lut14 = shdr->fcurve_val_lut[14];
	fcurve_val_lut7.bit.ife_f_fcurve_val_lut15 = shdr->fcurve_val_lut[15];
	nvt_writel(fcurve_val_lut7.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER7_OFS);

	fcurve_val_lut8.bit.ife_f_fcurve_val_lut16 = shdr->fcurve_val_lut[16];
	fcurve_val_lut8.bit.ife_f_fcurve_val_lut17 = shdr->fcurve_val_lut[17];
	nvt_writel(fcurve_val_lut8.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER8_OFS);

	fcurve_val_lut9.bit.ife_f_fcurve_val_lut18 = shdr->fcurve_val_lut[18];
	fcurve_val_lut9.bit.ife_f_fcurve_val_lut19 = shdr->fcurve_val_lut[19];
	nvt_writel(fcurve_val_lut9.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER9_OFS);

	fcurve_val_lut10.bit.ife_f_fcurve_val_lut20 = shdr->fcurve_val_lut[20];
	fcurve_val_lut10.bit.ife_f_fcurve_val_lut21 = shdr->fcurve_val_lut[21];
	nvt_writel(fcurve_val_lut10.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER10_OFS);

	fcurve_val_lut11.bit.ife_f_fcurve_val_lut22 = shdr->fcurve_val_lut[22];
	fcurve_val_lut11.bit.ife_f_fcurve_val_lut23 = shdr->fcurve_val_lut[23];
	nvt_writel(fcurve_val_lut11.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER11_OFS);

	fcurve_val_lut12.bit.ife_f_fcurve_val_lut24 = shdr->fcurve_val_lut[24];
	fcurve_val_lut12.bit.ife_f_fcurve_val_lut25 = shdr->fcurve_val_lut[25];
	nvt_writel(fcurve_val_lut12.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER12_OFS);

	fcurve_val_lut13.bit.ife_f_fcurve_val_lut26 = shdr->fcurve_val_lut[26];
	fcurve_val_lut13.bit.ife_f_fcurve_val_lut27 = shdr->fcurve_val_lut[27];
	nvt_writel(fcurve_val_lut13.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER13_OFS);

	fcurve_val_lut14.bit.ife_f_fcurve_val_lut28 = shdr->fcurve_val_lut[28];
	fcurve_val_lut14.bit.ife_f_fcurve_val_lut29 = shdr->fcurve_val_lut[29];
	nvt_writel(fcurve_val_lut14.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER14_OFS);

	fcurve_val_lut15.bit.ife_f_fcurve_val_lut30 = shdr->fcurve_val_lut[30];
	fcurve_val_lut15.bit.ife_f_fcurve_val_lut31 = shdr->fcurve_val_lut[31];
	nvt_writel(fcurve_val_lut15.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER15_OFS);

	fcurve_val_lut16.bit.ife_f_fcurve_val_lut32 = shdr->fcurve_val_lut[32];
	fcurve_val_lut16.bit.ife_f_fcurve_val_lut33 = shdr->fcurve_val_lut[33];
	nvt_writel(fcurve_val_lut16.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER16_OFS);

	fcurve_val_lut17.bit.ife_f_fcurve_val_lut34 = shdr->fcurve_val_lut[34];
	fcurve_val_lut17.bit.ife_f_fcurve_val_lut35 = shdr->fcurve_val_lut[35];
	nvt_writel(fcurve_val_lut17.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER17_OFS);

	fcurve_val_lut18.bit.ife_f_fcurve_val_lut36 = shdr->fcurve_val_lut[36];
	fcurve_val_lut18.bit.ife_f_fcurve_val_lut37 = shdr->fcurve_val_lut[37];
	nvt_writel(fcurve_val_lut18.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER18_OFS);

	fcurve_val_lut19.bit.ife_f_fcurve_val_lut38 = shdr->fcurve_val_lut[38];
	fcurve_val_lut19.bit.ife_f_fcurve_val_lut39 = shdr->fcurve_val_lut[39];
	nvt_writel(fcurve_val_lut19.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER19_OFS);

	fcurve_val_lut20.bit.ife_f_fcurve_val_lut40 = shdr->fcurve_val_lut[40];
	fcurve_val_lut20.bit.ife_f_fcurve_val_lut41 = shdr->fcurve_val_lut[41];
	nvt_writel(fcurve_val_lut20.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER20_OFS);

	fcurve_val_lut21.bit.ife_f_fcurve_val_lut42 = shdr->fcurve_val_lut[42];
	fcurve_val_lut21.bit.ife_f_fcurve_val_lut43 = shdr->fcurve_val_lut[43];
	nvt_writel(fcurve_val_lut21.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER21_OFS);

	fcurve_val_lut22.bit.ife_f_fcurve_val_lut44 = shdr->fcurve_val_lut[44];
	fcurve_val_lut22.bit.ife_f_fcurve_val_lut45 = shdr->fcurve_val_lut[45];
	nvt_writel(fcurve_val_lut22.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER22_OFS);

	fcurve_val_lut23.bit.ife_f_fcurve_val_lut46 = shdr->fcurve_val_lut[46];
	fcurve_val_lut23.bit.ife_f_fcurve_val_lut47 = shdr->fcurve_val_lut[47];
	nvt_writel(fcurve_val_lut23.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER23_OFS);

	fcurve_val_lut24.bit.ife_f_fcurve_val_lut48 = shdr->fcurve_val_lut[48];
	fcurve_val_lut24.bit.ife_f_fcurve_val_lut49 = shdr->fcurve_val_lut[49];
	nvt_writel(fcurve_val_lut24.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER24_OFS);

	fcurve_val_lut25.bit.ife_f_fcurve_val_lut50 = shdr->fcurve_val_lut[50];
	fcurve_val_lut25.bit.ife_f_fcurve_val_lut51 = shdr->fcurve_val_lut[51];
	nvt_writel(fcurve_val_lut25.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER25_OFS);

	fcurve_val_lut26.bit.ife_f_fcurve_val_lut52 = shdr->fcurve_val_lut[52];
	fcurve_val_lut26.bit.ife_f_fcurve_val_lut53 = shdr->fcurve_val_lut[53];
	nvt_writel(fcurve_val_lut26.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER26_OFS);

	fcurve_val_lut27.bit.ife_f_fcurve_val_lut54 = shdr->fcurve_val_lut[54];
	fcurve_val_lut27.bit.ife_f_fcurve_val_lut55 = shdr->fcurve_val_lut[55];
	nvt_writel(fcurve_val_lut27.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER27_OFS);

	fcurve_val_lut28.bit.ife_f_fcurve_val_lut56 = shdr->fcurve_val_lut[56];
	fcurve_val_lut28.bit.ife_f_fcurve_val_lut57 = shdr->fcurve_val_lut[57];
	nvt_writel(fcurve_val_lut28.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER28_OFS);

	fcurve_val_lut29.bit.ife_f_fcurve_val_lut58 = shdr->fcurve_val_lut[58];
	fcurve_val_lut29.bit.ife_f_fcurve_val_lut59 = shdr->fcurve_val_lut[59];
	nvt_writel(fcurve_val_lut29.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER29_OFS);

	fcurve_val_lut30.bit.ife_f_fcurve_val_lut60 = shdr->fcurve_val_lut[60];
	fcurve_val_lut30.bit.ife_f_fcurve_val_lut61 = shdr->fcurve_val_lut[61];
	nvt_writel(fcurve_val_lut30.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER30_OFS);

	fcurve_val_lut31.bit.ife_f_fcurve_val_lut62 = shdr->fcurve_val_lut[62];
	fcurve_val_lut31.bit.ife_f_fcurve_val_lut63 = shdr->fcurve_val_lut[63];
	nvt_writel(fcurve_val_lut31.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER31_OFS);

	fcurve_val_lut32.bit.ife_f_fcurve_val_lut64 = shdr->fcurve_val_lut[64];
	nvt_writel(fcurve_val_lut32.reg, reg_addr + RHE_FCURVE_VALUE_REGISTER32_OFS);
}

static void iq_builtin_set_nr(UINT32 id, ISP_BUILTIN_IQ_TRIG_MSG msg, UINT32 gain, ISP_BUILTIN_CGAIN *cgain, IQ_PARAM_PTR *iq_param, UINT32 ui_nr_ratio)
{
	T_IFE_EN_REGISTER                                                         IFE_en_reg;
	T_IME_FUNCTION_CONTROL_REGISTER0                                          ime_function_control_reg0;

	T_OUTLIER_REGISTER_0                                                      Outlier_reg_0;
	T_OUTLIER_REGISTER_1                                                      Outlier_reg_1;
	T_OUTLIER_REGISTER_2                                                      Outlier_reg_2;
	T_OUTLIER_REGISTER_3                                                      Outlier_reg_3;
	T_OUTLIER_REGISTER_4                                                      Outlier_reg_4;
	T_OUTLIER_REGISTER_5                                                      Outlier_reg_5;
	T_OUTLIER_REGISTER_6                                                      Outlier_reg_6;

	T_GBAL_REGISTER_0                                                         gbalance_reg_0;
	T_GBAL_REGISTER_1                                                         gbalance_reg_1;

	T_RANGEA_R_TH_0                                                           rangeA_R_reg_th0;
	T_RANGEA_R_TH_1                                                           rangeA_R_reg_th1;
	T_RANGEA_R_TH_2                                                           rangeA_R_reg_th2;

	T_RANGEA_Gr_TH_0                                                          rangeA_Gr_reg_th0;
	T_RANGEA_Gr_TH_1                                                          rangeA_Gr_reg_th1;
	T_RANGEA_Gr_TH_2                                                          rangeA_Gr_reg_th2;

	T_RANGEA_Gb_TH_0                                                          rangeA_Gb_reg_th0;
	T_RANGEA_Gb_TH_1                                                          rangeA_Gb_reg_th1;
	T_RANGEA_Gb_TH_2                                                          rangeA_Gb_reg_th2;

	T_RANGEA_B_TH_0                                                           rangeA_B_reg_th0;
	T_RANGEA_B_TH_1                                                           rangeA_B_reg_th1;
	T_RANGEA_B_TH_2                                                           rangeA_B_reg_th2;

	T_RANGEB_R_TH_0                                                           rangeB_R_reg_th0;
	T_RANGEB_R_TH_1                                                           rangeB_R_reg_th1;
	T_RANGEB_R_TH_2                                                           rangeB_R_reg_th2;

	T_RANGEB_Gr_TH_0                                                          rangeB_Gr_reg_th0;
	T_RANGEB_Gr_TH_1                                                          rangeB_Gr_reg_th1;
	T_RANGEB_Gr_TH_2                                                          rangeB_Gr_reg_th2;

	T_RANGEB_Gb_TH_0                                                          rangeB_Gb_reg_th0;
	T_RANGEB_Gb_TH_1                                                          rangeB_Gb_reg_th1;
	T_RANGEB_Gb_TH_2                                                          rangeB_Gb_reg_th2;

	T_RANGEB_B_TH_0                                                           rangeB_B_reg_th0;
	T_RANGEB_B_TH_1                                                           rangeB_B_reg_th1;
	T_RANGEB_B_TH_2                                                           rangeB_B_reg_th2;

	T_RANGEA_R_LUT_0                                                          rangeA_R_reg_lut0;
	T_RANGEA_R_LUT_1                                                          rangeA_R_reg_lut1;
	T_RANGEA_R_LUT_2                                                          rangeA_R_reg_lut2;
	T_RANGEA_R_LUT_3                                                          rangeA_R_reg_lut3;
	T_RANGEA_R_LUT_4                                                          rangeA_R_reg_lut4;
	T_RANGEA_R_LUT_5                                                          rangeA_R_reg_lut5;
	T_RANGEA_R_LUT_6                                                          rangeA_R_reg_lut6;
	T_RANGEA_R_LUT_7                                                          rangeA_R_reg_lut7;
	T_RANGEA_R_LUT_8                                                          rangeA_R_reg_lut8;

	T_RANGEA_Gr_LUT_0                                                         rangeA_Gr_reg_lut0;
	T_RANGEA_Gr_LUT_1                                                         rangeA_Gr_reg_lut1;
	T_RANGEA_Gr_LUT_2                                                         rangeA_Gr_reg_lut2;
	T_RANGEA_Gr_LUT_3                                                         rangeA_Gr_reg_lut3;
	T_RANGEA_Gr_LUT_4                                                         rangeA_Gr_reg_lut4;
	T_RANGEA_Gr_LUT_5                                                         rangeA_Gr_reg_lut5;
	T_RANGEA_Gr_LUT_6                                                         rangeA_Gr_reg_lut6;
	T_RANGEA_Gr_LUT_7                                                         rangeA_Gr_reg_lut7;
	T_RANGEA_Gr_LUT_8                                                         rangeA_Gr_reg_lut8;

	T_RANGEA_Gb_LUT_0                                                         rangeA_Gb_reg_lut0;
	T_RANGEA_Gb_LUT_1                                                         rangeA_Gb_reg_lut1;
	T_RANGEA_Gb_LUT_2                                                         rangeA_Gb_reg_lut2;
	T_RANGEA_Gb_LUT_3                                                         rangeA_Gb_reg_lut3;
	T_RANGEA_Gb_LUT_4                                                         rangeA_Gb_reg_lut4;
	T_RANGEA_Gb_LUT_5                                                         rangeA_Gb_reg_lut5;
	T_RANGEA_Gb_LUT_6                                                         rangeA_Gb_reg_lut6;
	T_RANGEA_Gb_LUT_7                                                         rangeA_Gb_reg_lut7;
	T_RANGEA_Gb_LUT_8                                                         rangeA_Gb_reg_lut8;

	T_RANGEA_B_LUT_0                                                          rangeA_B_reg_lut0;
	T_RANGEA_B_LUT_1                                                          rangeA_B_reg_lut1;
	T_RANGEA_B_LUT_2                                                          rangeA_B_reg_lut2;
	T_RANGEA_B_LUT_3                                                          rangeA_B_reg_lut3;
	T_RANGEA_B_LUT_4                                                          rangeA_B_reg_lut4;
	T_RANGEA_B_LUT_5                                                          rangeA_B_reg_lut5;
	T_RANGEA_B_LUT_6                                                          rangeA_B_reg_lut6;
	T_RANGEA_B_LUT_7                                                          rangeA_B_reg_lut7;
	T_RANGEA_B_LUT_8                                                          rangeA_B_reg_lut8;

	T_RANGEB_R_LUT_0                                                          rangeB_R_reg_lut0;
	T_RANGEB_R_LUT_1                                                          rangeB_R_reg_lut1;
	T_RANGEB_R_LUT_2                                                          rangeB_R_reg_lut2;
	T_RANGEB_R_LUT_3                                                          rangeB_R_reg_lut3;
	T_RANGEB_R_LUT_4                                                          rangeB_R_reg_lut4;
	T_RANGEB_R_LUT_5                                                          rangeB_R_reg_lut5;
	T_RANGEB_R_LUT_6                                                          rangeB_R_reg_lut6;
	T_RANGEB_R_LUT_7                                                          rangeB_R_reg_lut7;
	T_RANGEB_R_LUT_8                                                          rangeB_R_reg_lut8;

	T_RANGEB_Gr_LUT_0                                                         rangeB_Gr_reg_lut0;
	T_RANGEB_Gr_LUT_1                                                         rangeB_Gr_reg_lut1;
	T_RANGEB_Gr_LUT_2                                                         rangeB_Gr_reg_lut2;
	T_RANGEB_Gr_LUT_3                                                         rangeB_Gr_reg_lut3;
	T_RANGEB_Gr_LUT_4                                                         rangeB_Gr_reg_lut4;
	T_RANGEB_Gr_LUT_5                                                         rangeB_Gr_reg_lut5;
	T_RANGEB_Gr_LUT_6                                                         rangeB_Gr_reg_lut6;
	T_RANGEB_Gr_LUT_7                                                         rangeB_Gr_reg_lut7;
	T_RANGEB_Gr_LUT_8                                                         rangeB_Gr_reg_lut8;

	T_RANGEB_Gb_LUT_0                                                         rangeB_Gb_reg_lut0;
	T_RANGEB_Gb_LUT_1                                                         rangeB_Gb_reg_lut1;
	T_RANGEB_Gb_LUT_2                                                         rangeB_Gb_reg_lut2;
	T_RANGEB_Gb_LUT_3                                                         rangeB_Gb_reg_lut3;
	T_RANGEB_Gb_LUT_4                                                         rangeB_Gb_reg_lut4;
	T_RANGEB_Gb_LUT_5                                                         rangeB_Gb_reg_lut5;
	T_RANGEB_Gb_LUT_6                                                         rangeB_Gb_reg_lut6;
	T_RANGEB_Gb_LUT_7                                                         rangeB_Gb_reg_lut7;
	T_RANGEB_Gb_LUT_8                                                         rangeB_Gb_reg_lut8;

	T_RANGEB_B_LUT_0                                                          rangeB_B_reg_lut0;
	T_RANGEB_B_LUT_1                                                          rangeB_B_reg_lut1;
	T_RANGEB_B_LUT_2                                                          rangeB_B_reg_lut2;
	T_RANGEB_B_LUT_3                                                          rangeB_B_reg_lut3;
	T_RANGEB_B_LUT_4                                                          rangeB_B_reg_lut4;
	T_RANGEB_B_LUT_5                                                          rangeB_B_reg_lut5;
	T_RANGEB_B_LUT_6                                                          rangeB_B_reg_lut6;
	T_RANGEB_B_LUT_7                                                          rangeB_B_reg_lut7;
	T_RANGEB_B_LUT_8                                                          rangeB_B_reg_lut8;

	T_FILTER_CLAMP_REGISTER                                                   filter_clamp_reg;
	T_FILTER_BLEND_W_REGISTER                                                 filter_blend_w_reg;

	T_IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0                   ime_lca_filter_center_y;
	T_IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2                   ime_lca_filter_center_c;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0               ime_lca_filter_y0;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1               ime_lca_filter_y1;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0               ime_lca_filter_u0;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1               ime_lca_filter_u1;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0               ime_lca_filter_v0;
	T_IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1               ime_lca_filter_v1;

	T_IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0                 lca_ping_pong_reg;
	T_IME_CHROMA_ADAPTATION_REGISTER0                                         lca_reg_out_wt;
	T_IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0                 dbcs_reg;

	unsigned int                reg_addr, reg_addr2;
	IQ_NR_TUNE_PARAM            *final_nr_tune;

	UINT32                      iso_idx = 0;
	UINT32                      outl_cnt0, outl_cnt1;
	UINT32                      curr_rgain_sqrt_id = 0, curr_bgain_sqrt_id = 0;
	UINT32                      curr_rgain_sqrt = 0, curr_bgain_sqrt = 0;
	UINT32                      filter_th_r[IQ_NR_TH_NUM], filter_th_g[IQ_NR_TH_NUM], filter_th_b[IQ_NR_TH_NUM];
	UINT32                      filter_lut_r[IQ_NR_TH_LUT], filter_lut_g[IQ_NR_TH_LUT], filter_lut_b[IQ_NR_TH_LUT];
	UINT32                      filter_th_b_r[IQ_NR_TH_NUM], filter_th_b_g[IQ_NR_TH_NUM], filter_th_b_b[IQ_NR_TH_NUM];
	UINT32                      filter_lut_b_r[IQ_NR_TH_LUT], filter_lut_b_g[IQ_NR_TH_LUT], filter_lut_b_b[IQ_NR_TH_LUT];
	UINT32                      filter_clamp_th;
	int                         i;

	reg_addr = (unsigned int)ioremap_nocache(IFE_BASE_ADDR, IFE_SIZE);
	reg_addr2 = (unsigned int)ioremap_nocache(IME_BASE_ADDR, IME_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->nr->mode == IQ_OP_TYPE_AUTO) {
		final_nr_tune = &iq_param->nr->auto_param[iso_idx];
	} else {
		final_nr_tune = &iq_param->nr->manual_param;
	}

	if (final_nr_tune->outl_sel == IQ_NR_OUTL_8_NODE) {
		outl_cnt0 = 1;
		outl_cnt1 = 0;
	} else if (final_nr_tune->outl_sel == IQ_NR_OUTL_7_NODE) {
		outl_cnt0 = 3;
		outl_cnt1 = 2;
	} else {
		outl_cnt0 = 1;
		outl_cnt1 = 0;
	}

	// NR_enable
	IFE_en_reg.reg = nvt_readl(reg_addr + IFE_EN_REGISTER_OFS);
	IFE_en_reg.bit.outl_en = iq_param->nr->outl_enable;
	IFE_en_reg.bit.gbal_en = iq_param->nr->gbal_enable;
	IFE_en_reg.bit.filter_en = iq_param->nr->filter_enable;
	nvt_writel(IFE_en_reg.reg, reg_addr + IFE_EN_REGISTER_OFS);

	ime_function_control_reg0.reg = nvt_readl(reg_addr2 + IME_FUNCTION_CONTROL_REGISTER0_OFS);
	ime_function_control_reg0.bit.ime_dbcs_en = iq_param->nr->dbcs_enable;
	nvt_writel(ime_function_control_reg0.reg, reg_addr2 + IME_FUNCTION_CONTROL_REGISTER0_OFS);

	// Outlier
	Outlier_reg_0.bit.ife_outlth_bri0 = final_nr_tune->outl_bright_th[0];
	Outlier_reg_0.bit.ife_outlth_dark0 = final_nr_tune->outl_dark_th[0];
	nvt_writel(Outlier_reg_0.reg, reg_addr + OUTLIER_REGISTER_0_OFS);

	Outlier_reg_1.bit.ife_outlth_bri1 = final_nr_tune->outl_bright_th[1];
	Outlier_reg_1.bit.ife_outlth_dark1 = final_nr_tune->outl_dark_th[1];
	nvt_writel(Outlier_reg_1.reg, reg_addr + OUTLIER_REGISTER_1_OFS);

	Outlier_reg_2.bit.ife_outlth_bri2 = final_nr_tune->outl_bright_th[2];
	Outlier_reg_2.bit.ife_outlth_dark2 = final_nr_tune->outl_dark_th[2];
	nvt_writel(Outlier_reg_2.reg, reg_addr + OUTLIER_REGISTER_2_OFS);

	Outlier_reg_3.bit.ife_outlth_bri3 = final_nr_tune->outl_bright_th[3];
	Outlier_reg_3.bit.ife_outlth_dark3 = final_nr_tune->outl_dark_th[3];
	nvt_writel(Outlier_reg_3.reg, reg_addr + OUTLIER_REGISTER_3_OFS);

	Outlier_reg_4.bit.ife_outlth_bri4 = final_nr_tune->outl_bright_th[4];
	Outlier_reg_4.bit.ife_outlth_dark4 = final_nr_tune->outl_dark_th[4];
	nvt_writel(Outlier_reg_4.reg, reg_addr + OUTLIER_REGISTER_4_OFS);

	Outlier_reg_5.reg = nvt_readl(reg_addr + OUTLIER_REGISTER_5_OFS);
	Outlier_reg_5.bit.ife_outl_avg_mode = final_nr_tune->outl_avg_mode;
	Outlier_reg_5.bit.ife_outl_weight = final_nr_tune->outl_ord_blend_w;
	Outlier_reg_5.bit.ife_outl_cnt1 = outl_cnt0;
	Outlier_reg_5.bit.ife_outl_cnt2 = outl_cnt1;
	nvt_writel(Outlier_reg_5.reg, reg_addr + OUTLIER_REGISTER_5_OFS);

	Outlier_reg_6.reg = nvt_readl(reg_addr + OUTLIER_REGISTER_6_OFS);
	Outlier_reg_6.bit.ife_ord_protect_th = final_nr_tune->outl_ord_protect_th;
	nvt_writel(Outlier_reg_6.reg, reg_addr + OUTLIER_REGISTER_6_OFS);

	// Gbalance
	gbalance_reg_0.reg = nvt_readl(reg_addr + GBAL_REGISTER_0_OFS);
	gbalance_reg_0.bit.ife_gbal_diff_thr_str = final_nr_tune->gbal_diff_th_str;
	nvt_writel(gbalance_reg_0.reg, reg_addr + GBAL_REGISTER_0_OFS);

	gbalance_reg_1.bit.ife_gbal_edge_thr_0 = final_nr_tune->gbal_edge_protect_th;
	gbalance_reg_1.bit.ife_gbal_edge_thr_1 = final_nr_tune->gbal_edge_protect_th * 125 / 100;
	nvt_writel(gbalance_reg_1.reg, reg_addr + GBAL_REGISTER_1_OFS);

	// 2DNR Filter_th
	if (isp_builtin_get_shdr_enable(id)) {
		if (cgain->g == 0) {
			curr_rgain_sqrt = 256;
			curr_bgain_sqrt = 256;
		} else {
			curr_rgain_sqrt_id = (cgain->r * 5) / cgain->g;
			curr_bgain_sqrt_id = (cgain->b * 5) / cgain->g;
			curr_rgain_sqrt = iq_intpl(cgain->r, iq_sqrt_tab[curr_rgain_sqrt_id], iq_sqrt_tab[curr_rgain_sqrt_id + 1], (curr_rgain_sqrt_id * cgain->g) / 5, ((curr_rgain_sqrt_id + 1) * cgain->g) / 5);
			curr_bgain_sqrt = iq_intpl(cgain->b, iq_sqrt_tab[curr_bgain_sqrt_id], iq_sqrt_tab[curr_bgain_sqrt_id + 1], (curr_bgain_sqrt_id * cgain->g) / 5, ((curr_bgain_sqrt_id + 1) * cgain->g) / 5);
		}
	} else {
		curr_rgain_sqrt = 256;
		curr_bgain_sqrt = 256;
	}
	for (i = 0; i < IQ_NR_TH_NUM; i++) {
		filter_th_r[i] = (UINT32)iq_cal((curr_rgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_th[i], 0, 1023);
		filter_th_r[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_r[i], 0, 1023);

		filter_th_g[i] = (UINT32)iq_cal(ui_nr_ratio, final_nr_tune->filter_th[i], 0, 1023);
		filter_th_g[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_g[i], 0, 1023);

		filter_th_b[i] = (UINT32)iq_cal((curr_bgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_th[i], 0, 1023);
		filter_th_b[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_b[i], 0, 1023);

		filter_th_b_r[i] = (UINT32)iq_cal((curr_rgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_th_b[i], 0, 1023);
		filter_th_b_r[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_b_r[i], 0, 1023);

		filter_th_b_g[i] = (UINT32)iq_cal(ui_nr_ratio, final_nr_tune->filter_th_b[i], 0, 1023);
		filter_th_b_g[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_b_g[i], 0, 1023);

		filter_th_b_b[i] = (UINT32)iq_cal((curr_bgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_th_b[i], 0, 1023);
		filter_th_b_b[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm, filter_th_b_b[i], 0, 1023);
	}
	for (i = 0; i < IQ_NR_TH_LUT; i++) {
		filter_lut_r[i] = (UINT32)iq_cal((curr_rgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_lut[i], 0, 1023);
		filter_lut_r[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_r[i], 0, 1023);

		filter_lut_g[i] = (UINT32)iq_cal(ui_nr_ratio, final_nr_tune->filter_lut[i], 0, 1023);
		filter_lut_g[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_g[i], 0, 1023);

		filter_lut_b[i] = (UINT32)iq_cal((curr_bgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_lut[i], 0, 1023);
		filter_lut_b[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_b[i], 0, 1023);

		filter_lut_b_r[i] = (UINT32)iq_cal((curr_rgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_lut_b[i], 0, 1023);
		filter_lut_b_r[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_b_r[i], 0, 1023);

		filter_lut_b_g[i] = (UINT32)iq_cal(ui_nr_ratio, final_nr_tune->filter_lut_b[i], 0, 1023);
		filter_lut_b_g[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_b_g[i], 0, 1023);

		filter_lut_b_b[i] = (UINT32)iq_cal((curr_bgain_sqrt * ui_nr_ratio) >> 8, final_nr_tune->filter_lut_b[i], 0, 1023);
		filter_lut_b_b[i] = (UINT32)iq_cal(iq_front_factor.rth_nlm_lut, filter_lut_b_b[i], 0, 1023);
	}
	filter_clamp_th = (UINT32)iq_cal(200 - ui_nr_ratio, final_nr_tune->filter_clamp_th, 0, 1023);

	rangeA_R_reg_th0.bit.ife_rth_nlm_c0_0 = filter_th_r[0];
	rangeA_R_reg_th0.bit.ife_rth_nlm_c0_1 = filter_th_r[1];
	nvt_writel(rangeA_R_reg_th0.reg, reg_addr + RANGEA_R_TH_0_OFS);

	rangeA_R_reg_th1.bit.ife_rth_nlm_c0_2 = filter_th_r[2];
	rangeA_R_reg_th1.bit.ife_rth_nlm_c0_3 = filter_th_r[3];
	nvt_writel(rangeA_R_reg_th1.reg, reg_addr + RANGEA_R_TH_1_OFS);

	rangeA_R_reg_th2.bit.ife_rth_nlm_c0_4 = filter_th_r[4];
	rangeA_R_reg_th2.bit.ife_rth_nlm_c0_5 = filter_th_r[5];
	nvt_writel(rangeA_R_reg_th2.reg, reg_addr + RANGEA_R_TH_2_OFS);

	rangeA_Gr_reg_th0.bit.ife_rth_nlm_c1_0 = filter_th_g[0];
	rangeA_Gr_reg_th0.bit.ife_rth_nlm_c1_1 = filter_th_g[1];
	nvt_writel(rangeA_Gr_reg_th0.reg, reg_addr + RANGEA_Gr_TH_0_OFS);

	rangeA_Gr_reg_th1.bit.ife_rth_nlm_c1_2 = filter_th_g[2];
	rangeA_Gr_reg_th1.bit.ife_rth_nlm_c1_3 = filter_th_g[3];
	nvt_writel(rangeA_Gr_reg_th1.reg, reg_addr + RANGEA_Gr_TH_1_OFS);

	rangeA_Gr_reg_th2.bit.ife_rth_nlm_c1_4 = filter_th_g[4];
	rangeA_Gr_reg_th2.bit.ife_rth_nlm_c1_5 = filter_th_g[5];
	nvt_writel(rangeA_Gr_reg_th2.reg, reg_addr + RANGEA_Gr_TH_2_OFS);

	rangeA_Gb_reg_th0.bit.ife_rth_nlm_c2_0 = filter_th_g[0];
	rangeA_Gb_reg_th0.bit.ife_rth_nlm_c2_1 = filter_th_g[1];
	nvt_writel(rangeA_Gb_reg_th0.reg, reg_addr + RANGEA_Gb_TH_0_OFS);

	rangeA_Gb_reg_th1.bit.ife_rth_nlm_c2_2 = filter_th_g[2];
	rangeA_Gb_reg_th1.bit.ife_rth_nlm_c2_3 = filter_th_g[3];
	nvt_writel(rangeA_Gb_reg_th1.reg, reg_addr + RANGEA_Gb_TH_1_OFS);

	rangeA_Gb_reg_th2.bit.ife_rth_nlm_c2_4 = filter_th_g[4];
	rangeA_Gb_reg_th2.bit.ife_rth_nlm_c2_5 = filter_th_g[5];
	nvt_writel(rangeA_Gb_reg_th2.reg, reg_addr + RANGEA_Gb_TH_2_OFS);

	rangeA_B_reg_th0.bit.ife_rth_nlm_c3_0 = filter_th_b[0];
	rangeA_B_reg_th0.bit.ife_rth_nlm_c3_1 = filter_th_b[1];
	nvt_writel(rangeA_B_reg_th0.reg, reg_addr + RANGEA_B_TH_0_OFS);

	rangeA_B_reg_th1.bit.ife_rth_nlm_c3_2 = filter_th_b[2];
	rangeA_B_reg_th1.bit.ife_rth_nlm_c3_3 = filter_th_b[3];
	nvt_writel(rangeA_B_reg_th1.reg, reg_addr + RANGEA_B_TH_1_OFS);

	rangeA_B_reg_th2.bit.ife_rth_nlm_c3_4 = filter_th_b[4];
	rangeA_B_reg_th2.bit.ife_rth_nlm_c3_5 = filter_th_b[5];
	nvt_writel(rangeA_B_reg_th2.reg, reg_addr + RANGEA_B_TH_2_OFS);

	rangeB_R_reg_th0.bit.ife_rth_bilat_c0_0 = filter_th_b_r[0];
	rangeB_R_reg_th0.bit.ife_rth_bilat_c0_1 = filter_th_b_r[1];
	nvt_writel(rangeB_R_reg_th0.reg, reg_addr + RANGEB_R_TH_0_OFS);

	rangeB_R_reg_th1.bit.ife_rth_bilat_c0_2 = filter_th_b_r[2];
	rangeB_R_reg_th1.bit.ife_rth_bilat_c0_3 = filter_th_b_r[3];
	nvt_writel(rangeB_R_reg_th1.reg, reg_addr + RANGEB_R_TH_1_OFS);

	rangeB_R_reg_th2.bit.ife_rth_bilat_c0_4 = filter_th_b_r[4];
	rangeB_R_reg_th2.bit.ife_rth_bilat_c0_5 = filter_th_b_r[5];
	nvt_writel(rangeB_R_reg_th2.reg, reg_addr + RANGEB_R_TH_2_OFS);

	rangeB_Gr_reg_th0.bit.ife_rth_bilat_c1_0 = filter_th_b_g[0];
	rangeB_Gr_reg_th0.bit.ife_rth_bilat_c1_1 = filter_th_b_g[1];
	nvt_writel(rangeB_Gr_reg_th0.reg, reg_addr + RANGEB_Gr_TH_0_OFS);

	rangeB_Gr_reg_th1.bit.ife_rth_bilat_c1_2 = filter_th_b_g[2];
	rangeB_Gr_reg_th1.bit.ife_rth_bilat_c1_3 = filter_th_b_g[3];
	nvt_writel(rangeB_Gr_reg_th1.reg, reg_addr + RANGEB_Gr_TH_1_OFS);

	rangeB_Gr_reg_th2.bit.ife_rth_bilat_c1_4 = filter_th_b_g[4];
	rangeB_Gr_reg_th2.bit.ife_rth_bilat_c1_5 = filter_th_b_g[5];
	nvt_writel(rangeB_Gr_reg_th2.reg, reg_addr + RANGEB_Gr_TH_2_OFS);

	rangeB_Gb_reg_th0.bit.ife_rth_bilat_c2_0 = filter_th_b_g[0];
	rangeB_Gb_reg_th0.bit.ife_rth_bilat_c2_1 = filter_th_b_g[1];
	nvt_writel(rangeB_Gb_reg_th0.reg, reg_addr + RANGEB_Gb_TH_0_OFS);

	rangeB_Gb_reg_th1.bit.ife_rth_bilat_c2_2 = filter_th_b_g[2];
	rangeB_Gb_reg_th1.bit.ife_rth_bilat_c2_3 = filter_th_b_g[3];
	nvt_writel(rangeB_Gb_reg_th1.reg, reg_addr + RANGEB_Gb_TH_1_OFS);

	rangeB_Gb_reg_th2.bit.ife_rth_bilat_c2_4 = filter_th_b_g[4];
	rangeB_Gb_reg_th2.bit.ife_rth_bilat_c2_5 = filter_th_b_g[5];
	nvt_writel(rangeB_Gb_reg_th2.reg, reg_addr + RANGEB_Gb_TH_2_OFS);

	rangeB_B_reg_th0.bit.ife_rth_bilat_c3_0 = filter_th_b_b[0];
	rangeB_B_reg_th0.bit.ife_rth_bilat_c3_1 = filter_th_b_b[1];
	nvt_writel(rangeB_B_reg_th0.reg, reg_addr + RANGEB_B_TH_0_OFS);

	rangeB_B_reg_th1.bit.ife_rth_bilat_c3_2 = filter_th_b_b[2];
	rangeB_B_reg_th1.bit.ife_rth_bilat_c3_3 = filter_th_b_b[3];
	nvt_writel(rangeB_B_reg_th1.reg, reg_addr + RANGEB_B_TH_1_OFS);

	rangeB_B_reg_th2.bit.ife_rth_bilat_c3_4 = filter_th_b_b[4];
	rangeB_B_reg_th2.bit.ife_rth_bilat_c3_5 = filter_th_b_b[5];
	nvt_writel(rangeB_B_reg_th2.reg, reg_addr + RANGEB_B_TH_2_OFS);

	#if IQ_BUILTIN_DBG_MSG
	if(iq_frm_cnt[id] < iq_front_dtsi.smooth_thr) {
		printk("id = %d, cnt = %d, rth_nlm = %d, %d, %d, %d, %d, %d\r\n", id, iq_frm_cnt[id],
																rangeA_R_reg_th0.bit.ife_rth_nlm_c0_0,
																rangeA_R_reg_th0.bit.ife_rth_nlm_c0_1,
																rangeA_R_reg_th1.bit.ife_rth_nlm_c0_2,
																rangeA_R_reg_th1.bit.ife_rth_nlm_c0_3,
																rangeA_R_reg_th2.bit.ife_rth_nlm_c0_4,
																rangeA_R_reg_th2.bit.ife_rth_nlm_c0_5);
	}
	#endif

	// 2DNR Filter_lut
	rangeA_R_reg_lut0.bit.ife_rth_nlm_c0_lut_0 = filter_lut_r[0];
	rangeA_R_reg_lut0.bit.ife_rth_nlm_c0_lut_1 = filter_lut_r[1];
	nvt_writel(rangeA_R_reg_lut0.reg, reg_addr + RANGEA_R_LUT_0_OFS);

	rangeA_R_reg_lut1.bit.ife_rth_nlm_c0_lut_2 = filter_lut_r[2];
	rangeA_R_reg_lut1.bit.ife_rth_nlm_c0_lut_3 = filter_lut_r[3];
	nvt_writel(rangeA_R_reg_lut1.reg, reg_addr + RANGEA_R_LUT_1_OFS);

	rangeA_R_reg_lut2.bit.ife_rth_nlm_c0_lut_4 = filter_lut_r[4];
	rangeA_R_reg_lut2.bit.ife_rth_nlm_c0_lut_5 = filter_lut_r[5];
	nvt_writel(rangeA_R_reg_lut2.reg, reg_addr + RANGEA_R_LUT_2_OFS);

	rangeA_R_reg_lut3.bit.ife_rth_nlm_c0_lut_6 = filter_lut_r[6];
	rangeA_R_reg_lut3.bit.ife_rth_nlm_c0_lut_7 = filter_lut_r[7];
	nvt_writel(rangeA_R_reg_lut3.reg, reg_addr + RANGEA_R_LUT_3_OFS);

	rangeA_R_reg_lut4.bit.ife_rth_nlm_c0_lut_8 = filter_lut_r[8];
	rangeA_R_reg_lut4.bit.ife_rth_nlm_c0_lut_9 = filter_lut_r[9];
	nvt_writel(rangeA_R_reg_lut4.reg, reg_addr + RANGEA_R_LUT_4_OFS);

	rangeA_R_reg_lut5.bit.ife_rth_nlm_c0_lut_10 = filter_lut_r[10];
	rangeA_R_reg_lut5.bit.ife_rth_nlm_c0_lut_11 = filter_lut_r[11];
	nvt_writel(rangeA_R_reg_lut5.reg, reg_addr + RANGEA_R_LUT_5_OFS);

	rangeA_R_reg_lut6.bit.ife_rth_nlm_c0_lut_12 = filter_lut_r[12];
	rangeA_R_reg_lut6.bit.ife_rth_nlm_c0_lut_13 = filter_lut_r[13];
	nvt_writel(rangeA_R_reg_lut6.reg, reg_addr + RANGEA_R_LUT_6_OFS);

	rangeA_R_reg_lut7.bit.ife_rth_nlm_c0_lut_14 = filter_lut_r[14];
	rangeA_R_reg_lut7.bit.ife_rth_nlm_c0_lut_15 = filter_lut_r[15];
	nvt_writel(rangeA_R_reg_lut7.reg, reg_addr + RANGEA_R_LUT_7_OFS);

	rangeA_R_reg_lut8.bit.ife_rth_nlm_c0_lut_16 = filter_lut_r[16];
	nvt_writel(rangeA_R_reg_lut8.reg, reg_addr + RANGEA_R_LUT_8_OFS);

	#if IQ_BUILTIN_DBG_MSG
	if(iq_frm_cnt[id] < iq_front_dtsi.smooth_thr) {
		printk("id = %d, cnt = %d, rth_lut = %d, %d, %d, %d, %d, %d\r\n", id, iq_frm_cnt[id],
													   rangeA_R_reg_lut5.bit.ife_rth_nlm_c0_lut_11,
													   rangeA_R_reg_lut6.bit.ife_rth_nlm_c0_lut_12,
													   rangeA_R_reg_lut6.bit.ife_rth_nlm_c0_lut_13,
													   rangeA_R_reg_lut7.bit.ife_rth_nlm_c0_lut_14,
													   rangeA_R_reg_lut7.bit.ife_rth_nlm_c0_lut_15,
													   rangeA_R_reg_lut8.bit.ife_rth_nlm_c0_lut_16);
	}
	#endif

	rangeA_Gr_reg_lut0.bit.ife_rth_nlm_c1_lut_0 = filter_lut_g[0];
	rangeA_Gr_reg_lut0.bit.ife_rth_nlm_c1_lut_1 = filter_lut_g[1];
	nvt_writel(rangeA_Gr_reg_lut0.reg, reg_addr + RANGEA_Gr_LUT_0_OFS);

	rangeA_Gr_reg_lut1.bit.ife_rth_nlm_c1_lut_2 = filter_lut_g[2];
	rangeA_Gr_reg_lut1.bit.ife_rth_nlm_c1_lut_3 = filter_lut_g[3];
	nvt_writel(rangeA_Gr_reg_lut1.reg, reg_addr + RANGEA_Gr_LUT_1_OFS);

	rangeA_Gr_reg_lut2.bit.ife_rth_nlm_c1_lut_4 = filter_lut_g[4];
	rangeA_Gr_reg_lut2.bit.ife_rth_nlm_c1_lut_5 = filter_lut_g[5];
	nvt_writel(rangeA_Gr_reg_lut2.reg, reg_addr + RANGEA_Gr_LUT_2_OFS);

	rangeA_Gr_reg_lut3.bit.ife_rth_nlm_c1_lut_6 = filter_lut_g[6];
	rangeA_Gr_reg_lut3.bit.ife_rth_nlm_c1_lut_7 = filter_lut_g[7];
	nvt_writel(rangeA_Gr_reg_lut3.reg, reg_addr + RANGEA_Gr_LUT_3_OFS);

	rangeA_Gr_reg_lut4.bit.ife_rth_nlm_c1_lut_8 = filter_lut_g[8];
	rangeA_Gr_reg_lut4.bit.ife_rth_nlm_c1_lut_9 = filter_lut_g[9];
	nvt_writel(rangeA_Gr_reg_lut4.reg, reg_addr + RANGEA_Gr_LUT_4_OFS);

	rangeA_Gr_reg_lut5.bit.ife_rth_nlm_c1_lut_10 = filter_lut_g[10];
	rangeA_Gr_reg_lut5.bit.ife_rth_nlm_c1_lut_11 = filter_lut_g[11];
	nvt_writel(rangeA_Gr_reg_lut5.reg, reg_addr + RANGEA_Gr_LUT_5_OFS);

	rangeA_Gr_reg_lut6.bit.ife_rth_nlm_c1_lut_12 = filter_lut_g[12];
	rangeA_Gr_reg_lut6.bit.ife_rth_nlm_c1_lut_13 = filter_lut_g[13];
	nvt_writel(rangeA_Gr_reg_lut6.reg, reg_addr + RANGEA_Gr_LUT_6_OFS);

	rangeA_Gr_reg_lut7.bit.ife_rth_nlm_c1_lut_14 = filter_lut_g[14];
	rangeA_Gr_reg_lut7.bit.ife_rth_nlm_c1_lut_15 = filter_lut_g[15];
	nvt_writel(rangeA_Gr_reg_lut7.reg, reg_addr + RANGEA_Gr_LUT_7_OFS);

	rangeA_Gr_reg_lut8.bit.ife_rth_nlm_c1_lut_16 = filter_lut_g[16];
	nvt_writel(rangeA_Gr_reg_lut8.reg, reg_addr + RANGEA_Gr_LUT_8_OFS);

	rangeA_Gb_reg_lut0.bit.ife_rth_nlm_c2_lut_0 = filter_lut_g[0];
	rangeA_Gb_reg_lut0.bit.ife_rth_nlm_c2_lut_1 = filter_lut_g[1];
	nvt_writel(rangeA_Gb_reg_lut0.reg, reg_addr + RANGEA_Gb_LUT_0_OFS);

	rangeA_Gb_reg_lut1.bit.ife_rth_nlm_c2_lut_2 = filter_lut_g[2];
	rangeA_Gb_reg_lut1.bit.ife_rth_nlm_c2_lut_3 = filter_lut_g[3];
	nvt_writel(rangeA_Gb_reg_lut1.reg, reg_addr + RANGEA_Gb_LUT_1_OFS);

	rangeA_Gb_reg_lut2.bit.ife_rth_nlm_c2_lut_4 = filter_lut_g[4];
	rangeA_Gb_reg_lut2.bit.ife_rth_nlm_c2_lut_5 = filter_lut_g[5];
	nvt_writel(rangeA_Gb_reg_lut2.reg, reg_addr + RANGEA_Gb_LUT_2_OFS);

	rangeA_Gb_reg_lut3.bit.ife_rth_nlm_c2_lut_6 = filter_lut_g[6];
	rangeA_Gb_reg_lut3.bit.ife_rth_nlm_c2_lut_7 = filter_lut_g[7];
	nvt_writel(rangeA_Gb_reg_lut3.reg, reg_addr + RANGEA_Gb_LUT_3_OFS);

	rangeA_Gb_reg_lut4.bit.ife_rth_nlm_c2_lut_8 = filter_lut_g[8];
	rangeA_Gb_reg_lut4.bit.ife_rth_nlm_c2_lut_9 = filter_lut_g[9];
	nvt_writel(rangeA_Gb_reg_lut4.reg, reg_addr + RANGEA_Gb_LUT_4_OFS);

	rangeA_Gb_reg_lut5.bit.ife_rth_nlm_c2_lut_10 = filter_lut_g[10];
	rangeA_Gb_reg_lut5.bit.ife_rth_nlm_c2_lut_11 = filter_lut_g[11];
	nvt_writel(rangeA_Gb_reg_lut5.reg, reg_addr + RANGEA_Gb_LUT_5_OFS);

	rangeA_Gb_reg_lut6.bit.ife_rth_nlm_c2_lut_12 = filter_lut_g[12];
	rangeA_Gb_reg_lut6.bit.ife_rth_nlm_c2_lut_13 = filter_lut_g[13];
	nvt_writel(rangeA_Gb_reg_lut6.reg, reg_addr + RANGEA_Gb_LUT_6_OFS);

	rangeA_Gb_reg_lut7.bit.ife_rth_nlm_c2_lut_14 = filter_lut_g[14];
	rangeA_Gb_reg_lut7.bit.ife_rth_nlm_c2_lut_15 = filter_lut_g[15];
	nvt_writel(rangeA_Gb_reg_lut7.reg, reg_addr + RANGEA_Gb_LUT_7_OFS);

	rangeA_Gb_reg_lut8.bit.ife_rth_nlm_c2_lut_16 = filter_lut_g[16];
	nvt_writel(rangeA_Gb_reg_lut8.reg, reg_addr + RANGEA_Gb_LUT_8_OFS);

	rangeA_B_reg_lut0.bit.ife_rth_nlm_c3_lut_0 = filter_lut_b[0];
	rangeA_B_reg_lut0.bit.ife_rth_nlm_c3_lut_1 = filter_lut_b[1];
	nvt_writel(rangeA_B_reg_lut0.reg, reg_addr + RANGEA_B_LUT_0_OFS);

	rangeA_B_reg_lut1.bit.ife_rth_nlm_c3_lut_2 = filter_lut_b[2];
	rangeA_B_reg_lut1.bit.ife_rth_nlm_c3_lut_3 = filter_lut_b[3];
	nvt_writel(rangeA_B_reg_lut1.reg, reg_addr + RANGEA_B_LUT_1_OFS);

	rangeA_B_reg_lut2.bit.ife_rth_nlm_c3_lut_4 = filter_lut_b[4];
	rangeA_B_reg_lut2.bit.ife_rth_nlm_c3_lut_5 = filter_lut_b[5];
	nvt_writel(rangeA_B_reg_lut2.reg, reg_addr + RANGEA_B_LUT_2_OFS);

	rangeA_B_reg_lut3.bit.ife_rth_nlm_c3_lut_6 = filter_lut_b[6];
	rangeA_B_reg_lut3.bit.ife_rth_nlm_c3_lut_7 = filter_lut_b[7];
	nvt_writel(rangeA_B_reg_lut3.reg, reg_addr + RANGEA_B_LUT_3_OFS);

	rangeA_B_reg_lut4.bit.ife_rth_nlm_c3_lut_8 = filter_lut_b[8];
	rangeA_B_reg_lut4.bit.ife_rth_nlm_c3_lut_9 = filter_lut_b[9];
	nvt_writel(rangeA_B_reg_lut4.reg, reg_addr + RANGEA_B_LUT_4_OFS);

	rangeA_B_reg_lut5.bit.ife_rth_nlm_c3_lut_10 = filter_lut_b[10];
	rangeA_B_reg_lut5.bit.ife_rth_nlm_c3_lut_11 = filter_lut_b[11];
	nvt_writel(rangeA_B_reg_lut5.reg, reg_addr + RANGEA_B_LUT_5_OFS);

	rangeA_B_reg_lut6.bit.ife_rth_nlm_c3_lut_12 = filter_lut_b[12];
	rangeA_B_reg_lut6.bit.ife_rth_nlm_c3_lut_13 = filter_lut_b[13];
	nvt_writel(rangeA_B_reg_lut6.reg, reg_addr + RANGEA_B_LUT_6_OFS);

	rangeA_B_reg_lut7.bit.ife_rth_nlm_c3_lut_14 = filter_lut_b[14];
	rangeA_B_reg_lut7.bit.ife_rth_nlm_c3_lut_15 = filter_lut_b[15];
	nvt_writel(rangeA_B_reg_lut7.reg, reg_addr + RANGEA_B_LUT_7_OFS);

	rangeA_B_reg_lut8.bit.ife_rth_nlm_c3_lut_16 = filter_lut_b[16];
	nvt_writel(rangeA_B_reg_lut8.reg, reg_addr + RANGEA_B_LUT_8_OFS);

	rangeB_R_reg_lut0.bit.ife_rth_bilat_c0_lut_0 = filter_lut_b_r[0];
	rangeB_R_reg_lut0.bit.ife_rth_bilat_c0_lut_1 = filter_lut_b_r[1];
	nvt_writel(rangeB_R_reg_lut0.reg, reg_addr + RANGEB_R_LUT_0_OFS);

	rangeB_R_reg_lut1.bit.ife_rth_bilat_c0_lut_2 = filter_lut_b_r[2];
	rangeB_R_reg_lut1.bit.ife_rth_bilat_c0_lut_3 = filter_lut_b_r[3];
	nvt_writel(rangeB_R_reg_lut1.reg, reg_addr + RANGEB_R_LUT_1_OFS);

	rangeB_R_reg_lut2.bit.ife_rth_bilat_c0_lut_4 = filter_lut_b_r[4];
	rangeB_R_reg_lut2.bit.ife_rth_bilat_c0_lut_5 = filter_lut_b_r[5];
	nvt_writel(rangeB_R_reg_lut2.reg, reg_addr + RANGEB_R_LUT_2_OFS);

	rangeB_R_reg_lut3.bit.ife_rth_bilat_c0_lut_6 = filter_lut_b_r[6];
	rangeB_R_reg_lut3.bit.ife_rth_bilat_c0_lut_7 = filter_lut_b_r[7];
	nvt_writel(rangeB_R_reg_lut3.reg, reg_addr + RANGEB_R_LUT_3_OFS);

	rangeB_R_reg_lut4.bit.ife_rth_bilat_c0_lut_8 = filter_lut_b_r[8];
	rangeB_R_reg_lut4.bit.ife_rth_bilat_c0_lut_9 = filter_lut_b_r[9];
	nvt_writel(rangeB_R_reg_lut4.reg, reg_addr + RANGEB_R_LUT_4_OFS);

	rangeB_R_reg_lut5.bit.ife_rth_bilat_c0_lut_10 = filter_lut_b_r[10];
	rangeB_R_reg_lut5.bit.ife_rth_bilat_c0_lut_11 = filter_lut_b_r[11];
	nvt_writel(rangeB_R_reg_lut5.reg, reg_addr + RANGEB_R_LUT_5_OFS);

	rangeB_R_reg_lut6.bit.ife_rth_bilat_c0_lut_12 = filter_lut_b_r[12];
	rangeB_R_reg_lut6.bit.ife_rth_bilat_c0_lut_13 = filter_lut_b_r[13];
	nvt_writel(rangeB_R_reg_lut6.reg, reg_addr + RANGEB_R_LUT_6_OFS);

	rangeB_R_reg_lut7.bit.ife_rth_bilat_c0_lut_14 = filter_lut_b_r[14];
	rangeB_R_reg_lut7.bit.ife_rth_bilat_c0_lut_15 = filter_lut_b_r[15];
	nvt_writel(rangeB_R_reg_lut7.reg, reg_addr + RANGEB_R_LUT_7_OFS);

	rangeB_R_reg_lut8.bit.ife_rth_bilat_c0_lut_16 = filter_lut_b_r[16];
	nvt_writel(rangeB_R_reg_lut8.reg, reg_addr + RANGEB_R_LUT_8_OFS);

	rangeB_Gr_reg_lut0.bit.ife_rth_bilat_c1_lut_0 = filter_lut_b_g[0];
	rangeB_Gr_reg_lut0.bit.ife_rth_bilat_c1_lut_1 = filter_lut_b_g[1];
	nvt_writel(rangeB_Gr_reg_lut0.reg, reg_addr + RANGEB_Gr_LUT_0_OFS);

	rangeB_Gr_reg_lut1.bit.ife_rth_bilat_c1_lut_2 = filter_lut_b_g[2];
	rangeB_Gr_reg_lut1.bit.ife_rth_bilat_c1_lut_3 = filter_lut_b_g[3];
	nvt_writel(rangeB_Gr_reg_lut1.reg, reg_addr + RANGEB_Gr_LUT_1_OFS);

	rangeB_Gr_reg_lut2.bit.ife_rth_bilat_c1_lut_4 = filter_lut_b_g[4];
	rangeB_Gr_reg_lut2.bit.ife_rth_bilat_c1_lut_5 = filter_lut_b_g[5];
	nvt_writel(rangeB_Gr_reg_lut2.reg, reg_addr + RANGEB_Gr_LUT_2_OFS);

	rangeB_Gr_reg_lut3.bit.ife_rth_bilat_c1_lut_6 = filter_lut_b_g[6];
	rangeB_Gr_reg_lut3.bit.ife_rth_bilat_c1_lut_7 = filter_lut_b_g[7];
	nvt_writel(rangeB_Gr_reg_lut3.reg, reg_addr + RANGEB_Gr_LUT_3_OFS);

	rangeB_Gr_reg_lut4.bit.ife_rth_bilat_c1_lut_8 = filter_lut_b_g[8];
	rangeB_Gr_reg_lut4.bit.ife_rth_bilat_c1_lut_9 = filter_lut_b_g[9];
	nvt_writel(rangeB_Gr_reg_lut4.reg, reg_addr + RANGEB_Gr_LUT_4_OFS);

	rangeB_Gr_reg_lut5.bit.ife_rth_bilat_c1_lut_10 = filter_lut_b_g[10];
	rangeB_Gr_reg_lut5.bit.ife_rth_bilat_c1_lut_11 = filter_lut_b_g[11];
	nvt_writel(rangeB_Gr_reg_lut5.reg, reg_addr + RANGEB_Gr_LUT_5_OFS);

	rangeB_Gr_reg_lut6.bit.ife_rth_bilat_c1_lut_12 = filter_lut_b_g[12];
	rangeB_Gr_reg_lut6.bit.ife_rth_bilat_c1_lut_13 = filter_lut_b_g[13];
	nvt_writel(rangeB_Gr_reg_lut6.reg, reg_addr + RANGEB_Gr_LUT_6_OFS);

	rangeB_Gr_reg_lut7.bit.ife_rth_bilat_c1_lut_14 = filter_lut_b_g[14];
	rangeB_Gr_reg_lut7.bit.ife_rth_bilat_c1_lut_15 = filter_lut_b_g[15];
	nvt_writel(rangeB_Gr_reg_lut7.reg, reg_addr + RANGEB_Gr_LUT_7_OFS);

	rangeB_Gr_reg_lut8.bit.ife_rth_bilat_c1_lut_16 = filter_lut_b_g[16];
	nvt_writel(rangeB_Gr_reg_lut8.reg, reg_addr + RANGEB_Gr_LUT_8_OFS);

	rangeB_Gb_reg_lut0.bit.ife_rth_bilat_c2_lut_0 = filter_lut_b_g[0];
	rangeB_Gb_reg_lut0.bit.ife_rth_bilat_c2_lut_1 = filter_lut_b_g[1];
	nvt_writel(rangeB_Gb_reg_lut0.reg, reg_addr + RANGEB_Gb_LUT_0_OFS);

	rangeB_Gb_reg_lut1.bit.ife_rth_bilat_c2_lut_2 = filter_lut_b_g[2];
	rangeB_Gb_reg_lut1.bit.ife_rth_bilat_c2_lut_3 = filter_lut_b_g[3];
	nvt_writel(rangeB_Gb_reg_lut1.reg, reg_addr + RANGEB_Gb_LUT_1_OFS);

	rangeB_Gb_reg_lut2.bit.ife_rth_bilat_c2_lut_4 = filter_lut_b_g[4];
	rangeB_Gb_reg_lut2.bit.ife_rth_bilat_c2_lut_5 = filter_lut_b_g[5];
	nvt_writel(rangeB_Gb_reg_lut2.reg, reg_addr + RANGEB_Gb_LUT_2_OFS);

	rangeB_Gb_reg_lut3.bit.ife_rth_bilat_c2_lut_6 = filter_lut_b_g[6];
	rangeB_Gb_reg_lut3.bit.ife_rth_bilat_c2_lut_7 = filter_lut_b_g[7];
	nvt_writel(rangeB_Gb_reg_lut3.reg, reg_addr + RANGEB_Gb_LUT_3_OFS);

	rangeB_Gb_reg_lut4.bit.ife_rth_bilat_c2_lut_8 = filter_lut_b_g[8];
	rangeB_Gb_reg_lut4.bit.ife_rth_bilat_c2_lut_9 = filter_lut_b_g[9];
	nvt_writel(rangeB_Gb_reg_lut4.reg, reg_addr + RANGEB_Gb_LUT_4_OFS);

	rangeB_Gb_reg_lut5.bit.ife_rth_bilat_c2_lut_10 = filter_lut_b_g[10];
	rangeB_Gb_reg_lut5.bit.ife_rth_bilat_c2_lut_11 = filter_lut_b_g[11];
	nvt_writel(rangeB_Gb_reg_lut5.reg, reg_addr + RANGEB_Gb_LUT_5_OFS);

	rangeB_Gb_reg_lut6.bit.ife_rth_bilat_c2_lut_12 = filter_lut_b_g[12];
	rangeB_Gb_reg_lut6.bit.ife_rth_bilat_c2_lut_13 = filter_lut_b_g[13];
	nvt_writel(rangeB_Gb_reg_lut6.reg, reg_addr + RANGEB_Gb_LUT_6_OFS);

	rangeB_Gb_reg_lut7.bit.ife_rth_bilat_c2_lut_14 = filter_lut_b_g[14];
	rangeB_Gb_reg_lut7.bit.ife_rth_bilat_c2_lut_15 = filter_lut_b_g[15];
	nvt_writel(rangeB_Gb_reg_lut7.reg, reg_addr + RANGEB_Gb_LUT_7_OFS);

	rangeB_Gb_reg_lut8.bit.ife_rth_bilat_c2_lut_16 = filter_lut_b_g[16];
	nvt_writel(rangeB_Gb_reg_lut8.reg, reg_addr + RANGEB_Gb_LUT_8_OFS);

	rangeB_B_reg_lut0.bit.ife_rth_bilat_c3_lut_0 = filter_lut_b_b[0];
	rangeB_B_reg_lut0.bit.ife_rth_bilat_c3_lut_1 = filter_lut_b_b[1];
	nvt_writel(rangeB_B_reg_lut0.reg, reg_addr + RANGEB_B_LUT_0_OFS);

	rangeB_B_reg_lut1.bit.ife_rth_bilat_c3_lut_2 = filter_lut_b_b[2];
	rangeB_B_reg_lut1.bit.ife_rth_bilat_c3_lut_3 = filter_lut_b_b[3];
	nvt_writel(rangeB_B_reg_lut1.reg, reg_addr + RANGEB_B_LUT_1_OFS);

	rangeB_B_reg_lut2.bit.ife_rth_bilat_c3_lut_4 = filter_lut_b_b[4];
	rangeB_B_reg_lut2.bit.ife_rth_bilat_c3_lut_5 = filter_lut_b_b[5];
	nvt_writel(rangeB_B_reg_lut2.reg, reg_addr + RANGEB_B_LUT_2_OFS);

	rangeB_B_reg_lut3.bit.ife_rth_bilat_c3_lut_6 = filter_lut_b_b[6];
	rangeB_B_reg_lut3.bit.ife_rth_bilat_c3_lut_7 = filter_lut_b_b[7];
	nvt_writel(rangeB_B_reg_lut3.reg, reg_addr + RANGEB_B_LUT_3_OFS);

	rangeB_B_reg_lut4.bit.ife_rth_bilat_c3_lut_8 = filter_lut_b_b[8];
	rangeB_B_reg_lut4.bit.ife_rth_bilat_c3_lut_9 = filter_lut_b_b[9];
	nvt_writel(rangeB_B_reg_lut4.reg, reg_addr + RANGEB_B_LUT_4_OFS);

	rangeB_B_reg_lut5.bit.ife_rth_bilat_c3_lut_10 = filter_lut_b_b[10];
	rangeB_B_reg_lut5.bit.ife_rth_bilat_c3_lut_11 = filter_lut_b_b[11];
	nvt_writel(rangeB_B_reg_lut5.reg, reg_addr + RANGEB_B_LUT_5_OFS);

	rangeB_B_reg_lut6.bit.ife_rth_bilat_c3_lut_12 = filter_lut_b_b[12];
	rangeB_B_reg_lut6.bit.ife_rth_bilat_c3_lut_13 = filter_lut_b_b[13];
	nvt_writel(rangeB_B_reg_lut6.reg, reg_addr + RANGEB_B_LUT_6_OFS);

	rangeB_B_reg_lut7.bit.ife_rth_bilat_c3_lut_14 = filter_lut_b_b[14];
	rangeB_B_reg_lut7.bit.ife_rth_bilat_c3_lut_15 = filter_lut_b_b[15];
	nvt_writel(rangeB_B_reg_lut7.reg, reg_addr + RANGEB_B_LUT_7_OFS);

	rangeB_B_reg_lut8.bit.ife_rth_bilat_c3_lut_16 = filter_lut_b_b[16];
	nvt_writel(rangeB_B_reg_lut8.reg, reg_addr + RANGEB_B_LUT_8_OFS);

	// 2DNR Filter setting
	filter_clamp_reg.reg = nvt_readl(reg_addr + FILTER_CLAMP_REGISTER_OFS);
	filter_clamp_reg.bit.ife_clamp_th = (filter_clamp_th * iq_front_factor.clamp_th) / 100;
	filter_clamp_reg.bit.ife_clamp_mul = final_nr_tune->filter_clamp_mul;
	nvt_writel(filter_clamp_reg.reg, reg_addr + FILTER_CLAMP_REGISTER_OFS);

	filter_blend_w_reg.reg = nvt_readl(reg_addr + FILTER_BLEND_W_REGISTER_OFS);
	filter_blend_w_reg.bit.ife_bilat_w = final_nr_tune->filter_blend_w;
	filter_blend_w_reg.bit.ife_bilat_th1 = iq_bilateral_center[iso_idx][0];
	filter_blend_w_reg.bit.ife_bilat_th2 = iq_bilateral_center[iso_idx][1];
	nvt_writel(filter_blend_w_reg.reg, reg_addr + FILTER_BLEND_W_REGISTER_OFS);

	#if IQ_BUILTIN_DBG_MSG
	if(iq_frm_cnt[id] < iq_front_dtsi.smooth_thr) {
		printk("id = %d, cnt = %d, blend_w = %d, clamp = %d\r\n", id, iq_frm_cnt[id], filter_blend_w_reg.bit.ife_bilat_w, filter_clamp_reg.bit.ife_clamp_th);
	}
	#endif

	// LCA
	ime_lca_filter_center_y.reg = nvt_readl(reg_addr2 + IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS);
	ime_lca_filter_center_y.bit.ime_lcaf_y_rcth0 = final_nr_tune->lca_sub_center_filter_y;
	ime_lca_filter_center_y.bit.ime_lcaf_y_rcth1 = IQ_CLAMP(final_nr_tune->lca_sub_center_filter_y * 1414 / 1000, 0, 255);
	ime_lca_filter_center_y.bit.ime_lcaf_y_rcth2 = IQ_CLAMP(final_nr_tune->lca_sub_center_filter_y * 1732 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_center_y.reg, reg_addr2 + IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS);

	ime_lca_filter_center_c.reg = nvt_readl(reg_addr2 + IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS);
	ime_lca_filter_center_c.bit.ime_lcaf_uv_rcth0 = final_nr_tune->lca_sub_center_filter_c;
	ime_lca_filter_center_c.bit.ime_lcaf_uv_rcth1 = IQ_CLAMP(final_nr_tune->lca_sub_center_filter_c * 1414 / 1000, 0, 255);
	ime_lca_filter_center_c.bit.ime_lcaf_uv_rcth2 = IQ_CLAMP(final_nr_tune->lca_sub_center_filter_c * 1732 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_center_c.reg, reg_addr2 + IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS);

	ime_lca_filter_y0.bit.ime_lcaf_y_fth0 = final_nr_tune->lca_sub_filter_y;
	ime_lca_filter_y0.bit.ime_lcaf_y_fth1 = IQ_CLAMP(final_nr_tune->lca_sub_filter_y * 1414 / 1000, 0, 255);
	ime_lca_filter_y0.bit.ime_lcaf_y_fth2 = IQ_CLAMP(final_nr_tune->lca_sub_filter_y * 1732 / 1000, 0, 255);
	ime_lca_filter_y0.bit.ime_lcaf_y_fth3 = IQ_CLAMP(final_nr_tune->lca_sub_filter_y * 2000 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_y0.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0_OFS);

	ime_lca_filter_y1.bit.ime_lcaf_y_fth4 = IQ_CLAMP(final_nr_tune->lca_sub_filter_y * 2828 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_y1.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1_OFS);

	ime_lca_filter_u0.bit.ime_lcaf_u_fth0 = final_nr_tune->lca_sub_filter_c;
	ime_lca_filter_u0.bit.ime_lcaf_u_fth1 = IQ_CLAMP(final_nr_tune->lca_sub_filter_c * 1414 / 1000, 0, 255);
	ime_lca_filter_u0.bit.ime_lcaf_u_fth2 = IQ_CLAMP(final_nr_tune->lca_sub_filter_c * 1732 / 1000, 0, 255);
	ime_lca_filter_u0.bit.ime_lcaf_u_fth3 = IQ_CLAMP(final_nr_tune->lca_sub_filter_c * 2000 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_u0.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0_OFS);

	ime_lca_filter_u1.bit.ime_lcaf_u_fth4 = IQ_CLAMP(final_nr_tune->lca_sub_filter_c * 2828 / 1000, 0, 255);
	nvt_writel(ime_lca_filter_u1.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1_OFS);

	ime_lca_filter_v0.bit.ime_lcaf_v_fth0 = ime_lca_filter_u0.bit.ime_lcaf_u_fth0;
	ime_lca_filter_v0.bit.ime_lcaf_v_fth1 = ime_lca_filter_u0.bit.ime_lcaf_u_fth1;
	ime_lca_filter_v0.bit.ime_lcaf_v_fth2 = ime_lca_filter_u0.bit.ime_lcaf_u_fth2;
	ime_lca_filter_v0.bit.ime_lcaf_v_fth3 = ime_lca_filter_u0.bit.ime_lcaf_u_fth3;
	nvt_writel(ime_lca_filter_v0.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0_OFS);

	ime_lca_filter_v1.bit.ime_lcaf_v_fth4 = ime_lca_filter_u1.bit.ime_lcaf_u_fth4;
	nvt_writel(ime_lca_filter_v1.reg, reg_addr2 + IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1_OFS);

	lca_ping_pong_reg.reg = nvt_readl(reg_addr2 + IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);
	if (msg == ISP_BUILTIN_IQ_TRIG_IPP_RESET) {
		lca_ping_pong_reg.bit.chra_bypass = TRUE;
	} else {
		lca_ping_pong_reg.bit.chra_bypass = FALSE;
	}
	nvt_writel(lca_ping_pong_reg.reg, reg_addr2 + IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS);
	if (iq_param->nr->lca_enable == 1) {
		lca_reg_out_wt.reg = nvt_readl(reg_addr2 + IME_CHROMA_ADAPTATION_REGISTER0_OFS);
		lca_reg_out_wt.bit.chra_out_wt = final_nr_tune->lca_out_c_wt;
		lca_reg_out_wt.bit.luma_out_wt = final_nr_tune->lca_out_y_wt;
		nvt_writel(lca_reg_out_wt.reg, reg_addr2 + IME_CHROMA_ADAPTATION_REGISTER0_OFS);
	} else {
		lca_reg_out_wt.reg = nvt_readl(reg_addr2 + IME_CHROMA_ADAPTATION_REGISTER0_OFS);
		lca_reg_out_wt.bit.chra_out_wt = 0;
		lca_reg_out_wt.bit.luma_out_wt = 0;
		nvt_writel(lca_reg_out_wt.reg, reg_addr2 + IME_CHROMA_ADAPTATION_REGISTER0_OFS);
	}

	// DBCS
	dbcs_reg.reg = nvt_readl(reg_addr2 + IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);
	dbcs_reg.bit.dbcs_step_y = final_nr_tune->dbcs_step_y;
	dbcs_reg.bit.dbcs_step_uv = final_nr_tune->dbcs_step_c;
	nvt_writel(dbcs_reg.reg, reg_addr2 + IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS);
}

static void iq_builtin_set_vig(UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	T_IFE_EN_REGISTER                        IFE_en_reg;

	T_VIGNETTE_REGISTER_0                    vig_r_gain_reg_0;
	T_VIGNETTE_REGISTER_1                    vig_r_gain_reg_1;
	T_VIGNETTE_REGISTER_2                    vig_r_gain_reg_2;
	T_VIGNETTE_REGISTER_3                    vig_r_gain_reg_3;
	T_VIGNETTE_REGISTER_4                    vig_r_gain_reg_4;
	T_VIGNETTE_REGISTER_5                    vig_r_gain_reg_5;
	T_VIGNETTE_REGISTER_6                    vig_r_gain_reg_6;
	T_VIGNETTE_REGISTER_7                    vig_r_gain_reg_7;
	T_VIGNETTE_REGISTER_8                    vig_r_gain_reg_8;

	T_VIGNETTE_REGISTER_9                    vig_gr_gain_reg_0;
	T_VIGNETTE_REGISTER_10                   vig_gr_gain_reg_1;
	T_VIGNETTE_REGISTER_11                   vig_gr_gain_reg_2;
	T_VIGNETTE_REGISTER_12                   vig_gr_gain_reg_3;
	T_VIGNETTE_REGISTER_13                   vig_gr_gain_reg_4;
	T_VIGNETTE_REGISTER_14                   vig_gr_gain_reg_5;
	T_VIGNETTE_REGISTER_15                   vig_gr_gain_reg_6;
	T_VIGNETTE_REGISTER_16                   vig_gr_gain_reg_7;
	T_VIGNETTE_REGISTER_17                   vig_gr_gain_reg_8;

	T_VIGNETTE_REGISTER_18                   vig_gb_gain_reg_0;
	T_VIGNETTE_REGISTER_19                   vig_gb_gain_reg_1;
	T_VIGNETTE_REGISTER_20                   vig_gb_gain_reg_2;
	T_VIGNETTE_REGISTER_21                   vig_gb_gain_reg_3;
	T_VIGNETTE_REGISTER_22                   vig_gb_gain_reg_4;
	T_VIGNETTE_REGISTER_23                   vig_gb_gain_reg_5;
	T_VIGNETTE_REGISTER_24                   vig_gb_gain_reg_6;
	T_VIGNETTE_REGISTER_25                   vig_gb_gain_reg_7;
	T_VIGNETTE_REGISTER_26                   vig_gb_gain_reg_8;

	T_VIGNETTE_REGISTER_27                   vig_b_gain_reg_0;
	T_VIGNETTE_REGISTER_28                   vig_b_gain_reg_1;
	T_VIGNETTE_REGISTER_29                   vig_b_gain_reg_2;
	T_VIGNETTE_REGISTER_30                   vig_b_gain_reg_3;
	T_VIGNETTE_REGISTER_31                   vig_b_gain_reg_4;
	T_VIGNETTE_REGISTER_32                   vig_b_gain_reg_5;
	T_VIGNETTE_REGISTER_33                   vig_b_gain_reg_6;
	T_VIGNETTE_REGISTER_34                   vig_b_gain_reg_7;
	T_VIGNETTE_REGISTER_35                   vig_b_gain_reg_8;

	unsigned int                reg_addr;
	IQ_SHADING_PARAM            *shading;

	UINT32                      vig_zero[IQ_SHADING_VIG_LEN] = {0};
	UINT32                      curr_shading_vig[IQ_SHADING_VIG_LEN] = {0};

	reg_addr = (unsigned int)ioremap_nocache(IFE_BASE_ADDR, IFE_SIZE);
	shading = iq_param->shading;

	iq_intpl_tbl_uint32(shading->vig_lut, vig_zero, IQ_SHADING_VIG_LEN, curr_shading_vig, gain, shading->vig_reduce_th, shading->vig_zero_th);

	IFE_en_reg.reg = nvt_readl(reg_addr + IFE_EN_REGISTER_OFS);
	IFE_en_reg.bit.vig_en = shading->vig_enable;
	nvt_writel(IFE_en_reg.reg, reg_addr + IFE_EN_REGISTER_OFS);

	//vig_r_gain
	vig_r_gain_reg_0.bit.ife_vig_c0_lut_0 = curr_shading_vig[0];
	vig_r_gain_reg_0.bit.ife_vig_c0_lut_1 = curr_shading_vig[1];
	nvt_writel(vig_r_gain_reg_0.reg, reg_addr + VIGNETTE_REGISTER_0_OFS);

	vig_r_gain_reg_1.bit.ife_vig_c0_lut_2 = curr_shading_vig[2];
	vig_r_gain_reg_1.bit.ife_vig_c0_lut_3 = curr_shading_vig[3];
	nvt_writel(vig_r_gain_reg_1.reg, reg_addr + VIGNETTE_REGISTER_1_OFS);

	vig_r_gain_reg_2.bit.ife_vig_c0_lut_4 = curr_shading_vig[4];
	vig_r_gain_reg_2.bit.ife_vig_c0_lut_5 = curr_shading_vig[5];
	nvt_writel(vig_r_gain_reg_2.reg, reg_addr + VIGNETTE_REGISTER_2_OFS);

	vig_r_gain_reg_3.bit.ife_vig_c0_lut_6 = curr_shading_vig[6];
	vig_r_gain_reg_3.bit.ife_vig_c0_lut_7 = curr_shading_vig[7];
	nvt_writel(vig_r_gain_reg_3.reg, reg_addr + VIGNETTE_REGISTER_3_OFS);

	vig_r_gain_reg_4.bit.ife_vig_c0_lut_8 = curr_shading_vig[8];
	vig_r_gain_reg_4.bit.ife_vig_c0_lut_9 = curr_shading_vig[9];
	nvt_writel(vig_r_gain_reg_4.reg, reg_addr + VIGNETTE_REGISTER_4_OFS);

	vig_r_gain_reg_5.bit.ife_vig_c0_lut_10 = curr_shading_vig[10];
	vig_r_gain_reg_5.bit.ife_vig_c0_lut_11 = curr_shading_vig[11];
	nvt_writel(vig_r_gain_reg_5.reg, reg_addr + VIGNETTE_REGISTER_5_OFS);

	vig_r_gain_reg_6.bit.ife_vig_c0_lut_12 = curr_shading_vig[12];
	vig_r_gain_reg_6.bit.ife_vig_c0_lut_13 = curr_shading_vig[13];
	nvt_writel(vig_r_gain_reg_6.reg, reg_addr + VIGNETTE_REGISTER_6_OFS);

	vig_r_gain_reg_7.bit.ife_vig_c0_lut_14 = curr_shading_vig[14];
	vig_r_gain_reg_7.bit.ife_vig_c0_lut_15 = curr_shading_vig[15];
	nvt_writel(vig_r_gain_reg_7.reg, reg_addr + VIGNETTE_REGISTER_7_OFS);

	vig_r_gain_reg_8.bit.ife_vig_c0_lut_16 = curr_shading_vig[16];
	nvt_writel(vig_r_gain_reg_8.reg, reg_addr + VIGNETTE_REGISTER_8_OFS);

	//vig_gr_gain
	vig_gr_gain_reg_0.bit.ife_vig_c1_lut_0 = curr_shading_vig[0];
	vig_gr_gain_reg_0.bit.ife_vig_c1_lut_1 = curr_shading_vig[1];
	nvt_writel(vig_gr_gain_reg_0.reg, reg_addr + VIGNETTE_REGISTER_9_OFS);

	vig_gr_gain_reg_1.bit.ife_vig_c1_lut_2 = curr_shading_vig[2];
	vig_gr_gain_reg_1.bit.ife_vig_c1_lut_3 = curr_shading_vig[3];
	nvt_writel(vig_gr_gain_reg_1.reg, reg_addr + VIGNETTE_REGISTER_10_OFS);

	vig_gr_gain_reg_2.bit.ife_vig_c1_lut_4 = curr_shading_vig[4];
	vig_gr_gain_reg_2.bit.ife_vig_c1_lut_5 = curr_shading_vig[5];
	nvt_writel(vig_gr_gain_reg_2.reg, reg_addr + VIGNETTE_REGISTER_11_OFS);

	vig_gr_gain_reg_3.bit.ife_vig_c1_lut_6 = curr_shading_vig[6];
	vig_gr_gain_reg_3.bit.ife_vig_c1_lut_7 = curr_shading_vig[7];
	nvt_writel(vig_gr_gain_reg_3.reg, reg_addr + VIGNETTE_REGISTER_12_OFS);

	vig_gr_gain_reg_4.bit.ife_vig_c1_lut_8 = curr_shading_vig[8];
	vig_gr_gain_reg_4.bit.ife_vig_c1_lut_9 = curr_shading_vig[9];
	nvt_writel(vig_gr_gain_reg_4.reg, reg_addr + VIGNETTE_REGISTER_13_OFS);

	vig_gr_gain_reg_5.bit.ife_vig_c1_lut_10 = curr_shading_vig[10];
	vig_gr_gain_reg_5.bit.ife_vig_c1_lut_11 = curr_shading_vig[11];
	nvt_writel(vig_gr_gain_reg_5.reg, reg_addr + VIGNETTE_REGISTER_14_OFS);

	vig_gr_gain_reg_6.bit.ife_vig_c1_lut_12 = curr_shading_vig[12];
	vig_gr_gain_reg_6.bit.ife_vig_c1_lut_13 = curr_shading_vig[13];
	nvt_writel(vig_gr_gain_reg_6.reg, reg_addr + VIGNETTE_REGISTER_15_OFS);

	vig_gr_gain_reg_7.bit.ife_vig_c1_lut_14 = curr_shading_vig[14];
	vig_gr_gain_reg_7.bit.ife_vig_c1_lut_15 = curr_shading_vig[15];
	nvt_writel(vig_gr_gain_reg_7.reg, reg_addr + VIGNETTE_REGISTER_16_OFS);

	vig_gr_gain_reg_8.bit.ife_vig_c1_lut_16 = curr_shading_vig[16];
	nvt_writel(vig_gr_gain_reg_8.reg, reg_addr + VIGNETTE_REGISTER_17_OFS);

	//vig_gb_gain
	vig_gb_gain_reg_0.bit.ife_vig_c2_lut_0 = curr_shading_vig[0];
	vig_gb_gain_reg_0.bit.ife_vig_c2_lut_1 = curr_shading_vig[1];
	nvt_writel(vig_gb_gain_reg_0.reg, reg_addr + VIGNETTE_REGISTER_18_OFS);

	vig_gb_gain_reg_1.bit.ife_vig_c2_lut_2 = curr_shading_vig[2];
	vig_gb_gain_reg_1.bit.ife_vig_c2_lut_3 = curr_shading_vig[3];
	nvt_writel(vig_gb_gain_reg_1.reg, reg_addr + VIGNETTE_REGISTER_19_OFS);

	vig_gb_gain_reg_2.bit.ife_vig_c2_lut_4 = curr_shading_vig[4];
	vig_gb_gain_reg_2.bit.ife_vig_c2_lut_5 = curr_shading_vig[5];
	nvt_writel(vig_gb_gain_reg_2.reg, reg_addr + VIGNETTE_REGISTER_20_OFS);

	vig_gb_gain_reg_3.bit.ife_vig_c2_lut_6 = curr_shading_vig[6];
	vig_gb_gain_reg_3.bit.ife_vig_c2_lut_7 = curr_shading_vig[7];
	nvt_writel(vig_gb_gain_reg_3.reg, reg_addr + VIGNETTE_REGISTER_21_OFS);

	vig_gb_gain_reg_4.bit.ife_vig_c2_lut_8 = curr_shading_vig[8];
	vig_gb_gain_reg_4.bit.ife_vig_c2_lut_9 = curr_shading_vig[9];
	nvt_writel(vig_gb_gain_reg_4.reg, reg_addr + VIGNETTE_REGISTER_22_OFS);

	vig_gb_gain_reg_5.bit.ife_vig_c2_lut_10 = curr_shading_vig[10];
	vig_gb_gain_reg_5.bit.ife_vig_c2_lut_11 = curr_shading_vig[11];
	nvt_writel(vig_gb_gain_reg_5.reg, reg_addr + VIGNETTE_REGISTER_23_OFS);

	vig_gb_gain_reg_6.bit.ife_vig_c2_lut_12 = curr_shading_vig[12];
	vig_gb_gain_reg_6.bit.ife_vig_c2_lut_13 = curr_shading_vig[13];
	nvt_writel(vig_gb_gain_reg_6.reg, reg_addr + VIGNETTE_REGISTER_24_OFS);

	vig_gb_gain_reg_7.bit.ife_vig_c2_lut_14 = curr_shading_vig[14];
	vig_gb_gain_reg_7.bit.ife_vig_c2_lut_15 = curr_shading_vig[15];
	nvt_writel(vig_gb_gain_reg_7.reg, reg_addr + VIGNETTE_REGISTER_25_OFS);

	vig_gb_gain_reg_8.bit.ife_vig_c2_lut_16 = curr_shading_vig[16];
	nvt_writel(vig_gb_gain_reg_8.reg, reg_addr + VIGNETTE_REGISTER_26_OFS);

	//vig_b_gain
	vig_b_gain_reg_0.bit.ife_vig_c3_lut_0 = curr_shading_vig[0];
	vig_b_gain_reg_0.bit.ife_vig_c3_lut_1 = curr_shading_vig[1];
	nvt_writel(vig_b_gain_reg_0.reg, reg_addr + VIGNETTE_REGISTER_27_OFS);

	vig_b_gain_reg_1.bit.ife_vig_c3_lut_2 = curr_shading_vig[2];
	vig_b_gain_reg_1.bit.ife_vig_c3_lut_3 = curr_shading_vig[3];
	nvt_writel(vig_b_gain_reg_1.reg, reg_addr + VIGNETTE_REGISTER_28_OFS);

	vig_b_gain_reg_2.bit.ife_vig_c3_lut_4 = curr_shading_vig[4];
	vig_b_gain_reg_2.bit.ife_vig_c3_lut_5 = curr_shading_vig[5];
	nvt_writel(vig_b_gain_reg_2.reg, reg_addr + VIGNETTE_REGISTER_29_OFS);

	vig_b_gain_reg_3.bit.ife_vig_c3_lut_6 = curr_shading_vig[6];
	vig_b_gain_reg_3.bit.ife_vig_c3_lut_7 = curr_shading_vig[7];
	nvt_writel(vig_b_gain_reg_3.reg, reg_addr + VIGNETTE_REGISTER_30_OFS);

	vig_b_gain_reg_4.bit.ife_vig_c3_lut_8 = curr_shading_vig[8];
	vig_b_gain_reg_4.bit.ife_vig_c3_lut_9 = curr_shading_vig[9];
	nvt_writel(vig_b_gain_reg_4.reg, reg_addr + VIGNETTE_REGISTER_31_OFS);

	vig_b_gain_reg_5.bit.ife_vig_c3_lut_10 = curr_shading_vig[10];
	vig_b_gain_reg_5.bit.ife_vig_c3_lut_11 = curr_shading_vig[11];
	nvt_writel(vig_b_gain_reg_5.reg, reg_addr + VIGNETTE_REGISTER_32_OFS);

	vig_b_gain_reg_6.bit.ife_vig_c3_lut_12 = curr_shading_vig[12];
	vig_b_gain_reg_6.bit.ife_vig_c3_lut_13 = curr_shading_vig[13];
	nvt_writel(vig_b_gain_reg_6.reg, reg_addr + VIGNETTE_REGISTER_33_OFS);

	vig_b_gain_reg_7.bit.ife_vig_c3_lut_14 = curr_shading_vig[14];
	vig_b_gain_reg_7.bit.ife_vig_c3_lut_15 = curr_shading_vig[15];
	nvt_writel(vig_b_gain_reg_7.reg, reg_addr + VIGNETTE_REGISTER_34_OFS);

	vig_b_gain_reg_8.bit.ife_vig_c3_lut_16 = curr_shading_vig[16];
	nvt_writel(vig_b_gain_reg_8.reg, reg_addr + VIGNETTE_REGISTER_35_OFS);
}

static void iq_builtin_set_tonecurve(UINT32 id, IQ_PARAM_PTR *iq_param, UINT32 ui_shdr_tone_lv)
{
	T_DCE_FUNCTION_REGISTER                  dce_func_reg;
	T_WDR_INPUT_BLENDING_REGISTER0           blend_lut_reg0;
	T_WDR_INPUT_BLENDING_REGISTER4           blend_lut_reg4;

	T_WDR_TONE_CURVE_REGISTER0               tone_idx_reg0;

	T_WDR_TONE_CURVE_REGISTER8               tone_split_reg0;

	T_WDR_TONE_CURVE_REGISTER10              tone_val_reg0;
	T_WDR_TONE_CURVE_REGISTER42              tone_val_reg1;

	T_WDR_SATURATION_REDUCTION_REGISTER      wdr_sat_reg;

	unsigned int                reg_addr;
	IQ_GAMMA_PARAM              *gamma;

	UINT32                      iq_tone_lut_idx[IQ_GAMMA_TONE_IDX_NUM];
	UINT32                      iq_tone_lut_split[IQ_GAMMA_TONE_IDX_NUM];
	UINT32                      iq_tone_lut_val[IQ_GAMMA_TONE_VAL_NUM];
	INT32                       i = 0;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	gamma = iq_param->gamma;

	if (isp_builtin_get_shdr_enable(id)) {
		memcpy(iq_tone_lut_idx, gamma->tone_lut_shdr_idx, sizeof(UINT32)*IQ_GAMMA_TONE_IDX_NUM);
		memcpy(iq_tone_lut_split, gamma->tone_lut_shdr_split, sizeof(UINT32)*IQ_GAMMA_TONE_IDX_NUM);
		if (ui_shdr_tone_lv <= gamma->tone_lut_shdr_set0_lv) {
			memcpy(iq_tone_lut_val, gamma->tone_lut_shdr_set0_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
		} else if ((ui_shdr_tone_lv <= gamma->tone_lut_shdr_set1_lv) && (gamma->tone_lut_shdr_set_num >= 2)) {
			iq_intpl_tbl_uint32(gamma->tone_lut_shdr_set0_val, gamma->tone_lut_shdr_set1_val, IQ_GAMMA_TONE_VAL_NUM, iq_tone_lut_val, ui_shdr_tone_lv, gamma->tone_lut_shdr_set0_lv, gamma->tone_lut_shdr_set1_lv);
		} else if ((ui_shdr_tone_lv <= gamma->tone_lut_shdr_set2_lv) && (gamma->tone_lut_shdr_set_num >= 3)) {
			iq_intpl_tbl_uint32(gamma->tone_lut_shdr_set1_val, gamma->tone_lut_shdr_set2_val, IQ_GAMMA_TONE_VAL_NUM, iq_tone_lut_val, ui_shdr_tone_lv, gamma->tone_lut_shdr_set1_lv, gamma->tone_lut_shdr_set2_lv);
		} else if ((ui_shdr_tone_lv <= gamma->tone_lut_shdr_set3_lv) && (gamma->tone_lut_shdr_set_num >= 4)) {
			iq_intpl_tbl_uint32(gamma->tone_lut_shdr_set2_val, gamma->tone_lut_shdr_set3_val, IQ_GAMMA_TONE_VAL_NUM, iq_tone_lut_val, ui_shdr_tone_lv, gamma->tone_lut_shdr_set2_lv, gamma->tone_lut_shdr_set3_lv);
		} else if ((ui_shdr_tone_lv <= gamma->tone_lut_shdr_set4_lv) && (gamma->tone_lut_shdr_set_num >= 5)) {
			iq_intpl_tbl_uint32(gamma->tone_lut_shdr_set3_val, gamma->tone_lut_shdr_set4_val, IQ_GAMMA_TONE_VAL_NUM, iq_tone_lut_val, ui_shdr_tone_lv, gamma->tone_lut_shdr_set3_lv, gamma->tone_lut_shdr_set4_lv);
		} else {
			if (gamma->tone_lut_shdr_set_num == 2) {
				memcpy(iq_tone_lut_val, gamma->tone_lut_shdr_set1_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
			} else if(gamma->tone_lut_shdr_set_num == 3) {
				memcpy(iq_tone_lut_val, gamma->tone_lut_shdr_set2_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
			} else if(gamma->tone_lut_shdr_set_num == 4) {
				memcpy(iq_tone_lut_val, gamma->tone_lut_shdr_set3_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
			} else {
				memcpy(iq_tone_lut_val, gamma->tone_lut_shdr_set4_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
			}
		}
	}else {
		memcpy(iq_tone_lut_idx, gamma->tone_lut_linear_idx, sizeof(UINT32)*IQ_GAMMA_TONE_IDX_NUM);
		memcpy(iq_tone_lut_split, gamma->tone_lut_linear_split, sizeof(UINT32)*IQ_GAMMA_TONE_IDX_NUM);
		memcpy(iq_tone_lut_val, gamma->tone_lut_linear_val, sizeof(UINT32)*IQ_GAMMA_TONE_VAL_NUM);
	}

	dce_func_reg.reg = nvt_readl(reg_addr + DCE_FUNCTION_REGISTER_OFS);
	dce_func_reg.bit.TCURVE_EN = gamma->tone_enable;
	nvt_writel(dce_func_reg.reg, reg_addr + DCE_FUNCTION_REGISTER_OFS);

	for (i = 0; i < 4; i++) {
		blend_lut_reg0.bit.WDR_INPUT_BLDRTO0 = gamma->tone_input_blend_lut[4*i+0];
		blend_lut_reg0.bit.WDR_INPUT_BLDRTO1 = gamma->tone_input_blend_lut[4*i+1];
		blend_lut_reg0.bit.WDR_INPUT_BLDRTO2 = gamma->tone_input_blend_lut[4*i+2];
		blend_lut_reg0.bit.WDR_INPUT_BLDRTO3 = gamma->tone_input_blend_lut[4*i+3];
		nvt_writel(blend_lut_reg0.reg, reg_addr + (WDR_INPUT_BLENDING_REGISTER0_OFS + (i * 4)));
	}

	blend_lut_reg4.reg = nvt_readl(reg_addr + WDR_INPUT_BLENDING_REGISTER4_OFS);
	blend_lut_reg4.bit.WDR_INPUT_BLDRTO16 = gamma->tone_input_blend_lut[16];
	nvt_writel(blend_lut_reg4.reg, reg_addr + WDR_INPUT_BLENDING_REGISTER4_OFS);

	if (gamma->tone_enable) {
		for (i = 0; i < 8; i++) {
			tone_idx_reg0.bit.WDR_TCURVE_INDEX_LUT0 = iq_tone_lut_idx[4*i+0];
			tone_idx_reg0.bit.WDR_TCURVE_INDEX_LUT1 = iq_tone_lut_idx[4*i+1];
			tone_idx_reg0.bit.WDR_TCURVE_INDEX_LUT2 = iq_tone_lut_idx[4*i+2];
			tone_idx_reg0.bit.WDR_TCURVE_INDEX_LUT3 = iq_tone_lut_idx[4*i+3];
			nvt_writel(tone_idx_reg0.reg, reg_addr + (WDR_TONE_CURVE_REGISTER0_OFS + (i * 4)));
		}


		for (i = 0; i < 2; i++) {
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT0 = iq_tone_lut_split[16*i+0];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT1 = iq_tone_lut_split[16*i+1];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT2 = iq_tone_lut_split[16*i+2];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT3 = iq_tone_lut_split[16*i+3];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT4 = iq_tone_lut_split[16*i+4];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT5 = iq_tone_lut_split[16*i+5];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT6 = iq_tone_lut_split[16*i+6];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT7 = iq_tone_lut_split[16*i+7];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT8 = iq_tone_lut_split[16*i+8];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT9 = iq_tone_lut_split[16*i+9];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT10 = iq_tone_lut_split[16*i+10];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT11 = iq_tone_lut_split[16*i+11];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT12 = iq_tone_lut_split[16*i+12];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT13 = iq_tone_lut_split[16*i+13];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT14 = iq_tone_lut_split[16*i+14];
			tone_split_reg0.bit.WDR_TCURVE_SPLIT_LUT15 = iq_tone_lut_split[16*i+15];
			nvt_writel(tone_split_reg0.reg, reg_addr + (WDR_TONE_CURVE_REGISTER8_OFS + (i * 4)));
		}

		for (i = 0; i < 32; i++) {
			tone_val_reg0.bit.WDR_TCURVE_VAL_LUT0 = iq_tone_lut_val[2*i+0];
			tone_val_reg0.bit.WDR_TCURVE_VAL_LUT1 = iq_tone_lut_val[2*i+1];
			nvt_writel(tone_val_reg0.reg, reg_addr + (WDR_TONE_CURVE_REGISTER10_OFS + (i * 4)));
		}

		tone_val_reg1.reg = nvt_readl(reg_addr + WDR_TONE_CURVE_REGISTER42_OFS);
		tone_val_reg1.bit.WDR_TCURVE_VAL_LUT64 = iq_tone_lut_val[64];
		nvt_writel(tone_val_reg1.reg, reg_addr + WDR_TONE_CURVE_REGISTER42_OFS);

		wdr_sat_reg.reg = nvt_readl(reg_addr + WDR_SATURATION_REDUCTION_REGISTER_OFS);
		wdr_sat_reg.bit.WDR_SAT_WT_LOW = 255;
		wdr_sat_reg.bit.WDR_SAT_DELTA = 255;
		nvt_writel(wdr_sat_reg.reg, reg_addr + WDR_SATURATION_REDUCTION_REGISTER_OFS);
	}

}

static void iq_builtin_set_wdr(UINT32 id, ISP_BUILTIN_IQ_TRIG_MSG msg, UINT32 gain, IQ_PARAM_PTR *iq_param)
{
#if WDR_ENABLE
	T_DCE_FUNCTION_REGISTER                      dce_func_reg;
	T_WDR_SUBIMAGE_REGISTER_0                    wdr_subimg_reg0;
	T_WDR_SUBIMAGE_REGISTER_2                    wdr_subimg_reg2;
	T_WDR_SUBIMAGE_REGISTER_4                    wdr_subimg_reg4;
	T_WDR_OUTPUT_BLENDING_REGISTER               wdr_reg0;
	T_WDR_CONTROL_REGISTER                       wdr_reg1;
	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER0        wdr_out_bld_reg0;
	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER8        wdr_out_bld_reg8;
	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER10       wdr_out_bld_reg10;
	T_WDR_OUTPUT_BLENDING_CURVE_REGISTER42       wdr_out_bld_reg42;

	unsigned int                reg_addr;
	IQ_WDR_PARAM                *wdr;
	IQ_WDR_AUTO_PARAM           *wdr_auto;

	UINT32                      iso_idx = 0;
	UINT32                      wdr_target_level = 0;
	UINT32                      wdr_lib_level = 128;
	static UINT32               final_wdr_strength = 0;
	INT32                       i;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	wdr = iq_param->wdr;
	wdr_auto = &iq_param->wdr->auto_param[iso_idx];

	if (wdr->mode == IQ_OP_TYPE_AUTO) {
		wdr_lib_level = iq_lib_calc_wdr_nvt(id);
		wdr_lib_level = IQ_CLAMP(wdr_lib_level, 0, 240);
		wdr_target_level = (wdr_auto->level * wdr_lib_level) >> 7;
		wdr_target_level = IQ_CLAMP(wdr_target_level, wdr_auto->strength_min, wdr_auto->strength_max);
	} else {
		wdr_target_level = wdr->manual_param.strength;
	}

	//prevent dramatic change
	if (final_wdr_strength > wdr_target_level) {
		final_wdr_strength -= IQ_MAX(1, (final_wdr_strength - wdr_target_level) / 16 );
	} else if (final_wdr_strength < wdr_target_level){
		final_wdr_strength +=  IQ_MAX(1, (wdr_target_level - final_wdr_strength) / 16 );
	}

	if (wdr->enable == FALSE) {
		final_wdr_strength = 0;
	}

	dce_func_reg.reg = nvt_readl(reg_addr + DCE_FUNCTION_REGISTER_OFS);
	if (msg == ISP_BUILTIN_IQ_TRIG_IPP_RESET) {
		dce_func_reg.bit.WDR_EN = FALSE;
	} else {
		dce_func_reg.bit.WDR_EN = wdr->enable;
	}
	dce_func_reg.bit.HISTOGRAM_SEL = 0;
	nvt_writel(dce_func_reg.reg, reg_addr + DCE_FUNCTION_REGISTER_OFS);

	wdr_subimg_reg0.reg = nvt_readl(reg_addr + WDR_SUBIMAGE_REGISTER_0_OFS);
	wdr_subimg_reg0.bit.WDR_SUBIMG_WIDTH = IQ_CLAMP((wdr->subimg_size_h - 1), 4, 32);
	wdr_subimg_reg0.bit.WDR_SUBIMG_HEIGHT = IQ_CLAMP((wdr->subimg_size_v - 1), 4, 32);
	nvt_writel(wdr_subimg_reg0.reg, reg_addr + WDR_SUBIMAGE_REGISTER_0_OFS);

	wdr_subimg_reg2.bit.WDR_SUBIMG_LOFSI = (wdr->subimg_size_h * 8) >> 2;
	nvt_writel(wdr_subimg_reg2.reg, reg_addr + WDR_SUBIMAGE_REGISTER_2_OFS);

	wdr_subimg_reg4.bit.WDR_SUBIMG_LOFSO = (wdr->subimg_size_h * 8) >> 2;
	nvt_writel(wdr_subimg_reg4.reg, reg_addr + WDR_SUBIMAGE_REGISTER_4_OFS);

	wdr_reg0.bit.WDR_STRENGTH = final_wdr_strength;
	wdr_reg0.bit.WDR_CONTRAST = wdr->contrast;
	nvt_writel(wdr_reg0.reg, reg_addr + WDR_OUTPUT_BLENDING_REGISTER_OFS);

	wdr_reg1.reg = nvt_readl(reg_addr + WDR_CONTROL_REGISTER_OFS);
	wdr_reg1.bit.WDR_MAXGAIN = IQ_CLAMP(wdr->max_gain, 1, 255);
	wdr->min_gain = IQ_CLAMP(wdr->min_gain, 1, 255);
	wdr_reg1.bit.WDR_MINGAIN = 255 / wdr->min_gain;
	nvt_writel(wdr_reg1.reg, reg_addr + WDR_CONTROL_REGISTER_OFS);

	for (i = 0; i < 8; i++) {
		wdr_out_bld_reg0.bit.WDR_OUTBLD_INDEX_LUT0 = wdr->lut_idx[4*i+0];
		wdr_out_bld_reg0.bit.WDR_OUTBLD_INDEX_LUT1 = wdr->lut_idx[4*i+1];
		wdr_out_bld_reg0.bit.WDR_OUTBLD_INDEX_LUT2 = wdr->lut_idx[4*i+2];
		wdr_out_bld_reg0.bit.WDR_OUTBLD_INDEX_LUT3 = wdr->lut_idx[4*i+3];
		nvt_writel(wdr_out_bld_reg0.reg, reg_addr + (WDR_OUTPUT_BLENDING_CURVE_REGISTER0_OFS + (i * 4)));
	}

	for (i = 0; i < 2; i++) {
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT0 = wdr->lut_split[16*i+0];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT1 = wdr->lut_split[16*i+1];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT2 = wdr->lut_split[16*i+2];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT3 = wdr->lut_split[16*i+3];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT4 = wdr->lut_split[16*i+4];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT5 = wdr->lut_split[16*i+5];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT6 = wdr->lut_split[16*i+6];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT7 = wdr->lut_split[16*i+7];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT8 = wdr->lut_split[16*i+8];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT9 = wdr->lut_split[16*i+9];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT10 = wdr->lut_split[16*i+10];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT11 = wdr->lut_split[16*i+11];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT12 = wdr->lut_split[16*i+12];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT13 = wdr->lut_split[16*i+13];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT14 = wdr->lut_split[16*i+14];
		wdr_out_bld_reg8.bit.WDR_OUTBLD_SPLIT_LUT15 = wdr->lut_split[16*i+15];
		nvt_writel(wdr_out_bld_reg8.reg, reg_addr + (WDR_OUTPUT_BLENDING_CURVE_REGISTER8_OFS + (i * 4)));
	}

	for (i = 0; i < 32; i++) {
		wdr_out_bld_reg10.bit.WDR_OUTBLD_VAL_LUT0 = wdr->lut_val[2*i+0];
		wdr_out_bld_reg10.bit.WDR_OUTBLD_VAL_LUT1 = wdr->lut_val[2*i+1];
		nvt_writel(wdr_out_bld_reg10.reg, reg_addr + (WDR_OUTPUT_BLENDING_CURVE_REGISTER10_OFS + (i * 4)));
	}

	wdr_out_bld_reg42.bit.WDR_OUTBLD_VAL_LUT64 = wdr->lut_val[64];
	nvt_writel(wdr_out_bld_reg42.reg, reg_addr + WDR_OUTPUT_BLENDING_CURVE_REGISTER42_OFS);
#else
	T_DCE_FUNCTION_REGISTER                      dce_func_reg;
	unsigned int                reg_addr;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	dce_func_reg.reg = nvt_readl(reg_addr + DCE_FUNCTION_REGISTER_OFS);
	dce_func_reg.bit.WDR_EN = FALSE;
	nvt_writel(dce_func_reg.reg, reg_addr + DCE_FUNCTION_REGISTER_OFS);
#endif
}

static void iq_builtin_set_cfa(UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	T_COLOR_INTERPOLATION_REGISTER1             color_interpolation_reg1;
	T_COLOR_INTERPOLATION_REGISTER9             color_interpolation_reg9;
	T_COLOR_INTERPOLATION_REGISTER10            color_interpolation_reg10;
	T_COLOR_INTERPOLATION_REGISTER11            color_interpolation_reg11;
	T_COLOR_INTERPOLATION_REGISTER12            color_interpolation_reg12;

	unsigned int                reg_addr;
	IQ_CFA_TUNE_PARAM           *final_cfa_tune;

	UINT32                      iso_idx = 0;
	INT32                       i;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->cfa->mode == IQ_OP_TYPE_AUTO) {
		final_cfa_tune = &iq_param->cfa->auto_param[iso_idx];
	} else {
		final_cfa_tune = &iq_param->cfa->manual_param;
	}

	color_interpolation_reg1.bit.CFA_EDGE_DTH = final_cfa_tune->edge_dth;
	color_interpolation_reg1.bit.CFA_EDGE_DTH2 = final_cfa_tune->edge_dth2;
	nvt_writel(color_interpolation_reg1.reg, reg_addr + COLOR_INTERPOLATION_REGISTER1_OFS);

	color_interpolation_reg9.reg = nvt_readl(reg_addr + COLOR_INTERPOLATION_REGISTER9_OFS);
	color_interpolation_reg9.bit.CFA_FREQ_TH = final_cfa_tune->freq_th;
	nvt_writel(color_interpolation_reg9.reg, reg_addr + COLOR_INTERPOLATION_REGISTER9_OFS);

	color_interpolation_reg10.reg = nvt_readl(reg_addr + COLOR_INTERPOLATION_REGISTER10_OFS);
	color_interpolation_reg10.bit.CFA_FCS_WEIGHT = final_cfa_tune->fcs_weight;
	nvt_writel(color_interpolation_reg10.reg, reg_addr + COLOR_INTERPOLATION_REGISTER10_OFS);

	color_interpolation_reg11.bit.CFA_FCS_STRENGTH0 = final_cfa_tune->fcs_strength[0];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH1 = final_cfa_tune->fcs_strength[1];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH2 = final_cfa_tune->fcs_strength[2];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH3 = final_cfa_tune->fcs_strength[3];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH4 = final_cfa_tune->fcs_strength[4];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH5 = final_cfa_tune->fcs_strength[5];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH6 = final_cfa_tune->fcs_strength[6];
	color_interpolation_reg11.bit.CFA_FCS_STRENGTH7 = final_cfa_tune->fcs_strength[7];
	nvt_writel(color_interpolation_reg11.reg, reg_addr + COLOR_INTERPOLATION_REGISTER11_OFS);

	color_interpolation_reg12.bit.CFA_FCS_STRENGTH8 = final_cfa_tune->fcs_strength[8];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH9 = final_cfa_tune->fcs_strength[9];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH10 = final_cfa_tune->fcs_strength[10];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH11 = final_cfa_tune->fcs_strength[11];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH12 = final_cfa_tune->fcs_strength[12];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH13 = final_cfa_tune->fcs_strength[13];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH14 = final_cfa_tune->fcs_strength[14];
	color_interpolation_reg12.bit.CFA_FCS_STRENGTH15 = final_cfa_tune->fcs_strength[15];
	nvt_writel(color_interpolation_reg12.reg, reg_addr + COLOR_INTERPOLATION_REGISTER12_OFS);
}

#if LDC_ENABLE
static void iq_builtin_set_ldc(UINT32 id, IQ_PARAM_PTR *iq_param)
{
	T_DCE_FUNCTION_REGISTER                      dce_func_reg;
	T_GDC_FOV_REGISTER                           geo_fov_reg;
	T_GEO_ABERRATION_REGISTER0                   cac_r_reg;
	T_GEO_ABERRATION_REGISTER1                   cac_b_reg;

	T_GEOMETRIC_LOOK_UP_TABLEG1                  geo_lut_g_reg_0;
	T_GEOMETRIC_LOOK_UP_TABLEG2                  geo_lut_g_reg_1;
	T_GEOMETRIC_LOOK_UP_TABLEG3                  geo_lut_g_reg_2;
	T_GEOMETRIC_LOOK_UP_TABLEG4                  geo_lut_g_reg_3;
	T_GEOMETRIC_LOOK_UP_TABLEG5                  geo_lut_g_reg_4;
	T_GEOMETRIC_LOOK_UP_TABLEG6                  geo_lut_g_reg_5;
	T_GEOMETRIC_LOOK_UP_TABLEG7                  geo_lut_g_reg_6;
	T_GEOMETRIC_LOOK_UP_TABLEG8                  geo_lut_g_reg_7;
	T_GEOMETRIC_LOOK_UP_TABLEG9                  geo_lut_g_reg_8;
	T_GEOMETRIC_LOOK_UP_TABLEG10                 geo_lut_g_reg_9;
	T_GEOMETRIC_LOOK_UP_TABLEG11                 geo_lut_g_reg_10;
	T_GEOMETRIC_LOOK_UP_TABLEG12                 geo_lut_g_reg_11;
	T_GEOMETRIC_LOOK_UP_TABLEG13                 geo_lut_g_reg_12;
	T_GEOMETRIC_LOOK_UP_TABLEG14                 geo_lut_g_reg_13;
	T_GEOMETRIC_LOOK_UP_TABLEG15                 geo_lut_g_reg_14;
	T_GEOMETRIC_LOOK_UP_TABLEG16                 geo_lut_g_reg_15;
	T_GEOMETRIC_LOOK_UP_TABLEG17                 geo_lut_g_reg_16;
	T_GEOMETRIC_LOOK_UP_TABLEG18                 geo_lut_g_reg_17;
	T_GEOMETRIC_LOOK_UP_TABLEG19                 geo_lut_g_reg_18;
	T_GEOMETRIC_LOOK_UP_TABLEG20                 geo_lut_g_reg_19;
	T_GEOMETRIC_LOOK_UP_TABLEG21                 geo_lut_g_reg_20;
	T_GEOMETRIC_LOOK_UP_TABLEG22                 geo_lut_g_reg_21;
	T_GEOMETRIC_LOOK_UP_TABLEG23                 geo_lut_g_reg_22;
	T_GEOMETRIC_LOOK_UP_TABLEG24                 geo_lut_g_reg_23;
	T_GEOMETRIC_LOOK_UP_TABLEG25                 geo_lut_g_reg_24;
	T_GEOMETRIC_LOOK_UP_TABLEG26                 geo_lut_g_reg_25;
	T_GEOMETRIC_LOOK_UP_TABLEG27                 geo_lut_g_reg_26;
	T_GEOMETRIC_LOOK_UP_TABLEG28                 geo_lut_g_reg_27;
	T_GEOMETRIC_LOOK_UP_TABLEG29                 geo_lut_g_reg_28;
	T_GEOMETRIC_LOOK_UP_TABLEG30                 geo_lut_g_reg_29;
	T_GEOMETRIC_LOOK_UP_TABLEG31                 geo_lut_g_reg_30;
	T_GEOMETRIC_LOOK_UP_TABLEG32                 geo_lut_g_reg_31;
	T_GEOMETRIC_LOOK_UP_TABLEG33                 geo_lut_g_reg_32;
	T_DMA_LUT2D_IN_ADDRESS                       geo_2dlut_reg;

	unsigned int                                 reg_addr;
	IQ_LDC_PARAM                                 *ldc;
	int                                          dc_en, cac_en, dc_sel;
	VOS_ADDR                                     phy_adr;
	INT32                                        i;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	ldc = iq_param->ldc;

	if ((ldc->geo_enable == 1) && (ldc->lut_2d_enable == 1)) {
		ldc->lut_2d_enable = 0;
		printk("GEO and 2DLUT caanot enable at the same time, disable 2DLUT!!! \r\n");
	}

	if ((ldc->geo_enable == 1) && (ldc->lut_2d_enable == 0)) {
		dc_en = 1;
		cac_en = 1;
		dc_sel = 0; // GDC/CAC only
	} else if ((ldc->geo_enable == 0) && (ldc->lut_2d_enable == 1)) {
		dc_en = 1;
		cac_en = 0;
		dc_sel = 1; // 2DLUT only
	} else if ((ldc->geo_enable == 0) && (ldc->lut_2d_enable == 0)) {
		dc_en = 0;
		cac_en = 0;
		dc_sel = 0; // GDC/CAC only
	}

	dce_func_reg.reg = nvt_readl(reg_addr + DCE_FUNCTION_REGISTER_OFS);
	dce_func_reg.bit.DC_EN = dc_en;
	dce_func_reg.bit.CAC_EN = cac_en;
	dce_func_reg.bit.DC_SEL = dc_sel;
	nvt_writel(dce_func_reg.reg, reg_addr + DCE_FUNCTION_REGISTER_OFS);

	geo_fov_reg.reg = nvt_readl(reg_addr + GDC_FOV_REGISTER_OFS);
	geo_fov_reg.bit.GDC_FOVGAIN = ldc->geo_fov_gain;
	nvt_writel(geo_fov_reg.reg, reg_addr + GDC_FOV_REGISTER_OFS);

	cac_r_reg.reg = nvt_readl(reg_addr + GEO_ABERRATION_REGISTER0_OFS);
	cac_r_reg.bit.CAC_RLUTGAIN = ldc->geo_r_lut_gain;
	nvt_writel(cac_r_reg.reg, reg_addr + GEO_ABERRATION_REGISTER0_OFS);

	cac_b_reg.bit.CAC_BLUTGAIN = ldc->geo_b_lut_gain;
	nvt_writel(cac_b_reg.reg, reg_addr + GEO_ABERRATION_REGISTER1_OFS);

	geo_lut_g_reg_0.bit.GDC_LUTG0 = ldc->geo_lut_g[0];
	geo_lut_g_reg_0.bit.GDC_LUTG1 = ldc->geo_lut_g[1];
	nvt_writel(geo_lut_g_reg_0.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG1_OFS);

	geo_lut_g_reg_1.bit.GDC_LUTG2 = ldc->geo_lut_g[2];
	geo_lut_g_reg_1.bit.GDC_LUTG3 = ldc->geo_lut_g[3];
	nvt_writel(geo_lut_g_reg_1.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG2_OFS);

	geo_lut_g_reg_2.bit.GDC_LUTG4 = ldc->geo_lut_g[4];
	geo_lut_g_reg_2.bit.GDC_LUTG5 = ldc->geo_lut_g[5];
	nvt_writel(geo_lut_g_reg_2.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG3_OFS);

	geo_lut_g_reg_3.bit.GDC_LUTG6 = ldc->geo_lut_g[6];
	geo_lut_g_reg_3.bit.GDC_LUTG7 = ldc->geo_lut_g[7];
	nvt_writel(geo_lut_g_reg_3.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG4_OFS);

	geo_lut_g_reg_4.bit.GDC_LUTG8 = ldc->geo_lut_g[8];
	geo_lut_g_reg_4.bit.GDC_LUTG9 = ldc->geo_lut_g[9];
	nvt_writel(geo_lut_g_reg_4.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG5_OFS);

	geo_lut_g_reg_5.bit.GDC_LUTG10 = ldc->geo_lut_g[10];
	geo_lut_g_reg_5.bit.GDC_LUTG11 = ldc->geo_lut_g[11];
	nvt_writel(geo_lut_g_reg_5.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG6_OFS);

	geo_lut_g_reg_6.bit.GDC_LUTG12 = ldc->geo_lut_g[12];
	geo_lut_g_reg_6.bit.GDC_LUTG13 = ldc->geo_lut_g[13];
	nvt_writel(geo_lut_g_reg_6.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG7_OFS);

	geo_lut_g_reg_7.bit.GDC_LUTG14 = ldc->geo_lut_g[14];
	geo_lut_g_reg_7.bit.GDC_LUTG15 = ldc->geo_lut_g[15];
	nvt_writel(geo_lut_g_reg_7.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG8_OFS);

	geo_lut_g_reg_8.bit.GDC_LUTG16 = ldc->geo_lut_g[16];
	geo_lut_g_reg_8.bit.GDC_LUTG17 = ldc->geo_lut_g[17];
	nvt_writel(geo_lut_g_reg_8.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG9_OFS);

	geo_lut_g_reg_9.bit.GDC_LUTG18 = ldc->geo_lut_g[18];
	geo_lut_g_reg_9.bit.GDC_LUTG19 = ldc->geo_lut_g[19];
	nvt_writel(geo_lut_g_reg_9.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG10_OFS);

	geo_lut_g_reg_10.bit.GDC_LUTG20 = ldc->geo_lut_g[20];
	geo_lut_g_reg_10.bit.GDC_LUTG21 = ldc->geo_lut_g[21];
	nvt_writel(geo_lut_g_reg_10.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG11_OFS);

	geo_lut_g_reg_11.bit.GDC_LUTG22 = ldc->geo_lut_g[22];
	geo_lut_g_reg_11.bit.GDC_LUTG23 = ldc->geo_lut_g[23];
	nvt_writel(geo_lut_g_reg_11.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG12_OFS);

	geo_lut_g_reg_12.bit.GDC_LUTG24 = ldc->geo_lut_g[24];
	geo_lut_g_reg_12.bit.GDC_LUTG25 = ldc->geo_lut_g[25];
	nvt_writel(geo_lut_g_reg_12.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG13_OFS);

	geo_lut_g_reg_13.bit.GDC_LUTG26 = ldc->geo_lut_g[26];
	geo_lut_g_reg_13.bit.GDC_LUTG27 = ldc->geo_lut_g[27];
	nvt_writel(geo_lut_g_reg_13.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG14_OFS);

	geo_lut_g_reg_14.bit.GDC_LUTG28 = ldc->geo_lut_g[28];
	geo_lut_g_reg_14.bit.GDC_LUTG29 = ldc->geo_lut_g[29];
	nvt_writel(geo_lut_g_reg_14.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG15_OFS);

	geo_lut_g_reg_15.bit.GDC_LUTG30 = ldc->geo_lut_g[30];
	geo_lut_g_reg_15.bit.GDC_LUTG31 = ldc->geo_lut_g[31];
	nvt_writel(geo_lut_g_reg_15.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG16_OFS);

	geo_lut_g_reg_16.bit.GDC_LUTG32 = ldc->geo_lut_g[32];
	geo_lut_g_reg_16.bit.GDC_LUTG33 = ldc->geo_lut_g[33];
	nvt_writel(geo_lut_g_reg_16.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG17_OFS);

	geo_lut_g_reg_17.bit.GDC_LUTG34 = ldc->geo_lut_g[34];
	geo_lut_g_reg_17.bit.GDC_LUTG35 = ldc->geo_lut_g[35];
	nvt_writel(geo_lut_g_reg_17.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG18_OFS);

	geo_lut_g_reg_18.bit.GDC_LUTG36 = ldc->geo_lut_g[36];
	geo_lut_g_reg_18.bit.GDC_LUTG37 = ldc->geo_lut_g[37];
	nvt_writel(geo_lut_g_reg_18.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG19_OFS);

	geo_lut_g_reg_19.bit.GDC_LUTG38 = ldc->geo_lut_g[38];
	geo_lut_g_reg_19.bit.GDC_LUTG39 = ldc->geo_lut_g[39];
	nvt_writel(geo_lut_g_reg_19.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG20_OFS);

	geo_lut_g_reg_20.bit.GDC_LUTG40 = ldc->geo_lut_g[40];
	geo_lut_g_reg_20.bit.GDC_LUTG41 = ldc->geo_lut_g[41];
	nvt_writel(geo_lut_g_reg_20.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG21_OFS);

	geo_lut_g_reg_21.bit.GDC_LUTG42 = ldc->geo_lut_g[42];
	geo_lut_g_reg_21.bit.GDC_LUTG43 = ldc->geo_lut_g[43];
	nvt_writel(geo_lut_g_reg_21.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG22_OFS);

	geo_lut_g_reg_22.bit.GDC_LUTG44 = ldc->geo_lut_g[44];
	geo_lut_g_reg_22.bit.GDC_LUTG45 = ldc->geo_lut_g[45];
	nvt_writel(geo_lut_g_reg_22.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG23_OFS);

	geo_lut_g_reg_23.bit.GDC_LUTG46 = ldc->geo_lut_g[46];
	geo_lut_g_reg_23.bit.GDC_LUTG47 = ldc->geo_lut_g[47];
	nvt_writel(geo_lut_g_reg_23.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG24_OFS);

	geo_lut_g_reg_24.bit.GDC_LUTG48 = ldc->geo_lut_g[48];
	geo_lut_g_reg_24.bit.GDC_LUTG49 = ldc->geo_lut_g[49];
	nvt_writel(geo_lut_g_reg_24.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG25_OFS);

	geo_lut_g_reg_25.bit.GDC_LUTG50 = ldc->geo_lut_g[50];
	geo_lut_g_reg_25.bit.GDC_LUTG51 = ldc->geo_lut_g[51];
	nvt_writel(geo_lut_g_reg_25.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG26_OFS);

	geo_lut_g_reg_26.bit.GDC_LUTG52 = ldc->geo_lut_g[52];
	geo_lut_g_reg_26.bit.GDC_LUTG53 = ldc->geo_lut_g[53];
	nvt_writel(geo_lut_g_reg_26.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG27_OFS);

	geo_lut_g_reg_27.bit.GDC_LUTG54 = ldc->geo_lut_g[54];
	geo_lut_g_reg_27.bit.GDC_LUTG55 = ldc->geo_lut_g[55];
	nvt_writel(geo_lut_g_reg_27.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG28_OFS);

	geo_lut_g_reg_28.bit.GDC_LUTG56 = ldc->geo_lut_g[56];
	geo_lut_g_reg_28.bit.GDC_LUTG57 = ldc->geo_lut_g[57];
	nvt_writel(geo_lut_g_reg_28.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG29_OFS);

	geo_lut_g_reg_29.bit.GDC_LUTG58 = ldc->geo_lut_g[58];
	geo_lut_g_reg_29.bit.GDC_LUTG59 = ldc->geo_lut_g[59];
	nvt_writel(geo_lut_g_reg_29.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG30_OFS);

	geo_lut_g_reg_30.bit.GDC_LUTG60 = ldc->geo_lut_g[60];
	geo_lut_g_reg_30.bit.GDC_LUTG61 = ldc->geo_lut_g[61];
	nvt_writel(geo_lut_g_reg_30.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG31_OFS);

	geo_lut_g_reg_31.bit.GDC_LUTG62 = ldc->geo_lut_g[62];
	geo_lut_g_reg_31.bit.GDC_LUTG63 = ldc->geo_lut_g[63];
	nvt_writel(geo_lut_g_reg_31.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG32_OFS);

	geo_lut_g_reg_32.bit.GDC_LUTG64 = ldc->geo_lut_g[64];
	nvt_writel(geo_lut_g_reg_32.reg, reg_addr + GEOMETRIC_LOOK_UP_TABLEG33_OFS);

	if (ldc->lut_2d_enable) {
		for (i = 0; i < LUT2D_TABLE_SIZE; i++) {
			lut2d_buffer[id][i] = ldc->lut_2d_value[i];
		}

		vos_cpu_dcache_sync((VOS_ADDR)lut2d_buffer[id], LUT2D_BUFFER_SIZE, VOS_DMA_TO_DEVICE);
		phy_adr = vos_cpu_get_phy_addr((unsigned int)lut2d_buffer[id]);
		geo_2dlut_reg.bit.DRAM_SAI2DLUT = phy_adr >> 2;
		nvt_writel(geo_2dlut_reg.reg, reg_addr + DMA_LUT2D_IN_ADDRESS_OFS);
	}
}
#endif

static void iq_builtin_set_ccm(UINT32 id, UINT32 ct, IQ_PARAM_PTR *iq_param, BOOL ui_night_mode)
{
	T_CCM_0                                            ccm_0_reg;
	T_CCM_1                                            ccm_1_reg;
	T_CCM_2                                            ccm_2_reg;
	T_CCM_3                                            ccm_3_reg;
	T_CCM_4                                            ccm_4_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_0             hue_0_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_1             hue_1_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_2             hue_2_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_3             hue_3_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_4             hue_4_reg;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_5             hue_5_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0      sat_0_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1      sat_1_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2      sat_2_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3      sat_3_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4      sat_4_reg;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5      sat_5_reg;

	unsigned int                reg_addr;
	IQ_CCM_PARAM                *ccm;
	IQ_CCM_AUTO_PARAM           *ccm_l, *ccm_h;

	UINT32                      color_idx_l = 0, color_idx_h = 0;
	UINT32                      color_start = 0, color_end = 0;
	INT16                       night_coef[IQ_CCM_LEN] = {0x100, 0x0, 0x0, 0x0, 0x100, 0x0, 0x0, 0x0, 0x100};
	INT16                       final_coef[IQ_CCM_LEN] = {0x100, 0x0, 0x0, 0x0, 0x100, 0x0, 0x0, 0x0, 0x100};
	UINT8                       final_hue_tab[IQ_CCM_HUETAB_LEN] = {0};
	INT32                       final_sat_tab[IQ_CCM_SATTAB_LEN] = {0};
	INT32                       i;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);
	ccm = iq_param->ccm;

	if (ccm->mode == IQ_OP_TYPE_AUTO) {
		if (ct <=  iq_param->ccm->auto_param[IQ_COLOR_TEMPERATURE_L].ct) {
			color_idx_l = IQ_COLOR_TEMPERATURE_L;
			color_idx_h = IQ_COLOR_TEMPERATURE_L;
			color_start = iq_param->ccm->auto_param[IQ_COLOR_TEMPERATURE_L].ct;
			color_end = iq_param->ccm->auto_param[IQ_COLOR_TEMPERATURE_L].ct;
		}
		for (i = 1; i < IQ_COLOR_ID_MAX_NUM; i++) {
			if (ct > iq_param->ccm->auto_param[i].ct) {
				color_idx_l = i;
				color_idx_h = i-1;
				color_start = iq_param->ccm->auto_param[i].ct;
				color_end = iq_param->ccm->auto_param[i-1].ct;
				break;
			}
		}

		ccm_l = &iq_param->ccm->auto_param[color_idx_l];
		ccm_h = &iq_param->ccm->auto_param[color_idx_h];

		iq_intpl_tbl_cc(ccm_l->coef, ccm_h->coef, color_start, color_end, ct, final_coef);
		memcpy(final_hue_tab, ccm_l->hue_tab, sizeof(UINT8) * IQ_CCM_HUETAB_LEN);
		memcpy(final_sat_tab, ccm_l->sat_tab, sizeof(INT32) * IQ_CCM_SATTAB_LEN);
	} else {
		memcpy(final_coef, ccm->manual_param.coef, sizeof(INT16) * IQ_CCM_LEN);
		memcpy(final_hue_tab, ccm->manual_param.hue_tab, sizeof(UINT8) * IQ_CCM_HUETAB_LEN);
		memcpy(final_sat_tab, ccm->manual_param.sat_tab, sizeof(INT32) * IQ_CCM_SATTAB_LEN);
	}

	if (ui_night_mode == TRUE) {
		memcpy(final_coef, night_coef, sizeof(INT16) * IQ_CCM_LEN);
	}

	ccm_0_reg.reg = nvt_readl(reg_addr + CCM_0_OFS);
	ccm_0_reg.bit.reg_coef_rr = final_coef[0];
	nvt_writel(ccm_0_reg.reg, reg_addr + CCM_0_OFS);

	ccm_1_reg.bit.reg_coef_rg = final_coef[1];
	ccm_1_reg.bit.reg_coef_rb = final_coef[2];
	nvt_writel(ccm_1_reg.reg, reg_addr + CCM_1_OFS);

	ccm_2_reg.bit.reg_coef_gr = final_coef[3];
	ccm_2_reg.bit.reg_coef_gg = final_coef[4];
	nvt_writel(ccm_2_reg.reg, reg_addr + CCM_2_OFS);

	ccm_3_reg.bit.reg_coef_gb = final_coef[5];
	ccm_3_reg.bit.reg_coef_br = final_coef[6];
	nvt_writel(ccm_3_reg.reg, reg_addr + CCM_3_OFS);

	ccm_4_reg.bit.reg_coef_bg = final_coef[7];
	ccm_4_reg.bit.reg_coef_bb = final_coef[8];
	nvt_writel(ccm_4_reg.reg, reg_addr + CCM_4_OFS);

	hue_0_reg.bit.reg_chuem0 = final_hue_tab[0];
	hue_0_reg.bit.reg_chuem1 = final_hue_tab[1];
	hue_0_reg.bit.reg_chuem2 = final_hue_tab[2];
	hue_0_reg.bit.reg_chuem3 = final_hue_tab[3];
	nvt_writel(hue_0_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS);

	hue_1_reg.bit.reg_chuem4 = final_hue_tab[4];
	hue_1_reg.bit.reg_chuem5 = final_hue_tab[5];
	hue_1_reg.bit.reg_chuem6 = final_hue_tab[6];
	hue_1_reg.bit.reg_chuem7 = final_hue_tab[7];
	nvt_writel(hue_1_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS);

	hue_2_reg.bit.reg_chuem8 = final_hue_tab[8];
	hue_2_reg.bit.reg_chuem9 = final_hue_tab[9];
	hue_2_reg.bit.reg_chuem10 = final_hue_tab[10];
	hue_2_reg.bit.reg_chuem11 = final_hue_tab[11];
	nvt_writel(hue_2_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS);

	hue_3_reg.bit.reg_chuem12 = final_hue_tab[12];
	hue_3_reg.bit.reg_chuem13 = final_hue_tab[13];
	hue_3_reg.bit.reg_chuem14 = final_hue_tab[14];
	hue_3_reg.bit.reg_chuem15 = final_hue_tab[15];
	nvt_writel(hue_3_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS);

	hue_4_reg.bit.reg_chuem16 = final_hue_tab[16];
	hue_4_reg.bit.reg_chuem17 = final_hue_tab[17];
	hue_4_reg.bit.reg_chuem18 = final_hue_tab[18];
	hue_4_reg.bit.reg_chuem19 = final_hue_tab[19];
	nvt_writel(hue_4_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS);

	hue_5_reg.bit.reg_chuem20 = final_hue_tab[20];
	hue_5_reg.bit.reg_chuem21 = final_hue_tab[21];
	hue_5_reg.bit.reg_chuem22 = final_hue_tab[22];
	hue_5_reg.bit.reg_chuem23 = final_hue_tab[23];
	nvt_writel(hue_5_reg.reg, reg_addr + COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS);

	sat_0_reg.bit.reg_csatm0 = final_sat_tab[0];
	sat_0_reg.bit.reg_csatm1 = final_sat_tab[1];
	sat_0_reg.bit.reg_csatm2 = final_sat_tab[2];
	sat_0_reg.bit.reg_csatm3 = final_sat_tab[3];
	nvt_writel(sat_0_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS);

	sat_1_reg.bit.reg_csatm4 = final_sat_tab[4];
	sat_1_reg.bit.reg_csatm5 = final_sat_tab[5];
	sat_1_reg.bit.reg_csatm6 = final_sat_tab[6];
	sat_1_reg.bit.reg_csatm7 = final_sat_tab[7];
	nvt_writel(sat_1_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS);

	sat_2_reg.bit.reg_csatm8 = final_sat_tab[8];
	sat_2_reg.bit.reg_csatm9 = final_sat_tab[9];
	sat_2_reg.bit.reg_csatm10 = final_sat_tab[10];
	sat_2_reg.bit.reg_csatm11 = final_sat_tab[11];
	nvt_writel(sat_2_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS);

	sat_3_reg.bit.reg_csatm12 = final_sat_tab[12];
	sat_3_reg.bit.reg_csatm13 = final_sat_tab[13];
	sat_3_reg.bit.reg_csatm14 = final_sat_tab[14];
	sat_3_reg.bit.reg_csatm15 = final_sat_tab[15];
	nvt_writel(sat_3_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS);

	sat_4_reg.bit.reg_csatm16 = final_sat_tab[16];
	sat_4_reg.bit.reg_csatm17 = final_sat_tab[17];
	sat_4_reg.bit.reg_csatm18 = final_sat_tab[18];
	sat_4_reg.bit.reg_csatm19 = final_sat_tab[19];
	nvt_writel(sat_4_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS);

	sat_5_reg.bit.reg_csatm20 = final_sat_tab[20];
	sat_5_reg.bit.reg_csatm21 = final_sat_tab[21];
	sat_5_reg.bit.reg_csatm22 = final_sat_tab[22];
	sat_5_reg.bit.reg_csatm23 = final_sat_tab[23];
	nvt_writel(sat_5_reg.reg, reg_addr + COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS);
}

static UINT32 *iq_builtin_gammacurve_conv(UINT32 id, UINT32 *r_lut, UINT32 *g_lut, UINT32 *b_lut)
{
	INT32 i, page;

	if (gamma_buffer[id] == NULL) {
		return NULL;
	}

	page = 0;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			gamma_buffer[id][page + i / 2] = r_lut[i];
		} else {
			gamma_buffer[id][page + i / 2] |= (r_lut[i] << 10);
		}
	}

	page = 65;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			gamma_buffer[id][page + i / 2] = g_lut[i];
		} else {
			gamma_buffer[id][page + i / 2] |= (g_lut[i] << 10);
		}
	}

	page = 65 * 2;
	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			gamma_buffer[id][page + i / 2] = b_lut[i];
		} else {
			gamma_buffer[id][page + i / 2] |= (b_lut[i] << 10);
		}
	}

	vos_cpu_dcache_sync((VOS_ADDR)gamma_buffer[id], GAMMA_BUFFER_SIZE, VOS_DMA_TO_DEVICE);

	return gamma_buffer[id];
}

static void iq_builtin_set_gamma(UINT32 id, UINT32 lv, IQ_PARAM_PTR *iq_param)
{
	T_IPE_CONTROL_REGISTER           ipe_control;
	T_IPE_EN_REGISTER                ipe_en_reg;
	T_DMA_TO_IPE_REGISTER_5          ipe_reg_5;
	T_EDGE_GAMMA0                    ipe_edge_gamma_reg;
	T_EDGE_GAMMA21                   ipe_edge_gamma_reg21;

	unsigned int                     reg_addr;
	IQ_GAMMA_PARAM                   *gamma;
	IQ_GAMMA_AUTO_PARAM              *gamma_l, *gamma_h;

	UINT32                           input_addr[3];
	UINT32                           gamma_idx_l = 0, gamma_idx_h = 0;
	UINT32                           gamma_start = 0, gamma_end = 0;
	UINT32                           curr_gamma[IQ_GAMMA_GAMMA_LEN] = {0};
	UINT32                           curr_edge_gamma[IQ_EDGE_GAMMA_GAMMA_LEN] = {0};
	VOS_ADDR                         final_buffer_addr, phy_adr;
	INT32                            i = 0, j = 0;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);
	gamma = iq_param->gamma;

	if (gamma->mode == IQ_OP_TYPE_AUTO) {
		if (lv <= (iq_param->gamma->auto_param[IQ_GAMMA_NIGHT].lv * LV_BASE)) {
				gamma_idx_l = IQ_GAMMA_NIGHT;
				gamma_idx_h = IQ_GAMMA_NIGHT;
				gamma_start = iq_param->gamma->auto_param[IQ_GAMMA_NIGHT].lv * 100;
				gamma_end = iq_param->gamma->auto_param[IQ_GAMMA_NIGHT].lv * 100;
			}
			for (i = 1; i < IQ_GAMMA_ID_MAX_NUM; i++) {
				if (lv > (iq_param->gamma->auto_param[i].lv * LV_BASE)) {
					gamma_idx_l = i;
					gamma_idx_h = i - 1;
					gamma_start = iq_param->gamma->auto_param[i].lv * 100;
					gamma_end = iq_param->gamma->auto_param[i - 1].lv * 100;
					break;
				}
			}

		gamma_l = &iq_param->gamma->auto_param[gamma_idx_l];
		gamma_h = &iq_param->gamma->auto_param[gamma_idx_h];

		iq_intpl_tbl_gamma(gamma_l->gamma_lut, gamma_h->gamma_lut, gamma_start, gamma_end, lv / (LV_BASE / 100), curr_gamma);
	} else {
		memcpy(curr_gamma,  iq_param->gamma->manual_param.gamma_lut, sizeof(UINT32) * IQ_GAMMA_GAMMA_LEN);
	}

	input_addr[0] = (UINT32)&curr_gamma[0];
	input_addr[1] = (UINT32)&curr_gamma[0];
	input_addr[2] = (UINT32)&curr_gamma[0];

	ipe_en_reg.reg = nvt_readl(reg_addr + IPE_EN_REGISTER_OFS);
	ipe_en_reg.bit.reg_rgbgamma_en = gamma->gamma_enable;
	nvt_writel(ipe_en_reg.reg, reg_addr + IPE_EN_REGISTER_OFS);

	final_buffer_addr = (UINT32)iq_builtin_gammacurve_conv(id, (UINT32 *)input_addr[0], (UINT32 *)input_addr[1], (UINT32 *)input_addr[2]);
	if (final_buffer_addr != 0) {
		phy_adr = vos_cpu_get_phy_addr((unsigned int)final_buffer_addr);
		ipe_reg_5.bit.reg_dram_sai_gamma = phy_adr >> 2;
		nvt_writel(ipe_reg_5.reg, reg_addr + DMA_TO_IPE_REGISTER_5_OFS);

		ipe_control.reg = nvt_readl(reg_addr + IPE_CONTROL_REGISTER_OFS);
		ipe_control.bit.reg_ipe_rwgamma = 1;
		ipe_control.bit.reg_ipe_rwgamma_opt = 3;
		nvt_writel(ipe_control.reg, reg_addr + IPE_CONTROL_REGISTER_OFS);
	}

	for(i = 0; i < IQ_GAMMA_GAMMA_LEN; i += 2){
		curr_edge_gamma[j++] = curr_gamma[i];
	}

	for (i = 0; i < 21; i++) {
		ipe_edge_gamma_reg.bit.reg_edge_lut_0 = curr_edge_gamma[3*i+0];
		ipe_edge_gamma_reg.bit.reg_edge_lut_1 = curr_edge_gamma[3*i+1];
		ipe_edge_gamma_reg.bit.reg_edge_lut_2 = curr_edge_gamma[3*i+2];
		nvt_writel(ipe_edge_gamma_reg.reg, reg_addr + (EDGE_GAMMA0_OFS + (i * 4)));
	}

	ipe_edge_gamma_reg21.bit.reg_edge_lut_63 = curr_edge_gamma[63];
	ipe_edge_gamma_reg21.bit.reg_edge_lut_64 = curr_edge_gamma[64];
	nvt_writel(ipe_edge_gamma_reg21.reg, reg_addr + EDGE_GAMMA21_OFS);

}

static UINT32 *iq_builtin_ycurve_conv(UINT32 id, UINT32 *lut)
{
	INT32 i;

	if (ycurve_buffer[id] == NULL) {
		return NULL;
	}

	for (i = 0; i < 129; i++) {
		if (i % 2 == 0) {
			ycurve_buffer[id][i / 2] = lut[i];
		} else {
			ycurve_buffer[id][i / 2] |= (lut[i] << 10);
		}
	}

	vos_cpu_dcache_sync((VOS_ADDR)ycurve_buffer[id], YCURVE_BUFFER_SIZE, VOS_DMA_TO_DEVICE);

	return ycurve_buffer[id];
}

static void iq_builtin_set_ycurve(UINT32 id, IQ_PARAM_PTR *iq_param)
{
	T_IPE_CONTROL_REGISTER           ipe_control;
	T_IPE_EN_REGISTER                ipe_en_reg;
	T_DMA_TO_IPE_REGISTER_4          ipe_reg_4;

	unsigned int                     reg_addr;

	VOS_ADDR                         final_buffer_addr, phy_adr;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);

	ipe_en_reg.reg = nvt_readl(reg_addr + IPE_EN_REGISTER_OFS);
	ipe_en_reg.bit.reg_ycurve_en = iq_param->ycurve->enable;
	nvt_writel(ipe_en_reg.reg, reg_addr + IPE_EN_REGISTER_OFS);

	final_buffer_addr = (UINT32)iq_builtin_ycurve_conv(id, (UINT32 *)&iq_param->ycurve->ycurve_lut[0]);
	if (final_buffer_addr != 0) {
		phy_adr = vos_cpu_get_phy_addr((unsigned int)final_buffer_addr);
		ipe_reg_4.bit.dram_sai_ycurve = phy_adr >> 2;
		nvt_writel(ipe_reg_4.reg, reg_addr + DMA_TO_IPE_REGISTER_4_OFS);

		ipe_control.reg = nvt_readl(reg_addr + IPE_CONTROL_REGISTER_OFS);
		ipe_control.bit.reg_ipe_rwgamma = 1;
		ipe_control.bit.reg_ipe_rwgamma_opt = 3;
		nvt_writel(ipe_control.reg, reg_addr + IPE_CONTROL_REGISTER_OFS);
	}
}

static void iq_builtin_set_color(UINT32 gain, IQ_PARAM_PTR *iq_param, BOOL ui_night_mode, UINT32 ui_con_ratio, UINT32 ui_sat_ratio)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5                        color_adjust_reg;
	T_SAT_CONTROL_REGISTER                                         sat_control_reg;

	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_0                     fstab_reg_0;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_1                     fstab_reg_1;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_2                     fstab_reg_2;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_3                     fstab_reg_3;

	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0                     fdtab_reg_0;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1                     fdtab_reg_1;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2                     fdtab_reg_2;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3                     fdtab_reg_3;

	T_COLOR_CCONTAB_REGISTER_0                                     ccontab_reg_0;
	T_COLOR_CCONTAB_REGISTER_1                                     ccontab_reg_1;
	T_COLOR_CCONTAB_REGISTER_2                                     ccontab_reg_2;
	T_COLOR_CCONTAB_REGISTER_3                                     ccontab_reg_3;
	T_COLOR_CCONTAB_REGISTER_4                                     ccontab_reg_4;
	T_COLOR_CCONTAB_REGISTER_5                                     ccontab_reg_5;

	unsigned int                reg_addr;
	IQ_COLOR_TUNE_PARAM         *final_color_tune;

	UINT32                      iso_idx = 0;
	UINT8                       con2sat_adj;
	INT16                       sat_ofs = 0;
	UINT8                       c_con;
	INT32                       i;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->color->mode == IQ_OP_TYPE_AUTO) {
		final_color_tune = &iq_param->color->auto_param[iso_idx];
	} else {
		final_color_tune = &iq_param->color->manual_param;
	}

	if (ui_con_ratio > 100) {
		con2sat_adj = (UINT8)iq_intpl_value(ui_con_ratio, 0, 0, 18);
		c_con = (UINT8)IQ_CLAMP(final_color_tune->c_con + con2sat_adj, 0, 255);
	} else {
		con2sat_adj = (UINT8)iq_intpl_value(ui_con_ratio, 0, 18, 0);
		c_con = (UINT8)IQ_CLAMP(final_color_tune->c_con - con2sat_adj, 0, 255);
	}

	sat_ofs = (INT16)iq_intpl_value(ui_sat_ratio, 0, -128, 127);
	sat_control_reg.bit.reg_sat_ofs = sat_ofs;
	nvt_writel(sat_control_reg.reg, reg_addr + COLOR_CONTROL_REGISTER_OFS);

	color_adjust_reg.reg = nvt_readl(reg_addr + COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	color_adjust_reg.bit.reg_c_con = (c_con * iq_front_factor.c_con) / 100;
	if (ui_night_mode == TRUE) {
		color_adjust_reg.bit.reg_c_cbofs = 128;
		color_adjust_reg.bit.reg_c_crofs = 128;
		color_adjust_reg.bit.reg_c_con = 0;
	}
	nvt_writel(color_adjust_reg.reg, reg_addr + COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);

	fstab_reg_0.bit.reg_fstab0 = final_color_tune->fstab[0];
	fstab_reg_0.bit.reg_fstab1 = final_color_tune->fstab[1];
	fstab_reg_0.bit.reg_fstab2 = final_color_tune->fstab[2];
	fstab_reg_0.bit.reg_fstab3 = final_color_tune->fstab[3];
	nvt_writel(fstab_reg_0.reg, reg_addr + COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS);

	fstab_reg_1.bit.reg_fstab4 = final_color_tune->fstab[4];
	fstab_reg_1.bit.reg_fstab5 = final_color_tune->fstab[5];
	fstab_reg_1.bit.reg_fstab6 = final_color_tune->fstab[6];
	fstab_reg_1.bit.reg_fstab7 = final_color_tune->fstab[7];
	nvt_writel(fstab_reg_1.reg, reg_addr + COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS);

	fstab_reg_2.bit.reg_fstab8 = final_color_tune->fstab[8];
	fstab_reg_2.bit.reg_fstab9 = final_color_tune->fstab[9];
	fstab_reg_2.bit.reg_fstab10 = final_color_tune->fstab[10];
	fstab_reg_2.bit.reg_fstab11 = final_color_tune->fstab[11];
	nvt_writel(fstab_reg_2.reg, reg_addr + COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS);

	fstab_reg_3.bit.reg_fstab12 = final_color_tune->fstab[12];
	fstab_reg_3.bit.reg_fstab13 = final_color_tune->fstab[13];
	fstab_reg_3.bit.reg_fstab14 = final_color_tune->fstab[14];
	fstab_reg_3.bit.reg_fstab15 = final_color_tune->fstab[15];
	nvt_writel(fstab_reg_3.reg, reg_addr + COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS);

	fdtab_reg_0.bit.reg_fdtab0 = final_color_tune->fdtab[0];
	fdtab_reg_0.bit.reg_fdtab1 = final_color_tune->fdtab[1];
	fdtab_reg_0.bit.reg_fdtab2 = final_color_tune->fdtab[2];
	fdtab_reg_0.bit.reg_fdtab3 = final_color_tune->fdtab[3];
	nvt_writel(fdtab_reg_0.reg, reg_addr + COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS);

	fdtab_reg_1.bit.reg_fdtab4 = final_color_tune->fdtab[4];
	fdtab_reg_1.bit.reg_fdtab5 = final_color_tune->fdtab[5];
	fdtab_reg_1.bit.reg_fdtab6 = final_color_tune->fdtab[6];
	fdtab_reg_1.bit.reg_fdtab7 = final_color_tune->fdtab[7];
	nvt_writel(fdtab_reg_1.reg, reg_addr + COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS);

	fdtab_reg_2.bit.reg_fdtab8 = final_color_tune->fdtab[8];
	fdtab_reg_2.bit.reg_fdtab9 = final_color_tune->fdtab[9];
	fdtab_reg_2.bit.reg_fdtab10 = final_color_tune->fdtab[10];
	fdtab_reg_2.bit.reg_fdtab11 = final_color_tune->fdtab[11];
	nvt_writel(fdtab_reg_2.reg, reg_addr + COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS);

	fdtab_reg_3.bit.reg_fdtab12 = final_color_tune->fdtab[12];
	fdtab_reg_3.bit.reg_fdtab13 = final_color_tune->fdtab[13];
	fdtab_reg_3.bit.reg_fdtab14 = final_color_tune->fdtab[14];
	fdtab_reg_3.bit.reg_fdtab15 = final_color_tune->fdtab[15];
	nvt_writel(fdtab_reg_3.reg, reg_addr + COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS);

	ccontab_reg_0.bit.reg_ccontab0 = final_color_tune->cconlut[0];
	ccontab_reg_0.bit.reg_ccontab1 = final_color_tune->cconlut[1];
	ccontab_reg_0.bit.reg_ccontab2 = final_color_tune->cconlut[2];
	nvt_writel(ccontab_reg_0.reg, reg_addr + COLOR_CCONTAB_REGISTER_0_OFS);

	ccontab_reg_1.bit.reg_ccontab3 = final_color_tune->cconlut[3];
	ccontab_reg_1.bit.reg_ccontab4 = final_color_tune->cconlut[4];
	ccontab_reg_1.bit.reg_ccontab5 = final_color_tune->cconlut[5];
	nvt_writel(ccontab_reg_1.reg, reg_addr + COLOR_CCONTAB_REGISTER_1_OFS);

	ccontab_reg_2.bit.reg_ccontab6 = final_color_tune->cconlut[6];
	ccontab_reg_2.bit.reg_ccontab7 = final_color_tune->cconlut[7];
	ccontab_reg_2.bit.reg_ccontab8 = final_color_tune->cconlut[8];
	nvt_writel(ccontab_reg_2.reg, reg_addr + COLOR_CCONTAB_REGISTER_2_OFS);

	ccontab_reg_3.bit.reg_ccontab9 = final_color_tune->cconlut[9];
	ccontab_reg_3.bit.reg_ccontab10 = final_color_tune->cconlut[10];
	ccontab_reg_3.bit.reg_ccontab11 = final_color_tune->cconlut[11];
	nvt_writel(ccontab_reg_3.reg, reg_addr + COLOR_CCONTAB_REGISTER_3_OFS);

	ccontab_reg_4.bit.reg_ccontab12 = final_color_tune->cconlut[12];
	ccontab_reg_4.bit.reg_ccontab13 = final_color_tune->cconlut[13];
	ccontab_reg_4.bit.reg_ccontab14 = final_color_tune->cconlut[14];
	nvt_writel(ccontab_reg_4.reg, reg_addr + COLOR_CCONTAB_REGISTER_4_OFS);

	ccontab_reg_5.bit.reg_ccontab15 = final_color_tune->cconlut[15];
	ccontab_reg_5.bit.reg_ccontab16 = final_color_tune->cconlut[16];
	ccontab_reg_5.bit.reg_ccontab_sel = 0;
	nvt_writel(ccontab_reg_5.reg, reg_addr + COLOR_CCONTAB_REGISTER_5_OFS);
}

static void iq_builtin_set_contrast(ISP_BUILTIN_IQ_TRIG_MSG msg, UINT32 gain, IQ_PARAM_PTR *iq_param, UINT32 ui_bright_ratio, UINT32 ui_con_ratio)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1                     ycon_reg;
	T_LCE_REGISTER_1                                            lce_reg_1;
	T_LCE_REGISTER_2                                            lce_reg_2;
	T_LCE_REGISTER_3                                            lce_reg_3;
	T_IPE_MODE_REGISTER_1                                       ipe_mode_reg;
	T_DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER        defog_reg;
	T_LCE_REGISTER_0                                            lce_reg_0;
	T_COLOR_CONTROL_REGISTER                                    int_reg;

	unsigned int                                                reg_addr;
	IQ_CONTRAST_TUNE_PARAM                                      *final_contrast_tune;
	UINT32                                                      iso_idx = 0;
	UINT8                                                       lce_lum_wt_lut[IQ_CONTRAST_LCE_LEN];
	INT16                                                       con2int;
	INT16                                                       int_ofs = 0;
	INT16                                                       new_int_ofs = 0;
	UINT8                                                       y_con;
	INT32                                                       i;

	static UINT8                                                lce_frame_cnt;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->contrast->mode == IQ_OP_TYPE_AUTO) {
		final_contrast_tune = &iq_param->contrast->auto_param[iso_idx];
	} else {
		final_contrast_tune = &iq_param->contrast->manual_param;
	}

	y_con = (UINT8)iq_intpl_value(ui_con_ratio, final_contrast_tune->y_con, 0, 255);

	ycon_reg.reg = nvt_readl(reg_addr + COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	ycon_reg.bit.reg_y_con = y_con;
	nvt_writel(ycon_reg.reg, reg_addr + COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);

	int_reg.reg = nvt_readl(reg_addr + COLOR_CONTROL_REGISTER_OFS);
	new_int_ofs = (INT16)iq_intpl_value(ui_bright_ratio, int_ofs, -128, 127);
	con2int = 128 - y_con;
	int_reg.bit.reg_int_ofs = (INT16)IQ_CLAMP(new_int_ofs + con2int, -128, 127);
	nvt_writel(int_reg.reg, reg_addr + COLOR_CONTROL_REGISTER_OFS);

	ipe_mode_reg.reg = nvt_readl(reg_addr + IPE_MODE_REGISTER_1_OFS);
	if (msg == ISP_BUILTIN_IQ_TRIG_IPP_RESET) {
		ipe_mode_reg.bit.reg_lce_en = FALSE;
	} else {
		ipe_mode_reg.bit.reg_lce_en = TRUE;
	}
	nvt_writel(ipe_mode_reg.reg, reg_addr + IPE_MODE_REGISTER_1_OFS);

	if (lce_frame_cnt < IQ_LCE_STABLE_CNT) {
		for (i = 0; i < IQ_CONTRAST_LCE_LEN; i++) {
			lce_lum_wt_lut[i] = (UINT32)iq_cal(lce_frame_cnt * 100 / IQ_LCE_STABLE_CNT, final_contrast_tune->lce_lum_wt_lut[i], 0, 191);
		}
		lce_frame_cnt++;
	} else {
		memcpy(lce_lum_wt_lut, final_contrast_tune->lce_lum_wt_lut, sizeof(UINT8) * IQ_CONTRAST_LCE_LEN);
	}

	lce_reg_1.bit.reg_lce_lum_adj_lut0 = IQ_CLAMP((lce_lum_wt_lut[0] + 64), 0, 255);
	lce_reg_1.bit.reg_lce_lum_adj_lut1 = IQ_CLAMP((lce_lum_wt_lut[1] + 64), 0, 255);
	lce_reg_1.bit.reg_lce_lum_adj_lut2 = IQ_CLAMP((lce_lum_wt_lut[2] + 64), 0, 255);
	lce_reg_1.bit.reg_lce_lum_adj_lut3 = IQ_CLAMP((lce_lum_wt_lut[3] + 64), 0, 255);
	nvt_writel(lce_reg_1.reg, reg_addr + LCE_REGISTER_1_OFS);

	lce_reg_2.bit.reg_lce_lum_adj_lut4 = IQ_CLAMP((lce_lum_wt_lut[4] + 64), 0, 255);
	lce_reg_2.bit.reg_lce_lum_adj_lut5 = IQ_CLAMP((lce_lum_wt_lut[5] + 64), 0, 255);
	lce_reg_2.bit.reg_lce_lum_adj_lut6 = IQ_CLAMP((lce_lum_wt_lut[6] + 64), 0, 255);
	lce_reg_2.bit.reg_lce_lum_adj_lut7 = IQ_CLAMP((lce_lum_wt_lut[7] + 64), 0, 255);
	nvt_writel(lce_reg_2.reg, reg_addr + LCE_REGISTER_2_OFS);

	lce_reg_3.bit.reg_lce_lum_adj_lut8 = IQ_CLAMP((lce_lum_wt_lut[8] + 64), 0, 255);;
	nvt_writel(lce_reg_3.reg, reg_addr + LCE_REGISTER_3_OFS);

	lce_reg_0.bit.reg_lce_wt_diff_pos = 0;
	lce_reg_0.bit.reg_lce_wt_diff_neg = 0;
	lce_reg_0.bit.reg_lce_wt_diff_avg = 0x80;
	nvt_writel(lce_reg_0.reg, reg_addr + LCE_REGISTER_0_OFS);

	defog_reg.reg = nvt_readl(reg_addr + DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	defog_reg.bit.reg_defog_subimg_lofsi = 16;
	nvt_writel(defog_reg.reg, reg_addr + DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
}

static void iq_builtin_set_edge(UINT32 gain, IQ_PARAM_PTR *iq_param, UINT32 ui_sharp_ratio, ISP_BUILTIN_AE_STATUS ae_status)
{
	T_IPE_EN_REGISTER                             IPE_en_reg;
	T_OVERSHOOTING_REGISTER                       overshooting_reg;
	T_ES_MAP_LUT_0                                ES_map_lut_0;
	T_ES_MAP_LUT_1                                ES_map_lut_1;
	T_ES_MAP_LUT_2                                ES_map_lut_2;
	T_ES_MAP_LUT_3                                ES_map_lut_3;
	T_ED_MAP_LUT_0                                ED_map_lut_0;
	T_ED_MAP_LUT_1                                ED_map_lut_1;
	T_ED_MAP_LUT_2                                ED_map_lut_2;
	T_ED_MAP_LUT_3                                ED_map_lut_3;
	T_COLOR_EDGE_LUT_0                            COLOR_EDGE_lut_0;
	T_COLOR_EDGE_LUT_1                            COLOR_EDGE_lut_1;
	T_COLOR_EDGE_LUT_2                            COLOR_EDGE_lut_2;
	T_COLOR_EDGE_LUT_3                            COLOR_EDGE_lut_3;
	T_COLOR_EDGE_LUT_4                            COLOR_EDGE_lut_4;
	T_COLOR_EDGE_LUT_5                            COLOR_EDGE_lut_5;

	T_EDGE_ENH_ADJUSTMENT_0                       EDGE_ENH_adjustment_0;

	T_EDGE_REGION_EXTRACTION_REGISTER_1           EDGE_REGION_extraction_reg;
	T_EDGE_REGION_EXTRACTION_0                    EDGE_REGION_extraction_0;
	T_EDGE_REGION_EXTRACTION_1                    EDGE_REGION_extraction_1;
	T_EDGE_REGION_EXTRACTION_2                    EDGE_REGION_extraction_2;
	T_EDGE_REGION_EXTRACTION_3                    EDGE_REGION_extraction_3;
	T_EDGE_REGION_EXTRACTION_REGISTER_5           EDGE_REGION_extraction_5;

	T_EDGE_REGION_STRENGTH_REGISTER_2             edge_region_strength_reg2;
	T_EDGE_REGION_STRENGTH_REGISTER_3             edge_region_strength_reg3;

	T_OVERSHOOTING_SLOPE_0                        OVERSHOOTING_slope_0;

	T_ES_TABLE_0                                  ES_table_0;
	T_ED_TABLE_0                                  ED_table_0;

	unsigned int                                  reg_addr;
	IQ_EDGE_PARAM                                 *edge;
	IQ_EDGE_TUNE_PARAM                            *final_edge_tune;

	UINT32                                        iso_idx = 0;
	UINT32                                        edge_enh_p;
	UINT32                                        edge_enh_n;
	INT32                                         curr_slope_flat, curr_slope_edge;
	UINT32                                        slope_con_eng, slope_con_eng_hld, wlow, whigh, th_edge, th_flat;
	INT32                                         i;

	reg_addr = (unsigned int)ioremap_nocache(IPE_BASE_ADDR, IPE_SIZE);

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	edge = iq_param->edge;
	if (edge->mode == IQ_OP_TYPE_AUTO) {
		final_edge_tune = &iq_param->edge->auto_param[iso_idx];
	} else {
		final_edge_tune = &iq_param->edge->manual_param;
	}

	edge_enh_p = (UINT32)iq_cal(ui_sharp_ratio, final_edge_tune->edge_enh_p, 0, 1023);
	edge_enh_n = (UINT32)iq_cal(ui_sharp_ratio, final_edge_tune->edge_enh_n, 0, 1023);
	if (ae_status == ISP_BUILTIN_AE_STATUS_COARSE) {
		edge_enh_p = (edge_enh_p * iq_param->edge->reduce_ratio) >> 7;
		edge_enh_n = (edge_enh_n * iq_param->edge->reduce_ratio) >> 7;
	}

	IPE_en_reg.reg = nvt_readl(reg_addr + IPE_EN_REGISTER_OFS);
	IPE_en_reg.bit.reg_cadj_yenh_en = edge->enable;
	nvt_writel(IPE_en_reg.reg, reg_addr + IPE_EN_REGISTER_OFS);

	overshooting_reg.reg = nvt_readl(reg_addr + OVERSHOOTING_REGISTER_OFS);
	overshooting_reg.bit.reg_th_overshoot = edge->th_overshoot;
	overshooting_reg.bit.reg_th_undershoot = edge->th_undershoot;
	nvt_writel(overshooting_reg.reg, reg_addr + OVERSHOOTING_REGISTER_OFS);

	ES_map_lut_0.bit.reg_eslutl_0 = edge->es_map_lut[0];
	ES_map_lut_0.bit.reg_eslutl_1 = edge->es_map_lut[1];
	ES_map_lut_0.bit.reg_eslutl_2 = edge->es_map_lut[2];
	ES_map_lut_0.bit.reg_eslutl_3 = edge->es_map_lut[3];
	nvt_writel(ES_map_lut_0.reg, reg_addr + ES_MAP_LUT_0_OFS);

	ES_map_lut_1.bit.reg_eslutl_4 = edge->es_map_lut[4];
	ES_map_lut_1.bit.reg_eslutl_5 = edge->es_map_lut[5];
	ES_map_lut_1.bit.reg_eslutl_6 = edge->es_map_lut[6];
	ES_map_lut_1.bit.reg_eslutl_7 = edge->es_map_lut[7];
	nvt_writel(ES_map_lut_1.reg, reg_addr + ES_MAP_LUT_1_OFS);

	ES_map_lut_2.bit.reg_esluth_0 = edge->es_map_lut[8];
	ES_map_lut_2.bit.reg_esluth_1 = edge->es_map_lut[9];
	ES_map_lut_2.bit.reg_esluth_2 = edge->es_map_lut[10];
	ES_map_lut_2.bit.reg_esluth_3 = edge->es_map_lut[11];
	nvt_writel(ES_map_lut_2.reg, reg_addr + ES_MAP_LUT_2_OFS);

	ES_map_lut_3.bit.reg_esluth_4 = edge->es_map_lut[12];
	ES_map_lut_3.bit.reg_esluth_5 = edge->es_map_lut[13];
	ES_map_lut_3.bit.reg_esluth_6 = edge->es_map_lut[14];
	ES_map_lut_3.bit.reg_esluth_7 = edge->es_map_lut[15];
	nvt_writel(ES_map_lut_3.reg, reg_addr + ES_MAP_LUT_3_OFS);

	ED_map_lut_0.bit.reg_edlutl_0 = edge->edge_map_lut[0];
	ED_map_lut_0.bit.reg_edlutl_1 = edge->edge_map_lut[1];
	ED_map_lut_0.bit.reg_edlutl_2 = edge->edge_map_lut[2];
	ED_map_lut_0.bit.reg_edlutl_3 = edge->edge_map_lut[3];
	nvt_writel(ED_map_lut_0.reg, reg_addr + ED_MAP_LUT_0_OFS);

	ED_map_lut_1.bit.reg_edlutl_4 = edge->edge_map_lut[4];
	ED_map_lut_1.bit.reg_edlutl_5 = edge->edge_map_lut[5];
	ED_map_lut_1.bit.reg_edlutl_6 = edge->edge_map_lut[6];
	ED_map_lut_1.bit.reg_edlutl_7 = edge->edge_map_lut[7];
	nvt_writel(ED_map_lut_1.reg, reg_addr + ED_MAP_LUT_1_OFS);

	ED_map_lut_2.bit.reg_edluth_0 = edge->edge_map_lut[8];
	ED_map_lut_2.bit.reg_edluth_1 = edge->edge_map_lut[9];
	ED_map_lut_2.bit.reg_edluth_2 = edge->edge_map_lut[10];
	ED_map_lut_2.bit.reg_edluth_3 = edge->edge_map_lut[11];
	nvt_writel(ED_map_lut_2.reg, reg_addr + ED_MAP_LUT_2_OFS);

	ED_map_lut_3.bit.reg_edluth_4 = edge->edge_map_lut[12];
	ED_map_lut_3.bit.reg_edluth_5 = edge->edge_map_lut[13];
	ED_map_lut_3.bit.reg_edluth_6 = edge->edge_map_lut[14];
	ED_map_lut_3.bit.reg_edluth_7 = edge->edge_map_lut[15];
	nvt_writel(ED_map_lut_3.reg, reg_addr + ED_MAP_LUT_3_OFS);

	COLOR_EDGE_lut_0.bit.reg_cedgem0 = edge->edge_tab[0];
	COLOR_EDGE_lut_0.bit.reg_cedgem1 = edge->edge_tab[1];
	COLOR_EDGE_lut_0.bit.reg_cedgem2 = edge->edge_tab[2];
	COLOR_EDGE_lut_0.bit.reg_cedgem3 = edge->edge_tab[3];
	nvt_writel(COLOR_EDGE_lut_0.reg, reg_addr + COLOR_EDGE_LUT_0_OFS);

	COLOR_EDGE_lut_1.bit.reg_cedgem4 = edge->edge_tab[4];
	COLOR_EDGE_lut_1.bit.reg_cedgem5 = edge->edge_tab[5];
	COLOR_EDGE_lut_1.bit.reg_cedgem6 = edge->edge_tab[6];
	COLOR_EDGE_lut_1.bit.reg_cedgem7 = edge->edge_tab[7];
	nvt_writel(COLOR_EDGE_lut_1.reg, reg_addr + COLOR_EDGE_LUT_1_OFS);

	COLOR_EDGE_lut_2.bit.reg_cedgem8 = edge->edge_tab[8];
	COLOR_EDGE_lut_2.bit.reg_cedgem9 = edge->edge_tab[9];
	COLOR_EDGE_lut_2.bit.reg_cedgem10 = edge->edge_tab[10];
	COLOR_EDGE_lut_2.bit.reg_cedgem11 = edge->edge_tab[11];
	nvt_writel(COLOR_EDGE_lut_2.reg, reg_addr + COLOR_EDGE_LUT_2_OFS);

	COLOR_EDGE_lut_3.bit.reg_cedgem12 = edge->edge_tab[12];
	COLOR_EDGE_lut_3.bit.reg_cedgem13 = edge->edge_tab[13];
	COLOR_EDGE_lut_3.bit.reg_cedgem14 = edge->edge_tab[14];
	COLOR_EDGE_lut_3.bit.reg_cedgem15 = edge->edge_tab[15];
	nvt_writel(COLOR_EDGE_lut_3.reg, reg_addr + COLOR_EDGE_LUT_3_OFS);

	COLOR_EDGE_lut_4.bit.reg_cedgem16 = edge->edge_tab[16];
	COLOR_EDGE_lut_4.bit.reg_cedgem17 = edge->edge_tab[17];
	COLOR_EDGE_lut_4.bit.reg_cedgem18 = edge->edge_tab[18];
	COLOR_EDGE_lut_4.bit.reg_cedgem19 = edge->edge_tab[19];
	nvt_writel(COLOR_EDGE_lut_4.reg, reg_addr + COLOR_EDGE_LUT_4_OFS);

	COLOR_EDGE_lut_5.bit.reg_cedgem20 = edge->edge_tab[20];
	COLOR_EDGE_lut_5.bit.reg_cedgem21 = edge->edge_tab[21];
	COLOR_EDGE_lut_5.bit.reg_cedgem22 = edge->edge_tab[22];
	COLOR_EDGE_lut_5.bit.reg_cedgem23 = edge->edge_tab[23];
	nvt_writel(COLOR_EDGE_lut_5.reg, reg_addr + COLOR_EDGE_LUT_5_OFS);

	EDGE_ENH_adjustment_0.reg = nvt_readl(reg_addr + EDGE_ENH_ADJUSTMENT_0_OFS);
	EDGE_ENH_adjustment_0.bit.reg_y_enh_p = (edge_enh_p * iq_front_factor.overshoot) / 100;
	EDGE_ENH_adjustment_0.bit.reg_y_enh_n = (edge_enh_n * iq_front_factor.overshoot) / 100;
	nvt_writel(EDGE_ENH_adjustment_0.reg, reg_addr + EDGE_ENH_ADJUSTMENT_0_OFS);

	EDGE_REGION_extraction_reg.bit.reg_w_ker_thin = iq_edge_ker_tab[final_edge_tune->thin_freq][0];
	EDGE_REGION_extraction_reg.bit.reg_w_ker_robust = iq_edge_ker_tab[final_edge_tune->robust_freq][0];
	EDGE_REGION_extraction_reg.bit.reg_iso_ker_thin = iq_edge_ker_tab[final_edge_tune->thin_freq][1];
	EDGE_REGION_extraction_reg.bit.reg_iso_ker_robust = iq_edge_ker_tab[final_edge_tune->robust_freq][1];
	nvt_writel(EDGE_REGION_extraction_reg.reg, reg_addr + EDGE_REGION_EXTRACTION_REGISTER_1_OFS);

	EDGE_REGION_extraction_0.reg = nvt_readl(reg_addr + EDGE_REGION_EXTRACTION_0_OFS);
	EDGE_REGION_extraction_0.bit.reg_w_low = final_edge_tune->wt_low;
	EDGE_REGION_extraction_0.bit.reg_w_high = final_edge_tune->wt_high;
	nvt_writel(EDGE_REGION_extraction_0.reg, reg_addr + EDGE_REGION_EXTRACTION_0_OFS);

	EDGE_REGION_extraction_1.reg = nvt_readl(reg_addr + EDGE_REGION_EXTRACTION_1_OFS);
	EDGE_REGION_extraction_1.bit.reg_w_ker_thin_hld = iq_edge_ker_tab[IQ_CLAMP(final_edge_tune->thin_freq + 2, 0, 16)][0];;
	EDGE_REGION_extraction_1.bit.reg_w_ker_robust_hld = iq_edge_ker_tab[final_edge_tune->robust_freq][0];
	EDGE_REGION_extraction_1.bit.reg_iso_ker_thin_hld = iq_edge_ker_tab[IQ_CLAMP(final_edge_tune->thin_freq + 2, 0, 16)][1];
	EDGE_REGION_extraction_1.bit.reg_iso_ker_robust_hld = iq_edge_ker_tab[final_edge_tune->robust_freq][1];
	EDGE_REGION_extraction_1.bit.reg_w_hld_low = final_edge_tune->wt_low;
	EDGE_REGION_extraction_1.bit.reg_w_hld_high = final_edge_tune->wt_high;
	nvt_writel(EDGE_REGION_extraction_1.reg, reg_addr + EDGE_REGION_EXTRACTION_1_OFS);

	EDGE_REGION_extraction_2.bit.reg_th_flat = final_edge_tune->th_flat_high;
	EDGE_REGION_extraction_2.bit.reg_th_edge = final_edge_tune->th_edge_low;
	nvt_writel(EDGE_REGION_extraction_2.reg, reg_addr + EDGE_REGION_EXTRACTION_2_OFS);

	EDGE_REGION_extraction_3.reg = nvt_readl(reg_addr + EDGE_REGION_EXTRACTION_3_OFS);
	EDGE_REGION_extraction_3.bit.reg_th_flat_hld = (final_edge_tune->th_flat_high * 200) >> 8;
	EDGE_REGION_extraction_3.bit.reg_th_edge_hld = final_edge_tune->th_edge_low;
	nvt_writel(EDGE_REGION_extraction_3.reg, reg_addr + EDGE_REGION_EXTRACTION_3_OFS);

	curr_slope_flat = (final_edge_tune->th_flat_high == final_edge_tune->th_flat_low) ? 0x7fff : ((64 - (INT32)final_edge_tune->str_flat) << 10) / ((INT32)final_edge_tune->th_flat_high - (INT32)final_edge_tune->th_flat_low);
	if (curr_slope_flat > 0x7fff) {
		curr_slope_flat = 0x7fff;
	}
	edge_region_strength_reg2.bit.slope_flat = 0 - (INT16)curr_slope_flat;
	curr_slope_edge = (final_edge_tune->th_edge_high == final_edge_tune->th_edge_low) ? 0x7fff : (((INT32)final_edge_tune->str_edge - 64) << 10) / ((INT32)final_edge_tune->th_edge_high - (INT32)final_edge_tune->th_edge_low);
	if (curr_slope_edge > 0x7fff) {
		curr_slope_edge = 0x7fff;
	}
	edge_region_strength_reg2.bit.slope_edge = (INT16)curr_slope_edge;
	nvt_writel(edge_region_strength_reg2.reg, reg_addr + EDGE_REGION_STRENGTH_REGISTER_2_OFS);

	edge_region_strength_reg3.bit.str_flat = final_edge_tune->str_flat;
	edge_region_strength_reg3.bit.str_edge = final_edge_tune->str_edge;
	nvt_writel(edge_region_strength_reg3.reg, reg_addr + EDGE_REGION_STRENGTH_REGISTER_3_OFS);

	OVERSHOOTING_slope_0.bit.reg_slope_overshoot = final_edge_tune->overshoot_str;
	OVERSHOOTING_slope_0.bit.reg_slope_undershoot = final_edge_tune->undershoot_str;
	nvt_writel(OVERSHOOTING_slope_0.reg, reg_addr + OVERSHOOTING_SLOPE_OFS);

	ES_table_0.bit.reg_esthrl = final_edge_tune->es_ethr_low;
	ES_table_0.bit.reg_esthrh = final_edge_tune->es_ethr_high;
	ES_table_0.bit.reg_establ = final_edge_tune->es_etab_low;
	ES_table_0.bit.reg_estabh = final_edge_tune->es_etab_high;
	nvt_writel(ES_table_0.reg, reg_addr + ES_TABLE_0_OFS);

	ED_table_0.bit.reg_edthrl = final_edge_tune->edge_ethr_low;
	ED_table_0.bit.reg_edthrh = final_edge_tune->edge_ethr_high;
	ED_table_0.bit.reg_edtabl = final_edge_tune->edge_etab_low;
	ED_table_0.bit.reg_edtabh = final_edge_tune->edge_etab_high;
	ED_table_0.bit.reg_edin_sel = 1;
	nvt_writel(ED_table_0.reg, reg_addr + ED_TABLE_0_OFS);

	wlow = final_edge_tune->wt_low;
	whigh = final_edge_tune->wt_high;
	th_edge = final_edge_tune->th_edge_low;
	th_flat = final_edge_tune->th_flat_high;
	slope_con_eng = (th_edge == th_flat) ? 65535 : (((whigh -  wlow) * 1024) / abs(th_edge - th_flat));

	wlow = final_edge_tune->wt_low;
	whigh = final_edge_tune->wt_high;
	th_edge = final_edge_tune->th_edge_low;
	th_flat = (final_edge_tune->th_flat_high * 200) >> 8;
	slope_con_eng_hld = (th_edge == th_flat) ? 65535 : (((whigh -  wlow) * 1024) / abs(th_edge - th_flat));

	EDGE_REGION_extraction_5.bit.reg_slope_con_eng = slope_con_eng;
	EDGE_REGION_extraction_5.bit.reg_slope_hld_con_eng = slope_con_eng_hld;
	nvt_writel(EDGE_REGION_extraction_5.reg, reg_addr + EDGE_REGION_EXTRACTION_REGISTER_5_OFS);
}

static void iq_builtin_set_3dnr(UINT32 id, ISP_BUILTIN_IQ_TRIG_MSG msg, UINT32 gain, IQ_PARAM_PTR *iq_param, BOOL ui_night_mode, UINT32 ui_3dnr_ratio)
{
	T_IME_FUNCTION_CONTROL_REGISTER0                           ime_function_control_reg0;
	T_IME_TMNR_CONTROL_REGISTER0                               ime_3dnr_control_reg_0;
	T_IME_TMNR_CONTROL_REGISTER1                               ime_3dnr_control_reg_1;

	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0             ime_3dnr_sad_penalty_0;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1             ime_3dnr_sad_penalty_1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2             ime_3dnr_sad_penalty_2;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3             ime_3dnr_switch_th_0;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4             ime_3dnr_switch_th_1;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5             ime_3dnr_me_control_reg5;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6             ime_3dnr_detail_penalty;
	T_IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7             ime_3dnr_probability;

	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0           ime_3dnr_sad_base_0;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1           ime_3dnr_sad_base_1;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2           ime_3dnr_sad_base_2;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3           ime_3dnr_sad_base_3;

	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0              ime_3dnr_md_sad_coeffa_0;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1              ime_3dnr_md_sad_coeffa_1;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2              ime_3dnr_md_sad_coeffb_0;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3              ime_3dnr_md_sad_coeffb_1;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4              ime_3dnr_md_sad_coeffb_2;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5              ime_3dnr_md_sad_coeffb_3;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6              ime_3dnr_md_sad_std_0;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7              ime_3dnr_md_sad_std_1;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8              ime_3dnr_md_sad_std_2;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9              ime_3dnr_md_sad_std_3;
	T_IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10             ime_3dnr_md_k;

	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4           ime_3dnr_mc_sad_coeffa_0;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5           ime_3dnr_mc_sad_coeffa_1;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6           ime_3dnr_mc_sad_coeffb_0;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7           ime_3dnr_mc_sad_coeffb_1;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8           ime_3dnr_mc_sad_coeffb_2;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9           ime_3dnr_mc_sad_coeffb_3;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10          ime_3dnr_mc_sad_std_0;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11          ime_3dnr_mc_sad_std_1;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12          ime_3dnr_mc_sad_std_2;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13          ime_3dnr_mc_sad_std_3;
	T_IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14          ime_3dnr_mc_k;

	T_IME_3DNR_TF0_REGISTER0                                   ime_3dnr_tf0_reg0;
	T_IME_3DNR_TF0_REGISTER1                                   ime_3dnr_tf0_reg1;
	T_IME_3DNR_TF0_REGISTER2                                   ime_3dnr_tf0_reg2;

	T_IME_3DNR_PS_CONTROL_REGISTER0                            ime_3dnr_mv_th;
	T_IME_3DNR_PS_CONTROL_REGISTER1                            ime_3dnr_ps_mix_ratio;
	T_IME_3DNR_PS_CONTROL_REGISTER3                            ime_3dnr_ps_control;
	T_IME_3DNR_PS_CONTROL_REGISTER4                            ime_3dnr_ps_ds_th;

	T_IME_3DNR_NR_CONTROL_REGISTER0                            ime_3dnr_residue;
	T_IME_3DNR_NR_CONTROL_REGISTER1                            ime_3dnr_freq_w;
	T_IME_3DNR_NR_CONTROL_REGISTER3                            ime_3dnr_luma_w_0;
	T_IME_3DNR_NR_CONTROL_REGISTER4                            ime_3dnr_luma_w_1;
	T_IME_3DNR_NR_CONTROL_REGISTER6                            ime_3dnr_pre_filter_str;
	T_IME_3DNR_NR_CONTROL_REGISTER8                            ime_3dnr_pre_filter_ratio;
	T_IME_3DNR_NR_CONTROL_REGISTER9                            ime_3dnr_snr_tnr_str;
	T_IME_3DNR_NR_CONTROL_REGISTER10                           ime_3dnr_nr_base_th;
	T_IME_3DNR_NR_CONTROL_REGISTER11                           ime_3dnr_luma_3d_rto;
	T_IME_3DNR_NR_CONTROL_REGISTER12                           ime_3dnr_luma_3d_lut_0;
	T_IME_3DNR_NR_CONTROL_REGISTER13                           ime_3dnr_luma_3d_lut_1;
	T_IME_3DNR_NR_CONTROL_REGISTER14                           ime_3dnr_chroma_3d_lut_0;
	T_IME_3DNR_NR_CONTROL_REGISTER15                           ime_3dnr_chroma_3d_lut_1;
	T_IME_3DNR_NR_CONTROL_REGISTER16                           ime_3dnr_chroma_3d_ratio;

	T_IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0                 ime_3dnr_fast_converge;

	unsigned int                         reg_addr;
	IQ_3DNR_TUNE_PARAM                   *final_3dnr_tune;

	UINT32                          iso_idx = 0;
	UINT32                          cur_probability;
	static UINT32                   probability_auto_shift;
	UINT32                          probability[8]= {0}, final_probability[8]= {0};
	UINT32                          pre_filter_str[IQ_3DNR_FREQ_NUM];
	UINT32                          tnr_base_th;
	UINT32                          luma_3d_lut[IQ_3DNR_F4_LUT_LEN];
	UINT32                          chroma_3d_lut[IQ_3DNR_F4_LUT_LEN];
	UINT32                          y_residue_th[IQ_3DNR_OBJECT_NUM];
	UINT32                          scene_change_w = 0;
	static UINT32                   chroma_3d_lut_linear[8] = {0, 16, 32, 48, 64, 80, 96, 112};
	static UINT32                   count;
	INT32                           i, j;

	reg_addr = (unsigned int)ioremap_nocache(IME_BASE_ADDR, IME_SIZE);

	if(isp_builtin_get_night_mode(id) == 1) {
		scene_change_w = 100;
	} else {
		scene_change_w = isp_builtin_get_scene_chg_w(id);
	}

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->_3dnr->mode == IQ_OP_TYPE_AUTO) {
		final_3dnr_tune = &iq_param->_3dnr->auto_param[iso_idx];
	} else {
		final_3dnr_tune = &iq_param->_3dnr->manual_param;
	}

	if (ui_night_mode == TRUE) {
		final_3dnr_tune->chroma_residue_th = 0;
		for (j = 0; j < 8; j++) {
			final_3dnr_tune->chroma_3d_lut[j] = chroma_3d_lut_linear[j];
		}
	}

	for (i = 0; i < TMNR_PRE_FILTER_STRENGTH_TAB; i++) {
		pre_filter_str[i] = iq_cal(ui_3dnr_ratio, final_3dnr_tune->pre_filter_str[i], 0, 255);
	}
	tnr_base_th = iq_cal(ui_3dnr_ratio, final_3dnr_tune->tnr_base_th, 0, 65535);
	for (i = 0; i < TMNR_NR_LUMA_LUT_TAB; i++) {
		luma_3d_lut[i] = iq_cal(200 - ui_3dnr_ratio, final_3dnr_tune->luma_3d_lut[i], 0, 16 * i);
		chroma_3d_lut[i] = iq_cal(200 - ui_3dnr_ratio, final_3dnr_tune->chroma_3d_lut[i], 0, 16 * i);
	}

	ime_3dnr_control_reg_1.reg = nvt_readl(reg_addr + IME_TMNR_CONTROL_REGISTER1_OFS);
	ime_3dnr_control_reg_1.bit.ime_3dnr_dbg_mode = 0;
	ime_3dnr_control_reg_1.bit.ime_3dnr_ps_fastc_en = iq_param->_3dnr->fcvg_enable;
	nvt_writel(ime_3dnr_control_reg_1.reg, reg_addr + IME_TMNR_CONTROL_REGISTER1_OFS);

	ime_function_control_reg0.reg = nvt_readl(reg_addr + IME_FUNCTION_CONTROL_REGISTER0_OFS);
	if (msg == ISP_BUILTIN_IQ_TRIG_IPP_RESET) {
		ime_function_control_reg0.bit.ime_3dnr_en = FALSE;
	} else {
		ime_function_control_reg0.bit.ime_3dnr_en = iq_param->_3dnr->enable;
	}
	nvt_writel(ime_function_control_reg0.reg, reg_addr + IME_FUNCTION_CONTROL_REGISTER0_OFS);

	ime_3dnr_control_reg_0.reg = nvt_readl(reg_addr + IME_TMNR_CONTROL_REGISTER0_OFS);
	ime_3dnr_control_reg_0.bit.ime_3dnr_pf_type = final_3dnr_tune->pf_type;
	nvt_writel(ime_3dnr_control_reg_0.reg, reg_addr + IME_TMNR_CONTROL_REGISTER0_OFS);

	ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_0 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[0], 0, 100);
	ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_1 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[1], 0, 100);
	ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_2 = iq_intpl(scene_change_w, (INT32)0, (INT32)final_3dnr_tune->sad_penalty[2], 0, 100);
	nvt_writel(ime_3dnr_sad_penalty_0.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS);

	#if IQ_BUILTIN_DBG_MSG
	if(iq_frm_cnt[id] < iq_front_dtsi.smooth_thr) {
		printk("id = %d, cnt = %d, w = %d, sad_penalty = %d, %d, %d\r\n",id, iq_frm_cnt[id], scene_change_w, ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_0, ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_1, ime_3dnr_sad_penalty_0.bit.ime_3dnr_me_sad_penalty_2);
	}
	#endif

	ime_3dnr_sad_penalty_1.bit.ime_3dnr_me_sad_penalty_3 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[3], 0, 100);
	ime_3dnr_sad_penalty_1.bit.ime_3dnr_me_sad_penalty_4 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[4], 0, 100);
	ime_3dnr_sad_penalty_1.bit.ime_3dnr_me_sad_penalty_5 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[5], 0, 100);
	nvt_writel(ime_3dnr_sad_penalty_1.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS);

	ime_3dnr_sad_penalty_2.bit.ime_3dnr_me_sad_penalty_6 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[6], 0, 100);
	ime_3dnr_sad_penalty_2.bit.ime_3dnr_me_sad_penalty_7 = iq_intpl(scene_change_w, (INT32)1023, (INT32)final_3dnr_tune->sad_penalty[7], 0, 100);
	nvt_writel(ime_3dnr_sad_penalty_2.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS);

	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty0 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[0], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty1 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[1], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty2 = iq_intpl(scene_change_w, (INT32)0, (INT32)final_3dnr_tune->detail_penalty[2], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty3 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[3], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty4 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[4], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty5 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[5], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty6 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[6], 0, 100);
	ime_3dnr_detail_penalty.bit.ime_3dnr_me_detail_penalty7 = iq_intpl(scene_change_w, (INT32)15, (INT32)final_3dnr_tune->detail_penalty[7], 0, 100);
	nvt_writel(ime_3dnr_detail_penalty.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS);

	ime_3dnr_switch_th_0.bit.ime_3dnr_me_switch_th0 = final_3dnr_tune->switch_th[0];
	ime_3dnr_switch_th_0.bit.ime_3dnr_me_switch_th1 = final_3dnr_tune->switch_th[1];
	ime_3dnr_switch_th_0.bit.ime_3dnr_me_switch_th2 = final_3dnr_tune->switch_th[2];
	ime_3dnr_switch_th_0.bit.ime_3dnr_me_switch_th3 = final_3dnr_tune->switch_th[3];
	nvt_writel(ime_3dnr_switch_th_0.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3_OFS);

	ime_3dnr_switch_th_1.bit.ime_3dnr_me_switch_th4 = final_3dnr_tune->switch_th[4];
	ime_3dnr_switch_th_1.bit.ime_3dnr_me_switch_th5 = final_3dnr_tune->switch_th[5];
	ime_3dnr_switch_th_1.bit.ime_3dnr_me_switch_th6 = final_3dnr_tune->switch_th[6];
	ime_3dnr_switch_th_1.bit.ime_3dnr_me_switch_th7 = final_3dnr_tune->switch_th[7];
	nvt_writel(ime_3dnr_switch_th_1.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4_OFS);

	ime_3dnr_me_control_reg5.bit.ime_3dnr_me_switch_ratio = final_3dnr_tune->switch_rto;
	ime_3dnr_me_control_reg5.bit.ime_3dnr_me_cost_blend = final_3dnr_tune->cost_blend;
	nvt_writel(ime_3dnr_me_control_reg5.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS);

	ime_3dnr_probability.reg = nvt_readl(reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);
	cur_probability = final_3dnr_tune->probability;
	for (i = cur_probability; i > 0; i--) {
		probability[i - 1] = 1;
		cur_probability--;
	}
	for (i = 0; i < 8; i++) {
		final_probability[i] = probability[(i + probability_auto_shift) % 8];
	}
	probability_auto_shift++;
	ime_3dnr_probability.bit.ime_3dnr_me_probability0 = final_probability[0];
	ime_3dnr_probability.bit.ime_3dnr_me_probability1 = final_probability[1];
	ime_3dnr_probability.bit.ime_3dnr_me_probability2 = final_probability[2];
	ime_3dnr_probability.bit.ime_3dnr_me_probability3 = final_probability[3];
	ime_3dnr_probability.bit.ime_3dnr_me_probability4 = final_probability[4];
	ime_3dnr_probability.bit.ime_3dnr_me_probability5 = final_probability[5];
	ime_3dnr_probability.bit.ime_3dnr_me_probability6 = final_probability[6];
	ime_3dnr_probability.bit.ime_3dnr_me_probability7 = final_probability[7];
	nvt_writel(ime_3dnr_probability.reg, reg_addr + IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS);

	ime_3dnr_sad_base_0.bit.ime_3dnr_mc_sad_base0 = final_3dnr_tune->sad_base[0];
	ime_3dnr_sad_base_0.bit.ime_3dnr_mc_sad_base1 = final_3dnr_tune->sad_base[1];
	nvt_writel(ime_3dnr_sad_base_0.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS);

	ime_3dnr_sad_base_1.bit.ime_3dnr_mc_sad_base2 = final_3dnr_tune->sad_base[2];
	ime_3dnr_sad_base_1.bit.ime_3dnr_mc_sad_base3 = final_3dnr_tune->sad_base[3];
	nvt_writel(ime_3dnr_sad_base_1.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1_OFS);

	ime_3dnr_sad_base_2.bit.ime_3dnr_mc_sad_base4 = final_3dnr_tune->sad_base[4];
	ime_3dnr_sad_base_2.bit.ime_3dnr_mc_sad_base5 = final_3dnr_tune->sad_base[5];
	nvt_writel(ime_3dnr_sad_base_2.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2_OFS);

	ime_3dnr_sad_base_3.bit.ime_3dnr_mc_sad_base6 = final_3dnr_tune->sad_base[6];
	ime_3dnr_sad_base_3.bit.ime_3dnr_mc_sad_base7 = final_3dnr_tune->sad_base[7];
	nvt_writel(ime_3dnr_sad_base_3.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3_OFS);

	ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a0 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[0], 0, 100);
	ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a1 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[1], 0, 100);
	ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a2 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[2], 0, 100);
	ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a3 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[3], 0, 100);
	nvt_writel(ime_3dnr_md_sad_coeffa_0.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0_OFS);

	ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a4 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[4], 0, 100);
	ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a5 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[5], 0, 100);
	ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a6 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[6], 0, 100);
	ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a7 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->sad_coefa[7], 0, 100);
	nvt_writel(ime_3dnr_md_sad_coeffa_1.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1_OFS);

	ime_3dnr_md_sad_coeffb_0.bit.ime_3dnr_md_sad_coef_b0 = final_3dnr_tune->sad_coefb[0];
	ime_3dnr_md_sad_coeffb_0.bit.ime_3dnr_md_sad_coef_b1 = final_3dnr_tune->sad_coefb[1];
	nvt_writel(ime_3dnr_md_sad_coeffb_0.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2_OFS);

	ime_3dnr_md_sad_coeffb_1.bit.ime_3dnr_md_sad_coef_b2 = final_3dnr_tune->sad_coefb[2];
	ime_3dnr_md_sad_coeffb_1.bit.ime_3dnr_md_sad_coef_b3 = final_3dnr_tune->sad_coefb[3];
	nvt_writel(ime_3dnr_md_sad_coeffb_1.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3_OFS);

	ime_3dnr_md_sad_coeffb_2.bit.ime_3dnr_md_sad_coef_b4 = final_3dnr_tune->sad_coefb[4];
	ime_3dnr_md_sad_coeffb_2.bit.ime_3dnr_md_sad_coef_b5 = final_3dnr_tune->sad_coefb[5];
	nvt_writel(ime_3dnr_md_sad_coeffb_2.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4_OFS);

	ime_3dnr_md_sad_coeffb_3.bit.ime_3dnr_md_sad_coef_b6 = final_3dnr_tune->sad_coefb[6];
	ime_3dnr_md_sad_coeffb_3.bit.ime_3dnr_md_sad_coef_b7 = final_3dnr_tune->sad_coefb[7];
	nvt_writel(ime_3dnr_md_sad_coeffb_3.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5_OFS);

	ime_3dnr_md_sad_std_0.bit.ime_3dnr_md_sad_std0 = final_3dnr_tune->sad_std[0];
	ime_3dnr_md_sad_std_0.bit.ime_3dnr_md_sad_std1 = final_3dnr_tune->sad_std[1];
	nvt_writel(ime_3dnr_md_sad_std_0.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6_OFS);

	ime_3dnr_md_sad_std_1.bit.ime_3dnr_md_sad_std2 = final_3dnr_tune->sad_std[2];
	ime_3dnr_md_sad_std_1.bit.ime_3dnr_md_sad_std3 = final_3dnr_tune->sad_std[3];
	nvt_writel(ime_3dnr_md_sad_std_1.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7_OFS);

	ime_3dnr_md_sad_std_2.bit.ime_3dnr_md_sad_std4 = final_3dnr_tune->sad_std[4];
	ime_3dnr_md_sad_std_2.bit.ime_3dnr_md_sad_std5 = final_3dnr_tune->sad_std[5];
	nvt_writel(ime_3dnr_md_sad_std_2.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8_OFS);

	ime_3dnr_md_sad_std_3.bit.ime_3dnr_md_sad_std6 = final_3dnr_tune->sad_std[6];
	ime_3dnr_md_sad_std_3.bit.ime_3dnr_md_sad_std7 = final_3dnr_tune->sad_std[7];
	nvt_writel(ime_3dnr_md_sad_std_3.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9_OFS);

	ime_3dnr_md_k.bit.ime_3dnr_md_k1 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->fth[0], 0, 100);
	ime_3dnr_md_k.bit.ime_3dnr_md_k2 = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->fth[1], 0, 100);
	nvt_writel(ime_3dnr_md_k.reg, reg_addr + IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS);

	ime_3dnr_mc_sad_coeffa_0.bit.ime_3dnr_mc_sad_coef_a0 = ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a0;
	ime_3dnr_mc_sad_coeffa_0.bit.ime_3dnr_mc_sad_coef_a1 = ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a1;
	ime_3dnr_mc_sad_coeffa_0.bit.ime_3dnr_mc_sad_coef_a2 = ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a2;
	ime_3dnr_mc_sad_coeffa_0.bit.ime_3dnr_mc_sad_coef_a3 = ime_3dnr_md_sad_coeffa_0.bit.ime_3dnr_md_sad_coef_a3;
	nvt_writel(ime_3dnr_mc_sad_coeffa_0.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4_OFS);

	ime_3dnr_mc_sad_coeffa_1.bit.ime_3dnr_mc_sad_coef_a4 = ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a4;
	ime_3dnr_mc_sad_coeffa_1.bit.ime_3dnr_mc_sad_coef_a5 = ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a5;
	ime_3dnr_mc_sad_coeffa_1.bit.ime_3dnr_mc_sad_coef_a6 = ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a6;
	ime_3dnr_mc_sad_coeffa_1.bit.ime_3dnr_mc_sad_coef_a7 = ime_3dnr_md_sad_coeffa_1.bit.ime_3dnr_md_sad_coef_a7;
	nvt_writel(ime_3dnr_mc_sad_coeffa_1.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5_OFS);

	ime_3dnr_mc_sad_coeffb_0.bit.ime_3dnr_mc_sad_coef_b0 = ime_3dnr_md_sad_coeffb_0.bit.ime_3dnr_md_sad_coef_b0;
	ime_3dnr_mc_sad_coeffb_0.bit.ime_3dnr_mc_sad_coef_b1 = ime_3dnr_md_sad_coeffb_0.bit.ime_3dnr_md_sad_coef_b1;
	nvt_writel(ime_3dnr_mc_sad_coeffb_0.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6_OFS);

	ime_3dnr_mc_sad_coeffb_1.bit.ime_3dnr_mc_sad_coef_b2 = ime_3dnr_md_sad_coeffb_1.bit.ime_3dnr_md_sad_coef_b2;
	ime_3dnr_mc_sad_coeffb_1.bit.ime_3dnr_mc_sad_coef_b3 = ime_3dnr_md_sad_coeffb_1.bit.ime_3dnr_md_sad_coef_b3;
	nvt_writel(ime_3dnr_mc_sad_coeffb_1.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7_OFS);

	ime_3dnr_mc_sad_coeffb_2.bit.ime_3dnr_mc_sad_coef_b4 = ime_3dnr_md_sad_coeffb_2.bit.ime_3dnr_md_sad_coef_b4;
	ime_3dnr_mc_sad_coeffb_2.bit.ime_3dnr_mc_sad_coef_b5 = ime_3dnr_md_sad_coeffb_2.bit.ime_3dnr_md_sad_coef_b5;
	nvt_writel(ime_3dnr_mc_sad_coeffb_2.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8_OFS);

	ime_3dnr_mc_sad_coeffb_3.bit.ime_3dnr_mc_sad_coef_b6 = ime_3dnr_md_sad_coeffb_3.bit.ime_3dnr_md_sad_coef_b6;
	ime_3dnr_mc_sad_coeffb_3.bit.ime_3dnr_mc_sad_coef_b7 = ime_3dnr_md_sad_coeffb_3.bit.ime_3dnr_md_sad_coef_b7;
	nvt_writel(ime_3dnr_mc_sad_coeffb_3.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9_OFS);

	ime_3dnr_mc_sad_std_0.bit.ime_3dnr_mc_sad_std0 = ime_3dnr_md_sad_std_0.bit.ime_3dnr_md_sad_std0;
	ime_3dnr_mc_sad_std_0.bit.ime_3dnr_mc_sad_std1 = ime_3dnr_md_sad_std_0.bit.ime_3dnr_md_sad_std1;
	nvt_writel(ime_3dnr_mc_sad_std_0.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10_OFS);

	ime_3dnr_mc_sad_std_1.bit.ime_3dnr_mc_sad_std2 = ime_3dnr_md_sad_std_1.bit.ime_3dnr_md_sad_std2;
	ime_3dnr_mc_sad_std_1.bit.ime_3dnr_mc_sad_std3 = ime_3dnr_md_sad_std_1.bit.ime_3dnr_md_sad_std3;
	nvt_writel(ime_3dnr_mc_sad_std_1.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11_OFS);

	ime_3dnr_mc_sad_std_2.bit.ime_3dnr_mc_sad_std4 = ime_3dnr_md_sad_std_2.bit.ime_3dnr_md_sad_std4;
	ime_3dnr_mc_sad_std_2.bit.ime_3dnr_mc_sad_std5 = ime_3dnr_md_sad_std_2.bit.ime_3dnr_md_sad_std5;
	nvt_writel(ime_3dnr_mc_sad_std_2.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12_OFS);

	ime_3dnr_mc_sad_std_3.bit.ime_3dnr_mc_sad_std6 = ime_3dnr_md_sad_std_3.bit.ime_3dnr_md_sad_std6;
	ime_3dnr_mc_sad_std_3.bit.ime_3dnr_mc_sad_std7 = ime_3dnr_md_sad_std_3.bit.ime_3dnr_md_sad_std7;
	nvt_writel(ime_3dnr_mc_sad_std_3.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13_OFS);

	ime_3dnr_mc_k.bit.ime_3dnr_mc_k1 = ime_3dnr_md_k.bit.ime_3dnr_md_k1;
	ime_3dnr_mc_k.bit.ime_3dnr_mc_k2 = ime_3dnr_md_k.bit.ime_3dnr_md_k2;
	nvt_writel(ime_3dnr_mc_k.reg, reg_addr + IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS);

	ime_3dnr_mv_th.reg = nvt_readl(reg_addr + IME_3DNR_PS_CONTROL_REGISTER0_OFS);
	ime_3dnr_mv_th.bit.ime_3dnr_ps_mv_th = iq_intpl(scene_change_w, (INT32)63, (INT32)final_3dnr_tune->mv_th, 0, 100);
	nvt_writel(ime_3dnr_mv_th.reg, reg_addr + IME_3DNR_PS_CONTROL_REGISTER0_OFS);

	ime_3dnr_ps_mix_ratio.bit.ime_3dnr_ps_mix_ratio0 = final_3dnr_tune->mix_ratio[0];
	ime_3dnr_ps_mix_ratio.bit.ime_3dnr_ps_mix_ratio1 = final_3dnr_tune->mix_ratio[1];
	nvt_writel(ime_3dnr_ps_mix_ratio.reg, reg_addr + IME_3DNR_PS_CONTROL_REGISTER1_OFS);

	ime_3dnr_ps_control.bit.ime_3dnr_ps_mix_slope0 = (final_3dnr_tune->mix_ratio[0] * 1024) / 512;
	ime_3dnr_ps_control.bit.ime_3dnr_ps_mix_slope1 = ((final_3dnr_tune->mix_ratio[1] - final_3dnr_tune->mix_ratio[0]) * 1024) / (1024 - 512);
	nvt_writel(ime_3dnr_ps_control.reg, reg_addr + IME_3DNR_PS_CONTROL_REGISTER3_OFS);

	ime_3dnr_ps_ds_th.reg = nvt_readl(reg_addr + IME_3DNR_PS_CONTROL_REGISTER4_OFS);
	ime_3dnr_ps_ds_th.bit.ime_3dnr_ps_ds_th = iq_intpl(scene_change_w, (INT32)31, (INT32)final_3dnr_tune->ds_th, 0, 100);
	nvt_writel(ime_3dnr_ps_ds_th.reg, reg_addr + IME_3DNR_PS_CONTROL_REGISTER4_OFS);

	for (i = 0; i > IQ_3DNR_OBJECT_NUM; i++) {
		y_residue_th[i] = iq_intpl(scene_change_w, (INT32)1, (INT32)final_3dnr_tune->luma_residue_th[i], 0, 100);
	}

	// real setting: 0,0,0,1,0,0,0,1,0,0...
	if (count < 3) {
		count++;
	} else {
		for (i = 0; i > IQ_3DNR_OBJECT_NUM; i++) {
			if (y_residue_th[i] == 0) {
				y_residue_th[i] = 1;
			}
		}
		count = 0;
	}

	ime_3dnr_residue.bit.ime_3dnr_nr_residue_th_y0 = y_residue_th[0];
	ime_3dnr_residue.bit.ime_3dnr_nr_residue_th_y1 = y_residue_th[1];
	ime_3dnr_residue.bit.ime_3dnr_nr_residue_th_y2 = y_residue_th[2];
	ime_3dnr_residue.bit.ime_3dnr_nr_residue_th_c = final_3dnr_tune->chroma_residue_th;
	nvt_writel(ime_3dnr_residue.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER0_OFS);

	ime_3dnr_tf0_reg0.bit.ime_3dnr_nr_y_tf0_blur_str0 = final_3dnr_tune->tf0_blur_str[0];
	ime_3dnr_tf0_reg0.bit.ime_3dnr_nr_y_tf0_blur_str1 = final_3dnr_tune->tf0_blur_str[1];
	ime_3dnr_tf0_reg0.bit.ime_3dnr_nr_y_tf0_blur_str2 = final_3dnr_tune->tf0_blur_str[2];
	ime_3dnr_tf0_reg0.bit.ime_3dnr_nr_y_tf0_str0 = final_3dnr_tune->tf0_y_str[0];
	nvt_writel(ime_3dnr_tf0_reg0.reg, reg_addr + IME_3DNR_TF0_REGISTER0_OFS);

	ime_3dnr_tf0_reg1.bit.ime_3dnr_nr_y_tf0_str1 = final_3dnr_tune->tf0_y_str[1];
	ime_3dnr_tf0_reg1.bit.ime_3dnr_nr_y_tf0_str2 = final_3dnr_tune->tf0_y_str[2];
	ime_3dnr_tf0_reg1.bit.ime_3dnr_nr_c_tf0_str0 = final_3dnr_tune->tf0_c_str[0];
	ime_3dnr_tf0_reg1.bit.ime_3dnr_nr_c_tf0_str1 = final_3dnr_tune->tf0_c_str[1];
	nvt_writel(ime_3dnr_tf0_reg1.reg, reg_addr + IME_3DNR_TF0_REGISTER1_OFS);

	ime_3dnr_tf0_reg2.bit.ime_3dnr_nr_c_tf0_str2 = final_3dnr_tune->tf0_c_str[2];
	nvt_writel(ime_3dnr_tf0_reg2.reg, reg_addr + IME_3DNR_TF0_REGISTER2_OFS);

	ime_3dnr_pre_filter_str.bit.ime_3dnr_nr_pre_filtering_str0 = pre_filter_str[0];
	ime_3dnr_pre_filter_str.bit.ime_3dnr_nr_pre_filtering_str1 = pre_filter_str[1];
	ime_3dnr_pre_filter_str.bit.ime_3dnr_nr_pre_filtering_str2 = pre_filter_str[2];
	ime_3dnr_pre_filter_str.bit.ime_3dnr_nr_pre_filtering_str3 = pre_filter_str[3];
	nvt_writel(ime_3dnr_pre_filter_str.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER6_OFS);

	ime_3dnr_pre_filter_ratio.bit.ime_3dnr_nr_pre_filtering_ratio0 = iq_intpl(scene_change_w, (INT32)255, (INT32)final_3dnr_tune->pre_filter_rto[0], 0, 100);
	ime_3dnr_pre_filter_ratio.bit.ime_3dnr_nr_pre_filtering_ratio1 = iq_intpl(scene_change_w, (INT32)255, (INT32)final_3dnr_tune->pre_filter_rto[1], 0, 100);
	ime_3dnr_pre_filter_ratio.bit.ime_3dnr_nr_snr_str0 = final_3dnr_tune->snr_str[0];
	ime_3dnr_pre_filter_ratio.bit.ime_3dnr_nr_snr_str1 = iq_intpl(scene_change_w, (INT32)64, (INT32)final_3dnr_tune->snr_str[1], 0, 100);
	nvt_writel(ime_3dnr_pre_filter_ratio.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER8_OFS);

	ime_3dnr_nr_base_th.bit.ime_3dnr_nr_base_th_snr = final_3dnr_tune->snr_base_th;
	ime_3dnr_nr_base_th.bit.ime_3dnr_nr_base_th_tnr = tnr_base_th;
	nvt_writel(ime_3dnr_nr_base_th.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER10_OFS);

	ime_3dnr_freq_w.bit.ime_3dnr_nr_freq_w0 = final_3dnr_tune->freq_wet[0];
	ime_3dnr_freq_w.bit.ime_3dnr_nr_freq_w1 = final_3dnr_tune->freq_wet[1];
	ime_3dnr_freq_w.bit.ime_3dnr_nr_freq_w2 = final_3dnr_tune->freq_wet[2];
	ime_3dnr_freq_w.bit.ime_3dnr_nr_freq_w3 = final_3dnr_tune->freq_wet[3];
	nvt_writel(ime_3dnr_freq_w.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER1_OFS);

	ime_3dnr_luma_w_0.bit.ime_3dnr_nr_luma_w0 = final_3dnr_tune->luma_wet[0];
	ime_3dnr_luma_w_0.bit.ime_3dnr_nr_luma_w1 = final_3dnr_tune->luma_wet[1];
	ime_3dnr_luma_w_0.bit.ime_3dnr_nr_luma_w2 = final_3dnr_tune->luma_wet[2];
	ime_3dnr_luma_w_0.bit.ime_3dnr_nr_luma_w3 = final_3dnr_tune->luma_wet[3];
	nvt_writel(ime_3dnr_luma_w_0.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER3_OFS);

	ime_3dnr_luma_w_1.bit.ime_3dnr_nr_luma_w4 = final_3dnr_tune->luma_wet[4];
	ime_3dnr_luma_w_1.bit.ime_3dnr_nr_luma_w5 = final_3dnr_tune->luma_wet[5];
	ime_3dnr_luma_w_1.bit.ime_3dnr_nr_luma_w6 = final_3dnr_tune->luma_wet[6];
	ime_3dnr_luma_w_1.bit.ime_3dnr_nr_luma_w7 = final_3dnr_tune->luma_wet[7];
	nvt_writel(ime_3dnr_luma_w_1.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER4_OFS);

	ime_3dnr_snr_tnr_str.bit.ime_3dnr_nr_snr_str2 = iq_intpl(scene_change_w, (INT32)64, (INT32)final_3dnr_tune->snr_str[2], 0, 100);
	ime_3dnr_snr_tnr_str.bit.ime_3dnr_nr_tnr_str0 = iq_intpl(scene_change_w, (INT32)64, (INT32)final_3dnr_tune->tnr_str[0], 0, 100);
	ime_3dnr_snr_tnr_str.bit.ime_3dnr_nr_tnr_str1 = iq_intpl(scene_change_w, (INT32)64, (INT32)final_3dnr_tune->tnr_str[1], 0, 100);
	ime_3dnr_snr_tnr_str.bit.ime_3dnr_nr_tnr_str2 = iq_intpl(scene_change_w, (INT32)64, (INT32)final_3dnr_tune->tnr_str[2], 0, 100);
	nvt_writel(ime_3dnr_snr_tnr_str.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER9_OFS);

	ime_3dnr_luma_3d_lut_0.bit.ime_3dnr_nr_y_3d_lut0 = iq_intpl(scene_change_w, (INT32)0, luma_3d_lut[0], 0, 100);
	ime_3dnr_luma_3d_lut_0.bit.ime_3dnr_nr_y_3d_lut1 = iq_intpl(scene_change_w, (INT32)16, luma_3d_lut[1], 0, 100);
	ime_3dnr_luma_3d_lut_0.bit.ime_3dnr_nr_y_3d_lut2 = iq_intpl(scene_change_w, (INT32)32, luma_3d_lut[2], 0, 100);
	ime_3dnr_luma_3d_lut_0.bit.ime_3dnr_nr_y_3d_lut3 = iq_intpl(scene_change_w, (INT32)48, luma_3d_lut[3], 0, 100);
	nvt_writel(ime_3dnr_luma_3d_lut_0.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER12_OFS);

	ime_3dnr_luma_3d_lut_1.bit.ime_3dnr_nr_y_3d_lut4 = iq_intpl(scene_change_w, (INT32)64, luma_3d_lut[4], 0, 100);
	ime_3dnr_luma_3d_lut_1.bit.ime_3dnr_nr_y_3d_lut5 = iq_intpl(scene_change_w, (INT32)80, luma_3d_lut[5], 0, 100);
	ime_3dnr_luma_3d_lut_1.bit.ime_3dnr_nr_y_3d_lut6 = iq_intpl(scene_change_w, (INT32)96, luma_3d_lut[6], 0, 100);
	ime_3dnr_luma_3d_lut_1.bit.ime_3dnr_nr_y_3d_lut7 = iq_intpl(scene_change_w, (INT32)112, luma_3d_lut[7], 0, 100);
	nvt_writel(ime_3dnr_luma_3d_lut_1.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER13_OFS);

	ime_3dnr_luma_3d_rto.bit.ime_3dnr_nr_y_3d_ratio0 = final_3dnr_tune->luma_3d_rto[0];
	ime_3dnr_luma_3d_rto.bit.ime_3dnr_nr_y_3d_ratio0 = final_3dnr_tune->luma_3d_rto[1];
	nvt_writel(ime_3dnr_luma_3d_rto.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER11_OFS);

	ime_3dnr_chroma_3d_lut_0.bit.ime_3dnr_nr_c_3d_lut0 = chroma_3d_lut[0];
	ime_3dnr_chroma_3d_lut_0.bit.ime_3dnr_nr_c_3d_lut1 = chroma_3d_lut[1];
	ime_3dnr_chroma_3d_lut_0.bit.ime_3dnr_nr_c_3d_lut2 = chroma_3d_lut[2];
	ime_3dnr_chroma_3d_lut_0.bit.ime_3dnr_nr_c_3d_lut3 = chroma_3d_lut[3];
	nvt_writel(ime_3dnr_chroma_3d_lut_0.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER14_OFS);

	ime_3dnr_chroma_3d_lut_1.bit.ime_3dnr_nr_c_3d_lut4 = chroma_3d_lut[4];
	ime_3dnr_chroma_3d_lut_1.bit.ime_3dnr_nr_c_3d_lut5 = chroma_3d_lut[5];
	ime_3dnr_chroma_3d_lut_1.bit.ime_3dnr_nr_c_3d_lut6 = chroma_3d_lut[6];
	ime_3dnr_chroma_3d_lut_1.bit.ime_3dnr_nr_c_3d_lut7 = chroma_3d_lut[7];
	nvt_writel(ime_3dnr_chroma_3d_lut_1.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER15_OFS);

	ime_3dnr_chroma_3d_ratio.bit.ime_3dnr_nr_c_3d_ratio0 = final_3dnr_tune->chroma_3d_rto[0];
	ime_3dnr_chroma_3d_ratio.bit.ime_3dnr_nr_c_3d_ratio1 = final_3dnr_tune->chroma_3d_rto[1];
	nvt_writel(ime_3dnr_chroma_3d_ratio.reg, reg_addr + IME_3DNR_NR_CONTROL_REGISTER16_OFS);

	ime_3dnr_fast_converge.bit.ime_3dnr_fast_converge_sp = final_3dnr_tune->fcvg_start_point;
	ime_3dnr_fast_converge.bit.ime_3dnr_fast_converge_step = final_3dnr_tune->fcvg_step_size;
	nvt_writel(ime_3dnr_fast_converge.reg, reg_addr + IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0_OFS);
}

static void iq_builtin_set_post_sharpen(UINT32 id, UINT32 gain, IQ_PARAM_PTR *iq_param)
{
	KDRV_H26XENC_SPN               post_sharpen_cfg = {0};

	IQ_POST_SHARPEN_TUNE_PARAM     *final_post_sharpen;

	UINT32                         iso_idx = 0;
	INT32                          i;

	if (gain >= iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].iso) {
		iso_idx = iso_map_tab[IQ_GAIN_ID_MAX_NUM - 1].index;
	}

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i-1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	if (iq_param->post_sharpen->mode == IQ_OP_TYPE_AUTO) {
		final_post_sharpen = &iq_param->post_sharpen->auto_param[iso_idx];
	} else {
		final_post_sharpen = &iq_param->post_sharpen->manual_param;
	}

	post_sharpen_cfg.bEnable = iq_param->post_sharpen->enable;
	post_sharpen_cfg.ucConEng = 4;
	post_sharpen_cfg.ucSlopConEng = 0;
	post_sharpen_cfg.ucBHC = final_post_sharpen->bright_halo_clip;
	post_sharpen_cfg.ucDHC = final_post_sharpen->dark_halo_clip;
	post_sharpen_cfg.ucEWT = final_post_sharpen->edge_weight_th;
	post_sharpen_cfg.ucEWG = final_post_sharpen->edge_weight_gain;
	post_sharpen_cfg.ucEdgeSharpStr1 = final_post_sharpen->edge_sharp_str;
	post_sharpen_cfg.ucCT = final_post_sharpen->coring_th;
	post_sharpen_cfg.ucNL = final_post_sharpen->noise_level;
	post_sharpen_cfg.ucBIG = final_post_sharpen->blend_inv_gamma;
	post_sharpen_cfg.usFlatTh = final_post_sharpen->th_flat;
	post_sharpen_cfg.usEdgeTh = final_post_sharpen->th_edge;
	post_sharpen_cfg.ucLowWt = 0;
	post_sharpen_cfg.ucHightWt = 0;
	post_sharpen_cfg.usHLDFlatTh = 150;
	post_sharpen_cfg.usHLDEdgeTh = 350;
	post_sharpen_cfg.ucHLDLowWt = 0;
	post_sharpen_cfg.ucHLDHightWt = 16;
	post_sharpen_cfg.ucEdgeStr = final_post_sharpen->edge_region_str;
	post_sharpen_cfg.ucTransitionStr = final_post_sharpen->tarnsition_region_str;
	post_sharpen_cfg.ucMotionStr = final_post_sharpen->motion_edge_w_str;
	post_sharpen_cfg.ucStaticStr = final_post_sharpen->static_edge_w_str;
	post_sharpen_cfg.ucHLDSlopConEng = 81;
	post_sharpen_cfg.ucFlatStr = 6;
	post_sharpen_cfg.usHLDLumTh = 255;
	memcpy(post_sharpen_cfg.ucNC, final_post_sharpen->noise_curve, sizeof(UINT8) * IQ_POST_SHARPEN_CURVE_NUM);

	vdoenc_builtin_set(id, BUILTIN_VDOENC_ISP_ITEM_SPN, &post_sharpen_cfg);
}

void iq_builtin_front_dtsi_load(UINT32 id)
{
	CHAR node_path[SUB_NODE_LENGTH];
	CHAR sub_node_name[SUB_NODE_LENGTH];
	struct device_node* of_node;

	if (id == 0) {
		sprintf(node_path, "/fastboot/front_iq_0");
	} else if (id == 1) {
		sprintf(node_path, "/fastboot/front_iq_1");
	} else {
		sprintf(node_path, "/fastboot/front_iq_0");
	}

	sprintf(sub_node_name, "%s/%s", node_path, "smooth_frm");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.smooth_thr, sizeof(iq_front_dtsi.smooth_thr)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "iso_w");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)iq_front_dtsi.iso_w, sizeof(iq_front_dtsi.iso_w)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "rth_nlm");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.rth_nlm, sizeof(iq_front_dtsi.rth_nlm)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "rth_nlm_lut");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.rth_nlm_lut, sizeof(iq_front_dtsi.rth_nlm_lut)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "clamp_th");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.clamp_th, sizeof(iq_front_dtsi.clamp_th)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "saturation");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.c_con, sizeof(iq_front_dtsi.c_con)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "edge_enh");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&iq_front_dtsi.overshoot, sizeof(iq_front_dtsi.overshoot)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	#if IQ_BUILTIN_DBG_MSG
	printk("smooth_thr ========== %d\r\n", iq_front_dtsi.smooth_thr);
	printk("iso_w =============== %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r\n",
											iq_front_dtsi.iso_w[IQ_GAIN_1X],
											iq_front_dtsi.iso_w[IQ_GAIN_2X],
											iq_front_dtsi.iso_w[IQ_GAIN_4X],
											iq_front_dtsi.iso_w[IQ_GAIN_8X],
											iq_front_dtsi.iso_w[IQ_GAIN_16X],
											iq_front_dtsi.iso_w[IQ_GAIN_32X],
											iq_front_dtsi.iso_w[IQ_GAIN_64X],
											iq_front_dtsi.iso_w[IQ_GAIN_128X],
											iq_front_dtsi.iso_w[IQ_GAIN_256X],
											iq_front_dtsi.iso_w[IQ_GAIN_512X],
											iq_front_dtsi.iso_w[IQ_GAIN_1024X],
											iq_front_dtsi.iso_w[IQ_GAIN_2048X],
											iq_front_dtsi.iso_w[IQ_GAIN_4096X],
											iq_front_dtsi.iso_w[IQ_GAIN_8192X],
											iq_front_dtsi.iso_w[IQ_GAIN_16384X],
											iq_front_dtsi.iso_w[IQ_GAIN_32768X]
											);
	printk("rth_nlm ============= %d\r\n", iq_front_dtsi.rth_nlm);
	printk("rth_nlm_lut ========= %d\r\n", iq_front_dtsi.rth_nlm_lut);
	printk("clamp_th ============ %d\r\n", iq_front_dtsi.clamp_th);
	printk("c_con =============== %d\r\n", iq_front_dtsi.c_con);
	printk("overshoot =========== %d\r\n", iq_front_dtsi.overshoot);
	#endif
}

//=============================================================================
// external functions
//=============================================================================
BOOL iq_builtin_get_direct_mode(void)
{
	T_DCE_FUNCTION_REGISTER dce_func_reg;
	unsigned int reg_addr;

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);

	dce_func_reg.reg = nvt_readl(reg_addr + DCE_FUNCTION_REGISTER_OFS);

	if (dce_func_reg.bit.DCE_OP == 2) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void iq_builtin_get_histo(UINT32 id, ISP_BUILTIN_HISTO_RSLT *histo_rslt)
{
	UINT16 *p_stcs = histo_rslt->hist_stcs_pre_wdr;
	UINT32 addr_ofs, i = 0;
	T_HISTOGRAM_REGISTER1 histo_reg_0;
	unsigned int reg_addr;

	if (histo_rslt == NULL) {
		return;
	}

	reg_addr = (unsigned int)ioremap_nocache(DCE_BASE_ADDR, DCE_SIZE);
	for (i = 0; i < 64; i++) {
		addr_ofs = i * 4;
		histo_reg_0.reg = nvt_readl(reg_addr + HISTOGRAM_REGISTER1_OFS + addr_ofs);

		*(p_stcs + 2 * i) = (histo_reg_0.bit.HISTOGRAM_BIN0);
		*(p_stcs + 2 * i + 1) = (histo_reg_0.bit.HISTOGRAM_BIN1);
	}
}

void iq_builtin_get_win_info(UINT32 id, SIE_CA_WIN_INFO *sie_ca_win_info, SIE_LA_WIN_INFO *sie_la_win_info)
{
	T_STCS_CA_WIN_SZ            ca_win_sz;
	T_STCS_CA_WIN_NUM           ca_win_num;
	T_STCS_LA_WIN_SZ            la_win_sz;
	T_STCS_LA_WIN_NUM           la_win_num;
	unsigned int                reg_addr;

	switch (id) {
		default:
		case 0:
			reg_addr = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
	}

	ca_win_sz.reg = nvt_readl(reg_addr + STCS_CA_WIN_SZ_OFS);
	sie_ca_win_info->uiWinSzX = ca_win_sz.reg & 0xff;
	sie_ca_win_info->uiWinSzY = (ca_win_sz.reg >> 8) & 0xff;

	ca_win_num.reg = nvt_readl(reg_addr + STCS_CA_WIN_NUM_OFS);
	sie_ca_win_info->uiWinNmX = ca_win_num.reg & 0x1f;
	sie_ca_win_info->uiWinNmY = (ca_win_num.reg >> 8) & 0x1f;

	la_win_sz.reg = nvt_readl(reg_addr + STCS_LA_WIN_SZ_OFS);
	sie_la_win_info->uiWinSzX = la_win_sz.reg & 0xff;
	sie_la_win_info->uiWinSzY = (la_win_sz.reg >> 8) & 0xff;

	la_win_num.reg = nvt_readl(reg_addr + STCS_LA_WIN_NUM_OFS);
	sie_la_win_info->uiWinNmX = la_win_num.reg & 0x1f;
	sie_la_win_info->uiWinNmY = (la_win_num.reg >> 8) & 0x1f;
	#if 0
	printk("id = %d, ca win = %d %d %d %d, la win = %d %d %d %d \r\n", id, sie_ca_win_info->uiWinNmX, sie_ca_win_info->uiWinNmY, sie_ca_win_info->uiWinSzX, sie_ca_win_info->uiWinSzY,
																		sie_la_win_info->uiWinNmX, sie_la_win_info->uiWinNmY, sie_la_win_info->uiWinSzX, sie_la_win_info->uiWinSzY);
	#endif
}

void iq_builtin_set_ca_crop(UINT32 id, BOOL enable)
{
	T_R38_ENGINE_TIMING r38_eng_timing;
	T_R224_STCS_CA r224_stcs_ca;
	T_R228_STCS_CA r228_stcs_ca;
	unsigned int reg_addr;
	INT32 i;

	switch (id) {
		default:
		case 0:
			reg_addr = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
			break;
		case 1:
			reg_addr = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
			break;
		case 2:
			reg_addr = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
			break;
		case 3:
			reg_addr = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
			break;
		case 4:
			reg_addr = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
			break;
	}

	r38_eng_timing.reg = nvt_readl(reg_addr + R38_ENGINE_TIMING_OFS);

	r228_stcs_ca.reg = nvt_readl(reg_addr + R228_STCS_CA_OFS);
	if (enable) {
		r228_stcs_ca.bit.CA_CROP_SZY = r38_eng_timing.bit.CRP_SZY * 9 / 10;  // reduce 10% of ca window
	} else {
		r228_stcs_ca.bit.CA_CROP_SZY = r38_eng_timing.bit.CRP_SZY;
	}
	nvt_writel(r228_stcs_ca.reg, reg_addr + R228_STCS_CA_OFS);

	r224_stcs_ca.reg = nvt_readl(reg_addr + R224_STCS_CA_OFS);
	r224_stcs_ca.bit.SMPL_Y_FACT  =  ((r228_stcs_ca.bit.CA_CROP_SZY/2)-1)*1024/255;
	nvt_writel(r224_stcs_ca.reg, reg_addr + R224_STCS_CA_OFS);

	if (isp_builtin_get_shdr_enable(id)) {
		for (i = ISP_BUILTIN_ID_MAX_NUM - 1; i >= 0; i--) {
			if ((0x1 << i) & isp_builtin_get_shdr_id_mask(id)) {
				break;
			}
		}

		switch (i) {
			default:
			case 0:
				reg_addr = (unsigned int)ioremap_nocache(SIE_BASE_ADDR, SIE_SIZE);
				break;
			case 1:
				reg_addr = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);
				break;
			case 2:
				reg_addr = (unsigned int)ioremap_nocache(SIE3_BASE_ADDR, SIE_SIZE);
				break;
			case 3:
				reg_addr = (unsigned int)ioremap_nocache(SIE4_BASE_ADDR, SIE_SIZE);
				break;
			case 4:
				reg_addr = (unsigned int)ioremap_nocache(SIE5_BASE_ADDR, SIE_SIZE);
				break;
		}

		reg_addr = (unsigned int)ioremap_nocache(SIE2_BASE_ADDR, SIE_SIZE);

		nvt_writel(r228_stcs_ca.reg, reg_addr + R228_STCS_CA_OFS);
		nvt_writel(r224_stcs_ca.reg, reg_addr + R224_STCS_CA_OFS);
	}
	printk("img crop = %d, ca crop = %d, factor = %d \r\n", r38_eng_timing.bit.CRP_SZY, r228_stcs_ca.bit.CA_CROP_SZY, r224_stcs_ca.bit.SMPL_Y_FACT);
}

static INT32 iq_builtin_get_param_addr(IQ_ID id)
{
	UINT32 total_param_size;
	UINT32 temp_addr = 0;
	IQ_PARAM_PTR *iq_param_temp = NULL;
	void *param_mem_addr = NULL;
	static BOOL use_param_phy_addr = TRUE;
	int align_byte = 4;

	if (use_param_phy_addr == TRUE) {
		use_param_phy_addr = FALSE;
		iq_param_get_param(&temp_addr);
		iq_param[id] = (IQ_PARAM_PTR *)temp_addr;
	} else {
		total_param_size = ALIGN_CEIL(sizeof(IQ_OB_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_NR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_CFA_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_VA_PARAM), align_byte)+ ALIGN_CEIL(sizeof(IQ_GAMMA_PARAM), align_byte) +
							ALIGN_CEIL(sizeof(IQ_CCM_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_COLOR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_CONTRAST_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_EDGE_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_3DNR_PARAM), align_byte) +
							ALIGN_CEIL(sizeof(IQ_WDR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_SHDR_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_RGBIR_PARAM), align_byte) +
							ALIGN_CEIL(sizeof(IQ_COMPANDING_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_POST_SHARPEN_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_RGBIR_ENH_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_YCURVE_PARAM), align_byte) +
							ALIGN_CEIL(sizeof(IQ_DPC_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_SHADING_PARAM), align_byte) +ALIGN_CEIL(sizeof(IQ_SHADING_INTER_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_SHADING_EXT_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_LDC_PARAM), align_byte) + ALIGN_CEIL(sizeof(IQ_SHADING_EXT_PARAM), align_byte);

		param_mem_addr = kzalloc(total_param_size, GFP_KERNEL);
		if (param_mem_addr == NULL) {
			printk("fail to allocate iq parameter fail!\n");
			return -E_SYS;
		}

		iq_param_memalloc_addr[id].ob = (IQ_OB_PARAM *)param_mem_addr;
		iq_param_memalloc_addr[id].nr = (IQ_NR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].ob + ALIGN_CEIL(sizeof(IQ_OB_PARAM), align_byte));
		iq_param_memalloc_addr[id].cfa = (IQ_CFA_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].nr + ALIGN_CEIL(sizeof(IQ_NR_PARAM), align_byte));
		iq_param_memalloc_addr[id].va = (IQ_VA_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].cfa + ALIGN_CEIL(sizeof(IQ_CFA_PARAM), align_byte));
		iq_param_memalloc_addr[id].gamma = (IQ_GAMMA_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].va + ALIGN_CEIL(sizeof(IQ_VA_PARAM), align_byte));
		iq_param_memalloc_addr[id].ccm = (IQ_CCM_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].gamma + ALIGN_CEIL(sizeof(IQ_GAMMA_PARAM), align_byte));
		iq_param_memalloc_addr[id].color = (IQ_COLOR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].ccm + ALIGN_CEIL(sizeof(IQ_CCM_PARAM), align_byte));
		iq_param_memalloc_addr[id].contrast = (IQ_CONTRAST_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].color + ALIGN_CEIL(sizeof(IQ_COLOR_PARAM), align_byte));
		iq_param_memalloc_addr[id].edge = (IQ_EDGE_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].contrast + ALIGN_CEIL(sizeof(IQ_CONTRAST_PARAM), align_byte));
		iq_param_memalloc_addr[id]._3dnr = (IQ_3DNR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].edge + ALIGN_CEIL(sizeof(IQ_EDGE_PARAM), align_byte));
		iq_param_memalloc_addr[id].wdr = (IQ_WDR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id]._3dnr + ALIGN_CEIL(sizeof(IQ_3DNR_PARAM), align_byte));
		iq_param_memalloc_addr[id].shdr = (IQ_SHDR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].wdr + ALIGN_CEIL(sizeof(IQ_WDR_PARAM), align_byte));
		iq_param_memalloc_addr[id].rgbir = (IQ_RGBIR_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].shdr + ALIGN_CEIL(sizeof(IQ_SHDR_PARAM), align_byte));
		iq_param_memalloc_addr[id].companding = (IQ_COMPANDING_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].rgbir + ALIGN_CEIL(sizeof(IQ_RGBIR_PARAM), align_byte));
		iq_param_memalloc_addr[id].rgbir_enh = (IQ_RGBIR_ENH_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].companding + ALIGN_CEIL(sizeof(IQ_COMPANDING_PARAM), align_byte));
		iq_param_memalloc_addr[id].post_sharpen = (IQ_POST_SHARPEN_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].rgbir_enh + ALIGN_CEIL(sizeof(IQ_RGBIR_ENH_PARAM), align_byte));
		iq_param_memalloc_addr[id].dpc = (IQ_DPC_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].post_sharpen + ALIGN_CEIL(sizeof(IQ_POST_SHARPEN_PARAM), align_byte));
		iq_param_memalloc_addr[id].shading = (IQ_SHADING_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].dpc + ALIGN_CEIL(sizeof(IQ_DPC_PARAM), align_byte));
		iq_param_memalloc_addr[id].shading_inter = (IQ_SHADING_INTER_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].shading + ALIGN_CEIL(sizeof(IQ_SHADING_PARAM), align_byte));
		iq_param_memalloc_addr[id].shading_ext = (IQ_SHADING_EXT_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].shading_inter + ALIGN_CEIL(sizeof(IQ_SHADING_INTER_PARAM), align_byte));
		iq_param_memalloc_addr[id].ldc = (IQ_LDC_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].shading_ext + ALIGN_CEIL(sizeof(IQ_SHADING_EXT_PARAM), align_byte));
		iq_param_memalloc_addr[id].ycurve = (IQ_YCURVE_PARAM *)((UINT8 *)iq_param_memalloc_addr[id].ldc + ALIGN_CEIL(sizeof(IQ_LDC_PARAM), align_byte));
		iq_param_get_param(&temp_addr);
		iq_param_temp = (IQ_PARAM_PTR *)temp_addr;
		memcpy(iq_param_memalloc_addr[id].ob, iq_param_temp->ob, sizeof(IQ_OB_PARAM));
		memcpy(iq_param_memalloc_addr[id].nr, iq_param_temp->nr, sizeof(IQ_NR_PARAM));
		memcpy(iq_param_memalloc_addr[id].cfa, iq_param_temp->cfa, sizeof(IQ_CFA_PARAM));
		memcpy(iq_param_memalloc_addr[id].va, iq_param_temp->va, sizeof(IQ_VA_PARAM));
		memcpy(iq_param_memalloc_addr[id].gamma, iq_param_temp->gamma, sizeof(IQ_GAMMA_PARAM));
		memcpy(iq_param_memalloc_addr[id].ccm, iq_param_temp->ccm, sizeof(IQ_CCM_PARAM));
		memcpy(iq_param_memalloc_addr[id].color, iq_param_temp->color, sizeof(IQ_COLOR_PARAM));
		memcpy(iq_param_memalloc_addr[id].contrast, iq_param_temp->contrast, sizeof(IQ_CONTRAST_PARAM));
		memcpy(iq_param_memalloc_addr[id].edge, iq_param_temp->edge, sizeof(IQ_EDGE_PARAM));
		memcpy(iq_param_memalloc_addr[id]._3dnr, iq_param_temp->_3dnr, sizeof(IQ_3DNR_PARAM));
		memcpy(iq_param_memalloc_addr[id].wdr, iq_param_temp->wdr, sizeof(IQ_WDR_PARAM));
		memcpy(iq_param_memalloc_addr[id].shdr, iq_param_temp->shdr, sizeof(IQ_SHDR_PARAM));
		memcpy(iq_param_memalloc_addr[id].rgbir, iq_param_temp->rgbir, sizeof(IQ_RGBIR_PARAM));
		memcpy(iq_param_memalloc_addr[id].companding, iq_param_temp->companding, sizeof(IQ_COMPANDING_PARAM));
		memcpy(iq_param_memalloc_addr[id].post_sharpen, iq_param_temp->post_sharpen, sizeof(IQ_POST_SHARPEN_PARAM));
		memcpy(iq_param_memalloc_addr[id].rgbir_enh, iq_param_temp->rgbir_enh, sizeof(IQ_RGBIR_ENH_PARAM));
		memcpy(iq_param_memalloc_addr[id].dpc, iq_param_temp->dpc, sizeof(IQ_DPC_PARAM));
		memcpy(iq_param_memalloc_addr[id].shading, iq_param_temp->shading, sizeof(IQ_SHADING_PARAM));
		memcpy(iq_param_memalloc_addr[id].shading_inter, iq_param_temp->shading_inter, sizeof(IQ_SHADING_INTER_PARAM));
		memcpy(iq_param_memalloc_addr[id].shading_ext, iq_param_temp->shading_ext, sizeof(IQ_SHADING_EXT_PARAM));
		memcpy(iq_param_memalloc_addr[id].ldc, iq_param_temp->ldc, sizeof(IQ_LDC_PARAM));
		memcpy(iq_param_memalloc_addr[id].ycurve, iq_param_temp->ycurve, sizeof(IQ_YCURVE_PARAM));
		iq_param[id] = &iq_param_memalloc_addr[id];
		iq_param_memalloc_valid[id] = TRUE;
	}
	return E_OK;
}

void iq_builtin_get_param(UINT32 id, UINT32 *param)
{
	*param = (UINT32)(&iq_param[id]->ob);
}

INT32 iq_builtin_init(UINT32 id)
{
	ER rt = E_OK;

	gamma_buffer[id] = kzalloc(GAMMA_BUFFER_SIZE, GFP_KERNEL);
	if (gamma_buffer[id] == NULL) {
		printk("allocate gamma_buffer fail!\n");
		return -E_SYS;
	}
	ycurve_buffer[id] = kzalloc(YCURVE_BUFFER_SIZE, GFP_KERNEL);
	if (ycurve_buffer[id] == NULL) {
		printk("allocate ycurve_buffer fail!\n");
		return -E_SYS;
	}

	#if LDC_ENABLE
	lut2d_buffer[id] = kzalloc(LUT2D_BUFFER_SIZE, GFP_KERNEL);
	if (lut2d_buffer[id] == NULL) {
		printk("allocate lut2d_buffer fail!\n");
		return -E_SYS;
	}
	#endif

	#if DPC_ENABLE
	dpc_buffer[id] = kzalloc(DPC_BUFFER_SIZE, GFP_KERNEL);
	if (dpc_buffer[id] == NULL) {
		printk("allocate dpc_buffer fail!\n");
		return -E_SYS;
	}
	#endif

	#if ECS_ENABLE
	ecs_buffer[id] = kzalloc(ECS_BUFFER_SIZE, GFP_KERNEL);
	if (ecs_buffer[id] == NULL) {
		printk("allocate ecs_buffer fail!\n");
		return -E_SYS;
	}
	#endif

	rt = iq_builtin_get_param_addr(id);
	if (rt != E_OK) {
		return rt;
	}

	iq_dtsi_load(id, iq_param[id]);

	iq_builtin_front_dtsi_load(id);

	return 0;
}

INT32 iq_builtin_trig(UINT32 id, ISP_BUILTIN_IQ_TRIG_MSG msg)
{
	UINT32 gain = 0, dgain = 0;
	UINT32 ct = 0, lv = 0;
	ISP_BUILTIN_AE_STATUS ae_status = ISP_BUILTIN_AE_STATUS_STABLE;
	ISP_BUILTIN_CGAIN *awb_cgain = NULL;
	UINT32 ui_nr_ratio = 100;
	UINT32 ui_3dnr_ratio = 100;
	UINT32 ui_sharp_ratio = 100;
	UINT32 ui_sat_ratio = 100;
	UINT32 ui_con_ratio = 100;
	UINT32 ui_bright_ratio = 100;
	UINT32 ui_night_mode = FALSE;
	UINT32 ui_shdr_tone_lv = 50;
	UINT32 i, iso_idx;
#if IQ_BUILTIN_REG_MSG
	static UINT32 count;
#endif

	gain = isp_builtin_get_total_gain(id);
	if (gain == 0) {
		gain = 100;
		printk("isp builtin get total gain fail \r\n");
	}
	dgain = isp_builtin_get_dgain(id);
	if (dgain == 0) {
		dgain = 128;
		printk("isp builtin get dgain fail \r\n");
	}
	ct = isp_builtin_get_ct(id);
	if(ct == 0){
		ct = 6000;
		printk("isp builtin get ct fail \r\n");
	}
	lv = isp_builtin_get_lv(id);
	if(lv == 0){
		lv = 8 * LV_BASE;
		printk("isp builtin get lv fail \r\n");
	}
	ae_status = isp_builtin_get_ae_status(id);
	awb_cgain = isp_builtin_get_cgain(id);
	if ((awb_cgain->r == 0) | (awb_cgain->g == 0) | (awb_cgain->b == 0)) {
		awb_cgain->r = 256;
		awb_cgain->g = 256;
		awb_cgain->b = 256;
		printk("isp builtin get cgain fail \r\n");
	}

	ui_nr_ratio = isp_builtin_get_nr_lv(id);
	ui_3dnr_ratio = isp_builtin_get_3dnr_lv(id);
	ui_sharp_ratio = isp_builtin_get_sharpness_lv(id);
	ui_sat_ratio = isp_builtin_get_saturation_lv(id);
	ui_con_ratio = isp_builtin_get_contrast_lv(id);
	ui_bright_ratio = isp_builtin_get_brightness_lv(id);
	ui_night_mode = isp_builtin_get_night_mode(id);

	if (iq_frm_cnt[id] >= iq_front_dtsi.smooth_thr) {
		iq_frm_cnt[id] = iq_front_dtsi.smooth_thr;
	}

	iq_front_factor.rth_nlm = iq_intpl(iq_frm_cnt[id], (INT32)iq_front_dtsi.rth_nlm, (INT32)100, 0, iq_front_dtsi.smooth_thr);
	iq_front_factor.rth_nlm_lut = iq_intpl(iq_frm_cnt[id], (INT32)iq_front_dtsi.rth_nlm_lut, (INT32)100, 0, iq_front_dtsi.smooth_thr);
	iq_front_factor.clamp_th = iq_intpl(iq_frm_cnt[id], (INT32)iq_front_dtsi.clamp_th, (INT32)100, 0, iq_front_dtsi.smooth_thr);
	iq_front_factor.c_con = iq_intpl(iq_frm_cnt[id], (INT32)iq_front_dtsi.c_con, (INT32)100, 0, iq_front_dtsi.smooth_thr);
	iq_front_factor.overshoot = iq_intpl(iq_frm_cnt[id], (INT32)iq_front_dtsi.overshoot, (INT32)100, 0, iq_front_dtsi.smooth_thr);

	for (i = 1; i < IQ_GAIN_ID_MAX_NUM; i++) {
		if (gain < iso_map_tab[i].iso) {
			if (gain < (iso_map_tab[i].iso + iso_map_tab[i - 1].iso) / 2) {
				iso_idx = iso_map_tab[i - 1].index;
			} else {
				iso_idx = iso_map_tab[i].index;
			}
			break;
		}
	}

	iq_front_factor.rth_nlm = ((iq_front_factor.rth_nlm * iq_front_dtsi.iso_w[iso_idx]) + (100 * (100 - iq_front_dtsi.iso_w[iso_idx]))) / 100;
	iq_front_factor.rth_nlm_lut = ((iq_front_factor.rth_nlm_lut * iq_front_dtsi.iso_w[iso_idx]) + (100 * (100 - iq_front_dtsi.iso_w[iso_idx]))) / 100;
	iq_front_factor.clamp_th = ((iq_front_factor.clamp_th * iq_front_dtsi.iso_w[iso_idx]) + (100 * (100 - iq_front_dtsi.iso_w[iso_idx]))) / 100;
	iq_front_factor.c_con = ((iq_front_factor.c_con * iq_front_dtsi.iso_w[iso_idx]) + (100 * (100 - iq_front_dtsi.iso_w[iso_idx]))) / 100;
	iq_front_factor.overshoot = ((iq_front_factor.overshoot * iq_front_dtsi.iso_w[iso_idx]) + (100 * (100 - iq_front_dtsi.iso_w[iso_idx]))) / 100;

	#if IQ_BUILTIN_DBG_MSG
	if(iq_frm_cnt[id] < iq_front_dtsi.smooth_thr) {
		printk("id = %d, gain = %d, cnt = %d, iso_idx = %d, iso_w = %d, nlm = %3d, lut = %3d, clamp = %3d, c_con = %3d, overshoot = %d\n", id, gain, iq_frm_cnt[id], iso_idx,
																									iq_front_dtsi.iso_w[iso_idx],
																									iq_front_factor.rth_nlm,
																									iq_front_factor.rth_nlm_lut,
																									iq_front_factor.clamp_th,
																									iq_front_factor.c_con,
																									iq_front_factor.overshoot
																									);
	}
	#endif

	if (msg == ISP_BUILTIN_IQ_TRIG_SIE_RESET) {
		iq_builtin_set_sie_ob(id, gain, iq_param[id]);
		#if DPC_ENABLE
		iq_builtin_set_dpc(id, iq_param[id]);
		#endif
		#if ECS_ENABLE
		iq_builtin_set_ecs(id, ct, iq_param[id]);
		#endif
		iq_builtin_set_dgain(id, dgain);
		iq_builtin_set_companding(iq_param[id]);
	}

	if (msg == ISP_BUILTIN_IQ_TRIG_IPP_RESET) {
		iq_builtin_set_ife_ob(id, gain, iq_param[id]);
		iq_builtin_set_cgain(id, awb_cgain, iq_param[id], gain, ui_night_mode);
		iq_builtin_set_shdr(gain, iq_param[id]);
		iq_builtin_set_nr(id, msg, gain, awb_cgain, iq_param[id], ui_nr_ratio);
		iq_builtin_set_vig(gain, iq_param[id]);
		iq_builtin_set_tonecurve(id, iq_param[id], ui_shdr_tone_lv);
		iq_builtin_set_wdr(id, msg, gain, iq_param[id]);
		iq_builtin_set_cfa(gain, iq_param[id]);
		#if LDC_ENABLE
		iq_builtin_set_ldc(id, iq_param[id]);
		#endif
		iq_builtin_set_ccm(id, ct, iq_param[id], ui_night_mode);
		iq_builtin_set_gamma(id, lv, iq_param[id]);
		iq_builtin_set_ycurve(id, iq_param[id]);
		iq_builtin_set_color(gain, iq_param[id], ui_night_mode, ui_con_ratio, ui_sat_ratio);
		iq_builtin_set_contrast(msg, gain, iq_param[id], ui_bright_ratio, ui_con_ratio);
		iq_builtin_set_edge(gain, iq_param[id], ui_sharp_ratio, ae_status);
		iq_builtin_set_3dnr(id, msg, gain, iq_param[id], ui_night_mode, ui_3dnr_ratio);
	}

	if (msg == ISP_BUILTIN_IQ_TRIG_CGAIN) {
		iq_builtin_set_cgain(id, awb_cgain, iq_param[id], gain, ui_night_mode);
	}

	if (msg == ISP_BUILTIN_IQ_TRIG_SIE) {
		iq_builtin_set_sie_ob(id, gain, iq_param[id]);
		#if DPC_ENABLE
		iq_builtin_set_dpc(id, iq_param[id]);
		#endif
		#if ECS_ENABLE
		iq_builtin_set_ecs(id, ct, iq_param[id]);
		#endif
		iq_builtin_set_dgain(id, dgain);
		iq_builtin_set_companding(iq_param[id]);
	}

	if (msg == ISP_BUILTIN_IQ_TRIG_IPP) {
		iq_builtin_set_ife_ob(id, gain, iq_param[id]);

		iq_builtin_set_shdr(gain, iq_param[id]);
		iq_builtin_set_nr(id, msg, gain, awb_cgain, iq_param[id], ui_nr_ratio);
		iq_builtin_set_vig(gain, iq_param[id]);
		iq_builtin_set_tonecurve(id, iq_param[id], ui_shdr_tone_lv);
		iq_builtin_set_wdr(id, msg, gain, iq_param[id]);
		iq_builtin_set_cfa(gain, iq_param[id]);
		#if LDC_ENABLE
		iq_builtin_set_ldc(id, iq_param[id]);
		#endif
		iq_builtin_set_ccm(id, ct, iq_param[id], ui_night_mode);
		iq_builtin_set_gamma(id, lv, iq_param[id]);
		iq_builtin_set_ycurve(id, iq_param[id]);
		iq_builtin_set_color(gain, iq_param[id], ui_night_mode, ui_con_ratio, ui_sat_ratio);
		iq_builtin_set_contrast(msg, gain, iq_param[id], ui_bright_ratio, ui_con_ratio);
		iq_builtin_set_edge(gain, iq_param[id], ui_sharp_ratio, ae_status);
		iq_builtin_set_3dnr(id, msg, gain, iq_param[id], ui_night_mode, ui_3dnr_ratio);
		iq_frm_cnt[id]++;
	}

	if (msg == ISP_BUILTIN_IQ_TRIG_ENC) {
		iq_builtin_set_post_sharpen(id, gain, iq_param[id]);
	}

	#if IQ_BUILTIN_REG_MSG
	count ++;
	if (count == 70) {
		printk("================builtin================");
		printk("SIE :");
		debug_dumpmem(0xf0c00000, 0x8b0);
		printk("IFE :");
		debug_dumpmem(0xf0c70000, 0x760);
		printk("DCE :");
		debug_dumpmem(0xf0c20000, 0x630);
		printk("IPE :");
		debug_dumpmem(0xf0c30000, 0x8c0);
		printk("IFE2 :");
		debug_dumpmem(0xf0d00000, 0xb0);
		printk("IME :");
		debug_dumpmem(0xf0c40000, 0xa40);
		printk("ENC :");
		debug_dumpmem(0xf0a109b8, 0x30);
	}
	#endif

	return 0;
}

#endif
#endif

