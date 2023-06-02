#include "ad_tc358743.h"
#include "nvt_ad_tc358743_reg.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 02, 003, 00);

// mapping table: vin raw data -> meaningful vin info
/*static AD_TC358743_DET_MAP g_ad_tc358743_det_map[] = {
};*/

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_TC358743_VIN_MAP g_ad_tc358743_vin_map[] = {
	{.vin_mode = {.width =  640, .height =  480, .fps = 6000, .lane_num = 2}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 220}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .lane_num = 2}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 60}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .lane_num = 2}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 60}},
	{.vin_mode = {.width =  640, .height =  480, .fps = 6000, .lane_num = 4}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 272}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .lane_num = 4}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 340}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .lane_num = 4}, .ch_mode = {.fbd = 59, .prd = 2, .fifo_delay = 434}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 6000, .lane_num = 4}, .ch_mode = {.fbd = 69, .prd = 2, .fifo_delay = 60}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_TC358743_VOUT_MAP g_ad_tc358743_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_tc358743_slv_addr[] = {0x1E};

// decoder dump all register information
/*#define AD_TC358743_BANK_ADDR 0xFF
static AD_TC358743_REG_BANK g_ad_tc358743_bank[] = {
};
#define AD_TC358743_REG_DUMP_PER_ROW 16*/

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_TC358743_DFT_WIDTH 1920
#define AD_TC358743_DFT_HEIGHT 1080
#define AD_TC358743_DFT_FPS 6000
#define AD_TC358743_DFT_PROG TRUE
#define AD_TC358743_DFT_LANE_NUM 4

// i2c retry number when operation fail occur
#define AD_TC358743_I2C_RETRY_NUM 5

#define AD_TC358743_REF_CLK 27000000
//#define AD_TC358743_PLL_FBD_DFT 69
//#define AD_TC358743_PLL_PRD_DFT 2
//#define AD_TC358743_FIFO_DLY_DFT 60
#define AD_TC358743_FIFO_DLY_MAX 511

// global variable
#define AD_TC358743_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_tc358743_vin[AD_TC358743_CHIP_MAX][AD_TC358743_VIN_MAX];
static AD_VOUT g_ad_tc358743_vout[AD_TC358743_CHIP_MAX][AD_TC358743_VOUT_MAX];
static AD_INFO g_ad_tc358743_info[AD_TC358743_CHIP_MAX] = {
	{
		.name = "TC358743_0",
		.vin_max = AD_TC358743_VIN_MAX,
		.vin = &g_ad_tc358743_vin[0][0],
		.vout_max = AD_TC358743_VOUT_MAX,
		.vout = &g_ad_tc358743_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358743_1",
		.vin_max = AD_TC358743_VIN_MAX,
		.vin = &g_ad_tc358743_vin[1][0],
		.vout_max = AD_TC358743_VOUT_MAX,
		.vout = &g_ad_tc358743_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358743_2",
		.vin_max = AD_TC358743_VIN_MAX,
		.vin = &g_ad_tc358743_vin[2][0],
		.vout_max = AD_TC358743_VOUT_MAX,
		.vout = &g_ad_tc358743_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TC358743_3",
		.vin_max = AD_TC358743_VIN_MAX,
		.vin = &g_ad_tc358743_vin[3][0],
		.vout_max = AD_TC358743_VOUT_MAX,
		.vout = &g_ad_tc358743_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_TC358743_INFO g_ad_tc358743_info_private[AD_TC358743_CHIP_MAX] = {
	[0 ... (AD_TC358743_CHIP_MAX-1)] = {
		0,
	}
};

static ER ad_tc358743_open(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_close(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_init(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_uninit(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tc358743_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tc358743_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_tc358743_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_tc358743_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_tc358743_dbg(char *str_cmd);

static AD_DEV g_ad_tc358743_obj = {
	AD_TC358743_CHIP_MAX,
	g_ad_tc358743_info,

	sizeof(g_ad_tc358743_slv_addr) / sizeof(typeof(g_ad_tc358743_slv_addr[0])),
	g_ad_tc358743_slv_addr,

	{
		.open = ad_tc358743_open,
		.close = ad_tc358743_close,
		.init = ad_tc358743_init,
		.uninit = ad_tc358743_uninit,
		.get_cfg = ad_tc358743_get_cfg,
		.set_cfg = ad_tc358743_set_cfg,
		.chgmode = ad_tc358743_chgmode,
		.det_plug_in = ad_tc358743_watchdog_cb,
		.i2c_write = ad_tc358743_i2c_write,
		.i2c_read = ad_tc358743_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_tc358743_dbg,
	}
};

/*static AD_TC358743_REG_DUMP_INFO g_ad_tc358743_dumpreg = {
	.bank_addr = AD_TC358743_BANK_ADDR,
	.bank_num = sizeof(g_ad_tc358743_bank) / sizeof(typeof(g_ad_tc358743_bank[0])),
	.bank = &g_ad_tc358743_bank[0],
	.dump_per_row = AD_TC358743_REG_DUMP_PER_ROW,
};*/

// internal function
static AD_INFO *ad_tc358743_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TC358743_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TC358743_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tc358743_info[chip_id];
}

static AD_TC358743_INFO *ad_tc358743_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TC358743_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TC358743_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tc358743_info_private[chip_id];
}

static void ad_tc358743_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_TC358743_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_TC358743_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_TC358743_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_TC358743_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_TC358743_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_TC358743_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_TC358743_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_TC358743_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_TC358743_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_TC358743_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_TC358743_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_TC358743_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

static ER ad_tc358743_clr_int_sys(UINT32 chip_id)
{
	AD_TC358743_INT_STATUS_REG int_sts = {0};
	ER rt = E_OK;

	int_sts.bit.IR_DINT = 1;
	int_sts.bit.IR_EINT = 1;
	int_sts.bit.CEC_RINT = 1;
	int_sts.bit.CEC_TINT = 1;
	int_sts.bit.CEC_EINT = 1;
	int_sts.bit.SYS_INT = 1;
	int_sts.bit.CSI_INT = 1;
	int_sts.bit.HDMI_INT = 1;
	int_sts.bit.AMUTE_INT = 1;

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_INT_STATUS_REG_OFS, int_sts.reg, AD_TC358743_INT_STATUS_REG_LEN);

	return rt;
}

static ER ad_tc358743_clr_sys_int(UINT32 chip_id)
{
	AD_TC358743_SYS_INT_REG sys_int = {0};
	ER rt = E_OK;

	sys_int.bit.I_DDC = 1;
	sys_int.bit.I_TMDS = 1;
	sys_int.bit.I_DPMBDET = 1;
	sys_int.bit.I_NOPMBDET = 1;
	sys_int.bit.I_HDMI = 1;
	sys_int.bit.I_DVI = 1;
	sys_int.bit.I_ACRN = 1;
	sys_int.bit.I_ACR_CTS = 1;

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_SYS_INT_REG_OFS, sys_int.reg, AD_TC358743_SYS_INT_REG_LEN);

	return rt;
}

static ER ad_tc358743_clr_misc_int(UINT32 chip_id)
{
	AD_TC358743_MISC_INT_REG misc_int = {0};
	ER rt = E_OK;

	misc_int.bit.I_AUDIO_MUTE = 1;
	misc_int.bit.I_SYNC_CHG = 1;
	misc_int.bit.I_NO_VS = 1;
	misc_int.bit.I_NO_SPD = 1;
	misc_int.bit.I_AS_LAYOUT = 1;

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_MISC_INT_REG_OFS, misc_int.reg, AD_TC358743_MISC_INT_REG_LEN);

	return rt;
}

static ER ad_tc358743_get_interlace(UINT32 chip_id, BOOL *interlace)
{
	AD_TC358743_VI_STATUS1_REG vi_sts = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_VI_STATUS1_REG_OFS, &vi_sts.reg, AD_TC358743_VI_STATUS1_REG_LEN);

	*interlace = (vi_sts.bit.S_V_INTERLACE == TRUE);

	return rt;
}

static ER ad_tc358743_get_pxclk(UINT32 chip_id, UINT32 *pxclk)
{
	AD_TC358743_PX_FREQ_REG px_freq = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_PX_FREQ_REG_OFS, &px_freq.reg, AD_TC358743_PX_FREQ_REG_LEN);

	*pxclk = px_freq.bit.PX_FREQ * 8; // pxclk = real_clk(Hz) x 100 / 1000000 (MHz)

	return rt;
}

static ER ad_tc358743_get_h_total_size(UINT32 chip_id, UINT32 *h_total_size)
{
	AD_TC358743_H_TOTAL_REG h_size = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_H_TOTAL_REG_OFS, &h_size.reg, AD_TC358743_H_TOTAL_REG_LEN);

	*h_total_size = h_size.bit.H_TOTAL;

	return rt;
}

static ER ad_tc358743_get_v_total_size(UINT32 chip_id, UINT32 *v_total_size)
{
	AD_TC358743_V_TOTAL_REG v_size = {0};
	BOOL interlace = 0;
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_V_TOTAL_REG_OFS, &v_size.reg, AD_TC358743_V_TOTAL_REG_LEN);
	rt |= ad_tc358743_get_interlace(chip_id, &interlace);

	*v_total_size = v_size.bit.V_TOTAL;

	if (!interlace) {
		*v_total_size = *v_total_size >> 1;
	}

	return rt;
}

static ER ad_tc358743_get_h_blank_size(UINT32 chip_id, UINT32 *h_blank_size)
{
	AD_TC358743_H_STR_REG de_str = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_H_STR_REG_OFS, &de_str.reg, AD_TC358743_H_STR_REG_LEN);

	*h_blank_size = (de_str.bit.H_STR > 0) ? (de_str.bit.H_STR-1) : 0;

	return rt;
}

static ER ad_tc358743_get_v_blank_size_prog(UINT32 chip_id, UINT32 *v_blank_size)
{
	AD_TC358743_V_STR_A_REG de_v_str = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_V_STR_A_REG_OFS, &de_v_str.reg, AD_TC358743_V_STR_A_REG_LEN);

	*v_blank_size = (de_v_str.bit.V_STR_A > 1) ? ((de_v_str.bit.V_STR_A>>1)-1) : 0;

	return rt;
}

static ER ad_tc358743_get_h_active_size(UINT32 chip_id, UINT32 *h_active_size)
{
	AD_TC358743_H_ACT_REG de_h_size = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_H_ACT_REG_OFS, &de_h_size.reg, AD_TC358743_H_ACT_REG_LEN);

	*h_active_size = de_h_size.bit.H_ACT;

	return rt;
}

static ER ad_tc358743_get_v_active_size(UINT32 chip_id, UINT32 *v_active_size)
{
	AD_TC358743_V_ACT_REG de_v_size = {0};
	BOOL interlace = 0;
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_V_ACT_REG_OFS, &de_v_size.reg, AD_TC358743_V_ACT_REG_LEN);
	rt |= ad_tc358743_get_interlace(chip_id, &interlace);

	*v_active_size = de_v_size.bit.V_ACT;

	if (interlace) {
		*v_active_size = *v_active_size << 1;
	}

	return rt;
}

static ER ad_tc358743_get_fps_nearest(UINT32 fps, UINT32 *fps_out)
{
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define DIST(x, y) (MAX((x), (y)) - MIN((x), (y)))
	static UINT32 sup_fps_tbl[] = { 2500, 3000, 5000, 6000 };
	UINT32 i, err, min_err = 0xffffffff, min_i = 0;

	for (i = 0; i < sizeof(sup_fps_tbl)/sizeof(sup_fps_tbl[0]); i++) {

		err = DIST(fps, sup_fps_tbl[i]);

		AD_IND("fps%u sup_fps%u => err %u, min_err %u\r\n", fps, sup_fps_tbl[i], err, min_err);

		if (err < min_err) {
			AD_IND("min_i = %u\r\n", i);
			min_err = err;
			min_i = i;
		}
	}

	*fps_out = sup_fps_tbl[min_i];

	return E_OK;
}

static ER ad_tc358743_get_fps(UINT32 chip_id, UINT32 *fps)
{
	UINT32 pxclk = 0, h_tol = 0, v_tol = 0;
	BOOL interlace = 0;

	if (ad_tc358743_get_pxclk(chip_id, &pxclk) != E_OK ||
		ad_tc358743_get_h_total_size(chip_id, &h_tol) != E_OK ||
		ad_tc358743_get_v_total_size(chip_id, &v_tol) != E_OK ||
		ad_tc358743_get_interlace(chip_id, &interlace) != E_OK) {
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

static ER ad_tc358743_get_422(UINT32 chip_id, BOOL *_422)
{
	AD_TC358743_VI_STATUS1_REG vi_sts = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_VI_STATUS1_REG_OFS, &vi_sts.reg, AD_TC358743_VI_STATUS1_REG_LEN);

	*_422 = (vi_sts.bit.S_V_422 == TRUE);

	return rt;
}

static ER ad_tc358743_get_color_space(UINT32 chip_id, BOOL *is_rgb)
{
	AD_TC358743_VI_STATUS3_REG vi_sts = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_VI_STATUS3_REG_OFS, &vi_sts.reg, AD_TC358743_VI_STATUS3_REG_LEN);

	*is_rgb = ((vi_sts.bit.S_V_COLOR & (1 << 1)) == 0);

	AD_IND("%s mode detected\r\n", *is_rgb ? "RGB" : "YUV");

	return rt;
}

static ER ad_tc358743_set_color_mode(UINT32 chip_id, BOOL rgb_in)
{
	AD_TC358743_VOUT_SET2_REG vout_set2 = {0};
	ER rt = E_OK;

	rt |= ad_tc358743_i2c_read(chip_id, AD_TC358743_VOUT_SET2_REG_OFS, &vout_set2.reg, AD_TC358743_VOUT_SET2_REG_LEN);

	if (rgb_in) {
		vout_set2.bit.VOUT_COLOR_MODE = 1;
	} else {
		vout_set2.bit.VOUT_COLOR_MODE = 0;
	}

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_VOUT_SET2_REG_OFS, vout_set2.reg, AD_TC358743_VOUT_SET2_REG_LEN);

	return rt;
}

static ER ad_tc358743_set_mipi_data_lane(UINT32 chip_id, UINT32 lane_num)
{
	AD_TC358743_D0W_CNTRL_REG d0w_ctrl = {0};
	AD_TC358743_D1W_CNTRL_REG d1w_ctrl = {0};
	AD_TC358743_D2W_CNTRL_REG d2w_ctrl = {0};
	AD_TC358743_D3W_CNTRL_REG d3w_ctrl = {0};
	AD_TC358743_HSTXVREGEN_REG hstxvregen = {0};
	ER rt = E_OK;

	AD_IND("SET MIPI DATA LANE (%u)\r\n", lane_num);

	d0w_ctrl.bit.D0W_LANE_DISABLE = (lane_num < 1);
	d1w_ctrl.bit.D1W_LANE_DISABLE = (lane_num < 2);
	d2w_ctrl.bit.D2W_LANE_DISABLE = (lane_num < 3);
	d3w_ctrl.bit.D3W_LANE_DISABLE = (lane_num < 4);

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_D0W_CNTRL_REG_OFS, d0w_ctrl.reg, AD_TC358743_D0W_CNTRL_REG_LEN);
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_D1W_CNTRL_REG_OFS, d1w_ctrl.reg, AD_TC358743_D1W_CNTRL_REG_LEN);
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_D2W_CNTRL_REG_OFS, d2w_ctrl.reg, AD_TC358743_D2W_CNTRL_REG_LEN);
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_D3W_CNTRL_REG_OFS, d3w_ctrl.reg, AD_TC358743_D3W_CNTRL_REG_LEN);

	hstxvregen.bit.CLM_HSTXVREGEN = 1;
	hstxvregen.bit.D0M_HSTXVREGEN = (lane_num >= 1);
	hstxvregen.bit.D1M_HSTXVREGEN = (lane_num >= 2);
	hstxvregen.bit.D2M_HSTXVREGEN = (lane_num >= 3);
	hstxvregen.bit.D3M_HSTXVREGEN = (lane_num >= 4);

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_HSTXVREGEN_REG_OFS, hstxvregen.reg, AD_TC358743_HSTXVREGEN_REG_LEN);

	return rt;
}

static ER ad_tc358743_get_fifo_delay_val(UINT32 chip_id, UINT32 lane_num, UINT32 fbd, UINT32 prd, UINT32 *fifo_delay_val)
{
	UINT32 hact, pxclk, nbit, csi_speed, csi_lane, fifo_delay, tmp1, tmp2;
	ER rt = E_OK;

	rt |= ad_tc358743_get_h_active_size(chip_id, &hact);
	rt |= ad_tc358743_get_pxclk(chip_id, &pxclk);
	nbit = 16; // YCbCr422-16bit
	csi_speed = ((AD_TC358743_REF_CLK/1000000) / (prd+1)) * (fbd+1);
	csi_lane = lane_num;

	if (pxclk < 100 || csi_speed == 0 || csi_lane == 0) {
		AD_ERR("Set fifo delay fail. Wrong arg pxclk(%u) csi_speed(%u) csi_lane(%u)\r\n", pxclk, csi_speed, csi_lane);
		return E_SYS;
	}

	AD_IND("hact %u pxclk %u nbit %u csi_speed %u csi_lane %u\r\n", hact, pxclk/100, nbit, csi_speed, csi_lane);

	tmp1 = 100 * hact / (pxclk/100);
	tmp2 = (100 * hact * nbit) / (csi_speed * csi_lane);
	if (tmp1 <= tmp2) {
		fifo_delay = 16;
	} else {
		fifo_delay = ((tmp1 - tmp2) * (pxclk/100) * nbit) / 32 / 100;
	}

	if (fifo_delay & 1) { // prevent odd value
		if (fifo_delay == AD_TC358743_FIFO_DLY_MAX) { // prevent exceed max value
			fifo_delay = fifo_delay - 1;
		} else {
			fifo_delay = fifo_delay + 1;
		}
	}

	*fifo_delay_val = fifo_delay;

	AD_IND("FIFO DELAY %u\r\n", fifo_delay);

	return rt;
}

static ER ad_tc358743_set_fifo_delay(UINT32 chip_id, UINT32 fifo_delay)
{
	AD_TC358743_FIFO_CTL_REG fifo_ctrl = {0};
	ER rt = E_OK;

	if (fifo_delay > ((1 << (9+1)) - 1)) {
		AD_WRN("FIFO DELAY (%u) exceed max (%u). force set to max\r\n", fifo_delay, ((1 << (9+1)) - 1));
		fifo_delay = ((1 << (9+1)) - 1);
	}

	fifo_ctrl.bit.FIFO_LEVEL = fifo_delay;

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_FIFO_CTL_REG_OFS, fifo_ctrl.reg, AD_TC358743_FIFO_CTL_REG_LEN);

	return rt;
}

static ER ad_tc358743_set_tx_start(UINT32 chip_id)
{
	AD_TC358743_CONF_CTL_REG conf_ctl = {0};
	ER rt = E_OK;

	AD_IND("SET TX\r\n");

	// Start Video and Audio
	conf_ctl.bit.V_BUF_EN = 1;
	conf_ctl.bit.A_BUF_EN = 1;
	conf_ctl.bit.AUTO_INDEX = 1;
	conf_ctl.bit.AUD_OUT_SEL = 2;
	conf_ctl.bit.IN_FRM_EN = 1;
	conf_ctl.bit.Y_CB_CR_FMT = 3;
	conf_ctl.bit.I2S_DLY_OPT = 0;
	conf_ctl.bit.AUD_CH_SEL = 0;
	conf_ctl.bit.AUD_CH_NUM = 3;
	conf_ctl.bit.A_CLK_OPT = 0;
	conf_ctl.bit.PWR_ISO = 0;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_CONF_CTL_REG_OFS, conf_ctl.reg, AD_TC358743_CONF_CTL_REG_LEN);

	return rt;
}

static ER ad_tc358743_set_csi_clk(UINT32 chip_id, UINT32 fbd, UINT32 prd)
{
	AD_TC358743_PLL_CTL_0_REG pll_ctl0 = {0};
	ER rt = E_OK;

	AD_IND("SET CSI CLK\r\n");

	if (fbd > ((1 << (9+1)) - 1)) {
		AD_WRN("FBD (%u) exceed max (%u). force set to max\r\n", fbd, ((1 << (9+1)) - 1));
		fbd = ((1 << (9+1)) - 1);
	}

	if (prd > ((1 << (4+1)) - 1)) {
		AD_WRN("PRD (%u) exceed max (%u). force set to max\r\n", prd, ((1 << (4+1)) - 1));
		prd = ((1 << (4+1)) - 1);
	}

	// CSI speed = (REFCLK / (PLL_PRD+1)) x (PLL_FBD+1). REFCLK = 27M
	pll_ctl0.bit.PLL_FBD = fbd;
	pll_ctl0.bit.PLL_PRD = prd;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_PLL_CTL_0_REG_OFS, pll_ctl0.reg, AD_TC358743_PLL_CTL_0_REG_LEN);

	return rt;
}

static ER ad_tc358743_reset(UINT32 chip_id, BOOL mipi_only)
{
	AD_TC358743_CONF_CTL_REG conf_ctl = {0};
	AD_TC358743_SYS_CTL_REG sys_ctl = {0};
	//AD_TC358743_PLL_CTL_0_REG pll_ctl0 = {0};
	AD_TC358743_PLL_CTL_1_REG pll_ctl1 = {0};
	ER rt = E_OK;

	// Stop Video and Audio
	conf_ctl.bit.V_BUF_EN = 0;
	conf_ctl.bit.A_BUF_EN = 0;
	conf_ctl.bit.AUTO_INDEX = 1;
	conf_ctl.bit.AUD_OUT_SEL = 2;
	conf_ctl.bit.IN_FRM_EN = 1;
	conf_ctl.bit.Y_CB_CR_FMT = 3;
	conf_ctl.bit.I2S_DLY_OPT = 0;
	conf_ctl.bit.AUD_CH_SEL = 0;
	conf_ctl.bit.AUD_CH_NUM = 3;
	conf_ctl.bit.A_CLK_OPT = 0;
	conf_ctl.bit.PWR_ISO = 0;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_CONF_CTL_REG_OFS, conf_ctl.reg, AD_TC358743_CONF_CTL_REG_LEN);

	// Pull reset
	if (mipi_only) {
		sys_ctl.bit.SLEEP = 1;
		sys_ctl.bit.HDMI_RST = 0;
		sys_ctl.bit.C_TX_RST = 1;
		sys_ctl.bit.CEC_RST = 0;
		sys_ctl.bit.IR_RST = 0;

	} else {
		sys_ctl.bit.SLEEP = 1;
		sys_ctl.bit.HDMI_RST = 1;
		sys_ctl.bit.C_TX_RST = 1;
		sys_ctl.bit.CEC_RST = 1;
		sys_ctl.bit.IR_RST = 1;
	}
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_SYS_CTL_REG_OFS, sys_ctl.reg, AD_TC358743_SYS_CTL_REG_LEN);

	// CSI speed = (REFCLK / (PLL_PRD+1)) x (PLL_FBD+1) = (27M / 3) x 70 = 630M
	/*pll_ctl0.bit.PLL_FBD = AD_TC358743_PLL_FBD_DFT;
	pll_ctl0.bit.PLL_PRD = AD_TC358743_PLL_PRD_DFT;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_PLL_CTL_0_REG_OFS, pll_ctl0.reg, AD_TC358743_PLL_CTL_0_REG_LEN);*/

	// Toggle clock enable
	pll_ctl1.bit.PLL_EN = 1;
	pll_ctl1.bit.PLL_RESERB = 1;
	pll_ctl1.bit.PLL_CKEN = 0;
	pll_ctl1.bit.PLL_BYPCKEN = 0;
	pll_ctl1.bit.PLL_LFBREN = 0;
	pll_ctl1.bit.PLL_LBWS = 2;
	pll_ctl1.bit.PLL_FRS = 0;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_PLL_CTL_1_REG_OFS, pll_ctl1.reg, AD_TC358743_PLL_CTL_1_REG_LEN);

	vos_util_delay_us(10);

	pll_ctl1.bit.PLL_CKEN = 1;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_PLL_CTL_1_REG_OFS, pll_ctl1.reg, AD_TC358743_PLL_CTL_1_REG_LEN);

	// Release reset
	sys_ctl.bit.SLEEP = 0;
	sys_ctl.bit.HDMI_RST = 0;
	sys_ctl.bit.C_TX_RST = 0;
	sys_ctl.bit.CEC_RST = 0;
	sys_ctl.bit.IR_RST = 0;
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_SYS_CTL_REG_OFS, sys_ctl.reg, AD_TC358743_SYS_CTL_REG_LEN);

	vos_util_delay_us(10);

	return rt;
}

static ER ad_tc358743_set_hdmi(UINT32 chip_id)
{
	BOOL is_rgb = FALSE;
	ER rt = E_OK;

	AD_IND("SET HDMI\r\n");

	// Software Reset
	//rt |= ad_tc358743_reset(chip_id, FALSE);
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0F00, 2); // SysCtl
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0000, 2); // SysCtl
	// PLL Setting
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0020, 0x2045, 2); // PLLCtl0
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0022, 0x0203, 2); // PLLCtl1
	//vos_util_delay_us(10);
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0022, 0x0213, 2); // PLLCtl1
	// HDMI Interrupt Control
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0016, 0x073F, 2); // TOP_INTM
	rt |= ad_tc358743_i2c_write(chip_id, 0x0016, 0x053F, 2); // TOP_INTM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8502, 0xFF, 1); // SYS_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x850B, 0x1F, 1); // MISC_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x0014, 0x073F, 2); // TOP_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x8512, 0xFE, 1); // SYS_INTM
	rt |= ad_tc358743_i2c_write(chip_id, 0x851B, 0x1D, 1); // MISC_INTM
	// HDMI PHY
	rt |= ad_tc358743_i2c_write(chip_id, 0x8532, 0x80, 1); // PHY CTL1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8536, 0x40, 1); // PHY_BIAS
	rt |= ad_tc358743_i2c_write(chip_id, 0x853F, 0x0A, 1); // PHY_CSQ
	// HDMI SYSTEM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8543, 0x32, 1); // DDC_CTL
	rt |= ad_tc358743_i2c_write(chip_id, 0x8544, 0x10, 1); // HPD_CTL
	rt |= ad_tc358743_i2c_write(chip_id, 0x8545, 0x31, 1); // ANA_CTL
	rt |= ad_tc358743_i2c_write(chip_id, 0x8546, 0x2D, 1); // AVM_CTL
	// HDCP Setting
	// HDMI Audio REFCLK
	rt |= ad_tc358743_i2c_write(chip_id, 0x8531, 0x01, 1); // PHY_CTL0
	rt |= ad_tc358743_i2c_write(chip_id, 0x8532, 0x80, 1); // PHY_CTL1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8540, 0x8C, 1); // SYS_FREQ0
	rt |= ad_tc358743_i2c_write(chip_id, 0x8541, 0x0A, 1); // SYS_FREQ1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8630, 0xB0, 1); // LOCKDET_REF0
	rt |= ad_tc358743_i2c_write(chip_id, 0x8631, 0x1E, 1); // LOCKDET_REF1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8632, 0x04, 1); // LOCKDET_REF2
	rt |= ad_tc358743_i2c_write(chip_id, 0x8670, 0x01, 1); // NCO_F0_MOD
	// HDMI Audio Setting
	rt |= ad_tc358743_i2c_write(chip_id, 0x8600, 0x00, 1); // AUD_Auto_Mute
	rt |= ad_tc358743_i2c_write(chip_id, 0x8602, 0xF3, 1); // Auto_CMD0
	rt |= ad_tc358743_i2c_write(chip_id, 0x8603, 0x02, 1); // Auto_CMD1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8604, 0x0C, 1); // Auto_CMD2
	rt |= ad_tc358743_i2c_write(chip_id, 0x8606, 0x05, 1); // BUFINIT_START
	rt |= ad_tc358743_i2c_write(chip_id, 0x8607, 0x00, 1); // FS_MUTE
	rt |= ad_tc358743_i2c_write(chip_id, 0x8620, 0x22, 1); // FS_IMODE
	rt |= ad_tc358743_i2c_write(chip_id, 0x8640, 0x01, 1); // ACR_MODE
	rt |= ad_tc358743_i2c_write(chip_id, 0x8641, 0x65, 1); // ACR_MDF0
	rt |= ad_tc358743_i2c_write(chip_id, 0x8642, 0x07, 1); // ACR_MDF1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8652, 0x02, 1); // SDO_MODE1
	rt |= ad_tc358743_i2c_write(chip_id, 0x85AA, 0x50, 1); // FH_MIN0
	rt |= ad_tc358743_i2c_write(chip_id, 0x85AF, 0xC6, 1); // HV_RST
	rt |= ad_tc358743_i2c_write(chip_id, 0x85AB, 0x00, 1); // FH_MIN1
	rt |= ad_tc358743_i2c_write(chip_id, 0x8665, 0x10, 1); // DIV_MODE
	// Info Frame Extraction
	rt |= ad_tc358743_i2c_write(chip_id, 0x8709, 0xFF, 1); // PK_INT_MODE
	rt |= ad_tc358743_i2c_write(chip_id, 0x870B, 0x2C, 1); // NO_PKT_LIMIT
	rt |= ad_tc358743_i2c_write(chip_id, 0x870C, 0x53, 1); // NO_PKT_CLR
	rt |= ad_tc358743_i2c_write(chip_id, 0x870D, 0x01, 1); // ERR_PK_LIMIT
	rt |= ad_tc358743_i2c_write(chip_id, 0x870E, 0x30, 1); // NO_PKT_LIMIT2
	rt |= ad_tc358743_i2c_write(chip_id, 0x9007, 0x10, 1); // NO_GDB_LIMIT

	rt |= ad_tc358743_get_color_space(chip_id, &is_rgb);
	rt |= ad_tc358743_set_color_mode(chip_id, is_rgb);

	return rt;
}

static ER ad_tc358743_set_edid(UINT32 chip_id)
{
	ER rt = E_OK;

	AD_IND("SET EDID\r\n");

	// EDID
	rt |= ad_tc358743_i2c_write(chip_id, 0x85C7, 0x01, 1); // EDID_MODE
	rt |= ad_tc358743_i2c_write(chip_id, 0x85CA, 0x00, 1); // EDID_LEN1
	rt |= ad_tc358743_i2c_write(chip_id, 0x85CB, 0x01, 1); // EDID_LEN2
	// EDID Data
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C00, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C01, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C02, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C03, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C04, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C05, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C06, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C07, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C08, 0x52, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C09, 0x62, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0A, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0B, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0D, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0E, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C0F, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C10, 0x1C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C11, 0x15, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C12, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C13, 0x03, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C14, 0x80, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C15, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C16, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C17, 0x78, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C18, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C19, 0x0D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1A, 0xC9, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1B, 0xA0, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1C, 0x57, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1D, 0x47, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1E, 0x98, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C1F, 0x27, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C20, 0x12, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C21, 0x48, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C22, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C23, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C24, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C25, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C26, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C27, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C28, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C29, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2A, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2B, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2C, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2D, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2E, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C2F, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C30, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C31, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C32, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C33, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C34, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C35, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C36, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C37, 0x1D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C38, 0x80, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C39, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3A, 0x71, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3B, 0x38, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3C, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3D, 0x40, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3E, 0x58, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C3F, 0x2C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C40, 0x45, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C41, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C42, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C43, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C44, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C45, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C46, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C47, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C48, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C49, 0x1D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4A, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4B, 0x72, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4C, 0x51, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4D, 0xD0, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4E, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C4F, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C50, 0x6E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C51, 0x28, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C52, 0x55, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C53, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C54, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C55, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C56, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C57, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C58, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C59, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5A, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5B, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5D, 0xFC, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5E, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C5F, 0x54, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C60, 0x6F, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C61, 0x73, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C62, 0x68, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C63, 0x69, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C64, 0x62, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C65, 0x61, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C66, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C67, 0x48, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C68, 0x32, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C69, 0x43, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6A, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6B, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6C, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6D, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6E, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C6F, 0xFD, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C70, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C71, 0x14, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C72, 0x78, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C73, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C74, 0xFF, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C75, 0x10, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C76, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C77, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C78, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C79, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7A, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7B, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7C, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7D, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7E, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C7F, 0x9D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C80, 0x02, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C81, 0x03, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C82, 0x1A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C83, 0x70, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C84, 0x47, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C85, 0x22, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C86, 0x04, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C87, 0x10, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C88, 0x6E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C89, 0x02, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8A, 0x11, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8B, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8C, 0x23, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8D, 0x09, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8E, 0x07, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C8F, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C90, 0x83, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C91, 0x01, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C92, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C93, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C94, 0x65, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C95, 0x03, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C96, 0x0C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C97, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C98, 0x10, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C99, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9A, 0x02, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9B, 0x3A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9C, 0x80, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9D, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9E, 0x71, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8C9F, 0x38, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA0, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA1, 0x40, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA2, 0x58, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA3, 0x2C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA4, 0x45, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA5, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA6, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA7, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA8, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CA9, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAA, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAB, 0x1E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAC, 0x64, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAD, 0x19, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAE, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CAF, 0x40, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB0, 0x41, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB1, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB2, 0x26, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB3, 0x30, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB4, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB5, 0x88, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB6, 0x36, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB7, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB8, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CB9, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBA, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBB, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBC, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBD, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBE, 0x8C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CBF, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC0, 0xD0, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC1, 0x8A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC2, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC3, 0xE0, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC4, 0x2D, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC5, 0x10, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC6, 0x10, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC7, 0x3E, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC8, 0x96, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CC9, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCA, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCB, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCC, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCD, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCE, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CCF, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD0, 0x8C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD1, 0x0A, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD2, 0xD0, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD3, 0x90, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD4, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD5, 0x40, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD6, 0x31, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD7, 0x20, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD8, 0x0C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CD9, 0x40, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDA, 0x55, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDB, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDC, 0x66, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDD, 0x4C, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDE, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CDF, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE0, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE1, 0x18, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE2, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE3, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE4, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE5, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE6, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE7, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE8, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CE9, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CEA, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CEB, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CEC, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CED, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CEE, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CEF, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF0, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF1, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF2, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF3, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF4, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF5, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF6, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF7, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF8, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CF9, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFA, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFB, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFC, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFD, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFE, 0x00, 1); // EDID_RAM
	rt |= ad_tc358743_i2c_write(chip_id, 0x8CFF, 0xD2, 1); // EDID_RAM

	return rt;
}

static ER ad_tc358743_set_init_end(UINT32 chip_id)
{
	AD_TC358743_INIT_END_REG init_end = {0};
	ER rt = E_OK;

	init_end.bit.INIT_END = 1;

	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_INIT_END_REG_OFS, init_end.reg, AD_TC358743_INIT_END_REG_LEN);

	return rt;
}

static ER ad_tc358743_set_mipi(UINT32 chip_id, UINT32 data_lane)
{
	ER rt = E_OK;

	AD_IND("SET MIPI\r\n");

	if (data_lane < 1 || data_lane > 4) {
		AD_ERR("wrong data lane %u\r\n", data_lane);
		data_lane = 4;
		rt = E_SYS;
	}

	// MIPI Output Setting
	//rt |= ad_tc358743_reset(chip_id, TRUE);
	// Stop Video and Audio
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0004, 0x0CF4, 2); // ConfCtl
	// Reset CSI-TX Block, Enter Sleep mode
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0200, 2); // SysCtl
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0000, 2); // SysCtl
	// PLL Setting in Sleep mode, Int clear
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0001, 2); // SysCtl
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0020, 0x2045, 2); // PLLCtl0
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0022, 0x0203, 2); // PLLCtl1
	//vos_util_delay_us(10);
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0022, 0x0213, 2); // PLLCtl1
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0002, 0x0000, 2); // SysCtl
	//vos_util_delay_us(10);
	// Video Setting
	//rt |= ad_tc358743_i2c_write(chip_id, 0x8573, 0x80, 1); // VOUT_SET2
	// FIFO Delay Setting
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0006, 0x01FE, 2); // FIFO Ctl
	// CSI Lane Enable
	rt |= ad_tc358743_i2c_write(chip_id, 0x0140, 0x00000000, 4); // CLW_CNTRL
	rt |= ad_tc358743_set_mipi_data_lane(chip_id, data_lane);
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0144, 0x00000000, 4); // D0W_CNTRL
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0148, 0x00000000, 4); // D1W_CNTRL
	//rt |= ad_tc358743_i2c_write(chip_id, 0x014C, 0x00000001, 4); // D2W_CNTRL
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0150, 0x00000001, 4); // D3W_CNTRL
	// CSI Transition Timing
	rt |= ad_tc358743_i2c_write(chip_id, 0x0210, 0x00000FA0, 4); // LINEINITCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0214, 0x00000004, 4); // LPTXTIMECNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0218, 0x00001803, 4); // TCLK_HEADERCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x021C, 0x00000005, 4); // TCLK_TRAILCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0220, 0x00000D04, 4); // THS_HEADERCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0224, 0x00003E80, 4); // TWAKEUP
	rt |= ad_tc358743_i2c_write(chip_id, 0x0228, 0x0000000A, 4); // TCLK_POSTCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x022C, 0x00000002, 4); // THS_TRAILCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0230, 0x00000005, 4); // HSTXVREGCNT
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0234, 0x00000007, 4); // HSTXVREGEN
	rt |= ad_tc358743_i2c_write(chip_id, 0x0238, 0x00000001, 4); // TXOPTIONACNTRL
	rt |= ad_tc358743_i2c_write(chip_id, 0x0204, 0x00000001, 4); // STARTCNTRL
	rt |= ad_tc358743_i2c_write(chip_id, 0x0518, 0x00000001, 4); // CSI_START
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0500, 0xA3008083, 4); // CSI_CONFW
	rt |= ad_tc358743_i2c_write(chip_id, AD_TC358743_CSI_CONFW_REG_OFS, 0xA3008081+((data_lane-1)*2), AD_TC358743_CSI_CONFW_REG_LEN);

#if 0
	rt |= ad_tc358743_i2c_write(chip_id, 0x0218, 0x00001503, 4); // TCLK_HEADERCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x021C, 0x00000001, 4); // TCLK_TRAILCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0220, 0x00000103, 4); // THS_HEADERCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0224, 0x00003A98, 4); // TWAKEUP
	rt |= ad_tc358743_i2c_write(chip_id, 0x0228, 0x00000008, 4); // TCLK_POSTCNT
	rt |= ad_tc358743_i2c_write(chip_id, 0x0500, 0xA3008082, 4); // CSI_CONFW
#endif

	// Enable Interrupt
	rt |= ad_tc358743_i2c_write(chip_id, 0x8502, 0xFF, 1); // SYS_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x8503, 0x7F, 1); // CLK_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x8504, 0xFF, 1); // PACKET_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x8505, 0xFF, 1); // AUDIO_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x8506, 0xFF, 1); // ABUF_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x850B, 0x1F, 1); // MISC_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x0014, 0x073F, 2); // TOP_INTS_C
	rt |= ad_tc358743_i2c_write(chip_id, 0x0016, 0x053F, 2); // TOP_INTM
	// Start CSI output
	//rt |= ad_tc358743_i2c_write(chip_id, 0x0004, 0x0CF7, 2); // ConfCtl

	return rt;
}

static ER ad_tc358743_chk_connect(UINT32 chip_id, BOOL *connect)
{
#if 0
	AD_TC358743_INT_STATUS_REG int_sts = {0};
	AD_TC358743_SYS_INT_REG sys_int = {0};
#endif
	AD_TC358743_SYS_STATUS_REG sys_sts = {0};
#if 0
	AD_TC358743_MISC_INT_REG misc_int = {0};
#endif
	ER rt = E_OK;

#if 0
	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_INT_STATUS_REG_OFS, &int_sts.reg, AD_TC358743_INT_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get interrupt status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk connect] HDMI_INT %u\r\n", int_sts.bit.HDMI_INT);

	if (int_sts.bit.HDMI_INT == 0) {
		goto skip;
	}

	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_SYS_INT_REG_OFS, &sys_int.reg, AD_TC358743_SYS_INT_REG_LEN) != E_OK) {
		AD_ERR("get system interrupt fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk connect] I_DDC %u\r\n", sys_int.bit.I_DDC);

	if (sys_int.bit.I_DDC == 0) {
		goto skip;
	}
#endif

	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_SYS_STATUS_REG_OFS, &sys_sts.reg, AD_TC358743_SYS_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get system status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

#if 0
	AD_IND("[chk connect] S_DDC5V %u\r\n", sys_sts.bit.S_DDC5V);

	if (sys_sts.bit.S_DDC5V == 0) {
		goto skip;
	}

	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_MISC_INT_REG_OFS, &misc_int.reg, AD_TC358743_MISC_INT_REG_LEN) != E_OK) {
		AD_ERR("get misc interrupt fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk connect] I_SYNC_CHG %u\r\n", misc_int.bit.I_SYNC_CHG);

	if (misc_int.bit.I_SYNC_CHG == 0) {
		goto skip;
	}
#endif

	AD_IND("[chk connect] S_SYNC_ %u\r\n", sys_sts.bit.S_SYNC_);

	if (sys_sts.bit.S_SYNC_ == 0) {
		goto skip;
	}

	//rt |= ad_tc358743_set_tx_start(chip_id);

	// clear status
	rt |= ad_tc358743_clr_int_sys(chip_id);
	rt |= ad_tc358743_clr_sys_int(chip_id);
	rt |= ad_tc358743_clr_misc_int(chip_id);
	*connect = TRUE;
	return rt;

skip:
	*connect = FALSE;
	return rt;
}

static ER ad_tc358743_chk_disconnect(UINT32 chip_id, BOOL *disconnect)
{
#if 0
	AD_TC358743_INT_STATUS_REG int_sts = {0};
#endif
	AD_TC358743_SYS_INT_REG sys_int = {0};
	AD_TC358743_SYS_STATUS_REG sys_sts = {0};
#if 0
	AD_TC358743_MISC_INT_REG misc_int = {0};
#endif
	ER rt = E_OK;

#if 0
	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_INT_STATUS_REG_OFS, &int_sts.reg, AD_TC358743_INT_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get interrupt status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] HDMI_INT %u\r\n", int_sts.bit.HDMI_INT);

	if (int_sts.bit.HDMI_INT == 0) {
		goto skip;
	}
#endif

	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_SYS_INT_REG_OFS, &sys_int.reg, AD_TC358743_SYS_INT_REG_LEN) != E_OK) {
		AD_ERR("get system interrupt fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] I_DDC %u, I_TMDS %u\r\n", sys_int.bit.I_DDC, sys_int.bit.I_TMDS);

	if (sys_int.bit.I_DDC != 0 || sys_int.bit.I_TMDS != 0) {
		goto do_change;
	}

	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_SYS_STATUS_REG_OFS, &sys_sts.reg, AD_TC358743_SYS_STATUS_REG_LEN) != E_OK) {
		AD_ERR("get system status fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] S_DDC5V %u, S_TMDS %u\r\n", sys_sts.bit.S_DDC5V, sys_sts.bit.S_TMDS);

	if (sys_sts.bit.S_DDC5V != 0 && sys_sts.bit.S_TMDS != 0) {
		goto skip;
	}

#if 0
	if (ad_tc358743_i2c_read(chip_id, AD_TC358743_MISC_INT_REG_OFS, &misc_int.reg, AD_TC358743_MISC_INT_REG_LEN) != E_OK) {
		AD_ERR("get misc interrupt fail\r\n");
		rt = E_SYS;
		goto skip;
	}

	AD_IND("[chk disconnect] I_SYNC_CHG %u\r\n", misc_int.bit.I_SYNC_CHG);

	if (misc_int.bit.I_SYNC_CHG == 0) {
		goto skip;
	}

	AD_IND("[chk disconnect] S_SYNC_ %u\r\n", sys_sts.bit.S_SYNC_);

	if (sys_sts.bit.S_SYNC_ == 0) {
		goto do_change;
	}
#endif

do_change:
	//rt = ad_tc358743_common_init(chip_id);
	rt |= ad_tc358743_set_hdmi(chip_id);
	rt |= ad_tc358743_set_edid(chip_id);
	//rt |= ad_tc358743_set_mipi(chip_id);
	*disconnect = TRUE;
	return rt;

skip:
	rt |= ad_tc358743_clr_int_sys(chip_id);
	rt |= ad_tc358743_clr_sys_int(chip_id);
	rt |= ad_tc358743_clr_misc_int(chip_id);
	*disconnect = FALSE;
	return rt;
}

// implementation of video mode converting
/*static ER ad_tc358743_det_mode_to_vin_mode(AD_TC358743_DET_RAW_MODE raw_mode, AD_TC358743_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358743_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358743_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_tc358743_det_map[i_mode].raw_mode, sizeof(AD_TC358743_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_tc358743_det_map[i_mode].vin_mode, sizeof(AD_TC358743_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown raw mode %u\r\n", AD_TC358743_MODULE_NAME, raw_mode);
	return E_SYS;
}*/

static ER ad_tc358743_vin_mode_to_ch_mode(AD_TC358743_VIN_MODE vin_mode, AD_TC358743_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358743_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358743_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_tc358743_vin_map[i_mode].vin_mode, sizeof(AD_TC358743_VIN_MODE)) == 0) {
			*ch_mode = g_ad_tc358743_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown vin mode %u %u %u %u\r\n", AD_TC358743_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.lane_num);
	return E_SYS;
}

/*static ER ad_tc358743_vout_mode_to_port_mode(AD_TC358743_VOUT_MODE vout_mode, AD_TC358743_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tc358743_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tc358743_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_tc358743_vout_map[i_mode].vout_mode, sizeof(AD_TC358743_VOUT_MODE)) == 0) {
			port_mode = &g_ad_tc358743_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_TC358743_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

static ER ad_tc358743_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_TC358743_DET_VIDEO_INFO *video_info)
{
	AD_TC358743_INFO *ad_private_info;
	UINT32 width = 0, height = 0, fps = 0;
	BOOL chk = 0, interlace = 0;

	if (unlikely(chip_id >= AD_TC358743_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_TC358743_MODULE_NAME, chip_id, AD_TC358743_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_TC358743_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_TC358743_MODULE_NAME, vin_id, AD_TC358743_VIN_MAX);
		return E_SYS;
	}

	ad_private_info = ad_tc358743_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det video info fail. NULL ad private info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	if (ad_private_info->connect && unlikely(ad_tc358743_chk_disconnect(chip_id, &chk) == E_OK)) {

		if (chk) {
			ad_private_info->connect = FALSE;
			goto do_vloss;
		}

	} else if (!ad_private_info->connect && unlikely(ad_tc358743_chk_connect(chip_id, &chk) == E_OK)) {

		if (chk) {
			ad_private_info->connect = TRUE;

		} else {
			goto do_vloss;
		}

	} else {
		goto do_vloss;
	}

	if (unlikely(ad_tc358743_get_h_active_size(chip_id, &width) != E_OK ||
				ad_tc358743_get_v_active_size(chip_id, &height) != E_OK ||
				ad_tc358743_get_fps(chip_id, &fps) != E_OK ||
				ad_tc358743_get_interlace(chip_id, &interlace) != E_OK)) {
		AD_ERR("[%s] ad det video info fail. get info fail\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	ad_tc358743_get_422(chip_id, &ad_private_info->_422);
	ad_tc358743_get_pxclk(chip_id, &ad_private_info->pxclk);
	ad_tc358743_get_h_total_size(chip_id, &ad_private_info->h_total);
	ad_tc358743_get_v_total_size(chip_id, &ad_private_info->v_total);
	ad_tc358743_get_h_blank_size(chip_id, &ad_private_info->h_blank);
	ad_tc358743_get_v_blank_size_prog(chip_id, &ad_private_info->v_blank);

	video_info->vloss = FALSE;
	video_info->width = width;
	video_info->height = height;
	video_info->fps = fps;
	video_info->prog = !interlace;
	return E_OK;

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_TC358743_DFT_WIDTH;
	video_info->height = AD_TC358743_DFT_HEIGHT;
	video_info->fps = AD_TC358743_DFT_FPS;
	video_info->prog = AD_TC358743_DFT_PROG;
	return E_OK;
}

static ER ad_tc358743_dbg_dump_info(UINT32 chip_id)
{
#define GET_STR(strary, idx) ((UINT32)(idx) < sizeof(strary)/sizeof(typeof(strary[0])) ? strary[idx] : "Unknown")
	char _422_str[][4] = {
		"420",
		"422",
	};
	AD_TC358743_INFO *ad_private_info;

	ad_private_info = ad_tc358743_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad dump info fail. NULL ad private info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO BEGIN (CHIP %u) ===================================\r\n", AD_TC358743_MODULE_NAME, chip_id);

	AD_DUMP("%8s %8s %8s %8s %8s %8s %8s %8s %8s %12s\r\n",
		"connect", "422", "pxclk", "h_total", "v_total", "h_blank", "v_blank", "fbd", "prd", "fifo_delay");
	AD_DUMP("==================================================================================================\r\n");
	AD_DUMP("%8u %8s %8u %8u %8u %8u %8u %8u %8u %12u\r\n", ad_private_info->connect,
		GET_STR(_422_str, ad_private_info->_422), ad_private_info->pxclk, ad_private_info->h_total,
		ad_private_info->v_total, ad_private_info->h_blank, ad_private_info->v_blank, ad_private_info->fbd, ad_private_info->prd, ad_private_info->fifo_delay);

	AD_DUMP("\r\n=================================== AD %s DUMP INFO END ===================================\r\n", AD_TC358743_MODULE_NAME);

	AD_DUMP("\r\n");

	return E_OK;
}

#if 0
#endif

static ER ad_tc358743_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};

	ad_info = ad_tc358743_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	ad_tc358743_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_TC358743_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_TC358743_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_TC358743_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_TC358743_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_TC358743_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON;
	ad_drv_open(chip_id, &drv_open_info);

	return E_OK;
}

static ER ad_tc358743_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	return E_OK;
}

static ER ad_tc358743_init(UINT32 chip_id, void *ext_data)
{
	ER rt = E_OK;

	rt |= ad_tc358743_reset(chip_id, FALSE);
	rt |= ad_tc358743_set_hdmi(chip_id);
	rt |= ad_tc358743_set_edid(chip_id);
	rt |= ad_tc358743_set_init_end(chip_id);
	//rt |= ad_tc358743_set_mipi(chip_id);

	g_ad_tc358743_info[chip_id].status |= AD_STS_INIT;

	return rt;
}

static ER ad_tc358743_uninit(UINT32 chip_id, void *ext_data)
{
	g_ad_tc358743_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_tc358743_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_tc358743_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
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
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_TC358743_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			if (unlikely(mode_info->out_id >= ad_info->vout_max)) {
				AD_ERR("[%s] mode_info wrong output\r\n", AD_TC358743_MODULE_NAME);
				rt = E_PAR;
				break;
			}

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
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TC358743_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_TC358743_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tc358743_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	ER rt = E_OK;

	ad_info = ad_tc358743_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TC358743_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_TC358743_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tc358743_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TC358743_INFO *ad_private_info;
	AD_CHGMODE_INFO *chgmode_info;
	AD_TC358743_VIN_MODE vin_mode = {0};
	AD_TC358743_CH_MODE ch_mode = {0};
	UINT32 vin_id, vout_id, fps = 0;
	ER rt = E_OK;

	ad_info = ad_tc358743_get_info(chip_id);
	ad_private_info = ad_tc358743_get_private_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad private info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_TC358743_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_TC358743_MODULE_NAME, vin_id, AD_TC358743_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_TC358743_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_TC358743_MODULE_NAME, vout_id, AD_TC358743_VOUT_MAX);
		return E_SYS;
	}

	ad_tc358743_get_fps_nearest(chgmode_info->ui_info.fps, &fps);

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = fps;
	vin_mode.lane_num = ad_info->vout[vout_id].data_lane_num;
	AD_IND("chip%d vout%d: w = %d, h = %d, fps = %d, lane_num = %d!!\r\n", chip_id, vout_id, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.lane_num);

	if (ad_tc358743_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
		vin_mode.width = AD_TC358743_DFT_WIDTH;
		vin_mode.height = AD_TC358743_DFT_HEIGHT;
		vin_mode.fps = AD_TC358743_DFT_FPS;
		vin_mode.lane_num = AD_TC358743_DFT_LANE_NUM;
		if (ad_tc358743_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
			AD_ERR("Wrong default mode %u %u %u %u\r\n", vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.lane_num);
			return E_SYS;
		}
	}

	rt |= ad_tc358743_set_mipi(chip_id, ad_info->vout[vout_id].data_lane_num);
	rt |= ad_tc358743_set_csi_clk(chip_id, ch_mode.fbd, ch_mode.prd);
	rt |= ad_tc358743_set_fifo_delay(chip_id, ch_mode.fifo_delay);
	rt |= ad_tc358743_set_tx_start(chip_id);

	ad_private_info->fbd = ch_mode.fbd;
	ad_private_info->prd = ch_mode.prd;
	ad_private_info->fifo_delay = ch_mode.fifo_delay;

	chgmode_info->cur_info.width = chgmode_info->ui_info.width;
	chgmode_info->cur_info.height = chgmode_info->ui_info.height;
	chgmode_info->cur_info.fps = chgmode_info->ui_info.fps;
	chgmode_info->cur_info.prog = chgmode_info->ui_info.prog;
	chgmode_info->cur_update = TRUE;

	return rt;
}

static ER ad_tc358743_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TC358743_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_tc358743_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_TC358743_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_tc358743_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_tc358743_info[chip_id].status);
		return E_PAR;
	}

	i_vin = detect_info->vin_id;
	if (unlikely(ad_tc358743_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
		AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_TC358743_MODULE_NAME, i_vin);
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

static ER ad_tc358743_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_tc358743_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_tc358743_dbg(char *str_cmd)
{
#define AD_TC358743_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_TC358743_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_TC358743_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_TC358743_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_TC358743_DBG_CMD_MAX] = {0};
	UINT32 val = 0, /*val2 = 0,*/ i_chip = 0/*, i_vin*/;
	/*AD_TC358743_DET_VIDEO_INFO det_info = {0};*/

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_TC358743_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_TC358743_DBG_CMD_MAX);

	if (cmd_num == 0) {

	} else if (strcmp(cmd_list[0], "dumpinfo") == 0) {
		ad_tc358743_dbg_dump_info(i_chip);

	} else if (strcmp(cmd_list[0], "reset") == 0) {

		if (cmd_num > 1 && sscanf_s(cmd_list[1], "%u", &narg[0]) == 1) {
			val = narg[0];
			AD_IND("mipi_only = %u\r\n", val);
		} else {
			AD_ERR("reset err\r\n");
		}
		ad_tc358743_reset(i_chip, val);

	} else if (strcmp(cmd_list[0], "hdmi") == 0) {
		ad_tc358743_set_hdmi(i_chip);
		ad_tc358743_set_edid(i_chip);

	} else if (strcmp(cmd_list[0], "mipi") == 0) {
		if (cmd_num > 1 && sscanf_s(cmd_list[1], "%u", &narg[0]) == 1) {
			val = narg[0];
			AD_IND("mipi_lane = %u\r\n", val);
		} else {
			AD_ERR("mipi lane err\r\n");
		}
		ad_tc358743_set_mipi(i_chip, val);

	} else if (strcmp(cmd_list[0], "tx") == 0) {
		ad_tc358743_set_tx_start(i_chip);

	} else if (strcmp(cmd_list[0], "fifo") == 0) {

		if (cmd_num > 3 &&
			sscanf_s(cmd_list[1], "%u", &narg[0]) == 1 &&
			sscanf_s(cmd_list[2], "%u", &narg[1]) == 1 &&
			sscanf_s(cmd_list[3], "%u", &narg[2]) == 1) {
			ad_tc358743_get_fifo_delay_val(i_chip, narg[0], narg[1], narg[2], &val);
			AD_DUMP("lane=%u fbd=%u prd=%u => fifo delay = %u\r\n", narg[0], narg[1], narg[2], val);
		} else {
			AD_ERR("fifo delay err\r\n");
		}
	}

	return E_OK;
}

AD_DEV* ad_get_obj_tc358743(void)
{
	return &g_ad_tc358743_obj;
}

