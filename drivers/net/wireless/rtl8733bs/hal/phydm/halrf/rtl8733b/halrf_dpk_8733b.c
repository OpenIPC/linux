/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == 0x08)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8733B_SUPPORT == 1)

/*---------------------------Define Local Constant---------------------------*/

/*8733B DPK ver:0x06 20200803*/

void _backup_mac_bb_registers_8733b(struct dm_struct *dm,
				    u32 *reg,
				    u32 *reg_backup,
				    u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_read_4byte(dm, reg[i]);
#if (DPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup MAC/BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
}

void _backup_rf_registers_8733b(struct dm_struct *dm,
				u32 *rf_reg,
				u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM_8733B; i++) {
		rf_reg_backup[i][RF_PATH_A] = odm_get_rf_reg(dm, RF_PATH_A, rf_reg[i], RFREG_MASK);
		rf_reg_backup[i][RF_PATH_B] = odm_get_rf_reg(dm, RF_PATH_B, rf_reg[i], RFREG_MASK);
#if (DPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
}

void _reload_mac_bb_registers_8733b(struct dm_struct *dm,
				    u32 *reg,
				    u32 *reg_backup,
				    u32 reg_num)

{
	u32 i;

	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);

	for (i = 0; i < reg_num; i++) {
		odm_write_4byte(dm, reg[i], reg_backup[i]);
#if (DPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload MAC/BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x0);
	odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x1);
}

void _reload_rf_registers_8733b(struct dm_struct *dm,
				u32 *rf_reg,
				u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM_8733B; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_B]);
#if (DPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
}

void _dpk_information_8733b(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u32  reg_rf18;

#if 0
	if (odm_get_bb_reg(dm, R_0x1e7c, BIT(30)))
		dpk_info->is_tssi_mode = true;
	else
		dpk_info->is_tssi_mode = false;
#endif
	dpk_info->dpk_current_path = (u8)odm_get_bb_reg(dm, R_0x1884, BIT(20));

	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);

	dpk_info->dpk_band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	dpk_info->dpk_ch = (u8)reg_rf18 & 0xff;
	dpk_info->dpk_bw = (u8)((reg_rf18 & BIT(10)) >> 10); /*1/0:20/40*/

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] Path/ Band/ CH/ BW = S%d / %s / %d / %s\n",
	       dpk_info->dpk_current_path,
	       dpk_info->dpk_band == 0 ? "2G" : "5G",
	       dpk_info->dpk_ch,
	       dpk_info->dpk_bw == 1 ? "20M" : "40M");
}

void _dpk_tx_pause_8733b(struct dm_struct *dm)
{
	u8 reg_rf0_a, reg_rf0_b;
	u16 count = 0;

	odm_write_1byte(dm, R_0x522, 0xff);
	odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2); /*hw tx stop*/

	reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
	reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);

	while (((reg_rf0_a == 2) || (reg_rf0_b == 2)) && count < 2500) {
		reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
		reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);
		ODM_delay_us(2);
		count++;
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] Tx pause!!\n");

}

void _dpk_mac_bb_setting_8733b(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	_dpk_tx_pause_8733b(dm);

	/*01 AFE 0N BB setting*/
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);

	odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x0); /*r_path_en_en*/
	odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x1); /*path_en_seg0_sel*/
	odm_set_bb_reg(dm, R_0x824, 0x000f0000, 0x1); /*path_en_seg0_sel*/

	odm_set_bb_reg(dm, R_0x1cd0, 0xf0000000, 0x7); /*IQK clk on*/

	/*Block CCA*/
	/*Prevent CCKCCA at sine PSD*/
	odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1); /*CCK CCA*/
	odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x1); /*Prevent OFDM CCA*/

	/*trx gating clk force on*/
	odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x1);

	odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);  /*go_through_iqk*/

	/*AFE ADDA both ON setting*/
	/*ADDA fifo force off*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0); /*force ADDA*/
	odm_set_bb_reg(dm, R_0x1860, 0xf0000000, 0xf); /*ADDA all on*/
	odm_set_bb_reg(dm, R_0x1860, 0x0ffff000, 0x0041); /*ADDA gated on*/
	/*AD CLK rate:80M*/
	odm_set_bb_reg(dm, R_0x9f0, MASKLWORD, 0xbbbb);
	odm_set_bb_reg(dm, R_0x1d40, BIT(3), 0x1);
	odm_set_bb_reg(dm, R_0x1d40, 0x00000007, 0x3);
	/*DA CLK rate:160M*/
	odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x3);
	odm_set_bb_reg(dm, R_0x9b4, 0x00003800, 0x3);
	odm_set_bb_reg(dm, R_0x9b4, 0x0001C000, 0x3);
	odm_set_bb_reg(dm, R_0x9b4, 0x000E0000, 0x3);
	odm_set_bb_reg(dm, R_0x1c20, BIT(5), 0x1);
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xFFFFFFFF);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] MAC/BB setting for DPK mode\n");
}

u8 _dpk_rf_setting_8733b(struct dm_struct *dm,	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u8 txagc;

	if (dpk_info->dpk_band == 0x0) { /*2G*/

		odm_set_rf_reg(dm, RF_PATH_A, RF_0x5, BIT(0), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, RFREG_MASK, 0x50000);
		if (path == RF_PATH_A) {
			/*TXAGC */
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x1, MASKBYTE0, 0x1D);
			/*ATT Gain 000/001/011/111=-31/-37/-43/-49dB*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x83, 0x00007, 0x2);
		} else{

			odm_set_rf_reg(dm, RF_PATH_B, RF_0x5, BIT(0), 0x0);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, RFREG_MASK, 0x50000);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x1, MASKBYTE0, 0x1A);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x83, 0x00007, 0x7);
		}

		/*R1 Gain 0x0/1/3/7/f =-27/-21/-13/-9/-3.5dB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x83, 0x000f0, 0x3);
		/*TIA gain -6db*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf, BIT(12), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x9e, BIT(8), 0x1);
		/*PGA gain 2db/step*/
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, BIT(1), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x0e000, 0x3);
	} else { /*5G*/

		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5, BIT(0), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK, 0x50000);
		/*TXAGC */
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1, MASKBYTE0, 0x1D);

		/*ATT Gain 000~111=-27.3db ~-36.7dB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8c, 0x0e000, 0x7);

		/*R1 Gain 00/01/10/11 = 5/8/20/20dB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8c, 0x01800, 0x0);
		/*TIA gain -6db*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf, BIT(12), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x9e, BIT(8), 0x1);

		/*PGA gain 2db/step*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8f, BIT(1), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8f, 0x0e000, 0x3);
	}
	txagc = (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1, 0x0001f);
#if 1
	for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] S%d txagc=0x%x, RF_0x0=0x%x, 0x5=0x%x, 0x83=0x%x, 0x8f=0x%x\n",
		       path, txagc,
		       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK),
		       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x5, RFREG_MASK),
		       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x83, RFREG_MASK),
		       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x8f, RFREG_MASK));
	}
#endif
	return txagc;
}

u8 _dpk_timing_sync_report_8733b(struct dm_struct *dm,	u8 path)
{
	u8 fail_report = 0, sync_done = 0;
	u16 count = 0;

	/*Driver waits NCTL sync done*/
	sync_done = (u8)odm_get_bb_reg(dm, R_0x2d9c, MASKBYTE0) == 0x55;
	while (sync_done != 0x1 && count < 1000) {
		ODM_delay_us(20);
		if ((u8)odm_get_bb_reg(dm, R_0x2d9c, MASKBYTE0) == 0x55)
			sync_done = 1;
		count++;
	}
	//RF_DBG(dm, DBG_RF_DPK, "[DPK] timing sync count %d !!\n", count);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] timing sync 0x2d9c = 0x%x !!\n",
	       odm_get_bb_reg(dm, R_0x2d9c, MASKBYTE0));

	if (count == 1000)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] timing sync over 20ms!!\n");

	ODM_delay_ms(1);

	fail_report = odm_get_bb_reg(dm, R_0x1b08, BIT(26)) & 0x1;
#if 0
	while (fail_report != 0x0 && count < 500) {
		ODM_delay_us(10);
		if ((u8)odm_get_bb_reg(dm, R_0x1b08, BIT(26)) == 0x0)
			fail_report = 0;
		count++;
	}
	RF_DBG(dm, DBG_RF_DPK, "[DPK] fail report count %d !!\n", count);
#endif
	RF_DBG(dm, DBG_RF_DPK, "[DPK] NCTL TimingSync Do %s!!!\n",
	       fail_report ? "Fail" : "Success");

	return fail_report;
}

u8 _dpk_one_shot_8733b(struct dm_struct *dm, u8 path, u8 action)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 result = 0;
	u16 shot_code;

	//TBD
	//btc_set_gnt_wl_bt_8733b(dm, true);

	if (path == RF_PATH_A)
		shot_code = 0x0018;
	else
		shot_code = 0x002a;

	switch (action) {
	case GAIN_LOSS:
		if (path == RF_PATH_A)
			odm_write_1byte(dm, R_0x1bef, 0xa2);
		else
			odm_write_1byte(dm, R_0x1bef, 0xaa);
		shot_code |= 0x1100;
		break;
	case DPK_PAS:
		odm_write_1byte(dm, R_0x1bef, 0x2a);
		shot_code |= 0x1100;
		break;
	case DO_DPK:
		shot_code |= 0x1300;
		break;
	case DPK_ON:
		shot_code |= 0x1400;
		break;
	default:
		break;
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] one-shot for Action %d = %x\n",
	       action, shot_code);

	/*one shot*/
	odm_write_2byte(dm, R_0x1b00, shot_code);
	odm_write_2byte(dm, R_0x1b00, shot_code + 1);
	ODM_delay_us(20);

	if (action != DPK_ON)
		result = _dpk_timing_sync_report_8733b(dm, path);

	dpk_info->one_shot_cnt++;

	//btc_set_gnt_wl_bt_8733b(dm, false);

	return result; /*LMS fail report*/
}

u8 _dpk_thermal_read_8733b(void *dm_void,	u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_rf_reg(dm, RF_PATH_A, RF_0x42, BIT(19), 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x42, BIT(19), 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x42, BIT(19), 0x1);
	ODM_delay_us(15);

	return (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x42, 0x0007e);
}

u32 _dpk_dbg_report_read_8733b(struct dm_struct *dm, u8 index)
{
	u32 reg_1bfc;

	odm_write_1byte(dm, R_0x1bd6, index);
	reg_1bfc = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

	RF_DBG(dm, DBG_RF_DPK, "[DPK][DBG] idx 0x%x = 0x%x\n", index, reg_1bfc);

	return reg_1bfc;
}

void _dpk_pas_read_8733b(struct dm_struct *dm,	u8 path)
{
	u8 k, j;
	u32 reg_1bfc;

	odm_set_bb_reg(dm, R_0x1bcc, BIT(26), 0x0);

	for (k = 0; k < 8; k++) {
		odm_set_bb_reg(dm, R_0x1b90, MASKDWORD, 0x0105e038 + k);
		for (j = 0; j < 4; j++) {
			reg_1bfc = _dpk_dbg_report_read_8733b(dm, 0x06 + j);
#if 0
			RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d PAS read = 0x%x\n",
			       path, reg_1bfc);
#endif
		}
	}
}

boolean _dpk_lms_iq_check_8733b(struct dm_struct *dm,
				u8 addr,
				u32 reg_1bfc)
{
	u32 i_val = 0, q_val = 0;

	if (DPK_SRAM_IQ_DBG_8733B && addr < 16)
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] 0x1bfc[%2d] = 0x%x\n", addr, reg_1bfc);
#if 0
	if (reg_1bfc > 0x100000) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] LUT over 0db! LMS FAIL!!!\n");
		return 1;
	}
#endif
	i_val = (reg_1bfc & 0x003FF800) >> 11;
	q_val = reg_1bfc & 0x000007FF;

	if (((q_val & 0x400) >> 10) == 1)
		q_val = 0x800 - q_val;

	if (addr == 0 && ((i_val * i_val + q_val * q_val) < 0x197a9)) {
		/* LMS (I^2 + Q^2) < -4dB happen*/
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] LUT < -4dB happen, I=0x%x, Q=0x%x\n",
		       i_val, q_val);
		return 1;
	} else {
		return 0;
	}
	}

u8 _dpk_lut_sram_read_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 i;
	u32 reg_1bfc = 0;

	odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0x8);
	odm_write_1byte(dm, R_0x1b08, 0x80);

	/*even*/
	odm_write_1byte(dm, R_0x1bd6, 0x42);

	for (i = 0; i < 0x10; i++) {
		odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0xc0000081 | path << 5 | (i << 1));
		reg_1bfc = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		if (_dpk_lms_iq_check_8733b(dm, i, reg_1bfc))
			return 0;
		}

	/*odd*/
	odm_write_1byte(dm, R_0x1bd6, 0x43);

	for (i = 0; i < 0x10; i++) {
		odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0xc0000081 | path << 5 | (i << 1));
		reg_1bfc = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		if (_dpk_lms_iq_check_8733b(dm, i, reg_1bfc))
			return 0;
		}

	odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0x00000000);

	return 1;
}

void _dpk_lut_sram_clear_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	u8 i;

	odm_write_1byte(dm, R_0x1b00, 0x08);
	odm_write_1byte(dm, R_0x1b08, 0x80);

	//clear even
	for (i = 0; i < 0x20; i++)
		odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0xd0000001 | (i << 1));
	//clear odd
	for (i = 0; i < 0x20; i++)
		odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0x90000081 | (i << 1));
	odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0x00000000);
	/*GainLoss = 1db*/
	odm_set_bb_reg(dm, R_0x1bbc, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x1be8, MASKDWORD, 0x40004000);
}

void _dpk_manual_lut_write_8733b(struct dm_struct *dm)
{
	u8 i;
	u32 lut07;

	/*Fill lut 1-6th with lut7 value to fixed DPK Kfail issue*/
	odm_write_1byte(dm, R_0x1b00, 0x08);
	odm_write_1byte(dm, R_0x1b08, 0x80);

	//read even (LUT table 7th entrie)
	odm_write_1byte(dm, R_0x1bd6, 0x42);
	odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0xc0000087);
	lut07 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

	//write even
	/*enable lut even write*/
	odm_set_bb_reg(dm, R_0x1bdc, BIT(31) | BIT(30), 0x3);
	/*select even 0-2th entrie*/
	for (i = 0; i < 0x3; i++) {
		odm_write_1byte(dm, R_0x1bdc, 0x01 | (i << 1));
		odm_set_bb_reg(dm, R_0x1bdc, 0x3fffff00, lut07);
	}
	//write odd
	odm_set_bb_reg(dm, R_0x1bdc, BIT(31) | BIT(30), 0x2);
	/*select even 0-2th entrie*/
	for (i = 0; i < 0x3; i++) {
		odm_write_1byte(dm, R_0x1bdc, 0x81 | (i << 1));
		odm_set_bb_reg(dm, R_0x1bdc, 0x3fffff00, lut07);
	}
	odm_set_bb_reg(dm, R_0x1bdc, MASKDWORD, 0x00000000);
}

u32 _dpk_gainloss_result_8733b(struct dm_struct *dm, u8 path, u8 item)
{
	u32 result;

	switch (item) {
	case GL_BACK_VALUE:
		odm_set_bb_reg(dm, R_0x1bcc, BIT(26), 0x1);
		odm_set_bb_reg(dm, R_0x1b90, MASKDWORD, 0x0105e038);
		result = _dpk_dbg_report_read_8733b(dm, 0x06);
		RF_DBG(dm, DBG_RF_DPK, "[DPK][GL_CHECK] TXAGC_BACKOFF = 0x%x\n", result);
		break;

	case LOSS_CHK:
		/*the first point*/
		odm_set_bb_reg(dm, R_0x1bcc, BIT(26), 0x0);
		odm_set_bb_reg(dm, R_0x1b90, MASKDWORD, 0x0105e038);
		result = _dpk_dbg_report_read_8733b(dm, 0x06);
		RF_DBG(dm, DBG_RF_DPK, "[DPK][GL_CHECK] Loss = 0x%x\n", result);
		break;

	case GAIN_CHK:
		/*the last point*/
		odm_set_bb_reg(dm, R_0x1bcc, BIT(26), 0x0);
		odm_set_bb_reg(dm, R_0x1b90, MASKDWORD, 0x0105e03f);
		result = _dpk_dbg_report_read_8733b(dm, 0x09);
		RF_DBG(dm, DBG_RF_DPK, "[DPK][GL_CHECK] Gain = 0x%x\n", result);
		break;

	default:
		result = 0;
		break;
	}
	return result;
}

u8 _dpk_agc_tune_8733b(struct dm_struct *dm, u8 path, u8 ori_agc)
{
	u8 agc_backoff, new_agc = ori_agc;
	u8 result = 0;
	u32 loss = 0;

#if (DPK_PAS_DBG_8733B)
	_dpk_pas_read_8733b(dm, path);
#endif

	/*check auto_pga fail & retry*/
	loss = _dpk_gainloss_result_8733b(dm, path, LOSS_CHK);
	if(loss > 0x3FF0000) {
		result = 4;
		RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] GLoss overflow happen!!\n");
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK][AGC] 0x1b3c=0x%x\n",
		       odm_get_bb_reg(dm, R_0x1b3c, MASKDWORD));
		return result;
	}

	agc_backoff = (u8)_dpk_gainloss_result_8733b(dm, path, GL_BACK_VALUE);

	if (agc_backoff < 0x5) {
		result = 1;
		return result;
	} else if (agc_backoff == 0xA) {
		result = 2;
		return result;
	} else if (agc_backoff < 0xA && agc_backoff > 0x4) {
		result = 3;
		new_agc = ori_agc - (0xA - agc_backoff);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1, 0x0001F, new_agc);
		ODM_delay_us(10);
		RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] new_agc=0x%x\n", new_agc);
		return result;
	}
	/*check fail & retry*/
	//TBD
	//gain = _dpk_gainloss_result_8733b(dm, path, GAIN_CHK);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] check fail & retry !!!!!!\n");

	return result;
}

u8 _dpk_gainloss_auto_agc_8733b(struct dm_struct *dm, u8 path, u8 ori_agc)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 tmp_txagc = 0, ori_pga = 0, auto_pga = 0, i = 0;
	u8 goout = 0, agc_cnt = 0, agc_done = 0;
	boolean limit_pga = 0;

	ori_pga = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x0e000);
	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK][AGC] Start TXAGC=0x%x, PGA=0x%x\n",
	       ori_agc, ori_pga);

	do {
		switch (i) {
		case 0: /*AUTO PGA*/
			tmp_txagc = (u8)odm_get_rf_reg(dm, (enum rf_path)path,
						     RF_0x1, 0x0001f);
			_dpk_one_shot_8733b(dm, path, GAIN_LOSS);
#if 0
			RF_DBG(dm, DBG_RF_DPK,
			       "[DPK][AGC] PAScan AMAM before AUTO AGC!!\n");
			_dpk_pas_read_8733b(dm, path);
#endif
			auto_pga = _dpk_dbg_report_read_8733b(dm, 0x02) >> 16 & 0x7;
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x0e000, auto_pga);
			limit_pga = (auto_pga == 0x6) ? 1 : 0;
			ODM_delay_us(10);
			RF_DBG(dm, DBG_RF_DPK,
			       "[DPK][AGC] Limit_PGA=0x%x, RF_0x8f=0x%x\n",
			       limit_pga,
			       odm_get_rf_reg(dm, RF_PATH_A, RF_0x8f, RFREG_MASK));
			_dpk_one_shot_8733b(dm, path, DPK_PAS);
			i = _dpk_agc_tune_8733b(dm, path, tmp_txagc);
			agc_cnt++;
			break;

		case 1: /*GL_BACK < 0x5*/
			if (tmp_txagc < 0x2) {
				goout = 1;
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] TXAGC@ lower bound!!\n");
				break;
			}
			tmp_txagc = tmp_txagc - 2;
			odm_set_rf_reg(dm, (enum rf_path)path,
				       RF_0x1, 0x0001f, tmp_txagc);
			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] TXAGC(-2) = 0x%x\n",
			       tmp_txagc);
			i = 0;
			break;

		case 2:	/*GL_BACK = 0xA*/
			if (tmp_txagc == 0x1f) {
				goout = 1;
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] TXAGC@ upper bound!!\n");
				break;
			}
			tmp_txagc = tmp_txagc + 1;
			odm_set_rf_reg(dm, (enum rf_path)path,
				       RF_0x1, 0x0001f, tmp_txagc);
			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] TXAGC(+1) = 0x%x\n",
			       tmp_txagc);
			i = 0;
			break;
		case 3:
			agc_done = 1;

			if (!limit_pga) {
				auto_pga += (0xa - (u8)_dpk_gainloss_result_8733b(dm, path, GL_BACK_VALUE)) / 2;
				if (auto_pga > 0x6)
					auto_pga = 0x06;
				odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x0e000, auto_pga);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] new_PGA=0x%x, RF_0x8f=0x%x\n",
				       auto_pga,
				       odm_get_rf_reg(dm, RF_PATH_A, RF_0x8f, RFREG_MASK));
			}
			goout = 1;
			break;
		case 4: /*AUTO PGA fail--gain overflow*/
			if (auto_pga > 0)
				odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x0e000, 0x0);
			else if (!auto_pga && dpk_info->dpk_bw)
				/*R1 Gain 0x0/1/3/7/f =-27/-21/-13/-9/-3.5dB*/
				odm_set_rf_reg(dm, (enum rf_path)path, RF_0x83, 0x000f0, 0x1);
			else
				goout = 1;
			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] AUTO PGA fail!!!\n");
			i = 0;
			agc_cnt += 3;
			break;
		default:
			goout = 1;
			break;
		}
	} while (!goout && (agc_cnt < 6));

	RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] cnt %d, agc_done = 0x%x\n",
	       agc_cnt, agc_done);
	return agc_done;
}

u8 _dpk_gainloss_8733b(struct dm_struct *dm,	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 agc_done = 0, t1 = 0, t2 = 0;
	u8 tx_agc = 0, ori_txagc;

	ori_txagc = _dpk_rf_setting_8733b(dm, path);

	t1 = _dpk_thermal_read_8733b(dm, path);

	odm_write_1byte(dm, R_0x1b00, 0x08);
	odm_write_1byte(dm, R_0x1bd8, 0x00); /*RXSRAM*/

	/*TPG BW select*/
	if (dpk_info->dpk_bw == 1)
		odm_set_bb_reg(dm, R_0x1bf8, MASKDWORD, 0xd2000065); /*20M*/
	else
		odm_set_bb_reg(dm, R_0x1bf8, MASKDWORD, 0xd2000068); /*40M*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] TPG select for %s\n",
	       dpk_info->dpk_bw  ? "20M" : "40M");

	/*RXIQC fill default value*/
	odm_set_bb_reg(dm, R_0x1b3c, 0xFFFFFF00, 0x200000);

	odm_write_1byte(dm, R_0x1be3, 0x20); /*bypass RX DC_i/q*/
	odm_set_bb_reg(dm, R_0x1b88, MASKDWORD, 0x00b48000);

	agc_done = _dpk_gainloss_auto_agc_8733b(dm, path, ori_txagc);

#if (DPK_PAS_CHK_DBG_8733B)
		_dpk_dbg_report_read_8733b(dm, 0x01); /*LUT_gain*/
		_dpk_dbg_report_read_8733b(dm, 0x02); /*pag2*/
		_dpk_dbg_report_read_8733b(dm, 0x0C);
		_dpk_dbg_report_read_8733b(dm, 0x0D); /*RX DC_IQ*/
		_dpk_dbg_report_read_8733b(dm, 0x10);
#endif

	t2 = _dpk_thermal_read_8733b(dm, path);

	dpk_info->thermal_dpk[path] = t1;

	dpk_info->dpk_delta_thermal[path] = HALRF_ABS(t2, t1);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d thermal delta of GL = %d (%d - %d)\n",
	       path, dpk_info->dpk_delta_thermal[path], t2, t1);

	return agc_done;
}

u8 _dpk_one_path_8733b(struct dm_struct *dm,	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 tx_agc, result = 0;
	s16 pwsf_offset = 0;

	tx_agc = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1,
				RFREG_MASK) & 0x1f;

	dpk_info->txagc[path] = tx_agc;
	RF_DBG(dm, DBG_RF_DPK, "[DPK][DO_DPK] RF0x1 = 0x%x\n",
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1, RFREG_MASK));

	/*PWSF+1 = 0.125db; +8 = +1db*/
	pwsf_offset = (((0x19 - tx_agc) << 3) + 0x50) & 0x1ff;

	RF_DBG(dm, DBG_RF_DPK, "[DPK][DO_DPK] PWSF = 0x%x\n", pwsf_offset);

	if (path == RF_PATH_A)
		odm_set_bb_reg(dm, R_0x1bd8, 0x001ff000, pwsf_offset);
	else
		odm_set_bb_reg(dm, R_0x1bd8, 0x3fe00000, pwsf_offset);
	dpk_info->pwsf[path] = pwsf_offset;

	/*LUT_point 6th selected to do auto gainscaling cal*/
	odm_set_bb_reg(dm, R_0x1bec, 0x00e00000, 0x6);

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK][DO_DPK] S%d RF 0x1=0x%x, R_0x1bd8=0x%x\n",
	       path, odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1, RFREG_MASK),
	       odm_get_bb_reg(dm, R_0x1bd8, MASKDWORD));

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK][DO_DPK] =========== DPK fail report  ===========\n");
	result = _dpk_one_shot_8733b(dm, path, DPK_PAS);
	_dpk_dbg_report_read_8733b(dm, 0x0a);

	/*LMS only one-shot*/
	result = result | _dpk_one_shot_8733b(dm, path, DO_DPK);
	_dpk_dbg_report_read_8733b(dm, 0x0a);

	RF_DBG(dm, DBG_RF_DPK, "[DPK][DO_DPK] DPK Fail = %x\n", result);

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK][DO_DPK] =========== DPK fail report  ===========\n");

	/*dpk Kfail setting*/
	odm_write_2byte(dm, R_0x1bce, 0x1116);
	odm_set_bb_reg(dm, R_0x1b20, BIT(25) | BIT(24), 0x0);

#if (DPK_LMS_DBG_8733B)
	_dpk_pas_read_8733b(dm, path);
#endif
	return result;
}

void _dpk_on_8733b(struct dm_struct *dm, u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u8 rf_txagc = 0x11;

	/*PA not all on bypass dpk s0TXA=0xA s0TXG=0x11 s1TXG=0x10*/
	if (dpk_info->dpk_band == 0)
		rf_txagc = 0x11;
	else
		rf_txagc = 0xa;

	_dpk_one_shot_8733b(dm, path, DPK_ON);
	_dpk_manual_lut_write_8733b(dm);

	/*low rate bypass dpk*/
	odm_set_bb_reg(dm, R_0x1bcc, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x1bbc, 0x30000000, 0x3);
	if (path == RF_PATH_A) {
		/*ant0: 1F ; 11;*/
		odm_set_bb_reg(dm, R_0x1bbc, 0x0000001f, 0x1f);
		odm_set_bb_reg(dm, R_0x1bbc, 0x000003e0, rf_txagc);
	} else if (path == RF_PATH_B) {
		/*ant1: 1F; 10;*/
		odm_set_bb_reg(dm, R_0x1bbc, 0x00007c00, 0x1f);
		odm_set_bb_reg(dm, R_0x1bbc, 0x000f8000, rf_txagc - 1);
	}

	odm_set_bb_reg(dm, R_0x1bbc, BIT(27), 0x0); /* GainScaling AUTO*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DPD on!!!\n\n", path);
}

u8 _dpk_check_fail_8733b(struct dm_struct *dm,
			 boolean is_fail,
			 u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 result = 0;

	if (!is_fail && _dpk_lut_sram_read_8733b(dm, path)) {
		dpk_info->dpk_path_ok = dpk_info->dpk_path_ok | (1 << path);
		result = 1; /*check PASS*/
	}

	return result;
}

void _dpk_result_reset_8733b(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u8 path;

	dpk_info->dpk_path_ok = 0x0;
	dpk_info->one_shot_cnt = 0;

	for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {
		dpk_info->txagc[path] = 0;
		dpk_info->pwsf[path] = 0;
		dpk_info->last_offset[path] = 0;
		dpk_info->thermal_dpk[path] = 0;

		RF_DBG(dm, DBG_RF_DPK, "[DPK] init thermal S%d = %d\n", path,
		       dpk_info->thermal_dpk[path]);
	}
}

void _dpk_calibrate_8733b(struct dm_struct *dm,	u8 path)
{
	u8 dpk_fail = 1, retry_cnt;
	u8 agc_done = 0;
	u32 rf_mode;

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] =========== S%d DPK Start ===========\n", path);

	/*backup RF mode*/
	rf_mode = odm_get_rf_reg(dm, path, 0x0, RFREG_MASK);

	for (retry_cnt = 0; retry_cnt < 4; retry_cnt++) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] retry = %d\n", retry_cnt);

		agc_done = _dpk_gainloss_8733b(dm, path);

		if (agc_done)
			dpk_fail = _dpk_one_path_8733b(dm, path);

		if (_dpk_check_fail_8733b(dm, dpk_fail, path))
			break;

		/* resset RF mode if kfial*/
		odm_set_rf_reg(dm, path, 0x0, RFREG_MASK, rf_mode);
		RF_DBG(dm, DBG_RF_DPK, "[DPK]restore S%d rf0 = 0x%x\n",
		       path, odm_get_rf_reg(dm, path, 0x0, RFREG_MASK));
	}

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] =========== S%d DPK Finish ==========\n", path);
}

void _dpk_path_select_8733b(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

#if 1
	u8 path = dpk_info->dpk_current_path;

	_dpk_calibrate_8733b(dm, path);
	if ((dpk_info->dpk_path_ok & BIT(path)) >> path)
		_dpk_on_8733b(dm, path);
#else
#if (DPK_PATH_A_8733B)
	//odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x0);
	_dpk_calibrate_8733b(dm, RF_PATH_A);
	if ((dpk_info->dpk_path_ok & BIT(RF_PATH_A)) >> RF_PATH_A)
		_dpk_on_8733b(dm, RF_PATH_A);
#endif

#if (DPK_PATH_B_8733B)
	//odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x1);
	_dpk_calibrate_8733b(dm, RF_PATH_B);
	if ((dpk_info->dpk_path_ok & BIT(RF_PATH_B)) >> RF_PATH_B)
		_dpk_on_8733b(dm, RF_PATH_B);

#endif
#endif

}

void _dpk_result_summary_8733b(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	RF_DBG(dm, DBG_RF_DPK, "[DPK] ======== DPK Result Summary =======\n");

	for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] S%d txagc = 0x%x, pwsf offset = 0x%x\n",
		       path, dpk_info->txagc[path], dpk_info->pwsf[path]);

		RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DPK is %s\n", path,
		       ((dpk_info->dpk_path_ok & BIT(path)) >> path) ?
		       "Success" : "Fail");
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_path_ok = 0x%x\n",
	       dpk_info->dpk_path_ok);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_one_shot_cnt = 0x%x\n",
	       dpk_info->one_shot_cnt);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] ======== DPK Result Summary =======\n");
}

void dpk_reload_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	return;
}

void _dpk_force_bypass_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_write_1byte(dm, R_0x1bcc, 0x00);

	odm_set_bb_reg(dm, R_0x1b38, BIT(1) | BIT(0), 0x1);
	odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x1);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S0 DPK Force bypass !!!\n");

	odm_set_bb_reg(dm, R_0x1b38, BIT(1) | BIT(0), 0x3);
	odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x1);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S1 DPK Force bypass !!!\n");

	odm_set_bb_reg(dm, R_0x1b38, BIT(1) | BIT(0), 0x0);
}

void do_dpk_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	struct _hal_rf_ *rf = &dm->rf_table;

	u32 bb_reg_backup[DPK_BB_REG_NUM_8733B];
	u32 rf_reg_backup[DPK_RF_REG_NUM_8733B][DPK_RF_PATH_NUM_8733B];

	u32 bb_reg[DPK_BB_REG_NUM_8733B] = {R_0x522,
		R_0x1884, R_0x9f0, R_0x2a24, R_0x1830, R_0x1d40,
		R_0x1b38, R_0x1b3c, R_0x1bb8, R_0x1bf8, R_0x1e70,
		R_0x1c38, R_0x1c68, R_0x1864, R_0x180c, R_0x1880};
	u32 rf_reg[DPK_RF_REG_NUM_8733B] = {
		RF_0x5, RF_0x83, RF_0x8c, RF_0x8f, RF_0x9e,
		RF_0xde, RF_0xdf, RF_0xef};

	if (!dpk_info->is_dpk_pwr_on) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Skip DPK due to DPD PWR off !!\n");
		_dpk_lut_sram_clear_8733b(dm);
		_dpk_result_reset_8733b(dm);
		return;
	}

	if (!dpk_info->is_dpk_enable) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Force Bypass DPK !!\n");
		_dpk_force_bypass_8733b(dm);
		return;
	}

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] ************* DPK Start *************\n");

	_dpk_information_8733b(dm);
	_dpk_result_reset_8733b(dm);
	_backup_mac_bb_registers_8733b(dm, bb_reg, bb_reg_backup,
				       DPK_BB_REG_NUM_8733B);
	_backup_rf_registers_8733b(dm, rf_reg, rf_reg_backup);

	_dpk_mac_bb_setting_8733b(dm);
	_dpk_path_select_8733b(dm);
	_dpk_result_summary_8733b(dm);
	dpk_enable_disable_8733b(dm);

	_reload_rf_registers_8733b(dm, rf_reg, rf_reg_backup);
	_reload_mac_bb_registers_8733b(dm, bb_reg, bb_reg_backup,
				       DPK_BB_REG_NUM_8733B);
}

void dpk_enable_disable_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {
		if ((dpk_info->dpk_path_ok & BIT(path)) >> path) {
			if (dpk_info->is_dpk_enable) {
				odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x0);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK] S%d DPK enable !!!\n", path);
			} else {
				odm_write_1byte(dm, R_0x1bcc, 0x00);
				odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x1);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK] S%d DPK bypass !!!\n", path);
			}
		}
	}
}

void dpk_track_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	struct _hal_rf_ *rf = &dm->rf_table;
#if 1
	u8 i = 0, k = 0, path;
	u8 thermal_dpk_avg_count = 0, thermal_value[2] = {0};
	u32 new_pwsf[2] = {0};
	u32 thermal_dpk_avg[2] = {0};
	s8 offset[2], delta_dpk[2];

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	if (*dm->is_fcs_mode_enable) /*fast channel switch*/
		return;
#endif
	if (rf->is_dpk_in_progress || dm->rf_calibrate_info.is_iqk_in_progress ||
	    rf->is_tssi_in_progress || rf->is_txgapk_in_progress ||
	    !(rf->rf_supportability & HAL_RF_DPK_TRACK))
		return;

	if (dpk_info->thermal_dpk[0] == 0 && dpk_info->thermal_dpk[1] == 0) {
		RF_DBG(dm, DBG_RF_DPK_TRACK, "[DPK_track] Bypass DPK tracking!!!!\n");
			return;
	} else {
		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] ================[CH %d]================\n",
		       dpk_info->dpk_ch);
	}

	path = dpk_info->dpk_current_path;

	/*get current thermal meter*/
	thermal_value[path] = _dpk_thermal_read_8733b(dm, path);
	dpk_info->thermal_dpk_avg[path][dpk_info->thermal_dpk_avg_index] =
		thermal_value[path];

	RF_DBG(dm, DBG_RF_DPK_TRACK,
	       "[DPK_track] S%d thermal now = %d\n", path, thermal_value[path]);

	dpk_info->thermal_dpk_avg_index++;

	/*Average times */
	if (dpk_info->thermal_dpk_avg_index == THERMAL_DPK_AVG_NUM)
		dpk_info->thermal_dpk_avg_index = 0;

	for (i = 0; i < THERMAL_DPK_AVG_NUM; i++) {
		if (dpk_info->thermal_dpk_avg[path][i]) {
			thermal_dpk_avg[path] += dpk_info->thermal_dpk_avg[path][i];
			thermal_dpk_avg_count++;
		}
	}

	/*Calculate Average ThermalValue after average enough times*/
	if (thermal_dpk_avg_count) {
#if 0
		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S%d ThermalValue_DPK_AVG (count) = %d (%d))\n",
		       path, thermal_dpk_avg[path], thermal_dpk_avg_count);
#endif
		thermal_value[path] = (u8)(thermal_dpk_avg[path] / thermal_dpk_avg_count);

		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S%d thermal avg = %d (DPK @ %d)\n",
		       path, thermal_value[path], dpk_info->thermal_dpk[path]);
	}

	/*for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {*/
		delta_dpk[path] = dpk_info->thermal_dpk[path] - thermal_value[path];

		offset[path] = delta_dpk[path] - dpk_info->dpk_delta_thermal[path];

	RF_DBG(dm, DBG_RF_DPK_TRACK,
	       "[DPK_track] S%d thermal_diff= %d, cal_diff= %d, offset= %d\n",
	       path, delta_dpk[path], dpk_info->dpk_delta_thermal[path],
	       offset[path] > 128 ? offset[path] - 256 : offset[path]);
	//}

	/*for (path = 0; path < DPK_RF_PATH_NUM_8733B; path++) {*/
		if (offset[path] != dpk_info->last_offset[path]) {
			if (path == RF_PATH_A) {
				new_pwsf[0] = (dpk_info->pwsf[0] + offset[path]) & 0x1ff;
				odm_set_bb_reg(dm, R_0x1bd8, 0x001ff000, new_pwsf[0]);
			} else {
				new_pwsf[1] = (dpk_info->pwsf[1] + offset[path]) & 0x1ff;
				odm_set_bb_reg(dm, R_0x1bd8, 0x3fe00000, new_pwsf[1]);
			}
			dpk_info->last_offset[path] = offset[path];
			RF_DBG(dm, DBG_RF_DPK_TRACK,
			       "[DPK_track] S%d new pwsf is 0x%x, 0x1bd8=0x%x\n",
			       path, new_pwsf[path],
			       odm_get_bb_reg(dm, R_0x1bd8, MASKDWORD));
		} else {
			RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S%d pwsf unchanged (0x%x)\n", path,
		       dpk_info->pwsf[path] + dpk_info->last_offset[path]);
		}
	//}
#endif
}
#endif
