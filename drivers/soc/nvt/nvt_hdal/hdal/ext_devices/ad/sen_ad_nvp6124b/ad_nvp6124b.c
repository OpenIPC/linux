#include "ad_nvp6124b.h"
#include "nvt_ad_nvp6124b_reg.h"

#include "nvp6124_drv.c"

// AD driver version
AD_DRV_MODULE_VERSION(0, 01, 007, 00);

// mapping table: vin raw data -> meaningful vin info
static AD_NVP6124B_DET_MAP g_ad_nvp6124b_det_map[] = {
	{.raw_mode = {.mode = 0x01}, .vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE}}, ///< SD    NTSC      960x480  30FPS
	{.raw_mode = {.mode = 0x02}, .vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE}}, ///< SD     PAL      960x576  25FPS
	{.raw_mode = {.mode = 0x04}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}}, ///< AHD   720P     1280x720  30FPS
	{.raw_mode = {.mode = 0x08}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}}, ///< AHD   720P     1280x720  25FPS
	{.raw_mode = {.mode = 0x10}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE}}, ///< AHD   720P     1280x720  60FPS
	{.raw_mode = {.mode = 0x11}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}}, ///< EXC   720P     1280x720  30FPS
	{.raw_mode = {.mode = 0x12}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}}, ///< EXC   720P     1280x720  25FPS
	{.raw_mode = {.mode = 0x20}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE}}, ///< AHD   720P     1280x720  50FPS
	{.raw_mode = {.mode = 0x31}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}}, ///< EXT   720P     1280x720  30FPS
	{.raw_mode = {.mode = 0x32}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}}, ///< EXT   720P     1280x720  25FPS
	{.raw_mode = {.mode = 0x34}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}}, ///< EXT   720P     1280x720  30FPS
	{.raw_mode = {.mode = 0x38}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}}, ///< EXT   720P     1280x720  25FPS
	{.raw_mode = {.mode = 0x40}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}}, ///< AHD  1080P     1920x1080 30FPS
	{.raw_mode = {.mode = 0x51}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE}}, ///< AHD   720P     1280x720  60FPS
	{.raw_mode = {.mode = 0x52}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE}}, ///< AHD   720P     1280x720  50FPS
	{.raw_mode = {.mode = 0x54}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE}}, ///< EXT   720P     1280x720  50FPS
	{.raw_mode = {.mode = 0x58}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE}}, ///< EXT   720P     1280x720  60FPS
	{.raw_mode = {.mode = 0x71}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}}, ///< EXC  1080P     1920x1080 30FPS
	{.raw_mode = {.mode = 0x72}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}}, ///< EXC  1080P     1920x1080 25FPS
	{.raw_mode = {.mode = 0x74}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}}, ///< EXT  1080P     1920x1080 30FPS
	{.raw_mode = {.mode = 0x78}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}}, ///< EXT  1080P     1920x1080 25FPS
	{.raw_mode = {.mode = 0x80}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}}, ///< AHD  1080P     1920x1080 25FPS
	{.raw_mode = {.mode = 0x81}, .vin_mode = {.width = 2560, .height = 1440, .fps = 3000, .prog =  TRUE}}, ///< AHD     4M     2560x1440 30FPS
	{.raw_mode = {.mode = 0x82}, .vin_mode = {.width = 2560, .height = 1440, .fps = 2500, .prog =  TRUE}}, ///< AHD     4M     2560x1440 25FPS
	{.raw_mode = {.mode = 0x83}, .vin_mode = {.width = 2560, .height = 1440, .fps = 1500, .prog =  TRUE}}, ///< AHD     4M     2560x1440 15FPS
	{.raw_mode = {.mode = 0x90}, .vin_mode = {.width = 2048, .height = 1536, .fps = 1800, .prog =  TRUE}}, ///< AHD     3M     2048x1536 18FPS
	{.raw_mode = {.mode = 0x91}, .vin_mode = {.width = 2048, .height = 1536, .fps = 3000, .prog =  TRUE}}, ///< AHD     3M     2048x1536 30FPS
	{.raw_mode = {.mode = 0x92}, .vin_mode = {.width = 2048, .height = 1536, .fps = 2500, .prog =  TRUE}}, ///< AHD     3M     2048x1536 25FPS
	{.raw_mode = {.mode = 0x93}, .vin_mode = {.width = 2048, .height = 1536, .fps = 1800, .prog =  TRUE}}, ///< EXT     3M     2048x1536 18FPS
	{.raw_mode = {.mode = 0xA0}, .vin_mode = {.width = 2560, .height = 1944, .fps = 1250, .prog =  TRUE}}, ///< AHD     5M     2560x1944 12.5FPS
	{.raw_mode = {.mode = 0xA1}, .vin_mode = {.width = 2560, .height = 1944, .fps = 2000, .prog =  TRUE}}, ///< AHD     5M     2560x1944 20FPS
};

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_NVP6124B_VIN_MAP g_ad_nvp6124b_vin_map[] = {
	{.vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE}, .ch_mode = {.mode = NVP6124_VI_SD, 		 .pal = FALSE, .rate = AD_NVP6124B_RATE_37M}},
	{.vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE}, .ch_mode = {.mode = NVP6124_VI_SD, 		 .pal =  TRUE, .rate = AD_NVP6124B_RATE_37M}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_720P_2530,  .pal = FALSE, .rate = AD_NVP6124B_RATE_74M}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_720P_2530,  .pal =  TRUE, .rate = AD_NVP6124B_RATE_74M}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_720P_5060,  .pal = FALSE, .rate = AD_NVP6124B_RATE_148M}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_720P_5060,  .pal =  TRUE, .rate = AD_NVP6124B_RATE_148M}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_1080P_2530, .pal = FALSE, .rate = AD_NVP6124B_RATE_148M}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}, .ch_mode = {.mode = NVP6124_VI_1080P_2530, .pal =  TRUE, .rate = AD_NVP6124B_RATE_148M}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
static AD_NVP6124B_VOUT_MAP g_ad_nvp6124b_vout_map[] = {
	{.vout_mode = {.rate = AD_NVP6124B_RATE_37M,   .mux = AD_MUX_NUM_1}, .port_mode = {.mode = NVP6124_OUTMODE_1MUX_SD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_74M,   .mux = AD_MUX_NUM_1}, .port_mode = {.mode = NVP6124_OUTMODE_1MUX_HD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_148M,  .mux = AD_MUX_NUM_1}, .port_mode = {.mode = NVP6124_OUTMODE_1MUX_FHD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_37M,   .mux = AD_MUX_NUM_2}, .port_mode = {.mode = NVP6124_OUTMODE_2MUX_SD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_74M,   .mux = AD_MUX_NUM_2}, .port_mode = {.mode = NVP6124_OUTMODE_2MUX_HD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_148M,  .mux = AD_MUX_NUM_2}, .port_mode = {.mode = NVP6124_OUTMODE_2MUX_FHD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_37M,   .mux = AD_MUX_NUM_4}, .port_mode = {.mode = NVP6124_OUTMODE_4MUX_SD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_74M,   .mux = AD_MUX_NUM_4}, .port_mode = {.mode = NVP6124_OUTMODE_4MUX_HD}},
	{.vout_mode = {.rate = AD_NVP6124B_RATE_148M,  .mux = AD_MUX_NUM_4}, .port_mode = {.mode = NVP6124_OUTMODE_4MUX_FHD_X}},
};

// slave addresses supported by decoder
static UINT32 g_ad_nvp6124b_slv_addr[] = {0x60, 0x62, 0x64, 0x66};

// decoder dump all register information
#define AD_NVP6124B_BANK_ADDR 0xFF
static AD_DRV_DBG_REG_BANK g_ad_nvp6124b_bank[] = {
	{.id = 0x0, .len = 0x100},
	{.id = 0x1, .len = 0x100},
	{.id = 0x2, .len = 0x100},
	{.id = 0x3, .len = 0x100},
	{.id = 0x4, .len = 0x100},
	{.id = 0x5, .len = 0x100},
	{.id = 0x6, .len = 0x100},
	{.id = 0x7, .len = 0x100},
	{.id = 0x8, .len = 0x100},
	{.id = 0x9, .len = 0x100},
	{.id = 0xA, .len = 0x100},
	{.id = 0xB, .len = 0x100},
};
#define AD_NVP6124B_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_NVP6124B_DFT_WIDTH 960
#define AD_NVP6124B_DFT_HEIGHT 480
#define AD_NVP6124B_DFT_FPS 6000
#define AD_NVP6124B_DFT_PROG FALSE
#define AD_NVP6124B_DFT_RATE AD_NVP6124B_RATE_37M
#define AD_NVP6124B_DFT_MUX AD_MUX_NUM_1

// i2c retry number when operation fail occur
#define AD_NVP6124B_I2C_RETRY_NUM 5

// global variable
#define AD_NVP6124B_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_nvp6124b_vin[AD_NVP6124B_CHIP_MAX][AD_NVP6124B_VIN_MAX];
static AD_VOUT g_ad_nvp6124b_vout[AD_NVP6124B_CHIP_MAX][AD_NVP6124B_VOUT_MAX];
static AD_INFO g_ad_nvp6124b_info[AD_NVP6124B_CHIP_MAX] = {
	{
		.name = "NVP6124B_0",
		.vin_max = AD_NVP6124B_VIN_MAX,
		.vin = &g_ad_nvp6124b_vin[0][0],
		.vout_max = AD_NVP6124B_VOUT_MAX,
		.vout = &g_ad_nvp6124b_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6124B_1",
		.vin_max = AD_NVP6124B_VIN_MAX,
		.vin = &g_ad_nvp6124b_vin[1][0],
		.vout_max = AD_NVP6124B_VOUT_MAX,
		.vout = &g_ad_nvp6124b_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6124B_2",
		.vin_max = AD_NVP6124B_VIN_MAX,
		.vin = &g_ad_nvp6124b_vin[2][0],
		.vout_max = AD_NVP6124B_VOUT_MAX,
		.vout = &g_ad_nvp6124b_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6124B_3",
		.vin_max = AD_NVP6124B_VIN_MAX,
		.vin = &g_ad_nvp6124b_vin[3][0],
		.vout_max = AD_NVP6124B_VOUT_MAX,
		.vout = &g_ad_nvp6124b_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_NVP6124B_INFO g_ad_nvp6124b_info_private[AD_NVP6124B_CHIP_MAX] = {
	[0 ... (AD_NVP6124B_CHIP_MAX-1)] = {
		.is_inited = FALSE,
	}
};

static ER ad_nvp6124b_open(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_close(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_init(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_uninit(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_nvp6124b_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_nvp6124b_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_nvp6124b_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_nvp6124b_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_nvp6124b_dbg(char *str_cmd);

static AD_DEV g_ad_nvp6124b_obj = {
	AD_NVP6124B_CHIP_MAX,
	g_ad_nvp6124b_info,

	sizeof(g_ad_nvp6124b_slv_addr) / sizeof(typeof(g_ad_nvp6124b_slv_addr[0])),
	g_ad_nvp6124b_slv_addr,

	{
		.open = ad_nvp6124b_open,
		.close = ad_nvp6124b_close,
		.init = ad_nvp6124b_init,
		.uninit = ad_nvp6124b_uninit,
		.get_cfg = ad_nvp6124b_get_cfg,
		.set_cfg = ad_nvp6124b_set_cfg,
		.chgmode = ad_nvp6124b_chgmode,
		.det_plug_in = ad_nvp6124b_watchdog_cb,
		.i2c_write = ad_nvp6124b_i2c_write,
		.i2c_read = ad_nvp6124b_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_nvp6124b_dbg,
	}
};

// internal function
static AD_INFO *ad_nvp6124b_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_NVP6124B_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_NVP6124B_CHIP_MAX);
		return NULL;
	}
	return &g_ad_nvp6124b_info[chip_id];
}

static AD_NVP6124B_INFO *ad_nvp6124b_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_NVP6124B_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_NVP6124B_CHIP_MAX);
		return NULL;
	}
	return &g_ad_nvp6124b_info_private[chip_id];
}

static void ad_nvp6124b_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_NVP6124B_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_NVP6124B_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_NVP6124B_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_NVP6124B_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_NVP6124B_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_NVP6124B_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_NVP6124B_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_NVP6124B_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_NVP6124B_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_NVP6124B_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_NVP6124B_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_NVP6124B_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

// external i2c write register
void __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value)
{
	UINT32 chip_id;

	for (chip_id = 0; chip_id < AD_NVP6124B_CHIP_MAX; chip_id++) {

		if (g_ad_nvp6124b_info[chip_id].i2c_addr == chip_addr) {

			ad_nvp6124b_i2c_write(chip_id, reg_addr, value, AD_NVP6124B_I2C_DATA_LEN);
			return;
		}
	}
}

// external i2c read register
unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr)
{
	UINT32 chip_id, value = 0;

	for (chip_id = 0; chip_id < AD_NVP6124B_CHIP_MAX; chip_id++) {

		if (g_ad_nvp6124b_info[chip_id].i2c_addr == chip_addr) {

			ad_nvp6124b_i2c_read(chip_id, reg_addr, &value, AD_NVP6124B_I2C_DATA_LEN);
			return (value & 0xff);
		}
	}
	return 0;
}

// implementation of video mode converting
static ER ad_nvp6124b_det_mode_to_vin_mode(AD_NVP6124B_DET_RAW_MODE raw_mode, AD_NVP6124B_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6124b_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6124b_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_nvp6124b_det_map[i_mode].raw_mode, sizeof(AD_NVP6124B_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_nvp6124b_det_map[i_mode].vin_mode, sizeof(AD_NVP6124B_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u\r\n", AD_NVP6124B_MODULE_NAME, raw_mode.mode);
	return E_SYS;
}

static ER ad_nvp6124b_vin_mode_to_ch_mode(AD_NVP6124B_VIN_MODE vin_mode, AD_NVP6124B_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6124b_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6124b_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_nvp6124b_vin_map[i_mode].vin_mode, sizeof(AD_NVP6124B_VIN_MODE)) == 0) {
			*ch_mode = g_ad_nvp6124b_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_NVP6124B_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}

static ER ad_nvp6124b_vout_mode_to_port_mode(AD_NVP6124B_VOUT_MODE vout_mode, AD_NVP6124B_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6124b_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6124b_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_nvp6124b_vout_map[i_mode].vout_mode, sizeof(AD_NVP6124B_VOUT_MODE)) == 0) {
			*port_mode = g_ad_nvp6124b_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_NVP6124B_MODULE_NAME);
	return E_SYS;
}

// implementation of video mode detection

static ER ad_nvp6124b_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_NVP6124B_DET_VIDEO_INFO *video_info)
{
	UINT32 vch;
	nvp6124_input_videofmt videofmt = {0};
	AD_NVP6124B_DET_RAW_MODE raw_mode = {0};
	AD_NVP6124B_DET_VIN_MODE vin_mode = {0};

	vch = (chip_id * AD_NVP6124B_VIN_MAX) + vin_id;

	video_fmt_det(&videofmt);
	AD_IND("chip_id%u vch%u videofmt.getvideofmt[%u] %x\r\n", chip_id, vch, vch, videofmt.getvideofmt[vch]);

	raw_mode.mode = videofmt.getvideofmt[vch];
	if (ad_nvp6124b_det_mode_to_vin_mode(raw_mode, &vin_mode) != E_OK) {
		goto do_vloss;
	}

	video_info->vloss = FALSE;
	video_info->width = vin_mode.width;
	video_info->height = vin_mode.height;
	video_info->fps = vin_mode.fps;
	video_info->prog = vin_mode.prog;
	return E_OK;

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_NVP6124B_DFT_WIDTH;
	video_info->height = AD_NVP6124B_DFT_HEIGHT;
	video_info->fps = AD_NVP6124B_DFT_FPS;
	video_info->prog = AD_NVP6124B_DFT_PROG;
	return E_OK;
}

static ER ad_nvp6124b_dbg_wdt(UINT32 chip_id)
{
	AD_INFO *ad_info;
	AD_DECET_INFO detect_info = {0};
	UINT32 i_vin;

	ad_info = ad_nvp6124b_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad dbg wdt fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	for (i_vin = 0; i_vin < ad_info->vin_max; i_vin++) {
		detect_info.vin_id = i_vin;
		ad_nvp6124b_watchdog_cb(chip_id, &detect_info);
		ad_info->vin[i_vin].det_format = detect_info.det_info;
	}

	return E_OK;
}

static ER ad_nvp6124b_dbg_chgmode(UINT32 chip_id)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO chgmode_info = {0};
	UINT32 i_vin;

	ad_info = ad_nvp6124b_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad dbg chgmode fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	for (i_vin = 0; i_vin < 2; i_vin++) {
		chgmode_info.vin_id = i_vin;
		chgmode_info.vout_id = i_vin;
		chgmode_info.ui_info = ad_info->vin[i_vin].det_format;
		ad_nvp6124b_chgmode(chip_id, &chgmode_info);
		ad_info->vin[i_vin].cur_format = chgmode_info.cur_info;
	}

	return E_OK;
}

static ER ad_nvp6124b_dbg_mux(UINT32 chip_id, UINT32 mux)
{
	AD_INFO *ad_info;
	UINT32 i_vout;

	ad_info = ad_nvp6124b_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad dbg chgmode fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	for (i_vout = 0; i_vout < 2; i_vout++) {
		ad_info->vout[i_vout].mux_num = mux;
	}

	return E_OK;
}

#if 0
#endif

static ER ad_nvp6124b_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_nvp6124b_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	ad_nvp6124b_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	//set for next chip driver
	nvp6124_slave_addr[chip_id] = ad_info->i2c_addr;

	drv_open_info.module_name = AD_NVP6124B_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_NVP6124B_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_NVP6124B_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_NVP6124B_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_NVP6124B_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_NVP6124B_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_nvp6124b_bank) / sizeof(typeof(g_ad_nvp6124b_bank[0]));
	reg_info.bank = g_ad_nvp6124b_bank;
	reg_info.dump_per_row = AD_NVP6124B_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_nvp6124b_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_nvp6124b_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_nvp6124b_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_nvp6124b_init(UINT32 _chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_NVP6124B_INFO *ad_private_info;
	UINT32 i_vin, i_vout, vch;
	AD_NVP6124B_VIN_MODE vin_mode = {0};
	AD_NVP6124B_CH_MODE ch_mode = {0};
	AD_NVP6124B_VOUT_MODE vout_mode = {0};
	AD_NVP6124B_PORT_MODE port_mode = {0};

	ad_info = ad_nvp6124b_get_info(_chip_id);
	ad_private_info = ad_nvp6124b_get_private_info(_chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	/************ rewrite external driver's module init function ************/

	// nvp6124_module_init()

	printk("AHD2.0 EXT Driver %s\n", DRIVER_VER);

	nvp6124_cnt = 0;

	chip_id[_chip_id] = check_id(nvp6124_slave_addr[_chip_id]);
	rev_id[_chip_id]  = check_rev(nvp6124_slave_addr[_chip_id]);

	if (chip_id[_chip_id] != NVP6124B_R0_ID) {
		printk("AHD2.0 Device ID Error... %x\n", chip_id[_chip_id]);
	} else {
		printk("AHD2.0 Device (0x%x) ID OK... %x\n", nvp6124_slave_addr[_chip_id], chip_id[_chip_id]);
		printk("AHD2.0 Device (0x%x) REV ... %x\n", nvp6124_slave_addr[_chip_id], rev_id[_chip_id]);
		nvp6124_slave_addr[nvp6124_cnt] = nvp6124_slave_addr[_chip_id];
		chip_id[nvp6124_cnt] = chip_id[_chip_id];
		rev_id[nvp6124_cnt]  = rev_id[_chip_id];
		nvp6124_cnt++;
	}


	printk("AHD2.0 Count = %x\n", nvp6124_cnt);

	nvp6124_common_init();

	audio_init(1,0,16,0,0);


	/************ set default resolution ************/

	for (i_vin = 0; i_vin < ad_info->vin_max; i_vin++) {

		vch = (_chip_id * AD_NVP6124B_VIN_MAX) + i_vin;

		vin_mode.width = AD_NVP6124B_DFT_WIDTH;
		vin_mode.height = AD_NVP6124B_DFT_HEIGHT;
		vin_mode.fps = AD_NVP6124B_DFT_FPS;
		vin_mode.prog = AD_NVP6124B_DFT_PROG;
		ad_nvp6124b_vin_mode_to_ch_mode(vin_mode, &ch_mode);

		nvp6124_set_chnmode(vch, ch_mode.pal, ch_mode.mode);
	}

	for (i_vout = 0; i_vout < ad_info->vout_max; i_vout++) {

		vout_mode.rate = AD_NVP6124B_DFT_RATE;
		vout_mode.mux = AD_NVP6124B_DFT_MUX;
		ad_nvp6124b_vout_mode_to_port_mode(vout_mode, &port_mode);

		nvp6124b_set_portmode(_chip_id, i_vout, port_mode.mode, 0);
	}

	ad_private_info->is_inited = TRUE;

done:
	g_ad_nvp6124b_info[_chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_nvp6124b_uninit(UINT32 chip_id, void *ext_data)
{
	AD_NVP6124B_INFO *ad_private_info;

	ad_private_info = ad_nvp6124b_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	/************ rewrite external driver's module exit function ************/

	// jaguar1_module_exit()

	printk("JAGUAR1 DEVICE DRIVER UNLOAD SUCCESS\n");

	ad_private_info->is_inited = FALSE;
	g_ad_nvp6124b_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_nvp6124b_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_nvp6124b_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
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
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_NVP6124B_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			if (unlikely(mode_info->out_id >= ad_info->vout_max)) {
				AD_ERR("[%s] mode_info wrong input\r\n", AD_NVP6124B_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			mode_info->bus_type = AD_BUS_TYPE_PARALLEL;
			mode_info->mux_num = ad_info->vout[mode_info->out_id].mux_num;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 148500000;
			mode_info->data_rate = 148500000;
			mode_info->protocal = AD_DATA_PROTCL_CCIR656;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY;
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_NVP6124B_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_NVP6124B_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_nvp6124b_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_nvp6124b_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_NVP6124B_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_NVP6124B_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_nvp6124b_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id, vch;
	AD_NVP6124B_VIN_MODE vin_mode = {0};
	AD_NVP6124B_CH_MODE ch_mode = {0};
	AD_NVP6124B_VOUT_MODE vout_mode = {0};
	AD_NVP6124B_PORT_MODE port_mode = {0};

	ad_info = ad_nvp6124b_get_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_NVP6124B_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_NVP6124B_MODULE_NAME, vin_id, AD_NVP6124B_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_NVP6124B_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_NVP6124B_MODULE_NAME, vout_id, AD_NVP6124B_VOUT_MAX);
		return E_SYS;
	}

	vch = (chip_id * AD_NVP6124B_VIN_MAX) + vin_id;

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	//DBG_DUMP("chip%d vin%d: w = %d, h = %d, fps = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps);

	if (unlikely(ad_nvp6124b_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	nvp6124_set_chnmode(vch, ch_mode.pal, ch_mode.mode);

	vout_mode.rate = ch_mode.rate;
	vout_mode.mux = ad_info->vout[vout_id].mux_num;
	if (unlikely(ad_nvp6124b_vout_mode_to_port_mode(vout_mode, &port_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get port mode fail\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	if (vout_mode.mux == AD_MUX_NUM_1) {
		nvp6124b_set_portmode(chip_id, vout_id, port_mode.mode, vin_id);
	} else if (vout_mode.mux == AD_MUX_NUM_2) {
		nvp6124b_set_portmode(chip_id, vout_id, port_mode.mode, vin_id / 2);
	} else if (vout_mode.mux == AD_MUX_NUM_4) {
		nvp6124b_set_portmode(chip_id, vout_id, port_mode.mode, 0); // vin don't care
	} else {
		AD_ERR("[%s c%ui%uo%u] Wrong mux %u\r\n", AD_NVP6124B_MODULE_NAME, chip_id, vin_id, vout_id, vout_mode.mux);
	}

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;

	return E_OK;
}

static ER ad_nvp6124b_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_NVP6124B_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_nvp6124b_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_NVP6124B_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_nvp6124b_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_nvp6124b_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_nvp6124b_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_NVP6124B_MODULE_NAME, i_vin);
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

static ER ad_nvp6124b_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_nvp6124b_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_nvp6124b_dbg(char *str_cmd)
{
#define AD_NVP6124B_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_NVP6124B_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_NVP6124B_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_NVP6124B_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_NVP6124B_DBG_CMD_MAX] = {0};
	//UINT32 val = 0, val2 = 0, i_chip = 0, i_vin;

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_NVP6124B_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_NVP6124B_DBG_CMD_MAX);

	if (cmd_num == 1 && strcmp(cmd_list[0], "wdt") == 0) {
		ad_nvp6124b_dbg_wdt(0);
	} else if (cmd_num == 1 && strcmp(cmd_list[0], "chgmode") == 0) {
		ad_nvp6124b_dbg_chgmode(0);
	} else if (cmd_num == 2 && strcmp(cmd_list[0], "mux") == 0) {
		if (sscanf_s(cmd_list[1], "%u", &narg[0]) == 1) {
			ad_nvp6124b_dbg_mux(0, narg[0]);
		} else {
			DBG_DUMP("mux err\r\n");
		}
	}

	return E_OK;
}

AD_DEV* ad_get_obj_nvp6124b(void)
{
	return &g_ad_nvp6124b_obj;
}

