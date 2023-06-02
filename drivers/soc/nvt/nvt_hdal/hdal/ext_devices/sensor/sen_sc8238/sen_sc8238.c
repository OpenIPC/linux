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
VOS_MODULE_VERSION(nvt_sen_sc8238, 1, 44, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_SC8238_MODULE_NAME     "sen_sc8238"
#define SEN_MAX_MODE               2
#define MAX_VD_PERIOD              0xFFFF
#define MAX_EXPOSURE_LINE          0xFFFF
#define MIN_EXPOSURE_LINE          3
#define NON_EXPOSURE_LINE          10

#define SEN_I2C_ADDR 0x60>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_sc8238"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_sc8238(void);
static void sen_pwr_ctrl_sc8238(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_sc8238(CTL_SEN_ID id);
static ER sen_close_sc8238(CTL_SEN_ID id);
static ER sen_sleep_sc8238(CTL_SEN_ID id);
static ER sen_wakeup_sc8238(CTL_SEN_ID id);
static ER sen_write_reg_sc8238(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_sc8238(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_sc8238(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_sc8238(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_sc8238(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_sc8238(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_sc8238(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_sc8238(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_sc8238(CTL_SEN_ID id, void *param);
static void sen_set_expt_sc8238(CTL_SEN_ID id, void *param);
static void sen_set_preset_sc8238(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_sc8238(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_sc8238(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_sc8238(CTL_SEN_ID id, void *param);
void sen_get_expt_sc8238(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_sc8238(CTL_SEN_ID id, void *param);
static void sen_get_expt_sc8238(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_sc8238(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_sc8238(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_sc8238(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_sc8238(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_sc8238(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_sc8238(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_sc8238(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_sc8238(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_sc8238(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_sc8238(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_sc8238(CTL_SEN_ID id);
static void sen_set_chgmode_fps_sc8238(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_sc8238(CTL_SEN_ID id);
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
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR}
};

static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = {
	SEN_SC8238_MODULE_NAME,
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
		{CTL_SEN_I2C_W_ADDR_DFT,     0x60},
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
		27000000,
		283500000,
		141750000
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_SIEMCLK_SRC_DFT,
		27000000,
		270000000,
		135000000
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
		{3840, 2162},
		{{0, 2, 3840, 2160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3840, 2160},
		{0, 2100, 0, 2250},
		CTL_SEN_RATIO(16, 9),
		{1000, 500062},
		100
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1920, 1082},
		{{0, 2, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 4000, 0, 1125},
		CTL_SEN_RATIO(16, 9),
		{1000, 500062},
		200
	}
};

static CTL_SEN_CMD sc8238_mode_1[] = {
	//[paralist]
	{0x0103, 1, {0x01, 0x0}},
	{0x0100, 1, {0x00, 0x0}},
	{0x336c, 1, {0xc2, 0x0}},
	{0x3624, 1, {0x45, 0x0}},
	{0x33e1, 1, {0x08, 0x0}},
	{0x33e2, 1, {0x18, 0x0}},
	{0x33e3, 1, {0x10, 0x0}},
	{0x33e4, 1, {0x0c, 0x0}},
	{0x33e5, 1, {0x10, 0x0}},
	{0x33e6, 1, {0x06, 0x0}},
	{0x33e7, 1, {0x02, 0x0}},
	{0x33e8, 1, {0x18, 0x0}},
	{0x33e9, 1, {0x10, 0x0}},
	{0x33ea, 1, {0x0c, 0x0}},
	{0x33eb, 1, {0x10, 0x0}},
	{0x33ec, 1, {0x04, 0x0}},
	{0x33ed, 1, {0x02, 0x0}},
	{0x33ee, 1, {0xa0, 0x0}},
	{0x33ef, 1, {0x08, 0x0}},
	{0x33f4, 1, {0x18, 0x0}},
	{0x33f5, 1, {0x10, 0x0}},
	{0x33f6, 1, {0x0c, 0x0}},
	{0x33f7, 1, {0x10, 0x0}},
	{0x33f8, 1, {0x06, 0x0}},
	{0x33f9, 1, {0x02, 0x0}},
	{0x33fa, 1, {0x18, 0x0}},
	{0x33fb, 1, {0x10, 0x0}},
	{0x33fc, 1, {0x0c, 0x0}},
	{0x33fd, 1, {0x10, 0x0}},
	{0x33fe, 1, {0x04, 0x0}},
	{0x33ff, 1, {0x02, 0x0}},
	{0x5799, 1, {0x06, 0x0}},
	{0x3e16, 1, {0x00, 0x0}},
	{0x3e17, 1, {0xac, 0x0}},
	{0x3e18, 1, {0x00, 0x0}},
	{0x3e19, 1, {0xac, 0x0}},
	{0x3e0e, 1, {0x09, 0x0}},
	{0x3e1e, 1, {0x76, 0x0}},
	{0x3e25, 1, {0x23, 0x0}},
	{0x57aa, 1, {0x2f, 0x0}},
	{0x57ab, 1, {0xff, 0x0}},
	{0x3018, 1, {0x72, 0x0}},
	{0x3019, 1, {0x00, 0x0}},
	{0x3031, 1, {0x0a, 0x0}},
	{0x3037, 1, {0x20, 0x0}},
	{0x4501, 1, {0xa4, 0x0}},
	{0x4509, 1, {0x10, 0x0}},
	{0x4837, 1, {0x1c, 0x0}},
	{0x3366, 1, {0x92, 0x0}},
	{0x3314, 1, {0x94, 0x0}},
	{0x334c, 1, {0x10, 0x0}},
	{0x3e09, 1, {0x40, 0x0}},
	{0x3e08, 1, {0x03, 0x0}},
	{0x3637, 1, {0x52, 0x0}},
	{0x3631, 1, {0x80, 0x0}},
	{0x3308, 1, {0x20, 0x0}},
	{0x335d, 1, {0x60, 0x0}},
	{0x337f, 1, {0x33, 0x0}},
	{0x3368, 1, {0x07, 0x0}},
	{0x3369, 1, {0x00, 0x0}},
	{0x336a, 1, {0x00, 0x0}},
	{0x336b, 1, {0x00, 0x0}},
	{0x3367, 1, {0x08, 0x0}},
	{0x330e, 1, {0x58, 0x0}},
	{0x33af, 1, {0x48, 0x0}},
	{0x3638, 1, {0x0a, 0x0}},
	{0x3306, 1, {0xa8, 0x0}},
	{0x3309, 1, {0x68, 0x0}},
	{0x331f, 1, {0x59, 0x0}},
	{0x330d, 1, {0x28, 0x0}},
	{0x339e, 1, {0x24, 0x0}},
	{0x33aa, 1, {0x24, 0x0}},
	{0x3332, 1, {0x24, 0x0}},
	{0x3350, 1, {0x24, 0x0}},
	{0x3358, 1, {0x24, 0x0}},
	{0x335c, 1, {0x24, 0x0}},
	{0x3628, 1, {0x83, 0x0}},
	{0x3633, 1, {0x53, 0x0}},
	{0x3630, 1, {0x80, 0x0}},
	{0x3622, 1, {0xf7, 0x0}},
	{0x363d, 1, {0x01, 0x0}},
	{0x363e, 1, {0x00, 0x0}},
	{0x363a, 1, {0x88, 0x0}},
	{0x3e26, 1, {0x40, 0x0}},
	{0x360f, 1, {0x01, 0x0}},
	{0x367a, 1, {0x48, 0x0}},
	{0x367b, 1, {0x78, 0x0}},
	{0x3671, 1, {0xf7, 0x0}},
	{0x3672, 1, {0xf7, 0x0}},
	{0x3673, 1, {0x17, 0x0}},
	{0x3670, 1, {0x4a, 0x0}},
	{0x367c, 1, {0x48, 0x0}},
	{0x367d, 1, {0x78, 0x0}},
	{0x3674, 1, {0x80, 0x0}},
	{0x3675, 1, {0x85, 0x0}},
	{0x3676, 1, {0xa5, 0x0}},
	{0x369c, 1, {0x48, 0x0}},
	{0x369d, 1, {0x78, 0x0}},
	{0x3690, 1, {0x53, 0x0}},
	{0x3691, 1, {0x63, 0x0}},
	{0x3692, 1, {0x54, 0x0}},
	{0x36a2, 1, {0x48, 0x0}},
	{0x36a3, 1, {0x78, 0x0}},
	{0x3699, 1, {0x88, 0x0}},
	{0x369a, 1, {0x9f, 0x0}},
	{0x369b, 1, {0x9f, 0x0}},
	{0x36bb, 1, {0x48, 0x0}},
	{0x36bc, 1, {0x78, 0x0}},
	{0x36c9, 1, {0x05, 0x0}},
	{0x36ca, 1, {0x05, 0x0}},
	{0x36cb, 1, {0x05, 0x0}},
	{0x36d0, 1, {0x30, 0x0}},
	{0x36d1, 1, {0x48, 0x0}},
	{0x36d2, 1, {0x78, 0x0}},
	{0x36cc, 1, {0x00, 0x0}},
	{0x36cd, 1, {0x10, 0x0}},
	{0x36ce, 1, {0x1a, 0x0}},
	{0x3364, 1, {0x16, 0x0}},
	{0x3301, 1, {0x1c, 0x0}},
	{0x3393, 1, {0x1c, 0x0}},
	{0x3394, 1, {0x28, 0x0}},
	{0x3395, 1, {0x60, 0x0}},
	{0x3390, 1, {0x08, 0x0}},
	{0x3391, 1, {0x18, 0x0}},
	{0x3392, 1, {0x38, 0x0}},
	{0x3399, 1, {0x1c, 0x0}},
	{0x339a, 1, {0x1c, 0x0}},
	{0x339b, 1, {0x28, 0x0}},
	{0x339c, 1, {0x60, 0x0}},
	{0x3396, 1, {0x08, 0x0}},
	{0x3397, 1, {0x18, 0x0}},
	{0x3398, 1, {0x38, 0x0}},
	{0x3241, 1, {0x00, 0x0}},
	{0x3243, 1, {0x03, 0x0}},
	{0x3271, 1, {0x1c, 0x0}},
	{0x3273, 1, {0x1f, 0x0}},
	{0x394c, 1, {0x0f, 0x0}},
	{0x394d, 1, {0x20, 0x0}},
	{0x394e, 1, {0x08, 0x0}},
	{0x394f, 1, {0x90, 0x0}},
	{0x3981, 1, {0x70, 0x0}},
	{0x3984, 1, {0x20, 0x0}},
	{0x39ec, 1, {0x08, 0x0}},
	{0x39ed, 1, {0x00, 0x0}},
	{0x3982, 1, {0x00, 0x0}},
	{0x3983, 1, {0x00, 0x0}},
	{0x3980, 1, {0x71, 0x0}},
	{0x39b4, 1, {0x0c, 0x0}},
	{0x39b5, 1, {0x1c, 0x0}},
	{0x39b6, 1, {0x38, 0x0}},
	{0x39b7, 1, {0x5b, 0x0}},
	{0x39b8, 1, {0x50, 0x0}},
	{0x39b9, 1, {0x38, 0x0}},
	{0x39ba, 1, {0x20, 0x0}},
	{0x39bb, 1, {0x10, 0x0}},
	{0x39bc, 1, {0x0c, 0x0}},
	{0x39bd, 1, {0x16, 0x0}},
	{0x39be, 1, {0x21, 0x0}},
	{0x39bf, 1, {0x36, 0x0}},
	{0x39c0, 1, {0x3b, 0x0}},
	{0x39c1, 1, {0x2a, 0x0}},
	{0x39c2, 1, {0x16, 0x0}},
	{0x39c3, 1, {0x0c, 0x0}},
	{0x39a2, 1, {0x03, 0x0}},
	{0x39a3, 1, {0xe3, 0x0}},
	{0x39a4, 1, {0x03, 0x0}},
	{0x39a5, 1, {0xf2, 0x0}},
	{0x39a6, 1, {0x03, 0x0}},
	{0x39a7, 1, {0xf6, 0x0}},
	{0x39a8, 1, {0x03, 0x0}},
	{0x39a9, 1, {0xfa, 0x0}},
	{0x39aa, 1, {0x03, 0x0}},
	{0x39ab, 1, {0xff, 0x0}},
	{0x39ac, 1, {0x00, 0x0}},
	{0x39ad, 1, {0x06, 0x0}},
	{0x39ae, 1, {0x00, 0x0}},
	{0x39af, 1, {0x09, 0x0}},
	{0x39b0, 1, {0x00, 0x0}},
	{0x39b1, 1, {0x12, 0x0}},
	{0x39b2, 1, {0x00, 0x0}},
	{0x39b3, 1, {0x22, 0x0}},
	{0x39c6, 1, {0x07, 0x0}},
	{0x39c7, 1, {0xf8, 0x0}},
	{0x39c9, 1, {0x07, 0x0}},
	{0x39ca, 1, {0xf8, 0x0}},
	{0x3990, 1, {0x03, 0x0}},
	{0x3991, 1, {0xfd, 0x0}},
	{0x3992, 1, {0x03, 0x0}},
	{0x3993, 1, {0xfc, 0x0}},
	{0x3994, 1, {0x00, 0x0}},
	{0x3995, 1, {0x00, 0x0}},
	{0x3996, 1, {0x00, 0x0}},
	{0x3997, 1, {0x05, 0x0}},
	{0x3998, 1, {0x00, 0x0}},
	{0x3999, 1, {0x09, 0x0}},
	{0x399a, 1, {0x00, 0x0}},
	{0x399b, 1, {0x12, 0x0}},
	{0x399c, 1, {0x00, 0x0}},
	{0x399d, 1, {0x12, 0x0}},
	{0x399e, 1, {0x00, 0x0}},
	{0x399f, 1, {0x18, 0x0}},
	{0x39a0, 1, {0x00, 0x0}},
	{0x39a1, 1, {0x14, 0x0}},
	{0x39cc, 1, {0x00, 0x0}},
	{0x39cd, 1, {0x1b, 0x0}},
	{0x39ce, 1, {0x00, 0x0}},
	{0x39cf, 1, {0x00, 0x0}},
	{0x39d0, 1, {0x1b, 0x0}},
	{0x39d1, 1, {0x00, 0x0}},
	{0x39e2, 1, {0x15, 0x0}},
	{0x39e3, 1, {0x87, 0x0}},
	{0x39e4, 1, {0x12, 0x0}},
	{0x39e5, 1, {0xb7, 0x0}},
	{0x39e6, 1, {0x00, 0x0}},
	{0x39e7, 1, {0x8c, 0x0}},
	{0x39e8, 1, {0x01, 0x0}},
	{0x39e9, 1, {0x31, 0x0}},
	{0x39ea, 1, {0x01, 0x0}},
	{0x39eb, 1, {0xd7, 0x0}},
	{0x39c5, 1, {0x30, 0x0}},
	{0x330b, 1, {0x48, 0x0}},
	{0x320c, 1, {0x08, 0x0}},
	{0x320d, 1, {0x34, 0x0}},
	{0x320e, 1, {0x08, 0x0}}, 
	{0x320f, 1, {0xca, 0x0}},
	{0x3635, 1, {0x02, 0x0}},
	{0x3902, 1, {0xc5, 0x0}},
	{0x3038, 1, {0x44, 0x0}},
	{0x3632, 1, {0xa8, 0x0}},
	{0x3e14, 1, {0x31, 0x0}},
	{0x3e1b, 1, {0x3a, 0x0}},
	{0x3248, 1, {0x04, 0x0}},
	{0x3901, 1, {0x00, 0x0}},
	{0x3904, 1, {0x18, 0x0}},
	{0x3987, 1, {0x0b, 0x0}},
	{0x363b, 1, {0x06, 0x0}},
	{0x3905, 1, {0xd8, 0x0}},
	{0x301f, 1, {0x02, 0x0}},
	{0x3203, 1, {0x08, 0x0}},
	{0x3207, 1, {0x87, 0x0}},
	{0x3211, 1, {0x0f, 0x0}},//0x10	
	{0x3213, 1, {0x07, 0x0}},//0x08
	{0x3e00, 1, {0x01, 0x0}},
	{0x3e01, 1, {0x18, 0x0}},
	{0x3e02, 1, {0xa0, 0x0}},
	{0x3641, 1, {0x0c, 0x0}},
	{0x36f9, 1, {0x57, 0x0}},
	{0x36fa, 1, {0x39, 0x0}},
	{0x36fb, 1, {0x13, 0x0}},
	{0x36fc, 1, {0x10, 0x0}},
	{0x36fd, 1, {0x14, 0x0}},
	{0x36e9, 1, {0x53, 0x0}},
	{0x36ea, 1, {0x39, 0x0}},
	{0x36eb, 1, {0x06, 0x0}},
	{0x36ec, 1, {0x05, 0x0}},
	{0x36ed, 1, {0x24, 0x0}},
	{0x3641, 1, {0x00, 0x0}},
	{0x3699, 1, {0x9F, 0x0}}, //for 1x to 2x flash
	{0x320b, 1, {0x72, 0x0}}, //2162   	
	//DPC
	{0x5780, 1, {0x57, 0x0}},
	{0x5785, 1, {0x00, 0x0}},
	{0x5786, 1, {0x00, 0x0}},
	{0x5799, 1, {0x07, 0x0}},  
	{SEN_CMD_SETVD, 1, {0x00, 0x0}},
	{SEN_CMD_PRESET, 1, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x00, 0x0}},   	
	{0x0100, 1, {0x01, 0x0}},
};

static CTL_SEN_CMD sc8238_mode_2[] = {
	//[ParaList]
	{0x0103, 1, {0x01, 0x0}},
	{0x0100, 1, {0x00, 0x0}},
	{0x336c, 1, {0xc2, 0x0}},
	{0x363b, 1, {0x08, 0x0}},
	{0x3635, 1, {0x42, 0x0}},
	{0x3624, 1, {0x45, 0x0}},
	///////digital updates start////////////
	// 05-23-2018
	//0x33e0,0xa0,
	{0x33e1, 1, {0x08, 0x0}},
	{0x33e2, 1, {0x18, 0x0}},
	{0x33e3, 1, {0x10, 0x0}},
	{0x33e4, 1, {0x0c, 0x0}},
	{0x33e5, 1, {0x10, 0x0}},
	{0x33e6, 1, {0x06, 0x0}},
	{0x33e7, 1, {0x02, 0x0}},
	{0x33e8, 1, {0x18, 0x0}},
	{0x33e9, 1, {0x10, 0x0}},
	{0x33ea, 1, {0x0c, 0x0}},
	{0x33eb, 1, {0x10, 0x0}},
	{0x33ec, 1, {0x04, 0x0}},
	{0x33ed, 1, {0x02, 0x0}},
	{0x33ee, 1, {0xa0, 0x0}},
	{0x33ef, 1, {0x08, 0x0}},
	{0x33f4, 1, {0x18, 0x0}},
	{0x33f5, 1, {0x10, 0x0}},
	{0x33f6, 1, {0x0c, 0x0}},
	{0x33f7, 1, {0x10, 0x0}},
	{0x33f8, 1, {0x06, 0x0}},
	{0x33f9, 1, {0x02, 0x0}},
	{0x33fa, 1, {0x18, 0x0}},
	{0x33fb, 1, {0x10, 0x0}},
	{0x33fc, 1, {0x0c, 0x0}},
	{0x33fd, 1, {0x10, 0x0}},
	{0x33fe, 1, {0x04, 0x0}},
	{0x33ff, 1, {0x02, 0x0}},
	{0x5799, 1, {0x06, 0x0}},
	{0x3e1b, 1, {0x2a, 0x0}},//0x3e03 03 mode support fine gain 40-->7f 1204B
	{0x3e25, 1, {0x03, 0x0}},//blc 1x gain threshold
	{0x3e26, 1, {0x40, 0x0}},//blc 1x gain threshold
	{0x3e16, 1, {0x00, 0x0}},
	{0x3e17, 1, {0xac, 0x0}},//dcg gain value
	{0x3e18, 1, {0x00, 0x0}},
	{0x3e19, 1, {0xac, 0x0}},//dcg open point
	//for dcg always on
	{0x3e0e, 1, {0x09, 0x0}},
	{0x3e1e, 1, {0x76, 0x0}},//[7] gcvt_dcg_en [3] gcvt_se_dcg_en
	{0x3e25, 1, {0x23, 0x0}},//blc 1x gain threshold
	{0x3e26, 1, {0x40, 0x0}},//blc 1x gain threshold
	//end
	{0x3e1b, 1, {0x3a, 0x0}},//0x3e03 03 mode support fine gain 40-->7f
	{0x57aa, 1, {0x2f, 0x0}}, //dpc
	{0x57ab, 1, {0xff, 0x0}}, // [7] edge optimization
	//mipi
	{0x3018, 1, {0x72, 0x0}},//[7:5] lane_num-1 [4] digital mipi pad en 1:mipi 0:DVP
	{0x3019, 1, {0x00, 0x0}},//[3:0] lane disable
	{0x3031, 1, {0x0a, 0x0}},//[3:0] bitmode
	{0x3037, 1, {0x20, 0x0}},//[6:5] bit sel 0:8bit 1:10bit 10:12bit
	{0x4501, 1, {0xa4, 0x0}},
	{0x4509, 1, {0x10, 0x0}},
	{0x4837, 1, {0x1c, 0x0}},//[7:0] pclk period * 2
	//mipi end
	///////digital updates end////////////
	{0x320c, 1, {0x08, 0x0}},
	{0x320d, 1, {0x98, 0x0}},
	{0x330b, 1, {0xc0, 0x0}},
	{0x3306, 1, {0xe0, 0x0}},
	{0x3309, 1, {0x50, 0x0}},
	{0x331f, 1, {0x41, 0x0}},
	{0x3301, 1, {0x20, 0x0}},
	{0x3638, 1, {0x25, 0x0}},
	{0x3e00, 1, {0x01, 0x0}},
	{0x3e01, 1, {0x18, 0x0}},
	{0x3637, 1, {0x15, 0x0}},
	{0x3632, 1, {0x28, 0x0}},
	{0x3635, 1, {0x20, 0x0}},
	{0x3366, 1, {0x92, 0x0}},
	//1106
	{0x363d, 1, {0x01, 0x0}},
	{0x3637, 1, {0x11, 0x0}},
	{0x3632, 1, {0x08, 0x0}},
	{0x3628, 1, {0x80, 0x0}}, // cmp pd option
	{0x3638, 1, {0x08, 0x0}},
	{0x3306, 1, {0xb0, 0x0}},
	{0x3309, 1, {0x40, 0x0}},
	{0x331f, 1, {0x31, 0x0}},
	{0x330b, 1, {0xb0, 0x0}},
	{0x3633, 1, {0x22, 0x0}},
	{0x3301, 1, {0x30, 0x0}},
	{0x3e00, 1, {0x01, 0x0}}, //ms exp
	{0x3e01, 1, {0x00, 0x0}},
	{0x3e02, 1, {0x00, 0x0}},
	//preprecharge
	{0x3314, 1, {0x94, 0x0}},
	{0x330e, 1, {0x50, 0x0}}, //[50,60]
	{0x334c, 1, {0x10, 0x0}},
	{0x3e09, 1, {0x40, 0x0}}, // fine gain 0x40 -> 1x gain
	{0x3637, 1, {0x1e, 0x0}},
	//1107   3.5k ADC fullwell, 3.25k used 4.7K total linear
	{0x3e08, 1, {0x03, 0x0}},
	{0x3637, 1, {0x52, 0x0}},
	{0x3622, 1, {0x07, 0x0}},
	{0x3631, 1, {0x80, 0x0}},
	{0x3630, 1, {0x80, 0x0}},
	{0x3306, 1, {0xa8, 0x0}},
	{0x330b, 1, {0x48, 0x0}},
	//1203
	{0x3306, 1, {0xa8, 0x0}},
	{0x3638, 1, {0x22, 0x0}},
	//0x3304,0x50,
	//0x331e,0x41,
	{0x3309, 1, {0x70, 0x0}},
	{0x331f, 1, {0x61, 0x0}},
	{0x3301, 1, {0x40, 0x0}},
	{0x3308, 1, {0x20, 0x0}},
	//1204
	{0x320c, 1, {0x08, 0x0}}, //hts=4200
	{0x320d, 1, {0x34, 0x0}},
	{0x330b, 1, {0x28, 0x0}},
	{0x3306, 1, {0xa0, 0x0}},
	{0x3301, 1, {0x38, 0x0}},
	{0x335d, 1, {0x60, 0x0}},
	{0x337f, 1, {0x33, 0x0}}, //new auto precharge  330e in 33f0   [7:6] 11: close div_rst 00:open div_rst
	{0x3368, 1, {0x07, 0x0}},
	{0x3369, 1, {0x00, 0x0}},
	{0x336a, 1, {0x00, 0x0}},
	{0x336b, 1, {0x00, 0x0}},
	{0x3367, 1, {0x08, 0x0}},
	{0x330e, 1, {0x58, 0x0}},
	//1206
	{0x33af, 1, {0x48, 0x0}}, //0x331e - 0x11
	{0x3638, 1, {0x0a, 0x0}},
	{0x3306, 1, {0xa8, 0x0}},
	{0x3309, 1, {0x68, 0x0}},
	{0x331f, 1, {0x59, 0x0}},
	{0x330d, 1, {0x28, 0x0}},
	{0x339e, 1, {0x24, 0x0}},
	{0x33aa, 1, {0x24, 0x0}},
	{0x3332, 1, {0x24, 0x0}},
	{0x3350, 1, {0x24, 0x0}},
	{0x3358, 1, {0x24, 0x0}},
	{0x335c, 1, {0x24, 0x0}},
	//pixfpn optimize 1210
	{0x3628, 1, {0x83, 0x0}},
	{0x363e, 1, {0x00, 0x0}},
	{0x3633, 1, {0x23, 0x0}},  //logic
	{0x3630, 1, {0x80, 0x0}},
	{0x3622, 1, {0xf7, 0x0}},
	{0x363d, 1, {0x01, 0x0}},
	{0x363e, 1, {0x00, 0x0}},
	{0x363a, 1, {0x88, 0x0}},
	//20181127 digital logic
	{0x3e1b, 1, {0x3a, 0x0}},//fine gain 0x40~0x7f
	{0x3e26, 1, {0x40, 0x0}},
	//0x3622 auto logic read 0x3680 for auto value
	{0x360f, 1, {0x01, 0x0}},//[0] 3622 auto en 
	{0x367a, 1, {0x48, 0x0}},//gain0
	{0x367b, 1, {0x78, 0x0}},//gain1
	{0x3671, 1, {0xf7, 0x0}},//sel0
	{0x3672, 1, {0xf7, 0x0}},//sel1
	{0x3673, 1, {0x17, 0x0}},//sel2
	{0x3670, 1, {0x4a, 0x0}},//[1]3630 auto en [3] 3633 auto en, [6] 363a auto en
	{0x36d0, 1, {0x10, 0x0}},//[4] 363d auto en
	//0x3630 auto logic read 0x3681 for auto value
	{0x367c, 1, {0x48, 0x0}},//gain0  
	{0x367d, 1, {0x78, 0x0}},//gain1  
	{0x3674, 1, {0x80, 0x0}},//sel0
	{0x3675, 1, {0x85, 0x0}},//sel1
	{0x3676, 1, {0xa5, 0x0}},//sel2
	//0x3633 auto logic read 0x3683 for auto value
	{0x369c, 1, {0x48, 0x0}},//gain0  0x0740
	{0x369d, 1, {0x78, 0x0}},//gain1  0x1f40
	{0x3690, 1, {0x53, 0x0}},//sel0
	{0x3691, 1, {0x63, 0x0}},//sel1
	{0x3692, 1, {0x54, 0x0}},//sel2
	//0x363a auto logic read 0x3686 for auto value
	{0x36a2, 1, {0x48, 0x0}},//gain0  
	{0x36a3, 1, {0x78, 0x0}},//gain1  
	{0x3699, 1, {0x88, 0x0}},//sel0
	{0x369a, 1, {0x9f, 0x0}},//sel1
	{0x369b, 1, {0x9f, 0x0}},//sel2
	//0x363d auto logic read 0x368f for auto value
	{0x36bb, 1, {0x48, 0x0}},//gain0  
	{0x36bc, 1, {0x78, 0x0}},//gain1  
	{0x36c9, 1, {0x05, 0x0}},//sel0
	{0x36ca, 1, {0x05, 0x0}},//sel1
	{0x36cb, 1, {0x05, 0x0}},//sel2
	//0x363e auto logic read 0x36cf for auto value
	{0x36d0, 1, {0x30, 0x0}},
	{0x36d1, 1, {0x48, 0x0}}, //gain0
	{0x36d2, 1, {0x78, 0x0}},//gain 1
	{0x36cc, 1, {0x00, 0x0}},//sel 0
	{0x36cd, 1, {0x10, 0x0}},//sel 1
	{0x36ce, 1, {0x1a, 0x0}}, //sel 2
	//0x3301 auto logic read 0x33f2 for auto value
	{0x3364, 1, {0x16, 0x0}},//0x3364[4] comprst auto enable
	{0x3301, 1, {0x1c, 0x0}},//comprst sel0 when dcg off
	{0x3393, 1, {0x1c, 0x0}}, //comprst sel1 when dcg off
	{0x3394, 1, {0x28, 0x0}}, //comprst sel2 when dcg off
	{0x3395, 1, {0x38, 0x0}}, //comprst sel3 when dcg off
	{0x3390, 1, {0x08, 0x0}}, //comprst gain0 when dcg off
	{0x3391, 1, {0x18, 0x0}}, //comprst gain1 when dcg off
	{0x3392, 1, {0x38, 0x0}}, //comprst gain2 when dcg off
	{0x3399, 1, {0x1c, 0x0}},//comprst sel0 when dcg on gain<2-->[14,]
	{0x339a, 1, {0x1c, 0x0}}, //comprst sel1 when dcg on 2<=gain<4-->[14,]
	{0x339b, 1, {0x28, 0x0}}, //comprst sel2 when dcg on 4<=gain<8-->[14,]
	{0x339c, 1, {0x38, 0x0}}, //comprst sel3 when dcg on 8<=gain-->[16,40]
	{0x3396, 1, {0x08, 0x0}}, //comprst gain0 when dcg on
	{0x3397, 1, {0x18, 0x0}}, //comprst gain1 when dcg on
	{0x3398, 1, {0x38, 0x0}}, //comprst gain2 when dcg on
	////////auto logic end
	//1218 blc update
	{0x3241, 1, {0x00, 0x0}},
	{0x3243, 1, {0x03, 0x0}},
	{0x3271, 1, {0x1c, 0x0}},
	{0x3273, 1, {0x1f, 0x0}},
	{0x3248, 1, {0x00, 0x0}},
	{0x3901, 1, {0x08, 0x0}},
	//1214, 0x5011,0x85? TODO
	{0x394c, 1, {0x0f, 0x0}},
	{0x394d, 1, {0x20, 0x0}},
	{0x394e, 1, {0x08, 0x0}},
	{0x394f, 1, {0x90, 0x0}},
	{0x3981, 1, {0x70, 0x0}},
	{0x3984, 1, {0x20, 0x0}},//blc trigger，alpha trigger，解决blc逻辑点起始点闪烁问题
	{0x3987, 1, {0x08, 0x0}},
	{0x39ec, 1, {0x08, 0x0}},//BLC_OFFSET
	{0x39ed, 1, {0x00, 0x0}},
	{0x3982, 1, {0x00, 0x0}},//Auto Alpha
	{0x3983, 1, {0x00, 0x0}},
	{0x3980, 1, {0x71, 0x0}},//alpha Ratio & Offset
	{0x39b4, 1, {0x0c, 0x0}},//PosH
	{0x39b5, 1, {0x1c, 0x0}},
	{0x39b6, 1, {0x38, 0x0}},
	{0x39b7, 1, {0x5b, 0x0}},
	{0x39b8, 1, {0x50, 0x0}},
	{0x39b9, 1, {0x38, 0x0}},
	{0x39ba, 1, {0x20, 0x0}},
	{0x39bb, 1, {0x10, 0x0}},
	{0x39bc, 1, {0x0c, 0x0}},//PosV
	{0x39bd, 1, {0x16, 0x0}},
	{0x39be, 1, {0x21, 0x0}},
	{0x39bf, 1, {0x36, 0x0}},
	{0x39c0, 1, {0x3b, 0x0}},
	{0x39c1, 1, {0x2a, 0x0}},
	{0x39c2, 1, {0x16, 0x0}},
	{0x39c3, 1, {0x0c, 0x0}},
	{0x39a2, 1, {0x03, 0x0}},//KV
	{0x39a3, 1, {0xe3, 0x0}},
	{0x39a4, 1, {0x03, 0x0}},
	{0x39a5, 1, {0xf2, 0x0}},
	{0x39a6, 1, {0x03, 0x0}},
	{0x39a7, 1, {0xf6, 0x0}},
	{0x39a8, 1, {0x03, 0x0}},
	{0x39a9, 1, {0xfa, 0x0}},
	{0x39aa, 1, {0x03, 0x0}},
	{0x39ab, 1, {0xff, 0x0}},
	{0x39ac, 1, {0x00, 0x0}},
	{0x39ad, 1, {0x06, 0x0}},
	{0x39ae, 1, {0x00, 0x0}},
	{0x39af, 1, {0x09, 0x0}},
	{0x39b0, 1, {0x00, 0x0}},
	{0x39b1, 1, {0x12, 0x0}},
	{0x39b2, 1, {0x00, 0x0}},
	{0x39b3, 1, {0x22, 0x0}},
	{0x39c6, 1, {0x07, 0x0}},
	{0x39c7, 1, {0xf8, 0x0}},
	{0x39c9, 1, {0x07, 0x0}},
	{0x39ca, 1, {0xf8, 0x0}},
	{0x3990, 1, {0x03, 0x0}},//KH
	{0x3991, 1, {0xfd, 0x0}},
	{0x3992, 1, {0x03, 0x0}},
	{0x3993, 1, {0xfc, 0x0}},
	{0x3994, 1, {0x00, 0x0}},
	{0x3995, 1, {0x00, 0x0}},
	{0x3996, 1, {0x00, 0x0}},
	{0x3997, 1, {0x05, 0x0}},
	{0x3998, 1, {0x00, 0x0}},
	{0x3999, 1, {0x09, 0x0}},
	{0x399a, 1, {0x00, 0x0}},
	{0x399b, 1, {0x12, 0x0}},
	{0x399c, 1, {0x00, 0x0}},
	{0x399d, 1, {0x12, 0x0}},
	{0x399e, 1, {0x00, 0x0}},
	{0x399f, 1, {0x18, 0x0}},
	{0x39a0, 1, {0x00, 0x0}},
	{0x39a1, 1, {0x14, 0x0}},
	{0x39cc, 1, {0x00, 0x0}},//KLeft
	{0x39cd, 1, {0x1b, 0x0}},
	{0x39ce, 1, {0x00, 0x0}},
	{0x39cf, 1, {0x00, 0x0}},//KRight
	{0x39d0, 1, {0x1b, 0x0}},
	{0x39d1, 1, {0x00, 0x0}},
	{0x39e2, 1, {0x15, 0x0}},//slope
	{0x39e3, 1, {0x87, 0x0}},
	{0x39e4, 1, {0x12, 0x0}},
	{0x39e5, 1, {0xb7, 0x0}},
	{0x39e6, 1, {0x00, 0x0}},
	{0x39e7, 1, {0x8c, 0x0}},
	{0x39e8, 1, {0x01, 0x0}},
	{0x39e9, 1, {0x31, 0x0}},
	{0x39ea, 1, {0x01, 0x0}},
	{0x39eb, 1, {0xd7, 0x0}},
	{0x39c5, 1, {0x30, 0x0}},//Temp Thres
	//20181225
	{0x3038, 1, {0x22, 0x0}},//pump driver, mclk/2=27/2=13.5-->[10M,20M]
	//20181228
	{0x330b, 1, {0x48, 0x0}},
	//0x3641,0x0c,
	////567M count_clk
	//0x36f9,0x57,
	//0x36fa,0x39,
	//0x36fb,0x13,
	//0x36fc,0x10,
	//0x36fd,0x14,
	//
	//0x36e9,0x53,   //708.75M mipi 70.875M mipiclk
	//0x36ea,0x39,
	//0x36eb,0x06,
	//0x36ec,0x05,
	//0x36ed,0x24,
	//
	//0x3641,0x00,
	{0x320e, 1, {0x0a, 0x0}},//25fps
	{0x320f, 1, {0x8c, 0x0}},
	//hbin
	{0x3214, 1, {0x31, 0x0}},
	{0x3220, 1, {0x18, 0x0}},
	{0x3620, 1, {0x48, 0x0}},
	{0x3208, 1, {0x07, 0x0}},
	{0x3209, 1, {0x80, 0x0}},
	{0x3211, 1, {0x08, 0x0}},
	{0x450d, 1, {0x08, 0x0}},
	//vbin          
	{0x3215, 1, {0x31, 0x0}},
	{0x3220, 1, {0x1c, 0x0}},
	{0x320a, 1, {0x04, 0x0}},
	//0x320b,0x38,
	{0x320b, 1, {0x3a, 0x0}},
	{0x3213, 1, {0x08, 0x0}},
	{0x320e, 1, {0x05, 0x0}},
	{0x320f, 1, {0x46, 0x0}},
	{0x3641, 1, {0x0c, 0x0}},
	//mipiclk 720=27/3*2*4*10
	{0x36e9, 1, {0x44, 0x0}},
	{0x36ea, 1, {0x36, 0x0}},
	{0x36eb, 1, {0x06, 0x0}},
	{0x36ec, 1, {0x05, 0x0}},
	{0x36ed, 1, {0x14, 0x0}},
	//sclk 144.000000, 3.000000, 2.000000, 4.000000, 1.000000, 1.000000, 16.000000, 1.000000, 2.000000, 1.000000, 4.000000
	{0x36f9, 1, {0x44, 0x0}},
	{0x36fa, 1, {0x30, 0x0}},
	{0x36fb, 1, {0x13, 0x0}},
	{0x36fc, 1, {0x10, 0x0}},
	{0x36fd, 1, {0x14, 0x0}},
	{0x3641, 1, {0x00, 0x0}},
	{0x320c, 1, {0x08, 0x0}},
	{0x320d, 1, {0xa7, 0x0}},
	{0x320e, 1, {0x05, 0x0}},
	{0x320f, 1, {0x14, 0x0}},
	//20190105
	{0x3632, 1, {0x88, 0x0}},//idac_bufs
	//20190122       
	{0x320c, 1, {0x0a, 0x0}},//2560x1125
	{0x320d, 1, {0x00, 0x0}},
	{0x320e, 1, {0x04, 0x0}},
	{0x320f, 1, {0x65, 0x0}},
	//20190122       
	{0x320c, 1, {0x08, 0x0}},
	{0x320d, 1, {0x55, 0x0}},
	{0x320e, 1, {0x04, 0x0}},
	{0x320f, 1, {0x65, 0x0}},
	//20190325       
	{0x3635, 1, {0x02, 0x0}},
	{0x3902, 1, {0xc5, 0x0}},//mirror & flip, trig blc
	//0x3956,0x10,
	//20190408
	{0x3038, 1, {0x44, 0x0}},//pump driver, mclk/4=27/4
	{0x3632, 1, {0xa8, 0x0}},
	{0x3e14, 1, {0x31, 0x0}},//blc trig
	{0x3e1b, 1, {0x3a, 0x0}},
	{0x3248, 1, {0x04, 0x0}},
	{0x3901, 1, {0x00, 0x0}},//blc 24 rows
	{0x3904, 1, {0x18, 0x0}},
	{0x3987, 1, {0x0b, 0x0}},//flip
	{0x363b, 1, {0x06, 0x0}},//hvdd 3.186v
	{0x3905, 1, {0xd8, 0x0}},//one channel blc 
	{0x301f, 1, {0x29, 0x0}},//setting id
	//0410
	{0x320c, 1, {0x07, 0x0}},
	{0x320d, 1, {0xd0, 0x0}},
	//for bin mode 30fps
	{0x320c, 1, {0x0f, 0x0}},
	{0x320d, 1, {0xa0, 0x0}},
	//pll_dac vco=27/3*2*4*15=1080Mhz
	//sysclk=1080/2/4=135Mhz
	//cntclk=540Mhz
	{0x36f9, 1, {0x44, 0x0}}, // pre=3 divlane2mul=2
	{0x36fa, 1, {0x31, 0x0}}, // divpmul=15
	{0x36fb, 1, {0x13, 0x0}}, // divsys=2 divsysclk=4
	{0x36fc, 1, {0x10, 0x0}}, // divdac=2
	{0x36fd, 1, {0x14, 0x0}}, // divbitmul=4
	//pll_1 vco=27/3*5*15=675Mhz
	//mipiclk=67Mhz
	{0x36e9, 1, {0x40, 0x0}}, // pre=3
	{0x36ea, 1, {0x31, 0x0}}, // divpmul=15
	{0x36eb, 1, {0x06, 0x0}},
	{0x36ec, 1, {0x05, 0x0}},
	{0x36ed, 1, {0x24, 0x0}}, // divbitmul=5
	//countclk:sclk=4:1
	//0x3301,0x3a
	{0x3306, 1, {0xa0, 0x0}},//[hl,bs][1,84,c0]
	//0x330a,0x01,
	{0x330b, 1, {0x38, 0x0}},//[ns,of][1,117,172]
	//20190514 reduce active border
	{0x3203, 1, {0x08, 0x0}},
	{0x3207, 1, {0x87, 0x0}},
	{0x3211, 1, {0x07, 0x0}},//0x08	
	{0x3213, 1, {0x03, 0x0}},//0x04
	//init
	{0x3e00, 1, {0x00, 0x0}},
	{0x3e01, 1, {0x8c, 0x0}},
	{0x3e02, 1, {0x00, 0x0}},
	{0x3699, 1, {0x9F, 0x0}}, //for 1x to 2x flash
	//DPC
	{0x5780, 1, {0x57, 0x0}},
	{0x5785, 1, {0x00, 0x0}},
	{0x5786, 1, {0x00, 0x0}},
	{0x5799, 1, {0x07, 0x0}},  
	{SEN_CMD_SETVD, 1, {0x00, 0x0}},
	{SEN_CMD_PRESET, 1, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x00, 0x0}},      
	{0x0100, 1, {0x01, 0x0}}, // 567M count_clk 141.75M sysclk
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

static CTL_SEN_DRV_TAB sc8238_sen_drv_tab = {
	sen_open_sc8238,
	sen_close_sc8238,
	sen_sleep_sc8238,
	sen_wakeup_sc8238,
	sen_write_reg_sc8238,
	sen_read_reg_sc8238,
	sen_chg_mode_sc8238,
	sen_chg_fps_sc8238,
	sen_set_info_sc8238,
	sen_get_info_sc8238,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_sc8238(void)
{
	return &sc8238_sen_drv_tab;
}

static void sen_pwr_ctrl_sc8238(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_sc8238(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_sc8238(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_sc8238(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_sc8238);
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

static ER sen_close_sc8238(CTL_SEN_ID id)
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

static ER sen_sleep_sc8238(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_sc8238(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_sc8238(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static ER sen_read_reg_sc8238(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static UINT32 sen_get_cmd_tab_sc8238(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = sc8238_mode_1;
		return sizeof(sc8238_mode_1) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_2:
		*cmd_tab = sc8238_mode_2;
		return sizeof(sc8238_mode_2) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_sc8238(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
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
		sensor_ctrl.gain_ratio[1] = p_sensor_ctrl_temp->gain_ratio[1];
		p_sensor_ctrl_temp = isp_builtin_get_sensor_expt(id);
		sensor_ctrl.exp_time[0] = p_sensor_ctrl_temp->exp_time[0];
		sensor_ctrl.exp_time[1] = p_sensor_ctrl_temp->exp_time[1];
		sen_set_chgmode_fps_sc8238(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_sc8238(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_sc8238(id, &sensor_ctrl);
		sen_set_expt_sc8238(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_sc8238(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_sc8238(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_sc8238(0x320E, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_sc8238(id, &cmd);
			cmd = sen_set_cmd_info_sc8238(0x320F, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_sc8238(id, &cmd);
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

			sen_set_gain_sc8238(id, &sensor_ctrl);
			sen_set_expt_sc8238(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_sc8238(id, &flip);
		} else {
			cmd = sen_set_cmd_info_sc8238(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_sc8238(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_sc8238(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_sc8238(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_sc8238(0x320E, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_sc8238(id, &cmd);
	cmd = sen_set_cmd_info_sc8238(0x320F, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_sc8238(id, &cmd);

	return rt;
}

static ER sen_set_info_sc8238(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_sc8238(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_sc8238(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_sc8238(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_sc8238(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_sc8238(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_sc8238(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_sc8238(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_sc8238((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_sc8238((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_sc8238(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_sc8238((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_sc8238(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_sc8238(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_sc8238((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_sc8238((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_sc8238((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_sc8238(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_sc8238(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_sc8238(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_sc8238(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_sc8238(id, fps);
	sen_set_cur_fps_sc8238(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_sc8238(id, fps);
		sen_set_cur_fps_sc8238(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_sc8238(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_sc8238(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_sc8238(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data3[ISP_SEN_MFRAME_MAX_NUM] = {0};	
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 a_gain_h = 0, a_gain_l = 0, d_gain_h = 0, d_gain_l = 0;
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

		if (15875 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
			if (2000 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				a_gain_h = 0x03;
				a_gain_l = (sensor_ctrl->gain_ratio[frame_cnt]) * 2 * (a_gain_h + 1) / 125;
			} else if (4000 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				a_gain_h= 0x07;
				a_gain_l = (sensor_ctrl->gain_ratio[frame_cnt]) * (a_gain_h + 1) / 2 / 125;
			} else if (8000 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				a_gain_h = 0x0F;
				a_gain_l = (sensor_ctrl->gain_ratio[frame_cnt]) * (a_gain_h + 1) / 8 / 125;
			} else {
				a_gain_h = 0x1F;
				a_gain_l = (sensor_ctrl->gain_ratio[frame_cnt]) * (a_gain_h + 1) / 32 / 125;
			}

			if (0x7F < a_gain_l) { 
				a_gain_l = 0x7F;
			} else if (0x40 > a_gain_l) {
				a_gain_l = 0x40;				
			}
			
			d_gain_h = 0;
			d_gain_l = 0x80; 
		} else {
			if (31750 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				d_gain_h = 0;
			} else if (63500 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				d_gain_h = 1;
			} else if (127000 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				d_gain_h = 3;
			} else if (254000 > (sensor_ctrl->gain_ratio[frame_cnt])) {
				d_gain_h = 7;
			} else {
				d_gain_h = 0x0F;
			}

			d_gain_l = (sensor_ctrl->gain_ratio[frame_cnt]) / (d_gain_h + 1) / 124;
			if (((d_gain_l - 128) % 4) != 0) {
				d_gain_l = (d_gain_l - 128) / 4 * 4 + 128;
			}
			
			if (0xFC < d_gain_l) {
				d_gain_l = 0xFC;
			} else if (0x80 > d_gain_l) {
				d_gain_l = 0x80;
			}
			
			a_gain_h = 0x1F;
			a_gain_l = 0x7F;
		}
		data1[frame_cnt] = a_gain_h;
		data2[frame_cnt] = a_gain_l;
		data3[frame_cnt] = (d_gain_h << 16) | d_gain_l;
	}

	//Set analog high gain
	cmd = sen_set_cmd_info_sc8238(0x3E08, 1, data1[0] & 0xFF, 0x0);
	rt |= sen_write_reg_sc8238(id, &cmd);
	//Set analog low gain
	cmd = sen_set_cmd_info_sc8238(0x3E09, 1, data2[0] & 0xFF, 0x0);
	rt |= sen_write_reg_sc8238(id, &cmd);
	
	//Set digital high gain
	cmd = sen_set_cmd_info_sc8238(0x3E06, 1, (data3[0] >> 16) & 0xFF, 0x0);
	rt |= sen_write_reg_sc8238(id, &cmd);
	//Set digiral low gain
	cmd = sen_set_cmd_info_sc8238(0x3E07, 1, data3[0] & 0xFF, 0x0);
	rt |= sen_write_reg_sc8238(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_sc8238(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
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
		t_row = sen_calc_rowtime_sc8238(id, cur_sen_mode[id]);
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

	// Get fps
	chgmode_fps = sen_get_chgmode_fps_sc8238(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_sc8238(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;
	temp_line[0] = line[0];

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_sc8238(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_sc8238(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	cmd = sen_set_cmd_info_sc8238(0x320E, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_sc8238(id, &cmd);

	cmd = sen_set_cmd_info_sc8238(0x320F, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_sc8238(id, &cmd);

	//Check max exp line reg
	if (line[0] > (MAX_EXPOSURE_LINE)) {
		DBG_ERR("max line overflow \r\n");
		line[0] = MAX_EXPOSURE_LINE;
	}

	//Check max exp line
	if (line[0] > ((sensor_vd * 2) - NON_EXPOSURE_LINE)) {
		line[0] = (sensor_vd * 2) - NON_EXPOSURE_LINE;
	}


	if (MIN_EXPOSURE_LINE < (line[0])) {
		line[0] = (line[0]) / 2 * 2;
	}

	compensation_ratio[id][0] = 100 * temp_line[0] / line[0];	

	// set exposure line to sensor
	cmd = sen_set_cmd_info_sc8238(0x3E00, 1, (line[0] >> 12) & 0x0F, 0);
	rt |= sen_write_reg_sc8238(id, &cmd);
	cmd = sen_set_cmd_info_sc8238(0x3E01, 1, (line[0] >> 4) & 0xFF , 0);
	rt |= sen_write_reg_sc8238(id, &cmd);
	cmd = sen_set_cmd_info_sc8238(0x3E02, 1, (line[0] & 0x0F) << 4, 0);
	rt |= sen_write_reg_sc8238(id, &cmd);
		
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_sc8238(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_sc8238(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_sc8238(0x3221, 1, 0x0, 0x0);
	rt |= sen_read_reg_sc8238(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
				cmd.data[0] |= 0x6;
		} else {
				cmd.data[0] &= (~0x06);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x60;
		} else {
			cmd.data[0] &= (~0x60);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}
	
	rt |= sen_write_reg_sc8238(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_sc8238(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_sc8238(0x3221, 1, 0x0, 0x0);
	rt |= sen_read_reg_sc8238(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (cmd.data[0] & 0x6) {
		*flip |= CTL_SEN_FLIP_H;
	}
	
	if (cmd.data[0] & 0x60) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_sc8238(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_sc8238(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_sc8238(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_sc8238(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_sc8238(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_sc8238(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1; // if linear and shdr mode is different,the value need to separate.
}

static void sen_get_mode_basic_sc8238(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_sc8238(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_sc8238(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_sc8238(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_sc8238(id);
	data->chg_fps = sen_get_chgmode_fps_sc8238(id);
}

static void sen_get_speed_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_sc8238(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_sc8238(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
		if (data->frame_rate <= 3000) {
			data->mode = CTL_SEN_MODE_2;
			return;
		}
	} else { //8M
		if (data->frame_rate <= 3000) {
			data->mode = CTL_SEN_MODE_1;
			return;
		}
	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_sc8238(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 2;
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_sc8238(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 100 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 100000);

	return row_time;
}

static void sen_get_rowtime_sc8238(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_sc8238(id, data->mode);
	data->row_time = sen_calc_rowtime_sc8238(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_sc8238(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_sc8238(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_sc8238(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_sc8238(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_sc8238(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_sc8238(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_sc8238(SENSOR_DTSI_INFO *info)
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

	sprintf(compatible, "nvt,sen_sc8238");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_sc8238");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_sc8238;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_sc8238();
	rt = ctl_sen_reg_sendrv("nvt_sen_sc8238", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_sc8238(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_sc8238");
}

#else
static int __init sen_init_sc8238(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_sc8238;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_sc8238();
	rt = ctl_sen_reg_sendrv("nvt_sen_sc8238", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_sc8238(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_sc8238");
}

module_init(sen_init_sc8238);
module_exit(sen_exit_sc8238);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_SC8238_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

