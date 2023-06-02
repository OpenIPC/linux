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
VOS_MODULE_VERSION(nvt_sen_imx415, 1, 44, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_IMX415_MODULE_NAME     "sen_imx415"
#define SEN_MAX_MODE               5
#define MAX_VD_PERIOD              0xFFFFF
#define BRL_ALL_PIXEL              2228
#define BRL_BINNING                1115
#define SENSOR_ID                  0x514

#define SEN_I2C_ADDR 0x34>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_imx415"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx415(void);
static void sen_pwr_ctrl_imx415(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_imx415(CTL_SEN_ID id);
static ER sen_close_imx415(CTL_SEN_ID id);
static ER sen_sleep_imx415(CTL_SEN_ID id);
static ER sen_wakeup_imx415(CTL_SEN_ID id);
static ER sen_write_reg_imx415(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_imx415(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_imx415(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_imx415(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_imx415(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_imx415(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_imx415(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_imx415(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_imx415(CTL_SEN_ID id, void *param);
static void sen_set_expt_imx415(CTL_SEN_ID id, void *param);
static void sen_set_preset_imx415(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_imx415(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_imx415(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_imx415(CTL_SEN_ID id, void *param);
void sen_get_expt_imx415(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_imx415(CTL_SEN_ID id, void *param);
static void sen_get_expt_imx415(CTL_SEN_ID id, void *param);
#endif
// TODO:
#if 0
static void sen_set_status_imx415(CTL_SEN_ID id, CTL_SEN_STATUS *status);
#endif
static void sen_get_min_expt_imx415(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_imx415(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_imx415(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_imx415(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_imx415(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_imx415(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_imx415(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_imx415(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_imx415(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_imx415(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_imx415(CTL_SEN_ID id);
static void sen_set_chgmode_fps_imx415(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_imx415(CTL_SEN_ID id);
static void sen_ext_get_probe_sen_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_PROBE_SEN_PARAM *data);
static void sen_get_hdr_rhs_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM *data);
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
	SEN_IMX415_MODULE_NAME,
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
		144000000
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
		27000000,
		74250000,
		148500000
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		144000000
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		288000000
	}
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
		{1, 0, 0, 0},
		SEN_BIT_OFS_0|SEN_BIT_OFS_1
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
		CTL_SEN_DATALANE_2,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
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
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2190},
		{{12, 21, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 1066, 0, 2251},
		CTL_SEN_RATIO(16, 9),
		{1000, 3981000},
		100
	},	
	{
		CTL_SEN_MODE_2,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_STAGGER_HDR,
		3000,
		2,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2190},
		{{12, 21, 3840, 2160}, {12, 21, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 532, 0, 2256},
		CTL_SEN_RATIO(16, 9),
		{1000, 3981000},
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
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1932, 1095},
		{{6, 9, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 550, 0, 2250},
		CTL_SEN_RATIO(16, 9),
		{1000, 3981000},
		200
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2190},
		{{12, 21, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 1066, 0, 2251},
		CTL_SEN_RATIO(16, 9),
		{1000, 3981000},
		100
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3864, 2190},
		{{12, 21, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 533, 0, 2251},
		CTL_SEN_RATIO(16, 9),
		{1000, 3981000},
		100
	}
};

static CTL_SEN_CMD imx415_mode_1[] = {
	// MCLK=24Mhz MIPI 4lane 3840*2160 30FPS 10bit
	{0x3000, 1, {0x01, 0x0}}, // standby
	{0x3002, 1, {0x01, 0x0}}, //Master mode stop
	{0x3008, 1, {0x54, 0x0}},
	{0x300A, 1, {0x3B, 0x0}},
	{0x3024, 1, {0xCB, 0x0}},
	{0x3028, 1, {0x2A, 0x0}},
	{0x3029, 1, {0x04, 0x0}},
	{0x3031, 1, {0x00, 0x0}},
	{0x3032, 1, {0x00, 0x0}},
	{0x3033, 1, {0x09, 0x0}},
	{0x3050, 1, {0x08, 0x0}},
	{0x30C1, 1, {0x00, 0x0}},
	{0x3116, 1, {0x23, 0x0}},
	{0x3118, 1, {0xB4, 0x0}},
	{0x311A, 1, {0xFC, 0x0}},
	{0x311E, 1, {0x23, 0x0}},
	{0x32D4, 1, {0x21, 0x0}},
	{0x32EC, 1, {0xA1, 0x0}},
	{0x344C, 1, {0x2B, 0x0}},
	{0x344D, 1, {0x01, 0x0}},
	{0x344E, 1, {0xED, 0x0}},
	{0x344F, 1, {0x01, 0x0}},
	{0x3450, 1, {0xF6, 0x0}},
	{0x3451, 1, {0x02, 0x0}},
	{0x3452, 1, {0x7F, 0x0}},
	{0x3453, 1, {0x03, 0x0}},
	{0x358A, 1, {0x04, 0x0}},
	{0x35A1, 1, {0x02, 0x0}},
	{0x36BC, 1, {0x0C, 0x0}},
	{0x36CC, 1, {0x53, 0x0}},
	{0x36CD, 1, {0x00, 0x0}},
	{0x36CE, 1, {0x3C, 0x0}},
	{0x36D0, 1, {0x8C, 0x0}},
	{0x36D1, 1, {0x00, 0x0}},
	{0x36D2, 1, {0x71, 0x0}},
	{0x36D4, 1, {0x3C, 0x0}},
	{0x36D6, 1, {0x53, 0x0}},
	{0x36D7, 1, {0x00, 0x0}},
	{0x36D8, 1, {0x71, 0x0}},
	{0x36DA, 1, {0x8C, 0x0}},
	{0x36DB, 1, {0x00, 0x0}},
	{0x3701, 1, {0x00, 0x0}},
	{0x3720, 1, {0x00, 0x0}},
	{0x3724, 1, {0x02, 0x0}},
	{0x3726, 1, {0x02, 0x0}},
	{0x3732, 1, {0x02, 0x0}},
	{0x3734, 1, {0x03, 0x0}},
	{0x3736, 1, {0x03, 0x0}},
	{0x3742, 1, {0x03, 0x0}},
	{0x3862, 1, {0xE0, 0x0}},
	{0x38CC, 1, {0x30, 0x0}},
	{0x38CD, 1, {0x2F, 0x0}},
	{0x395C, 1, {0x0C, 0x0}},
	{0x39A4, 1, {0x07, 0x0}},
	{0x39A8, 1, {0x32, 0x0}},
	{0x39AA, 1, {0x32, 0x0}},
	{0x39AC, 1, {0x32, 0x0}},
	{0x39AE, 1, {0x32, 0x0}},
	{0x39B0, 1, {0x32, 0x0}},
	{0x39B2, 1, {0x2F, 0x0}},
	{0x39B4, 1, {0x2D, 0x0}},
	{0x39B6, 1, {0x28, 0x0}},
	{0x39B8, 1, {0x30, 0x0}},
	{0x39BA, 1, {0x30, 0x0}},
	{0x39BC, 1, {0x30, 0x0}},
	{0x39BE, 1, {0x30, 0x0}},
	{0x39C0, 1, {0x30, 0x0}},
	{0x39C2, 1, {0x2E, 0x0}},
	{0x39C4, 1, {0x2B, 0x0}},
	{0x39C6, 1, {0x25, 0x0}},
	{0x3A42, 1, {0xD1, 0x0}},
	{0x3A4C, 1, {0x77, 0x0}},
	{0x3AE0, 1, {0x02, 0x0}},
	{0x3AEC, 1, {0x0C, 0x0}},
	{0x3B00, 1, {0x2E, 0x0}},
	{0x3B06, 1, {0x29, 0x0}},
	{0x3B98, 1, {0x25, 0x0}},
	{0x3B99, 1, {0x21, 0x0}},
	{0x3B9B, 1, {0x13, 0x0}},
	{0x3B9C, 1, {0x13, 0x0}},
	{0x3B9D, 1, {0x13, 0x0}},
	{0x3B9E, 1, {0x13, 0x0}},
	{0x3BA1, 1, {0x00, 0x0}},
	{0x3BA2, 1, {0x06, 0x0}},
	{0x3BA3, 1, {0x0B, 0x0}},
	{0x3BA4, 1, {0x10, 0x0}},
	{0x3BA5, 1, {0x14, 0x0}},
	{0x3BA6, 1, {0x18, 0x0}},
	{0x3BA7, 1, {0x1A, 0x0}},
	{0x3BA8, 1, {0x1A, 0x0}},
	{0x3BA9, 1, {0x1A, 0x0}},
	{0x3BAC, 1, {0xED, 0x0}},
	{0x3BAD, 1, {0x01, 0x0}},
	{0x3BAE, 1, {0xF6, 0x0}},
	{0x3BAF, 1, {0x02, 0x0}},
	{0x3BB0, 1, {0xA2, 0x0}},
	{0x3BB1, 1, {0x03, 0x0}},
	{0x3BB2, 1, {0xE0, 0x0}},
	{0x3BB3, 1, {0x03, 0x0}},
	{0x3BB4, 1, {0xE0, 0x0}},
	{0x3BB5, 1, {0x03, 0x0}},
	{0x3BB6, 1, {0xE0, 0x0}},
	{0x3BB7, 1, {0x03, 0x0}},
	{0x3BB8, 1, {0xE0, 0x0}},
	{0x3BBA, 1, {0xE0, 0x0}},
	{0x3BBC, 1, {0xDA, 0x0}},
	{0x3BBE, 1, {0x88, 0x0}},
	{0x3BC0, 1, {0x44, 0x0}},
	{0x3BC2, 1, {0x7B, 0x0}},
	{0x3BC4, 1, {0xA2, 0x0}},
	{0x3BC8, 1, {0xBD, 0x0}},
	{0x3BCA, 1, {0xBD, 0x0}},
	{0x4001, 1, {0x03, 0x0}},	
	{0x4004, 1, {0x00, 0x0}},
	{0x4005, 1, {0x06, 0x0}},
	{0x400C, 1, {0x00, 0x0}},
	{0x4018, 1, {0x6F, 0x0}},
	{0x401A, 1, {0x2F, 0x0}},
	{0x401C, 1, {0x2F, 0x0}},
	{0x401E, 1, {0xBF, 0x0}},
	{0x401F, 1, {0x00, 0x0}},
	{0x4020, 1, {0x2F, 0x0}},
	{0x4022, 1, {0x57, 0x0}},
	{0x4024, 1, {0x2F, 0x0}},
	{0x4026, 1, {0x4F, 0x0}},
	{0x4028, 1, {0x27, 0x0}},
	{0x4074, 1, {0x01, 0x0}},
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},
	{0x3000, 1, {0x00, 0x0}},  // standby cancel
	{SEN_CMD_DELAY, 1, {25, 0x0}},
	{0x3002, 1, {0x00, 0x0}},  //Master mode start
};

static CTL_SEN_CMD imx415_mode_2[] = {
	// MCLK=24Mhz MIPI 4lane 3840x2160 30FPS 10bit 2-frame dol hdr
	{0x3000, 1, {0x01, 0x0}}, // standby
	{0x3002, 1, {0x01, 0x0}}, //Master mode stop
	{0x3008, 1, {0x54, 0x0}},
	{0x300A, 1, {0x3B, 0x0}},
	{0x3024, 1, {0xD0, 0x0}},
	{0x3028, 1, {0x14, 0x0}},
	{0x3029, 1, {0x02, 0x0}},
	{0x302C, 1, {0x01, 0x0}},
	{0x302D, 1, {0x01, 0x0}},
	{0x3031, 1, {0x00, 0x0}},
	{0x3032, 1, {0x00, 0x0}},
	{0x3033, 1, {0x08, 0x0}},
	{0x3050, 1, {0x18, 0x0}},
	{0x3051, 1, {0x11, 0x0}},
	{0x3054, 1, {0x09, 0x0}},
	{0x3060, 1, {0x19, 0x0}},
	{0x30C1, 1, {0x00, 0x0}},
	{0x30CF, 1, {0x01, 0x0}},
	{0x3116, 1, {0x23, 0x0}},
	{0x3118, 1, {0xB4, 0x0}},
	{0x311A, 1, {0xFC, 0x0}},
	{0x311E, 1, {0x23, 0x0}},
	{0x32D4, 1, {0x21, 0x0}},
	{0x32EC, 1, {0xA1, 0x0}},
	{0x344C, 1, {0x2B, 0x0}},
	{0x344D, 1, {0x01, 0x0}},
	{0x344E, 1, {0xED, 0x0}},
	{0x344F, 1, {0x01, 0x0}},
	{0x3450, 1, {0xF6, 0x0}},
	{0x3451, 1, {0x02, 0x0}},
	{0x3452, 1, {0x7F, 0x0}},
	{0x3453, 1, {0x03, 0x0}},
	{0x358A, 1, {0x04, 0x0}},
	{0x35A1, 1, {0x02, 0x0}},
	{0x36BC, 1, {0x0C, 0x0}},
	{0x36CC, 1, {0x53, 0x0}},
	{0x36CD, 1, {0x00, 0x0}},
	{0x36CE, 1, {0x3C, 0x0}},
	{0x36D0, 1, {0x8C, 0x0}},
	{0x36D1, 1, {0x00, 0x0}},
	{0x36D2, 1, {0x71, 0x0}},
	{0x36D4, 1, {0x3C, 0x0}},
	{0x36D6, 1, {0x53, 0x0}},
	{0x36D7, 1, {0x00, 0x0}},
	{0x36D8, 1, {0x71, 0x0}},
	{0x36DA, 1, {0x8C, 0x0}},
	{0x36DB, 1, {0x00, 0x0}},
	{0x3701, 1, {0x00, 0x0}},
	{0x3720, 1, {0x00, 0x0}},
	{0x3724, 1, {0x02, 0x0}},
	{0x3726, 1, {0x02, 0x0}},
	{0x3732, 1, {0x02, 0x0}},
	{0x3734, 1, {0x03, 0x0}},
	{0x3736, 1, {0x03, 0x0}},
	{0x3742, 1, {0x03, 0x0}},
	{0x3862, 1, {0xE0, 0x0}},
	{0x38CC, 1, {0x30, 0x0}},
	{0x38CD, 1, {0x2F, 0x0}},
	{0x395C, 1, {0x0C, 0x0}},
	{0x39A4, 1, {0x07, 0x0}},
	{0x39A8, 1, {0x32, 0x0}},
	{0x39AA, 1, {0x32, 0x0}},
	{0x39AC, 1, {0x32, 0x0}},
	{0x39AE, 1, {0x32, 0x0}},
	{0x39B0, 1, {0x32, 0x0}},
	{0x39B2, 1, {0x2F, 0x0}},
	{0x39B4, 1, {0x2D, 0x0}},
	{0x39B6, 1, {0x28, 0x0}},
	{0x39B8, 1, {0x30, 0x0}},
	{0x39BA, 1, {0x30, 0x0}},
	{0x39BC, 1, {0x30, 0x0}},
	{0x39BE, 1, {0x30, 0x0}},
	{0x39C0, 1, {0x30, 0x0}},
	{0x39C2, 1, {0x2E, 0x0}},
	{0x39C4, 1, {0x2B, 0x0}},
	{0x39C6, 1, {0x25, 0x0}},
	{0x3A42, 1, {0xD1, 0x0}},
	{0x3A4C, 1, {0x77, 0x0}},
	{0x3AE0, 1, {0x02, 0x0}},
	{0x3AEC, 1, {0x0C, 0x0}},
	{0x3B00, 1, {0x2E, 0x0}},
	{0x3B06, 1, {0x29, 0x0}},
	{0x3B98, 1, {0x25, 0x0}},
	{0x3B99, 1, {0x21, 0x0}},
	{0x3B9B, 1, {0x13, 0x0}},
	{0x3B9C, 1, {0x13, 0x0}},
	{0x3B9D, 1, {0x13, 0x0}},
	{0x3B9E, 1, {0x13, 0x0}},
	{0x3BA1, 1, {0x00, 0x0}},
	{0x3BA2, 1, {0x06, 0x0}},
	{0x3BA3, 1, {0x0B, 0x0}},
	{0x3BA4, 1, {0x10, 0x0}},
	{0x3BA5, 1, {0x14, 0x0}},
	{0x3BA6, 1, {0x18, 0x0}},
	{0x3BA7, 1, {0x1A, 0x0}},
	{0x3BA8, 1, {0x1A, 0x0}},
	{0x3BA9, 1, {0x1A, 0x0}},
	{0x3BAC, 1, {0xED, 0x0}},
	{0x3BAD, 1, {0x01, 0x0}},
	{0x3BAE, 1, {0xF6, 0x0}},
	{0x3BAF, 1, {0x02, 0x0}},
	{0x3BB0, 1, {0xA2, 0x0}},
	{0x3BB1, 1, {0x03, 0x0}},
	{0x3BB2, 1, {0xE0, 0x0}},
	{0x3BB3, 1, {0x03, 0x0}},
	{0x3BB4, 1, {0xE0, 0x0}},
	{0x3BB5, 1, {0x03, 0x0}},
	{0x3BB6, 1, {0xE0, 0x0}},
	{0x3BB7, 1, {0x03, 0x0}},
	{0x3BB8, 1, {0xE0, 0x0}},
	{0x3BBA, 1, {0xE0, 0x0}},
	{0x3BBC, 1, {0xDA, 0x0}},
	{0x3BBE, 1, {0x88, 0x0}},
	{0x3BC0, 1, {0x44, 0x0}},
	{0x3BC2, 1, {0x7B, 0x0}},
	{0x3BC4, 1, {0xA2, 0x0}},
	{0x3BC8, 1, {0xBD, 0x0}},
	{0x3BCA, 1, {0xBD, 0x0}},
	{0x4001, 1, {0x03, 0x0}},
	{0x4004, 1, {0x00, 0x0}},
	{0x4005, 1, {0x06, 0x0}},
	{0x400C, 1, {0x01, 0x0}},
	{0x4018, 1, {0x9F, 0x0}},
	{0x401A, 1, {0x57, 0x0}},
	{0x401C, 1, {0x57, 0x0}},
	{0x401E, 1, {0x87, 0x0}},
	{0x401F, 1, {0x01, 0x0}},
	{0x4020, 1, {0x5F, 0x0}},
	{0x4022, 1, {0xA7, 0x0}},
	{0x4024, 1, {0x5F, 0x0}},
	{0x4026, 1, {0x97, 0x0}},
	{0x4028, 1, {0x4F, 0x0}},
	{0x4074, 1, {0x00, 0x0}},
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},
	{0x3000, 1, {0x00, 0x0}},  // standby cancel
	{SEN_CMD_DELAY, 1, {25, 0x0}},
	{0x3002, 1, {0x00, 0x0}},  //Master mode start
};

static CTL_SEN_CMD imx415_mode_3[] = {
	// MCLK=24Mhz MIPI 4lane 1920*1080 60FPS 10bit
	{0x3000, 1, {0x01, 0x0}}, // standby
	{0x3002, 1, {0x01, 0x0}}, //Master mode stop
	{0x3008, 1, {0x5D, 0x0}},
	{0x300A, 1, {0x42, 0x0}},
	{0x3020, 1, {0x01, 0x0}}, //horizontal 2 binning
	{0x3021, 1, {0x01, 0x0}}, //vertical 2 binning
	{0x3022, 1, {0x01, 0x0}}, //h/v 2/2-line binning
	{0x3024, 1, {0xCA, 0x0}}, //VMAX
	{0x3025, 1, {0x08, 0x0}},
	{0x3028, 1, {0x26, 0x0}}, //HMAX
	{0x3029, 1, {0x02, 0x0}},
	{0x3031, 1, {0x00, 0x0}},
	{0x3032, 1, {0x01, 0x0}}, //12bit
	{0x3033, 1, {0x05, 0x0}},
	{0x3050, 1, {0x08, 0x0}},
	{0x30C1, 1, {0x00, 0x0}},
	{0x30D9, 1, {0x02, 0x0}},
	{0x30DA, 1, {0x01, 0x0}},
	{0x3116, 1, {0x23, 0x0}},
	{0x3118, 1, {0xC6, 0x0}},
	{0x311A, 1, {0xE7, 0x0}},
	{0x311E, 1, {0x23, 0x0}},
	{0x32D4, 1, {0x21, 0x0}},
	{0x32EC, 1, {0xA1, 0x0}},
	{0x344C, 1, {0x2B, 0x0}},
	{0x344D, 1, {0x01, 0x0}},
	{0x344E, 1, {0xED, 0x0}},
	{0x344F, 1, {0x01, 0x0}},
	{0x3450, 1, {0xF6, 0x0}},
	{0x3451, 1, {0x02, 0x0}},
	{0x3452, 1, {0x7F, 0x0}},
	{0x3453, 1, {0x03, 0x0}},
	{0x358A, 1, {0x04, 0x0}},
	{0x35A1, 1, {0x02, 0x0}},
	{0x36BC, 1, {0x0C, 0x0}},
	{0x36CC, 1, {0x53, 0x0}},
	{0x36CD, 1, {0x00, 0x0}},
	{0x36CE, 1, {0x3C, 0x0}},
	{0x36D0, 1, {0x8C, 0x0}},
	{0x36D1, 1, {0x00, 0x0}},
	{0x36D2, 1, {0x71, 0x0}},
	{0x36D4, 1, {0x3C, 0x0}},
	{0x36D6, 1, {0x53, 0x0}},
	{0x36D7, 1, {0x00, 0x0}},
	{0x36D8, 1, {0x71, 0x0}},
	{0x36DA, 1, {0x8C, 0x0}},
	{0x36DB, 1, {0x00, 0x0}},
	{0x3701, 1, {0x00, 0x0}},
	{0x3720, 1, {0x00, 0x0}},
	{0x3724, 1, {0x02, 0x0}},
	{0x3726, 1, {0x02, 0x0}},
	{0x3732, 1, {0x02, 0x0}},
	{0x3734, 1, {0x03, 0x0}},
	{0x3736, 1, {0x03, 0x0}},
	{0x3742, 1, {0x03, 0x0}},
	{0x3862, 1, {0xE0, 0x0}},
	{0x38CC, 1, {0x30, 0x0}},
	{0x38CD, 1, {0x2F, 0x0}},
	{0x395C, 1, {0x0C, 0x0}},
	{0x39A4, 1, {0x07, 0x0}},
	{0x39A8, 1, {0x32, 0x0}},
	{0x39AA, 1, {0x32, 0x0}},
	{0x39AC, 1, {0x32, 0x0}},
	{0x39AE, 1, {0x32, 0x0}},
	{0x39B0, 1, {0x32, 0x0}},
	{0x39B2, 1, {0x2F, 0x0}},
	{0x39B4, 1, {0x2D, 0x0}},
	{0x39B6, 1, {0x28, 0x0}},
	{0x39B8, 1, {0x30, 0x0}},
	{0x39BA, 1, {0x30, 0x0}},
	{0x39BC, 1, {0x30, 0x0}},
	{0x39BE, 1, {0x30, 0x0}},
	{0x39C0, 1, {0x30, 0x0}},
	{0x39C2, 1, {0x2E, 0x0}},
	{0x39C4, 1, {0x2B, 0x0}},
	{0x39C6, 1, {0x25, 0x0}},
	{0x3A42, 1, {0xD1, 0x0}},
	{0x3A4C, 1, {0x77, 0x0}},
	{0x3AE0, 1, {0x02, 0x0}},
	{0x3AEC, 1, {0x0C, 0x0}},
	{0x3B00, 1, {0x2E, 0x0}},
	{0x3B06, 1, {0x29, 0x0}},
	{0x3B98, 1, {0x25, 0x0}},
	{0x3B99, 1, {0x21, 0x0}},
	{0x3B9B, 1, {0x13, 0x0}},
	{0x3B9C, 1, {0x13, 0x0}},
	{0x3B9D, 1, {0x13, 0x0}},
	{0x3B9E, 1, {0x13, 0x0}},
	{0x3BA1, 1, {0x00, 0x0}},
	{0x3BA2, 1, {0x06, 0x0}},
	{0x3BA3, 1, {0x0B, 0x0}},
	{0x3BA4, 1, {0x10, 0x0}},
	{0x3BA5, 1, {0x14, 0x0}},
	{0x3BA6, 1, {0x18, 0x0}},
	{0x3BA7, 1, {0x1A, 0x0}},
	{0x3BA8, 1, {0x1A, 0x0}},
	{0x3BA9, 1, {0x1A, 0x0}},
	{0x3BAC, 1, {0xED, 0x0}},
	{0x3BAD, 1, {0x01, 0x0}},
	{0x3BAE, 1, {0xF6, 0x0}},
	{0x3BAF, 1, {0x02, 0x0}},
	{0x3BB0, 1, {0xA2, 0x0}},
	{0x3BB1, 1, {0x03, 0x0}},
	{0x3BB2, 1, {0xE0, 0x0}},
	{0x3BB3, 1, {0x03, 0x0}},
	{0x3BB4, 1, {0xE0, 0x0}},
	{0x3BB5, 1, {0x03, 0x0}},
	{0x3BB6, 1, {0xE0, 0x0}},
	{0x3BB7, 1, {0x03, 0x0}},
	{0x3BB8, 1, {0xE0, 0x0}},
	{0x3BBA, 1, {0xE0, 0x0}},
	{0x3BBC, 1, {0xDA, 0x0}},
	{0x3BBE, 1, {0x88, 0x0}},
	{0x3BC0, 1, {0x44, 0x0}},
	{0x3BC2, 1, {0x7B, 0x0}},
	{0x3BC4, 1, {0xA2, 0x0}},
	{0x3BC8, 1, {0xBD, 0x0}},
	{0x3BCA, 1, {0xBD, 0x0}},
	{0x4004, 1, {0xC0, 0x0}},
	{0x4001, 1, {0x03, 0x0}},		
	{0x4005, 1, {0x06, 0x0}},
	{0x400C, 1, {0x00, 0x0}},
	{0x4018, 1, {0x7F, 0x0}},
	{0x401A, 1, {0x37, 0x0}},
	{0x401C, 1, {0x37, 0x0}},
	{0x401E, 1, {0xF7, 0x0}},
	{0x401F, 1, {0x00, 0x0}},
	{0x4020, 1, {0x3F, 0x0}},
	{0x4022, 1, {0x6F, 0x0}},
	{0x4024, 1, {0x3F, 0x0}},
	{0x4026, 1, {0x5F, 0x0}},
	{0x4028, 1, {0x2F, 0x0}},
	{0x4074, 1, {0x01, 0x0}},
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},
	{0x3000, 1, {0x00, 0x0}},  // standby cancel
	{SEN_CMD_DELAY, 1, {25, 0x0}},
	{0x3002, 1, {0x00, 0x0}},  //Master mode start
};

static CTL_SEN_CMD imx415_mode_4[] = {
	// MCLK=24Mhz MIPI 2lane 3840*2160 30FPS 10bit
	{0x3000, 1, {0x01, 0x0}}, // standby
	{0x3002, 1, {0x01, 0x0}}, //Master mode stop
	{0x3008, 1, {0x54, 0x0}},
	{0x300A, 1, {0x3B, 0x0}},
	{0x3024, 1, {0xCB, 0x0}},
	{0x3028, 1, {0x2A, 0x0}},
	{0x3029, 1, {0x04, 0x0}},
	{0x3031, 1, {0x00, 0x0}},
	{0x3032, 1, {0x00, 0x0}},
	{0x3033, 1, {0x08, 0x0}},
	{0x3050, 1, {0x08, 0x0}},
	{0x30C1, 1, {0x00, 0x0}},
	{0x3116, 1, {0x23, 0x0}},
	{0x3118, 1, {0xB4, 0x0}},
	{0x311A, 1, {0xFC, 0x0}},
	{0x311E, 1, {0x23, 0x0}},
	{0x32D4, 1, {0x21, 0x0}},
	{0x32EC, 1, {0xA1, 0x0}},
	{0x344C, 1, {0x2B, 0x0}},
	{0x344D, 1, {0x01, 0x0}},
	{0x344E, 1, {0xED, 0x0}},
	{0x344F, 1, {0x01, 0x0}},
	{0x3450, 1, {0xF6, 0x0}},
	{0x3451, 1, {0x02, 0x0}},
	{0x3452, 1, {0x7F, 0x0}},
	{0x3453, 1, {0x03, 0x0}},
	{0x358A, 1, {0x04, 0x0}},
	{0x35A1, 1, {0x02, 0x0}},
	{0x36BC, 1, {0x0C, 0x0}},
	{0x36CC, 1, {0x53, 0x0}},
	{0x36CD, 1, {0x00, 0x0}},
	{0x36CE, 1, {0x3C, 0x0}},
	{0x36D0, 1, {0x8C, 0x0}},
	{0x36D1, 1, {0x00, 0x0}},
	{0x36D2, 1, {0x71, 0x0}},
	{0x36D4, 1, {0x3C, 0x0}},
	{0x36D6, 1, {0x53, 0x0}},
	{0x36D7, 1, {0x00, 0x0}},
	{0x36D8, 1, {0x71, 0x0}},
	{0x36DA, 1, {0x8C, 0x0}},
	{0x36DB, 1, {0x00, 0x0}},
	{0x3701, 1, {0x00, 0x0}},
	{0x3720, 1, {0x00, 0x0}},
	{0x3724, 1, {0x02, 0x0}},
	{0x3726, 1, {0x02, 0x0}},
	{0x3732, 1, {0x02, 0x0}},
	{0x3734, 1, {0x03, 0x0}},
	{0x3736, 1, {0x03, 0x0}},
	{0x3742, 1, {0x03, 0x0}},
	{0x3862, 1, {0xE0, 0x0}},
	{0x38CC, 1, {0x30, 0x0}},
	{0x38CD, 1, {0x2F, 0x0}},
	{0x395C, 1, {0x0C, 0x0}},
	{0x39A4, 1, {0x07, 0x0}},
	{0x39A8, 1, {0x32, 0x0}},
	{0x39AA, 1, {0x32, 0x0}},
	{0x39AC, 1, {0x32, 0x0}},
	{0x39AE, 1, {0x32, 0x0}},
	{0x39B0, 1, {0x32, 0x0}},
	{0x39B2, 1, {0x2F, 0x0}},
	{0x39B4, 1, {0x2D, 0x0}},
	{0x39B6, 1, {0x28, 0x0}},
	{0x39B8, 1, {0x30, 0x0}},
	{0x39BA, 1, {0x30, 0x0}},
	{0x39BC, 1, {0x30, 0x0}},
	{0x39BE, 1, {0x30, 0x0}},
	{0x39C0, 1, {0x30, 0x0}},
	{0x39C2, 1, {0x2E, 0x0}},
	{0x39C4, 1, {0x2B, 0x0}},
	{0x39C6, 1, {0x25, 0x0}},
	{0x3A42, 1, {0xD1, 0x0}},
	{0x3A4C, 1, {0x77, 0x0}},
	{0x3AE0, 1, {0x02, 0x0}},
	{0x3AEC, 1, {0x0C, 0x0}},
	{0x3B00, 1, {0x2E, 0x0}},
	{0x3B06, 1, {0x29, 0x0}},
	{0x3B98, 1, {0x25, 0x0}},
	{0x3B99, 1, {0x21, 0x0}},
	{0x3B9B, 1, {0x13, 0x0}},
	{0x3B9C, 1, {0x13, 0x0}},
	{0x3B9D, 1, {0x13, 0x0}},
	{0x3B9E, 1, {0x13, 0x0}},
	{0x3BA1, 1, {0x00, 0x0}},
	{0x3BA2, 1, {0x06, 0x0}},
	{0x3BA3, 1, {0x0B, 0x0}},
	{0x3BA4, 1, {0x10, 0x0}},
	{0x3BA5, 1, {0x14, 0x0}},
	{0x3BA6, 1, {0x18, 0x0}},
	{0x3BA7, 1, {0x1A, 0x0}},
	{0x3BA8, 1, {0x1A, 0x0}},
	{0x3BA9, 1, {0x1A, 0x0}},
	{0x3BAC, 1, {0xED, 0x0}},
	{0x3BAD, 1, {0x01, 0x0}},
	{0x3BAE, 1, {0xF6, 0x0}},
	{0x3BAF, 1, {0x02, 0x0}},
	{0x3BB0, 1, {0xA2, 0x0}},
	{0x3BB1, 1, {0x03, 0x0}},
	{0x3BB2, 1, {0xE0, 0x0}},
	{0x3BB3, 1, {0x03, 0x0}},
	{0x3BB4, 1, {0xE0, 0x0}},
	{0x3BB5, 1, {0x03, 0x0}},
	{0x3BB6, 1, {0xE0, 0x0}},
	{0x3BB7, 1, {0x03, 0x0}},
	{0x3BB8, 1, {0xE0, 0x0}},
	{0x3BBA, 1, {0xE0, 0x0}},
	{0x3BBC, 1, {0xDA, 0x0}},
	{0x3BBE, 1, {0x88, 0x0}},
	{0x3BC0, 1, {0x44, 0x0}},
	{0x3BC2, 1, {0x7B, 0x0}},
	{0x3BC4, 1, {0xA2, 0x0}},
	{0x3BC8, 1, {0xBD, 0x0}},
	{0x3BCA, 1, {0xBD, 0x0}},
	{0x4001, 1, {0x01, 0x0}},	
	{0x4004, 1, {0x00, 0x0}},
	{0x4005, 1, {0x06, 0x0}},
	{0x400C, 1, {0x01, 0x0}},
	{0x4018, 1, {0x9F, 0x0}},
	{0x401A, 1, {0x57, 0x0}},
	{0x401C, 1, {0x57, 0x0}},
	{0x401E, 1, {0x87, 0x0}},
	{0x401F, 1, {0x01, 0x0}},
	{0x4020, 1, {0x5F, 0x0}},
	{0x4022, 1, {0xA7, 0x0}},
	{0x4024, 1, {0x5F, 0x0}},
	{0x4026, 1, {0x97, 0x0}},
	{0x4028, 1, {0x4F, 0x0}},
	{0x4074, 1, {0x00, 0x0}},
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},
	{0x3000, 1, {0x00, 0x0}},  // standby cancel
	{SEN_CMD_DELAY, 1, {25, 0x0}},
	{0x3002, 1, {0x00, 0x0}},  //Master mode start
};

static CTL_SEN_CMD imx415_mode_5[] = {
	// MCLK=24Mhz MIPI 4lane 3840*2160 60FPS 10bit
	{0x3000, 1, {0x01, 0x0}}, // standby
	{0x3002, 1, {0x01, 0x0}}, //Master mode stop
	{0x3008, 1, {0x54, 0x0}},
	{0x300A, 1, {0x3B, 0x0}},
	{0x3024, 1, {0xCB, 0x0}},
	{0x3028, 1, {0x15, 0x0}},
	{0x3029, 1, {0x02, 0x0}},
	{0x3031, 1, {0x00, 0x0}},
	{0x3032, 1, {0x00, 0x0}},
	{0x3033, 1, {0x08, 0x0}},
	{0x3050, 1, {0x08, 0x0}},
	{0x30C1, 1, {0x00, 0x0}},
	{0x3116, 1, {0x23, 0x0}},
	{0x3118, 1, {0xB4, 0x0}},
	{0x311A, 1, {0xFC, 0x0}},
	{0x311E, 1, {0x23, 0x0}},
	{0x32D4, 1, {0x21, 0x0}},
	{0x32EC, 1, {0xA1, 0x0}},
	{0x344C, 1, {0x2B, 0x0}},
	{0x344D, 1, {0x01, 0x0}},
	{0x344E, 1, {0xED, 0x0}},
	{0x344F, 1, {0x01, 0x0}},
	{0x3450, 1, {0xF6, 0x0}},
	{0x3451, 1, {0x02, 0x0}},
	{0x3452, 1, {0x7F, 0x0}},
	{0x3453, 1, {0x03, 0x0}},
	{0x358A, 1, {0x04, 0x0}},
	{0x35A1, 1, {0x02, 0x0}},
	{0x36BC, 1, {0x0C, 0x0}},
	{0x36CC, 1, {0x53, 0x0}},
	{0x36CD, 1, {0x00, 0x0}},
	{0x36CE, 1, {0x3C, 0x0}},
	{0x36D0, 1, {0x8C, 0x0}},
	{0x36D1, 1, {0x00, 0x0}},
	{0x36D2, 1, {0x71, 0x0}},
	{0x36D4, 1, {0x3C, 0x0}},
	{0x36D6, 1, {0x53, 0x0}},
	{0x36D7, 1, {0x00, 0x0}},
	{0x36D8, 1, {0x71, 0x0}},
	{0x36DA, 1, {0x8C, 0x0}},
	{0x36DB, 1, {0x00, 0x0}},
	{0x3701, 1, {0x00, 0x0}},
	{0x3720, 1, {0x00, 0x0}},
	{0x3724, 1, {0x02, 0x0}},
	{0x3726, 1, {0x02, 0x0}},
	{0x3732, 1, {0x02, 0x0}},
	{0x3734, 1, {0x03, 0x0}},
	{0x3736, 1, {0x03, 0x0}},
	{0x3742, 1, {0x03, 0x0}},
	{0x3862, 1, {0xE0, 0x0}},
	{0x38CC, 1, {0x30, 0x0}},
	{0x38CD, 1, {0x2F, 0x0}},
	{0x395C, 1, {0x0C, 0x0}},
	{0x39A4, 1, {0x07, 0x0}},
	{0x39A8, 1, {0x32, 0x0}},
	{0x39AA, 1, {0x32, 0x0}},
	{0x39AC, 1, {0x32, 0x0}},
	{0x39AE, 1, {0x32, 0x0}},
	{0x39B0, 1, {0x32, 0x0}},
	{0x39B2, 1, {0x2F, 0x0}},
	{0x39B4, 1, {0x2D, 0x0}},
	{0x39B6, 1, {0x28, 0x0}},
	{0x39B8, 1, {0x30, 0x0}},
	{0x39BA, 1, {0x30, 0x0}},
	{0x39BC, 1, {0x30, 0x0}},
	{0x39BE, 1, {0x30, 0x0}},
	{0x39C0, 1, {0x30, 0x0}},
	{0x39C2, 1, {0x2E, 0x0}},
	{0x39C4, 1, {0x2B, 0x0}},
	{0x39C6, 1, {0x25, 0x0}},
	{0x3A42, 1, {0xD1, 0x0}},
	{0x3A4C, 1, {0x77, 0x0}},
	{0x3AE0, 1, {0x02, 0x0}},
	{0x3AEC, 1, {0x0C, 0x0}},
	{0x3B00, 1, {0x2E, 0x0}},
	{0x3B06, 1, {0x29, 0x0}},
	{0x3B98, 1, {0x25, 0x0}},
	{0x3B99, 1, {0x21, 0x0}},
	{0x3B9B, 1, {0x13, 0x0}},
	{0x3B9C, 1, {0x13, 0x0}},
	{0x3B9D, 1, {0x13, 0x0}},
	{0x3B9E, 1, {0x13, 0x0}},
	{0x3BA1, 1, {0x00, 0x0}},
	{0x3BA2, 1, {0x06, 0x0}},
	{0x3BA3, 1, {0x0B, 0x0}},
	{0x3BA4, 1, {0x10, 0x0}},
	{0x3BA5, 1, {0x14, 0x0}},
	{0x3BA6, 1, {0x18, 0x0}},
	{0x3BA7, 1, {0x1A, 0x0}},
	{0x3BA8, 1, {0x1A, 0x0}},
	{0x3BA9, 1, {0x1A, 0x0}},
	{0x3BAC, 1, {0xED, 0x0}},
	{0x3BAD, 1, {0x01, 0x0}},
	{0x3BAE, 1, {0xF6, 0x0}},
	{0x3BAF, 1, {0x02, 0x0}},
	{0x3BB0, 1, {0xA2, 0x0}},
	{0x3BB1, 1, {0x03, 0x0}},
	{0x3BB2, 1, {0xE0, 0x0}},
	{0x3BB3, 1, {0x03, 0x0}},
	{0x3BB4, 1, {0xE0, 0x0}},
	{0x3BB5, 1, {0x03, 0x0}},
	{0x3BB6, 1, {0xE0, 0x0}},
	{0x3BB7, 1, {0x03, 0x0}},
	{0x3BB8, 1, {0xE0, 0x0}},
	{0x3BBA, 1, {0xE0, 0x0}},
	{0x3BBC, 1, {0xDA, 0x0}},
	{0x3BBE, 1, {0x88, 0x0}},
	{0x3BC0, 1, {0x44, 0x0}},
	{0x3BC2, 1, {0x7B, 0x0}},
	{0x3BC4, 1, {0xA2, 0x0}},
	{0x3BC8, 1, {0xBD, 0x0}},
	{0x3BCA, 1, {0xBD, 0x0}},
	{0x4001, 1, {0x03, 0x0}},	
	{0x4004, 1, {0x00, 0x0}},
	{0x4005, 1, {0x06, 0x0}},
	{0x400C, 1, {0x01, 0x0}},
	{0x4018, 1, {0x9F, 0x0}},
	{0x401A, 1, {0x57, 0x0}},
	{0x401C, 1, {0x57, 0x0}},
	{0x401E, 1, {0x87, 0x0}},
	{0x401F, 1, {0x01, 0x0}},
	{0x4020, 1, {0x5F, 0x0}},
	{0x4022, 1, {0xA7, 0x0}},
	{0x4024, 1, {0x5F, 0x0}},
	{0x4026, 1, {0x97, 0x0}},
	{0x4028, 1, {0x4F, 0x0}},
	{0x4074, 1, {0x00, 0x0}},
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},
	{0x3000, 1, {0x00, 0x0}},  // standby cancel
	{SEN_CMD_DELAY, 1, {25, 0x0}},
	{0x3002, 1, {0x00, 0x0}},  //Master mode start
};

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static UINT32 compensation_ratio[CTL_SEN_ID_MAX][ISP_SEN_MFRAME_MAX_NUM] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB imx415_sen_drv_tab = {
	sen_open_imx415,
	sen_close_imx415,
	sen_sleep_imx415,
	sen_wakeup_imx415,
	sen_write_reg_imx415,
	sen_read_reg_imx415,
	sen_chg_mode_imx415,
	sen_chg_fps_imx415,
	sen_set_info_imx415,
	sen_get_info_imx415,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx415(void)
{
	return &imx415_sen_drv_tab;
}

static void sen_pwr_ctrl_imx415(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_imx415(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_imx415(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_imx415(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_imx415);
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

static ER sen_close_imx415(CTL_SEN_ID id)
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

static ER sen_sleep_imx415(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_imx415(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_imx415(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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
		while(1){
			if (sen_i2c_transfer(id, &msgs, 1) == 0)
				break;
			i++;
			if (i == 5)
				return E_SYS;
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, &msgs, 1);
		#endif
	}

	return E_OK;
}

static ER sen_read_reg_imx415(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1){
			if (sen_i2c_transfer(id, msgs, 2) == 0)
				break;
			i++;
			if (i == 5)
				return E_SYS;
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, msgs, 2);
		#endif
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_imx415(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = imx415_mode_1;
		return sizeof(imx415_mode_1) / sizeof(CTL_SEN_CMD);
		
	case CTL_SEN_MODE_2:
		*cmd_tab = imx415_mode_2;
		return sizeof(imx415_mode_2) / sizeof(CTL_SEN_CMD);	

	case CTL_SEN_MODE_3:
		*cmd_tab = imx415_mode_3;
		return sizeof(imx415_mode_3) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_4:
		*cmd_tab = imx415_mode_4;
		return sizeof(imx415_mode_4) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_5:
		*cmd_tab = imx415_mode_5;
		return sizeof(imx415_mode_5) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_imx415(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;
	
	if (cur_sen_mode[id] != chgmode_obj.mode) {
		gpio_direction_output(S_GPIO(sen_power[id].rst_pin), 0);
		gpio_set_value(S_GPIO(sen_power[id].rst_pin), 0);
		vos_util_delay_ms(sen_power[id].rst_time);
		gpio_set_value(S_GPIO(sen_power[id].rst_pin), 1);
		vos_util_delay_ms(sen_power[id].stable_time);
	}

	cur_sen_mode[id] = chgmode_obj.mode;

	if (is_fastboot[id]) {
		#if defined(__KERNEL__)
		ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl_temp;

		p_sensor_ctrl_temp = isp_builtin_get_sensor_gain(id);
		sensor_ctrl.gain_ratio[0] = p_sensor_ctrl_temp->gain_ratio[0];
		sensor_ctrl.gain_ratio[1] = p_sensor_ctrl_temp->gain_ratio[1];
		p_sensor_ctrl_temp = isp_builtin_get_sensor_expt(id);
		sensor_ctrl.exp_time[0] = p_sensor_ctrl_temp->exp_time[0];
		sensor_ctrl.exp_time[1] = p_sensor_ctrl_temp->exp_time[1];
		sen_set_chgmode_fps_imx415(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_imx415(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_imx415(id, &sensor_ctrl);
		sen_set_expt_imx415(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_imx415(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx415(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_imx415(0x3024, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3025, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3026, 1, (sensor_vd >> 16) & 0x0F, 0x00);
			rt |= sen_write_reg_imx415(id, &cmd);
			//DBG_ERR("sensor_vd=%d\n", sensor_vd);
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

			sen_set_gain_imx415(id, &sensor_ctrl);
			sen_set_expt_imx415(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_imx415(id, &flip);
		} else {
			cmd = sen_set_cmd_info_imx415(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_imx415(id, &cmd);
			//DBG_ERR("write addr=0x%x, data0=0x%x, data1=0x%x\r\n", p_cmd_list[idx].addr, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);		 
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_imx415(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx415(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_imx415(0x3024, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_imx415(id, &cmd);
	cmd = sen_set_cmd_info_imx415(0x3025, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_imx415(id, &cmd);
	cmd = sen_set_cmd_info_imx415(0x3026, 1, (sensor_vd >> 16) & 0x0F, 0x00);
	rt |= sen_write_reg_imx415(id, &cmd);

	return rt;
}

static ER sen_set_info_imx415(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_imx415(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_imx415(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_imx415(id, (CTL_SEN_FLIP *)(data));
		break;
	// TODO:
	#if 0
	case CTL_SENDRV_CFGID_SET_STATUS:
		sen_set_status_imx415(id, (CTL_SEN_STATUS *)(data));
		break;
	#endif
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_imx415(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_imx415(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_imx415(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_imx415(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_imx415((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_imx415((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_imx415(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_imx415((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_imx415(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_imx415(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_imx415((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_imx415((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_imx415((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_imx415(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_imx415(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_PROBE_SEN:
		sen_ext_get_probe_sen_imx415(id, (CTL_SENDRV_GET_PROBE_SEN_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_imx415(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_MFR_OUTPUT_TIMING:
		sen_get_hdr_rhs_imx415(id, (CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM *)(data));
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_imx415(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

  //DBG_ERR("fps= %d \r\n", fps);

	sen_set_chgmode_fps_imx415(id, fps);
	sen_set_cur_fps_imx415(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_imx415(id, fps);
		sen_set_cur_fps_imx415(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_imx415(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_imx415(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_imx415(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
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
		if (100 <= (compensation_ratio[id][frame_cnt])) {
			sensor_ctrl->gain_ratio[frame_cnt] = (sensor_ctrl->gain_ratio[frame_cnt]) * (compensation_ratio[id][frame_cnt]) / 100;
		}
		if (sensor_ctrl->gain_ratio[frame_cnt] < (mode_basic_param[cur_sen_mode[id]].gain.min)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.min);
		} else if (sensor_ctrl->gain_ratio[frame_cnt] > (mode_basic_param[cur_sen_mode[id]].gain.max)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.max);			
		}
		data[frame_cnt] = (6 * sen_common_calc_log_2(sensor_ctrl->gain_ratio[frame_cnt], 1000)) / 30;
		if (data[frame_cnt] > (0xF0)) {
			DBG_ERR("gain overflow gain_ratio = %d data[0] = 0x%.8x \r\n", sensor_ctrl->gain_ratio[frame_cnt], data[frame_cnt]);
			data[frame_cnt] = 0xF0;
		}
	}	
		

	// GAIN_PGC_FIDMD
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {
		cmd = sen_set_cmd_info_imx415(0x3260, 1, 0x01, 0x0);
	} else {
		cmd = sen_set_cmd_info_imx415(0x3260, 1, 0x00, 0x0);
	}
	rt |= sen_write_reg_imx415(id, &cmd);

	// Write sensor gain
	cmd = sen_set_cmd_info_imx415(0x3090, 1, data[0] & 0xFF, 0x0);
	rt |= sen_write_reg_imx415(id, &cmd);
	cmd = sen_set_cmd_info_imx415(0x3091, 1, (data[0] >> 8) & 0x07, 0x0);
	rt |= sen_write_reg_imx415(id, &cmd);
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		cmd = sen_set_cmd_info_imx415(0x3092, 1, data[1] & 0xFF, 0x0);
		rt |= sen_write_reg_imx415(id, &cmd);
		cmd = sen_set_cmd_info_imx415(0x3093, 1, (data[1] >> 8) & 0x01, 0x0);
		rt |= sen_write_reg_imx415(id, &cmd);
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_get_min_shr_imx415(CTL_SEN_ID id, UINT32 *min_shr, UINT32 *min_exp)
{
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {
		*min_shr = 5;
		*min_exp = 1;
	} else if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		*min_shr = 9;
		*min_exp = 8; //short exp frame min exp = rhs1 - shr1 = 8
	}
	return;
}

static void sen_set_expt_imx415(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
	UINT32 min_shr = 0, min_exp_line = 0;
	UINT32 shr0 = 0, rhs1 = 0, shr1 = 0, brl = 0, fcs = 0, temp_max_short_line = 0;
	UINT32 temp_line[ISP_SEN_MFRAME_MAX_NUM] = {0};		
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
		t_row = sen_calc_rowtime_imx415(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;

		sen_get_min_shr_imx415(id, &min_shr, &min_exp_line);

		// Limit minimun exposure line
		if (line[frame_cnt] < min_exp_line) {
			line[frame_cnt] = min_exp_line;
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_imx415(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_imx415(id, cur_sen_mode[id]);
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		if ((mode_basic_param[cur_sen_mode[id]].frame_num) == 2) {
			expt_time = (line[0] + line[1]) * t_row / 10;
			temp_line[0] = line[0];
			temp_line[1] = line[1];
		}
	} else {
		expt_time = (line[0]) * t_row / 10;
		temp_line[0] = line[0];	 		
	}

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_imx415(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_imx415(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		if (0 != ( sensor_vd % 4)){
			sensor_vd = sensor_vd / 4 * 4 + 4;
		}
	}

	// Set Vmax to sensor 
	cmd = sen_set_cmd_info_imx415(0x3024, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_imx415(id, &cmd);
	cmd = sen_set_cmd_info_imx415(0x3025, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx415(id, &cmd);
	cmd = sen_set_cmd_info_imx415(0x3026, 1, (sensor_vd >> 16 ) & 0x0F, 0);
	rt |= sen_write_reg_imx415(id, &cmd);

	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {
		//Check max exp line reg
		if (line[0] > (MAX_VD_PERIOD-min_shr)) {
			DBG_ERR("max line overflow \r\n");
			line[0] = (MAX_VD_PERIOD-min_shr);
		}

		//Check min exp line
		if (line[0] < min_exp_line) {
			DBG_ERR("min line overflow\r\n");
			line[0] = min_exp_line;
		}

		//calculate shr
		if (line[0] > (sensor_vd - min_shr)) {
			shr0 = min_shr;
		} else {
			shr0 = sensor_vd - line[0];
		}
		compensation_ratio[id][0] = 100 * temp_line[0] / (sensor_vd - shr0);	

		// set exposure line to sensor
		cmd = sen_set_cmd_info_imx415(0x3050, 1, shr0 & 0xFF , 0);
		rt |= sen_write_reg_imx415(id, &cmd);
		cmd = sen_set_cmd_info_imx415(0x3051, 1, (shr0 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx415(id, &cmd);
		cmd = sen_set_cmd_info_imx415(0x3052, 1, (shr0 >> 16) & 0x0F , 0);
		rt |= sen_write_reg_imx415(id, &cmd);
	} else if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		if (mode_basic_param[cur_sen_mode[id]].valid_size.w > 1920)
		{
			brl = BRL_ALL_PIXEL;
		} else {
			brl = BRL_BINNING;
		}

		if ((mode_basic_param[cur_sen_mode[id]].frame_num) == 2) {
			fcs = sensor_vd * 2;
			line[1] &= 0xFFFFFFFE;
			temp_max_short_line = (100000000 / cur_fps ) * 10 / 9 / t_row; //max support 1:8
			// NOTE: RHS1(N+1) >= RHS1(N)+BRL*2-FSC+2
			// NOTE: 4n+1, n = 0, 1, 2, ..., SHR1+8 <= RHS1 <= SHR0-9, RHS1 < BRL*2
			if (mode_basic_param[cur_sen_mode[id]].valid_size.w > 1920) {
				rhs1 = (fcs - (brl * 2)) - 2;
			} else {
				rhs1 = 2 * (fcs - (brl * 2)) - 4;
			}
			if (rhs1 >= (brl * 2)) {
				rhs1 = brl * 2 - 1;
			} else if (rhs1 > (9 + 8 + temp_max_short_line)) {
				rhs1 = 9 + 8 + temp_max_short_line;//min shr1 = 9, min rhs1 = shr1 + 8 =17
			}
			
			if (0 != (( rhs1 - 1) % 4)) {
				rhs1 = (rhs1 - 1) / 4 * 4 + 1;
			}

			// NOTE: 2n+1, n = 0, 1, 2, ..., 9 <= SHR1 <= RHS1-8
			if (line[1] > rhs1) {
				shr1 = 9;
			} else {
				shr1 = rhs1 - line[1];
			}
			if (shr1 < 9) {
				shr1 = 9;
			}
			if (shr1 >= (rhs1 - 8)) {
				shr1 = rhs1 - 8;
			}
			if (0 != ((shr1 - 1) % 2)) {
				shr1 = (shr1 - 1) / 2 * 2 + 1;
			}

			// NOTE: 2n, n = 0, 1, 2, ..., RHS1+9 <= SHR0 <= FSC-8
			if ((line[0]) > (fcs - (rhs1 + 9))) {
				shr0 = rhs1 + 9;
			} else {
				shr0 = fcs - line[0];
			}
			if (shr0 < (rhs1 + 9)) {
				shr0 = rhs1 + 9;
			}
			if (shr0 > (fcs - 8)) {
				shr0 = fcs - 8;
			}
			if (0 != (shr0 % 2)) {
				shr0 = shr0 / 2 * 2;
			}
			compensation_ratio[id][0] = 100 * temp_line[0] / (fcs - shr0);
			compensation_ratio[id][1] = 100 * temp_line[1] / (rhs1 - shr1);			

			cmd = sen_set_cmd_info_imx415(0x3050, 1, shr0 & 0xFF , 0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3051, 1, (shr0 >> 8) & 0xFF, 0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3052, 1, (shr0 >> 16) & 0x0F , 0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3060, 1, rhs1 & 0xFF, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3061, 1, (rhs1 >> 8)& 0xFF, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3062, 1, (rhs1 >> 16)& 0x0F, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3054, 1, shr1 & 0xFF, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3055, 1, (shr1 >> 8)& 0xFF, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
			cmd = sen_set_cmd_info_imx415(0x3056, 1, (shr1 >> 16)& 0x0F, 0x0);
			rt |= sen_write_reg_imx415(id, &cmd);
		}
	}
	
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_imx415(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_imx415(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx415(0x3030, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx415(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
			cmd.data[0] |= 0x1;
		} else {
			cmd.data[0] &= (~0x01);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x2;
		} else {
			cmd.data[0] &= (~0x02);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}
	
	rt |= sen_write_reg_imx415(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_imx415(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx415(0x3030, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx415(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (cmd.data[0] & 0x1) {
		*flip |= CTL_SEN_FLIP_H;
	}
	if (cmd.data[0] & 0x2) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

#if 0
static void sen_set_status_imx415(CTL_SEN_ID id, CTL_SEN_STATUS *status)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	switch (*status) {
		case CTL_SEN_STATUS_STANDBY:
			cmd = sen_set_cmd_info_imx415(0x3000, 1, 0x01, 0x0);  // STANDBY ON
			rt = sen_write_reg_imx415(id, &cmd);
			if (rt != E_OK) {
				DBG_ERR("write register error %d \r\n", (INT)rt);
			}
			break;

		default:
			DBG_ERR("set status error %d \r\n", *status);
	}
}
#endif

#if defined(__FREERTOS)
void sen_get_gain_imx415(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_imx415(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_imx415(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_imx415(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_imx415(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row = 0;
	UINT32 min_shr = 0, min_exp_line = 0;

	sen_get_min_shr_imx415(id, &min_shr, &min_exp_line);

	t_row = sen_calc_rowtime_imx415(id, cur_sen_mode[id]);
	*min_exp_time = t_row * min_exp_line / 10 + 1;
}

static void sen_get_mode_basic_imx415(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_imx415(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_imx415(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_imx415(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_ext_get_probe_sen_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_PROBE_SEN_PARAM *data)
{
	CTL_SEN_CMD cmd;
	UINT32 read_data_h = 0, read_data_l = 0, sensor_id = 0;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx415(0x3F13, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx415(id, &cmd);
	read_data_h = cmd.data[0];

	cmd = sen_set_cmd_info_imx415(0x3F12, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx415(id, &cmd);
	read_data_l = cmd.data[0];	

	sensor_id = ((read_data_h & 0x0F) << 8) | (read_data_l & 0xFF);

	if (SENSOR_ID == sensor_id) {
		data->probe_rst = 0;
	} else if (rt != E_OK) { 
		data->probe_rst = rt;
	} else { // (SENSOR_ID != sensor_id) {
		data->probe_rst = -1;
	} 
}

static void sen_get_hdr_rhs_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_MFR_OUTPUT_TIMING_PARAM *data)
{
	//UINT32 diff_row[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 fcs =0, temp_vd = 0, temp_max_short_line = 0, t_row = 0,  i = 0;
	UINT32 brl = 0, rhs1 = 0;

	for (i =0 ; i < ISP_SEN_MFRAME_MAX_NUM ; i++) {
		data->diff_row[i] = 0;
	}

	if (mode_basic_param[data->mode].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		if (((mode_basic_param[data->mode].frame_num) == data->num)  && ( mode_basic_param[data->mode].mode == (data->mode))) {
			if (2 == data->num) {
				if (mode_basic_param[data->mode].valid_size.w > 1920)
				{
					brl = BRL_ALL_PIXEL;
				} else {
					brl = BRL_BINNING;
				}
				temp_vd = mode_basic_param[data->mode].signal_info.vd_period * (mode_basic_param[data->mode].dft_fps) / (data->frame_rate);
				fcs = temp_vd * 2;
				t_row = sen_calc_rowtime_imx415(id, data->mode);
				temp_max_short_line = (100000000 / (data->frame_rate) ) * 10 / 9 / t_row; //max support 1:8

				// NOTE: RHS1(N+1) >= RHS1(N)+BRL*2-FSC+2
				// NOTE: 4n+1, n = 0, 1, 2, ..., SHR1+8 <= RHS1 <= SHR0-9, RHS1 < BRL*2
				if (mode_basic_param[data->mode].valid_size.w > 1920) {
					rhs1 = (fcs - (brl * 2)) - 2;
				} else {
					rhs1 = 2 * (fcs - (brl * 2)) - 4;
				}
				
				if (rhs1 >= (brl * 2)) {
					rhs1 = brl * 2 - 1;
				} else if (rhs1 > (9 + 8 + temp_max_short_line)) {
					rhs1 = 9 + 8 + temp_max_short_line;//min shr1 = 9, min rhs1 = shr1 + 8 =17
				}

				if (0 != (( rhs1 - 1) % 4)) {
					rhs1 = (rhs1 - 1) / 4 * 4 + 1;
				}
				
				data->diff_row[0] = rhs1;
			}
		} else {
			DBG_ERR("get hdr mode rhs value fail \r\n");
		}
	}
}

static void sen_get_fps_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_imx415(id);
	data->chg_fps = sen_get_chgmode_fps_imx415(id);
}

static void sen_get_speed_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_imx415(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_imx415(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}
	if (data->data_lane == CTL_SEN_DATALANE_2) {
		if ((data->size.w <= 3840) && (data->size.h <= 2160)) {
			if (data->frame_rate <= 3000) {
				data->mode = CTL_SEN_MODE_4;
				return;
			} 
		}		
	} else { // default: CTL_SEN_DATALANE_4 or no define lane num
		if (data->frame_num == 1) {
			if ((data->size.w <= 3840) && (data->size.h <= 2160)) {
				if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
					if (data->frame_rate <= 6000) {
						data->mode = CTL_SEN_MODE_3;
						return;
					}
				} else {
					if (data->frame_rate <= 3000) {
						data->mode = CTL_SEN_MODE_1;
						return;
					} else if (data->frame_rate <= 6000) {
						data->mode = CTL_SEN_MODE_5;
						return;
					}
				}
			}
		} else if (data->frame_num == 2) {
			if ((data->size.w <= 3840) && (data->size.h <= 2160)) {
				if (data->frame_rate <= 3000) {
					data->mode = CTL_SEN_MODE_2;
					return;
				}
			}
		}
	} 					

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_imx415(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		div_step = 2;
	} else {
		div_step = 1;
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_imx415(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 10 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 1000000);

	return row_time;
}

static void sen_get_rowtime_imx415(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_imx415(id, data->mode);	
	data->row_time = sen_calc_rowtime_imx415(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_imx415(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_imx415(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_imx415(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_imx415(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_imx415(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_imx415(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}
int sen_init_imx415(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX; id++ ) {
		is_fastboot[id] = 0;
		fastboot_i2c_id[id] = 0xFFFFFFFF;
		fastboot_i2c_addr[id] = 0x0;
	}

	sprintf(compatible, "nvt,sen_imx415");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_imx415");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx415;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx415();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx415", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_imx415(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx415");
}

#else
static int __init sen_init_imx415(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx415;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx415();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx415", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_imx415(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx415");
}

module_init(sen_init_imx415);
module_exit(sen_exit_imx415);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_IMX415_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

