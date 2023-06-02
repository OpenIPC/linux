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
#ifndef __INFINITY3E_REG_ISP4__
#define __INFINITY3E_REG_ISP4__
typedef struct {
	// h0000, bit: 3
	/* Load to the needed sram id*/
	#define offset_of_reg_isp_load_sram_id (0)
	#define mask_of_reg_isp_load_sram_id (0xf)
	unsigned int reg_isp_load_sram_id:4;

	// h0000, bit: 7
	/* Read the needed sram data*/
	#define offset_of_reg_isp_sram_read_id (0)
	#define mask_of_reg_isp_sram_read_id (0xf0)
	unsigned int reg_isp_sram_read_id:4;

	// h0000, bit: 8
	/* Read or write srams.  1: write*/
	#define offset_of_reg_isp_sram_rw (0)
	#define mask_of_reg_isp_sram_rw (0x100)
	unsigned int reg_isp_sram_rw:1;

	// h0000, bit: 9
	/* Enable miu loader. 1: enable*/
	#define offset_of_reg_isp_miu2sram_en (0)
	#define mask_of_reg_isp_miu2sram_en (0x200)
	unsigned int reg_isp_miu2sram_en:1;

	// h0000, bit: 10
	/* If it is full, the bit is 0  (16 entries)*/
	#define offset_of_reg_isp_load_register_non_full (0)
	#define mask_of_reg_isp_load_register_non_full (0x400)
	unsigned int reg_isp_load_register_non_full:1;

	// h0000, bit: 11
	/* 1: load done*/
	#define offset_of_reg_isp_load_done (0)
	#define mask_of_reg_isp_load_done (0x800)
	unsigned int reg_isp_load_done:1;

	// h0000, bit: 12
	/* 1: wait hardware not busy*/
	#define offset_of_reg_isp_load_wait_hardware_ready_en (0)
	#define mask_of_reg_isp_load_wait_hardware_ready_en (0x1000)
	unsigned int reg_isp_load_wait_hardware_ready_en:1;

	// h0000, bit: 14
	/* 0: wait fifo space == 4, 1: wait space == 8, 2: wait space  == 12, 3: wait fifo empty (16)*/
	#define offset_of_reg_isp_load_water_level (0)
	#define mask_of_reg_isp_load_water_level (0x6000)
	unsigned int reg_isp_load_water_level:2;

	// h0000, bit: 15
	/* 0: software reset*/
	#define offset_of_reg_isp_load_sw_rst (0)
	#define mask_of_reg_isp_load_sw_rst (0x8000)
	unsigned int reg_isp_load_sw_rst:1;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 14
	/* How many 16-bits data minus one*/
	#define offset_of_reg_isp_load_amount (2)
	#define mask_of_reg_isp_load_amount (0xffff)
	unsigned int reg_isp_load_amount:16;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 14
	/* Load data from the sram address*/
	#define offset_of_reg_isp_sram_st_addr (4)
	#define mask_of_reg_isp_sram_st_addr (0xffff)
	unsigned int reg_isp_sram_st_addr:16;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 14
	/* Load data from MIU 16-byte address*/
	#define offset_of_reg_isp_load_st_addr (6)
	#define mask_of_reg_isp_load_st_addr (0xffff)
	unsigned int reg_isp_load_st_addr:16;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 14
	/* Load data from MIU 16-byte address*/
	#define offset_of_reg_isp_load_st_addr_1 (8)
	#define mask_of_reg_isp_load_st_addr_1 (0xffff)
	unsigned int reg_isp_load_st_addr_1:16;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 14
	/* Read sram data from set address from set sram*/
	#define offset_of_reg_isp_sram_rd_addr (10)
	#define mask_of_reg_isp_sram_rd_addr (0xffff)
	unsigned int reg_isp_sram_rd_addr:16;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 14
	/* Sram read data from corresponding address and sram*/
	#define offset_of_reg_isp_sram_read_data (12)
	#define mask_of_reg_isp_sram_read_data (0xffff)
	unsigned int reg_isp_sram_read_data:16;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 0
	/* Fire write register control into register queue*/
	#define offset_of_reg_isp_load_register_w1r (14)
	#define mask_of_reg_isp_load_register_w1r (0x1)
	unsigned int reg_isp_load_register_w1r:1;

	// h0007, bit: 1
	/* Fire load sram*/
	#define offset_of_reg_isp_load_st_w1r (14)
	#define mask_of_reg_isp_load_st_w1r (0x2)
	unsigned int reg_isp_load_st_w1r:1;

	// h0007, bit: 2
	/* Fire read sram*/
	#define offset_of_reg_isp_sram_read_w1r (14)
	#define mask_of_reg_isp_sram_read_w1r (0x4)
	unsigned int reg_isp_sram_read_w1r:1;

	// h0007, bit: 3
	/* clear error flag*/
	#define offset_of_reg_isp_load_start_error_clear_w1r (14)
	#define mask_of_reg_isp_load_start_error_clear_w1r (0x8)
	unsigned int reg_isp_load_start_error_clear_w1r:1;

	// h0007, bit: 14
	/* */
	unsigned int :12;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 0
	/* 1: Frame start when loading is not finished yet*/
	#define offset_of_reg_isp_load_start_error (16)
	#define mask_of_reg_isp_load_start_error (0x1)
	unsigned int reg_isp_load_start_error:1;

	// h0008, bit: 14
	/* */
	unsigned int :15;

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

	// h0040, bit: 0
	/* fpn enable*/
	#define offset_of_reg_fpn_en (128)
	#define mask_of_reg_fpn_en (0x1)
	unsigned int reg_fpn_en:1;

	// h0040, bit: 1
	/* 0: statis for every frame set 1: statis for just first frame set*/
	#define offset_of_reg_fpn_mode (128)
	#define mask_of_reg_fpn_mode (0x2)
	unsigned int reg_fpn_mode:1;

	// h0040, bit: 2
	/* 0:don’t clear sum result 1:  clear sum result*/
	#define offset_of_reg_fpn_sum_clr (128)
	#define mask_of_reg_fpn_sum_clr (0x4)
	unsigned int reg_fpn_sum_clr:1;

	// h0040, bit: 3
	/* SW setting eable*/
	#define offset_of_reg_sw_offset_on (128)
	#define mask_of_reg_sw_offset_on (0x8)
	unsigned int reg_sw_offset_on:1;

	// h0040, bit: 14
	/* */
	unsigned int :12;

	// h0040
	unsigned int /* padding 16 bit */:16;

	// h0041, bit: 11
	/* ob start x */
	#define offset_of_reg_fpn_start_x (130)
	#define mask_of_reg_fpn_start_x (0xfff)
	unsigned int reg_fpn_start_x:12;

	// h0041, bit: 14
	/* */
	unsigned int :4;

	// h0041
	unsigned int /* padding 16 bit */:16;

	// h0042, bit: 11
	/* ob start xy*/
	#define offset_of_reg_fpn_start_y (132)
	#define mask_of_reg_fpn_start_y (0xfff)
	unsigned int reg_fpn_start_y:12;

	// h0042, bit: 14
	/* */
	unsigned int :4;

	// h0042
	unsigned int /* padding 16 bit */:16;

	// h0043, bit: 11
	/* ob width*/
	#define offset_of_reg_fpn_width (134)
	#define mask_of_reg_fpn_width (0xfff)
	unsigned int reg_fpn_width:12;

	// h0043, bit: 14
	/* */
	unsigned int :4;

	// h0043
	unsigned int /* padding 16 bit */:16;

	// h0044, bit: 4
	/* ob height*/
	#define offset_of_reg_fpn_height (136)
	#define mask_of_reg_fpn_height (0x1f)
	unsigned int reg_fpn_height:5;

	// h0044, bit: 14
	/* */
	unsigned int :11;

	// h0044
	unsigned int /* padding 16 bit */:16;

	// h0045, bit: 14
	/* pre-offset for pixels at active area*/
	#define offset_of_reg_fpn_preoffset (138)
	#define mask_of_reg_fpn_preoffset (0x7fff)
	unsigned int reg_fpn_preoffset:15;

	// h0045, bit: 15
	/* sign of pre-offset */
	#define offset_of_reg_fpn_preoffset_sign (138)
	#define mask_of_reg_fpn_preoffset_sign (0x8000)
	unsigned int reg_fpn_preoffset_sign:1;

	// h0045
	unsigned int /* padding 16 bit */:16;

	// h0046, bit: 14
	/* midium pixel and pavg diff threshould*/
	#define offset_of_reg_fpn_diff_th (140)
	#define mask_of_reg_fpn_diff_th (0xffff)
	unsigned int reg_fpn_diff_th:16;

	// h0046
	unsigned int /* padding 16 bit */:16;

	// h0047, bit: 11
	/* compare ratio*/
	#define offset_of_reg_fpn_cmp_ratio (142)
	#define mask_of_reg_fpn_cmp_ratio (0xfff)
	unsigned int reg_fpn_cmp_ratio:12;

	// h0047, bit: 14
	/* */
	unsigned int :4;

	// h0047
	unsigned int /* padding 16 bit */:16;

	// h0048, bit: 4
	/* statis frame number*/
	#define offset_of_reg_fpn_frame_num (144)
	#define mask_of_reg_fpn_frame_num (0x1f)
	unsigned int reg_fpn_frame_num:5;

	// h0048, bit: 14
	/* */
	unsigned int :11;

	// h0048
	unsigned int /* padding 16 bit */:16;

	// h0049, bit: 14
	/* SW offset setting write data*/
	#define offset_of_reg_sw_offset_wd (146)
	#define mask_of_reg_sw_offset_wd (0xffff)
	unsigned int reg_sw_offset_wd:16;

	// h0049
	unsigned int /* padding 16 bit */:16;

	// h004a, bit: 11
	/* SW offset setting write address*/
	#define offset_of_reg_sw_offset_a (148)
	#define mask_of_reg_sw_offset_a (0xfff)
	unsigned int reg_sw_offset_a:12;

	// h004a, bit: 14
	/* */
	unsigned int :4;

	// h004a
	unsigned int /* padding 16 bit */:16;

	// h004b, bit: 0
	/* SW offset setting write control*/
	#define offset_of_reg_sw_offset_we (150)
	#define mask_of_reg_sw_offset_we (0x1)
	unsigned int reg_sw_offset_we:1;

	// h004b, bit: 1
	/* SW setting eable*/
	#define offset_of_reg_sw_offset_re (150)
	#define mask_of_reg_sw_offset_re (0x2)
	unsigned int reg_sw_offset_re:1;

	// h004b, bit: 14
	/* */
	unsigned int :14;

	// h004b
	unsigned int /* padding 16 bit */:16;

	// h004c, bit: 14
	/* SW setting read data*/
	#define offset_of_reg_sw_offset_rd (152)
	#define mask_of_reg_sw_offset_rd (0xffff)
	unsigned int reg_sw_offset_rd:16;

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

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp4;
#endif
