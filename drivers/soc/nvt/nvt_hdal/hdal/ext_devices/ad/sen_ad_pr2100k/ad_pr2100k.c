#include "ad_pr2100k.h"
#include "nvt_ad_pr2100k_reg.h"

#include "pr2100k.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 00, 001, 00);

// mapping table: vin raw data -> meaningful vin info
/*
static AD_PR2100K_DET_MAP g_ad_pr2100k_det_map[] = {
	{.raw_mode = {.mode = 0}, .vin_mode = {.width = 960, .height =  480, .prog =  FALSE}}, // SD 480i
	{.raw_mode = {.mode = 1}, .vin_mode = {.width = 960, .height =  576, .prog =  FALSE}}, // SD 576i
	{.raw_mode = {.mode = 2}, .vin_mode = {.width = 1280, .height = 720, .prog =  TRUE}}, // HD720P
	{.raw_mode = {.mode = 3}, .vin_mode = {.width = 1920, .height = 1080, .prog =  TRUE}}, // HD1080P
};
*/
// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_PR2100K_VIN_MAP g_ad_pr2100k_vin_map[] = {
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_720P25, .std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_720P30, .std = STD_HDA}},
    {.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_720P50, .std = STD_HDA}},
    {.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_720P60, .std = STD_HDA}},
    {.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_1080P25, .std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = PR2100K_1080P30, .std = STD_HDA}},

    {.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_720P25, .std = STD_HDT}},
    {.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_720P30, .std = STD_HDT}},
    {.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_720P50, .std = STD_HDT}},
    {.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_720P60, .std = STD_HDT}},
    {.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_1080P25, .std = STD_HDT}},
    {.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDT}, .ch_mode = {.mode = PR2100K_1080P30, .std = STD_HDT}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_PR2100K_VOUT_MAP g_ad_pr2100k_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_pr2100k_slv_addr[] = {0xB8, 0xBA, 0xBC, 0xBE};

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_PR2100K_DFT_WIDTH 1920
#define AD_PR2100K_DFT_HEIGHT 1080
#define AD_PR2100K_DFT_FPS 3000
#define AD_PR2100K_DFT_PROG TRUE
#define AD_PR2100K_DFT_STD STD_HDA

// i2c retry number when operation fail occur
#define AD_PR2100K_I2C_RETRY_NUM 5

// global variable
#define AD_PR2100K_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_pr2100k_vin[AD_PR2100K_CHIP_MAX][AD_PR2100K_VIN_MAX];
static AD_VOUT g_ad_pr2100k_vout[AD_PR2100K_CHIP_MAX][AD_PR2100K_VOUT_MAX];
static AD_INFO g_ad_pr2100k_info[AD_PR2100K_CHIP_MAX] = {
	{
		.name = "PR2100K_0",
		.vin_max = AD_PR2100K_VIN_MAX,
		.vin = &g_ad_pr2100k_vin[0][0],
		.vout_max = AD_PR2100K_VOUT_MAX,
		.vout = &g_ad_pr2100k_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "PR2100K_1",
		.vin_max = AD_PR2100K_VIN_MAX,
		.vin = &g_ad_pr2100k_vin[1][0],
		.vout_max = AD_PR2100K_VOUT_MAX,
		.vout = &g_ad_pr2100k_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_PR2100K_INFO g_ad_pr2100k_info_private[AD_PR2100K_CHIP_MAX] = {
	[0 ... (AD_PR2100K_CHIP_MAX-1)] = {
		.is_inited = FALSE,
	}
};

static ER ad_pr2100k_open(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_close(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_init(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_uninit(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_pr2100k_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_pr2100k_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_pr2100k_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_pr2100k_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_pr2100k_dbg(char *str_cmd);

static AD_DEV g_ad_pr2100k_obj = {
	AD_PR2100K_CHIP_MAX,
	g_ad_pr2100k_info,

	sizeof(g_ad_pr2100k_slv_addr) / sizeof(typeof(g_ad_pr2100k_slv_addr[0])),
	g_ad_pr2100k_slv_addr,

	{
		.open = ad_pr2100k_open,
		.close = ad_pr2100k_close,
		.init = ad_pr2100k_init,
		.uninit = ad_pr2100k_uninit,
		.get_cfg = ad_pr2100k_get_cfg,
		.set_cfg = ad_pr2100k_set_cfg,
		.chgmode = ad_pr2100k_chgmode,
		.det_plug_in = ad_pr2100k_watchdog_cb,
		.i2c_write = ad_pr2100k_i2c_write,
		.i2c_read = ad_pr2100k_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_pr2100k_dbg,
	}
};

// internal function
static AD_INFO *ad_pr2100k_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_PR2100K_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_PR2100K_CHIP_MAX);
		return NULL;
	}
	return &g_ad_pr2100k_info[chip_id];
}

static AD_PR2100K_INFO *ad_pr2100k_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_PR2100K_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_PR2100K_CHIP_MAX);
		return NULL;
	}
	return &g_ad_pr2100k_info_private[chip_id];
}

static void ad_pr2100k_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_PR2100K_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_PR2100K_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_PR2100K_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_PR2100K_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_PR2100K_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_PR2100K_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_PR2100K_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_PR2100K_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_PR2100K_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_PR2100K_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_PR2100K_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_PR2100K_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

static ER ad_pr2100k_set_reg_page(UINT32 chip_id, UINT32 page)
{
	ad_pr2100k_i2c_write(chip_id, AD_PR2100K_PAGE_REG_OFS, page, AD_PR2100K_I2C_DATA_LEN);
	return E_OK;
}
/*
static ER ad_pr2100k_get_vloss(UINT32 chip_id, BOOL *vloss)
{
	AD_PR2100K_DETECTION_STATUS_REG det_sts = {0};

	ad_pr2100k_i2c_read(chip_id, AD_PR2100K_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_PR2100K_I2C_DATA_LEN);

	*vloss = (det_sts.bit.VDO_DET != TRUE);

	return E_OK;
}

static ER ad_pr2100k_get_cvstd(UINT32 chip_id, UINT32 *cvstd)
{
	AD_PR2100K_DETECTION_STATUS_REG det_sts = {0};

	ad_pr2100k_i2c_read(chip_id, AD_PR2100K_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_PR2100K_I2C_DATA_LEN);

	*cvstd = det_sts.bit.STD_DET;

	return E_OK;
}
*/
/*
#define AD_PR2100K_UNIT_INTERVAL(rate) ((rate) < 1 ? 1000000000 : (1000000000 / (rate)))
#define AD_PR2100K_REG_DLY(rate) (AD_PR2100K_UNIT_INTERVAL(rate) * 8)
#define AD_PR2100K_REG_DLY_2_TIME_DLY(reg_dly, rate) (((reg_dly) + 1) * AD_PR2100K_REG_DLY(rate))
#define AD_PR2100K_TIME_DLY_2_REG_DLY(time_dly, rate) ((time_dly) < AD_PR2100K_REG_DLY(rate) ? 0 : (((time_dly) / AD_PR2100K_REG_DLY(rate)) - 1))
*/

typedef struct {
	UINT8 addr;
	UINT8 val;
} AD_PR2100K_I2C_REG;

static ER ad_pr2100k_i2c_write_tbl(UINT32 chip_id, const AD_PR2100K_I2C_REG *tbl, UINT32 cnt)
{
	UINT32 i;

	for (i = 0; i < cnt; i++) {

		if (ad_pr2100k_i2c_write(chip_id, tbl[i].addr, tbl[i].val, AD_PR2100K_I2C_DATA_LEN) != E_OK) {
			return E_SYS;
		}
	}

	return E_OK;
}

const AD_PR2100K_I2C_REG ad_pr2100k_vdo_setting[] = {
	{0xFF,0x00},
	{0x10,0x93},	
    //    {0x10,0x82},    
	{0x11,0x0F},	
	{0x12,0x00},	
	{0x13,0x00},	
	{0x14,0x21},	
	{0x15,0x44},	
	{0x16,0x0D},	
	{0x30,0x93},	
    //    {0x30,0x82},    
	{0x31,0x0F},	
	{0x32,0x00},	
	{0x33,0x00},	
	{0x34,0x21},	
	{0x35,0x44},	
	{0x36,0x0D},	
	{0x80,0x80},	
	{0x81,0x0E},	
	{0x82,0x0D},	
	{0x84,0xF0},	
	{0x8A,0x00},	
	{0x90,0x00},	
	{0x91,0x00},	
	{0x94,0xFF},	
	{0x95,0xFF},	
	{0xA0,0x33},	
	{0xB0,0x33},	
	{0xC0,0x00},	
	{0xC1,0x00},	
	{0xC2,0x00},	
	{0xC3,0x00},	
	{0xC4,0x00},	
	{0xC5,0x00},	
	{0xC6,0x00},	
	{0xC7,0x00},	
	{0xC8,0x00},	
	{0xC9,0x00},	
	{0xCA,0x00},	
	{0xCB,0x00},	
	{0xD0,0x06},	
	{0xD1,0x23},	
	{0xD2,0x21},	
	{0xD3,0x44},	
	{0xD4,0x06},	
	{0xD5,0x23},	
	{0xD6,0x21},	
	{0xD7,0x44},	
	{0xD8,0x06},	
	{0xD9,0x22},	
	{0xDA,0x2C},	
	{0xE0,0x05},	
	{0xE1,0x05},	
	{0xE2,0x00},	
	{0xE3,0x04},	
	{0xE4,0x20},	
	{0xE5,0x64},	
	{0xE6,0x20},	
	{0xE7,0x64},	
	{0xE8,0x40},	
	{0xE9,0x40},	
	{0xEA,0x00},	
	{0xEB,0x01},	
	{0xF0,0x03},	
	{0xF1,0xFF},	
	{0xF3,0x06},	
	{0xF4,0x66},	

};

const AD_PR2100K_I2C_REG ad_pr2100k_mode_1080p30[] = {
	{0x00,0xE5},	
	{0x01,0x61},	
	{0x02,0x00},	
	{0x03,0x56},	
	{0x04,0x0C},	
	{0x05,0x88},	
	{0x06,0x04},	
	{0x07,0xB2},	
	{0x08,0x44},	
	{0x09,0x34},	
	{0x0A,0x02},	
	{0x0B,0x14},	
	{0x0C,0x04},	
	{0x0D,0x08},	
	{0x0E,0x5E},	
	{0x0F,0x5E},	
	{0x10,0x26},	
	{0x11,0x00},	
	{0x12,0x87},	
	{0x13,0x24},	
	{0x14,0x80},	
	{0x15,0x2A},	
	{0x16,0x38},	
	{0x17,0x00},	
	{0x18,0x80},	
	{0x19,0x48},	
	{0x1A,0x6C},	
	{0x1B,0x05},	
	{0x1C,0x61},	
	{0x1D,0x07},	
	{0x1E,0x7E},	
	{0x1F,0x80},	
	{0x20,0x80},	
	{0x21,0x80},	
	{0x22,0x90},	
	{0x23,0x80},	
	{0x24,0x80},	
	{0x25,0x80},	
	{0x26,0x84},	
	{0x27,0x82},	
	{0x28,0x00},	
	{0x29,0xFF},	
	{0x2A,0xFF},	
	{0x2B,0x00},	
	{0x2C,0x00},	
	{0x2D,0x00},	
	{0x2E,0x00},	
	{0x2F,0x00},	
	{0x30,0x00},	
	{0x31,0x00},	
	{0x32,0xC0},	
	{0x33,0x14},	
	{0x34,0x14},	
	{0x35,0x80},	
	{0x36,0x80},	
	{0x37,0xAD},	
	{0x38,0x4B},	
	{0x39,0x08},	
	{0x3A,0x21},	
	{0x3B,0x02},	
	{0x3C,0x01},	
	{0x3D,0x23},	
	{0x3E,0x05},	
	{0x3F,0xC8},	
	{0x40,0x05},	
	{0x41,0x55},	
	{0x42,0x01},	
	{0x43,0x38},	
	{0x44,0x6A},	
	{0x45,0x00},	
	{0x46,0x14},	
	{0x47,0xB0},	
	{0x48,0xDF},	
	{0x49,0x00},	
	{0x4A,0x7B},	
	{0x4B,0x60},	
	{0x4C,0x00},	
	{0x4D,0x26},	
	{0x4E,0x00},	
	{0x4F,0x0C},	
	{0x50,0x21},	
	{0x51,0x28},	
	{0x52,0x40},	
	{0x53,0x0C},	
	{0x54,0x0F},	
	{0x55,0x8D},	
	{0x70,0x06},	
	{0x71,0x08},	
	{0x72,0x0A},	
	{0x73,0x0C},	
	{0x74,0x0E},	
	{0x75,0x10},	
	{0x76,0x12},	
	{0x77,0x14},	
	{0x78,0x06},	
	{0x79,0x08},	
	{0x7A,0x0A},	
	{0x7B,0x0C},	
	{0x7C,0x0E},	
	{0x7D,0x10},	
	{0x7E,0x12},	
	{0x7F,0x14},	
	{0x80,0x00},	
	{0x81,0x09},	
	{0x82,0x00},	
	{0x83,0x07},	
	{0x84,0x00},	
	{0x85,0x17},	
	{0x86,0x03},	
	{0x87,0xE5},	
	{0x88,0x05},	
	{0x89,0x24},	
	{0x8A,0x05},	
	{0x8B,0x24},	
	{0x8C,0x08},	
	{0x8D,0xE8},	
	{0x8E,0x05},	
	{0x8F,0x47},	
	{0x90,0x02},	
	{0x91,0xB4},	
	{0x92,0x73},	
	{0x93,0xE8},	
	{0x94,0x0F},	
	{0x95,0x5E},	
	{0x96,0x03},	
	{0x97,0xD0},	
	{0x98,0x17},	
	{0x99,0x34},	
	{0x9A,0x13},	
	{0x9B,0x56},	
	{0x9C,0x0B},	
	{0x9D,0x9A},	
	{0x9E,0x09},	
	{0x9F,0xAB},	
	{0xA0,0x01},	
	{0xA1,0x74},	
	{0xA2,0x01},	
	{0xA3,0x6B},	
	{0xA4,0x00},	
	{0xA5,0xBA},	
	{0xA6,0x00},	
	{0xA7,0xA3},	
	{0xA8,0x01},	
	{0xA9,0x39},	
	{0xAA,0x01},	
	{0xAB,0x39},	
	{0xAC,0x00},	
	{0xAD,0xC1},	
	{0xAE,0x00},	
	{0xAF,0xC1},	
	{0xB0,0x05},	
	{0xB1,0xCC},	
	{0xB2,0x09},	
	{0xB3,0x6D},	
	{0xB4,0x00},	
	{0xB5,0x17},	
	{0xB6,0x08},	
	{0xB7,0xE8},	
	{0xB8,0xB0},	
	{0xB9,0xCE},	
	{0xBA,0x90},	
	{0xBB,0x00},	
	{0xBC,0x00},	
	{0xBD,0x04},	
	{0xBE,0x07},	
	{0xBF,0x80},	
	{0xC0,0x00},	
	{0xC1,0x00},	
	{0xC2,0x04},	
	{0xC3,0x39},	
	{0xC9,0x00},	
	{0xCA,0x02},	
	{0xCB,0x07},	
	{0xCC,0x80},	
	{0xCD,0x08},	
	{0xCE,0x20},	
	{0xCF,0x04},	
	{0xD0,0x38},	
	{0xD1,0x00},	
	{0xD2,0x00},	
	{0xD3,0x00},	
};

const AD_PR2100K_I2C_REG ad_pr2100k_mipi_log[] = {
	{0xFF,0x05},
	{0x09,0x00},	
	{0x0A,0x03},	
	{0x0E,0x80},	
	{0x0F,0x10},	
	{0x11,0x90},	
	{0x12,0x6E},	
	{0x13,0x00},	
	{0x14,0x6E},	
	{0x15,0x00},	
	{0x16,0x00},	
	{0x17,0x00},	
	{0x18,0x00},	
	{0x19,0x00},	
	{0x1A,0x00},	
	{0x1B,0x00},	
	{0x1C,0x00},	
	{0x1D,0x00},	
	{0x1E,0x00},	
	{0x20,0x88},	
	{0x21,0x07},	
	{0x22,0x80},	
	{0x23,0x04},	
	{0x24,0x38},	
	{0x25,0x0F},	
	{0x26,0x00},	
	{0x27,0x0F},	
	{0x28,0x00},	
	{0x29,0x0B},	
	{0x2A,0x40},	
	{0x30,0x98},	
	{0x31,0x07},	
	{0x32,0x80},	
	{0x33,0x04},	
	{0x34,0x38},	
	{0x35,0x0F},	
	{0x36,0x00},	
	{0x37,0x0F},	
	{0x38,0x00},	
	{0x39,0x07},	
	{0x3A,0x80},	
	{0x40,0x28},	
	{0x41,0x07},	
	{0x42,0x80},	
	{0x43,0x04},	
	{0x44,0x38},	
	{0x45,0x0F},	
	{0x46,0x00},	
	{0x47,0x0F},	
	{0x48,0x00},	
	{0x49,0x03},	
	{0x4A,0xC0},	
	{0x50,0x38},	
	{0x51,0x07},	
	{0x52,0x80},	
	{0x53,0x04},	
	{0x54,0x38},	
	{0x55,0x0F},	
	{0x56,0x00},	
	{0x57,0x0F},	
	{0x58,0x00},	
	{0x59,0x00},	
	{0x5A,0x00},	
	{0x60,0x05},	
	{0x61,0x28},	
	{0x62,0x05},	
	{0x63,0x28},	
	{0x64,0x05},	
	{0x65,0x28},	
	{0x66,0x05},	
	{0x67,0x28},	
	{0x68,0xFF},	
	{0x69,0xFF},	
	{0x6A,0xFF},	
	{0x6B,0xFF},	
	{0x6C,0xFF},	
	{0x6D,0xFF},	
	{0x6E,0xFF},	
	{0x6F,0xFF},	
	{0x10,0xB1},	
};

const AD_PR2100K_I2C_REG ad_pr2100k_mipi_phy[] = {
	{0xFF,0x06},
	{0x04,0x10},	
	{0x05,0x04},	
	{0x06,0x00},	
	{0x07,0xAA},	
	{0x08,0xC5},	
	{0x1c,0x09},	
	{0x1d,0x08},	
	{0x1e,0x09},	
	{0x1f,0x11},	
	{0x20,0x0c},	
	{0x21,0x28},	
	{0x22,0x0b},	
	{0x23,0x01},	
	{0x24,0x12},	
	{0x25,0x82},	
	{0x26,0x11},	
	{0x27,0x11},	
	{0x36,0x0F},	
	{0x37,0x00},	
	{0x38,0x0F},	
	{0x39,0x00},	
	{0x3A,0x0F},	
	{0x3B,0x00},	
	{0x3C,0x0F},	
	{0x3D,0x00},	
	{0x46,0x1E},	
	{0x47,0x5E},	
	{0x48,0x9E},	
	{0x49,0xDE},	
 	{0x04,0x50},	
};

static ER ad_pr2100k_set_video_init(UINT32 chip_id)
{
	ad_pr2100k_i2c_write_tbl(chip_id, ad_pr2100k_vdo_setting, sizeof(ad_pr2100k_vdo_setting)/sizeof(typeof(ad_pr2100k_vdo_setting[0])));

	return E_OK;
}


static ER ad_pr2100k_set_video_mode(UINT32 chip_id, UINT32 ch, UINT32 fmt, UINT32 std, BOOL init_tbl)
{
    UINT32 page, pg0_oft, pg5_oft, pg1_2_Hsize, pg1_2_Vsize, pg5_Hsize, pg5_Vsize;
    AD_PR2100K_SET_INPUT_REG pg0_data;

    switch (ch) {
        default:
        case 0:
            page = CH_0_PAGE;
            pg0_oft = 0;
            pg5_oft = 0;
            break;
        case 1:
            page = CH_1_PAGE;
            pg0_oft = 0x20;
            pg5_oft = 0x10;
            break;
    }
	AD_IND("SET VIDEO MODE chip%u ch%u fmt%u(0x%x) std%u\r\n", chip_id, ch, fmt, fmt, std);

	ad_pr2100k_set_reg_page(chip_id, page);
    if (init_tbl) {
        ad_pr2100k_i2c_write_tbl(chip_id, ad_pr2100k_mode_1080p30, sizeof(ad_pr2100k_mode_1080p30)/sizeof(typeof(ad_pr2100k_mode_1080p30[0])));
    } else {
    	switch (fmt) {
        	case PR2100K_720P25:
                pg0_data.bit.RES_SET = 2; // 2:720p, 3:1080p
                pg0_data.bit.RATE_SET = 0; // 0:25Hz, 1:30Hz, 2:50Hz, 3:60Hz
                pg1_2_Hsize = 1280;
                pg1_2_Vsize = 720;
                pg5_Hsize = 1280;
                pg5_Vsize = 720;
        		break;
        	case PR2100K_720P30:
                pg0_data.bit.RES_SET = 2;
                pg0_data.bit.RATE_SET = 1;
                pg1_2_Hsize = 1280;
                pg1_2_Vsize = 720;
                pg5_Hsize = 1280;
                pg5_Vsize = 720;
        		break;
        	case PR2100K_720P50:
                pg0_data.bit.RES_SET = 2;
                pg0_data.bit.RATE_SET = 2;
                pg1_2_Hsize = 1280;
                pg1_2_Vsize = 720;
                pg5_Hsize = 1280;
                pg5_Vsize = 720;
        		break;
        	case PR2100K_720P60:
                pg0_data.bit.RES_SET = 2;
                pg0_data.bit.RATE_SET = 3;
                pg1_2_Hsize = 1280;
                pg1_2_Vsize = 720;
                pg5_Hsize = 1280;
                pg5_Vsize = 720;
        		break;
        	case PR2100K_1080P25:
                pg0_data.bit.RES_SET = 3;
                pg0_data.bit.RATE_SET = 0;
                pg1_2_Hsize = 1920;
                pg1_2_Vsize = 1080;
                pg5_Hsize = 1920;
                pg5_Vsize = 1080;
        		break;
        	case PR2100K_1080P30:
                pg0_data.bit.RES_SET = 3;
                pg0_data.bit.RATE_SET = 1;
                pg1_2_Hsize = 1920;
                pg1_2_Vsize = 1080;
                pg5_Hsize = 1920;
                pg5_Vsize = 1080;
        		break;
        	default:
        		AD_ERR("[%s] Unknown video fmt %d\r\n", AD_PR2100K_MODULE_NAME, fmt);
        		return E_SYS;
    	}
        pg0_data.bit.STD_SET = std; // 2:720p, 3:1080p
        ad_pr2100k_i2c_write(chip_id, AD_PR2100K_PAGE_REG_OFS, DATA_PAGE, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, AD_PR2100K_SET_INPUT_REG_OFS + pg0_oft, pg0_data.reg, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, AD_PR2100K_PAGE_REG_OFS, page, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0xBE, (pg1_2_Hsize >> 8) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0xBF, (pg1_2_Hsize) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0xC2, (pg1_2_Vsize >> 8) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0xC3, (pg1_2_Vsize) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, AD_PR2100K_PAGE_REG_OFS, MIPI_LOG_PAGE, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0x21 + pg5_oft, (pg5_Hsize >> 8) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0x22 + pg5_oft, (pg5_Hsize) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0x23 + pg5_oft, (pg5_Vsize >> 8) & 0xff, AD_PR2100K_I2C_DATA_LEN);
        ad_pr2100k_i2c_write(chip_id, 0x24 + pg5_oft, (pg5_Vsize) & 0xff, AD_PR2100K_I2C_DATA_LEN);
    }
	return E_OK;
}

static ER ad_pr2100k_set_mipi_init(UINT32 chip_id)
{
	ad_pr2100k_i2c_write_tbl(chip_id, ad_pr2100k_mipi_log, sizeof(ad_pr2100k_mipi_log)/sizeof(typeof(ad_pr2100k_mipi_log[0])));
	ad_pr2100k_i2c_write_tbl(chip_id, ad_pr2100k_mipi_phy, sizeof(ad_pr2100k_mipi_phy)/sizeof(typeof(ad_pr2100k_mipi_phy[0])));

	return E_OK;
}

// implementation of video mode converting
/*
static ER ad_pr2100k_det_mode_to_vin_mode(AD_PR2100K_DET_RAW_MODE raw_mode, AD_PR2100K_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_pr2100k_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_pr2100k_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_pr2100k_det_map[i_mode].raw_mode, sizeof(AD_PR2100K_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_pr2100k_det_map[i_mode].vin_mode, sizeof(AD_PR2100K_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u, eq %u\r\n", AD_PR2100K_MODULE_NAME, raw_mode.mode, raw_mode.eq);
	return E_SYS;
}
*/
static ER ad_pr2100k_vin_mode_to_ch_mode(AD_PR2100K_VIN_MODE vin_mode, AD_PR2100K_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_pr2100k_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_pr2100k_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_pr2100k_vin_map[i_mode].vin_mode, sizeof(AD_PR2100K_VIN_MODE)) == 0) {
			*ch_mode = g_ad_pr2100k_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_PR2100K_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}

// implementation of video mode detection

static ER ad_pr2100k_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_PR2100K_DET_VIDEO_INFO *video_info)
{
    UINT32 ch_offset;
    AD_PR2100K_DETECTION_STATUS_REG reg_val;

    if (vin_id > 1) {
        AD_ERR("vin_id = %d > max 1\r\n", vin_id);
    }
    ch_offset = CH2_OFST * vin_id;

    if (ad_pr2100k_i2c_write(chip_id, AD_PR2100K_PAGE_REG_OFS, DATA_PAGE, AD_PR2100K_I2C_DATA_LEN) != E_OK) {
        return E_SYS;
    }


    if (ad_pr2100k_i2c_read(chip_id, AD_PR2100K_DETECTION_STATUS_REG_OFS + ch_offset, &reg_val.reg, AD_PR2100K_I2C_DATA_LEN) != E_OK) {
        return E_SYS;
    }

    video_info->vloss = reg_val.bit.VDO_DET ? FALSE : TRUE;
    switch (reg_val.bit.RES_DET) {
        case 0: //480i
            video_info->width = 960;
            video_info->height = 480;
            video_info->prog = FALSE;
            break;
        case 1: //576i
            video_info->width = 960;
            video_info->height = 576;
            video_info->prog = FALSE;
            break;
        case 2: //720p
            video_info->width = 1280;
            video_info->height = 720;
            video_info->prog = TRUE;
            break;
        case 3: //720p
            video_info->width = 1920;
            video_info->height = 1080;
            video_info->prog = TRUE;
            break;
    }
    
    switch (reg_val.bit.RATE_DET) {
        case 0: //480i
            video_info->fps = 2500;
            break;
        case 1: //576i
            video_info->fps = 3000;
            break;
        case 2: //720p
            video_info->fps = 5000;
            break;
        case 3: //720p
            video_info->fps = 6000;
            break;
    }

    video_info->std = reg_val.bit.STD_DET;
    return E_OK;
}


#if 0
#endif

static ER ad_pr2100k_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};

	ad_info = ad_pr2100k_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	ad_pr2100k_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_PR2100K_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_PR2100K_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_PR2100K_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_PR2100K_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_PR2100K_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	drv_open_info.dbg_bank_reg_dump_info = NULL;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_pr2100k_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_pr2100k_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_pr2100k_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_pr2100k_init(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_PR2100K_INFO *ad_private_info;
	AD_PR2100K_VIN_MODE vin_mode = {0};
	AD_PR2100K_CH_MODE ch_mode = {0};

	ad_info = ad_pr2100k_get_info(chip_id);
	ad_private_info = ad_pr2100k_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	//ad_pr2100k_set_reg_page(chip_id, CH_ALL);

	// set video mode
	vin_mode.width = AD_PR2100K_DFT_WIDTH;
	vin_mode.height = AD_PR2100K_DFT_HEIGHT;
	vin_mode.fps = AD_PR2100K_DFT_FPS;
	vin_mode.prog = AD_PR2100K_DFT_PROG;
	vin_mode.std = AD_PR2100K_DFT_STD;
	if (ad_pr2100k_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
		AD_WRN("[%s] ad init wrn. get ch mode fail\r\n", AD_PR2100K_MODULE_NAME);
	}

    ad_pr2100k_set_video_init(chip_id);
	ad_pr2100k_set_video_mode(chip_id, CH_0_PAGE, ch_mode.mode, ch_mode.std, TRUE);
	ad_pr2100k_set_video_mode(chip_id, CH_1_PAGE, ch_mode.mode, ch_mode.std, TRUE);
	//ad_pr2100k_set_pattern_gen_mode(chip_id, CH_ALL, AD_PR2100K_PATTERN_GEN_MODE_AUTO);
    ad_pr2100k_set_mipi_init(chip_id);

	// set mipi mode
	//ad_pr2100k_set_mipi_clk_lane_stop(chip_id, 0, TRUE);
	//ad_pr2100k_set_mipi_output(chip_id, MIPI_4CH4LANE_594M);
	//ad_pr2100k_set_mipi_clk_lane_stop(chip_id, 0, FALSE);
    ad_private_info->det_vin_mode[0] = vin_mode;
    ad_private_info->det_vin_mode[1] = vin_mode;
	ad_private_info->is_inited = TRUE;
    //AD_ERR("ad std = %d\r\n", ad_private_info->det_vin_mode[0].std);

done:
	g_ad_pr2100k_info[chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_pr2100k_uninit(UINT32 chip_id, void *ext_data)
{
	AD_PR2100K_INFO *ad_private_info;

	ad_private_info = ad_pr2100k_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	ad_private_info->is_inited = FALSE;
	g_ad_pr2100k_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_pr2100k_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_pr2100k_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
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
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_PR2100K_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			mode_info->bus_type = AD_BUS_TYPE_SERIAL;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 74250000;
			mode_info->data_rate = 297000000;
			mode_info->mipi_lane_num = AD_MIPI_LANE_NUM_2;
			mode_info->protocal = AD_DATA_PROTCL_CCIR601;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY;
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_PR2100K_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_PR2100K_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_pr2100k_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_PR2100K_INFO *ad_private_info;
	ER rt = E_OK;

	ad_info = ad_pr2100k_get_info(chip_id);
	ad_private_info = ad_pr2100k_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_PR2100K_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_PR2100K_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_pr2100k_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_PR2100K_INFO *ad_private_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id;
	AD_PR2100K_VIN_MODE vin_mode = {0};
	AD_PR2100K_CH_MODE ch_mode = {0};
    //AD_DECET_INFO det_info;

	ad_info = ad_pr2100k_get_info(chip_id);
	ad_private_info = ad_pr2100k_get_private_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_PR2100K_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_PR2100K_MODULE_NAME, vin_id, AD_PR2100K_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_PR2100K_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_PR2100K_MODULE_NAME, vout_id, AD_PR2100K_VOUT_MAX);
		return E_SYS;
	}

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	vin_mode.std = ad_private_info->det_vin_mode[vin_id].std;

#if 0 // for detection test
    det_info.vin_id = vin_id;
    ad_pr2100k_watchdog_cb(chip_id, &det_info);
    vin_mode.width = det_info.det_info.width;
    vin_mode.height = det_info.det_info.height;
    vin_mode.fps = det_info.det_info.fps;
    vin_mode.prog = det_info.det_info.prog;
	vin_mode.std = ad_private_info->det_vin_mode[vin_id].std;
#endif

    
	AD_IND("chip%d vin%d: w = %d, h = %d, fps = %d, std = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.std);

	if (unlikely(ad_pr2100k_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	ad_pr2100k_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std, FALSE);
	//ad_pr2100k_set_csi_out_dly(chip_id);

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;
	ad_private_info->cur_vin_mode[vin_id].std = vin_mode.std;

	return E_OK;
}

static ER ad_pr2100k_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_PR2100K_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	AD_PR2100K_INFO *ad_private_info;
	UINT32 i_vin;

	ad_info = ad_pr2100k_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;
	ad_private_info = ad_pr2100k_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_PR2100K_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_pr2100k_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_pr2100k_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_pr2100k_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_PR2100K_MODULE_NAME, i_vin);
		return E_SYS;
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height >> (video_info.prog ? 0 : 1);
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
    ad_private_info->det_vin_mode[i_vin].std = video_info.std;
    
	AD_IND("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d std=%d\r\n", i_vin, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog, video_info.std);

	return E_OK;
}

static ER ad_pr2100k_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_pr2100k_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_pr2100k_dbg(char *str_cmd)
{
	return E_OK;
}

AD_DEV* ad_get_obj_pr2100k(void)
{
	return &g_ad_pr2100k_obj;
}

