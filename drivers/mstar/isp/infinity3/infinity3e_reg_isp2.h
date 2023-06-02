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
#ifndef __INFINITY3E_REG_ISP2__
#define __INFINITY3E_REG_ISP2__
typedef struct {
	// h0000, bit: 0
	/* DPC enable*/
	#define offset_of_reg_dpc_en (0)
	#define mask_of_reg_dpc_en (0x1)
	unsigned int reg_dpc_en:1;

	// h0000, bit: 2
	/* DPC mode*/
	#define offset_of_reg_dpc_mode (0)
	#define mask_of_reg_dpc_mode (0x6)
	unsigned int reg_dpc_mode:2;

	// h0000, bit: 3
	/* DPC awb-gain enable*/
	#define offset_of_reg_dpc_awbg_en (0)
	#define mask_of_reg_dpc_awbg_en (0x8)
	unsigned int reg_dpc_awbg_en:1;

	// h0000, bit: 4
	/* DPC edge mode enable*/
	#define offset_of_reg_dpc_edge_mode (0)
	#define mask_of_reg_dpc_edge_mode (0x10)
	unsigned int reg_dpc_edge_mode:1;

	// h0000, bit: 5
	/* DPC light enable*/
	#define offset_of_reg_dpc_light_en (0)
	#define mask_of_reg_dpc_light_en (0x20)
	unsigned int reg_dpc_light_en:1;

	// h0000, bit: 6
	/* DPC dark enable*/
	#define offset_of_reg_dpc_dark_en (0)
	#define mask_of_reg_dpc_dark_en (0x40)
	unsigned int reg_dpc_dark_en:1;

	// h0000, bit: 14
	/* */
	unsigned int :9;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 11
	/* DPC threshold R */
	#define offset_of_reg_dpc_th_r (2)
	#define mask_of_reg_dpc_th_r (0xfff)
	unsigned int reg_dpc_th_r:12;

	// h0001, bit: 14
	/* */
	unsigned int :4;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 11
	/* DPC threshold Gr */
	#define offset_of_reg_dpc_th_gr (4)
	#define mask_of_reg_dpc_th_gr (0xfff)
	unsigned int reg_dpc_th_gr:12;

	// h0002, bit: 14
	/* */
	unsigned int :4;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 11
	/* DPC threshold Gb */
	#define offset_of_reg_dpc_th_gb (6)
	#define mask_of_reg_dpc_th_gb (0xfff)
	unsigned int reg_dpc_th_gb:12;

	// h0003, bit: 14
	/* */
	unsigned int :4;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 11
	/* DPC threshold B*/
	#define offset_of_reg_dpc_th_b (8)
	#define mask_of_reg_dpc_th_b (0xfff)
	unsigned int reg_dpc_th_b:12;

	// h0004, bit: 14
	/* */
	unsigned int :4;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 0
	/* DPC cluster light-R enable*/
	#define offset_of_reg_dpc_cluster_lr_en (10)
	#define mask_of_reg_dpc_cluster_lr_en (0x1)
	unsigned int reg_dpc_cluster_lr_en:1;

	// h0005, bit: 1
	/* DPC cluster light-Gr enable*/
	#define offset_of_reg_dpc_cluster_lgr_en (10)
	#define mask_of_reg_dpc_cluster_lgr_en (0x2)
	unsigned int reg_dpc_cluster_lgr_en:1;

	// h0005, bit: 2
	/* DPC cluster light-Gb enable*/
	#define offset_of_reg_dpc_cluster_lgb_en (10)
	#define mask_of_reg_dpc_cluster_lgb_en (0x4)
	unsigned int reg_dpc_cluster_lgb_en:1;

	// h0005, bit: 3
	/* DPC cluster light-B enable*/
	#define offset_of_reg_dpc_cluster_lb_en (10)
	#define mask_of_reg_dpc_cluster_lb_en (0x8)
	unsigned int reg_dpc_cluster_lb_en:1;

	// h0005, bit: 4
	/* DPC cluster light mode off*/
	#define offset_of_reg_dpc_cluster_mode_loff (10)
	#define mask_of_reg_dpc_cluster_mode_loff (0x10)
	unsigned int reg_dpc_cluster_mode_loff:1;

	// h0005, bit: 7
	/* */
	unsigned int :3;

	// h0005, bit: 8
	/* DPC cluster dark-R enable*/
	#define offset_of_reg_dpc_cluster_dr_en (10)
	#define mask_of_reg_dpc_cluster_dr_en (0x100)
	unsigned int reg_dpc_cluster_dr_en:1;

	// h0005, bit: 9
	/* DPC cluster dark-Gr enable*/
	#define offset_of_reg_dpc_cluster_dgr_en (10)
	#define mask_of_reg_dpc_cluster_dgr_en (0x200)
	unsigned int reg_dpc_cluster_dgr_en:1;

	// h0005, bit: 10
	/* DPC cluster dark-Gb enable*/
	#define offset_of_reg_dpc_cluster_dgb_en (10)
	#define mask_of_reg_dpc_cluster_dgb_en (0x400)
	unsigned int reg_dpc_cluster_dgb_en:1;

	// h0005, bit: 11
	/* DPC cluster dark-B enable*/
	#define offset_of_reg_dpc_cluster_db_en (10)
	#define mask_of_reg_dpc_cluster_db_en (0x800)
	unsigned int reg_dpc_cluster_db_en:1;

	// h0005, bit: 12
	/* DPC cluster dark mode off*/
	#define offset_of_reg_dpc_cluster_mode_doff (10)
	#define mask_of_reg_dpc_cluster_mode_doff (0x1000)
	unsigned int reg_dpc_cluster_mode_doff:1;

	// h0005, bit: 14
	/* */
	unsigned int :3;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 1
	/* DPC threshold selection*/
	#define offset_of_reg_dpc_th_sel (12)
	#define mask_of_reg_dpc_th_sel (0x3)
	unsigned int reg_dpc_th_sel:2;

	// h0006, bit: 2
	/* DPC light threshold adjust*/
	#define offset_of_reg_dpc_light_th_adj (12)
	#define mask_of_reg_dpc_light_th_adj (0x4)
	unsigned int reg_dpc_light_th_adj:1;

	// h0006, bit: 3
	/* DPC dark threshold adjust*/
	#define offset_of_reg_dpc_dark_th_adj (12)
	#define mask_of_reg_dpc_dark_th_adj (0x8)
	unsigned int reg_dpc_dark_th_adj:1;

	// h0006, bit: 14
	/* */
	unsigned int :12;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 3
	/* DPC light threshold source*/
	#define offset_of_reg_dpc_th_light_src (14)
	#define mask_of_reg_dpc_th_light_src (0xf)
	unsigned int reg_dpc_th_light_src:4;

	// h0007, bit: 7
	/* DPC light threshold base*/
	#define offset_of_reg_dpc_th_light_base (14)
	#define mask_of_reg_dpc_th_light_base (0xf0)
	unsigned int reg_dpc_th_light_base:4;

	// h0007, bit: 11
	/* DPC dark threshold source*/
	#define offset_of_reg_dpc_th_dark_src (14)
	#define mask_of_reg_dpc_th_dark_src (0xf00)
	unsigned int reg_dpc_th_dark_src:4;

	// h0007, bit: 14
	/* DPC dark threshold base*/
	#define offset_of_reg_dpc_th_dark_base (14)
	#define mask_of_reg_dpc_th_dark_base (0xf000)
	unsigned int reg_dpc_th_dark_base:4;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 7
	/* DPC neighbor smooth parameter*/
	#define offset_of_reg_dpc_nei_smooth (16)
	#define mask_of_reg_dpc_nei_smooth (0xff)
	unsigned int reg_dpc_nei_smooth:8;

	// h0008, bit: 14
	/* */
	unsigned int :8;

	// h0008
	unsigned int /* padding 16 bit */:16;

	// h0009, bit: 11
	/* DPC neighbor delta threshold*/
	#define offset_of_reg_dpc_nei_delta_th (18)
	#define mask_of_reg_dpc_nei_delta_th (0xfff)
	unsigned int reg_dpc_nei_delta_th:12;

	// h0009, bit: 14
	/* */
	unsigned int :4;

	// h0009
	unsigned int /* padding 16 bit */:16;

	// h000a, bit: 3
	/* DPC neighbor delta shift parameter x0 */
	#define offset_of_reg_dpc_nei_delta_sft_x0 (20)
	#define mask_of_reg_dpc_nei_delta_sft_x0 (0xf)
	unsigned int reg_dpc_nei_delta_sft_x0:4;

	// h000a, bit: 7
	/* */
	unsigned int :4;

	// h000a, bit: 11
	/* DPC neighbor delta shift parameter x1*/
	#define offset_of_reg_dpc_nei_delta_sft_x1 (20)
	#define mask_of_reg_dpc_nei_delta_sft_x1 (0xf00)
	unsigned int reg_dpc_nei_delta_sft_x1:4;

	// h000a, bit: 14
	/* */
	unsigned int :4;

	// h000a
	unsigned int /* padding 16 bit */:16;

	// h000b, bit: 3
	/* DPC neighbor delta shift parameter x2*/
	#define offset_of_reg_dpc_nei_delta_sft_x2 (22)
	#define mask_of_reg_dpc_nei_delta_sft_x2 (0xf)
	unsigned int reg_dpc_nei_delta_sft_x2:4;

	// h000b, bit: 7
	/* */
	unsigned int :4;

	// h000b, bit: 11
	/* DPC neighbor delta shift parameter x3*/
	#define offset_of_reg_dpc_nei_delta_sft_x3 (22)
	#define mask_of_reg_dpc_nei_delta_sft_x3 (0xf00)
	unsigned int reg_dpc_nei_delta_sft_x3:4;

	// h000b, bit: 14
	/* */
	unsigned int :4;

	// h000b
	unsigned int /* padding 16 bit */:16;

	// h000c, bit: 3
	/* DPC neighbor delta shift parameter x4*/
	#define offset_of_reg_dpc_nei_delta_sft_x4 (24)
	#define mask_of_reg_dpc_nei_delta_sft_x4 (0xf)
	unsigned int reg_dpc_nei_delta_sft_x4:4;

	// h000c, bit: 14
	/* */
	unsigned int :12;

	// h000c
	unsigned int /* padding 16 bit */:16;

	// h000d, bit: 7
	/* DPC neighbor delta gain0*/
	#define offset_of_reg_dpc_nei_delta_gain0 (26)
	#define mask_of_reg_dpc_nei_delta_gain0 (0xff)
	unsigned int reg_dpc_nei_delta_gain0:8;

	// h000d, bit: 14
	/* DPC neighbor delta gain1*/
	#define offset_of_reg_dpc_nei_delta_gain1 (26)
	#define mask_of_reg_dpc_nei_delta_gain1 (0xff00)
	unsigned int reg_dpc_nei_delta_gain1:8;

	// h000d
	unsigned int /* padding 16 bit */:16;

	// h000e, bit: 7
	/* DPC neighbor delta gain2*/
	#define offset_of_reg_dpc_nei_delta_gain2 (28)
	#define mask_of_reg_dpc_nei_delta_gain2 (0xff)
	unsigned int reg_dpc_nei_delta_gain2:8;

	// h000e, bit: 14
	/* DPC neighbor delta gain3*/
	#define offset_of_reg_dpc_nei_delta_gain3 (28)
	#define mask_of_reg_dpc_nei_delta_gain3 (0xff00)
	unsigned int reg_dpc_nei_delta_gain3:8;

	// h000e
	unsigned int /* padding 16 bit */:16;

	// h000f, bit: 7
	/* DPC neighbor delta gain4*/
	#define offset_of_reg_dpc_nei_delta_gain4 (30)
	#define mask_of_reg_dpc_nei_delta_gain4 (0xff)
	unsigned int reg_dpc_nei_delta_gain4:8;

	// h000f, bit: 14
	/* DPC neighbor delta gain5*/
	#define offset_of_reg_dpc_nei_delta_gain5 (30)
	#define mask_of_reg_dpc_nei_delta_gain5 (0xff00)
	unsigned int reg_dpc_nei_delta_gain5:8;

	// h000f
	unsigned int /* padding 16 bit */:16;

	// h0010, bit: 11
	/* DPC color ratio light-R parameter*/
	#define offset_of_reg_dpc_color_ratio_lr (32)
	#define mask_of_reg_dpc_color_ratio_lr (0xfff)
	unsigned int reg_dpc_color_ratio_lr:12;

	// h0010, bit: 14
	/* */
	unsigned int :4;

	// h0010
	unsigned int /* padding 16 bit */:16;

	// h0011, bit: 11
	/* DPC color ratio light-Gr parameter*/
	#define offset_of_reg_dpc_color_ratio_lgr (34)
	#define mask_of_reg_dpc_color_ratio_lgr (0xfff)
	unsigned int reg_dpc_color_ratio_lgr:12;

	// h0011, bit: 14
	/* */
	unsigned int :4;

	// h0011
	unsigned int /* padding 16 bit */:16;

	// h0012, bit: 11
	/* DPC color ratio lightGb parameter*/
	#define offset_of_reg_dpc_color_ratio_lgb (36)
	#define mask_of_reg_dpc_color_ratio_lgb (0xfff)
	unsigned int reg_dpc_color_ratio_lgb:12;

	// h0012, bit: 14
	/* */
	unsigned int :4;

	// h0012
	unsigned int /* padding 16 bit */:16;

	// h0013, bit: 11
	/* DPC color ratio light-B parameter*/
	#define offset_of_reg_dpc_color_ratio_lb (38)
	#define mask_of_reg_dpc_color_ratio_lb (0xfff)
	unsigned int reg_dpc_color_ratio_lb:12;

	// h0013, bit: 14
	/* */
	unsigned int :4;

	// h0013
	unsigned int /* padding 16 bit */:16;

	// h0014, bit: 11
	/* DPC color ratio light-off parameter*/
	#define offset_of_reg_dpc_color_ratio_loff (40)
	#define mask_of_reg_dpc_color_ratio_loff (0xfff)
	unsigned int reg_dpc_color_ratio_loff:12;

	// h0014, bit: 14
	/* */
	unsigned int :4;

	// h0014
	unsigned int /* padding 16 bit */:16;

	// h0015, bit: 11
	/* DPC color ratio dark-R parameter*/
	#define offset_of_reg_dpc_color_ratio_dr (42)
	#define mask_of_reg_dpc_color_ratio_dr (0xfff)
	unsigned int reg_dpc_color_ratio_dr:12;

	// h0015, bit: 14
	/* */
	unsigned int :4;

	// h0015
	unsigned int /* padding 16 bit */:16;

	// h0016, bit: 11
	/* DPC color ratio dark-Gr parameter*/
	#define offset_of_reg_dpc_color_ratio_dgr (44)
	#define mask_of_reg_dpc_color_ratio_dgr (0xfff)
	unsigned int reg_dpc_color_ratio_dgr:12;

	// h0016, bit: 14
	/* */
	unsigned int :4;

	// h0016
	unsigned int /* padding 16 bit */:16;

	// h0017, bit: 11
	/* DPC color ratio dark-Gb parameter*/
	#define offset_of_reg_dpc_color_ratio_dgb (46)
	#define mask_of_reg_dpc_color_ratio_dgb (0xfff)
	unsigned int reg_dpc_color_ratio_dgb:12;

	// h0017, bit: 14
	/* */
	unsigned int :4;

	// h0017
	unsigned int /* padding 16 bit */:16;

	// h0018, bit: 11
	/* DPC color ratio dark-B parameter*/
	#define offset_of_reg_dpc_color_ratio_db (48)
	#define mask_of_reg_dpc_color_ratio_db (0xfff)
	unsigned int reg_dpc_color_ratio_db:12;

	// h0018, bit: 14
	/* */
	unsigned int :4;

	// h0018
	unsigned int /* padding 16 bit */:16;

	// h0019, bit: 11
	/* DPC color ratio dark-off parameter*/
	#define offset_of_reg_dpc_color_ratio_doff (50)
	#define mask_of_reg_dpc_color_ratio_doff (0xfff)
	unsigned int reg_dpc_color_ratio_doff:12;

	// h0019, bit: 14
	/* */
	unsigned int :4;

	// h0019
	unsigned int /* padding 16 bit */:16;

	// h001a, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add00 (52)
	#define mask_of_reg_dpc_light_th_add00 (0xff)
	unsigned int reg_dpc_light_th_add00:8;

	// h001a, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add01 (52)
	#define mask_of_reg_dpc_light_th_add01 (0xff00)
	unsigned int reg_dpc_light_th_add01:8;

	// h001a
	unsigned int /* padding 16 bit */:16;

	// h001b, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add02 (54)
	#define mask_of_reg_dpc_light_th_add02 (0xff)
	unsigned int reg_dpc_light_th_add02:8;

	// h001b, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add03 (54)
	#define mask_of_reg_dpc_light_th_add03 (0xff00)
	unsigned int reg_dpc_light_th_add03:8;

	// h001b
	unsigned int /* padding 16 bit */:16;

	// h001c, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add04 (56)
	#define mask_of_reg_dpc_light_th_add04 (0xff)
	unsigned int reg_dpc_light_th_add04:8;

	// h001c, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add05 (56)
	#define mask_of_reg_dpc_light_th_add05 (0xff00)
	unsigned int reg_dpc_light_th_add05:8;

	// h001c
	unsigned int /* padding 16 bit */:16;

	// h001d, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add06 (58)
	#define mask_of_reg_dpc_light_th_add06 (0xff)
	unsigned int reg_dpc_light_th_add06:8;

	// h001d, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add07 (58)
	#define mask_of_reg_dpc_light_th_add07 (0xff00)
	unsigned int reg_dpc_light_th_add07:8;

	// h001d
	unsigned int /* padding 16 bit */:16;

	// h001e, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add08 (60)
	#define mask_of_reg_dpc_light_th_add08 (0xff)
	unsigned int reg_dpc_light_th_add08:8;

	// h001e, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add09 (60)
	#define mask_of_reg_dpc_light_th_add09 (0xff00)
	unsigned int reg_dpc_light_th_add09:8;

	// h001e
	unsigned int /* padding 16 bit */:16;

	// h001f, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add10 (62)
	#define mask_of_reg_dpc_light_th_add10 (0xff)
	unsigned int reg_dpc_light_th_add10:8;

	// h001f, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add11 (62)
	#define mask_of_reg_dpc_light_th_add11 (0xff00)
	unsigned int reg_dpc_light_th_add11:8;

	// h001f
	unsigned int /* padding 16 bit */:16;

	// h0020, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add12 (64)
	#define mask_of_reg_dpc_light_th_add12 (0xff)
	unsigned int reg_dpc_light_th_add12:8;

	// h0020, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add13 (64)
	#define mask_of_reg_dpc_light_th_add13 (0xff00)
	unsigned int reg_dpc_light_th_add13:8;

	// h0020
	unsigned int /* padding 16 bit */:16;

	// h0021, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add14 (66)
	#define mask_of_reg_dpc_light_th_add14 (0xff)
	unsigned int reg_dpc_light_th_add14:8;

	// h0021, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add15 (66)
	#define mask_of_reg_dpc_light_th_add15 (0xff00)
	unsigned int reg_dpc_light_th_add15:8;

	// h0021
	unsigned int /* padding 16 bit */:16;

	// h0022, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add16 (68)
	#define mask_of_reg_dpc_light_th_add16 (0xff)
	unsigned int reg_dpc_light_th_add16:8;

	// h0022, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add17 (68)
	#define mask_of_reg_dpc_light_th_add17 (0xff00)
	unsigned int reg_dpc_light_th_add17:8;

	// h0022
	unsigned int /* padding 16 bit */:16;

	// h0023, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add18 (70)
	#define mask_of_reg_dpc_light_th_add18 (0xff)
	unsigned int reg_dpc_light_th_add18:8;

	// h0023, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add19 (70)
	#define mask_of_reg_dpc_light_th_add19 (0xff00)
	unsigned int reg_dpc_light_th_add19:8;

	// h0023
	unsigned int /* padding 16 bit */:16;

	// h0024, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add20 (72)
	#define mask_of_reg_dpc_light_th_add20 (0xff)
	unsigned int reg_dpc_light_th_add20:8;

	// h0024, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add21 (72)
	#define mask_of_reg_dpc_light_th_add21 (0xff00)
	unsigned int reg_dpc_light_th_add21:8;

	// h0024
	unsigned int /* padding 16 bit */:16;

	// h0025, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add22 (74)
	#define mask_of_reg_dpc_light_th_add22 (0xff)
	unsigned int reg_dpc_light_th_add22:8;

	// h0025, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add23 (74)
	#define mask_of_reg_dpc_light_th_add23 (0xff00)
	unsigned int reg_dpc_light_th_add23:8;

	// h0025
	unsigned int /* padding 16 bit */:16;

	// h0026, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add24 (76)
	#define mask_of_reg_dpc_light_th_add24 (0xff)
	unsigned int reg_dpc_light_th_add24:8;

	// h0026, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add25 (76)
	#define mask_of_reg_dpc_light_th_add25 (0xff00)
	unsigned int reg_dpc_light_th_add25:8;

	// h0026
	unsigned int /* padding 16 bit */:16;

	// h0027, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add26 (78)
	#define mask_of_reg_dpc_light_th_add26 (0xff)
	unsigned int reg_dpc_light_th_add26:8;

	// h0027, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add27 (78)
	#define mask_of_reg_dpc_light_th_add27 (0xff00)
	unsigned int reg_dpc_light_th_add27:8;

	// h0027
	unsigned int /* padding 16 bit */:16;

	// h0028, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add28 (80)
	#define mask_of_reg_dpc_light_th_add28 (0xff)
	unsigned int reg_dpc_light_th_add28:8;

	// h0028, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add29 (80)
	#define mask_of_reg_dpc_light_th_add29 (0xff00)
	unsigned int reg_dpc_light_th_add29:8;

	// h0028
	unsigned int /* padding 16 bit */:16;

	// h0029, bit: 7
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add30 (82)
	#define mask_of_reg_dpc_light_th_add30 (0xff)
	unsigned int reg_dpc_light_th_add30:8;

	// h0029, bit: 14
	/* DPC light threshold add*/
	#define offset_of_reg_dpc_light_th_add31 (82)
	#define mask_of_reg_dpc_light_th_add31 (0xff00)
	unsigned int reg_dpc_light_th_add31:8;

	// h0029
	unsigned int /* padding 16 bit */:16;

	// h002a, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add00 (84)
	#define mask_of_reg_dpc_dark_th_add00 (0xff)
	unsigned int reg_dpc_dark_th_add00:8;

	// h002a, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add01 (84)
	#define mask_of_reg_dpc_dark_th_add01 (0xff00)
	unsigned int reg_dpc_dark_th_add01:8;

	// h002a
	unsigned int /* padding 16 bit */:16;

	// h002b, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add02 (86)
	#define mask_of_reg_dpc_dark_th_add02 (0xff)
	unsigned int reg_dpc_dark_th_add02:8;

	// h002b, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add03 (86)
	#define mask_of_reg_dpc_dark_th_add03 (0xff00)
	unsigned int reg_dpc_dark_th_add03:8;

	// h002b
	unsigned int /* padding 16 bit */:16;

	// h002c, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add04 (88)
	#define mask_of_reg_dpc_dark_th_add04 (0xff)
	unsigned int reg_dpc_dark_th_add04:8;

	// h002c, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add05 (88)
	#define mask_of_reg_dpc_dark_th_add05 (0xff00)
	unsigned int reg_dpc_dark_th_add05:8;

	// h002c
	unsigned int /* padding 16 bit */:16;

	// h002d, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add06 (90)
	#define mask_of_reg_dpc_dark_th_add06 (0xff)
	unsigned int reg_dpc_dark_th_add06:8;

	// h002d, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add07 (90)
	#define mask_of_reg_dpc_dark_th_add07 (0xff00)
	unsigned int reg_dpc_dark_th_add07:8;

	// h002d
	unsigned int /* padding 16 bit */:16;

	// h002e, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add08 (92)
	#define mask_of_reg_dpc_dark_th_add08 (0xff)
	unsigned int reg_dpc_dark_th_add08:8;

	// h002e, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add09 (92)
	#define mask_of_reg_dpc_dark_th_add09 (0xff00)
	unsigned int reg_dpc_dark_th_add09:8;

	// h002e
	unsigned int /* padding 16 bit */:16;

	// h002f, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add10 (94)
	#define mask_of_reg_dpc_dark_th_add10 (0xff)
	unsigned int reg_dpc_dark_th_add10:8;

	// h002f, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add11 (94)
	#define mask_of_reg_dpc_dark_th_add11 (0xff00)
	unsigned int reg_dpc_dark_th_add11:8;

	// h002f
	unsigned int /* padding 16 bit */:16;

	// h0030, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add12 (96)
	#define mask_of_reg_dpc_dark_th_add12 (0xff)
	unsigned int reg_dpc_dark_th_add12:8;

	// h0030, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add13 (96)
	#define mask_of_reg_dpc_dark_th_add13 (0xff00)
	unsigned int reg_dpc_dark_th_add13:8;

	// h0030
	unsigned int /* padding 16 bit */:16;

	// h0031, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add14 (98)
	#define mask_of_reg_dpc_dark_th_add14 (0xff)
	unsigned int reg_dpc_dark_th_add14:8;

	// h0031, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add15 (98)
	#define mask_of_reg_dpc_dark_th_add15 (0xff00)
	unsigned int reg_dpc_dark_th_add15:8;

	// h0031
	unsigned int /* padding 16 bit */:16;

	// h0032, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add16 (100)
	#define mask_of_reg_dpc_dark_th_add16 (0xff)
	unsigned int reg_dpc_dark_th_add16:8;

	// h0032, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add17 (100)
	#define mask_of_reg_dpc_dark_th_add17 (0xff00)
	unsigned int reg_dpc_dark_th_add17:8;

	// h0032
	unsigned int /* padding 16 bit */:16;

	// h0033, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add18 (102)
	#define mask_of_reg_dpc_dark_th_add18 (0xff)
	unsigned int reg_dpc_dark_th_add18:8;

	// h0033, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add19 (102)
	#define mask_of_reg_dpc_dark_th_add19 (0xff00)
	unsigned int reg_dpc_dark_th_add19:8;

	// h0033
	unsigned int /* padding 16 bit */:16;

	// h0034, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add20 (104)
	#define mask_of_reg_dpc_dark_th_add20 (0xff)
	unsigned int reg_dpc_dark_th_add20:8;

	// h0034, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add21 (104)
	#define mask_of_reg_dpc_dark_th_add21 (0xff00)
	unsigned int reg_dpc_dark_th_add21:8;

	// h0034
	unsigned int /* padding 16 bit */:16;

	// h0035, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add22 (106)
	#define mask_of_reg_dpc_dark_th_add22 (0xff)
	unsigned int reg_dpc_dark_th_add22:8;

	// h0035, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add23 (106)
	#define mask_of_reg_dpc_dark_th_add23 (0xff00)
	unsigned int reg_dpc_dark_th_add23:8;

	// h0035
	unsigned int /* padding 16 bit */:16;

	// h0036, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add24 (108)
	#define mask_of_reg_dpc_dark_th_add24 (0xff)
	unsigned int reg_dpc_dark_th_add24:8;

	// h0036, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add25 (108)
	#define mask_of_reg_dpc_dark_th_add25 (0xff00)
	unsigned int reg_dpc_dark_th_add25:8;

	// h0036
	unsigned int /* padding 16 bit */:16;

	// h0037, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add26 (110)
	#define mask_of_reg_dpc_dark_th_add26 (0xff)
	unsigned int reg_dpc_dark_th_add26:8;

	// h0037, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add27 (110)
	#define mask_of_reg_dpc_dark_th_add27 (0xff00)
	unsigned int reg_dpc_dark_th_add27:8;

	// h0037
	unsigned int /* padding 16 bit */:16;

	// h0038, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add28 (112)
	#define mask_of_reg_dpc_dark_th_add28 (0xff)
	unsigned int reg_dpc_dark_th_add28:8;

	// h0038, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add29 (112)
	#define mask_of_reg_dpc_dark_th_add29 (0xff00)
	unsigned int reg_dpc_dark_th_add29:8;

	// h0038
	unsigned int /* padding 16 bit */:16;

	// h0039, bit: 7
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add30 (114)
	#define mask_of_reg_dpc_dark_th_add30 (0xff)
	unsigned int reg_dpc_dark_th_add30:8;

	// h0039, bit: 14
	/* DPC dark threshold add*/
	#define offset_of_reg_dpc_dark_th_add31 (114)
	#define mask_of_reg_dpc_dark_th_add31 (0xff00)
	unsigned int reg_dpc_dark_th_add31:8;

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

	// h0040, bit: 7
	/* BNR LPF weight1 parameter*/
	#define offset_of_reg_bnr_lpf_w1 (128)
	#define mask_of_reg_bnr_lpf_w1 (0xff)
	unsigned int reg_bnr_lpf_w1:8;

	// h0040, bit: 14
	/* BNR LPF weight2 parameter*/
	#define offset_of_reg_bnr_lpf_w2 (128)
	#define mask_of_reg_bnr_lpf_w2 (0xff00)
	unsigned int reg_bnr_lpf_w2:8;

	// h0040
	unsigned int /* padding 16 bit */:16;

	// h0041, bit: 7
	/* BNR LPF weight3 parameter*/
	#define offset_of_reg_bnr_lpf_w3 (130)
	#define mask_of_reg_bnr_lpf_w3 (0xff)
	unsigned int reg_bnr_lpf_w3:8;

	// h0041, bit: 11
	/* BNR LPF shift parameter*/
	#define offset_of_reg_bnr_lpf_sft (130)
	#define mask_of_reg_bnr_lpf_sft (0xf00)
	unsigned int reg_bnr_lpf_sft:4;

	// h0041, bit: 14
	/* */
	unsigned int :4;

	// h0041
	unsigned int /* padding 16 bit */:16;

	// h0042, bit: 0
	/* BNR enable*/
	#define offset_of_reg_bnr_en (132)
	#define mask_of_reg_bnr_en (0x1)
	unsigned int reg_bnr_en:1;

	// h0042, bit: 1
	/* BNR LPF enable*/
	#define offset_of_reg_bnr_lpf_en (132)
	#define mask_of_reg_bnr_lpf_en (0x2)
	unsigned int reg_bnr_lpf_en:1;

	// h0042, bit: 14
	/* */
	unsigned int :14;

	// h0042
	unsigned int /* padding 16 bit */:16;

	// h0043, bit: 11
	/* BNR LW parameter*/
	#define offset_of_reg_bnr_lw (134)
	#define mask_of_reg_bnr_lw (0xfff)
	unsigned int reg_bnr_lw:12;

	// h0043, bit: 14
	/* */
	unsigned int :4;

	// h0043
	unsigned int /* padding 16 bit */:16;

	// h0044, bit: 3
	/* BNR WD*/
	#define offset_of_reg_bnr_wd (136)
	#define mask_of_reg_bnr_wd (0xf)
	unsigned int reg_bnr_wd:4;

	// h0044, bit: 7
	/* */
	unsigned int :4;

	// h0044, bit: 14
	/* BNR gain all*/
	#define offset_of_reg_bnr_gain_all (136)
	#define mask_of_reg_bnr_gain_all (0xff00)
	unsigned int reg_bnr_gain_all:8;

	// h0044
	unsigned int /* padding 16 bit */:16;

	// h0045, bit: 7
	/* BNR gain direction*/
	#define offset_of_reg_bnr_gain_dir (138)
	#define mask_of_reg_bnr_gain_dir (0xff)
	unsigned int reg_bnr_gain_dir:8;

	// h0045, bit: 14
	/* */
	unsigned int :8;

	// h0045
	unsigned int /* padding 16 bit */:16;

	// h0046, bit: 11
	/* BNR clip*/
	#define offset_of_reg_bnr_clip (140)
	#define mask_of_reg_bnr_clip (0xfff)
	unsigned int reg_bnr_clip:12;

	// h0046, bit: 14
	/* */
	unsigned int :4;

	// h0046
	unsigned int /* padding 16 bit */:16;

	// h0047, bit: 11
	/* BNR detail lw*/
	#define offset_of_reg_bnr_detail_lw (142)
	#define mask_of_reg_bnr_detail_lw (0xfff)
	unsigned int reg_bnr_detail_lw:12;

	// h0047, bit: 14
	/* BNR detail wd*/
	#define offset_of_reg_bnr_detail_wd (142)
	#define mask_of_reg_bnr_detail_wd (0xf000)
	unsigned int reg_bnr_detail_wd:4;

	// h0047
	unsigned int /* padding 16 bit */:16;

	// h0048, bit: 11
	/* AWB gain  R */
	#define offset_of_reg_awb_gain_r (144)
	#define mask_of_reg_awb_gain_r (0xfff)
	unsigned int reg_awb_gain_r:12;

	// h0048, bit: 14
	/* */
	unsigned int :4;

	// h0048
	unsigned int /* padding 16 bit */:16;

	// h0049, bit: 11
	/* AWB gain  gr*/
	#define offset_of_reg_awb_gain_gr (146)
	#define mask_of_reg_awb_gain_gr (0xfff)
	unsigned int reg_awb_gain_gr:12;

	// h0049, bit: 14
	/* */
	unsigned int :4;

	// h0049
	unsigned int /* padding 16 bit */:16;

	// h004a, bit: 11
	/* AWB gain gb*/
	#define offset_of_reg_awb_gain_gb (148)
	#define mask_of_reg_awb_gain_gb (0xfff)
	unsigned int reg_awb_gain_gb:12;

	// h004a, bit: 14
	/* */
	unsigned int :4;

	// h004a
	unsigned int /* padding 16 bit */:16;

	// h004b, bit: 11
	/* AWB gain  b*/
	#define offset_of_reg_awb_gain_b (150)
	#define mask_of_reg_awb_gain_b (0xfff)
	unsigned int reg_awb_gain_b:12;

	// h004b, bit: 14
	/* */
	unsigned int :4;

	// h004b
	unsigned int /* padding 16 bit */:16;

	// h004c, bit: 11
	/* AWB gain  R for 2nd*/
	#define offset_of_reg_awb_gain_r_f1 (152)
	#define mask_of_reg_awb_gain_r_f1 (0xfff)
	unsigned int reg_awb_gain_r_f1:12;

	// h004c, bit: 14
	/* */
	unsigned int :4;

	// h004c
	unsigned int /* padding 16 bit */:16;

	// h004d, bit: 11
	/* AWB gain  gr for 2nd*/
	#define offset_of_reg_awb_gain_gr_f1 (154)
	#define mask_of_reg_awb_gain_gr_f1 (0xfff)
	unsigned int reg_awb_gain_gr_f1:12;

	// h004d, bit: 14
	/* */
	unsigned int :4;

	// h004d
	unsigned int /* padding 16 bit */:16;

	// h004e, bit: 11
	/* AWB gain gb for 2nd*/
	#define offset_of_reg_awb_gain_gb_f1 (156)
	#define mask_of_reg_awb_gain_gb_f1 (0xfff)
	unsigned int reg_awb_gain_gb_f1:12;

	// h004e, bit: 14
	/* */
	unsigned int :4;

	// h004e
	unsigned int /* padding 16 bit */:16;

	// h004f, bit: 11
	/* AWB gain  b for 2nd*/
	#define offset_of_reg_awb_gain_b_f1 (158)
	#define mask_of_reg_awb_gain_b_f1 (0xfff)
	unsigned int reg_awb_gain_b_f1:12;

	// h004f, bit: 14
	/* */
	unsigned int :4;

	// h004f
	unsigned int /* padding 16 bit */:16;

	// h0050, bit: 0
	/* New sorting enable*/
	#define offset_of_reg_dpc_sort_en (160)
	#define mask_of_reg_dpc_sort_en (0x1)
	unsigned int reg_dpc_sort_en:1;

	// h0050, bit: 1
	/* New sorting R enable*/
	#define offset_of_reg_dpc_sort_r_en (160)
	#define mask_of_reg_dpc_sort_r_en (0x2)
	unsigned int reg_dpc_sort_r_en:1;

	// h0050, bit: 2
	/* New sorting GR enable*/
	#define offset_of_reg_dpc_sort_gr_en (160)
	#define mask_of_reg_dpc_sort_gr_en (0x4)
	unsigned int reg_dpc_sort_gr_en:1;

	// h0050, bit: 3
	/* New sorting GB enable*/
	#define offset_of_reg_dpc_sort_gb_en (160)
	#define mask_of_reg_dpc_sort_gb_en (0x8)
	unsigned int reg_dpc_sort_gb_en:1;

	// h0050, bit: 4
	/* New sorting B enable*/
	#define offset_of_reg_dpc_sort_b_en (160)
	#define mask_of_reg_dpc_sort_b_en (0x10)
	unsigned int reg_dpc_sort_b_en:1;

	// h0050, bit: 5
	/* New sorting 1x3 mode enable*/
	#define offset_of_reg_dpc_sort_1x3_mode_en (160)
	#define mask_of_reg_dpc_sort_1x3_mode_en (0x20)
	unsigned int reg_dpc_sort_1x3_mode_en:1;

	// h0050, bit: 14
	/* */
	unsigned int :10;

	// h0050
	unsigned int /* padding 16 bit */:16;

	// h0051, bit: 3
	/* New sorting luma table shift x0 */
	#define offset_of_reg_dpc_sort_lumtbl_x0 (162)
	#define mask_of_reg_dpc_sort_lumtbl_x0 (0xf)
	unsigned int reg_dpc_sort_lumtbl_x0:4;

	// h0051, bit: 7
	/* New sorting luma table shift x1 */
	#define offset_of_reg_dpc_sort_lumtbl_x1 (162)
	#define mask_of_reg_dpc_sort_lumtbl_x1 (0xf0)
	unsigned int reg_dpc_sort_lumtbl_x1:4;

	// h0051, bit: 11
	/* New sorting luma table shift x2*/
	#define offset_of_reg_dpc_sort_lumtbl_x2 (162)
	#define mask_of_reg_dpc_sort_lumtbl_x2 (0xf00)
	unsigned int reg_dpc_sort_lumtbl_x2:4;

	// h0051, bit: 14
	/* New sorting luma table shift x3*/
	#define offset_of_reg_dpc_sort_lumtbl_x3 (162)
	#define mask_of_reg_dpc_sort_lumtbl_x3 (0xf000)
	unsigned int reg_dpc_sort_lumtbl_x3:4;

	// h0051
	unsigned int /* padding 16 bit */:16;

	// h0052, bit: 3
	/* New sorting luma table shift x4*/
	#define offset_of_reg_dpc_sort_lumtbl_x4 (164)
	#define mask_of_reg_dpc_sort_lumtbl_x4 (0xf)
	unsigned int reg_dpc_sort_lumtbl_x4:4;

	// h0052, bit: 14
	/* */
	unsigned int :12;

	// h0052
	unsigned int /* padding 16 bit */:16;

	// h0053, bit: 11
	/* New sorting luma table light 0*/
	#define offset_of_reg_dpc_sort_lumtbl_l0 (166)
	#define mask_of_reg_dpc_sort_lumtbl_l0 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l0:12;

	// h0053, bit: 14
	/* */
	unsigned int :4;

	// h0053
	unsigned int /* padding 16 bit */:16;

	// h0054, bit: 11
	/* New sorting luma table light 1*/
	#define offset_of_reg_dpc_sort_lumtbl_l1 (168)
	#define mask_of_reg_dpc_sort_lumtbl_l1 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l1:12;

	// h0054, bit: 14
	/* */
	unsigned int :4;

	// h0054
	unsigned int /* padding 16 bit */:16;

	// h0055, bit: 11
	/* New sorting luma table light 2*/
	#define offset_of_reg_dpc_sort_lumtbl_l2 (170)
	#define mask_of_reg_dpc_sort_lumtbl_l2 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l2:12;

	// h0055, bit: 14
	/* */
	unsigned int :4;

	// h0055
	unsigned int /* padding 16 bit */:16;

	// h0056, bit: 11
	/* New sorting luma table light 3*/
	#define offset_of_reg_dpc_sort_lumtbl_l3 (172)
	#define mask_of_reg_dpc_sort_lumtbl_l3 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l3:12;

	// h0056, bit: 14
	/* */
	unsigned int :4;

	// h0056
	unsigned int /* padding 16 bit */:16;

	// h0057, bit: 11
	/* New sorting luma table light 4*/
	#define offset_of_reg_dpc_sort_lumtbl_l4 (174)
	#define mask_of_reg_dpc_sort_lumtbl_l4 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l4:12;

	// h0057, bit: 14
	/* */
	unsigned int :4;

	// h0057
	unsigned int /* padding 16 bit */:16;

	// h0058, bit: 11
	/* New sorting luma table light 5*/
	#define offset_of_reg_dpc_sort_lumtbl_l5 (176)
	#define mask_of_reg_dpc_sort_lumtbl_l5 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_l5:12;

	// h0058, bit: 14
	/* */
	unsigned int :4;

	// h0058
	unsigned int /* padding 16 bit */:16;

	// h0059, bit: 11
	/* New sorting luma table dark 0*/
	#define offset_of_reg_dpc_sort_lumtbl_d0 (178)
	#define mask_of_reg_dpc_sort_lumtbl_d0 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d0:12;

	// h0059, bit: 14
	/* */
	unsigned int :4;

	// h0059
	unsigned int /* padding 16 bit */:16;

	// h005a, bit: 11
	/* New sorting luma table dark 1*/
	#define offset_of_reg_dpc_sort_lumtbl_d1 (180)
	#define mask_of_reg_dpc_sort_lumtbl_d1 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d1:12;

	// h005a, bit: 14
	/* */
	unsigned int :4;

	// h005a
	unsigned int /* padding 16 bit */:16;

	// h005b, bit: 11
	/* New sorting luma table dark 2*/
	#define offset_of_reg_dpc_sort_lumtbl_d2 (182)
	#define mask_of_reg_dpc_sort_lumtbl_d2 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d2:12;

	// h005b, bit: 14
	/* */
	unsigned int :4;

	// h005b
	unsigned int /* padding 16 bit */:16;

	// h005c, bit: 11
	/* New sorting luma table dark 3*/
	#define offset_of_reg_dpc_sort_lumtbl_d3 (184)
	#define mask_of_reg_dpc_sort_lumtbl_d3 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d3:12;

	// h005c, bit: 14
	/* */
	unsigned int :4;

	// h005c
	unsigned int /* padding 16 bit */:16;

	// h005d, bit: 11
	/* New sorting luma table dark 4*/
	#define offset_of_reg_dpc_sort_lumtbl_d4 (186)
	#define mask_of_reg_dpc_sort_lumtbl_d4 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d4:12;

	// h005d, bit: 14
	/* */
	unsigned int :4;

	// h005d
	unsigned int /* padding 16 bit */:16;

	// h005e, bit: 11
	/* New sorting luma table dark 5*/
	#define offset_of_reg_dpc_sort_lumtbl_d5 (188)
	#define mask_of_reg_dpc_sort_lumtbl_d5 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_d5:12;

	// h005e, bit: 14
	/* */
	unsigned int :4;

	// h005e
	unsigned int /* padding 16 bit */:16;

	// h005f, bit: 11
	/* New sorting luma table second 0*/
	#define offset_of_reg_dpc_sort_lumtbl_s0 (190)
	#define mask_of_reg_dpc_sort_lumtbl_s0 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s0:12;

	// h005f, bit: 14
	/* */
	unsigned int :4;

	// h005f
	unsigned int /* padding 16 bit */:16;

	// h0060, bit: 11
	/* New sorting luma table second 1*/
	#define offset_of_reg_dpc_sort_lumtbl_s1 (192)
	#define mask_of_reg_dpc_sort_lumtbl_s1 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s1:12;

	// h0060, bit: 14
	/* */
	unsigned int :4;

	// h0060
	unsigned int /* padding 16 bit */:16;

	// h0061, bit: 11
	/* New sorting luma table second 2*/
	#define offset_of_reg_dpc_sort_lumtbl_s2 (194)
	#define mask_of_reg_dpc_sort_lumtbl_s2 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s2:12;

	// h0061, bit: 14
	/* */
	unsigned int :4;

	// h0061
	unsigned int /* padding 16 bit */:16;

	// h0062, bit: 11
	/* New sorting luma table second 3*/
	#define offset_of_reg_dpc_sort_lumtbl_s3 (196)
	#define mask_of_reg_dpc_sort_lumtbl_s3 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s3:12;

	// h0062, bit: 14
	/* */
	unsigned int :4;

	// h0062
	unsigned int /* padding 16 bit */:16;

	// h0063, bit: 11
	/* New sorting luma table second 4*/
	#define offset_of_reg_dpc_sort_lumtbl_s4 (198)
	#define mask_of_reg_dpc_sort_lumtbl_s4 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s4:12;

	// h0063, bit: 14
	/* */
	unsigned int :4;

	// h0063
	unsigned int /* padding 16 bit */:16;

	// h0064, bit: 11
	/* New sorting luma table second 5*/
	#define offset_of_reg_dpc_sort_lumtbl_s5 (200)
	#define mask_of_reg_dpc_sort_lumtbl_s5 (0xfff)
	unsigned int reg_dpc_sort_lumtbl_s5:12;

	// h0064, bit: 14
	/* */
	unsigned int :4;

	// h0064
	unsigned int /* padding 16 bit */:16;

	// h0065, bit: 14
	/* register test dummy 0*/
	#define offset_of_reg_dummy_0 (202)
	#define mask_of_reg_dummy_0 (0xffff)
	unsigned int reg_dummy_0:16;

	// h0065
	unsigned int /* padding 16 bit */:16;

	// h0066, bit: 14
	/* register test dummy 1*/
	#define offset_of_reg_dummy_1 (204)
	#define mask_of_reg_dummy_1 (0xffff)
	unsigned int reg_dummy_1:16;

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

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp2;
#endif
