#include "ad_drv.h"

#define AD_DRV_MODULE_NAME_LEN 16
#define AD_DRV_VERSION_LEN 16

typedef struct {
	char module_name[AD_DRV_MODULE_NAME_LEN];
	char version[AD_DRV_VERSION_LEN];
	AD_INFO *comm_info;
	UINT32 i2c_addr_len;
	UINT32 i2c_data_len;
	UINT32 i2c_data_msb;
	UINT32 i2c_retry_num;
	AD_PWR_CTL pwr_ctl;
	UINT32 dbg_func;		// AD_DRV_DBG_FUNC
	AD_DRV_DBG_BANK_REG_DUMP_INFO dbg_bank_reg_dump_info;
	AD_DRV_DBG_REG_DUMP_INFO dbg_reg_dump_info;
	UINT32 status;
} AD_DRV_INFO;

AD_DRV_DEBUG_LEVEL_TYPE ad_drv_debug_level = 2;
AD_DRV_DEBUG_FLAG_TYPE ad_drv_debug_flag[] = __DBGFLT__;

static AD_DRV_INFO g_ad_drv_info[AD_DRV_CHIP_MAX];
static ad_i2c_rw_if g_ad_drv_i2c_rw_if = NULL;

static AD_DRV_INFO *ad_drv_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_DRV_CHIP_MAX)) {
		AD_ERR("ad drv get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_DRV_CHIP_MAX);
		return NULL;
	}
	return &g_ad_drv_info[chip_id];
}

ER ad_drv_open(UINT32 chip_id, AD_DRV_OPEN_INFO *open_info)
{
	AD_DRV_INFO *ad_drv_info;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv open fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	if (unlikely(open_info->i2c_addr_len > AD_DRV_I2C_ADDR_LEN_MAX)) {
		AD_ERR("[id%d] ad drv open fail. i2c addr len (%u) > max (%u)\r\n", chip_id, open_info->i2c_addr_len, AD_DRV_I2C_ADDR_LEN_MAX);
		return E_SYS;
	}

	if (unlikely(open_info->i2c_addr_len > AD_DRV_I2C_ADDR_LEN_MAX)) {
		AD_ERR("[id%d] ad drv open fail. i2c addr len (%u) > max (%u)\r\n", chip_id, open_info->i2c_addr_len, AD_DRV_I2C_ADDR_LEN_MAX);
		return E_SYS;
	}

	snprintf(ad_drv_info->module_name, AD_DRV_MODULE_NAME_LEN, "%s", open_info->module_name);
	snprintf(ad_drv_info->version, AD_DRV_VERSION_LEN, "%s", open_info->version);
	ad_drv_info->comm_info = open_info->comm_info;
	ad_drv_info->i2c_addr_len = open_info->i2c_addr_len;
	ad_drv_info->i2c_data_len = open_info->i2c_data_len;
	ad_drv_info->i2c_data_msb = open_info->i2c_data_msb;
	ad_drv_info->i2c_retry_num = open_info->i2c_retry_num;
	ad_drv_info->pwr_ctl = open_info->pwr_ctl;
	ad_drv_info->dbg_func = open_info->dbg_func;
	if (open_info->dbg_bank_reg_dump_info) {
		ad_drv_info->dbg_bank_reg_dump_info = *open_info->dbg_bank_reg_dump_info;
	}
	if (open_info->dbg_reg_dump_info) {
		ad_drv_info->dbg_reg_dump_info = *open_info->dbg_reg_dump_info;
	}
	ad_drv_info->status |= AD_STS_OPEN;

	return E_OK;
}

ER ad_drv_close(UINT32 chip_id)
{
	AD_DRV_INFO *ad_drv_info;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv close fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	ad_drv_info->status &= ~AD_STS_OPEN;
	return E_OK;
}

ER ad_drv_i2c_set_i2c_rw_if(ad_i2c_rw_if i2c_rw_if)
{
	g_ad_drv_i2c_rw_if = i2c_rw_if;
	return E_OK;
}

ER ad_drv_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	AD_DRV_INFO *ad_drv_info;
	AD_I2C_INFO i2c_data;
	unsigned char buf[AD_DRV_I2C_ADDR_LEN_MAX+AD_DRV_I2C_DATA_LEN_MAX] = {0};
	UINT32 i, i_buf, retry_cnt;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv i2c write fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	if (unlikely((ad_drv_info->status & AD_STS_OPEN) != AD_STS_OPEN)) {
		AD_ERR("ad drv i2c write fail. Wrong status 0x%08x\r\n", ad_drv_info->status);
		return E_SYS;
	}

	if (unlikely(g_ad_drv_i2c_rw_if == NULL)) {
		AD_ERR("[%s] ad drv i2c write fail. NULL g_ad_drv_i2c_rw_if\r\n", ad_drv_info->comm_info->name);
		return E_SYS;
	}

	if (unlikely(data_len > ad_drv_info->i2c_data_len)) {
		AD_ERR("[%s] ad drv i2c write fail. input addr len (%u) > current max (%u)\r\n", ad_drv_info->comm_info->name, data_len, ad_drv_info->i2c_data_len);
		return E_SYS;
	}

	i_buf = 0;
	for (i = 0; i < ad_drv_info->i2c_addr_len; i++) {
		buf[i_buf++] = (reg_addr >> (((ad_drv_info->i2c_addr_len-1)-i)*8)) & 0xFF;
	}
	for (i = 0; i < data_len; i++) {
		if (ad_drv_info->i2c_data_msb) {
			buf[i_buf++] = (value >> (((data_len-1)-i)*8)) & 0xFF;
		} else {
			buf[i_buf++] = (value >> (i*8)) & 0xFF;
		}
	}

	i2c_data.data = buf;
	i2c_data.addr_len = ad_drv_info->i2c_addr_len;
	i2c_data.data_len = data_len;

	retry_cnt = 0;
	while (retry_cnt++ < ad_drv_info->i2c_retry_num) {

		if (g_ad_drv_i2c_rw_if(chip_id, AD_I2C_W, i2c_data) == E_OK) {
			break;
		}

		if (retry_cnt == ad_drv_info->i2c_retry_num) {
			AD_ERR("[%s] write reg fail. chip_id %u, addr 0x%x, value 0x%x\r\n", ad_drv_info->comm_info->name, chip_id, reg_addr, value);
			return E_SYS;
		}
	}

	return E_OK;
}

ER ad_drv_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	AD_DRV_INFO *ad_drv_info;
	AD_I2C_INFO i2c_data;
	unsigned char buf[AD_DRV_I2C_ADDR_LEN_MAX+AD_DRV_I2C_DATA_LEN_MAX] = {0};
	UINT32 i, i_buf, retry_cnt;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv i2c read fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	if (unlikely((ad_drv_info->status & AD_STS_OPEN) != AD_STS_OPEN)) {
		AD_ERR("ad drv i2c read fail. Wrong status 0x%08x\r\n", ad_drv_info->status);
		return E_SYS;
	}

	if (unlikely(g_ad_drv_i2c_rw_if == NULL)) {
		AD_ERR("[%s] ad drv i2c read fail. NULL i2c_rw_if\r\n", ad_drv_info->comm_info->name);
		return E_SYS;
	}

	if (unlikely(data_len > ad_drv_info->i2c_data_len)) {
		AD_ERR("[%s] ad drv i2c read fail. input addr len (%u) > current max (%u)\r\n", ad_drv_info->comm_info->name, data_len, ad_drv_info->i2c_data_len);
		return E_SYS;
	}

	i_buf = 0;
	for (i = 0; i < ad_drv_info->i2c_addr_len; i++) {
		buf[i_buf++] = (reg_addr >> (((ad_drv_info->i2c_addr_len-1)-i)*8)) & 0xFF;
	}

	i2c_data.data = buf;
	i2c_data.addr_len = ad_drv_info->i2c_addr_len;
	i2c_data.data_len = data_len;

	retry_cnt = 0;
	while (retry_cnt++ < ad_drv_info->i2c_retry_num) {

		if (g_ad_drv_i2c_rw_if(chip_id, AD_I2C_R, i2c_data) == E_OK) {

			*value = 0;
			for (i = 0; i < data_len; i++) {
				if (ad_drv_info->i2c_data_msb) {
					*value |= ((UINT32)buf[i_buf++]) << (((data_len-1)-i)*8);
				} else {
					*value |= ((UINT32)buf[i_buf++]) << (i*8);
				}
			}
			break;
		}

		if (retry_cnt == ad_drv_info->i2c_retry_num) {
			AD_ERR("[%s] read reg fail. chip_id %u, addr 0x%x\r\n", ad_drv_info->comm_info->name, chip_id, reg_addr);
			return E_SYS;
		}
	}

	return E_OK;
}

#if !defined(__UITRON) && !defined(__ECOS)
ER ad_drv_pwr_ctrl(UINT32 chip_id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
#define AD_DRV_PWR_CTRL_STS_TURN_ON 0xFFFE
#define AD_DRV_PWR_CTRL_STS_TURN_OFF 0xFFFD
	AD_DRV_INFO *ad_drv_info;
	CTL_SEN_PWR_CTRL_FLAG pwr_ctrl_sts = CTL_SEN_PWR_CTRL_TURN_ON;
	UINT32 i;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv pwr ctrl fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	if (unlikely((ad_drv_info->status & AD_STS_OPEN) != AD_STS_OPEN)) {
		AD_ERR("ad drv pwr ctrl fail. Wrong status 0x%08x\r\n", ad_drv_info->status);
		return E_SYS;
	}

	AD_DUMP("ad drv pwr on: chip_id=%u flag=%d mclk=%u\r\n", chip_id, flag, ad_drv_info->pwr_ctl.mclk_sel);

	// clock control
	if (clk_cb != NULL) {
		if (flag == CTL_SEN_PWR_CTRL_TURN_ON) {
			if (ad_drv_info->pwr_ctl.mclk_sel != CTL_SEN_IGNORE) {
				clk_cb(ad_drv_info->pwr_ctl.mclk_sel, TRUE);
			}
		}
		if (flag == CTL_SEN_PWR_CTRL_TURN_OFF) {
			clk_cb(ad_drv_info->pwr_ctl.mclk_sel, FALSE);
		}
	}

	// io pin control
	for (i = 0; i < ad_drv_info->pwr_ctl.total_ctl_cnt; i++) {

		switch (ad_drv_info->pwr_ctl.pin_ctrl[i].pin) {
		case AD_DRV_PWR_CTRL_STS_TURN_ON:
			AD_DUMP("io pin%u sts=on\r\n", i);
			pwr_ctrl_sts = CTL_SEN_PWR_CTRL_TURN_ON;
			break;

		case AD_DRV_PWR_CTRL_STS_TURN_OFF:
			AD_DUMP("io pin%u sts=off\r\n", i);
			pwr_ctrl_sts = CTL_SEN_PWR_CTRL_TURN_OFF;
			break;

		default:
			AD_DUMP("io pin%u %u=%u, time %uus. %s\r\n", i, ad_drv_info->pwr_ctl.pin_ctrl[i].pin, ad_drv_info->pwr_ctl.pin_ctrl[i].lvl,
				ad_drv_info->pwr_ctl.pin_ctrl[i].hold_time, (pwr_ctrl_sts == flag) ? "do" : "skip");

			// skip io pin control if current user config pwr ctrl sts not match with input flag
			if (pwr_ctrl_sts == flag) {
				gpio_direction_output(ad_drv_info->pwr_ctl.pin_ctrl[i].pin, 0);
				gpio_set_value(ad_drv_info->pwr_ctl.pin_ctrl[i].pin, ad_drv_info->pwr_ctl.pin_ctrl[i].lvl);
				vos_util_delay_us(ad_drv_info->pwr_ctl.pin_ctrl[i].hold_time);
			}
		}
	}

	return E_OK;
}
#endif

static ER ad_drv_dbg_dump_help(UINT32 chip_id)
{
	AD_DRV_INFO *ad_drv_info;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv dbg dump help fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP HELP BEGIN (CHIP %u) ===================================\r\n", chip_id);

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_INFO) {
		AD_DUMP("CMD: dumpinfo\r\n");
		AD_DUMP("DESC: Show decoder information.\r\n");
	}
	AD_DUMP("\r\n");

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG) {
		AD_DUMP("CMD: dumpreg\r\n");
		AD_DUMP("DESC: Dump decoder all registers of all banks.\r\n");
	}
	AD_DUMP("\r\n");

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_WRITE) {
		AD_DUMP("CMD: w ADDR(x) VAL(x)\r\n");
		AD_DUMP("DESC: I2C write register.\r\n");
	}
	AD_DUMP("\r\n");

	if ((ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_WRITE) && (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG)) {
		AD_DUMP("CMD: wb BANK(x) ADDR(x) VAL(x)\r\n");
		AD_DUMP("DESC: I2C write register with bank.\r\n");
	}
	AD_DUMP("\r\n");

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_READ) {
		AD_DUMP("CMD: r ADDR(x)\r\n");
		AD_DUMP("DESC: I2C read register.\r\n");
	}
	AD_DUMP("\r\n");

	if ((ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_READ) && (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG)) {
		AD_DUMP("CMD: rb BANK(x) ADDR(x)\r\n");
		AD_DUMP("DESC: I2C read register with bank.\r\n");
	}
	AD_DUMP("\r\n");

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_HELP) {
		AD_DUMP("CMD: help\r\n");
		AD_DUMP("CMD: ?\r\n");
		AD_DUMP("DESC: Show command usage.\r\n");
	}
	AD_DUMP("\r\n");

	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DBGLV) {
		AD_DUMP("CMD: dbglv LV(s)\r\n");
		AD_DUMP("DESC: Change debug level. LV: err, wrn, ind.\r\n");
		AD_DUMP("CMD: dbglv LV(u)\r\n");
		AD_DUMP("DESC: Change debug level. LV: 1(err), 2(wrn), 5(ind).\r\n");
	}
	AD_DUMP("\r\n");

	AD_DUMP("\r\n=================================== AD DRV DUMP HELP END ===================================\r\n");

	return E_OK;
}

static ER ad_drv_dbg_dump_info(UINT32 chip_id)
{
#define GET_STR(strary, idx) ((UINT32)(idx) < sizeof(strary)/sizeof(typeof(strary[0])) ? strary[idx] : "Unknown")
	char sts_str[][8] = {
		"Open",
		"Close",
		"Init",
		"Uninit",
	};
	char prog_str[][8] = {
		"inter",
		"prog",
	};
	char bus_type_str[][8] = {
		"par",
		"mipi",
	};
	char fmt_str[][4] = {
		"422",
		"420",
	};
	char protocal_str[][8] = {
		"BT601",
		"BT656",
		"BT709",
		"BT1120",
	};
	char depth_str[][2] = {
		"8",
		"16",
	};
	char dbg_func_str[][16] = {
		"Dump info",
		"Write reg",
		"Read reg",
		"Dump bank reg",
		"Help",
		"Debug level",
	};
	AD_DRV_INFO *ad_drv_info;
	AD_INFO *ad_info;
	UINT32 i_vin, i_vout;
	AD_VIN *vin;
	AD_VOUT *vout;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv dump info fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	ad_info = ad_drv_info->comm_info;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("ad drv dump info fail. NULL ad info\r\n");
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP INFO BEGIN (CHIP %u) ===================================\r\n", chip_id);

	AD_DUMP("Module name: %s\r\n", ad_drv_info->module_name);
	AD_DUMP("Module version: %s (AD_DRV version: %s)\r\n", ad_drv_info->version, AD_DRV_VERSION);
	AD_DUMP("I2C addr: %#x\r\n", ad_info->i2c_addr);
	AD_DUMP("Status: %s, %s (AD_DRV status: %s)\r\n",
		ad_info->status & AD_STS_OPEN  ? sts_str[0] : sts_str[1],
		ad_info->status & AD_STS_INIT ? sts_str[2] : sts_str[3],
		ad_drv_info->status & AD_STS_OPEN  ? sts_str[0] : sts_str[1]);
	AD_DUMP("Built-in debug func: ");
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_INFO) AD_DUMP("%s, ", dbg_func_str[0]);
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_WRITE) AD_DUMP("%s, ", dbg_func_str[1]);
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_READ) AD_DUMP("%s, ", dbg_func_str[2]);
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG) AD_DUMP("%s, ", dbg_func_str[3]);
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_HELP) AD_DUMP("%s, ", dbg_func_str[4]);
	if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DBGLV) AD_DUMP("%s, ", dbg_func_str[5]);
	AD_DUMP("\r\n");

	AD_DUMP("\r\n");


	AD_DUMP("%3s | %6s  %5s  %5s  %5s  %7s  %7s  %5s  %5s  %6s  %5s  %5s  %7s\r\n",
		"vin", "active", "vloss", "det_w", "det_h", "det_fps", "prog", "ui_w", "ui_h", "ui_fps", "cur_w", "cur_h", "cur_fps");
	AD_DUMP("==================================================================================================\r\n");
	for (i_vin = 0; i_vin < ad_info->vin_max; i_vin++) {
		vin = &ad_info->vin[i_vin];

		AD_DUMP("%3u | %6u  %5u  %5u  %5u  %4u.%02u  %7s  %5u  %5u  %3u.%02u  %5u  %5u  %4u.%02u\r\n",
			i_vin, vin->active, vin->vloss, vin->det_format.width, vin->det_format.height,
			vin->det_format.fps/100, vin->det_format.fps%100, GET_STR(prog_str, vin->det_format.prog),
			vin->ui_format.width, vin->ui_format.height, vin->ui_format.fps/100, vin->ui_format.fps%100,
			vin->cur_format.width, vin->cur_format.height, vin->cur_format.fps/100, vin->cur_format.fps%100);
	}

	AD_DUMP("\r\n");

	AD_DUMP("%4s | %6s  %7s  %7s  %8s  %7s  %3s  %9s  %9s  %5s  %6s\r\n", "vout", "active", "bus", "fmt", "protocal", "depth", "mux", "clk_rate", "data_rate", "dlane", "vir_ch");
	AD_DUMP("==================================================================================================\r\n");
	for (i_vout = 0; i_vout < ad_info->vout_max; i_vout++) {
		vout = &ad_info->vout[i_vout];

		AD_DUMP("%4u | %6u  %7s  %7s  %8s  %7s  %3u  %9u  %9u  %5u  %6u\r\n",
			i_vout, vout->active, GET_STR(bus_type_str, vout->bus_type), GET_STR(fmt_str, vout->fmt), GET_STR(protocal_str, vout->protocal),
			GET_STR(depth_str, vout->depth), vout->mux_num, vout->clk_rate, vout->data_rate, vout->data_lane_num, vout->virtual_ch);
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP INFO END ===================================\r\n");

	return E_OK;
}

static ER ad_drv_dbg_dump_bank_reg(UINT32 chip_id)
{
	AD_DRV_INFO *ad_drv_info;
	AD_DRV_DBG_BANK_REG_DUMP_INFO *reg_info;
	UINT32 i_bank, i_addr, j_addr, val;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv dump bank reg fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	reg_info = &ad_drv_info->dbg_bank_reg_dump_info;

	if (unlikely(reg_info->bank_num && reg_info->bank == NULL)) {
		AD_ERR("ad drv dump bank reg fail. NULL reg info bank\r\n");
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP BANK REG BEGIN (CHIP %u) ===================================\r\n", chip_id);

	for (i_bank = 0; i_bank < reg_info->bank_num; i_bank++) {

		AD_DUMP("=========== Bank %u(%#x) ===========\r\n", reg_info->bank[i_bank].id, reg_info->bank[i_bank].id);

		if (unlikely(ad_drv_i2c_write(chip_id, reg_info->bank_addr, reg_info->bank[i_bank].id, ad_drv_info->i2c_data_len) != E_OK)) {
			AD_ERR("dump reg fail\r\n");
			return E_SYS;
		}

		for (i_addr = 0; i_addr < reg_info->bank[i_bank].len; i_addr+=reg_info->dump_per_row) {
			AD_DUMP("%0*x: ", (int)ad_drv_info->i2c_addr_len*2, i_addr);
			for (j_addr = 0; j_addr < reg_info->dump_per_row; j_addr+=ad_drv_info->i2c_data_len) {
				if (unlikely(ad_drv_i2c_read(chip_id, i_addr+j_addr, &val, ad_drv_info->i2c_data_len))) {
					AD_ERR("dump reg fail\r\n");
					return E_SYS;
				}
				AD_DUMP("%0*x ", (int)ad_drv_info->i2c_data_len*2, val);
			}
			AD_DUMP("\r\n");
		}
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP BANK REG END ===================================\r\n");

	return E_OK;
}

static ER ad_drv_dbg_dump_reg(UINT32 chip_id)
{
	AD_DRV_INFO *ad_drv_info;
	AD_DRV_DBG_REG_DUMP_INFO *reg_info;
	UINT32 i_tbl, i_addr, j_addr, val;

	ad_drv_info = ad_drv_get_info(chip_id);

	if (unlikely(ad_drv_info == NULL)) {
		AD_ERR("ad drv dump reg fail. NULL ad drv info\r\n");
		return E_SYS;
	}

	reg_info = &ad_drv_info->dbg_reg_dump_info;

	if (unlikely(reg_info->tbl_num && reg_info->tbl == NULL)) {
		AD_ERR("ad drv dump reg fail. NULL reg info tbl\r\n");
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP REG BEGIN (CHIP %u) ===================================\r\n", chip_id);

	for (i_tbl = 0; i_tbl < reg_info->tbl_num; i_tbl++) {

		AD_DUMP("\r\n");

		for (i_addr = 0; i_addr < reg_info->tbl[i_tbl].len; i_addr+=reg_info->dump_per_row) {
			AD_DUMP("%0*x: ", (int)ad_drv_info->i2c_addr_len*2, reg_info->tbl[i_tbl].addr+i_addr);
			for (j_addr = 0; j_addr < reg_info->dump_per_row; j_addr+=ad_drv_info->i2c_data_len) {
				if (unlikely(ad_drv_i2c_read(chip_id, reg_info->tbl[i_tbl].addr+i_addr+j_addr, &val, ad_drv_info->i2c_data_len))) {
					AD_ERR("dump reg fail\r\n");
					return E_SYS;
				}
				AD_DUMP("%0*x ", (int)ad_drv_info->i2c_data_len*2, val);
			}
			AD_DUMP("\r\n");
		}
	}

	AD_DUMP("\r\n=================================== AD DRV DUMP REG END ===================================\r\n");

	return E_OK;
}

UINT32 ad_drv_dbg_parse(char *str_cmd, char **cmd_list, UINT32 cmd_num_max)
{
#define AD_DRV_SHOW_DBG_PARSE_MSG 0
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	UINT32 cmd_num = 0;
	char *token;

#if AD_DRV_SHOW_DBG_PARSE_MSG
	AD_DUMP("str_cmd = %s\r\n", str_cmd);
	AD_DUMP("cmd_num_max = %u\r\n", cmd_num_max);
#endif

#if defined(__UITRON) || defined(__ECOS)
	for (token = strtok(str_cmd, delimiters); token != NULL; token = strtok(NULL, delimiters))
#else
	for (token = strsep(&str_cmd, delimiters); token != NULL; token = strsep(&str_cmd, delimiters))
#endif
	{
		if (cmd_num >= cmd_num_max) {
			AD_WRN("Cmd num exceed max num (%u)\r\n", cmd_num_max);
			break;
		}

		cmd_list[cmd_num] = token;

#if AD_DRV_SHOW_DBG_PARSE_MSG
		AD_DUMP("cmd_list[%u] = %s\r\n", cmd_num, (strcmp(cmd_list[cmd_num], "") == 0) ? "\"\"" : cmd_list[cmd_num]);
#endif

		cmd_num++;
	}

#if AD_DRV_SHOW_DBG_PARSE_MSG
	AD_DUMP("Total cmd_num = %u\r\n", cmd_num);
#endif

	return cmd_num;
}

ER ad_drv_dbg(char *str_cmd)
{
#define AD_DRV_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_DRV_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_DRV_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_DRV_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_DRV_DBG_CMD_MAX] = {0};
	AD_DRV_INFO *ad_drv_info;
	AD_DRV_DBG_BANK_REG_DUMP_INFO *reg_info;

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_DRV_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_DRV_DBG_CMD_MAX);

	if (cmd_num == 0 || strcmp(cmd_list[0], "") == 0 || strcmp(cmd_list[0], "help") == 0 || strcmp(cmd_list[0], "?") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_HELP) {
			ad_drv_dbg_dump_help(0);
		}

	} else if (strcmp(cmd_list[0], "dumpinfo") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_INFO) {
			ad_drv_dbg_dump_info(0);
		}

	} else if (strcmp(cmd_list[0], "dumpreg") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG) {
			ad_drv_dbg_dump_bank_reg(0);
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_REG) {
			ad_drv_dbg_dump_reg(0);
		}

	} else if (cmd_num == 3 && strcmp(cmd_list[0], "w") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_WRITE) {

			if (sscanf_s(cmd_list[1], "%x", &narg[0]) == 1 && sscanf_s(cmd_list[2], "%x", &narg[1]) == 1) {
				AD_DUMP("write 0x%x = 0x%x\r\n", narg[0], narg[1]);
				ad_drv_i2c_write(0, narg[0], narg[1], ad_drv_info->i2c_data_len);
			} else {
				AD_ERR("write err\r\n");
			}
		}

	} else if (cmd_num == 4 && strcmp(cmd_list[0], "wb") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if ((ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_WRITE) && (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG)) {

			reg_info = &ad_drv_info->dbg_bank_reg_dump_info;

			if (sscanf_s(cmd_list[1], "%x", &narg[0]) == 1 && sscanf_s(cmd_list[2], "%x", &narg[1]) == 1 && sscanf_s(cmd_list[3], "%x", &narg[2]) == 1) {
				AD_DUMP("write [bank 0x%x] 0x%x = 0x%x\r\n", narg[0], narg[1], narg[2]);
				ad_drv_i2c_write(0, reg_info->bank_addr, narg[0], ad_drv_info->i2c_data_len);
				ad_drv_i2c_write(0, narg[1], narg[2], ad_drv_info->i2c_data_len);
			} else {
				AD_ERR("write bank err\r\n");
			}
		}

	} else if (cmd_num == 2 && strcmp(cmd_list[0], "r") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_READ) {

			if (sscanf_s(cmd_list[1], "%x", &narg[0]) == 1) {
				ad_drv_i2c_read(0, narg[0], &narg[1], ad_drv_info->i2c_data_len);
				AD_DUMP("read 0x%x = 0x%x\r\n", narg[0], narg[1]);
			} else {
				AD_ERR("read err\r\n");
			}
		}

	} else if (cmd_num == 3 && strcmp(cmd_list[0], "rb") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if ((ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_I2C_READ) && (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DUMP_BANK_REG)) {

			reg_info = &ad_drv_info->dbg_bank_reg_dump_info;

			if (sscanf_s(cmd_list[1], "%x", &narg[0]) == 1 && sscanf_s(cmd_list[2], "%x", &narg[1]) == 1) {
				ad_drv_i2c_write(0, reg_info->bank_addr, narg[0], ad_drv_info->i2c_data_len);
				ad_drv_i2c_read(0, narg[1], &narg[2], ad_drv_info->i2c_data_len);
				AD_DUMP("read [bank 0x%x] 0x%x = 0x%x\r\n", narg[0], narg[1], narg[2]);
			} else {
				AD_ERR("read err\r\n");
			}
		}

	} else if (cmd_num == 2 && strcmp(cmd_list[0], "dbglv") == 0) {

		ad_drv_info = ad_drv_get_info(0);

		if (unlikely(ad_drv_info == NULL)) {
			AD_ERR("ad drv dbg fail. NULL ad drv info\r\n");
			return E_SYS;
		}

		if (ad_drv_info->dbg_func & AD_DRV_DBG_FUNC_DBGLV) {

			if (strcmp(cmd_list[1], "err") == 0 || (sscanf_s(cmd_list[1], "%u", &narg[0]) == 1 && narg[0] == 1)) {
				ad_drv_debug_level = 1;
				AD_DUMP("dbglv = %u(%s)\r\n", 1, "err");
			} else if (strcmp(cmd_list[1], "wrn") == 0 || (sscanf_s(cmd_list[1], "%u", &narg[0]) == 1 && narg[0] == 2)) {
				ad_drv_debug_level = 2;
				AD_DUMP("dbglv = %u(%s)\r\n", 2, "wrn");
			} else if (strcmp(cmd_list[1], "ind") == 0 || (sscanf_s(cmd_list[1], "%u", &narg[0]) == 1 && narg[0] == 5)) {
				ad_drv_debug_level = 5;
				AD_DUMP("dbglv = %u(%s)\r\n", 5, "ind");
			} else {
				AD_ERR("dbglv err\r\n");
			}

		}
	}

	return E_OK;
}
