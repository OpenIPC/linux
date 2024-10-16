/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *
 *  File name   :       de_fcm.h
 *
 *  Description :       display engine 2.0 fcm base functions implement
 *
 *  History     :       2022/03/23  v0.1  Initial version
 *
 ******************************************************************************/
#ifndef __DE_FCM_H_
#define __DE_FCM_H_
typedef struct fcm_hardware_data {
	char name[32];
	u32 lut_id;

	s32 hbh_hue[28];
	s32 sbh_hue[28];
	s32 ybh_hue[28];

	s32 angle_hue[28];
	s32 angle_sat[13];
	s32 angle_lum[13];

	s32 hbh_sat[364];
	s32 sbh_sat[364];
	s32 ybh_sat[364];

	s32 hbh_lum[364];
	s32 sbh_lum[364];
	s32 ybh_lum[364];
} fcm_hardware_data_t;
#endif
