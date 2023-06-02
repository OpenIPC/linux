#include "ad_dummy.h"
#include "nvt_ad_dummy_reg.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 00, 000, 00);

// mapping table: vin raw data -> meaningful vin info
/*static AD_DUMMY_DET_MAP g_ad_dummy_det_map[] = {
};*/

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
/*static AD_DUMMY_VIN_MAP g_ad_dummy_vin_map[] = {
};*/

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_DUMMY_VOUT_MAP g_ad_dummy_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_dummy_slv_addr[] = {0x00, 0x00, 0x00, 0x00};

// decoder dump all register information
#define AD_DUMMY_BANK_ADDR 0x00
static AD_DRV_DBG_REG_BANK g_ad_dummy_bank[] = {
	{.id = 0x0, .len = 0x000},
};
#define AD_DUMMY_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_DUMMY_DFT_WIDTH 1920
#define AD_DUMMY_DFT_HEIGHT 1080
#define AD_DUMMY_DFT_FPS 2500
#define AD_DUMMY_DFT_PROG TRUE

// i2c retry number when operation fail occur
#define AD_DUMMY_I2C_RETRY_NUM 5

// global variable
#define AD_DUMMY_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_dummy_vin[AD_DUMMY_CHIP_MAX][AD_DUMMY_VIN_MAX];
static AD_VOUT g_ad_dummy_vout[AD_DUMMY_CHIP_MAX][AD_DUMMY_VOUT_MAX];
static AD_INFO g_ad_dummy_info[AD_DUMMY_CHIP_MAX] = {
	{
		.name = "DUMMY_0",
		.vin_max = AD_DUMMY_VIN_MAX,
		.vin = &g_ad_dummy_vin[0][0],
		.vout_max = AD_DUMMY_VOUT_MAX,
		.vout = &g_ad_dummy_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "DUMMY_1",
		.vin_max = AD_DUMMY_VIN_MAX,
		.vin = &g_ad_dummy_vin[1][0],
		.vout_max = AD_DUMMY_VOUT_MAX,
		.vout = &g_ad_dummy_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "DUMMY_2",
		.vin_max = AD_DUMMY_VIN_MAX,
		.vin = &g_ad_dummy_vin[2][0],
		.vout_max = AD_DUMMY_VOUT_MAX,
		.vout = &g_ad_dummy_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "DUMMY_3",
		.vin_max = AD_DUMMY_VIN_MAX,
		.vin = &g_ad_dummy_vin[3][0],
		.vout_max = AD_DUMMY_VOUT_MAX,
		.vout = &g_ad_dummy_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_DUMMY_INFO g_ad_dummy_info_private[AD_DUMMY_CHIP_MAX] = {
	[0 ... (AD_DUMMY_CHIP_MAX-1)] = {
		.is_inited = FALSE,
	}
};

static ER ad_dummy_open(UINT32 chip_id, void *ext_data);
static ER ad_dummy_close(UINT32 chip_id, void *ext_data);
static ER ad_dummy_init(UINT32 chip_id, void *ext_data);
static ER ad_dummy_uninit(UINT32 chip_id, void *ext_data);
static ER ad_dummy_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_dummy_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_dummy_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_dummy_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_dummy_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_dummy_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_dummy_dbg(char *str_cmd);

static AD_DEV g_ad_dummy_obj = {
	AD_DUMMY_CHIP_MAX,
	g_ad_dummy_info,

	sizeof(g_ad_dummy_slv_addr) / sizeof(typeof(g_ad_dummy_slv_addr[0])),
	g_ad_dummy_slv_addr,

	{
		.open = ad_dummy_open,
		.close = ad_dummy_close,
		.init = ad_dummy_init,
		.uninit = ad_dummy_uninit,
		.get_cfg = ad_dummy_get_cfg,
		.set_cfg = ad_dummy_set_cfg,
		.chgmode = ad_dummy_chgmode,
		.det_plug_in = ad_dummy_watchdog_cb,
		.i2c_write = ad_dummy_i2c_write,
		.i2c_read = ad_dummy_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_dummy_dbg,
	}
};

// internal function
static AD_INFO *ad_dummy_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_DUMMY_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_DUMMY_CHIP_MAX);
		return NULL;
	}
	return &g_ad_dummy_info[chip_id];
}

static AD_DUMMY_INFO *ad_dummy_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_DUMMY_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_DUMMY_CHIP_MAX);
		return NULL;
	}
	return &g_ad_dummy_info_private[chip_id];
}

static void ad_dummy_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_DUMMY_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_DUMMY_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_DUMMY_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_DUMMY_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_DUMMY_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_DUMMY_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_DUMMY_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_DUMMY_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_DUMMY_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_DUMMY_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_DUMMY_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_DUMMY_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

// implementation of video mode converting
/*static ER ad_dummy_det_mode_to_vin_mode(AD_DUMMY_DET_RAW_MODE raw_mode, AD_DUMMY_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_dummy_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_dummy_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_dummy_det_map[i_mode].raw_mode, sizeof(AD_DUMMY_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_dummy_det_map[i_mode].vin_mode, sizeof(AD_DUMMY_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u\r\n", AD_DUMMY_MODULE_NAME, raw_mode.mode);
	return E_SYS;
}*/

/*static ER ad_dummy_vin_mode_to_ch_mode(AD_DUMMY_VIN_MODE vin_mode, AD_DUMMY_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_dummy_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_dummy_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_dummy_vin_map[i_mode].vin_mode, sizeof(AD_DUMMY_VIN_MODE)) == 0) {
			*ch_mode = g_ad_dummy_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_DUMMY_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}*/

/*static ER ad_dummy_vout_mode_to_port_mode(AD_DUMMY_VOUT_MODE vout_mode, AD_DUMMY_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_dummy_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_dummy_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_dummy_vout_map[i_mode].vout_mode, sizeof(AD_DUMMY_VOUT_MODE)) == 0) {
			*port_mode = g_ad_dummy_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_DUMMY_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_dummy_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_DUMMY_DET_VIDEO_INFO *video_info)
{
	video_info->vloss = FALSE;
	video_info->width = AD_DUMMY_DFT_WIDTH;
	video_info->height = AD_DUMMY_DFT_HEIGHT;
	video_info->fps = AD_DUMMY_DFT_FPS;
	video_info->prog = AD_DUMMY_DFT_PROG;
	return E_OK;
}

#if 0
#endif

static ER ad_dummy_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_dummy_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	ad_dummy_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_DUMMY_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_DUMMY_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_DUMMY_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_DUMMY_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_DUMMY_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_DUMMY_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_dummy_bank) / sizeof(typeof(g_ad_dummy_bank[0]));
	reg_info.bank = g_ad_dummy_bank;
	reg_info.dump_per_row = AD_DUMMY_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_dummy_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_dummy_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_dummy_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_dummy_init(UINT32 _chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_DUMMY_INFO *ad_private_info;

	ad_info = ad_dummy_get_info(_chip_id);
	ad_private_info = ad_dummy_get_private_info(_chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	ad_private_info->is_inited = TRUE;

done:
	g_ad_dummy_info[_chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_dummy_uninit(UINT32 chip_id, void *ext_data)
{
	AD_DUMMY_INFO *ad_private_info;

	ad_private_info = ad_dummy_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	ad_private_info->is_inited = FALSE;
	g_ad_dummy_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_dummy_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_dummy_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_DATA_FORMAT:
			data_fmt = (AD_VOUT_DATA_FMT_INFO *)data;

			data_fmt->fmt = AD_DATA_FMT_422;
			data_fmt->protocal = AD_DATA_PROTCL_CCIR601;
			//AD_DUMP("[ad GET outport fmt = %d protocol= %d\r\n", data_fmt->fmt, data_fmt->protocal);
			break;
		case AD_CFGID_VIRTUAL_CH_ID:
			virtual_ch = (AD_VIRTUAL_CHANNEL_INFO *)data;

			if (unlikely(virtual_ch->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_DUMMY_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			if (unlikely(mode_info->out_id >= ad_info->vout_max)) {
				AD_ERR("[%s] mode_info wrong input\r\n", AD_DUMMY_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			mode_info->bus_type = ad_info->vout[mode_info->out_id].bus_type;
			mode_info->mux_num = ad_info->vout[mode_info->out_id].mux_num;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 148500000;
			mode_info->data_rate = 148500000;
			mode_info->mipi_lane_num = ad_info->vout[mode_info->out_id].data_lane_num;
			mode_info->protocal = (ad_info->vout[mode_info->out_id].bus_type == AD_BUS_TYPE_PARALLEL) ? AD_DATA_PROTCL_CCIR656 : AD_DATA_PROTCL_CCIR601;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY;
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_DUMMY_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_DUMMY_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_dummy_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_dummy_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_DUMMY_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_DUMMY_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_dummy_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id;
	AD_DUMMY_VIN_MODE vin_mode = {0};

	ad_info = ad_dummy_get_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_DUMMY_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_DUMMY_MODULE_NAME, vin_id, AD_DUMMY_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_DUMMY_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_DUMMY_MODULE_NAME, vout_id, AD_DUMMY_VOUT_MAX);
		return E_SYS;
	}


	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	//DBG_DUMP("chip%d vin%d: w = %d, h = %d, fps = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps);

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;

	return E_OK;
}

static ER ad_dummy_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_DUMMY_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_dummy_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_DUMMY_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_dummy_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_dummy_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_dummy_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_DUMMY_MODULE_NAME, i_vin);
		return E_SYS;
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height >> (video_info.prog ? 0 : 1);
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
	//DBG_ERR("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d\r\n", i_vin, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog);

	return E_OK;
}

static ER ad_dummy_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_dummy_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_dummy_dbg(char *str_cmd)
{
	ad_drv_dbg(str_cmd);

	return E_OK;
}

AD_DEV* ad_get_obj_dummy(void)
{
	return &g_ad_dummy_obj;
}

