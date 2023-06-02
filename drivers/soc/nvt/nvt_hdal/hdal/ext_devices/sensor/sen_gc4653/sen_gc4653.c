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
VOS_MODULE_VERSION(nvt_sen_gc4653, 1, 45, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_GC4653_MODULE_NAME    "sen_gc4653"
#define SEN_MAX_MODE              1
#define MAX_VD_PERIOD             0x3FFF
#define MAX_EXPOSURE_LINE         0x3FFF
#define MIN_EXPOSURE_LINE         2
#define NON_EXPOSURE_LINE         4
#define SENSOR_ID                  0x4653
#define SEN_I2C_ADDR 0x52 >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_gc4653"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_gc4653(void);
static void sen_pwr_ctrl_gc4653(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_gc4653(CTL_SEN_ID id);
static ER sen_close_gc4653(CTL_SEN_ID id);
static ER sen_sleep_gc4653(CTL_SEN_ID id);
static ER sen_wakeup_gc4653(CTL_SEN_ID id);
static ER sen_write_reg_gc4653(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_gc4653(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_gc4653(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_gc4653(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_gc4653(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_gc4653(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_gc4653(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_gc4653(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_gc4653(CTL_SEN_ID id, void *param);
static void sen_set_expt_gc4653(CTL_SEN_ID id, void *param);
static void sen_set_preset_gc4653(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_gc4653(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_gc4653(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_gc4653(CTL_SEN_ID id, void *param);
void sen_get_expt_gc4653(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_gc4653(CTL_SEN_ID id, void *param);
static void sen_get_expt_gc4653(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_gc4653(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_gc4653(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_gc4653(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_gc4653(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_gc4653(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_gc4653(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_gc4653(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_gc4653(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_gc4653(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_gc4653(CTL_SEN_ID id);
static void sen_set_chgmode_fps_gc4653(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_gc4653(CTL_SEN_ID id);
static void sen_ext_get_probe_sen_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_PROBE_SEN_PARAM *data);
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
	SEN_GC4653_MODULE_NAME,
	CTL_SEN_VENDOR_OTHERS,
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
		{CTL_SEN_I2C_W_ADDR_DFT,     0x52},
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
		216000000,
		216000000
	},
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
	},
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{2560, 1442},
		{{0, 2, 2560, 1440}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{2560, 1440},
		{0, 4800, 0, 1500},
		CTL_SEN_RATIO(16, 9),
		{1000, 75828*8},
		100
	},
};

static CTL_SEN_CMD gc4653_mode_1[] = {
	/****************************************/
	//version 6.8.5
	//mclk 27Mhz
	//mipi_data_rate 648Mbps
	//framelength 1500
	//linelength 4800
	//pclk 216Mhz
	//rowtime 22.2222us
	//pattern grbg
	{0x03fe, 1, {0xf0, 0x0}},
	{0x03fe, 1, {0x00, 0x0}},
	{0x0317, 1, {0x00, 0x0}},
	{0x0320, 1, {0x77, 0x0}},
	{0x0324, 1, {0xc8, 0x0}},
	{0x0325, 1, {0x06, 0x0}},
	{0x0326, 1, {0x60, 0x0}},
	{0x0327, 1, {0x03, 0x0}},
	{0x0334, 1, {0x40, 0x0}},
	{0x0336, 1, {0x60, 0x0}},
	{0x0337, 1, {0x82, 0x0}},
	{0x0315, 1, {0x25, 0x0}},
	{0x031c, 1, {0xc6, 0x0}},
	{0x0287, 1, {0x18, 0x0}},
	{0x0084, 1, {0x00, 0x0}},
	{0x0087, 1, {0x50, 0x0}},
	{0x029d, 1, {0x08, 0x0}},
	{0x0290, 1, {0x00, 0x0}},
	{0x0340, 1, {0x05, 0x0}},
	{0x0341, 1, {0xdc, 0x0}},
	{0x0345, 1, {0x06, 0x0}},
	{0x034b, 1, {0xb0, 0x0}},
	{0x0352, 1, {0x08, 0x0}},
	{0x0354, 1, {0x07, 0x0}},
	{0x02d1, 1, {0xe0, 0x0}},
	{0x0223, 1, {0xf2, 0x0}},
	{0x0238, 1, {0xa4, 0x0}},
	{0x02ce, 1, {0x7f, 0x0}},
	{0x0232, 1, {0xc4, 0x0}},
	{0x02d3, 1, {0x01, 0x0}},
	{0x0243, 1, {0x06, 0x0}},
	{0x02ee, 1, {0x30, 0x0}},
	{0x026f, 1, {0x70, 0x0}},
	{0x0257, 1, {0x09, 0x0}},
	{0x0211, 1, {0x02, 0x0}},
	{0x0219, 1, {0x09, 0x0}},
	{0x023f, 1, {0x2d, 0x0}},
	{0x0518, 1, {0x00, 0x0}},
	{0x0519, 1, {0x01, 0x0}},
	{0x0515, 1, {0x08, 0x0}},
	{0x02d9, 1, {0x3f, 0x0}},
	{0x02da, 1, {0x02, 0x0}},
	{0x02db, 1, {0xe8, 0x0}},
	{0x02e6, 1, {0x20, 0x0}},
	{0x021b, 1, {0x10, 0x0}},
	{0x0252, 1, {0x22, 0x0}},
	{0x024e, 1, {0x22, 0x0}},
	{0x02c4, 1, {0x01, 0x0}},
	{0x021d, 1, {0x17, 0x0}},
	{0x024a, 1, {0x01, 0x0}},
	{0x02ca, 1, {0x02, 0x0}},
	{0x0262, 1, {0x10, 0x0}},
	{0x029a, 1, {0x20, 0x0}},
	{0x021c, 1, {0x0e, 0x0}},
	{0x0298, 1, {0x03, 0x0}},
	{0x029c, 1, {0x00, 0x0}},
	{0x027e, 1, {0x14, 0x0}},
	{0x02c2, 1, {0x10, 0x0}},
	{0x0540, 1, {0x20, 0x0}},
	{0x0546, 1, {0x01, 0x0}},
	{0x0548, 1, {0x01, 0x0}},
	{0x0544, 1, {0x01, 0x0}},
	{0x0242, 1, {0x1b, 0x0}},
	{0x02c0, 1, {0x1b, 0x0}},
	{0x02c3, 1, {0x20, 0x0}},
	{0x02e4, 1, {0x10, 0x0}},
	{0x022e, 1, {0x00, 0x0}},
	{0x027b, 1, {0x3f, 0x0}},
	{0x0269, 1, {0x0f, 0x0}},
	{0x02d2, 1, {0x40, 0x0}},
	{0x027c, 1, {0x08, 0x0}},
	{0x023a, 1, {0x2e, 0x0}},
	{0x0245, 1, {0xce, 0x0}},
	{0x0530, 1, {0x20, 0x0}},
	{0x0531, 1, {0x02, 0x0}},
	{0x0228, 1, {0x50, 0x0}},
	{0x02ab, 1, {0x00, 0x0}},
	{0x0250, 1, {0x00, 0x0}},
	{0x0221, 1, {0x50, 0x0}},
	{0x02ac, 1, {0x00, 0x0}},
	{0x02a5, 1, {0x02, 0x0}},
	{0x0260, 1, {0x0b, 0x0}},
	{0x0216, 1, {0x04, 0x0}},
	{0x0299, 1, {0x1C, 0x0}},
	{0x02bb, 1, {0x0d, 0x0}},
	{0x02a3, 1, {0x02, 0x0}},
	{0x02a4, 1, {0x02, 0x0}},
	{0x021e, 1, {0x02, 0x0}},
	{0x024f, 1, {0x08, 0x0}},
	{0x028c, 1, {0x08, 0x0}},
	{0x0532, 1, {0x3f, 0x0}},
	{0x0533, 1, {0x02, 0x0}},
	{0x0277, 1, {0xc0, 0x0}},
	{0x0276, 1, {0xc0, 0x0}},
	{0x0239, 1, {0xc0, 0x0}},
	{0x0202, 1, {0x05, 0x0}},
	{0x0203, 1, {0xd0, 0x0}},
	{0x0205, 1, {0xc0, 0x0}},
	{0x02b0, 1, {0x60, 0x0}},
	{0x0002, 1, {0xa9, 0x0}},
	{0x0004, 1, {0x01, 0x0}},
	{0x021a, 1, {0x98, 0x0}},
	{0x0266, 1, {0xa0, 0x0}},
	{0x0020, 1, {0x01, 0x0}},
	{0x0021, 1, {0x03, 0x0}},
	{0x0022, 1, {0x00, 0x0}},
	{0x0023, 1, {0x04, 0x0}},
	{0x0342, 1, {0x06, 0x0}},
	{0x0343, 1, {0x40, 0x0}},
	{0x034e, 1, {0x05,0x00}},// out win hight
	{0x034f, 1, {0xa2,0x00}},// 1442	
	{0x03fe, 1, {0x10, 0x0}},
	{0x03fe, 1, {0x00, 0x0}},
	{0x0106, 1, {0x78, 0x0}},
	{0x0108, 1, {0x0c, 0x0}},
	{0x0114, 1, {0x01, 0x0}},
	{0x0115, 1, {0x12, 0x0}},
	{0x0180, 1, {0x46, 0x0}},
	{0x0181, 1, {0x30, 0x0}},
	{0x0182, 1, {0x05, 0x0}},
	{0x0185, 1, {0x01, 0x0}},
	{0x03fe, 1, {0x10, 0x0}},
	{0x03fe, 1, {0x00, 0x0}},
	{SEN_CMD_SETVD,1,{0x00,0x00}},
	{SEN_CMD_PRESET,1,{0x00,0x00}},
	{SEN_CMD_DIRECTION, 1, {0x00, 0x0}},	
	{0x0100, 1, {0x09, 0x0}},
	{0x0080, 1, {0x02, 0x0}},
	{0x0097, 1, {0x0a, 0x0}},
	{0x0098, 1, {0x10, 0x0}},
	{0x0099, 1, {0x05, 0x0}},
	{0x009a, 1, {0xb0, 0x0}},
	{0x0317, 1, {0x08, 0x0}},
	{0x0a67, 1, {0x80, 0x0}},
	{0x0a70, 1, {0x03, 0x0}},
	{0x0a82, 1, {0x00, 0x0}},
	{0x0a83, 1, {0x10, 0x0}},
	{0x0a80, 1, {0x2b, 0x0}},
	{0x05be, 1, {0x00, 0x0}},
	{0x05a9, 1, {0x01, 0x0}},
	{0x0313, 1, {0x80, 0x0}},
	{0x05be, 1, {0x01, 0x0}},
	{0x0317, 1, {0x00, 0x0}},
	{0x0a67, 1, {0x00, 0x0}},
};


static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB gc4653_sen_drv_tab = {
	sen_open_gc4653,
	sen_close_gc4653,
	sen_sleep_gc4653,
	sen_wakeup_gc4653,
	sen_write_reg_gc4653,
	sen_read_reg_gc4653,
	sen_chg_mode_gc4653,
	sen_chg_fps_gc4653,
	sen_set_info_gc4653,
	sen_get_info_gc4653
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_gc4653(void)
{
	return &gc4653_sen_drv_tab;
}

static void sen_pwr_ctrl_gc4653(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;
	DBG_IND("enter flag %d \r\n", flag);

	if ((flag == CTL_SEN_PWR_CTRL_TURN_ON) && ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr))) {

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
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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
			if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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

static CTL_SEN_CMD sen_set_cmd_info_gc4653(UINT32 addr, UINT32 data)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = 1;
	cmd.data[0] = data;

	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_gc4653(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_gc4653(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_gc4653);
	#endif

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_DEFAULT;
	i2c_valid[id] = TRUE;
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		rt = sen_i2c_init_driver(id, &sen_i2c[id]);

		if (rt != E_OK) {
			i2c_valid[id] = FALSE;

			DBG_ERR("init. i2c driver fail (%d) \r\n", id);
		}
	}

	return rt;
}

static ER sen_close_gc4653(CTL_SEN_ID id)
{
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		if (i2c_valid[id]) {
			sen_i2c_remove_driver(id);
		}
	} else {
		is_fastboot[id] = 0;
		#if defined(__KERNEL__)
		isp_builtin_uninit_i2c(id);
		#endif
	}

	i2c_valid[id] = FALSE;

	return E_OK;
}

static ER sen_sleep_gc4653(CTL_SEN_ID id)
{
	return E_OK;
}

static ER sen_wakeup_gc4653(CTL_SEN_ID id)
{
	return E_OK;
}

static ER sen_write_reg_gc4653(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1) {
			if (sen_i2c_transfer(id, &msgs, 1) == 0) {
				break;
			}
			i++;
			if (i == 5) {
				return E_SYS;
			}
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, &msgs, 1);
		#endif
	}

	return E_OK;
}

static ER sen_read_reg_gc4653(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs[2];
	unsigned char tmp[2], tmp2[2];
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

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1) {
			if (sen_i2c_transfer(id, msgs, 2) == 0) {
				break;
			}
			i++;
			if (i == 5) {
				return E_SYS;
			}
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, msgs, 2);
		#endif
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_gc4653(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = gc4653_mode_1;
		return sizeof(gc4653_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table \r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_gc4653(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	if (is_fastboot[id]) {
		#if defined(__KERNEL__)
		ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl_temp;

		p_sensor_ctrl_temp = isp_builtin_get_sensor_gain(id);
		sensor_ctrl.gain_ratio[0] = p_sensor_ctrl_temp->gain_ratio[0];
		p_sensor_ctrl_temp = isp_builtin_get_sensor_expt(id);
		sensor_ctrl.exp_time[0] = p_sensor_ctrl_temp->exp_time[0];
		sen_set_chgmode_fps_gc4653(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_gc4653(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_gc4653(id, &sensor_ctrl);
		sen_set_expt_gc4653(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_gc4653(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_gc4653(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_gc4653(0x0340, (sensor_vd >> 8) & 0x3F);
			rt |= sen_write_reg_gc4653(id, &cmd);
			cmd = sen_set_cmd_info_gc4653(0x0341, sensor_vd & 0xFF);
			rt |= sen_write_reg_gc4653(id, &cmd);			
		} else if (p_cmd_list[idx].addr == SEN_CMD_PRESET) {
			switch (preset_ctrl[id].mode) {
				default:
				case ISP_SENSOR_PRESET_DEFAULT:
					sensor_ctrl.gain_ratio[0] = sen_preset[id].gain_ratio;
					sensor_ctrl.exp_time[0] = sen_preset[id].expt_time;
					break;

				case ISP_SENSOR_PRESET_CHGMODE:
					memcpy(&sensor_ctrl, &sensor_ctrl_last[id], sizeof(ISP_SENSOR_CTRL));
					break;

				case ISP_SENSOR_PRESET_AE:
					sensor_ctrl.exp_time[0] = preset_ctrl[id].exp_time[0];
					sensor_ctrl.gain_ratio[0] = preset_ctrl[id].gain_ratio[0];
				break;
			}
			sen_set_gain_gc4653(id, &sensor_ctrl);
			sen_set_expt_gc4653(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_gc4653(id, &flip);
		} else {
			cmd = sen_set_cmd_info_gc4653(p_cmd_list[idx].addr, p_cmd_list[idx].data[0]);
			rt |= sen_write_reg_gc4653(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_gc4653(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_gc4653(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_gc4653(0x0340, (sensor_vd >> 8) & 0x3F);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x0341, sensor_vd & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);			

	return rt;
}

static ER sen_set_info_gc4653(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_gc4653(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_gc4653(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_gc4653(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_gc4653(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}

static ER sen_get_info_gc4653(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_gc4653(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_gc4653(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_gc4653((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_gc4653((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_gc4653(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_gc4653((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_gc4653(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_gc4653(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_gc4653((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_gc4653((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_gc4653((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_gc4653(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_gc4653(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_PROBE_SEN:
		sen_ext_get_probe_sen_gc4653(id, (CTL_SENDRV_GET_PROBE_SEN_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_gc4653(id, data);
		break;	
	default:
		rt = E_NOSPT;
	}

	return rt;
}

static UINT32 sen_calc_chgmode_vd_gc4653(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_gc4653(id, fps);
	sen_set_cur_fps_gc4653(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_gc4653(id, fps);
		sen_set_cur_fps_gc4653(id, fps);
	}

	if (sensor_vd < mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_gc4653(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_gc4653(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;

	}

	if (sensor_vd < mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	return sensor_vd;
}

typedef struct gain_set {
	UINT32 gain_reg1;
	UINT32 gain_reg2;
	UINT32 gain_reg3;
	UINT32 gain_reg4;
	UINT32 gain_reg5;
	UINT32 gain_reg6;
	UINT32 gain_reg7;
	UINT32 total_gain;
} gain_set_t;

static gain_set_t gain_table[] = // gain*1000
{
	//2b3   2b4   2b8    2b9   515    519   2d9
	{0x00, 0x00, 0x01, 0x00, 0x30, 0x1e, 0x5C,  1000},
	{0x20, 0x00, 0x01, 0x0B, 0x30, 0x1e, 0x5C,  1172},
	{0x01, 0x00, 0x01, 0x19, 0x30, 0x1d, 0x5B,  1391},
	{0x21, 0x00, 0x01, 0x2A, 0x30, 0x1e, 0x5C,  1656},
	{0x02, 0x00, 0x02, 0x00, 0x30, 0x1e, 0x5C,  2000},
	{0x22, 0x00, 0x02, 0x17, 0x30, 0x1d, 0x5B,  2359},
	{0x03, 0x00, 0x02, 0x33, 0x20, 0x16, 0x54,  2797},
	{0x23, 0x00, 0x03, 0x14, 0x20, 0x17, 0x55,  3313},
	{0x04, 0x00, 0x04, 0x00, 0x20, 0x17, 0x55,  4000},
	{0x24, 0x00, 0x04, 0x2F, 0x20, 0x19, 0x57,  4734},
	{0x05, 0x00, 0x05, 0x26, 0x20, 0x19, 0x57,  5594},
	{0x25, 0x00, 0x06, 0x28, 0x20, 0x1b, 0x59,  6625},
	{0x0c, 0x00, 0x08, 0x00, 0x20, 0x1d, 0x5B,  8000},
	{0x2C, 0x00, 0x09, 0x1E, 0x20, 0x1f, 0x5D,  9469},
	{0x0D, 0x00, 0x0B, 0x0C, 0x20, 0x21, 0x5F, 11188},
	{0x2D, 0x00, 0x0D, 0x11, 0x20, 0x24, 0x62, 13266},
	{0x1C, 0x00, 0x10, 0x00, 0x20, 0x26, 0x64, 16000},
	{0x3C, 0x00, 0x12, 0x3D, 0x18, 0x2a, 0x68, 18953},
	{0x5C, 0x00, 0x16, 0x19, 0x18, 0x2c, 0x6A, 22391},
	{0x7C, 0x00, 0x1A, 0x22, 0x18, 0x2e, 0x6C, 26531},
	{0x9C, 0x00, 0x20, 0x00, 0x18, 0x32, 0x70, 32000},
	{0xBC, 0x00, 0x25, 0x3A, 0x18, 0x35, 0x73, 37906},
	{0xDC, 0x00, 0x2C, 0x33, 0x10, 0x36, 0x74, 44797},
	{0xFC, 0x00, 0x35, 0x05, 0x10, 0x38, 0x76, 53078},
	{0x1C, 0x01, 0x40, 0x00, 0x10, 0x3c, 0x7A, 64000},
	{0x3C, 0x01, 0x4B, 0x35, 0x10, 0x42, 0x80, 75828}
};
#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

static void sen_set_gain_gc4653(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 i, tbl_idx = NUM_OF_GAINSET-1, digital_gain = 0, decimal = 0, gain_data[3];
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	if (sensor_ctrl->gain_ratio[0] < mode_basic_param[cur_sen_mode[id]].gain.min) {
		sensor_ctrl->gain_ratio[0] = mode_basic_param[cur_sen_mode[id]].gain.min;
	} else if (sensor_ctrl->gain_ratio[0] > mode_basic_param[cur_sen_mode[id]].gain.max) {
		sensor_ctrl->gain_ratio[0] = mode_basic_param[cur_sen_mode[id]].gain.max;
	}

	sensor_ctrl_last[id].gain_ratio[0] = sensor_ctrl->gain_ratio[0];

	for (i = 0; i <= (NUM_OF_GAINSET-1); i++) {
		if (gain_table[i].total_gain > sensor_ctrl->gain_ratio[0]) {
			tbl_idx = i;
			break;
		}
	}

	// collect the gain setting
	if (tbl_idx < 1) {
		digital_gain = sensor_ctrl->gain_ratio[0] * 1000 / gain_table[0].total_gain;
		gain_data[0] = (gain_table[0].gain_reg6 << 16) | (gain_table[0].gain_reg1 << 8) | (gain_table[0].gain_reg2);
		gain_data[1] = (gain_table[0].gain_reg7 << 16) | (gain_table[0].gain_reg3 << 8) | (gain_table[0].gain_reg4);
		gain_data[2] = (gain_table[0].gain_reg5 << 16);
	} else {
		digital_gain = sensor_ctrl->gain_ratio[0] * 1000 / gain_table[tbl_idx-1].total_gain;
		gain_data[0] = (gain_table[tbl_idx-1].gain_reg6 << 16) | (gain_table[tbl_idx-1].gain_reg1 << 8) | (gain_table[tbl_idx-1].gain_reg2);
		gain_data[1] = (gain_table[tbl_idx-1].gain_reg7 << 16) | (gain_table[tbl_idx-1].gain_reg3 << 8) | (gain_table[tbl_idx-1].gain_reg4);
		gain_data[2] = (gain_table[tbl_idx-1].gain_reg5 << 16);
	}

	if (digital_gain <= 1000) {
		decimal = 64;
	} else {
		decimal = digital_gain * 64 / 1000;
	}

	cmd = sen_set_cmd_info_gc4653(0x02B3, (gain_data[0] >> 8) & 0xFF);
	rt = sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x02B4, gain_data[0] & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x02B8, (gain_data[1] >> 8) & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x02B9, gain_data[1] & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x0515, (gain_data[2] >> 16) & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x0519, (gain_data[0] >> 16) & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x02D9, (gain_data[1] >> 16) & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x020E, (decimal >> 6) & 0x0F);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x020F, (decimal & 0x3F) << 2);
	rt |= sen_write_reg_gc4653(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_gc4653(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line = 0;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
	ER rt = E_OK;

	sensor_ctrl_last[id].exp_time[0] = sensor_ctrl->exp_time[0];

	// calculate exposure line
	t_row = sen_calc_rowtime_gc4653(id, cur_sen_mode[id]);
	if (t_row == 0) {
		DBG_WRN("t_row = 0, must >= 1 \r\n");
		t_row = 1;
	}
	line = sensor_ctrl->exp_time[0] * 10 / t_row;

	// limit minimun exposure line
	if (line < MIN_EXPOSURE_LINE) {
		line = MIN_EXPOSURE_LINE;
	}

	// write exposure line
	// get fps
	chgmode_fps = sen_get_chgmode_fps_gc4653(id);

	// calculate exposure time
	t_row = sen_calc_rowtime_gc4653(id, cur_sen_mode[id]);
	expt_time = line * t_row / 10;

	// calculate fps
	if (expt_time == 0) {
		DBG_WRN("expt_time = 0, must >= 1 \r\n");
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_gc4653(id, cur_fps);

	// calculate new vd
	sensor_vd = sen_calc_exp_vd_gc4653(id, cur_fps);

	// check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow \r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	// write change mode VD
	cmd = sen_set_cmd_info_gc4653(0x0340, (sensor_vd >> 8) & 0x3F);
	rt = sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x0341, sensor_vd & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);

	// check max exp line reg
	if (line > MAX_EXPOSURE_LINE) {
		DBG_ERR("max line overflow \r\n");
		line = MAX_EXPOSURE_LINE;
	}

	// check max exp line
	if (line > (sensor_vd - NON_EXPOSURE_LINE)) {
		line = sensor_vd - NON_EXPOSURE_LINE;
	}

	// set exposure line to sensor
	cmd = sen_set_cmd_info_gc4653(0x0202, (line >> 8) & 0x3F);
	rt |= sen_write_reg_gc4653(id, &cmd);
	cmd = sen_set_cmd_info_gc4653(0x0203, line & 0xFF);
	rt |= sen_write_reg_gc4653(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_gc4653(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_gc4653(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	UINT32 temp_reg = 0;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_gc4653(0x0101, 0x00);
	rt = sen_read_reg_gc4653(id, &cmd);
	temp_reg = cmd.data[0];

	cmd = sen_set_cmd_info_gc4653(0x31d, 0x2d);
	rt = sen_write_reg_gc4653(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
				temp_reg |= 0x01;
		} else {
				temp_reg &= (~0x01);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			temp_reg |= 0x02;
		} else {
			temp_reg &= (~0x02);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	cmd = sen_set_cmd_info_gc4653(0x0101, temp_reg);
	rt |= sen_write_reg_gc4653(id, &cmd);

	cmd = sen_set_cmd_info_gc4653(0x31d, 0x29);
	rt |= sen_write_reg_gc4653(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_gc4653(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_gc4653(0x0101, 0x00);
	rt = sen_read_reg_gc4653(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;


	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (cmd.data[0] & 0x01) {
			*flip |= CTL_SEN_FLIP_H;
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (cmd.data[0] & 0x02) {
			*flip |= CTL_SEN_FLIP_V;
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_gc4653(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_gc4653(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
}

#if defined(__FREERTOS)
void sen_get_expt_gc4653(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_gc4653(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
}

static void sen_get_min_expt_gc4653(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_gc4653(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_gc4653(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}

	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_gc4653(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_gc4653(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_gc4653(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		return E_OK;
	}

	return E_NOSPT;
}

static void sen_ext_get_probe_sen_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_PROBE_SEN_PARAM *data)
{
	CTL_SEN_CMD cmd;
	UINT32 read_data_h = 0, read_data_l = 0, sensor_id = 0;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_gc4653(0x3F0, 0x00);
	rt |= sen_read_reg_gc4653(id, &cmd);
	read_data_h = cmd.data[0];

	cmd = sen_set_cmd_info_gc4653(0x3F1, 0x00);
	rt |= sen_read_reg_gc4653(id, &cmd);
	read_data_l = cmd.data[0];

	sensor_id = ((read_data_h & 0xFF) << 8) | (read_data_l & 0xFF);

	if (SENSOR_ID == sensor_id) {
		data->probe_rst = 0;
	} else if (rt != E_OK) { 
		data->probe_rst = rt;
	} else { // (SENSOR_ID != sensor_id) {
		data->probe_rst = -1;
	} 
}

static void sen_get_fps_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_gc4653(id);
	data->chg_fps = sen_get_chgmode_fps_gc4653(id);
}

static void sen_get_speed_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}

	memcpy(data, &speed_param[mode], sizeof(CTL_SENDRV_GET_SPEED_PARAM));

	if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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

static void sen_get_mode_mipi_gc4653(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}

	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_gc4653(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if ((data->size.w <= 2560) && (data->size.h <= 1440)) {
		if (data->frame_rate <= 3000) {
			data->mode = CTL_SEN_MODE_1;
			return;
		}
	}

	data->mode = CTL_SEN_MODE_1;

	DBG_ERR("fail (frame_rate%d, size%d*%d, if_type%d, data_fmt%d) \r\n", data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt);
}

static UINT32 sen_calc_rowtime_gc4653(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	// precision * 100
	row_time = 100 * mode_basic_param[mode].signal_info.hd_period / (speed_param[mode].pclk / 100000);

	return row_time;
}

static void sen_get_rowtime_gc4653(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = 1;
	data->row_time = sen_calc_rowtime_gc4653(id, data->mode) * data->row_time_step;
}

static void sen_set_cur_fps_gc4653(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_gc4653(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_gc4653(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_gc4653(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_gc4653(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_gc4653(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_gc4653(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX ; id++ ) {
		is_fastboot[id] = 0;
		fastboot_i2c_id[id] = 0xFFFFFFFF;
		fastboot_i2c_addr[id] = 0x0;
	}

	sprintf(compatible, "nvt,sen_gc4653");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_gc4653");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_gc4653;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_gc4653();
	rt = ctl_sen_reg_sendrv("nvt_sen_gc4653", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_gc4653(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_gc4653");
}

#else
static int __init sen_init_gc4653(void)
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	CFG_FILE_FMT *pcfg_file;
	CTL_SEN_REG_OBJ reg_obj;
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++ ) {
		is_fastboot[id] = kdrv_builtin_is_fastboot();
		fastboot_i2c_id[id] = isp_builtin_get_i2c_id(id);
		fastboot_i2c_addr[id] = isp_builtin_get_i2c_addr(id);
	}

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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_gc4653;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_gc4653();
	rt = ctl_sen_reg_sendrv("nvt_sen_gc4653", &reg_obj);

	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_gc4653(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_gc4653");
}

module_init(sen_init_gc4653);
module_exit(sen_exit_gc4653);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_GC4653_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

