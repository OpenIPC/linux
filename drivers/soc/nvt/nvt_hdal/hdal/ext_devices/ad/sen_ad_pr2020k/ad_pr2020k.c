#include "ad_pr2020k.h"
#include "nvt_ad_pr2020k_reg.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 02, 000, 00);

// mapping table: vin raw data -> meaningful vin info
static AD_PR2020K_DET_MAP g_ad_pr2020k_det_map[] = {
	{.raw_mode = {.mode = 0x8A}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}},
	{.raw_mode = {.mode = 0x9A}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}},
	{.raw_mode = {.mode = 0x8B}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}},
	{.raw_mode = {.mode = 0x9B}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}},
};

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_PR2020K_VIN_MAP g_ad_pr2020k_vin_map[] = {
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE}, .ch_mode = {.mode = AD_PR2020K_VMODE_720P_25}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE}, .ch_mode = {.mode = AD_PR2020K_VMODE_720P_30}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE}, .ch_mode = {.mode = AD_PR2020K_VMODE_1080P_25}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE}, .ch_mode = {.mode = AD_PR2020K_VMODE_1080P_30}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_PR2020K_VOUT_MAP g_ad_pr2020k_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_pr2020k_slv_addr[] = {0xB8, 0xBA, 0xBC, 0xBE};

// decoder dump all register information
#define AD_PR2020K_BANK_ADDR AD_PR2020K_PAGE_SEL_REG_OFS
static AD_DRV_DBG_REG_BANK g_ad_pr2020k_bank[] = {
	{.id = 0x00, .len = 0x100},
	{.id = 0x01, .len = 0x100},
	{.id = 0x02, .len = 0x100},
};
#define AD_PR2020K_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_PR2020K_DFT_WIDTH 1280
#define AD_PR2020K_DFT_HEIGHT 720
#define AD_PR2020K_DFT_FPS 2500
#define AD_PR2020K_DFT_PROG TRUE

// i2c retry number when operation fail occur
#define AD_PR2020K_I2C_RETRY_NUM 5

// global variable
#define AD_PR2020K_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_pr2020k_vin[AD_PR2020K_CHIP_MAX][AD_PR2020K_VIN_MAX];
static AD_VOUT g_ad_pr2020k_vout[AD_PR2020K_CHIP_MAX][AD_PR2020K_VOUT_MAX];
static AD_INFO g_ad_pr2020k_info[AD_PR2020K_CHIP_MAX] = {
	{
		.name = "PR2020K_0",
		.vin_max = AD_PR2020K_VIN_MAX,
		.vin = &g_ad_pr2020k_vin[0][0],
		.vout_max = AD_PR2020K_VOUT_MAX,
		.vout = &g_ad_pr2020k_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "PR2020K_1",
		.vin_max = AD_PR2020K_VIN_MAX,
		.vin = &g_ad_pr2020k_vin[1][0],
		.vout_max = AD_PR2020K_VOUT_MAX,
		.vout = &g_ad_pr2020k_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "PR2020K_2",
		.vin_max = AD_PR2020K_VIN_MAX,
		.vin = &g_ad_pr2020k_vin[2][0],
		.vout_max = AD_PR2020K_VOUT_MAX,
		.vout = &g_ad_pr2020k_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "PR2020K_3",
		.vin_max = AD_PR2020K_VIN_MAX,
		.vin = &g_ad_pr2020k_vin[3][0],
		.vout_max = AD_PR2020K_VOUT_MAX,
		.vout = &g_ad_pr2020k_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_PR2020K_INFO g_ad_pr2020k_info_private[AD_PR2020K_CHIP_MAX] = {
	[0 ... (AD_PR2020K_CHIP_MAX-1)] = {
		.is_inited = FALSE,
	}
};

static ER ad_pr2020k_open(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_close(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_init(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_uninit(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_pr2020k_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_pr2020k_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_pr2020k_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_pr2020k_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_pr2020k_dbg(char *str_cmd);

static AD_DEV g_ad_pr2020k_obj = {
	AD_PR2020K_CHIP_MAX,
	g_ad_pr2020k_info,

	sizeof(g_ad_pr2020k_slv_addr) / sizeof(typeof(g_ad_pr2020k_slv_addr[0])),
	g_ad_pr2020k_slv_addr,

	{
		.open = ad_pr2020k_open,
		.close = ad_pr2020k_close,
		.init = ad_pr2020k_init,
		.uninit = ad_pr2020k_uninit,
		.get_cfg = ad_pr2020k_get_cfg,
		.set_cfg = ad_pr2020k_set_cfg,
		.chgmode = ad_pr2020k_chgmode,
		.det_plug_in = ad_pr2020k_watchdog_cb,
		.i2c_write = ad_pr2020k_i2c_write,
		.i2c_read = ad_pr2020k_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_pr2020k_dbg,
	}
};

// internal function
static AD_INFO *ad_pr2020k_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_PR2020K_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_PR2020K_CHIP_MAX);
		return NULL;
	}
	return &g_ad_pr2020k_info[chip_id];
}

static AD_PR2020K_INFO *ad_pr2020k_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_PR2020K_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_PR2020K_CHIP_MAX);
		return NULL;
	}
	return &g_ad_pr2020k_info_private[chip_id];
}

static void ad_pr2020k_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_PR2020K_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_PR2020K_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_PR2020K_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_PR2020K_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_PR2020K_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_PR2020K_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_PR2020K_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_PR2020K_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_PR2020K_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_PR2020K_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_PR2020K_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_PR2020K_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

typedef struct {
	UINT8 addr;
	UINT8 val;
} AD_PR2020K_I2C_REG;

static ER ad_pr2020k_i2c_write_tbl(UINT32 chip_id, const AD_PR2020K_I2C_REG *tbl, UINT32 cnt)
{
	UINT32 i;

	for (i = 0; i < cnt; i++) {

		if (ad_pr2020k_i2c_write(chip_id, tbl[i].addr, tbl[i].val, AD_PR2020K_I2C_DATA_LEN) != E_OK) {
			return E_SYS;
		}
	}

	return E_OK;
}

static AD_PR2020K_I2C_REG ad_pr2020k_mode_720p[] = {
	{0xff, 0x00},
	{0x10, 0x82},
	//{0x11, 0x07},
	{0x11, 0x00},
	{0x12, 0x00},
	{0x13, 0x00},
	{0x14, 0x21},
	{0x15, 0x44},
	{0x16, 0x0d},
	{0x40, 0x00},
	{0x47, 0x3a},
	{0x4e, 0x3f},
	{0x80, 0x56},
	{0x81, 0x0e},
	{0x82, 0x0d},
	{0x84, 0x30},
	{0x86, 0x20},
	{0x87, 0x00},
	{0x8a, 0x00},
	{0x90, 0x00},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x94, 0xff},
	{0x95, 0xff},
	{0x96, 0xff},
	{0xa0, 0x01},
	{0xa1, 0xe8},
	{0xa4, 0x00},
	{0xa5, 0x01}, //0x11
	{0xa6, 0x00},
	{0xa7, 0x05}, //0x81
	{0xa8, 0x00},
	{0xd0, 0x30},
	{0xd1, 0x08},
	{0xd2, 0x21},
	{0xd3, 0x00},
	{0xd8, 0x31},
	{0xd9, 0x08},
	{0xda, 0x21},
	{0xe0, 0x39},
	{0xe1, 0xd0}, //output 74.25MHz
	{0xe2, 0x38},
	{0xe3, 0x19},
	{0xe4, 0x19},
	{0xea, 0x01},
	{0xeb, 0xff},
	{0xf1, 0x44},
	{0xf2, 0x01},
	{0xff, 0x01},

	{0x00, 0xe5},
	{0x01, 0x61},
	{0x02, 0x00},
	{0x03, 0x57},
	{0x04, 0x0c},
	{0x05, 0x88},
	{0x06, 0x04},
	{0x07, 0xb2},
	{0x08, 0x44},
	{0x09, 0x34},
	{0x0a, 0x02},
	{0x0b, 0x14},
	{0x0c, 0x04},
	{0x0d, 0x08},
	{0x0e, 0x5e},
	{0x0f, 0x5e},
	{0x10, 0x26},
	{0x11, 0x01},
	{0x12, 0x45},
	{0x13, 0x0c},
	{0x14, 0x00},
	{0x15, 0x1b},
	{0x16, 0xd0},
	{0x17, 0x00},
	{0x18, 0x41},
	{0x19, 0x46},
	{0x1a, 0x22},
	{0x1b, 0x05},
	{0x1c, 0xea},
	{0x1d, 0x45},
	{0x1e, 0x4c},
	{0x1f, 0x00},
	{0x20, 0x80},
	{0x21, 0x80},
	{0x22, 0x60}, //0x90
	{0x23, 0x80},
	{0x24, 0x80},
	{0x25, 0x80},
	{0x26, 0x84},
	{0x27, 0x82},
	{0x28, 0x00},
	{0x29, 0x7d},
	{0x2a, 0x00},
	{0x2b, 0x00},
	{0x2c, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	{0x2f, 0x00},
	{0x30, 0x00},
	{0x31, 0x00},
	{0x32, 0xc0},
	{0x33, 0x14},
	{0x34, 0x14},
	{0x35, 0x80},
	{0x36, 0x80},
	{0x37, 0xaa},
	{0x38, 0x48},
	{0x39, 0x00}, //0x08
	{0x3a, 0x27},
	{0x3b, 0x02},
	{0x3c, 0x01},
	{0x3d, 0x23},
	{0x3e, 0x02},
	{0x3f, 0xc4},
	{0x40, 0x05},
	{0x41, 0x55},
	{0x42, 0x01},
	{0x43, 0x33},
	{0x44, 0x6a},
	{0x45, 0x00},
	{0x46, 0x09},
	{0x47, 0xe2},
	{0x48, 0x01},
	{0x49, 0x00},
	{0x4a, 0x7b},
	{0x4b, 0x60},
	{0x4c, 0x00},
	{0x4d, 0x4a},
	{0x4e, 0x00},
	{0x4f, 0x20}, //0x24
	{0x50, 0x01},
	{0x51, 0x28},
	{0x52, 0x40},
	{0x53, 0x0c},
	{0x54, 0x0f},
	{0x55, 0x8d},
	{0x70, 0x06},
	{0x71, 0x08},
	{0x72, 0x0a},
	{0x73, 0x0c},
	{0x74, 0x0e},
	{0x75, 0x10},
	{0x76, 0x12},
	{0x77, 0x14},
	{0x78, 0x06},
	{0x79, 0x08},
	{0x7a, 0x0a},
	{0x7b, 0x0c},
	{0x7c, 0x0e},
	{0x7d, 0x10},
	{0x7e, 0x12},
	{0x7f, 0x14},
	{0x80, 0x00},
	{0x81, 0x09},
	{0x82, 0x00},
	{0x83, 0x07},
	{0x84, 0x00},
	{0x85, 0x17},
	{0x86, 0x03},
	{0x87, 0xe5},
	{0x88, 0x0a},
	{0x89, 0x48},
	{0x8a, 0x0a},
	{0x8b, 0x48},
	{0x8c, 0x0b},
	{0x8d, 0xe0},
	{0x8e, 0x05},
	{0x8f, 0x47},
	{0x90, 0x05},
	{0x91, 0x69},
	{0x92, 0x73},
	{0x93, 0xe8},
	{0x94, 0x0f},
	{0x95, 0x5e},
	{0x96, 0x07},
	{0x97, 0x90},
	{0x98, 0x17},
	{0x99, 0x34},
	{0x9a, 0x13},
	{0x9b, 0x56},
	{0x9c, 0x0b},
	{0x9d, 0x9a},
	{0x9e, 0x09},
	{0x9f, 0xab},
	{0xa0, 0x01},
	{0xa1, 0x74},
	{0xa2, 0x01},
	{0xa3, 0x6b},
	{0xa4, 0x00},
	{0xa5, 0xba},
	{0xa6, 0x00},
	{0xa7, 0xa3},
	{0xa8, 0x01},
	{0xa9, 0x39},
	{0xaa, 0x01},
	{0xab, 0x39},
	{0xac, 0x00},
	{0xad, 0xc1},
	{0xae, 0x00},
	{0xaf, 0xc1},
	{0xb0, 0x0b},
	{0xb1, 0x99},
	{0xb2, 0x12},
	{0xb3, 0xca},
	{0xb4, 0x00},
	{0xb5, 0x17},
	{0xb6, 0x08},
	{0xb7, 0xe8},
	{0xb8, 0xb0},
	{0xb9, 0xce},
	{0xba, 0x90},
	{0xbb, 0x00},
	{0xbc, 0x00},
	{0xbd, 0x04},
	{0xbe, 0x05},
	{0xbf, 0x00},
	{0xc0, 0x00},
	{0xc1, 0x19},//{0x12, 0x00}},//v delay
	{0xc2, 0x02},
	{0xc3, 0xd2},//{0xd0, 0x00}
	{0xff, 0x01},
	{0x54, 0x0e},
	{0xff, 0x01},
	{0x54, 0x0f},

	{0xff, 0x00}, //test
};

static AD_PR2020K_I2C_REG ad_pr2020k_mode_1080p[] = {
	{0xff, 0x00},
	{0x10, 0x83},
	//{0x11, 0x07},
	{0x11, 0x00},
	{0x12, 0x00},
	{0x13, 0x00},
	{0x14, 0x21},
	{0x15, 0x44},
	{0x16, 0x0d},
	{0x40, 0x00},
	{0x47, 0x3a},
	{0x4e, 0x3f},
	{0x80, 0x56},
	{0x81, 0x0e},
	{0x82, 0x0d},
	{0x84, 0x30},
	{0x86, 0x20},
	{0x87, 0x00},
	{0x8a, 0x00},
	{0x90, 0x00},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x94, 0xff},
	{0x95, 0xff},
	{0x96, 0xff},
	{0xa0, 0x00},
	{0xa1, 0x20},
	{0xa4, 0x01},
	{0xa5, 0xe3}, //0x11
	{0xa6, 0x00},
	{0xa7, 0x12}, //0x81
	{0xa8, 0x00},
	{0xd0, 0x30},
	{0xd1, 0x08},
	{0xd2, 0x21},
	{0xd3, 0x00},
	{0xd8, 0x30},
	{0xd9, 0x08},
	{0xda, 0x21},
	{0xe0, 0x35},
	{0xe1, 0xc0}, //output 148.5MHz
	{0xe2, 0x18},
	{0xe3, 0x00},
	{0xe4, 0x00},
	{0xea, 0x01},
	{0xeb, 0xff},
	{0xf1, 0x44},
	{0xf2, 0x01},

	{0xff, 0x01},
	{0x00, 0xe4},
	{0x01, 0x61},
	{0x02, 0x00},
	{0x03, 0x57},
	{0x04, 0x0c},
	{0x05, 0x88},
	{0x06, 0x04},
	{0x07, 0xb2},
	{0x08, 0x44},
	{0x09, 0x34},
	{0x0a, 0x02},
	{0x0b, 0x14},
	{0x0c, 0x04},
	{0x0d, 0x08},
	{0x0e, 0x5e},
	{0x0f, 0x5e},
	{0x10, 0x26},
	{0x11, 0x00},
	{0x12, 0x87},
	{0x13, 0x24},
	{0x14, 0x80},
	{0x15, 0x2a},
	{0x16, 0x38},
	{0x17, 0x00},
	{0x18, 0x80},
	{0x19, 0x48},
	{0x1a, 0x6c},
	{0x1b, 0x05},
	{0x1c, 0x61},
	{0x1d, 0x07},
	{0x1e, 0x7e},
	{0x1f, 0x80},
	{0x20, 0x80},
	{0x21, 0x80},
	{0x22, 0x90}, //0x90
	{0x23, 0x80},
	{0x24, 0x80},
	{0x25, 0x80},
	{0x26, 0x84},
	{0x27, 0x82},
	{0x28, 0x00},
	{0x29, 0xff},
	{0x2a, 0xff},
	{0x2b, 0x00},
	{0x2c, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	{0x2f, 0x00},
	{0x30, 0x00},
	{0x31, 0x00},
	{0x32, 0xc0},
	{0x33, 0x14},
	{0x34, 0x14},
	{0x35, 0x80},
	{0x36, 0x80},
	{0x37, 0xad},
	{0x38, 0x4b},
	{0x39, 0x08}, //0x08
	{0x3a, 0x21},
	{0x3b, 0x02},
	{0x3c, 0x01},
	{0x3d, 0x23},
	{0x3e, 0x05},
	{0x3f, 0xc8},
	{0x40, 0x05},
	{0x41, 0x55},
	{0x42, 0x01},
	{0x43, 0x38},
	{0x44, 0x6a},
	{0x45, 0x00},
	{0x46, 0x14},
	{0x47, 0xb0},
	{0x48, 0xdf},
	{0x49, 0x00},
	{0x4a, 0x7b},
	{0x4b, 0x60},
	{0x4c, 0x00},
	{0x4d, 0x26},
	{0x4e, 0x00},
	{0x4f, 0x20}, //0x24
	{0x50, 0x01},
	{0x51, 0x28},
	{0x52, 0x40},
	{0x53, 0x0c},
	{0x54, 0x0f},
	{0x55, 0x8d},
	{0x70, 0x06},
	{0x71, 0x08},
	{0x72, 0x0a},
	{0x73, 0x0c},
	{0x74, 0x0e},
	{0x75, 0x10},
	{0x76, 0x12},
	{0x77, 0x14},
	{0x78, 0x06},
	{0x79, 0x08},
	{0x7a, 0x0a},
	{0x7b, 0x0c},
	{0x7c, 0x0e},
	{0x7d, 0x10},
	{0x7e, 0x12},
	{0x7f, 0x14},
	{0x80, 0x00},
	{0x81, 0x09},
	{0x82, 0x00},
	{0x83, 0x07},
	{0x84, 0x00},
	{0x85, 0x17},
	{0x86, 0x03},
	{0x87, 0xe5},
	{0x88, 0x05},
	{0x89, 0x24},
	{0x8a, 0x05},
	{0x8b, 0x24},
	{0x8c, 0x08},
	{0x8d, 0xe8},
	{0x8e, 0x05},
	{0x8f, 0x47},
	{0x90, 0x02},
	{0x91, 0xb4},
	{0x92, 0x73},
	{0x93, 0xe8},
	{0x94, 0x0f},
	{0x95, 0x5e},
	{0x96, 0x03},
	{0x97, 0xd0},
	{0x98, 0x17},
	{0x99, 0x34},
	{0x9a, 0x13},
	{0x9b, 0x56},
	{0x9c, 0x0b},
	{0x9d, 0x9a},
	{0x9e, 0x09},
	{0x9f, 0xab},
	{0xa0, 0x01},
	{0xa1, 0x74},
	{0xa2, 0x01},
	{0xa3, 0x6b},
	{0xa4, 0x00},
	{0xa5, 0xba},
	{0xa6, 0x00},
	{0xa7, 0xa3},
	{0xa8, 0x01},
	{0xa9, 0x39},
	{0xaa, 0x01},
	{0xab, 0x39},
	{0xac, 0x00},
	{0xad, 0xc1},
	{0xae, 0x00},
	{0xaf, 0xc1},
	{0xb0, 0x05},
	{0xb1, 0xcc},
	{0xb2, 0x09},
	{0xb3, 0x6d},
	{0xb4, 0x00},
	{0xb5, 0x17},
	{0xb6, 0x08},
	{0xb7, 0xe8},
	{0xb8, 0xb0},
	{0xb9, 0xce},
	{0xba, 0x90},
	{0xbb, 0x00},
	{0xbc, 0x00},
	{0xbd, 0x04},
	{0xbe, 0x07},
	{0xbf, 0x80},
	{0xc0, 0x00},
	{0xc1, 0x20}, //{0x12, 0x00}},//v delay
	{0xc2, 0x04},
	{0xc3, 0x38}, //{0xd0, 0x00}
	{0xff, 0x01},
	{0x54, 0x0e},
	{0xff, 0x01},
	{0x54, 0x0f},

	{0xff, 0x00}, //test
};

static ER ad_pr2020k_set_video_mode(UINT32 chip_id, UINT32 fmt)
{
	AD_IND("SET VIDEO MODE chip%u fmt%u(0x%x)\r\n", chip_id, fmt, fmt);

	switch (fmt) {
	case AD_PR2020K_VMODE_720P_25:
	case AD_PR2020K_VMODE_720P_30:
		ad_pr2020k_i2c_write_tbl(chip_id, ad_pr2020k_mode_720p, sizeof(ad_pr2020k_mode_720p)/sizeof(typeof(ad_pr2020k_mode_720p[0])));
		break;
	case AD_PR2020K_VMODE_1080P_25:
	case AD_PR2020K_VMODE_1080P_30:
		ad_pr2020k_i2c_write_tbl(chip_id, ad_pr2020k_mode_1080p, sizeof(ad_pr2020k_mode_1080p)/sizeof(typeof(ad_pr2020k_mode_1080p[0])));
		break;
	default:
		AD_ERR("[%s] Unknown video fmt %d\r\n", AD_PR2020K_MODULE_NAME, fmt);
		return E_SYS;
	}

	return E_OK;
}

// implementation of video mode converting
static ER ad_pr2020k_det_mode_to_vin_mode(AD_PR2020K_DET_RAW_MODE raw_mode, AD_PR2020K_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_pr2020k_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_pr2020k_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_pr2020k_det_map[i_mode].raw_mode, sizeof(AD_PR2020K_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_pr2020k_det_map[i_mode].vin_mode, sizeof(AD_PR2020K_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u\r\n", AD_PR2020K_MODULE_NAME, raw_mode.mode);
	return E_SYS;
}

static ER ad_pr2020k_vin_mode_to_ch_mode(AD_PR2020K_VIN_MODE vin_mode, AD_PR2020K_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_pr2020k_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_pr2020k_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_pr2020k_vin_map[i_mode].vin_mode, sizeof(AD_PR2020K_VIN_MODE)) == 0) {
			*ch_mode = g_ad_pr2020k_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode w:%u h:%u fps:%u prog:%u\r\n", AD_PR2020K_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}

/*static ER ad_pr2020k_vout_mode_to_port_mode(AD_PR2020K_VOUT_MODE vout_mode, AD_PR2020K_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_pr2020k_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_pr2020k_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_pr2020k_vout_map[i_mode].vout_mode, sizeof(AD_PR2020K_VOUT_MODE)) == 0) {
			*port_mode = g_ad_pr2020k_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_PR2020K_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_pr2020k_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_PR2020K_DET_VIDEO_INFO *video_info)
{
	AD_PR2020K_DET_RAW_MODE raw_mode = {0};
	AD_PR2020K_DET_VIN_MODE vin_mode = {0};
	AD_PR2020K_VID_STATUS_1_REG vid_status_1 = {0};

	if (unlikely(chip_id >= AD_PR2020K_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_PR2020K_MODULE_NAME, chip_id, AD_PR2020K_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_PR2020K_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_PR2020K_MODULE_NAME, vin_id, AD_PR2020K_VIN_MAX);
		return E_SYS;
	}

	ad_pr2020k_i2c_write(chip_id, AD_PR2020K_PAGE_SEL_REG_OFS, 0x00, AD_PR2020K_I2C_DATA_LEN);
	ad_pr2020k_i2c_read(chip_id, AD_PR2020K_VID_STATUS_0_REG_OFS, &raw_mode.mode, AD_PR2020K_I2C_DATA_LEN);
	ad_pr2020k_i2c_read(chip_id, AD_PR2020K_VID_STATUS_1_REG_OFS, &vid_status_1.reg, AD_PR2020K_I2C_DATA_LEN);

	AD_IND("STS0 0x%x, STS1 0x%x\r\n", raw_mode.mode, vid_status_1.reg);

	if (vid_status_1.bit.LOCK_HPLL == 0 || vid_status_1.bit.LOCK_STD == 0) {
		goto do_vloss;
	}

	if (ad_pr2020k_det_mode_to_vin_mode(raw_mode, &vin_mode) != E_OK) {
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
	video_info->width = AD_PR2020K_DFT_WIDTH;
	video_info->height = AD_PR2020K_DFT_HEIGHT;
	video_info->fps = AD_PR2020K_DFT_FPS;
	video_info->prog = AD_PR2020K_DFT_PROG;
	return E_OK;
}

#if 0
#endif

static ER ad_pr2020k_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_pr2020k_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	ad_pr2020k_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_PR2020K_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_PR2020K_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_PR2020K_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_PR2020K_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_PR2020K_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_PR2020K_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_pr2020k_bank) / sizeof(typeof(g_ad_pr2020k_bank[0]));
	reg_info.bank = g_ad_pr2020k_bank;
	reg_info.dump_per_row = AD_PR2020K_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_pr2020k_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_pr2020k_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_pr2020k_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_pr2020k_init(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_PR2020K_INFO *ad_private_info;
	AD_PR2020K_VIN_MODE vin_mode = {0};
	AD_PR2020K_CH_MODE ch_mode = {0};

	ad_info = ad_pr2020k_get_info(chip_id);
	ad_private_info = ad_pr2020k_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	/************ set default resolution ************/
	vin_mode.width = AD_PR2020K_DFT_WIDTH;
	vin_mode.height = AD_PR2020K_DFT_HEIGHT;
	vin_mode.fps = AD_PR2020K_DFT_FPS;
	vin_mode.prog = AD_PR2020K_DFT_PROG;
	if (ad_pr2020k_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
		AD_WRN("[%s] ad init wrn. get ch mode fail\r\n", AD_PR2020K_MODULE_NAME);
	}

	ad_pr2020k_set_video_mode(chip_id, ch_mode.mode);

	ad_private_info->is_inited = TRUE;

done:
	g_ad_pr2020k_info[chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_pr2020k_uninit(UINT32 chip_id, void *ext_data)
{
	AD_PR2020K_INFO *ad_private_info;

	ad_private_info = ad_pr2020k_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	ad_private_info->is_inited = FALSE;
	g_ad_pr2020k_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_pr2020k_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_pr2020k_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_DATA_FORMAT:
			data_fmt = (AD_VOUT_DATA_FMT_INFO *)data;

			data_fmt->fmt = AD_DATA_FMT_422;
			data_fmt->protocal = AD_DATA_PROTCL_CCIR656;
			AD_IND("[ad GET outport fmt = %d protocol= %d\r\n", data_fmt->fmt, data_fmt->protocal);
			break;
		case AD_CFGID_VIRTUAL_CH_ID:
			virtual_ch = (AD_VIRTUAL_CHANNEL_INFO *)data;

			if (unlikely(virtual_ch->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_PR2020K_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			mode_info->bus_type = AD_BUS_TYPE_PARALLEL;
			mode_info->mux_num = AD_MUX_NUM_1;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 148500000;
			mode_info->data_rate = 148500000;
			mode_info->protocal = AD_DATA_PROTCL_CCIR656;
			mode_info->yuv_order = AD_DATA_ORDER_YUYV;
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_PR2020K_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_PR2020K_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_pr2020k_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_pr2020k_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_PR2020K_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_PR2020K_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_pr2020k_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id;
	AD_PR2020K_VIN_MODE vin_mode = {0};
	AD_PR2020K_CH_MODE ch_mode = {0};

	ad_info = ad_pr2020k_get_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_PR2020K_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_PR2020K_MODULE_NAME, vin_id, AD_PR2020K_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_PR2020K_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_PR2020K_MODULE_NAME, vout_id, AD_PR2020K_VOUT_MAX);
		return E_SYS;
	}

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	AD_IND("chip%d vin%d: w = %d, h = %d, fps = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps);

	if (unlikely(ad_pr2020k_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	ad_pr2020k_set_video_mode(chip_id, ch_mode.mode);

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;

	return E_OK;
}

static ER ad_pr2020k_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_PR2020K_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_pr2020k_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_PR2020K_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_pr2020k_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_pr2020k_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_pr2020k_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_PR2020K_MODULE_NAME, i_vin);
		return E_SYS;
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height;
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
	AD_IND("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d\r\n", detect_info->vin_id, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog);

	return E_OK;
}

static ER ad_pr2020k_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_pr2020k_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_pr2020k_dbg(char *str_cmd)
{
	ad_drv_dbg(str_cmd);

	return E_OK;
}

AD_DEV* ad_get_obj_pr2020k(void)
{
	return &g_ad_pr2020k_obj;
}

