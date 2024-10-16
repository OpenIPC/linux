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

void _backup_bb_registers_8733b(void *dm_void,
				u32 *reg,
				u32 *reg_backup,
				u32 reg_num)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_get_bb_reg(dm, reg[i], MASKDWORD);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

void _reload_bb_registers_8733b(void *dm_void,
				u32 *reg,
				u32 *reg_backup,
				u32 reg_num)

{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		odm_set_bb_reg(dm, reg[i], MASKDWORD, reg_backup[i]);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Reload BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

#if 1
u8 _halrf_driver_rate_to_tssi_rate_8733b(void *dm_void, u8 rate)
{
	u8 tssi_rate = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == ODM_MGN_1M)
		tssi_rate = 0;
	else if (rate == ODM_MGN_2M)
		tssi_rate = 1;
	else if (rate == ODM_MGN_5_5M)
		tssi_rate = 2;
	else if (rate == ODM_MGN_11M)
		tssi_rate = 3;
	else if (rate == ODM_MGN_6M)
		tssi_rate = 4;
	else if (rate == ODM_MGN_9M)
		tssi_rate = 5;
	else if (rate == ODM_MGN_12M)
		tssi_rate = 6;
	else if (rate == ODM_MGN_18M)
		tssi_rate = 7;
	else if (rate == ODM_MGN_24M)
		tssi_rate = 8;
	else if (rate == ODM_MGN_36M)
		tssi_rate = 9;
	else if (rate == ODM_MGN_48M)
		tssi_rate = 10;
	else if (rate == ODM_MGN_54M)
		tssi_rate = 11;
	else if (rate >= ODM_MGN_MCS0 && rate <= ODM_MGN_MCS7)// ODM_MGN_VHT4SS_MCS9
		tssi_rate = rate - ODM_MGN_MCS0 + 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF]======>%s not exit tx rate\n", __func__);

	return tssi_rate;
}

u8 _halrf_tssi_rate_to_driver_rate_8733b(void *dm_void, u8 rate)
{
	u8 driver_rate = 0;
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == 0)
		driver_rate = ODM_MGN_1M;
	else if (rate == 1)
		driver_rate = ODM_MGN_2M;
	else if (rate == 2)
		driver_rate = ODM_MGN_5_5M;
	else if (rate == 3)
		driver_rate = ODM_MGN_11M;
	else if (rate == 4)
		driver_rate = ODM_MGN_6M;
	else if (rate == 5)
		driver_rate = ODM_MGN_9M;
	else if (rate == 6)
		driver_rate = ODM_MGN_12M;
	else if (rate == 7)
		driver_rate = ODM_MGN_18M;
	else if (rate == 8)
		driver_rate = ODM_MGN_24M;
	else if (rate == 9)
		driver_rate = ODM_MGN_36M;
	else if (rate == 10)
		driver_rate = ODM_MGN_48M;
	else if (rate == 11)
		driver_rate = ODM_MGN_54M;
	else if (rate >= 12 && rate <= 19)  //83
		driver_rate = rate + ODM_MGN_MCS0 - 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF]======>%s not exit tx rate\n", __func__);
	return driver_rate;
}
#endif
u32 _halrf_get_efuse_tssi_offset_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;
	u32 offset = 0;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 2)
		offset_index = 6;
	else if (channel >= 3 && channel <= 5)
		offset_index = 7;
	else if (channel >= 6 && channel <= 8)
		offset_index = 8;
	else if (channel >= 9 && channel <= 11)
		offset_index = 9;
	else if (channel >= 12 && channel <= 14)
		offset_index = 10;
	else if (channel >= 36 && channel <= 40)
		offset_index = 11;
	else if (channel >= 42 && channel <= 48)
		offset_index = 12;
	else if (channel >= 50 && channel <= 58)
		offset_index = 13;
	else if (channel >= 60 && channel <= 64)
		offset_index = 14;
	else if (channel >= 100 && channel <= 104)
		offset_index = 15;
	else if (channel >= 106 && channel <= 112)
		offset_index = 16;
	else if (channel >= 114 && channel <= 120)
		offset_index = 17;
	else if (channel >= 122 && channel <= 128)
		offset_index = 18;
	else if (channel >= 130 && channel <= 136)
		offset_index = 19;
	else if (channel >= 138 && channel <= 144)
		offset_index = 20;
	else if (channel >= 149 && channel <= 153)
		offset_index = 21;
	else if (channel >= 155 && channel <= 161)
		offset_index = 22;
	else if (channel >= 163 && channel <= 169)
		offset_index = 23;
	else if (channel >= 171 && channel <= 177)
		offset_index = 24;

	offset = (u32)tssi->tssi_efuse[path][offset_index];

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF]=====>%s channel=%d offset_index(Chn Group)=%d offset=%d\n",
	       __func__, channel, offset_index, offset);

	return offset;
}

#if 0
s8 _halrf_get_kfree_tssi_offset_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	struct odm_power_trim_data *power_trim_info = &dm->power_trim_data;
	u8 channel = *dm->channel;
	s8 offset = 0;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 7)
		offset_index = 0;
	if (channel >= 8 && channel <= 14)
		offset_index = 1;
	else if (channel >= 36 && channel <= 50)
		offset_index = 2;
	else if (channel >= 52 && channel <= 64)
		offset_index = 3;
	else if (channel >= 100 && channel <= 128)
		offset_index = 4;
	else if (channel >= 129 && channel <= 144)
		offset_index = 5;
	else if (channel >= 149 && channel <= 163)
		offset_index = 6;
	else if (channel >= 164 && channel <= 177)
		offset_index = 7;

	//return offset = tssi->tssi_kfree_efuse[path][offset_index];
	return = power_trim_info->tssi_trim[offset_index][path];
}
#endif
void halrf_tssi_get_efuse_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 pg_tssi = 0xff, i, j;
	u32 pg_tssi_tmp = 0xff;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI]===>%s\n", __func__);

	/*path s0*/
	j = 0;
	for (i = 0x10; i <= 0x1a; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF]tssi->tssi_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_efuse[0][j]);
		j++;
	}

	for (i = 0x22; i <= 0x2f; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF]tssi->tssi_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_efuse[0][j]);
		j++;
	}

	/*path s1*/
	j = 0;
	for (i = 0x3a; i <= 0x44; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[1][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF]tssi->tssi_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_efuse[1][j]);
		j++;
	}
#if 0
	for (i = 0x4c; i <= 0x59; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[1][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"tssi->tssi_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_efuse[1][j]);
		j++;
	}
#endif
}

u32 halrf_get_online_tssi_de_8733b(void *dm_void, u8 path, s32 pout)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;
	u32 offset_index = 0;
	u32 tssi_de = 0;
	u32 tssi_offset;
	s32 de = 0;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	de = ((pout - 16000) * 8) / 1000;
#if 0
	if (path == RF_PATH_A)
		tssi_offset = odm_get_bb_reg(dm, R_0x4334, 0x0FF00000);
	else
		tssi_offset = odm_get_bb_reg(dm, R_0x4344, 0x0FF00000);

	if (tssi_offset & BIT(7))
		tssi_offset = tssi_offset | 0xffffff00;

	de = de + tssi_offset;
#endif
	if (de & BIT(7))
		tssi_de = (u32)(de | 0xffffff00);
	else
		tssi_de = (u32)de;

	tssi->tssi_de = tssi_de & 0xff;

	//halrf_tssi_set_de_8733b(dm, tssi_de);
	//halrf_tssi_set_de_for_tx_verify_8733b(dm, tssi_de, path);

	return tssi_de;
}

void halrf_tssi_set_efuse_de_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 i;
	//u32 addr_d[2] = {R_0x4334, R_0x4344};
	//u32 addr_cck_d[2] = {R_0x433c, R_0x434c};
	s8 tssi_offest_de;
	u32 offset_index = 0;
	u8 channel = *dm->channel;
	s32 tmp;
	s8 efuse = 0xff;

	if (dm->rf_calibrate_info.txpowertrack_control == 4) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "==>%s txpowertrack_control=%d return!!!\n", __func__,
		       dm->rf_calibrate_info.txpowertrack_control);

		if (path == RF_PATH_A) {
			tmp = phydm_get_tssi_trim_de(dm, RF_PATH_A);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp = 0x%x\n", tmp);

			if (tmp > 127)
				tmp = 127;
			else if (tmp < -128)
				tmp = -128;

			tmp = tmp & 0xff;
			odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);	//HT40,CCK
			odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, tmp);	//OFDM
			odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, tmp);	//HT20
			odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, tmp);	// RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, tmp);	// RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, tmp);  //CCK
		} else {
			tmp = phydm_get_tssi_trim_de(dm, RF_PATH_B);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp = 0x%x\n", tmp);

			if (tmp > 127)
				tmp = 127;
			else if (tmp < -128)
				tmp = -128;

			tmp = tmp & 0xff;
			odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, tmp);	//HT40,CCK
			odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, tmp);	//HT40
			odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, tmp);	//OFDM
			odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, tmp);	//RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, tmp);	//RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, tmp);	//HT20
			odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, tmp);	//CCK
		}
		return;
	}

	for (i = 0; i < MAX_PATH_NUM_8733B; i++) {
		if (channel >= 1 && channel <= 2)
			offset_index = 0;
		else if (channel >= 3 && channel <= 5)
			offset_index = 1;
		else if (channel >= 6 && channel <= 8)
			offset_index = 2;
		else if (channel >= 9 && channel <= 11)
			offset_index = 3;
		else if (channel >= 12 && channel <= 13)
			offset_index = 4;
		else if (channel == 14)
			offset_index = 5;

		efuse = tssi->tssi_efuse[i][offset_index];
		if (efuse == 0xff)
			tmp = phydm_get_tssi_trim_de(dm, i);
		else
			tmp = tssi->tssi_efuse[i][offset_index] + phydm_get_tssi_trim_de(dm, i);

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;

		tmp = tmp & 0xff;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp CCK = 0x%x\n", tmp);
		if (path == RF_PATH_A)
			odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);	//HT40,CCK
		else
			odm_set_bb_reg(dm, R_0x4348, 0x0FF00000, tmp);  //HT40,CCK

		efuse = (s8)_halrf_get_efuse_tssi_offset_8733b(dm, i);
		if (efuse == 0xff)
			tssi_offest_de = 0;
		else
			tssi_offest_de = efuse;

		tmp = tssi_offest_de + phydm_get_tssi_trim_de(dm, i);

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;

		tmp = tmp & 0xff;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp OFDM = 0x%x\n", tmp);
		if (path == RF_PATH_A) {
			odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);	//HT40,CCK
			odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, tmp);	//OFDM
			odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, tmp);	//HT20
			odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, tmp);	// RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, tmp);	// RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, tmp);	//CCK
		} else {
			odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, tmp);	//HT40,CCK
			odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, tmp);	//HT40
			odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, tmp);	//OFDM
			odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, tmp);	//RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, tmp);	//RF40M OFDM 6M
			odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, tmp);	//HT20
			odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, tmp);	//CCK
		}

	}
}

u32 halrf_tssi_set_de_8733b(void *dm_void, u32 tssi_de)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 de = 0;
	s32 offset = 0, db_temp;
	u8 i, rate, channel = *dm->channel, bandwidth = *dm->band_width;
	u8 idxbyrate[20];
	u32 tssi_dbm;
	u32 reg0x3axx, idxoffset;
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	rf->is_tssi_in_progress = 1;
	halrf_disable_tssi_8733b(dm);
	i = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	_halrf_tssi_8733b(dm, i);
	
	de = tssi->tssi_de;
	de = de & 0xff;
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]de= 0x%x\n", de);
	
	if (i == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, de);  //HT40,CCK
		odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, de);  //OFDM
		odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, de);  //HT20
		odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, de);  // RF40M OFDM 6M,txsc=1/2
		odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, de);  // RF40M OFDM 6M,txsc=0
		odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, de);  //CCK
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4334= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4334, MASKDWORD));
	} else {
		odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, de);  //HT40,CCK
		odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, de);  //HT40
		odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, de);  //OFDM
		odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, de);  //RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, de);  //RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, de);  //HT20
		odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, de);  //CCK
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4344= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4344, MASKDWORD));
	}
	
	halrf_enable_tssi_8733b(dm);
	rf->is_tssi_in_progress = 0;
#if 0
	//01_set_txpwr_bb_com.txt, txagc_by_rate

	odm_set_bb_reg(dm, R_0x3a00, MASKDWORD, 0x03020100);  //CCK11M/5M/2M/1M
	odm_set_bb_reg(dm, R_0x3a04, MASKDWORD, 0x07060504);  //OFDM18/12/9/6
	odm_set_bb_reg(dm, R_0x3a08, MASKDWORD, 0x0b0a0908);  //OFDM54/48/36/24
	odm_set_bb_reg(dm, R_0x3a0c, MASKDWORD, 0x0f0e0d0c);  //MCS3/2/1/0
	odm_set_bb_reg(dm, R_0x3a10, MASKDWORD, 0x13121110);  //MCS7/6/5/4
	//m_set_bb_reg(dm, R_0x3a14, MASKDWORD, 0x17161514);

	for (i = 0; i < 20; i++) {  //ODM_MGN_MCS7 = 0x87,tssi_rate = 19
		rate = _halrf_tssi_rate_to_driver_rate_8733b(dm, i);
		db_temp = (s32)phydm_get_tx_power_mdbm(dm, RF_PATH_A, rate, bandwidth, channel);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]db_temp = 0x%x\n", db_temp);
		offset = (db_temp - 16) * 4;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]offset = 0x%x\n", offset);
		if (offset > 127)
			offset = 127;
		else if (offset < -128)
			offset = -128;
		if (offset & BIT(8))
			idxbyrate[i] = (offset & 0xff) | BIT(7);
		else
			idxbyrate[i] = offset & 0xff;
	}
		for (i = 0; i < 24; i = i + 4) {
			idxoffset = (idxbyrate[i] & 0xff) |
					(idxbyrate[i + 1] & 0xff) << 8|
					(idxbyrate[i + 2] & 0xff) << 16|
					(idxbyrate[i + 3] & 0xff) << 24;
			reg0x3axx = (u16)(0x3a00 + i);
			odm_set_bb_reg(dm, reg0x3axx, MASKDWORD, idxoffset);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] reg0x%x = 0x%x\n",
				reg0x3axx, odm_get_bb_reg(dm, reg0x3axx, MASKDWORD));
		}
#endif
	offset = (s32)((de + 0x80) & 0xff);
	tssi_dbm = (offset * 100 + 5) / 8;
	return tssi_dbm;
}

void halrf_tssi_set_de_for_tx_verify_8733b(void *dm_void, u32 tssi_de, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	u32 addr_d[2] = {R_0x4334, R_0x4344};
	u32 addr_cck_d[2] = {R_0x433c, R_0x434c};
	u32 addr_d_bitmask = 0x0FF00000;
	s32 tmp, tssi_de_tmp;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	rf->is_tssi_in_progress = 1;
	halrf_disable_tssi_8733b(dm);
	path = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	_halrf_tssi_8733b(dm, path);

	if (tssi_de & BIT(7))
		tssi_de_tmp = tssi_de | 0xffffff00;
	else
		tssi_de_tmp = tssi_de;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s tssi_de(%d) tssi_de_tmp(%d)\n",
		__func__,
		tssi_de, tssi_de_tmp);

	//tmp = tssi_de_tmp + phydm_get_tssi_trim_de(dm, path);
	tmp = tssi_de_tmp;

	if (tmp > 127)
		tmp = 127;
	else if (tmp < -128)
		tmp = -128;

	tmp = tmp & 0xff;
	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);	//HT40,CCK
		odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, tmp);	//OFDM
		odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, tmp);	//HT20
		odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, tmp);	// RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, tmp);	// RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, tmp);  //CCK
	} else {
		odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, tmp);  //HT40,CCK
		odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, tmp);	//HT40
		odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, tmp);	//OFDM
		odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, tmp);	//RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, tmp);	//RF40M OFDM 6M
		odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, tmp);	//HT20
		odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, tmp);	//CCK
	}
	halrf_enable_tssi_8733b(dm);
	rf->is_tssi_in_progress = 0;
}

void halrf_tssi_clean_de_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;

	rf->is_tssi_in_progress = 1;
	//pathA
	odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, 0x0);
	odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, 0x0);	//OFDM
	odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, 0x0);	//HT20
	odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, 0x0);	// RF40M OFDM 6M
	odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, 0x0);	// RF40M OFDM 6M
	odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, 0x0);
	//pathB
	odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, 0x0);
	odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, 0x0);  //HT40
	odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, 0x0);  //OFDM
	odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, 0x0);  //RF40M OFDM 6M
	odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, 0x0);  //RF40M OFDM 6M
	odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, 0x0);  //HT20
	odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, 0x0);	//CCK

	rf->is_tssi_in_progress = 0;
}

void _halrf_tssi_anapar_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32  reg_rf18;
	u8  band;
	//u32 tssi_setting[2] = {R_0x1830, R_0x4130};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);
	band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF]==>tssi in band %s\n", (band == 0 ? "2G" : "5G"));
	/*00_set_tssi_sys_2G/5G.txt*/
	odm_set_bb_reg(dm, R_0x1860, BIT(30), 0x0);//anapar non dbg mode
	if (band == 0) {/*ANAPAR*/
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	} else {//5G,only s0
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	}  //0xFFA1005E
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffb5005e); //AD/DA fifo rst
	odm_set_bb_reg(dm, R_0x1d40, BIT(3), 0x0);
	odm_set_bb_reg(dm, R_0x1e1c, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x1e1c, BIT(26), 0x1);
	odm_set_bb_reg(dm, R_0x1ca4, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x1e1c, 0x0000F000, 0x8);//0xc ADC 160M,0x8 ADC 10M,
}

void _halrf_tssi_rf_setting_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, BIT(8), 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x55, BIT(7), 0x1); //Enable RF power tracking at RFC
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x7f, BIT(8), 0x1);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x55, BIT(7), 0x1); //Enable RF power tracking at RFC
#if 0
	if(!odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, BIT(16))) {  //2G
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, 0x00002, 0x1);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x00078, 0xf);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6d, 0x00004, 0x0); //0 for high power,1
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x30000, 0x1);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x03000, 0x3);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x93, 0x38000, 0x7);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x92, 0x00060, 0x3);//RF_PATH_B
		//odm_set_rf_reg(dm, path, RF_0x6d, 0x00004, 0x1); //1 for low power
		odm_set_rf_reg(dm, path, RF_0x55, 0x00080, 0x1); //Enable RF power tracking at RFC
	} else {//5G
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, BIT(8), 0x1);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6f, 0x00078, 0xf);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6d, BIT(7), 0x0); //0 for high power,1
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6f, 0x30000, 0x1);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x6f, 0x03000, 0x3);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x93, 0x38000, 0x7);
		//odm_set_rf_reg(dm, RF_PATH_A, RF_0x92, 0x00060, 0x3);//RF_PATH_B
		//odm_set_rf_reg(dm, path, RF_0x6d, 0x00004, 0x1); //1 for low power
		odm_set_rf_reg(dm, path, RF_0x55, BIT(7), 0x1); //Enable RF power tracking at RFC
	}
	if (path == RF_PATH_B) {
		odm_set_rf_reg(dm, path, RF_0x7f, 0x00002, 0x1);
		//odm_set_rf_reg(dm, path, RF_0x6e, 0x00078, 0xf);
		//odm_set_rf_reg(dm, path, RF_0x6d, 0x00004, 0x0); //0 for high power,1
		//odm_set_rf_reg(dm, path, RF_0x6e, 0x30000, 0x1);
		//odm_set_rf_reg(dm, path, RF_0x6e, 0x03000, 0x3);
		//odm_set_rf_reg(dm, path, RF_0x93, 0x38000, 0x7);
		//odm_set_rf_reg(dm, path, RF_0x92, 0x00060, 0x3);//RF_PATH_B
		//odm_set_rf_reg(dm, path, RF_0x6d, 0x00004, 0x1); //1 for low power
		odm_set_rf_reg(dm, path, RF_0x55, 0x00080, 0x1); //Enable RF power tracking at RFC
	}
#endif
}

void halrf_tssi_dck_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	u8 channel = *dm->channel;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s channel=%d ======\n",
	       __func__, channel);
	//05_set_tssi_dck.txt  auto DCK
	odm_set_bb_reg(dm, R_0x4328, BIT(24), 0x1);
	odm_set_bb_reg(dm, R_0x4328, BIT(25), 0x1);
	odm_set_bb_reg(dm, R_0x4328, BIT(29) | BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0x4328, BIT(30), 0x1);
	odm_set_bb_reg(dm, R_0x432c, 0x000000FF, 0x53);
	//odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x000003fe);
	odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x00000002);
	odm_set_bb_reg(dm, R_0x4378, MASKDWORD, 0x000003fd);
	odm_set_bb_reg(dm, R_0x436c, MASKDWORD, 0x00000000);
}

void _halrf_tssi_set_txpwr_bb_com_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u32 db_temp, idxoffset;
	s32 de = 0, offset = 0;
	u8 i, rate, channel = *dm->channel, bandwidth = *dm->band_width;
	u8 idxbyrate[20];
	u16 reg0x3axx;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);

	//01_set_txpwr_bb_com.txt, txagc_by_rate
#if 1
	if (*dm->mp_mode == 1) {  //mp mode define
		if (cali_info->txpowertrack_control != 3) { //TSSI_OFF/CAL
			odm_set_bb_reg(dm, R_0x3a00, MASKDWORD, 0x00000000);
			odm_set_bb_reg(dm, R_0x3a04, MASKDWORD, 0x00000000);
			odm_set_bb_reg(dm, R_0x3a08, MASKDWORD, 0x00000000);
			odm_set_bb_reg(dm, R_0x3a0c, MASKDWORD, 0x00000000);
			odm_set_bb_reg(dm, R_0x3a10, MASKDWORD, 0x00000000);
			odm_set_bb_reg(dm, R_0x3a14, MASKDWORD, 0x00000000);	
		} else {
			for (i = 0; i < 20; i++) {  //ODM_MGN_MCS7 = 0x87,tssi_rate = 19
				rate = _halrf_tssi_rate_to_driver_rate_8733b(dm, i);
				db_temp = (s32)phydm_get_tx_power_mdbm(dm, RF_PATH_A, rate, bandwidth, channel);
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]db_temp = 0x%x\n", db_temp);
				offset = (db_temp - 1600) * 4 / 100;
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]offset = 0x%x\n", offset);
				if (offset > 127)
					offset = 127;
				else if (offset < -128)
					offset = -128;
				if (offset & BIT(8))
					idxbyrate[i] = (offset & 0xff) | BIT(7);
				else
					idxbyrate[i] = offset & 0xff;
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]idxbyrate[%d] = 0x%x\n", i, offset);
			}
			for (i = 0; i < 20; i = i + 4) {
				reg0x3axx = (u16)(0x3a00 + i);
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] before reg0x%x = 0x%x\n",
					reg0x3axx, odm_get_bb_reg(dm, reg0x3axx, MASKDWORD));
				idxoffset = (idxbyrate[i] & 0xff) |
					(idxbyrate[i + 1] & 0xff) << 8 |
					(idxbyrate[i + 2] & 0xff) << 16 |
					(idxbyrate[i + 3] & 0xff) << 24;
				odm_set_bb_reg(dm, reg0x3axx, MASKDWORD, idxoffset);
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] after reg0x%x = 0x%x\n",
					reg0x3axx, odm_get_bb_reg(dm, reg0x3axx, MASKDWORD));
			}
		}
	}
#endif
	//02_ini_txpwr_ctrl_bb.txt
	odm_set_bb_reg(dm, R_0x4300, 0x1F, 0x00);
	odm_set_bb_reg(dm, R_0x4300, 0x00FFFF00, 0x00ff);
	odm_set_bb_reg(dm, R_0x4300, 0x07000000, 0x4);
	odm_set_bb_reg(dm, R_0x4300, 0xF0000000, 0x4);
	odm_set_bb_reg(dm, R_0x4304, 0x0000FFFF, 0x8080);
	odm_set_bb_reg(dm, R_0x4304, 0xFFFF0000, 0x0000);
	odm_set_bb_reg(dm, R_0x4308, MASKDWORD, 0x40404040);
	odm_set_bb_reg(dm, R_0x430c, MASKDWORD, 0x3f3f3f3f);
	odm_set_bb_reg(dm, R_0x4310, MASKDWORD, 0x003f3f3f);
	odm_set_bb_reg(dm, R_0x4314, 0x000001FF, 0x000);
	odm_set_bb_reg(dm, R_0x4314, 0x00007000, 0x7);
	odm_set_bb_reg(dm, R_0x4314, 0x00038000, 0x7);
	odm_set_bb_reg(dm, R_0x4314, 0x007C0000, 0x1f);
	odm_set_bb_reg(dm, R_0x4314, 0x0F800000, 0x00);
	odm_set_bb_reg(dm, R_0x4318, 0x0000FFFF, 0x807f);
	odm_set_bb_reg(dm, R_0x4318, 0x7FFF0000, 0x0);
	odm_set_bb_reg(dm, R_0x431c, MASKDWORD, 0x0076280a);
	odm_set_bb_reg(dm, R_0x4320, 0x0000007F, 0x00);
	odm_set_bb_reg(dm, R_0x4320, 0x00000100, 0x1);
	odm_set_bb_reg(dm, R_0x4320, 0x0000FE00, 0x00);
	odm_set_bb_reg(dm, R_0x4320, 0x00FF0000, 0x88);
	odm_set_bb_reg(dm, R_0x4320, 0x0F000000, 0xA);
	odm_set_bb_reg(dm, R_0x4324, MASKDWORD, 0x807f807f);
	odm_set_bb_reg(dm, R_0x4328, 0x00FFFFFF, 0x280200);
	odm_set_bb_reg(dm, R_0x4328, 0x7F000000, 0x43);
	odm_set_bb_reg(dm, R_0x432c, 0x000000FF, 0x50);
	odm_set_bb_reg(dm, R_0x432c, 0x0001FF00, 0x0ff);
	odm_set_bb_reg(dm, R_0x432c, 0x1FF00000, 0x100);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x03FF0000, 0x000);
	/*odm_set_bb_reg(dm, R_0x4334, MASKDWORD, 0x00000000);*/
	odm_set_bb_reg(dm, R_0x4338, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4338, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x433c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4340, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4340, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4348, 0x00000FFF, 0x800);
	/*odm_set_bb_reg(dm, R_0x4348, 0x03FF0000, 0x000);*/
	odm_set_bb_reg(dm, R_0x434c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4350, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4354, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4358, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x435c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4360, 0x00000003, 0x0);
	odm_set_bb_reg(dm, R_0x4360, 0x01FFFFF0, 0x1f1f1f);
	odm_set_bb_reg(dm, R_0x4364, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x436c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4370, 0x001FFFFF, 0x1f1f1f);
	odm_set_bb_reg(dm, R_0x4374, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4378, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x437c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4380, MASKDWORD, 0x00000002);
	odm_set_bb_reg(dm, R_0x4384, MASKDWORD, 0x100000ff);
	odm_set_bb_reg(dm, R_0x4388, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x438c, 0x00007FFF, 0x4040);
	odm_set_bb_reg(dm, R_0x438c, 0xFFFF0000, 0xA0A0); //(s0)c_cut continue
	odm_set_bb_reg(dm, R_0x4390, 0x0000FFFF, 0x4040);
	odm_set_bb_reg(dm, R_0x4390, 0xFFFF0000, 0x8080);
	odm_set_bb_reg(dm, R_0x4394, 0x00007FFF, 0x4040);
	odm_set_bb_reg(dm, R_0x4394, 0xFFFF0000, 0x9090); //(s1)c_cut continue
	odm_set_bb_reg(dm, R_0x4398, 0x0000FFFF, 0x8080);
	odm_set_bb_reg(dm, R_0x4398, 0xFFFF0000, 0x8080);
	odm_set_bb_reg(dm, R_0x439c, 0x00000007, 0x1);
	odm_set_bb_reg(dm, R_0x439c, 0x0FFFFFF0, 0x080080);
	odm_set_bb_reg(dm, R_0x439c, 0x30000000, 0x0);
	odm_set_bb_reg(dm, R_0x43a0, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x43a4, 0x00001FFF, 0x0000);
	odm_set_bb_reg(dm, R_0x43a4, BIT(16), 0x1);
	odm_set_bb_reg(dm, R_0x43a8, 0x0000001F, 0x00);
	odm_set_bb_reg(dm, R_0x43a8, 0x00000F00, 0xd);
	odm_set_bb_reg(dm, R_0x43a8, 0x0000F000, 0xf);
	odm_set_bb_reg(dm, R_0x43a8, 0x00070000, 0x7);
	odm_set_bb_reg(dm, R_0x43a8, 0x00380000, 0x0);
	odm_set_bb_reg(dm, R_0x43a8, 0x03C00000, 0xd);
	odm_set_bb_reg(dm, R_0x43a8, 0x7C000000, 0x1d);
	odm_set_bb_reg(dm, R_0x43ac, 0x0000FFFF, 0x4040);
	odm_set_bb_reg(dm, R_0x1ca4, BIT(30), 0x1);
	/*odm_set_bb_reg(dm, R_0x43b0, MASKDWORD, 0x00000000);*/
	/*odm_set_bb_reg(dm, R_0x43b4, MASKDWORD, 0x00000000);*/
	/*odm_set_bb_reg(dm, R_0x43b8, MASKDWORD, 0x00000000);*/

}

void _halrf_tssi_set_tmeter_tbl_zero_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	//struct _halrf_tssi_info *tssi = &rf->halrf_tssi_info;
	u8 i = 0x0, thermal = 0xff;
	u32 thermal_offset_tmp = 0x0;
	s8 thermal_offset[64] = {0};
	u16 reg0x42xx;
	u8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};
	u8 thermal_up_b[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_b[DELTA_SWINGIDX_SIZE] = {0};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI] ======>%s\n", __func__);

	odm_set_bb_reg(dm, R_0x4380, 0x00000007, 0x3);
	odm_set_bb_reg(dm, R_0x4380, 0x000FFFF0, 0x0000);
	odm_set_bb_reg(dm, R_0x4380, 0xFFF00000, 0x000);

	for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
			(thermal_offset[i + 1] & 0xff) << 8 |
			(thermal_offset[i + 2] & 0xff) << 16 |
			(thermal_offset[i + 3] & 0xff) << 24;
		reg0x42xx = (u16)(0x4200 + i);
		odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
  		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] reg0x%x = 0x%x\n",
			reg0x42xx, odm_get_bb_reg(dm, reg0x42xx, MASKDWORD));
	}
}

void _halrf_tssi_set_tmeter_tbl_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel, i, thermal;
	s8 j;
	u16 reg0x42xx;
	u8 rate = phydm_get_tx_rate(dm);
	u32 thermal_offset_tmp = 0, thermal_offset_index = 0x10, thermal_tmp = 0xff, tmp;
	s8 thermal_offset[64] = {0};
	u8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};
	u8 thermal_up_b[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_b[DELTA_SWINGIDX_SIZE] = {0};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI] ======>%s\n", __func__);

	tssi->thermal[RF_PATH_A] = 0;
	tssi->thermal[RF_PATH_B] = 0;

	odm_set_bb_reg(dm, R_0x4380, 0x00000007, 0x3);
	odm_set_bb_reg(dm, R_0x4380, 0x000FFFF0, 0x0000);
	odm_set_bb_reg(dm, R_0x4380, 0xFFF00000, 0x000);

	if (cali_info->txpowertrack_control == 4) { //TSSI_ON_CALIBRATE
		for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
			(thermal_offset[i + 1] & 0xff) << 8 |
			(thermal_offset[i + 2] & 0xff) << 16 |
			(thermal_offset[i + 3] & 0xff) << 24;
		reg0x42xx = (u16)(0x4200 + i);
		odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
  		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]TSSI_CAL reg0x%x = 0x%x\n",
			reg0x42xx, odm_get_bb_reg(dm, reg0x42xx, MASKDWORD));
		}
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]TSSI_CAL, return!\n");
		return;
	}

	if (rate == ODM_MGN_1M || rate == ODM_MGN_2M || rate == ODM_MGN_5_5M || rate == ODM_MGN_11M) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2g_cck_a_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2g_cck_a_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2g_cck_b_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2g_cck_b_n, sizeof(thermal_down_b));
	} else if (channel >= 1 && channel <= 14) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2ga_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2ga_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2gb_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2gb_n, sizeof(thermal_down_b));
	} else if (channel >= 36 && channel <= 64) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[0], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[0], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[0], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[0], sizeof(thermal_down_b));
	} else if (channel >= 100 && channel <= 144) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[1], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[1], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[1], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[1], sizeof(thermal_down_b));
	} else if (channel >= 149 && channel <= 177) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[2], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[2], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[2], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[2], sizeof(thermal_down_b));
	}

	i = 0;
	for (j = 0; j < 32; j++) {
		if (i < DELTA_SWINGIDX_SIZE)
			thermal_offset[j] = thermal_up_a[i++];
		else
			thermal_offset[j] = thermal_up_a[DELTA_SWINGIDX_SIZE - 1];

	}
	i = 1;
	for (j = 63; j >= 32; j--) {
		if (i < DELTA_SWINGIDX_SIZE)
			thermal_offset[j] = -1 * thermal_down_a[i++];
		else
			thermal_offset[j] = -1 * thermal_down_a[DELTA_SWINGIDX_SIZE - 1];
	}

	for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
				(thermal_offset[i + 1] & 0xff) << 8 |
				(thermal_offset[i + 2] & 0xff) << 16 |
				(thermal_offset[i + 3] & 0xff) << 24;
		reg0x42xx = (u16)(0x4200 + i);
		odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] reg 0x%x =0x%x\n",
		       reg0x42xx, thermal_offset_tmp);
	}
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]set_tmeter_tbl over!\n");
}

void _halrf_tssi_set_rf_gap_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4350, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4354, MASKDWORD, 0x00000000);
}

void _halrf_tssi_set_slope_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	odm_set_bb_reg(dm, R_0x4320, BIT(24), 0x1);
	odm_set_bb_reg(dm, R_0x4328, 0x00FFFFFF, 0x280200);//640,512pts
	odm_set_bb_reg(dm, R_0x4320, 0x0000F000, 0xf);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x03FF0000, 0x000);
	//odm_set_bb_reg(dm, R_0x4334, MASKDWORD, 0x0c900000);//128-183=-55=0xC9
	/*odm_set_bb_reg(dm, R_0x4334, MASKDWORD, 0x00000000);*/
	odm_set_bb_reg(dm, R_0x4338, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4338, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x433c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4340, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4340, 0x03FF0000, 0x000);
	//odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x0ca00000);//16dBm tssi_C-tssi_val=128-182=-54=0xCA
	odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4348, 0x00000FFF, 0x800);
	/*odm_set_bb_reg(dm, R_0x4348, 0x03FF0000, 0x000);*/
	odm_set_bb_reg(dm, R_0x434c, MASKDWORD, 0x00000000);
}

void _halrf_tssi_set_slope_cal_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4390, MASKDWORD, 0x80808080);
	//odm_set_bb_reg(dm, R_0x4390, MASKDWORD, 0x50568080);
	odm_set_bb_reg(dm, R_0x4398, MASKDWORD, 0x80808080);
	//odm_set_bb_reg(dm, R_0x4398, MASKDWORD, 0x50568080);
	odm_set_bb_reg(dm, R_0x439c, BIT(0), 0x1);
#if 0
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	odm_set_bb_reg(dm, R_0x4320, BIT(24), 0x1);
	odm_set_bb_reg(dm, R_0x4328, 0x00FFFFFF, 0x280200);
	odm_set_bb_reg(dm, R_0x4320, 0x0000F000, 0xf);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x4334, MASKDWORD, 0x0c900000);
	odm_set_bb_reg(dm, R_0x4338, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x433c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4340, 0x00000FFF, 0x700);
	odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x0ca00000);
	odm_set_bb_reg(dm, R_0x4348, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x434c, MASKDWORD, 0x00000000);
#endif
}

void _halrf_tssi_set_tssi_track_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4320, BIT(24), 0x0);
	odm_set_bb_reg(dm, R_0x439c, 0x0FFFFFF0, 0x080080);//0.125db/cw
}

void _halrf_run_tssi_slope_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	//wire r_tssi_en
	odm_set_bb_reg(dm, R_0x4318, BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0x4318, BIT(28), 0x1);
}

void _halrf_rpt_tssi_adc_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32  adc_preamble, dck_auto_avg, dck_auto_max, dck_auto_min;
	//wire  r_tssi_en
	odm_set_bb_reg(dm, R_0x43a8, 0x0000001F, 0x0d);
	adc_preamble = odm_get_bb_reg(dm, R_0x4380, 0x000003FF);
	dck_auto_avg = odm_get_bb_reg(dm, R_0x42b0, 0x000003FF);
	dck_auto_max = odm_get_bb_reg(dm, R_0x42b0, 0x003FF000);
	dck_auto_min = odm_get_bb_reg(dm, R_0x42b4, 0x003FF000);
}

void halrf_enable_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	_halrf_tssi_set_tssi_track_8733b(dm);//08
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x7);
	iqk_info->is_tssi_mode = true;
}

void halrf_disable_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);

	//dm->is_tssi_mode[RF_PATH_A] = false;
	iqk_info->is_tssi_mode = false;
}

void _halrf_tssi_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	_halrf_tssi_anapar_8733b(dm, path);//00
	_halrf_tssi_rf_setting_8733b(dm, path);//00 ?==>radioa&b
	_halrf_tssi_set_txpwr_bb_com_8733b(dm); //01,02
	//_halrf_tssi_set_tmeter_tbl_zero_8733b(dm); //03
	_halrf_tssi_set_tmeter_tbl_8733b(dm); //03
	_halrf_tssi_set_rf_gap_8733b(dm); //04
	halrf_tssi_dck_8733b(dm);//05
	_halrf_tssi_set_slope_8733b(dm);//06
	_halrf_tssi_set_slope_cal_8733b(dm);//07
	
	//_halrf_run_tssi_slope_8733b(dm);
	//_halrf_rpt_tssi_adc_8733b(dm);
}
void halrf_do_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u32 bb_reg[10] = {R_0x9f0, R_0x9b4, R_0x1c38, R_0x1860, R_0x1cd0,
			  R_0x824, R_0x2a24, R_0x1d40, R_0x1c20, R_0x1880};
	u32 bb_reg_backup[10] = {0};
	u32 backup_num = 10;
	u8 i;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);

	i = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));

	rf->is_tssi_in_progress = 1;
	//_backup_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
	
	halrf_disable_tssi_8733b(dm);
	//halrf_tssi_set_efuse_de_8733b(dm, i);


	//for (i = RF_PATH_A; i < RF_PATH_MAX_8733B; i++) { //Amode err issue
	_halrf_tssi_8733b(dm, i);
	//}

	if (!(rf->rf_supportability & HAL_RF_TX_PWR_TRACK)) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI] rf_supportability HAL_RF_TX_PWR_TRACK=%d, return!!!\n",
		       (rf->rf_supportability & HAL_RF_TX_PWR_TRACK));
		halrf_disable_tssi_8733b(dm);
		//_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
		rf->is_tssi_in_progress = 0;
		return;
	}

	if (*dm->mp_mode == 1) {  //mp mode define
		if (cali_info->txpowertrack_control >= 3) { //TSSI ON
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "[RF][TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
			       cali_info->txpowertrack_control);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4334= 0x%x\n",
				odm_get_bb_reg(dm, R_0x4334, MASKDWORD));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x43b0= 0x%x\n",
				odm_get_bb_reg(dm, R_0x43b0, MASKDWORD));
			halrf_enable_tssi_8733b(dm);
			//_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
			rf->is_tssi_in_progress = 0;
			return;
		}
	} else {  //efuse 0xC8 define 0x4h-0x7h for power tracking by TSSI
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"[RF][TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
				cali_info->txpowertrack_control);
			halrf_enable_tssi_8733b(dm);
			//_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
			rf->is_tssi_in_progress = 0;
			return;
		}
	}

	//_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
	rf->is_tssi_in_progress = 0;
}

#if 0
void halrf_get_efuse_thermal_pwrtype_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u32 thermal_tmp, pg_tmp;

	tssi->thermal[RF_PATH_A] = 0xff;
	tssi->thermal[RF_PATH_B] = 0xff;

	/*path s0*/
	odm_efuse_logical_map_read(dm, 1, 0xd0, &thermal_tmp);
	tssi->thermal[RF_PATH_A] = (u8)thermal_tmp;

	/*path s1*/
	odm_efuse_logical_map_read(dm, 1, 0xd1, &thermal_tmp);
	tssi->thermal[RF_PATH_B] = (u8)thermal_tmp;

	/*power tracking type*/
	odm_efuse_logical_map_read(dm, 1, 0xc8, &pg_tmp);
	rf->power_track_type = (u8)((pg_tmp >> 4) & 0xf);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[TSSI] ======>%s thermal pahtA=0x%x pahtB=0x%x power_track_type=0x%x\n",
	       __func__, tssi->thermal[RF_PATH_A],  tssi->thermal[RF_PATH_B],
	       rf->power_track_type);
}

u32 halrf_query_tssi_value_8733b(void *dm_void)
{
	s32 tssi_codeword = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 tssi_rate;
	u8 rate = phydm_get_tx_rate(dm);
	s8 efuse, kfree;

	tssi_rate = _halrf_driver_rate_to_tssi_rate_8733b(dm, phydm_get_tx_rate(dm));
	tssi_codeword = tssi->tssi_codeword[tssi_rate];

	if (rate == ODM_MGN_1M || rate == ODM_MGN_2M || rate == ODM_MGN_5_5M || rate == ODM_MGN_11M) {
		efuse = _halrf_get_efuse_tssi_offset_8733b(dm, 3);
		kfree = _halrf_get_kfree_tssi_offset_8733b(dm);
	} else {
		efuse = _halrf_get_efuse_tssi_offset_8733b(dm, 19);
		kfree = _halrf_get_kfree_tssi_offset_8733b(dm);
	}

	tssi_codeword = tssi_codeword + efuse + kfree;

	if (tssi_codeword <= 0)
		tssi_codeword = 0;
	else if (tssi_codeword >= 255)
		tssi_codeword = 255;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "======>%s tx_rate=0x%X tssi_codeword(0x%x) = tssi_codeword(%d) + efuse(%d) + kfree(%d)\n",
	       __func__, phydm_get_tx_rate(dm), tssi_codeword,
	       tssi->tssi_codeword[tssi_rate], efuse, kfree);

	return (u32)tssi_codeword;
}
#endif

#endif
