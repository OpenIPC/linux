
#include "comm/pwm.h"
#include "mtr_drv_ti8833.h"


//=============================================================================
// global variable
//=============================================================================
static ID SEMID_MTR_PWM, SEMID_MTR_MOVE;

const UINT32 g_focus_distance_tab[FOCUS_DISTANCE_TABLE_NUM] = { 5, 10, 15, 20, 30, 50, 70, 100, 140, 200, 300, 500, 9999 };    // unit: cm

#define APERTURE_INIT_DAC    0x00

const UINT32 g_aperture_2_target_pos[] = { APERTURE_INIT_DAC, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };    // *

static MOTOR_DRV_INFO g_lens_drv_info[MTR_ID_MAX] = {
	{
	.motor_config[0].chnel_sel         = PWM_MS_SET_1,                 // *
	.motor_config[0].gear_ratio        = 2,                            // *
	.motor_config[0].def_positive_dir  = 0,                            // *
	.motor_config[0].pwm_freq          = 8*700,                        // * 500~900 pps
	.motor_config[0].pwm_clk_div	   = 106,
	.motor_config[0].work_status       = MOTOR_STATUS_FREE,
	.motor_config[0].max_step_size     = 64,
	.motor_config[0].move_2_home       = FALSE,
	.motor_config[0].curr_position     = 0,
	.motor_config[0].section_pos       = 0,
	.motor_config[0].move_dir          = MOVE_DIR_INIT,
	.motor_config[0].phs_ste           = 0,
	.motor_config[0].move_speed        = MTR_SPEED_2X,
	.motor_config[0].gear_backlash     = 1,                            // *
	.motor_config[0].set_move_stop     = FALSE,
	.motor_config[0].init_pos_ofst     = 10,                           // *
	.motor_config[0].max_move_distance = 1420,                         // *

	.motor_config[1].chnel_sel         = PWM_MS_SET_0,                 // *
	.motor_config[1].gear_ratio 	   = 2,                            // *
	.motor_config[1].def_positive_dir  = 0,                            // *
	.motor_config[1].pwm_freq          = 8*1000,                       // * 500~900 pps
	.motor_config[1].pwm_clk_div	   = 149,
	.motor_config[1].work_status       = MOTOR_STATUS_FREE,
	.motor_config[1].max_step_size     = 32,
	.motor_config[1].move_2_home       = FALSE,
	.motor_config[1].curr_position     = 0,
	.motor_config[1].section_pos       = ZOOM_SECTION_WIDE,
	.motor_config[1].move_dir          = MOVE_DIR_INIT,
	.motor_config[1].phs_ste           = 0,
	.motor_config[1].move_speed        = MTR_SPEED_1X,
	.motor_config[1].gear_backlash     = 1,                            // *
	.motor_config[1].set_move_stop     = FALSE,
	.motor_config[1].init_pos_ofst     = 10,                           // *
	.motor_config[1].max_move_distance = 660,                          // *

	.motor_config[2].chnel_sel         = PWM_MS_SET_0,                 // *
	.motor_config[2].gear_ratio 	   = 1,                            // *
	.motor_config[2].def_positive_dir  = 0,                            // *
	.motor_config[2].pwm_freq          = 8*200,                        // * 100~200 pps
	.motor_config[2].pwm_clk_div	   = 749,
	.motor_config[2].work_status       = MOTOR_STATUS_FREE,
	.motor_config[2].max_step_size     = 8,
	.motor_config[2].move_2_home       = FALSE,
	.motor_config[2].curr_position     = APERTURE_INIT_DAC,
	.motor_config[2].section_pos       = 0,
	.motor_config[2].move_dir          = MOVE_DIR_INIT,
	.motor_config[2].phs_ste           = 0,
	.motor_config[2].move_speed        = MTR_SPEED_1X,
	.motor_config[2].gear_backlash     = 2,
	.motor_config[2].set_move_stop     = FALSE,
	.motor_config[2].init_pos_ofst     = 0,                            // *
	.motor_config[2].max_move_distance = 100,                          // *

	.ircut_state       = IRCUT_OPEN,
	.aperture_sec_pos  = APERTURE_POS_BIG,
	.shutter_state     = SHUTTER_OPEN,

	.lens_proty.focal_len_min = 3200,    // unit: um
	.lens_proty.focal_len_max = 9800,    // unit: um

	.lens_hw.pin_def = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	.zoom_focus_tab[0] =  { 0,      0,  140  },     // * 0
	.zoom_focus_tab[1] =  { 20,   110,  250  },     // * 1
	.zoom_focus_tab[2] =  { 50,   315,  455  },     // * 2
	.zoom_focus_tab[3] =  { 100,  575,  715  },     // * 3
	.zoom_focus_tab[4] =  { 160,  795,  935  },     // * 4
	.zoom_focus_tab[5] =  { 220,  955, 1095  },     // * 5
	.zoom_focus_tab[6] =  { 290, 1095, 1235  },     // * 6
	.zoom_focus_tab[7] =  { 360, 1180, 1320  },     // * 7
	.zoom_focus_tab[8] =  { 440, 1240, 1380  },     // * 8
	.zoom_focus_tab[9] =  { 520, 1270, 1410  },     // * 9
	.zoom_focus_tab[10] = { 650, 1275, 1415  },     // * 10

                          // 0.05   0.1  0.15   0.2   0.3   0.5   0.7   1.0   1.4   2.0   3.0   5.0  infinite (unit: meter)
	.focus_distance_tab[0]  = {{   0,    0,    7,   13,   18,   21,   23,   25,   27,   29,   30,  31,  33  }},    // * 0
	.focus_distance_tab[1]  = {{ 120,  120,  127,  133,  138,  141,  143,  145,  147,  149,  150, 151,  153 }},    // * 1
	.focus_distance_tab[2]  = {{ 230,  230,  237,  243,  248,  251,  253,  255,  257,  259,  260, 261,  263 }},    // * 2
	.focus_distance_tab[3]  = {{ 335,  335,  342,  348,  353,  356,  358,  360,  362,  364,  365, 366,  368 }},    // * 3
	.focus_distance_tab[4]  = {{ 435,  435,  442,  448,  453,  456,  458,  460,  462,  464,  465, 466,  468 }},    // * 4
	.focus_distance_tab[5]  = {{ 530,  530,  537,  543,  548,  551,  553,  555,  557,  559,  560, 561,  563 }},    // * 5
	.focus_distance_tab[6]  = {{ 620,  620,  627,  633,  638,  641,  643,  645,  647,  649,  650, 651,  653 }},    // * 6
	.focus_distance_tab[7]  = {{ 705,  705,  712,  718,  723,  726,  728,  730,  732,  734,  735, 736,  738 }},    // * 7
	.focus_distance_tab[8]  = {{ 795,  795,  802,  808,  813,  816,  818,  820,  822,  824,  825, 826,  828 }},    // * 8
	.focus_distance_tab[9]  = {{ 900,  900,  907,  913,  918,  921,  923,  925,  927,  929,  930, 931,  933 }},    // * 9
	.focus_distance_tab[10] = {{ 900,  900,  907,  913,  918,  921,  923,  925,  927,  929,  930, 931,  933 }},    // * 10
	},
};

const UINT32 g_pwm_chanel[][4] = {
	// A1 A2 B1 B2
	{ 0, 4, 2, 6 },    //Step0
	{ 2, 6, 4, 0 },    //Step1
	{ 4, 0, 6, 2 },    //Step2
	{ 6, 2, 0, 4 },    //Step3
};


//=============================================================================
// routine
//=============================================================================
MOTOR_DRV_INFO* mdrv_get_motor_info(UINT32 id)
{
	id = MTR_MIN(id, MTR_ID_MAX-1);

	return &g_lens_drv_info[id];
}

static INT32 mdrv_init_pwm(UINT32 id)
{
	static BOOL init_already = FALSE;
	INT32 ret_sts = 0;
	UINT32 i, tmp;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (init_already == TRUE) {
		DBG_WRN("GPIO already init!\r\n");
		return -1;
	}

#ifdef __KERNEL__
	// request GPIO
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], "MTR_RST")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN], "MTR_PDN")) < 0) {
			return ret_sts;
		}
	}
	if ((ret_sts = gpio_request(MOTOR_IRIS_ENABLE_PIN, "MOTOR_IRIS_ENABLE")) < 0) {
		return ret_sts;
	}
#endif

	if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN], 0);
	}
	gpio_direction_output(MOTOR_IRIS_ENABLE_PIN, 1);

	for (i = 0; i < 2; i++) {
		if (pwm_open_set(pmotor_info->motor_config[i].chnel_sel) != E_OK) {
			DBG_ERR("micro-step #%u open err!\r\n", i);
			return -1;
		}

		// output freq = 120M / (clk_div+1) / base_period
		tmp = 120000000 / 100 / (pmotor_info->motor_config[i].pwm_freq * pmotor_info->motor_config[i].move_speed / MTR_SPEED_1X);
		pmotor_info->motor_config[i].pwm_clk_div = MTR_CLAMP(tmp, 60, 2000);
		pwm_mstep_config_clock_div(pmotor_info->motor_config[i].chnel_sel, pmotor_info->motor_config[i].pwm_clk_div);
	}

	tmp = 120000000 / 100 / (pmotor_info->motor_config[2].pwm_freq * pmotor_info->motor_config[2].move_speed / MTR_SPEED_1X);
	pmotor_info->motor_config[2].pwm_clk_div = MTR_CLAMP(tmp, 60, 2000);

	init_already = TRUE;

	return ret_sts;
}

static void mdrv_generate_pwm(UINT32 id, MOTOR_SEL motor_sel, INT32 step_pos)
{
	UINT32 tbl_idx = 0;
	static MSCOMMON_CFG ms_cfg = { .ui_dir = 0, .ui_on_cycle = 0, .ui_step_per_phase = TOTAL_08_STEP_PER_PHASE, .ui_phase_type = PWM_MS_2_2_PHASE_TYPE, .is_square_wave = TRUE, .ui_threshold_en = FALSE, .ui_threshold = 0 };
	static MS_CH_PHASE_CFG ms_phs_cfg;
	MOTOR_CONFIG *pmotor_cfg = NULL;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (motor_sel == MOTOR_FOCUS) {
		pmotor_cfg = &pmotor_info->motor_config[0];
	} else if (motor_sel == MOTOR_ZOOM) {
		pmotor_cfg = &pmotor_info->motor_config[1];

		pwm_mstep_config_clock_div(pmotor_cfg->chnel_sel, pmotor_cfg->pwm_clk_div);
		gpio_set_value(MOTOR_IRIS_ENABLE_PIN, 1);
	} else if (motor_sel == MOTOR_IRIS) {
		pmotor_cfg = &pmotor_info->motor_config[2];

		pwm_mstep_config_clock_div(pmotor_cfg->chnel_sel, pmotor_cfg->pwm_clk_div);
		gpio_set_value(MOTOR_IRIS_ENABLE_PIN, 0);
	}

	if (pmotor_cfg == NULL) {
		DBG_ERR("pmotor_cfg is NULL!\r\n");

		return;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_PWM) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return;
	}

	tbl_idx = pmotor_cfg->phs_ste;
	pmotor_cfg->phs_ste = (pmotor_cfg->phs_ste + (MTR_ABS(step_pos) % 4)) % 4;

	if (pmotor_cfg->def_positive_dir == 1) {
		ms_cfg.ui_dir = (step_pos >= 0) ? MS_DIR_DECREASE : MS_DIR_INCREASE;
	} else {
		ms_cfg.ui_dir = (step_pos >= 0) ? MS_DIR_INCREASE : MS_DIR_DECREASE;
	}

	ms_cfg.ui_on_cycle = MTR_ABS(step_pos);

	ms_phs_cfg.ui_ch0_phase = g_pwm_chanel[tbl_idx][0];
	ms_phs_cfg.ui_ch1_phase = g_pwm_chanel[tbl_idx][1];
	ms_phs_cfg.ui_ch2_phase = g_pwm_chanel[tbl_idx][2];
	ms_phs_cfg.ui_ch3_phase = g_pwm_chanel[tbl_idx][3];

	pwm_mstep_config_set(pmotor_cfg->chnel_sel, &ms_phs_cfg, &ms_cfg);
	pwm_pwm_enable_set(pmotor_cfg->chnel_sel);    // excitation
	vos_util_delay_ms(8);

	pwm_mstep_enable_set(pmotor_cfg->chnel_sel);    // enable u-step function
	pwm_wait_set(pmotor_cfg->chnel_sel);

	vos_util_delay_ms(8);
	pwm_pwm_disable_set(pmotor_cfg->chnel_sel);

	vos_sem_sig(SEMID_MTR_PWM);
}

#if ZOOM_FOCUS_SIM_MOVE
static void mdrv_generate_focus_zoom_pwm(UINT32 id, INT32 fstep_pos, INT32 zstep_pos)
{
	UINT32 tbl_idx[2];
	static MSCOMMON_CFG ms_cfg[2] = { { .ui_dir = MS_DIR_INCREASE, .ui_on_cycle = 0, .ui_step_per_phase = TOTAL_08_STEP_PER_PHASE, .ui_phase_type = PWM_MS_2_2_PHASE_TYPE, .is_square_wave = TRUE, .ui_threshold_en = FALSE, .ui_threshold = 0 },
					  { .ui_dir = MS_DIR_INCREASE, .ui_on_cycle = 0, .ui_step_per_phase = TOTAL_08_STEP_PER_PHASE, .ui_phase_type = PWM_MS_2_2_PHASE_TYPE, .is_square_wave = TRUE, .ui_threshold_en = FALSE, .ui_threshold = 0 } };
	static MS_CH_PHASE_CFG ms_phs_cfg[2];
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_PWM) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return;
	}

	tbl_idx[1] = pmotor_info->motor_config[1].phs_ste;
	tbl_idx[0] = pmotor_info->motor_config[0].phs_ste;
	pmotor_info->motor_config[1].phs_ste = (pmotor_info->motor_config[1].phs_ste + (MTR_ABS(zstep_pos) % 4)) % 4;
	pmotor_info->motor_config[0].phs_ste = (pmotor_info->motor_config[0].phs_ste + (MTR_ABS(fstep_pos) % 4)) % 4;

	pwm_mstep_config_clock_div(pmotor_info->motor_config[1].chnel_sel, pmotor_info->motor_config[1].pwm_clk_div);
	gpio_set_value(MOTOR_IRIS_ENABLE_PIN, 1);

	if (pmotor_info->motor_config[1].def_positive_dir == 1) {
		ms_cfg[1].ui_dir = (zstep_pos >= 0) ? MS_DIR_DECREASE : MS_DIR_INCREASE;
	} else {
		ms_cfg[1].ui_dir = (zstep_pos >= 0) ? MS_DIR_INCREASE : MS_DIR_DECREASE;
	}
	if (pmotor_info->motor_config[0].def_positive_dir == 1) {
		ms_cfg[0].ui_dir = (fstep_pos >= 0) ? MS_DIR_DECREASE : MS_DIR_INCREASE;
	} else {
		ms_cfg[0].ui_dir = (fstep_pos >= 0) ? MS_DIR_INCREASE : MS_DIR_DECREASE;
	}

	ms_cfg[1].ui_on_cycle = MTR_ABS(zstep_pos);
	ms_cfg[0].ui_on_cycle = MTR_ABS(fstep_pos);

	ms_phs_cfg[1].ui_ch0_phase = g_pwm_chanel[tbl_idx[1]][0];
	ms_phs_cfg[1].ui_ch1_phase = g_pwm_chanel[tbl_idx[1]][1];
	ms_phs_cfg[1].ui_ch2_phase = g_pwm_chanel[tbl_idx[1]][2];
	ms_phs_cfg[1].ui_ch3_phase = g_pwm_chanel[tbl_idx[1]][3];
	ms_phs_cfg[0].ui_ch0_phase = g_pwm_chanel[tbl_idx[0]][0];
	ms_phs_cfg[0].ui_ch1_phase = g_pwm_chanel[tbl_idx[0]][1];
	ms_phs_cfg[0].ui_ch2_phase = g_pwm_chanel[tbl_idx[0]][2];
	ms_phs_cfg[0].ui_ch3_phase = g_pwm_chanel[tbl_idx[0]][3];

	if (zstep_pos != 0) {
		pwm_mstep_config_set(pmotor_info->motor_config[1].chnel_sel, &ms_phs_cfg[1], &ms_cfg[1]);
		pwm_pwm_enable_set(pmotor_info->motor_config[1].chnel_sel);    // excitation
	}
	if (fstep_pos != 0) {
		pwm_mstep_config_set(pmotor_info->motor_config[0].chnel_sel, &ms_phs_cfg[0], &ms_cfg[0]);
		pwm_pwm_enable_set(pmotor_info->motor_config[0].chnel_sel);    // excitation
	}

	vos_util_delay_ms(8);

	if (zstep_pos != 0) {
		pwm_mstep_enable_set(pmotor_info->motor_config[1].chnel_sel);    // enable u-step function
	}
	if (fstep_pos != 0) {
		pwm_mstep_enable_set(pmotor_info->motor_config[0].chnel_sel);    // enable u-step function
	}

	if ((MTR_ABS(zstep_pos) * pmotor_info->motor_config[1].pwm_clk_div) >= (MTR_ABS(fstep_pos) * pmotor_info->motor_config[0].pwm_clk_div)) {
		if (fstep_pos != 0) {
			pwm_wait_set(pmotor_info->motor_config[0].chnel_sel);
		}
		if (zstep_pos != 0) {
			pwm_wait_set(pmotor_info->motor_config[1].chnel_sel);
		}
	} else {
		if (zstep_pos != 0) {
			pwm_wait_set(pmotor_info->motor_config[1].chnel_sel);
		}
		if (fstep_pos != 0) {
			pwm_wait_set(pmotor_info->motor_config[0].chnel_sel);
		}
	}

	vos_util_delay_ms(8);

	if (zstep_pos != 0) {
		pwm_pwm_disable_set(pmotor_info->motor_config[1].chnel_sel);
	}
	if (fstep_pos != 0) {
		pwm_pwm_disable_set(pmotor_info->motor_config[0].chnel_sel);
	}

	vos_sem_sig(SEMID_MTR_PWM);
}
#endif

static INT32 mdrv_init_motor(UINT32 id, MOTOR_SEL motor_sel)
{
	if (motor_sel > MOTOR_IRIS) {
		DBG_WRN("unknown motor chnel!\r\n");

		return -1;
	}

	return 0;
}

static INT32 mdrv_set_single_position(UINT32 id, MOTOR_SEL motor_sel, INT32 step_pos)
{
	MOTOR_CONFIG *pmotor_cfg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg = (motor_sel == MOTOR_FOCUS) ? &pmotor_info->motor_config[0] :
		    ((motor_sel == MOTOR_ZOOM) ? &pmotor_info->motor_config[1] :
		    ((motor_sel == MOTOR_IRIS) ? &pmotor_info->motor_config[2] : NULL));

	if (pmotor_cfg == NULL) {
		DBG_WRN("unknown motor chnel!\r\n");

		return -1;
	}

	if (step_pos != 0) {
		pmotor_cfg->work_status = MOTOR_STATUS_BUSY;

		mdrv_generate_pwm(id, motor_sel, pmotor_cfg->gear_ratio*step_pos);

		pmotor_cfg->work_status = MOTOR_STATUS_FREE;
	}

	return step_pos;
}

#if ZOOM_FOCUS_SIM_MOVE
static void mdrv_set_position(UINT32 id, INT32 fstep_pos, INT32 zstep_pos)
{
	MOTOR_CONFIG *pmotor_cfg[2] = { NULL, NULL };
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg[0] = &pmotor_info->motor_config[0];
	pmotor_cfg[1] = &pmotor_info->motor_config[1];

	if ((fstep_pos != 0) || (zstep_pos != 0)) {
		pmotor_cfg[0]->work_status = (fstep_pos != 0) ? MOTOR_STATUS_BUSY : MOTOR_STATUS_FREE;
		pmotor_cfg[1]->work_status = (zstep_pos != 0) ? MOTOR_STATUS_BUSY : MOTOR_STATUS_FREE;

		mdrv_generate_focus_zoom_pwm(id, pmotor_cfg[0]->gear_ratio*fstep_pos, pmotor_cfg[1]->gear_ratio*zstep_pos);

		pmotor_cfg[0]->work_status = pmotor_cfg[1]->work_status = MOTOR_STATUS_FREE;
	}
}
#endif

void ti8833_open(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	vos_sem_create(&SEMID_MTR_PWM, 1, "SEMID_MTR_PWM");
	vos_sem_create(&SEMID_MTR_MOVE, 1, "SEMID_MTR_MOVE");

	if (mdrv_init_pwm(id) == 0) {
		// reset motor driver IC
		if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] > 0) {
			gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
			vos_util_delay_ms(5);
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 1);
		}

		mdrv_init_motor(id, MOTOR_ZOOM);
		mdrv_init_motor(id, MOTOR_FOCUS);
		mdrv_init_motor(id, MOTOR_IRIS);
	} else {
		DBG_ERR("mdrv_init_pwm err!\r\n");
	}
}

void ti8833_close(UINT32 id)
{
	UINT32 i;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

#ifdef __KERNEL__
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN]);
	}
	if (MOTOR_IRIS_ENABLE_PIN != 0) {
		gpio_free(MOTOR_IRIS_ENABLE_PIN);
	}
#endif

	for (i = 0; i < 2; i++) {
		pwm_close_set(pmotor_info->motor_config[i].chnel_sel, FALSE);
	}

	vos_sem_destroy(SEMID_MTR_PWM);
	vos_sem_destroy(SEMID_MTR_MOVE);
}


//----------------------------------------------------------------------------------------------------------------------
// focus
//----------------------------------------------------------------------------------------------------------------------
static UINT32 mdrv_calc_focus_range(UINT32 id, INT32 *pmin_value, INT32 *pmax_value, INT32 zoom_pos, BOOL limit_range)
{
	UINT32 i = ZOOM_SECTION_WIDE;
	INT32 tmp, fmin, fmax;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (limit_range == TRUE) {
		fmin = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
		fmax = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_max;

		zoom_pos = (zoom_pos < 0) ? pmotor_info->motor_config[1].curr_position : zoom_pos;

		for (i = ZOOM_SECTION_WIDE; i < ZOOM_SECTION_NUM; i++) {
			if (pmotor_info->zoom_focus_tab[i].zoom_position >= zoom_pos) {
				tmp = (i != ZOOM_SECTION_WIDE) ? (pmotor_info->zoom_focus_tab[i].zoom_position - pmotor_info->zoom_focus_tab[i-1].zoom_position) : 1;

				if (tmp != 0) {
					fmin = (i != ZOOM_SECTION_WIDE) ? (pmotor_info->zoom_focus_tab[i-1].focus_min
						 + (pmotor_info->zoom_focus_tab[i].focus_min - pmotor_info->zoom_focus_tab[i-1].focus_min) * (pmotor_info->motor_config[1].curr_position - pmotor_info->zoom_focus_tab[i-1].zoom_position) / tmp) : fmin;
					fmax = (i != ZOOM_SECTION_WIDE) ? (pmotor_info->zoom_focus_tab[i-1].focus_max
						 + (pmotor_info->zoom_focus_tab[i].focus_max - pmotor_info->zoom_focus_tab[i-1].focus_max) * (pmotor_info->motor_config[1].curr_position - pmotor_info->zoom_focus_tab[i-1].zoom_position) / tmp) : fmax;
				} else {
					DBG_ERR("incorrect zoom_focus_tab!\r\n");
				}
				break;
			}
		}
	} else {
		fmin = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
		fmax = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].focus_max;
	}

	*pmin_value = fmin;
	*pmax_value = fmax;

	return i;
}

static UINT32 mdrv_get_focus_speed(UINT32 id)
{
	UINT32 focus_spd;

	switch ((UINT32)mdrv_get_motor_info(id)->motor_config[0].move_speed) {
	case MTR_SPEED_QUARTER:
		focus_spd = 20;
		break;

	case MTR_SPEED_HALF:
		focus_spd = 40;
		break;

	case MTR_SPEED_1X:
	default:
		focus_spd = 60;
		break;

	case MTR_SPEED_2X:
		focus_spd = 80;
		break;

	case MTR_SPEED_4X:
		focus_spd = 100;
		break;
	}

	return focus_spd;
}

static void mdrv_set_focus_speed(UINT32 id, UINT32 speed)
{
	UINT32 i, tmp, idx = 1;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	speed = MTR_CLAMP(speed, MTR_SPEED_FULL/5, MTR_SPEED_FULL);
	tmp = (speed / (MTR_SPEED_FULL / 5)) - 1;

	if (tmp != 0) {
		for (i = 0; i < tmp; i++) {
			idx *= 2;
		}
	}

	pmotor_info->motor_config[0].move_speed = (MTR_MOTOR_SPEED)idx;

	tmp = 120000000 / 100 / (pmotor_info->motor_config[0].pwm_freq * pmotor_info->motor_config[0].move_speed / MTR_SPEED_1X);
	pmotor_info->motor_config[0].pwm_clk_div = MTR_CLAMP(tmp, 60, 2000);
}

static BOOL mdrv_set_focus_move(UINT32 id, MOTOR_ROTATE_DIR state, UINT32 param)
{
	BOOL ret_state = TRUE;
	INT32 steps, gear_backlash, move_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	steps = (INT32)(param & 0xFFFF);
	gear_backlash = (INT32)((param >> 16) & 0xFFFF);

	switch ((UINT32)state) {
	case MOTOR_ROTATE_FWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[0].move_dir = MOVE_DIR_FWD;
		}

		mdrv_set_single_position(id, MOTOR_FOCUS, move_step);

		pmotor_info->motor_config[0].curr_position += steps;
		break;

	case MOTOR_ROTATE_BWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[0].move_dir = MOVE_DIR_BWD;
		}

		mdrv_set_single_position(id, MOTOR_FOCUS, -move_step);

		pmotor_info->motor_config[0].curr_position -= steps;
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}

	return ret_state;
}

static INT32 mdrv_set_focus_pos(UINT32 id, MOTOR_DRV_INFO *pmotor_info, INT32 target_pos, BOOL upd_by_zoom)
{
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step, acc_step = 0;

	if (upd_by_zoom == FALSE) {
		pmotor_info->motor_config[0].set_move_stop = FALSE;
	}

	if (target_pos > pmotor_info->motor_config[0].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[0].gear_backlash != 0) && (pmotor_info->motor_config[0].move_dir != MOVE_DIR_FWD)) {
			rest_step = pmotor_info->motor_config[0].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
						  ? pmotor_info->motor_config[0].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = target_pos - pmotor_info->motor_config[0].curr_position;

		do {
			set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
					  ? pmotor_info->motor_config[0].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;

			if ((upd_by_zoom == FALSE) && ((rest_step <= 0) || (pmotor_info->motor_config[0].set_move_stop == TRUE))) {
				rest_step = 0;
			}

			mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	} else if (target_pos < pmotor_info->motor_config[0].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[0].gear_backlash != 0) && (pmotor_info->motor_config[0].move_dir != MOVE_DIR_BWD)) {
			rest_step = pmotor_info->motor_config[0].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
						  ? pmotor_info->motor_config[0].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = pmotor_info->motor_config[0].curr_position - target_pos;

		do {
			set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
					  ? pmotor_info->motor_config[0].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;

			if ((upd_by_zoom == FALSE) && ((rest_step <= 0) || (pmotor_info->motor_config[0].set_move_stop == TRUE))) {
				rest_step = 0;
			}

			mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, cmd_param);
		} while (rest_step > 0);
	} else {
		DBG_WRN("set the same pos!\r\n");
	}

	return acc_step;
}

INT32 ti8833_init_focus(UINT32 id)
{
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	rest_step = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].focus_max + pmotor_info->motor_config[0].init_pos_ofst
			  + (pmotor_info->motor_config[0].max_move_distance / 10);

	do {
		set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
				  ? pmotor_info->motor_config[0].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_FOCUS_GOHOME_DIR
		mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// increase go home consistency
	vos_util_delay_ms(500);

	// tighten motor and gear
	rest_step = pmotor_info->motor_config[0].max_move_distance / 30;

	do {
		set_step = (rest_step > pmotor_info->motor_config[0].max_step_size) ? pmotor_info->motor_config[0].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_FOCUS_GOHOME_DIR
		mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// compensate backlash first
	if ((pmotor_info->motor_config[0].gear_backlash != 0) && (pmotor_info->motor_config[0].move_dir != MOVE_DIR_FWD)) {
		rest_step = pmotor_info->motor_config[0].gear_backlash;

		do {
			set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
					  ? pmotor_info->motor_config[0].max_step_size : rest_step;
			cmd_param = (set_step << 16) + 0;
			rest_step -= set_step;

			mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	}

#if REVERSE_FOCUS_GOHOME_DIR
	rest_step = MTR_MAX(pmotor_info->motor_config[0].init_pos_ofst+pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].focus_max, 0);
#else
	rest_step = MTR_MAX(pmotor_info->motor_config[0].init_pos_ofst+pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min, 0);
#endif

	do {
		set_step = (rest_step > pmotor_info->motor_config[0].max_step_size)
				  ? pmotor_info->motor_config[0].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_FOCUS_GOHOME_DIR
		mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, cmd_param);
#else
		mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, cmd_param);
#endif
	} while (rest_step > 0);

	pmotor_info->motor_config[0].curr_position = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
	pmotor_info->motor_config[0].move_2_home = TRUE;

	return E_OK;
}

UINT32 ti8833_get_focus_speed(UINT32 id)
{
	return mdrv_get_focus_speed(id);
}

void ti8833_set_focus_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category)
{
	UINT32 curr_spd, focus_spd = 20;

	curr_spd = mdrv_get_focus_speed(id);

	switch ((UINT32)spd_category) {
	case MOTOR_SPEED_VERY_LOW:
		focus_spd = 20;
		break;

	case MOTOR_SPEED_LOW:
		focus_spd = 40;
		break;

	case MOTOR_SPEED_MEDIUM:
		focus_spd = 60;
		break;

	case MOTOR_SPEED_HIGH:
		focus_spd = 80;
		break;

	case MOTOR_SPEED_VERY_HIGH:
		focus_spd = 100;
		break;
	}

	if (curr_spd != focus_spd) {
		mdrv_set_focus_speed(id, focus_spd);
	} else {
		DBG_WRN("set the same speed!\r\n");
	}
}

void ti8833_assign_focus_position(UINT32 id, INT32 focus_pos, INT32 dir)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_info->motor_config[0].curr_position = focus_pos;
	pmotor_info->motor_config[0].move_dir = (dir >= 1) ? MOVE_DIR_FWD : MOVE_DIR_BWD;

	pmotor_info->motor_config[0].move_2_home = TRUE;
}

UINT32 ti8833_get_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos)
{
	UINT32 focal_length;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	zoom_sec_pos = MTR_CLAMP(zoom_sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	if (zoom_sec_pos > ZOOM_SECTION_WIDE) {
		focal_length = pmotor_info->lens_proty.focal_len_min + (zoom_sec_pos - ZOOM_SECTION_WIDE)
					* (pmotor_info->lens_proty.focal_len_max - pmotor_info->lens_proty.focal_len_min + (ZOOM_SECTION_TELE - ZOOM_SECTION_WIDE) / 2) / (ZOOM_SECTION_TELE - ZOOM_SECTION_WIDE);
	} else {
		focal_length = pmotor_info->lens_proty.focal_len_min;
	}

	return focal_length;
}

void ti8833_set_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos, UINT32 focal_length)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	zoom_sec_pos = MTR_CLAMP(zoom_sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	if (zoom_sec_pos == ZOOM_SECTION_WIDE) {
		pmotor_info->lens_proty.focal_len_min = MTR_MIN(focal_length, 15000);     // max: 15 mm
	} else if (zoom_sec_pos == ZOOM_SECTION_TELE) {
		pmotor_info->lens_proty.focal_len_max = MTR_MIN(focal_length, 200000);    // max: 200 mm
	}
}

INT32 ti8833_get_focus_range(UINT32 id)
{
	INT32 low_bound = 0, high_bound = 0;

	mdrv_calc_focus_range(id, &low_bound, &high_bound, -1, TRUE);

	return ((high_bound << 16) + low_bound);
}

INT32 ti8833_get_focus_position(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (pmotor_info->motor_config[0].move_dir << 16) | pmotor_info->motor_config[0].curr_position;
}

INT32 ti8833_set_focus_position(UINT32 id, INT32 focus_pos, BOOL limit_range)
{
	INT32 acc_step = 0, near_bound = 0, inf_bound = 1024;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (pmotor_info->motor_config[0].move_2_home == FALSE) {
		DBG_ERR("not initialized yet!\r\n");

		return 0;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_MOVE) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return 0;
	}

	mdrv_calc_focus_range(id, &near_bound, &inf_bound, -1, limit_range);

	if ((focus_pos >= near_bound) && (focus_pos <= inf_bound)) {
		if (focus_pos != pmotor_info->motor_config[0].curr_position) {
			pmotor_info->motor_config[0].set_move_stop = FALSE;

			acc_step = mdrv_set_focus_pos(id, pmotor_info, focus_pos, FALSE);
		} else {
			DBG_WRN("step=0!\r\n");
		}
	} else {
		DBG_WRN("set out of range pos!\r\n");
	}

	vos_sem_sig(SEMID_MTR_MOVE);

	return acc_step;
}

INT32 ti8833_press_focus_move(UINT32 id, BOOL move_dir)
{
	INT32 target_pos;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (move_dir == FALSE) {    // FOCUS_WIDE = 0
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
	} else {    // FOCUS_TELE = 1
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].focus_max;
	}

	ti8833_set_focus_position(id, target_pos, FALSE);

	return E_OK;
}

void ti8833_release_focus_move(UINT32 id)
{
	mdrv_get_motor_info(id)->motor_config[0].set_move_stop = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// zoom
//----------------------------------------------------------------------------------------------------------------------
#if ZOOM_FOCUS_SIM_MOVE
static BOOL mdrv_set_move(UINT32 id, MOTOR_ROTATE_DIR state, UINT32 fparam, UINT32 zparam)
{
	BOOL ret_state = TRUE;
	INT32 zsteps, fsteps, zgear_backlash, fgear_backlash, zmove_step = 0, fmove_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	fsteps = (INT32)(fparam & 0xFFFF);
	fgear_backlash = (INT32)((fparam >> 16) & 0xFFFF);
	zsteps = (INT32)(zparam & 0xFFFF);
	zgear_backlash = (INT32)((zparam >> 16) & 0xFFFF);

	switch (state) {
	case MOTOR_ROTATE_FWD:
		fmove_step = fsteps + fgear_backlash;
		zmove_step = zsteps + zgear_backlash;

		if (fmove_step != 0) {
			pmotor_info->motor_config[0].move_dir = MOVE_DIR_FWD;
		}
		if (zmove_step != 0) {
			pmotor_info->motor_config[1].move_dir = MOVE_DIR_FWD;
		}

		mdrv_set_position(id, fmove_step, zmove_step);

		pmotor_info->motor_config[0].curr_position += fsteps;
		pmotor_info->motor_config[1].curr_position += zsteps;
		break;

	case MOTOR_ROTATE_BWD:
		fmove_step = fsteps + fgear_backlash;
		zmove_step = zsteps + zgear_backlash;

		if (fmove_step != 0) {
			pmotor_info->motor_config[0].move_dir = MOVE_DIR_BWD;
		}
		if (zmove_step != 0) {
			pmotor_info->motor_config[1].move_dir = MOVE_DIR_BWD;
		}

		mdrv_set_position(id, -fmove_step, -zmove_step);

		pmotor_info->motor_config[0].curr_position -= fsteps;
		pmotor_info->motor_config[1].curr_position -= zsteps;
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}

	return ret_state;
}
#endif

static UINT32 mdrv_get_zoom_speed(UINT32 id)
{
	UINT32 zoom_spd;

	switch ((UINT32)mdrv_get_motor_info(id)->motor_config[1].move_speed) {
	case MTR_SPEED_QUARTER:
		zoom_spd = 20;
		break;

	case MTR_SPEED_HALF:
		zoom_spd = 40;
		break;

	case MTR_SPEED_1X:
	default:
		zoom_spd = 60;
		break;

	case MTR_SPEED_2X:
		zoom_spd = 80;
		break;

	case MTR_SPEED_4X:
		zoom_spd = 100;
		break;
	}

	return zoom_spd;
}

static void mdrv_set_zoom_speed(UINT32 id, UINT32 speed)
{
	UINT32 i, tmp, idx = 1;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	speed = MTR_CLAMP(speed, MTR_SPEED_FULL/5, MTR_SPEED_FULL);
	tmp = (speed / (MTR_SPEED_FULL / 5)) - 1;

	if (tmp != 0) {
		for (i = 0; i < tmp; i++) {
			idx *= 2;
		}
	}

	pmotor_info->motor_config[1].move_speed = (MTR_MOTOR_SPEED)idx;

	tmp = 120000000 / 100 / (pmotor_info->motor_config[1].pwm_freq * pmotor_info->motor_config[1].move_speed / MTR_SPEED_1X);
	pmotor_info->motor_config[1].pwm_clk_div = MTR_CLAMP(tmp, 60, 2000);
}

static BOOL mdrv_set_zoom_move(UINT32 id, MOTOR_ROTATE_DIR state, UINT32 param)
{
	BOOL ret_state = TRUE;
	INT32 steps, gear_backlash, move_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	steps = (INT32)(param & 0xFFFF);
	gear_backlash = (INT32)((param >> 16) & 0xFFFF);

	switch (state) {
	case MOTOR_ROTATE_FWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[1].move_dir = MOVE_DIR_FWD;
		}

		mdrv_set_single_position(id, MOTOR_ZOOM, move_step);

		pmotor_info->motor_config[1].curr_position += steps;
		break;

	case MOTOR_ROTATE_BWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[1].move_dir = MOVE_DIR_BWD;
		}

		mdrv_set_single_position(id, MOTOR_ZOOM, -move_step);

		pmotor_info->motor_config[1].curr_position -= steps;
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}

	return ret_state;
}

static INT32 mdrv_set_zoom_pos(UINT32 id, MOTOR_DRV_INFO *pmotor_info, INT32 target_pos)
{
#if !ZOOM_FOCUS_SIM_MOVE
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step, acc_step = 0;

	if (target_pos > pmotor_info->motor_config[1].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[1].gear_backlash != 0) && (pmotor_info->motor_config[1].move_dir != MOVE_DIR_FWD)) {
			rest_step = pmotor_info->motor_config[1].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
						  ? pmotor_info->motor_config[1].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = target_pos - pmotor_info->motor_config[1].curr_position;

		do {
			set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
					  ? pmotor_info->motor_config[1].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;
	
			if ((rest_step <= 0) || (pmotor_info->motor_config[1].set_move_stop == TRUE)) {
				rest_step = 0;
			}

			mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	} else if (target_pos < pmotor_info->motor_config[1].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[1].gear_backlash != 0) && (pmotor_info->motor_config[1].move_dir != MOVE_DIR_BWD)) {
			rest_step = pmotor_info->motor_config[1].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
						  ? pmotor_info->motor_config[1].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = pmotor_info->motor_config[1].curr_position - target_pos;

		do {
			set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
					  ? pmotor_info->motor_config[1].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;

			if ((rest_step <= 0) || (pmotor_info->motor_config[1].set_move_stop == TRUE)) {
				rest_step = 0;
			}

			mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, cmd_param);
		} while (rest_step > 0);
	}
#else
	UINT32 zcmd_param = 0, fcmd_param = 0;
	INT32 zset_step, fset_step, zrest_step, frest_step, acc_step = 0;
	INT32 focus_tar_pos, focus_ofst, near_bound = 0, inf_bound = 1024, zstep_size;

	mdrv_calc_focus_range(id, &near_bound, &inf_bound, -1, TRUE);

	focus_ofst = MTR_CLAMP((pmotor_info->motor_config[0].curr_position - near_bound), 0, (inf_bound - near_bound));
	zstep_size = 5;

	if (target_pos > pmotor_info->motor_config[1].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[1].gear_backlash != 0) || (pmotor_info->motor_config[0].gear_backlash != 0)) {
			zrest_step = (pmotor_info->motor_config[1].move_dir != MOVE_DIR_FWD) ? pmotor_info->motor_config[1].gear_backlash : 0;
			frest_step = (pmotor_info->motor_config[0].move_dir != MOVE_DIR_FWD) ? pmotor_info->motor_config[0].gear_backlash : 0;

			do {
				zset_step = (zrest_step > pmotor_info->motor_config[1].max_step_size)
						  ? pmotor_info->motor_config[1].max_step_size : zrest_step;
				fset_step = (frest_step > pmotor_info->motor_config[0].max_step_size)
						  ? pmotor_info->motor_config[0].max_step_size : frest_step;
				zrest_step = (zrest_step != 0) ? (zrest_step - zset_step) : 0;
				frest_step = (frest_step != 0) ? (frest_step - fset_step) : 0;

				zcmd_param = (zset_step << 16) + 0;
				fcmd_param = (fset_step << 16) + 0;

				mdrv_set_move(id, MOTOR_ROTATE_FWD, fcmd_param, zcmd_param);
			} while ((zrest_step > 0) || (frest_step > 0));
		}

		zrest_step = target_pos - pmotor_info->motor_config[1].curr_position;

		do {
			zset_step = (zrest_step > zstep_size) ? zstep_size : zrest_step;
			zcmd_param = (0 << 16) + zset_step;
			zrest_step -= zset_step;
			acc_step += zset_step;
	
			if ((zrest_step <= 0) || (pmotor_info->motor_config[1].set_move_stop == TRUE)) {
				zrest_step = 0;
			}

			mdrv_calc_focus_range(id, &near_bound, &inf_bound, (pmotor_info->motor_config[1].curr_position + zset_step), TRUE);
			focus_tar_pos = MTR_CLAMP((near_bound + focus_ofst), near_bound, inf_bound);
			fset_step = (focus_tar_pos >= pmotor_info->motor_config[0].curr_position) ? MTR_MIN((focus_tar_pos - pmotor_info->motor_config[0].curr_position), pmotor_info->motor_config[0].max_step_size) : 0;
			fcmd_param = (0 << 16) + fset_step;

			mdrv_set_move(id, MOTOR_ROTATE_FWD, fcmd_param, zcmd_param);
		} while (zrest_step > 0);
	} else if (target_pos < pmotor_info->motor_config[1].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[1].gear_backlash != 0) || (pmotor_info->motor_config[0].gear_backlash != 0)) {
			zrest_step = (pmotor_info->motor_config[1].move_dir != MOVE_DIR_BWD) ? pmotor_info->motor_config[1].gear_backlash : 0;
			frest_step = (pmotor_info->motor_config[0].move_dir != MOVE_DIR_BWD) ? pmotor_info->motor_config[0].gear_backlash : 0;

			do {
				zset_step = (zrest_step > pmotor_info->motor_config[1].max_step_size)
						  ? pmotor_info->motor_config[1].max_step_size : zrest_step;
				fset_step = (frest_step > pmotor_info->motor_config[0].max_step_size)
						  ? pmotor_info->motor_config[0].max_step_size : frest_step;
				zrest_step = (zrest_step != 0) ? (zrest_step - zset_step) : 0;
				frest_step = (frest_step != 0) ? (frest_step - fset_step) : 0;

				zcmd_param = (zset_step << 16) + 0;
				fcmd_param = (fset_step << 16) + 0;

				mdrv_set_move(id, MOTOR_ROTATE_BWD, fcmd_param, zcmd_param);
			} while ((zrest_step > 0) || (frest_step > 0));
		}

		zrest_step = pmotor_info->motor_config[1].curr_position - target_pos;

		do {
			zset_step = (zrest_step > zstep_size) ? zstep_size : zrest_step;
			zcmd_param = (0 << 16) + zset_step;
			zrest_step -= zset_step;
			acc_step += zset_step;

			if ((zrest_step <= 0) || (pmotor_info->motor_config[1].set_move_stop == TRUE)) {
				zrest_step = 0;
			}

			mdrv_calc_focus_range(id, &near_bound, &inf_bound, (pmotor_info->motor_config[1].curr_position - zset_step), TRUE);
			focus_tar_pos = MTR_CLAMP((near_bound + focus_ofst), near_bound, inf_bound);
			fset_step = (pmotor_info->motor_config[0].curr_position >= focus_tar_pos) ? MTR_MIN((pmotor_info->motor_config[0].curr_position - focus_tar_pos), pmotor_info->motor_config[0].max_step_size) : 0;
			fcmd_param = (0 << 16) + fset_step;

			mdrv_set_move(id, MOTOR_ROTATE_BWD, fcmd_param, zcmd_param);
		} while (zrest_step > 0);
	}
#endif

	return acc_step;
}

static UINT32 mdrv_search_zoom_section(UINT32 id)
{
	UINT32 sec_pos;
	INT32 sec_zone_pts;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	for (sec_pos = ZOOM_SECTION_WIDE; sec_pos < ZOOM_SECTION_TELE; sec_pos++) {
		sec_zone_pts = (pmotor_info->zoom_focus_tab[sec_pos].zoom_position + pmotor_info->zoom_focus_tab[sec_pos+1].zoom_position) / 2;

		if (pmotor_info->motor_config[1].curr_position <= sec_zone_pts) {
			break;
		}
	}

	sec_pos = MTR_CLAMP(sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	return sec_pos;
}

static void mdrv_upd_focus_pos(UINT32 id, MOTOR_DRV_INFO *pmotor_info)
{
	INT32 focus_tar_pos, near_bound = 0, inf_bound = 1024;

	mdrv_calc_focus_range(id, &near_bound, &inf_bound, -1, TRUE);

	if ((pmotor_info->motor_config[0].curr_position < near_bound) || (pmotor_info->motor_config[0].curr_position > inf_bound)) {
		focus_tar_pos = (near_bound + inf_bound) / 2;

		mdrv_set_focus_pos(id, pmotor_info, focus_tar_pos, TRUE);
	}
}

INT32 ti8833_init_zoom(UINT32 id)
{
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	rest_step = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].zoom_position + pmotor_info->motor_config[1].init_pos_ofst
			  + (pmotor_info->motor_config[1].max_move_distance / 10);

	do {
		set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
				  ? pmotor_info->motor_config[1].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_ZOOM_GOHOME_DIR
		mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// increase go home consistency
	vos_util_delay_ms(500);

	// tighten motor and gear
	rest_step = pmotor_info->motor_config[1].max_move_distance / 30;

	do {
		set_step = (rest_step > pmotor_info->motor_config[1].max_step_size) ? pmotor_info->motor_config[1].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_ZOOM_GOHOME_DIR
		mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// compensate backlash first
	if ((pmotor_info->motor_config[1].gear_backlash != 0) && (pmotor_info->motor_config[1].move_dir != MOVE_DIR_FWD)) {
		rest_step = pmotor_info->motor_config[1].gear_backlash;

		do {
			set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
					 ? pmotor_info->motor_config[1].max_step_size : rest_step;
			cmd_param = (set_step << 16) + 0;
			rest_step -= set_step;

			mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	}

#if REVERSE_ZOOM_GOHOME_DIR
	rest_step = MTR_MAX(pmotor_info->motor_config[1].init_pos_ofst+pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].zoom_position, 0);
#else
	rest_step = MTR_MAX(pmotor_info->motor_config[1].init_pos_ofst+pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position, 0);
#endif

	do {
		set_step = (rest_step > pmotor_info->motor_config[1].max_step_size)
				 ? pmotor_info->motor_config[1].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_ZOOM_GOHOME_DIR
		mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, cmd_param);
#else
		mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, cmd_param);
#endif
	} while (rest_step > 0);

	pmotor_info->motor_config[1].curr_position = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position;

	mdrv_upd_focus_pos(id, pmotor_info);

	pmotor_info->motor_config[1].move_2_home = TRUE;

	return E_OK;
}

UINT32 ti8833_get_zoom_speed(UINT32 id)
{
	return mdrv_get_zoom_speed(id);
}

void ti8833_set_zoom_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category)
{
	UINT32 curr_spd, zoom_spd = 20;

	curr_spd = mdrv_get_zoom_speed(id);

	switch ((UINT32)spd_category) {
	case MOTOR_SPEED_VERY_LOW:
		zoom_spd = 20;
		break;

	case MOTOR_SPEED_LOW:
		zoom_spd = 40;
		break;

	case MOTOR_SPEED_MEDIUM:
		zoom_spd = 60;
		break;

	case MOTOR_SPEED_HIGH:
		zoom_spd = 80;
		break;

	case MOTOR_SPEED_VERY_HIGH:
		zoom_spd = 100;
		break;
	}

	if (curr_spd != zoom_spd) {
		mdrv_set_zoom_speed(id, zoom_spd);
	} else {
		DBG_WRN("set the same speed!\r\n");
	}
}

void ti8833_assign_zoom_position(UINT32 id, INT32 zoom_pos, INT32 dir)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_info->motor_config[1].curr_position = zoom_pos;
	pmotor_info->motor_config[1].move_dir = (dir >= 1) ? MOVE_DIR_FWD : MOVE_DIR_BWD;

	pmotor_info->motor_config[1].move_2_home = TRUE;
}

UINT32 ti8833_get_zoom_section_pos(UINT32 id)
{
	return mdrv_search_zoom_section(id);
}

UINT32 ti8833_get_zoom_max_section_pos(UINT32 id)
{
	return ZOOM_SECTION_TELE;
}

INT32 ti8833_set_zoom_section_pos(UINT32 id, UINT32 zoom_sec_pos)
{
	INT32 tar_pos, acc_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (pmotor_info->motor_config[1].move_2_home == FALSE) {
		DBG_ERR("not initialized yet!\r\n");

		return 0;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_MOVE) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return 0;
	}

	zoom_sec_pos = MTR_CLAMP(zoom_sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	tar_pos = pmotor_info->zoom_focus_tab[zoom_sec_pos].zoom_position;

	if (tar_pos != pmotor_info->motor_config[1].curr_position) {
		pmotor_info->motor_config[1].set_move_stop = FALSE;

		acc_step = mdrv_set_zoom_pos(id, pmotor_info, tar_pos);

		pmotor_info->motor_config[1].section_pos = zoom_sec_pos;

		mdrv_upd_focus_pos(id, pmotor_info);
	} else {
		DBG_WRN("set the same pos!\r\n");
	}

	vos_sem_sig(SEMID_MTR_MOVE);

	return acc_step;
}

INT32 ti8833_get_zoom_position(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (pmotor_info->motor_config[1].move_dir << 16) | pmotor_info->motor_config[1].curr_position;
}

INT32 ti8833_set_zoom_position(UINT32 id, INT32 zoom_pos)
{
	INT32 acc_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (pmotor_info->motor_config[1].move_2_home == FALSE) {
		DBG_ERR("not initialized yet!\r\n");

		return 0;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_MOVE) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return 0;
	}

	if ((zoom_pos >= pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position) && (zoom_pos <= pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].zoom_position)) {
		if (zoom_pos != pmotor_info->motor_config[1].curr_position) {
			pmotor_info->motor_config[1].set_move_stop = FALSE;

			acc_step = mdrv_set_zoom_pos(id, pmotor_info, zoom_pos);

			pmotor_info->motor_config[1].section_pos = mdrv_search_zoom_section(id);

			mdrv_upd_focus_pos(id, pmotor_info);
		} else {
			DBG_WRN("step=0!\r\n");
		}
	} else {
		DBG_WRN("set out of range pos!\r\n");
	}

	vos_sem_sig(SEMID_MTR_MOVE);

	return acc_step;
}

INT32 ti8833_press_zoom_move(UINT32 id, BOOL move_dir)
{
	INT32 target_pos;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (move_dir == FALSE) {    // ZOOM_OUT = 0
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position;
	} else {    // ZOOM_IN = 1
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].zoom_position;
	}

	ti8833_set_zoom_position(id, target_pos);

	return E_OK;
}

void ti8833_release_zoom_move(UINT32 id)
{
	mdrv_get_motor_info(id)->motor_config[1].set_move_stop = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// IR cut
//----------------------------------------------------------------------------------------------------------------------
UINT32 ti8833_get_ircut_state(UINT32 id)
{
	return mdrv_get_motor_info(id)->ircut_state;
}

void ti8833_set_ircut_state(UINT32 id, IRCUT_ACT state, UINT32 param)
{

}


//----------------------------------------------------------------------------------------------------------------------
// aperture
//----------------------------------------------------------------------------------------------------------------------
#if P_IRIS_FUNC_ENABLE
static BOOL mdrv_set_aperture_move(UINT32 id, MOTOR_ROTATE_DIR state, UINT32 param)
{
	BOOL ret_state = TRUE;
	INT32 steps, gear_backlash, move_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	steps = (INT32)(param & 0xFFFF);
	gear_backlash = (INT32)((param >> 16) & 0xFFFF);

	switch ((UINT32)state) {
	case MOTOR_ROTATE_FWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[2].move_dir = MOVE_DIR_FWD;
		}

		mdrv_set_single_position(id, MOTOR_IRIS, move_step);

		pmotor_info->motor_config[2].curr_position += steps;
		break;

	case MOTOR_ROTATE_BWD:
		move_step = steps + gear_backlash;

		if (move_step != 0) {
			pmotor_info->motor_config[2].move_dir = MOVE_DIR_BWD;
		}

		mdrv_set_single_position(id, MOTOR_IRIS, -move_step);

		pmotor_info->motor_config[2].curr_position -= steps;
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}

	return ret_state;
}

static INT32 mdrv_set_aperture_pos(UINT32 id, INT32 target_pos)
{
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step, acc_step = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (target_pos > pmotor_info->motor_config[2].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[2].gear_backlash != 0) && (pmotor_info->motor_config[2].move_dir != MOVE_DIR_FWD)) {
			rest_step = pmotor_info->motor_config[2].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
						  ? pmotor_info->motor_config[2].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = target_pos - pmotor_info->motor_config[2].curr_position;

		do {
			set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
					  ? pmotor_info->motor_config[2].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;
	
			if (rest_step <= 0) {
				rest_step = 0;
			}

			mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	} else if (target_pos < pmotor_info->motor_config[2].curr_position) {
		// compensate backlash first
		if ((pmotor_info->motor_config[2].gear_backlash != 0) && (pmotor_info->motor_config[2].move_dir != MOVE_DIR_BWD)) {
			rest_step = pmotor_info->motor_config[2].gear_backlash;

			do {
				set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
						  ? pmotor_info->motor_config[2].max_step_size : rest_step;
				cmd_param = (set_step << 16) + 0;
				rest_step -= set_step;
				mdrv_set_aperture_move(id, MOTOR_ROTATE_BWD, cmd_param);
			} while (rest_step > 0);
		}

		rest_step = pmotor_info->motor_config[2].curr_position - target_pos;

		do {
			set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
					  ? pmotor_info->motor_config[2].max_step_size : rest_step;
			cmd_param = (0 << 16) + set_step;
			rest_step -= set_step;
			acc_step += set_step;

			if (rest_step <= 0) {
				rest_step = 0;
			}

			mdrv_set_aperture_move(id, MOTOR_ROTATE_BWD, cmd_param);
		} while (rest_step > 0);
	}

	return acc_step;
}
#endif

void ti8833_init_aperture(UINT32 id)
{
#if P_IRIS_FUNC_ENABLE
	UINT32 cmd_param = 0;
	INT32 set_step, rest_step;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	rest_step = pmotor_info->motor_config[2].max_move_distance + pmotor_info->motor_config[2].init_pos_ofst + (pmotor_info->motor_config[2].max_move_distance / 10);

	do {
		set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
				  ? pmotor_info->motor_config[2].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_IRIS_GOHOME_DIR
		mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_aperture_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// increase go home consistency
	vos_util_delay_ms(500);

	// tighten motor and gear
	rest_step = pmotor_info->motor_config[2].max_move_distance / 30;

	do {
		set_step = (rest_step > pmotor_info->motor_config[2].max_step_size) ? pmotor_info->motor_config[2].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_IRIS_GOHOME_DIR
		mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
#else
		mdrv_set_aperture_move(id, MOTOR_ROTATE_BWD, cmd_param);
#endif
	} while (rest_step > 0);

	// compensate backlash first
	if ((pmotor_info->motor_config[2].gear_backlash != 0) && (pmotor_info->motor_config[2].move_dir != MOVE_DIR_FWD)) {
		rest_step = pmotor_info->motor_config[2].gear_backlash;

		do {
			set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
					 ? pmotor_info->motor_config[2].max_step_size : rest_step;
			cmd_param = (set_step << 16) + 0;
			rest_step -= set_step;

			mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
		} while (rest_step > 0);
	}

	rest_step = MTR_MAX(pmotor_info->motor_config[2].init_pos_ofst, 0);

	do {
		set_step = (rest_step > pmotor_info->motor_config[2].max_step_size)
				  ? pmotor_info->motor_config[2].max_step_size : rest_step;
		cmd_param = (0 << 16) + set_step;
		rest_step -= set_step;

#if REVERSE_IRIS_GOHOME_DIR
		mdrv_set_aperture_move(id, MOTOR_ROTATE_BWD, cmd_param);
#else
		mdrv_set_aperture_move(id, MOTOR_ROTATE_FWD, cmd_param);
#endif
	} while (rest_step > 0);

	pmotor_info->motor_config[2].curr_position = 0;
	pmotor_info->motor_config[2].move_2_home = TRUE;
#endif
}

UINT32 ti8833_get_aperture_fno(UINT32 id, UINT32 zoom_sec_pos, UINT32 iris_pos)
{
	return 0;
}

APERTURE_SEC_POS ti8833_get_aperture_section_pos(UINT32 id)
{
	return mdrv_get_motor_info(id)->aperture_sec_pos;
}

void ti8833_set_aperture_section_pos(UINT32 id, APERTURE_SEC_POS iris_sec_pos)
{
#if P_IRIS_FUNC_ENABLE
	iris_sec_pos = MTR_MIN(iris_sec_pos, APERTURE_POS_SMALL);

	ti8833_set_aperture_position(id, g_aperture_2_target_pos[iris_sec_pos]);

	mdrv_get_motor_info(id)->aperture_sec_pos = iris_sec_pos;
#endif
}

UINT32 ti8833_get_aperture_position(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (pmotor_info->motor_config[2].move_dir << 16) | pmotor_info->motor_config[2].curr_position;
}

void ti8833_set_aperture_position(UINT32 id, UINT32 iris_pos)
{
#if P_IRIS_FUNC_ENABLE
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (pmotor_info->motor_config[2].move_2_home == FALSE) {
		DBG_ERR("not initialized yet!\r\n");

		return;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_MOVE) != E_OK) {
		DBG_ERR("wait semaphore fail!\r\n");

		return;
	}

	if ((iris_pos >= 0) && (iris_pos <= pmotor_info->motor_config[2].max_move_distance)) {
		if (iris_pos != pmotor_info->motor_config[2].curr_position) {
			pmotor_info->motor_config[2].set_move_stop = FALSE;

			mdrv_set_aperture_pos(id, iris_pos);
		} else {
			DBG_WRN("step=0!\r\n");
		}
	} else {
		DBG_WRN("set out of range pos!\r\n");
	}

	vos_sem_sig(SEMID_MTR_MOVE);
#endif
}


//----------------------------------------------------------------------------------------------------------------------
// shutter
//----------------------------------------------------------------------------------------------------------------------
SHUTTER_ACT ti8833_get_shutter_state(UINT32 id)
{
	return mdrv_get_motor_info(id)->shutter_state;
}

void ti8833_set_shutter_state(UINT32 id, SHUTTER_ACT state, UINT32 param)
{

}


//----------------------------------------------------------------------------------------------------------------------
// miscellaneous
//----------------------------------------------------------------------------------------------------------------------
UINT32 ti8833_get_capability_info(UINT32 id)
{
	if (id == 0) {
		return (LENS_SUPPORT_APERTURE | LENS_SUPPORT_ZOOM | LENS_SUPPORT_FOCUS);
	} else {    // id == 1
		return 0;
	}
}

UINT32 ti8833_get_busy_status(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (UINT32)((pmotor_info->motor_config[2].work_status << 2) | (pmotor_info->motor_config[1].work_status << 1) | pmotor_info->motor_config[0].work_status);
}

UINT32 ti8833_get_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(zf_rtab, &pmotor_info->zoom_focus_tab[tab_idx].zoom_position, sizeof(LENS_ZOOM_FOCUS_TAB));

	return tab_idx;
}

UINT32 ti8833_set_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(&pmotor_info->zoom_focus_tab[tab_idx].zoom_position, zf_rtab, sizeof(LENS_ZOOM_FOCUS_TAB));

	return tab_idx;
}

UINT32 ti8833_get_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(zf_ctab, &pmotor_info->focus_distance_tab[tab_idx].lens_position[0], sizeof(LENS_FOCUS_DISTANCE_TAB));

	return tab_idx;
}

UINT32 ti8833_set_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(&pmotor_info->focus_distance_tab[tab_idx].lens_position[0], zf_ctab, sizeof(LENS_FOCUS_DISTANCE_TAB));

	return tab_idx;
}

UINT32 ti8833_get_signal_state(UINT32 id, MOTOR_SIGNAL signal)
{
	switch (signal) {
	case MOTOR_SIGNAL_FPI:
		break;

	case MOTOR_SIGNAL_ZPI:
		break;

	case MOTOR_SIGNAL_LED:
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}

	return 0;
}

void ti8833_set_signal_state(UINT32 id, MOTOR_SIGNAL signal, UINT32 state)
{
	switch (signal) {
	case MOTOR_SIGNAL_FPI:
		break;

	case MOTOR_SIGNAL_ZPI:
		break;

	case MOTOR_SIGNAL_LED:
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}
}

void ti8833_get_fz_backlash_value(UINT32 id, INT32 *focus_backlash, INT32 *zoom_backlash)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	*focus_backlash = pmotor_info->motor_config[0].gear_backlash;
	*zoom_backlash = pmotor_info->motor_config[1].gear_backlash;
}

void ti8833_set_fz_backlash_value(UINT32 id, INT32 focus_backlash, INT32 zoom_backlash)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (focus_backlash != -1) {
		if (focus_backlash >= 0) {
			pmotor_info->motor_config[0].gear_backlash = focus_backlash;
		} else {
			DBG_WRN("not allowable focus backlash!\r\n");
		}
	}

	if (zoom_backlash != -1) {
		if (zoom_backlash >= 0) {
			pmotor_info->motor_config[1].gear_backlash = zoom_backlash;
		} else {
			DBG_WRN("not allowable zoom backlash!\r\n");
		}
	}
}

