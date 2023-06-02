
#ifdef __KERNEL__
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#else
#include <stdio.h>
#endif

#include "kwrap/type.h"
#include "mtr_cfg.h"
#include "mtr_dtsi.h"
#include "mtr_common.h"


//=============================================================================
// config file
//=============================================================================
CFG_FILE_FMT *mtr_common_open_cfg(INT8 *pfile_name)
{
	return mtr_cfg_open(pfile_name, O_RDONLY);
}

void mtr_common_close_cfg(CFG_FILE_FMT *pcfg_file)
{
	mtr_cfg_close(pcfg_file);
}

void mtr_common_load_cfg(CFG_FILE_FMT *pcfg_file, void *param)
{
	char section_str[16], key_str[16];
	INT8 cfg_buf[LINE_LEN];
	UINT32 i, j, commu_type = 0, move_spd;
	INT32 pin_value;
	MOTOR_DRV_INFO *pmotor_info;

	for (i = 0; i < MTR_ID_MAX; i++) {
		pmotor_info = (MOTOR_DRV_INFO *)((UINT32)param + i * sizeof(MOTOR_DRV_INFO));

		sprintf(section_str, "MTR_HW_%u", (unsigned int)i);

		if (mtr_cfg_get_field_str(section_str, "rst_pin", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
				pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] = pin_value;
			}
		} else {
			DBG_WRN("rst_pin not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "fz_vd_pin", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
				pmotor_info->lens_hw.pin_def[HW_GPIO_FZ] = pin_value;
			}
		} else {
			DBG_WRN("fz_vd_pin not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "is_vd_pin", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
				pmotor_info->lens_hw.pin_def[HW_GPIO_IS] = pin_value;
			}
		} else {
			DBG_WRN("is_vd_pin not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "ircut_ctl_pin", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			mtr_cfg_str2tab_u32(cfg_buf, &pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], 2);
		} else {
			DBG_WRN("ircut_ctl_pin not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "drive_sts_pin", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			mtr_cfg_str2tab_u32(cfg_buf, &pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS], 2);
		} else {
			DBG_WRN("drive_sts_pin not exist!\r\n");
		}

		sprintf(section_str, "MTR_PROTOCOL_%u", (unsigned int)i);

		if (mtr_cfg_get_field_str(section_str, "commu_type", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			commu_type = CFG_STR2UINT((char *)cfg_buf);
		} else {
			DBG_WRN("commu_type not exist!\r\n");
		}

		if (commu_type == 2) {
			if (mtr_cfg_get_field_str(section_str, "cs_gpio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
					pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs = pin_value;
				}
			} else {
				DBG_WRN("cs_gpio not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "clk_gpio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
					pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk = pin_value;
				}
			} else {
				DBG_WRN("clk_gpio not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "tx_gpio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
					pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx = pin_value;
				}
			} else {
				DBG_WRN("tx_gpio not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "rx_gpio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				if ((pin_value = CFG_STR2INT((char *)cfg_buf)) >= 0) {
					pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx = pin_value;
				}
			} else {
				DBG_WRN("rx_gpio not exist!\r\n");
			}
		} else {
			if (mtr_cfg_get_field_str(section_str, "commu_chnel", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_chnel = CFG_STR2UINT((char *)cfg_buf);
			} else {
				DBG_WRN("commu_chnel not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "commu_mode", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_mode = CFG_STR2UINT((char *)cfg_buf);
			} else {
				DBG_WRN("commu_mode not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "packet_bit", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_pkt_len = CFG_STR2UINT((char *)cfg_buf);
			} else {
				DBG_WRN("packet_bit not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "braud_rate", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_freq = CFG_STR2UINT((char *)cfg_buf);
			} else {
				DBG_WRN("braud_rate not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "data_order", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_order = (CFG_STR2UINT((char *)cfg_buf) != 0) ? TRUE : FALSE;
			} else {
				DBG_WRN("data_order not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "cs_polarity", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.cs_polarity = (CFG_STR2UINT((char *)cfg_buf) != 0) ? TRUE : FALSE;
			} else {
				DBG_WRN("cs_polarity not exist!\r\n");
			}

			if (mtr_cfg_get_field_str(section_str, "packet_dly", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				pmotor_info->lens_hw.spi_cfg.spi_pkt_dly = CFG_STR2UINT((char *)cfg_buf);
			} else {
				DBG_WRN("packet_dly not exist!\r\n");
			}
		}

		sprintf((char *)section_str, "MTR_LENS_%u", (unsigned int)i);

		if (mtr_cfg_get_field_str(section_str, "wide_focal", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->lens_proty.focal_len_min = CFG_STR2UINT((char *)cfg_buf);
		} else {
			DBG_WRN("wide_focal not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "tele_focal", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->lens_proty.focal_len_max = CFG_STR2UINT((char *)cfg_buf);
		} else {
			DBG_WRN("tele_focal not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_chnel_sel", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].chnel_sel = MTR_MIN(CFG_STR2INT((char *)cfg_buf), 1);
		} else {
			DBG_WRN("focus_chnel_sel not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_chnel_sel", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].chnel_sel = MTR_MIN(CFG_STR2INT((char *)cfg_buf), 1);
		} else {
			DBG_WRN("zoom_chnel_sel not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_gear_ratio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].gear_ratio = MTR_MIN(CFG_STR2INT((char *)cfg_buf), 64);
		} else {
			DBG_WRN("focus_gear_ratio not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_gear_ratio", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].gear_ratio = MTR_MIN(CFG_STR2INT((char *)cfg_buf), 64);
		} else {
			DBG_WRN("zoom_gear_ratio not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_pos_dir", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].def_positive_dir = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_pos_dir not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_pos_dir", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].def_positive_dir = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_pos_dir not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_pwm_freq", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].pwm_freq = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_pwm_freq not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_pwm_freq", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].pwm_freq = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_pwm_freq not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_init_ofst", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].init_pos_ofst = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_init_ofst not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_init_ofst", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].init_pos_ofst = CFG_STR2UINT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_init_ofst not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_dis_max", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].max_move_distance = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_dis_max not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_dis_max", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].max_move_distance = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_dis_max not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_move_spd", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			move_spd = CFG_STR2UINT((char *)cfg_buf);
			move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

			if (move_spd == 20) {
				pmotor_info->motor_config[0].move_speed = MTR_SPEED_QUARTER;
			} else if (move_spd == 40) {
				pmotor_info->motor_config[0].move_speed = MTR_SPEED_HALF;
			} else if (move_spd == 60) {
				pmotor_info->motor_config[0].move_speed = MTR_SPEED_1X;
			} else if (move_spd == 80) {
				pmotor_info->motor_config[0].move_speed = MTR_SPEED_2X;
			} else {
				pmotor_info->motor_config[0].move_speed = MTR_SPEED_4X;
			}
		} else {
			DBG_WRN("focus_move_spd not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_move_spd", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			move_spd = CFG_STR2UINT((char *)cfg_buf);
			move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

			if (move_spd == 20) {
				pmotor_info->motor_config[1].move_speed = MTR_SPEED_QUARTER;
			} else if (move_spd == 40) {
				pmotor_info->motor_config[1].move_speed = MTR_SPEED_HALF;
			} else if (move_spd == 60) {
				pmotor_info->motor_config[1].move_speed = MTR_SPEED_1X;
			} else if (move_spd == 80) {
				pmotor_info->motor_config[1].move_speed = MTR_SPEED_2X;
			} else {
				pmotor_info->motor_config[1].move_speed = MTR_SPEED_4X;
			}
		} else {
			DBG_WRN("zoom_move_spd not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_backlash", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].gear_backlash = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_backlash not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_backlash", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].gear_backlash = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_backlash not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "focus_dly_time", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[0].dly_time = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("focus_dly_time not exist!\r\n");
		}

		if (mtr_cfg_get_field_str(section_str, "zoom_dly_time", cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			pmotor_info->motor_config[1].dly_time = CFG_STR2INT((char *)cfg_buf);
		} else {
			DBG_WRN("zoom_dly_time not exist!\r\n");
		}

		for (j = 0; j < ZOOM_SECTION_NUM; j++) {
			sprintf(key_str, "fz_tab%u", (unsigned int)j);

			if (mtr_cfg_get_field_str(section_str, key_str, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
				mtr_cfg_str2tab_s32(cfg_buf, &pmotor_info->zoom_focus_tab[j].zoom_position, 3);
			} else {
				DBG_WRN("%s not exist!\r\n", key_str);
			}
		}
	}
}


//=============================================================================
// dtsi file
//=============================================================================
#if MTR_DTSI_FROM_FILE
static MTR_DTSI_FILE mtr_dtsi_file[MTR_DTSI_ITEM_MAX_NUM] = {
	{ "spi_cfg",      "type",            NULL, sizeof(UINT32)              },    // 0
	{ "spi_cfg",      "chnel",           NULL, sizeof(UINT32)              },
	{ "spi_cfg",      "mode",            NULL, sizeof(UINT32)              },
	{ "spi_cfg",      "packet_bit",      NULL, sizeof(UINT32)              },
	{ "spi_cfg",      "braud_rate",      NULL, sizeof(UINT32)              },
	{ "spi_cfg",      "data_order",      NULL, sizeof(UINT32)              },    // 5
	{ "spi_cfg",      "cs_polarity",     NULL, sizeof(UINT32)              },
	{ "spi_cfg",      "packet_dly",      NULL, sizeof(UINT32)              },

	{ "i2c_cfg",      "id",              NULL, sizeof(UINT32)              },
	{ "i2c_cfg",      "addr",            NULL, sizeof(UINT32)              },

	{ "spi_gpio_cfg", "cs",              NULL, sizeof(UINT32)              },    // 10
	{ "spi_gpio_cfg", "clk",             NULL, sizeof(UINT32)              },
	{ "spi_gpio_cfg", "tx",              NULL, sizeof(UINT32)              },
	{ "spi_gpio_cfg", "rx",              NULL, sizeof(UINT32)              },

	{ "pwm_cfg",      "pwm_mode",        NULL, sizeof(UINT32)              },
	{ "pwm_cfg",      "focus_freq",      NULL, sizeof(UINT32)              },    // 15
	{ "pwm_cfg",      "zoom_freq",       NULL, sizeof(UINT32)              },
	{ "pwm_cfg",      "focus_port_sel",  NULL, sizeof(UINT32)              },
	{ "pwm_cfg",      "zoom_port_sel",   NULL, sizeof(UINT32)              },

	{ "gpio_cfg",     "rst_pin",         NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "pwdn_pin",        NULL, sizeof(UINT32)              },    // 20
	{ "gpio_cfg",     "fz_vd_pin",       NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "is_vd_pin",       NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "focus_sts_pin",   NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "zoom_sts_pin",    NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "ircut_0_pin",     NULL, sizeof(UINT32)              },    // 25
	{ "gpio_cfg",     "ircut_1_pin",     NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "ext_0_pin",       NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "ext_1_pin",       NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "ext_2_pin",       NULL, sizeof(UINT32)              },
	{ "gpio_cfg",     "ext_3_pin",       NULL, sizeof(UINT32)              },    // 30

	{ "lens_param",   "wide_focal",      NULL, sizeof(UINT32)              },
	{ "lens_param",   "tele_focal",      NULL, sizeof(UINT32)              },
	{ "lens_param",   "focus_pos_dir",   NULL, sizeof(UINT32)              },
	{ "lens_param",   "zoom_pos_dir",    NULL, sizeof(UINT32)              },
	{ "lens_param",   "focus_init_ofst", NULL, sizeof(UINT32)              },    // 35
	{ "lens_param",   "zoom_init_ofst",  NULL, sizeof(UINT32)              },
	{ "lens_param",   "focus_dis_max",   NULL, sizeof(UINT32)              },
	{ "lens_param",   "zoom_dis_max",    NULL, sizeof(UINT32)              },
	{ "lens_param",   "focus_move_spd",  NULL, sizeof(UINT32)              },
 	{ "lens_param",   "zoom_move_spd",   NULL, sizeof(UINT32)              },    // 40
 	{ "lens_param",   "focus_backlash",  NULL, sizeof(UINT32)              },
	{ "lens_param",   "zoom_backlash",   NULL, sizeof(UINT32)              },
	{ "lens_param",   "focus_dly_time",  NULL, sizeof(UINT32)              },
	{ "lens_param",   "zoom_dly_time",   NULL, sizeof(UINT32)              },
	{ "lens_param",   "fz_tab0",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },    // 45
	{ "lens_param",   "fz_tab1",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab2",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab3",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab4",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab5", 	     NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },    // 50
	{ "lens_param",   "fz_tab6",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab7",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab8",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab9",         NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },
	{ "lens_param",   "fz_tab10",        NULL, sizeof(LENS_ZOOM_FOCUS_TAB) },    // 55
};

static UINT32 mtr_big_little_swap(UINT8 *pvalue)
{
	UINT32 tmp;

	tmp = (pvalue[0] << 24) | (pvalue[1] << 16) | (pvalue[2] << 8) | pvalue[3];

	return tmp;
}

ER mtr_common_load_dtsi_file(UINT8 *node_path, UINT8 *file_path, UINT8 *buf_addr, void *param)
{
	CHAR node_name[64];
	UINT8 *pfdt_addr;
	INT32 i, node_ofst = 0, data_size;
	UINT32 move_spd;
	const void *pfdt_node;
	ER rt = E_OK;
	MOTOR_DRV_INFO *pmotor_info = (MOTOR_DRV_INFO *)param;

#ifdef __KERNEL__
	UINT32 read_size = 2*1024;
	VOS_FILE fp;

	pfdt_addr = kzalloc(read_size, GFP_KERNEL);
	if (pfdt_addr == NULL) {
		DBG_WRN("fail to allocate pfdt_addr!\r\n");

		return E_SYS;
	}

	fp = vos_file_open((CHAR *)file_path, O_RDONLY, 0x777);
	if (fp == (VOS_FILE)(-1)) {
		kfree(pfdt_addr);
		pfdt_addr = NULL;
		DBG_WRN("open %s fail!\r\n", file_path);

		return -E_SYS;
	}

	if (vos_file_read(fp, pfdt_addr, read_size) < 1) {
		vos_file_close(fp);
		kfree(pfdt_addr);
		pfdt_addr = NULL;
		DBG_WRN("read %s fail!\r\n", file_path);

		return -E_SYS;
	}

	vos_file_close(fp);
#else
	pfdt_addr = buf_addr;
	if (pfdt_addr == NULL) {
		DBG_WRN("pfdt_addr is NULL!\r\n");

		return E_SYS;
	}
#endif

	node_ofst = fdt_path_offset(pfdt_addr, (CHAR *)node_path);
	if (node_ofst < 0) {
#ifdef __KERNEL__
		kfree(pfdt_addr);
		pfdt_addr = NULL;
#endif
		DBG_WRN("%s not available!\r\n", node_path);

		return E_SYS;
	}

	for (i = 0; i < MTR_DTSI_ITEM_MAX_NUM; i++) {
		sprintf(node_name, "%s/%s", node_path, mtr_dtsi_file[i].node_name);
		node_ofst = fdt_path_offset(pfdt_addr, node_name);

		if (node_ofst >= 0) {
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, (CHAR *)mtr_dtsi_file[i].data_name, (int *)&data_size);
			if ((pfdt_node != NULL) && (data_size == mtr_dtsi_file[i].size)) {
				mtr_dtsi_file[i].pdata = (UINT8 *)pfdt_node;
			} else {
				mtr_dtsi_file[i].pdata = NULL;
				DBG_WRN("%s size not match (%d) (%d)!\r\n", node_name, data_size, mtr_dtsi_file[i].size);
			}
		} else {
			mtr_dtsi_file[i].pdata = NULL;
			DBG_WRN("%s not available!\r\n", node_name);
		}
	}

	if (mtr_dtsi_file[0].pdata != NULL) {
		pmotor_info->lens_hw.protocol_type = mtr_big_little_swap(mtr_dtsi_file[0].pdata);
	}
	if (mtr_dtsi_file[1].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_chnel = mtr_big_little_swap(mtr_dtsi_file[1].pdata);
	}
	if (mtr_dtsi_file[2].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_mode = mtr_big_little_swap(mtr_dtsi_file[2].pdata);
	}
	if (mtr_dtsi_file[3].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_pkt_len = mtr_big_little_swap(mtr_dtsi_file[3].pdata);
	}
	if (mtr_dtsi_file[4].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_freq = mtr_big_little_swap(mtr_dtsi_file[4].pdata);
	}
	if (mtr_dtsi_file[5].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_order = mtr_big_little_swap(mtr_dtsi_file[5].pdata);
	}
	if (mtr_dtsi_file[6].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.cs_polarity = mtr_big_little_swap(mtr_dtsi_file[6].pdata);
	}
	if (mtr_dtsi_file[7].pdata != NULL) {
		pmotor_info->lens_hw.spi_cfg.spi_pkt_dly = mtr_big_little_swap(mtr_dtsi_file[7].pdata);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("spi_cfg:%u,%u,%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_hw.protocol_type, pmotor_info->lens_hw.spi_cfg.spi_chnel, pmotor_info->lens_hw.spi_cfg.spi_mode, pmotor_info->lens_hw.spi_cfg.spi_pkt_len, pmotor_info->lens_hw.spi_cfg.spi_freq, pmotor_info->lens_hw.spi_cfg.spi_order, pmotor_info->lens_hw.spi_cfg.cs_polarity, pmotor_info->lens_hw.spi_cfg.spi_pkt_dly);
#endif

	if (mtr_dtsi_file[8].pdata != NULL) {
		pmotor_info->lens_hw.i2c_cfg.i2c_id = mtr_big_little_swap(mtr_dtsi_file[8].pdata);
	}
	if (mtr_dtsi_file[9].pdata != NULL) {
		pmotor_info->lens_hw.i2c_cfg.i2c_addr = mtr_big_little_swap(mtr_dtsi_file[9].pdata);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("i2c_cfg:%u,0x%02X\n", pmotor_info->lens_hw.i2c_cfg.i2c_id, pmotor_info->lens_hw.i2c_cfg.i2c_addr);
#endif

	if (pmotor_info->lens_hw.protocol_type == 2) {
		if (mtr_dtsi_file[10].pdata != NULL) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs = mtr_big_little_swap(mtr_dtsi_file[10].pdata);
		}
		if (mtr_dtsi_file[11].pdata != NULL) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk = mtr_big_little_swap(mtr_dtsi_file[11].pdata);
		}
		if (mtr_dtsi_file[12].pdata != NULL) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx = mtr_big_little_swap(mtr_dtsi_file[12].pdata);
		}
		if (mtr_dtsi_file[13].pdata != NULL) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx = mtr_big_little_swap(mtr_dtsi_file[13].pdata);
		}
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("gpio_spi_cfg:%u,%u,%u,%u\n", pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx);
#endif

	if (mtr_dtsi_file[14].pdata != NULL) {
		pmotor_info->lens_hw.pwm_cfg[0].pwm_mode = pmotor_info->lens_hw.pwm_cfg[1].pwm_mode = pmotor_info->lens_hw.pwm_cfg[2].pwm_mode = mtr_big_little_swap(mtr_dtsi_file[14].pdata);
	}
	if (mtr_dtsi_file[15].pdata != NULL) {
		pmotor_info->lens_hw.pwm_cfg[0].pwm_freq = mtr_big_little_swap(mtr_dtsi_file[15].pdata);
	}
	if (mtr_dtsi_file[16].pdata != NULL) {
		pmotor_info->lens_hw.pwm_cfg[0].pwm_port_sel = mtr_big_little_swap(mtr_dtsi_file[16].pdata);
	}
	if (mtr_dtsi_file[17].pdata != NULL) {
		pmotor_info->lens_hw.pwm_cfg[1].pwm_freq = mtr_big_little_swap(mtr_dtsi_file[17].pdata);
	}
	if (mtr_dtsi_file[18].pdata != NULL) {
		pmotor_info->lens_hw.pwm_cfg[1].pwm_port_sel = mtr_big_little_swap(mtr_dtsi_file[18].pdata);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("pwm_cfg:%u,(%u,%u),(%u,%u)\n", pmotor_info->lens_hw.pwm_cfg[0].pwm_mode, pmotor_info->lens_hw.pwm_cfg[0].pwm_freq, pmotor_info->lens_hw.pwm_cfg[0].pwm_port_sel, pmotor_info->lens_hw.pwm_cfg[1].pwm_freq, pmotor_info->lens_hw.pwm_cfg[1].pwm_port_sel);
#endif

	if (mtr_dtsi_file[19].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[19].pdata), 95);
	}
	if (mtr_dtsi_file[20].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[20].pdata), 95);
	}
	if (mtr_dtsi_file[21].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_FZ] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[21].pdata), 95);
	}
	if (mtr_dtsi_file[22].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IS] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[22].pdata), 95);
	}
	if (mtr_dtsi_file[23].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[23].pdata), 95);
	}
	if (mtr_dtsi_file[24].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[24].pdata), 95);
	}
	if (mtr_dtsi_file[25].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[25].pdata), 95);
	}
	if (mtr_dtsi_file[26].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[26].pdata), 95);
	}
	if (mtr_dtsi_file[27].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT0] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[27].pdata), 95);
	}
	if (mtr_dtsi_file[28].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT1] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[28].pdata), 95);
	}
	if (mtr_dtsi_file[29].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT2] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[29].pdata), 95);
	}
	if (mtr_dtsi_file[30].pdata != NULL) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT3] = MTR_MIN(mtr_big_little_swap(mtr_dtsi_file[30].pdata), 95);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("lens_hw:%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN], pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], pmotor_info->lens_hw.pin_def[HW_GPIO_IS], pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1]);
#endif

	if (mtr_dtsi_file[31].pdata != NULL) {
		pmotor_info->lens_proty.focal_len_min = mtr_big_little_swap(mtr_dtsi_file[31].pdata);
	}
	if (mtr_dtsi_file[32].pdata != NULL) {
		pmotor_info->lens_proty.focal_len_max = mtr_big_little_swap(mtr_dtsi_file[32].pdata);
	}
	if (mtr_dtsi_file[33].pdata != NULL) {
		pmotor_info->motor_config[0].def_positive_dir = mtr_big_little_swap(mtr_dtsi_file[33].pdata);
	}
	if (mtr_dtsi_file[34].pdata != NULL) {
		pmotor_info->motor_config[1].def_positive_dir = mtr_big_little_swap(mtr_dtsi_file[34].pdata);
	}
	if (mtr_dtsi_file[35].pdata != NULL) {
		pmotor_info->motor_config[0].init_pos_ofst = mtr_big_little_swap(mtr_dtsi_file[35].pdata);
	}
	if (mtr_dtsi_file[36].pdata != NULL) {
		pmotor_info->motor_config[1].init_pos_ofst = mtr_big_little_swap(mtr_dtsi_file[36].pdata);
	}
	if (mtr_dtsi_file[37].pdata != NULL) {
		pmotor_info->motor_config[0].max_move_distance = mtr_big_little_swap(mtr_dtsi_file[37].pdata);
	}
	if (mtr_dtsi_file[38].pdata != NULL) {
		pmotor_info->motor_config[1].max_move_distance = mtr_big_little_swap(mtr_dtsi_file[38].pdata);
	}
	if (mtr_dtsi_file[39].pdata != NULL) {
		move_spd = mtr_big_little_swap(mtr_dtsi_file[39].pdata);
		move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

		if (move_spd == 20) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_QUARTER;
		} else if (move_spd == 40) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_HALF;
		} else if (move_spd == 60) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_1X;
		} else if (move_spd == 80) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_2X;
		} else {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_4X;
		}
	}
	if (mtr_dtsi_file[40].pdata != NULL) {
		move_spd = mtr_big_little_swap(mtr_dtsi_file[40].pdata);
		move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

		if (move_spd == 20) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_QUARTER;
		} else if (move_spd == 40) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_HALF;
		} else if (move_spd == 60) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_1X;
		} else if (move_spd == 80) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_2X;
		} else {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_4X;
		}
	}
	if (mtr_dtsi_file[41].pdata != NULL) {
		pmotor_info->motor_config[0].gear_backlash = mtr_big_little_swap(mtr_dtsi_file[41].pdata);
	}
	if (mtr_dtsi_file[42].pdata != NULL) {
		pmotor_info->motor_config[1].gear_backlash = mtr_big_little_swap(mtr_dtsi_file[42].pdata);
	}
	if (mtr_dtsi_file[43].pdata != NULL) {
		pmotor_info->motor_config[0].dly_time = mtr_big_little_swap(mtr_dtsi_file[43].pdata);
	}
	if (mtr_dtsi_file[44].pdata != NULL) {
		pmotor_info->motor_config[1].dly_time = mtr_big_little_swap(mtr_dtsi_file[44].pdata);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("motor_config:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_proty.focal_len_min, pmotor_info->lens_proty.focal_len_max, pmotor_info->motor_config[0].def_positive_dir, pmotor_info->motor_config[1].def_positive_dir, pmotor_info->motor_config[0].init_pos_ofst, pmotor_info->motor_config[1].init_pos_ofst,
		pmotor_info->motor_config[0].max_move_distance, pmotor_info->motor_config[1].max_move_distance, pmotor_info->motor_config[0].move_speed, pmotor_info->motor_config[1].move_speed, pmotor_info->motor_config[0].gear_backlash, pmotor_info->motor_config[1].gear_backlash, pmotor_info->motor_config[0].dly_time, pmotor_info->motor_config[1].dly_time);
#endif

	for (i = 0; i < ZOOM_SECTION_NUM; i++) {
		if (mtr_dtsi_file[45+i].pdata != NULL) {
			pmotor_info->zoom_focus_tab[i].zoom_position = mtr_big_little_swap(mtr_dtsi_file[45+i].pdata);
			pmotor_info->zoom_focus_tab[i].focus_min = mtr_big_little_swap((UINT8 *)((UINT32)mtr_dtsi_file[45+i].pdata + sizeof(UINT32)));
			pmotor_info->zoom_focus_tab[i].focus_max = mtr_big_little_swap((UINT8 *)((UINT32)mtr_dtsi_file[45+i].pdata + 2 * sizeof(UINT32)));
#if MTR_SHOW_DBG_MSG
			DBG_DUMP("zoom_focus_tab:%d,%d,%d\n", pmotor_info->zoom_focus_tab[i].zoom_position, pmotor_info->zoom_focus_tab[i].focus_min, pmotor_info->zoom_focus_tab[i].focus_max);
#endif
		}
	}

#ifdef __KERNEL__
	kfree(pfdt_addr);
	pfdt_addr = NULL;
#endif

	return rt;
}
#endif


//=============================================================================
// system dtsi
//=============================================================================
#ifdef __KERNEL__
static MTR_DTSI mtr_dtsi[MTR_DTSI_ITEM_MAX_NUM] = {
	{ "spi_cfg",      "type",            0xFFFFFFFF },    // 0
	{ "spi_cfg",      "chnel",           0xFFFFFFFF },
	{ "spi_cfg",      "mode",            0xFFFFFFFF },
	{ "spi_cfg",      "packet_bit",      0xFFFFFFFF },
	{ "spi_cfg",      "braud_rate",      0xFFFFFFFF },
	{ "spi_cfg",      "data_order",      0xFFFFFFFF },    // 5
	{ "spi_cfg",      "cs_polarity",     0xFFFFFFFF },
	{ "spi_cfg",      "packet_dly",      0xFFFFFFFF },

	{ "i2c_cfg",      "id",              0xFFFFFFFF },
	{ "i2c_cfg",      "addr",            0xFFFFFFFF },

	{ "spi_gpio_cfg", "cs",              0xFFFFFFFF },    // 10
	{ "spi_gpio_cfg", "clk",             0xFFFFFFFF },
	{ "spi_gpio_cfg", "tx",              0xFFFFFFFF },
	{ "spi_gpio_cfg", "rx",              0xFFFFFFFF },

	{ "pwm_cfg",      "pwm_mode",        0xFFFFFFFF },
	{ "pwm_cfg",      "focus_freq",      0xFFFFFFFF },    // 15
	{ "pwm_cfg",      "zoom_freq",       0xFFFFFFFF },
	{ "pwm_cfg",      "focus_port_sel",  0xFFFFFFFF },
	{ "pwm_cfg",      "zoom_port_sel",   0xFFFFFFFF },

	{ "gpio_cfg",     "rst_pin",         0xFFFFFFFF },
	{ "gpio_cfg",     "pwdn_pin",        0xFFFFFFFF },    // 20
	{ "gpio_cfg",     "fz_vd_pin",       0xFFFFFFFF },
	{ "gpio_cfg",     "is_vd_pin",       0xFFFFFFFF },
	{ "gpio_cfg",     "focus_sts_pin",   0xFFFFFFFF },
	{ "gpio_cfg",     "zoom_sts_pin",    0xFFFFFFFF },
	{ "gpio_cfg",     "ircut_0_pin",     0xFFFFFFFF },    // 25
	{ "gpio_cfg",     "ircut_1_pin",     0xFFFFFFFF },
	{ "gpio_cfg",     "ext_0_pin",       0xFFFFFFFF },
	{ "gpio_cfg",     "ext_1_pin",       0xFFFFFFFF },
	{ "gpio_cfg",     "ext_2_pin",       0xFFFFFFFF },
	{ "gpio_cfg",     "ext_3_pin",       0xFFFFFFFF },    // 30

	{ "lens_param",   "wide_focal",      0xFFFFFFFF },
	{ "lens_param",   "tele_focal",      0xFFFFFFFF },
	{ "lens_param",   "focus_pos_dir",   0xFFFFFFFF },
	{ "lens_param",   "zoom_pos_dir",    0xFFFFFFFF },
	{ "lens_param",   "focus_init_ofst", 0xFFFFFFFF },    // 35
	{ "lens_param",   "zoom_init_ofst",  0xFFFFFFFF },
	{ "lens_param",   "focus_dis_max",   0xFFFFFFFF },
	{ "lens_param",   "zoom_dis_max",    0xFFFFFFFF },
	{ "lens_param",   "focus_move_spd",  0xFFFFFFFF },
 	{ "lens_param",   "zoom_move_spd",   0xFFFFFFFF },    // 40
 	{ "lens_param",   "focus_backlash",  0xFFFFFFFF },
	{ "lens_param",   "zoom_backlash",   0xFFFFFFFF },
	{ "lens_param",   "focus_dly_time",  0xFFFFFFFF },
	{ "lens_param",   "zoom_dly_time",   0xFFFFFFFF },
	{ "lens_param",   "fz_tab0",         0xFFFFFFFF },    // 45
	{ "lens_param",   "fz_tab1",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab2",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab3",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab4",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab5", 	     0xFFFFFFFF },    // 50
	{ "lens_param",   "fz_tab6",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab7",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab8",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab9",         0xFFFFFFFF },
	{ "lens_param",   "fz_tab10",        0xFFFFFFFF },    // 55
};

ER mtr_common_load_dtsi(struct device_node *node, void *param)
{
	CHAR node_name[24];
	UINT32 i, move_spd, tmp[3] = { 0 };
	ER rt = E_OK;
	struct device_node *child;
	MOTOR_DRV_INFO *pmotor_info = (MOTOR_DRV_INFO *)param;

	for (i = 0; i < MTR_DTSI_ITEM_MAX_NUM; i++) {
		snprintf(node_name, sizeof(node_name), "%s", mtr_dtsi[i].data_name);

		for_each_child_of_node(node, child) {
			if (i < 45) {    // fz_tab0
				if (of_property_read_u32(child, node_name, &mtr_dtsi[i].tmp_buf) == 0) {
					break;
				}
			} else {
				if (of_property_read_u32_array(child, node_name, tmp, sizeof(LENS_ZOOM_FOCUS_TAB)/sizeof(UINT32)) == 0) {
					pmotor_info->zoom_focus_tab[i-45].zoom_position = tmp[0];
					pmotor_info->zoom_focus_tab[i-45].focus_min = tmp[1];
					pmotor_info->zoom_focus_tab[i-45].focus_max = tmp[2];
#if MTR_SHOW_DBG_MSG
					DBG_DUMP("zoom_focus_tab:%d,%d,%d\n", pmotor_info->zoom_focus_tab[i-45].zoom_position, pmotor_info->zoom_focus_tab[i-45].focus_min, pmotor_info->zoom_focus_tab[i-45].focus_max);
#endif
					break;
				}
			}
		}
	}

	if (mtr_dtsi[0].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.protocol_type = mtr_dtsi[0].tmp_buf;
	}
	if (mtr_dtsi[1].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_chnel = mtr_dtsi[1].tmp_buf;
	}
	if (mtr_dtsi[2].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_mode = mtr_dtsi[2].tmp_buf;
	}
	if (mtr_dtsi[3].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_pkt_len = mtr_dtsi[3].tmp_buf;
	}
	if (mtr_dtsi[4].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_freq = mtr_dtsi[4].tmp_buf;
	}
	if (mtr_dtsi[5].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_order = mtr_dtsi[5].tmp_buf;
	}
	if (mtr_dtsi[6].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.cs_polarity = mtr_dtsi[6].tmp_buf;
	}
	if (mtr_dtsi[7].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.spi_cfg.spi_pkt_dly = mtr_dtsi[7].tmp_buf;
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("spi_cfg:%u,%u,%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_hw.protocol_type, pmotor_info->lens_hw.spi_cfg.spi_chnel, pmotor_info->lens_hw.spi_cfg.spi_mode, pmotor_info->lens_hw.spi_cfg.spi_pkt_len, pmotor_info->lens_hw.spi_cfg.spi_freq, pmotor_info->lens_hw.spi_cfg.spi_order, pmotor_info->lens_hw.spi_cfg.cs_polarity, pmotor_info->lens_hw.spi_cfg.spi_pkt_dly);
#endif

	if (mtr_dtsi[8].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.i2c_cfg.i2c_id = mtr_dtsi[8].tmp_buf;
	}
	if (mtr_dtsi[9].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.i2c_cfg.i2c_addr = mtr_dtsi[9].tmp_buf;
	}

#if MTR_SHOW_DBG_MSG
		DBG_DUMP("i2c_cfg:%u,0x%02X\n", pmotor_info->lens_hw.i2c_cfg.i2c_id, pmotor_info->lens_hw.i2c_cfg.i2c_addr);
#endif

	if (pmotor_info->lens_hw.protocol_type == 2) {
		if (mtr_dtsi[10].tmp_buf != 0xFFFFFFFF) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs = mtr_dtsi[10].tmp_buf;
		}
		if (mtr_dtsi[11].tmp_buf != 0xFFFFFFFF) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk = mtr_dtsi[11].tmp_buf;
		}
		if (mtr_dtsi[12].tmp_buf != 0xFFFFFFFF) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx = mtr_dtsi[12].tmp_buf;
		}
		if (mtr_dtsi[13].tmp_buf != 0xFFFFFFFF) {
			pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx = mtr_dtsi[13].tmp_buf;
		}
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("gpio_spi_cfg:%u,%u,%u,%u\n", pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_cs, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_clk, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_tx, pmotor_info->lens_hw.gpio_spi_cfg.gpio_spi_rx);
#endif

	if (mtr_dtsi[14].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pwm_cfg[2].pwm_mode = pmotor_info->lens_hw.pwm_cfg[1].pwm_mode = pmotor_info->lens_hw.pwm_cfg[0].pwm_mode = mtr_dtsi[14].tmp_buf;
	}
	if (mtr_dtsi[15].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pwm_cfg[0].pwm_freq = mtr_dtsi[15].tmp_buf;
	}
	if (mtr_dtsi[16].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pwm_cfg[0].pwm_port_sel = mtr_dtsi[16].tmp_buf;
	}
	if (mtr_dtsi[17].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pwm_cfg[1].pwm_freq = mtr_dtsi[17].tmp_buf;
	}
	if (mtr_dtsi[18].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pwm_cfg[1].pwm_port_sel = mtr_dtsi[18].tmp_buf;
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("pwm_cfg:%u,(%u,%u),(%u,%u)\n", pmotor_info->lens_hw.pwm_cfg[0].pwm_mode, pmotor_info->lens_hw.pwm_cfg[0].pwm_freq, pmotor_info->lens_hw.pwm_cfg[0].pwm_port_sel, pmotor_info->lens_hw.pwm_cfg[1].pwm_freq, pmotor_info->lens_hw.pwm_cfg[1].pwm_port_sel);
#endif

	if (mtr_dtsi[19].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_RESET] = MTR_MIN(mtr_dtsi[19].tmp_buf, 95);
	}
	if (mtr_dtsi[20].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN] = MTR_MIN(mtr_dtsi[20].tmp_buf, 95);
	}
	if (mtr_dtsi[21].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_FZ] = MTR_MIN(mtr_dtsi[21].tmp_buf, 95);
	}
	if (mtr_dtsi[22].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IS] = MTR_MIN(mtr_dtsi[22].tmp_buf, 95);
	}
	if (mtr_dtsi[23].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_FSTS] = MTR_MIN(mtr_dtsi[23].tmp_buf, 95);
	}
	if (mtr_dtsi[24].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_ZSTS] = MTR_MIN(mtr_dtsi[24].tmp_buf, 95);
	}
	if (mtr_dtsi[25].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0] = MTR_MIN(mtr_dtsi[25].tmp_buf, 95);
	}
	if (mtr_dtsi[26].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1] = MTR_MIN(mtr_dtsi[26].tmp_buf, 95);
	}
	if (mtr_dtsi[27].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT0] = MTR_MIN(mtr_dtsi[27].tmp_buf, 95);
	}
	if (mtr_dtsi[28].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT1] = MTR_MIN(mtr_dtsi[28].tmp_buf, 95);
	}
	if (mtr_dtsi[29].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT2] = MTR_MIN(mtr_dtsi[29].tmp_buf, 95);
	}
	if (mtr_dtsi[30].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_hw.pin_def[HW_GPIO_EXT3] = MTR_MIN(mtr_dtsi[30].tmp_buf, 95);
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("lens_hw:%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_hw.pin_def[HW_GPIO_RESET], pmotor_info->lens_hw.pin_def[HW_GPIO_POWER_DN], pmotor_info->lens_hw.pin_def[HW_GPIO_FZ], pmotor_info->lens_hw.pin_def[HW_GPIO_IS], pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT0], pmotor_info->lens_hw.pin_def[HW_GPIO_IRCUT1]);
#endif

	if (mtr_dtsi[31].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_proty.focal_len_min = mtr_dtsi[31].tmp_buf;
	}
	if (mtr_dtsi[32].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->lens_proty.focal_len_max = mtr_dtsi[32].tmp_buf;
	}
	if (mtr_dtsi[33].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[0].def_positive_dir = mtr_dtsi[33].tmp_buf;
	}
	if (mtr_dtsi[34].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[1].def_positive_dir = mtr_dtsi[34].tmp_buf;
	}
	if (mtr_dtsi[35].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[0].init_pos_ofst = mtr_dtsi[35].tmp_buf;
	}
	if (mtr_dtsi[36].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[1].init_pos_ofst = mtr_dtsi[36].tmp_buf;
	}
	if (mtr_dtsi[37].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[0].max_move_distance = mtr_dtsi[37].tmp_buf;
	}
	if (mtr_dtsi[38].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[1].max_move_distance = mtr_dtsi[38].tmp_buf;
	}
	if (mtr_dtsi[39].tmp_buf != 0xFFFFFFFF) {
		move_spd = mtr_dtsi[39].tmp_buf;
		move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

		if (move_spd == 20) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_QUARTER;
		} else if (move_spd == 40) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_HALF;
		} else if (move_spd == 60) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_1X;
		} else if (move_spd == 80) {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_2X;
		} else {
			pmotor_info->motor_config[0].move_speed = MTR_SPEED_4X;
		}
	}
	if (mtr_dtsi[40].tmp_buf != 0xFFFFFFFF) {
		move_spd = mtr_dtsi[40].tmp_buf;
		move_spd = MTR_CLAMP((move_spd / 20) * 20, 20, 100);

		if (move_spd == 20) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_QUARTER;
		} else if (move_spd == 40) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_HALF;
		} else if (move_spd == 60) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_1X;
		} else if (move_spd == 80) {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_2X;
		} else {
			pmotor_info->motor_config[1].move_speed = MTR_SPEED_4X;
		}
	}
	if (mtr_dtsi[41].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[0].gear_backlash = mtr_dtsi[41].tmp_buf;
	}
	if (mtr_dtsi[42].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[1].gear_backlash = mtr_dtsi[42].tmp_buf;
	}
	if (mtr_dtsi[43].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[0].dly_time = mtr_dtsi[43].tmp_buf;
	}
	if (mtr_dtsi[44].tmp_buf != 0xFFFFFFFF) {
		pmotor_info->motor_config[1].dly_time = mtr_dtsi[44].tmp_buf;
	}

#if MTR_SHOW_DBG_MSG
	DBG_DUMP("motor_config:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", pmotor_info->lens_proty.focal_len_min, pmotor_info->lens_proty.focal_len_max, pmotor_info->motor_config[0].def_positive_dir, pmotor_info->motor_config[1].def_positive_dir, pmotor_info->motor_config[0].init_pos_ofst, pmotor_info->motor_config[1].init_pos_ofst,
		pmotor_info->motor_config[0].max_move_distance, pmotor_info->motor_config[1].max_move_distance, pmotor_info->motor_config[0].move_speed, pmotor_info->motor_config[1].move_speed, pmotor_info->motor_config[0].gear_backlash, pmotor_info->motor_config[1].gear_backlash, pmotor_info->motor_config[0].dly_time, pmotor_info->motor_config[1].dly_time);
#endif

	return rt;
}
#endif


