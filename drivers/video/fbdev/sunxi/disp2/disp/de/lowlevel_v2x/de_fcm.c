/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *
 *  File name   :       de_fcm.c
 *
 *  Description :       display engine 2.0 fcm base functions implement
 *
 *  History     :       2021/04/07  v0.1  Initial version
 *
 ******************************************************************************/

#include "de_fcm_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"
#include "de_fcm.h"


#define FCM_MODE_MASK 0xff
#define FCM_ENHANCE_MASK 0xff00

#define HUE_BIN_LEN	28
#define SAT_BIN_LEN	13
#define LUM_BIN_LEN	13
#define SAT_MASK	0xff
#define LUM_MASK	0xff
#define HBH_HUE_MASK	0x1fff
#define SBH_HUE_MASK	0x7ff
#define VBH_HUE_MASK	0x1ff
#define FCM_MODE_CNT	10

static volatile struct __fcm_reg_t *fcm_dev[DE_NUM][CHN_NUM];
static struct de_reg_blocks fcm_block[DE_NUM][CHN_NUM];
static struct de_reg_blocks fcm_basic_block[DE_NUM][CHN_NUM];
static fcm_hardware_data_t *fcm_mode[DE_NUM];
static int fcm_lut_init[FCM_MODE_CNT];
static int initialized[DE_NUM];/*used to enable fcm after lut is ready*/
static u32 lut_using[DE_NUM];
static u32 enhance[DE_NUM];

static unsigned int angle_hue_lut[] = {
	0, 91, 182, 273, 364, 455, 569,
	683, 796, 933, 1115, 1297, 1479, 1661,
	1843, 2025, 2207, 2389, 2571, 2776, 2980,
	3185, 3367, 3526, 3663, 3799, 3913, 4004,
};
static unsigned int angle_sat_lut[] = {
	0, 123, 205, 286, 368, 450, 532,
	614, 696, 777, 859, 941, 1023,
};
static unsigned int angle_lum_lut[] = {
	0, 31, 51, 71, 92, 112, 133,
	153, 173, 194, 214, 235, 255,
};
static unsigned int enhance_hbh[] = {
	0, 0, 0, 0, 0, 0, 0,
	0, 25, 40, 60, 30, 0, 0,
	0, 27, 40, 50, 60, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};
static unsigned int enhance_sbh[] = {
	0, 11, 20, 18, 11, 0, 0,
	0, 0, 20, 29, 44, 27, 25,
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};
static unsigned int enhance_ybh[] = {
	0, 40, 70, 60, 40, 30, 0,
	0, 0, 0, 0, 0, 0, 60,
	80, 70, 60, 40, 30, 20, 18,
	0, 0, 0, 0, 0, 0, 0
};

static int de_fcm_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	fcm_dev[sel][chno] = (struct __fcm_reg_t *) base;
	return 0;
}

static int de_fcm_set_fix_info(unsigned int sel, unsigned int chno)
{
	memcpy((void *)&fcm_dev[sel][chno]->fcm_angle_hue[0], angle_hue_lut, sizeof(angle_hue_lut));
	memcpy((void *)&fcm_dev[sel][chno]->fcm_angle_sat[0], angle_sat_lut, sizeof(angle_sat_lut));
	memcpy((void *)&fcm_dev[sel][chno]->fcm_angle_lum[0], angle_lum_lut, sizeof(angle_lum_lut));
	return 0;
}

static struct fcm_hardware_data *de_fcm_find_lut(u32 sel, u32 id)
{
	int i;
	struct fcm_hardware_data *ptr = fcm_mode[sel];

	for (i = 0; i < FCM_MODE_CNT; i++) {
		if (ptr->lut_id == id && fcm_lut_init[i])
			return ptr;
		ptr++;
	}
	return NULL;
}

static struct fcm_hardware_data *de_fcm_lut_get_or_add(u32 sel, u32 id)
{
	int i;
	struct fcm_hardware_data *ptr;

	ptr = de_fcm_find_lut(sel, id);
	if (ptr)
		return ptr;

	ptr = fcm_mode[sel];
	for (i = 0; i < FCM_MODE_CNT; i++) {
		if (!fcm_lut_init[i]) {
			fcm_lut_init[i] = 1;
			return ptr + i;
		}
	}
	__wrn("fcm lut exceed, max count is %d\n", FCM_MODE_CNT);
	return NULL;
}

static void de_fcm_enhance_extend(struct fcm_hardware_data *data, unsigned int enhance)
{
	int i;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		data->hbh_hue[i] += enhance_hbh[i] * enhance / 100;
		data->sbh_hue[i] += enhance_sbh[i] * enhance / 100;
		data->ybh_hue[i] += enhance_ybh[i] * enhance / 100;
	}
}

static void de_fcm_set_hue_lut(unsigned int sel, unsigned int chno, unsigned int *h, unsigned int *s, unsigned int *v)
{
	int i;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		fcm_dev[sel][chno]->fcm_hbh_hue[i].bits.hue_low = h[i] & HBH_HUE_MASK;
		fcm_dev[sel][chno]->fcm_sbh_hue[i].bits.hue_low = s[i] & SBH_HUE_MASK;
		fcm_dev[sel][chno]->fcm_vbh_hue[i].bits.hue_low = v[i] & VBH_HUE_MASK;
		if (i == HUE_BIN_LEN - 1) {
			fcm_dev[sel][chno]->fcm_hbh_hue[i].bits.hue_high = h[0] & HBH_HUE_MASK;
			fcm_dev[sel][chno]->fcm_sbh_hue[i].bits.hue_high = s[0] & SBH_HUE_MASK;
			fcm_dev[sel][chno]->fcm_vbh_hue[i].bits.hue_high = v[0] & VBH_HUE_MASK;
		} else {
			fcm_dev[sel][chno]->fcm_hbh_hue[i].bits.hue_high = h[i + 1] & HBH_HUE_MASK;
			fcm_dev[sel][chno]->fcm_sbh_hue[i].bits.hue_high = s[i + 1] & SBH_HUE_MASK;
			fcm_dev[sel][chno]->fcm_vbh_hue[i].bits.hue_high = v[i + 1] & VBH_HUE_MASK;
		}
	}
}

static void de_fcm_set_sat_lut(unsigned int sel, unsigned int chno, unsigned int *h, unsigned int *s, unsigned int *y)
{
	int i, j, m, n;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		for (j = 0; j < SAT_BIN_LEN; j++) {
			n = j;
			m = i;

			fcm_dev[sel][chno]->fcm_hbh_sat[i * SAT_BIN_LEN + j].bits.lut_0 = h[i * SAT_BIN_LEN + j] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_sbh_sat[i * SAT_BIN_LEN + j].bits.lut_0 = s[i * SAT_BIN_LEN + j] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_vbh_sat[i * SAT_BIN_LEN + j].bits.lut_0 = y[i * SAT_BIN_LEN + j] & SAT_MASK;

			if (j == SAT_BIN_LEN - 1)
				n = j - 1;
			if (i == HUE_BIN_LEN - 1)
				m = -1;

			fcm_dev[sel][chno]->fcm_hbh_sat[i * SAT_BIN_LEN + j].bits.lut_1 = h[i * SAT_BIN_LEN + n + 1] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_sbh_sat[i * SAT_BIN_LEN + j].bits.lut_1 = s[i * SAT_BIN_LEN + n + 1] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_vbh_sat[i * SAT_BIN_LEN + j].bits.lut_1 = y[i * SAT_BIN_LEN + n + 1] & SAT_MASK;

			fcm_dev[sel][chno]->fcm_hbh_sat[i * SAT_BIN_LEN + j].bits.lut_2 = h[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_sbh_sat[i * SAT_BIN_LEN + j].bits.lut_2 = s[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_vbh_sat[i * SAT_BIN_LEN + j].bits.lut_2 = y[(m + 1) * SAT_BIN_LEN + j] & SAT_MASK;

			fcm_dev[sel][chno]->fcm_hbh_sat[i * SAT_BIN_LEN + j].bits.lut_3 = h[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_sbh_sat[i * SAT_BIN_LEN + j].bits.lut_3 = s[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
			fcm_dev[sel][chno]->fcm_vbh_sat[i * SAT_BIN_LEN + j].bits.lut_3 = y[(m + 1) * SAT_BIN_LEN + n + 1] & SAT_MASK;
		}
	}
}

static void de_fcm_set_lum_lut(unsigned int sel, unsigned int chno, unsigned int *h, unsigned int *s, unsigned int *y)
{
	int i, j, m, n;
	for (i = 0; i < HUE_BIN_LEN; i++) {
		for (j = 0; j < LUM_BIN_LEN; j++) {
			n = j;
			m = i;

			fcm_dev[sel][chno]->fcm_hbh_lum[i * LUM_BIN_LEN + j].bits.lut_0 = h[i * LUM_BIN_LEN + j] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_sbh_lum[i * LUM_BIN_LEN + j].bits.lut_0 = s[i * LUM_BIN_LEN + j] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_vbh_lum[i * LUM_BIN_LEN + j].bits.lut_0 = y[i * LUM_BIN_LEN + j] & LUM_MASK;

			if (j == LUM_BIN_LEN - 1)
				n = j - 1;
			if (i == HUE_BIN_LEN - 1)
				m = -1;

			fcm_dev[sel][chno]->fcm_hbh_lum[i * LUM_BIN_LEN + j].bits.lut_1 = h[i * LUM_BIN_LEN + n + 1] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_sbh_lum[i * LUM_BIN_LEN + j].bits.lut_1 = s[i * LUM_BIN_LEN + n + 1] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_vbh_lum[i * LUM_BIN_LEN + j].bits.lut_1 = y[i * LUM_BIN_LEN + n + 1] & LUM_MASK;

			fcm_dev[sel][chno]->fcm_hbh_lum[i * LUM_BIN_LEN + j].bits.lut_2 = h[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_sbh_lum[i * LUM_BIN_LEN + j].bits.lut_2 = s[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_vbh_lum[i * LUM_BIN_LEN + j].bits.lut_2 = y[(m + 1) * LUM_BIN_LEN + j] & LUM_MASK;

			fcm_dev[sel][chno]->fcm_hbh_lum[i * LUM_BIN_LEN + j].bits.lut_3 = h[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_sbh_lum[i * LUM_BIN_LEN + j].bits.lut_3 = s[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
			fcm_dev[sel][chno]->fcm_vbh_lum[i * LUM_BIN_LEN + j].bits.lut_3 = y[(m + 1) * LUM_BIN_LEN + n + 1] & LUM_MASK;
		}
	}
}

int de_fcm_init(unsigned int sel, unsigned int chno, uintptr_t reg_base)
{
	uintptr_t fcm_base;
	void *memory;
	/*FIXME cal fcm_base using chno*/
	fcm_base = reg_base + (sel + 1) * 0x00100000 + FCM_OFST;

	memory = kmalloc(sizeof(struct __fcm_reg_t) +  FCM_MODE_CNT * sizeof(struct fcm_hardware_data), GFP_KERNEL | __GFP_ZERO);
	if (memory == NULL) {
		__wrn("malloc vep fcm[%d][%d] memory fail! size=0x%x\n", sel,
		      chno, (unsigned int)sizeof(struct __fcm_reg_t));
		return -1;
	}

	/*we used this block to update all fcm reg except lut*/
	fcm_basic_block[sel][chno].off = fcm_base;
	fcm_basic_block[sel][chno].val = memory;
	fcm_basic_block[sel][chno].size = 0x14;
	fcm_basic_block[sel][chno].dirty = 0;

	fcm_block[sel][chno].off = fcm_base;
	fcm_block[sel][chno].val = memory;
	fcm_block[sel][chno].size = 0;/*not used*/
	fcm_block[sel][chno].dirty = 0;

	de_fcm_set_reg_base(sel, chno, memory);
	de_fcm_set_fix_info(sel, chno);

	fcm_mode[sel] = (struct fcm_hardware_data *)(((u8 *)memory) + sizeof(struct __fcm_reg_t));
	return 0;
}

int de_fcm_exit(unsigned int sel, unsigned int chno)
{
	kfree(fcm_basic_block[sel][chno].val);
	return 0;
}

int de_fcm_update_regs(unsigned int sel, unsigned int chno)
{
	u8 *paddr = (u8 *)(fcm_block[sel][chno].off);
	u8 *vaddr = (u8 *)(fcm_block[sel][chno].val);

	if (fcm_block[sel][chno].dirty == 0x1) {
		memcpy(paddr, vaddr, 4);
		memcpy(paddr + 0x8, vaddr + 0x8, 4 * 3);

		memcpy(paddr + 0x20, vaddr + 0x20, 4 * 28);
		memcpy(paddr + 0xa0, vaddr + 0xa0, 4 * 13);
		memcpy(paddr + 0xe0, vaddr + 0xe0, 4 * 13);

		memcpy(paddr + 0x120, vaddr + 0x120, 4 * 28);
		memcpy(paddr + 0x1a0, vaddr + 0x1a0, 4 * 28);
		memcpy(paddr + 0x220, vaddr + 0x220, 4 * 28);

		memcpy(paddr + 0x300, vaddr + 0x300, 4 * 364);
		memcpy(paddr + 0x900, vaddr + 0x900, 4 * 364);
		memcpy(paddr + 0xf00, vaddr + 0xf00, 4 * 364);

		memcpy(paddr + 0x1500, vaddr + 0x1500, 4 * 364);
		memcpy(paddr + 0x1b00, vaddr + 0x1b00, 4 * 364);
		memcpy(paddr + 0x2100, vaddr + 0x2100, 4 * 364);
		fcm_block[sel][chno].dirty = 0x0;
	} else if (fcm_basic_block[sel][chno].dirty == 0x1 && initialized[sel]) {
		memcpy((void *)fcm_basic_block[sel][chno].off,
			fcm_basic_block[sel][chno].val,
			fcm_basic_block[sel][chno].size);
		fcm_basic_block[sel][chno].dirty = 0x0;
	}
	return 0;
}

int de_fcm_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	fcm_dev[sel][chno]->fcm_ctl.bits.en = en;
	fcm_basic_block[sel][chno].dirty = 1;
	return 0;
}

int de_fcm_set_size(unsigned int sel, unsigned int chno, unsigned int width,
		    unsigned int height)
{
	fcm_dev[sel][chno]->fcm_size.bits.width = width == 0 ? 0 : width - 1;
	fcm_dev[sel][chno]->fcm_size.bits.height = height == 0 ? 0 : height - 1;
	fcm_basic_block[sel][chno].dirty = 1;
	return 0;
}

int de_fcm_set_window(unsigned int sel, unsigned int chno, unsigned int win_en,
		      struct de_rect window)
{
	fcm_dev[sel][chno]->fcm_ctl.bits.win_en = win_en & 0x1;

	if (win_en) {
		fcm_dev[sel][chno]->fcm_win0.bits.left = window.x;
		fcm_dev[sel][chno]->fcm_win0.bits.top = window.y;
		fcm_dev[sel][chno]->fcm_win1.bits.right =
		    window.x + window.w - 1;
		fcm_dev[sel][chno]->fcm_win1.bits.bot = window.y + window.h - 1;
	}
	fcm_basic_block[sel][chno].dirty = 1;
	return 0;
}

int de_fcm_get_lut(unsigned int sel, fcm_hardware_data_t *data)
{
	struct fcm_hardware_data *lut;
	lut = de_fcm_find_lut(sel, data->lut_id);
	if (lut == NULL) {
		__wrn("intput param invalid id = %d\n", data->lut_id);
		return -1;
	}
	memcpy(data, lut, sizeof(*lut));
	return 0;
}
/*
void print_hardware(void)
{
	u32 *tmp = (u32 *)(fcm_dev[0][0]);
	u32 i = 0;
	while(i < 2475) {
		printk("%p ,0x%8x: 0x%8x 0x%8x 0x%8x 0x%8x\n",&(tmp[i]),  0x051aa000 + 4 * i, tmp[i],tmp[i+1],tmp[i+2],tmp[i+3]);
		i+=4;
	}
}

 void print_data(struct fcm_hardware_data *lut)
 {
	int i = 0;
	printk("hbh_hue: \n");
	for(i = 1; i < 28 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->hbh_hue[i-1]);
		if (i % 7 == 0)
			printk("\n");
	}

	printk("sbh_hue: \n");
	for(i = 1; i < 28 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->sbh_hue[i-1]);
		if (i % 7 == 0)
			printk("\n");
	}

	printk("ybh_hue: \n");
	for(i = 1; i < 28 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->ybh_hue[i-1]);
		if (i % 7 == 0)
			printk("\n");
	}

	printk("\n\n");
	printk("angle_hue: \n");
	for(i = 1; i < 28 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->angle_hue[i-1]);
		if (i % 7 == 0)
			printk("\n");
	}

	printk("angle_sat: \n");
	for(i = 1; i < 13 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->angle_sat[i-1]);
	}
	printk("\n");

	printk("angle_lum: \n");
	for(i = 1; i < 13 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->angle_lum[i-1]);
	}
	printk("\n\n");
	printk("hbh_sat: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->hbh_sat[i-1]);
	if (i % 13 == 0)
		printk("\n");
}

	printk("sbh_sat: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->sbh_sat[i-1]);
		if (i % 13 == 0)
			printk("\n");
}

	printk("ybh_sat: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->ybh_sat[i-1]);
		if (i % 13 == 0)
			printk("\n");
}

	printk("\n\n");
	printk("hbh_lum: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->hbh_lum[i-1]);
		if (i % 13 == 0)
			printk("\n");
}

	printk("sbh_lum: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->sbh_lum[i-1]);
		if (i % 13 == 0)
			printk("\n");
	}

	printk("ybh_lum: \n");
	for(i = 1; i < 364 + 1; i++) {
		printk("[%d]=%d ", i-1, lut->ybh_lum[i-1]);
		if (i % 13 == 0)
			printk("\n");
	}

}
*/
static int de_fcm_lut_apply(unsigned int sel, struct fcm_hardware_data *lut)
{
	int ret = -1;
	int j = 0;
	struct fcm_hardware_data *lut_set;
	lut_set = kmalloc(sizeof(*lut_set), GFP_KERNEL | __GFP_ZERO);
	memcpy(lut_set, lut, sizeof(*lut));
	de_fcm_enhance_extend(lut_set, enhance[sel]);
	while (j < CHN_NUM) {
		if (de_feat_is_support_fcm_by_chn(sel, j)) {
			de_fcm_set_hue_lut(sel, j, lut_set->hbh_hue, lut_set->sbh_hue, lut_set->ybh_hue);
			de_fcm_set_sat_lut(sel, j, lut_set->hbh_sat, lut_set->sbh_sat, lut_set->ybh_sat);
			de_fcm_set_lum_lut(sel, j, lut_set->hbh_lum, lut_set->sbh_lum, lut_set->ybh_lum);
			fcm_block[sel][j].dirty = 1;
			ret = 0;
		}
		j++;
	}
	if (ret)
		__wrn("display %d not support fcm\n", sel);
	kfree(lut_set);
	return ret;
}

int de_fcm_set_lut(unsigned int sel, fcm_hardware_data_t *data, unsigned int update)
{
	int ret = 0;
	struct fcm_hardware_data *lut;
	int init = !initialized[sel] && lut_using[sel] == data->lut_id;
	lut = de_fcm_lut_get_or_add(sel, data->lut_id);
	if (lut == NULL) {
		__wrn("intput param invalid id = %d\n", data->lut_id);
		return -1;
	}
	memcpy(lut, data, sizeof(*lut));
	if (update || init) {
		ret = de_fcm_lut_apply(sel, lut);
		if (init)
			initialized[sel] = 1;
	}
	return ret;
}

//TODO de_fcm_set_para and de_fcm_set_lut should be access fcm_mode using mutex
int de_fcm_set_para(unsigned int sel, unsigned int enh, unsigned mode)
{
	struct fcm_hardware_data *lut;
	if (mode > FCM_MODE_CNT || enh > 100) {
		__wrn("display %d invalide fcm para\n", sel);
		return -1;
	}
	enhance[sel] = enh;
	lut_using[sel] = mode;
	lut = de_fcm_find_lut(sel, mode);
	if (lut == NULL) {
		__wrn("fcm lut %d not find, auto retry after init\n", mode);
		return 0;
	} else {
		initialized[sel] = 1;
	}
	return de_fcm_lut_apply(sel, lut);
}

