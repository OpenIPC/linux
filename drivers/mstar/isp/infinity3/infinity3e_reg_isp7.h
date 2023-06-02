// Generate Time: 2016-10-11 16:57:25.127600
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2016 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INFINITY3E_REG_ISP7__
#define __INFINITY3E_REG_ISP7__
typedef struct {
	// h0000, bit: 0
	/* timing generator enable*/
	#define offset_of_reg_pat_tgen_en (0)
	#define mask_of_reg_pat_tgen_en (0x1)
	unsigned int reg_pat_tgen_en:1;

	// h0000, bit: 1
	/* data generator enable*/
	#define offset_of_reg_pat_dgen_en (0)
	#define mask_of_reg_pat_dgen_en (0x2)
	unsigned int reg_pat_dgen_en:1;

	// h0000, bit: 2
	/* hsync generator*/
	#define offset_of_reg_pat_hsgen_en (0)
	#define mask_of_reg_pat_hsgen_en (0x4)
	unsigned int reg_pat_hsgen_en:1;

	// h0000, bit: 3
	/* data generator  reset*/
	#define offset_of_reg_pat_dgen_rst (0)
	#define mask_of_reg_pat_dgen_rst (0x8)
	unsigned int reg_pat_dgen_rst:1;

	// h0000, bit: 5
	/* bayer format*/
	#define offset_of_reg_pat_sensor_array (0)
	#define mask_of_reg_pat_sensor_array (0x30)
	unsigned int reg_pat_sensor_array:2;

	// h0000, bit: 7
	/* */
	unsigned int :2;

	// h0000, bit: 10
	/* data enable valid rate control*/
	#define offset_of_reg_pat_de_rate (0)
	#define mask_of_reg_pat_de_rate (0x700)
	unsigned int reg_pat_de_rate:3;

	// h0000, bit: 14
	/* */
	unsigned int :5;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 11
	/* image width minus one*/
	#define offset_of_reg_pat_img_width_m1 (2)
	#define mask_of_reg_pat_img_width_m1 (0xfff)
	unsigned int reg_pat_img_width_m1:12;

	// h0001, bit: 14
	/* */
	unsigned int :4;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 11
	/* image height minus one*/
	#define offset_of_reg_pat_img_height_m1 (4)
	#define mask_of_reg_pat_img_height_m1 (0xfff)
	unsigned int reg_pat_img_height_m1:12;

	// h0002, bit: 14
	/* */
	unsigned int :4;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 7
	/* vsync pulse position, line number before v-active*/
	#define offset_of_reg_pat_vs_line (6)
	#define mask_of_reg_pat_vs_line (0xff)
	unsigned int reg_pat_vs_line:8;

	// h0003, bit: 14
	/* hsync pulse position, pixel number before v-active*/
	#define offset_of_reg_pat_hs_pxl (6)
	#define mask_of_reg_pat_hs_pxl (0xff00)
	unsigned int reg_pat_hs_pxl:8;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 7
	/* H-blanking time (x2)*/
	#define offset_of_reg_pat_hblank (8)
	#define mask_of_reg_pat_hblank (0xff)
	unsigned int reg_pat_hblank:8;

	// h0004, bit: 14
	/* V-blanking time*/
	#define offset_of_reg_pat_vblank (8)
	#define mask_of_reg_pat_vblank (0xff00)
	unsigned int reg_pat_vblank:8;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 11
	/* pattern block width minus one*/
	#define offset_of_reg_pat_blk_width_m1 (10)
	#define mask_of_reg_pat_blk_width_m1 (0xfff)
	unsigned int reg_pat_blk_width_m1:12;

	// h0005, bit: 14
	/* */
	unsigned int :4;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 11
	/* pattern block height minus one*/
	#define offset_of_reg_pat_blk_height_m1 (12)
	#define mask_of_reg_pat_blk_height_m1 (0xfff)
	unsigned int reg_pat_blk_height_m1:12;

	// h0006, bit: 14
	/* */
	unsigned int :4;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 2
	/* initial color bar index*/
	#define offset_of_reg_pat_color_init_idx (14)
	#define mask_of_reg_pat_color_init_idx (0x7)
	unsigned int reg_pat_color_init_idx:3;

	// h0007, bit: 3
	/* */
	unsigned int :1;

	// h0007, bit: 5
	/* color bar value percentage*/
	#define offset_of_reg_pat_color_percent (14)
	#define mask_of_reg_pat_color_percent (0x30)
	unsigned int reg_pat_color_percent:2;

	// h0007, bit: 14
	/* */
	unsigned int :10;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 7
	/* frame pattern change rate*/
	#define offset_of_reg_pat_frm_chg_rate (16)
	#define mask_of_reg_pat_frm_chg_rate (0xff)
	unsigned int reg_pat_frm_chg_rate:8;

	// h0008, bit: 11
	/* pattern shift pixel number when frame change*/
	#define offset_of_reg_pat_frm_chg_x (16)
	#define mask_of_reg_pat_frm_chg_x (0xf00)
	unsigned int reg_pat_frm_chg_x:4;

	// h0008, bit: 14
	/* pattern shift line number when frame change*/
	#define offset_of_reg_pat_frm_chg_y (16)
	#define mask_of_reg_pat_frm_chg_y (0xf000)
	unsigned int reg_pat_frm_chg_y:4;

	// h0008
	unsigned int /* padding 16 bit */:16;

	// h0009, bit: 14
	/* */
	unsigned int :16;

	// h0009
	unsigned int /* padding 16 bit */:16;

	// h000a, bit: 14
	/* */
	unsigned int :16;

	// h000a
	unsigned int /* padding 16 bit */:16;

	// h000b, bit: 14
	/* */
	unsigned int :16;

	// h000b
	unsigned int /* padding 16 bit */:16;

	// h000c, bit: 14
	/* */
	unsigned int :16;

	// h000c
	unsigned int /* padding 16 bit */:16;

	// h000d, bit: 14
	/* */
	unsigned int :16;

	// h000d
	unsigned int /* padding 16 bit */:16;

	// h000e, bit: 14
	/* */
	unsigned int :16;

	// h000e
	unsigned int /* padding 16 bit */:16;

	// h000f, bit: 14
	/* */
	unsigned int :16;

	// h000f
	unsigned int /* padding 16 bit */:16;

	// h0010, bit: 14
	/* */
	unsigned int :16;

	// h0010
	unsigned int /* padding 16 bit */:16;

	// h0011, bit: 14
	/* */
	unsigned int :16;

	// h0011
	unsigned int /* padding 16 bit */:16;

	// h0012, bit: 14
	/* */
	unsigned int :16;

	// h0012
	unsigned int /* padding 16 bit */:16;

	// h0013, bit: 14
	/* */
	unsigned int :16;

	// h0013
	unsigned int /* padding 16 bit */:16;

	// h0014, bit: 14
	/* */
	unsigned int :16;

	// h0014
	unsigned int /* padding 16 bit */:16;

	// h0015, bit: 14
	/* */
	unsigned int :16;

	// h0015
	unsigned int /* padding 16 bit */:16;

	// h0016, bit: 14
	/* */
	unsigned int :16;

	// h0016
	unsigned int /* padding 16 bit */:16;

	// h0017, bit: 14
	/* */
	unsigned int :16;

	// h0017
	unsigned int /* padding 16 bit */:16;

	// h0018, bit: 14
	/* */
	unsigned int :16;

	// h0018
	unsigned int /* padding 16 bit */:16;

	// h0019, bit: 14
	/* */
	unsigned int :16;

	// h0019
	unsigned int /* padding 16 bit */:16;

	// h001a, bit: 14
	/* */
	unsigned int :16;

	// h001a
	unsigned int /* padding 16 bit */:16;

	// h001b, bit: 14
	/* */
	unsigned int :16;

	// h001b
	unsigned int /* padding 16 bit */:16;

	// h001c, bit: 14
	/* */
	unsigned int :16;

	// h001c
	unsigned int /* padding 16 bit */:16;

	// h001d, bit: 14
	/* */
	unsigned int :16;

	// h001d
	unsigned int /* padding 16 bit */:16;

	// h001e, bit: 14
	/* */
	unsigned int :16;

	// h001e
	unsigned int /* padding 16 bit */:16;

	// h001f, bit: 14
	/* */
	unsigned int :16;

	// h001f
	unsigned int /* padding 16 bit */:16;

	// h0020, bit: 14
	/* */
	unsigned int :16;

	// h0020
	unsigned int /* padding 16 bit */:16;

	// h0021, bit: 14
	/* */
	unsigned int :16;

	// h0021
	unsigned int /* padding 16 bit */:16;

	// h0022, bit: 14
	/* */
	unsigned int :16;

	// h0022
	unsigned int /* padding 16 bit */:16;

	// h0023, bit: 14
	/* */
	unsigned int :16;

	// h0023
	unsigned int /* padding 16 bit */:16;

	// h0024, bit: 14
	/* */
	unsigned int :16;

	// h0024
	unsigned int /* padding 16 bit */:16;

	// h0025, bit: 14
	/* */
	unsigned int :16;

	// h0025
	unsigned int /* padding 16 bit */:16;

	// h0026, bit: 14
	/* */
	unsigned int :16;

	// h0026
	unsigned int /* padding 16 bit */:16;

	// h0027, bit: 14
	/* */
	unsigned int :16;

	// h0027
	unsigned int /* padding 16 bit */:16;

	// h0028, bit: 14
	/* */
	unsigned int :16;

	// h0028
	unsigned int /* padding 16 bit */:16;

	// h0029, bit: 14
	/* */
	unsigned int :16;

	// h0029
	unsigned int /* padding 16 bit */:16;

	// h002a, bit: 14
	/* */
	unsigned int :16;

	// h002a
	unsigned int /* padding 16 bit */:16;

	// h002b, bit: 14
	/* */
	unsigned int :16;

	// h002b
	unsigned int /* padding 16 bit */:16;

	// h002c, bit: 14
	/* */
	unsigned int :16;

	// h002c
	unsigned int /* padding 16 bit */:16;

	// h002d, bit: 14
	/* */
	unsigned int :16;

	// h002d
	unsigned int /* padding 16 bit */:16;

	// h002e, bit: 14
	/* */
	unsigned int :16;

	// h002e
	unsigned int /* padding 16 bit */:16;

	// h002f, bit: 14
	/* */
	unsigned int :16;

	// h002f
	unsigned int /* padding 16 bit */:16;

	// h0030, bit: 14
	/* */
	unsigned int :16;

	// h0030
	unsigned int /* padding 16 bit */:16;

	// h0031, bit: 14
	/* */
	unsigned int :16;

	// h0031
	unsigned int /* padding 16 bit */:16;

	// h0032, bit: 14
	/* */
	unsigned int :16;

	// h0032
	unsigned int /* padding 16 bit */:16;

	// h0033, bit: 14
	/* */
	unsigned int :16;

	// h0033
	unsigned int /* padding 16 bit */:16;

	// h0034, bit: 14
	/* */
	unsigned int :16;

	// h0034
	unsigned int /* padding 16 bit */:16;

	// h0035, bit: 14
	/* */
	unsigned int :16;

	// h0035
	unsigned int /* padding 16 bit */:16;

	// h0036, bit: 14
	/* */
	unsigned int :16;

	// h0036
	unsigned int /* padding 16 bit */:16;

	// h0037, bit: 14
	/* */
	unsigned int :16;

	// h0037
	unsigned int /* padding 16 bit */:16;

	// h0038, bit: 14
	/* */
	unsigned int :16;

	// h0038
	unsigned int /* padding 16 bit */:16;

	// h0039, bit: 14
	/* */
	unsigned int :16;

	// h0039
	unsigned int /* padding 16 bit */:16;

	// h003a, bit: 14
	/* */
	unsigned int :16;

	// h003a
	unsigned int /* padding 16 bit */:16;

	// h003b, bit: 14
	/* */
	unsigned int :16;

	// h003b
	unsigned int /* padding 16 bit */:16;

	// h003c, bit: 14
	/* */
	unsigned int :16;

	// h003c
	unsigned int /* padding 16 bit */:16;

	// h003d, bit: 14
	/* */
	unsigned int :16;

	// h003d
	unsigned int /* padding 16 bit */:16;

	// h003e, bit: 14
	/* */
	unsigned int :16;

	// h003e
	unsigned int /* padding 16 bit */:16;

	// h003f, bit: 14
	/* */
	unsigned int :16;

	// h003f
	unsigned int /* padding 16 bit */:16;

	// h0040, bit: 14
	/* */
	unsigned int :16;

	// h0040
	unsigned int /* padding 16 bit */:16;

	// h0041, bit: 14
	/* */
	unsigned int :16;

	// h0041
	unsigned int /* padding 16 bit */:16;

	// h0042, bit: 14
	/* */
	unsigned int :16;

	// h0042
	unsigned int /* padding 16 bit */:16;

	// h0043, bit: 14
	/* */
	unsigned int :16;

	// h0043
	unsigned int /* padding 16 bit */:16;

	// h0044, bit: 14
	/* */
	unsigned int :16;

	// h0044
	unsigned int /* padding 16 bit */:16;

	// h0045, bit: 14
	/* */
	unsigned int :16;

	// h0045
	unsigned int /* padding 16 bit */:16;

	// h0046, bit: 14
	/* */
	unsigned int :16;

	// h0046
	unsigned int /* padding 16 bit */:16;

	// h0047, bit: 14
	/* */
	unsigned int :16;

	// h0047
	unsigned int /* padding 16 bit */:16;

	// h0048, bit: 14
	/* */
	unsigned int :16;

	// h0048
	unsigned int /* padding 16 bit */:16;

	// h0049, bit: 14
	/* */
	unsigned int :16;

	// h0049
	unsigned int /* padding 16 bit */:16;

	// h004a, bit: 14
	/* */
	unsigned int :16;

	// h004a
	unsigned int /* padding 16 bit */:16;

	// h004b, bit: 14
	/* */
	unsigned int :16;

	// h004b
	unsigned int /* padding 16 bit */:16;

	// h004c, bit: 14
	/* */
	unsigned int :16;

	// h004c
	unsigned int /* padding 16 bit */:16;

	// h004d, bit: 14
	/* */
	unsigned int :16;

	// h004d
	unsigned int /* padding 16 bit */:16;

	// h004e, bit: 14
	/* */
	unsigned int :16;

	// h004e
	unsigned int /* padding 16 bit */:16;

	// h004f, bit: 14
	/* */
	unsigned int :16;

	// h004f
	unsigned int /* padding 16 bit */:16;

	// h0050, bit: 14
	/* */
	unsigned int :16;

	// h0050
	unsigned int /* padding 16 bit */:16;

	// h0051, bit: 14
	/* */
	unsigned int :16;

	// h0051
	unsigned int /* padding 16 bit */:16;

	// h0052, bit: 14
	/* */
	unsigned int :16;

	// h0052
	unsigned int /* padding 16 bit */:16;

	// h0053, bit: 14
	/* */
	unsigned int :16;

	// h0053
	unsigned int /* padding 16 bit */:16;

	// h0054, bit: 14
	/* */
	unsigned int :16;

	// h0054
	unsigned int /* padding 16 bit */:16;

	// h0055, bit: 14
	/* */
	unsigned int :16;

	// h0055
	unsigned int /* padding 16 bit */:16;

	// h0056, bit: 14
	/* */
	unsigned int :16;

	// h0056
	unsigned int /* padding 16 bit */:16;

	// h0057, bit: 14
	/* */
	unsigned int :16;

	// h0057
	unsigned int /* padding 16 bit */:16;

	// h0058, bit: 14
	/* */
	unsigned int :16;

	// h0058
	unsigned int /* padding 16 bit */:16;

	// h0059, bit: 14
	/* */
	unsigned int :16;

	// h0059
	unsigned int /* padding 16 bit */:16;

	// h005a, bit: 14
	/* */
	unsigned int :16;

	// h005a
	unsigned int /* padding 16 bit */:16;

	// h005b, bit: 14
	/* */
	unsigned int :16;

	// h005b
	unsigned int /* padding 16 bit */:16;

	// h005c, bit: 14
	/* */
	unsigned int :16;

	// h005c
	unsigned int /* padding 16 bit */:16;

	// h005d, bit: 14
	/* */
	unsigned int :16;

	// h005d
	unsigned int /* padding 16 bit */:16;

	// h005e, bit: 14
	/* */
	unsigned int :16;

	// h005e
	unsigned int /* padding 16 bit */:16;

	// h005f, bit: 14
	/* */
	unsigned int :16;

	// h005f
	unsigned int /* padding 16 bit */:16;

	// h0060, bit: 14
	/* */
	unsigned int :16;

	// h0060
	unsigned int /* padding 16 bit */:16;

	// h0061, bit: 14
	/* */
	unsigned int :16;

	// h0061
	unsigned int /* padding 16 bit */:16;

	// h0062, bit: 14
	/* */
	unsigned int :16;

	// h0062
	unsigned int /* padding 16 bit */:16;

	// h0063, bit: 14
	/* */
	unsigned int :16;

	// h0063
	unsigned int /* padding 16 bit */:16;

	// h0064, bit: 14
	/* */
	unsigned int :16;

	// h0064
	unsigned int /* padding 16 bit */:16;

	// h0065, bit: 14
	/* */
	unsigned int :16;

	// h0065
	unsigned int /* padding 16 bit */:16;

	// h0066, bit: 14
	/* */
	unsigned int :16;

	// h0066
	unsigned int /* padding 16 bit */:16;

	// h0067, bit: 14
	/* */
	unsigned int :16;

	// h0067
	unsigned int /* padding 16 bit */:16;

	// h0068, bit: 14
	/* */
	unsigned int :16;

	// h0068
	unsigned int /* padding 16 bit */:16;

	// h0069, bit: 14
	/* */
	unsigned int :16;

	// h0069
	unsigned int /* padding 16 bit */:16;

	// h006a, bit: 14
	/* */
	unsigned int :16;

	// h006a
	unsigned int /* padding 16 bit */:16;

	// h006b, bit: 14
	/* */
	unsigned int :16;

	// h006b
	unsigned int /* padding 16 bit */:16;

	// h006c, bit: 14
	/* */
	unsigned int :16;

	// h006c
	unsigned int /* padding 16 bit */:16;

	// h006d, bit: 14
	/* */
	unsigned int :16;

	// h006d
	unsigned int /* padding 16 bit */:16;

	// h006e, bit: 14
	/* */
	unsigned int :16;

	// h006e
	unsigned int /* padding 16 bit */:16;

	// h006f, bit: 14
	/* */
	unsigned int :16;

	// h006f
	unsigned int /* padding 16 bit */:16;

	// h0070, bit: 14
	/* */
	unsigned int :16;

	// h0070
	unsigned int /* padding 16 bit */:16;

	// h0071, bit: 14
	/* */
	unsigned int :16;

	// h0071
	unsigned int /* padding 16 bit */:16;

	// h0072, bit: 14
	/* */
	unsigned int :16;

	// h0072
	unsigned int /* padding 16 bit */:16;

	// h0073, bit: 14
	/* */
	unsigned int :16;

	// h0073
	unsigned int /* padding 16 bit */:16;

	// h0074, bit: 14
	/* */
	unsigned int :16;

	// h0074
	unsigned int /* padding 16 bit */:16;

	// h0075, bit: 14
	/* */
	unsigned int :16;

	// h0075
	unsigned int /* padding 16 bit */:16;

	// h0076, bit: 14
	/* */
	unsigned int :16;

	// h0076
	unsigned int /* padding 16 bit */:16;

	// h0077, bit: 14
	/* */
	unsigned int :16;

	// h0077
	unsigned int /* padding 16 bit */:16;

	// h0078, bit: 0
	/* i2c clock enable*/
	#define offset_of_reg_i2c_en (240)
	#define mask_of_reg_i2c_en (0x1)
	unsigned int reg_i2c_en:1;

	// h0078, bit: 7
	/* */
	unsigned int :7;

	// h0078, bit: 8
	/* i2c resetz*/
	#define offset_of_reg_i2c_rstz (240)
	#define mask_of_reg_i2c_rstz (0x100)
	unsigned int reg_i2c_rstz:1;

	// h0078, bit: 14
	/* */
	unsigned int :7;

	// h0078
	unsigned int /* padding 16 bit */:16;

	// h0079, bit: 7
	/* 2nd Register Address for 16 bit register address series mode
	 (single buffer)*/
	#define offset_of_reg_sen_m2s_2nd_reg_adr (242)
	#define mask_of_reg_sen_m2s_2nd_reg_adr (0xff)
	unsigned int reg_sen_m2s_2nd_reg_adr:8;

	// h0079, bit: 14
	/* */
	unsigned int :8;

	// h0079
	unsigned int /* padding 16 bit */:16;

	// h007a, bit: 0
	/* M2S Signal Control
	# 1'b0: by HW
	# 1'b1: by SW
	 ( single buffer register)*/
	#define offset_of_reg_sen_m2s_sw_ctrl (244)
	#define mask_of_reg_sen_m2s_sw_ctrl (0x1)
	unsigned int reg_sen_m2s_sw_ctrl:1;

	// h007a, bit: 1
	/* M2S mode.
	# 1'b1: OmniVision SIF.
	# 1'b0: Others.
	 ( single buffer register )*/
	#define offset_of_reg_sen_m2s_mode (244)
	#define mask_of_reg_sen_m2s_mode (0x2)
	unsigned int reg_sen_m2s_mode:1;

	// h007a, bit: 2
	/* M2S burst command length.
	# 1'b0: length is 1.
	# 1'b1: length is 2.
	 ( single buffer register )*/
	#define offset_of_reg_sen_m2s_cmd_bl (244)
	#define mask_of_reg_sen_m2s_cmd_bl (0x4)
	unsigned int reg_sen_m2s_cmd_bl:1;

	// h007a, bit: 3
	/* M2S Register Address Mode
	# 1'b0 : 8 bit mode
	# 1'b1 : 16 bit series mode
	 (single buffer register)*/
	#define offset_of_reg_sen_m2s_reg_adr_mode (244)
	#define mask_of_reg_sen_m2s_reg_adr_mode (0x8)
	unsigned int reg_sen_m2s_reg_adr_mode:1;

	// h007a, bit: 13
	/* */
	unsigned int :10;

	// h007a, bit: 14
	/* M2S SDA Input
	 ( single buffer register )*/
	#define offset_of_reg_sda_i (244)
	#define mask_of_reg_sda_i (0x4000)
	unsigned int reg_sda_i:1;

	// h007a, bit: 15
	/* M2S Status
	# 1'b0: Idle
	# 1'b1: Busy
	 ( single buffer register )*/
	#define offset_of_reg_m2s_status (244)
	#define mask_of_reg_m2s_status (0x8000)
	unsigned int reg_m2s_status:1;

	// h007a
	unsigned int /* padding 16 bit */:16;

	// h007b, bit: 14
	/* M2S command
	# [0]: 1: read, 0: write.
	# [7:1]: Slave address.
	# [15:8]: Register address for 8 bit mode / first register address for 16 bit series mode
	 ( single buffer register )*/
	#define offset_of_reg_sen_m2s_cmd (246)
	#define mask_of_reg_sen_m2s_cmd (0xffff)
	unsigned int reg_sen_m2s_cmd:16;

	// h007b
	unsigned int /* padding 16 bit */:16;

	// h007c, bit: 14
	/* M2S Write data.
	# [7:0]: The first byte of Write data.
	# [15:8]: The second byte of Write data.
	Not valid if reg_sen_m2s_cmd_bl is 0.
	 ( single buffer register )*/
	#define offset_of_reg_sen_m2s_rw_d (248)
	#define mask_of_reg_sen_m2s_rw_d (0xffff)
	unsigned int reg_sen_m2s_rw_d:16;

	// h007c
	unsigned int /* padding 16 bit */:16;

	// h007d, bit: 9
	/* Factor of division for generating M2S clock.
	Must be programmed less than SYSCLK /(4*M2S_frequency) minus 1
	 ( single buffer register )*/
	#define offset_of_reg_sen_prescale (250)
	#define mask_of_reg_sen_prescale (0x3ff)
	unsigned int reg_sen_prescale:10;

	// h007d, bit: 14
	/* */
	unsigned int :6;

	// h007d
	unsigned int /* padding 16 bit */:16;

	// h007e, bit: 0
	/* M2S scl control
	# 1'b0: scl low
	# 1'b1: scl high*/
	#define offset_of_reg_sen_sw_scl_oen (252)
	#define mask_of_reg_sen_sw_scl_oen (0x1)
	unsigned int reg_sen_sw_scl_oen:1;

	// h007e, bit: 1
	/* M2S sda control
	# 1'b0: sda output low
	# 1'b1: sda input or sda output high*/
	#define offset_of_reg_sen_sw_sda_oen (252)
	#define mask_of_reg_sen_sw_sda_oen (0x2)
	unsigned int reg_sen_sw_sda_oen:1;

	// h007e, bit: 14
	/* */
	unsigned int :14;

	// h007e
	unsigned int /* padding 16 bit */:16;

	// h007f, bit: 14
	/* M2S Read data.
	# [7:0]: The first byte of Read data.
	# [15:8]: The second byte of Read data.
	Not valid if reg_sen_m2s_cmd_bl is 0.
	 ( single buffer register )*/
	#define offset_of_reg_sen_rd (254)
	#define mask_of_reg_sen_rd (0xffff)
	unsigned int reg_sen_rd:16;

	// h007f
	unsigned int /* padding 16 bit */:16;

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp7;
#endif
