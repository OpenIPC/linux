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
#ifndef __INFINITY3E_REG_ISP9__
#define __INFINITY3E_REG_ISP9__
typedef struct {
	// h0000, bit: 0
	/* HDR 16to12 gain  enable*/
	#define offset_of_reg_hdr_16to12_gain_en (0)
	#define mask_of_reg_hdr_16to12_gain_en (0x1)
	unsigned int reg_hdr_16to12_gain_en:1;

	// h0000, bit: 1
	/* HDR 16to12 gain smooth enable*/
	#define offset_of_reg_hdr_16to12_gbl_smooth_en (0)
	#define mask_of_reg_hdr_16to12_gbl_smooth_en (0x2)
	unsigned int reg_hdr_16to12_gbl_smooth_en:1;

	// h0000, bit: 5
	/* HDR 16to12 gain base*/
	#define offset_of_reg_hdr_16to12_gain_base (0)
	#define mask_of_reg_hdr_16to12_gain_base (0x3c)
	unsigned int reg_hdr_16to12_gain_base:4;

	// h0000, bit: 14
	/* */
	unsigned int :10;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 14
	/* HDR 16to12 gain low bound*/
	#define offset_of_reg_hdr_16to12_gain_lw (2)
	#define mask_of_reg_hdr_16to12_gain_lw (0xffff)
	unsigned int reg_hdr_16to12_gain_lw:16;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 14
	/* HDR 16to12 gain high bound*/
	#define offset_of_reg_hdr_16to12_gain_hi (4)
	#define mask_of_reg_hdr_16to12_gain_hi (0xffff)
	unsigned int reg_hdr_16to12_gain_hi:16;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 14
	/* HDR 16to12 offset for R*/
	#define offset_of_reg_hdr_16to12_offset_r (6)
	#define mask_of_reg_hdr_16to12_offset_r (0xffff)
	unsigned int reg_hdr_16to12_offset_r:16;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 14
	/* HDR 16to12 offset for G*/
	#define offset_of_reg_hdr_16to12_offset_g (8)
	#define mask_of_reg_hdr_16to12_offset_g (0xffff)
	unsigned int reg_hdr_16to12_offset_g:16;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 14
	/* HDR 16to12 offset for B*/
	#define offset_of_reg_hdr_16to12_offset_b (10)
	#define mask_of_reg_hdr_16to12_offset_b (0xffff)
	unsigned int reg_hdr_16to12_offset_b:16;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 3
	/* HDR 16to12 curve x 0*/
	#define offset_of_reg_hdr_16to12_curve_x_0 (12)
	#define mask_of_reg_hdr_16to12_curve_x_0 (0xf)
	unsigned int reg_hdr_16to12_curve_x_0:4;

	// h0006, bit: 7
	/* HDR 16to12 curve x 1*/
	#define offset_of_reg_hdr_16to12_curve_x_1 (12)
	#define mask_of_reg_hdr_16to12_curve_x_1 (0xf0)
	unsigned int reg_hdr_16to12_curve_x_1:4;

	// h0006, bit: 11
	/* HDR 16to12 curve x 2*/
	#define offset_of_reg_hdr_16to12_curve_x_2 (12)
	#define mask_of_reg_hdr_16to12_curve_x_2 (0xf00)
	unsigned int reg_hdr_16to12_curve_x_2:4;

	// h0006, bit: 14
	/* HDR 16to12 curve x 3*/
	#define offset_of_reg_hdr_16to12_curve_x_3 (12)
	#define mask_of_reg_hdr_16to12_curve_x_3 (0xf000)
	unsigned int reg_hdr_16to12_curve_x_3:4;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 3
	/* HDR 16to12 curve x 4*/
	#define offset_of_reg_hdr_16to12_curve_x_4 (14)
	#define mask_of_reg_hdr_16to12_curve_x_4 (0xf)
	unsigned int reg_hdr_16to12_curve_x_4:4;

	// h0007, bit: 7
	/* HDR 16to12 curve x 5*/
	#define offset_of_reg_hdr_16to12_curve_x_5 (14)
	#define mask_of_reg_hdr_16to12_curve_x_5 (0xf0)
	unsigned int reg_hdr_16to12_curve_x_5:4;

	// h0007, bit: 11
	/* HDR 16to12 curve x 6*/
	#define offset_of_reg_hdr_16to12_curve_x_6 (14)
	#define mask_of_reg_hdr_16to12_curve_x_6 (0xf00)
	unsigned int reg_hdr_16to12_curve_x_6:4;

	// h0007, bit: 14
	/* HDR 16to12 curve x 7*/
	#define offset_of_reg_hdr_16to12_curve_x_7 (14)
	#define mask_of_reg_hdr_16to12_curve_x_7 (0xf000)
	unsigned int reg_hdr_16to12_curve_x_7:4;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 3
	/* HDR 16to12 curve x 8*/
	#define offset_of_reg_hdr_16to12_curve_x_8 (16)
	#define mask_of_reg_hdr_16to12_curve_x_8 (0xf)
	unsigned int reg_hdr_16to12_curve_x_8:4;

	// h0008, bit: 7
	/* HDR 16to12 curve x 9*/
	#define offset_of_reg_hdr_16to12_curve_x_9 (16)
	#define mask_of_reg_hdr_16to12_curve_x_9 (0xf0)
	unsigned int reg_hdr_16to12_curve_x_9:4;

	// h0008, bit: 11
	/* HDR 16to12 curve x 10*/
	#define offset_of_reg_hdr_16to12_curve_x_10 (16)
	#define mask_of_reg_hdr_16to12_curve_x_10 (0xf00)
	unsigned int reg_hdr_16to12_curve_x_10:4;

	// h0008, bit: 14
	/* HDR 16to12 curve x 11*/
	#define offset_of_reg_hdr_16to12_curve_x_11 (16)
	#define mask_of_reg_hdr_16to12_curve_x_11 (0xf000)
	unsigned int reg_hdr_16to12_curve_x_11:4;

	// h0008
	unsigned int /* padding 16 bit */:16;

	// h0009, bit: 3
	/* HDR 16to12 curve x 12*/
	#define offset_of_reg_hdr_16to12_curve_x_12 (18)
	#define mask_of_reg_hdr_16to12_curve_x_12 (0xf)
	unsigned int reg_hdr_16to12_curve_x_12:4;

	// h0009, bit: 7
	/* HDR 16to12 curve x 13*/
	#define offset_of_reg_hdr_16to12_curve_x_13 (18)
	#define mask_of_reg_hdr_16to12_curve_x_13 (0xf0)
	unsigned int reg_hdr_16to12_curve_x_13:4;

	// h0009, bit: 11
	/* HDR 16to12 curve x 14*/
	#define offset_of_reg_hdr_16to12_curve_x_14 (18)
	#define mask_of_reg_hdr_16to12_curve_x_14 (0xf00)
	unsigned int reg_hdr_16to12_curve_x_14:4;

	// h0009, bit: 14
	/* HDR 16to12 curve x 15*/
	#define offset_of_reg_hdr_16to12_curve_x_15 (18)
	#define mask_of_reg_hdr_16to12_curve_x_15 (0xf000)
	unsigned int reg_hdr_16to12_curve_x_15:4;

	// h0009
	unsigned int /* padding 16 bit */:16;

	// h000a, bit: 3
	/* HDR 16to12 curve x 16*/
	#define offset_of_reg_hdr_16to12_curve_x_16 (20)
	#define mask_of_reg_hdr_16to12_curve_x_16 (0xf)
	unsigned int reg_hdr_16to12_curve_x_16:4;

	// h000a, bit: 7
	/* HDR 16to12 curve x 17*/
	#define offset_of_reg_hdr_16to12_curve_x_17 (20)
	#define mask_of_reg_hdr_16to12_curve_x_17 (0xf0)
	unsigned int reg_hdr_16to12_curve_x_17:4;

	// h000a, bit: 11
	/* HDR 16to12 curve x 18*/
	#define offset_of_reg_hdr_16to12_curve_x_18 (20)
	#define mask_of_reg_hdr_16to12_curve_x_18 (0xf00)
	unsigned int reg_hdr_16to12_curve_x_18:4;

	// h000a, bit: 14
	/* HDR 16to12 curve x 19*/
	#define offset_of_reg_hdr_16to12_curve_x_19 (20)
	#define mask_of_reg_hdr_16to12_curve_x_19 (0xf000)
	unsigned int reg_hdr_16to12_curve_x_19:4;

	// h000a
	unsigned int /* padding 16 bit */:16;

	// h000b, bit: 14
	/* HDR 16to12 curve 0*/
	#define offset_of_reg_hdr_16to12_curve_0 (22)
	#define mask_of_reg_hdr_16to12_curve_0 (0xffff)
	unsigned int reg_hdr_16to12_curve_0:16;

	// h000b
	unsigned int /* padding 16 bit */:16;

	// h000c, bit: 14
	/* HDR 16to12 curve 1*/
	#define offset_of_reg_hdr_16to12_curve_1 (24)
	#define mask_of_reg_hdr_16to12_curve_1 (0xffff)
	unsigned int reg_hdr_16to12_curve_1:16;

	// h000c
	unsigned int /* padding 16 bit */:16;

	// h000d, bit: 14
	/* HDR 16to12 curve 2*/
	#define offset_of_reg_hdr_16to12_curve_2 (26)
	#define mask_of_reg_hdr_16to12_curve_2 (0xffff)
	unsigned int reg_hdr_16to12_curve_2:16;

	// h000d
	unsigned int /* padding 16 bit */:16;

	// h000e, bit: 14
	/* HDR 16to12 curve 3*/
	#define offset_of_reg_hdr_16to12_curve_3 (28)
	#define mask_of_reg_hdr_16to12_curve_3 (0xffff)
	unsigned int reg_hdr_16to12_curve_3:16;

	// h000e
	unsigned int /* padding 16 bit */:16;

	// h000f, bit: 14
	/* HDR 16to12 curve 4*/
	#define offset_of_reg_hdr_16to12_curve_4 (30)
	#define mask_of_reg_hdr_16to12_curve_4 (0xffff)
	unsigned int reg_hdr_16to12_curve_4:16;

	// h000f
	unsigned int /* padding 16 bit */:16;

	// h0010, bit: 14
	/* HDR 16to12 curve 5*/
	#define offset_of_reg_hdr_16to12_curve_5 (32)
	#define mask_of_reg_hdr_16to12_curve_5 (0xffff)
	unsigned int reg_hdr_16to12_curve_5:16;

	// h0010
	unsigned int /* padding 16 bit */:16;

	// h0011, bit: 14
	/* HDR 16to12 curve 6*/
	#define offset_of_reg_hdr_16to12_curve_6 (34)
	#define mask_of_reg_hdr_16to12_curve_6 (0xffff)
	unsigned int reg_hdr_16to12_curve_6:16;

	// h0011
	unsigned int /* padding 16 bit */:16;

	// h0012, bit: 14
	/* HDR 16to12 curve 7*/
	#define offset_of_reg_hdr_16to12_curve_7 (36)
	#define mask_of_reg_hdr_16to12_curve_7 (0xffff)
	unsigned int reg_hdr_16to12_curve_7:16;

	// h0012
	unsigned int /* padding 16 bit */:16;

	// h0013, bit: 14
	/* HDR 16to12 curve 8*/
	#define offset_of_reg_hdr_16to12_curve_8 (38)
	#define mask_of_reg_hdr_16to12_curve_8 (0xffff)
	unsigned int reg_hdr_16to12_curve_8:16;

	// h0013
	unsigned int /* padding 16 bit */:16;

	// h0014, bit: 14
	/* HDR 16to12 curve 9*/
	#define offset_of_reg_hdr_16to12_curve_9 (40)
	#define mask_of_reg_hdr_16to12_curve_9 (0xffff)
	unsigned int reg_hdr_16to12_curve_9:16;

	// h0014
	unsigned int /* padding 16 bit */:16;

	// h0015, bit: 14
	/* HDR 16to12 curve 10*/
	#define offset_of_reg_hdr_16to12_curve_10 (42)
	#define mask_of_reg_hdr_16to12_curve_10 (0xffff)
	unsigned int reg_hdr_16to12_curve_10:16;

	// h0015
	unsigned int /* padding 16 bit */:16;

	// h0016, bit: 14
	/* HDR 16to12 curve 11*/
	#define offset_of_reg_hdr_16to12_curve_11 (44)
	#define mask_of_reg_hdr_16to12_curve_11 (0xffff)
	unsigned int reg_hdr_16to12_curve_11:16;

	// h0016
	unsigned int /* padding 16 bit */:16;

	// h0017, bit: 14
	/* HDR 16to12 curve 12*/
	#define offset_of_reg_hdr_16to12_curve_12 (46)
	#define mask_of_reg_hdr_16to12_curve_12 (0xffff)
	unsigned int reg_hdr_16to12_curve_12:16;

	// h0017
	unsigned int /* padding 16 bit */:16;

	// h0018, bit: 14
	/* HDR 16to12 curve 13*/
	#define offset_of_reg_hdr_16to12_curve_13 (48)
	#define mask_of_reg_hdr_16to12_curve_13 (0xffff)
	unsigned int reg_hdr_16to12_curve_13:16;

	// h0018
	unsigned int /* padding 16 bit */:16;

	// h0019, bit: 14
	/* HDR 16to12 curve 14*/
	#define offset_of_reg_hdr_16to12_curve_14 (50)
	#define mask_of_reg_hdr_16to12_curve_14 (0xffff)
	unsigned int reg_hdr_16to12_curve_14:16;

	// h0019
	unsigned int /* padding 16 bit */:16;

	// h001a, bit: 14
	/* HDR 16to12 curve 15*/
	#define offset_of_reg_hdr_16to12_curve_15 (52)
	#define mask_of_reg_hdr_16to12_curve_15 (0xffff)
	unsigned int reg_hdr_16to12_curve_15:16;

	// h001a
	unsigned int /* padding 16 bit */:16;

	// h001b, bit: 14
	/* HDR 16to12 curve 16*/
	#define offset_of_reg_hdr_16to12_curve_16 (54)
	#define mask_of_reg_hdr_16to12_curve_16 (0xffff)
	unsigned int reg_hdr_16to12_curve_16:16;

	// h001b
	unsigned int /* padding 16 bit */:16;

	// h001c, bit: 14
	/* HDR 16to12 curve 17*/
	#define offset_of_reg_hdr_16to12_curve_17 (56)
	#define mask_of_reg_hdr_16to12_curve_17 (0xffff)
	unsigned int reg_hdr_16to12_curve_17:16;

	// h001c
	unsigned int /* padding 16 bit */:16;

	// h001d, bit: 14
	/* HDR 16to12 curve 18*/
	#define offset_of_reg_hdr_16to12_curve_18 (58)
	#define mask_of_reg_hdr_16to12_curve_18 (0xffff)
	unsigned int reg_hdr_16to12_curve_18:16;

	// h001d
	unsigned int /* padding 16 bit */:16;

	// h001e, bit: 14
	/* HDR 16to12 curve 19*/
	#define offset_of_reg_hdr_16to12_curve_19 (60)
	#define mask_of_reg_hdr_16to12_curve_19 (0xffff)
	unsigned int reg_hdr_16to12_curve_19:16;

	// h001e
	unsigned int /* padding 16 bit */:16;

	// h001f, bit: 14
	/* HDR 16to12 curve 20*/
	#define offset_of_reg_hdr_16to12_curve_20 (62)
	#define mask_of_reg_hdr_16to12_curve_20 (0xffff)
	unsigned int reg_hdr_16to12_curve_20:16;

	// h001f
	unsigned int /* padding 16 bit */:16;

	// h0020, bit: 0
	/* Histogram enable*/
	#define offset_of_reg_hdr_16to12_hist_en (64)
	#define mask_of_reg_hdr_16to12_hist_en (0x1)
	unsigned int reg_hdr_16to12_hist_en:1;

	// h0020, bit: 14
	/* */
	unsigned int :15;

	// h0020
	unsigned int /* padding 16 bit */:16;

	// h0021, bit: 11
	/* Histogram block offsetx*/
	#define offset_of_reg_hdr_16to12_hist_offsetx (66)
	#define mask_of_reg_hdr_16to12_hist_offsetx (0xfff)
	unsigned int reg_hdr_16to12_hist_offsetx:12;

	// h0021, bit: 14
	/* */
	unsigned int :4;

	// h0021
	unsigned int /* padding 16 bit */:16;

	// h0022, bit: 11
	/* Histogram block offsety*/
	#define offset_of_reg_hdr_16to12_hist_offsety (68)
	#define mask_of_reg_hdr_16to12_hist_offsety (0xfff)
	unsigned int reg_hdr_16to12_hist_offsety:12;

	// h0022, bit: 14
	/* */
	unsigned int :4;

	// h0022
	unsigned int /* padding 16 bit */:16;

	// h0023, bit: 7
	/* Block Width= (reg_hist_blk_width + 1)*/
	#define offset_of_reg_hdr_16to12_hist_blk_width (70)
	#define mask_of_reg_hdr_16to12_hist_blk_width (0xff)
	unsigned int reg_hdr_16to12_hist_blk_width:8;

	// h0023, bit: 14
	/* Block Height= (reg_hist_blk_height + 1)*/
	#define offset_of_reg_hdr_16to12_hist_blk_height (70)
	#define mask_of_reg_hdr_16to12_hist_blk_height (0xff00)
	unsigned int reg_hdr_16to12_hist_blk_height:8;

	// h0023
	unsigned int /* padding 16 bit */:16;

	// h0024, bit: 7
	/* ROI offsetx (unit: ROI)*/
	#define offset_of_reg_hdr_16to12_roi_offsetx (72)
	#define mask_of_reg_hdr_16to12_roi_offsetx (0xff)
	unsigned int reg_hdr_16to12_roi_offsetx:8;

	// h0024, bit: 14
	/* ROI offsety (unit: ROI)*/
	#define offset_of_reg_hdr_16to12_roi_offsety (72)
	#define mask_of_reg_hdr_16to12_roi_offsety (0xff00)
	unsigned int reg_hdr_16to12_roi_offsety:8;

	// h0024
	unsigned int /* padding 16 bit */:16;

	// h0025, bit: 6
	/* ROI X size = (reg_roi_sizex + 1) ROI*/
	#define offset_of_reg_hdr_16to12_roi_sizex (74)
	#define mask_of_reg_hdr_16to12_roi_sizex (0x7f)
	unsigned int reg_hdr_16to12_roi_sizex:7;

	// h0025, bit: 7
	/* */
	unsigned int :1;

	// h0025, bit: 14
	/* ROI Y size = (reg_roi_sizey + 1) ROI*/
	#define offset_of_reg_hdr_16to12_roi_sizey (74)
	#define mask_of_reg_hdr_16to12_roi_sizey (0xff00)
	unsigned int reg_hdr_16to12_roi_sizey:8;

	// h0025
	unsigned int /* padding 16 bit */:16;

	// h0026, bit: 14
	/* Hist = (sum_ir * reg_roi_div) >> (16+4)*/
	#define offset_of_reg_hdr_16to12_roi_div (76)
	#define mask_of_reg_hdr_16to12_roi_div (0xffff)
	unsigned int reg_hdr_16to12_roi_div:16;

	// h0026
	unsigned int /* padding 16 bit */:16;

	// h0027, bit: 14
	/* MIU Base address (total 256 bytes)*/
	#define offset_of_reg_hdr_16to12_miu_base_addr (78)
	#define mask_of_reg_hdr_16to12_miu_base_addr (0xffff)
	unsigned int reg_hdr_16to12_miu_base_addr:16;

	// h0027
	unsigned int /* padding 16 bit */:16;

	// h0028, bit: 14
	/* MIU Base address (total 256 bytes)*/
	#define offset_of_reg_hdr_16to12_miu_base_addr_1 (80)
	#define mask_of_reg_hdr_16to12_miu_base_addr_1 (0xffff)
	unsigned int reg_hdr_16to12_miu_base_addr_1:16;

	// h0028
	unsigned int /* padding 16 bit */:16;

	// h0029, bit: 1
	/* Histogram arbiter mi lenx4*/
	#define offset_of_reg_hdr_16to12_mi_lenx4 (82)
	#define mask_of_reg_hdr_16to12_mi_lenx4 (0x3)
	unsigned int reg_hdr_16to12_mi_lenx4:2;

	// h0029, bit: 7
	/* Histogram arbiter mi mask cycx4*/
	#define offset_of_reg_hdr_16to12_mi_mask_cycx4 (82)
	#define mask_of_reg_hdr_16to12_mi_mask_cycx4 (0xfc)
	unsigned int reg_hdr_16to12_mi_mask_cycx4:6;

	// h0029, bit: 14
	/* */
	unsigned int :8;

	// h0029
	unsigned int /* padding 16 bit */:16;

	// h002a, bit: 14
	/* Histogram arbiter max wadr*/
	#define offset_of_reg_hdr_16to12_max_wadr (84)
	#define mask_of_reg_hdr_16to12_max_wadr (0xffff)
	unsigned int reg_hdr_16to12_max_wadr:16;

	// h002a
	unsigned int /* padding 16 bit */:16;

	// h002b, bit: 10
	/* Histogram arbiter max wadr*/
	#define offset_of_reg_hdr_16to12_max_wadr_1 (86)
	#define mask_of_reg_hdr_16to12_max_wadr_1 (0x7ff)
	unsigned int reg_hdr_16to12_max_wadr_1:11;

	// h002b, bit: 14
	/* */
	unsigned int :5;

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

	// h0030, bit: 0
	/* Decompressor enable*/
	#define offset_of_reg_decomp_en (96)
	#define mask_of_reg_decomp_en (0x1)
	unsigned int reg_decomp_en:1;

	// h0030, bit: 14
	/* */
	unsigned int :15;

	// h0030
	unsigned int /* padding 16 bit */:16;

	// h0031, bit: 14
	/* Decomp factor 0 range 0*/
	#define offset_of_reg_decomp_r0_f0 (98)
	#define mask_of_reg_decomp_r0_f0 (0xffff)
	unsigned int reg_decomp_r0_f0:16;

	// h0031
	unsigned int /* padding 16 bit */:16;

	// h0032, bit: 3
	/* Decomp factor 1 range 0*/
	#define offset_of_reg_decomp_r0_f1 (100)
	#define mask_of_reg_decomp_r0_f1 (0xf)
	unsigned int reg_decomp_r0_f1:4;

	// h0032, bit: 14
	/* */
	unsigned int :12;

	// h0032
	unsigned int /* padding 16 bit */:16;

	// h0033, bit: 14
	/* Decomp factor 2 range 0*/
	#define offset_of_reg_decomp_r0_f2 (102)
	#define mask_of_reg_decomp_r0_f2 (0xffff)
	unsigned int reg_decomp_r0_f2:16;

	// h0033
	unsigned int /* padding 16 bit */:16;

	// h0034, bit: 14
	/* Decomp factor 0 range 1*/
	#define offset_of_reg_decomp_r1_f0 (104)
	#define mask_of_reg_decomp_r1_f0 (0xffff)
	unsigned int reg_decomp_r1_f0:16;

	// h0034
	unsigned int /* padding 16 bit */:16;

	// h0035, bit: 3
	/* Decomp factor 1 range 1*/
	#define offset_of_reg_decomp_r1_f1 (106)
	#define mask_of_reg_decomp_r1_f1 (0xf)
	unsigned int reg_decomp_r1_f1:4;

	// h0035, bit: 14
	/* */
	unsigned int :12;

	// h0035
	unsigned int /* padding 16 bit */:16;

	// h0036, bit: 14
	/* Decomp factor 2 range 1*/
	#define offset_of_reg_decomp_r1_f2 (108)
	#define mask_of_reg_decomp_r1_f2 (0xffff)
	unsigned int reg_decomp_r1_f2:16;

	// h0036
	unsigned int /* padding 16 bit */:16;

	// h0037, bit: 14
	/* Decomp factor 0 range 2*/
	#define offset_of_reg_decomp_r2_f0 (110)
	#define mask_of_reg_decomp_r2_f0 (0xffff)
	unsigned int reg_decomp_r2_f0:16;

	// h0037
	unsigned int /* padding 16 bit */:16;

	// h0038, bit: 3
	/* Decomp factor 1 range 2*/
	#define offset_of_reg_decomp_r2_f1 (112)
	#define mask_of_reg_decomp_r2_f1 (0xf)
	unsigned int reg_decomp_r2_f1:4;

	// h0038, bit: 14
	/* */
	unsigned int :12;

	// h0038
	unsigned int /* padding 16 bit */:16;

	// h0039, bit: 14
	/* Decomp factor 2 range 2*/
	#define offset_of_reg_decomp_r2_f2 (114)
	#define mask_of_reg_decomp_r2_f2 (0xffff)
	unsigned int reg_decomp_r2_f2:16;

	// h0039
	unsigned int /* padding 16 bit */:16;

	// h003a, bit: 14
	/* Decomp factor 0 range 3*/
	#define offset_of_reg_decomp_r3_f0 (116)
	#define mask_of_reg_decomp_r3_f0 (0xffff)
	unsigned int reg_decomp_r3_f0:16;

	// h003a
	unsigned int /* padding 16 bit */:16;

	// h003b, bit: 3
	/* Decomp factor 1 range 3*/
	#define offset_of_reg_decomp_r3_f1 (118)
	#define mask_of_reg_decomp_r3_f1 (0xf)
	unsigned int reg_decomp_r3_f1:4;

	// h003b, bit: 14
	/* */
	unsigned int :12;

	// h003b
	unsigned int /* padding 16 bit */:16;

	// h003c, bit: 14
	/* Decomp factor 2 range 3*/
	#define offset_of_reg_decomp_r3_f2 (120)
	#define mask_of_reg_decomp_r3_f2 (0xffff)
	unsigned int reg_decomp_r3_f2:16;

	// h003c
	unsigned int /* padding 16 bit */:16;

	// h003d, bit: 14
	/* Decomp range 0*/
	#define offset_of_reg_decomp_range0 (122)
	#define mask_of_reg_decomp_range0 (0xffff)
	unsigned int reg_decomp_range0:16;

	// h003d
	unsigned int /* padding 16 bit */:16;

	// h003e, bit: 14
	/* Decomp range 1*/
	#define offset_of_reg_decomp_range1 (124)
	#define mask_of_reg_decomp_range1 (0xffff)
	unsigned int reg_decomp_range1:16;

	// h003e
	unsigned int /* padding 16 bit */:16;

	// h003f, bit: 14
	/* Decomp range 2*/
	#define offset_of_reg_decomp_range2 (126)
	#define mask_of_reg_decomp_range2 (0xffff)
	unsigned int reg_decomp_range2:16;

	// h003f
	unsigned int /* padding 16 bit */:16;

	// h0040, bit: 0
	/* Histogram by pixel enable*/
	#define offset_of_reg_histo_p_en (128)
	#define mask_of_reg_histo_p_en (0x1)
	unsigned int reg_histo_p_en:1;

	// h0040, bit: 14
	/* */
	unsigned int :15;

	// h0040
	unsigned int /* padding 16 bit */:16;

	// h0041, bit: 11
	/* Histogram by pixel horizontal offset*/
	#define offset_of_reg_histo_p_x_offset (130)
	#define mask_of_reg_histo_p_x_offset (0xfff)
	unsigned int reg_histo_p_x_offset:12;

	// h0041, bit: 14
	/* */
	unsigned int :4;

	// h0041
	unsigned int /* padding 16 bit */:16;

	// h0042, bit: 11
	/* Histogram by pixel vertical offset*/
	#define offset_of_reg_histo_p_y_offset (132)
	#define mask_of_reg_histo_p_y_offset (0xfff)
	unsigned int reg_histo_p_y_offset:12;

	// h0042, bit: 14
	/* */
	unsigned int :4;

	// h0042
	unsigned int /* padding 16 bit */:16;

	// h0043, bit: 11
	/* Histogram by pixel horizontal size minus1*/
	#define offset_of_reg_histo_p_x_size_m1 (134)
	#define mask_of_reg_histo_p_x_size_m1 (0xfff)
	unsigned int reg_histo_p_x_size_m1:12;

	// h0043, bit: 14
	/* */
	unsigned int :4;

	// h0043
	unsigned int /* padding 16 bit */:16;

	// h0044, bit: 11
	/* Histogram by pixel vertical size minus 1*/
	#define offset_of_reg_histo_p_y_size_m1 (136)
	#define mask_of_reg_histo_p_y_size_m1 (0xfff)
	unsigned int reg_histo_p_y_size_m1:12;

	// h0044, bit: 14
	/* */
	unsigned int :4;

	// h0044
	unsigned int /* padding 16 bit */:16;

	// h0045, bit: 7
	/* Histogram by pixel Ratio R*/
	#define offset_of_reg_histo_p_ratio_r (138)
	#define mask_of_reg_histo_p_ratio_r (0xff)
	unsigned int reg_histo_p_ratio_r:8;

	// h0045, bit: 14
	/* */
	unsigned int :8;

	// h0045
	unsigned int /* padding 16 bit */:16;

	// h0046, bit: 7
	/* Histogram by pixel Ratio G*/
	#define offset_of_reg_histo_p_ratio_g (140)
	#define mask_of_reg_histo_p_ratio_g (0xff)
	unsigned int reg_histo_p_ratio_g:8;

	// h0046, bit: 14
	/* */
	unsigned int :8;

	// h0046
	unsigned int /* padding 16 bit */:16;

	// h0047, bit: 7
	/* Histogram by pixel Ratio B*/
	#define offset_of_reg_histo_p_ratio_b (142)
	#define mask_of_reg_histo_p_ratio_b (0xff)
	unsigned int reg_histo_p_ratio_b:8;

	// h0047, bit: 14
	/* */
	unsigned int :8;

	// h0047
	unsigned int /* padding 16 bit */:16;

	// h0048, bit: 0
	/* Histogram by pixel SW mode enable*/
	#define offset_of_reg_histo_p_sw_en (144)
	#define mask_of_reg_histo_p_sw_en (0x1)
	unsigned int reg_histo_p_sw_en:1;

	// h0048, bit: 14
	/* */
	unsigned int :15;

	// h0048
	unsigned int /* padding 16 bit */:16;

	// h0049, bit: 0
	/* Histogram by pixel SW mode inverse chip enable*/
	#define offset_of_reg_histo_p_sw_ce_n (146)
	#define mask_of_reg_histo_p_sw_ce_n (0x1)
	unsigned int reg_histo_p_sw_ce_n:1;

	// h0049, bit: 7
	/* */
	unsigned int :7;

	// h0049, bit: 14
	/* Histogram by pixel SW mode address*/
	#define offset_of_reg_histo_p_sw_adr (146)
	#define mask_of_reg_histo_p_sw_adr (0xff00)
	unsigned int reg_histo_p_sw_adr:8;

	// h0049
	unsigned int /* padding 16 bit */:16;

	// h004a, bit: 14
	/* Histogram by pixel R SRAM read data*/
	#define offset_of_reg_histo_p_r_sram_rd (148)
	#define mask_of_reg_histo_p_r_sram_rd (0xffff)
	unsigned int reg_histo_p_r_sram_rd:16;

	// h004a
	unsigned int /* padding 16 bit */:16;

	// h004b, bit: 4
	/* Histogram by pixel R SRAM read data*/
	#define offset_of_reg_histo_p_r_sram_rd_1 (150)
	#define mask_of_reg_histo_p_r_sram_rd_1 (0x1f)
	unsigned int reg_histo_p_r_sram_rd_1:5;

	// h004b, bit: 14
	/* */
	unsigned int :11;

	// h004b
	unsigned int /* padding 16 bit */:16;

	// h004c, bit: 14
	/* Histogram by pixel G SRAM read data*/
	#define offset_of_reg_histo_p_g_sram_rd (152)
	#define mask_of_reg_histo_p_g_sram_rd (0xffff)
	unsigned int reg_histo_p_g_sram_rd:16;

	// h004c
	unsigned int /* padding 16 bit */:16;

	// h004d, bit: 4
	/* Histogram by pixel G SRAM read data*/
	#define offset_of_reg_histo_p_g_sram_rd_1 (154)
	#define mask_of_reg_histo_p_g_sram_rd_1 (0x1f)
	unsigned int reg_histo_p_g_sram_rd_1:5;

	// h004d, bit: 14
	/* */
	unsigned int :11;

	// h004d
	unsigned int /* padding 16 bit */:16;

	// h004e, bit: 14
	/* Histogram by pixel B SRAM read data*/
	#define offset_of_reg_histo_p_b_sram_rd (156)
	#define mask_of_reg_histo_p_b_sram_rd (0xffff)
	unsigned int reg_histo_p_b_sram_rd:16;

	// h004e
	unsigned int /* padding 16 bit */:16;

	// h004f, bit: 4
	/* Histogram by pixel B SRAM read data*/
	#define offset_of_reg_histo_p_b_sram_rd_1 (158)
	#define mask_of_reg_histo_p_b_sram_rd_1 (0x1f)
	unsigned int reg_histo_p_b_sram_rd_1:5;

	// h004f, bit: 14
	/* */
	unsigned int :11;

	// h004f
	unsigned int /* padding 16 bit */:16;

	// h0050, bit: 14
	/* Histogram by pixel Y SRAM read data*/
	#define offset_of_reg_histo_p_y_sram_rd (160)
	#define mask_of_reg_histo_p_y_sram_rd (0xffff)
	unsigned int reg_histo_p_y_sram_rd:16;

	// h0050
	unsigned int /* padding 16 bit */:16;

	// h0051, bit: 4
	/* Histogram by pixel Y SRAM read data*/
	#define offset_of_reg_histo_p_y_sram_rd_1 (162)
	#define mask_of_reg_histo_p_y_sram_rd_1 (0x1f)
	unsigned int reg_histo_p_y_sram_rd_1:5;

	// h0051, bit: 14
	/* */
	unsigned int :11;

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
	/* For opr test*/
	#define offset_of_reg_hdr_16to12_dummy (254)
	#define mask_of_reg_hdr_16to12_dummy (0xffff)
	unsigned int reg_hdr_16to12_dummy:16;

	// h007f
	unsigned int /* padding 16 bit */:16;

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp9;
#endif
