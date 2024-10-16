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
 *  File name   :       de_FCM_type.h
 *
 *  Description :       display engine 2.0 FCM base struct declaration
 *
 *  History     :       2021/04/7    v0.1  Initial version
 *
 ******************************************************************************/

#ifndef __DE_FCM_TYPE__
#define __DE_FCM_TYPE__

#include "de_rtmx.h"
#include "de_fcm.h"

union FCM_CTRL_REG {
	unsigned int dwval;
	struct {
		unsigned int en:1;
		unsigned int res0:3;
		unsigned int win_en:1;
		unsigned int res1:27;
	} bits;
};

union FCM_SIZE_REG {
	unsigned int dwval;
	struct {
		unsigned int width:13;
		unsigned int res0:3;
		unsigned int height:13;
		unsigned int res1:3;
	} bits;
};

union FCM_WIN0_REG {
	unsigned int dwval;
	struct {
		unsigned int left:13;
		unsigned int res0:3;
		unsigned int top:13;
		unsigned int res1:3;
	} bits;
};

union FCM_WIN1_REG {
	unsigned int dwval;
	struct {
		unsigned int right:13;
		unsigned int res0:3;
		unsigned int bot:13;
		unsigned int res1:3;
	} bits;
};

union FCM_ANGLE_HUE_LUT {
	unsigned int dwval;
	struct {
		unsigned int hue:12;
		unsigned int res0:20;
	} bits;
};

union FCM_ANGLE_SAT_LUT {
	unsigned int dwval;
	struct {
		unsigned int sat:10;
		unsigned int res0:22;
	} bits;
};

union FCM_ANGLE_LUM_LUT {
	unsigned int dwval;
	struct {
		unsigned int sat:8;
		unsigned int res0:24;
	} bits;
};

union FCM_HBH_HUE_LUT {
	unsigned int dwval;
	struct {
		unsigned int hue_low:13;
		unsigned int res0:3;
		unsigned int hue_high:13;
		unsigned int res1:3;
	} bits;
};

union FCM_SBH_HUE_LUT {
	unsigned int dwval;
	struct {
		unsigned int hue_low:11;
		unsigned int res0:5;
		unsigned int hue_high:11;
		unsigned int res1:5;
	} bits;
};

union FCM_VBH_HUE_LUT {

	unsigned int dwval;
	struct {
		unsigned int hue_low:9;
		unsigned int res0:7;
		unsigned int hue_high:9;
		unsigned int res1:7;
	} bits;
};

union FCM_LUT {
	unsigned int dwval;
	struct {
		unsigned int lut_0:8;
		unsigned int lut_1:8;
		unsigned int lut_2:8;
		unsigned int lut_3:8;
	} bits;
};

struct __fcm_reg_t {
	union FCM_CTRL_REG fcm_ctl;			/*0x0000			*/
	unsigned int res0;				/*0x0004			*/
	union FCM_SIZE_REG fcm_size;			/*0x0008			*/
	union FCM_WIN0_REG fcm_win0;			/*0x000c			*/
	union FCM_WIN1_REG fcm_win1; 			/*0x0010			*/
	unsigned int res1[3];				/*0x0014	-	0x001c	*/
	union FCM_ANGLE_HUE_LUT fcm_angle_hue[28];	/*0x0020	-	0x008c 	*/
	unsigned int res2[4];				/*0x0090	-	0x009c	*/
	union FCM_ANGLE_SAT_LUT fcm_angle_sat[13];	/*0x00a0	-	0x00d0	*/
	unsigned int res3[3];				/*0x00d4	-	0x00dc	*/
	union FCM_ANGLE_LUM_LUT fcm_angle_lum[13];	/*0x00e0	-	0x0110	*/
	unsigned int res4[3];				/*0x0114	-	0x011c	*/
	union FCM_HBH_HUE_LUT fcm_hbh_hue[28];		/*0x0120 	- 	0x018c	*/
	unsigned int res5[4];				/*0x0190 	- 	0x019c	*/
	union FCM_SBH_HUE_LUT fcm_sbh_hue[28];		/*0x01a0 	- 	0x020c	*/
	unsigned int res6[4];				/*0x0210 	- 	0x021c	*/
	union FCM_VBH_HUE_LUT fcm_vbh_hue[28];		/*0x0220 	- 	0x028c	*/
	unsigned int res7[28];				/*0x0290 	- 	0x02fc	*/
	union FCM_LUT fcm_hbh_sat[364];			/*0x0300 	- 	0x08ac	*/
	unsigned int res8[20];				/*0x08b0 	- 	0x08fc	*/
	union FCM_LUT fcm_sbh_sat[364];			/*0x0900 	- 	0x0eac	*/
	unsigned int res9[20];				/*0x0eb0 	- 	0x0efc	*/
	union FCM_LUT fcm_vbh_sat[364];			/*0x0f00 	- 	0x14ac	*/
	unsigned int res10[20];				/*0x14b0 	- 	0x14fc	*/
	union FCM_LUT fcm_hbh_lum[364];			/*0x1500 	- 	0x1aac	*/
	unsigned int res11[20];				/*0x1ab0 	- 	0x1afc	*/
	union FCM_LUT fcm_sbh_lum[364];			/*0x1b00 	- 	0x20ac	*/
	unsigned int res12[20];				/*0x20b0 	- 	0x20fc	*/
	union FCM_LUT fcm_vbh_lum[364];			/*0x2100 	- 	0x26ac	*/
};

#endif
