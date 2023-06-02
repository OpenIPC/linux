
#include "mtr_drv_an41908.h"


//=============================================================================
// global variable
//=============================================================================
static ID SEMID_MTR_SPI, SEMID_MTR_MOVE;

const UINT32 g_f_number_mapping[] = { 16, 20, 24, 28, 34, 40, 48, 56, 68, 96, 110, 140, 160, 190, 220, 250 };

const UINT32 g_focus_distance_tab[FOCUS_DISTANCE_TABLE_NUM] = { 5, 10, 15, 20, 30, 50, 70, 100, 140, 200, 300, 500, 9999 };    // unit: cm

const UINT8 g_reg_address_ofst[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0E, 0x20, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x27, 0x28, 0x29, 0x2A,
	0x2C
};

// following table is based on 27MHz source clock to AN41908
const UINT32 g_pwm_freq_tab[][12] = {{ 27200,  30100,  40200,  54400,  64900,  76700, 108900, 129800, 160700, 210900, 306800, 562500},    // unit: Hz
									 {0x5F00, 0x5C00, 0x5500, 0x3F00, 0x3A00, 0x3600, 0x1F00, 0x1A00, 0x1500, 0x1000, 0x0B00, 0x0600}};

#define MAX_PWM_TAB_SIZE    sizeof(g_pwm_freq_tab)/sizeof(UINT32)/2

#define APERTURE_INIT_DAC    0x0064

const UINT32 g_aperture_2_dac_target[] = { APERTURE_INIT_DAC, 0x00A0, 0x00DC, 0x010E, 0x0145, 0x017C, 0x01AF, 0x01E5,
					   0x021C, 0x0251, 0x0288, 0x02BC, 0x02F0, 0x032A, 0x035C };    // *

static MOTOR_DRV_INFO g_lens_drv_info[MTR_ID_MAX] = {
	{
	.motor_drv_reg.REG[0]  = APERTURE_INIT_DAC,    // open IRIS to initial pos
	.motor_drv_reg.REG[1]  = 0xA000,
	.motor_drv_reg.REG[2]  = 0x0300,
	.motor_drv_reg.REG[3]  = 0x0210,
	.motor_drv_reg.REG[4]  = 0x6048,
	.motor_drv_reg.REG[5]  = 0x0328,
	.motor_drv_reg.REG[6]  = 0x0000,
	.motor_drv_reg.REG[7]  = 0x0000,
	.motor_drv_reg.REG[8]  = 0x0000,
	.motor_drv_reg.REG[9]  = 0x0000,
	.motor_drv_reg.REG[10] = 0x0000,
	.motor_drv_reg.REG[11] = 0x0080,    // disable IRIS function block (bit10)
	.motor_drv_reg.REG[12] = 0x0000,
	.motor_drv_reg.REG[13] = 0x1200,
#if USE_GPIO_SIMULATION_SPI
	.motor_drv_reg.REG[14] = 0x5508,    // set SPI communication time interval to 2427 usec = 8*303.4 usec
#else
	.motor_drv_reg.REG[14] = 0x5501,    // set SPI communication time interval to 303.4 usec = 1*303.4 usec
#endif
	.motor_drv_reg.REG[15] = 0x0087,
	.motor_drv_reg.REG[16] = 0x1601,    // set start excitation time to 303.4 usec = 1*303.4 usec
	.motor_drv_reg.REG[17] = STANDBY_PWR_PULSE_WIDTH,
	.motor_drv_reg.REG[18] = 0x0000,
	.motor_drv_reg.REG[19] = 0x009D,
	.motor_drv_reg.REG[20] = 0x1601,
	.motor_drv_reg.REG[21] = STANDBY_PWR_PULSE_WIDTH,
	.motor_drv_reg.REG[22] = 0x0000,
	.motor_drv_reg.REG[23] = 0x0139,
	.motor_drv_reg.REG[24] = 0x0004,

	.motor_config[0].chnel_sel         = 0,                            // *
	.motor_config[0].gear_ratio        = 16,                           // *
	.motor_config[0].excite_enable     = 0x0400,
	.motor_config[0].def_positive_dir  = 1,                            // *
	.motor_config[0].pwm_freq          = 4500,                         // *
	.motor_config[0].work_status       = MOTOR_STATUS_FREE,
	.motor_config[0].motor_status      = MOTOR_STATUS_FREE,
	.motor_config[0].max_step_size     = 15,
	.motor_config[0].move_2_home       = FALSE,
	.motor_config[0].curr_position     = 0,
	.motor_config[0].section_pos       = 0,
	.motor_config[0].move_dir          = MOVE_DIR_INIT,
	.motor_config[0].move_speed        = MTR_SPEED_2X,
	.motor_config[0].gear_backlash     = 0,                            // *
	.motor_config[0].max_pulse_width   = STANDBY_PWR_PULSE_WIDTH,
	.motor_config[0].dly_time          = MOVE_LOOP_FDELAY_TIME,        // *
	.motor_config[0].set_move_stop     = FALSE,
	.motor_config[0].init_pos_ofst     = 0,                            // *
	.motor_config[0].max_move_distance = 1520,                         // *

	.motor_config[1].chnel_sel         = 1,                            // *
	.motor_config[1].gear_ratio        = 16,                           // *
	.motor_config[1].excite_enable     = 0x0400,
	.motor_config[1].def_positive_dir  = 1,                            // *
	.motor_config[1].pwm_freq          = 4500,                         // *
	.motor_config[1].work_status       = MOTOR_STATUS_FREE,
	.motor_config[1].motor_status      = MOTOR_STATUS_FREE,
	.motor_config[1].max_step_size     = 15,
	.motor_config[1].move_2_home       = FALSE,
	.motor_config[1].curr_position     = 0,
	.motor_config[1].section_pos       = ZOOM_SECTION_WIDE,
	.motor_config[1].move_dir          = MOVE_DIR_INIT,
	.motor_config[1].move_speed        = MTR_SPEED_1X,
	.motor_config[1].gear_backlash     = 0,                            // *
	.motor_config[1].max_pulse_width   = STANDBY_PWR_PULSE_WIDTH,
	.motor_config[1].dly_time          = MOVE_LOOP_ZDELAY_TIME,        // *
	.motor_config[1].set_move_stop     = FALSE,
	.motor_config[1].init_pos_ofst     = 0,                            // *
	.motor_config[1].max_move_distance = 600,                          // *

	.ircut_state       = IRCUT_OPEN,
	.aperture_sec_pos  = APERTURE_POS_BIG,
	.aperture_position = APERTURE_INIT_DAC / 5,
	.shutter_state     = SHUTTER_OPEN,

	.lens_proty.focal_len_min = 2800,    // unit: um
	.lens_proty.focal_len_max = 12000,   // unit: um

	.lens_hw.protocol_type = 2,    // 0:SPI / 1:I2C / 2:GPIO

	.lens_hw.spi_cfg.spi_chnel   = 2,
	.lens_hw.spi_cfg.spi_mode    = 3,
	.lens_hw.spi_cfg.spi_pkt_len = 24,
	.lens_hw.spi_cfg.spi_freq    = 4000000,
	.lens_hw.spi_cfg.spi_order   = FALSE,
	.lens_hw.spi_cfg.cs_polarity = TRUE,
	.lens_hw.spi_cfg.spi_pkt_dly = 0,

	.lens_hw.gpio_spi_cfg.gpio_spi_cs  = 0,
	.lens_hw.gpio_spi_cfg.gpio_spi_clk = 0,
	.lens_hw.gpio_spi_cfg.gpio_spi_tx  = 0,
	.lens_hw.gpio_spi_cfg.gpio_spi_rx  = 0,

	.lens_hw.pin_def = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	.zoom_focus_tab[0] =  { 0,      0,  260  },     // * 0
	.zoom_focus_tab[1] =  { 16,     6,  326  },     // * 1
	.zoom_focus_tab[2] =  { 50,   233,  553  },     // * 2
	.zoom_focus_tab[3] =  { 100,  460,  780  },     // * 3
	.zoom_focus_tab[4] =  { 160,  640,  960  },     // * 4
	.zoom_focus_tab[5] =  { 220,  820, 1140  },     // * 5
	.zoom_focus_tab[6] =  { 290, 1000, 1320  },     // * 6
	.zoom_focus_tab[7] =  { 360, 1050, 1370  },     // * 7
	.zoom_focus_tab[8] =  { 440, 1100, 1420  },     // * 8
	.zoom_focus_tab[9] =  { 520, 1150, 1470  },     // * 9
	.zoom_focus_tab[10] = { 600, 1200, 1520  },     // * 10

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


//=============================================================================
// routine
//=============================================================================
MOTOR_DRV_INFO* mdrv_get_motor_info(UINT32 id)
{
	id = MTR_MIN(id, MTR_ID_MAX-1);

	return &g_lens_drv_info[id];
}

static INT32 mdrv_init_spi(UINT32 id)
{
	static BOOL init_already = FALSE;
	INT32 ret_sts = 0;
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
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_FZ] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], "MTR_FZ")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IS] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_IS], "MTR_IS")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS], "MTR_FSTS")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS], "MTR_ZSTS")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], "MTR_IRC0")) < 0) {
			return ret_sts;
		}
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] != 0) {
		if ((ret_sts = gpio_request(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1], "MTR_IRC1")) < 0) {
			return ret_sts;
		}
	}

	#if USE_GPIO_SIMULATION_SPI
	if ((pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs == 0) || (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk == 0) || (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx == 0)) {
		return -1;
	}

	if ((ret_sts = gpio_request(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, "SPI_CS")) < 0) {
		return ret_sts;
	}
	if ((ret_sts = gpio_request(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, "SPI_CLK")) < 0) {
		return ret_sts;
	}
	if ((ret_sts = gpio_request(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, "SPI_TXD")) < 0) {
		return ret_sts;
	}
	if ((ret_sts = gpio_request(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx, "SPI_RXD")) < 0) {
		return ret_sts;
	}
	#else
	if (pmotor_info->pspi_device == NULL) {
		DBG_ERR("pspi_device is NULL!\n");

		return -1;
	}
	#endif
#endif

	if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN], 0);
	}
	gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], 0);
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IS] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_IS], 0);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS] != 0) {
		gpio_direction_input(pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS] != 0) {
		gpio_direction_input(pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], 0);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] != 0) {
		gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1], 0);
	}

#if USE_GPIO_SIMULATION_SPI
	gpio_direction_output(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, 0);
	gpio_direction_output(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	gpio_direction_output(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 0);
	gpio_direction_input(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx);
#endif

	init_already = TRUE;

	return ret_sts;
}

#if USE_GPIO_SIMULATION_SPI
static void mdrv_spi_write_packet(UINT32 spi_data)
{
	UINT32 i;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(0);

	// set CS high => send data => set CS low
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, 1);

	for (i = 0; i < 6; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);

		if ((spi_data >> i) & 0x01) {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 1);
		} else {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 0);
		}

		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	}
	for (i = 0; i < 2; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 0);
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	}

	for (i = 8; i < 24; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);

		if ((spi_data >> i) & 0x01) {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 1);
		} else {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 0);
		}

		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	}

	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, 0);
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
}
#endif

static void mdrv_write_2_spi(UINT32 id, VD_SIGNAL func_type, UINT32 *pvalue, UINT32 data_cnt)
{
	UINT32 i;
	VOS_TICK curr_stamp, time_diff;
	static VOS_TICK last_stamp = 0;
#if USE_GPIO_SIMULATION_SPI
	unsigned long flags;
#endif
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	vos_perf_mark(&curr_stamp);
	time_diff = vos_perf_duration(last_stamp, curr_stamp);

	// separate 2 SPI cmds depending on 0x20[7:0] + 0x22[7:0]
	while (time_diff < 2000) {
		vos_util_delay_us_polling(200);    // vos_util_delay_us
		vos_perf_mark(&curr_stamp);
		time_diff = vos_perf_duration(last_stamp, curr_stamp);
	}

#if USE_GPIO_SIMULATION_SPI
	vk_spin_lock_irqsave(&pmotor_info->mtr_lock, flags);

	for (i = 0; i < data_cnt; i++) {
		mdrv_spi_write_packet(pvalue[i] & 0x00FFFF3F);
	}

	vk_spin_unlock_irqrestore(&pmotor_info->mtr_lock, flags);
#else
	data_cnt = (data_cnt <= 4) ? data_cnt : 4;

	for (i = 0; i < data_cnt; i++) {
		pvalue[i] = pvalue[i] & 0x00FFFF3F;
	}

	if (data_cnt >= 2) {
		pvalue[0] |= (pvalue[1] & 0xFF) << 24;
		pvalue[1] = (pvalue[1] & 0x00FFFF00) >> 8;
	}
	if (data_cnt >= 3) {
		pvalue[1] |= (pvalue[2] & 0xFFFF) << 16;
		pvalue[2] = (pvalue[2] & 0x00FF0000) >> 16;
	}
	if (data_cnt >= 4) {
		pvalue[2] |= (pvalue[3] & 0x00FFFFFF) << 8;
		pvalue[3] = 0x00000000;
	}

	//mdrv_spi_write_packet(id, func_type, pvalue, data_cnt);
	if (spi_write(pmotor_info->pspi_device, pvalue, 3*data_cnt) != 0) {
		DBG_ERR("send motor IC SPI fail!\n");
	}
#endif

	if (func_type == VD_FZ_SIGNAL) {
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], 1);
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], 0);
	} else if (func_type == VD_IS_SIGNAL) {
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IS], 1);
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IS], 0);
	}

	vos_perf_mark(&last_stamp);
}

static UINT32 mdrv_read_4_spi(UINT32 id, UINT8 addr)
{
#if USE_GPIO_SIMULATION_SPI
	UINT32 i, rd_data = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	// set CS high => send data => set CS low
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, 1);

	for (i = 0; i < 6; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);

		if ((addr >> i) & 0x01) {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 1);
		} else {
			gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 0);
		}

		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	}
	for (i = 0; i < 2; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);

		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, 1);
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);
	}

	for (i = 0; i < 16; i++) {
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);
		gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);

		if (gpio_get_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx)) {
			rd_data |= (1 << i);
		}
	}

	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, 0);
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 0);
	gpio_set_value(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, 1);

	return rd_data;
#else
	UINT32 wr_data, rd_data = 0;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	wr_data = (UINT32)(addr | 0x40);

	if (spi_write_then_read(pmotor_info->pspi_device, &wr_data, 1, &rd_data, 2) != 0) {
		rd_data = 0xFFFF;
		DBG_ERR("receive motor IC SPI fail!\n");
	}

	return (rd_data & 0xFFFF);
#endif
}

static INT32 mdrv_transmit_cmd(UINT32 id, UINT32 chnel, AN41908_CMD_TYPE cmd_type, INT32 cmd_arg1, INT32 cmd_arg2)
{
	INT32 cmd_status = 0, cmd_idx, reg_tmp;
	UINT32 i, spi_data_len = 0, spi_data[4] = { 0, 0, 0, 0 }, wr_idx[4] = { 0, 0, 0, 0 };
	UINT32 *pwr_reg;
	MOTOR_CONFIG *pmotor_cfg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg = (pmotor_info->motor_config[0].chnel_sel == chnel) ? &pmotor_info->motor_config[0]
		  : ((pmotor_info->motor_config[1].chnel_sel == chnel) ? &pmotor_info->motor_config[1] : NULL);

	if (pmotor_cfg == NULL) {
		return -1;
	}

	pwr_reg = (UINT32 *)&pmotor_info->motor_drv_reg.REG[0];

	switch (cmd_type) {
	case AN41908_SET_CFG:
		spi_data_len = 1;
		cmd_idx = cmd_arg1 & AN41908_REG_UNKNOWN;

		if ((cmd_idx >= AN41908_PULSE1_START_TIME) && (cmd_idx <= AN41908_IRCUT_CTL)) {
			spi_data[0] = (cmd_arg1 & 0x00FFFF00) + cmd_idx;
			spi_data[1] = cmd_arg2 & (0x00FFFF00 | AN41908_REG_UNKNOWN);
		} else {
			cmd_status = -1;
		}

		for (i = 0; i < MTR_ARRAY_SIZE(g_reg_address_ofst); i++) {
			if (cmd_idx == g_reg_address_ofst[i]) {
				wr_idx[0] = i;
				break;
			}
		}
		break;

	case AN41908_SET_SPD:
		spi_data_len = 1;

		spi_data[0] = AN41908_PULSE_FREQUENCY;
		spi_data[0] += ((cmd_arg1 & 0xFFFF) << 8);
		break;

	case AN41908_SET_STEP:
		spi_data_len = 2;

		if (pmotor_cfg->chnel_sel == 0) {
			spi_data[0] = AN41908_PULSE1_MOVE_STEP;
			spi_data[1] = (cmd_arg1 != 0) ? (((UINT32)pmotor_cfg->max_pulse_width << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH)
										  : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH);
		} else if (pmotor_cfg->chnel_sel == 1) {
			spi_data[0] = AN41908_PULSE2_MOVE_STEP;
			spi_data[1] = (cmd_arg1 != 0) ? (((UINT32)pmotor_cfg->max_pulse_width << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH)
										  : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH);
		} else {
			cmd_status = -1;
		}

		reg_tmp = 0x3000;    // use 64 micro-step only

		if (pmotor_cfg->def_positive_dir == 0) {
			spi_data[0] += (cmd_arg1 >= 0) ? (((0x0100 | pmotor_cfg->excite_enable | reg_tmp) | (cmd_arg1 & 0x00FF)) << 8)
										   : (((pmotor_cfg->excite_enable | reg_tmp) | (MTR_ABS(cmd_arg1) & 0x00FF)) << 8);
		} else {
			spi_data[0] += (cmd_arg1 >= 0) ? (((pmotor_cfg->excite_enable | reg_tmp) | (cmd_arg1 & 0x00FF)) << 8)
										   : (((0x0100 | pmotor_cfg->excite_enable | reg_tmp) | (MTR_ABS(cmd_arg1) & 0x00FF)) << 8);
		}
		break;

	case AN41908_SET_BOTH_MOVE:
		spi_data_len = 4;

		if (pmotor_cfg->chnel_sel == 0) {
			spi_data[0] = AN41908_PULSE1_MOVE_STEP;
			spi_data[1] = (cmd_arg1 != 0) ? (((UINT32)g_lens_drv_info[id].motor_config[0].max_pulse_width << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH)
                                          : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH);
			spi_data[2] = AN41908_PULSE2_MOVE_STEP;
			spi_data[3] = (cmd_arg2 != 0) ? (((UINT32)g_lens_drv_info[id].motor_config[1].max_pulse_width << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH)
                                          : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH);
		} else if (pmotor_cfg->chnel_sel == 1) {
			spi_data[0] = AN41908_PULSE2_MOVE_STEP;
			spi_data[1] = (cmd_arg1 != 0) ? (((UINT32)g_lens_drv_info[id].motor_config[0].max_pulse_width << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH)
                                          : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE2_MAX_DUTY_WIDTH);
			spi_data[2] = AN41908_PULSE1_MOVE_STEP;
			spi_data[3] = (cmd_arg2 != 0) ? (((UINT32)g_lens_drv_info[id].motor_config[1].max_pulse_width << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH)
                                          : ((STANDBY_PWR_PULSE_WIDTH << 8) + AN41908_PULSE1_MAX_DUTY_WIDTH);
		}

		reg_tmp = 0x3000;    // use 64 micro-step only

		if (g_lens_drv_info[id].motor_config[0].def_positive_dir == 0) {
			spi_data[0] += (cmd_arg1 >= 0) ? (((0x0100 | g_lens_drv_info[id].motor_config[0].excite_enable | reg_tmp) | (cmd_arg1 & 0x00FF)) << 8)
                                           : (((g_lens_drv_info[id].motor_config[0].excite_enable | reg_tmp) | (MTR_ABS(cmd_arg1) & 0x00FF)) << 8);
		} else {
			spi_data[0] += (cmd_arg1 >= 0) ? (((g_lens_drv_info[id].motor_config[0].excite_enable | reg_tmp) | (cmd_arg1 & 0x00FF)) << 8)
                                           : (((0x0100 | g_lens_drv_info[id].motor_config[0].excite_enable | reg_tmp) | (MTR_ABS(cmd_arg1) & 0x00FF)) << 8);
		}
		if (g_lens_drv_info[id].motor_config[1].def_positive_dir == 0) {
			spi_data[2] += (cmd_arg2 >= 0) ? (((0x0100 | g_lens_drv_info[id].motor_config[1].excite_enable | reg_tmp) | (cmd_arg2 & 0x00FF)) << 8)
                                           : (((g_lens_drv_info[id].motor_config[1].excite_enable | reg_tmp) | (MTR_ABS(cmd_arg2) & 0x00FF)) << 8);
		} else {
			spi_data[2] += (cmd_arg2 >= 0) ? (((g_lens_drv_info[id].motor_config[1].excite_enable | reg_tmp) | (cmd_arg2 & 0x00FF)) << 8)
                                           : (((0x0100 | g_lens_drv_info[id].motor_config[1].excite_enable | reg_tmp) | (MTR_ABS(cmd_arg2) & 0x00FF)) << 8);
		}
		break;

	default:
		cmd_status = -1;
		break;
	}

	wr_idx[0] = MTR_MIN(wr_idx[0], MTR_ARRAY_SIZE(g_reg_address_ofst)-1);
	wr_idx[1] = MTR_MIN(wr_idx[1], MTR_ARRAY_SIZE(g_reg_address_ofst)-1);
	wr_idx[2] = MTR_MIN(wr_idx[2], MTR_ARRAY_SIZE(g_reg_address_ofst)-1);
	wr_idx[3] = MTR_MIN(wr_idx[3], MTR_ARRAY_SIZE(g_reg_address_ofst)-1);

	if (cmd_status != -1) {
		mdrv_write_2_spi(id, VD_FZ_SIGNAL, &spi_data[0], spi_data_len);

		pwr_reg[wr_idx[0]] = spi_data[0];

		if (cmd_type != AN41908_SET_CFG) {
			if (spi_data[1] != 0) {
				pwr_reg[wr_idx[1]] = spi_data[1];
			}
			if (spi_data[2] != 0) {
				pwr_reg[wr_idx[2]] = spi_data[2];
			}
			if (spi_data[3] != 0) {
				pwr_reg[wr_idx[3]] = spi_data[3];
			}
		}
	} else {
		DBG_WRN("err!\r\n");
	}

	return cmd_status;
}

static UINT16 mdrv_receive_cmd(UINT32 id, UINT32 chnel, AN41908_CMD_TYPE cmd_type, INT32 cmd_arg)
{
	UINT8 i, rd_idx = 0;
	INT32 cmd_status = 0;
	UINT32 *prd_reg;
	MOTOR_CONFIG *pmotor_cfg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg = (pmotor_info->motor_config[0].chnel_sel == chnel) ? &pmotor_info->motor_config[0]
		  : ((pmotor_info->motor_config[1].chnel_sel == chnel) ? &pmotor_info->motor_config[1] : NULL);

	if (pmotor_cfg == NULL) {
		return 0xFFFF;
	}

	prd_reg = (UINT32 *)&pmotor_info->motor_drv_reg.REG[0];

	switch (cmd_type) {
	case AN41908_SET_CFG:
		rd_idx = (UINT8)(cmd_arg & AN41908_REG_UNKNOWN);

		for (i = 0; i < MTR_ARRAY_SIZE(g_reg_address_ofst); i++) {
			if (rd_idx == g_reg_address_ofst[i]) {
				rd_idx = i;
				break;
			}
		}

		cmd_status = (i >= MTR_ARRAY_SIZE(g_reg_address_ofst)) ? -1 : 0;
		break;

	case AN41908_SET_SPD:
		rd_idx = 14;	// AN41908_PULSE_FREQUENCY
		break;

	case AN41908_SET_STEP:
	case AN41908_SET_BOTH_MOVE:
		if (pmotor_cfg->chnel_sel == 0) {
			rd_idx = 18;    // AN41908_PULSE1_MOVE_STEP;
		} else if (pmotor_cfg->chnel_sel == 1) {
			rd_idx = 22;    // AN41908_PULSE2_MOVE_STEP;
		} else {
			cmd_status = -1;
		}
		break;

	default:
		cmd_status = -1;
		break;
	}

	rd_idx = MTR_MIN(rd_idx, MTR_ARRAY_SIZE(g_reg_address_ofst)-1);

	if (cmd_status != -1) {
		return (UINT16)prd_reg[rd_idx];
	} else {
		DBG_WRN("err!\r\n");

		return 0xFFFF;
	}
}

static UINT32 mdrv_sel_global_freq(UINT32 freq)
{
	UINT32 i, output_freq;

	output_freq = MTR_CLAMP(freq, g_pwm_freq_tab[0][0], g_pwm_freq_tab[0][MAX_PWM_TAB_SIZE-1]);    // unit: Hz

	for (i = 0; i < MAX_PWM_TAB_SIZE; i++) {
		if (output_freq <= g_pwm_freq_tab[0][i]) {
			break;
		}
	}

	i = MTR_MIN(i, MAX_PWM_TAB_SIZE-1);

	return g_pwm_freq_tab[1][i];
}

static UINT32 mdrv_calc_speed_2_frequency(UINT32 id, UINT32 pwm_freq, MTR_MOTOR_SPEED motor_speed)
{
	UINT32 ret_reg, rotate_freq;

	rotate_freq = (pwm_freq + 240 / 2) / 240;    // 240 is our max pulse number

	switch (motor_speed) {
	case MTR_SPEED_4X:
		rotate_freq *= 4;
		break;

	case MTR_SPEED_2X:
		rotate_freq *= 2;
		break;

	case MTR_SPEED_1X:
	default:
		rotate_freq *= 1;
		break;

	case MTR_SPEED_HALF:
		rotate_freq /= 2;
		break;

	case MTR_SPEED_QUARTER:
		rotate_freq /= 4;
		break;
	}

	ret_reg = (27000000 / rotate_freq + 240 * 24 / 2) / (240 * 24);

	return ret_reg;
}

static INT32 mdrv_init_motor(UINT32 id, MOTOR_SEL motor_sel)
{
	UINT16 rd_reg, reg_tmp;
	UINT32 wt_reg;
	MOTOR_CONFIG *pmotor_cfg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg = (motor_sel == MOTOR_FOCUS) ? &pmotor_info->motor_config[0] :
		    ((motor_sel == MOTOR_ZOOM) ? &pmotor_info->motor_config[1] : NULL);

	if ((motor_sel >= MOTOR_IRIS) || (pmotor_cfg == NULL)) {
		DBG_WRN("unknown motor chnel!\r\n");

		return -1;
	}

	pmotor_cfg->max_step_size = 255 / pmotor_cfg->gear_ratio;

	// update global PWM frequency
	wt_reg = mdrv_sel_global_freq(10*pmotor_cfg->pwm_freq);
	reg_tmp = (wt_reg >> 8) & 0x001F;    // save out PWMMOD value

	if ((rd_reg = mdrv_receive_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_SPD, 0)) == 0xFFFF) {
		DBG_WRN("motor IC may not exist!\r\n");

		return -1;
	}

	wt_reg |= (UINT32)(rd_reg & 0x00FF);
	mdrv_transmit_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_SPD, wt_reg, 0);

	pmotor_cfg->max_pulse_width = 11 * 8 * reg_tmp / 10;    // set 110% peak pulse width
	pmotor_cfg->max_pulse_width = MTR_CLAMP(pmotor_cfg->max_pulse_width, 0x10, 0xFF);
	pmotor_cfg->max_pulse_width |= ((pmotor_cfg->max_pulse_width << 8) & 0xFF00);

	// update respective PWM frequency for achieving desired moving speed
	wt_reg = mdrv_calc_speed_2_frequency(id, pmotor_cfg->pwm_freq, pmotor_cfg->move_speed) & 0xFFFF;

	if (pmotor_cfg->chnel_sel == 0) {
		mdrv_transmit_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, 0);
	} else if (pmotor_cfg->chnel_sel == 1) {
		mdrv_transmit_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, 0);
	}

	return 0;
}

static INT32 mdrv_set_single_position(UINT32 id, MOTOR_SEL motor_sel, INT32 step_pos)
{
	INT32 dly_value;
	MOTOR_CONFIG *pmotor_cfg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg = (motor_sel == MOTOR_FOCUS) ? &pmotor_info->motor_config[0] :
		    ((motor_sel == MOTOR_ZOOM) ? &pmotor_info->motor_config[1] : NULL);

	if (pmotor_cfg == NULL) {
		DBG_WRN("unknown motor chnel!\r\n");

		return -1;
	}

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_SPI) != E_OK) {
		DBG_ERR("set single position fail!\r\n");
		return -1;
	}

	if (step_pos != 0) {
		pmotor_cfg->work_status = pmotor_cfg->motor_status = MOTOR_STATUS_BUSY;

		mdrv_transmit_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_STEP, pmotor_cfg->gear_ratio*step_pos, 0);

		dly_value = (pmotor_cfg->dly_time * MTR_ABS(step_pos) / pmotor_cfg->max_step_size) + 3;
		vos_util_delay_ms(dly_value);

		pmotor_cfg->motor_status = MOTOR_STATUS_FREE;
	} else {
		mdrv_transmit_cmd(id, pmotor_cfg->chnel_sel, AN41908_SET_STEP, 0, 0);

		pmotor_cfg->work_status = MOTOR_STATUS_FREE;
	}

	vos_sem_sig(SEMID_MTR_SPI);

	return step_pos;
}

static void mdrv_set_position(UINT32 id, INT32 fstep_pos, INT32 zstep_pos)
{
	INT32 dly_value;
	MOTOR_CONFIG *pmotor_cfg[2] = { NULL, NULL };
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_cfg[0] = &pmotor_info->motor_config[0];
	pmotor_cfg[1] = &pmotor_info->motor_config[1];

	// wait semaphore
	if (vos_sem_wait(SEMID_MTR_SPI) != E_OK) {
		DBG_ERR("set position fail!\r\n");
		return;
	}

	if (fstep_pos != 0) {
		pmotor_cfg[0]->work_status = pmotor_cfg[0]->motor_status = MOTOR_STATUS_BUSY;
	} else {
		pmotor_cfg[0]->work_status = MOTOR_STATUS_FREE;
	}

	if (zstep_pos != 0) {
		pmotor_cfg[1]->work_status = pmotor_cfg[1]->motor_status = MOTOR_STATUS_BUSY;
	} else {
		pmotor_cfg[1]->work_status = MOTOR_STATUS_FREE;
	}

	mdrv_transmit_cmd(id, pmotor_cfg[0]->chnel_sel, AN41908_SET_BOTH_MOVE, pmotor_cfg[0]->gear_ratio*fstep_pos, pmotor_cfg[1]->gear_ratio*zstep_pos);

	dly_value = MTR_MAX((pmotor_cfg[0]->dly_time * MTR_ABS(fstep_pos) / pmotor_cfg[0]->max_step_size) + 3, (pmotor_cfg[1]->dly_time * MTR_ABS(zstep_pos) / pmotor_cfg[1]->max_step_size) + 3);
	vos_util_delay_ms(dly_value);

	if (fstep_pos != 0) {
		pmotor_cfg[0]->motor_status = MOTOR_STATUS_FREE;
	}

	if (zstep_pos != 0) {
		pmotor_cfg[1]->motor_status = MOTOR_STATUS_FREE;
	}

	vos_sem_sig(SEMID_MTR_SPI);
}

void an41908_open(UINT32 id)
{
	UINT32 i, init_data, rd_data;
	UINT32 *pinit_reg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	vos_sem_create(&SEMID_MTR_SPI, 1, "SEMID_MTR_SPI");
	vos_sem_create(&SEMID_MTR_MOVE, 1, "SEMID_MTR_MOVE");

	if (mdrv_init_spi(id) == 0) {
		// reset motor driver IC
		if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] > 0) {
			gpio_direction_output(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 0);
			vos_util_delay_ms(5);
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], 1);
			vos_util_delay_ms(2);    // for separate following SPI commands
		}

		// write initial table to AN41908 register
		pinit_reg = (UINT32 *)&pmotor_info->motor_drv_reg.REG[0];

		for (i = 0; i < MTR_ARRAY_SIZE(g_reg_address_ofst); i++) {
			if (g_reg_address_ofst[i] != AN41908_IRIS_ADC_OUTPUT) {
				init_data = (pinit_reg[i] << 8) + (UINT32)g_reg_address_ofst[i];
				if ((i <= 9) || (g_reg_address_ofst[i] == 0x0B) || (g_reg_address_ofst[i] == 0x0E)) {
					mdrv_write_2_spi(id, VD_IS_SIGNAL, &init_data, 1);
				} else {
					mdrv_write_2_spi(id, VD_FZ_SIGNAL, &init_data, 1);
				}
				vos_util_delay_ms(2);
			}

			rd_data = mdrv_read_4_spi(id, g_reg_address_ofst[i]) & 0xFFFF;

			DBG_MSG("SPI read back @ 0x%02X = 0x%04X\r\n", g_reg_address_ofst[i], rd_data);
		}

		mdrv_init_motor(id, MOTOR_ZOOM);
		mdrv_init_motor(id, MOTOR_FOCUS);
	} else {
		DBG_ERR("mdrv_init_spi err!\r\n");
	}
}

void an41908_close(UINT32 id)
{
#ifdef __KERNEL__
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_RESET]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN]);
	}
	gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ]);
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IS] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_IS]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0]);
	}
	if (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] != 0) {
		gpio_free(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1]);
	}

	#if USE_GPIO_SIMULATION_SPI
	if (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs != 0) {
		gpio_free(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs);
	}
	if (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk != 0) {
		gpio_free(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk);
	}
	if (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx != 0) {
		gpio_free(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx);
	}
	if (pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx != 0) {
		gpio_free(pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx);
	}
	#endif
#endif

	vos_sem_destroy(SEMID_MTR_SPI);
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
	static BOOL ever_entry[MTR_ID_MAX] = { FALSE };
	static INT32 focus_dly_time[MTR_ID_MAX] = { MOVE_LOOP_FDELAY_TIME };
	static MTR_MOTOR_SPEED focus_move_spd[MTR_ID_MAX] = { MTR_SPEED_1X };
	UINT32 i, tmp, idx = 1, wt_reg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (ever_entry[id] == FALSE) {
		ever_entry[id] = TRUE;

		focus_move_spd[id] = pmotor_info->motor_config[0].move_speed;
		focus_dly_time[id] = pmotor_info->motor_config[0].dly_time;
	}

	speed = MTR_CLAMP(speed, MTR_SPEED_FULL/5, MTR_SPEED_FULL);
	tmp = (speed / (MTR_SPEED_FULL / 5)) - 1;

	if (tmp != 0) {
		for (i = 0; i < tmp; i++) {
			idx *= 2;
		}
	}

	pmotor_info->motor_config[0].move_speed = (MTR_MOTOR_SPEED)idx;
	pmotor_info->motor_config[0].dly_time = focus_dly_time[id] * focus_move_spd[id] / pmotor_info->motor_config[0].move_speed;

	// update respective PWM frequency for achieving desired moving speed
	wt_reg = mdrv_calc_speed_2_frequency(id, pmotor_info->motor_config[0].pwm_freq, pmotor_info->motor_config[0].move_speed) & 0xFFFF;

	while ((pmotor_info->motor_config[0].motor_status == MOTOR_STATUS_BUSY) || (pmotor_info->motor_config[1].motor_status == MOTOR_STATUS_BUSY)) {
		vos_util_delay_ms(1);
	}

	if (pmotor_info->motor_config[0].chnel_sel == 0) {
		if (pmotor_info->motor_config[0].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, ((pmotor_info->motor_config[0].excite_enable | 0x3000) << 8) | AN41908_PULSE1_MOVE_STEP);
		} else if (pmotor_info->motor_config[1].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, ((pmotor_info->motor_config[1].excite_enable | 0x3000) << 8) | AN41908_PULSE2_MOVE_STEP);
		} else {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, 0);
		}
	} else if (pmotor_info->motor_config[0].chnel_sel == 1) {
		if (pmotor_info->motor_config[0].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, ((pmotor_info->motor_config[0].excite_enable | 0x3000) << 8) | AN41908_PULSE2_MOVE_STEP);
		} else if (pmotor_info->motor_config[1].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, ((pmotor_info->motor_config[1].excite_enable | 0x3000) << 8) | AN41908_PULSE1_MOVE_STEP);
		} else {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, 0);
		}
	}
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

		mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, 0);
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

		mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, 0);
	} else {
		DBG_WRN("set the same pos!\r\n");
	}

	return acc_step;
}

INT32 an41908_init_focus(UINT32 id)
{
	UINT16 pulse_width;
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

#if REVERSE_FOCUS_GOHOME_DIR
	mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, 0);
#else
	mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, 0);
#endif

	// increase go home consistency
	pmotor_info->motor_config[0].excite_enable = 0x0000;    // release motor excitation
	mdrv_transmit_cmd(id, pmotor_info->motor_config[0].chnel_sel, AN41908_SET_STEP, 0, 0);

	vos_util_delay_ms(500);

	pulse_width = pmotor_info->motor_config[0].max_pulse_width;    // backup max pulse width
	pmotor_info->motor_config[0].max_pulse_width = 0x5454;

	// tighten motor and gear
	pmotor_info->motor_config[0].excite_enable = 0x0400;

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

#if REVERSE_FOCUS_GOHOME_DIR
	mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, 0);
#else
	mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, 0);
#endif

	pmotor_info->motor_config[0].max_pulse_width = pulse_width;    // restore max pulse width

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

#if REVERSE_FOCUS_GOHOME_DIR
	mdrv_set_focus_move(id, MOTOR_ROTATE_BWD, 0);
#else
	mdrv_set_focus_move(id, MOTOR_ROTATE_FWD, 0);
#endif

	pmotor_info->motor_config[0].curr_position = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
	pmotor_info->motor_config[0].move_2_home = TRUE;

	return E_OK;
}

UINT32 an41908_get_focus_speed(UINT32 id)
{
	return mdrv_get_focus_speed(id);
}

void an41908_set_focus_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category)
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

void an41908_assign_focus_position(UINT32 id, INT32 focus_pos, INT32 dir)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_info->motor_config[0].curr_position = focus_pos;
	pmotor_info->motor_config[0].move_dir = (dir >= 1) ? MOVE_DIR_FWD : MOVE_DIR_BWD;

	pmotor_info->motor_config[0].move_2_home = TRUE;
}

UINT32 an41908_get_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos)
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

void an41908_set_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos, UINT32 focal_length)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	zoom_sec_pos = MTR_CLAMP(zoom_sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	if (zoom_sec_pos == ZOOM_SECTION_WIDE) {
		pmotor_info->lens_proty.focal_len_min = MTR_MIN(focal_length, 15000);     // max: 15 mm
	} else if (zoom_sec_pos == ZOOM_SECTION_TELE) {
		pmotor_info->lens_proty.focal_len_max = MTR_MIN(focal_length, 200000);    // max: 200 mm
	}
}

INT32 an41908_get_focus_range(UINT32 id)
{
	INT32 low_bound = 0, high_bound = 0;

	mdrv_calc_focus_range(id, &low_bound, &high_bound, -1, TRUE);

	return ((high_bound << 16) + low_bound);
}

INT32 an41908_get_focus_position(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (pmotor_info->motor_config[0].move_dir << 16) | pmotor_info->motor_config[0].curr_position;
}

INT32 an41908_set_focus_position(UINT32 id, INT32 focus_pos, BOOL limit_range)
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

INT32 an41908_press_focus_move(UINT32 id, BOOL move_dir)
{
	INT32 target_pos;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (move_dir == FALSE) {    // FOCUS_WIDE = 0
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].focus_min;
	} else {    // FOCUS_TELE = 1
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].focus_max;
	}

	an41908_set_focus_position(id, target_pos, FALSE);

	return E_OK;
}

void an41908_release_focus_move(UINT32 id)
{
	mdrv_get_motor_info(id)->motor_config[0].set_move_stop = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// zoom
//----------------------------------------------------------------------------------------------------------------------
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
	static BOOL ever_entry[MTR_ID_MAX] = { FALSE };
	static INT32 zoom_dly_time[MTR_ID_MAX] = { MOVE_LOOP_ZDELAY_TIME };
	static MTR_MOTOR_SPEED zoom_move_spd[MTR_ID_MAX] = { MTR_SPEED_1X };
	UINT32 i, tmp, idx = 1, wt_reg;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (ever_entry[id] == FALSE) {
		ever_entry[id] = TRUE;

		zoom_move_spd[id] = pmotor_info->motor_config[1].move_speed;
		zoom_dly_time[id] = pmotor_info->motor_config[1].dly_time;
	}

	speed = MTR_CLAMP(speed, MTR_SPEED_FULL/5, MTR_SPEED_FULL);
	tmp = (speed / (MTR_SPEED_FULL / 5)) - 1;

	if (tmp != 0) {
		for (i = 0; i < tmp; i++) {
			idx *= 2;
		}
	}

	pmotor_info->motor_config[1].move_speed = (MTR_MOTOR_SPEED)idx;
	pmotor_info->motor_config[1].dly_time = zoom_dly_time[id] * zoom_move_spd[id] / pmotor_info->motor_config[1].move_speed;

	// update respective PWM frequency for achieving desired moving speed
	wt_reg = mdrv_calc_speed_2_frequency(id, pmotor_info->motor_config[1].pwm_freq, pmotor_info->motor_config[1].move_speed) & 0xFFFF;

	while ((pmotor_info->motor_config[0].motor_status == MOTOR_STATUS_BUSY) || (pmotor_info->motor_config[1].motor_status == MOTOR_STATUS_BUSY)) {
		vos_util_delay_ms(1);
	}

	if (pmotor_info->motor_config[1].chnel_sel == 0) {
		if (pmotor_info->motor_config[0].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, ((pmotor_info->motor_config[0].excite_enable | 0x3000) << 8) | AN41908_PULSE2_MOVE_STEP);
		} else if (pmotor_info->motor_config[1].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, ((pmotor_info->motor_config[1].excite_enable | 0x3000) << 8) | AN41908_PULSE1_MOVE_STEP);
		} else {
			mdrv_transmit_cmd(id, 0, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE1_STEP_CYCLE, 0);
		}
	} else if (pmotor_info->motor_config[1].chnel_sel == 1) {
		if (pmotor_info->motor_config[0].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, ((pmotor_info->motor_config[0].excite_enable | 0x3000) << 8) | AN41908_PULSE1_MOVE_STEP);
		} else if (pmotor_info->motor_config[1].work_status == MOTOR_STATUS_BUSY) {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, ((pmotor_info->motor_config[1].excite_enable | 0x3000) << 8) | AN41908_PULSE2_MOVE_STEP);
		} else {
			mdrv_transmit_cmd(id, 1, AN41908_SET_CFG, (wt_reg << 8) | AN41908_PULSE2_STEP_CYCLE, 0);
		}
	}
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
	UINT32 zcmd_param = 0, fcmd_param = 0;
	INT32 zset_step, fset_step, zrest_step, frest_step, acc_step = 0;
	INT32 focus_tar_pos, focus_ofst, near_bound = 0, inf_bound = 1024, zstep_size;

	mdrv_calc_focus_range(id, &near_bound, &inf_bound, -1, TRUE);

	focus_ofst = MTR_CLAMP((pmotor_info->motor_config[0].curr_position - near_bound), 0, (inf_bound - near_bound));
	zstep_size = 3;    // check 3*(zf table slope) < motor_config[0].max_step_size

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

		mdrv_set_move(id, MOTOR_ROTATE_FWD, 0, 0);
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

		mdrv_set_move(id, MOTOR_ROTATE_BWD, 0, 0);
	}

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

INT32 an41908_init_zoom(UINT32 id)
{
	UINT16 pulse_width;
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

#if REVERSE_ZOOM_GOHOME_DIR
	mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, 0);
#else
	mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, 0);
#endif

	// increase go home consistency
	pmotor_info->motor_config[1].excite_enable = 0x0000;    // release motor excitation
	mdrv_transmit_cmd(id, pmotor_info->motor_config[1].chnel_sel, AN41908_SET_STEP, 0, 0);

	vos_util_delay_ms(500);

	pulse_width = pmotor_info->motor_config[1].max_pulse_width;    // backup max pulse width
	pmotor_info->motor_config[1].max_pulse_width = 0x5454;

	// tighten motor and gear
	pmotor_info->motor_config[1].excite_enable = 0x0400;

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

#if REVERSE_ZOOM_GOHOME_DIR
	mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, 0);
#else
	mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, 0);
#endif

	pmotor_info->motor_config[1].max_pulse_width = pulse_width;    // restore max pulse width

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

#if REVERSE_ZOOM_GOHOME_DIR
	mdrv_set_zoom_move(id, MOTOR_ROTATE_BWD, 0);
#else
	mdrv_set_zoom_move(id, MOTOR_ROTATE_FWD, 0);
#endif

	pmotor_info->motor_config[1].curr_position = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position;

	mdrv_upd_focus_pos(id, pmotor_info);

	pmotor_info->motor_config[1].move_2_home = TRUE;

	return E_OK;
}

UINT32 an41908_get_zoom_speed(UINT32 id)
{
	return mdrv_get_zoom_speed(id);
}

void an41908_set_zoom_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category)
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

void an41908_assign_zoom_position(UINT32 id, INT32 zoom_pos, INT32 dir)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	pmotor_info->motor_config[1].curr_position = zoom_pos;
	pmotor_info->motor_config[1].move_dir = (dir >= 1) ? MOVE_DIR_FWD : MOVE_DIR_BWD;

	pmotor_info->motor_config[1].move_2_home = TRUE;
}

UINT32 an41908_get_zoom_section_pos(UINT32 id)
{
	return mdrv_search_zoom_section(id);
}

UINT32 an41908_get_zoom_max_section_pos(UINT32 id)
{
	return ZOOM_SECTION_TELE;
}

INT32 an41908_set_zoom_section_pos(UINT32 id, UINT32 zoom_sec_pos)
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

INT32 an41908_get_zoom_position(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (pmotor_info->motor_config[1].move_dir << 16) | pmotor_info->motor_config[1].curr_position;
}

INT32 an41908_set_zoom_position(UINT32 id, INT32 zoom_pos)
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

INT32 an41908_press_zoom_move(UINT32 id, BOOL move_dir)
{
	INT32 target_pos;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (move_dir == FALSE) {    // ZOOM_OUT = 0
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_WIDE].zoom_position;
	} else {    // ZOOM_IN = 1
		target_pos = pmotor_info->zoom_focus_tab[ZOOM_SECTION_TELE].zoom_position;
	}

	an41908_set_zoom_position(id, target_pos);

	return E_OK;
}

void an41908_release_zoom_move(UINT32 id)
{
	mdrv_get_motor_info(id)->motor_config[1].set_move_stop = TRUE;
}


//----------------------------------------------------------------------------------------------------------------------
// IR cut
//----------------------------------------------------------------------------------------------------------------------
UINT32 an41908_get_ircut_state(UINT32 id)
{
	return mdrv_get_motor_info(id)->ircut_state;
}

void an41908_set_ircut_state(UINT32 id, IRCUT_ACT state, UINT32 param)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

#if 0
	UINT32 ircut_reg;
	T_IRCUT_CONTROL_2C reg_0x2C;

	param = MTR_MIN(param, 200);

	reg_0x2C.Reg = pmotor_info->motor_drv_reg.REG[24].Reg;

	if (state == IRCUT_OPEN) {    // 1 => open
		reg_0x2C.Bit.IR2 = 1;
		reg_0x2C.Bit.IR1 = 0;
	} else if (state == IRCUT_CLOSE) {    // 2 => close
		reg_0x2C.Bit.IR2 = 0;
		reg_0x2C.Bit.IR1 = 1;
	}

	ircut_reg = (reg_0x2C.Reg << 8) + AN41908_IRCUT_CTL;
	mdrv_write_2_spi(id, VD_NONE_SIGNAL, &ircut_reg, 1);
	pmotor_info->motor_drv_reg.REG[24].Reg = reg_0x2C.Reg;

	vos_util_delay_ms(param);

	// switch back to standby mode
	reg_0x2C.Bit.IR2 = 0;
	reg_0x2C.Bit.IR1 = 0;

	ircut_reg = (reg_0x2C.Reg << 8) + AN41908_IRCUT_CTL;
	mdrv_write_2_spi(id, VD_NONE_SIGNAL, &ircut_reg, 1);
	pmotor_info->motor_drv_reg.REG[24].Reg = reg_0x2C.Reg;

	pmotor_info->ircut_state = state;
#else
	param = MTR_MIN(param, 200);

	if ((pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] != 0) && (pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] != 0)) {
		if (state == IRCUT_OPEN) {
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], 1);
		} else if (state == IRCUT_CLOSE) {
			gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1], 1);
		}

		vos_util_delay_ms(param);

		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], 0);
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1], 0);

		pmotor_info->ircut_state = state;
	} else {
		DBG_ERR("no IR cut ctrl pin!\r\n");
	}
#endif
}


//----------------------------------------------------------------------------------------------------------------------
// aperture
//----------------------------------------------------------------------------------------------------------------------
static void mdrv_set_aperture_state(UINT32 id, APERTURE_ACT_MODE act_item, UINT32 param)
{
	UINT32 target_iris, iris_reg;
	static UINT32 curr_iris[MTR_ID_MAX] = { APERTURE_INIT_DAC };

	switch ((UINT32)act_item) {
	case APERTURE_ACT_SECTION:
		target_iris = g_aperture_2_dac_target[param];

		do {
			if (target_iris >= curr_iris[id]) {
				curr_iris[id] = ((target_iris - curr_iris[id]) > 10) ? (curr_iris[id] + 10) : target_iris;
			} else {
				curr_iris[id] = ((curr_iris[id] - target_iris) > 10) ? (curr_iris[id] - 10) : target_iris;
			}

			iris_reg = (curr_iris[id] << 8) + AN41908_IRIS_TARGET;

			mdrv_write_2_spi(id, VD_IS_SIGNAL, &iris_reg, 1);

			vos_util_delay_ms(10);
		} while (curr_iris[id] != target_iris);
		break;

	case APERTURE_ACT_STEPLESS:
		target_iris = MTR_CLAMP(5*param, g_aperture_2_dac_target[APERTURE_POS_BIG], g_aperture_2_dac_target[APERTURE_POS_SMALL]);

		do {
			if (target_iris >= curr_iris[id]) {
				curr_iris[id] = ((target_iris - curr_iris[id]) > 10) ? (curr_iris[id] + 10) : target_iris;
			} else {
				curr_iris[id] = ((curr_iris[id] - target_iris) > 10) ? (curr_iris[id] - 10) : target_iris;
			}

			iris_reg = (curr_iris[id] << 8) + AN41908_IRIS_TARGET;

			mdrv_write_2_spi(id, VD_IS_SIGNAL, &iris_reg, 1);

			vos_util_delay_ms(10);
		} while (curr_iris[id] != target_iris);
		break;

	default:
		DBG_ERR("unknown!\r\n");
		break;
	}
}

void an41908_init_aperture(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	mdrv_set_aperture_state(id, APERTURE_ACT_SECTION, APERTURE_POS_BIG);

	pmotor_info->aperture_sec_pos = APERTURE_POS_BIG;
	pmotor_info->aperture_position = g_aperture_2_dac_target[APERTURE_POS_BIG] / 5;
}

UINT32 an41908_get_aperture_fno(UINT32 id, UINT32 zoom_sec_pos, UINT32 iris_pos)
{
	UINT32 fno_value, focal_length;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	zoom_sec_pos = MTR_CLAMP(zoom_sec_pos, ZOOM_SECTION_WIDE, ZOOM_SECTION_TELE);

	fno_value = g_f_number_mapping[pmotor_info->aperture_sec_pos];

	if (zoom_sec_pos > ZOOM_SECTION_WIDE) {
		focal_length = pmotor_info->lens_proty.focal_len_min + (zoom_sec_pos - ZOOM_SECTION_WIDE)
					 * (pmotor_info->lens_proty.focal_len_max - pmotor_info->lens_proty.focal_len_min + (ZOOM_SECTION_TELE - ZOOM_SECTION_WIDE) / 2) / (ZOOM_SECTION_TELE - ZOOM_SECTION_WIDE);
	} else {
		focal_length = pmotor_info->lens_proty.focal_len_min;
	}

	fno_value *= (focal_length / pmotor_info->lens_proty.focal_len_min);

	return fno_value;
}

APERTURE_SEC_POS an41908_get_aperture_section_pos(UINT32 id)
{
	return mdrv_get_motor_info(id)->aperture_sec_pos;
}

void an41908_set_aperture_section_pos(UINT32 id, APERTURE_SEC_POS iris_sec_pos)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	iris_sec_pos = MTR_MIN(iris_sec_pos, APERTURE_POS_SMALL);

	mdrv_set_aperture_state(id, APERTURE_ACT_SECTION, iris_sec_pos);

	pmotor_info->aperture_sec_pos = iris_sec_pos;
	pmotor_info->aperture_position = g_aperture_2_dac_target[iris_sec_pos] / 5;
}

UINT32 an41908_get_aperture_position(UINT32 id)
{
	return mdrv_get_motor_info(id)->aperture_position;
}

void an41908_set_aperture_position(UINT32 id, UINT32 iris_pos)
{
	UINT32 i;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	iris_pos = MTR_MIN(iris_pos, 200);

	mdrv_set_aperture_state(id, APERTURE_ACT_STEPLESS, iris_pos);

	pmotor_info->aperture_position = iris_pos;

	for (i = APERTURE_POS_BIG; i < APERTURE_POS_SMALL; i++) {
		if (pmotor_info->aperture_position <= (g_aperture_2_dac_target[i] / 5)) {
			pmotor_info->aperture_sec_pos = (APERTURE_SEC_POS)i;
			break;
		}
	}
}


//----------------------------------------------------------------------------------------------------------------------
// shutter
//----------------------------------------------------------------------------------------------------------------------
SHUTTER_ACT an41908_get_shutter_state(UINT32 id)
{
	return mdrv_get_motor_info(id)->shutter_state;
}

void an41908_set_shutter_state(UINT32 id, SHUTTER_ACT state, UINT32 param)
{

}


//----------------------------------------------------------------------------------------------------------------------
// miscellaneous
//----------------------------------------------------------------------------------------------------------------------
UINT32 an41908_get_capability_info(UINT32 id)
{
	if (id == 0) {
		return (LENS_SUPPORT_APERTURE | LENS_SUPPORT_IRCUT | LENS_SUPPORT_ZOOM | LENS_SUPPORT_FOCUS);
	} else {    // id == 1
		return 0;
	}
}

UINT32 an41908_get_busy_status(UINT32 id)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	return (UINT32)((pmotor_info->motor_config[1].work_status << 1) | pmotor_info->motor_config[0].work_status);
}

UINT32 an41908_get_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(zf_rtab, &pmotor_info->zoom_focus_tab[tab_idx].zoom_position, sizeof(LENS_ZOOM_FOCUS_TAB));

	return tab_idx;
}

UINT32 an41908_set_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(&pmotor_info->zoom_focus_tab[tab_idx].zoom_position, zf_rtab, sizeof(LENS_ZOOM_FOCUS_TAB));

	return tab_idx;
}

UINT32 an41908_get_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(zf_ctab, &pmotor_info->focus_distance_tab[tab_idx].lens_position[0], sizeof(LENS_FOCUS_DISTANCE_TAB));

	return tab_idx;
}

UINT32 an41908_set_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	tab_idx = MTR_MIN(tab_idx, ZOOM_SECTION_NUM-1);

	memcpy(&pmotor_info->focus_distance_tab[tab_idx].lens_position[0], zf_ctab, sizeof(LENS_FOCUS_DISTANCE_TAB));

	return tab_idx;
}

UINT32 an41908_get_register(UINT32 id, UINT32 reg_addr)
{
	UINT32 i, reg_idx = 0, spi_rd_data = 0;

	for (i = 0; i < MTR_ARRAY_SIZE(g_reg_address_ofst); i++) {
		if (((UINT8)reg_addr) == g_reg_address_ofst[i]) {
			reg_idx = i;
			break;
		}
	}

	if (reg_idx < MTR_ARRAY_SIZE(g_reg_address_ofst)) {
		spi_rd_data = mdrv_read_4_spi(id, g_reg_address_ofst[reg_idx]) & 0xFFFF;
		spi_rd_data = (spi_rd_data != 0xFFFF) ? spi_rd_data : 0;
	} else {
		DBG_WRN("not allowable register address!\r\n");
	}

	return spi_rd_data;
}

void an41908_set_register(UINT32 id, UINT32 reg_addr, UINT32 reg_value)
{
	UINT32 i, reg_idx = 0, spi_wr_data = 0;
#if 0    // for TVT
	unsigned long flags;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	if (reg_addr == 0xFA) {
		vk_spin_lock_irqsave(&pmotor_info->mtr_lock, flags);
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], 1);
		gpio_set_value(pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], 0);
		vk_spin_unlock_irqrestore(&pmotor_info->mtr_lock, flags);

		return;
	}
#endif

	for (i = 0; i < MTR_ARRAY_SIZE(g_reg_address_ofst); i++) {
		if (((UINT8)reg_addr) == g_reg_address_ofst[i]) {
			reg_idx = i;
			break;
		}
	}

	if ((reg_idx < MTR_ARRAY_SIZE(g_reg_address_ofst)) && (reg_addr != AN41908_IRIS_ADC_OUTPUT)) {
		reg_value &= 0xFFFF;
		spi_wr_data = (reg_value << 8) + (UINT32)g_reg_address_ofst[reg_idx];

		mdrv_write_2_spi(id, VD_NONE_SIGNAL, &spi_wr_data, 1);
	} else {
		DBG_WRN("not allowable register address!\r\n");
	}
}

void an41908_get_fz_backlash_value(UINT32 id, INT32 *focus_backlash, INT32 *zoom_backlash)
{
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(id);

	*focus_backlash = pmotor_info->motor_config[0].gear_backlash;
	*zoom_backlash = pmotor_info->motor_config[1].gear_backlash;
}

void an41908_set_fz_backlash_value(UINT32 id, INT32 focus_backlash, INT32 zoom_backlash)
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


