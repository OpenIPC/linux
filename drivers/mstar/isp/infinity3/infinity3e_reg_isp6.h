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
#ifndef __INFINITY3E_REG_ISP6__
#define __INFINITY3E_REG_ISP6__
typedef struct {
	// h0000, bit: 0
	/* ISP RDMA enable*/
	#define offset_of_reg_isp_rdma_en (0)
	#define mask_of_reg_isp_rdma_en (0x1)
	unsigned int reg_isp_rdma_en:1;

	// h0000, bit: 1
	/* */
	unsigned int :1;

	// h0000, bit: 2
	/* ISP DMA ring buffer mode enable*/
	#define offset_of_reg_isp_dma_ring_buf_en (0)
	#define mask_of_reg_isp_dma_ring_buf_en (0x4)
	unsigned int reg_isp_dma_ring_buf_en:1;

	// h0000, bit: 3
	/* */
	unsigned int :1;

	// h0000, bit: 4
	/* ISP RDMA auto vsync trigger mode*/
	#define offset_of_reg_isp_rdma_auto (0)
	#define mask_of_reg_isp_rdma_auto (0x10)
	unsigned int reg_isp_rdma_auto:1;

	// h0000, bit: 6
	/* */
	unsigned int :2;

	// h0000, bit: 7
	/* ISP RDMA high priority set*/
	#define offset_of_reg_isp_rdma_rreq_hpri_set (0)
	#define mask_of_reg_isp_rdma_rreq_hpri_set (0x80)
	unsigned int reg_isp_rdma_rreq_hpri_set:1;

	// h0000, bit: 9
	/* ISP RDMA depack mode:
	2'd0: 8-bit
	2'd1: 10-bit
	2'd2: 16-bit
	2'd3: 12-bit*/
	#define offset_of_reg_isp_rdma_mode (0)
	#define mask_of_reg_isp_rdma_mode (0x300)
	unsigned int reg_isp_rdma_mode:2;

	// h0000, bit: 14
	/* */
	unsigned int :5;

	// h0000, bit: 15
	/* ISP RDMA MIU request reset*/
	#define offset_of_reg_isp_rdma_rreq_rst (0)
	#define mask_of_reg_isp_rdma_rreq_rst (0x8000)
	unsigned int reg_isp_rdma_rreq_rst:1;

	// h0000
	unsigned int /* padding 16 bit */:16;

	// h0001, bit: 11
	/* ISP RDMA pitch*/
	#define offset_of_reg_isp_rdma_pitch (2)
	#define mask_of_reg_isp_rdma_pitch (0xfff)
	unsigned int reg_isp_rdma_pitch:12;

	// h0001, bit: 14
	/* */
	unsigned int :4;

	// h0001
	unsigned int /* padding 16 bit */:16;

	// h0002, bit: 3
	/* Ring buffer of 2^(ring_buf_sz+1) lines memory space (2~1024 lines)*/
	#define offset_of_reg_isp_dma_ring_buf_sz (4)
	#define mask_of_reg_isp_dma_ring_buf_sz (0xf)
	unsigned int reg_isp_dma_ring_buf_sz:4;

	// h0002, bit: 14
	/* */
	unsigned int :12;

	// h0002
	unsigned int /* padding 16 bit */:16;

	// h0003, bit: 14
	/* */
	unsigned int :16;

	// h0003
	unsigned int /* padding 16 bit */:16;

	// h0004, bit: 14
	/* */
	unsigned int :16;

	// h0004
	unsigned int /* padding 16 bit */:16;

	// h0005, bit: 14
	/* */
	unsigned int :16;

	// h0005
	unsigned int /* padding 16 bit */:16;

	// h0006, bit: 14
	/* */
	unsigned int :16;

	// h0006
	unsigned int /* padding 16 bit */:16;

	// h0007, bit: 14
	/* */
	unsigned int :16;

	// h0007
	unsigned int /* padding 16 bit */:16;

	// h0008, bit: 14
	/* ISP RDMA MIU base address*/
	#define offset_of_reg_isp_rdma_base (16)
	#define mask_of_reg_isp_rdma_base (0xffff)
	unsigned int reg_isp_rdma_base:16;

	// h0008
	unsigned int /* padding 16 bit */:16;

	// h0009, bit: 10
	/* ISP RDMA MIU base address*/
	#define offset_of_reg_isp_rdma_base_1 (18)
	#define mask_of_reg_isp_rdma_base_1 (0x7ff)
	unsigned int reg_isp_rdma_base_1:11;

	// h0009, bit: 14
	/* */
	unsigned int :5;

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

	// h000c, bit: 2
	/* ISP RDMA MIU request threshold*/
	#define offset_of_reg_isp_rdma_rreq_thrd (24)
	#define mask_of_reg_isp_rdma_rreq_thrd (0x7)
	unsigned int reg_isp_rdma_rreq_thrd:3;

	// h000c, bit: 3
	/* */
	unsigned int :1;

	// h000c, bit: 7
	/* ISP RDMA MIU high priority threshold*/
	#define offset_of_reg_isp_rdma_rreq_hpri (24)
	#define mask_of_reg_isp_rdma_rreq_hpri (0xf0)
	unsigned int reg_isp_rdma_rreq_hpri:4;

	// h000c, bit: 11
	/* ISP RDMA MIU burst number*/
	#define offset_of_reg_isp_rdma_rreq_max (24)
	#define mask_of_reg_isp_rdma_rreq_max (0xf00)
	unsigned int reg_isp_rdma_rreq_max:4;

	// h000c, bit: 14
	/* */
	unsigned int :4;

	// h000c
	unsigned int /* padding 16 bit */:16;

	// h000d, bit: 5
	/* ISP RDMA output blanking*/
	#define offset_of_reg_isp_rdma_tg_hblk (26)
	#define mask_of_reg_isp_rdma_tg_hblk (0x3f)
	unsigned int reg_isp_rdma_tg_hblk:6;

	// h000d, bit: 14
	/* */
	unsigned int :10;

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

	// h0010, bit: 0
	/* ISP RDMA start trigger*/
	#define offset_of_reg_isp_rdma_trigger (32)
	#define mask_of_reg_isp_rdma_trigger (0x1)
	unsigned int reg_isp_rdma_trigger:1;

	// h0010, bit: 14
	/* */
	unsigned int :15;

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

	// h0017, bit: 7
	/* */
	#define offset_of_reg_sw_rst_mode (46)
	#define mask_of_reg_sw_rst_mode (0xff)
	unsigned int reg_sw_rst_mode:8;

	// h0017, bit: 14
	/* */
	unsigned int :8;

	// h0017
	unsigned int /* padding 16 bit */:16;

	// h0018, bit: 0
	/* ISP RDMA valid data msb align at 16bit mode*/
	#define offset_of_reg_isp_rdma_msb_align (48)
	#define mask_of_reg_isp_rdma_msb_align (0x1)
	unsigned int reg_isp_rdma_msb_align:1;

	// h0018, bit: 2
	/* ISP RDMA valid data mode at 16bit mode
	2'd0: 8-bit
	2'd1:10-bit
	2'd2: 16-bit
	2'd3: 12-bit*/
	#define offset_of_reg_isp_rdma_valid_mode (48)
	#define mask_of_reg_isp_rdma_valid_mode (0x6)
	unsigned int reg_isp_rdma_valid_mode:2;

	// h0018, bit: 14
	/* */
	unsigned int :13;

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

	// h001f, bit: 0
	/* ISP RDMA end of frame status*/
	#define offset_of_reg_isp_rdma_eof_read (62)
	#define mask_of_reg_isp_rdma_eof_read (0x1)
	unsigned int reg_isp_rdma_eof_read:1;

	// h001f, bit: 14
	/* */
	unsigned int :15;

	// h001f
	unsigned int /* padding 16 bit */:16;

	// h0020, bit: 0
	/* ISP WDMA enable*/
	#define offset_of_reg_isp_wdma_en (64)
	#define mask_of_reg_isp_wdma_en (0x1)
	unsigned int reg_isp_wdma_en:1;

	// h0020, bit: 2
	/* */
	unsigned int :2;

	// h0020, bit: 3
	/* ISP WDMA MIU wbe mask*/
	#define offset_of_reg_isp_wdma_wbe_mask (64)
	#define mask_of_reg_isp_wdma_wbe_mask (0x8)
	unsigned int reg_isp_wdma_wbe_mask:1;

	// h0020, bit: 4
	/* ISP WDMA auto vsync trigger mode*/
	#define offset_of_reg_isp_wdma_auto (64)
	#define mask_of_reg_isp_wdma_auto (0x10)
	unsigned int reg_isp_wdma_auto:1;

	// h0020, bit: 6
	/* */
	unsigned int :2;

	// h0020, bit: 7
	/* ISP WDMA high priority set*/
	#define offset_of_reg_isp_wdma_wreq_hpri_set (64)
	#define mask_of_reg_isp_wdma_wreq_hpri_set (0x80)
	unsigned int reg_isp_wdma_wreq_hpri_set:1;

	// h0020, bit: 9
	/* ISP WDMA pack mode:
	2'd0: 8-bit
	2'd1: 10-bit
	2'd2: 16-bit
	2'd3: 12-bit*/
	#define offset_of_reg_isp_wdma_mode (64)
	#define mask_of_reg_isp_wdma_mode (0x300)
	unsigned int reg_isp_wdma_mode:2;

	// h0020, bit: 14
	/* */
	unsigned int :5;

	// h0020, bit: 15
	/* ISP WDMA MIU request reset*/
	#define offset_of_reg_isp_wdma_wreq_rst (64)
	#define mask_of_reg_isp_wdma_wreq_rst (0x8000)
	unsigned int reg_isp_wdma_wreq_rst:1;

	// h0020
	unsigned int /* padding 16 bit */:16;

	// h0021, bit: 12
	/* ISP WDMA pitch*/
	#define offset_of_reg_isp_wdma_pitch (66)
	#define mask_of_reg_isp_wdma_pitch (0x1fff)
	unsigned int reg_isp_wdma_pitch:13;

	// h0021, bit: 14
	/* */
	unsigned int :3;

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
	unsigned int :15;

	// h0025, bit: 15
	/* ISP WDMA height limit enable*/
	#define offset_of_reg_isp_wdma_height_en (74)
	#define mask_of_reg_isp_wdma_height_en (0x8000)
	unsigned int reg_isp_wdma_height_en:1;

	// h0025
	unsigned int /* padding 16 bit */:16;

	// h0026, bit: 14
	/* ISP WDMA write limit address*/
	#define offset_of_reg_isp_wdma_w_limit_adr (76)
	#define mask_of_reg_isp_wdma_w_limit_adr (0xffff)
	unsigned int reg_isp_wdma_w_limit_adr:16;

	// h0026
	unsigned int /* padding 16 bit */:16;

	// h0027, bit: 10
	/* ISP WDMA write limit address*/
	#define offset_of_reg_isp_wdma_w_limit_adr_1 (78)
	#define mask_of_reg_isp_wdma_w_limit_adr_1 (0x7ff)
	unsigned int reg_isp_wdma_w_limit_adr_1:11;

	// h0027, bit: 14
	/* */
	unsigned int :4;

	// h0027, bit: 15
	/* ISP WDMA write limit enable*/
	#define offset_of_reg_isp_wdma_w_limit_en (78)
	#define mask_of_reg_isp_wdma_w_limit_en (0x8000)
	unsigned int reg_isp_wdma_w_limit_en:1;

	// h0027
	unsigned int /* padding 16 bit */:16;

	// h0028, bit: 14
	/* ISP WDMA MIU base address*/
	#define offset_of_reg_isp_wdma_base (80)
	#define mask_of_reg_isp_wdma_base (0xffff)
	unsigned int reg_isp_wdma_base:16;

	// h0028
	unsigned int /* padding 16 bit */:16;

	// h0029, bit: 10
	/* ISP WDMA MIU base address*/
	#define offset_of_reg_isp_wdma_base_1 (82)
	#define mask_of_reg_isp_wdma_base_1 (0x7ff)
	unsigned int reg_isp_wdma_base_1:11;

	// h0029, bit: 14
	/* */
	unsigned int :5;

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

	// h002c, bit: 2
	/* ISP WDMA MIU request threshold (*4+1)*/
	#define offset_of_reg_isp_wdma_wreq_thrd (88)
	#define mask_of_reg_isp_wdma_wreq_thrd (0x7)
	unsigned int reg_isp_wdma_wreq_thrd:3;

	// h002c, bit: 3
	/* */
	unsigned int :1;

	// h002c, bit: 7
	/* ISP WDMA MIU high priority threshold*/
	#define offset_of_reg_isp_wdma_wreq_hpri (88)
	#define mask_of_reg_isp_wdma_wreq_hpri (0xf0)
	unsigned int reg_isp_wdma_wreq_hpri:4;

	// h002c, bit: 11
	/* ISP WDMA MIU burst number*/
	#define offset_of_reg_isp_wdma_wreq_max (88)
	#define mask_of_reg_isp_wdma_wreq_max (0xf00)
	unsigned int reg_isp_wdma_wreq_max:4;

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
	/* ISP WDMA start trigger*/
	#define offset_of_reg_isp_wdma_trigger (96)
	#define mask_of_reg_isp_wdma_trigger (0x1)
	unsigned int reg_isp_wdma_trigger:1;

	// h0030, bit: 1
	/* ISP WDMA start trigger vsync aligned mode*/
	#define offset_of_reg_isp_wdma_trigger_mode (96)
	#define mask_of_reg_isp_wdma_trigger_mode (0x2)
	unsigned int reg_isp_wdma_trigger_mode:1;

	// h0030, bit: 7
	/* */
	unsigned int :6;

	// h0030, bit: 8
	/* ISP WDMA input auto-align enable*/
	#define offset_of_reg_isp_wdma_align_en (96)
	#define mask_of_reg_isp_wdma_align_en (0x100)
	unsigned int reg_isp_wdma_align_en:1;

	// h0030, bit: 14
	/* */
	unsigned int :7;

	// h0030
	unsigned int /* padding 16 bit */:16;

	// h0031, bit: 0
	/* ISP WDMA valid data msb align at 16bit mode*/
	#define offset_of_reg_isp_wdma_msb_align (98)
	#define mask_of_reg_isp_wdma_msb_align (0x1)
	unsigned int reg_isp_wdma_msb_align:1;

	// h0031, bit: 14
	/* */
	unsigned int :15;

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

	// h003f, bit: 0
	/* ISP WDMA end of frame status*/
	#define offset_of_reg_isp_wdma_eof_read (126)
	#define mask_of_reg_isp_wdma_eof_read (0x1)
	unsigned int reg_isp_wdma_eof_read:1;

	// h003f, bit: 7
	/* */
	unsigned int :7;

	// h003f, bit: 8
	/* ISP WDMA LB full status*/
	#define offset_of_reg_isp_wdma_lb_full_read (126)
	#define mask_of_reg_isp_wdma_lb_full_read (0x100)
	unsigned int reg_isp_wdma_lb_full_read:1;

	// h003f, bit: 14
	/* */
	unsigned int :6;

	// h003f, bit: 15
	/* ISP WDMA status clear*/
	#define offset_of_reg_isp_wdma_status_clr (126)
	#define mask_of_reg_isp_wdma_status_clr (0x8000)
	unsigned int reg_isp_wdma_status_clr:1;

	// h003f
	unsigned int /* padding 16 bit */:16;

	// h0040, bit: 0
	/* ISP DMA mclk req force active*/
	#define offset_of_reg_mreq_always_active (128)
	#define mask_of_reg_mreq_always_active (0x1)
	unsigned int reg_mreq_always_active:1;

	// h0040, bit: 1
	/* ISP DMA mclk req force off*/
	#define offset_of_reg_mreq_force_off (128)
	#define mask_of_reg_mreq_force_off (0x2)
	unsigned int reg_mreq_force_off:1;

	// h0040, bit: 2
	/* ISP DMA mclk req wdma mode*/
	#define offset_of_reg_mreq_wdma_mode (128)
	#define mask_of_reg_mreq_wdma_mode (0x4)
	unsigned int reg_mreq_wdma_mode:1;

	// h0040, bit: 14
	/* */
	unsigned int :13;

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

	// h0047, bit: 0
	/* Post White Balance Gain Dither Enable*/
	#define offset_of_reg_wbg1_dith_en (142)
	#define mask_of_reg_wbg1_dith_en (0x1)
	unsigned int reg_wbg1_dith_en:1;

	// h0047, bit: 14
	/* */
	unsigned int :15;

	// h0047
	unsigned int /* padding 16 bit */:16;

	// h0048, bit: 12
	/* Post White Balance Gain for R channel (3.10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_r_gain (144)
	#define mask_of_reg_isp_wbg1_r_gain (0x1fff)
	unsigned int reg_isp_wbg1_r_gain:13;

	// h0048, bit: 14
	/* */
	unsigned int :3;

	// h0048
	unsigned int /* padding 16 bit */:16;

	// h0049, bit: 10
	/* Post White Balance Offset for R channel (s10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_r_offset (146)
	#define mask_of_reg_isp_wbg1_r_offset (0x7ff)
	unsigned int reg_isp_wbg1_r_offset:11;

	// h0049, bit: 14
	/* */
	unsigned int :5;

	// h0049
	unsigned int /* padding 16 bit */:16;

	// h004a, bit: 12
	/* Post White Balance Gain for Gr channel (3.10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_gr_gain (148)
	#define mask_of_reg_isp_wbg1_gr_gain (0x1fff)
	unsigned int reg_isp_wbg1_gr_gain:13;

	// h004a, bit: 14
	/* */
	unsigned int :3;

	// h004a
	unsigned int /* padding 16 bit */:16;

	// h004b, bit: 10
	/* Post White Balance Offset for Gr channel (s10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_gr_offset (150)
	#define mask_of_reg_isp_wbg1_gr_offset (0x7ff)
	unsigned int reg_isp_wbg1_gr_offset:11;

	// h004b, bit: 14
	/* */
	unsigned int :5;

	// h004b
	unsigned int /* padding 16 bit */:16;

	// h004c, bit: 12
	/* Post White Balance Gain for B channel (3.10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_b_gain (152)
	#define mask_of_reg_isp_wbg1_b_gain (0x1fff)
	unsigned int reg_isp_wbg1_b_gain:13;

	// h004c, bit: 14
	/* */
	unsigned int :3;

	// h004c
	unsigned int /* padding 16 bit */:16;

	// h004d, bit: 10
	/* Post White Balance Offset for B channel (s10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_b_offset (154)
	#define mask_of_reg_isp_wbg1_b_offset (0x7ff)
	unsigned int reg_isp_wbg1_b_offset:11;

	// h004d, bit: 14
	/* */
	unsigned int :5;

	// h004d
	unsigned int /* padding 16 bit */:16;

	// h004e, bit: 12
	/* Post White Balance Gain for Gb channel (3.10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_gb_gain (156)
	#define mask_of_reg_isp_wbg1_gb_gain (0x1fff)
	unsigned int reg_isp_wbg1_gb_gain:13;

	// h004e, bit: 14
	/* */
	unsigned int :3;

	// h004e
	unsigned int /* padding 16 bit */:16;

	// h004f, bit: 10
	/* Post White Balance Offset for Gb channel (s10)
	 ( double buffer register )*/
	#define offset_of_reg_isp_wbg1_gb_offset (158)
	#define mask_of_reg_isp_wbg1_gb_offset (0x7ff)
	unsigned int reg_isp_wbg1_gb_offset:11;

	// h004f, bit: 14
	/* */
	unsigned int :5;

	// h004f
	unsigned int /* padding 16 bit */:16;

	// h0050, bit: 0
	/* HSV Adjust enable*/
	#define offset_of_reg_hsv_adj_en (160)
	#define mask_of_reg_hsv_adj_en (0x1)
	unsigned int reg_hsv_adj_en:1;

	// h0050, bit: 14
	/* */
	unsigned int :15;

	// h0050
	unsigned int /* padding 16 bit */:16;

	// h0051, bit: 7
	/* HSV hue adjust LUT parameter x0*/
	#define offset_of_reg_hue_adj_lut_x0 (162)
	#define mask_of_reg_hue_adj_lut_x0 (0xff)
	unsigned int reg_hue_adj_lut_x0:8;

	// h0051, bit: 14
	/* HSV hue adjust LUT parameter x1*/
	#define offset_of_reg_hue_adj_lut_x1 (162)
	#define mask_of_reg_hue_adj_lut_x1 (0xff00)
	unsigned int reg_hue_adj_lut_x1:8;

	// h0051
	unsigned int /* padding 16 bit */:16;

	// h0052, bit: 7
	/* HSV hue adjust LUT parameter x2*/
	#define offset_of_reg_hue_adj_lut_x2 (164)
	#define mask_of_reg_hue_adj_lut_x2 (0xff)
	unsigned int reg_hue_adj_lut_x2:8;

	// h0052, bit: 14
	/* HSV hue adjust LUT parameter x3*/
	#define offset_of_reg_hue_adj_lut_x3 (164)
	#define mask_of_reg_hue_adj_lut_x3 (0xff00)
	unsigned int reg_hue_adj_lut_x3:8;

	// h0052
	unsigned int /* padding 16 bit */:16;

	// h0053, bit: 7
	/* HSV hue adjust LUT parameter x4*/
	#define offset_of_reg_hue_adj_lut_x4 (166)
	#define mask_of_reg_hue_adj_lut_x4 (0xff)
	unsigned int reg_hue_adj_lut_x4:8;

	// h0053, bit: 14
	/* HSV hue adjust LUT parameter x5*/
	#define offset_of_reg_hue_adj_lut_x5 (166)
	#define mask_of_reg_hue_adj_lut_x5 (0xff00)
	unsigned int reg_hue_adj_lut_x5:8;

	// h0053
	unsigned int /* padding 16 bit */:16;

	// h0054, bit: 7
	/* HSV hue adjust LUT parameter x6*/
	#define offset_of_reg_hue_adj_lut_x6 (168)
	#define mask_of_reg_hue_adj_lut_x6 (0xff)
	unsigned int reg_hue_adj_lut_x6:8;

	// h0054, bit: 14
	/* HSV hue adjust LUT parameter x7*/
	#define offset_of_reg_hue_adj_lut_x7 (168)
	#define mask_of_reg_hue_adj_lut_x7 (0xff00)
	unsigned int reg_hue_adj_lut_x7:8;

	// h0054
	unsigned int /* padding 16 bit */:16;

	// h0055, bit: 7
	/* HSV hue adjust LUT parameter x8*/
	#define offset_of_reg_hue_adj_lut_x8 (170)
	#define mask_of_reg_hue_adj_lut_x8 (0xff)
	unsigned int reg_hue_adj_lut_x8:8;

	// h0055, bit: 14
	/* HSV hue adjust LUT parameter x9*/
	#define offset_of_reg_hue_adj_lut_x9 (170)
	#define mask_of_reg_hue_adj_lut_x9 (0xff00)
	unsigned int reg_hue_adj_lut_x9:8;

	// h0055
	unsigned int /* padding 16 bit */:16;

	// h0056, bit: 7
	/* HSV hue adjust LUT parameter x10*/
	#define offset_of_reg_hue_adj_lut_x10 (172)
	#define mask_of_reg_hue_adj_lut_x10 (0xff)
	unsigned int reg_hue_adj_lut_x10:8;

	// h0056, bit: 14
	/* HSV hue adjust LUT parameter x11*/
	#define offset_of_reg_hue_adj_lut_x11 (172)
	#define mask_of_reg_hue_adj_lut_x11 (0xff00)
	unsigned int reg_hue_adj_lut_x11:8;

	// h0056
	unsigned int /* padding 16 bit */:16;

	// h0057, bit: 7
	/* HSV hue adjust LUT parameter x12*/
	#define offset_of_reg_hue_adj_lut_x12 (174)
	#define mask_of_reg_hue_adj_lut_x12 (0xff)
	unsigned int reg_hue_adj_lut_x12:8;

	// h0057, bit: 14
	/* HSV hue adjust LUT parameter x13*/
	#define offset_of_reg_hue_adj_lut_x13 (174)
	#define mask_of_reg_hue_adj_lut_x13 (0xff00)
	unsigned int reg_hue_adj_lut_x13:8;

	// h0057
	unsigned int /* padding 16 bit */:16;

	// h0058, bit: 7
	/* HSV hue adjust LUT parameter x14*/
	#define offset_of_reg_hue_adj_lut_x14 (176)
	#define mask_of_reg_hue_adj_lut_x14 (0xff)
	unsigned int reg_hue_adj_lut_x14:8;

	// h0058, bit: 14
	/* HSV hue adjust LUT parameter x15*/
	#define offset_of_reg_hue_adj_lut_x15 (176)
	#define mask_of_reg_hue_adj_lut_x15 (0xff00)
	unsigned int reg_hue_adj_lut_x15:8;

	// h0058
	unsigned int /* padding 16 bit */:16;

	// h0059, bit: 7
	/* HSV hue adjust LUT parameter x16*/
	#define offset_of_reg_hue_adj_lut_x16 (178)
	#define mask_of_reg_hue_adj_lut_x16 (0xff)
	unsigned int reg_hue_adj_lut_x16:8;

	// h0059, bit: 14
	/* HSV hue adjust LUT parameter x17*/
	#define offset_of_reg_hue_adj_lut_x17 (178)
	#define mask_of_reg_hue_adj_lut_x17 (0xff00)
	unsigned int reg_hue_adj_lut_x17:8;

	// h0059
	unsigned int /* padding 16 bit */:16;

	// h005a, bit: 7
	/* HSV hue adjust LUT parameter x18*/
	#define offset_of_reg_hue_adj_lut_x18 (180)
	#define mask_of_reg_hue_adj_lut_x18 (0xff)
	unsigned int reg_hue_adj_lut_x18:8;

	// h005a, bit: 14
	/* HSV hue adjust LUT parameter x19*/
	#define offset_of_reg_hue_adj_lut_x19 (180)
	#define mask_of_reg_hue_adj_lut_x19 (0xff00)
	unsigned int reg_hue_adj_lut_x19:8;

	// h005a
	unsigned int /* padding 16 bit */:16;

	// h005b, bit: 7
	/* HSV hue adjust LUT parameter x20*/
	#define offset_of_reg_hue_adj_lut_x20 (182)
	#define mask_of_reg_hue_adj_lut_x20 (0xff)
	unsigned int reg_hue_adj_lut_x20:8;

	// h005b, bit: 14
	/* HSV hue adjust LUT parameter x21*/
	#define offset_of_reg_hue_adj_lut_x21 (182)
	#define mask_of_reg_hue_adj_lut_x21 (0xff00)
	unsigned int reg_hue_adj_lut_x21:8;

	// h005b
	unsigned int /* padding 16 bit */:16;

	// h005c, bit: 7
	/* HSV hue adjust LUT parameter x22*/
	#define offset_of_reg_hue_adj_lut_x22 (184)
	#define mask_of_reg_hue_adj_lut_x22 (0xff)
	unsigned int reg_hue_adj_lut_x22:8;

	// h005c, bit: 14
	/* HSV hue adjust LUT parameter x23*/
	#define offset_of_reg_hue_adj_lut_x23 (184)
	#define mask_of_reg_hue_adj_lut_x23 (0xff00)
	unsigned int reg_hue_adj_lut_x23:8;

	// h005c
	unsigned int /* padding 16 bit */:16;

	// h005d, bit: 7
	/* HSV sat adjust LUT parameter x0*/
	#define offset_of_reg_sat_adj_lut_x0 (186)
	#define mask_of_reg_sat_adj_lut_x0 (0xff)
	unsigned int reg_sat_adj_lut_x0:8;

	// h005d, bit: 14
	/* HSV sat adjust LUT parameter x1*/
	#define offset_of_reg_sat_adj_lut_x1 (186)
	#define mask_of_reg_sat_adj_lut_x1 (0xff00)
	unsigned int reg_sat_adj_lut_x1:8;

	// h005d
	unsigned int /* padding 16 bit */:16;

	// h005e, bit: 7
	/* HSV sat adjust LUT parameter x2*/
	#define offset_of_reg_sat_adj_lut_x2 (188)
	#define mask_of_reg_sat_adj_lut_x2 (0xff)
	unsigned int reg_sat_adj_lut_x2:8;

	// h005e, bit: 14
	/* HSV sat adjust LUT parameter x3*/
	#define offset_of_reg_sat_adj_lut_x3 (188)
	#define mask_of_reg_sat_adj_lut_x3 (0xff00)
	unsigned int reg_sat_adj_lut_x3:8;

	// h005e
	unsigned int /* padding 16 bit */:16;

	// h005f, bit: 7
	/* HSV sat adjust LUT parameter x4*/
	#define offset_of_reg_sat_adj_lut_x4 (190)
	#define mask_of_reg_sat_adj_lut_x4 (0xff)
	unsigned int reg_sat_adj_lut_x4:8;

	// h005f, bit: 14
	/* HSV sat adjust LUT parameter x5*/
	#define offset_of_reg_sat_adj_lut_x5 (190)
	#define mask_of_reg_sat_adj_lut_x5 (0xff00)
	unsigned int reg_sat_adj_lut_x5:8;

	// h005f
	unsigned int /* padding 16 bit */:16;

	// h0060, bit: 7
	/* HSV sat adjust LUT parameter x6*/
	#define offset_of_reg_sat_adj_lut_x6 (192)
	#define mask_of_reg_sat_adj_lut_x6 (0xff)
	unsigned int reg_sat_adj_lut_x6:8;

	// h0060, bit: 14
	/* HSV sat adjust LUT parameter x7*/
	#define offset_of_reg_sat_adj_lut_x7 (192)
	#define mask_of_reg_sat_adj_lut_x7 (0xff00)
	unsigned int reg_sat_adj_lut_x7:8;

	// h0060
	unsigned int /* padding 16 bit */:16;

	// h0061, bit: 7
	/* HSV sat adjust LUT parameter x8*/
	#define offset_of_reg_sat_adj_lut_x8 (194)
	#define mask_of_reg_sat_adj_lut_x8 (0xff)
	unsigned int reg_sat_adj_lut_x8:8;

	// h0061, bit: 14
	/* HSV sat adjust LUT parameter x9*/
	#define offset_of_reg_sat_adj_lut_x9 (194)
	#define mask_of_reg_sat_adj_lut_x9 (0xff00)
	unsigned int reg_sat_adj_lut_x9:8;

	// h0061
	unsigned int /* padding 16 bit */:16;

	// h0062, bit: 7
	/* HSV sat adjust LUT parameter x10*/
	#define offset_of_reg_sat_adj_lut_x10 (196)
	#define mask_of_reg_sat_adj_lut_x10 (0xff)
	unsigned int reg_sat_adj_lut_x10:8;

	// h0062, bit: 14
	/* HSV sat adjust LUT parameter x11*/
	#define offset_of_reg_sat_adj_lut_x11 (196)
	#define mask_of_reg_sat_adj_lut_x11 (0xff00)
	unsigned int reg_sat_adj_lut_x11:8;

	// h0062
	unsigned int /* padding 16 bit */:16;

	// h0063, bit: 7
	/* HSV sat adjust LUT parameter x12*/
	#define offset_of_reg_sat_adj_lut_x12 (198)
	#define mask_of_reg_sat_adj_lut_x12 (0xff)
	unsigned int reg_sat_adj_lut_x12:8;

	// h0063, bit: 14
	/* HSV sat adjust LUT parameter x13*/
	#define offset_of_reg_sat_adj_lut_x13 (198)
	#define mask_of_reg_sat_adj_lut_x13 (0xff00)
	unsigned int reg_sat_adj_lut_x13:8;

	// h0063
	unsigned int /* padding 16 bit */:16;

	// h0064, bit: 7
	/* HSV sat adjust LUT parameter x14*/
	#define offset_of_reg_sat_adj_lut_x14 (200)
	#define mask_of_reg_sat_adj_lut_x14 (0xff)
	unsigned int reg_sat_adj_lut_x14:8;

	// h0064, bit: 14
	/* HSV sat adjust LUT parameter x15*/
	#define offset_of_reg_sat_adj_lut_x15 (200)
	#define mask_of_reg_sat_adj_lut_x15 (0xff00)
	unsigned int reg_sat_adj_lut_x15:8;

	// h0064
	unsigned int /* padding 16 bit */:16;

	// h0065, bit: 7
	/* HSV sat adjust LUT parameter x16*/
	#define offset_of_reg_sat_adj_lut_x16 (202)
	#define mask_of_reg_sat_adj_lut_x16 (0xff)
	unsigned int reg_sat_adj_lut_x16:8;

	// h0065, bit: 14
	/* HSV sat adjust LUT parameter x17*/
	#define offset_of_reg_sat_adj_lut_x17 (202)
	#define mask_of_reg_sat_adj_lut_x17 (0xff00)
	unsigned int reg_sat_adj_lut_x17:8;

	// h0065
	unsigned int /* padding 16 bit */:16;

	// h0066, bit: 7
	/* HSV sat adjust LUT parameter x18*/
	#define offset_of_reg_sat_adj_lut_x18 (204)
	#define mask_of_reg_sat_adj_lut_x18 (0xff)
	unsigned int reg_sat_adj_lut_x18:8;

	// h0066, bit: 14
	/* HSV sat adjust LUT parameter x19*/
	#define offset_of_reg_sat_adj_lut_x19 (204)
	#define mask_of_reg_sat_adj_lut_x19 (0xff00)
	unsigned int reg_sat_adj_lut_x19:8;

	// h0066
	unsigned int /* padding 16 bit */:16;

	// h0067, bit: 7
	/* HSV sat adjust LUT parameter x20*/
	#define offset_of_reg_sat_adj_lut_x20 (206)
	#define mask_of_reg_sat_adj_lut_x20 (0xff)
	unsigned int reg_sat_adj_lut_x20:8;

	// h0067, bit: 14
	/* HSV sat adjust LUT parameter x21*/
	#define offset_of_reg_sat_adj_lut_x21 (206)
	#define mask_of_reg_sat_adj_lut_x21 (0xff00)
	unsigned int reg_sat_adj_lut_x21:8;

	// h0067
	unsigned int /* padding 16 bit */:16;

	// h0068, bit: 7
	/* HSV sat adjust LUT parameter x22*/
	#define offset_of_reg_sat_adj_lut_x22 (208)
	#define mask_of_reg_sat_adj_lut_x22 (0xff)
	unsigned int reg_sat_adj_lut_x22:8;

	// h0068, bit: 14
	/* HSV sat adjust LUT parameter x23*/
	#define offset_of_reg_sat_adj_lut_x23 (208)
	#define mask_of_reg_sat_adj_lut_x23 (0xff00)
	unsigned int reg_sat_adj_lut_x23:8;

	// h0068
	unsigned int /* padding 16 bit */:16;

	// h0069, bit: 7
	/* HSV val adjust LUT parameter x0*/
	#define offset_of_reg_val_adj_lut_x0 (210)
	#define mask_of_reg_val_adj_lut_x0 (0xff)
	unsigned int reg_val_adj_lut_x0:8;

	// h0069, bit: 14
	/* HSV val adjust LUT parameter x1*/
	#define offset_of_reg_val_adj_lut_x1 (210)
	#define mask_of_reg_val_adj_lut_x1 (0xff00)
	unsigned int reg_val_adj_lut_x1:8;

	// h0069
	unsigned int /* padding 16 bit */:16;

	// h006a, bit: 7
	/* HSV val adjust LUT parameter x2*/
	#define offset_of_reg_val_adj_lut_x2 (212)
	#define mask_of_reg_val_adj_lut_x2 (0xff)
	unsigned int reg_val_adj_lut_x2:8;

	// h006a, bit: 14
	/* HSV val adjust LUT parameter x3*/
	#define offset_of_reg_val_adj_lut_x3 (212)
	#define mask_of_reg_val_adj_lut_x3 (0xff00)
	unsigned int reg_val_adj_lut_x3:8;

	// h006a
	unsigned int /* padding 16 bit */:16;

	// h006b, bit: 7
	/* HSV val adjust LUT parameter x4*/
	#define offset_of_reg_val_adj_lut_x4 (214)
	#define mask_of_reg_val_adj_lut_x4 (0xff)
	unsigned int reg_val_adj_lut_x4:8;

	// h006b, bit: 14
	/* HSV val adjust LUT parameter x5*/
	#define offset_of_reg_val_adj_lut_x5 (214)
	#define mask_of_reg_val_adj_lut_x5 (0xff00)
	unsigned int reg_val_adj_lut_x5:8;

	// h006b
	unsigned int /* padding 16 bit */:16;

	// h006c, bit: 7
	/* HSV val adjust LUT parameter x6*/
	#define offset_of_reg_val_adj_lut_x6 (216)
	#define mask_of_reg_val_adj_lut_x6 (0xff)
	unsigned int reg_val_adj_lut_x6:8;

	// h006c, bit: 14
	/* HSV val adjust LUT parameter x7*/
	#define offset_of_reg_val_adj_lut_x7 (216)
	#define mask_of_reg_val_adj_lut_x7 (0xff00)
	unsigned int reg_val_adj_lut_x7:8;

	// h006c
	unsigned int /* padding 16 bit */:16;

	// h006d, bit: 7
	/* HSV val adjust LUT parameter x8*/
	#define offset_of_reg_val_adj_lut_x8 (218)
	#define mask_of_reg_val_adj_lut_x8 (0xff)
	unsigned int reg_val_adj_lut_x8:8;

	// h006d, bit: 14
	/* HSV val adjust LUT parameter x9*/
	#define offset_of_reg_val_adj_lut_x9 (218)
	#define mask_of_reg_val_adj_lut_x9 (0xff00)
	unsigned int reg_val_adj_lut_x9:8;

	// h006d
	unsigned int /* padding 16 bit */:16;

	// h006e, bit: 7
	/* HSV val adjust LUT parameter x10*/
	#define offset_of_reg_val_adj_lut_x10 (220)
	#define mask_of_reg_val_adj_lut_x10 (0xff)
	unsigned int reg_val_adj_lut_x10:8;

	// h006e, bit: 14
	/* HSV val adjust LUT parameter x11*/
	#define offset_of_reg_val_adj_lut_x11 (220)
	#define mask_of_reg_val_adj_lut_x11 (0xff00)
	unsigned int reg_val_adj_lut_x11:8;

	// h006e
	unsigned int /* padding 16 bit */:16;

	// h006f, bit: 7
	/* HSV val adjust LUT parameter x12*/
	#define offset_of_reg_val_adj_lut_x12 (222)
	#define mask_of_reg_val_adj_lut_x12 (0xff)
	unsigned int reg_val_adj_lut_x12:8;

	// h006f, bit: 14
	/* HSV val adjust LUT parameter x13*/
	#define offset_of_reg_val_adj_lut_x13 (222)
	#define mask_of_reg_val_adj_lut_x13 (0xff00)
	unsigned int reg_val_adj_lut_x13:8;

	// h006f
	unsigned int /* padding 16 bit */:16;

	// h0070, bit: 7
	/* HSV val adjust LUT parameter x14*/
	#define offset_of_reg_val_adj_lut_x14 (224)
	#define mask_of_reg_val_adj_lut_x14 (0xff)
	unsigned int reg_val_adj_lut_x14:8;

	// h0070, bit: 14
	/* HSV val adjust LUT parameter x15*/
	#define offset_of_reg_val_adj_lut_x15 (224)
	#define mask_of_reg_val_adj_lut_x15 (0xff00)
	unsigned int reg_val_adj_lut_x15:8;

	// h0070
	unsigned int /* padding 16 bit */:16;

	// h0071, bit: 7
	/* HSV val adjust LUT parameter x16*/
	#define offset_of_reg_val_adj_lut_x16 (226)
	#define mask_of_reg_val_adj_lut_x16 (0xff)
	unsigned int reg_val_adj_lut_x16:8;

	// h0071, bit: 14
	/* HSV val adjust LUT parameter x17*/
	#define offset_of_reg_val_adj_lut_x17 (226)
	#define mask_of_reg_val_adj_lut_x17 (0xff00)
	unsigned int reg_val_adj_lut_x17:8;

	// h0071
	unsigned int /* padding 16 bit */:16;

	// h0072, bit: 7
	/* HSV val adjust LUT parameter x18*/
	#define offset_of_reg_val_adj_lut_x18 (228)
	#define mask_of_reg_val_adj_lut_x18 (0xff)
	unsigned int reg_val_adj_lut_x18:8;

	// h0072, bit: 14
	/* HSV val adjust LUT parameter x19*/
	#define offset_of_reg_val_adj_lut_x19 (228)
	#define mask_of_reg_val_adj_lut_x19 (0xff00)
	unsigned int reg_val_adj_lut_x19:8;

	// h0072
	unsigned int /* padding 16 bit */:16;

	// h0073, bit: 7
	/* HSV val adjust LUT parameter x20*/
	#define offset_of_reg_val_adj_lut_x20 (230)
	#define mask_of_reg_val_adj_lut_x20 (0xff)
	unsigned int reg_val_adj_lut_x20:8;

	// h0073, bit: 14
	/* HSV val adjust LUT parameter x21*/
	#define offset_of_reg_val_adj_lut_x21 (230)
	#define mask_of_reg_val_adj_lut_x21 (0xff00)
	unsigned int reg_val_adj_lut_x21:8;

	// h0073
	unsigned int /* padding 16 bit */:16;

	// h0074, bit: 7
	/* HSV val adjust LUT parameter x22*/
	#define offset_of_reg_val_adj_lut_x22 (232)
	#define mask_of_reg_val_adj_lut_x22 (0xff)
	unsigned int reg_val_adj_lut_x22:8;

	// h0074, bit: 14
	/* HSV val adjust LUT parameter x23*/
	#define offset_of_reg_val_adj_lut_x23 (232)
	#define mask_of_reg_val_adj_lut_x23 (0xff00)
	unsigned int reg_val_adj_lut_x23:8;

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

}  __attribute__((packed, aligned(1))) infinity3e_reg_isp6;
#endif
