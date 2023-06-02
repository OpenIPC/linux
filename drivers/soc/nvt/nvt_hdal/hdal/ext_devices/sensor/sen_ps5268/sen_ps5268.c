#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
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

int fae_enable = 1;
module_param_named(fae_enable, fae_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(fae_enable, "Fast auto exposure enable");
#else
static int fae_enable = 1;
#endif

//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_sen_ps5268, 1, 43, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_PS5268_MODULE_NAME     "sen_ps5268"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0xFFFF
#define MIN_EXPOSURE_LINE          2
#define NON_EXPOSURE_LINE          2

#define SEN_I2C_ADDR 0x90 >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_ps5268"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_ps5268(void);
static void sen_pwr_ctrl_ps5268(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_ps5268(CTL_SEN_ID id);
static ER sen_close_ps5268(CTL_SEN_ID id);
static ER sen_sleep_ps5268(CTL_SEN_ID id);
static ER sen_wakeup_ps5268(CTL_SEN_ID id);
static ER sen_write_reg_ps5268(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_ps5268(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_ps5268(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_ps5268(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_ps5268(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_ps5268(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_ps5268(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_ps5268(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_ps5268(CTL_SEN_ID id, void *param);
static void sen_set_expt_ps5268(CTL_SEN_ID id, void *param);
static void sen_set_preset_ps5268(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_ps5268(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_ps5268(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static void sen_get_gain_ps5268(CTL_SEN_ID id, void *param);
static BOOL sen_get_fae(CTL_SEN_ID id, ISP_SENSOR_CTRL *sensor_ctrl);
static void sen_get_expt_ps5268(CTL_SEN_ID id, void *param);
static void sen_get_min_expt_ps5268(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_ps5268(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_ps5268(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_ps5268(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_ps5268(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_ps5268(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_ps5268(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_ps5268(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_ps5268(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_ps5268(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_ps5268(CTL_SEN_ID id);
static void sen_set_chgmode_fps_ps5268(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_ps5268(CTL_SEN_ID id);
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
	{CTL_SEN_CLK_SEL_SIEMCLK, 0x47, 0x48, 1, 1},//MCLK, pwdn, reset, reset_time,stable_time
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
	SEN_PS5268_MODULE_NAME,
	CTL_SEN_VENDOR_OMNIVISION,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_MIRROR|CTL_SEN_SUPPORT_PROPERTY_FLIP|CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x90},
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
		27000000,//MCLK
		74250000,//PCLK
		200000000,//DATA RATE
	}
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_2,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	}
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_B,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1920, 1082},//sensor output
		{{0, 2, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)
		{1920, 1080},
		{0, 2200, 0, 1125},//HTS,VTS
		CTL_SEN_RATIO(16, 9),
		{1000, 32000},//GAIN range
		100
	}
};

static CTL_SEN_CMD ps5268_mode_1[] = {
	{0x0110, 1, {0x00, 0x0}},
	{0x010B, 1, {0x07, 0x0}},
	{0x0114, 1, {0x08, 0x0}},
	{0x0115, 1, {0x98, 0x0}},
	{0x0162, 1, {0x02, 0x0}},
	{0x0178, 1, {0xC0, 0x0}},
	{0x0179, 1, {0x1A, 0x0}},
	{0x022D, 1, {0x01, 0x0}},
	{0x0226, 1, {0xB1, 0x0}},
	{0x0227, 1, {0x39, 0x0}},
	{0x021C, 1, {0x00, 0x0}},
	{0x0233, 1, {0x70, 0x0}},
	{0x023C, 1, {0x57, 0x0}},
	{0x024B, 1, {0x05, 0x0}},
	{0x024D, 1, {0x11, 0x0}},
	{0x0252, 1, {0x19, 0x0}},
	{0x0253, 1, {0x26, 0x0}},
	{0x0254, 1, {0x61, 0x0}},
	{0x0255, 1, {0x11, 0x0}},
	{0x0624, 1, {0x05, 0x0}},
	{0x0625, 1, {0x0D, 0x0}},
	{0x0B02, 1, {0x02, 0x0}},
	{0x0B0C, 1, {0x00, 0x0}},
	{0x0D00, 1, {0x00, 0x0}},
	{0x0E0C, 1, {0x02, 0x0}},
	{0x0E0E, 1, {0x3A, 0x0}},
	{0x0E10, 1, {0x04, 0x0}},
	{0x0E12, 1, {0x80, 0x0}},
	{0x1409, 1, {0x1A, 0x0}},
	{0x140A, 1, {0x15, 0x0}},
	{0x1411, 1, {0x01, 0x0}},
	{0x1415, 1, {0x04, 0x0}},
	{0x1417, 1, {0x03, 0x0}},
	{0x1418, 1, {0x02, 0x0}},
	{0x1406, 1, {0x04, 0x0}},
	{0x1410, 1, {0x02, 0x0}},
	{0x140F, 1, {0x01, 0x0}},
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},	
	{SEN_CMD_DIRECTION, 1, {00, 0x0}},
	{0x0111, 1, {0x01, 0x0}},//cmd update	
	{0x010F, 1, {0x01, 0x0}},
	{SEN_CMD_PRESET, 1, {00, 0x0}},
};

typedef struct gain_wdr_set {
	UINT16 a_gain;
	UINT16 d_gain;
	UINT32 total_gain;
} gain_wdr_set_t;

// A Gain 0~32X, D Gain 0~15.5X = 496X
static const gain_wdr_set_t gain_wdr_table[] = {
	{0 , 0 , 1000  }, {1 , 0 , 1063  }, {2 , 0 , 1125  }, {3 , 0 , 1188   },
	{4 , 0 , 1250  }, {5 , 0 , 1313  }, {6 , 0 , 1375  }, {7 , 0 , 1438   },
	{8 , 0 , 1500  }, {9 , 0 , 1563  }, {10, 0 , 1625  }, {11, 0 , 1688   },
	{12, 0 , 1750  }, {13, 0 , 1813  }, {14, 0 , 1875  }, {15, 0 , 1938   },
	{16, 0 , 2000  }, {17, 0 , 2125  }, {18, 0 , 2250  }, {19, 0 , 2375   },
	{20, 0 , 2500  }, {21, 0 , 2625  }, {22, 0 , 2750  }, {23, 0 , 2875   },
	{24, 0 , 3000  }, {25, 0 , 3125  }, {26, 0 , 3250  }, {27, 0 , 3375   },
	{28, 0 , 3500  }, {29, 0 , 3625  }, {30, 0 , 3750  }, {31, 0 , 3875   },
	{32, 0 , 4000  }, {33, 0 , 4250  }, {34, 0 , 4500  }, {35, 0 , 4750   },
	{36, 0 , 5000  }, {37, 0 , 5250  }, {38, 0 , 5500  }, {39, 0 , 5750   },
	{40, 0 , 6000  }, {41, 0 , 6250  }, {42, 0 , 6500  }, {43, 0 , 6750   },
	{44, 0 , 7000  }, {45, 0 , 7250  }, {46, 0 , 7500  }, {47, 0 , 7750   },
	{48, 0 , 8000  }, {49, 0 , 8500  }, {50, 0 , 9000  }, {51, 0 , 9500   },
	{52, 0 , 10000 }, {53, 0 , 10500 }, {54, 0 , 11000 }, {55, 0 , 11500  },
	{56, 0 , 12000 }, {57, 0 , 12500 }, {58, 0 , 13000 }, {59, 0 , 13500  },
	{60, 0 , 14000 }, {61, 0 , 14500 }, {62, 0 , 15000 }, {63, 0 , 15500  },
	{64, 0 , 16000 }, {65, 0 , 17000 }, {66, 0 , 18000 }, {67, 0 , 19000  },
	{68, 0 , 20000 }, {69, 0 , 21000 }, {70, 0 , 22000 }, {71, 0 , 23000  },
	{72, 0 , 24000 }, {73, 0 , 25000 }, {74, 0 , 26000 }, {75, 0 , 27000  },
	{76, 0 , 28000 }, {77, 0 , 29000 }, {78, 0 , 30000 }, {79, 0 , 31000  },
	{80, 0 , 32000 }, {80, 1 , 34000 }, {80, 2 , 36000 }, {80, 3 , 38000  },
	{80, 4 , 40000 }, {80, 5 , 42000 }, {80, 6 , 44000 }, {80, 7 , 46000  },
	{80, 8 , 48000 }, {80, 9 , 50000 }, {80, 10, 52000 }, {80, 11, 54000  },
	{80, 12, 56000 }, {80, 13, 58000 }, {80, 14, 60000 }, {80, 15, 62000  },
	{80, 16, 64000 }, {80, 17, 68000 }, {80, 18, 72000 }, {80, 19, 76000  },
	{80, 20, 80000 }, {80, 21, 84000 }, {80, 22, 88000 }, {80, 23, 92000  },
	{80, 24, 96000 }, {80, 25, 100000}, {80, 26, 104000}, {80, 27,	108000},
	{80, 28, 112000}, {80, 29, 116000}, {80, 30, 120000}, {80, 31,	124000},
	{80, 32, 128000}, {80, 33, 136000}, {80, 34, 144000}, {80, 35,	152000},
	{80, 36, 160000}, {80, 37, 168000}, {80, 38, 176000}, {80, 39,	184000},
	{80, 40, 192000}, {80, 41, 200000}, {80, 42, 208000}, {80, 43,	216000},
	{80, 44, 224000}, {80, 45, 232000}, {80, 46, 240000}, {80, 47,	248000},
	{80, 48, 256000}, {80, 49, 272000}, {80, 50, 288000}, {80, 51,	304000},
	{80, 52, 320000}, {80, 53, 336000}, {80, 54, 352000}, {80, 55,	368000},
	{80, 56, 384000}, {80, 57, 400000}, {80, 58, 416000}, {80, 59,	432000},
	{80, 60, 448000}, {80, 61, 464000}, {80, 62, 480000}, {80, 63,  496000},	
};
#define NUM_OF_WDRGAINSET (sizeof(gain_wdr_table) / sizeof(gain_wdr_set_t))

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB ps5268_sen_drv_tab = {
	sen_open_ps5268,
	sen_close_ps5268,
	sen_sleep_ps5268,
	sen_wakeup_ps5268,
	sen_write_reg_ps5268,
	sen_read_reg_ps5268,
	sen_chg_mode_ps5268,
	sen_chg_fps_ps5268,
	sen_set_info_ps5268,
	sen_get_info_ps5268,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_ps5268(void)
{
	return &ps5268_sen_drv_tab;
}

static void sen_pwr_ctrl_ps5268(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_ps5268(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;
	
	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_ps5268(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_ps5268(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_ps5268);
	#endif

	rt = sen_i2c_init_driver(id, &sen_i2c[id]);

	if (rt != E_OK) {
		i2c_valid[id] = FALSE;

		DBG_ERR("init. i2c driver fail (%d) \r\n", id);
	} else {
		i2c_valid[id] = TRUE;
	}

	return rt;
}

static ER sen_close_ps5268(CTL_SEN_ID id)
{
	if (i2c_valid[id]) {
		sen_i2c_remove_driver(id);
		i2c_valid[id] = FALSE;
	}

	return E_OK;
}

static ER sen_sleep_ps5268(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");

	return E_OK;
}

static ER sen_wakeup_ps5268(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");

	return E_OK;
}

static ER sen_write_reg_ps5268(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static ER sen_read_reg_ps5268(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static UINT32 sen_get_cmd_tab_ps5268(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = ps5268_mode_1;
		return sizeof(ps5268_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_ps5268(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_ps5268(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ps5268(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_ps5268(0x0116, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_ps5268(id, &cmd);
			cmd = sen_set_cmd_info_ps5268(0x0117, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_ps5268(id, &cmd);
		} else if (p_cmd_list[idx].addr == SEN_CMD_PRESET) {
			switch (preset_ctrl[id].mode) {
				default:
				case ISP_SENSOR_PRESET_DEFAULT:
					if (!sen_get_fae(id, &sensor_ctrl)) {
						DBG_MSG("using default expt/gain \r\n");
						sensor_ctrl.gain_ratio[0] = sen_preset[id].gain_ratio;
						sensor_ctrl.exp_time[0] = sen_preset[id].expt_time;
						if (mode_basic_param[cur_sen_mode[id]].frame_num == 2) {
							sensor_ctrl.exp_time[1] = sen_preset[id].expt_time >> 3;
						}
					}
					break;

				case ISP_SENSOR_PRESET_CHGMODE:
					memcpy(&sensor_ctrl, &sensor_ctrl_last[id], sizeof(ISP_SENSOR_CTRL));
					break;

				case ISP_SENSOR_PRESET_AE:
					sensor_ctrl.exp_time[0] = preset_ctrl[id].exp_time[0];
					sensor_ctrl.exp_time[1] = preset_ctrl[id].exp_time[1];
					sensor_ctrl.gain_ratio[0] = preset_ctrl[id].gain_ratio[0];
					sensor_ctrl.gain_ratio[1] = preset_ctrl[id].gain_ratio[1];
				break;
			}

			// NOTE: Test start
			#if 1
			// NOTE: Test end
			sen_set_gain_ps5268(id, &sensor_ctrl);
			sen_set_expt_ps5268(id, &sensor_ctrl);
			#endif
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
					flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
					flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_ps5268(id, &flip);
		} else {
			cmd = sen_set_cmd_info_ps5268(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_ps5268(id, &cmd);//init table  
			//DBG_ERR("write addr=0x%x, data0=0x%x, data1=0x%x\r\n", p_cmd_list[idx].addr, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);		 
		}
	}

	// NOTE: FAE Start
	//sen_get_fae(id, &sensor_ctrl);
	// NOTE: FAE end

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_ps5268(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ps5268(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_ps5268(0x0116, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_ps5268(id, &cmd);
	cmd = sen_set_cmd_info_ps5268(0x0117, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_ps5268(id, &cmd);
	
	return rt;
}

static ER sen_set_info_ps5268(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_ps5268(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_ps5268(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_ps5268(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_ps5268(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_ps5268(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_ps5268(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_ps5268(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_ps5268((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_ps5268((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_ps5268(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_ps5268((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_ps5268(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_ps5268(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_ps5268((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_ps5268((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_ps5268((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_ps5268(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_ps5268(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_ps5268(id, data);
		break;
	default:
		  rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_ps5268(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_ps5268(id, fps);
	sen_set_cur_fps_ps5268(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_ps5268(id, fps);
		sen_set_cur_fps_ps5268(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_ps5268(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_ps5268(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;

	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	return sensor_vd;
}

static void sen_set_gain_ps5268(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	ER rt = E_OK;
	UINT16 a_gain = 0, d_gain = 0, i = 0;
	static UINT16 sghd = 0;

	sensor_ctrl_last[id].gain_ratio[0] = sensor_ctrl->gain_ratio[0];
	sensor_ctrl_last[id].gain_ratio[1] = sensor_ctrl->gain_ratio[1];

	// Calculate sensor gain
	if (mode_basic_param[cur_sen_mode[id]].frame_num == 0) {
		DBG_WRN("total_frame = 0, force to 1 \r\n");
		total_frame = 1;
	} else {
		total_frame = mode_basic_param[cur_sen_mode[id]].frame_num;
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
			if (sensor_ctrl->gain_ratio[frame_cnt] < (mode_basic_param[cur_sen_mode[id]].gain.min)) {
				sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.min);
			} else if (sensor_ctrl->gain_ratio[frame_cnt] > (mode_basic_param[cur_sen_mode[id]].gain.max)) {
				sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.max);	
			}
	}

	if(gain_wdr_table[i].total_gain > 4000)
	{          	    				
		sghd = 0;//High senstivity mode
		//gain_wdr_table[i].total_gain >>=1;
		sensor_ctrl->gain_ratio[0] = gain_wdr_table[i].total_gain >> 1;
		//DBG_ERR("gain_ratio=%d \r\n", sensor_ctrl->gain_ratio[0]);
	}else if(gain_wdr_table[i].total_gain < 3000)
	{
		sghd = 1;//Low senstivity mode
		//DBG_ERR("Low senstivity mode=%d, total_gain=%d \r\n", sghd, gain_wdr_table[i].total_gain);
	}

	// search most suitable gain into gain table		
	for (i=0; i<NUM_OF_WDRGAINSET; i++) {
		if (gain_wdr_table[i].total_gain > sensor_ctrl->gain_ratio[0])
			break;
	}	

	if(i != 0){
		a_gain = gain_wdr_table[i-1].a_gain;
		d_gain = gain_wdr_table[i-1].d_gain;
		//Write Sensor reg
		cmd = sen_set_cmd_info_ps5268(0x0128, 1, sghd, 0);//sghd		
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x012B, 1, a_gain, 0);//a gain		
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x012A, 1, d_gain, 0);//d gain		
		rt |= sen_write_reg_ps5268(id, &cmd);
	}
	//update flag 
	cmd = sen_set_cmd_info_ps5268(0x0111, 1, 0x01, 0);
	rt |= sen_write_reg_ps5268(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_ps5268(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
	ER rt = E_OK;

	sensor_ctrl_last[id].exp_time[0] = sensor_ctrl->exp_time[0];
	sensor_ctrl_last[id].exp_time[1] = sensor_ctrl->exp_time[1];

	if (mode_basic_param[cur_sen_mode[id]].frame_num == 0) {
		DBG_WRN("total_frame = 0, force to 1 \r\n");
		total_frame = 1;
	} else {
		total_frame = mode_basic_param[cur_sen_mode[id]].frame_num;
	}

	// Calculate exposure line
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Calculates the exposure setting
		t_row = sen_calc_rowtime_ps5268(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;
		
		// Limit minimun exposure line
		if (line[frame_cnt] < MIN_EXPOSURE_LINE) {
			line[frame_cnt] = MIN_EXPOSURE_LINE;
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_ps5268(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_ps5268(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_ps5268(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_ps5268(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		  DBG_ERR("max vts overflow\r\n");
		  sensor_vd = MAX_VD_PERIOD;
	}

	//Write change mode VD
	cmd = sen_set_cmd_info_ps5268(0x0116, 1, (sensor_vd >> 8) & 0xFF, 0);//Cmd_Lpf[15:8]	
	rt |= sen_write_reg_ps5268(id, &cmd);
	cmd = sen_set_cmd_info_ps5268(0x0117, 1, sensor_vd & 0xFF, 0);//Cmd_Lpf[7:0]  
	rt |= sen_write_reg_ps5268(id, &cmd);

	//Check max exp line reg
	if (line[0] > (MAX_VD_PERIOD - NON_EXPOSURE_LINE)) {
		DBG_ERR("max line overflow \r\n");
		line[0] = (MAX_VD_PERIOD - NON_EXPOSURE_LINE);
	}

	//Check min exp line
	if (line[0] < MIN_EXPOSURE_LINE) {
		DBG_ERR("min line overflow\r\n");
		line[0] = MIN_EXPOSURE_LINE;
	}

	//Check max exp line
	if (line[0] > (sensor_vd - NON_EXPOSURE_LINE)) {
		line[0] = sensor_vd - NON_EXPOSURE_LINE;
	}

	// set exposure line to sensor
	cmd = sen_set_cmd_info_ps5268(0x0118, 1, ((sensor_vd - line[0]) >> 8) & 0xFF, 0);//Cmd_OffNy1[15:8]
	rt |= sen_write_reg_ps5268(id, &cmd);
	cmd = sen_set_cmd_info_ps5268(0x0119, 1, ((sensor_vd - line[0]) & 0xFF) , 0);//Cmd_OffNy1[7:0]
	rt |= sen_write_reg_ps5268(id, &cmd);
	cmd = sen_set_cmd_info_ps5268(0x0111, 1, 0x01 , 0);//update flag 
	rt |= sen_write_reg_ps5268(id, &cmd);	 

	//DBG_ERR("sensor_vd=%d, Cmd_OffNy1=%d\r\n", sensor_vd, (sensor_vd - line[0]));
	
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_ps5268(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_ps5268(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;
	
	cmd = sen_set_cmd_info_ps5268(0x0140, 1, 0x0, 0x0);
	rt |= sen_read_reg_ps5268(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {//mirror
		if (*flip & CTL_SEN_FLIP_H) {
				cmd.data[0] |= 0x01;
		} else {
				cmd.data[0] &= (~0x01);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	rt |= sen_write_reg_ps5268(id, &cmd);

	cmd = sen_set_cmd_info_ps5268(0x0111, 1, 0x01 , 0);//update flag 
	rt |= sen_write_reg_ps5268(id, &cmd);

	cmd = sen_set_cmd_info_ps5268(0x0141, 1, 0x0, 0x0);
	rt |= sen_read_reg_ps5268(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {//flip
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x01;
		} else {
			cmd.data[0] &= (~0x01);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	rt |= sen_write_reg_ps5268(id, &cmd);

	if (*flip & CTL_SEN_FLIP_V) {
		cmd = sen_set_cmd_info_ps5268(0x0147, 1, 0x04 , 0);//Vsize[10:8]
		rt |= sen_write_reg_ps5268(id, &cmd); 
		cmd = sen_set_cmd_info_ps5268(0x0148, 1, 0x43 , 0);//Vsize[7:0]
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x0149, 1, 0x04 , 0);//VStart[10:8]
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x014A, 1, 0x44 , 0);//VStart[7:0] 
		rt |= sen_write_reg_ps5268(id, &cmd);
	} else {
		cmd = sen_set_cmd_info_ps5268(0x0147, 1, 0x04 , 0);//Vsize[10:8]
		rt |= sen_write_reg_ps5268(id, &cmd); 
		cmd = sen_set_cmd_info_ps5268(0x0148, 1, 0x43 , 0);//Vsize[7:0]
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x0149, 1, 0x00 , 0);//VStart[10:8]
		rt |= sen_write_reg_ps5268(id, &cmd);
		cmd = sen_set_cmd_info_ps5268(0x014A, 1, 0x02 , 0);//VStart[7:0] 
		rt |= sen_write_reg_ps5268(id, &cmd); 
	}

	cmd = sen_set_cmd_info_ps5268(0x0111, 1, 0x01 , 0);//update flag
	rt |= sen_write_reg_ps5268(id, &cmd);
	
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_ps5268(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_ps5268(0x0140, 1, 0x0, 0x0);
	rt |= sen_read_reg_ps5268(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (cmd.data[0] & 0x01) {
		*flip |= CTL_SEN_FLIP_H;
	}

	cmd = sen_set_cmd_info_ps5268(0x0141, 1, 0x0, 0x0);
	rt |= sen_read_reg_ps5268(id, &cmd);

	if (cmd.data[0] & 0x01) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

static BOOL sen_get_fae(CTL_SEN_ID id, ISP_SENSOR_CTRL *sensor_ctrl)
{
	CTL_SEN_CMD cmd;
	BOOL valid = FALSE;
	UINT32 sensor_vd, exp_time, exp_line, exp_line_h, exp_line_l, gain_idx, gain_ratio, /*hcg,*/ tmp_1 = 0;
	ER rt = E_OK;

	if (fae_enable == 0) {
		return FALSE;
	}

	sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;

	//AE_FW_READY[6]
	cmd = sen_set_cmd_info_ps5268(0x0F37, 1, 0x0, 0x0);
	rt = sen_read_reg_ps5268(id, &cmd);
	if (rt != E_OK) {
		DBG_ERR("read register error %d \r\n", (INT)rt);
		return FALSE;
	}

	//DBG_ERR("0x0F37 = 0x%X \r\n", cmd.data[0]);

	// Check AE_FW_READY, 0xF37[6]
	if (cmd.data[0] & 0x40) {
		//exp
		cmd = sen_set_cmd_info_ps5268(0x0F3C, 1, 0x0, 0x0);//NY[15:8]
		rt = sen_read_reg_ps5268(id, &cmd);
		exp_line_h = cmd.data[0] << 8;
		cmd = sen_set_cmd_info_ps5268(0x0F3D, 1, 0x0, 0x0);//NY[7:0]
		rt = sen_read_reg_ps5268(id, &cmd);
		exp_line_l = cmd.data[0];
		exp_line = sensor_vd - (exp_line_h + exp_line_l);
		exp_time = exp_line * sen_calc_rowtime_ps5268(id, cur_sen_mode[id]) / 10;
		//A gain
		cmd = sen_set_cmd_info_ps5268(0x0F38, 1, 0x0, 0x0);
		rt = sen_read_reg_ps5268(id, &cmd);		
		gain_idx = cmd.data[0] & 0x7F;
		gain_ratio = gain_wdr_table[gain_idx].total_gain;
		//Read AE_SGHD
		cmd = sen_set_cmd_info_ps5268(0x0F37, 1, 0x0, 0x0);//0xF37[0]
		rt = sen_read_reg_ps5268(id, &cmd);
		if((cmd.data[0] & 0x1) == 0){
		    //gain_ratio <<=1;
		    tmp_1 = gain_ratio << 1;
		}
		if (rt != E_OK) {
			DBG_ERR("write register error %d \r\n", (INT)rt);
			return rt;
		}

		//hcg = cmd.data[0] & 0x01;        		

		sensor_ctrl->exp_time[0] = exp_time;
		sensor_ctrl->gain_ratio[0] = tmp_1;
		DBG_MSG("AE_FW_READY, expl %d(%d+%d) , expt %d, gain %d %d, hcg %d\r\n", exp_line,exp_line_h,exp_line_l, exp_time, gain_idx, gain_ratio/*, hcg*/);

		valid = TRUE;
	}


	return valid;
}

static void sen_get_gain_ps5268(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

static void sen_get_expt_ps5268(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_ps5268(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_ps5268(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_ps5268(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_ps5268(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_ps5268(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_ps5268(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_ps5268(id);
	data->chg_fps = sen_get_chgmode_fps_ps5268(id);
}

static void sen_get_speed_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_ps5268(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_ps5268(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->frame_num == 1) {
		if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
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

static UINT32 sen_calc_rowtime_step_ps5268(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 1;
	}/* else {
		div_step = 1;
	}
*/
	return div_step;
}

static UINT32 sen_calc_rowtime_ps5268(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 10 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 1000000);

	//DBG_ERR("hd_period=%d, row_time=%d\r\n",mode_basic_param[mode].signal_info.hd_period, row_time);

	return row_time;
}

static void sen_get_rowtime_ps5268(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_ps5268(id, data->mode);	
	data->row_time = sen_calc_rowtime_ps5268(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_ps5268(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_ps5268(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_ps5268(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_ps5268(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
int sen_init_ps5268(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	ER rt = E_OK;

	sprintf(compatible, "nvt,sen_ps5268");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_ps5268");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ps5268;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ps5268();
	rt = ctl_sen_reg_sendrv("nvt_sen_ps5268", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_ps5268(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ps5268");
}

#else
static int __init sen_init_ps5268(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ps5268;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ps5268();
	rt = ctl_sen_reg_sendrv("nvt_sen_ps5268", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_ps5268(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ps5268");
}

module_init(sen_init_ps5268);
module_exit(sen_exit_ps5268);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_PS5268_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

