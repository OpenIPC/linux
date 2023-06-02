#if defined(__KERNEL__)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>

#include "awb_builtin.h"

#if (NVT_FAST_ISP_FLOW)
//=============================================================================
// define
//=============================================================================
#define MIN(a, b)       ((a) <= (b) ? (a) : (b))
#define MAX(a, b)       ((a) >= (b) ? (a) : (b))
#define CLAMP(x, a, b)  MAX(MIN(x, b), a)

#define SUB_NODE_LENGTH 64

//=============================================================================
// global
//=============================================================================
static AWB_BUILTIN_INFO awb_info[ISP_BUILTIN_ID_MAX_NUM] = {0};
static AWB_BUILTIN_CT_INFO awb_builtin_ct_info[ISP_BUILTIN_ID_MAX_NUM] = {0};
static AWB_BUILTIN_TH awb_th[ISP_BUILTIN_ID_MAX_NUM] = {0};

static AWB_BUILTIN_CT_INFO awb_builtin_ct_info_imx290 = {
	{2300, 2800, 3700, 4700, 6500, 11000},
	{249, 285, 340, 398, 480, 586},
	{256, 256, 256, 256, 256, 256},
	{839, 806, 654, 520, 377, 316}
};

static AWB_BUILTIN_CT_INFO awb_builtin_ct_info_f37 = {
	{2300, 2800, 3700, 4700, 6500, 11000},
	{216, 253, 383, 472, 443, 654},
	{256, 256, 256, 256, 256, 256},
	{574, 553, 516, 442, 371, 280}
};

static AWB_BUILTIN_CT_INFO awb_builtin_ct_info_os02k10 = {
	{2300, 2800, 3700, 4700, 6500, 11000},
	{278, 315, 386, 470, 573, 730},
	{256, 256, 256, 256, 256, 256},
	{1052, 890, 708, 585, 421, 360}
};

static AWB_BUILTIN_CT_INFO awb_builtin_ct_info_os05a10 = {
	{2300, 2800, 3700, 4700, 6500, 11000},
	{296, 346, 421, 505, 609, 748},
	{256, 256, 256, 256, 256, 256},
	{870, 747, 639, 525, 392, 334}
};

static AWB_BUILTIN_CT_INFO awb_builtin_ct_info_gc4653 = {
	{2300, 2800, 3700, 4700, 6500, 11000},
	{249, 285, 340, 398, 480, 586},
	{256, 256, 256, 256, 256, 256},
	{839, 806, 654, 520, 377, 316}
};

static AWB_BUILTIN_TH awb_th_imx290 = {5, 245, 49, 165, 20, 90};

static AWB_BUILTIN_TH awb_th_f37 = {5, 230, 44, 186, 47, 119};

static AWB_BUILTIN_TH awb_th_os02k10 = {5, 240, 38, 150, 19, 90};

static AWB_BUILTIN_TH awb_th_os05a10 = {5, 235, 37, 141, 27, 96};

static AWB_BUILTIN_TH awb_th_gc4653 = {5, 245, 49, 165, 20, 90};


AWB_BUILTIN_FRONT_DTSI awb_front_dtsi = {
	DISABLE,
	{256, 256, 256}
};

//=============================================================================
// function declaration
//=============================================================================
void awb_builtin_get_cur_ct(UINT32 id, ISP_BUILTIN_CGAIN cur_gain, UINT32 *ct);

//=============================================================================
// external functions
//=============================================================================
void awb_builtin_front_dtsi_load(void)
{
	CHAR node_path[SUB_NODE_LENGTH];
	CHAR sub_node_name[SUB_NODE_LENGTH];
	struct device_node* of_node;

	sprintf(node_path, "/fastboot/front_awb_0");

	sprintf(sub_node_name, "%s/%s", node_path, "enable");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&awb_front_dtsi.enable, sizeof(awb_front_dtsi.enable)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "color_gain");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&awb_front_dtsi.color_gain, sizeof(awb_front_dtsi.color_gain)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

#if 0
	printk("enable ========== %d\r\n", awb_front_dtsi.enable);
	printk("color_gain ====== %d, %d, %d\r\n", awb_front_dtsi.color_gain.r, awb_front_dtsi.color_gain.g, awb_front_dtsi.color_gain.b);
#endif
}

INT32 awb_builtin_init(UINT32 id)
{
	awb_info[id].awb_app_mode = 0;

	if (isp_builtin_get_shdr_enable(id) == 0) {
		awb_info[id].awb_app_mode = 0;
	} else if(isp_builtin_get_shdr_enable(id) == 1) {
		awb_info[id].awb_app_mode = 1;
	}

	if (isp_builtin_get_sensor_name(id) == 0) {
		awb_info[id].awb_gain.r = 398;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 520;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_imx290;
		awb_th[id] = awb_th_imx290;
	} else if (isp_builtin_get_sensor_name(id) == 1) {
		awb_info[id].awb_gain.r = 472;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 442;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_f37;
		awb_th[id] = awb_th_f37;
	} else if (isp_builtin_get_sensor_name(id) == 2) {
		awb_info[id].awb_gain.r = 470;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 585;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_os02k10;
		awb_th[id] = awb_th_os02k10;
	} else if (isp_builtin_get_sensor_name(id) == 3) {
		awb_info[id].awb_gain.r = 505;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 525;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_os05a10;
		awb_th[id] = awb_th_os05a10;
	} else if (isp_builtin_get_sensor_name(id) == 4) {
		awb_info[id].awb_gain.r = 472;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 442;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_f37;
		awb_th[id] = awb_th_f37;
	} else if (isp_builtin_get_sensor_name(id) == 5) {
		awb_info[id].awb_gain.r = 472;
		awb_info[id].awb_gain.g = 256;
		awb_info[id].awb_gain.b = 442;
		awb_builtin_ct_info[id] = awb_builtin_ct_info_gc4653;
		awb_th[id] = awb_th_gc4653;
	}

	awb_builtin_front_dtsi_load();

	if(awb_front_dtsi.enable == ENABLE) {
		awb_info[id].awb_gain.r = awb_front_dtsi.color_gain.r;
		awb_info[id].awb_gain.g = awb_front_dtsi.color_gain.g;
		awb_info[id].awb_gain.b = awb_front_dtsi.color_gain.b;
	}

#if 0
	printk("awb_gain ====== %d, %d, %d\r\n", awb_info.awb_gain.r, awb_info.awb_gain.g, awb_info.awb_gain.b);
#endif

	awb_info[id].cur_ct = 4700;
	awb_info[id].status = AWBALG_STATUS_INIT;

	isp_builtin_set_cgain(id, &awb_info[id].awb_gain);
	isp_builtin_set_ct(id, awb_info[id].cur_ct);
	return 0;
}

INT32 awb_builtin_trig(UINT32 id, ISP_BUILTIN_AWB_TRIG_MSG msg)
{
	ISP_BUILTIN_CA_RSLT *ca_rslt = NULL;
	UINT16 i;
	UINT16 r = 0, g = 0, b = 0;
	UINT16 y = 0, r2g = 0, b2g = 0, g2r = 0, g2b = 0;
	UINT32 gray_g2r_sum = 0, gray_g2b_sum = 0;
	UINT16 gray_w_cnt = 0;

	if (msg == ISP_BUILTIN_AWB_TRIG_RESET) {
		return 0;
	}

	if (awb_info[id].awb_app_mode == 0) {
		ca_rslt = isp_builtin_get_ca(id);
	} else if (awb_info[id].awb_app_mode == 1) {
		ca_rslt = isp_builtin_get_ca(1);
	} else {
		printk("awb_app_mode = %d is not support\r\n", awb_info[id].awb_app_mode);
	}

	if(ca_rslt == NULL) {
		printk("isp_builtin_get_ca is NULL\r\n");
	}

	for (i = 0; i < 1024; i++) {
		r = ca_rslt->r[i];
		g = ca_rslt->g[i];
		b = ca_rslt->b[i];

		if (r == 0 || g == 0 || b == 0) {
			continue;
		}

		y = (r + b + g + g) >> 2;
		r2g = (r << 7) / g;
		b2g = (b << 7) / g;

		if ((y > (awb_th[id].y_l << 4)) && (y < (awb_th[id].y_u << 4)) && (r2g > (awb_th[id].r2g_l)) &&
			(r2g < (awb_th[id].r2g_u)) && (b2g > (awb_th[id].b2g_l)) && (b2g < (awb_th[id].b2g_u))) {

			g2r = (g << 8) / r;
			g2b = (g << 8) / b;

			// Gray world process
			gray_g2r_sum += g2r;
			gray_g2b_sum += g2b;
			gray_w_cnt++;
		}
	}

	if(gray_w_cnt == 0) {
		awb_info[id].status = AWBALG_STATUS_FREEZE;
		return -1;
	}

	awb_info[id].awb_gain.r = gray_g2r_sum / gray_w_cnt;
	awb_info[id].awb_gain.g = 256;
	awb_info[id].awb_gain.b = gray_g2b_sum / gray_w_cnt;

	awb_builtin_get_cur_ct(id, awb_info[id].awb_gain, &awb_info[id].cur_ct);

	awb_info[id].status = AWBALG_STATUS_CONVERGE;
	isp_builtin_set_cgain(id, &awb_info[id].awb_gain);
	isp_builtin_set_ct(id, awb_info[id].cur_ct);
	//printk("awb id = %d, status: %d, WB(%d, %d, %d), total win = %d \r\n",id , awb_info[id].status, awb_info[id].awb_gain.r, awb_info[id].awb_gain.g, awb_info[id].awb_gain.b, gray_w_cnt);

	return 0;
}

void awb_builtin_get_cur_ct(UINT32 id, ISP_BUILTIN_CGAIN cur_gain, UINT32 *ct)
{
	UINT16 i;
	INT32 r2b_table[ISP_AWB_CT_NUM];
	INT32 cur_r2b, cur_ct;
	INT32 idx1, idx2, delta1, delta2, delta3;

	for (i = 0; i < ISP_AWB_CT_NUM; i++) {
		r2b_table[i] = awb_builtin_ct_info[id].r_gain[i] * 256 / awb_builtin_ct_info[id].b_gain[i];
	}

	cur_r2b = cur_gain.r * 256 / cur_gain.b;

	if (cur_r2b <= r2b_table[0]) {
		cur_ct = awb_builtin_ct_info[id].temperature[0];
	} else if (cur_r2b >= r2b_table[ISP_AWB_CT_NUM - 1]) {
		cur_ct = awb_builtin_ct_info[id].temperature[ISP_AWB_CT_NUM - 1];
	} else {
		for (idx2 = 1; idx2 <= (ISP_AWB_CT_NUM - 1); idx2++) {
			if (cur_r2b < r2b_table[idx2]) {
				break;
			}
		}
		idx2 = CLAMP(idx2, 1, ISP_AWB_CT_NUM - 1);
		idx1 = idx2 - 1;
		delta1 = cur_r2b - r2b_table[idx1];
		delta2 = r2b_table[idx2] - cur_r2b;
		if ((delta1 + delta2) == 0) {
			printk("Interpolation Divide by 0");
			delta3 = 1;
		} else {
			delta3 = delta1 + delta2;
		}
		cur_ct = ((awb_builtin_ct_info[id].temperature[idx1] * delta2) + (awb_builtin_ct_info[id].temperature[idx2] * delta1)) / delta3;
	}

	*ct = (UINT32)cur_ct;

	return;
}

#endif
#endif

