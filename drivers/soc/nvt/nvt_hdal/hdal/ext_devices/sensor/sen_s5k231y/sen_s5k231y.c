#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <kdrv_builtin/kdrv_builtin.h>
#include "isp_builtin.h"
#else
#include "plat/gpio.h"
#endif
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/util.h"
#include <kwrap/verinfo.h>
#include "kflow_videocapture/ctl_sen.h"
#include "isp_api.h"

#include "sen_cfg.h"
#include "sen_common.h"
#include "sen_inc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#if defined(__KERNEL__)
char *sen_cfg_path = "null";
module_param_named(sen_cfg_path, sen_cfg_path, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sen_cfg_path, "Path of cfg file");

#ifdef DEBUG
unsigned int sen_debug_level = THIS_DBGLVL;
module_param_named(sen_debug_level, sen_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sen_debug_level, "Debug message level");
#endif
#endif

//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_sen_s5k231y, 1, 43, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_S5K231Y_MODULE_NAME     "sen_s5k231y"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0xFFFF

#define SEN_I2C_ADDR 0x20>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_s5k231y"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_s5k231y(void);
static void sen_pwr_ctrl_s5k231y(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_s5k231y(CTL_SEN_ID id);
static ER sen_close_s5k231y(CTL_SEN_ID id);
static ER sen_sleep_s5k231y(CTL_SEN_ID id);
static ER sen_wakeup_s5k231y(CTL_SEN_ID id);
static ER sen_write_reg_s5k231y(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_s5k231y(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_s5k231y(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static void sen_get_mode_basic_s5k231y(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_s5k231y(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_s5k231y(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_s5k231y(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_s5k231y(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_s5k231y(CTL_SENDRV_GET_MODESEL_PARAM *data);
static void sen_get_rowtime_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_s5k231y(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_s5k231y(CTL_SEN_ID id);
static void sen_set_chgmode_fps_s5k231y(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_s5k231y(CTL_SEN_ID id);
static void sen_get_dvs_info_s5k231y(CTL_SENDRV_DVS_INFO_PARAM *data);
//static void sen_get_iadj_s5k231y(CTL_SEN_ID id,  CTL_SENDRV_GET_MODE_MANUAL_IADJ *data);
//=============================================================================
// global variable
//=============================================================================
static UINT32 sen_map = SEN_PATH_1;

static SEN_PRESET sen_preset[CTL_SEN_ID_MAX] = {
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000}
};

static SEN_DIRECTION sen_direction[CTL_SEN_ID_MAX] = {
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE}
};

static SEN_POWER sen_power[CTL_SEN_ID_MAX] = {
	//C_GPIO:+0x0; P_GPIO:+0x20; S_GPIO:+0x40; L_GPIO:0x60
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1}
};

static SEN_I2C sen_i2c[CTL_SEN_ID_MAX] = {
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR}
};

static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = {
	SEN_S5K231Y_MODULE_NAME,
	CTL_SEN_VENDOR_OTHERS,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_NONE,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x20},
		{CTL_SEN_I2C_W_ADDR_OPTION1, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION2, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION3, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION4, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION5, 0xFF}
	}
};

static CTL_SENDRV_GET_SPEED_PARAM speed_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_SIEMCLK_SRC_DFT,
		20000000,
		100000000,
		250000000
	}
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_8BIT, 0x2A}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	}
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_DVS,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_8BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{7936, 20},
		{{0, 0, 7936, 20}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{7936, 20},
		{0, 1000, 0, 1000},// don't care
		CTL_SEN_RATIO(396, 1),
		{1000, 1000}, //don't care
		100
	}
};

static CTL_SEN_CMD s5k231y_mode_1[] = {
	//////////////////////////////////////////////////////////////////
	//////////////        System Setting       ///////////////////////
	//////////////////////////////////////////////////////////////////
	{0x300D, 1, {0x04, 0x0}}, // PLL_P_r
	{0x300F, 1, {0xC8, 0x0}},	// PLL_M_r
	{0x3010, 1, {0x00, 0x0}}, // PLL_S_r
	{0x3500, 1, {0x01, 0x0}},	// ACTDCS_BYPASS_r
	{0x3600, 1, {0x00, 0x0}},	// SHIST_OFF_r
	{0x3300, 1, {0x01, 0x0}},	// CROP_BYPASS_r
	{0x3043, 1, {0x01, 0x0}},	// ESP_CTRL0
	{0x3266, 1, {0x08, 0x0}},	// PSEQ_PXL_CON
	//////////////////////////////////////////////////////////////////
	//////////////        Analog Setting       ///////////////////////
	//////////////////////////////////////////////////////////////////
	{0x000D, 1, {0x04, 0x0}},	// REG_CTRL_OTP_TRIM
	{0x0010, 1, {0xFF, 0x0}}, // REG_EN_DBGN
	{0x000c, 1, {0x14, 0x0}}, // 
	{0x000F, 1, {0x05, 0x0}}, // REG_EN_DBGP
	{0x0013, 1, {0x03, 0x0}},	// REG_EN_BUFP
	{0x0014, 1, {0x7f, 0x0}},	// REG_EN_BUFN
	{0x0018, 1, {0x03, 0x0}},	// REG_CTRL_DUMM_NUM_AMP_B
	{0x000b, 1, {0x04, 0x0}},	// ON MSB
	{0x001c, 1, {0x07, 0x0}},	// REG_CTRL_DUMM_NUM_ON_B
	{0x0012, 1, {0x7f, 0x0}},	// REG_CTRL_IBGN
	{0x001e, 1, {0x03, 0x0}},	// REG_CTRL_DUMM_NUM_nOFF_B
	//////////////////////////////////////////////////////////////////
	//////////////        OUTIF Setting        ///////////////////////
	//////////////////////////////////////////////////////////////////
	{0x3900, 1, {0xf4, 0x0}},	// set mipi enable | 4-lane | use_fram
	{0x390B, 1, {0x08, 0x0}},	// set use big endian
	{0x391A, 1, {0x14, 0x0}},	// packet_cnt_low  (20 Lines)
	{0x391B, 1, {0x00, 0x0}},	// packet_cnt_high
	{0x3901, 1, {0x2A, 0x0}},	// csi2_header_B0 set csi2_header : ID(RAW8 -8bit)
	{0x3902, 1, {0x00, 0x0}},	// csi2_header_B1 set data frame size low
	{0x3903, 1, {0x1F, 0x0}},	// csi2_header_B2 set data frame size high (7936 dec)
	{0x301E, 1, {0x00, 0x0}},	// PARA_OUT_EN_r
	{0x3908, 1, {0x40, 0x0}},	// set mipi_auto_enable(use_mipi) 
	{0x390C, 1, {0x01, 0x0}},	// timeout_cnt
	{0x3034, 1, {0x00, 0x0}},	// clr use_mgrp (default : 00h) --> Need to Check
	{0x3234, 1, {0x63, 0x0}}, // for 100MHz InternalCLK(100-1) --> Need to Check
	//////////////////////////////////////////////////////////////////
	//////////////     Analog Timming Setting     ////////////////////
	//////////////////////////////////////////////////////////////////
	{0x3248, 1, {0x00, 0x0}},	// DTAG_GRS_ON_TR_r            // GRS Mode (Reset on Detection Time - don't change, should be 00)
	{0x3249, 1, {0x00, 0x0}},	// DTAG_GRS_CNT_ON_TR_r        // Not Needed, 00 recommended
	{0x324A, 1, {0x01, 0x0}},	// DTAG_GRS_CNT_ON_TR_r        // Not Needed, 01 recommended
	{0x325A, 1, {0x00, 0x0}},	// DTAG_GRS_END_CNT_ON_TR_r    // Not Needed, 00 recommended
	{0x325B, 1, {0x01, 0x0}},	// DTAG_GRS_END_CNT_ON_TR_r    // Not Needed, 01 recommended
	{0x3244, 1, {0x00, 0x0}},	// DTAG_GRS_CNT_r              // GRS_START
	{0x3245, 1, {0x00, 0x0}},	// DTAG_GRS_COARSE_CNT_r       // GRS_START
	{0x3246, 1, {0x00, 0x0}},	// DTAG_GRS_COARSE_CNT_r       // GRS_START
	{0x3247, 1, {0x32, 0x0}},	// DTAG_GRS_FINE_CNT_r         // GRS_START (1 usec)
	{0x3256, 1, {0x00, 0x0}},	// DTAG_GRS_END_CNT_r          // GRS_END
	{0x3257, 1, {0x00, 0x0}},	// DTAG_GRS_END_COARSE_CNT_r   // GRS_END
	{0x3258, 1, {0x00, 0x0}},	// DTAG_GRS_END_COARSE_CNT_r   // GRS_END
	{0x3259, 1, {0x64, 0x0}},	// DTAG_GRS_END_FINE_CNT_r     // GRS_END (2 usec)
	{0x324C, 1, {0x0C, 0x0}},	// DTAG_SELX_END_r             // SELW (120ns)
	{0x324E, 1, {0x08, 0x0}},	// DTAG_AED_UPDATE_NUM_r       // SEL2AY_r (80ns)
	{0x324F, 1, {0x0C, 0x0}},	// DTAG_AY_END_r               // SEL2AY_f (120ns)
	{0x3253, 1, {0x10, 0x0}},	// DTAG_R_START_r              // SEL2R_r (160ns)
	{0x3254, 1, {0x14, 0x0}},	// DTAG_R_END_r                // SEL2R_f (200ns)
	{0x323C, 1, {0x08, 0x0}},	// DTAG_FST_SELX_START_r       // GH2SEL (80ns)
	{0x3261, 1, {0x00, 0x0}},	// DTAG_NEXT_SELX_START_r      // NEXT_SELX
	{0x3262, 1, {0x1E, 0x0}},	// DTAG_NEXT_SELX_START_r      // NEXT_SELX (300ns)
	{0x3251, 1, {0x19, 0x0}},	// DTAG_MAX_EVENT_NUM_r        // NEXT_SELX-5, Max 0x3C
	{0x324B, 1, {0x14, 0x0}},	// DTAG_NEXT_GH_CNT_r          // NEXT_GH (200ns)
	{0x3240, 1, {0x20, 0x0}},	// DTAG_GH_CNT_r               // ms		# x1msec
	{0x3241, 1, {0x02, 0x0}},	// DTAG_GH_COARSE_CNT_r        // us[MSB]
	{0x3242, 1, {0x35, 0x0}},	// DTAG_GH_COARSE_CNT_r        // us[LSB]	# x1usec  //Test for 999fps
	{0x3243, 1, {0x64, 0x0}},	// DTAG_GH_FINE_CNT_r          // ns		# x10nsec
	{0x325D, 1, {0xB9, 0x0}},	// DTAG_READ_TIME_INTERVAL_r   // READ_FIXED (usec MSB)
	{0x325E, 1, {0x28, 0x0}},	// DTAG_READ_TIME_INTERVAL_r   // READ_FIXED (usec LSB)
	{0x3260, 1, {0x00, 0x0}},	// DTAG_EXT_TRIG_MODE_r
	{0x3255, 1, {0x0D, 0x0}},	// G.HOLD1, G.RST0
	{0x3201, 1, {0x01, 0x0}},	// DTAG_MANUAL_ON_r
	//////////////////////////////////////////////////////////////////
	//////////////  Full & Fixed Frame Setting    ////////////////////
	//////////////////////////////////////////////////////////////////
	{0x325C, 1, {0x01, 0x0}},	// DTAG_FIXED_READ_TIME_ON_r
	{0x3231, 1, {0x01, 0x0}},	// DTAG_EDS_TEST_MODE_SEL_r
	{0x3068, 1, {0x01, 0x0}},	// ALIGN_EDS_TEST_MODE_r
	//////////////////////////////////////////////////////////////////
	//////////////       Time Stamp Setting       ////////////////////
	//////////////////////////////////////////////////////////////////
	{0x3204, 1, {0x63, 0x0}}, // TSTAMP_SUB_UNIT_VAL_r
	{0x3235, 1, {0x03, 0x0}},	// TSTAMP_REF_UNIT_VAL_r_MSB
	{0x3236, 1, {0xE7, 0x0}},	// TSTAMP_REF_UNIT_VAL_r_LSB
	{0x3237, 1, {0x00, 0x0}},	// TSTAMP_TIME_TICK_VAL_r
	//////////////////////////////////////////////////////////////////
	//////////////       Forced On Setting        ////////////////////
	//////////////////////////////////////////////////////////////////
	// s000D24	// REG_CTRL_OTP_TRIM
	// s001500	// REG_EN_DOB
	// s00083B	//
	// s000B04	//
	// s001C00	// REG_CTRL_DUMM_NUM_ON_B
	// s00127D	// REG_CTRL_IBGN
	// s00147F	// REG_EN_BUFN
	// s001E00	// REG_CTRL_DUMM_NUM_nOFF_B
	////////////////////////////////////////////////////////////////////////
	/*
	//integration test
	{0x0000, 1, {0x24, 0x0}},
	{0x0015, 1, {0x00, 0x0}},
	{0x0008, 1, {0x38, 0x0}},
	{0x0008, 1, {0x04, 0x0}},
	{0x001C, 1, {0x00, 0x0}},
	{0x0012, 1, {0x70, 0x0}},
	{0x0014, 1, {0x7F, 0x0}},
	{0x001E, 1, {0x00, 0x0}},
	*/
	//integration test
	{0x3000, 1, {0x02, 0x0}}, // Stream On
	{0x3238, 1, {0x01, 0x0}},	// dvs timestamp reset
	{0x3238, 1, {0x00, 0x0}},	// dvs timestamp restartt
};


static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB s5k231y_sen_drv_tab = {
	sen_open_s5k231y,
	sen_close_s5k231y,
	sen_sleep_s5k231y,
	sen_wakeup_s5k231y,
	sen_write_reg_s5k231y,
	sen_read_reg_s5k231y,
	sen_chg_mode_s5k231y,
	sen_chg_fps_s5k231y,
	sen_set_info_s5k231y,
	sen_get_info_s5k231y,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_s5k231y(void)
{
	return &s5k231y_sen_drv_tab;
}

static void sen_pwr_ctrl_s5k231y(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;	
	DBG_IND("enter flag %d \r\n", flag);

	if (flag == CTL_SEN_PWR_CTRL_TURN_ON) {

		if (sen_power[id].pwdn_pin != CTL_SEN_IGNORE) {
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( pwdn_ctrl_count[i] == (sen_power[id].pwdn_pin)) {
					pwdn_count++;
				}
			}
			pwdn_ctrl_count[id] = (sen_power[id].pwdn_pin);			

			if (!pwdn_count) {
				gpio_direction_output((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 1);
			}
		}

		if (clk_cb != NULL) {
			if (sen_power[id].mclk != CTL_SEN_IGNORE) {
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK] += 1;
				} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK2] += 1;
				} else { //CTL_SEN_CLK_SEL_SIEMCLK3
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK3] += 1;
				}
				if (1 == (power_ctrl_mclk[sen_power[id].mclk])) {
					clk_cb(sen_power[id].mclk, TRUE);
				}
			}
		}

		if (sen_power[id].rst_pin != CTL_SEN_IGNORE) {
			vos_util_delay_ms(sen_power[id].stable_time);
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( reset_ctrl_count[i] == (sen_power[id].rst_pin)) {
					reset_count++;
				}
			}
			reset_ctrl_count[id] = (sen_power[id].rst_pin);			

			if (!reset_count) {
				gpio_direction_output((sen_power[id].rst_pin), 0);
				gpio_set_value((sen_power[id].rst_pin), 0);
				vos_util_delay_ms(sen_power[id].rst_time);
				gpio_set_value((sen_power[id].rst_pin), 1);
				vos_util_delay_ms(sen_power[id].stable_time);
			}
		}
	}

	if (flag == CTL_SEN_PWR_CTRL_TURN_OFF) {
		
		if (sen_power[id].pwdn_pin != CTL_SEN_IGNORE) {
			pwdn_ctrl_count[id] = 0;

			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( pwdn_ctrl_count[i] == (sen_power[id].pwdn_pin)) {
					pwdn_count++;
				}
			}

			if (!pwdn_count) {
				gpio_direction_output((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 0);
			}
		}
	
		if (sen_power[id].rst_pin != CTL_SEN_IGNORE) {
			reset_ctrl_count[id] = 0;

			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( reset_ctrl_count[i] == (sen_power[id].rst_pin)) {
					reset_count++;
				}
			}

			if (!reset_count) {
				gpio_direction_output((sen_power[id].rst_pin), 0);
				gpio_set_value((sen_power[id].rst_pin), 0);
				vos_util_delay_ms(sen_power[id].stable_time);
			}
		}		
		
		if (clk_cb != NULL) {
			if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
				power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK] -= 1;
			} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
				power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK2] -= 1;
			} else { //CTL_SEN_CLK_SEL_SIEMCLK3
				power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK3] -= 1;
			}
			if (!power_ctrl_mclk[sen_power[id].mclk]) {
				clk_cb(sen_power[id].mclk, FALSE);
			}
		}
	}
}

static CTL_SEN_CMD sen_set_cmd_info_s5k231y(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

static ER sen_open_s5k231y(CTL_SEN_ID id)
{
	ER rt = E_OK;

	rt = sen_i2c_init_driver(id, &sen_i2c[id]);

	if (rt != E_OK) {
		i2c_valid[id] = FALSE;

		DBG_ERR("init. i2c driver fail (%d) \r\n", id);
	} else {
		i2c_valid[id] = TRUE;
	}

	return rt;
}

static ER sen_close_s5k231y(CTL_SEN_ID id)
{
	if (i2c_valid[id]) {
		sen_i2c_remove_driver(id);
		i2c_valid[id] = FALSE;
	}

	return E_OK;
}

static ER sen_sleep_s5k231y(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_s5k231y(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_s5k231y(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[3];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = (cmd->addr >> 8) & 0xFF;
	buf[1]     = cmd->addr & 0xFF;
	buf[2]     = cmd->data[0] & 0xFF;
	msgs.addr  = sen_i2c[id].addr;
	msgs.flags = 0;
	msgs.len   = 3;
	msgs.buf   = buf;

	i = 0;
	while(1){
		if (sen_i2c_transfer(id, &msgs, 1) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	return E_OK;
}

static ER sen_read_reg_s5k231y(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg  msgs[2];
	unsigned char   tmp[2], tmp2[2];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	tmp[0]        = (cmd->addr >> 8) & 0xFF;
	tmp[1]        = cmd->addr & 0xFF;
	msgs[0].addr  = sen_i2c[id].addr;
	msgs[0].flags = 0;
	msgs[0].len   = 2;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].addr  = sen_i2c[id].addr;
	msgs[1].flags = 1;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	i = 0;
	while(1){
		if (sen_i2c_transfer(id, msgs, 2) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_s5k231y(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = s5k231y_mode_1;
		return sizeof(s5k231y_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_s5k231y(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	DBG_WRN(" not support fps adjust \r\n");
	sen_set_cur_fps_s5k231y(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	sen_set_chgmode_fps_s5k231y(id, mode_basic_param[cur_sen_mode[id]].dft_fps);

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else {
			cmd = sen_set_cmd_info_s5k231y(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_s5k231y(id, &cmd);
		}
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_s5k231y(CTL_SEN_ID id, UINT32 fps)
{
	ER rt = E_OK;

	DBG_WRN(" not support fps adjust \r\n");
	sen_set_cur_fps_s5k231y(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	sen_set_chgmode_fps_s5k231y(id, mode_basic_param[cur_sen_mode[id]].dft_fps);

	return rt;
}

static ER sen_set_info_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_s5k231y(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_s5k231y((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_s5k231y((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_s5k231y(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_s5k231y((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_s5k231y(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_s5k231y(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_s5k231y((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_s5k231y((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_s5k231y((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_s5k231y(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_DVS_INFO:
		sen_get_dvs_info_s5k231y((CTL_SENDRV_DVS_INFO_PARAM *)(data));
		break;
/*
	case CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ:
		sen_get_iadj_s5k231y(id, (CTL_SENDRV_GET_MODE_MANUAL_IADJ *)(data));
		break;
*/
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static void sen_get_mode_basic_s5k231y(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_s5k231y(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_s5k231y(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_s5k231y(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	#if 1
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		return E_OK;
	}
	return E_NOSPT;
	#else
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		memcpy(&data->info.mipi, &mipi, sizeof(CTL_SENDRV_MIPI));
		return E_OK;
	}
	return E_NOSPT;
	#endif
}

static void sen_get_fps_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_s5k231y(id);
	data->chg_fps = sen_get_chgmode_fps_s5k231y(id);
}

static void sen_get_speed_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &speed_param[mode], sizeof(CTL_SENDRV_GET_SPEED_PARAM));

	if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK;
	} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK2;
	} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK3) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK3;
	} else if (sen_power[id].mclk == CTL_SEN_IGNORE) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_IGNORE;
	} else {
		DBG_ERR("mclk source is fail \r\n");
	}
}

static void sen_get_mode_mipi_s5k231y(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_s5k231y(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_DVS) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->frame_num == 1) {
		if ((data->size.w <= 7936) && (data->size.h <= 20)) {
			if (data->frame_rate <= 3000) {
				data->mode = CTL_SEN_MODE_1;
				return;
			}
		}
	} 

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static void sen_get_rowtime_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = 1; //don't care
	data->row_time = 100; //don't care
}

static void sen_set_cur_fps_s5k231y(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_s5k231y(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_s5k231y(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_s5k231y(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

static void sen_get_dvs_info_s5k231y(CTL_SENDRV_DVS_INFO_PARAM *data)
{
	data->size.w = 15360;//(60*8*32)
	data->size.h = 20;
}

/*
static void sen_get_iadj_s5k231y(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_MANUAL_IADJ *data)
{
	data->sel = CTL_SEN_MANUAL_IADJ_SEL_IADJ;
	data->val.iadj = 3;
}
*/

#if defined(__FREERTOS)
void sen_get_i2c_id_s5k231y(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_s5k231y(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_s5k231y(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	ER rt = E_OK;

	sprintf(compatible, "nvt,sen_s5k231y");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_s5k231y");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_s5k231y;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_s5k231y();
	rt = ctl_sen_reg_sendrv("nvt_sen_s5k231y", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_s5k231y(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_s5k231y");
}

#else
static int __init sen_init_s5k231y(void)
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	CFG_FILE_FMT *pcfg_file;
	CTL_SEN_REG_OBJ reg_obj;
	ER rt = E_OK;

	// Parsing cfc file if exist
	if ((strstr(sen_cfg_path, "null")) || (strstr(sen_cfg_path, "NULL"))) {
		DBG_WRN("cfg file no exist \r\n");
		cfg_path[0] = '\0';
	} else {
		if ((sen_cfg_path != NULL) && (strlen(sen_cfg_path) <= MAX_PATH_NAME_LENGTH)) {
			strncpy((char *)cfg_path, sen_cfg_path, MAX_PATH_NAME_LENGTH);
		}

		if ((pcfg_file = sen_common_open_cfg(cfg_path)) != NULL) {
			DBG_MSG("load %s success \r\n", sen_cfg_path);
			sen_common_load_cfg_map(pcfg_file, &sen_map);
			sen_common_load_cfg_preset(pcfg_file, &sen_preset);
			sen_common_load_cfg_direction(pcfg_file, &sen_direction);
			sen_common_load_cfg_power(pcfg_file, &sen_power);
			sen_common_load_cfg_i2c(pcfg_file, &sen_i2c);
			sen_common_close_cfg(pcfg_file);
		} else {
			DBG_WRN("load cfg fail \r\n");
		}
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_s5k231y;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_s5k231y();
	rt = ctl_sen_reg_sendrv("nvt_sen_s5k231y", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_s5k231y(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_s5k231y");
}

module_init(sen_init_s5k231y);
module_exit(sen_exit_s5k231y);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_S5K231Y_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif


