#if defined(__KERNEL__)

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/div64.h>
#include <linux/math64.h>
#include <linux/of.h>

#include "ae_builtin.h"
#include "ae_common.h"

#if (NVT_FAST_ISP_FLOW)
//=============================================================================
// define
//=============================================================================

#define AE_BUILTIN_DBG_MSG    0
#define AE_BUILTIN_DBG_MSG2   0


#define EXPT_MIN_IMX290     50
#define GAIN_MIN_IMX290     100
#define EXPT_MAX_IMX290     33333
#define GAIN_MAX_IMX290     409600
#define DGAIN_TH_IMX290     409600

#define EXPT_MIN_F37        50
#define GAIN_MIN_F37        100
#define EXPT_MAX_F37        33333
#define GAIN_MAX_F37        12400
#define DGAIN_TH_F37        1550

#define EXPT_MIN_OS02K10    50
#define GAIN_MIN_OS02K10    100
#define EXPT_MAX_OS02K10    33333
#define GAIN_MAX_OS02K10    204800
#define DGAIN_TH_OS02K10    99200

#define EXPT_MIN_OS05A10    50
#define GAIN_MIN_OS05A10    100
#define EXPT_MAX_OS05A10    33333
#define GAIN_MAX_OS05A10    204800
#define DGAIN_TH_OS05A10    24800

#define EXPT_MIN_F35        50
#define GAIN_MIN_F35        100
#define EXPT_MAX_F35        33333
#define GAIN_MAX_F35        12400
#define DGAIN_TH_F35        1550

#define EXPT_MIN_GC4653     50
#define GAIN_MIN_GC4653     100
#define EXPT_MAX_GC4653     33333
#define GAIN_MAX_GC4653     75828*8
#define DGAIN_TH_GC4653     75828*8


#define FLICKER_BASE    10000
#define LA_X            32
#define LA_Y            32
#define LA_NUM          (LA_X * LA_Y)
#define AE_WIN_X        8
#define AE_WIN_Y        8
#define AE_WIN_NUM      (AE_WIN_X * AE_WIN_Y)

#define TARGET_Y        50
#define TARGET_IR_Y     48

#define P_FACTOR        12
#define AE_SPEED        128
#define AE_STABLE_RNG   10
#define AE_ISO_THRES    1600

#define SHDR_EV_RATIO   16

#define AE_FNO                      40

#define SUB_NODE_LENGTH 64

//=============================================================================
// global
//=============================================================================
typedef enum _AE_SHOOT_DIR {
	SHOOT_UP = 0,   // AE shoot up (increase gain)
	SHOOT_DOWN,     // AE shoot down (decrease gain)
} AE_SHOOT_DIR;

static AE_BUILTIN_INFO ae_builtin_info[ISP_BUILTIN_ID_MAX_NUM];
static AE_BUILTIN_SHDR ae_builtin_shdr[ISP_BUILTIN_ID_MAX_NUM];

static AE_BUILTIN_INFO ae_builtin_info_imx290 = {
	EXPT_MIN_IMX290, EXPT_MAX_IMX290,
	GAIN_MIN_IMX290, GAIN_MAX_IMX290,
	DGAIN_TH_IMX290,
	(UINT64)((UINT64)EXPT_MIN_IMX290*(UINT64)GAIN_MIN_IMX290), (UINT64)((UINT64)EXPT_MAX_IMX290*(UINT64)GAIN_MAX_IMX290)
};

static AE_BUILTIN_INFO ae_builtin_info_f37 = {
	EXPT_MIN_F37, EXPT_MAX_F37,
	GAIN_MIN_F37, GAIN_MAX_F37,
	DGAIN_TH_F37,
	(UINT64)((UINT64)EXPT_MIN_F37*(UINT64)GAIN_MIN_F37), (UINT64)((UINT64)EXPT_MAX_F37*(UINT64)GAIN_MAX_F37)
};

static AE_BUILTIN_INFO ae_builtin_info_os02k10 = {
	EXPT_MIN_OS02K10, EXPT_MAX_OS02K10,
	GAIN_MIN_OS02K10, GAIN_MAX_OS02K10,
	DGAIN_TH_OS02K10,
	(UINT64)((UINT64)EXPT_MIN_OS02K10*(UINT64)GAIN_MIN_OS02K10), (UINT64)((UINT64)EXPT_MAX_OS02K10*(UINT64)GAIN_MAX_OS02K10)
};

static AE_BUILTIN_INFO ae_builtin_info_os05a10 = {
	EXPT_MIN_OS05A10, EXPT_MAX_OS05A10,
	GAIN_MIN_OS05A10, GAIN_MAX_OS05A10,
	DGAIN_TH_OS05A10,
	(UINT64)((UINT64)EXPT_MIN_OS05A10*(UINT64)GAIN_MIN_OS05A10), (UINT64)((UINT64)EXPT_MAX_OS05A10*(UINT64)GAIN_MAX_OS05A10)
};

static AE_BUILTIN_INFO ae_builtin_info_f35 = {
	EXPT_MIN_F35, EXPT_MAX_F35,
	GAIN_MIN_F35, GAIN_MAX_F35,
	DGAIN_TH_F35,
	(UINT64)((UINT64)EXPT_MIN_F35*(UINT64)GAIN_MIN_F35), (UINT64)((UINT64)EXPT_MAX_F35*(UINT64)GAIN_MAX_F35)
};

static AE_BUILTIN_INFO ae_builtin_info_gc4653 = {
	EXPT_MIN_GC4653, EXPT_MAX_GC4653,
	GAIN_MIN_GC4653, GAIN_MAX_GC4653,
	DGAIN_TH_GC4653,
	(UINT64)((UINT64)EXPT_MIN_GC4653*(UINT64)GAIN_MIN_GC4653), (UINT64)((UINT64)EXPT_MAX_GC4653*(UINT64)GAIN_MAX_GC4653)
};

static UINT32 ae_sen_expt[ISP_BUILTIN_ID_MAX_NUM] = {10000, 10000, 10000, 10000, 10000};
static UINT32 ae_sen_gain[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 ae_isp_dgain[ISP_BUILTIN_ID_MAX_NUM] = {128, 128, 128, 128, 128};

static UINT32 ae_meter_matrix[ISP_BUILTIN_ID_MAX_NUM][AE_WIN_NUM] = {
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1
},
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1
},
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1
},
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1
},
{
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 4, 4, 3, 2, 1,
	1, 2, 3, 3, 3, 3, 2, 1,
	1, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1
}
};

AE_BUILTIN_FRONT_DTSI ae_front_dtsi = {
	DISABLE,
	50,
	33333
};

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// external functions
//=============================================================================
void ae_builtin_front_dtsi_load(void)
{
	CHAR node_path[SUB_NODE_LENGTH];
	CHAR sub_node_name[SUB_NODE_LENGTH];
	struct device_node* of_node;

	sprintf(node_path, "/fastboot/front_ae_0");

	sprintf(sub_node_name, "%s/%s", node_path, "enable");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&ae_front_dtsi.enable, sizeof(ae_front_dtsi.enable)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "sen_expt_min");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&ae_front_dtsi.sen_expt_min, sizeof(ae_front_dtsi.sen_expt_min)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

	sprintf(sub_node_name, "%s/%s", node_path, "sen_expt_max");
	of_node = of_find_node_by_path(sub_node_name);
	if (of_node) {
		if (of_property_read_u8_array(of_node, "data", (UINT8 *)&ae_front_dtsi.sen_expt_max, sizeof(ae_front_dtsi.sen_expt_max)) != 0) {
			printk("cannot find %s/data \r\n", sub_node_name);
		}
	} else {
		printk("cannot find %s \r\n", sub_node_name);
	}

#if 0
	printk("enable ========== %d\r\n", ae_front_dtsi.enable);
	printk("sen_expt ====== %d, %d\r\n", ae_front_dtsi.sen_expt_min, ae_front_dtsi.sen_expt_max);
#endif	
}

INT32 ae_builtin_init(UINT32 id)
{
	ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl;
	UINT32 sen_expt, sen_gain, isp_dgain;
	UINT32 isp_total_gain;
	UINT32 sen_fps = 3000;

	ae_builtin_front_dtsi_load();

	if (isp_builtin_get_sensor_name(id) == 0) {
		//printk("===== load ae_info_imx290 =====\n");
		ae_builtin_info[id] = ae_builtin_info_imx290;
	} else if (isp_builtin_get_sensor_name(id) == 1) {
		//printk("===== load ae_info_f37 =====\n");
		ae_builtin_info[id] = ae_builtin_info_f37;
	} else if (isp_builtin_get_sensor_name(id) == 2) {
		//printk("===== load ae_info_os02k10 =====\n");
		ae_builtin_info[id] = ae_builtin_info_os02k10;
	} else if (isp_builtin_get_sensor_name(id) == 3) {
		//printk("===== load ae_info_os05a10 =====\n");
		ae_builtin_info[id] = ae_builtin_info_os05a10;
	} else if (isp_builtin_get_sensor_name(id) == 4) {
		//printk("===== load ae_info_f35 =====\n");
		ae_builtin_info[id] = ae_builtin_info_f35;
	} else if (isp_builtin_get_sensor_name(id) == 5) {
		//printk("===== load ae_info_gc4653=\n");
		ae_builtin_info[id] = ae_builtin_info_gc4653;
	}

	sen_fps = isp_builtin_get_chgmode_fps(id);
	
	if(sen_fps < 1) {
		sen_fps = 1;
	}

	if(ae_front_dtsi.enable == ENABLE) {
		ae_builtin_info[id].expt_min = ae_front_dtsi.sen_expt_min;
		ae_builtin_info[id].expt_max = ae_front_dtsi.sen_expt_max;
	} else {
		ae_builtin_info[id].expt_max = isp_builtin_get_sensor_expt_max(id);
	}

#if 0
	printk("sen_expt    = %d, %d\n", ae_builtin_info.expt_min, ae_builtin_info.expt_max);
	printk("sen_gain    = %d, %d\n", ae_builtin_info.gain_min, ae_builtin_info.gain_max);
	printk("isp_gain_th = %d\n", ae_builtin_info.isp_gain_th);
	printk("total_gain  = %lld, %lld\n", ae_builtin_info.total_gain_min, ae_builtin_info.total_gain_max);
#endif
	// shdr parameter
	ae_builtin_shdr[id].enable = isp_builtin_get_shdr_enable(id);
	ae_builtin_shdr[id].frm_num = (ae_builtin_shdr[id].enable == ENABLE) ? 2 : 1;
	ae_builtin_shdr[id].mask = 0x00;
	ae_builtin_shdr[id].le_id = 1;
	ae_builtin_shdr[id].se_id = 0;
	ae_builtin_shdr[id].ev_ratio = SHDR_EV_RATIO;
	ae_builtin_shdr[id].expt_max = ((ae_builtin_info[id].expt_max*ae_builtin_shdr[id].ev_ratio)/(ae_builtin_shdr[id].ev_ratio+1));

	p_sensor_ctrl = isp_builtin_get_sensor_expt(id);
	sen_expt = p_sensor_ctrl->exp_time[0];
	p_sensor_ctrl = isp_builtin_get_sensor_gain(id);
	sen_gain = p_sensor_ctrl->gain_ratio[0]/10;
	isp_dgain = isp_builtin_get_dgain(id);
	isp_total_gain = ((sen_gain * isp_dgain)>>7);
	isp_builtin_set_total_gain(id, isp_total_gain);

	return 0;
}

UINT32 ae_builtin_get_lum(UINT32 id)
{
	ISP_BUILTIN_LA_RSLT *la_rslt;
	UINT32 ix, iy, iyw, win_ofst;
	UINT32 la_raw, raw_total, w_total;
	UINT32 ratio_x, ratio_y;
	UINT32 lum = 0;

	la_rslt = isp_builtin_get_la(id);

	if(la_rslt == NULL) {
		printk("isp_builtin_get_la is NULL\r\n");
		return 0;
	}

	raw_total = 0;
	w_total = 0;
	ratio_x = LA_X / AE_WIN_X;
	ratio_y = LA_Y / AE_WIN_Y;
	
	for (iy = 0; iy < LA_Y; iy++) {
		iyw = iy * LA_X;
		for (ix = 0; ix < LA_X; ix++) {
			la_raw = (UINT32)la_rslt->lum_1[iyw + ix];
			la_raw = AE_CLAMP(la_raw, 0, 4095);
			win_ofst = (((iy / ratio_y)*AE_WIN_X) + (ix / ratio_x));
			raw_total += (ae_meter_matrix[id][win_ofst] * la_raw);
			w_total += ae_meter_matrix[id][win_ofst];
		}
	}

	lum = ((raw_total / w_total)>>4);
	return lum;
}

INT32 ae_builtin_trig(UINT32 id, ISP_BUILTIN_AE_TRIG_MSG msg)
{
	INT64 total_gain, target_total_gain;
	INT64 curr_total_gain_delta, delta;
	UINT32 sen_expt, sen_gain;
	UINT32 curlum, speed;
	static UINT32 shoot_dir[ISP_BUILTIN_ID_MAX_NUM] = {SHOOT_DOWN, SHOOT_DOWN, SHOOT_DOWN, SHOOT_DOWN, SHOOT_DOWN};
	INT64 p_part;
	static INT64 prev_target_total_gain[ISP_BUILTIN_ID_MAX_NUM] = {0};
	UINT32 expt, gain, expt_idx;
	UINT32 converge_th_lo, converge_th_hi;
	UINT32 converge_rng_lo, converge_rng_hi;
	static UINT32 first_in[ISP_BUILTIN_ID_MAX_NUM] = {TRUE, TRUE, TRUE, TRUE, TRUE};
	UINT32 lv, isp_total_gain;
	UINT64 ev;
	static UINT32 ae_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};
	ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl;
	ISP_BUILTIN_SENSOR_CTRL sensor_ctrl;
	UINT32 scene_change_w;
	UINT32 sen_fps = 3000;
	UINT32 lum_ratio = 100;
	static UINT32 first_stable_flag[ISP_BUILTIN_ID_MAX_NUM] = {FALSE, FALSE, FALSE, FALSE, FALSE};
	UINT32 y_tar;

	if (msg == ISP_BUILTIN_AE_TRIG_RESET) {
		return 0;
	}

	ae_cnt[id]++;

	if((ae_cnt[id] >= 2)&&(ae_cnt[id] <= 3)) {
		return 0;
	}

	sen_fps = isp_builtin_get_chgmode_fps(id);
	
	if(sen_fps < 1) {
		sen_fps = 1;
	}

	if(ae_front_dtsi.enable == ENABLE) {
		ae_builtin_info[id].expt_min = ae_front_dtsi.sen_expt_min;
		ae_builtin_info[id].expt_max = ae_front_dtsi.sen_expt_max;
	} else {
		ae_builtin_info[id].expt_max = isp_builtin_get_sensor_expt_max(id);
	}

	if(ae_builtin_shdr[id].enable == ENABLE) {
		curlum = ae_builtin_get_lum(ae_builtin_shdr[id].le_id);
	} else {
		curlum = ae_builtin_get_lum(id);
	}
	
	curlum = AE_CLAMP(curlum, 1, 255);

#if 1
	converge_rng_lo = AE_STABLE_RNG;
	converge_rng_hi = AE_STABLE_RNG;
#else
	if(shoot_dir[id] == SHOOT_DOWN) {
		converge_rng_lo = (((AE_STABLE_RNG<<1)*1)>>2);
		converge_rng_hi = (((AE_STABLE_RNG<<1)*3)>>2);
	} else {
		converge_rng_lo = (((AE_STABLE_RNG<<1)*3)>>2);
		converge_rng_hi = (((AE_STABLE_RNG<<1)*1)>>2);
	}
#endif

	if(isp_builtin_get_night_mode(id)==TRUE) {
		y_tar = TARGET_IR_Y;
	} else {
		y_tar = TARGET_Y;
	}

	converge_th_lo = (y_tar*(100-converge_rng_lo))/100;
	converge_th_hi = (y_tar*(100+converge_rng_hi))/100;

	if((curlum >= converge_th_lo)&&(curlum <= converge_th_hi)) {
		//printk("===== AE_STABLE =====\r\n");
		first_stable_flag[id] = TRUE;
		scene_change_w = 100;//AE_CLAMP((ae_stable_cnt * 10), 0, 100);
		isp_builtin_set_scene_chg_w(id, scene_change_w);
		return 0;
	}	

	if(first_in[id] == true) {
		first_in[id] = false;
		p_sensor_ctrl = isp_builtin_get_sensor_expt(id);
		ae_sen_expt[id] = p_sensor_ctrl->exp_time[0];
		p_sensor_ctrl = isp_builtin_get_sensor_gain(id);
		ae_sen_gain[id] = p_sensor_ctrl->gain_ratio[0]/10;
	}

	ae_isp_dgain[id] = isp_builtin_get_dgain(id);
	
	sen_expt = ae_sen_expt[id];
	sen_gain = ae_sen_gain[id];

	total_gain = (INT64)(((UINT64)sen_expt * (UINT64)sen_gain*(UINT64)ae_isp_dgain[id])>>7);	
	target_total_gain = (INT64)ae_comm_uint64_div((UINT64)((UINT64)total_gain*(UINT64)y_tar), (UINT64)curlum);
	target_total_gain = AE_CLAMP(target_total_gain, ae_builtin_info[id].total_gain_min, ae_builtin_info[id].total_gain_max);

	if (prev_target_total_gain[id] == 0) {
		prev_target_total_gain[id] = total_gain;
	}

	shoot_dir[id] = (curlum > y_tar) ? SHOOT_DOWN : SHOOT_UP;

	curr_total_gain_delta = (INT64)((INT64)target_total_gain - (INT64)total_gain);

	p_part = (curr_total_gain_delta * P_FACTOR) >> 5;

	delta = p_part;

	if (shoot_dir[id] == SHOOT_DOWN) {
		delta = AE_MIN(0, delta);
		speed = AE_SPEED;
	} else {
		delta = AE_MAX(0, delta);
		speed = (AE_SPEED>>1);
	}

	delta = (delta * speed) >> 7;

	target_total_gain = (INT64)((INT64)prev_target_total_gain[id] + (INT64)delta);
	target_total_gain = AE_CLAMP(target_total_gain, ae_builtin_info[id].total_gain_min, ae_builtin_info[id].total_gain_max);

	if(target_total_gain < (FLICKER_BASE*ae_builtin_info[id].gain_min)) {
		gain = ae_builtin_info[id].gain_min;
		expt = (INT64)ae_comm_uint64_div((UINT64)target_total_gain, (UINT64)gain);
	} else {

		expt = sen_expt;
		gain = (UINT32)ae_comm_uint64_div((UINT64)target_total_gain, (UINT64)sen_expt);
	
		if(gain <= AE_ISO_THRES) {
			
			if(gain < ae_builtin_info[id].gain_min) {
				for(expt_idx=sen_expt; expt_idx>=FLICKER_BASE; expt_idx-=FLICKER_BASE) {
					expt = expt_idx;
					gain = (UINT32)ae_comm_uint64_div((UINT64)target_total_gain, (UINT64)expt);
					if(gain >= ae_builtin_info[id].gain_min) {
						break;
					}
				}
			}
			
			if(expt < FLICKER_BASE) {
				expt = FLICKER_BASE;
				gain = (UINT32)ae_comm_uint64_div((UINT64)target_total_gain, (UINT64)sen_expt);
			}
		} else {
			for(expt_idx=FLICKER_BASE; expt_idx<=ae_builtin_info[id].expt_max; expt_idx+=FLICKER_BASE) {
				expt = expt_idx;
				gain = (UINT32)ae_comm_uint64_div((UINT64)target_total_gain, (UINT64)expt);
				if(gain <= AE_ISO_THRES) {
					break;
				}
			}
		}
	}

	prev_target_total_gain[id] = target_total_gain;

	ae_sen_expt[id] = expt;
	ae_sen_gain[id] = gain;

	if(gain > ae_builtin_info[id].isp_gain_th) {
		ae_isp_dgain[id] = (UINT32)((gain<<7)/ae_builtin_info[id].isp_gain_th);
		ae_sen_gain[id] = ae_builtin_info[id].isp_gain_th;
	} else {
		ae_isp_dgain[id] = 128;
		ae_sen_gain[id] = gain;
	}
	
	ae_isp_dgain[id] = AE_CLAMP(ae_isp_dgain[id], 128, 1023);

	isp_total_gain = (UINT32)(((UINT64)ae_sen_gain[id] * (UINT64)ae_isp_dgain[id]) >> 7);
	
	ev = ae_comm_cal_ev((UINT64)expt, (UINT64)isp_total_gain, (UINT32)AE_FNO);
	lv = ae_comm_op_log2(ev, 10);

	sensor_ctrl.exp_time[0] = ae_sen_expt[id];
	sensor_ctrl.gain_ratio[0] = ae_sen_gain[id]*10;
	
	if(ae_builtin_shdr[id].enable == ENABLE) {			
		sensor_ctrl.exp_time[0] = AE_CLAMP(sensor_ctrl.exp_time[0], (ae_builtin_info[id].expt_min*ae_builtin_shdr[id].ev_ratio), ae_builtin_shdr[id].expt_max);		
		sensor_ctrl.exp_time[1] = (sensor_ctrl.exp_time[0]/ae_builtin_shdr[id].ev_ratio);
		sensor_ctrl.exp_time[1] = AE_CLAMP(sensor_ctrl.exp_time[1], ae_builtin_info[id].expt_min, (ae_builtin_shdr[id].expt_max/ae_builtin_shdr[id].ev_ratio));	
		sensor_ctrl.gain_ratio[1] = sensor_ctrl.gain_ratio[0];
	}

#if AE_BUILTIN_DBG_MSG2	
	if(id==0){
		printk("cnt=%d, simple %d exp=%d, gain=%d, isp_dgain=%d \r\n", ae_cnt[id], id, ae_sen_expt[id], ae_sen_gain[id], ae_isp_dgain[id]);
		
	}else{
		printk("cnt=%d, simple %d exp=%d, gain=%d, isp_dgain=%d \r\n", ae_cnt[id], id, ae_sen_expt[id], ae_sen_gain[id], ae_isp_dgain[id]);

	}
#endif

	
	isp_builtin_set_lv(id, lv);
	isp_builtin_set_sensor_expt(id, &sensor_ctrl);
	isp_builtin_set_sensor_gain(id, &sensor_ctrl);
	isp_builtin_set_total_gain(id, isp_total_gain);
	isp_builtin_set_dgain(id, ae_isp_dgain[id]);

	if(first_stable_flag[id] == TRUE) {
		scene_change_w = 100;
	} else {
		if(isp_total_gain >= ae_builtin_info[id].total_gain_max) {
			scene_change_w = 100;
		} else {
			lum_ratio = ((curlum*100)/y_tar);
			
			if(lum_ratio < 70) {
				scene_change_w = 0;
			} else {
				scene_change_w = lum_ratio;
			}
		}
	}

	scene_change_w = AE_CLAMP(scene_change_w, 0, 100);
	isp_builtin_set_scene_chg_w(id, scene_change_w);
	
#if AE_BUILTIN_DBG_MSG	
	//printk("curlum = %d, expt = %d, gain = %d\r\n", ae_builtin_get_lum(id), expt, gain);
	//printk("ev = %lld, lv = %d\r\n", ev, lv);
	printk("lum = %d, lv = %d, y_tar = %d, sen_expt = %d, sen_gain = %d, dgain = %d\r\n", curlum, lv, y_tar, sensor_ctrl.exp_time[0], sensor_ctrl.gain_ratio[0], ae_isp_dgain[id]);
#endif

	return 0;
}

#endif
#endif

