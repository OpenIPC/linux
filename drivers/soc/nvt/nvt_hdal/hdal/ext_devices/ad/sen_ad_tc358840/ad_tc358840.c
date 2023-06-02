#include "ad_tc358840.h"
#include "nvt_ad_tc358840_reg.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 02, 005, 00);

// mapping table: vin raw data -> meaningful vin info
/*static AD_TC358840_DET_MAP g_ad_tc358840_det_map[] = {
};*/

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
/*static AD_TC358840_VIN_MAP g_ad_tc358840_vin_map[] = {
};*/

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_TC358840_VOUT_MAP g_ad_tc358840_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_tc358840_slv_addr[] = {0x1E, 0x3E};

// decoder dump all register information
static AD_DRV_DBG_REG_DUMP_TBL g_ad_tc358840_reg_tbl[] = {
	{.addr = 0x0000, .len = 0x320},
	{.addr = 0x5000, .len = 0x100},
	{.addr = 0x8400, .len = 0x900},
};
#define AD_TC358840_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_TC358840_DFT_WIDTH 1920
#define AD_TC358840_DFT_HEIGHT 1080
#define AD_TC358840_DFT_FPS 3000
#define AD_TC358840_DFT_PROG TRUE
#define AD_TC358840_DFT_LANE_NUM 4

// i2c retry number when operation fail occur
#define AD_TC358840_I2C_RETRY_NUM 5

// global variable
#define AD_TC358840_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_tc358840_vin[AD_TC358840_CHIP_MAX][AD_TC358840_VIN_MAX];
static AD_VOUT g_ad_tc358840_vout[AD_TC358840_CHIP_MAX][AD_TC358840_VOUT_MAX];
static AD_INFO g_ad_tc358840_info[AD_TC358840_CHIP_MAX] = {
	{
		.name = "TC358840_0",
		.vin_max = AD_TC358840_VIN_MAX,
		.vin = &g_ad_tc358840_vin[0][0],
		.vout_max = AD_TC358840_VOUT_MAX,
		.vout = &g_ad_tc358840_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358840_1",
		.vin_max = AD_TC358840_VIN_MAX,
		.vin = &g_ad_tc358840_vin[1][0],
		.vout_max = AD_TC358840_VOUT_MAX,
		.vout = &g_ad_tc358840_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358840_2",
		.vin_max = AD_TC358840_VIN_MAX,
		.vin = &g_ad_tc358840_vin[2][0],
		.vout_max = AD_TC358840_VOUT_MAX,
		.vout = &g_ad_tc358840_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358840_3",
		.vin_max = AD_TC358840_VIN_MAX,
		.vin = &g_ad_tc358840_vin[3][0],
		.vout_max = AD_TC358840_VOUT_MAX,
		.vout = &g_ad_tc358840_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_TC358840_INFO g_ad_tc358840_info_private[AD_TC358840_CHIP_MAX] = {
	[0 ... (AD_TC358840_CHIP_MAX-1)] = {
		0,
	}
};

static ER ad_tc358840_open(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_close(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_init(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_uninit(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tc358840_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tc358840_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_tc358840_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_tc358840_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_tc358840_dbg(char *str_cmd);

static AD_DEV g_ad_tc358840_obj = {
	AD_TC358840_CHIP_MAX,
	g_ad_tc358840_info,

	sizeof(g_ad_tc358840_slv_addr) / sizeof(typeof(g_ad_tc358840_slv_addr[0])),
	g_ad_tc358840_slv_addr,

	{
		.open = ad_tc358840_open,
		.close = ad_tc358840_close,
		.init = ad_tc358840_init,
		.uninit = ad_tc358840_uninit,
		.get_cfg = ad_tc358840_get_cfg,
		.set_cfg = ad_tc358840_set_cfg,
		.chgmode = ad_tc358840_chgmode,
		.det_plug_in = ad_tc358840_watchdog_cb,
		.i2c_write = ad_tc358840_i2c_write,
		.i2c_read = ad_tc358840_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_tc358840_dbg,
	}
};

// internal function
static AD_INFO *ad_tc358840_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TC358840_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TC358840_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tc358840_info[chip_id];
}

static AD_TC358840_INFO *ad_tc358840_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TC358840_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TC358840_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tc358840_info_private[chip_id];
}

static void ad_tc358840_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_TC358840_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_TC358840_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_TC358840_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_TC358840_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_TC358840_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_TC358840_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_TC358840_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_TC358840_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_TC358840_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_TC358840_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_TC358840_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_TC358840_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

static ER ad_tc358840_set_mipi_data_lane(UINT32 chip_id, UINT32 lane_num)
{
	AD_TC358840_LANE_ENABLE_REG lane_enable = {0};
	AD_TC358840_PPI_DPHY_HSTXVREGEN_REG ppi_dphy_hstxvregen = {0};
	ER rt = E_OK;

	AD_IND("SET MIPI DATA LANE (%u)\r\n", lane_num);

	if (lane_num > 4) {
		AD_ERR("SET MIPI DATA LANE fail. lane_num(%u) must <= 4\r\n", lane_num);
		return E_SYS;
	}

	lane_enable.bit.DTLANEEN = lane_num;
	lane_enable.bit.CLANEEN = 1;
	rt |= ad_tc358840_i2c_write(chip_id, AD_TC358840_LANE_ENABLE_REG_OFS, lane_enable.reg, AD_TC358840_LANE_ENABLE_REG_LEN);

	ppi_dphy_hstxvregen.bit.CLM_HSTXVREGEN = 1;
	ppi_dphy_hstxvregen.bit.D0M_HSTXVREGEN = (lane_num >= 1);
	ppi_dphy_hstxvregen.bit.D1M_HSTXVREGEN = (lane_num >= 2);
	ppi_dphy_hstxvregen.bit.D2M_HSTXVREGEN = (lane_num >= 3);
	ppi_dphy_hstxvregen.bit.D3M_HSTXVREGEN = (lane_num >= 4);
	rt |= ad_tc358840_i2c_write(chip_id, AD_TC358840_PPI_DPHY_HSTXVREGEN_REG_OFS, ppi_dphy_hstxvregen.reg, AD_TC358840_PPI_DPHY_HSTXVREGEN_REG_LEN);

	return rt;
}

static ER ad_tc358840_set_edid_with_chksum(UINT32 chip_id, UINT32 addr, UINT32 val, UINT32 len, UINT32 *chksum)
{
	*chksum += val;
	return ad_tc358840_i2c_write(chip_id, addr, val, len);
}

static ER ad_tc358840_common_init(UINT32 chip_id, UINT32 data_lane)
{
	ER rt = E_OK;
	UINT32 chksum = 0;

	// Software Reset
	rt |= ad_tc358840_i2c_write(chip_id, 0x0004, 0x80C4, 2); // ConfCtl0
	rt |= ad_tc358840_i2c_write(chip_id, 0x0002, 0x3F01, 2); // SysCtl
	rt |= ad_tc358840_i2c_write(chip_id, 0x0002, 0x0000, 2); // SysCtl
	rt |= ad_tc358840_i2c_write(chip_id, 0x0006, 0x0008, 2); // ConfCtl1
	// CSI-TX0 Transition Timing
	rt |= ad_tc358840_i2c_write(chip_id, 0x0108, 0x00000001, 4); // CSI_TX_CLKEN
	rt |= ad_tc358840_i2c_write(chip_id, 0x010C, 0x00000001, 4); // CSI_TX_CLKSEL
	rt |= ad_tc358840_i2c_write(chip_id, 0x02A0, 0x00000001, 4); // MIPI_PLL_CONTROL
	rt |= ad_tc358840_i2c_write(chip_id, 0x02AC, 0x0000907C, 4); // MIPI_PLL_CNF
	vos_util_delay_us(1);
	rt |= ad_tc358840_i2c_write(chip_id, 0x02A0, 0x00000003, 4); // MIPI_PLL_CONTROL
	rt |= ad_tc358840_set_mipi_data_lane(chip_id, data_lane);
	//rt |= ad_tc358840_i2c_write(chip_id, 0x0118, 0x00000014, 4); // LANE_ENABLE
	rt |= ad_tc358840_i2c_write(chip_id, 0x0120, 0x00000FA0, 4); // LINE_INIT_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0124, 0x00000000, 4); // HSTX_TO_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0128, 0x00000101, 4); // FUNC_ENABLE
	rt |= ad_tc358840_i2c_write(chip_id, 0x0130, 0x00010000, 4); // CSI_TATO_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0134, 0x00005000, 4); // CSI_PRESP_BTA_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0138, 0x00010000, 4); // CSI_PRESP_LPR_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x013C, 0x00010000, 4); // CSI_PRESP_LPW_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0140, 0x00010000, 4); // CSI_PRESP_HSR_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0144, 0x00010000, 4); // CSI_PRESP_HSW_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0148, 0x00001000, 4); // CSI_PR_TO_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x014C, 0x00010000, 4); // CSI_LRX-H_TO_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0150, 0x00000160, 4); // FUNC_MODE
	rt |= ad_tc358840_i2c_write(chip_id, 0x0154, 0x00000001, 4); // CSI_RX_VC_ENABLE
	rt |= ad_tc358840_i2c_write(chip_id, 0x0158, 0x000000C8, 4); // IND_TO_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0168, 0x0000002A, 4); // CSI_HSYNC_STOP_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x01A4, 0x00000000, 4); // CSI_RX_STATE_INT_MASK
	rt |= ad_tc358840_i2c_write(chip_id, 0x01C0, 0x00000015, 4); // CSI_LPRX_THRESH_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0214, 0x00000000, 4); // APP_SIDE_ERR_INT_MASK
	rt |= ad_tc358840_i2c_write(chip_id, 0x021C, 0x00000080, 4); // CSI_RX_ERR_INT_MASK
	rt |= ad_tc358840_i2c_write(chip_id, 0x0224, 0x00000000, 4); // CSI_LPTX_INT_MASK
	rt |= ad_tc358840_i2c_write(chip_id, 0x0254, 0x00000004, 4); // LPTXTIMECNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0258, 0x00180203, 4); // TCLK_HEADERCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x025C, 0x00040005, 4); // TCLK_TRAILCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0260, 0x000D0004, 4); // THS_HEADERCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0264, 0x00003E80, 4); // TWAKEUPCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0268, 0x0000000A, 4); // TCLK_POSTCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x026C, 0x00090006, 4); // THS_TRAILCNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x0270, 0x00000020, 4); // HSTXVREGCNT
	//rt |= ad_tc358840_i2c_write(chip_id, 0x0274, 0x0000001F, 4); // HSTXVREGEN
	rt |= ad_tc358840_i2c_write(chip_id, 0x0278, 0x00040003, 4); // BTA_COUNT
	rt |= ad_tc358840_i2c_write(chip_id, 0x027C, 0x00000002, 4); // DPHY_TX ADJUST
	rt |= ad_tc358840_i2c_write(chip_id, 0x011C, 0x00000001, 4); // CSITX_START
	// CSI-TX1 Transition Timing
	// Sync Signal Polarity
	rt |= ad_tc358840_i2c_write(chip_id, 0x0110, 0x00000006, 4); // MODE_CONFIG
	rt |= ad_tc358840_i2c_write(chip_id, 0x0310, 0x00000006, 4); // MODE_CONFIG
	// Split Control, VPID
	rt |= ad_tc358840_i2c_write(chip_id, 0x5000, 0x0100, 2); // STX0_CTL
	rt |= ad_tc358840_i2c_write(chip_id, 0x5004, 0x3600, 2); // STX0_VPID2
	rt |= ad_tc358840_i2c_write(chip_id, 0x500C, 0x8000, 2); // STX0_FPX
	rt |= ad_tc358840_i2c_write(chip_id, 0x5080, 0x0100, 2); // STX1_CTL
	rt |= ad_tc358840_i2c_write(chip_id, 0x5084, 0x3600, 2); // STX1_VPID2
	// Frame Count Control
	// HDMI PHY
	rt |= ad_tc358840_i2c_write(chip_id, 0x8410, 0x03, 1); // PHY CTL
	rt |= ad_tc358840_i2c_write(chip_id, 0x8413, 0x3F, 1); // PHY_ENB
	rt |= ad_tc358840_i2c_write(chip_id, 0x8420, 0x07, 1); // EQ_BYPS
	rt |= ad_tc358840_i2c_write(chip_id, 0x84F0, 0x31, 1); // APLL_CTL
	rt |= ad_tc358840_i2c_write(chip_id, 0x84F4, 0x01, 1); // DDCIO_CTL
	// HDMI Clock
	rt |= ad_tc358840_i2c_write(chip_id, 0x8540, 0x1388, 2); // SYS_FREQ0_1
	rt |= ad_tc358840_i2c_write(chip_id, 0x8630, 0x20, 1); // LOCKDET_FREQ0
	rt |= ad_tc358840_i2c_write(chip_id, 0x8631, 0x07A1, 2); // LOCKDET_REF1_2
	rt |= ad_tc358840_i2c_write(chip_id, 0x8670, 0x02, 1); // NCO_F0_MOD
	rt |= ad_tc358840_i2c_write(chip_id, 0x8A0C, 0x1388, 2); // CSC_SCLK0_1
	// HDMI Interrupt Mask, Clear
	rt |= ad_tc358840_i2c_write(chip_id, 0x8502, 0xFF, 1); // SYS_INT
	rt |= ad_tc358840_i2c_write(chip_id, 0x8512, 0xFE, 1); // SYS_INTM
	// Interrupt Control (TOP level)
	rt |= ad_tc358840_i2c_write(chip_id, 0x0014, 0x0FBF, 2); // IntStatus
	rt |= ad_tc358840_i2c_write(chip_id, 0x0016, 0x0DBF, 2); // IntMask
	// EDID
	rt |= ad_tc358840_i2c_write(chip_id, 0x85E0, 0x01, 1); // EDID_MODE
	rt |= ad_tc358840_i2c_write(chip_id, 0x85E3, 0x0100, 2); // EDID_LEN1_2
	// EDID Data
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C00, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C01, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C02, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C03, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C04, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C05, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C06, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C07, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C08, 0x52, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C09, 0x62, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0A, 0x88, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0B, 0x88, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0D, 0x88, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0E, 0x88, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C0F, 0x88, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C10, 0x1C, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C11, 0x15, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C12, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C13, 0x03, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C14, 0x80, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C15, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C16, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C17, 0x78, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C18, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C19, 0x0D, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1A, 0xC9, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1B, 0xA0, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1C, 0x57, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1D, 0x47, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1E, 0x98, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C1F, 0x27, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C20, 0x12, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C21, 0x48, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C22, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C23, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C24, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C25, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C26, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C27, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C28, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C29, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2A, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2B, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2C, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2D, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2E, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C2F, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C30, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C31, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C32, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C33, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C34, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C35, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C36, 0x02, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C37, 0x3A, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C38, 0x80, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C39, 0x18, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3A, 0x71, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3B, 0x38, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3C, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3D, 0x40, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3E, 0x58, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C3F, 0x2C, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C40, 0x45, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C41, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C42, 0xC4, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C43, 0x8E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C44, 0x21, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C45, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C46, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C47, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C48, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C49, 0x1D, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4A, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4B, 0x72, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4C, 0x51, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4D, 0xD0, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4E, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C4F, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C50, 0x6E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C51, 0x28, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C52, 0x55, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C53, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C54, 0xC4, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C55, 0x8E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C56, 0x21, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C57, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C58, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C59, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5A, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5B, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5D, 0xFC, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5E, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C5F, 0x54, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C60, 0x6F, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C61, 0x73, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C62, 0x68, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C63, 0x69, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C64, 0x62, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C65, 0x61, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C66, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C67, 0x55, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C68, 0x48, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C69, 0x32, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6A, 0x44, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6B, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6D, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6E, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C6F, 0xFD, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C70, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C71, 0x17, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C72, 0x3D, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C73, 0x0F, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C74, 0x8C, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C75, 0x17, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C76, 0x00, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C77, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C78, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C79, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7A, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7B, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7C, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7D, 0x20, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7E, 0x01, 1); // EDID_RAM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8C7F, 0x5D, 1); // EDID_RAM

	// EDID check sum
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C80, 0x02, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C81, 0x03, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C82, 0x1A, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C83, 0x74, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C84, 0x47, 1, &chksum);  // EDID_RAM
	// ID code (descriptor 1~7)
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C85, 0x10, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C86, 0x04, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C87, 0x22, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C88, 0x04, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C89, 0x10, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8A, 0x32, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8B, 0x32, 1, &chksum);  // EDID_RAM
	// internel
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8C, 0x23, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8D, 0x09, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8E, 0x07, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C8F, 0x01, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C90, 0x83, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C91, 0x01, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C92, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C93, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C94, 0x65, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C95, 0x03, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C96, 0x0C, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C97, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C98, 0x10, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C99, 0x00, 1, &chksum);  // EDID_RAM
	// descriptor 3 (1080p30)
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9A, 0x01, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9B, 0x1D, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9C, 0x80, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9D, 0x18, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9E, 0x71, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8C9F, 0x38, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA0, 0x2D, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA1, 0x40, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA2, 0x58, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA3, 0x2C, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA4, 0x45, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA5, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA6, 0xC0, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA7, 0x6C, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA8, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CA9, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAA, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAB, 0x18, 1, &chksum);  // EDID_RAM
	// descriptor 4 (720p60)
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAC, 0x01, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAD, 0x1D, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAE, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CAF, 0x72, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB0, 0x51, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB1, 0xD0, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB2, 0x1E, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB3, 0x20, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB4, 0x6E, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB5, 0x28, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB6, 0x55, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB7, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB8, 0xC4, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CB9, 0x8E, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBA, 0x21, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBB, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBC, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBD, 0x1E, 1, &chksum);  // EDID_RAM
	// descriptor 5 (1080p60)
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBE, 0x02, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CBF, 0x3A, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC0, 0x80, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC1, 0x18, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC2, 0x71, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC3, 0x38, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC4, 0x2D, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC5, 0x40, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC6, 0x58, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC7, 0x2C, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC8, 0x45, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CC9, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCA, 0xC4, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCB, 0x8E, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCC, 0x21, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCD, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCE, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CCF, 0x1E, 1, &chksum);  // EDID_RAM
	// descriptor 6
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD0, 0x02, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD1, 0x3A, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD2, 0x80, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD3, 0x18, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD4, 0x71, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD5, 0x38, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD6, 0x2D, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD7, 0x40, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD8, 0x58, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CD9, 0x2C, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDA, 0x45, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDB, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDC, 0x80, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDD, 0x38, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDE, 0x74, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CDF, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE0, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE1, 0x18, 1, &chksum);  // EDID_RAM
	// padding
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE2, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE3, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE4, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE5, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE6, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE7, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE8, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CE9, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CEA, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CEB, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CEC, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CED, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CEE, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CEF, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF0, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF1, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF2, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF3, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF4, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF5, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF6, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF7, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF8, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CF9, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CFA, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CFB, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CFC, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CFD, 0x00, 1, &chksum);  // EDID_RAM
	rt |= ad_tc358840_set_edid_with_chksum(chip_id, 0x8CFE, 0x00, 1, &chksum);  // EDID_RAM
	// checksum
	chksum = 256 - (chksum % 256);
	AD_IND("EDID CHKSUM 0x%x\r\n", chksum);
	rt |= ad_tc358840_i2c_write(chip_id, 0x8CFF, chksum, 1); // EDID_RAM
	//rt |= ad_tc358840_i2c_write(chip_id, 0x8CFF, 0x89, 1); // EDID_RAM

	// HDCP Setting
	// Video Color Format Setting
	rt |= ad_tc358840_i2c_write(chip_id, 0x8A00, 0x01, 1); // VOUT_FMT
	rt |= ad_tc358840_i2c_write(chip_id, 0x8A01, 0x14, 1); // VOUT_FIL
	rt |= ad_tc358840_i2c_write(chip_id, 0x8A02, 0x42, 1); // VOUT_SYNC0
	rt |= ad_tc358840_i2c_write(chip_id, 0x8A08, 0x31, 1); // VOUT_COLOR
	// HDMI SYSTEM
	rt |= ad_tc358840_i2c_write(chip_id, 0x8543, 0x02, 1); // DDC_CTL
	rt |= ad_tc358840_i2c_write(chip_id, 0x8544, 0x10, 1); // HPD_CTL
	// HDMI Audio Setting
	rt |= ad_tc358840_i2c_write(chip_id, 0x8600, 0x00, 1); // AUD_Auto_Mute
	rt |= ad_tc358840_i2c_write(chip_id, 0x8602, 0xF3, 1); // Auto_CMD0
	rt |= ad_tc358840_i2c_write(chip_id, 0x8603, 0x02, 1); // Auto_CMD1
	rt |= ad_tc358840_i2c_write(chip_id, 0x8604, 0x0C, 1); // Auto_CMD2
	rt |= ad_tc358840_i2c_write(chip_id, 0x8606, 0x05, 1); // BUFINIT_START
	rt |= ad_tc358840_i2c_write(chip_id, 0x8607, 0x00, 1); // FS_MUTE
	rt |= ad_tc358840_i2c_write(chip_id, 0x8652, 0x02, 1); // SDO_MODE1
	rt |= ad_tc358840_i2c_write(chip_id, 0x8671, 0x01F75104, 4); // NCO_48F0A_D
	rt |= ad_tc358840_i2c_write(chip_id, 0x8675, 0x01CE6C09, 4); // NCO_44F0A_D
	rt |= ad_tc358840_i2c_write(chip_id, 0x8680, 0x00, 1); // AUD_MODE
	// Let HDMI Source start access
	rt |= ad_tc358840_i2c_write(chip_id, 0x854A, 0x01, 1); // INIT_END
	// Wait until HDMI sync is established
	//rt |= ad_tc358840_i2c_read(chip_id, 0x8520, 0x00, 1); // SYS_STATUS
	// Sequence: Check bit7 of 8x8520
	// Start Video TX
	//rt |= ad_tc358840_i2c_write(chip_id, 0x0004, 0x8CF5, 2); // ConfCtl0
	//rt |= ad_tc358840_i2c_write(chip_id, 0x0006, 0x0000, 2); // ConfCtl1
	return rt;
}

static ER ad_tc358840_set_tx_start(UINT32 chip_id)
{
	ER rt = E_OK;

	// Start Video TX
	rt |= ad_tc358840_i2c_write(chip_id, 0x0004, 0x8CF5, 2); // ConfCtl0
	rt |= ad_tc358840_i2c_write(chip_id, 0x0006, 0x0000, 2); // ConfCtl1

	return rt;
}

static ER ad_tc358840_clr_int_sys(UINT32 chip_id)
{
	AD_TC358840_INT_STATUS_REG int_sts = {0};
	ER rt = E_OK;

	int_sts.bit.IR_DINT = 1;
	int_sts.bit.IR_EINT = 1;
	int_sts.bit.CEC_RINT = 1;
	int_sts.bit.CEC_TINT = 1;
	int_sts.bit.CEC_EINT = 1;
	int_sts.bit.SYS_INT = 1;
	int_sts.bit.CSITX0_INT = 1;
	int_sts.bit.HDMI_INT = 1;
	int_sts.bit.AMUTE_INT = 1;

	rt |= ad_tc358840_i2c_write(chip_id, AD_TC358840_INT_STATUS_REG_OFS, int_sts.reg, AD_TC358840_INT_STATUS_REG_LEN);

	return rt;
}

static ER ad_tc358840_clr_sys_int(UINT32 chip_id)
{
	AD_TC358840_SYS_INT_REG sys_int = {0};
	ER rt = E_OK;

	sys_int.bit.I_DDC = 1;
	sys_int.bit.I_TMDS = 1;
	sys_int.bit.I_DPMBDET = 1;
	sys_int.bit.I_NOPMBDET = 1;
	sys_int.bit.I_HDMI = 1;
	sys_int.bit.I_DVI = 1;
	sys_int.bit.I_ACRN = 1;
	sys_int.bit.I_ACR_CTS = 1;

	rt |= ad_tc358840_i2c_write(chip_id, AD_TC358840_SYS_INT_REG_OFS, sys_int.reg, AD_TC358840_SYS_INT_REG_LEN);

	return rt;
}

static ER ad_tc358840_chk_connect(UINT32 chip_id, BOOL *connect)
{
	AD_TC358840_SYS_STATUS_REG sys_sts = {0};
	ER rt = E_OK;

	if (ad_tc358840_i2c_read(chip_id, AD_TC358840_SYS_STATUS_REG_OFS, &sys_sts.reg, AD_TC358840_SYS_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get system status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk connect] S_SYNC_ %u\r\n", sys_sts.bit.S_SYNC_);

	if (sys_sts.bit.S_SYNC_ == 0) {
		goto skip;
	}

	rt |= ad_tc358840_set_tx_start(chip_id);

	// clear status
	rt |= ad_tc358840_clr_int_sys(chip_id);
	rt |= ad_tc358840_clr_sys_int(chip_id);
	*connect = TRUE;
	return rt;

skip:
	*connect = FALSE;
	return rt;
}

static ER ad_tc358840_chk_disconnect(UINT32 chip_id, BOOL *disconnect)
{
#if 0
	AD_TC358840_INT_STATUS_REG int_sys = {0};
#endif
	AD_TC358840_SYS_INT_REG sys_int = {0};
	AD_TC358840_SYS_STATUS_REG sys_sts = {0};
	ER rt = E_OK;

#if 0
	if (ad_tc358840_i2c_read(chip_id, AD_TC358840_INT_STATUS_REG_OFS, &int_sys.reg, AD_TC358840_INT_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get interrupt status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] HDMI_INT %u\r\n", int_sys.bit.HDMI_INT);

	if (int_sys.bit.HDMI_INT == 0) {
		goto skip;
	}
#endif

	if (ad_tc358840_i2c_read(chip_id, AD_TC358840_SYS_INT_REG_OFS, &sys_int.reg, AD_TC358840_SYS_INT_REG_LEN) != E_OK) {
		AD_ERR("get system interrupt fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] I_DDC %u, I_TMDS %u\r\n", sys_int.bit.I_DDC, sys_int.bit.I_TMDS);

	if (sys_int.bit.I_DDC != 0 || sys_int.bit.I_TMDS != 0) {
		goto do_change;
	}

	if (ad_tc358840_i2c_read(chip_id, AD_TC358840_SYS_STATUS_REG_OFS, &sys_sts.reg, AD_TC358840_SYS_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get system status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] S_DDC5V %u, S_TMDS %u\r\n", sys_sts.bit.S_DDC5V, sys_sts.bit.S_TMDS);

	if (sys_sts.bit.S_DDC5V != 0 && sys_sts.bit.S_TMDS != 0) {
		goto skip;
	}

do_change:
	rt = ad_tc358840_common_init(chip_id, AD_TC358840_DFT_LANE_NUM);
	*disconnect = TRUE;
	return rt;

skip:
	rt |= ad_tc358840_clr_int_sys(chip_id);
	rt |= ad_tc358840_clr_sys_int(chip_id);
	*disconnect = FALSE;
	return rt;
}

static ER ad_tc358840_get_interlace(UINT32 chip_id, BOOL *interlace)
{
	AD_TC358840_VI_STATUS1_REG vi_sts = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_VI_STATUS1_REG_OFS, &vi_sts.reg, AD_TC358840_VI_STATUS1_REG_LEN);

	*interlace = (vi_sts.bit.S_V_INTERLACE == TRUE);

	return rt;
}

static ER ad_tc358840_clr_in_hv_measure(UINT32 chip_id)
{
	AD_TC358840_HV_CLR_REG hv_clr = {0};
	ER rt = E_OK;

	hv_clr.bit.H_CLR = 1;
	hv_clr.bit.V_CLR = 1;
	hv_clr.bit.DEH_CLR = 1;
	hv_clr.bit.DEV_CLR = 1;

	rt |= ad_tc358840_i2c_write(chip_id, AD_TC358840_HV_CLR_REG_OFS, hv_clr.reg, AD_TC358840_HV_CLR_REG_LEN);

	return rt;
}

static ER ad_tc358840_get_pxclk(UINT32 chip_id, UINT32 *pxclk)
{
	AD_TC358840_PX_FREQ_REG px_freq = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_PX_FREQ_REG_OFS, &px_freq.reg, AD_TC358840_PX_FREQ_REG_LEN);

	*pxclk = px_freq.bit.PX_FREQ * 8; // pxclk = real_clk(Hz) x 100 / 1000000 (MHz)

	return rt;
}

static ER ad_tc358840_get_h_total_size(UINT32 chip_id, UINT32 *h_total_size)
{
	AD_TC358840_IN_H_SIZE_REG h_size = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_IN_H_SIZE_REG_OFS, &h_size.reg, AD_TC358840_IN_H_SIZE_REG_LEN);

	*h_total_size = h_size.bit.IN_H_SIZE;

	return rt;
}

static ER ad_tc358840_get_v_total_size(UINT32 chip_id, UINT32 *v_total_size)
{
	AD_TC358840_IN_V_SIZE_REG v_size = {0};
	BOOL interlace;
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_IN_V_SIZE_REG_OFS, &v_size.reg, AD_TC358840_IN_V_SIZE_REG_LEN);
	rt |= ad_tc358840_get_interlace(chip_id, &interlace);

	*v_total_size = v_size.bit.IN_V_SIZE;

	if (!interlace) {
		*v_total_size = *v_total_size >> 1;
	}

	return rt;
}

static ER ad_tc358840_get_h_blank_size(UINT32 chip_id, UINT32 *h_blank_size)
{
	AD_TC358840_DE_STR_REG de_str = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_DE_STR_REG_OFS, &de_str.reg, AD_TC358840_DE_STR_REG_LEN);

	*h_blank_size = (de_str.bit.DE_H_START > 0) ? (de_str.bit.DE_H_START-1) : 0;

	return rt;
}

static ER ad_tc358840_get_v_blank_size_prog(UINT32 chip_id, UINT32 *v_blank_size)
{
	AD_TC358840_DE_V_START_A_REG de_v_str = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_DE_V_START_A_REG_OFS, &de_v_str.reg, AD_TC358840_DE_V_START_A_REG_LEN);

	*v_blank_size = (de_v_str.bit.DE_V_START_A > 1) ? ((de_v_str.bit.DE_V_START_A>>1)-1) : 0;

	return rt;
}

static ER ad_tc358840_get_h_active_size(UINT32 chip_id, UINT32 *h_active_size)
{
	AD_TC358840_DE_H_SIZE_REG de_h_size = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_DE_H_SIZE_REG_OFS, &de_h_size.reg, AD_TC358840_DE_H_SIZE_REG_LEN);

	*h_active_size = de_h_size.bit.DE_H_SIZE;

	return rt;
}

static ER ad_tc358840_get_v_active_size(UINT32 chip_id, UINT32 *v_active_size)
{
	AD_TC358840_DE_V_SIZE_REG de_v_size = {0};
	BOOL interlace;
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_DE_V_SIZE_REG_OFS, &de_v_size.reg, AD_TC358840_DE_V_SIZE_REG_LEN);
	rt |= ad_tc358840_get_interlace(chip_id, &interlace);

	*v_active_size = de_v_size.bit.DE_V_SIZE;

	if (interlace) {
		*v_active_size = *v_active_size << 1;
	}

	return rt;
}

static ER ad_tc358840_get_fps(UINT32 chip_id, UINT32 *fps)
{
	UINT32 pxclk = 0, h_tol = 0, v_tol = 0;
	BOOL interlace = 0;

	if (ad_tc358840_get_pxclk(chip_id, &pxclk) != E_OK ||
		ad_tc358840_get_h_total_size(chip_id, &h_tol) != E_OK ||
		ad_tc358840_get_v_total_size(chip_id, &v_tol) != E_OK ||
		ad_tc358840_get_interlace(chip_id, &interlace) != E_OK) {
		AD_ERR("get fps fail!\r\n");
		return E_SYS;
	}

	if (h_tol == 0 || v_tol == 0) {
		AD_ERR("detect zero value. h_tol(%u) v_tol(%u)\r\n", h_tol, v_tol);
		return E_SYS;
	}

	*fps = (pxclk * (1000000 / h_tol)) / v_tol; // fps = real_fps x 100

	if (interlace) {
		*fps = *fps << 1;
	}

	return E_OK;
}

static ER ad_tc358840_get_422(UINT32 chip_id, BOOL *_422)
{
	AD_TC358840_VI_STATUS1_REG vi_sts = {0};
	ER rt = E_OK;

	rt |= ad_tc358840_i2c_read(chip_id, AD_TC358840_VI_STATUS1_REG_OFS, &vi_sts.reg, AD_TC358840_VI_STATUS1_REG_LEN);

	*_422 = (vi_sts.bit.S_V_422 == TRUE);

	return rt;
}

static ER ad_tc358840_set_color_space_conv(UINT32 chip_id)
{
#if 0
	UINT32 color_spa, fmt_conv;

	if (unlikely(ad_tc358840_i2c_read(chip_id, 0x8528, &color_spa, 2))) { // 0x8528[4:0] Input video color space
		AD_ERR("[%s id%d] get color space info fail\r\n", AD_TC358840_MODULE_NAME, chip_id);
		return E_SYS;
	}

	if (color_spa & 0x2) { // YCbCr format
		AD_DUMP("[%s id%d] Color format: YCbCr\r\n", AD_TC358840_MODULE_NAME, chip_id);
		fmt_conv = 0x80;
	} else { // RGB format
		AD_DUMP("[%s id%d] Color format: RGB\r\n", AD_TC358840_MODULE_NAME, chip_id);
		fmt_conv = 0x81;
	}

	if (unlikely(ad_tc358840_i2c_write(chip_id, 0x8573, fmt_conv, 2))) { // 0x8573[1:0] RGB888 to YUV422 conversion mode setting
		AD_ERR("[%s id%d] set format conversion fail\r\n", AD_TC358840_MODULE_NAME, chip_id);
		return E_SYS;
	}
#endif
	return E_OK;
}

#if 0
static ER ad_tc358840_cal_nearest_fps(UINT32 vd_cyc, UINT32 *fps_rt)
{
#define AD_TC358840_FPS_DFT 30
#define AD_TC358840_ACCURACY 5
#define AD_TC358840_CAL_FPS(c) ((10000*AD_TC358840_ACCURACY) / (c))
#define AD_TC358840_CAL_FPS_ERR(SOURCE_FPS, TARGET_FPS) (((TARGET_FPS)*AD_TC358840_ACCURACY > (SOURCE_FPS)) ? ((TARGET_FPS)*AD_TC358840_ACCURACY - (SOURCE_FPS)) : ((SOURCE_FPS) - (TARGET_FPS)*AD_TC358840_ACCURACY))
	UINT32 fps_supported_tbl[] = {24, 25, 30, 50, 60};
	UINT32 tbl_size;
	UINT32 fps;
	UINT32 min_err = 0xffffffff, min_err_tmp;
	UINT32 i;

	*fps_rt = AD_TC358840_FPS_DFT;

	if (vd_cyc == 0) {
		AD_WRN("frame interval = 0\r\n");
		return E_SYS;
	}

	fps = AD_TC358840_CAL_FPS(vd_cyc);

	if (sizeof(fps_supported_tbl) == 0) {
		AD_ERR("find no fps\r\n");
		return E_SYS;
	}
	tbl_size = sizeof(fps_supported_tbl) / sizeof(typeof(fps_supported_tbl[0]));

	for (i = 0; i < tbl_size; i++) {
		min_err_tmp = AD_TC358840_CAL_FPS_ERR(fps, fps_supported_tbl[i]);
		//AD_DUMP("c=%u, tar_fps=%u, err=%u\r\n", vd_cyc, fps_supported_tbl[i], min_err_tmp);
		if (min_err_tmp < min_err) {
			min_err = min_err_tmp;
			*fps_rt = fps_supported_tbl[i];
		}
	}

	return E_OK;
}
#endif

// implementation of video mode converting
/*static ER ad_tc358840_det_mode_to_vin_mode(AD_TC358840_DET_RAW_MODE raw_mode, AD_TC358840_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358840_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358840_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_tc358840_det_map[i_mode].raw_mode, sizeof(AD_TC358840_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_tc358840_det_map[i_mode].vin_mode, sizeof(AD_TC358840_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown raw mode %u\r\n", AD_TC358840_MODULE_NAME, raw_mode);
	return E_SYS;
}*/

/*static ER ad_tc358840_vin_mode_to_ch_mode(AD_TC358840_VIN_MODE vin_mode, AD_TC358840_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358840_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358840_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_tc358840_vin_map[i_mode].vin_mode, sizeof(AD_TC358840_VIN_MODE)) == 0) {
			*ch_mode = g_ad_tc358840_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_TC358840_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}*/

/*static ER ad_tc358840_vout_mode_to_port_mode(AD_TC358840_VOUT_MODE vout_mode, AD_TC358840_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358840_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358840_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_tc358840_vout_map[i_mode].vout_mode, sizeof(AD_TC358840_VOUT_MODE)) == 0) {
			port_mode = &g_ad_tc358840_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_TC358840_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_tc358840_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_TC358840_DET_VIDEO_INFO *video_info)
{
	AD_TC358840_INFO *ad_private_info;
	UINT32 width = 0, height = 0, fps = 0;
	BOOL chk = 0, interlace = 0;

	if (unlikely(chip_id >= AD_TC358840_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_TC358840_MODULE_NAME, chip_id, AD_TC358840_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_TC358840_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_TC358840_MODULE_NAME, vin_id, AD_TC358840_VIN_MAX);
		return E_SYS;
	}

	ad_private_info = ad_tc358840_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det video info fail. NULL ad private info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	if (ad_private_info->connect && unlikely(ad_tc358840_chk_disconnect(chip_id, &chk) == E_OK)) {

		if (chk) {
			ad_private_info->connect = FALSE;
			goto do_vloss;
		}

	} else if (!ad_private_info->connect && unlikely(ad_tc358840_chk_connect(chip_id, &chk) == E_OK)) {

		if (chk) {
			ad_private_info->connect = TRUE;

		} else {
			goto do_vloss;
		}

	} else {
		goto do_vloss;
	}

	if (unlikely(ad_tc358840_clr_in_hv_measure(chip_id) != E_OK)) {
		AD_ERR("[%s] ad det video info fail. clr status fail\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	vos_util_delay_ms(100);

	if (unlikely(ad_tc358840_get_h_active_size(chip_id, &width) != E_OK ||
				ad_tc358840_get_v_active_size(chip_id, &height) != E_OK ||
				ad_tc358840_get_fps(chip_id, &fps) != E_OK ||
				ad_tc358840_get_interlace(chip_id, &interlace) != E_OK)) {
		AD_ERR("[%s] ad det video info fail. get info fail\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	ad_tc358840_get_422(chip_id, &ad_private_info->_422);
	ad_tc358840_get_pxclk(chip_id, &ad_private_info->pxclk);
	ad_tc358840_get_h_total_size(chip_id, &ad_private_info->h_total);
	ad_tc358840_get_v_total_size(chip_id, &ad_private_info->v_total);
	ad_tc358840_get_h_blank_size(chip_id, &ad_private_info->h_blank);
	ad_tc358840_get_v_blank_size_prog(chip_id, &ad_private_info->v_blank);

	video_info->vloss = FALSE;
	video_info->width = width;
	video_info->height = height;
	video_info->fps = fps;
	video_info->prog = !interlace;
	return E_OK;

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_TC358840_DFT_WIDTH;
	video_info->height = AD_TC358840_DFT_HEIGHT;
	video_info->fps = AD_TC358840_DFT_FPS;
	video_info->prog = AD_TC358840_DFT_PROG;
	return E_OK;
}

static ER ad_tc358840_dbg_dump_help(UINT32 chip_id)
{
	AD_DUMP("\r\n=================================== AD %s DUMP HELP BEGIN (CHIP %u) ===================================\r\n", AD_TC358840_MODULE_NAME, chip_id);

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

	AD_DUMP("\r\n=================================== AD %s DUMP HELP END ===================================\r\n", AD_TC358840_MODULE_NAME);

	return E_OK;
}

static ER ad_tc358840_dbg_dump_info(UINT32 chip_id)
{
#define GET_STR(strary, idx) ((UINT32)(idx) < sizeof(strary)/sizeof(typeof(strary[0])) ? strary[idx] : "Unknown")
	char _422_str[][4] = {
		"420",
		"422",
	};
	AD_TC358840_INFO *ad_private_info;

	ad_private_info = ad_tc358840_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad dump info fail. NULL ad private info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO BEGIN (CHIP %u) ===================================\r\n", AD_TC358840_MODULE_NAME, chip_id);

	AD_DUMP("%8s %8s %8s %8s %8s %8s %8s\r\n",
		"connect", "422", "pxclk", "h_total", "v_total", "h_blank", "v_blank");
	AD_DUMP("==================================================================================================\r\n");
	AD_DUMP("%8u %8s %8u %8u %8u %8u %8u\r\n", ad_private_info->connect,
		GET_STR(_422_str, ad_private_info->_422), ad_private_info->pxclk, ad_private_info->h_total,
		ad_private_info->v_total, ad_private_info->h_blank, ad_private_info->v_blank);

	AD_DUMP("\r\n=================================== AD %s DUMP INFO END ===================================\r\n", AD_TC358840_MODULE_NAME);

	AD_DUMP("\r\n");

	return E_OK;
}

#if 0
#endif

static ER ad_tc358840_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_tc358840_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	ad_tc358840_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_TC358840_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_TC358840_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_TC358840_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_TC358840_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_TC358840_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_REG;
	reg_info.tbl_num = sizeof(g_ad_tc358840_reg_tbl) / sizeof(typeof(g_ad_tc358840_reg_tbl[0]));
	reg_info.tbl = g_ad_tc358840_reg_tbl;
	reg_info.dump_per_row = AD_TC358840_REG_DUMP_PER_ROW;
	drv_open_info.dbg_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	return E_OK;
}

static ER ad_tc358840_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	return E_OK;
}

static ER ad_tc358840_init(UINT32 chip_id, void *ext_data)
{
	ER rt = E_OK;

	AD_DUMP("id%d, AD_TC358840 init...\r\n", chip_id);

	rt |= ad_tc358840_common_init(chip_id, AD_TC358840_DFT_LANE_NUM);
	rt |= ad_tc358840_set_color_space_conv(chip_id);

	AD_DUMP("id%d, AD_TC358840 init done.\r\n", chip_id);

	g_ad_tc358840_info[chip_id].status |= AD_STS_INIT;

	return rt;
}

static ER ad_tc358840_uninit(UINT32 chip_id, void *ext_data)
{
	g_ad_tc358840_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_tc358840_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_tc358840_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
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
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_TC358840_MODULE_NAME);
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
			mode_info->data_rate = 148500000;
			mode_info->mipi_lane_num = ad_info->vout[mode_info->out_id].data_lane_num;
			mode_info->protocal = AD_DATA_PROTCL_CCIR601;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TC358840_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_TC358840_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tc358840_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_tc358840_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TC358840_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_TC358840_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tc358840_chgmode(UINT32 chip_id, void *ext_data)
{
#define AD_TC358840_CHK_CONNECT_MAX 100
#define AD_TC358840_CHK_CONNECT_DLY 10
	AD_INFO *ad_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vout_id, cnt = 0, width, height, fps, lane_num;
	BOOL connect = FALSE;
	ER rt = E_OK;

	ad_info = ad_tc358840_get_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	vout_id = chgmode_info->vout_id;

	if (unlikely(vout_id >= AD_TC358840_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_TC358840_MODULE_NAME, vout_id, AD_TC358840_VOUT_MAX);
		return E_SYS;
	}

	rt |= ad_tc358840_common_init(chip_id, ad_info->vout[vout_id].data_lane_num);

	// after common init, need wait until system status stable to prevent immediately check disconnect
	while ((!connect) && (cnt < AD_TC358840_CHK_CONNECT_MAX)) {
		if (ad_tc358840_chk_connect(chip_id, &connect) != E_OK) {
			AD_ERR("[%s] ad chgmode fail. chk connect fail\r\n", AD_TC358840_MODULE_NAME);
			return E_SYS;
		}
		vos_util_delay_ms(AD_TC358840_CHK_CONNECT_DLY);
		cnt += AD_TC358840_CHK_CONNECT_DLY;
	}

	if (!connect) {
		AD_WRN("[%s] ad chgmode wrn. disconnect after chgmode (chk %ums)\r\n", AD_TC358840_MODULE_NAME, AD_TC358840_CHK_CONNECT_MAX);
	}

	rt |= ad_tc358840_set_color_space_conv(chip_id);

	width = chgmode_info->ui_info.width;
	height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	fps = chgmode_info->ui_info.fps;
	lane_num = ad_info->vout[vout_id].data_lane_num;

	if (lane_num == 0) {
		AD_WRN("[%s] ad chgmode wrn. lane_num == 0\r\n", AD_TC358840_MODULE_NAME);
	} else if ((width * height * (fps/100) / lane_num) > (1920 * 1080 * 15)) {
		AD_WRN("[%s] ad chgmode wrn. data rate (%u x %u x %u / %u) exceed max (%u)\r\n", AD_TC358840_MODULE_NAME,
			width, height, (fps/100), lane_num, (1920 * 1080 * 15));
	}

	return rt;
}

static ER ad_tc358840_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TC358840_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_tc358840_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_TC358840_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_tc358840_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_tc358840_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_tc358840_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_TC358840_MODULE_NAME, i_vin);
		return E_SYS;
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height;
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
	//DBG_ERR("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d\r\n", i_vin, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog);

	return E_OK;
}

static ER ad_tc358840_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_tc358840_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_tc358840_dbg(char *str_cmd)
{
#define AD_TC358840_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_TC358840_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_TC358840_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_TC358840_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	//UINT32 narg[AD_TC358840_DBG_CMD_MAX] = {0};
	UINT32 /*val = 0, val2 = 0,*/ i_chip = 0, i_vin;
	AD_TC358840_DET_VIDEO_INFO det_info = {0};

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_TC358840_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_TC358840_DBG_CMD_MAX);

	if (cmd_num == 0 || strcmp(cmd_list[0], "") == 0 || strcmp(cmd_list[0], "help") == 0 || strcmp(cmd_list[0], "?") == 0) {
		ad_tc358840_dbg_dump_help(i_chip);

	} else if (strcmp(cmd_list[0], "dumpinfo") == 0) {
		ad_tc358840_dbg_dump_info(i_chip);

	} else if (strcmp(cmd_list[0], "det") == 0) {
		for (i_vin = 0; i_vin < AD_TC358840_VIN_MAX; i_vin++) {
			ad_tc358840_det_video_info(i_chip, i_vin, &det_info);
			g_ad_tc358840_info[i_chip].vin[i_vin].vloss = det_info.vloss;
			g_ad_tc358840_info[i_chip].vin[i_vin].det_format.width = det_info.width;
			g_ad_tc358840_info[i_chip].vin[i_vin].det_format.height = det_info.height;
			g_ad_tc358840_info[i_chip].vin[i_vin].det_format.fps = det_info.fps;
			g_ad_tc358840_info[i_chip].vin[i_vin].det_format.prog = det_info.prog;
		}

	} else if (strcmp(cmd_list[0], "init") == 0) {
		ad_tc358840_common_init(i_chip, AD_TC358840_DFT_LANE_NUM);
	} else if (strcmp(cmd_list[0], "start") == 0) {
		ad_tc358840_set_tx_start(i_chip);
	}

	return E_OK;
}

AD_DEV* ad_get_obj_tc358840(void)
{
	return &g_ad_tc358840_obj;
}

