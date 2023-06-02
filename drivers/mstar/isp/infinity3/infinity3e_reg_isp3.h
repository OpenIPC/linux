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
#ifndef __INFINITY3E_REG_ISP3__
#define __INFINITY3E_REG_ISP3__
typedef struct {
	// h0000, bit: 0
	/* ISP statistic enable*/
	#define offset_of_reg_isp_statis_en (0)
	#define mask_of_reg_isp_statis_en (0x1)
	unsigned int reg_isp_statis_en:1;

	// h0000, bit: 1
	/* Trigger of ISP statistic one frame mode*/
	#define offset_of_reg_statis_once_trig (0)
	#define mask_of_reg_statis_once_trig (0x2)
	unsigned int reg_statis_once_trig:1;

	// h0000, bit: 2
	/* ISP statistic AE enable*/
	#define offset_of_reg_isp_statis_ae_en (0)
	#define mask_of_reg_isp_statis_ae_en (0x4)
	unsigned int reg_isp_statis_ae_en:1;

	// h0000, bit: 3
	/* ISP statistic AWB enable*/
	#define offset_of_reg_isp_statis_awb_en (0)
	#define mask_of_reg_isp_statis_awb_en (0x8)
	unsigned int reg_isp_statis_awb_en:1;

	// h0000, bit: 7
	/* */
	unsigned int :4;

	// h0000, bit: 14
	/* AE block row number for interrupt*/
	#define offset_of_reg_ae_int_row_num (0)
	#define mask_of_reg_ae_int_row_num (0x7f00)
	unsigned int reg_ae_int_row_num:7;

	// h0000, bit: 15
	/* */
	unsigned int :1;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 9
	/* AE block width minus one*/
	#define offset_of_reg_ae_blksize_x_m1 (2)
	#define mask_of_reg_ae_blksize_x_m1 (0x3ff)
	unsigned int reg_ae_blksize_x_m1:10;

	// h0001, bit: 14
	/* */
	unsigned int :6;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 9
	/* AE block height minus one*/
	#define offset_of_reg_ae_blksize_y_m1 (4)
	#define mask_of_reg_ae_blksize_y_m1 (0x3ff)
	unsigned int reg_ae_blksize_y_m1:10;

	// h0002, bit: 14
	/* */
	unsigned int :6;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 6
	/* AE Horizontal block number minus one*/
	#define offset_of_reg_ae_blknum_x_m1 (6)
	#define mask_of_reg_ae_blknum_x_m1 (0x7f)
	unsigned int reg_ae_blknum_x_m1:7;

	// h0003, bit: 7
	/* */
	unsigned int :1;

	// h0003, bit: 14
	/* AE vertical block number minus one*/
	#define offset_of_reg_ae_blknum_y_m1 (6)
	#define mask_of_reg_ae_blknum_y_m1 (0x7f00)
	unsigned int reg_ae_blknum_y_m1:7;

	// h0003, bit: 15
	/* */
	unsigned int :1;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 11
	/* AE horizontal block pixel offset*/
	#define offset_of_reg_ae_blk_h_offset (8)
	#define mask_of_reg_ae_blk_h_offset (0xfff)
	unsigned int reg_ae_blk_h_offset:12;

	// h0004, bit: 14
	/* */
	unsigned int :4;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 11
	/* AE veritcal block pixel offset*/
	#define offset_of_reg_ae_blk_v_offset (10)
	#define mask_of_reg_ae_blk_v_offset (0xfff)
	unsigned int reg_ae_blk_v_offset:12;

	// h0005, bit: 14
	/* */
	unsigned int :4;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 14
	/* AE statistic  frame buffer (unit : 16bytes)
	Offset 0~4 : Red histogram
	Offset 5~9 : Green histogram
	Offset 10~14 : Blue histogram
	Offset 15~30 : WIn0 histogram
	Offset 31~46 : WIn1 histogram
	Offset 47~ : R/G/B/Y block average*/
	#define offset_of_reg_ae_statis_base (12)
	#define mask_of_reg_ae_statis_base (0xffff)
	unsigned int reg_ae_statis_base:16;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 10
	/* AE statistic  frame buffer (unit : 16bytes)
	Offset 0~4 : Red histogram
	Offset 5~9 : Green histogram
	Offset 10~14 : Blue histogram
	Offset 15~30 : WIn0 histogram
	Offset 31~46 : WIn1 histogram
	Offset 47~ : R/G/B/Y block average*/
	#define offset_of_reg_ae_statis_base_1 (14)
	#define mask_of_reg_ae_statis_base_1 (0x7ff)
	unsigned int reg_ae_statis_base_1:11;

	// h0007, bit: 14
	/* */
	unsigned int :5;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 14
	/* AWB statistic  frame buffer (unit : 16bytes)*/
	#define offset_of_reg_awb_statis_base (16)
	#define mask_of_reg_awb_statis_base (0xffff)
	unsigned int reg_awb_statis_base:16;

	// h0008
	unsigned int /* padding 16 bit */:16;

	// h0009, bit: 10
	/* AWB statistic  frame buffer (unit : 16bytes)*/
	#define offset_of_reg_awb_statis_base_1 (18)
	#define mask_of_reg_awb_statis_base_1 (0x7ff)
	unsigned int reg_awb_statis_base_1:11;

	// h0009, bit: 14
	/* */
	unsigned int :5;

	// h0009
	unsigned int /* padding 16 bit */:16;

	// h000a, bit: 14
	/* Histo statistic  frame buffer (unit : 16bytes)*/
	#define offset_of_reg_histo_statis_base (20)
	#define mask_of_reg_histo_statis_base (0xffff)
	unsigned int reg_histo_statis_base:16;

	// h000a
	unsigned int /* padding 16 bit */:16;

	// h000b, bit: 10
	/* Histo statistic  frame buffer (unit : 16bytes)*/
	#define offset_of_reg_histo_statis_base_1 (22)
	#define mask_of_reg_histo_statis_base_1 (0x7ff)
	unsigned int reg_histo_statis_base_1:11;

	// h000b, bit: 14
	/* */
	unsigned int :5;

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

	// h000e, bit: 1
	/* AWB write miu burst length x4*/
	#define offset_of_reg_statis_awb_mi_lenx4 (28)
	#define mask_of_reg_statis_awb_mi_lenx4 (0x3)
	unsigned int reg_statis_awb_mi_lenx4:2;

	// h000e, bit: 7
	/* AWB write miu mask period between each burst*/
	#define offset_of_reg_statis_awb_mi_mask_cycx16 (28)
	#define mask_of_reg_statis_awb_mi_mask_cycx16 (0xfc)
	unsigned int reg_statis_awb_mi_mask_cycx16:6;

	// h000e, bit: 9
	/* Block average write miu burst length x4*/
	#define offset_of_reg_statis_avg_mi_lenx4 (28)
	#define mask_of_reg_statis_avg_mi_lenx4 (0x300)
	unsigned int reg_statis_avg_mi_lenx4:2;

	// h000e, bit: 14
	/* Block average write miu mask period between each burst*/
	#define offset_of_reg_statis_avg_mi_mask_cycx16 (28)
	#define mask_of_reg_statis_avg_mi_mask_cycx16 (0xfc00)
	unsigned int reg_statis_avg_mi_mask_cycx16:6;

	// h000e
	unsigned int /* padding 16 bit */:16;

	// h000f, bit: 1
	/* Histogram write miu burst length x4*/
	#define offset_of_reg_statis_histo_mi_lenx4 (30)
	#define mask_of_reg_statis_histo_mi_lenx4 (0x3)
	unsigned int reg_statis_histo_mi_lenx4:2;

	// h000f, bit: 7
	/* Histogram  write miu mask period between each burst*/
	#define offset_of_reg_statis_histo_mi_mask_cycx16 (30)
	#define mask_of_reg_statis_histo_mi_mask_cycx16 (0xfc)
	unsigned int reg_statis_histo_mi_mask_cycx16:6;

	// h000f, bit: 14
	/* */
	unsigned int :8;

	// h000f
	unsigned int /* padding 16 bit */:16;

	// h0010, bit: 14
	/* AE average statistic  max miu write address*/
	#define offset_of_reg_ae_max_wadr (32)
	#define mask_of_reg_ae_max_wadr (0xffff)
	unsigned int reg_ae_max_wadr:16;

	// h0010
	unsigned int /* padding 16 bit */:16;

	// h0011, bit: 10
	/* AE average statistic  max miu write address*/
	#define offset_of_reg_ae_max_wadr_1 (34)
	#define mask_of_reg_ae_max_wadr_1 (0x7ff)
	unsigned int reg_ae_max_wadr_1:11;

	// h0011, bit: 14
	/* */
	unsigned int :5;

	// h0011
	unsigned int /* padding 16 bit */:16;

	// h0012, bit: 14
	/* AWB statistic  max miu write address*/
	#define offset_of_reg_awb_max_wadr (36)
	#define mask_of_reg_awb_max_wadr (0xffff)
	unsigned int reg_awb_max_wadr:16;

	// h0012
	unsigned int /* padding 16 bit */:16;

	// h0013, bit: 10
	/* AWB statistic  max miu write address*/
	#define offset_of_reg_awb_max_wadr_1 (38)
	#define mask_of_reg_awb_max_wadr_1 (0x7ff)
	unsigned int reg_awb_max_wadr_1:11;

	// h0013, bit: 14
	/* */
	unsigned int :5;

	// h0013
	unsigned int /* padding 16 bit */:16;

	// h0014, bit: 14
	/* Histogram statistic  max miu write address*/
	#define offset_of_reg_histo_max_wadr (40)
	#define mask_of_reg_histo_max_wadr (0xffff)
	unsigned int reg_histo_max_wadr:16;

	// h0014
	unsigned int /* padding 16 bit */:16;

	// h0015, bit: 10
	/* Histogram statistic  max miu write address*/
	#define offset_of_reg_histo_max_wadr_1 (42)
	#define mask_of_reg_histo_max_wadr_1 (0x7ff)
	unsigned int reg_histo_max_wadr_1:11;

	// h0015, bit: 14
	/* */
	unsigned int :5;

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
	/* AE block average divisor A minus one*/
	#define offset_of_reg_ae_div_a_m1 (48)
	#define mask_of_reg_ae_div_a_m1 (0xffff)
	unsigned int reg_ae_div_a_m1:16;

	// h0018
	unsigned int /* padding 16 bit */:16;

	// h0019, bit: 14
	/* AE block average divisor B minus one*/
	#define offset_of_reg_ae_div_b_m1 (50)
	#define mask_of_reg_ae_div_b_m1 (0xffff)
	unsigned int reg_ae_div_b_m1:16;

	// h0019
	unsigned int /* padding 16 bit */:16;

	// h001a, bit: 14
	/* AE block average divisor C minus one*/
	#define offset_of_reg_ae_div_c_m1 (52)
	#define mask_of_reg_ae_div_c_m1 (0xffff)
	unsigned int reg_ae_div_c_m1:16;

	// h001a
	unsigned int /* padding 16 bit */:16;

	// h001b, bit: 14
	/* AE block average divisor D minus one*/
	#define offset_of_reg_ae_div_d_m1 (54)
	#define mask_of_reg_ae_div_d_m1 (0xffff)
	unsigned int reg_ae_div_d_m1:16;

	// h001b
	unsigned int /* padding 16 bit */:16;

	// h001c, bit: 14
	/* AE block average divisor E minus one*/
	#define offset_of_reg_ae_div_e_m1 (56)
	#define mask_of_reg_ae_div_e_m1 (0xffff)
	unsigned int reg_ae_div_e_m1:16;

	// h001c
	unsigned int /* padding 16 bit */:16;

	// h001d, bit: 14
	/* AE block average divisor F minuso one*/
	#define offset_of_reg_ae_div_f_m1 (58)
	#define mask_of_reg_ae_div_f_m1 (0xffff)
	unsigned int reg_ae_div_f_m1:16;

	// h001d
	unsigned int /* padding 16 bit */:16;

	// h001e, bit: 3
	/* AE accumulation shift register*/
	#define offset_of_reg_ae_acc_sft (60)
	#define mask_of_reg_ae_acc_sft (0xf)
	unsigned int reg_ae_acc_sft:4;

	// h001e, bit: 14
	/* */
	unsigned int :12;

	// h001e
	unsigned int /* padding 16 bit */:16;

	// h001f, bit: 14
	/* */
	unsigned int :16;

	// h001f
	unsigned int /* padding 16 bit */:16;

	// h0020, bit: 0
	/* ALSC enable*/
	#define offset_of_reg_en_alsc (64)
	#define mask_of_reg_en_alsc (0x1)
	unsigned int reg_en_alsc:1;

	// h0020, bit: 1
	/* */
	unsigned int :1;

	// h0020, bit: 2
	/* ALSC dither enable*/
	#define offset_of_reg_alsc_dither_en (64)
	#define mask_of_reg_alsc_dither_en (0x4)
	unsigned int reg_alsc_dither_en:1;

	// h0020, bit: 3
	/* */
	unsigned int :1;

	// h0020, bit: 5
	/* ALSC x block mode :  (blk size)
	0: 8
	1: 16
	2: 32
	3: 64*/
	#define offset_of_reg_alsc_x_blk_mode (64)
	#define mask_of_reg_alsc_x_blk_mode (0x30)
	unsigned int reg_alsc_x_blk_mode:2;

	// h0020, bit: 7
	/* ALSC y block mode :  (blk size)
	0: 8
	1: 16
	2: 32
	3: 64*/
	#define offset_of_reg_alsc_y_blk_mode (64)
	#define mask_of_reg_alsc_y_blk_mode (0xc0)
	unsigned int reg_alsc_y_blk_mode:2;

	// h0020, bit: 10
	/* ALSC debug mode enable ([0]:R; [1]:G;[2]:B)*/
	#define offset_of_reg_alsc_dbg_en (64)
	#define mask_of_reg_alsc_dbg_en (0x700)
	unsigned int reg_alsc_dbg_en:3;

	// h0020, bit: 12
	/* ALSC debug gain right shift*/
	#define offset_of_reg_alsc_dbg_shift (64)
	#define mask_of_reg_alsc_dbg_shift (0x1800)
	unsigned int reg_alsc_dbg_shift:2;

	// h0020, bit: 14
	/* */
	unsigned int :3;

	// h0020
	unsigned int /* padding 16 bit */:16;

	// h0021, bit: 11
	/* ALSC horizontal offset*/
	#define offset_of_reg_alsc_offsetx (66)
	#define mask_of_reg_alsc_offsetx (0xfff)
	unsigned int reg_alsc_offsetx:12;

	// h0021, bit: 14
	/* */
	unsigned int :4;

	// h0021
	unsigned int /* padding 16 bit */:16;

	// h0022, bit: 11
	/* ALSC vertical offset*/
	#define offset_of_reg_alsc_offsety (68)
	#define mask_of_reg_alsc_offsety (0xfff)
	unsigned int reg_alsc_offsety:12;

	// h0022, bit: 14
	/* */
	unsigned int :4;

	// h0022
	unsigned int /* padding 16 bit */:16;

	// h0023, bit: 7
	/* ALSC gain table width minus one*/
	#define offset_of_reg_alsc_gtbl_width_m1 (70)
	#define mask_of_reg_alsc_gtbl_width_m1 (0xff)
	unsigned int reg_alsc_gtbl_width_m1:8;

	// h0023, bit: 14
	/* ALSC gain table height minus one*/
	#define offset_of_reg_alsc_gtbl_height_m1 (70)
	#define mask_of_reg_alsc_gtbl_height_m1 (0xff00)
	unsigned int reg_alsc_gtbl_height_m1:8;

	// h0023
	unsigned int /* padding 16 bit */:16;

	// h0024, bit: 7
	/* ALSC gain table pitch*/
	#define offset_of_reg_alsc_gtbl_pitch (72)
	#define mask_of_reg_alsc_gtbl_pitch (0xff)
	unsigned int reg_alsc_gtbl_pitch:8;

	// h0024, bit: 14
	/* */
	unsigned int :8;

	// h0024
	unsigned int /* padding 16 bit */:16;

	// h0025, bit: 14
	/* ALSC interpolation coefficients*/
	#define offset_of_reg_coef_buf_wd (74)
	#define mask_of_reg_coef_buf_wd (0xffff)
	unsigned int reg_coef_buf_wd:16;

	// h0025
	unsigned int /* padding 16 bit */:16;

	// h0026, bit: 14
	/* ALSC interpolation coefficients*/
	#define offset_of_reg_coef_buf_wd_1 (76)
	#define mask_of_reg_coef_buf_wd_1 (0xffff)
	unsigned int reg_coef_buf_wd_1:16;

	// h0026
	unsigned int /* padding 16 bit */:16;

	// h0027, bit: 5
	/* ALSC coefficient  table address
	H : 0~32
	V : 0~32*/
	#define offset_of_reg_coef_buf_adr (78)
	#define mask_of_reg_coef_buf_adr (0x3f)
	unsigned int reg_coef_buf_adr:6;

	// h0027, bit: 6
	/* */
	unsigned int :1;

	// h0027, bit: 7
	/* ALSC coefficient  table select :
	0 : Horizontal
	1 : Vertical*/
	#define offset_of_reg_coef_buf_sel_v (78)
	#define mask_of_reg_coef_buf_sel_v (0x80)
	unsigned int reg_coef_buf_sel_v:1;

	// h0027, bit: 8
	/* ALSC coefficient  table write*/
	#define offset_of_reg_coef_buf_we (78)
	#define mask_of_reg_coef_buf_we (0x100)
	unsigned int reg_coef_buf_we:1;

	// h0027, bit: 9
	/* ALSC coefficient  table read*/
	#define offset_of_reg_coef_buf_re (78)
	#define mask_of_reg_coef_buf_re (0x200)
	unsigned int reg_coef_buf_re:1;

	// h0027, bit: 14
	/* */
	unsigned int :6;

	// h0027
	unsigned int /* padding 16 bit */:16;

	// h0028, bit: 14
	/* ALSC interpolation coefficients*/
	#define offset_of_reg_coef_buf_rd (80)
	#define mask_of_reg_coef_buf_rd (0xffff)
	unsigned int reg_coef_buf_rd:16;

	// h0028
	unsigned int /* padding 16 bit */:16;

	// h0029, bit: 14
	/* ALSC interpolation coefficients*/
	#define offset_of_reg_coef_buf_rd_1 (82)
	#define mask_of_reg_coef_buf_rd_1 (0xffff)
	unsigned int reg_coef_buf_rd_1:16;

	// h0029
	unsigned int /* padding 16 bit */:16;

	// h002a, bit: 2
	/* ALSC table maping
	3'b000: no rotation
	3'b001: clockwise 90
	3'b010: clockwise 180
	3'b011: clockwise 270
	3'b100: horizontal mirror
	3'b101: vertical mirror*/
	#define offset_of_reg_asymlsc_orientation (84)
	#define mask_of_reg_asymlsc_orientation (0x7)
	unsigned int reg_asymlsc_orientation:3;

	// h002a, bit: 14
	/* */
	unsigned int :13;

	// h002a
	unsigned int /* padding 16 bit */:16;

	// h002b, bit: 11
	/* ALSC table scalar (input frame height mapping  to 1920)*/
	#define offset_of_reg_asymlsc_scale_x (86)
	#define mask_of_reg_asymlsc_scale_x (0xfff)
	unsigned int reg_asymlsc_scale_x:12;

	// h002b, bit: 14
	/* */
	unsigned int :4;

	// h002b
	unsigned int /* padding 16 bit */:16;

	// h002c, bit: 11
	/* ALSC table scalar (input frame height mapping  to 1080)*/
	#define offset_of_reg_asymlsc_scale_y (88)
	#define mask_of_reg_asymlsc_scale_y (0xfff)
	unsigned int reg_asymlsc_scale_y:12;

	// h002c, bit: 14
	/* */
	unsigned int :4;

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

	// h0030, bit: 0
	/* DNR enable*/
	#define offset_of_reg_dnr_en (96)
	#define mask_of_reg_dnr_en (0x1)
	unsigned int reg_dnr_en:1;

	// h0030, bit: 1
	/* DNR luma adaptive enable*/
	#define offset_of_reg_dnr_lum_en (96)
	#define mask_of_reg_dnr_lum_en (0x2)
	unsigned int reg_dnr_lum_en:1;

	// h0030, bit: 2
	/* DNR luma adaptive shift bits:
	0: 3-bits
	1: 4-bits*/
	#define offset_of_reg_dnr_lum_sh (96)
	#define mask_of_reg_dnr_lum_sh (0x4)
	unsigned int reg_dnr_lum_sh:1;

	// h0030, bit: 3
	/* */
	unsigned int :1;

	// h0030, bit: 4
	/* DNR motion filter enable*/
	#define offset_of_reg_dnr_filter_en (96)
	#define mask_of_reg_dnr_filter_en (0x10)
	unsigned int reg_dnr_filter_en:1;

	// h0030, bit: 6
	/* DNR motion filter mode*/
	#define offset_of_reg_dnr_filter_mode (96)
	#define mask_of_reg_dnr_filter_mode (0x60)
	unsigned int reg_dnr_filter_mode:2;

	// h0030, bit: 7
	/* */
	unsigned int :1;

	// h0030, bit: 10
	/* DNR Div 0 for motion filter*/
	#define offset_of_reg_dnr_filter_div0 (96)
	#define mask_of_reg_dnr_filter_div0 (0x700)
	unsigned int reg_dnr_filter_div0:3;

	// h0030, bit: 11
	/* */
	unsigned int :1;

	// h0030, bit: 13
	/* DNR Div 1 for motion filter*/
	#define offset_of_reg_dnr_filter_div1 (96)
	#define mask_of_reg_dnr_filter_div1 (0x3000)
	unsigned int reg_dnr_filter_div1:2;

	// h0030, bit: 14
	/* */
	unsigned int :2;

	// h0030
	unsigned int /* padding 16 bit */:16;

	// h0031, bit: 14
	/* DNR luma adaptive table*/
	#define offset_of_reg_dnr_lum_table (98)
	#define mask_of_reg_dnr_lum_table (0xffff)
	unsigned int reg_dnr_lum_table:16;

	// h0031
	unsigned int /* padding 16 bit */:16;

	// h0032, bit: 14
	/* DNR luma adaptive table*/
	#define offset_of_reg_dnr_lum_table_1 (100)
	#define mask_of_reg_dnr_lum_table_1 (0xffff)
	unsigned int reg_dnr_lum_table_1:16;

	// h0032
	unsigned int /* padding 16 bit */:16;

	// h0033, bit: 0
	/* DNR table select*/
	#define offset_of_reg_nr_table_sel (102)
	#define mask_of_reg_nr_table_sel (0x1)
	unsigned int reg_nr_table_sel:1;

	// h0033, bit: 1
	/* DNR non-dither method enable*/
	#define offset_of_reg_dnr_nondither_method (102)
	#define mask_of_reg_dnr_nondither_method (0x2)
	unsigned int reg_dnr_nondither_method:1;

	// h0033, bit: 2
	/* DNR non-dither method select*/
	#define offset_of_reg_dnr_nondither_method_mode (102)
	#define mask_of_reg_dnr_nondither_method_mode (0x4)
	unsigned int reg_dnr_nondither_method_mode:1;

	// h0033, bit: 4
	/* DNR rounding mode for blending*/
	#define offset_of_reg_round_mode (102)
	#define mask_of_reg_round_mode (0x18)
	unsigned int reg_round_mode:2;

	// h0033, bit: 7
	/* */
	unsigned int :3;

	// h0033, bit: 12
	/* DNR sticky solver threshold*/
	#define offset_of_reg_sticky_solver_th (102)
	#define mask_of_reg_sticky_solver_th (0x1f00)
	unsigned int reg_sticky_solver_th:5;

	// h0033, bit: 14
	/* */
	unsigned int :2;

	// h0033, bit: 15
	/* DNR sticky solver enable*/
	#define offset_of_reg_sticky_solver_en (102)
	#define mask_of_reg_sticky_solver_en (0x8000)
	unsigned int reg_sticky_solver_en:1;

	// h0033
	unsigned int /* padding 16 bit */:16;

	// h0034, bit: 14
	/* DNR table*/
	#define offset_of_reg_dnr_tabley (104)
	#define mask_of_reg_dnr_tabley (0xffff)
	unsigned int reg_dnr_tabley:16;

	// h0034
	unsigned int /* padding 16 bit */:16;

	// h0035, bit: 14
	/* DNR table*/
	#define offset_of_reg_dnr_tabley_1 (106)
	#define mask_of_reg_dnr_tabley_1 (0xffff)
	unsigned int reg_dnr_tabley_1:16;

	// h0035
	unsigned int /* padding 16 bit */:16;

	// h0036, bit: 14
	/* DNR table*/
	#define offset_of_reg_dnr_tabley_2 (108)
	#define mask_of_reg_dnr_tabley_2 (0xffff)
	unsigned int reg_dnr_tabley_2:16;

	// h0036
	unsigned int /* padding 16 bit */:16;

	// h0037, bit: 14
	/* DNR table*/
	#define offset_of_reg_dnr_tabley_3 (110)
	#define mask_of_reg_dnr_tabley_3 (0xffff)
	unsigned int reg_dnr_tabley_3:16;

	// h0037
	unsigned int /* padding 16 bit */:16;

	// h0038, bit: 14
	/* DNR frame buffer base address 0*/
	#define offset_of_reg_dnr_fb_base_0 (112)
	#define mask_of_reg_dnr_fb_base_0 (0xffff)
	unsigned int reg_dnr_fb_base_0:16;

	// h0038
	unsigned int /* padding 16 bit */:16;

	// h0039, bit: 10
	/* DNR frame buffer base address 0*/
	#define offset_of_reg_dnr_fb_base_0_1 (114)
	#define mask_of_reg_dnr_fb_base_0_1 (0x7ff)
	unsigned int reg_dnr_fb_base_0_1:11;

	// h0039, bit: 14
	/* */
	unsigned int :5;

	// h0039
	unsigned int /* padding 16 bit */:16;

	// h003a, bit: 14
	/* DNR frame buffer base address 1*/
	#define offset_of_reg_dnr_fb_base_1 (116)
	#define mask_of_reg_dnr_fb_base_1 (0xffff)
	unsigned int reg_dnr_fb_base_1:16;

	// h003a
	unsigned int /* padding 16 bit */:16;

	// h003b, bit: 10
	/* DNR frame buffer base address 1*/
	#define offset_of_reg_dnr_fb_base_1_1 (118)
	#define mask_of_reg_dnr_fb_base_1_1 (0x7ff)
	unsigned int reg_dnr_fb_base_1_1:11;

	// h003b, bit: 14
	/* */
	unsigned int :5;

	// h003b
	unsigned int /* padding 16 bit */:16;

	// h003c, bit: 3
	/* Pitch width/192*/
	#define offset_of_reg_dnr_fb_pitch_in_tile (120)
	#define mask_of_reg_dnr_fb_pitch_in_tile (0xf)
	unsigned int reg_dnr_fb_pitch_in_tile:4;

	// h003c, bit: 7
	/* DNR write dma miu high priority threshold*/
	#define offset_of_reg_dnr_wdma_pri_thd (120)
	#define mask_of_reg_dnr_wdma_pri_thd (0xf0)
	unsigned int reg_dnr_wdma_pri_thd:4;

	// h003c, bit: 11
	/* DNR read dma miu high priority threshold*/
	#define offset_of_reg_dnr_rdma_pri_thd (120)
	#define mask_of_reg_dnr_rdma_pri_thd (0xf00)
	unsigned int reg_dnr_rdma_pri_thd:4;

	// h003c, bit: 12
	/* DNR read dma miu high priority enable*/
	#define offset_of_reg_dnr_rdma_hpri_en (120)
	#define mask_of_reg_dnr_rdma_hpri_en (0x1000)
	unsigned int reg_dnr_rdma_hpri_en:1;

	// h003c, bit: 14
	/* */
	unsigned int :3;

	// h003c
	unsigned int /* padding 16 bit */:16;

	// h003d, bit: 0
	/* Motion (SAD) enable*/
	#define offset_of_reg_motion_en (122)
	#define mask_of_reg_motion_en (0x1)
	unsigned int reg_motion_en:1;

	// h003d, bit: 4
	/* Motion MIU store burst byte count
	(1count=16bytes, MIU defaule=128 bytes)*/
	#define offset_of_reg_mot_burst_cnt (122)
	#define mask_of_reg_mot_burst_cnt (0x1e)
	unsigned int reg_mot_burst_cnt:4;

	// h003d, bit: 7
	/* */
	unsigned int :3;

	// h003d, bit: 14
	/* Motion threshold*/
	#define offset_of_reg_motion_thrd (122)
	#define mask_of_reg_motion_thrd (0xff00)
	unsigned int reg_motion_thrd:8;

	// h003d
	unsigned int /* padding 16 bit */:16;

	// h003e, bit: 14
	/* Motion frame buffer base address*/
	#define offset_of_reg_mot_fb_base (124)
	#define mask_of_reg_mot_fb_base (0xffff)
	unsigned int reg_mot_fb_base:16;

	// h003e
	unsigned int /* padding 16 bit */:16;

	// h003f, bit: 10
	/* Motion frame buffer base address*/
	#define offset_of_reg_mot_fb_base_1 (126)
	#define mask_of_reg_mot_fb_base_1 (0x7ff)
	unsigned int reg_mot_fb_base_1:11;

	// h003f, bit: 14
	/* */
	unsigned int :5;

	// h003f
	unsigned int /* padding 16 bit */:16;

	// h0040, bit: 14
	/* DNR dma write address upper bound*/
	#define offset_of_reg_dnr_wdma_uaddr (128)
	#define mask_of_reg_dnr_wdma_uaddr (0xffff)
	unsigned int reg_dnr_wdma_uaddr:16;

	// h0040
	unsigned int /* padding 16 bit */:16;

	// h0041, bit: 10
	/* DNR dma write address upper bound*/
	#define offset_of_reg_dnr_wdma_uaddr_1 (130)
	#define mask_of_reg_dnr_wdma_uaddr_1 (0x7ff)
	unsigned int reg_dnr_wdma_uaddr_1:11;

	// h0041, bit: 14
	/* */
	unsigned int :4;

	// h0041, bit: 15
	/* DNR dma write address over upper bound (clear by reg_dnr_en)*/
	#define offset_of_reg_dnr_wdma_adr_over_max (130)
	#define mask_of_reg_dnr_wdma_adr_over_max (0x8000)
	unsigned int reg_dnr_wdma_adr_over_max:1;

	// h0041
	unsigned int /* padding 16 bit */:16;

	// h0042, bit: 1
	/* DNR output frame selection:
	2'd0: Previous frame
	2'd1: Current frame
	2'd2: Debug data*/
	#define offset_of_reg_dnr_ofrm_sel (132)
	#define mask_of_reg_dnr_ofrm_sel (0x3)
	unsigned int reg_dnr_ofrm_sel:2;

	// h0042, bit: 2
	/* ROT output first frame selection:
	1'b0: Black frame
	1'b1: Current frame*/
	#define offset_of_reg_rot_ofrm_sel (132)
	#define mask_of_reg_rot_ofrm_sel (0x4)
	unsigned int reg_rot_ofrm_sel:1;

	// h0042, bit: 14
	/* */
	unsigned int :13;

	// h0042
	unsigned int /* padding 16 bit */:16;

	// h0043, bit: 0
	/* ROT enable*/
	#define offset_of_reg_rot_en (134)
	#define mask_of_reg_rot_en (0x1)
	unsigned int reg_rot_en:1;

	// h0043, bit: 4
	/* ROT output rate control */
	#define offset_of_reg_rot_rate_cyc (134)
	#define mask_of_reg_rot_rate_cyc (0x1e)
	unsigned int reg_rot_rate_cyc:4;

	// h0043, bit: 14
	/* */
	unsigned int :11;

	// h0043
	unsigned int /* padding 16 bit */:16;

	// h0044, bit: 14
	/* dnr wdma status*/
	#define offset_of_reg_dnr_wdma_status (136)
	#define mask_of_reg_dnr_wdma_status (0xffff)
	unsigned int reg_dnr_wdma_status:16;

	// h0044
	unsigned int /* padding 16 bit */:16;

	// h0045, bit: 14
	/* dnr wdma status*/
	#define offset_of_reg_dnr_rdma_status (138)
	#define mask_of_reg_dnr_rdma_status (0xffff)
	unsigned int reg_dnr_rdma_status:16;

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

	// h0050, bit: 7
	/* */
	unsigned int :8;

	// h0050, bit: 14
	/* AWB block row number for interrupt*/
	#define offset_of_reg_awb_int_row_num (160)
	#define mask_of_reg_awb_int_row_num (0x7f00)
	unsigned int reg_awb_int_row_num:7;

	// h0050, bit: 15
	/* */
	unsigned int :1;

	// h0050
	unsigned int /* padding 16 bit */:16;

	// h0051, bit: 9
	/* AWB block width minus one*/
	#define offset_of_reg_awb_blksize_x_m1 (162)
	#define mask_of_reg_awb_blksize_x_m1 (0x3ff)
	unsigned int reg_awb_blksize_x_m1:10;

	// h0051, bit: 14
	/* */
	unsigned int :6;

	// h0051
	unsigned int /* padding 16 bit */:16;

	// h0052, bit: 9
	/* AWB block height minus one*/
	#define offset_of_reg_awb_blksize_y_m1 (164)
	#define mask_of_reg_awb_blksize_y_m1 (0x3ff)
	unsigned int reg_awb_blksize_y_m1:10;

	// h0052, bit: 14
	/* */
	unsigned int :6;

	// h0052
	unsigned int /* padding 16 bit */:16;

	// h0053, bit: 6
	/* AWB horizontal block number minus one*/
	#define offset_of_reg_awb_blknum_x_m1 (166)
	#define mask_of_reg_awb_blknum_x_m1 (0x7f)
	unsigned int reg_awb_blknum_x_m1:7;

	// h0053, bit: 7
	/* */
	unsigned int :1;

	// h0053, bit: 14
	/* AWB vertical block number minus one*/
	#define offset_of_reg_awb_blknum_y_m1 (166)
	#define mask_of_reg_awb_blknum_y_m1 (0x7f00)
	unsigned int reg_awb_blknum_y_m1:7;

	// h0053, bit: 15
	/* */
	unsigned int :1;

	// h0053
	unsigned int /* padding 16 bit */:16;

	// h0054, bit: 11
	/* AWB horizontal block pixel offset*/
	#define offset_of_reg_awb_blk_h_offset (168)
	#define mask_of_reg_awb_blk_h_offset (0xfff)
	unsigned int reg_awb_blk_h_offset:12;

	// h0054, bit: 14
	/* */
	unsigned int :4;

	// h0054
	unsigned int /* padding 16 bit */:16;

	// h0055, bit: 11
	/* AWB veritcal block pixel offset*/
	#define offset_of_reg_awb_blk_v_offset (170)
	#define mask_of_reg_awb_blk_v_offset (0xfff)
	unsigned int reg_awb_blk_v_offset:12;

	// h0055, bit: 14
	/* */
	unsigned int :4;

	// h0055
	unsigned int /* padding 16 bit */:16;

	// h0056, bit: 9
	/* AWB horizontal block pixel valid minus one*/
	#define offset_of_reg_awb_blk_valid_x_m1 (172)
	#define mask_of_reg_awb_blk_valid_x_m1 (0x3ff)
	unsigned int reg_awb_blk_valid_x_m1:10;

	// h0056, bit: 14
	/* */
	unsigned int :6;

	// h0056
	unsigned int /* padding 16 bit */:16;

	// h0057, bit: 9
	/* AWB vertical block pixel valid minus one*/
	#define offset_of_reg_awb_blk_valid_y_m1 (174)
	#define mask_of_reg_awb_blk_valid_y_m1 (0x3ff)
	unsigned int reg_awb_blk_valid_y_m1:10;

	// h0057, bit: 14
	/* */
	unsigned int :6;

	// h0057
	unsigned int /* padding 16 bit */:16;

	// h0058, bit: 14
	/* AWB block average divisor A minus one*/
	#define offset_of_reg_awb_div_a_m1 (176)
	#define mask_of_reg_awb_div_a_m1 (0xffff)
	unsigned int reg_awb_div_a_m1:16;

	// h0058
	unsigned int /* padding 16 bit */:16;

	// h0059, bit: 14
	/* AWB block average divisor B minus one*/
	#define offset_of_reg_awb_div_b_m1 (178)
	#define mask_of_reg_awb_div_b_m1 (0xffff)
	unsigned int reg_awb_div_b_m1:16;

	// h0059
	unsigned int /* padding 16 bit */:16;

	// h005a, bit: 14
	/* AWB block average divisor C minus one*/
	#define offset_of_reg_awb_div_c_m1 (180)
	#define mask_of_reg_awb_div_c_m1 (0xffff)
	unsigned int reg_awb_div_c_m1:16;

	// h005a
	unsigned int /* padding 16 bit */:16;

	// h005b, bit: 14
	/* AWB block average divisor D minus one*/
	#define offset_of_reg_awb_div_d_m1 (182)
	#define mask_of_reg_awb_div_d_m1 (0xffff)
	unsigned int reg_awb_div_d_m1:16;

	// h005b
	unsigned int /* padding 16 bit */:16;

	// h005c, bit: 14
	/* AWB block average divisor E minus one*/
	#define offset_of_reg_awb_div_e_m1 (184)
	#define mask_of_reg_awb_div_e_m1 (0xffff)
	unsigned int reg_awb_div_e_m1:16;

	// h005c
	unsigned int /* padding 16 bit */:16;

	// h005d, bit: 14
	/* AWB block average divisor F minus one*/
	#define offset_of_reg_awb_div_f_m1 (186)
	#define mask_of_reg_awb_div_f_m1 (0xffff)
	unsigned int reg_awb_div_f_m1:16;

	// h005d
	unsigned int /* padding 16 bit */:16;

	// h005e, bit: 3
	/* AWB accumulation shift register*/
	#define offset_of_reg_awb_acc_sft (188)
	#define mask_of_reg_awb_acc_sft (0xf)
	unsigned int reg_awb_acc_sft:4;

	// h005e, bit: 14
	/* */
	unsigned int :12;

	// h005e
	unsigned int /* padding 16 bit */:16;

	// h005f, bit: 14
	/* */
	unsigned int :16;

	// h005f
	unsigned int /* padding 16 bit */:16;

	// h0060, bit: 1
	/* */
	unsigned int :2;

	// h0060, bit: 2
	/* Window0 histogram enable*/
	#define offset_of_reg_histo_win0_en (192)
	#define mask_of_reg_histo_win0_en (0x4)
	unsigned int reg_histo_win0_en:1;

	// h0060, bit: 3
	/* Window1 histogram enable*/
	#define offset_of_reg_histo_win1_en (192)
	#define mask_of_reg_histo_win1_en (0x8)
	unsigned int reg_histo_win1_en:1;

	// h0060, bit: 7
	/* */
	unsigned int :4;

	// h0060, bit: 14
	/* Histogram block row number for interrupt*/
	#define offset_of_reg_histo_int_row_num (192)
	#define mask_of_reg_histo_int_row_num (0x7f00)
	unsigned int reg_histo_int_row_num:7;

	// h0060, bit: 15
	/* */
	unsigned int :1;

	// h0060
	unsigned int /* padding 16 bit */:16;

	// h0061, bit: 9
	/* Histogram block width minus one*/
	#define offset_of_reg_histo_blksize_x_m1 (194)
	#define mask_of_reg_histo_blksize_x_m1 (0x3ff)
	unsigned int reg_histo_blksize_x_m1:10;

	// h0061, bit: 14
	/* */
	unsigned int :6;

	// h0061
	unsigned int /* padding 16 bit */:16;

	// h0062, bit: 9
	/* Histogram block height minus one*/
	#define offset_of_reg_histo_blksize_y_m1 (196)
	#define mask_of_reg_histo_blksize_y_m1 (0x3ff)
	unsigned int reg_histo_blksize_y_m1:10;

	// h0062, bit: 14
	/* */
	unsigned int :6;

	// h0062
	unsigned int /* padding 16 bit */:16;

	// h0063, bit: 6
	/* Histogram horizontal block number minus one*/
	#define offset_of_reg_histo_blknum_x_m1 (198)
	#define mask_of_reg_histo_blknum_x_m1 (0x7f)
	unsigned int reg_histo_blknum_x_m1:7;

	// h0063, bit: 7
	/* */
	unsigned int :1;

	// h0063, bit: 14
	/* Histogram vertical block number minus one*/
	#define offset_of_reg_histo_blknum_y_m1 (198)
	#define mask_of_reg_histo_blknum_y_m1 (0x7f00)
	unsigned int reg_histo_blknum_y_m1:7;

	// h0063, bit: 15
	/* */
	unsigned int :1;

	// h0063
	unsigned int /* padding 16 bit */:16;

	// h0064, bit: 11
	/* Histogram horizontal block pixel offset*/
	#define offset_of_reg_histo_blk_h_offset (200)
	#define mask_of_reg_histo_blk_h_offset (0xfff)
	unsigned int reg_histo_blk_h_offset:12;

	// h0064, bit: 14
	/* */
	unsigned int :4;

	// h0064
	unsigned int /* padding 16 bit */:16;

	// h0065, bit: 11
	/* Histogram veritcal block pixel offset*/
	#define offset_of_reg_histo_blk_v_offset (202)
	#define mask_of_reg_histo_blk_v_offset (0xfff)
	unsigned int reg_histo_blk_v_offset:12;

	// h0065, bit: 14
	/* */
	unsigned int :4;

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
	/* Histogram block average divisor A minus one*/
	#define offset_of_reg_histo_div_a_m1 (208)
	#define mask_of_reg_histo_div_a_m1 (0xffff)
	unsigned int reg_histo_div_a_m1:16;

	// h0068
	unsigned int /* padding 16 bit */:16;

	// h0069, bit: 14
	/* Histogram block average divisor B minus one*/
	#define offset_of_reg_histo_div_b_m1 (210)
	#define mask_of_reg_histo_div_b_m1 (0xffff)
	unsigned int reg_histo_div_b_m1:16;

	// h0069
	unsigned int /* padding 16 bit */:16;

	// h006a, bit: 14
	/* Histogram block average divisor C minus one*/
	#define offset_of_reg_histo_div_c_m1 (212)
	#define mask_of_reg_histo_div_c_m1 (0xffff)
	unsigned int reg_histo_div_c_m1:16;

	// h006a
	unsigned int /* padding 16 bit */:16;

	// h006b, bit: 14
	/* Histogram block average divisor D minus one*/
	#define offset_of_reg_histo_div_d_m1 (214)
	#define mask_of_reg_histo_div_d_m1 (0xffff)
	unsigned int reg_histo_div_d_m1:16;

	// h006b
	unsigned int /* padding 16 bit */:16;

	// h006c, bit: 14
	/* Histogram block average divisor E minus one*/
	#define offset_of_reg_histo_div_e_m1 (216)
	#define mask_of_reg_histo_div_e_m1 (0xffff)
	unsigned int reg_histo_div_e_m1:16;

	// h006c
	unsigned int /* padding 16 bit */:16;

	// h006d, bit: 14
	/* Histogram block average divisor F minus one*/
	#define offset_of_reg_histo_div_f_m1 (218)
	#define mask_of_reg_histo_div_f_m1 (0xffff)
	unsigned int reg_histo_div_f_m1:16;

	// h006d
	unsigned int /* padding 16 bit */:16;

	// h006e, bit: 3
	/* Histogram accumulation shift register*/
	#define offset_of_reg_histo_acc_sft (220)
	#define mask_of_reg_histo_acc_sft (0xf)
	unsigned int reg_histo_acc_sft:4;

	// h006e, bit: 14
	/* */
	unsigned int :12;

	// h006e
	unsigned int /* padding 16 bit */:16;

	// h006f, bit: 14
	/* */
	unsigned int :16;

	// h006f
	unsigned int /* padding 16 bit */:16;

	// h0070, bit: 6
	/* Horizontal block offset of histogram window0*/
	#define offset_of_reg_histo_stawin_x_offset_0 (224)
	#define mask_of_reg_histo_stawin_x_offset_0 (0x7f)
	unsigned int reg_histo_stawin_x_offset_0:7;

	// h0070, bit: 7
	/* */
	unsigned int :1;

	// h0070, bit: 14
	/* Horizontal block number minus one of histogram window0*/
	#define offset_of_reg_histo_stawin_x_size_m1_0 (224)
	#define mask_of_reg_histo_stawin_x_size_m1_0 (0x7f00)
	unsigned int reg_histo_stawin_x_size_m1_0:7;

	// h0070, bit: 15
	/* */
	unsigned int :1;

	// h0070
	unsigned int /* padding 16 bit */:16;

	// h0071, bit: 6
	/* Vertical block offset of histogram window0*/
	#define offset_of_reg_histo_stawin_y_offset_0 (226)
	#define mask_of_reg_histo_stawin_y_offset_0 (0x7f)
	unsigned int reg_histo_stawin_y_offset_0:7;

	// h0071, bit: 7
	/* */
	unsigned int :1;

	// h0071, bit: 14
	/* Vertical block number minus one of histogram window0*/
	#define offset_of_reg_histo_stawin_y_size_m1_0 (226)
	#define mask_of_reg_histo_stawin_y_size_m1_0 (0x7f00)
	unsigned int reg_histo_stawin_y_size_m1_0:7;

	// h0071, bit: 15
	/* */
	unsigned int :1;

	// h0071
	unsigned int /* padding 16 bit */:16;

	// h0072, bit: 6
	/* Horizontal block offset of histogram window1*/
	#define offset_of_reg_histo_stawin_x_offset_1 (228)
	#define mask_of_reg_histo_stawin_x_offset_1 (0x7f)
	unsigned int reg_histo_stawin_x_offset_1:7;

	// h0072, bit: 7
	/* */
	unsigned int :1;

	// h0072, bit: 14
	/* Horizontal block number minus one of histogram window1*/
	#define offset_of_reg_histo_stawin_x_size_m1_1 (228)
	#define mask_of_reg_histo_stawin_x_size_m1_1 (0x7f00)
	unsigned int reg_histo_stawin_x_size_m1_1:7;

	// h0072, bit: 15
	/* */
	unsigned int :1;

	// h0072
	unsigned int /* padding 16 bit */:16;

	// h0073, bit: 6
	/* Vertical block offset of histogram window1*/
	#define offset_of_reg_histo_stawin_y_offset_1 (230)
	#define mask_of_reg_histo_stawin_y_offset_1 (0x7f)
	unsigned int reg_histo_stawin_y_offset_1:7;

	// h0073, bit: 7
	/* */
	unsigned int :1;

	// h0073, bit: 14
	/* Vertical block number minus one of histogram window1*/
	#define offset_of_reg_histo_stawin_y_size_m1_1 (230)
	#define mask_of_reg_histo_stawin_y_size_m1_1 (0x7f00)
	unsigned int reg_histo_stawin_y_size_m1_1:7;

	// h0073, bit: 15
	/* */
	unsigned int :1;

	// h0073
	unsigned int /* padding 16 bit */:16;

	// h0074, bit: 0
	/* Histogram window0 roi mode*/
	#define offset_of_reg_histo_roi_0_mode (232)
	#define mask_of_reg_histo_roi_0_mode (0x1)
	unsigned int reg_histo_roi_0_mode:1;

	// h0074, bit: 1
	/* Histogram window1 roi mode*/
	#define offset_of_reg_histo_roi_1_mode (232)
	#define mask_of_reg_histo_roi_1_mode (0x2)
	unsigned int reg_histo_roi_1_mode:1;

	// h0074, bit: 14
	/* */
	unsigned int :14;

	// h0074
	unsigned int /* padding 16 bit */:16;

	// h0075, bit: 3
	/* AE source selection
	4'b0000: from wb pre-gain
	4'b0001: from wb pre-gain multiply LS gain
	4'b0010: from wb pre-gain multiply CS gain
	4'b0011: from wb pre-gain multiply LS and CS gain
	4'b0100: from wb pre-gain multiply post-WB gain
	4'b0101: from wb pre-gain multiply LS and post-WB gain
	4'b0110: from wb pre-gain multiply CS and post-WB gain
	4'b0111: from wb pre-gain multiply LS, CS and post-WB
	gain
	4'b1000: from CS output
	4'b1100: from WB post-gain output*/
	#define offset_of_reg_isp_ae_in_mode (234)
	#define mask_of_reg_isp_ae_in_mode (0xf)
	unsigned int reg_isp_ae_in_mode:4;

	// h0075, bit: 7
	/* AWB source selection
	4'b0000: from wb pre-gain
	4'b0001: from wb pre-gain multiply LS gain
	4'b0010: from wb pre-gain multiply CS gain
	4'b0011: from wb pre-gain multiply LS and CS gain
	4'b1000: from CS output*/
	#define offset_of_reg_isp_awb_in_mode (234)
	#define mask_of_reg_isp_awb_in_mode (0xf0)
	unsigned int reg_isp_awb_in_mode:4;

	// h0075, bit: 14
	/* */
	unsigned int :8;

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

	// h0078, bit: 14
	/* */
	unsigned int :16;

	// h0078
	unsigned int /* padding 16 bit */:16;

	// h0079, bit: 14
	/* */
	unsigned int :16;

	// h0079
	unsigned int /* padding 16 bit */:16;

	// h007a, bit: 14
	/* */
	unsigned int :16;

	// h007a
	unsigned int /* padding 16 bit */:16;

	// h007b, bit: 14
	/* */
	unsigned int :16;

	// h007b
	unsigned int /* padding 16 bit */:16;

	// h007c, bit: 14
	/* */
	unsigned int :16;

	// h007c
	unsigned int /* padding 16 bit */:16;

	// h007d, bit: 14
	/* */
	unsigned int :16;

	// h007d
	unsigned int /* padding 16 bit */:16;

	// h007e, bit: 14
	/* */
	unsigned int :16;

	// h007e
	unsigned int /* padding 16 bit */:16;

	// h007f, bit: 14
	/* */
	unsigned int :16;

	// h007f
	unsigned int /* padding 16 bit */:16;

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp3;
#endif
