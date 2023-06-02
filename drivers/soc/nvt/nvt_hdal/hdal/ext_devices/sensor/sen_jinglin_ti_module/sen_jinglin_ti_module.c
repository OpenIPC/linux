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
VOS_MODULE_VERSION(nvt_sen_jinglin_ti_module, 1, 42, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_JINGLIN_TI_MODULE_MODULE_NAME     "sen_jinglin_ti_module"
#define SEN_MAX_MODE               1
//#define SEN_I2C_NAME "sen_jinglin_ti_module"
#define SEN_I2C_ADDR 0x60>>1
//#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_jinglin_ti_module(void);
static void sen_pwr_ctrl_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_jinglin_ti_module(CTL_SEN_ID id);
static ER sen_close_jinglin_ti_module(CTL_SEN_ID id);
static ER sen_sleep_jinglin_ti_module(CTL_SEN_ID id);
static ER sen_wakeup_jinglin_ti_module(CTL_SEN_ID id);
static ER sen_write_reg_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static void sen_get_mode_basic_jinglin_ti_module(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_jinglin_ti_module(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_jinglin_ti_module(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_jinglin_ti_module(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_modesel_jinglin_ti_module(CTL_SENDRV_GET_MODESEL_PARAM *data);
static void sen_set_cur_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_jinglin_ti_module(CTL_SEN_ID id);
static void sen_set_chgmode_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_jinglin_ti_module(CTL_SEN_ID id);
static void sen_get_mode_dvi_jinglin_ti_module(CTL_SENDRV_GET_MODE_DVI_PARAM *data);
static void sen_get_mode_para_jinglin_ti_module(CTL_SENDRV_GET_MODE_PARA_PARAM *data);
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
	SEN_JINGLIN_TI_MODULE_MODULE_NAME,
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
		27000000,
		27000000
	},
};


static CTL_SENDRV_GET_MODE_DVI_PARAM dvi_param[SEN_MAX_MODE] = { 
	{
		CTL_SEN_MODE_1,
		CTL_SEN_DVI_CCIR656_EAV,
		CTL_SEN_DVI_DATA_MODE_SD
	},
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_PARALLEL,
		CTL_SEN_DATA_FMT_YUV,
		CTL_SEN_MODE_CCIR,
		2500,
		1,
		CTL_SEN_STPIX_YUV_UYVY,
		CTL_SEN_PIXDEPTH_8BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{768, 288},//384
		{{0, 0, 768, 288}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{768, 288},
		{0, 100, 0, 100},//don't care
		CTL_SEN_RATIO(4, 3),
		{1000, 1000},//don"t care
		100
	},
};

static CTL_SENDRV_GET_MODE_PARA_PARAM mode_param[SEN_MAX_MODE] = {
	{CTL_SEN_MODE_1, {1}},
};

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB jinglin_ti_module_sen_drv_tab = {
	sen_open_jinglin_ti_module,
	sen_close_jinglin_ti_module,
	sen_sleep_jinglin_ti_module,
	sen_wakeup_jinglin_ti_module,
	sen_write_reg_jinglin_ti_module,
	sen_read_reg_jinglin_ti_module,
	sen_chg_mode_jinglin_ti_module,
	sen_chg_fps_jinglin_ti_module,
	sen_set_info_jinglin_ti_module,
	sen_get_info_jinglin_ti_module,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_jinglin_ti_module(void)
{
	return &jinglin_ti_module_sen_drv_tab;
}

static void sen_pwr_ctrl_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	DBG_IND("enter flag %d \r\n", flag);

	if ((flag == CTL_SEN_PWR_CTRL_TURN_ON) && ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr))) {
		if (clk_cb != NULL) {
			if (sen_power[id].mclk != CTL_SEN_IGNORE) {
				clk_cb(sen_power[id].mclk, TRUE);
			}
		}

		if (sen_power[id].rst_pin != CTL_SEN_IGNORE) {
			#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
			sen_power[id].rst_pin = 0x23; //P_GPIO 0x23
			#endif
			gpio_direction_output(sen_power[id].rst_pin, 0);
			gpio_set_value(sen_power[id].rst_pin, 0);
			vos_util_delay_ms(sen_power[id].rst_time);
			gpio_set_value(sen_power[id].rst_pin, 1);
			vos_util_delay_ms(sen_power[id].stable_time);
		}
	}

	if (flag == CTL_SEN_PWR_CTRL_TURN_OFF) {
		if (clk_cb != NULL) {
			clk_cb(sen_power[id].mclk, FALSE);
		}
	}
}

static ER sen_open_jinglin_ti_module(CTL_SEN_ID id)
{
	ER rt = E_OK;

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_DEFAULT;

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		rt = E_OK; //sen_i2c_init_driver(id, sen_i2c[id].id);
	}

	if (rt != E_OK) {
		DBG_ERR("init. i2c driver fail (%d) \r\n",id);
	}

	return rt;
}

static ER sen_close_jinglin_ti_module(CTL_SEN_ID id)
{
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		//sen_i2c_remove_driver(id);
	} else {
		is_fastboot[id] = 0;
		#if defined(__KERNEL__)
		//isp_builtin_uninit_i2c(id);
		#endif
	}

	return E_OK;
}

static ER sen_sleep_jinglin_ti_module(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_jinglin_ti_module(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	return E_OK;
}

static ER sen_read_reg_jinglin_ti_module(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	return E_OK;
}

static ER sen_chg_mode_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;
	sen_set_cur_fps_jinglin_ti_module(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	sen_set_chgmode_fps_jinglin_ti_module(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	if (is_fastboot[id]) {
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;
		return E_OK;
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps)
{
	ER rt = E_OK;
	return rt;
}

static ER sen_set_info_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_jinglin_ti_module((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_jinglin_ti_module((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_jinglin_ti_module(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_jinglin_ti_module((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_jinglin_ti_module(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_jinglin_ti_module(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_jinglin_ti_module((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_jinglin_ti_module((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_DVI:
		sen_get_mode_dvi_jinglin_ti_module((CTL_SENDRV_GET_MODE_DVI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_PARA:
		sen_get_mode_para_jinglin_ti_module((CTL_SENDRV_GET_MODE_PARA_PARAM *)(data));
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static void sen_get_mode_basic_jinglin_ti_module(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_jinglin_ti_module(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_jinglin_ti_module(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static void sen_get_mode_dvi_jinglin_ti_module(CTL_SENDRV_GET_MODE_DVI_PARAM *data)
{
	UINT32 mode = data->mode;
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}	
	memcpy(data, &dvi_param[mode], sizeof(CTL_SENDRV_GET_MODE_DVI_PARAM));
}

static ER sen_get_attr_if_jinglin_ti_module(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	#if 1
	if (data->type == CTL_SEN_IF_TYPE_PARALLEL) {
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

static void sen_get_fps_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_jinglin_ti_module(id);
	data->chg_fps = sen_get_chgmode_fps_jinglin_ti_module(id);
}

static void sen_get_speed_jinglin_ti_module(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_para_jinglin_ti_module(CTL_SENDRV_GET_MODE_PARA_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mode_param[mode], sizeof(CTL_SENDRV_GET_MODE_PARA_PARAM));
}

static void sen_get_modesel_jinglin_ti_module(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_PARALLEL) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_YUV) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->frame_num == 1) {
		if ((data->size.w <= 768) && (data->size.h <= 288)) {
			if (data->frame_rate <= 2500) {
				data->mode = CTL_SEN_MODE_1;
				return;
			}
		}
	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static void sen_set_cur_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_jinglin_ti_module(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_jinglin_ti_module(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_jinglin_ti_module(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_jinglin_ti_module(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_jinglin_ti_module(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}
int sen_init_jinglin_ti_module(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX ; id++ ) {
		is_fastboot[id] = 0;
		fastboot_i2c_id[id] = 0xFFFFFFFF;
		fastboot_i2c_addr[id] = 0x0;
	}

	if (info->addr != NULL) {
		sprintf(node_path, "/sensor/sen_cfg/sen_jinglin_ti_module");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_jinglin_ti_module;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_jinglin_ti_module();
	rt = ctl_sen_reg_sendrv("nvt_sen_jinglin_ti_module", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_jinglin_ti_module(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_jinglin_ti_module");
}

#else
static int __init sen_init_jinglin_ti_module(void)
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	CFG_FILE_FMT *pcfg_file;
	CTL_SEN_REG_OBJ reg_obj;
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX ; id++ ) {
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_jinglin_ti_module;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_jinglin_ti_module();
	rt = ctl_sen_reg_sendrv("nvt_sen_jinglin_ti_module", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_jinglin_ti_module(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_jinglin_ti_module");
}

module_init(sen_init_jinglin_ti_module);
module_exit(sen_exit_jinglin_ti_module);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_JINGLIN_TI_MODULE_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

