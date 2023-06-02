#include "ad_nvp6188.h"
#include "nvt_ad_nvp6188_reg.h"

#include "raptor5_drv.c"

// AD driver version
AD_DRV_MODULE_VERSION(0, 00, 006, 00);

// mapping table: vin raw data -> meaningful vin info
static AD_NVP6188_DET_MAP g_ad_nvp6188_det_map[] = {
	//			  mode				  width 	height	fps		prog	std
	{.raw_mode = {0x00}, .vin_mode = {1920,		480, 	6000, 	FALSE, 	AD_NVP6188_STD_AHD}}, // SD_H960_2EX_Btype_NT
	{.raw_mode = {0x10}, .vin_mode = {1920,		576, 	5000, 	FALSE, 	AD_NVP6188_STD_AHD}}, // SD_H960_2EX_Btype_PAL
	{.raw_mode = {0x20}, .vin_mode = {1280, 	720, 	3000, 	TRUE, 	AD_NVP6188_STD_AHD}}, // AHD_720P_30P_EX_Btype
	{.raw_mode = {0x21}, .vin_mode = {1280, 	720, 	2500, 	TRUE, 	AD_NVP6188_STD_AHD}}, // AHD_720P_25P_EX_Btype
	{.raw_mode = {0x30}, .vin_mode = {1920, 	1080, 	3000, 	TRUE, 	AD_NVP6188_STD_AHD}}, // AHD_1080P_30P
	{.raw_mode = {0x31}, .vin_mode = {1920, 	1080, 	2500, 	TRUE, 	AD_NVP6188_STD_AHD}}, // AHD_1080P_25P
};

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_NVP6188_VIN_MAP g_ad_nvp6188_vin_map[] = {
	//			  width 	height 	fps 	prog 	std 							 std 		resolution 			fps
	{.vin_mode = {960, 		480, 	6000, 	FALSE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_SD, 	FMT_H960_2EX, 		FMT_NT}},
	{.vin_mode = {960, 		576, 	5000, 	FALSE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_SD, 	FMT_H960_2EX, 		FMT_PAL}},
	{.vin_mode = {1920,		480, 	6000, 	FALSE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_SD, 	FMT_H960_Btype_2EX, FMT_NT}},
	{.vin_mode = {1920,		576, 	5000, 	FALSE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_SD, 	FMT_H960_Btype_2EX, FMT_PAL}},
	{.vin_mode = {1280, 	720, 	3000, 	TRUE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_AHD, 	FMT_720P_Btype_EX, 	FMT_30P}},
	{.vin_mode = {1280, 	720, 	2500, 	TRUE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_AHD, 	FMT_720P_Btype_EX, 	FMT_25P}},
	{.vin_mode = {1920, 	1080, 	3000, 	TRUE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_AHD, 	FMT_1080P, 			FMT_30P}},
	{.vin_mode = {1920, 	1080, 	2500, 	TRUE, 	AD_NVP6188_STD_AHD}, .ch_mode = {FMT_AHD, 	FMT_1080P, 			FMT_25P}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_NVP6188_VOUT_MAP g_ad_nvp6188_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_nvp6188_slv_addr[] = {0x60, 0x62, 0x64, 0x66};

// decoder dump all register information
#define AD_NVP6188_BANK_ADDR 0xFF
static AD_DRV_DBG_REG_BANK g_ad_nvp6188_bank[] = {
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
	{.id = 0x13,.len = 0x100},
	{.id = 0x20,.len = 0x100},
	{.id = 0x23,.len = 0x100},
};
#define AD_NVP6188_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_NVP6188_DFT_WIDTH 1920
#define AD_NVP6188_DFT_HEIGHT 1080
#define AD_NVP6188_DFT_FPS 2500
#define AD_NVP6188_DFT_PROG 1
#define AD_NVP6188_DFT_STD AD_NVP6188_STD_AHD

// i2c retry number when operation fail occur
#define AD_NVP6188_I2C_RETRY_NUM 5

// global variable
#define AD_NVP6188_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_nvp6188_vin[AD_NVP6188_CHIP_MAX][AD_NVP6188_VIN_MAX];
static AD_VOUT g_ad_nvp6188_vout[AD_NVP6188_CHIP_MAX][AD_NVP6188_VOUT_MAX];
static AD_INFO g_ad_nvp6188_info[AD_NVP6188_CHIP_MAX] = {
	{
		.name = "NVP6188_0",
		.vin_max = AD_NVP6188_VIN_MAX,
		.vin = &g_ad_nvp6188_vin[0][0],
		.vout_max = AD_NVP6188_VOUT_MAX,
		.vout = &g_ad_nvp6188_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6188_1",
		.vin_max = AD_NVP6188_VIN_MAX,
		.vin = &g_ad_nvp6188_vin[1][0],
		.vout_max = AD_NVP6188_VOUT_MAX,
		.vout = &g_ad_nvp6188_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6188_2",
		.vin_max = AD_NVP6188_VIN_MAX,
		.vin = &g_ad_nvp6188_vin[2][0],
		.vout_max = AD_NVP6188_VOUT_MAX,
		.vout = &g_ad_nvp6188_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "NVP6188_3",
		.vin_max = AD_NVP6188_VIN_MAX,
		.vin = &g_ad_nvp6188_vin[3][0],
		.vout_max = AD_NVP6188_VOUT_MAX,
		.vout = &g_ad_nvp6188_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_NVP6188_INFO g_ad_nvp6188_info_private[AD_NVP6188_CHIP_MAX] = {
	[0 ... (AD_NVP6188_CHIP_MAX-1)] = {
		.is_inited = FALSE,
	}
};

static ER ad_nvp6188_open(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_close(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_init(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_uninit(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_nvp6188_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_nvp6188_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_nvp6188_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_nvp6188_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_nvp6188_dbg(char *str_cmd);

static AD_DEV g_ad_nvp6188_obj = {
	AD_NVP6188_CHIP_MAX,
	g_ad_nvp6188_info,

	sizeof(g_ad_nvp6188_slv_addr) / sizeof(typeof(g_ad_nvp6188_slv_addr[0])),
	g_ad_nvp6188_slv_addr,

	{
		.open = ad_nvp6188_open,
		.close = ad_nvp6188_close,
		.init = ad_nvp6188_init,
		.uninit = ad_nvp6188_uninit,
		.get_cfg = ad_nvp6188_get_cfg,
		.set_cfg = ad_nvp6188_set_cfg,
		.chgmode = ad_nvp6188_chgmode,
		.det_plug_in = ad_nvp6188_watchdog_cb,
		.i2c_write = ad_nvp6188_i2c_write,
		.i2c_read = ad_nvp6188_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_nvp6188_dbg,
	}
};

// internal function
static AD_INFO *ad_nvp6188_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_NVP6188_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_NVP6188_CHIP_MAX);
		return NULL;
	}
	return &g_ad_nvp6188_info[chip_id];
}

static AD_NVP6188_INFO *ad_nvp6188_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_NVP6188_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_NVP6188_CHIP_MAX);
		return NULL;
	}
	return &g_ad_nvp6188_info_private[chip_id];
}

static void ad_nvp6188_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_NVP6188_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_NVP6188_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_NVP6188_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_NVP6188_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_NVP6188_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_NVP6188_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_NVP6188_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_NVP6188_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_NVP6188_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_NVP6188_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_NVP6188_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_NVP6188_DFT_PROG;
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

	for (chip_id = 0; chip_id < AD_NVP6188_CHIP_MAX; chip_id++) {

		if (g_ad_nvp6188_info[chip_id].i2c_addr == chip_addr) {

			ad_nvp6188_i2c_write(chip_id, reg_addr, value, AD_NVP6188_I2C_DATA_LEN);
			return;
		}
	}
}

// external i2c read register
unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr)
{
	UINT32 chip_id, value = 0;

	for (chip_id = 0; chip_id < AD_NVP6188_CHIP_MAX; chip_id++) {

		if (g_ad_nvp6188_info[chip_id].i2c_addr == chip_addr) {

			ad_nvp6188_i2c_read(chip_id, reg_addr, &value, AD_NVP6188_I2C_DATA_LEN);
			return (value & 0xff);
		}
	}
	return 0;
}

// external mipi clock string
// rewrite - char *nc_drv_common_mipi_clock_str_get(NC_MIPI_CLK_E clk)
static char *ad_nvp6188_nc_drv_common_mipi_clock_str_get(AD_NVP6188_NC_MIPI_BPS_E bps)
{
	if(bps == AD_NVP6188_NC_MIPI_BPS_594Mbps) 			return "NC_MIPI_BPS_594Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_756Mbps)		return "NC_MIPI_BPS_756Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_972Mbps) 		return "NC_MIPI_BPS_972Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_1242Mbps) 	return "NC_MIPI_BPS_1242Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_1458Mbps) 	return "NC_MIPI_BPS_1458Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_297Mbps) 		return "NC_MIPI_BPS_297Mbps";
	else if(bps == AD_NVP6188_NC_MIPI_BPS_1188Mbps) 	return "NC_MIPI_BPS_1188Mbps";
	else												return "Unknown CLOCK";

	return "Unknown ID";
}

// external mipi dphy initialization
// rewrite - void nc_drv_mipi_dphy_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, NC_MIPI_LANE_E lane, NC_MIPI_CLK_E clk, NC_MIPI_SWAP_E swap)
static void ad_nvp6188_nc_drv_mipi_dphy_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, NC_MIPI_LANE_E lane, AD_NVP6188_NC_MIPI_BPS_E bps, NC_MIPI_SWAP_E swap)
{
	int phy = 2;
	int jj  = 0;
	int ii 	= 0;

	NC_U8 lane_hs_tlpx_time = 0;
    NC_U8 lane_prepare_time = 0;
    NC_U8 lane_zero_time = 0;
    NC_U8 lane_trail_time = 0;

	if(chipid != NC_DECODER_6188 && chipid != NC_DECODER_6188C)
	{
		printk("[%s::%d]Unknown chip id!!!(%x)\n", __func__, __LINE__, chipid);
		return;
	}
	else
	{
		if(chipid == NC_DECODER_6188)
			phy = 2;
		else if(chipid == NC_DECODER_6188C)
			phy = 1;
		else
			return;
	}

	for(jj=0; jj<phy; jj++)
	{
		printk("[%s::%d]%x : dev(%d) lane(%s) bps(%s)\n", __func__, __LINE__, BANK_23+(0x10*jj), dev, nc_drv_common_mipi_lane_str_get(lane), ad_nvp6188_nc_drv_common_mipi_clock_str_get(bps));

		/* LANE & PLL SET */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_23+(0x10*jj));

		if(lane == NC_MIPI_LANE_4)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x03);
		}
		else if(lane == NC_MIPI_LANE_2)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x01);
		}
		else if(lane == NC_MIPI_LANE_1)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x00);
		}
		else
		{
			printk("[%s::%d]mipi lane cfg Error!! %d\n", __FILE__, __LINE__, lane);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x03);
		}

		/* Common Reset */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0xe4);	// PLL Enable & Analog reset on
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x7d);	// All lanes Enable on Analog part
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0xe0);	// Analog reset off
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20, 0x1e);	// Digital reset on
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20, 0x1f);	// Digital reset off
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xeb, 0x8d); // Short packet enable

		/* Clock & Data Lane speed configuration */
		if(bps == AD_NVP6188_NC_MIPI_BPS_594Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x2c);
        }
        else if(bps == AD_NVP6188_NC_MIPI_BPS_756Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x38);
        }
        else if(bps == AD_NVP6188_NC_MIPI_BPS_972Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x48);
		}
        else if(bps == AD_NVP6188_NC_MIPI_BPS_1242Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x5C);
		}
        else if(bps == AD_NVP6188_NC_MIPI_BPS_1458Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x6C);
		}
		else if(bps == AD_NVP6188_NC_MIPI_BPS_297Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x16);
		}
		else if(bps == AD_NVP6188_NC_MIPI_BPS_1188Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x58);
		}
		else
		{
            printk("[%s::%d]mipi clock cfg Error!! %d\n", __FILE__, __LINE__, bps);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x70);
		}

		/* Clock & Data Lane Setting */
        if(bps == AD_NVP6188_NC_MIPI_BPS_594Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

            lane_hs_tlpx_time = 0xc5;
            lane_prepare_time = 0x08;
            lane_zero_time    = 0x09;
            lane_trail_time   = 0x30;
        }
        else if(bps == AD_NVP6188_NC_MIPI_BPS_756Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

            lane_hs_tlpx_time = 0xc5;
            lane_prepare_time = 0x08;
            lane_zero_time    = 0x09;
            lane_trail_time   = 0x30;
	    }
	    else if(bps == AD_NVP6188_NC_MIPI_BPS_972Mbps)
		{
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

           lane_hs_tlpx_time = 0xc5;
           lane_prepare_time = 0x08;
           lane_zero_time    = 0x09;
           lane_trail_time   = 0x30;
	    }
	    else if(bps == AD_NVP6188_NC_MIPI_BPS_1242Mbps)
	    {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc9);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x03);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x32);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xc9;
            lane_prepare_time = 0x03;
            lane_zero_time    = 0x14;
            lane_trail_time   = 0x0f;
	    }
	    else if(bps == AD_NVP6188_NC_MIPI_BPS_1458Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xcd);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x42);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x36);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xcd;
            lane_prepare_time = 0x42;
            lane_zero_time    = 0x0e;
            lane_trail_time   = 0x0f;
		}
		else if(bps == AD_NVP6188_NC_MIPI_BPS_297Mbps)
	    {
	        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
	        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
	        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
	        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

	        lane_hs_tlpx_time = 0xc5;
	        lane_prepare_time = 0x7f;
	        lane_zero_time    = 0x10;
	        lane_trail_time   = 0x30;
		}
		else if(bps == AD_NVP6188_NC_MIPI_BPS_1188Mbps)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc9);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x03);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x32);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xc9;
            lane_prepare_time = 0x03;
            lane_zero_time    = 0x14;
            lane_trail_time   = 0x0f;
		}
		else
		{
            printk("[%s::%d]mipi clock cfg Error!! %d\n", __FILE__, __LINE__, bps);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xcd);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x42);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x36);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xcd;
            lane_prepare_time = 0x42;
            lane_zero_time    = 0x0e;
            lane_trail_time   = 0x0f;
    	}

		for(ii=0; ii<4; ii++)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x65+(ii*0x20), lane_hs_tlpx_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+(ii*0x20), lane_prepare_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x67+(ii*0x20), lane_zero_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+(ii*0x20), lane_trail_time);
		}

		if(swap == NC_MIPI_SWAP_ENABLE)
		{
			for(ii=0; ii<4; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+(ii*0x20), 0x5a);	// MIPI Lane
			}
		}
		else
		{
			for(ii=0; ii<4; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+(ii*0x20), 0x4a);
			}
		}

		msleep(10);
	}
}

// external decoder initialization
// rewrite - void nc_drv_decoder_init_set(void)
static void ad_nvp6188_nc_drv_decoder_init_set(NC_MIPI_LANE_E lane, AD_NVP6188_NC_MIPI_BPS_E bps)
{
	int dev = 0;
	int chn = 0;
	NC_U8 chip_id = 0;

	for(dev=0; dev<(int)s_nc_drv_chip_cnt; dev++)
	{
		printk("[%s::%d]Addr[0x%02X], ID[0x%02X : %s] Initialize!!\n", __func__, __LINE__, g_nc_drv_i2c_addr[dev], g_nc_drv_chip_id[dev], nc_drv_common_chip_id_str_get(g_nc_drv_chip_id[dev]));

		chip_id = nc_drv_common_info_chip_id_get(dev);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);

		/* Internal Clock Divider Reset */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x40);

		/* External Interface Clock - PD_VCLK[5:4] */
		if(chip_id == NC_DECODER_6188 || chip_id == NC_DECODER_6188C)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98, 0x30);
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98, 0x30);
			printk("[%s::%d]Error!! unknown id!![%x :: %x]\n", __func__, __LINE__, g_nc_drv_chip_id[dev], chip_id);
		}

		/* Clock Auto OFF */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, 0x00);

		/* MIPI D-PHY Initialize */
		ad_nvp6188_nc_drv_mipi_dphy_init_set(dev, nc_drv_common_info_chip_id_get(dev), lane, bps, NC_MIPI_SWAP_DISABLE);

		/* Decoder Initialize */
		for(chn=0; chn<4; chn++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0xd0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x04);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08, 0x55);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0xEE);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x59, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x76, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x77, 0x80);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x79, 0x11);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xB8);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7B, 0x11);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb9, 0x72);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xB8);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c, 0x00);

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+chn, 0x10);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22+(chn*0x04), 0x0b);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x23+(chn*0x04), 0x41);
		}

		/* VFC Auto Mode Setting */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0xA0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12, 0x04);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2E, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x70);	// Auto 0x7F, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0xff);	// Auto 0x7F, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3a, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3b, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3d, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3e, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3f, 0x0f);	// Auto 0xFF, Manual 0x00

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, 0x30);	// Auto 0x30 or f0, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x72, 0x05);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7A, 0xf0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x47);

		/* EQ Stage get level */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x74, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x76, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x75, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x77, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x79, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x0c);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2f, 0xc8);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x73, 0x23);

		/* H Scaler */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_9);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x96, 0x03);

		/* Audio Default Setting */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x09);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x88);	// Master Mode, 16Khz, 16bit_Width
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x1B);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0xD0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x49, 0x88);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x59, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5d, 0x00); // NR Enable 0x9a, Disable 0x00

		/* MPP Coaxial mode select Ch1~4  */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA8, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA9, 0x09);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAA, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAB, 0x0B);

		/* Disable Video clock, Video digital output */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xca, 0x00);

		/* Output port initialize */
		nc_drv_dec_output_port_init_set(dev, nc_drv_common_info_chip_id_get(dev));

		/* Common Arbiter initialize */
		nc_drv_arbiter_common_init_set(dev, nc_drv_common_info_chip_id_get(dev), EMBEDDED_DATA_OFF, NC_MIPI_BPS_1458Mbps);

		/* PLL Clock ****************************************
		 * 148.5Mhz : 1x82->0x14, 1x83->0x2c
		 * 297Mhz   : 1x82->0x12, 1x83->0x2c
		 ****************************************************/
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x82, 0x12);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x61);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x60);

		/* TM Clock */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa0, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa1, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa2, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa3, 0x20);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7b, 0xff);


		nc_drv_video_input_initialize_set(dev);

		/* MIPI, Arbiter[7:4], Decoder[3:0] Reset */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97, 0xf0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97, 0x0f);


		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x7f);
	}
}

static ER ad_nvp6188_set_pattern_gen(UINT32 chip_id, AD_NVP6188_PATTERN_COLOR color, UINT32 fix)
{
	AD_NVP6188_BGDCOL_1_2_REG bgdcol_12_reg = {0};
	AD_NVP6188_BGDCOL_3_4_REG bgdcol_34_reg = {0};
	UINT32 i_vin;

	AD_IND("Set pattern gen color %u, fix %u\r\n", color, fix);

	if (color >= AD_NVP6188_PATTERN_COLOR_MAX) {
		color = AD_NVP6188_PATTERN_COLOR_GRID;
	}

	ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BANK_REG_OFS, BANK_0, AD_NVP6188_I2C_DATA_LEN);

	bgdcol_12_reg.bit.BGDCOL_1 = color;
	bgdcol_12_reg.bit.BGDCOL_2 = color;
	ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BGDCOL_1_2_REG_OFS, bgdcol_12_reg.reg, AD_NVP6188_I2C_DATA_LEN);

	bgdcol_34_reg.bit.BGDCOL_3 = color;
	bgdcol_34_reg.bit.BGDCOL_4 = color;
	ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BGDCOL_3_4_REG_OFS, bgdcol_34_reg.reg, AD_NVP6188_I2C_DATA_LEN);

	if (fix) {
		if (color < AD_NVP6188_PATTERN_COLOR_GRID) {
			for (i_vin = 0; i_vin < AD_NVP6188_VIN_MAX; i_vin++) {
				ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BANK_REG_OFS, BANK_5 + i_vin, AD_NVP6188_I2C_DATA_LEN);
				ad_nvp6188_i2c_write(chip_id, 0x2c, 0x00, AD_NVP6188_I2C_DATA_LEN);
				ad_nvp6188_i2c_write(chip_id, 0x6a, 0x00, AD_NVP6188_I2C_DATA_LEN);
			}
		} else {
			for (i_vin = 0; i_vin < AD_NVP6188_VIN_MAX; i_vin++) {
				ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BANK_REG_OFS, BANK_5 + i_vin, AD_NVP6188_I2C_DATA_LEN);
				ad_nvp6188_i2c_write(chip_id, 0x2c, 0x08, AD_NVP6188_I2C_DATA_LEN); // internal color bar can't disable color bar reg (0x2c) even in fix mode
				ad_nvp6188_i2c_write(chip_id, 0x6a, 0x00, AD_NVP6188_I2C_DATA_LEN); // only disable moving white block reg (0x6a)
			}
		}
	} else {
		for (i_vin = 0; i_vin < AD_NVP6188_VIN_MAX; i_vin++) {
			ad_nvp6188_i2c_write(chip_id, AD_NVP6188_BANK_REG_OFS, BANK_5 + i_vin, AD_NVP6188_I2C_DATA_LEN);
			ad_nvp6188_i2c_write(chip_id, 0x2c, 0x08, AD_NVP6188_I2C_DATA_LEN);
			ad_nvp6188_i2c_write(chip_id, 0x6a, 0x80, AD_NVP6188_I2C_DATA_LEN);
		}
	}

	return E_OK;
}

// implementation of video mode converting
static ER ad_nvp6188_det_mode_to_vin_mode(AD_NVP6188_DET_RAW_MODE raw_mode, AD_NVP6188_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6188_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6188_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_nvp6188_det_map[i_mode].raw_mode, sizeof(AD_NVP6188_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_nvp6188_det_map[i_mode].vin_mode, sizeof(AD_NVP6188_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u\r\n", AD_NVP6188_MODULE_NAME, raw_mode.mode);
	return E_SYS;
}

static ER ad_nvp6188_vin_mode_to_ch_mode(AD_NVP6188_VIN_MODE vin_mode, AD_NVP6188_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6188_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6188_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_nvp6188_vin_map[i_mode].vin_mode, sizeof(AD_NVP6188_VIN_MODE)) == 0) {
			*ch_mode = g_ad_nvp6188_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u %u\r\n", AD_NVP6188_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog, vin_mode.std);
	return E_SYS;
}

/*static ER ad_nvp6188_vout_mode_to_port_mode(AD_NVP6188_VOUT_MODE vout_mode, AD_NVP6188_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_nvp6188_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_nvp6188_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_nvp6188_vout_map[i_mode].vout_mode, sizeof(AD_NVP6188_VOUT_MODE)) == 0) {
			*port_mode = g_ad_nvp6188_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_NVP6188_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_nvp6188_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_NVP6188_DET_VIDEO_INFO *video_info)
{
	AD_INFO *ad_info;
	AD_NVP6188_INFO *ad_private_info;
	nc_decoder_s ncde_rp5 = {0};
	NC_VIDEO_FMT_INIT_TABLE_S *fmt_tbl;
	AD_NVP6188_DET_RAW_MODE raw_mode = {0};
	AD_NVP6188_DET_VIN_MODE vin_mode = {0};

	ad_info = ad_nvp6188_get_info(chip_id);
	ad_private_info = ad_nvp6188_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad det fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det fail. NULL ad private info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	ncde_rp5.Chn = (chip_id * AD_NVP6188_VIN_MAX) + vin_id;

	/* Get video status */
	nc_drv_video_input_vfc_status_get(&ncde_rp5);

	if (!ncde_rp5.FmtChange) {
		goto do_skip;
	}

	if (ncde_rp5.VideoFormat == NC_VIVO_CH_FORMATDEF_UNKNOWN) {
		AD_IND("chip_id%u Chn%u vfc=%x. vloss\r\n", chip_id, ncde_rp5.Chn, ncde_rp5.Vfc);
		goto do_vloss;
	}

	/* Get video format name */
	fmt_tbl = nc_drv_table_video_init_vlaue_get(ncde_rp5.VideoFormat);
	if (fmt_tbl) {
		AD_IND("chip_id%u Chn%u vfc=%x fmt=%s\r\n", chip_id, ncde_rp5.Chn, ncde_rp5.Vfc, fmt_tbl->name);
	} else {
		AD_IND("chip_id%u Chn%u vfc=%x fmt=%s\r\n", chip_id, ncde_rp5.Chn, ncde_rp5.Vfc, "Unknown");
	}

	/* Decoder setting */
	ncde_rp5.VideoSetMode = NC_VIDEO_SET_MODE_AUTO;
	nc_drv_video_input_set(&ncde_rp5);

	/* EQ Stage get */
	nc_drv_video_input_eq_stage_get(&ncde_rp5);

	/* EQ Stage set */
	nc_drv_video_input_eq_stage_set(&ncde_rp5);

	raw_mode.mode = ncde_rp5.Vfc;
	if (ad_nvp6188_det_mode_to_vin_mode(raw_mode, &vin_mode) != E_OK) {
		goto do_vloss;
	}

	ad_private_info->det_vin_mode[vin_id] = vin_mode;

	video_info->vloss = FALSE;
	video_info->width = vin_mode.width;
	video_info->height = vin_mode.height;
	video_info->fps = vin_mode.fps;
	video_info->prog = vin_mode.prog;
	return E_OK;

do_skip:
	video_info->vloss = ad_info->vin[vin_id].vloss;
	video_info->width = ad_private_info->det_vin_mode[vin_id].width;
	video_info->height = ad_private_info->det_vin_mode[vin_id].height;
	video_info->fps = ad_private_info->det_vin_mode[vin_id].fps;
	video_info->prog = ad_private_info->det_vin_mode[vin_id].prog;
	return E_OK;

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_NVP6188_DFT_WIDTH;
	video_info->height = AD_NVP6188_DFT_HEIGHT;
	video_info->fps = AD_NVP6188_DFT_FPS;
	video_info->prog = AD_NVP6188_DFT_PROG;
	return E_OK;
}

/*static ER ad_nvp6188_dbg_wdt(UINT32 chip_id)
{
	AD_INFO *ad_info;
	AD_DECET_INFO detect_info = {0};
	UINT32 i_vin;

	ad_info = ad_nvp6188_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad dbg wdt fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	for (i_vin = 0; i_vin < ad_info->vin_max; i_vin++) {
		detect_info.vin_id = i_vin;
		ad_nvp6188_watchdog_cb(chip_id, &detect_info);
		ad_info->vin[i_vin].det_format = detect_info.det_info;
	}

	return E_OK;
}

static ER ad_nvp6188_dbg_chgmode(UINT32 chip_id)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO chgmode_info = {0};
	UINT32 i_vin;

	ad_info = ad_nvp6188_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad dbg chgmode fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	for (i_vin = 0; i_vin < 2; i_vin++) {
		chgmode_info.vin_id = i_vin;
		chgmode_info.vout_id = i_vin;
		chgmode_info.ui_info = ad_info->vin[i_vin].det_format;
		ad_nvp6188_chgmode(chip_id, &chgmode_info);
		ad_info->vin[i_vin].cur_format = chgmode_info.cur_info;
	}

	return E_OK;
}*/

#if 0
#endif

static ER ad_nvp6188_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_nvp6188_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	ad_nvp6188_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_NVP6188_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_NVP6188_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_NVP6188_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_NVP6188_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_NVP6188_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_NVP6188_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_nvp6188_bank) / sizeof(typeof(g_ad_nvp6188_bank[0]));
	reg_info.bank = g_ad_nvp6188_bank;
	reg_info.dump_per_row = AD_NVP6188_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_nvp6188_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_nvp6188_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_nvp6188_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_nvp6188_init(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_NVP6188_INFO *ad_private_info;

	ad_info = ad_nvp6188_get_info(chip_id);
	ad_private_info = ad_nvp6188_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	/************ rewrite external driver's module init function ************/

	// nvp6188_module_init()

	printk("================================= raptor5_module_init_start =================================================\n");

	if (s_nc_drv_chip_cnt >= AD_NVP6188_CHIP_MAX) {
		AD_ERR("[%s] ad init fail. chip cnt overflow\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	g_nc_drv_i2c_addr[s_nc_drv_chip_cnt++] = ad_info->i2c_addr;
	s_nc_drv_channel_cnt = s_nc_drv_chip_cnt * AD_NVP6188_VIN_MAX;

	/* Decoder Device ID Check */
	gpio_i2c_write(g_nc_drv_i2c_addr[chip_id], 0xFF, BANK_0);
	g_nc_drv_chip_id[chip_id] = gpio_i2c_read(g_nc_drv_i2c_addr[chip_id], 0xF4);
	if(g_nc_drv_chip_id[chip_id] == 0xD3)
		g_nc_drv_chip_id[chip_id] = 0xD0;

	/* Decoder Device Revision Check */
	gpio_i2c_write(g_nc_drv_i2c_addr[chip_id], 0xFF, BANK_0);
	g_nc_drv_chip_rev[chip_id] = gpio_i2c_read(g_nc_drv_i2c_addr[chip_id], 0xF5);

	printk("********************** Decoder Chip Information *********************\n");
	printk("Decoder Chip Count = %d\n", s_nc_drv_chip_cnt);
	printk("SlaveAddress    [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_i2c_addr[0],g_nc_drv_i2c_addr[1],g_nc_drv_i2c_addr[2],g_nc_drv_i2c_addr[3]);
	printk("DecoderID       [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_chip_id[0],g_nc_drv_chip_id[1],g_nc_drv_chip_id[2],g_nc_drv_chip_id[3]);
	printk("DecoderRevision [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_chip_rev[0],g_nc_drv_chip_rev[1],g_nc_drv_chip_rev[2],g_nc_drv_chip_rev[3]);
	printk("**********************************************************************\n");

	nc_drv_version_information_print();
	nc_drv_common_info_chip_data_init_set(s_nc_drv_chip_cnt, g_nc_drv_chip_id, g_nc_drv_chip_rev, g_nc_drv_i2c_addr, NC_VO_WORK_MODE_4MUX);
	ad_nvp6188_nc_drv_decoder_init_set(NC_MIPI_LANE_4, AD_NVP6188_NC_MIPI_BPS_1188Mbps);

	ad_private_info->is_inited = TRUE;

done:
	g_ad_nvp6188_info[chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_nvp6188_uninit(UINT32 chip_id, void *ext_data)
{
	AD_NVP6188_INFO *ad_private_info;

	ad_private_info = ad_nvp6188_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	/************ rewrite external driver's module exit function ************/

	// nvp6188_module_exit()

	if (s_nc_drv_chip_cnt == 0) {
		AD_ERR("[%s] ad uninit fail. chip cnt overflow\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	s_nc_drv_chip_cnt--;

	printk("RAPTOR5 DEVICE DRIVER UNLOAD SUCCESS\n");

	ad_private_info->is_inited = FALSE;
	g_ad_nvp6188_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_nvp6188_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_nvp6188_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
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
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_NVP6188_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			if (unlikely(mode_info->out_id >= ad_info->vout_max)) {
				AD_ERR("[%s] mode_info wrong input\r\n", AD_NVP6188_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			mode_info->bus_type = ad_info->vout[mode_info->out_id].bus_type;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 74250000;
			mode_info->data_rate = 300000000;
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
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_NVP6188_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_NVP6188_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_nvp6188_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_nvp6188_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_NVP6188_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_NVP6188_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_nvp6188_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id/*, vch*/;
	AD_NVP6188_VIN_MODE vin_mode = {0};
	AD_NVP6188_CH_MODE ch_mode = {0};
	//nc_decoder_s ncde_rp5 = {0};

	ad_info = ad_nvp6188_get_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_NVP6188_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_NVP6188_MODULE_NAME, vin_id, AD_NVP6188_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_NVP6188_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_NVP6188_MODULE_NAME, vout_id, AD_NVP6188_VOUT_MAX);
		return E_SYS;
	}

	//vch = (chip_id * AD_NVP6188_VIN_MAX) + vin_id;

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	//DBG_DUMP("chip%d vin%d: w = %d, h = %d, fps = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps);

	if (unlikely(ad_nvp6188_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

#if 0
	ncde_rp5.Chn = vch;
	ncde_rp5.VideoCable  = CABLE_3C2V;
	ncde_rp5.FormatStandard = ch_mode.FormatStandard;
	ncde_rp5.FormatResolution = ch_mode.FormatResolution;
	ncde_rp5.FormatFps = ch_mode.FormatFps;
	nc_drv_video_input_manual_set(&ncde_rp5);
#endif

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;

	return E_OK;
}

static ER ad_nvp6188_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_NVP6188_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_nvp6188_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_NVP6188_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_nvp6188_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_nvp6188_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_nvp6188_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_NVP6188_MODULE_NAME, i_vin);
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

static ER ad_nvp6188_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_nvp6188_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_nvp6188_dbg(char *str_cmd)
{
#define AD_NVP6188_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_NVP6188_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_NVP6188_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_NVP6188_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_NVP6188_DBG_CMD_MAX] = {0};
	UINT32 /*val = 0, val2 = 0,*/ i_chip = 0, i_vin;
	AD_NVP6188_VIN_MODE vin_mode = {0};
	AD_NVP6188_CH_MODE ch_mode = {0};
	AD_NVP6188_NC_MIPI_BPS_E bps = AD_NVP6188_NC_MIPI_BPS_1188Mbps;

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_NVP6188_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_NVP6188_DBG_CMD_MAX);

	if (0) {
	} else if (strcmp(cmd_list[0], "set") == 0) {

		if (cmd_num == 1) {
			AD_DUMP("vfmt VIN(u) W(u) H(u) FPS(u) [prog(u)] [std(u)] [mip_spd(u)]: set video format of VIN. std: 0-AHD, 1-TVI. mip_spd: 37/148\r\n");
			AD_DUMP("patgen COLOR(s) [FIX(u)]: set pattern gen color. COLOR: blue/white/yellow/cyan/green/magenta/red/black/gray/grid/horizontal/vertical/bw. FIX: 0/1\r\n");

		} else if (cmd_num >= 6 && strcmp(cmd_list[1], "vfmt") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1 && // vin_id
				sscanf_s(cmd_list[3], "%u", &narg[1]) == 1 && // width
				sscanf_s(cmd_list[4], "%u", &narg[2]) == 1 && // height
				sscanf_s(cmd_list[5], "%u", &narg[3]) == 1) { // fps

				if (cmd_num < 7 || sscanf_s(cmd_list[6], "%u", &narg[4]) != 1) {
					narg[4] = TRUE; // prog
				}

				if (cmd_num < 8 || sscanf_s(cmd_list[7], "%u", &narg[5]) != 1) {
					narg[5] = AD_NVP6188_STD_AHD; // std
				}

				if (cmd_num < 9 || sscanf_s(cmd_list[8], "%u", &narg[6]) != 1 || (narg[6] != 148 && narg[6] != 37)) {
					narg[6] = 148; // mipi speed
				}

				//i_vin = narg[0];
				vin_mode.width = narg[1];
				vin_mode.height = narg[2];
				vin_mode.fps = narg[3] * 100;
				vin_mode.prog = narg[4];
				vin_mode.std = narg[5];
				switch (narg[6]) {
				case 37:
					bps = AD_NVP6188_NC_MIPI_BPS_297Mbps;
					break;
				case 148:
				default:
					bps = AD_NVP6188_NC_MIPI_BPS_1188Mbps;
					break;
				}

				if (ad_nvp6188_vin_mode_to_ch_mode(vin_mode, &ch_mode) == E_OK) {
					static nc_decoder_s ncde_rp5 = {0}; // prevent frame size overflow build error

					ad_nvp6188_nc_drv_decoder_init_set(NC_MIPI_LANE_4, bps);

					ncde_rp5.OP_Mode = NC_VIDEO_SET_MODE_MANUAL;
					ncde_rp5.Chn = (i_chip * AD_NVP6188_VIN_MAX);
					nc_drv_video_auto_manual_mode_set(&ncde_rp5);

					for (i_vin = 0; i_vin < AD_NVP6188_VIN_MAX; i_vin++) {
						ncde_rp5.Chn = (i_chip * AD_NVP6188_VIN_MAX) + i_vin;
						ncde_rp5.VideoCable  = CABLE_3C2V;
						ncde_rp5.FormatStandard = ch_mode.FormatStandard;
						ncde_rp5.FormatResolution = ch_mode.FormatResolution;
						ncde_rp5.FormatFps = ch_mode.FormatFps;
						nc_drv_video_input_manual_set(&ncde_rp5);

						ncde_rp5.Value = 1;
						nc_drv_video_output_color_pattern_set(&ncde_rp5);
					}

				} else {
					AD_ERR("set err\r\n");
				}

			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "patgen") == 0) {
			if (0) {
			} else if (strcmp(cmd_list[2], "blue") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_BLUE; // color
			} else if (strcmp(cmd_list[2], "white") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_WHITE;
			} else if (strcmp(cmd_list[2], "yellow") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_YELLOW;
			} else if (strcmp(cmd_list[2], "cyan") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_CYAN;
			} else if (strcmp(cmd_list[2], "green") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_GREEN;
			} else if (strcmp(cmd_list[2], "magenta") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_MAGENTA;
			} else if (strcmp(cmd_list[2], "red") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_RED;
			} else if (strcmp(cmd_list[2], "black") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_BLACK;
			} else if (strcmp(cmd_list[2], "gray") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_GRAY;
			} else if (strcmp(cmd_list[2], "grid") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_GRID;
			} else if (strcmp(cmd_list[2], "horizontal") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_HORIZONTAL;
			} else if (strcmp(cmd_list[2], "vertical") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_VERTICAL;
			} else if (strcmp(cmd_list[2], "bw") == 0) {
				narg[0] = AD_NVP6188_PATTERN_COLOR_BW;
			} else {
				AD_ERR("Unknown color %s\n", cmd_list[2]);
				narg[0] = AD_NVP6188_PATTERN_COLOR_GRID;
			}

			if (cmd_num < 4 || sscanf_s(cmd_list[3], "%u", &narg[1]) != 1) {
				narg[1] = 0; // fix
			}

			ad_nvp6188_set_pattern_gen(i_chip, narg[0], narg[1]);
		}
	}

	return E_OK;
}

AD_DEV* ad_get_obj_nvp6188(void)
{
	return &g_ad_nvp6188_obj;
}

