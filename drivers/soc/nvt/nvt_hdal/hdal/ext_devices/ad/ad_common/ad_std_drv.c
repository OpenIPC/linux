/**
    @file       ad_std_drv.c

    @brief      ad_std_drv

    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/
//----------------------------------------------------------------------------------------------------

#if defined(__FREERTOS)
#include <string.h>
#include "plat/gpio.h"
#include <stdio.h>
#else
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <plat/nvt-gpio.h>
#endif

#include <kwrap/cmdsys.h>
#include "kwrap/file.h"
#include "kwrap/mem.h"
#include "kwrap/util.h"
#include "kwrap/sxcmd.h"
#include "kwrap/type.h"
#include "kflow_videocapture/ctl_sen_ext.h"

#include "ad_std_drv_param_int.h"
#include "ad_util_int.h"
#include "ad.h"
#include "ad_dbg_int.h"
#include "ad_info_parser_int.h"
#include "ad_i2c_int.h"



#define ad_i2c_init_driver AD_UTIL_DECLARE_FUNC(ad_i2c_init_driver)
#define ad_i2c_uninit_driver AD_UTIL_DECLARE_FUNC(ad_i2c_uninit_driver)
#define ad_i2c_transfer AD_UTIL_DECLARE_FUNC(ad_i2c_transfer)

#define SEN_AD_STD_MODULE_NAME AD_UTIL_STR(sen_ad)
//Internal API
static AD_DEV *g_ad_dev = NULL;

#define MAX_DBG_SET_ITEM 30
#define NULL_DBG_VAL 0xfefefefe
static UINT32 ad_dbg_set_item[CTL_SEN_ID_MAX][MAX_DBG_SET_ITEM];
//Function prototype
#if 0
#endif
#define AD_OUT_P_MAX 4
#define AD_MAX_CHIP_IDX 4

#ifdef __KERNEL__
static struct i2c_client *dev_client_info[AD_MAX_CHIP_IDX] = {
	[0 ... (AD_MAX_CHIP_IDX-1)] = NULL
};

static struct of_device_id ad_dt_match[] = {
	{.compatible = AD_UTIL_GEN_COMPAT_NAME(0)},
	{.compatible = AD_UTIL_GEN_COMPAT_NAME(1)},
	{.compatible = AD_UTIL_GEN_COMPAT_NAME(2)},
	{.compatible = AD_UTIL_GEN_COMPAT_NAME(3)},
    {}
};
MODULE_DEVICE_TABLE(of, ad_dt_match);

static struct i2c_device_id ad_i2c_ids[] = {
	{AD_UTIL_GEN_I2C_ID_NAME(0), 0},
	{AD_UTIL_GEN_I2C_ID_NAME(1), 1},
	{AD_UTIL_GEN_I2C_ID_NAME(2), 2},
	{AD_UTIL_GEN_I2C_ID_NAME(3), 3},
    {}
};
MODULE_DEVICE_TABLE(i2c, ad_i2c_ids);
#else
static UINT8 *dtsi_cfg_path = NULL;
static struct i2c_device_id ad_i2c_ids[AD_MAX_CHIP_IDX] = {
	{AD_UTIL_GEN_COMPAT_NAME(0), 0},
	{AD_UTIL_GEN_COMPAT_NAME(1), 1},
	{AD_UTIL_GEN_COMPAT_NAME(2), 2},
	{AD_UTIL_GEN_COMPAT_NAME(3), 3},
};
#endif

static AD_INFO_OUT ad0_out_p[AD_OUT_P_MAX];
static AD_INFO_OUT ad1_out_p[AD_OUT_P_MAX];
static AD_INFO_OUT ad2_out_p[AD_OUT_P_MAX];
static AD_INFO_OUT ad3_out_p[AD_OUT_P_MAX];
static AD_CFG_INFO ad_info[AD_MAX_CHIP_IDX] = {
	[0] = {
		.out_max_num = AD_OUT_P_MAX,
		.out = ad0_out_p,
	},

	[1] = {
		.out_max_num = AD_OUT_P_MAX,
		.out = ad1_out_p,
	},
	[2] = {
		.out_max_num = AD_OUT_P_MAX,
		.out = ad2_out_p,
	},
	[3] = {
		.out_max_num = AD_OUT_P_MAX,
		.out = ad3_out_p,
	},
};

static AD_ID_MAP ad_map[CTL_SEN_ID_MAX] = {
	[0] = {
		.chip_id = 0,
		.vin_id = 0,
		.vout_id = 0,
        .inited = 0,
	},

	[1] = {
		.chip_id = 0,
		.vin_id = 1,
		.vout_id = 1,
        .inited = 0,
	},

	[2] = {
		.chip_id = 0,
		.vin_id = 2,
		.vout_id = 2,
        .inited = 0,
	},

	[3] = {
		.chip_id = 0,
		.vin_id = 3,
		.vout_id = 3,
        .inited = 0,
	},
};
static UINT32 ad_init_cnt[AD_CHIP_MAX] = {
	[0 ... (AD_CHIP_MAX-1)] = 0
};

static UINT32 ad_pwr_status[AD_CHIP_MAX] = {
	[0 ... (AD_CHIP_MAX-1)] = 0
};

static SEM_HANDLE ad_chip_sem[AD_MAX_CHIP_IDX];

//=============================================================================
// global variable
//=============================================================================
/*
static SEN_I2C sen_i2c[CTL_SEN_ID_MAX] = {
	{AD_I2C_ID_1, 0x0},
};
*/
static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = { //Fixed setting
	SEN_AD_STD_MODULE_NAME,
	CTL_SEN_VENDOR_OTHERS,
	1,
	CTL_SEN_SUPPORT_PROPERTY_NONE,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = { //Fixed setting, need to modify by dtsi
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};


static CTL_SENDRV_GET_SPEED_PARAM speed_param[CTL_SEN_ID_MAX] = { //get from driver
    [0 ... (CTL_SEN_ID_MAX-1)] =
    {
		CTL_SEN_MODE_1,
		CTL_SEN_SIEMCLK_SRC_MCLK,  // need to modify by dtsi
		27000000,
		74250000,
		148500000
	},
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[CTL_SEN_ID_MAX] = { //get from driver
    [0 ... (CTL_SEN_ID_MAX-1)] =
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,   // need to modify by dtsi
		CTL_SEN_DATALANE_2,
		{ {CTL_SEN_MIPI_MANUAL_YUV422, CTL_SEN_MIPI_PIXEL_DATA}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },// fixed
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
};

static CTL_SENDRV_GET_MODE_PARA_PARAM parallel_param[CTL_SEN_ID_MAX] = {
    [0 ... (CTL_SEN_ID_MAX-1)] =
	{CTL_SEN_MODE_1, {1}}
};

static CTL_SENDRV_GET_MODE_DVI_PARAM dvi_param[CTL_SEN_ID_MAX] = { //get from driver
    [0 ... (CTL_SEN_ID_MAX-1)] =
	{
		CTL_SEN_MODE_1,
		CTL_SEN_DVI_CCIR601,
		CTL_SEN_DVI_DATA_MODE_SD // depend on : CTL_SEN_DVI_FMT fmt;
	},
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[CTL_SEN_ID_MAX] = { //get from driver
    [0 ... (CTL_SEN_ID_MAX-1)] =
	{
		// 1920x1080p30
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_YUV, //fixed
		CTL_SEN_MODE_CCIR, //depend on prog
		3000, //by fps
		1, //fixed
		CTL_SEN_STPIX_YUV_UYVY,
		CTL_SEN_PIXDEPTH_8BIT, //fixed
		CTL_SEN_FMT_POGRESSIVE, //fixed
		{3840, 1080}, //by size
		{{0, 0, 3840, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, //by size
		{3840, 1080}, //by size
		{0, 1920, 0, 1080}, //ignore
		CTL_SEN_RATIO(16, 9),
		{0, 10000}, //ignore
		100 //ignore
	},
};

static CTL_SEN_CSI_EN_CB ad_std_csi_en_cb = NULL;

static UINT8 ad_get_outport_idx(UINT8 chip_id, UINT8 inport)
{
	UINT8 i, out_port = AD_OUT_P_MAX;

	if (chip_id >= AD_MAX_CHIP_IDX) {
		DBG_ERR("chip id %d > max (%d)\r\n", chip_id, AD_MAX_CHIP_IDX-1);
        chip_id = 0;
	}

	for (i = 0; i < AD_OUT_P_MAX; i++) {
		if (ad_info[chip_id].out[i].in & (1 << inport)) {
			out_port = i;
			break;
		}
	}

	if (out_port == AD_OUT_P_MAX) {
		DBG_ERR("outport search fail!! chip id=%d, inport=%d\r\n", chip_id, inport);
		out_port = 0;
	}

	return out_port;
}

static void sen_get_mode_basic_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode != g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx) {
		DBG_ERR("mode %d not match current setting(%d)\r\n", mode, g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx);
	}
	memcpy(mode_basic, &mode_basic_param[id], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static void sen_get_speed_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
{
	//UINT32 mode = data->mode;
	AD_MODE_INFO mode_info;

	if (data->mode != g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx) {
        mode_info.mode = CTL_SEN_MODE_1;
	} else {
    	mode_info.mode = data->mode;
    }

    mode_info.out_id = ad_map[id].vout_id;
	if (g_ad_dev->op.get_cfg(ad_map[id].chip_id, AD_CFGID_MODE_INFO, (void *)&mode_info) == E_OK) {
		//ad_update_mode_info(id, mode_info);
		speed_param[id].mclk = mode_info.in_clock;
		speed_param[id].pclk = mode_info.out_clock;
		speed_param[id].data_rate = mode_info.data_rate;
	} else {
		DBG_ERR("id%d: chip%d, out_id%d get mode sel fail!!\r\n", id, ad_map[id].chip_id, ad_map[id].vout_id);
	}

	memcpy(data, &speed_param[id], sizeof(CTL_SENDRV_GET_SPEED_PARAM));
}

static void sen_get_mode_mipi_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	AD_VIRTUAL_CHANNEL_INFO virtual_ch = {0};
	AD_MIPI_MANUAL_DATA_TYPE data_type[CTL_SEN_MIPI_MAX_MANUAL] = {0};
	UINT32 mode = data->mode, i;

	if (mode != g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx) {
		DBG_ERR("mode %d not match current setting(%d)\r\n", mode, g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx);
	}

	virtual_ch.vin_id = ad_map[id].vin_id;
	virtual_ch.vout_id = ad_map[id].vout_id;
	g_ad_dev->op.get_cfg(ad_map[id].chip_id, AD_CFGID_VIRTUAL_CH_ID, &virtual_ch);
	mipi_param[id].sel_frm_id[0] = virtual_ch.id;

	for (i = 0; i < CTL_SEN_MIPI_MAX_MANUAL; i++) {
		data_type[i].vin_id = ad_map[id].vin_id;
		data_type[i].vout_id = ad_map[id].vout_id;
		data_type[i].manual_idx = i;
		data_type[i].data_info.bit = CTL_SEN_MIPI_MANUAL_NONE;

		g_ad_dev->op.get_cfg(ad_map[id].chip_id, AD_CFGID_MIPI_MANUAL_ID, &data_type[i]);
		if (data_type[i].data_info.bit != CTL_SEN_MIPI_MANUAL_NONE)
			mipi_param[id].manual_info[i] = data_type[i].data_info;
	}

	memcpy(data, &mipi_param[id], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}


static void sen_get_mode_parallel_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_PARA_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode != g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx) {
		DBG_ERR("mode %d not match current setting(%d)\r\n", mode, g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx);
	}
	memcpy(data, &parallel_param[id], sizeof(CTL_SENDRV_GET_MODE_PARA_PARAM));
}

static void sen_get_mode_dvi_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_DVI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode != g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx) {
		DBG_ERR("mode %d not match current setting(%d)\r\n", mode, g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx);
	}
	memcpy(data, &dvi_param[id], sizeof(CTL_SENDRV_GET_MODE_DVI_PARAM));
}

static ER sen_get_attr_cmdif_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	data->info.i2c.ch = ad_info[ad_map[id].chip_id].i2c.ch;
	data->info.i2c.w_addr_info[0].w_addr = ad_info[ad_map[id].chip_id].i2c.slv_addr << 1;
	data->info.i2c.w_addr_info[0].w_addr_sel = CTL_SEN_I2C_W_ADDR_DFT;

	return E_OK;
}

static ER sen_get_attr_if_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		return E_OK;
	}
	return E_NOSPT;
}

static void sen_get_fps_ad_std(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].cur_format.fps;
	data->chg_fps = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.fps;
}

static void ad_update_mode_info(CTL_SEN_ID id, AD_MODE_INFO mode_info)
{
	VDO_INFO ui_setting;

	ui_setting = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format;
	speed_param[id].mclk = mode_info.in_clock;
	speed_param[id].pclk = mode_info.out_clock;
	speed_param[id].data_rate = mode_info.data_rate;

	switch(mode_info.protocal) {
		case AD_DATA_PROTCL_CCIR601:
			dvi_param[id].fmt = CTL_SEN_DVI_CCIR601;
			dvi_param[id].data_mode = CTL_SEN_DVI_DATA_MODE_SD;
			break;
		case AD_DATA_PROTCL_CCIR656:
			dvi_param[id].fmt = CTL_SEN_DVI_CCIR656_EAV;
			dvi_param[id].data_mode = CTL_SEN_DVI_DATA_MODE_SD;
			break;
		case AD_DATA_PROTCL_CCIR709:
			dvi_param[id].fmt = CTL_SEN_DVI_CCIR709;
			dvi_param[id].data_mode = CTL_SEN_DVI_DATA_MODE_HD;
			break;
		case AD_DATA_PROTCL_CCIR1120:
			dvi_param[id].fmt = CTL_SEN_DVI_CCIR656_1120_EAV;
			dvi_param[id].data_mode = CTL_SEN_DVI_DATA_MODE_HD;
			break;
		default:
			dvi_param[id].fmt = CTL_SEN_DVI_CCIR601;
			dvi_param[id].data_mode = CTL_SEN_DVI_DATA_MODE_SD;
			DBG_ERR("AD protocal error(%d)\r\n", mode_info.protocal);
			break;
	}


	switch(mode_info.bus_type) {
		case AD_BUS_TYPE_PARALLEL:
			mode_basic_param[id].if_type = CTL_SEN_IF_TYPE_PARALLEL;
			parallel_param[id].mux_info.mux_data_num = mode_info.mux_num; //the mux_idx is selected by isf_vdocap
			break;
		case AD_BUS_TYPE_SERIAL:
			mode_basic_param[id].if_type = CTL_SEN_IF_TYPE_MIPI;
			mipi_param[id].data_lane = mode_info.mipi_lane_num;
			break;
		default:
			DBG_ERR("AD bus_type error(%d)\r\n", mode_info.bus_type);
			break;
	}

	if (ui_setting.prog == TRUE)
		mode_basic_param[id].mode_type = CTL_SEN_MODE_CCIR;
	else
		mode_basic_param[id].mode_type = CTL_SEN_MODE_CCIR_INTERLACE;

	mode_basic_param[id].dft_fps = ui_setting.fps;

	switch(mode_info.yuv_order) {
		case AD_DATA_ORDER_YUYV:
			mode_basic_param[id].stpix = CTL_SEN_STPIX_YUV_YUYV;
			break;
		case AD_DATA_ORDER_YVYU:
			mode_basic_param[id].stpix = CTL_SEN_STPIX_YUV_YVYU;
			break;
		case AD_DATA_ORDER_UYVY:
			mode_basic_param[id].stpix = CTL_SEN_STPIX_YUV_UYVY;
			break;
		case AD_DATA_ORDER_VYUY:
			mode_basic_param[id].stpix = CTL_SEN_STPIX_YUV_VYUY;
			break;
		default:
			DBG_ERR("AD yuv_order error(%d)\r\n", mode_info.yuv_order);
			break;
	}

	if (dvi_param[id].data_mode == CTL_SEN_DVI_DATA_MODE_SD)
		ui_setting.width *= 2;

	mode_basic_param[id].valid_size.w = ui_setting.width;
	mode_basic_param[id].valid_size.h = ui_setting.height;
	mode_basic_param[id].crp_size.w = ui_setting.width;
	mode_basic_param[id].crp_size.h = ui_setting.height;
	mode_basic_param[id].act_size[0].w = ui_setting.width;
	mode_basic_param[id].act_size[0].h = ui_setting.height;
	mode_basic_param[id].act_size[0].x = mode_info.crop_start.ofst_x;
	mode_basic_param[id].act_size[0].y = mode_info.crop_start.ofst_y;
}

static ER init_ad_std_drv(CTL_SEN_ID id);
static ER uninit_ad_std_drv(CTL_SEN_ID id);
static ER sleep_ad_std_drv(CTL_SEN_ID id);
static ER wakeup_ad_std_drv(CTL_SEN_ID id);
static ER write_reg_ad_std_drv(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER read_reg_ad_std_drv(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER chg_mode_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER chg_fps_ad_std_drv(CTL_SEN_ID id, UINT32 frame_rate);
static ER set_info_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER get_info_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);

static CTL_SEN_DRV_TAB m_p_ad_std_drv_drv = {
	init_ad_std_drv,
	uninit_ad_std_drv,
	sleep_ad_std_drv,
	wakeup_ad_std_drv,
	write_reg_ad_std_drv,
	read_reg_ad_std_drv,
	chg_mode_ad_std_drv,
	chg_fps_ad_std_drv,
	set_info_ad_std_drv,
	get_info_ad_std_drv,
};

#if 0
-Intern api
#endif

static void ad_wait_sem(CTL_SEN_ID id)
{
	vos_sem_wait(ad_chip_sem[ad_map[id].chip_id]);
}

static void ad_sig_sem(CTL_SEN_ID id)
{
	vos_sem_sig(ad_chip_sem[ad_map[id].chip_id]);
}

static ER sen_i2c_rw_if(UINT32 chip_id, AD_I2C_RW rw_flg, AD_I2C_INFO i2c_data)
{
	struct i2c_msg msgs[2];
	ER rlt;

	if(rw_flg == AD_I2C_W) {
		msgs[0].addr  = g_ad_dev->chip[chip_id].i2c_addr;
		msgs[0].flags = 0;
		msgs[0].len   = i2c_data.addr_len + i2c_data.data_len;
		msgs[0].buf   = i2c_data.data;
		//DBG_DUMP("buf= 0x%x 0x%x 0x%x 0x%x\r\n", i2c_data.data[0], i2c_data.data[1], i2c_data.data[2], i2c_data.data[3]);
		rlt = ad_i2c_transfer(chip_id, msgs, 1);
	} else {
		msgs[0].addr  = g_ad_dev->chip[chip_id].i2c_addr;
		msgs[0].flags = 0;
		msgs[0].len   = i2c_data.addr_len;
		msgs[0].buf   = i2c_data.data;

		msgs[1].addr  = g_ad_dev->chip[chip_id].i2c_addr;
		msgs[1].flags = 1;
		msgs[1].len   = i2c_data.data_len;
		msgs[1].buf   = &i2c_data.data[i2c_data.addr_len];

		rlt = ad_i2c_transfer(chip_id, msgs, 2);
	}
	return rlt;
}

static void reset_dbg_default_value(void)
{
    UINT8 sen_id, item_idx;

    for (sen_id = 0; sen_id < CTL_SEN_ID_MAX; sen_id ++) {
        for (item_idx = 0; item_idx < MAX_DBG_SET_ITEM; item_idx ++) {
            ad_dbg_set_item[sen_id][item_idx] = NULL_DBG_VAL;
        }
    }
}
static ER init_ad_std_drv(CTL_SEN_ID id)
{
	ER rt = E_OK;
	AD_INIT_INFO ad_init_info = {0};
    UINT8 i;

	g_ad_dev->chip[ad_map[id].chip_id].rev[0] = id;
	ad_init_info.i2c_ch = ad_info[ad_map[id].chip_id].i2c.ch;
	ad_init_info.i2c_addr = ad_info[ad_map[id].chip_id].i2c.slv_addr;
	ad_init_info.pwr_info.mclk_sel = ad_info[ad_map[id].chip_id].pwr.mclk_sel;
    switch (ad_init_info.pwr_info.mclk_sel) {
        case CTL_SEN_CLK_SEL_SIEMCLK:
            speed_param[id].mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK;
            break;
        case CTL_SEN_CLK_SEL_SIEMCLK2:
            speed_param[id].mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK2;
            break;
        case CTL_SEN_CLK_SEL_SIEMCLK3:
            speed_param[id].mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK3;
            break;
        case CTL_SEN_CLK_SEL_SIEMCLK_IGNORE:
            speed_param[id].mclk_src = CTL_SEN_SIEMCLK_SRC_IGNORE;
            break;
        default:
            DBG_ERR("id%d, clk src error(%d)!\r\n", (int)id, ad_init_info.pwr_info.mclk_sel);
            speed_param[id].mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK;
            break;
    }

	if (ad_init_cnt[ad_map[id].chip_id] != 0) {
		ad_init_cnt[ad_map[id].chip_id] += 1;
		return rt;
	}
	ad_init_cnt[ad_map[id].chip_id] += 1;

    for (i = 0 ; i < AD_PIN_CTRL_MAX; i++) {
        if (ad_info[ad_map[id].chip_id].pwr.io_des[i].pin == AD_CFG_INVALID_VALUE)
            break;
        ad_init_info.pwr_info.pin_ctrl[i].pin = ad_info[ad_map[id].chip_id].pwr.io_des[i].pin;
        ad_init_info.pwr_info.pin_ctrl[i].lvl = ad_info[ad_map[id].chip_id].pwr.io_des[i].lvl;
        ad_init_info.pwr_info.pin_ctrl[i].hold_time = ad_info[ad_map[id].chip_id].pwr.io_des[i].hold_time;
    }
    if (i == AD_PIN_CTRL_MAX) {
		DBG_ERR("id%d, out pin ctrl over than %d!\r\n", (int)id, AD_PIN_CTRL_MAX);
        ad_init_info.pwr_info.total_ctl_cnt = AD_PIN_CTRL_MAX;
    } else {
        ad_init_info.pwr_info.total_ctl_cnt = i;
    }

	//rt |= g_ad_dev->op.init(ad_map[id].chip_id, NULL);
#if defined(__FREERTOS)
	ad_i2c_init_driver(SEN_AD_STD_MODULE_NAME, ad_map[id].chip_id, ad_init_info.i2c_ch, ad_init_info.i2c_addr);
#else
    ad_i2c_init_driver(ad_map[id].chip_id, dev_client_info[ad_map[id].chip_id]);
#endif
	rt |= g_ad_dev->op.open(ad_map[id].chip_id, &ad_init_info);

	if (rt != E_OK) {
		DBG_ERR("id%d, open fail\r\n", (int)id);
	}

	return rt;
}

static ER uninit_ad_std_drv(CTL_SEN_ID id)
{
	ER rt = E_OK;

	if (ad_init_cnt[ad_map[id].chip_id] > 0) {
		ad_init_cnt[ad_map[id].chip_id] -= 1;
	}

	if (ad_init_cnt[ad_map[id].chip_id] != 0) {
		return rt;
	}

	rt |= g_ad_dev->op.uninit(ad_map[id].chip_id, NULL);
	rt |= g_ad_dev->op.close(ad_map[id].chip_id, NULL);

	ad_i2c_uninit_driver(ad_map[id].chip_id);

	return rt;
}

static ER sleep_ad_std_drv(CTL_SEN_ID id)
{
	return E_OK;
}

static ER wakeup_ad_std_drv(CTL_SEN_ID id)
{
	return E_OK;
}

static ER write_reg_ad_std_drv(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	if (cmd->data_len == 2) {
		cmd->data[0] = cmd->data[0] | ((cmd->data[1] << 8) &0xff00);
	}
	return g_ad_dev->op.i2c_write(ad_map[id].chip_id, cmd->addr, cmd->data[0], cmd->data_len);
}

static ER read_reg_ad_std_drv(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	return g_ad_dev->op.i2c_read(ad_map[id].chip_id, cmd->addr, &(cmd->data[0]), cmd->data_len);
}

static ER chg_mode_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{

	AD_CHGMODE_INFO ad_chgmode_info;
	ER rt = E_OK;

	ad_wait_sem(id);
	g_ad_dev->chip[ad_map[id].chip_id].vout[ad_map[id].vout_id].depth = AD_DATA_DEPTH_8BIT;
	g_ad_dev->chip[ad_map[id].chip_id].vout[ad_map[id].vout_id].mux_num = ad_info[ad_map[id].chip_id].out[ad_map[id].vout_id].in_num;

	ad_chgmode_info.cur_update = FALSE;
	ad_chgmode_info.vin_id = ad_map[id].vin_id;
	ad_chgmode_info.vout_id = ad_map[id].vout_id;
	ad_chgmode_info.mode = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format.mode_idx;
	ad_chgmode_info.ui_info = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format;
	ad_chgmode_info.cur_info = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].cur_format;
	rt = g_ad_dev->op.chgmode(ad_map[id].chip_id, &ad_chgmode_info);
	ad_sig_sem(id);

    if ((mode_basic_param[id].if_type == CTL_SEN_IF_TYPE_MIPI) && ad_std_csi_en_cb) {
        ad_std_csi_en_cb(id, TRUE);
    }

	if (rt != E_OK) {
		DBG_ERR("id%d, chgmode fail\r\n", (int)id);
		return rt;
	}
	DBG_DUMP("^Gid%d chgmode done.\r\n", id);

	if (ad_chgmode_info.cur_update == TRUE)
		g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].cur_format = ad_chgmode_info.cur_info;
	return E_OK;
}

static BOOL sen_plug_in_ad_std(CTL_SEN_ID id)
{
	AD_DECET_INFO ad_detection;

	ad_detection.vin_id = ad_map[id].vin_id;
	g_ad_dev->op.det_plug_in(ad_map[id].chip_id, &ad_detection);

	g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].vloss = ad_detection.vin_loss;
	if (ad_detection.vin_loss == FALSE) {
		g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].det_format = ad_detection.det_info;
		return TRUE;
	} else {
		return FALSE;
	}
}


static ER chg_fps_ad_std_drv(CTL_SEN_ID id, UINT32 frame_rate)
{
	ER rt = E_OK;
	return rt;
}

#if 0
#endif

static ER set_info_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;
	CTL_SENDRV_AD_ID_MAP_PARAM *ad_map_setting;
	CTL_SENDRV_GET_MODE_MIPI_EN_USER mipi_en_user;
    CTL_SENDRV_AD_IMAGE_PARAM *ad_type;
    AD_INFO_CFG cfg_info;
    UINT8 idx;

    for (idx = 0; idx < MAX_DBG_SET_ITEM; idx++) {
        if (ad_dbg_set_item[id][idx] == drv_cfg_id)
            break;
        if (ad_dbg_set_item[id][idx] == NULL_DBG_VAL) {
            ad_dbg_set_item[id][idx] = drv_cfg_id;
            break;
        }
    }

	switch (drv_cfg_id) {
/*
		case CTL_SENDRV_CFGID_AD_CUR_VCAP_ID:
			ad_map[id].chip_id = (UINT32 *)data;
			break;
*/
		case CTL_SENDRV_CFGID_AD_ID_MAP:
			ad_map_setting = (CTL_SENDRV_AD_ID_MAP_PARAM *)data;
			ad_map[id].chip_id = ad_map_setting->chip_id;
			ad_map[id].vin_id = ad_map_setting->vin_id;
			//ad_map[id].vout_id = ad_map_setting->vout_id;
			ad_map[id].vout_id = ad_get_outport_idx(ad_map_setting->chip_id, ad_map_setting->vin_id);
            ad_map[id].inited = TRUE;
			break;
		case CTL_SENDRV_CFGID_FLIP_TYPE:
			//sen_set_flip_ad_tc358743(id, (CTL_SEN_FLIP *)(data));
			break;
		case CTL_SENDRV_CFGID_AD_INIT:
			g_ad_dev->op.init(ad_map[id].chip_id, (void *)&ad_map[id]);
			break;
        case CTL_SENDRV_CFGID_GET_MODE_MIPI_EN_USER:
            mipi_en_user = *(CTL_SENDRV_GET_MODE_MIPI_EN_USER *)data;
            ad_std_csi_en_cb = mipi_en_user.csi_en_cb;
            break;
        case CTL_SENDRV_CFGID_AD_TYPE:
            ad_type = (CTL_SENDRV_AD_IMAGE_PARAM *)data;
            cfg_info.vin_id = ad_type->vin_id;
            cfg_info.data = ad_type->val;
            g_ad_dev->op.set_cfg(ad_map[id].chip_id, AD_CFGID_DET_CAM_TYPE, (void *)&cfg_info);
            break;
		default:
			return E_NOSPT;
	}

	return rt;
}

#if 0
#endif

static ER get_info_ad_std_drv(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;
	CTL_SENDRV_GET_MODESEL_PARAM *mode_setting;
	VDO_INFO usr_setting;
	AD_MODE_INFO mode_info = {0};
	CTL_SENDRV_GET_PLUG_INFO_PARAM *plug_info;

	switch (drv_cfg_id) {
		case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
			sen_get_attr_basic_ad_std(id, (CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
			sen_get_attr_signal_ad_std(id, (CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
			rt = sen_get_attr_cmdif_ad_std(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_ATTR_IF:
			rt = sen_get_attr_if_ad_std(id, (CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_FPS:
			sen_get_fps_ad_std(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_SPEED:
			sen_get_speed_ad_std(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_MODE_BASIC:
			sen_get_mode_basic_ad_std(id, (CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_MODE_MIPI:
			sen_get_mode_mipi_ad_std(id, (CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_MODE_PARA:
			sen_get_mode_parallel_ad_std(id, (CTL_SENDRV_GET_MODE_PARA_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_MODESEL:
			mode_setting = (CTL_SENDRV_GET_MODESEL_PARAM *)data;
			//usr_setting.width = mode_setting->size.w << 1;
			usr_setting.width = mode_setting->size.w;
			usr_setting.height = mode_setting->size.h;
			usr_setting.fps = mode_setting->frame_rate;
			usr_setting.prog = !mode_setting->ccir.interlace;
			//rt = g_ad_dev->op.get_cfg(ad_map[id].chip_id, AD_CFGID_MODE_IDX_SEL, (void *)&usr_setting); //get mode_idx from driver
			usr_setting.mode_idx = CTL_SEN_MODE_1;
			//if (rt != E_OK) {
			//	DBG_ERR("id%d: chip%d, out_id%d get mode sel fail!!\r\n", id, ad_map[id].chip_id, ad_map[id].vout_id);
			//} else {
				mode_setting->mode = usr_setting.mode_idx;
				g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].ui_format = usr_setting;
                g_ad_dev->chip[ad_map[id].chip_id].vout[ad_map[id].vout_id].data_lane_num = mode_setting->data_lane;

				mode_info.mode = usr_setting.mode_idx;
				mode_info.out_id = ad_map[id].vout_id;
				if (g_ad_dev->op.get_cfg(ad_map[id].chip_id, AD_CFGID_MODE_INFO, (void *)&mode_info) == E_OK) {
					ad_update_mode_info(id, mode_info);
				} else {
					DBG_ERR("id%d: chip%d, out_id%d get mode sel fail!!\r\n", id, ad_map[id].chip_id, ad_map[id].vout_id);
				}
			//}
			break;
		case CTL_SENDRV_CFGID_GET_MODE_DVI:
			sen_get_mode_dvi_ad_std(id, (CTL_SENDRV_GET_MODE_DVI_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_FLIP_TYPE:
			//sen_get_flip_ad_std(id, (CTL_SEN_FLIP *)(data));
			break;
		case CTL_SENDRV_CFGID_GET_PROBE_SEN:
			//sen_ext_get_probe_sen_ad_std(id, (CTL_SENDRV_GET_PROBE_SEN_PARAM *)(data));
			break;
		case CTL_SENDRV_CFGID_USER_DEFINE2:
			//sen_get_min_expt_ad_std(id, data);
			break;
		case CTL_SENDRV_CFGID_GET_PLUG_INFO:
			if (g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].vloss == FALSE) {
				plug_info = (CTL_SENDRV_GET_PLUG_INFO_PARAM *)(data);
				plug_info->size.w = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].det_format.width;
				plug_info->size.h = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].det_format.height;
				plug_info->fps = g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].det_format.fps;
                plug_info->interlace = !g_ad_dev->chip[ad_map[id].chip_id].vin[ad_map[id].vin_id].det_format.prog;
				rt = E_OK;
			} else {
				rt = E_NOEXS;
			}
			break;
        case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
            break;
		default:
			rt = E_NOSPT;
            break;
	}
	return rt;
}

#if 0
-Extern api
#endif

static void sen_pwr_ctrl_ad_std_drv(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
    UINT8 chip_id = ad_map[id].chip_id;
	BOOL by_pass_setting = FALSE;

    if (id >= CTL_SEN_ID_MAX)
        return;

    switch (flag) {
        case CTL_SEN_PWR_CTRL_TURN_ON:
            if (ad_pwr_status[chip_id] != 0)
                by_pass_setting = TRUE;
            else
                by_pass_setting = FALSE;

            ad_pwr_status[chip_id] |= (1 << id);
            break;
        case CTL_SEN_PWR_CTRL_TURN_OFF:
            ad_pwr_status[chip_id] &= (~(1 << id));

            if (ad_pwr_status[chip_id] != 0)
                by_pass_setting = TRUE;
            else {
                by_pass_setting = FALSE;
				if ((mode_basic_param[id].if_type = CTL_SEN_IF_TYPE_MIPI) && ad_std_csi_en_cb) {
					ad_std_csi_en_cb(id, FALSE);
				}
            }
            break;
        default:
            by_pass_setting = FALSE;
            break;
    }

    if (by_pass_setting == FALSE)
        g_ad_dev->op.pwr_ctl(chip_id, flag, clk_cb);
}

static BOOL det_plug_in_ad_std_drv(CTL_SEN_ID id)
{
	BOOL plug_in;
	plug_in = sen_plug_in_ad_std(id);
	return plug_in;
}


#if 0
#endif

/*
    PROC CMD ENTRY
*/
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     30 //10

#define AD_PROC_NAME AD_UTIL_STR(ad_proc)//"ad_proc"
static BOOL nvt_ad_proc_ad_mapinfo(unsigned char argc, char **pargv)
{
	int sen_id = 0xff;

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("sen id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

	DBG_DUMP("sen id [%d] : chip_id[%d] - vin[%d] / vout[%d] / inited=%d\r\n", sen_id, ad_map[sen_id].chip_id, ad_map[sen_id].vin_id, ad_map[sen_id].vout_id, ad_map[sen_id].inited);

	return 0;
}

static BOOL nvt_ad_proc_i2c_info(unsigned char argc, char **pargv)
{
	int chip_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &chip_id);

	if (chip_id >= AD_MAX_CHIP_IDX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", chip_id, AD_MAX_CHIP_IDX-1);
		return -1;
	}

	DBG_DUMP("chip id [%d] : ch[%d] / slave addr[0x%x](7-bit)\r\n", chip_id, ad_info[chip_id].i2c.ch, ad_info[chip_id].i2c.slv_addr);

	return 0;
}

static BOOL nvt_ad_proc_basic_info(unsigned char argc, char **pargv)
{
	int chip_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &chip_id);

	if (chip_id >= AD_MAX_CHIP_IDX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", chip_id, AD_MAX_CHIP_IDX-1);
		return -1;
	}

	DBG_DUMP("chip id [%d] : name[%s]\r\n", chip_id, basic_param.name);

	return 0;
}

static BOOL nvt_ad_proc_speed_info(unsigned char argc, char **pargv)
{
	int sen_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

	DBG_DUMP("sen id [%d] : mclk[%d] / pclk[%d] / data rate[%d]\r\n", sen_id, speed_param[sen_id].mclk, speed_param[sen_id].pclk, speed_param[sen_id].data_rate);

	return 0;
}

static BOOL nvt_ad_proc_mipi_info(unsigned char argc, char **pargv)
{
	int sen_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

	DBG_DUMP("sen id [%d] : data_lane[%d] / channel_id[%d]\r\n", sen_id, mipi_param[sen_id].data_lane, mipi_param[sen_id].sel_frm_id[0]);

	return 0;
}

static BOOL nvt_ad_proc_dvi_info(unsigned char argc, char **pargv)
{
	int sen_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

	DBG_DUMP("sen id [%d] : format[%d] / data_bit[%d]\r\n", sen_id, dvi_param[sen_id].fmt, 8 << dvi_param[sen_id].data_mode);

	return 0;
}

static BOOL nvt_ad_proc_size_info(unsigned char argc, char **pargv)
{
	int sen_id = 0xff;
	//char cmd[30];

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

	DBG_DUMP("sen id [%d] : startpixel=%d, crop size(%dx%d)\r\n", sen_id, mode_basic_param[sen_id].stpix, mode_basic_param[sen_id].crp_size.w, mode_basic_param[sen_id].crp_size.h);

	return 0;
}

static BOOL nvt_ad_proc_dtsi_info(unsigned char argc, char **pargv)
{
#ifdef __KERNEL__
	int sen_id = 0xff;

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= AD_MAX_CHIP_IDX) {
		DBG_ERR("chip id = %d over than max(%d)\r\n", sen_id, AD_MAX_CHIP_IDX-1);
		return -1;
	}
	AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(dev_client_info[sen_id]);
#else

	int sen_id = 0xff;
	UINT32 i;
	struct i2c_device_id *dev_id = 0;

	sscanf(pargv[0], "%d", &sen_id);

	for (i = 0; i < AD_MAX_CHIP_IDX; i ++) {
		if (ad_i2c_ids[i].rev == (UINT32)sen_id) {
			dev_id = &ad_i2c_ids[i];
			break;
		}
	}
	if (dev_id != 0) {
		AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(dtsi_cfg_path, dev_id);
	}
#endif
	return 0;
}


static BOOL nvt_ad_proc_set_det_fmt(unsigned char argc, char **pargv)
{
	int sen_id = 0xff, fmt = 0;
    AD_INFO_CFG cfg_info;
#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
    char *data;

    data = pargv[0];

	sscanf(data, "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("sensor id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}

    sscanf(data+1, "%d", &fmt);
#else
	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("sensor id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}
    sscanf(pargv[1], "%d", &fmt);
#endif
    cfg_info.vin_id = ad_map[sen_id].vin_id;
    cfg_info.data = fmt;
	DBG_DUMP("chip[%d]/Vin[%d] : det_fmt=%d\r\n", ad_map[sen_id].chip_id, ad_map[sen_id].vin_id, fmt);
    g_ad_dev->op.set_cfg(ad_map[sen_id].chip_id, AD_CFGID_DET_CAM_TYPE, (void *)&cfg_info);
	return 0;
}

static BOOL nvt_ad_proc_dumpinfo(unsigned char argc, char **pargv)
{
	int sen_id = 0xff, chip_id, idx;
    char *fmt_name[CTL_SEN_DVI_FMT_MAX_NUM] = {"601", "656_EAV", "656_ACT", "709", "xxx_0", "1120_EAV", "1120_ACT"};
    char *st_px_name[4] = {"YUYV", "YVYU", "UYVY", "VYUY"};
    char *bus_name[CTL_SEN_IF_TYPE_MAX_NUM] = {"parallel", "lvds", "mipi", "slvsec", "dummy", "sie_pat"};

	sscanf(pargv[0], "%d", &sen_id);

	if (sen_id >= CTL_SEN_ID_MAX) {
		DBG_ERR("sen id = %d over than max(%d)\r\n", sen_id, CTL_SEN_ID_MAX-1);
		return -1;
	}
	chip_id = ad_map[sen_id].chip_id;

	DBG_DUMP("==================== ad std driver ver : %s ====================\r\n", AD_MODULE_VERSION);
	DBG_DUMP("signal : sen_id[%d] --> chip %s id[%d] - vin[%d] / vout[%d] / inited=%d\r\n", sen_id, g_ad_dev->chip[chip_id].name, chip_id, ad_map[sen_id].vin_id, ad_map[sen_id].vout_id, ad_map[sen_id].inited);
    DBG_DUMP("vout   : mux_num[%d] / contain vin[0x%x](bit mapping)\r\n", g_ad_dev->chip[chip_id].vout[ad_map[sen_id].vout_id].mux_num, g_ad_dev->chip[chip_id].vout[ad_map[sen_id].vout_id].vout2vin_map);
	DBG_DUMP("i2c    : i2c ch[%d] / slave addr[0x%x](7-bit)\r\n", ad_info[chip_id].i2c.ch, ad_info[chip_id].i2c.slv_addr);
	DBG_DUMP("clock  : mclk_src[%d] / mclk[%d] / pclk[%d] / data rate[%d]\r\n", speed_param[sen_id].mclk_src, speed_param[sen_id].mclk, speed_param[sen_id].pclk, speed_param[sen_id].data_rate);
	DBG_DUMP("mipi   : data_lane[%d/%d] / channel_id[%d]\r\n", mipi_param[sen_id].data_lane, g_ad_dev->chip[chip_id].vout[ad_map[sen_id].vout_id].data_lane_num, mipi_param[sen_id].sel_frm_id[0]);
	DBG_DUMP("dvi    : format[CCIR%s] / field[%s] / pixel array[%s]\r\n", fmt_name[dvi_param[sen_id].fmt], mode_basic_param[sen_id].mode_type == CTL_SEN_MODE_CCIR_INTERLACE ? "interlace" : "progressive", st_px_name[mode_basic_param[sen_id].stpix-CTL_SEN_STPIX_YUV_YUYV]);
	DBG_DUMP("roi    : crop size[%dx%d] / act start[%d,%d]\r\n", mode_basic_param[sen_id].crp_size.w, mode_basic_param[sen_id].crp_size.h, mode_basic_param[sen_id].act_size[0].x, mode_basic_param[sen_id].act_size[0].y);
	DBG_DUMP("data   : bus[%s] / bit[%d]\r\n", bus_name[mode_basic_param[sen_id].if_type], 8 << dvi_param[sen_id].data_mode);
	DBG_DUMP("----------------------------------------------------------------------\r\n");
    DBG_DUMP("usr setting w=%d h=%d fps=%d prog=%s\r\n", g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].ui_format.width, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].ui_format.height, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].ui_format.fps, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].ui_format.prog ? "TRUE" : "FALSE");
    DBG_DUMP("cur setting w=%d h=%d fps=%d prog=%s\r\n", g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].cur_format.width, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].cur_format.height, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].cur_format.fps, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].cur_format.prog ? "TRUE" : "FALSE");
    DBG_DUMP("det setting w=%d h=%d fps=%d prog=%s, vloss=%d\r\n", g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].det_format.width, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].det_format.height, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].det_format.fps, g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].det_format.prog ? "TRUE" : "FALSE", g_ad_dev->chip[chip_id].vin[ad_map[sen_id].vin_id].vloss);
	DBG_DUMP("----------------------------------------------------------------------\r\n");

    DBG_DUMP("set item : ");
    for (idx = 0; idx < MAX_DBG_SET_ITEM; idx ++) {
        if (ad_dbg_set_item[sen_id][idx] == NULL_DBG_VAL) {
            if (idx == 0) {
                DBG_DUMP("none");
            }
            break;
        }
        DBG_DUMP("0x%x ",ad_dbg_set_item[sen_id][idx]);
    }
    DBG_DUMP("\r\n");
	return 0;
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
static BOOL nvt_ad_proc_drv_info(unsigned char argc, char **pargv)
{
	if (g_ad_dev->op.dbg_info != NULL) {
		 g_ad_dev->op.dbg_info(*pargv);
	}

	return 0;
}
#endif

static AD_SXCMD_BEGIN(AD_UTIL_DECLARE_FUNC(ad), AD_PROC_NAME)
AD_SXCMD_ITEM("i2c_info",		  nvt_ad_proc_i2c_info, 	 "[chip_id]")
AD_SXCMD_ITEM("ad_map",          nvt_ad_proc_ad_mapinfo,    "[sen_id]")
AD_SXCMD_ITEM("basc_info",		  nvt_ad_proc_basic_info, 	 "[chip_id]")
AD_SXCMD_ITEM("speed_info",	  nvt_ad_proc_speed_info, 	 "[sen_id]")
AD_SXCMD_ITEM("mipi_info",		  nvt_ad_proc_mipi_info, 	 "[sen_id]")
AD_SXCMD_ITEM("dvi_info", 		  nvt_ad_proc_dvi_info, 	 "[sen_id]")
AD_SXCMD_ITEM("size_info", 	  nvt_ad_proc_size_info, 	 "[sen_id]")
AD_SXCMD_ITEM("dtsi_info",	  nvt_ad_proc_dtsi_info,	 "[sen_id]")
AD_SXCMD_ITEM("dumpinfo", 	  	  nvt_ad_proc_dumpinfo,		 "[sen_id]")
AD_SXCMD_ITEM("det_fmt",         nvt_ad_proc_set_det_fmt,      "[sen_id] [fmt]")
#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
AD_SXCMD_ITEM("drv",			  nvt_ad_proc_drv_info, 	 "input : [id]")
#endif
AD_SXCMD_END()

static int ad_proc_cmd_showhelp(void)
{
	UINT32 cmd_num = SXCMD_NUM(AD_UTIL_DECLARE_FUNC(ad));
	UINT32 loop = 1;

	DBG_DUMP("---------------------------------------------------------------------\r\n");
	DBG_DUMP("%15s : %s\r\n", "ad command", "input param");
	DBG_DUMP("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		DBG_DUMP("%15s : %s\r\n", AD_UTIL_DECLARE_FUNC(ad)[loop].p_name, AD_UTIL_DECLARE_FUNC(ad)[loop].p_desc);
	}
	return 0;
}

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
//MAINFUNC_ENTRY(ad_cmd, argc, argv)
AD_MAINFUNC_ENTRY(AD_UTIL_DECLARE_FUNC(ad), argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(AD_UTIL_DECLARE_FUNC(ad));
	UINT32 loop, len;
	int    ret;
	unsigned char ucargc = 0;
	char cmd[MAX_CMD_LENGTH] = {'\0'}, i;
	char *p_strcmd, *p_cur, *p_end, *p_tmp;

	//DBG_DUMP("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);

	if (argc == 1 || strncmp(argv[1], "?", 2) == 0) {
		ad_proc_cmd_showhelp();
		return -1;
	}

	if (argc-1 < 2) {
		DBG_ERR("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	p_cur = &cmd[0];
	p_end = &cmd[MAX_CMD_LENGTH - 1];

	for (i = 0; i < ucargc; i++) {

		len = strlen(argv[i + 2]) + 1; // strlen() doesn't include end char '\0'
		if (len > (unsigned int)(p_end - p_cur)) {
			p_tmp = p_cur - 1;
			*p_tmp = 0x0;
			DBG_ERR("cmd too long, buf not enough\r\n");
			return -1;
		}

		memcpy(p_cur, argv[i + 2], len);
		p_cur += len;
		if (i < (ucargc - 1)) {
			p_tmp = p_cur - 1;
			*p_tmp = 0x20; //modify end to space
		}
	}

	p_strcmd = &cmd[0];
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], AD_UTIL_DECLARE_FUNC(ad)[loop].p_name, strlen(argv[1])) == 0) {
			ret = AD_UTIL_DECLARE_FUNC(ad)[loop].p_func(ucargc, &p_strcmd);
			return ret;
		}
	}

	return 0;
}
#else
static int ad_proc_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(AD_UTIL_DECLARE_FUNC(ad));
	UINT32 loop;
	int    ret;
	unsigned char ucargc = 0;

	//DBG_DUMP("%d, %s, %s, %s, %s\r\n", (int)argc, argv[0], argv[1], argv[2], argv[3]);

	if (strncmp(argv[0], "", 2) == 0 || strncmp(argv[0], "?", 2) == 0) {
		ad_proc_cmd_showhelp();
		return -1;
	}

	if (argc < 2) {
		DBG_ERR("input param error\r\n");
		return -1;
	}
	ucargc = argc - 2;
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], AD_UTIL_DECLARE_FUNC(ad)[loop].p_name, strlen(argv[0])) == 0) {
			ret = AD_UTIL_DECLARE_FUNC(ad)[loop].p_func(ucargc, &argv[1]);
			return ret;
		}
	}

	return 0;
}
#endif


extern AD_DEV* AD_UTIL_DECLARE_FUNC(ad_get_obj)(void);

#if defined(__FREERTOS)
int AD_UTIL_DECLARE_FUNC(sen_init_ad)(SENSOR_DTSI_INFO *info)
{
	UINT32 i, j;
	ER rt = E_OK;
	CTL_SEN_REG_OBJ reg_obj;

	if (info && info->addr) {
		rt = AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(info->addr, ad_i2c_ids, ad_info, AD_MAX_CHIP_IDX);
		DBG_ERR("%s\r\n", AD_UTIL_STR(sen_ad));
		dtsi_cfg_path = info->addr;
	}

	if (rt < 0) {
		DBG_ERR("get ad dtsi(%s) fail\r\n", AD_UTIL_STR(sen_ad));
		return rt;
	}

	//get ad dev information
	g_ad_dev = AD_UTIL_DECLARE_FUNC(ad_get_obj)();

	reset_dbg_default_value();

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ad_std_drv;
	reg_obj.det_plug_in = det_plug_in_ad_std_drv;
	reg_obj.drv_tab = &m_p_ad_std_drv_drv;
	rt = ctl_sen_reg_sendrv(AD_UTIL_STR(nvt_sen_ad), &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	for (i = 0; i < AD_MAX_CHIP_IDX; i ++) {
		vos_sem_create(&ad_chip_sem[i], 1, AD_UTIL_STR(ad_semi));
		if (ad_info[i].status & AD_CFG_STS_OK) {
			g_ad_dev->chip[i].i2c_addr = ad_info[i].i2c.slv_addr;
			if (ad_info[i].out_max_num <= AD_OUT_P_MAX) {
				//g_ad_dev->chip[i].vout_max = ad_info[i].out_max_num;
				for (j = 0; j < g_ad_dev->chip[i].vout_max; j++) {
					g_ad_dev->chip[i].vout[j].bus_type = (AD_BUS_TYPE)ad_info[i].out[j].sig.type;// 0: parallel, 1:mipi
					g_ad_dev->chip[i].vout[j].mux_num = ad_info[i].out[j].in_num;
					g_ad_dev->chip[i].vout[j].vout2vin_map = ad_info[i].out[j].in;
				}
				rt = g_ad_dev->op.set_cfg(i, AD_CFGID_I2C_FUNC, (void *)sen_i2c_rw_if); //get mode_idx from driver
			} else {
				DBG_ERR("chip(%d) out num=%d, over than max%d\r\n", i, ad_info[i].out_max_num, AD_OUT_P_MAX);
			}
		} else {
			//DBG_ERR("chip(%d) dtsi parser fail!!\r\n", i);
		}
	}
	//g_ad_dev->op.i2c_rw_drv = sen_i2c_rw_if;

	return rt;
}

void AD_UTIL_DECLARE_FUNC(sen_exit_ad)(void)
{
	UINT32 i;

	for (i = 0; i < AD_MAX_CHIP_IDX; i ++) {
		vos_sem_destroy(ad_chip_sem[i]);
	}
}
#else
static int nvt_ad_proc_std_show(struct seq_file *sfile, void *v)
{
	ad_proc_cmd_showhelp();
	return 0;
}

static int nvt_ad_proc_std_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ad_proc_std_show, NULL);
}

static ssize_t nvt_ad_proc_std(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	} else if (len < 1) {
		nvt_dbg(ERR, "Command length is too short!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);
	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (ad_proc_cmd_execute(ucargc, &argv[0]) == 0) {
		return size;
	}

	return size;

ERR_OUT:
	return -1;
}


static int nvt_ad_proc_drv_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_ad_proc_drv_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ad_proc_drv_show, NULL);
}

static ssize_t nvt_ad_proc_drv(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	} else if (len < 1) {
		nvt_dbg(ERR, "Command length is too short!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	if (g_ad_dev->op.dbg_info != NULL) {
		 g_ad_dev->op.dbg_info(cmdstr);
	}

	// parse command string
	return size;

ERR_OUT:
	return -1;
}


static struct file_operations proc_ad_std_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ad_proc_std_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ad_proc_std
};
static struct file_operations proc_ad_drv_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ad_proc_drv_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ad_proc_drv
};

static struct proc_dir_entry *pproc_module_root;
static struct proc_dir_entry *pproc_ad_std_entry;
static struct proc_dir_entry *pproc_ad_drv_entry;


static int nvt_ad_proc_init(void)
{
    int ret = 0;
    struct proc_dir_entry *pmodule_root = NULL;
    struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir(SEN_AD_STD_MODULE_NAME, NULL);
	if(pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
        ret = -EINVAL;
		goto err_end;
	}
	pproc_module_root = pmodule_root;

    pentry = proc_create("std", S_IRUGO | S_IXUGO, pmodule_root, &proc_ad_std_fops);
    if (pentry == NULL) {
        nvt_dbg(ERR, "failed to create proc cmd!\n");
        ret = -EINVAL;
        goto remove_root;
    }
    pproc_ad_std_entry = pentry;

    pentry = proc_create("drv", S_IRUGO | S_IXUGO, pmodule_root, &proc_ad_drv_fops);
    if (pentry == NULL) {
        nvt_dbg(ERR, "failed to create proc cmd!\n");
        ret = -EINVAL;
        goto remove_cmd;
    }
    pproc_ad_drv_entry = pentry;
	return ret;

remove_cmd:
	proc_remove(pproc_ad_std_entry);
remove_root:
	proc_remove(pproc_module_root);
err_end:
     return ret;
}

static int nvt_ad_proc_remove(void)
{
	proc_remove(pproc_ad_drv_entry);
	proc_remove(pproc_ad_std_entry);
	proc_remove(pproc_module_root);
	return 0;
}

static int sen_ad_std_drv_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	UINT32 chip_id;

	//set i2c data
	chip_id = (unsigned int)i2c_id->driver_data;

	AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(client, &ad_info[chip_id]);

	//keep of_node information for dump debug message
	dev_client_info[chip_id] = client;
    return 0;
}

static struct i2c_driver ad_platform_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = AD_UTIL_STR(sen_ad),
		.of_match_table = of_match_ptr(ad_dt_match),
	},
	.probe = sen_ad_std_drv_probe,
	.id_table = ad_i2c_ids,
};

static int __init sen_init_ad_std_drv(void)
{
	INT ret;
	UINT32 i, j;
	ER rt = E_OK;
	CTL_SEN_REG_OBJ reg_obj;

	ret = i2c_add_driver(&ad_platform_driver);
	if (ret < 0) {
		return DBG_ERR("i2c_add_driver fail (%d)\r\n", ret);
	}
	//rt = AD_UTIL_DECLARE_FUNC(ad_get_dtsi_data)(sen_cfg_path, AD_UTIL_STR(sen_ad), ad_info, AD_MAX_CHIP_IDX);
    reset_dbg_default_value();

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ad_std_drv;
	reg_obj.det_plug_in = det_plug_in_ad_std_drv;
	reg_obj.drv_tab = &m_p_ad_std_drv_drv;
	rt = ctl_sen_reg_sendrv(AD_UTIL_STR(nvt_sen_ad), &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	//get ad dev information
	g_ad_dev = AD_UTIL_DECLARE_FUNC(ad_get_obj)();

	for (i = 0; i < AD_MAX_CHIP_IDX; i ++) {
		vos_sem_create(&ad_chip_sem[i], 1, AD_UTIL_STR(ad_semi));
		if (ad_info[i].status & AD_CFG_STS_OK) {
			g_ad_dev->chip[i].i2c_addr = ad_info[i].i2c.slv_addr;
			if (ad_info[i].out_max_num <= AD_OUT_P_MAX) {
				//g_ad_dev->chip[i].vout_max = ad_info[i].out_max_num;
				for (j = 0; j < g_ad_dev->chip[i].vout_max; j++) {
					g_ad_dev->chip[i].vout[j].bus_type = (AD_BUS_TYPE)ad_info[i].out[j].sig.type;// 0: parallel, 1:mipi
					g_ad_dev->chip[i].vout[j].mux_num = ad_info[i].out[j].in_num;
					g_ad_dev->chip[i].vout[j].vout2vin_map = ad_info[i].out[j].in;
				}
				rt = g_ad_dev->op.set_cfg(i, AD_CFGID_I2C_FUNC, (void *)sen_i2c_rw_if); //get mode_idx from driver
			} else {
				DBG_ERR("chip(%d) out num=%d, over than max%d\r\n", i, ad_info[i].out_max_num, AD_OUT_P_MAX);
			}
		} else {
			//DBG_ERR("chip(%d) dtsi parser fail!!\r\n", i);
		}
	}
	DBG_DUMP("--register sensor driver %s--\r\n", AD_UTIL_STR(sen_ad));
	nvt_ad_proc_init();
	return rt;
}

static void __exit sen_exit_ad_std_drv(void)
{
	UINT32 i;

	for (i = 0; i < AD_MAX_CHIP_IDX; i ++) {
		vos_sem_destroy(ad_chip_sem[i]);
	}
	nvt_ad_proc_remove();

	i2c_del_driver(&ad_platform_driver);
}

module_init(sen_init_ad_std_drv);
module_exit(sen_exit_ad_std_drv);
MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(AD_UTIL_STR(sen_ad));
MODULE_LICENSE("GPL");
#endif
//@@ EOF
