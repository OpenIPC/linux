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
#endif

//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_sen_imx317, 1, 43, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_IMX317_MODULE_NAME     "sen_imx317"
#define SEN_MAX_MODE               4
#define MIN_EXPOSURE_LINE          4
#define MAX_VD_PERIOD              0xFFFFF

#define SEN_I2C_ADDR 0x34>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_imx317"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx317(void);
static void sen_pwr_ctrl_imx317(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_imx317(CTL_SEN_ID id);
static ER sen_close_imx317(CTL_SEN_ID id);
static ER sen_sleep_imx317(CTL_SEN_ID id);
static ER sen_wakeup_imx317(CTL_SEN_ID id);
static ER sen_write_reg_imx317(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_imx317(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_imx317(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_imx317(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_imx317(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_imx317(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_imx317(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_imx317(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_imx317(CTL_SEN_ID id, void *param);
static void sen_set_expt_imx317(CTL_SEN_ID id, void *param);
static void sen_set_preset_imx317(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_imx317(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_imx317(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
// TODO:
#if 0
static void sen_set_status_imx317(CTL_SEN_ID id, CTL_SEN_STATUS *status);
#endif
static void sen_get_gain_imx317(CTL_SEN_ID id, void *param);
static void sen_get_expt_imx317(CTL_SEN_ID id, void *param);
static void sen_get_min_expt_imx317(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_imx317(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_imx317(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_imx317(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_imx317(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_imx317(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_imx317(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_imx317(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_imx317(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_imx317(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_imx317(CTL_SEN_ID id);
static void sen_set_chgmode_fps_imx317(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_imx317(CTL_SEN_ID id);
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
	#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x23, 1, 1},
	#else 
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	#endif
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
	SEN_IMX317_MODULE_NAME,
	CTL_SEN_VENDOR_SONY,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_FLIP|CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	1
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x34},
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
		24000000,
		72000000,
		288000000
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		288000000
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		288000000
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		288000000
	},
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{2, 1, 4, 0}, 
		SEN_BIT_OFS_0|SEN_BIT_OFS_1|SEN_BIT_OFS_2|SEN_BIT_OFS_3
	}
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		12000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1932, 1094},
		{{6, 10, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 260, 0, 2310},
		CTL_SEN_RATIO(16, 9),
		{1000, 133861},
		200
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2202},
		{{12, 24, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 520, 0, 4620},
		CTL_SEN_RATIO(16, 9),
		{1000, 133861},
		100
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2202},
		{{12, 24, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 264, 0, 4550},
		CTL_SEN_RATIO(16, 9),
		{1000, 133861},
		100
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_STAGGER_HDR,
		3000,
		2,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2162},
		{{12, 2, 3840, 2160}, {12, 2, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 1052, 0, 2284},
		CTL_SEN_RATIO(16, 9),
		{1000, 133861},
		100
	}
};

static CTL_SEN_CMD imx317_mode_1[] = {
	// standby cancel sequence
	{0x3000, 1, {0x12, 0x00}},
	//{SEN_CMD_DELAY, 1, {0x14, 0x00}},
	// PLRD1~5 input 24MHz
	{0x3120, 1, {0xF0, 0x00}},
	{0x3121, 1, {0x00, 0x00}},
	{0x3122, 1, {0x02, 0x00}},
	{0x3129, 1, {0x9C, 0x00}},
	{0x312A, 1, {0x02, 0x00}},
	{0x312D, 1, {0x02, 0x00}},
	{0x310B, 1, {0x00, 0x00}},
	// PLSTMG
	{0x304C, 1, {0x00, 0x00}},
	{0x304D, 1, {0x03, 0x00}},
	{0x331C, 1, {0x1A, 0x00}},
	{0x331D, 1, {0x00, 0x00}},
	{0x3502, 1, {0x02, 0x00}},
	{0x3529, 1, {0x0E, 0x00}},
	{0x352A, 1, {0x0E, 0x00}},
	{0x352B, 1, {0x0E, 0x00}},
	{0x3538, 1, {0x0E, 0x00}},
	{0x3539, 1, {0x0E, 0x00}},
	{0x3553, 1, {0x00, 0x00}},
	{0x357D, 1, {0x05, 0x00}},
	{0x357F, 1, {0x05, 0x00}},
	{0x3581, 1, {0x04, 0x00}},
	{0x3583, 1, {0x76, 0x00}},
	{0x3587, 1, {0x01, 0x00}},
	{0x35BB, 1, {0x0E, 0x00}},
	{0x35BC, 1, {0x0E, 0x00}},
	{0x35BD, 1, {0x0E, 0x00}},
	{0x35BE, 1, {0x0E, 0x00}},
	{0x35BF, 1, {0x0E, 0x00}},
	{0x366E, 1, {0x00, 0x00}},
	{0x366F, 1, {0x00, 0x00}},
	{0x3670, 1, {0x00, 0x00}},
	{0x3671, 1, {0x00, 0x00}},
	// PSMOVEN
	{0x30EE, 1, {0x01, 0x00}},
	// PSMIPI
	{0x3304, 1, {0x32, 0x00}},
	{0x3305, 1, {0x00, 0x00}},
	{0x3306, 1, {0x32, 0x00}},
	{0x3307, 1, {0x00, 0x00}},
	{0x3590, 1, {0x32, 0x00}},
	{0x3591, 1, {0x00, 0x00}},
	{0x3686, 1, {0x32, 0x00}},
	{0x3687, 1, {0x00, 0x00}},
	// MDSEL
	{0x3004, 1, {0x02, 0x00}},
	{0x3005, 1, {0x21, 0x00}},
	{0x3006, 1, {0x00, 0x00}},
	{0x3007, 1, {0x11, 0x00}},
	{0x3037, 1, {0x00, 0x00}},
	{0x3038, 1, {0x00, 0x00}},
	{0x3039, 1, {0x00, 0x00}},
	{0x303A, 1, {0x00, 0x00}},
	{0x303B, 1, {0x00, 0x00}},
	{0x306B, 1, {0x05, 0x00}},
	{0x30DD, 1, {0x00, 0x00}},
	{0x30DE, 1, {0x00, 0x00}},
	{0x30DF, 1, {0x00, 0x00}},
	{0x30E0, 1, {0x00, 0x00}},
	{0x30E1, 1, {0x00, 0x00}},
	{0x30E2, 1, {0x02, 0x00}},
	{0x30EE, 1, {0x01, 0x00}},
	{0x30F6, 1, {0x04, 0x00}},
	{0x30F7, 1, {0x01, 0x00}},
	{0x30F8, 1, {0x06, 0x00}},
	{0x30F9, 1, {0x09, 0x00}},
	{0x30FA, 1, {0x00, 0x00}},
	{0x3130, 1, {0x4E, 0x00}},
	{0x3131, 1, {0x04, 0x00}},
	{0x3132, 1, {0x46, 0x00}},
	{0x3133, 1, {0x04, 0x00}},
	{0x3342, 1, {0x0A, 0x00}},
	{0x3343, 1, {0x00, 0x00}},
	{0x3344, 1, {0x1A, 0x00}},
	{0x3345, 1, {0x00, 0x00}},
	{0x33A6, 1, {0x01, 0x00}},
	{0x3528, 1, {0x0E, 0x00}},
	{0x3554, 1, {0x00, 0x00}},
	{0x3555, 1, {0x01, 0x00}},
	{0x3556, 1, {0x01, 0x00}},
	{0x3557, 1, {0x01, 0x00}},
	{0x3558, 1, {0x01, 0x00}},
	{0x3559, 1, {0x00, 0x00}},
	{0x355A, 1, {0x00, 0x00}},
	{0x35BA, 1, {0x0E, 0x00}},
	{0x366A, 1, {0x1B, 0x00}},
	{0x366B, 1, {0x1A, 0x00}},
	{0x366C, 1, {0x19, 0x00}},
	{0x366D, 1, {0x17, 0x00}},
	{0x3A41, 1, {0x08, 0x00}},
	{0x3A43, 1, {0x00, 0x00}},//non continue mode 1, continue mode 0
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},	
	{SEN_CMD_PRESET, 1, {0x00, 0x00} },		
	{SEN_CMD_DIRECTION, 1, {0x00, 0x00}},
	{SEN_CMD_DELAY, 1, {0x0A, 0x00}},
	{0x3000, 1, {0x00, 0x00}},// operating
	{0x303E, 1, {0x02, 0x00}},
	{SEN_CMD_DELAY, 1, {0x07, 0x00}},
	{0x30F4, 1, {0x00, 0x00}},
	{0x3018, 1, {0xA2, 0x00}},
};

static CTL_SEN_CMD imx317_mode_2[] = {
	// standby cancel sequence
	{0x3000, 1, {0x12, 0x00}},
	//{SEN_CMD_DELAY, 1, {0x14, 0x00}},
	// PLRD1~5 input 24MHz
	{0x3120, 1, {0xF0, 0x00}},
	{0x3121, 1, {0x00, 0x00}},
	{0x3122, 1, {0x02, 0x00}},
	{0x3129, 1, {0x9C, 0x00}},
	{0x312A, 1, {0x02, 0x00}},
	{0x312D, 1, {0x02, 0x00}},
	{0x310B, 1, {0x00, 0x00}},
	// PLSTMG
	{0x304C, 1, {0x00, 0x00}},
	{0x304D, 1, {0x03, 0x00}},
	{0x331C, 1, {0x1A, 0x00}},
	{0x331D, 1, {0x00, 0x00}},
	{0x3502, 1, {0x02, 0x00}},
	{0x3529, 1, {0x0E, 0x00}},
	{0x352A, 1, {0x0E, 0x00}},
	{0x352B, 1, {0x0E, 0x00}},
	{0x3538, 1, {0x0E, 0x00}},
	{0x3539, 1, {0x0E, 0x00}},
	{0x3553, 1, {0x00, 0x00}},
	{0x357D, 1, {0x05, 0x00}},
	{0x357F, 1, {0x05, 0x00}},
	{0x3581, 1, {0x04, 0x00}},
	{0x3583, 1, {0x76, 0x00}},
	{0x3587, 1, {0x01, 0x00}},
	{0x35BB, 1, {0x0E, 0x00}},
	{0x35BC, 1, {0x0E, 0x00}},
	{0x35BD, 1, {0x0E, 0x00}},
	{0x35BE, 1, {0x0E, 0x00}},
	{0x35BF, 1, {0x0E, 0x00}},
	{0x366E, 1, {0x00, 0x00}},
	{0x366F, 1, {0x00, 0x00}},
	{0x3670, 1, {0x00, 0x00}},
	{0x3671, 1, {0x00, 0x00}},
	// PSMOVEN
	{0x30EE, 1, {0x01, 0x00}},
	// PSMIPI
	{0x3304, 1, {0x32, 0x00}},
	{0x3305, 1, {0x00, 0x00}},
	{0x3306, 1, {0x32, 0x00}},
	{0x3307, 1, {0x00, 0x00}},
	{0x3590, 1, {0x32, 0x00}},
	{0x3591, 1, {0x00, 0x00}},
	{0x3686, 1, {0x32, 0x00}},
	{0x3687, 1, {0x00, 0x00}},
	// MDSEL
	{0x3004, 1, {0x00, 0x00}},
	{0x3005, 1, {0x07, 0x00}},
	{0x3006, 1, {0x00, 0x00}},
	{0x3007, 1, {0x02, 0x00}},
	{0x3037, 1, {0x00, 0x00}},
	{0x3038, 1, {0x00, 0x00}},
	{0x3039, 1, {0x00, 0x00}},
	{0x303A, 1, {0x00, 0x00}},
	{0x303B, 1, {0x00, 0x00}},
	{0x306B, 1, {0x07, 0x00}},
	{0x30DD, 1, {0x00, 0x00}},
	{0x30DE, 1, {0x00, 0x00}},
	{0x30DF, 1, {0x00, 0x00}},
	{0x30E1, 1, {0x00, 0x00}},
	{0x30E2, 1, {0x00, 0x00}},
	{0x30EE, 1, {0x01, 0x00}},
	{0x30F6, 1, {0x08, 0x00}},
	{0x30F7, 1, {0x02, 0x00}},
	{0x30F8, 1, {0x0C, 0x00}},
	{0x30F9, 1, {0x12, 0x00}},
	{0x30FA, 1, {0x00, 0x00}},
	{0x3130, 1, {0xAA, 0x00}},
	{0x3131, 1, {0x08, 0x00}},
	{0x3132, 1, {0x9A, 0x00}},
	{0x3133, 1, {0x08, 0x00}},
	{0x3342, 1, {0xFF, 0x00}},
	{0x3343, 1, {0x01, 0x00}},
	{0x3344, 1, {0xFF, 0x00}},
	{0x3345, 1, {0x01, 0x00}},
	{0x33A6, 1, {0x01, 0x00}},
	{0x3528, 1, {0x0F, 0x00}},
	{0x3554, 1, {0x00, 0x00}},
	{0x3555, 1, {0x00, 0x00}},
	{0x3556, 1, {0x00, 0x00}},
	{0x3557, 1, {0x00, 0x00}},
	{0x3558, 1, {0x00, 0x00}},
	{0x3559, 1, {0x1F, 0x00}},
	{0x355A, 1, {0x1F, 0x00}},
	{0x35BA, 1, {0x0F, 0x00}},
	{0x366A, 1, {0x00, 0x00}},
	{0x366B, 1, {0x00, 0x00}},
	{0x366C, 1, {0x00, 0x00}},
	{0x366D, 1, {0x00, 0x00}},
	{0x3A41, 1, {0x10, 0x00}},
	{0x3A43, 1, {0x00, 0x00}},//non continue mode 1, continue mode 0
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},	
	{SEN_CMD_PRESET, 1, {0x00, 0x00} },		
	{SEN_CMD_DIRECTION, 1, {0x00, 0x00}},
	{SEN_CMD_DELAY, 1, {0x0A, 0x00}},
	{0x3000, 1, {0x00, 0x00}},// operating
	{0x303E, 1, {0x02, 0x00}},
	{SEN_CMD_DELAY, 1, {0x07, 0x00}},
	{0x30F4, 1, {0x00, 0x00}},
	{0x3018, 1, {0xA2, 0x00}},
};

static CTL_SEN_CMD imx317_mode_3[] = {
	// standby cancel sequence
	{0x3000, 1, {0x12, 0x00}},
	//{SEN_CMD_DELAY, 1, {0x14, 0x00}},
	// PLRD1~5 input 24MHz
	{0x3120, 1, {0xF0, 0x00}},
	{0x3121, 1, {0x00, 0x00}},
	{0x3122, 1, {0x02, 0x00}},
	{0x3129, 1, {0x9C, 0x00}},
	{0x312A, 1, {0x02, 0x00}},
	{0x312D, 1, {0x02, 0x00}},
	{0x310B, 1, {0x00, 0x00}},
	// PLSTMG
	{0x304C, 1, {0x00, 0x00}},
	{0x304D, 1, {0x03, 0x00}},
	{0x331C, 1, {0x1A, 0x00}},
	{0x331D, 1, {0x00, 0x00}},
	{0x3502, 1, {0x02, 0x00}},
	{0x3529, 1, {0x0E, 0x00}},
	{0x352A, 1, {0x0E, 0x00}},
	{0x352B, 1, {0x0E, 0x00}},
	{0x3538, 1, {0x0E, 0x00}},
	{0x3539, 1, {0x0E, 0x00}},
	{0x3553, 1, {0x00, 0x00}},
	{0x357D, 1, {0x05, 0x00}},
	{0x357F, 1, {0x05, 0x00}},
	{0x3581, 1, {0x04, 0x00}},
	{0x3583, 1, {0x76, 0x00}},
	{0x3587, 1, {0x01, 0x00}},
	{0x35BB, 1, {0x0E, 0x00}},
	{0x35BC, 1, {0x0E, 0x00}},
	{0x35BD, 1, {0x0E, 0x00}},
	{0x35BE, 1, {0x0E, 0x00}},
	{0x35BF, 1, {0x0E, 0x00}},
	{0x366E, 1, {0x00, 0x00}},
	{0x366F, 1, {0x00, 0x00}},
	{0x3670, 1, {0x00, 0x00}},
	{0x3671, 1, {0x00, 0x00}},
	// PSMOVEN
	{0x30EE, 1, {0x01, 0x00}},
	// PSMIPI
	{0x3304, 1, {0x32, 0x00}},
	{0x3305, 1, {0x00, 0x00}},
	{0x3306, 1, {0x32, 0x00}},
	{0x3307, 1, {0x00, 0x00}},
	{0x3590, 1, {0x32, 0x00}},
	{0x3591, 1, {0x00, 0x00}},
	{0x3686, 1, {0x32, 0x00}},
	{0x3687, 1, {0x00, 0x00}},
	// MDSEL
	{0x3004, 1, {0x01, 0x00}},
	{0x3005, 1, {0x01, 0x00}},
	{0x3006, 1, {0x00, 0x00}},
	{0x3007, 1, {0x02, 0x00}},
	{0x3037, 1, {0x00, 0x00}},
	{0x3038, 1, {0x00, 0x00}},
	{0x3039, 1, {0x00, 0x00}},
	{0x303A, 1, {0x00, 0x00}},
	{0x303B, 1, {0x00, 0x00}},
	{0x306B, 1, {0x05, 0x00}},
	{0x30DD, 1, {0x00, 0x00}},
	{0x30DE, 1, {0x00, 0x00}},
	{0x30DF, 1, {0x00, 0x00}},
	{0x30E0, 1, {0x00, 0x00}},
	{0x30E1, 1, {0x00, 0x00}},
	{0x30E2, 1, {0x01, 0x00}},
	{0x30EE, 1, {0x01, 0x00}},
	{0x30F6, 1, {0x08, 0x00}},
	{0x30F7, 1, {0x01, 0x00}},
	{0x30F8, 1, {0xC6, 0x00}},
	{0x30F9, 1, {0x11, 0x00}},
	{0x30FA, 1, {0x00, 0x00}},
	{0x3130, 1, {0x86, 0x00}},
	{0x3131, 1, {0x08, 0x00}},
	{0x3132, 1, {0x7E, 0x00}},
	{0x3133, 1, {0x08, 0x00}},
	{0x3342, 1, {0x0A, 0x00}},
	{0x3343, 1, {0x00, 0x00}},
	{0x3344, 1, {0x16, 0x00}},
	{0x3345, 1, {0x00, 0x00}},
	{0x33A6, 1, {0x01, 0x00}},
	{0x3528, 1, {0x0E, 0x00}},
	{0x3554, 1, {0x1F, 0x00}},
	{0x3555, 1, {0x01, 0x00}},
	{0x3556, 1, {0x01, 0x00}},
	{0x3557, 1, {0x01, 0x00}},
	{0x3558, 1, {0x01, 0x00}},
	{0x3559, 1, {0x00, 0x00}},
	{0x355A, 1, {0x00, 0x00}},
	{0x35BA, 1, {0x0E, 0x00}},
	{0x366A, 1, {0x1B, 0x00}},
	{0x366B, 1, {0x1A, 0x00}},
	{0x366C, 1, {0x19, 0x00}},
	{0x366D, 1, {0x17, 0x00}},
	{0x3A41, 1, {0x08, 0x00}},
	{0x3A43, 1, {0x00, 0x00}},//non continue mode 1, continue mode 0
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},	
	{SEN_CMD_PRESET, 1, {0x00, 0x00} },		
	{SEN_CMD_DIRECTION, 1, {0x00, 0x00}},
	{SEN_CMD_DELAY, 1, {0x0A, 0x00}},
	{0x3000, 1, {0x00, 0x00}},// operating
	{0x303E, 1, {0x02, 0x00}},
	{SEN_CMD_DELAY, 1, {0x07, 0x00}},
	{0x30F4, 1, {0x00, 0x00}},
	{0x3018, 1, {0xA2, 0x00}},
};

static CTL_SEN_CMD imx317_mode_4[] = {
	// standby cancel sequence
	{0x3000, 1, {0x12, 0x00}},
	//{CMD_DELAY, 1, {0x14, 0x00}},
	// PLRD1~5 input 24MHz
	{0x3120, 1, {0xF0, 0x00}},
	{0x3121, 1, {0x00, 0x00}},
	{0x3122, 1, {0x02, 0x00}},
	{0x3129, 1, {0x9C, 0x00}},
	{0x312A, 1, {0x02, 0x00}},
	{0x312D, 1, {0x02, 0x00}},
	{0x310B, 1, {0x00, 0x00}},
	// PLSTMG
	{0x304C, 1, {0x00, 0x00}},
	{0x304D, 1, {0x03, 0x00}},
	{0x331C, 1, {0x1A, 0x00}},
	{0x331D, 1, {0x00, 0x00}},
	{0x3502, 1, {0x02, 0x00}},
	{0x3529, 1, {0x0E, 0x00}},
	{0x352A, 1, {0x0E, 0x00}},
	{0x352B, 1, {0x0E, 0x00}},
	{0x3538, 1, {0x0E, 0x00}},
	{0x3539, 1, {0x0E, 0x00}},
	{0x3553, 1, {0x00, 0x00}},
	{0x357D, 1, {0x05, 0x00}},
	{0x357F, 1, {0x05, 0x00}},
	{0x3581, 1, {0x04, 0x00}},
	{0x3583, 1, {0x76, 0x00}},
	{0x3587, 1, {0x01, 0x00}},
	{0x35BB, 1, {0x0E, 0x00}},
	{0x35BC, 1, {0x0E, 0x00}},
	{0x35BD, 1, {0x0E, 0x00}},
	{0x35BE, 1, {0x0E, 0x00}},
	{0x35BF, 1, {0x0E, 0x00}},
	{0x366E, 1, {0x00, 0x00}},
	{0x366F, 1, {0x00, 0x00}},
	{0x3670, 1, {0x00, 0x00}},
	{0x3671, 1, {0x00, 0x00}},
	// PSMIPI
	{0x3304, 1, {0x32, 0x00}},
	{0x3305, 1, {0x00, 0x00}},
	{0x3306, 1, {0x32, 0x00}},
	{0x3307, 1, {0x00, 0x00}},
	{0x3590, 1, {0x32, 0x00}},
	{0x3591, 1, {0x00, 0x00}},
	{0x3686, 1, {0x32, 0x00}},
	{0x3687, 1, {0x00, 0x00}},
	// MDSEL
	{0x3004, 1, {0x06, 0x00}},
	{0x3005, 1, {0x01, 0x00}},
	{0x3006, 1, {0x00, 0x00}},
	{0x3007, 1, {0x02, 0x00}},
	{0x300E, 1, {0x00, 0x00}},
	{0x300F, 1, {0x00, 0x00}},
	{0x3019, 1, {0x11, 0x00}},
	{0x301A, 1, {0x00, 0x00}},
	{0x302E, 1, {0x06, 0x00}},
	{0x302F, 1, {0x00, 0x00}},
	{0x3030, 1, {0x1C, 0x00}},
	{0x3031, 1, {0x06, 0x00}},
	{0x3032, 1, {0x56, 0x00}},
	{0x3033, 1, {0x00, 0x00}},
	{0x3041, 1, {0x31, 0x00}},
	{0x3042, 1, {0x07, 0x00}},
	{0x3043, 1, {0x01, 0x00}},
	{0x306B, 1, {0x05, 0x00}},
	{0x30E2, 1, {0x01, 0x00}},
	{0x30E9, 1, {0x01, 0x00}},
	{0x30F6, 1, {0x1C, 0x00}},
	{0x30F7, 1, {0x04, 0x00}},
	{0x30FA, 1, {0x00, 0x00}},
	{0x30EE, 1, {0x01, 0x00}},
	{0x3130, 1, {0x86, 0x00}},
	{0x3131, 1, {0x08, 0x00}},
	{0x3342, 1, {0x0A, 0x00}},
	{0x3343, 1, {0x00, 0x00}},
	{0x3344, 1, {0x16, 0x00}},
	{0x3345, 1, {0x00, 0x00}},
	{0x33A6, 1, {0x01, 0x00}},
	{0x3528, 1, {0x0E, 0x00}},
	{0x3554, 1, {0x1F, 0x00}},
	{0x3555, 1, {0x01, 0x00}},
	{0x3556, 1, {0x01, 0x00}},
	{0x3557, 1, {0x01, 0x00}},
	{0x3558, 1, {0x01, 0x00}},
	{0x3559, 1, {0x00, 0x00}},
	{0x355A, 1, {0x00, 0x00}},
	{0x35BA, 1, {0x0E, 0x00}},
	{0x366A, 1, {0x1B, 0x00}},
	{0x366B, 1, {0x1A, 0x00}},
	{0x366C, 1, {0x19, 0x00}},
	{0x366D, 1, {0x17, 0x00}},
	{0x3A41, 1, {0x08, 0x00}},
	{0x3A43, 1, {0x00, 0x00}},//non continue mode 1, continue mode 0
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},	
	{SEN_CMD_PRESET, 1, {0x00, 0x00} },		
	{SEN_CMD_DIRECTION, 1, {0x00, 0x00}},
	{SEN_CMD_DELAY, 1, {0x0A, 0x00}},
	{0x3000, 1, {0x00, 0x00}},// operating
	{0x303E, 1, {0x02, 0x00}},
	{SEN_CMD_DELAY, 1, {0x07, 0x00}},
	{0x30F4, 1, {0x00, 0x00}},
	{0x3018, 1, {0xA2, 0x00}},
};

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB imx317_sen_drv_tab = {
	sen_open_imx317,
	sen_close_imx317,
	sen_sleep_imx317,
	sen_wakeup_imx317,
	sen_write_reg_imx317,
	sen_read_reg_imx317,
	sen_chg_mode_imx317,
	sen_chg_fps_imx317,
	sen_set_info_imx317,
	sen_get_info_imx317,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx317(void)
{
	return &imx317_sen_drv_tab;
}

static void sen_pwr_ctrl_imx317(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_imx317(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_imx317(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_imx317(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_imx317);
	#endif

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_DEFAULT;

	rt = sen_i2c_init_driver(id, &sen_i2c[id]);

	if (rt != E_OK) {
		i2c_valid[id] = FALSE;

		DBG_ERR("init. i2c driver fail (%d) \r\n", id);
	} else {
		i2c_valid[id] = TRUE;
	}

	return rt;
}

static ER sen_close_imx317(CTL_SEN_ID id)
{
	if (i2c_valid[id]) {
		sen_i2c_remove_driver(id);
		i2c_valid[id] = FALSE;
	}

	return E_OK;
}

static ER sen_sleep_imx317(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_imx317(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_imx317(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static ER sen_read_reg_imx317(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static UINT32 sen_get_cmd_tab_imx317(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = imx317_mode_1;
		return sizeof(imx317_mode_1) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_2:
		*cmd_tab = imx317_mode_2;
		return sizeof(imx317_mode_2) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_3:
		*cmd_tab = imx317_mode_3;
		return sizeof(imx317_mode_3) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_4:
		*cmd_tab = imx317_mode_4;
		return sizeof(imx317_mode_4) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_imx317(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_imx317(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx317(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_imx317(0x30F8, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_imx317(id, &cmd);
			cmd = sen_set_cmd_info_imx317(0x30F9, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_imx317(id, &cmd);
			cmd = sen_set_cmd_info_imx317(0x30FA, 1, (sensor_vd >> 16) & 0x0F, 0x00);
			rt |= sen_write_reg_imx317(id, &cmd);
		} else if (p_cmd_list[idx].addr == SEN_CMD_PRESET) {
			switch (preset_ctrl[id].mode) {
				default:
				case ISP_SENSOR_PRESET_DEFAULT:
					sensor_ctrl.gain_ratio[0] = sen_preset[id].gain_ratio;
					sensor_ctrl.exp_time[0] = sen_preset[id].expt_time;
					if (mode_basic_param[cur_sen_mode[id]].frame_num == 2) {
						sensor_ctrl.exp_time[1] = sen_preset[id].expt_time >> 3;
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

			sen_set_gain_imx317(id, &sensor_ctrl);
			sen_set_expt_imx317(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_imx317(id, &flip);
		} else {
			cmd = sen_set_cmd_info_imx317(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_imx317(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_imx317(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx317(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_imx317(0x30F8, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_imx317(id, &cmd);
	cmd = sen_set_cmd_info_imx317(0x30F9, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_imx317(id, &cmd);
	cmd = sen_set_cmd_info_imx317(0x30FA, 1, (sensor_vd >> 16) & 0x0F, 0x00);
	rt |= sen_write_reg_imx317(id, &cmd);

	return rt;
}

static ER sen_set_info_imx317(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_imx317(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_imx317(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_imx317(id, (CTL_SEN_FLIP *)(data));
		break;
	// TODO:
	#if 0
	case CTL_SENDRV_CFGID_SET_STATUS:
		sen_set_status_imx317(id, (CTL_SEN_STATUS *)(data));
		break;
	#endif
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_imx317(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_imx317(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_imx317(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_imx317(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_imx317((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_imx317((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_imx317(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_imx317((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_imx317(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_imx317(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_imx317((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_imx317((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_imx317((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_imx317(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_imx317(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_imx317(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_imx317(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_imx317(id, fps);
	sen_set_cur_fps_imx317(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_imx317(id, fps);
		sen_set_cur_fps_imx317(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_imx317(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_imx317(CTL_SEN_ID id, UINT32 fps)
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

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	return sensor_vd;
}

static ER get_min_shr_imx317(CTL_SEN_MODE mode,UINT32 frame_cnt, UINT32 *min_shr, UINT32 *min_exp)
{
	ER rt = E_OK;

	switch (mode) {
	case CTL_SEN_MODE_1:// 1920*1080 p30 linear 4lane
		*min_shr = 8;
		*min_exp = 4;
		break;
	case CTL_SEN_MODE_2:// 3840*2160 p30 linear 4lane
	case CTL_SEN_MODE_3:// 3840*2160 p60 linear 4lane
		*min_shr = 12;
		*min_exp = 4;
		break;		
	case CTL_SEN_MODE_4:// 3840*2160 p30 hdr 4lane	
		if (0 == frame_cnt) {
			*min_exp = 4; //long exp
		} else if (1 == frame_cnt) {
			*min_exp = 2; //short exp
		}
		*min_shr = 12; //dont't care
		break;

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		break;
	}
	return rt;
}

static void sen_set_gain_imx317(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	UINT32 digital_gain = 0, analog_gain = 0;
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

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

		sensor_ctrl->gain_ratio[frame_cnt] = sensor_ctrl->gain_ratio[frame_cnt] * 1345 / 1000;

	if ( 22380 >= sensor_ctrl->gain_ratio[frame_cnt] ) {
		digital_gain=0;
		analog_gain=(2048-(2048000/(sensor_ctrl->gain_ratio[frame_cnt])));
	} else if ( 44760 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
		digital_gain=1;
		analog_gain=(2048-(4096000/(sensor_ctrl->gain_ratio[frame_cnt])));
	} else if ( 89520 >= (sensor_ctrl->gain_ratio[frame_cnt]) ) {
		digital_gain=2;
		analog_gain=(2048-(8192000/(sensor_ctrl->gain_ratio[frame_cnt])));
	} else {
		digital_gain=3;
		analog_gain=(2048-(16384000/(sensor_ctrl->gain_ratio[frame_cnt])));
	}

	// collect the gain setting
	data1[frame_cnt] = analog_gain;
	data2[frame_cnt] = digital_gain;
	}
	DBG_IND(" mode %d, gain_ratio[0] %d\r\n", cur_sen_mode[id], sensor_ctrl->gain_ratio[0]);

	// Write analog gain
	cmd = sen_set_cmd_info_imx317(0x300A, 1, data1[0] & 0xFF, 0x0);
	rt |= sen_write_reg_imx317(id, &cmd);
	cmd = sen_set_cmd_info_imx317(0x300B, 1, (data1[0] >> 8) & 0x07, 0x0);
	rt |= sen_write_reg_imx317(id, &cmd);

	// Write digital gain
	cmd = sen_set_cmd_info_imx317(0x3012, 1, (data2[0] & 0x0F), 0);
	rt |= sen_write_reg_imx317(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_imx317(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt = 0, total_frame = 0;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps, cur_fps = 0, clac_fps = 0;
	UINT32 shr = 0, shr2 = 0, rhs1 = 0;
	UINT32 min_shr = 0, min_exp_line = 0, t_row = 0;
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
		t_row = sen_calc_rowtime_imx317(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = 10 * (sensor_ctrl->exp_time[frame_cnt]) / t_row;	
		get_min_shr_imx317(cur_sen_mode[id], frame_cnt, &min_shr, &min_exp_line);

		// Limit minimun exposure line
		if (line[frame_cnt] < min_exp_line) {
			line[frame_cnt] = min_exp_line;
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_imx317(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_imx317(id, cur_sen_mode[id]);
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		expt_time = (line[0]+line[1]) * t_row / 10;
	} else {
		expt_time = (line[0]) * t_row / 10;
	}

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_imx317(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_imx317(id, cur_fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vd overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	if ((mode_basic_param[cur_sen_mode[id]].mode_type) == CTL_SEN_MODE_STAGGER_HDR) {
		// calculate shr, shr2, rhs1
		shr = 6;
		shr2 = sensor_vd  - (line[0]);  
		if (shr2 <= (shr + 2 + 6)) {
			shr2 = shr + 2 + 6; 
		}
		 
		if(shr2 > (sensor_vd  - 4)) {
			shr2 = sensor_vd  - 4;
		}
		 
		rhs1 = shr + (line[1]);
		 
		if (rhs1 > (sensor_vd - 2198)) {
			rhs1 = sensor_vd - 2198;
		}
		 
		if (rhs1 < (shr + 2)) {
			rhs1 = shr + 2;
		}	
	
		cmd = sen_set_cmd_info_imx317(0x302D, 1, 0x01, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		 
		// set VD to sensor
		cmd = sen_set_cmd_info_imx317(0x30F8, 1, sensor_vd & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x30F9, 1, (sensor_vd >> 8 ) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x30FA, 1, (sensor_vd >> 16 )& 0x0F, 0);
		rt |= sen_write_reg_imx317(id, &cmd);

		// set exposure line to sensor (shr1)
		cmd = sen_set_cmd_info_imx317(0x302E, 1, shr & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x302F, 1, (shr >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);

		// set exposure line to sensor (shr2)
		cmd = sen_set_cmd_info_imx317(0x3030, 1, shr2 & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x3031, 1, (shr2 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		
		// set exposure line to sensor (rhs1)
		cmd = sen_set_cmd_info_imx317(0x3032, 1, rhs1 & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x3033, 1, (rhs1 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		
		cmd = sen_set_cmd_info_imx317(0x302D, 1, 0x00, 0);
		rt |= sen_write_reg_imx317(id, &cmd);	 
	} else {
		if (line[0] > (MAX_VD_PERIOD-min_shr)) {
			DBG_ERR("max line overflow\r\n");
			line[0] = MAX_VD_PERIOD - min_shr;
		}

		if (line[0] < min_exp_line) {
			DBG_ERR("min line overflow\r\n");
			line[0] = min_exp_line;
		}

		// Calculate shr
		if (line[0] > (sensor_vd-min_shr)) {
			shr = min_shr;
		} else {
			shr = sensor_vd-line[0];
		}
	
		// Set vmax to sensor
		cmd = sen_set_cmd_info_imx317(0x30F8, 1, sensor_vd & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x30F9, 1, (sensor_vd >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x30FA, 1, (sensor_vd >> 16) & 0x0F, 0);
		rt |= sen_write_reg_imx317(id, &cmd);

		// Set exposure line to sensor (shr)
		cmd = sen_set_cmd_info_imx317(0x300C, 1, shr & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
		cmd = sen_set_cmd_info_imx317(0x300D, 1, (shr >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx317(id, &cmd);
	}
	
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_imx317(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_imx317(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx317(0x301A, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx317(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x1;
		} else {
			cmd.data[0] &= (~0x01);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}
	
	rt |= sen_write_reg_imx317(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_imx317(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx317(0x301A, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx317(id, &cmd);
	
	if (cmd.data[0] & 0x1) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}


#if 0
static void sen_set_status_imx317(CTL_SEN_ID id, CTL_SEN_STATUS *status)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	switch (*status) {
		case CTL_SEN_STATUS_STANDBY:
			cmd = sen_set_cmd_info_imx317(0x3000, 1, 0x01, 0x0);  // STANDBY ON
			rt = sen_write_reg_imx317(id, &cmd);
			if (rt != E_OK) {
				DBG_ERR("write register error %d \r\n", (INT)rt);
			}
			break;

		default:
			DBG_ERR("set status error %d \r\n", *status);
	}
}
#endif

static void sen_get_gain_imx317(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

static void sen_get_expt_imx317(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_imx317(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_imx317(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_imx317(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_imx317(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_imx317(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_imx317(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_imx317(id);
	data->chg_fps = sen_get_chgmode_fps_imx317(id);
}

static void sen_get_speed_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_imx317(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_imx317(CTL_SENDRV_GET_MODESEL_PARAM *data)
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
			if (data->frame_rate <= 12000) {
				data->mode = CTL_SEN_MODE_1;
				return;
			}
		} else if ((data->size.w <= 3840) && (data->size.h <= 2160)) {
			if (data->frame_rate <= 3000) {
				data->mode = CTL_SEN_MODE_2;
				return;
			} else if (data->frame_rate <= 6000) {
				data->mode = CTL_SEN_MODE_3;
				return;
			}
		}
	} else if (data->frame_num == 2) {
		if ((data->size.w <= 3840) && (data->size.h <= 2160)) {
			if (data->frame_rate <= 3000) {
				data->mode = CTL_SEN_MODE_4;
				return;
			}
		}
	}				

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_imx317(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if ((mode_basic_param[mode].mode_type == CTL_SEN_MODE_STAGGER_HDR) || (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR))	{
		div_step = 4;
	}/* else {
		div_step = 4;
	}
*/
	return div_step;
}

static UINT32 sen_calc_rowtime_imx317(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 10 * (mode_basic_param[mode].signal_info.hd_period ) / ((speed_param[mode].pclk) / 1000000);

	return row_time;
}

static void sen_get_rowtime_imx317(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_imx317(id, data->mode);	
	data->row_time = sen_calc_rowtime_imx317(id, data->mode) * (data->row_time_step);	
}

static void sen_set_cur_fps_imx317(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_imx317(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_imx317(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_imx317(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
int sen_init_imx317(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	ER rt = E_OK;

	sprintf(compatible, "nvt,sen_imx317");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_imx317");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx317;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx317();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx317", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_imx317(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx317");
}

#else
static int __init sen_init_imx317(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx317;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx317();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx317", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_imx317(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx317");
}

module_init(sen_init_imx317);
module_exit(sen_exit_imx317);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_IMX317_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

