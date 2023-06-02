#include "ad_tp9930.h"
#include "nvt_ad_tp9930_reg.h"

#include "tp2802.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 00, 003, 00);

// mapping table: vin raw data -> meaningful vin info
static AD_TP9930_DET_MAP g_ad_tp9930_det_map[] = {
	{.raw_mode = {.mode = TP2802_SD, 		.eq = 0}, .vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE, .std = STD_TVI}}, ///< CVBS  PAL    960x576  25(50i)FPS
	{.raw_mode = {.mode = TP2802_SD, 		.eq = 1}, .vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE, .std = STD_TVI}}, ///< CVBS NTSC    960x480  30(60i)FPS
	{.raw_mode = {.mode = TP2802_720P30, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  30FPS
	{.raw_mode = {.mode = TP2802_720P30V2, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD2 720P   1280x720  30FPS
	{.raw_mode = {.mode = TP2802_720P30, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  30FPS
	{.raw_mode = {.mode = TP2802_720P25, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  25FPS
	{.raw_mode = {.mode = TP2802_720P25V2, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD2 720P   1280x720  25FPS
	{.raw_mode = {.mode = TP2802_720P25, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  25FPS
	{.raw_mode = {.mode = TP2802_720P60, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  60FPS
	{.raw_mode = {.mode = TP2802_720P60, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  60FPS
	{.raw_mode = {.mode = TP2802_720P50, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  50FPS
	{.raw_mode = {.mode = TP2802_720P50, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  50FPS
	{.raw_mode = {.mode = TP2802_1080P15, 	.eq = 0}, .vin_mode = {.width = 1920, .height = 1080, .fps = 1500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD 1080P  1920x1080  15FPS
	{.raw_mode = {.mode = TP2802_1080P30, 	.eq = 0}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD 1080P  1920x1080  30FPS
	{.raw_mode = {.mode = TP2802_1080P30, 	.eq = 1}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1080P  1920x1080  30FPS
	{.raw_mode = {.mode = TP2802_1080P25, 	.eq = 0}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD 1080P  1920x1080  25FPS
	{.raw_mode = {.mode = TP2802_1080P25, 	.eq = 1}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1080P  1920x1080  25FPS
	{.raw_mode = {.mode = TP2802_QHD30, 	.eq = 0}, .vin_mode = {.width = 2560, .height = 1440, .fps = 3000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1440P  2560x1440  30FPS
	{.raw_mode = {.mode = TP2802_QHD25, 	.eq = 0}, .vin_mode = {.width = 2560, .height = 1440, .fps = 2500, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1440P  2560x1440  25FPS
};

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_TP9930_VIN_MAP g_ad_tp9930_vin_map[] = {
	{.vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_PAL, 		.std = STD_TVI}},
	{.vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_NTSC, 		.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P25V2, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P25V2, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P30V2, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P30V2, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P50, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P50, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P60, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P60, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 1500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_1080P15, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 1500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_1080P15, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_1080P25, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_1080P25, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_1080P30, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_1080P30, 	.std = STD_HDA}},
	{.vin_mode = {.width = 2560, .height = 1440, .fps = 3000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_QHD30, 	.std = STD_TVI}},
	{.vin_mode = {.width = 2560, .height = 1440, .fps = 2500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_QHD25, 	.std = STD_TVI}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_TP9930_VOUT_MAP g_ad_tp9930_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_tp9930_slv_addr[] = {0x88, 0x8A, 0x8C, 0x8E};

// decoder dump all register information
#define AD_TP9930_BANK_ADDR 0x40
static AD_DRV_DBG_REG_BANK g_ad_tp9930_bank[] = {
	{.id = 0x00, .len = 0x100},
	{.id = 0x01, .len = 0x100},
	{.id = 0x02, .len = 0x100},
	{.id = 0x03, .len = 0x100},
	{.id = 0x40, .len = 0x100},
};
#define AD_TP9930_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_TP9930_DFT_WIDTH 1280
#define AD_TP9930_DFT_HEIGHT 720
#define AD_TP9930_DFT_FPS 2500
#define AD_TP9930_DFT_PROG TRUE
#define AD_TP9930_DFT_STD STD_HDA

// i2c retry number when operation fail occur
#define AD_TP9930_I2C_RETRY_NUM 5

// global variable
#define AD_TP9930_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_tp9930_vin[AD_TP9930_CHIP_MAX][AD_TP9930_VIN_MAX];
static AD_VOUT g_ad_tp9930_vout[AD_TP9930_CHIP_MAX][AD_TP9930_VOUT_MAX];
static AD_INFO g_ad_tp9930_info[AD_TP9930_CHIP_MAX] = {
	{
		.name = "TP9930_0",
		.vin_max = AD_TP9930_VIN_MAX,
		.vin = &g_ad_tp9930_vin[0][0],
		.vout_max = AD_TP9930_VOUT_MAX,
		.vout = &g_ad_tp9930_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9930_1",
		.vin_max = AD_TP9930_VIN_MAX,
		.vin = &g_ad_tp9930_vin[1][0],
		.vout_max = AD_TP9930_VOUT_MAX,
		.vout = &g_ad_tp9930_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9930_2",
		.vin_max = AD_TP9930_VIN_MAX,
		.vin = &g_ad_tp9930_vin[2][0],
		.vout_max = AD_TP9930_VOUT_MAX,
		.vout = &g_ad_tp9930_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9930_3",
		.vin_max = AD_TP9930_VIN_MAX,
		.vin = &g_ad_tp9930_vin[3][0],
		.vout_max = AD_TP9930_VOUT_MAX,
		.vout = &g_ad_tp9930_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_TP9930_INFO g_ad_tp9930_info_private[AD_TP9930_CHIP_MAX] = {
	[0 ... (AD_TP9930_CHIP_MAX-1)] = {
		.is_inited = FALSE,
		.raw_mode = {
			[0 ... (AD_TP9930_VIN_MAX-1)] = {
				.mode = 0,
				.eq = 1,
			},
		},
		.det_vin_mode = {
			[0 ... (AD_TP9930_VIN_MAX-1)] = {0}
		},
		.pat_gen_mode = {
			[0 ... (AD_TP9930_VIN_MAX-1)] = AD_TP9930_PATTERN_GEN_MODE_AUTO
		},
		.wdi = {
			[0 ... (AD_TP9930_VIN_MAX-1)] = {
				.count = 0,
				.mode = INVALID_FORMAT,
				.scan = SCAN_AUTO,
				.gain = {0, 0, 0, 0},
				.std = AD_TP9930_DFT_STD,
				.state = VIDEO_UNPLUG,
				.force = 0,
			}
		},
		.det_type = {
			[0 ... (AD_TP9930_VIN_MAX-1)] = AD_TYPE_AUTO
		},
	}
};

static ER ad_tp9930_open(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_close(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_init(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_uninit(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tp9930_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tp9930_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_tp9930_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_tp9930_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_tp9930_dbg(char *str_cmd);

static AD_DEV g_ad_tp9930_obj = {
	AD_TP9930_CHIP_MAX,
	g_ad_tp9930_info,

	sizeof(g_ad_tp9930_slv_addr) / sizeof(typeof(g_ad_tp9930_slv_addr[0])),
	g_ad_tp9930_slv_addr,

	{
		.open = ad_tp9930_open,
		.close = ad_tp9930_close,
		.init = ad_tp9930_init,
		.uninit = ad_tp9930_uninit,
		.get_cfg = ad_tp9930_get_cfg,
		.set_cfg = ad_tp9930_set_cfg,
		.chgmode = ad_tp9930_chgmode,
		.det_plug_in = ad_tp9930_watchdog_cb,
		.i2c_write = ad_tp9930_i2c_write,
		.i2c_read = ad_tp9930_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_tp9930_dbg,
	}
};

// internal function
static AD_INFO *ad_tp9930_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TP9930_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TP9930_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tp9930_info[chip_id];
}

static AD_TP9930_INFO *ad_tp9930_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TP9930_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TP9930_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tp9930_info_private[chip_id];
}

static void ad_tp9930_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_TP9930_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_TP9930_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_TP9930_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_TP9930_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_TP9930_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_TP9930_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_TP9930_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_TP9930_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_TP9930_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_TP9930_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_TP9930_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_TP9930_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

void tp28xx_byte_write(UINT32 chip, UINT32 reg_addr, UINT32 value)
{
	ad_tp9930_i2c_write(chip, reg_addr, value, AD_TP9930_I2C_DATA_LEN);
}

UINT32 tp28xx_byte_read(UINT32 chip, UINT32 reg_addr)
{
	UINT32 value = 0;

	ad_tp9930_i2c_read(chip, reg_addr, &value, AD_TP9930_I2C_DATA_LEN);

	return value;
}

static ER ad_tp9930_set_reg_page(UINT32 chip_id, UINT32 page)
{
	switch (page) {
	case CH_1:
	case CH_2:
	case CH_3:
	case CH_4:
	case CH_ALL:
		ad_tp9930_i2c_write(chip_id, AD_TP9930_PAGE_REG_OFS, page, AD_TP9930_I2C_DATA_LEN);
		break;
	case DATA_PAGE:
		ad_tp9930_i2c_write(chip_id, AD_TP9930_PAGE_REG_OFS, 0x10, AD_TP9930_I2C_DATA_LEN);
		break;
	case AUDIO_PAGE:
		ad_tp9930_i2c_write(chip_id, AD_TP9930_PAGE_REG_OFS, 0x40, AD_TP9930_I2C_DATA_LEN);
		break;
	default:
		AD_IND("Unknown page number 0x%x(%d)\r\n", page, page);
		ad_tp9930_i2c_write(chip_id, AD_TP9930_PAGE_REG_OFS, page, AD_TP9930_I2C_DATA_LEN);
	}

	return E_OK;
}

#if 0
static ER ad_tp9930_set_vdo_mux(UINT32 chip_id, UINT32 ch)
{
	AD_TP9930_AFE_TEST_REG afe_test = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_AFE_TEST_REG_OFS, &afe_test.reg, AD_TP9930_I2C_DATA_LEN);

	afe_test.bit.AIN_SEL = ch;

	ad_tp9930_i2c_write(chip_id, AD_TP9930_AFE_TEST_REG_OFS, afe_test.reg, AD_TP9930_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9930_set_output_nbit(UINT32 chip_id, BOOL is_16b)
{
	AD_TP9930_DECODING_CTRL_REG decoding_ctrl = {0};

	ad_tp9930_set_reg_page(chip_id, AD_TP9930_DEC_PAGE);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_DECODING_CTRL_REG_OFS, &decoding_ctrl.reg, AD_TP9930_I2C_DATA_LEN);

	if (is_16b) {
		decoding_ctrl.bit.MD656 = 0;
		decoding_ctrl.bit.MD1120 = 0;
	} else {
		decoding_ctrl.bit.MD656 = 1;
		decoding_ctrl.bit.MD1120 = 1;
	}

	ad_tp9930_i2c_write(chip_id, AD_TP9930_DECODING_CTRL_REG_OFS, decoding_ctrl.reg, AD_TP9930_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9930_get_device_id(UINT32 chip_id, UINT32 *device_id)
{
	AD_TP9930_DEVICE_ID_15_8_REG id_h = {0};
	AD_TP9930_DEVICE_ID_7_0_REG id_l = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_DEVICE_ID_15_8_REG_OFS, &id_h.reg, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_DEVICE_ID_7_0_REG_OFS, &id_l.reg, AD_TP9930_I2C_DATA_LEN);

	*device_id = (id_h.bit.DEVICE_ID << 8) | id_l.bit.DEVICE_ID;

	return E_OK;
}

static ER ad_tp9930_get_revision_id(UINT32 chip_id, UINT32 *revision_id)
{
	AD_TP9930_REVISION_REG id = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_REVISION_REG_OFS, &id.reg, AD_TP9930_I2C_DATA_LEN);

	*revision_id = id.bit.REVISION;

	return E_OK;
}
#endif

static ER ad_tp9930_get_vloss(UINT32 chip_id, BOOL *vloss)
{
	AD_TP9930_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*vloss = (input_sts.bit.VDLOSS != FALSE);

	return E_OK;
}

static ER ad_tp9930_get_hlock(UINT32 chip_id, BOOL *hlock)
{
	AD_TP9930_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*hlock = (input_sts.bit.HLOCK != FALSE);

	return E_OK;
}

static ER ad_tp9930_get_vlock(UINT32 chip_id, BOOL *vlock)
{
	AD_TP9930_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*vlock = (input_sts.bit.VLOCK != FALSE);

	return E_OK;
}

static ER ad_tp9930_get_carrier(UINT32 chip_id, BOOL *carrier)
{
	AD_TP9930_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*carrier = (input_sts.bit.CDET == FALSE);

	return E_OK;
}

static ER ad_tp9930_get_eq_detect(UINT32 chip_id, UINT32 *eq_detect)
{
	AD_TP9930_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*eq_detect = input_sts.bit.EQDET;

	return E_OK;
}

static ER ad_tp9930_get_cvstd(UINT32 chip_id, UINT32 *cvstd)
{
	AD_TP9930_DETECTION_STATUS_REG det_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*cvstd = det_sts.bit.CVSTD;

	return E_OK;
}

static ER ad_tp9930_get_pattern_gen_mode(UINT32 chip_id, UINT32 vin_id, AD_TP9930_PATTERN_GEN_MODE *mode)
{
	AD_TP9930_COLOR_HPLL_FREERUN_CTRL_REG ctrl_reg = {0};

	ad_tp9930_set_reg_page(chip_id, vin_id);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_COLOR_HPLL_FREERUN_CTRL_REG_OFS, &ctrl_reg.reg, AD_TP9930_I2C_DATA_LEN);

	if (ctrl_reg.bit.LCS == 1 && ctrl_reg.bit.FCS == 1) {
		*mode = AD_TP9930_PATTERN_GEN_MODE_FORCE;
	} else if (ctrl_reg.bit.LCS == 1 && ctrl_reg.bit.FCS == 0) {
		*mode = AD_TP9930_PATTERN_GEN_MODE_AUTO;
	} else {
		*mode = AD_TP9930_PATTERN_GEN_MODE_DISABLE;
	}

	return E_OK;
}

static ER ad_tp9930_set_pattern_gen_mode(UINT32 chip_id, UINT32 vin_id, AD_TP9930_PATTERN_GEN_MODE mode)
{
	AD_TP9930_COLOR_HPLL_FREERUN_CTRL_REG ctrl_reg = {0};

	ad_tp9930_set_reg_page(chip_id, vin_id);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_COLOR_HPLL_FREERUN_CTRL_REG_OFS, &ctrl_reg.reg, AD_TP9930_I2C_DATA_LEN);

	switch (mode) {
	case AD_TP9930_PATTERN_GEN_MODE_DISABLE:
		ctrl_reg.bit.LCS = 0;
		ctrl_reg.bit.FCS = 0;
		break;
	case AD_TP9930_PATTERN_GEN_MODE_AUTO:
	default:
		ctrl_reg.bit.LCS = 1;
		ctrl_reg.bit.FCS = 0;
		break;
	case AD_TP9930_PATTERN_GEN_MODE_FORCE:
		ctrl_reg.bit.LCS = 1;
		ctrl_reg.bit.FCS = 1;
		break;
	}

	ad_tp9930_i2c_write(chip_id, AD_TP9930_COLOR_HPLL_FREERUN_CTRL_REG_OFS, ctrl_reg.reg, AD_TP9930_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9930_get_sywd(UINT32 chip_id, UINT32 *sywd)
{
	AD_TP9930_DETECTION_STATUS_REG det_sts = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_TP9930_I2C_DATA_LEN);

	*sywd = det_sts.bit.SYWD;

	return E_OK;
}

static ER ad_tp9930_set_clamp_ctrl(UINT32 chip_id, UINT32 mode)
{
	AD_TP9930_CLAMP_CTRL_REG clamp_ctrl = {0};

	ad_tp9930_i2c_read(chip_id, AD_TP9930_CLAMP_CTRL_REG_OFS, &clamp_ctrl.reg, AD_TP9930_I2C_DATA_LEN);

	clamp_ctrl.bit.CLMD = (mode & 0x1);

	ad_tp9930_i2c_write(chip_id, AD_TP9930_CLAMP_CTRL_REG_OFS, clamp_ctrl.reg, AD_TP9930_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9930_read_egain(UINT32 chip_id, UINT32 *egain)
{
	ad_tp9930_i2c_write(chip_id, AD_TP9930_TEST_REG_OFS, 0x0, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_INTERNAL_STATUS_REG_OFS, egain, AD_TP9930_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9930_egain(UINT32 chip_id, UINT32 CGAIN_STD)
{
	UINT32 tmp = 0, cgain = 0, retry = 30;

	ad_tp9930_i2c_write(chip_id, AD_TP9930_TEST_REG_OFS, 0x06, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9930_I2C_DATA_LEN);

	if (cgain < CGAIN_STD) {

		while (retry) {
			retry--;

			ad_tp9930_i2c_read(chip_id, AD_TP9930_EQ2_CTRL_REG_OFS, &tmp, AD_TP9930_I2C_DATA_LEN);
			tmp &= 0x3f;
			while (max(CGAIN_STD, cgain) - min(CGAIN_STD, cgain)) {
				if (tmp) tmp--;
				else break;
				cgain++;
			}

			ad_tp9930_i2c_write(chip_id, AD_TP9930_EQ2_CTRL_REG_OFS, 0x80|tmp, AD_TP9930_I2C_DATA_LEN);
			if (0 == tmp) break;
			vos_util_delay_ms(40);
			ad_tp9930_i2c_write(chip_id, AD_TP9930_TEST_REG_OFS, 0x06, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_read(chip_id, AD_TP9930_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9930_I2C_DATA_LEN);

			if (cgain > (CGAIN_STD+1)) {
				ad_tp9930_i2c_read(chip_id, AD_TP9930_EQ2_CTRL_REG_OFS, &tmp, AD_TP9930_I2C_DATA_LEN);
				tmp &= 0x3f;
				tmp += 0x02;
				if (tmp > 0x3f) tmp = 0x3f;
				ad_tp9930_i2c_write(chip_id, AD_TP9930_EQ2_CTRL_REG_OFS, 0x80|tmp, AD_TP9930_I2C_DATA_LEN);
				if (0x3f == tmp) break;
				vos_util_delay_ms(40);
				ad_tp9930_i2c_read(chip_id, AD_TP9930_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9930_I2C_DATA_LEN);
			}
			if ((max(cgain, CGAIN_STD) - min(cgain, CGAIN_STD)) < 2) break;
		}
	}

	return E_OK;
}

typedef struct {
	UINT8 addr;
	UINT8 val;
} AD_TP9930_I2C_REG;

static ER ad_tp9930_i2c_write_tbl(UINT32 chip_id, const AD_TP9930_I2C_REG *tbl, UINT32 cnt)
{
	UINT32 i;

	for (i = 0; i < cnt; i++) {

		if (ad_tp9930_i2c_write(chip_id, tbl[i].addr, tbl[i].val, AD_TP9930_I2C_DATA_LEN) != E_OK) {
			return E_SYS;
		}
	}

	return E_OK;
}

const AD_TP9930_I2C_REG ad_tp9930_reset[] = {
	{0x3b, 0x20},
	{0x3d, 0xe0},
	{0x3d, 0x60},
	{0x3b, 0x25},
	{0x40, 0x40},
	{0x7a, 0x20},
	{0x3c, 0x20},
	{0x3c, 0x00},
	{0x7a, 0x25},
};

static void ad_tp9930_reset_default(UINT32 chip_id, UINT32 ch)
{
	ad_tp9930_set_reg_page(chip_id, ch);
	ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_reset, sizeof(ad_tp9930_reset)/sizeof(typeof(ad_tp9930_reset[0])));
	ad_tp9930_set_reg_page(chip_id, ch);
	ad_tp9930_i2c_write(chip_id, 0x44, 0x17, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x43, 0x12, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x45, 0x09, AD_TP9930_I2C_DATA_LEN);
}

const AD_TP9930_I2C_REG ad_tp9930_video_mode_pal[] = {
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x51},

	{0x20, 0x48},
	{0x21, 0x84},
	{0x22, 0x37},
	{0x23, 0x3f},

	{0x25, 0xff},
	{0x26, 0x05},
	{0x27, 0x2d},
	{0x28, 0x00},

	{0x2b, 0x70},
	{0x2c, 0x2a},
	{0x2d, 0x64},
	{0x2e, 0x56},

	{0x30, 0x7a},
	{0x31, 0x4a},
	{0x32, 0x4d},
	{0x33, 0xf0},
	{0x35, 0x25},
	{0x38, 0x00},
	{0x39, 0x04},
	{0x3a, 0x32},
	{0x3B, 0x25},

	{0x18, 0x17},

	{0x13, 0x00},
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_ntsc[] = {
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x50},

	{0x20, 0x40},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},

	{0x25, 0xff},
	{0x26, 0x05},
	{0x27, 0x2d},
	{0x28, 0x00},

	{0x2b, 0x70},
	{0x2c, 0x2a},
	{0x2d, 0x68},
	{0x2e, 0x57},

	{0x30, 0x62},
	{0x31, 0xbb},
	{0x32, 0x96},
	{0x33, 0xc0},
	{0x35, 0x25},
	{0x38, 0x00},
	{0x39, 0x04},
	{0x3a, 0x32},
	{0x3B, 0x25},

	{0x18, 0x12},

	{0x13, 0x00},
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_720p25_tvi[] = {
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_720p25_ahd[] = {
	{0x02, 0xce},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x71},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x16},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x07},
	{0x1d, 0xbc},
	{0x20, 0x40},
	{0x21, 0x46},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x2c, 0x3a},
	{0x2d, 0x5a},
	{0x2e, 0x40},
	{0x30, 0x9e},
	{0x31, 0x20},
	{0x32, 0x10},
	{0x33, 0x90},
	{0x35, 0x25},
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_720p30_tvi[] = {
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_720p30_ahd[] = {
	{0x02, 0xce},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x70},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x13},
	{0x16, 0x16},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x06},
	{0x1d, 0x72},
	{0x20, 0x40},
	{0x21, 0x46},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x2c, 0x3a},
	{0x2d, 0x5a},
	{0x2e, 0x40},
	{0x30, 0x9d},
	{0x31, 0xca},
	{0x32, 0x01},
	{0x33, 0xd0},
	{0x35, 0x25},
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_1080p25_tvi[] = {
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_1080p25_ahd[] = {
	{0x02, 0xcc},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x73},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x03},
	{0x16, 0xcc},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x0a},
	{0x1d, 0x50},
	{0x20, 0x3c},
	{0x21, 0x46},
	{0x25, 0xfe},
	{0x26, 0x0d},
	{0x2c, 0x3a},
	{0x2d, 0x54},
	{0x2e, 0x40},
	{0x30, 0xa5},
	{0x31, 0x86},
	{0x32, 0xfb},
	{0x33, 0x60},
	{0x35, 0x05},
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_1080p30_tvi[] = {
};

const AD_TP9930_I2C_REG ad_tp9930_video_mode_1080p30_ahd[] = {
	{0x02, 0xcc},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x72},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x15, 0x03},
	{0x16, 0xcc},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x08},
	{0x1d, 0x98},
	{0x20, 0x38},
	{0x21, 0x46},
	{0x25, 0xfe},
	{0x26, 0x0d},
	{0x2c, 0x3a},
	{0x2d, 0x54},
	{0x2e, 0x40},
	{0x30, 0xa5},
	{0x31, 0x95},
	{0x32, 0xe0},
	{0x33, 0x60},
	{0x35, 0x05},
};

static ER ad_tp9930_set_video_mode(UINT32 chip_id, UINT32 ch, UINT32 fmt, UINT32 std)
{
	const unsigned char SYS_MODE[5] = {0x01, 0x02, 0x04, 0x08, 0x0f};
	AD_TP9930_SYSTEM_CLOCK_CTRL_REG sys_clk_reg = {0};
	AD_TP9930_CHID_CTRL_REG chid_ctrl_reg = {0};

	AD_IND("SET MODE chip%u ch%u fmt%u(0x%x) std%u\r\n", chip_id, ch, fmt, fmt, std);

	ad_tp9930_set_reg_page(chip_id, ch);
	ad_tp9930_i2c_read(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, &sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
	sys_clk_reg.bit.VADCK1POL = 1;
	sys_clk_reg.bit.VADCK2POL = 1;
	sys_clk_reg.bit.VADCK3POL = 1;
	sys_clk_reg.bit.VADCK4POL = 1;

	// set bt656 sync code channel id
	chid_ctrl_reg.bit.CHID = ch;
	chid_ctrl_reg.bit.CIDEN = 1;
	ad_tp9930_i2c_write(chip_id, AD_TP9930_CHID_CTRL_REG_OFS, chid_ctrl_reg.reg, AD_TP9930_I2C_DATA_LEN);

	// set video mode
	switch (fmt) {
	case TP2802_PAL:
		sys_clk_reg.reg |= SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_pal, sizeof(ad_tp9930_video_mode_pal)/sizeof(typeof(ad_tp9930_video_mode_pal[0])));
		break;
	case TP2802_NTSC:
		sys_clk_reg.reg |= SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_ntsc, sizeof(ad_tp9930_video_mode_ntsc)/sizeof(typeof(ad_tp9930_video_mode_ntsc[0])));
		break;
	case TP2802_720P25:
	case TP2802_720P25V2:
		sys_clk_reg.reg |= SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		//if (std == STD_HDA) {
		if (1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_720p25_ahd, sizeof(ad_tp9930_video_mode_720p25_ahd)/sizeof(typeof(ad_tp9930_video_mode_720p25_ahd[0])));
		} else {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_720p25_tvi, sizeof(ad_tp9930_video_mode_720p25_tvi)/sizeof(typeof(ad_tp9930_video_mode_720p25_tvi[0])));
		}
		break;
	case TP2802_720P30:
	case TP2802_720P30V2:
		sys_clk_reg.reg |= SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		//if (std == STD_HDA) {
		if (1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_720p30_ahd, sizeof(ad_tp9930_video_mode_720p30_ahd)/sizeof(typeof(ad_tp9930_video_mode_720p30_ahd[0])));
		} else {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_720p30_tvi, sizeof(ad_tp9930_video_mode_720p30_tvi)/sizeof(typeof(ad_tp9930_video_mode_720p30_tvi[0])));
		}
		break;
	case TP2802_1080P15:
		sys_clk_reg.reg |= SYS_MODE[ch]; // 1080p30 setting with 74.25M clock
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		//if (std == STD_HDA) {
		if (1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p30_ahd, sizeof(ad_tp9930_video_mode_1080p30_ahd)/sizeof(typeof(ad_tp9930_video_mode_1080p30_ahd[0])));
		} else {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p30_tvi, sizeof(ad_tp9930_video_mode_1080p30_tvi)/sizeof(typeof(ad_tp9930_video_mode_1080p30_tvi[0])));
		}
		break;
	case TP2802_1080P25:
		sys_clk_reg.reg &= ~SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		//if (std == STD_HDA) {
		if (1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p25_ahd, sizeof(ad_tp9930_video_mode_1080p25_ahd)/sizeof(typeof(ad_tp9930_video_mode_1080p25_ahd[0])));
		} else {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p25_tvi, sizeof(ad_tp9930_video_mode_1080p25_tvi)/sizeof(typeof(ad_tp9930_video_mode_1080p25_tvi[0])));
		}
		break;
	case TP2802_1080P30:
		sys_clk_reg.reg &= ~SYS_MODE[ch];
		ad_tp9930_i2c_write(chip_id, AD_TP9930_SYSTEM_CLOCK_CTRL_REG_OFS, sys_clk_reg.reg, AD_TP9930_I2C_DATA_LEN);
		//if (std == STD_HDA) {
		if (1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p30_ahd, sizeof(ad_tp9930_video_mode_1080p30_ahd)/sizeof(typeof(ad_tp9930_video_mode_1080p30_ahd[0])));
		} else {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_video_mode_1080p30_tvi, sizeof(ad_tp9930_video_mode_1080p30_tvi)/sizeof(typeof(ad_tp9930_video_mode_1080p30_tvi[0])));
		}
		break;
	default:
		AD_ERR("[%s] Unknown video fmt %d\r\n", AD_TP9930_MODULE_NAME, fmt);
		return E_SYS;
	}

	return E_OK;
}

const AD_TP9930_I2C_REG ad_tp9930_output_mode_1ch[] = {
	{0x45, 0x09},
	{0xF4, 0x80},
	{0xF6, 0x00},
	{0xF7, 0x11},
	{0xF8, 0x22},
	{0xF9, 0x33},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0xF3, 0x00},
	{0xF2, 0x00},
};

const AD_TP9930_I2C_REG ad_tp9930_output_mode_2ch[] = {
	{0x45, 0x09},
	{0xF4, 0x80},
	{0xF6, 0x10},
	{0xF7, 0x10},
	{0xF8, 0x23},
	{0xF9, 0x23},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0xF3, 0x00},
	{0xF2, 0x00},
};

static ER ad_tp9930_set_output_mode(UINT32 chip_id, UINT32 ch0, UINT32 ch1, UINT32 port, UINT32 fmt, UINT32 mux)
{
	AD_IND("SET OUTPUT MODE chip%u ch%u,%u port%u fmt%u(0x%x) mux%u\r\n", chip_id, ch0, ch1, port, fmt, fmt, mux);

	ad_tp9930_set_reg_page(chip_id, ch0);

	ad_tp9930_i2c_write(chip_id, 0xF1, 0x14, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x4D, 0x07, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x4E, 0x05, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x4f, 0x03, AD_TP9930_I2C_DATA_LEN);

	// set output mode
	switch (fmt) {
	case TP2802_PAL:
	case TP2802_NTSC:
	case TP2802_720P25V2:
	case TP2802_720P30V2:
		if (mux == AD_MUX_NUM_1) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_output_mode_1ch, sizeof(ad_tp9930_output_mode_1ch)/sizeof(typeof(ad_tp9930_output_mode_1ch[0])));
			if (port == 0) {
				ad_tp9930_i2c_write(chip_id, 0xFA, 0x89, AD_TP9930_I2C_DATA_LEN);
			} else if (port == 1) {
				ad_tp9930_i2c_write(chip_id, 0xFB, 0x89, AD_TP9930_I2C_DATA_LEN);
			}
		} else if (mux == AD_MUX_NUM_2) {
			ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_output_mode_2ch, sizeof(ad_tp9930_output_mode_2ch)/sizeof(typeof(ad_tp9930_output_mode_2ch[0])));
			if (port == 0) {
				ad_tp9930_i2c_write(chip_id, 0xFA, 0x88, AD_TP9930_I2C_DATA_LEN);
			} else if (port == 1) {
				ad_tp9930_i2c_write(chip_id, 0xFB, 0x88, AD_TP9930_I2C_DATA_LEN);
			}
		} else {
			AD_ERR("[%s] Unknown mux %d\r\n", AD_TP9930_MODULE_NAME, mux);
			return E_SYS;
		}
		break;
	case TP2802_1080P15:
	case TP2802_1080P25:
	case TP2802_1080P30:
		ad_tp9930_i2c_write_tbl(chip_id, ad_tp9930_output_mode_1ch, sizeof(ad_tp9930_output_mode_1ch)/sizeof(typeof(ad_tp9930_output_mode_1ch[0])));
		if (port == 0) {
			ad_tp9930_i2c_write(chip_id, 0xFA, 0x88, AD_TP9930_I2C_DATA_LEN);
		} else if (port == 1) {
			ad_tp9930_i2c_write(chip_id, 0xFB, 0x88, AD_TP9930_I2C_DATA_LEN);
		}
		break;
	default:
		AD_ERR("[%s] Unknown video fmt %d\r\n", AD_TP9930_MODULE_NAME, fmt);
		return E_SYS;
	}

	// set data output port
	if (port == 0) {
		if (mux == AD_MUX_NUM_1) {
			ad_tp9930_i2c_write(chip_id, 0xF6, ch0 | (ch0 << 4), AD_TP9930_I2C_DATA_LEN);
		} else if (mux == AD_MUX_NUM_2) {
			ad_tp9930_i2c_write(chip_id, 0xF6, ch0 | (ch1 << 4), AD_TP9930_I2C_DATA_LEN);
		} else {
			AD_ERR("[%s] Unknown mux %d\r\n", AD_TP9930_MODULE_NAME, mux);
			return E_SYS;
		}
	} else if (port == 1) {
		if (mux == AD_MUX_NUM_1) {
			ad_tp9930_i2c_write(chip_id, 0xF8, ch0 | (ch0 << 4), AD_TP9930_I2C_DATA_LEN);
		} else if (mux == AD_MUX_NUM_2) {
			ad_tp9930_i2c_write(chip_id, 0xF8, ch0 | (ch1 << 4), AD_TP9930_I2C_DATA_LEN);
		} else {
			AD_ERR("[%s] Unknown mux %d\r\n", AD_TP9930_MODULE_NAME, mux);
			return E_SYS;
		}
	} else {
		AD_ERR("[%s] Unknown port %d\r\n", AD_TP9930_MODULE_NAME, port);
		return E_SYS;
	}

	return E_OK;
}

static int ad_tp9930_audio_config_rmpos(UINT32 chip, UINT32 format, UINT32 chn_num)
{
	UINT32 i;

	//clear first
	for (i = 0; i < 20; i++) {
		ad_tp9930_i2c_write(chip, i, 0, AD_TP9930_I2C_DATA_LEN);
	}

	switch (chn_num) {
	case 2:
		if (format) {
			ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x1, 2, AD_TP9930_I2C_DATA_LEN);
		} else {
			ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x8, 2, AD_TP9930_I2C_DATA_LEN);
		}
		break;
	case 4:
		if (format) {
			ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x1, 2, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x2, 3, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x3, 4, AD_TP9930_I2C_DATA_LEN);
		} else {
			ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x1, 2, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x8, 3, AD_TP9930_I2C_DATA_LEN);
			ad_tp9930_i2c_write(chip, 0x9, 4, AD_TP9930_I2C_DATA_LEN);
		}
		break;
	case 8:
		if (0 == (chip%4)) {
			if (format) {
				ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x1, 2, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x2, 3, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x3, 4, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x4, 5, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x5, 6, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x6, 7, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x7, 8, AD_TP9930_I2C_DATA_LEN);
			} else {
				ad_tp9930_i2c_write(chip, 0x0, 1, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x1, 2, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x2, 3, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x3, 4, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x8, 5, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x9, 6, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0xa, 7, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0xb, 8, AD_TP9930_I2C_DATA_LEN);
			}
		} else if (1 == (chip%4)) {
			if (format) {
				ad_tp9930_i2c_write(chip, 0x0, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x1, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x2, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x3, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x4, 1, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x5, 2, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x6, 3, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x7, 4, AD_TP9930_I2C_DATA_LEN);
			} else {
				ad_tp9930_i2c_write(chip, 0x0, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x1, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x2, 1, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x3, 2, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x8, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0x9, 0, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0xa, 3, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip, 0xb, 4, AD_TP9930_I2C_DATA_LEN);
			}
		}
		break;
	case 16:
		if (0 == (chip%4)) {
			for (i = 0; i < 16; i++) {
				ad_tp9930_i2c_write(chip, i, (i+1)-0, AD_TP9930_I2C_DATA_LEN);
			}
		} else if (1 == (chip%4)) {
			for (i = 4; i < 16; i++) {
				ad_tp9930_i2c_write(chip, i, (i+1)-4, AD_TP9930_I2C_DATA_LEN);
			}
		} else if (2 == (chip%4)) {
			for (i = 8; i < 16; i++) {
				ad_tp9930_i2c_write(chip, i, (i+1)-8, AD_TP9930_I2C_DATA_LEN);
			}
		} else {
			for (i = 12; i < 16; i++) {
				ad_tp9930_i2c_write(chip, i, (i+1)-12, AD_TP9930_I2C_DATA_LEN);
			}
		}
		break;
	case 20:
		for (i = 0; i < 20; i++) {
			ad_tp9930_i2c_write(chip, i, i+1, AD_TP9930_I2C_DATA_LEN);
		}
		break;
	default:
		for (i = 0; i < 20; i++) {
			ad_tp9930_i2c_write(chip, i, i+1, AD_TP9930_I2C_DATA_LEN);
		}
	}
	vos_util_delay_ms(10);

	return E_OK;
}

// chn_num: 1-20
// format: 0-I2S 1-DSP
// data_bit: 0-16bit 1-8bit
// sample_rate: 0-8K 1-16K
static ER ad_tp9930_set_audio_mode(UINT32 chip_id, UINT32 chn_num, UINT32 format, UINT32 data_bit, UINT32 sample_rate)
{
	ad_tp9930_set_reg_page(chip_id, AUDIO_PAGE);

	ad_tp9930_audio_config_rmpos(chip_id, format, chn_num);

	ad_tp9930_i2c_write(chip_id, 0x17, 0x00|(data_bit<<2), AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x1B, 0x01|(data_bit<<6), AD_TP9930_I2C_DATA_LEN);

	if (chn_num == 20) {
		ad_tp9930_i2c_write(chip_id, 0x18, 0x90|0x20|(sample_rate), AD_TP9930_I2C_DATA_LEN); // AMD mode 1 for 20ch. add bit5 for I2C BCLK waveform
	} else {
		ad_tp9930_i2c_write(chip_id, 0x18, 0x80|0x20|(sample_rate), AD_TP9930_I2C_DATA_LEN); // AMD mode 0 for 16ch. add bit5 for I2C BCLK waveform
	}

	if (chn_num >= 8) {
		ad_tp9930_i2c_write(chip_id, 0x19, 0x1F, AD_TP9930_I2C_DATA_LEN);
	} else {
		ad_tp9930_i2c_write(chip_id, 0x19, 0x0F, AD_TP9930_I2C_DATA_LEN);
	}

	ad_tp9930_i2c_write(chip_id, 0x1A, 0x15, AD_TP9930_I2C_DATA_LEN);

	ad_tp9930_i2c_write(chip_id, 0x37, 0x20, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x38, 0x38, AD_TP9930_I2C_DATA_LEN);
	ad_tp9930_i2c_write(chip_id, 0x3E, 0x00, AD_TP9930_I2C_DATA_LEN);

	ad_tp9930_i2c_write(chip_id, 0x3d, 0x01, AD_TP9930_I2C_DATA_LEN);//audio reset

	return E_OK;
}

static UINT32 ad_tp9930_get_scan_mode(UINT32 chip_id, UINT32 vin_id)
{
	AD_TP9930_INFO *ad_private_info;
	UINT32 i, cnt;

	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] get scan mode fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return SCAN_AUTO;
	}

	// count total bits of det_type
	i = 0;
	cnt = 0;
	while ((1 << i) < AD_TYPE_MAX) {
		if (ad_private_info->det_type[vin_id] & (1 << i)) {
			cnt++;
		}
		i++;
	}

	if (cnt != 1){ // only support detect one type. otherwise, use auto mode to detect all type
		return SCAN_AUTO;
	} else if (ad_private_info->det_type[vin_id] & AD_TYPE_AHD) {
		return STD_HDA;
	} else if (ad_private_info->det_type[vin_id] & AD_TYPE_TVI) {
		return STD_TVI;
	} else {
		AD_ERR("[%s vin%u] Unsupport det_type 0x%x\r\n", AD_TP9930_MODULE_NAME, vin_id, ad_private_info->det_type[vin_id]);
	}

	return SCAN_AUTO;
}

// implementation of video mode converting
static ER ad_tp9930_det_mode_to_vin_mode(AD_TP9930_DET_RAW_MODE raw_mode, AD_TP9930_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9930_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9930_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_tp9930_det_map[i_mode].raw_mode, sizeof(AD_TP9930_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_tp9930_det_map[i_mode].vin_mode, sizeof(AD_TP9930_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u, eq %u\r\n", AD_TP9930_MODULE_NAME, raw_mode.mode, raw_mode.eq);
	return E_SYS;
}

static ER ad_tp9930_vin_mode_to_ch_mode(AD_TP9930_VIN_MODE vin_mode, AD_TP9930_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9930_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9930_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_tp9930_vin_map[i_mode].vin_mode, sizeof(AD_TP9930_VIN_MODE)) == 0) {
			*ch_mode = g_ad_tp9930_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_TP9930_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}

/*static ER ad_tp9930_vout_mode_to_port_mode(AD_TP9930_VOUT_MODE vout_mode, AD_TP9930_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9930_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9930_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_tp9930_vout_map[i_mode].vout_mode, sizeof(AD_TP9930_VOUT_MODE)) == 0) {
			*port_mode = g_ad_tp9930_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_TP9930_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_tp9930_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_TP9930_DET_VIDEO_INFO *video_info)
{
	char state_str[][16] = {
		"UNPLUG",
		"VIDEO_IN",
		"VDHD_LOCK",
		"UNLOCK",
		"EQ_LOCK",
	};
	UINT32 scan_std[] = {
		[SCAN_DISABLE] = AD_TP9930_DFT_STD,
		[SCAN_AUTO] = AD_TP9930_DFT_STD,
		[SCAN_TVI] = STD_TVI,
		[SCAN_HDA] = STD_HDA,
		[SCAN_HDC] = STD_HDC,
		[SCAN_MANUAL] = AD_TP9930_DFT_STD,
		[SCAN_TEST] = AD_TP9930_DFT_STD,
	};
	AD_TP9930_INFO *ad_private_info;
	AD_TP9930_DET_RAW_MODE det_raw_mode = {0};
	AD_TP9930_DET_VIN_MODE det_vin_mode = {0};
	AD_TP9930_VIN_MODE vin_mode = {0};
	AD_TP9930_CH_MODE ch_mode = {0};
	AD_TP9930_WATCHDOG_INFO *wdi;
	UINT32 i, eq = 1, cvstd = INVALID_FORMAT, sywd = 0, tmp = 0, gain = 0;
	UINT32 sts_str_num = sizeof(state_str) / sizeof(typeof(state_str[0])), scan_std_num = sizeof(scan_std) / sizeof(typeof(scan_std[0]));
	BOOL vloss = TRUE, hlock = FALSE, vlock = FALSE, carrier = FALSE;

	if (unlikely(chip_id >= AD_TP9930_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_TP9930_MODULE_NAME, chip_id, AD_TP9930_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_TP9930_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_TP9930_MODULE_NAME, vin_id, AD_TP9930_VIN_MAX);
		return E_SYS;
	}

	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det video info fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	wdi = &ad_private_info->wdi[vin_id];
	wdi->scan = ad_tp9930_get_scan_mode(chip_id, vin_id);

	ad_tp9930_set_reg_page(chip_id, vin_id);
	ad_tp9930_get_vloss(chip_id, &vloss);


/******************************************************************************/
/*                                                                            */
/*                    state machine for video checking                        */
/*                                                                            */
/******************************************************************************/
	if (vloss) { // no video

		if (VIDEO_UNPLUG != wdi->state) { // switch to no video
			AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNPLUG]);
			wdi->state = VIDEO_UNPLUG;
			wdi->count = 0;
			if (SCAN_MANUAL != wdi->scan) {
				wdi->mode = INVALID_FORMAT;
			}
		}

		if (0 == wdi->count) { // first time into no video
			// set to default mode
			vin_mode.width = AD_TP9930_DFT_WIDTH;
			vin_mode.height = AD_TP9930_DFT_HEIGHT;
			vin_mode.fps = AD_TP9930_DFT_FPS;
			vin_mode.prog = AD_TP9930_DFT_PROG;
			vin_mode.std = (wdi->scan >= scan_std_num) ? AD_TP9930_DFT_STD : scan_std[wdi->scan];
			if (ad_tp9930_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
				AD_WRN("[%s] ad det wrn. get ch mode fail (VLOSS)\r\n", AD_TP9930_MODULE_NAME);
			}
			ad_tp9930_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

			ad_tp9930_reset_default(chip_id, vin_id);
			wdi->count++;

		} else if (wdi->count < MAX_COUNT) {
			wdi->count++;
		}

	} else { // there is video

		ad_tp9930_get_hlock(chip_id, &hlock);
		ad_tp9930_get_vlock(chip_id, &vlock);

		if (hlock && vlock) { // video locked

			if (VIDEO_LOCKED == wdi->state) { // previous state already locked
				if (wdi->eq_locked) { // first carrier lock
					AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[AD_TP9930_VIDEO_EQ_LOCKED]);
					wdi->state = AD_TP9930_VIDEO_EQ_LOCKED;
					wdi->count = 0;
				} else if (wdi->count < MAX_COUNT) {
					wdi->count++;
				}

			} else if (AD_TP9930_VIDEO_EQ_LOCKED == wdi->state) {
				if (wdi->count < MAX_COUNT) {
					wdi->count++;
				}

			} else if (VIDEO_UNPLUG == wdi->state) { // previous state is unplug
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else if (INVALID_FORMAT != wdi->mode) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_LOCKED]);
				wdi->state = VIDEO_LOCKED;
				wdi->eq_locked = 0;
				wdi->count = 0;
			}

		} else { // video in but unlocked

			if (VIDEO_UNPLUG == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else if (VIDEO_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else if (AD_TP9930_VIDEO_EQ_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else {
				if (wdi->count < MAX_COUNT) wdi->count++;
				if (VIDEO_UNLOCK == wdi->state && wdi->count > 2) {
					AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
					wdi->state = VIDEO_IN;
					wdi->count = 0;
					if (SCAN_MANUAL != wdi->scan) ad_tp9930_reset_default(chip_id, vin_id);
				}
			}
		}

		if (wdi->force) { // manual reset for V1/2 switching
			wdi->state = VIDEO_UNPLUG;
			wdi->count = 0;
			wdi->mode = INVALID_FORMAT;
			wdi->force = 0;
			ad_tp9930_reset_default(chip_id, vin_id);
			AD_IND("[chip%u ch%u] FORCE RESET\r\n", chip_id, vin_id);
		}
	}

	AD_IND("[chip%u ch%u STS = %s (count %u)]\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], wdi->count);

/******************************************************************************/
/*                                                                            */
/*               video in but vd/hd unlock, check video mode                  */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_IN == wdi->state) { // current state is video in

		if (SCAN_MANUAL != wdi->scan) {
			ad_tp9930_get_cvstd(chip_id, &cvstd);
			ad_tp9930_get_sywd(chip_id, &sywd);
			ad_tp9930_get_eq_detect(chip_id, &eq);

			AD_IND("[VIDEO IN DETECTION] cvstd %x, sywd %x, eq %x\r\n", cvstd, sywd, eq);

			det_raw_mode.mode = cvstd;
			if (cvstd == TP2802_SD) { // using eq to determine NTSC or PAL
				if ((wdi->count/2) % 2) { // TP2802_PAL
					det_raw_mode.eq = 0;
				} else { // TP2802_NTSC
					det_raw_mode.eq = 1;
				}
			} else {
				det_raw_mode.eq = eq;
			}

			ad_private_info->raw_mode[vin_id] = det_raw_mode;

			if (ad_tp9930_det_mode_to_vin_mode(det_raw_mode, &det_vin_mode) != E_OK) {
				UINT32 int_sts = 0;

				// extra mode detection
				ad_tp9930_i2c_write(chip_id, AD_TP9930_TEST_REG_OFS, 0x09, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_read(chip_id, AD_TP9930_INTERNAL_STATUS_REG_OFS, &int_sts, AD_TP9930_I2C_DATA_LEN);

				switch (int_sts) {
				case 0x89:
					det_raw_mode.mode = TP2802_1080P15;
					det_raw_mode.eq = 0;
					break;
				default:
					goto do_vloss;
				}

				if (ad_tp9930_det_mode_to_vin_mode(det_raw_mode, &det_vin_mode) != E_OK) {
					goto do_vloss;
				}
			}

			if (SCAN_AUTO != wdi->scan) {
				det_vin_mode.std = (wdi->scan >= scan_std_num) ? AD_TP9930_DFT_STD : scan_std[wdi->scan];
			}

			ad_private_info->det_vin_mode[vin_id] = det_vin_mode;

			vin_mode.width = det_vin_mode.width;
			vin_mode.height = det_vin_mode.height;
			vin_mode.fps = det_vin_mode.fps;
			vin_mode.prog = det_vin_mode.prog;
			vin_mode.std = det_vin_mode.std;
			if (ad_tp9930_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
				AD_WRN("[%s] ad det wrn. get ch mode fail (VIDEO IN)\r\n", AD_TP9930_MODULE_NAME);
			}
			ad_tp9930_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

			wdi->std = ch_mode.std;
			wdi->mode = ch_mode.mode;
		}
	}

/******************************************************************************/
/*                                                                            */
/*                    vd/hd lock but eq unlock, set egain                     */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_LOCKED == wdi->state) { // check eq stable

		if (0 == wdi->count) {
			ad_tp9930_set_clamp_ctrl(chip_id, 1);

			if (SCAN_AUTO == wdi->scan || SCAN_TVI == wdi->scan) {
#if 0
				if (TP2802_720P30V2 == wdi->mode || TP2802_720P25V2 == wdi->mode) {
					ad_tp9930_get_sywd(chip_id, &sywd);
					AD_IND("CVSTD%02x  ch%02x chip%2x\r\n", sywd, vin_id, chip_id);

					if (sywd == 0) {
						AD_IND("720P V1 Detected ch%02x chip%2x\r\n", vin_id, chip_id);
						wdi->mode &= 0xf7;
						ad_tp9930_set_video_mode(chip_id, vin_id, wdi->mode, STD_TVI); // to speed the switching
					}

				} else if (TP2802_720P30 == wdi->mode || TP2802_720P25 == wdi->mode) {
					ad_tp9930_get_sywd(chip_id, &sywd);
					AD_IND("CVSTD%02x  ch%02x chip%2x\r\n", sywd, vin_id, chip_id);

					if (sywd != 0) {
						AD_IND("720P V2 Detected ch%02x chip%2x\r\n", vin_id, chip_id);
						wdi->mode |= 0x08;
						ad_tp9930_set_video_mode(chip_id, vin_id, wdi->mode, STD_TVI); // to speed the switching
					}
				}
#endif

				// these code need to keep bottom
				// disable rx and reset v/h pos
				ad_tp9930_i2c_read(chip_id, AD_TP9930_RX_CTRL_REG_OFS, &tmp, AD_TP9930_I2C_DATA_LEN);
				tmp &= 0xFE;
				ad_tp9930_i2c_write(chip_id, AD_TP9930_RX_CTRL_REG_OFS, tmp, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip_id, AD_TP9930_READ_V_POS_CTRL_REG_OFS, 0x06, AD_TP9930_I2C_DATA_LEN);
				ad_tp9930_i2c_write(chip_id, AD_TP9930_READ_H_POS_CTRL_REG_OFS, 0x60, AD_TP9930_I2C_DATA_LEN);
			}

			vos_util_delay_ms(100);

			// enable rx
			ad_tp9930_i2c_read(chip_id, AD_TP9930_RX_CTRL_REG_OFS, &tmp, AD_TP9930_I2C_DATA_LEN);
			tmp |= 0x01;
			ad_tp9930_i2c_write(chip_id, AD_TP9930_RX_CTRL_REG_OFS, tmp, AD_TP9930_I2C_DATA_LEN);

			vos_util_delay_ms(300); // pre-delay for eq stable
		}

		for (i = 0; i < AD_TP9930_WATCHDOG_GAIN_NUM; i++) {
			ad_tp9930_read_egain(chip_id, &gain);
			wdi->gain[i] = gain;
			vos_util_delay_ms(30);
		}
		AD_IND("[EQ GAIN] %u %u %u %u\r\n", wdi->gain[0], wdi->gain[1], wdi->gain[2], wdi->gain[3]);

		wdi->eq_locked = 1;
		for (i = 0; i < AD_TP9930_WATCHDOG_GAIN_NUM-1; i++) {
			if ((max(wdi->gain[i], wdi->gain[AD_TP9930_WATCHDOG_GAIN_NUM-1]) - min(wdi->gain[i], wdi->gain[AD_TP9930_WATCHDOG_GAIN_NUM-1])) >= 0x02) {
				wdi->eq_locked = 0;
				break;
			}
		}

		if (wdi->eq_locked) {
			if (STD_TVI != wdi->std) {
				ad_tp9930_i2c_write(chip_id, AD_TP9930_EQ2_CTRL_REG_OFS, 0x80|(wdi->gain[AD_TP9930_WATCHDOG_GAIN_NUM-1]>>2), AD_TP9930_I2C_DATA_LEN); // manual mode

			} else { // TVI
			}
			vos_util_delay_ms(100);

			if (SCAN_AUTO == wdi->scan) {
				// TBD
			}

			if (STD_TVI != wdi->std) {
				ad_tp9930_egain(chip_id, 0x0c);

			} else { // TVI
			}
		}
	}

/******************************************************************************/
/*                                                                            */
/*                          eq lock, check carrier                            */
/*                                                                            */
/******************************************************************************/
	if (AD_TP9930_VIDEO_EQ_LOCKED == wdi->state) { // check carrier lock
		ad_tp9930_get_carrier(chip_id, &carrier);

		if (SCAN_AUTO == wdi->scan) {

			if ((wdi->mode & (~FLAG_HALF_MODE)) < TP2802_3M18) {

				ad_tp9930_get_cvstd(chip_id, &cvstd);
				if (cvstd != (wdi->mode&0x07) && cvstd < TP2802_SD) {
					AD_IND("Current mode %02x not match with detected cvstd %02x. Force reset\r\n", wdi->mode, cvstd);
					wdi->force = 1;
				} else if (!carrier) {
					AD_IND("Egain stable but no carrier. Force reset\r\n");
					wdi->force = 1;
				} else {
					goto do_video_present;
				}
			}

		} else { // non-auto scan
			if (carrier) { // if no carrier caused by user wrong mode, vloss forever
				goto do_video_present;
			} else {
				ad_tp9930_get_eq_detect(chip_id, &eq);
				AD_IND("[ch%u] Egain stable but no carrier. det_type %x, eq %x\r\n", vin_id, ad_private_info->det_type[vin_id], eq);
			}
		}
	}

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_TP9930_DFT_WIDTH;
	video_info->height = AD_TP9930_DFT_HEIGHT;
	video_info->fps = AD_TP9930_DFT_FPS;
	video_info->prog = AD_TP9930_DFT_PROG;
	video_info->std = AD_TP9930_DFT_STD;
	return E_OK;

do_video_present:
	video_info->vloss = FALSE;
	video_info->width = ad_private_info->det_vin_mode[vin_id].width;
	video_info->height = ad_private_info->det_vin_mode[vin_id].height;
	video_info->fps = ad_private_info->det_vin_mode[vin_id].fps;
	video_info->prog = ad_private_info->det_vin_mode[vin_id].prog;
	video_info->std = ad_private_info->det_vin_mode[vin_id].std;
	return E_OK;
}

static ER ad_tp9930_dbg_dump_help(UINT32 chip_id)
{
	AD_DUMP("\r\n=================================== AD %s DUMP HELP BEGIN (CHIP %u) ===================================\r\n", AD_TP9930_MODULE_NAME, chip_id);

	AD_DUMP("CMD: dumpinfo\r\n");
	AD_DUMP("DESC: Show decoder information.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: det\r\n");
	AD_DUMP("DESC: Trigger detection process once.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: get\r\n");
	AD_DUMP("DESC: Get configuration.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: set\r\n");
	AD_DUMP("DESC: Set configuration.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("\r\n=================================== AD %s DUMP HELP END ===================================\r\n", AD_TP9930_MODULE_NAME);

	return E_OK;
}

static ER ad_tp9930_dbg_dump_info(UINT32 chip_id)
{
#define GET_STR(strary, idx) ((UINT32)(idx) < sizeof(strary)/sizeof(typeof(strary[0])) ? strary[idx] : "Unknown")
	char std_str[][4] = {
		"TVI",
		"AHD",
	};
	char det_type_str[][4] = {
		"AHD",
		"TVI",
		"CVI",
		"SDI",
	};
	AD_TP9930_INFO *ad_private_info;
	UINT32 i, i_vin;

	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad dump info fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO BEGIN (CHIP %u) ===================================\r\n", AD_TP9930_MODULE_NAME, chip_id);

	AD_DUMP("%3s | %8s %4s %8s   %s\r\n",
		"vin", "raw_mode", "eq", "det_std", "det_type");
	AD_DUMP("==================================================================================================\r\n");
	for (i_vin = 0; i_vin < AD_TP9930_VIN_MAX; i_vin++) {

		AD_DUMP("%3u | %8x %4u %8s   ",
			i_vin, ad_private_info->raw_mode[i_vin].mode, ad_private_info->raw_mode[i_vin].eq, GET_STR(std_str, ad_private_info->det_vin_mode[i_vin].std));

		if (ad_private_info->det_type[i_vin] == 0) {
			AD_DUMP("AUTO");
		} else {
			for (i = 0; i < sizeof(det_type_str) / sizeof(typeof(det_type_str[0])); i++) {
				if (ad_private_info->det_type[i_vin] & (1 << i)) {
					AD_DUMP("%s ", det_type_str[i]);
				}
			}
		}
		AD_DUMP("\r\n");
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO END ===================================\r\n", AD_TP9930_MODULE_NAME);

	AD_DUMP("\r\n");

	return E_OK;
}

#if 0
#endif

static ER ad_tp9930_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_tp9930_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	ad_tp9930_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_TP9930_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_TP9930_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_TP9930_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_TP9930_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_TP9930_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_TP9930_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_tp9930_bank) / sizeof(typeof(g_ad_tp9930_bank[0]));
	reg_info.bank = g_ad_tp9930_bank;
	reg_info.dump_per_row = AD_TP9930_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_tp9930_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_tp9930_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_tp9930_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_tp9930_init(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9930_INFO *ad_private_info;
	AD_TP9930_VIN_MODE vin_mode = {0};
	AD_TP9930_CH_MODE ch_mode = {0};
	UINT32 i_vout;

	ad_info = ad_tp9930_get_info(chip_id);
	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	// set video mode
	vin_mode.width = AD_TP9930_DFT_WIDTH;
	vin_mode.height = AD_TP9930_DFT_HEIGHT;
	vin_mode.fps = AD_TP9930_DFT_FPS;
	vin_mode.prog = AD_TP9930_DFT_PROG;
	vin_mode.std = AD_TP9930_DFT_STD;
	if (ad_tp9930_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
		AD_WRN("[%s] ad init wrn. get ch mode fail\r\n", AD_TP9930_MODULE_NAME);
	}

	ad_tp9930_reset_default(chip_id, CH_ALL);
	ad_tp9930_set_video_mode(chip_id, CH_ALL, ch_mode.mode, ch_mode.std);
	ad_tp9930_set_pattern_gen_mode(chip_id, CH_ALL, AD_TP9930_PATTERN_GEN_MODE_AUTO);
	for (i_vout = 0; i_vout < AD_TP9930_VOUT_MAX; i_vout++) {
		ad_tp9930_set_output_mode(chip_id, CH_1, 0, i_vout, ch_mode.mode, AD_MUX_NUM_1);
	}
	ad_tp9930_set_audio_mode(chip_id, 4, 0, 0, 1); // 4ch, I2S, 16bit, 16khz

	ad_private_info->is_inited = TRUE;

done:
	g_ad_tp9930_info[chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_tp9930_uninit(UINT32 chip_id, void *ext_data)
{
	AD_TP9930_INFO *ad_private_info;

	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	ad_private_info->is_inited = FALSE;
	g_ad_tp9930_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_tp9930_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_tp9930_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_DATA_FORMAT:
			data_fmt = (AD_VOUT_DATA_FMT_INFO *)data;

			data_fmt->fmt = AD_DATA_FMT_422;
			data_fmt->protocal = AD_DATA_PROTCL_CCIR601;
			AD_IND("[ad GET outport fmt = %d protocol= %d\r\n", data_fmt->fmt, data_fmt->protocal);
			break;
		case AD_CFGID_VIRTUAL_CH_ID:
			virtual_ch = (AD_VIRTUAL_CHANNEL_INFO *)data;

			if (unlikely(virtual_ch->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_TP9930_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			mode_info->bus_type = ad_info->vout[mode_info->out_id].bus_type;
			mode_info->mux_num = ad_info->vout[mode_info->out_id].mux_num;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 148000000;
			mode_info->data_rate = 148000000;
			mode_info->protocal = AD_DATA_PROTCL_CCIR656;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY; // tmp
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TP9930_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_TP9930_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tp9930_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_TP9930_INFO *ad_private_info;
	AD_INFO_CFG *cfg_info;
	ER rt = E_OK;

	ad_info = ad_tp9930_get_info(chip_id);
	ad_private_info = ad_tp9930_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		case AD_CFGID_DET_CAM_TYPE:
			cfg_info = (AD_INFO_CFG *)data;

			if (unlikely(cfg_info->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] cfg_info wrong input\r\n", AD_TP9930_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			ad_private_info->det_type[cfg_info->vin_id] = cfg_info->data;
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TP9930_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_TP9930_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tp9930_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9930_INFO *ad_private_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id, ch[AD_MUX_NUM_2] = {0}, i, j;
	AD_TP9930_VIN_MODE vin_mode = {0};
	AD_TP9930_CH_MODE ch_mode = {0};

	ad_info = ad_tp9930_get_info(chip_id);
	ad_private_info = ad_tp9930_get_private_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_TP9930_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_TP9930_MODULE_NAME, vin_id, AD_TP9930_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_TP9930_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_TP9930_MODULE_NAME, vout_id, AD_TP9930_VOUT_MAX);
		return E_SYS;
	}

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	vin_mode.std = ad_private_info->det_vin_mode[vin_id].std;
	AD_IND("chip%d vin%d: w = %d, h = %d, fps = %d, std = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.std);

	if (unlikely(ad_tp9930_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	// find every vin id for multi-mux
	j = 0;
	if (ad_info->vout[vout_id].mux_num == AD_MUX_NUM_2) {
		for (i = 0; i < AD_TP9930_VIN_MAX; i++) {
			if (ad_info->vout[vout_id].vout2vin_map & (1 << i)) {
				ch[j++] = i;
				if (j >= AD_MUX_NUM_2) {
					break;
				}
			}
		}
	} else {
		ch[0] = vin_id;
	}

	ad_tp9930_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);
	ad_tp9930_set_output_mode(chip_id, ch[0], ch[1], vout_id, ch_mode.mode, ad_info->vout[vout_id].mux_num);

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;
	ad_private_info->det_vin_mode[vin_id].std = vin_mode.std;

	return E_OK;
}

static ER ad_tp9930_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9930_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_tp9930_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_TP9930_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_tp9930_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_tp9930_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_tp9930_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_TP9930_MODULE_NAME, i_vin);
		return E_SYS;
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height >> (video_info.prog ? 0 : 1);
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
	AD_IND("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d std=%d\r\n", i_vin, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog, video_info.std);

	return E_OK;
}

static ER ad_tp9930_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_tp9930_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_tp9930_dbg(char *str_cmd)
{
#define AD_TP9930_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_TP9930_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_TP9930_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_TP9930_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_TP9930_DBG_CMD_MAX] = {0};
	UINT32 val = 0, /*val2 = 0,*/ i_chip = 0, i_vin;
	AD_TP9930_DET_VIDEO_INFO det_info = {0};
	AD_TP9930_VIN_MODE vin_mode = {0};
	AD_TP9930_CH_MODE ch_mode = {0};

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_TP9930_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_TP9930_DBG_CMD_MAX);

	if (cmd_num == 0 || strcmp(cmd_list[0], "") == 0 || strcmp(cmd_list[0], "help") == 0 || strcmp(cmd_list[0], "?") == 0) {
		ad_tp9930_dbg_dump_help(i_chip);

	} else if (strcmp(cmd_list[0], "dumpinfo") == 0) {
		ad_tp9930_dbg_dump_info(i_chip);

	} else if (strcmp(cmd_list[0], "det") == 0) {
		for (i_vin = 0; i_vin < AD_TP9930_VIN_MAX; i_vin++) {
			ad_tp9930_det_video_info(i_chip, i_vin, &det_info);
			g_ad_tp9930_info[i_chip].vin[i_vin].vloss = det_info.vloss;
			g_ad_tp9930_info[i_chip].vin[i_vin].det_format.width = det_info.width;
			g_ad_tp9930_info[i_chip].vin[i_vin].det_format.height = det_info.height;
			g_ad_tp9930_info[i_chip].vin[i_vin].det_format.fps = det_info.fps;
			g_ad_tp9930_info[i_chip].vin[i_vin].det_format.prog = det_info.prog;
		}

	} else if (strcmp(cmd_list[0], "get") == 0) {

		if (cmd_num == 1) {
#if 0
			AD_DUMP("lp: get lower-power time information.\r\n");
			AD_DUMP("clane: get clock lane enable.\r\n");
			AD_DUMP("dlane: get data lane enable.\r\n");
			AD_DUMP("lanenum: get data lane number.\r\n");
			AD_DUMP("vch: get virtual channel's vin source id.\r\n");
#endif
			AD_DUMP("patgen: get pattern gen mode.\r\n");

#if 0
		} else if (strcmp(cmd_list[1], "lp") == 0) {
			ad_tp9930_get_mipi_t_prep_reg(i_chip, &val);
			ad_tp9930_get_mipi_t_prep_time(i_chip, &val2);
			AD_DUMP("tprep = 0x%x(%u) = %u(ns)\r\n", val, val, val2);
			ad_tp9930_get_mipi_t_trail_reg(i_chip, &val);
			ad_tp9930_get_mipi_t_trail_time(i_chip, &val2);
			AD_DUMP("ttrail = 0x%x(%u) = %u(ns)\r\n", val, val, val2);

		} else if (strcmp(cmd_list[1], "clane") == 0) {
			ad_tp9930_get_mipi_clk_lane_en(i_chip, 0, &val);
			AD_DUMP("clk_lane%u = %s\r\n", 0, val ? "ENABLE" : "DISABLE");

		} else if (strcmp(cmd_list[1], "clane_stop") == 0) {
			ad_tp9930_get_mipi_clk_lane_stop(i_chip, 0, &val);
			AD_DUMP("clk_lane%u = %s\r\n", 0, val ? "STOP" : "NORMAL");

		} else if (strcmp(cmd_list[1], "dlane") == 0) {
			ad_tp9930_get_mipi_data_lane_en(i_chip, 0, &val);
			AD_DUMP("data_lane%u = %s\r\n", 0, val ? "ENABLE" : "DISABLE");
			ad_tp9930_get_mipi_data_lane_en(i_chip, 1, &val);
			AD_DUMP("data_lane%u = %s\r\n", 1, val ? "ENABLE" : "DISABLE");
			ad_tp9930_get_mipi_data_lane_en(i_chip, 2, &val);
			AD_DUMP("data_lane%u = %s\r\n", 2, val ? "ENABLE" : "DISABLE");
			ad_tp9930_get_mipi_data_lane_en(i_chip, 3, &val);
			AD_DUMP("data_lane%u = %s\r\n", 3, val ? "ENABLE" : "DISABLE");

		} else if (strcmp(cmd_list[1], "lanenum") == 0) {
			ad_tp9930_get_mipi_lane_num(i_chip, &val);
			AD_DUMP("data lane num = %u\r\n", val);

		} else if (strcmp(cmd_list[1], "vch") == 0) {
			ad_tp9930_get_virtual_ch_src(i_chip, 0, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 0, val);
			ad_tp9930_get_virtual_ch_src(i_chip, 1, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 1, val);
			ad_tp9930_get_virtual_ch_src(i_chip, 2, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 2, val);
			ad_tp9930_get_virtual_ch_src(i_chip, 3, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 3, val);

#endif
		} else if (strcmp(cmd_list[1], "patgen") == 0) {
			for (i_vin = 0; i_vin < AD_TP9930_VIN_MAX; i_vin++) {
				ad_tp9930_get_pattern_gen_mode(i_chip, i_vin, (AD_TP9930_PATTERN_GEN_MODE *)&val);
				AD_DUMP("vin%u patgen mode = %s\r\n", i_vin,
					val == AD_TP9930_PATTERN_GEN_MODE_FORCE ? "FORCE" :
					val == AD_TP9930_PATTERN_GEN_MODE_AUTO ? "AUTO" :
					"DISABLE");
			}
		}

	} else if (strcmp(cmd_list[0], "set") == 0) {

		if (cmd_num == 1) {
			AD_DUMP("vfmt VIN(u) W(u) H(u) FPS(u) [prog(u)] [std(u)]: set video format of VIN. std: 0-TVI, 1-AHD\r\n");
#if 0
			AD_DUMP("tprep_reg VAL(x): set t_prep register.\r\n");
			AD_DUMP("tprep_time VAL(x): set t_prep time(ns).\r\n");
			AD_DUMP("ttrail_reg VAL(x): set t_trail register.\r\n");
			AD_DUMP("ttrail_time VAL(x): set t_trail time(ns).\r\n");
			AD_DUMP("clane EN(u): set clock lane enable. 0: disable, 1: enable\r\n");
			AD_DUMP("clane_stop STOP(u): set clock lane stop. 0: normal, 1: stop\r\n");
			AD_DUMP("dlane EN(u): set data lane enable. 0: disable, 1: enable\r\n");
			AD_DUMP("lanenum NUM(u): set data lane number. 0~3\r\n");
			AD_DUMP("vch VCH_ID(u) VIN_ID(u): set virtual channel's vin source id. 0~3\r\n");
#endif
			AD_DUMP("patgen MODE(u): set pattern gen mode. 0: disable, 1: auto, 2: force\r\n");

		} else if (cmd_num >= 6 && strcmp(cmd_list[1], "vfmt") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1 && // vin_id
				sscanf_s(cmd_list[3], "%u", &narg[1]) == 1 && // width
				sscanf_s(cmd_list[4], "%u", &narg[2]) == 1 && // height
				sscanf_s(cmd_list[5], "%u", &narg[3]) == 1) { // fps

				if (cmd_num < 7 || sscanf_s(cmd_list[6], "%u", &narg[4]) != 1) {
					narg[4] = TRUE; // prog
				}

				if (cmd_num < 8 || sscanf_s(cmd_list[7], "%u", &narg[5]) != 1) {
					narg[5] = STD_TVI; // std
				}

				vin_mode.width = narg[1];
				vin_mode.height = narg[2];
				vin_mode.fps = narg[3] * 100;
				vin_mode.prog = narg[4];
				vin_mode.std = narg[5];
				if (unlikely(ad_tp9930_vin_mode_to_ch_mode(vin_mode, &ch_mode) == E_OK)) {
					ad_tp9930_set_video_mode(i_chip, narg[0], ch_mode.mode, ch_mode.std);
				} else {
					AD_ERR("set err\r\n");
				}

			} else {
				AD_ERR("set err\r\n");
			}

#if 0
		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "tprep_reg") == 0) {
			if (sscanf_s(cmd_list[2], "%x", &narg[0]) == 1) {
				ad_tp9930_set_mipi_t_prep_reg(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "tprep_time") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_t_prep_time(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "ttrail_reg") == 0) {
			if (sscanf_s(cmd_list[2], "%x", &narg[0]) == 1) {
				ad_tp9930_set_mipi_t_trail_reg(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "ttrail_time") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_t_trail_time(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "clane") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_clk_lane_en(i_chip, 0, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "clane_stop") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_clk_lane_stop(i_chip, 0, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "dlane") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_data_lane_en(i_chip, 0, narg[0]);
				ad_tp9930_set_mipi_data_lane_en(i_chip, 1, narg[0]);
				ad_tp9930_set_mipi_data_lane_en(i_chip, 2, narg[0]);
				ad_tp9930_set_mipi_data_lane_en(i_chip, 3, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "lanenum") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_mipi_lane_num(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 4 && strcmp(cmd_list[1], "vch") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1 && sscanf_s(cmd_list[3], "%u", &narg[1]) == 1) {
				ad_tp9930_set_virtual_ch_src(i_chip, narg[0], narg[1]);
			} else {
				AD_ERR("set err\r\n");
			}

#endif
		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "patgen") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9930_set_pattern_gen_mode(i_chip, CH_ALL, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}
		}
	}

	return E_OK;
}

AD_DEV* ad_get_obj_tp9930(void)
{
	return &g_ad_tp9930_obj;
}

